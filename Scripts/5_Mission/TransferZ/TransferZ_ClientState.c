class TransferZPreferences
{
    string preferred_slot = "";
    ref array<string> preferred_path;

    void TransferZPreferences()
    {
        preferred_path = new array<string>();
    }
}

class TransferZClientState
{
    static const string PROFILE_DIR = "$profile:TransferZ";
    static const string PREFERENCES_PATH = "$profile:TransferZ/preferences.json";
    static const int REQUEST_DEBOUNCE_MS = 150;

    protected static ref TransferZClientState s_Instance;
    protected EntityAI m_Destination;
    protected bool m_DestinationVicinity;
    protected EntityAI m_LinkAnchor;
    protected ref map<EntityAI, EntityAI> m_Links;
    protected ref TransferZPreferences m_Preferences;

    protected int m_LastRequestTime = -1000;
    protected int m_LastRequestOperation = -1;
    protected EntityAI m_LastRequestSource;
    protected EntityAI m_LastRequestDestination;
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
        m_Links = new map<EntityAI, EntityAI>();
        m_Preferences = new TransferZPreferences();
        LoadPreferences();
    }

    protected void LoadPreferences()
    {
        MakeDirectory(PROFILE_DIR);
        if (!FileExist(PREFERENCES_PATH))
            return;

        string errorMessage;
        ref TransferZPreferences loaded = new TransferZPreferences();
        if (JsonFileLoader<TransferZPreferences>.LoadFile(PREFERENCES_PATH, loaded, errorMessage) && loaded)
        {
            if (!loaded.preferred_path)
                loaded.preferred_path = new array<string>();
            m_Preferences = loaded;
        }
    }

    protected void SavePreferences()
    {
        MakeDirectory(PROFILE_DIR);
        string errorMessage;
        JsonFileLoader<TransferZPreferences>.SaveFile(PREFERENCES_PATH, m_Preferences, errorMessage);
    }

    protected bool IsParticipantReachable(EntityAI entity)
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || !entity || !entity.GetInventory().GetCargo())
            return false;

        EntityAI root = entity.GetHierarchyRoot();
        if (!root)
            root = entity;
        if (root == player)
            return true;
        if (root.IsMan())
            return false;

        return GameInventory.CheckManipulatedObjectsDistances(entity, player, GameInventory.c_MaxItemDistanceRadius);
    }

    protected bool IsTransientParticipantAvailable(EntityAI entity)
    {
        if (!IsParticipantReachable(entity))
            return false;
        return TransferZHeaderControls.IsEntityOpenOrInHands(entity);
    }

    protected void ClearDestination()
    {
        m_Destination = null;
        m_DestinationVicinity = false;
    }

    EntityAI GetDestination()
    {
        if (m_DestinationVicinity)
            return null;
        if (IsTransientParticipantAvailable(m_Destination))
            return m_Destination;
        m_Destination = null;
        return null;
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

    void SetDestination(EntityAI destination)
    {
        if (!IsTransientParticipantAvailable(destination))
            return;
        m_Destination = destination;
        m_DestinationVicinity = false;
    }

    void SetVicinityDestination()
    {
        if (!TransferZVicinityHeaderControls.IsVicinityOpen())
            return;
        m_Destination = null;
        m_DestinationVicinity = true;
    }

    bool IsDestination(EntityAI entity)
    {
        return entity && !m_DestinationVicinity && GetDestination() == entity;
    }

    void ToggleDestinationSelection(EntityAI destination)
    {
        if (!destination)
            return;
        if (IsDestination(destination))
        {
            ClearDestination();
            return;
        }
        SetDestination(destination);
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
        if (m_Links)
            m_Links.Clear();
    }

    void ToggleLink(EntityAI container)
    {
        if (!IsTransientParticipantAvailable(container))
            return;

        if (m_LinkAnchor && !IsTransientParticipantAvailable(m_LinkAnchor))
            m_LinkAnchor = null;

        if (m_Links.Contains(container))
        {
            ClearAllLinks();
            m_LinkAnchor = null;
            return;
        }

        if (m_LinkAnchor)
        {
            if (m_LinkAnchor == container)
            {
                m_LinkAnchor = null;
                return;
            }

            EntityAI first = m_LinkAnchor;
            m_LinkAnchor = null;
            ClearAllLinks();
            m_Links.Set(first, container);
            m_Links.Set(container, first);
            return;
        }

        if (m_Links.Count() > 0)
            ClearAllLinks();
        m_LinkAnchor = container;
    }

    EntityAI GetLinkedDestination(EntityAI source)
    {
        if (!source || !m_Links.Contains(source))
            return null;

        EntityAI destination = m_Links.Get(source);
        if (!IsTransientParticipantAvailable(source) || !IsTransientParticipantAvailable(destination))
        {
            ClearAllLinks();
            m_LinkAnchor = null;
            return null;
        }
        return destination;
    }

    bool IsLinked(EntityAI entity)
    {
        return GetLinkedDestination(entity) != null;
    }

    bool IsLinkAnchor(EntityAI entity)
    {
        if (m_LinkAnchor && !IsTransientParticipantAvailable(m_LinkAnchor))
            m_LinkAnchor = null;
        return entity && m_LinkAnchor == entity;
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
            changed = true;
        }
        if (m_Links && m_Links.Contains(container))
        {
            ClearAllLinks();
            m_LinkAnchor = null;
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
        else if (m_Destination && !IsTransientParticipantAvailable(m_Destination))
        {
            ClearDestination();
            changed = true;
        }

        if (m_LinkAnchor && !IsTransientParticipantAvailable(m_LinkAnchor))
        {
            m_LinkAnchor = null;
            changed = true;
        }

        if (m_Links && m_Links.Count() > 0)
        {
            EntityAI first = m_Links.GetKey(0);
            EntityAI second = m_Links.GetElement(0);
            if (!IsTransientParticipantAvailable(first) || !IsTransientParticipantAvailable(second))
            {
                ClearAllLinks();
                m_LinkAnchor = null;
                changed = true;
            }
        }
        return changed;
    }

    bool SetPreferred(EntityAI container)
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || !container || !container.GetInventory().GetCargo())
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
        SavePreferences();
        return true;
    }

    bool ClearPreferred()
    {
        if (!m_Preferences)
            m_Preferences = new TransferZPreferences();
        if (!m_Preferences.preferred_path)
            m_Preferences.preferred_path = new array<string>();

        bool changed = m_Preferences.preferred_path.Count() > 0 || m_Preferences.preferred_slot != "";
        m_Preferences.preferred_path.Clear();
        m_Preferences.preferred_slot = "";
        SavePreferences();
        return changed;
    }

    bool TogglePreferred(EntityAI container)
    {
        if (!container)
            return false;
        if (IsPreferred(container))
            return ClearPreferred();
        return SetPreferred(container);
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
            if (current.GetInventory().GetCargo())
                return current;
            return null;
        }

        if (m_Preferences.preferred_slot == "")
            return null;
        EntityAI legacyDestination = player.FindAttachmentBySlotName(m_Preferences.preferred_slot);
        if (!legacyDestination || !legacyDestination.GetInventory().GetCargo())
            return null;
        return legacyDestination;
    }

    bool IsPreferred(EntityAI entity)
    {
        return entity && GetPreferredDestination() == entity;
    }

    bool CanPreferredAcceptItem(EntityAI item, EntityAI destination = null)
    {
        if (!item)
            return false;
        if (!destination)
            destination = GetPreferredDestination();
        if (!destination || item == destination)
            return false;

        CargoBase cargo = destination.GetInventory().GetCargo();
        if (!cargo || !destination.CanReceiveItemIntoCargo(item))
            return false;

        InventoryLocation dst = new InventoryLocation();
        if (!destination.GetInventory().FindFreeLocationFor(item, FindInventoryLocationType.CARGO, dst))
            return false;
        return dst.IsValid() && dst.GetType() == InventoryLocationType.CARGO && dst.GetParent() == destination;
    }

    protected bool IsDuplicateRequest(int operation, EntityAI source, EntityAI destination, EntityAI item, bool destinationIsVicinity)
    {
        int now = GetGame().GetTime();
        bool sameRequest = operation == m_LastRequestOperation && source == m_LastRequestSource && destination == m_LastRequestDestination && item == m_LastRequestItem && destinationIsVicinity == m_LastRequestDestinationVicinity;
        if (sameRequest && now - m_LastRequestTime >= 0 && now - m_LastRequestTime < REQUEST_DEBOUNCE_MS)
            return true;

        m_LastRequestTime = now;
        m_LastRequestOperation = operation;
        m_LastRequestSource = source;
        m_LastRequestDestination = destination;
        m_LastRequestItem = item;
        m_LastRequestDestinationVicinity = destinationIsVicinity;
        return false;
    }

    protected void ExecuteOfflineRequest(int operation, PlayerBase player, EntityAI source, EntityAI destination, EntityAI item, bool destinationIsVicinity)
    {
        if (destinationIsVicinity)
        {
            if (operation == TransferZOperation.TRANSFER)
                TransferZServerService.TransferToVicinity(player, source);
            else if (operation == TransferZOperation.UNPACK)
                TransferZServerService.UnpackToVicinity(player, source);
            else if (operation == TransferZOperation.MOVE_ITEM)
                TransferZServerService.MoveItemToVicinity(player, item);
            else if (operation == TransferZOperation.TRANSFER_CLASS)
                TransferZServerService.TransferClassToVicinity(player, source, item);
        }
        else
        {
            if (operation == TransferZOperation.TRANSFER)
                TransferZServerService.Transfer(player, source, destination);
            else if (operation == TransferZOperation.UNPACK)
                TransferZServerService.Unpack(player, source, destination);
            else if (operation == TransferZOperation.MOVE_ITEM)
                TransferZServerService.MoveItem(player, item, destination);
            else if (operation == TransferZOperation.TRANSFER_CLASS)
                TransferZServerService.TransferClass(player, source, destination, item);
        }
        player.UpdateInventoryMenu();
    }

    protected void SendRequest(int operation, EntityAI source, EntityAI destination, EntityAI item, bool destinationIsVicinity = false)
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
            return;
        if (!destinationIsVicinity && !IsParticipantReachable(destination))
            return;
        if (IsDuplicateRequest(operation, source, destination, item, destinationIsVicinity))
            return;

        if (!GetGame().IsMultiplayer())
        {
            ExecuteOfflineRequest(operation, player, source, destination, item, destinationIsVicinity);
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
        rpc.Write(destinationLow);
        rpc.Write(destinationHigh);
        rpc.Write(itemLow);
        rpc.Write(itemHigh);
        rpc.Write(destinationIsVicinity);
        rpc.Send(player, TransferZRPC.REQUEST, true, player.GetIdentity());
    }

    protected void SendNestedUnpackRequest(EntityAI source, EntityAI destination, bool destinationIsVicinity)
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || !source || !source.GetInventory().GetCargo())
            return;
        if (!destinationIsVicinity && !IsParticipantReachable(destination))
            return;
        if (IsDuplicateRequest(TransferZNestedUnpackRPC.CLIENT_OPERATION, source, destination, null, destinationIsVicinity))
            return;

        if (!GetGame().IsMultiplayer())
        {
            if (destinationIsVicinity)
                TransferZNestedUnpackService.UnpackToVicinity(player, source);
            else
                TransferZNestedUnpackService.Unpack(player, source, destination);
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
        rpc.Write(destinationLow);
        rpc.Write(destinationHigh);
        rpc.Write(destinationIsVicinity);
        rpc.Send(player, TransferZNestedUnpackRPC.REQUEST, true, player.GetIdentity());
    }

    bool RequestTransferTo(EntityAI source, EntityAI destination)
    {
        if (!source || !IsParticipantReachable(destination) || source == destination)
            return false;
        SendRequest(TransferZOperation.TRANSFER, source, destination, null, false);
        return true;
    }

    bool RequestTransferToVicinity(EntityAI source)
    {
        if (!source || !source.GetInventory().GetCargo())
            return false;
        SendRequest(TransferZOperation.TRANSFER, source, null, null, true);
        return true;
    }

    bool RequestTransfer(EntityAI source)
    {
        if (IsDestinationVicinity())
            return RequestTransferToVicinity(source);
        return RequestTransferTo(source, GetDestination());
    }

    bool RequestClassTransferTo(EntityAI source, EntityAI destination, EntityAI representative)
    {
        if (!source || !representative || !IsParticipantReachable(destination) || source == destination)
            return false;

        InventoryLocation representativeLocation = new InventoryLocation();
        if (!representative.GetInventory().GetCurrentInventoryLocation(representativeLocation))
            return false;
        if (representativeLocation.GetType() != InventoryLocationType.CARGO || representativeLocation.GetParent() != source)
            return false;

        SendRequest(TransferZOperation.TRANSFER_CLASS, source, destination, representative, false);
        return true;
    }

    bool RequestClassTransferToVicinity(EntityAI source, EntityAI representative)
    {
        if (!source || !representative)
            return false;

        InventoryLocation representativeLocation = new InventoryLocation();
        if (!representative.GetInventory().GetCurrentInventoryLocation(representativeLocation))
            return false;
        if (representativeLocation.GetType() != InventoryLocationType.CARGO || representativeLocation.GetParent() != source)
            return false;

        SendRequest(TransferZOperation.TRANSFER_CLASS, source, null, representative, true);
        return true;
    }

    bool RequestUnpackTo(EntityAI source, EntityAI destination)
    {
        if (!source || !IsParticipantReachable(destination))
            return false;
        SendRequest(TransferZOperation.UNPACK, source, destination, null, false);
        return true;
    }

    bool RequestUnpackToVicinity(EntityAI source)
    {
        if (!source || !source.GetInventory().GetCargo())
            return false;
        SendRequest(TransferZOperation.UNPACK, source, null, null, true);
        return true;
    }

    bool RequestUnpack(EntityAI source)
    {
        if (IsDestinationVicinity())
            return RequestUnpackToVicinity(source);
        return RequestUnpackTo(source, GetDestination());
    }

    bool RequestNestedUnpackTo(EntityAI source, EntityAI destination)
    {
        if (!source || !source.GetInventory().GetCargo() || !IsParticipantReachable(destination))
            return false;
        SendNestedUnpackRequest(source, destination, false);
        return true;
    }

    bool RequestNestedUnpackToVicinity(EntityAI source)
    {
        if (!source || !source.GetInventory().GetCargo())
            return false;
        SendNestedUnpackRequest(source, null, true);
        return true;
    }

    bool RequestNestedUnpack(EntityAI source)
    {
        if (IsDestinationVicinity())
            return RequestNestedUnpackToVicinity(source);
        return RequestNestedUnpackTo(source, GetDestination());
    }

    bool RequestMoveItem(EntityAI item, EntityAI destination)
    {
        if (!item || !IsParticipantReachable(destination) || item == destination)
            return false;
        SendRequest(TransferZOperation.MOVE_ITEM, null, destination, item, false);
        return true;
    }

    bool RequestMoveItemToVicinity(EntityAI item)
    {
        if (!item)
            return false;
        SendRequest(TransferZOperation.MOVE_ITEM, null, null, item, true);
        return true;
    }

    protected bool TransferZItemAlreadyInCargo(EntityAI item, EntityAI destination)
    {
        if (!item || !destination)
            return false;
        InventoryLocation location = new InventoryLocation();
        if (!item.GetInventory().GetCurrentInventoryLocation(location))
            return false;
        return location.GetType() == InventoryLocationType.CARGO && location.GetParent() == destination;
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
        if (!destination || item == destination || TransferZItemAlreadyInCargo(item, destination))
            return false;
        return RequestMoveItem(item, destination);
    }

    bool RequestItemToPreferred(EntityAI item)
    {
        if (!item)
            return false;
        EntityAI destination = GetPreferredDestination();
        if (!destination || item == destination || TransferZItemAlreadyInCargo(item, destination))
            return false;
        return RequestMoveItem(item, destination);
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

    bool RequestVicinityTransferTo(notnull array<EntityAI> items, EntityAI destination)
    {
        if (!IsParticipantReachable(destination))
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
                if (TransferZServerService.MoveItem(player, offlineItem, destination))
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
            if (RequestMoveItem(item, destination))
                requested = true;
        }
        return requested;
    }

    bool RequestVicinityTransfer(notnull array<EntityAI> items)
    {
        if (IsDestinationVicinity())
            return false;
        return RequestVicinityTransferTo(items, GetDestination());
    }

    bool RequestVicinityUnpackTo(notnull array<EntityAI> items, EntityAI destination)
    {
        if (!IsParticipantReachable(destination))
            return false;

        bool requested = false;
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
            return false;

        if (!GetGame().IsMultiplayer())
        {
            foreach (EntityAI offlineContainer : items)
            {
                if (!offlineContainer || offlineContainer == destination || !offlineContainer.GetInventory().GetCargo())
                    continue;
                if (TransferZServerService.Unpack(player, offlineContainer, destination) > 0)
                    requested = true;
            }
            if (requested)
                player.UpdateInventoryMenu();
            return requested;
        }

        foreach (EntityAI container : items)
        {
            if (!container || container == destination || !container.GetInventory().GetCargo())
                continue;
            if (RequestUnpackTo(container, destination))
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
                if (!offlineContainer || !offlineContainer.GetInventory().GetCargo())
                    continue;
                if (TransferZServerService.UnpackToVicinity(player, offlineContainer) > 0)
                    requested = true;
            }
            if (requested)
                player.UpdateInventoryMenu();
            return requested;
        }

        foreach (EntityAI container : items)
        {
            if (!container || !container.GetInventory().GetCargo())
                continue;
            if (RequestUnpackToVicinity(container))
                requested = true;
        }
        return requested;
    }

    bool RequestVicinityUnpack(notnull array<EntityAI> items)
    {
        if (IsDestinationVicinity())
            return RequestVicinityUnpackToVicinity(items);
        return RequestVicinityUnpackTo(items, GetDestination());
    }

    bool TryRouteCargoDoubleClick(EntityAI source, EntityAI item)
    {
        EntityAI destination = GetLinkedDestination(source);
        if (!destination)
            return false;
        return RequestMoveItem(item, destination);
    }

    bool TryRouteVicinityDoubleClick(EntityAI item)
    {
        EntityAI destination = GetPreferredDestination();
        if (!destination || !CanPreferredAcceptItem(item, destination))
            return false;
        return RequestMoveItem(item, destination);
    }
}
