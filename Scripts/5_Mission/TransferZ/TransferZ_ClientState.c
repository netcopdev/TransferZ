class TransferZClientState
{
    static const string PROFILE_DIR = "$profile:TransferZ";
    static const string PREFERENCES_PATH = "$profile:TransferZ/preferences.json";
    static const int REQUEST_DEBOUNCE_MS = 150;

    protected static ref TransferZClientState s_Instance;
    protected EntityAI m_Destination;
    protected int m_DestinationCargoIndex = 0;
    protected bool m_DestinationVicinity;
    protected EntityAI m_LinkAnchor;
    protected int m_LinkAnchorCargoIndex = 0;
    protected EntityAI m_LinkA;
    protected int m_LinkACargoIndex = 0;
    protected EntityAI m_LinkB;
    protected int m_LinkBCargoIndex = 0;
    protected ref TransferZPreferences m_Preferences;

    protected int m_LastRequestTime = -1000;
    protected int m_LastRequestOperation = -1;
    protected EntityAI m_LastRequestSource;
    protected int m_LastRequestSourceCargoIndex = 0;
    protected EntityAI m_LastRequestDestination;
    protected int m_LastRequestDestinationCargoIndex = 0;
    protected EntityAI m_LastRequestItem;
    protected bool m_LastRequestDestinationVicinity;

    static TransferZClientState Get()
    {
        if (!s_Instance)
            s_Instance = new TransferZClientState();
        return s_Instance;
    }

    void TransferZClientState()
    {
        m_Preferences = new TransferZPreferences();
        LoadPreferences();
    }

    protected void SyncSplitPreferences()
    {
        if (!m_Preferences)
            m_Preferences = new TransferZPreferences();
        if (!m_Preferences.preferred_path)
            m_Preferences.preferred_path = new array<string>();

        TransferZSplitPreferenceResolver.Update(m_Preferences.preferred_slot, m_Preferences.preferred_path, m_Preferences.preferred_cargo_index);
    }

    protected void LoadPreferences()
    {
        MakeDirectory(PROFILE_DIR);
        if (!FileExist(PREFERENCES_PATH))
        {
            SyncSplitPreferences();
            return;
        }

        string errorMessage;
        ref TransferZPreferences loaded = new TransferZPreferences();
        if (!JsonFileLoader<TransferZPreferences>.LoadFile(PREFERENCES_PATH, loaded, errorMessage) || !loaded)
        {
            Print("[TransferZ] Failed to load preferences: " + errorMessage);
            SyncSplitPreferences();
            return;
        }

        if (!loaded.preferred_path)
            loaded.preferred_path = new array<string>();
        m_Preferences = loaded;
        SyncSplitPreferences();
    }

    protected void SavePreferences()
    {
        MakeDirectory(PROFILE_DIR);
        SyncSplitPreferences();

        string errorMessage;
        JsonFileLoader<TransferZPreferences>.SaveFile(PREFERENCES_PATH, m_Preferences, errorMessage);
        if (errorMessage != "")
            Print("[TransferZ] Failed to save preferences: " + errorMessage);
    }

    protected bool IsParticipantReachable(EntityAI entity, int cargoIndex = 0)
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || !entity || !TransferZCargo.Exists(entity, cargoIndex))
            return false;

        EntityAI root = entity.GetHierarchyRoot();
        if (!root)
            root = entity;
        if (root == player)
            return true;
        if (root.IsMan())
            return false;

        if (root.IsInherited(Transport))
            return root.CanDisplayCargo();

        return GameInventory.CheckManipulatedObjectsDistances(entity, player, GameInventory.c_MaxItemDistanceRadius);
    }

    protected bool IsTransientParticipantAvailable(EntityAI entity, int cargoIndex = 0)
    {
        if (!IsParticipantReachable(entity, cargoIndex))
            return false;
        return TransferZHeaderControls.IsEntityOpenOrInHands(entity);
    }

    protected void ClearDestination()
    {
        m_Destination = null;
        m_DestinationCargoIndex = 0;
        m_DestinationVicinity = false;
    }

    EntityAI GetDestination()
    {
        if (m_DestinationVicinity)
            return null;
        if (IsTransientParticipantAvailable(m_Destination, m_DestinationCargoIndex))
            return m_Destination;
        m_Destination = null;
        m_DestinationCargoIndex = 0;
        return null;
    }



    int GetDestinationCargoIndex()
    {
        if (!GetDestination())
            return 0;
        return m_DestinationCargoIndex;
    }

    bool IsDestinationVicinity()
    {
        if (!m_DestinationVicinity)
            return false;
        if (TransferZVicinityHeaderControls.IsVicinityOpen())
            return true;
        m_DestinationVicinity = false;
        return false;
    }

    bool HasDestination()
    {
        if (IsDestinationVicinity())
            return true;
        return GetDestination() != null;
    }

    void SetDestination(EntityAI destination, int cargoIndex = 0)
    {
        if (!IsTransientParticipantAvailable(destination, cargoIndex))
            return;
        m_Destination = destination;
        m_DestinationCargoIndex = cargoIndex;
        m_DestinationVicinity = false;
    }

    void SetVicinityDestination()
    {
        if (!TransferZVicinityHeaderControls.IsVicinityOpen())
            return;
        m_Destination = null;
        m_DestinationCargoIndex = 0;
        m_DestinationVicinity = true;
    }

    bool IsDestination(EntityAI entity, int cargoIndex = 0)
    {
        return entity && !m_DestinationVicinity && GetDestination() == entity && m_DestinationCargoIndex == cargoIndex;
    }

    void ToggleDestinationSelection(EntityAI destination, int cargoIndex = 0)
    {
        if (!destination)
            return;
        if (IsDestination(destination, cargoIndex))
        {
            ClearDestination();
            return;
        }
        SetDestination(destination, cargoIndex);
    }

    void ToggleVicinityDestinationSelection()
    {
        if (IsDestinationVicinity())
        {
            ClearDestination();
            return;
        }
        SetVicinityDestination();
    }

    protected void ClearAllLinks()
    {
        m_LinkA = null;
        m_LinkACargoIndex = 0;
        m_LinkB = null;
        m_LinkBCargoIndex = 0;
    }

    void ToggleLink(EntityAI container, int cargoIndex = 0)
    {
        if (!IsTransientParticipantAvailable(container, cargoIndex))
            return;

        if (m_LinkAnchor && !IsTransientParticipantAvailable(m_LinkAnchor, m_LinkAnchorCargoIndex))
        {
            m_LinkAnchor = null;
            m_LinkAnchorCargoIndex = 0;
        }

        if (IsLinked(container, cargoIndex))
        {
            ClearAllLinks();
            m_LinkAnchor = null;
            m_LinkAnchorCargoIndex = 0;
            return;
        }

        if (m_LinkAnchor)
        {
            if (m_LinkAnchor == container && m_LinkAnchorCargoIndex == cargoIndex)
            {
                m_LinkAnchor = null;
                m_LinkAnchorCargoIndex = 0;
                return;
            }

            m_LinkA = m_LinkAnchor;
            m_LinkACargoIndex = m_LinkAnchorCargoIndex;
            m_LinkB = container;
            m_LinkBCargoIndex = cargoIndex;
            m_LinkAnchor = null;
            m_LinkAnchorCargoIndex = 0;
            return;
        }

        ClearAllLinks();
        m_LinkAnchor = container;
        m_LinkAnchorCargoIndex = cargoIndex;
    }

    EntityAI GetLinkedDestination(EntityAI source, int sourceCargoIndex = 0)
    {
        if (!source)
            return null;

        if (m_LinkA == source && m_LinkACargoIndex == sourceCargoIndex)
        {
            if (!IsTransientParticipantAvailable(m_LinkA, m_LinkACargoIndex) || !IsTransientParticipantAvailable(m_LinkB, m_LinkBCargoIndex))
            {
                ClearAllLinks();
                return null;
            }
            return m_LinkB;
        }

        if (m_LinkB == source && m_LinkBCargoIndex == sourceCargoIndex)
        {
            if (!IsTransientParticipantAvailable(m_LinkA, m_LinkACargoIndex) || !IsTransientParticipantAvailable(m_LinkB, m_LinkBCargoIndex))
            {
                ClearAllLinks();
                return null;
            }
            return m_LinkA;
        }

        return null;
    }



    int GetLinkedDestinationCargoIndex(EntityAI source, int sourceCargoIndex = 0)
    {
        EntityAI destination = GetLinkedDestination(source, sourceCargoIndex);
        if (!destination)
            return 0;
        if (m_LinkA == source && m_LinkACargoIndex == sourceCargoIndex)
            return m_LinkBCargoIndex;
        return m_LinkACargoIndex;
    }

    bool IsLinked(EntityAI entity, int cargoIndex = 0)
    {
        return GetLinkedDestination(entity, cargoIndex) != null;
    }

    bool IsLinkAnchor(EntityAI entity, int cargoIndex = 0)
    {
        if (m_LinkAnchor && !IsTransientParticipantAvailable(m_LinkAnchor, m_LinkAnchorCargoIndex))
        {
            m_LinkAnchor = null;
            m_LinkAnchorCargoIndex = 0;
        }
        return entity && m_LinkAnchor == entity && m_LinkAnchorCargoIndex == cargoIndex;
    }

    bool OnContainerLeftHands(EntityAI container)
    {
        if (!container)
            return false;

        bool changed = false;
        if (m_Destination == container)
        {
            ClearDestination();
            changed = true;
        }
        if (m_LinkAnchor == container)
        {
            m_LinkAnchor = null;
            m_LinkAnchorCargoIndex = 0;
            changed = true;
        }
        if (m_LinkA == container || m_LinkB == container)
        {
            ClearAllLinks();
            changed = true;
        }
        return changed;
    }

    bool ValidateTransientState()
    {
        bool changed = false;
        if (m_DestinationVicinity)
        {
            if (!TransferZVicinityHeaderControls.IsVicinityOpen())
            {
                ClearDestination();
                changed = true;
            }
        }
        else if (m_Destination && !IsTransientParticipantAvailable(m_Destination, m_DestinationCargoIndex))
        {
            ClearDestination();
            changed = true;
        }

        if (m_LinkAnchor && !IsTransientParticipantAvailable(m_LinkAnchor, m_LinkAnchorCargoIndex))
        {
            m_LinkAnchor = null;
            m_LinkAnchorCargoIndex = 0;
            changed = true;
        }

        if (m_LinkA && m_LinkB)
        {
            if (!IsTransientParticipantAvailable(m_LinkA, m_LinkACargoIndex) || !IsTransientParticipantAvailable(m_LinkB, m_LinkBCargoIndex))
            {
                ClearAllLinks();
                changed = true;
            }
        }
        return changed;
    }

    bool SetPreferred(EntityAI container, int cargoIndex = 0)
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || !container || !TransferZCargo.Exists(container, cargoIndex))
            return false;

        ref array<string> reversePath = new array<string>();
        EntityAI current = container;
        int depth = 0;
        while (current && current != player && depth < 16)
        {
            InventoryLocation location = new InventoryLocation();
            if (!current.GetInventory().GetCurrentInventoryLocation(location) || location.GetType() != InventoryLocationType.ATTACHMENT)
                return false;

            EntityAI parent = location.GetParent();
            if (!parent)
                return false;

            string slotName = InventorySlots.GetSlotName(location.GetSlot());
            if (slotName == "")
                return false;

            reversePath.Insert(slotName);
            current = parent;
            depth++;
        }

        if (current != player || reversePath.Count() == 0)
            return false;

        if (!m_Preferences.preferred_path)
            m_Preferences.preferred_path = new array<string>();
        m_Preferences.preferred_path.Clear();
        for (int i = reversePath.Count() - 1; i >= 0; i--)
            m_Preferences.preferred_path.Insert(reversePath.Get(i));

        m_Preferences.preferred_slot = "";
        if (m_Preferences.preferred_path.Count() == 1)
            m_Preferences.preferred_slot = m_Preferences.preferred_path.Get(0);
        m_Preferences.preferred_cargo_index = cargoIndex;
        SavePreferences();
        return true;
    }

    bool ClearPreferred()
    {
        if (!m_Preferences)
            m_Preferences = new TransferZPreferences();
        if (!m_Preferences.preferred_path)
            m_Preferences.preferred_path = new array<string>();

        bool changed = m_Preferences.preferred_path.Count() > 0 || m_Preferences.preferred_slot != "" || m_Preferences.preferred_cargo_index != 0;
        m_Preferences.preferred_path.Clear();
        m_Preferences.preferred_slot = "";
        m_Preferences.preferred_cargo_index = 0;
        SavePreferences();
        return changed;
    }

    bool TogglePreferred(EntityAI container, int cargoIndex = 0)
    {
        if (!container)
            return false;
        if (IsPreferred(container, cargoIndex))
            return ClearPreferred();
        return SetPreferred(container, cargoIndex);
    }

    EntityAI GetPreferredDestination()
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || !m_Preferences)
            return null;

        if (m_Preferences.preferred_path && m_Preferences.preferred_path.Count() > 0)
        {
            EntityAI current = player;
            for (int i = 0; i < m_Preferences.preferred_path.Count(); i++)
            {
                string slotName = m_Preferences.preferred_path.Get(i);
                if (slotName == "")
                    return null;
                current = current.FindAttachmentBySlotName(slotName);
                if (!current)
                    return null;
            }
            if (TransferZCargo.Exists(current, m_Preferences.preferred_cargo_index))
                return current;
            return null;
        }

        if (m_Preferences.preferred_slot == "")
            return null;
        EntityAI legacyDestination = player.FindAttachmentBySlotName(m_Preferences.preferred_slot);
        if (!legacyDestination || !TransferZCargo.Exists(legacyDestination, m_Preferences.preferred_cargo_index))
            return null;
        return legacyDestination;
    }

    int GetPreferredCargoIndex()
    {
        if (!GetPreferredDestination())
            return 0;
        return m_Preferences.preferred_cargo_index;
    }

    bool IsPreferred(EntityAI entity, int cargoIndex = 0)
    {
        return entity && GetPreferredDestination() == entity && GetPreferredCargoIndex() == cargoIndex;
    }

    bool CanPreferredAcceptItem(EntityAI item, EntityAI destination = null, int destinationCargoIndex = -1)
    {
        if (!item)
            return false;
        if (!destination)
        {
            destination = GetPreferredDestination();
            destinationCargoIndex = GetPreferredCargoIndex();
        }
        else if (destinationCargoIndex < 0)
        {
            destinationCargoIndex = 0;
        }

        if (!destination || item == destination || !TransferZCargo.Exists(destination, destinationCargoIndex))
            return false;
        if (!destination.CanReceiveItemIntoCargo(item))
            return false;

        InventoryLocation dst;
        return TransferZCargo.FindFreeLocation(destination, destinationCargoIndex, item, dst) && TransferZCargo.LocationMatches(dst, destination, destinationCargoIndex);
    }

    protected bool IsDuplicateRequest(int operation, EntityAI source, int sourceCargoIndex, EntityAI destination, int destinationCargoIndex, EntityAI item, bool destinationIsVicinity)
    {
        int now = GetGame().GetTime();
        bool sameRequest = operation == m_LastRequestOperation && source == m_LastRequestSource && sourceCargoIndex == m_LastRequestSourceCargoIndex && destination == m_LastRequestDestination && destinationCargoIndex == m_LastRequestDestinationCargoIndex && item == m_LastRequestItem && destinationIsVicinity == m_LastRequestDestinationVicinity;
        if (sameRequest && now - m_LastRequestTime >= 0 && now - m_LastRequestTime < REQUEST_DEBOUNCE_MS)
            return true;

        m_LastRequestTime = now;
        m_LastRequestOperation = operation;
        m_LastRequestSource = source;
        m_LastRequestSourceCargoIndex = sourceCargoIndex;
        m_LastRequestDestination = destination;
        m_LastRequestDestinationCargoIndex = destinationCargoIndex;
        m_LastRequestItem = item;
        m_LastRequestDestinationVicinity = destinationIsVicinity;
        return false;
    }

    protected void ExecuteOfflineRequest(int operation, PlayerBase player, EntityAI source, int sourceCargoIndex, EntityAI destination, int destinationCargoIndex, EntityAI item, bool destinationIsVicinity)
    {
        if (destinationIsVicinity)
        {
            if (operation == TransferZOperation.TRANSFER)
                TransferZServerService.TransferToVicinity(player, source, sourceCargoIndex);
            else if (operation == TransferZOperation.MOVE_ITEM)
                TransferZServerService.MoveItemToVicinity(player, item);
            else if (operation == TransferZOperation.TRANSFER_CLASS)
                TransferZServerService.TransferClassToVicinity(player, source, item, sourceCargoIndex);
        }
        else
        {
            if (operation == TransferZOperation.TRANSFER)
                TransferZServerService.Transfer(player, source, destination, sourceCargoIndex, destinationCargoIndex);
            else if (operation == TransferZOperation.MOVE_ITEM)
                TransferZServerService.MoveItem(player, item, destination, destinationCargoIndex);
            else if (operation == TransferZOperation.TRANSFER_CLASS)
                TransferZServerService.TransferClass(player, source, destination, item, sourceCargoIndex, destinationCargoIndex);
        }
        player.UpdateInventoryMenu();
    }

    protected void SendRequest(int operation, EntityAI source, int sourceCargoIndex, EntityAI destination, int destinationCargoIndex, EntityAI item, bool destinationIsVicinity = false)
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
            return;
        if (source && !IsParticipantReachable(source, sourceCargoIndex))
            return;
        if (!destinationIsVicinity && !IsParticipantReachable(destination, destinationCargoIndex))
            return;
        if (IsDuplicateRequest(operation, source, sourceCargoIndex, destination, destinationCargoIndex, item, destinationIsVicinity))
            return;

        if (!GetGame().IsMultiplayer())
        {
            ExecuteOfflineRequest(operation, player, source, sourceCargoIndex, destination, destinationCargoIndex, item, destinationIsVicinity);
            return;
        }

        int sourceLow;
        int sourceHigh;
        int destinationLow;
        int destinationHigh;
        int itemLow;
        int itemHigh;
        TransferZNet.GetEntityNetworkId(source, sourceLow, sourceHigh);
        TransferZNet.GetEntityNetworkId(destination, destinationLow, destinationHigh);
        TransferZNet.GetEntityNetworkId(item, itemLow, itemHigh);

        ScriptRPC rpc = new ScriptRPC();
        rpc.Write(operation);
        rpc.Write(sourceLow);
        rpc.Write(sourceHigh);
        rpc.Write(sourceCargoIndex);
        rpc.Write(destinationLow);
        rpc.Write(destinationHigh);
        rpc.Write(destinationCargoIndex);
        rpc.Write(itemLow);
        rpc.Write(itemHigh);
        rpc.Write(destinationIsVicinity);
        rpc.Send(player, TransferZRPC.REQUEST, true, player.GetIdentity());
    }

    protected void SendNestedUnpackRequest(EntityAI source, int sourceCargoIndex, EntityAI destination, int destinationCargoIndex, bool destinationIsVicinity)
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || !source || !TransferZCargo.Exists(source, sourceCargoIndex))
            return;
        if (!destinationIsVicinity && !IsParticipantReachable(destination, destinationCargoIndex))
            return;
        if (IsDuplicateRequest(TransferZNestedUnpackRPC.CLIENT_OPERATION, source, sourceCargoIndex, destination, destinationCargoIndex, null, destinationIsVicinity))
            return;

        if (!GetGame().IsMultiplayer())
        {
            if (destinationIsVicinity)
                TransferZNestedUnpackService.UnpackToVicinity(player, source, sourceCargoIndex);
            else
                TransferZNestedUnpackService.Unpack(player, source, destination, sourceCargoIndex, destinationCargoIndex);
            player.UpdateInventoryMenu();
            return;
        }

        int sourceLow;
        int sourceHigh;
        int destinationLow;
        int destinationHigh;
        TransferZNet.GetEntityNetworkId(source, sourceLow, sourceHigh);
        TransferZNet.GetEntityNetworkId(destination, destinationLow, destinationHigh);

        ScriptRPC rpc = new ScriptRPC();
        rpc.Write(sourceLow);
        rpc.Write(sourceHigh);
        rpc.Write(sourceCargoIndex);
        rpc.Write(destinationLow);
        rpc.Write(destinationHigh);
        rpc.Write(destinationCargoIndex);
        rpc.Write(destinationIsVicinity);
        rpc.Send(player, TransferZNestedUnpackRPC.REQUEST, true, player.GetIdentity());
    }

    bool RequestTransferTo(EntityAI source, EntityAI destination, int sourceCargoIndex = 0, int destinationCargoIndex = 0)
    {
        if (!source || !IsParticipantReachable(source, sourceCargoIndex) || !IsParticipantReachable(destination, destinationCargoIndex))
            return false;
        if (source == destination && sourceCargoIndex == destinationCargoIndex)
            return false;
        SendRequest(TransferZOperation.TRANSFER, source, sourceCargoIndex, destination, destinationCargoIndex, null, false);
        return true;
    }

    bool RequestTransferToVicinity(EntityAI source, int sourceCargoIndex = 0)
    {
        if (!source || !TransferZCargo.Exists(source, sourceCargoIndex))
            return false;
        SendRequest(TransferZOperation.TRANSFER, source, sourceCargoIndex, null, 0, null, true);
        return true;
    }

    bool RequestTransfer(EntityAI source, int sourceCargoIndex = 0)
    {
        if (IsDestinationVicinity())
            return RequestTransferToVicinity(source, sourceCargoIndex);
        EntityAI destination = GetDestination();
        return RequestTransferTo(source, destination, sourceCargoIndex, GetDestinationCargoIndex());
    }

    bool RequestClassTransferTo(EntityAI source, EntityAI destination, EntityAI representative, int sourceCargoIndex = 0, int destinationCargoIndex = 0)
    {
        if (!source || !representative || !IsParticipantReachable(source, sourceCargoIndex) || !IsParticipantReachable(destination, destinationCargoIndex))
            return false;
        if (source == destination && sourceCargoIndex == destinationCargoIndex)
            return false;

        InventoryLocation representativeLocation = new InventoryLocation();
        if (!representative.GetInventory().GetCurrentInventoryLocation(representativeLocation))
            return false;
        if (!TransferZCargo.LocationMatches(representativeLocation, source, sourceCargoIndex))
            return false;

        SendRequest(TransferZOperation.TRANSFER_CLASS, source, sourceCargoIndex, destination, destinationCargoIndex, representative, false);
        return true;
    }

    bool RequestClassTransferToVicinity(EntityAI source, EntityAI representative, int sourceCargoIndex = 0)
    {
        if (!source || !representative || !IsParticipantReachable(source, sourceCargoIndex))
            return false;

        InventoryLocation representativeLocation = new InventoryLocation();
        if (!representative.GetInventory().GetCurrentInventoryLocation(representativeLocation))
            return false;
        if (!TransferZCargo.LocationMatches(representativeLocation, source, sourceCargoIndex))
            return false;

        SendRequest(TransferZOperation.TRANSFER_CLASS, source, sourceCargoIndex, null, 0, representative, true);
        return true;
    }

    bool RequestNestedUnpackTo(EntityAI source, EntityAI destination, int sourceCargoIndex = 0, int destinationCargoIndex = 0)
    {
        if (!source || !TransferZCargo.Exists(source, sourceCargoIndex) || !IsParticipantReachable(destination, destinationCargoIndex))
            return false;
        SendNestedUnpackRequest(source, sourceCargoIndex, destination, destinationCargoIndex, false);
        return true;
    }

    bool RequestNestedUnpackToVicinity(EntityAI source, int sourceCargoIndex = 0)
    {
        if (!source || !TransferZCargo.Exists(source, sourceCargoIndex))
            return false;
        SendNestedUnpackRequest(source, sourceCargoIndex, null, 0, true);
        return true;
    }

    bool RequestNestedUnpack(EntityAI source, int sourceCargoIndex = 0)
    {
        if (IsDestinationVicinity())
            return RequestNestedUnpackToVicinity(source, sourceCargoIndex);
        EntityAI destination = GetDestination();
        return RequestNestedUnpackTo(source, destination, sourceCargoIndex, GetDestinationCargoIndex());
    }

    bool RequestMoveItem(EntityAI item, EntityAI destination, int destinationCargoIndex = 0)
    {
        if (!item || !IsParticipantReachable(destination, destinationCargoIndex) || item == destination)
            return false;
        SendRequest(TransferZOperation.MOVE_ITEM, null, 0, destination, destinationCargoIndex, item, false);
        return true;
    }

    bool RequestMoveItemToVicinity(EntityAI item)
    {
        if (!item)
            return false;
        SendRequest(TransferZOperation.MOVE_ITEM, null, 0, null, 0, item, true);
        return true;
    }

    protected bool TransferZItemAlreadyInCargo(EntityAI item, EntityAI destination, int destinationCargoIndex = 0)
    {
        if (!item || !destination)
            return false;
        InventoryLocation location = new InventoryLocation();
        if (!item.GetInventory().GetCurrentInventoryLocation(location))
            return false;
        return TransferZCargo.LocationMatches(location, destination, destinationCargoIndex);
    }

    protected bool TransferZItemAlreadyInVicinity(EntityAI item)
    {
        if (!item)
            return false;
        InventoryLocation location = new InventoryLocation();
        if (!item.GetInventory().GetCurrentInventoryLocation(location))
            return false;
        return location.GetType() == InventoryLocationType.GROUND;
    }

    bool RequestItemToDestination(EntityAI item)
    {
        if (!item)
            return false;
        if (IsDestinationVicinity())
        {
            if (TransferZItemAlreadyInVicinity(item))
                return false;
            return RequestMoveItemToVicinity(item);
        }

        EntityAI destination = GetDestination();
        int destinationCargoIndex = GetDestinationCargoIndex();
        if (!destination || item == destination || TransferZItemAlreadyInCargo(item, destination, destinationCargoIndex))
            return false;
        return RequestMoveItem(item, destination, destinationCargoIndex);
    }

    bool RequestItemToPreferred(EntityAI item)
    {
        if (!item)
            return false;
        EntityAI destination = GetPreferredDestination();
        int destinationCargoIndex = GetPreferredCargoIndex();
        if (!destination || item == destination || TransferZItemAlreadyInCargo(item, destination, destinationCargoIndex))
            return false;
        return RequestMoveItem(item, destination, destinationCargoIndex);
    }

    protected bool IsVicinityTransferCandidate(EntityAI item, EntityAI destination)
    {
        if (!item || item == destination)
            return false;

        // A cargo-bearing ground item is still a movable vicinity item.
        // Server-side MoveItem() rejects moving a destination into one of its
        // own descendants, so containers use the same exact-cargo move path.
        ItemBase itemBase = ItemBase.Cast(item);
        return itemBase && itemBase.IsTakeable() && item.GetInventory().CanRemoveEntity();
    }

    bool RequestVicinityTransferTo(notnull array<EntityAI> items, EntityAI destination, int destinationCargoIndex = 0)
    {
        if (!IsParticipantReachable(destination, destinationCargoIndex))
            return false;

        bool requested = false;
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
            return false;

        if (!GetGame().IsMultiplayer())
        {
            foreach (EntityAI offlineItem : items)
            {
                if (!IsVicinityTransferCandidate(offlineItem, destination))
                    continue;
                if (TransferZServerService.MoveItem(player, offlineItem, destination, destinationCargoIndex))
                    requested = true;
            }
            if (requested)
                player.UpdateInventoryMenu();
            return requested;
        }

        foreach (EntityAI item : items)
        {
            if (!IsVicinityTransferCandidate(item, destination))
                continue;
            if (RequestMoveItem(item, destination, destinationCargoIndex))
                requested = true;
        }
        return requested;
    }

    bool RequestVicinityTransfer(notnull array<EntityAI> items)
    {
        if (IsDestinationVicinity())
            return false;
        EntityAI destination = GetDestination();
        return RequestVicinityTransferTo(items, destination, GetDestinationCargoIndex());
    }

    bool RequestVicinityUnpackTo(notnull array<EntityAI> items, EntityAI destination, int destinationCargoIndex = 0)
    {
        if (!IsParticipantReachable(destination, destinationCargoIndex))
            return false;

        bool requested = false;
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
            return false;

        if (!GetGame().IsMultiplayer())
        {
            foreach (EntityAI offlineContainer : items)
            {
                if (!offlineContainer || offlineContainer == destination || !TransferZCargo.Exists(offlineContainer, 0))
                    continue;
                if (TransferZNestedUnpackService.Unpack(player, offlineContainer, destination, 0, destinationCargoIndex) > 0)
                    requested = true;
            }
            if (requested)
                player.UpdateInventoryMenu();
            return requested;
        }

        foreach (EntityAI container : items)
        {
            if (!container || container == destination || !TransferZCargo.Exists(container, 0))
                continue;
            if (RequestNestedUnpackTo(container, destination, 0, destinationCargoIndex))
                requested = true;
        }
        return requested;
    }

    bool RequestVicinityUnpackToVicinity(notnull array<EntityAI> items)
    {
        bool requested = false;
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
            return false;

        if (!GetGame().IsMultiplayer())
        {
            foreach (EntityAI offlineContainer : items)
            {
                if (!offlineContainer || !TransferZCargo.Exists(offlineContainer, 0))
                    continue;
                if (TransferZNestedUnpackService.UnpackToVicinity(player, offlineContainer) > 0)
                    requested = true;
            }
            if (requested)
                player.UpdateInventoryMenu();
            return requested;
        }

        foreach (EntityAI container : items)
        {
            if (!container || !TransferZCargo.Exists(container, 0))
                continue;
            if (RequestNestedUnpackToVicinity(container))
                requested = true;
        }
        return requested;
    }

    bool RequestVicinityUnpack(notnull array<EntityAI> items)
    {
        if (IsDestinationVicinity())
            return RequestVicinityUnpackToVicinity(items);
        EntityAI destination = GetDestination();
        return RequestVicinityUnpackTo(items, destination, GetDestinationCargoIndex());
    }

    bool TryRouteCargoDoubleClick(EntityAI source, EntityAI item, int sourceCargoIndex = 0)
    {
        EntityAI destination = GetLinkedDestination(source, sourceCargoIndex);
        if (!destination)
            return false;
        return RequestMoveItem(item, destination, GetLinkedDestinationCargoIndex(source, sourceCargoIndex));
    }

    bool TryRouteVicinityDoubleClick(EntityAI item)
    {
        EntityAI destination = GetPreferredDestination();
        int destinationCargoIndex = GetPreferredCargoIndex();
        if (!destination || !CanPreferredAcceptItem(item, destination, destinationCargoIndex))
            return false;
        return RequestMoveItem(item, destination, destinationCargoIndex);
    }
}
