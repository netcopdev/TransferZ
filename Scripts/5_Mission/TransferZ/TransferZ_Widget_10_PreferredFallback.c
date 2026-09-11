modded class TransferZClientState
{
    bool CanPreferredAcceptItem(EntityAI item, EntityAI destination = null)
    {
        if (!item)
            return false;

        if (!destination)
            destination = GetPreferredDestination();
        if (!destination || item == destination)
            return false;

        CargoBase cargo = destination.GetInventory().GetCargo();
        if (!cargo)
            return false;

        if (!destination.CanReceiveItemIntoCargo(item))
            return false;

        InventoryLocation dst = new InventoryLocation();
        if (!destination.GetInventory().FindFreeLocationFor(item, FindInventoryLocationType.CARGO, dst))
            return false;

        return dst.IsValid() && dst.GetType() == InventoryLocationType.CARGO && dst.GetParent() == destination;
    }

    override bool TryRouteVicinityDoubleClick(EntityAI item)
    {
        EntityAI destination = GetPreferredDestination();
        if (!destination)
            return false;

        // Preferred routing is opportunistic. If P* cannot accept this item,
        // return false so vanilla vicinity double-click can pick it normally.
        if (!CanPreferredAcceptItem(item, destination))
            return false;

        return RequestMoveItem(item, destination);
    }
}
