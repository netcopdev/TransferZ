class TransferZCargo
{
    static CargoBase Get(EntityAI owner, int cargoIndex = 0)
    {
        if (!owner || cargoIndex < 0)
            return null;

        CargoBase cargo = owner.GetInventory().GetCargoFromIndex(cargoIndex);
        if (!cargo || cargo.GetOwnerCargoIndex() != cargoIndex)
            return null;
        return cargo;
    }

    static bool Exists(EntityAI owner, int cargoIndex = 0)
    {
        return Get(owner, cargoIndex) != null;
    }

    static bool LocationMatches(InventoryLocation location, EntityAI owner, int cargoIndex)
    {
        return location && owner && location.GetType() == InventoryLocationType.CARGO && location.GetParent() == owner && location.GetIdx() == cargoIndex;
    }

    static bool FindFreeLocation(EntityAI owner, int cargoIndex, EntityAI item, out InventoryLocation destination)
    {
        destination = new InventoryLocation();
        if (!owner || !item)
            return false;

        CargoBase cargo = Get(owner, cargoIndex);
        if (!cargo)
            return false;

        int cargoWidth = cargo.GetWidth();
        int cargoHeight = cargo.GetHeight();
        if (cargoWidth <= 0 || cargoHeight <= 0)
            return false;

        bool currentFlip = item.GetInventory().GetFlipCargo();
        for (int orientationPass = 0; orientationPass < 2; orientationPass++)
        {
            bool flip = currentFlip;
            if (orientationPass == 1)
                flip = !currentFlip;

            for (int row = 0; row < cargoHeight; row++)
            {
                for (int col = 0; col < cargoWidth; col++)
                {
                    InventoryLocation candidate = new InventoryLocation();
                    candidate.SetCargo(owner, item, cargoIndex, row, col, flip);
                    if (!GameInventory.LocationCanAddEntity(candidate))
                        continue;

                    destination = candidate;
                    return true;
                }
            }
        }

        return false;
    }
}
