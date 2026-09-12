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

    protected bool TransferZFindSplitLocationInSource(EntityAI source, out InventoryLocation destination)
    {
        destination = new InventoryLocation();
        if (!source || !source.GetInventory().GetCargo())
            return false;

        if (!source.GetInventory().FindFirstFreeLocationForNewEntity(GetType(), FindInventoryLocationType.CARGO, destination))
            return false;
        if (!destination.IsValid() || destination.GetType() != InventoryLocationType.CARGO || destination.GetParent() != source)
            return false;

        // DayZ serializes the source item in a split destination location even
        // though the server creates a new entity there. Preserve that protocol
        // while using a new-entity probe so the source stack cannot satisfy its
        // own free-space search.
        destination.SetCargo(source, this, destination.GetIdx(), destination.GetRow(), destination.GetCol(), destination.GetFlip());
        return true;
    }

    protected bool TransferZSplitInHandsCargo()
    {
        if (!CanBeSplit() || GetDayZGame().IsLeftCtrlDown())
            return false;

        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || player.GetInventory().HasInventoryReservation(this, null))
            return false;

        EntityAI source;
        if (!TransferZResolveInHandsCargoSource(source))
            return false;

        InventoryLocation destination;
        if (!TransferZFindSplitLocationInSource(source, destination))
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

    override void OnRightClick()
    {
        if (TransferZSplitInHandsCargo())
            return;

        super.OnRightClick();
    }
}
