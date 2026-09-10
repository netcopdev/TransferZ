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
    protected EntityAI m_LinkAnchor;
    protected ref map<EntityAI, EntityAI> m_Links;
    protected ref TransferZPreferences m_Preferences;

    protected int m_LastRequestTime = -1000;
    protected int m_LastRequestOperation = -1;
    protected EntityAI m_LastRequestSource;
    protected EntityAI m_LastRequestDestination;
    protected EntityAI m_LastRequestItem;

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

    protected bool IsParticipantAvailable(EntityAI entity)
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

    EntityAI GetDestination()
    {
        if (IsParticipantAvailable(m_Destination))
            return m_Destination;

        m_Destination = null;
        return null;
    }

    void SetDestination(EntityAI destination)
    {
        if (IsParticipantAvailable(destination))
            m_Destination = destination;
    }

    bool IsDestination(EntityAI entity)
    {
        return entity && GetDestination() == entity;
    }

    protected void ClearAllLinks()
    {
        if (m_Links)
            m_Links.Clear();
    }

    void ToggleLink(EntityAI container)
    {
        if (!IsParticipantAvailable(container))
            return;

        if (m_LinkAnchor && !IsParticipantAvailable(m_LinkAnchor))
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
        if (!IsParticipantAvailable(source) || !IsParticipantAvailable(destination))
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
        if (m_LinkAnchor && !IsParticipantAvailable(m_LinkAnchor))
            m_LinkAnchor = null;
        return entity && m_LinkAnchor == entity;
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
            if (!current.GetInventory().GetCurrentInventoryLocation(location))
                return false;
            if (location.GetType() != InventoryLocationType.ATTACHMENT)
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

    protected bool IsDuplicateRequest(int operation, EntityAI source, EntityAI destination, EntityAI item)
    {
        int now = GetGame().GetTime();
        bool sameRequest = operation == m_LastRequestOperation && source == m_LastRequestSource && destination == m_LastRequestDestination && item == m_LastRequestItem;
        if (sameRequest && now - m_LastRequestTime >= 0 && now - m_LastRequestTime < REQUEST_DEBOUNCE_MS)
            return true;

        m_LastRequestTime = now;
        m_LastRequestOperation = operation;
        m_LastRequestSource = source;
        m_LastRequestDestination = destination;
        m_LastRequestItem = item;
        return false;
    }

    protected void ExecuteOfflineRequest(int operation, PlayerBase player, EntityAI source, EntityAI destination, EntityAI item)
    {
        if (operation == TransferZOperation.TRANSFER)
            TransferZServerService.Transfer(player, source, destination);
        else if (operation == TransferZOperation.UNPACK)
            TransferZServerService.Unpack(player, source, destination);
        else if (operation == TransferZOperation.MOVE_ITEM)
            TransferZServerService.MoveItem(player, item, destination);

        player.UpdateInventoryMenu();
    }

    protected void SendRequest(int operation, EntityAI source, EntityAI destination, EntityAI item)
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || !IsParticipantAvailable(destination))
            return;

        if (IsDuplicateRequest(operation, source, destination, item))
            return;

        if (!GetGame().IsMultiplayer())
        {
            ExecuteOfflineRequest(operation, player, source, destination, item);
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
        rpc.Send(player, TransferZRPC.REQUEST, true, player.GetIdentity());
    }

    bool RequestTransferTo(EntityAI source, EntityAI destination)
    {
        if (!source || !IsParticipantAvailable(destination) || source == destination)
            return false;
        SendRequest(TransferZOperation.TRANSFER, source, destination, null);
        return true;
    }

    bool RequestTransfer(EntityAI source)
    {
        return RequestTransferTo(source, GetDestination());
    }

    bool RequestUnpackTo(EntityAI source, EntityAI destination)
    {
        if (!source || !IsParticipantAvailable(destination) || source == destination)
            return false;
        SendRequest(TransferZOperation.UNPACK, source, destination, null);
        return true;
    }

    bool RequestUnpack(EntityAI source)
    {
        return RequestUnpackTo(source, GetDestination());
    }

    bool RequestMoveItem(EntityAI item, EntityAI destination)
    {
        if (!item || !IsParticipantAvailable(destination) || item == destination)
            return false;
        SendRequest(TransferZOperation.MOVE_ITEM, null, destination, item);
        return true;
    }

    protected bool IsLooseVicinityTransferCandidate(EntityAI item, EntityAI destination)
    {
        if (!item || item == destination)
            return false;
        if (item.GetInventory().GetCargo())
            return false;

        ItemBase itemBase = ItemBase.Cast(item);
        return itemBase && itemBase.IsTakeable() && item.GetInventory().CanRemoveEntity();
    }

    bool RequestVicinityTransferTo(notnull array<EntityAI> items, EntityAI destination)
    {
        if (!IsParticipantAvailable(destination))
            return false;

        bool requested = false;
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
            return false;

        if (!GetGame().IsMultiplayer())
        {
            foreach (EntityAI offlineItem : items)
            {
                if (!IsLooseVicinityTransferCandidate(offlineItem, destination))
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
            if (!IsLooseVicinityTransferCandidate(item, destination))
                continue;
            if (RequestMoveItem(item, destination))
                requested = true;
        }
        return requested;
    }

    bool RequestVicinityTransfer(notnull array<EntityAI> items)
    {
        return RequestVicinityTransferTo(items, GetDestination());
    }

    bool RequestVicinityUnpackTo(notnull array<EntityAI> items, EntityAI destination)
    {
        if (!IsParticipantAvailable(destination))
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

    bool RequestVicinityUnpack(notnull array<EntityAI> items)
    {
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
        if (!destination)
            return false;
        return RequestMoveItem(item, destination);
    }
}
