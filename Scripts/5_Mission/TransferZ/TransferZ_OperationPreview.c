enum TransferZOperationPreviewResult
{
    IMPOSSIBLE = 0,
    PARTIAL = 1,
    READY = 2
}

class TransferZOperationPreview
{
    protected static bool IsDescendantOf(EntityAI entity, EntityAI ancestor)
    {
        if (!entity || !ancestor)
            return false;

        EntityAI current = entity;
        while (current)
        {
            if (current == ancestor)
                return true;
            current = current.GetHierarchyParent();
        }
        return false;
    }

    protected static bool CanReleaseFromCurrentLocation(EntityAI item)
    {
        if (!item || !item.GetInventory().CanRemoveEntity())
            return false;

        InventoryLocation location = new InventoryLocation();
        if (!item.GetInventory().GetCurrentInventoryLocation(location))
            return false;

        if (location.GetType() == InventoryLocationType.CARGO)
        {
            EntityAI parent = location.GetParent();
            if (parent && !parent.CanReleaseCargo(item))
                return false;
        }

        return true;
    }

    protected static void SnapshotDirectCargo(EntityAI source, int sourceCargoIndex, notnull array<EntityAI> items)
    {
        items.Clear();
        if (!source)
            return;

        CargoBase cargo = TransferZCargo.Get(source, sourceCargoIndex);
        if (!cargo)
            return;

        for (int i = 0; i < cargo.GetItemCount(); i++)
        {
            EntityAI item = cargo.GetItem(i);
            if (item)
                items.Insert(item);
        }
    }

    protected static void CollectUnpackLeaves(EntityAI container, EntityAI excludedDestination, notnull array<EntityAI> leaves)
    {
        if (!container)
            return;

        for (int cargoIndex = 0; ; cargoIndex++)
        {
            CargoBase cargo = TransferZCargo.Get(container, cargoIndex);
            if (!cargo)
                break;

            for (int i = 0; i < cargo.GetItemCount(); i++)
            {
                EntityAI item = cargo.GetItem(i);
                if (!item || item == excludedDestination)
                    continue;

                if (TransferZCargo.Exists(item, 0))
                    CollectUnpackLeaves(item, excludedDestination, leaves);
                else
                    leaves.Insert(item);
            }
        }
    }

    protected static void CollectUnpackLeavesForOperation(EntityAI source, int sourceCargoIndex, EntityAI destination, notnull array<EntityAI> leaves)
    {
        leaves.Clear();
        if (!source)
            return;

        CargoBase cargo = TransferZCargo.Get(source, sourceCargoIndex);
        if (!cargo)
            return;

        for (int i = 0; i < cargo.GetItemCount(); i++)
        {
            EntityAI child = cargo.GetItem(i);
            if (!child || child == destination || !TransferZCargo.Exists(child, 0))
                continue;

            CollectUnpackLeaves(child, destination, leaves);
        }
    }

    protected static int ItemArea(EntityAI item)
    {
        InventoryItem inventoryItem = InventoryItem.Cast(item);
        if (!inventoryItem)
            return 0;

        int width;
        int height;
        GetGame().GetInventoryItemSize(inventoryItem, width, height);
        if (width < 1)
            width = 1;
        if (height < 1)
            height = 1;
        return width * height;
    }

    protected static int FreeCargoArea(EntityAI destination, int destinationCargoIndex)
    {
        if (!destination)
            return 0;

        CargoBase cargo = TransferZCargo.Get(destination, destinationCargoIndex);
        if (!cargo)
            return 0;

        int total = cargo.GetWidth() * cargo.GetHeight();
        int used = 0;
        int itemWidth;
        int itemHeight;

        for (int i = 0; i < cargo.GetItemCount(); i++)
        {
            itemWidth = 0;
            itemHeight = 0;
            cargo.GetItemSize(i, itemWidth, itemHeight);
            used += itemWidth * itemHeight;
        }

        int freeArea = total - used;
        if (freeArea < 0)
            freeArea = 0;
        return freeArea;
    }

    protected static bool CanFitIndividually(EntityAI item, EntityAI destination, int destinationCargoIndex)
    {
        if (!item || !destination || item == destination)
            return false;
        if (!TransferZCargo.Exists(destination, destinationCargoIndex))
            return false;
        if (!destination.CanReceiveItemIntoCargo(item))
            return false;

        InventoryLocation freeLocation;
        return TransferZCargo.FindFreeLocation(destination, destinationCargoIndex, item, freeLocation) && TransferZCargo.LocationMatches(freeLocation, destination, destinationCargoIndex);
    }

    protected static int EvaluateCandidates(notnull array<EntityAI> candidates, EntityAI destination, int destinationCargoIndex, bool destinationIsVicinity)
    {
        if (candidates.Count() == 0)
            return TransferZOperationPreviewResult.IMPOSSIBLE;

        int releasable = 0;
        int individuallyFits = 0;
        int requiredArea = 0;

        foreach (EntityAI item : candidates)
        {
            if (!CanReleaseFromCurrentLocation(item))
                continue;

            releasable++;
            if (destinationIsVicinity)
            {
                individuallyFits++;
                continue;
            }

            if (!CanFitIndividually(item, destination, destinationCargoIndex))
                continue;

            individuallyFits++;
            requiredArea += ItemArea(item);
        }

        if (releasable == 0 || individuallyFits == 0)
            return TransferZOperationPreviewResult.IMPOSSIBLE;

        if (destinationIsVicinity)
        {
            if (releasable < candidates.Count())
                return TransferZOperationPreviewResult.PARTIAL;
            return TransferZOperationPreviewResult.READY;
        }

        if (individuallyFits < candidates.Count())
            return TransferZOperationPreviewResult.PARTIAL;

        int freeArea = FreeCargoArea(destination, destinationCargoIndex);
        if (requiredArea > freeArea)
            return TransferZOperationPreviewResult.PARTIAL;

        // Individual fits plus aggregate free area do not prove that several
        // items can be packed together without fragmentation. TransferZ's
        // server moves items authoritatively one-by-one, so only a single-item
        // cargo preview can honestly claim READY here. A multi-item batch is
        // advisory PARTIAL/uncertain until the server executes it.
        if (candidates.Count() > 1)
            return TransferZOperationPreviewResult.PARTIAL;

        return TransferZOperationPreviewResult.READY;
    }

    static int EvaluateContainerOperation(int operation, EntityAI source, int sourceCargoIndex = 0)
    {
        if (!source || !TransferZCargo.Exists(source, sourceCargoIndex))
            return TransferZOperationPreviewResult.IMPOSSIBLE;

        TransferZClientState state = TransferZClientState.Get();
        bool destinationIsVicinity = state.IsDestinationVicinity();
        EntityAI destination = null;
        int destinationCargoIndex = 0;
        if (!destinationIsVicinity)
        {
            destination = state.GetDestination();
            destinationCargoIndex = state.GetDestinationCargoIndex();
        }

        if (!destinationIsVicinity && !destination)
            return TransferZOperationPreviewResult.IMPOSSIBLE;

        ref array<EntityAI> candidates = new array<EntityAI>();

        if (operation == TransferZOperation.TRANSFER)
        {
            if (!destinationIsVicinity && source == destination && sourceCargoIndex == destinationCargoIndex)
                return TransferZOperationPreviewResult.IMPOSSIBLE;
            if (!destinationIsVicinity && destination != source && IsDescendantOf(destination, source))
                return TransferZOperationPreviewResult.IMPOSSIBLE;

            SnapshotDirectCargo(source, sourceCargoIndex, candidates);
        }
        else if (operation == TransferZOperation.UNPACK)
        {
            if (!destinationIsVicinity && destination != source && IsDescendantOf(destination, source))
                return TransferZOperationPreviewResult.IMPOSSIBLE;

            CollectUnpackLeavesForOperation(source, sourceCargoIndex, destination, candidates);
        }
        else
        {
            return TransferZOperationPreviewResult.IMPOSSIBLE;
        }

        return EvaluateCandidates(candidates, destination, destinationCargoIndex, destinationIsVicinity);
    }

    static int EvaluateVicinityOperation(int operation, notnull array<EntityAI> visibleItems)
    {
        TransferZClientState state = TransferZClientState.Get();
        bool destinationIsVicinity = state.IsDestinationVicinity();
        EntityAI destination = null;
        int destinationCargoIndex = 0;
        if (!destinationIsVicinity)
        {
            destination = state.GetDestination();
            destinationCargoIndex = state.GetDestinationCargoIndex();
        }

        if (!destinationIsVicinity && !destination)
            return TransferZOperationPreviewResult.IMPOSSIBLE;

        ref array<EntityAI> candidates = new array<EntityAI>();

        if (operation == TransferZOperation.TRANSFER)
        {
            if (destinationIsVicinity)
                return TransferZOperationPreviewResult.IMPOSSIBLE;

            foreach (EntityAI item : visibleItems)
            {
                if (!item || item == destination || TransferZCargo.Exists(item, 0))
                    continue;

                ItemBase itemBase = ItemBase.Cast(item);
                if (itemBase && itemBase.IsTakeable())
                    candidates.Insert(item);
            }
        }
        else if (operation == TransferZOperation.UNPACK)
        {
            foreach (EntityAI container : visibleItems)
            {
                if (!container || container == destination || !TransferZCargo.Exists(container, 0))
                    continue;
                CollectUnpackLeaves(container, destination, candidates);
            }
        }
        else
        {
            return TransferZOperationPreviewResult.IMPOSSIBLE;
        }

        return EvaluateCandidates(candidates, destination, destinationCargoIndex, destinationIsVicinity);
    }
}
