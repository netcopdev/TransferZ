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

        // Native DayZ split serialization expects the source item in the
        // destination location even though the server creates a new entity.
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

        // Preferred order for a split from cargo in/under the held container:
        // 1. keep the new stack in the exact source cargo;
        // 2. if source cargo is full, use P* when it resolves and has space;
        // 3. otherwise return false so vanilla DayZ handles the split normally.
        if (TransferZExecuteSplitTo(source, false))
            return true;

        EntityAI preferred = TransferZClientState.Get().GetPreferredDestination();
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
