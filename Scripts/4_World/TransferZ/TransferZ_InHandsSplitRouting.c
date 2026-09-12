class TransferZSplitPreferences
{
    string preferred_slot = "";
    ref array<string> preferred_path;

    void TransferZSplitPreferences()
    {
        preferred_path = new array<string>();
    }
}

class TransferZSplitPreferenceResolver
{
    static const string PREFERENCES_PATH = "$profile:TransferZ/preferences.json";

    static EntityAI Resolve(PlayerBase player)
    {
        if (!player || !FileExist(PREFERENCES_PATH))
            return null;

        string errorMessage;
        ref TransferZSplitPreferences preferences = new TransferZSplitPreferences();
        if (!JsonFileLoader<TransferZSplitPreferences>.LoadFile(PREFERENCES_PATH, preferences, errorMessage) || !preferences)
            return null;

        if (preferences.preferred_path && preferences.preferred_path.Count() > 0)
        {
            EntityAI current = player;
            for (int i = 0; i < preferences.preferred_path.Count(); i++)
            {
                string slotName = preferences.preferred_path.Get(i);
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

        if (preferences.preferred_slot == "")
            return null;

        EntityAI legacyDestination = player.FindAttachmentBySlotName(preferences.preferred_slot);
        if (!legacyDestination || !legacyDestination.GetInventory().GetCargo())
            return null;
        return legacyDestination;
    }
}

modded class ItemBase
{
    protected bool TransferZResolveInHandsCargoSource(out EntityAI source)
    {
        source = null;

        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
            return false;

        EntityAI hands = player.GetEntityInHands();
        if (!hands || hands == this)
            return false;

        InventoryLocation currentLocation = new InventoryLocation();
        if (!GetInventory().GetCurrentInventoryLocation(currentLocation))
            return false;
        if (currentLocation.GetType() != InventoryLocationType.CARGO)
            return false;

        source = currentLocation.GetParent();
        if (!source || !source.GetInventory().GetCargo())
            return false;

        EntityAI current = source;
        while (current && current != player)
        {
            if (current == hands)
                return true;
            current = current.GetHierarchyParent();
        }

        source = null;
        return false;
    }

    protected bool TransferZFindSplitLocation(EntityAI destinationEntity, bool verifyReceive, out InventoryLocation destination)
    {
        destination = new InventoryLocation();
        if (!destinationEntity || !destinationEntity.GetInventory().GetCargo())
            return false;
        if (verifyReceive && !destinationEntity.CanReceiveItemIntoCargo(this))
            return false;

        if (!destinationEntity.GetInventory().FindFirstFreeLocationForNewEntity(GetType(), FindInventoryLocationType.CARGO, destination))
            return false;
        if (!destination.IsValid() || destination.GetType() != InventoryLocationType.CARGO || destination.GetParent() != destinationEntity)
            return false;

        destination.SetCargo(destinationEntity, this, destination.GetIdx(), destination.GetRow(), destination.GetCol(), destination.GetFlip());
        return true;
    }

    protected bool TransferZExecuteSplitTo(EntityAI destinationEntity, bool verifyReceive)
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
            return false;

        InventoryLocation destination;
        if (!TransferZFindSplitLocation(destinationEntity, verifyReceive, destination))
            return false;

        if (g_Game.IsClient())
        {
            if (!ScriptInputUserData.CanStoreInputUserData())
                return false;
            if (player.GetInventory().HasInventoryReservation(this, destination))
                return false;

            player.GetInventory().AddInventoryReservationEx(null, destination, GameInventory.c_InventoryReservationTimeoutShortMS);

            ScriptInputUserData ctx = new ScriptInputUserData();
            ctx.Write(INPUT_UDT_ITEM_MANIPULATION);
            ctx.Write(4);
            ItemBase splitSource = this;
            ctx.Write(splitSource);
            destination.WriteToContext(ctx);
            ctx.Write(true);
            ctx.Send();
            return true;
        }

        if (!g_Game.IsMultiplayer())
        {
            SplitItemToInventoryLocation(destination);
            return true;
        }

        return false;
    }

    protected bool TransferZSplitFromInHandsCargo()
    {
        if (!CanBeSplit() || GetDayZGame().IsLeftCtrlDown())
            return false;

        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || player.GetInventory().HasInventoryReservation(this, null))
            return false;

        EntityAI source;
        if (!TransferZResolveInHandsCargoSource(source))
            return false;

        // Exact source cargo -> P* -> vanilla DayZ fallback.
        if (TransferZExecuteSplitTo(source, false))
            return true;

        EntityAI preferred = TransferZSplitPreferenceResolver.Resolve(player);
        if (preferred && preferred != source && TransferZExecuteSplitTo(preferred, true))
            return true;

        return false;
    }

    override void OnRightClick()
    {
        if (TransferZSplitFromInHandsCargo())
            return;

        super.OnRightClick();
    }
}
