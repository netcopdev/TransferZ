class TransferZNestedUnpackService
{
    static bool CollectUnpackItemsFromSource(EntityAI source, int sourceCargoIndex, EntityAI destination, notnull array<EntityAI> items, TransferZUnpackScanBudget budget)
    {
        if (!source || !budget || budget.exceeded)
            return false;

        CargoBase cargo = TransferZCargo.Get(source, sourceCargoIndex);
        if (!cargo)
            return false;

        for (int i = 0; i < cargo.GetItemCount(); i++)
        {
            if (!TransferZServerService.ConsumeUnpackScanNode(budget))
                return false;

            EntityAI child = cargo.GetItem(i);
            if (!child || child == destination)
                continue;

            if (TransferZCargo.Exists(child, 0))
            {
                if (!TransferZServerService.CollectUnpackItems(child, destination, items, budget, 1))
                    return false;
            }

            if (!TransferZServerService.AppendUnpackItem(child, items, budget))
                return false;
        }

        return true;
    }

    protected static bool CargoIsEmpty(EntityAI container)
    {
        if (!container)
            return false;

        int cargoIndex = 0;
        while (true)
        {
            CargoBase cargo = TransferZCargo.Get(container, cargoIndex);
            if (!cargo)
                break;
            if (cargo.GetItemCount() > 0)
                return false;
            cargoIndex++;
        }
        return true;
    }

    protected static bool CanMoveFlattenedItem(EntityAI item)
    {
        if (!item)
            return false;
        if (!TransferZCargo.Exists(item, 0))
            return true;
        return CargoIsEmpty(item);
    }

    static int Unpack(PlayerBase player, EntityAI source, EntityAI destination, int sourceCargoIndex = 0, int destinationCargoIndex = 0)
    {
        if (!player || !source || !destination)
            return 0;

        if (!TransferZServerService.IsReachable(player, source) || !TransferZServerService.IsReachable(player, destination))
            return 0;

        if (!TransferZCargo.Exists(source, sourceCargoIndex) || !TransferZCargo.Exists(destination, destinationCargoIndex))
            return 0;

        if (destination != source && TransferZServerService.IsDescendantOf(destination, source))
            return 0;

        ref array<EntityAI> items = new array<EntityAI>();
        TransferZUnpackScanBudget budget = new TransferZUnpackScanBudget();
        if (!CollectUnpackItemsFromSource(source, sourceCargoIndex, destination, items, budget))
        {
            Print("[TransferZ] Nested unpack rejected: traversal budget exceeded scanned=" + budget.scannedNodes.ToString() + " items=" + items.Count().ToString());
            return 0;
        }

        int moved = 0;
        foreach (EntityAI item : items)
        {
            if (!CanMoveFlattenedItem(item))
                continue;
            if (TransferZServerService.TryMoveToExactCargo(player, item, destination, destinationCargoIndex))
                moved++;
        }
        return moved;
    }

    static int UnpackToVicinity(PlayerBase player, EntityAI source, int sourceCargoIndex = 0)
    {
        if (!player || !source || !TransferZServerService.IsReachable(player, source) || !TransferZCargo.Exists(source, sourceCargoIndex))
            return 0;

        ref array<EntityAI> items = new array<EntityAI>();
        TransferZUnpackScanBudget budget = new TransferZUnpackScanBudget();
        if (!CollectUnpackItemsFromSource(source, sourceCargoIndex, null, items, budget))
        {
            Print("[TransferZ] Nested unpack to vicinity rejected: traversal budget exceeded scanned=" + budget.scannedNodes.ToString() + " items=" + items.Count().ToString());
            return 0;
        }

        int moved = 0;
        foreach (EntityAI item : items)
        {
            if (!CanMoveFlattenedItem(item))
                continue;
            if (TransferZServerService.TryMoveToVicinity(player, item))
                moved++;
        }
        return moved;
    }

    static int UnpackMany(PlayerBase player, notnull array<EntityAI> sources, EntityAI destination, int destinationCargoIndex, bool destinationIsVicinity)
    {
        if (!player || sources.Count() < 1 || sources.Count() > TransferZServerService.MAX_BATCH_ITEMS)
            return 0;

        if (!destinationIsVicinity)
        {
            if (!destination || !TransferZServerService.IsReachable(player, destination) || !TransferZCargo.Exists(destination, destinationCargoIndex))
                return 0;
        }

        ref array<EntityAI> items = new array<EntityAI>();
        TransferZUnpackScanBudget budget = new TransferZUnpackScanBudget();

        foreach (EntityAI source : sources)
        {
            if (!source || !TransferZServerService.IsReachable(player, source) || !TransferZCargo.Exists(source, 0))
                continue;
            if (!destinationIsVicinity && destination != source && TransferZServerService.IsDescendantOf(destination, source))
                continue;

            EntityAI excludedDestination = destination;
            if (destinationIsVicinity)
                excludedDestination = null;

            if (!CollectUnpackItemsFromSource(source, 0, excludedDestination, items, budget))
            {
                Print("[TransferZ] Nested unpack batch rejected: traversal budget exceeded scanned=" + budget.scannedNodes.ToString() + " items=" + items.Count().ToString());
                return 0;
            }
        }

        int moved = 0;
        foreach (EntityAI item : items)
        {
            if (!CanMoveFlattenedItem(item))
                continue;

            if (destinationIsVicinity)
            {
                if (TransferZServerService.TryMoveToVicinity(player, item))
                    moved++;
            }
            else if (TransferZServerService.TryMoveToExactCargo(player, item, destination, destinationCargoIndex))
            {
                moved++;
            }
        }
        return moved;
    }

    static void HandleRequest(PlayerBase player, PlayerIdentity sender, ParamsReadContext ctx)
    {
        if (!player)
            return;

        if (GetGame().IsMultiplayer())
        {
            PlayerIdentity playerIdentity = player.GetIdentity();
            if (!sender || !playerIdentity)
            {
                Print("[TransferZ] Nested unpack RPC rejected: missing multiplayer identity");
                return;
            }

            if (sender.GetId() != playerIdentity.GetId())
            {
                Print("[TransferZ] Nested unpack RPC rejected: sender does not own player");
                return;
            }
        }

        int sourceLow;
        int sourceHigh;
        int sourceCargoIndex;
        int destinationLow;
        int destinationHigh;
        int destinationCargoIndex;
        bool destinationIsVicinity;

        if (!ctx.Read(sourceLow))
            return;
        if (!ctx.Read(sourceHigh))
            return;
        if (!ctx.Read(sourceCargoIndex))
            return;
        if (!ctx.Read(destinationLow))
            return;
        if (!ctx.Read(destinationHigh))
            return;
        if (!ctx.Read(destinationCargoIndex))
            return;
        if (!ctx.Read(destinationIsVicinity))
            return;

        if (sourceCargoIndex < 0 || destinationCargoIndex < 0 || !TransferZServerService.CanPlayerManipulate(player))
            return;

        if (!TransferZRequestGuard.AcceptStandard(player))
        {
            Print("[TransferZ] Nested unpack RPC throttled");
            return;
        }

        EntityAI source = TransferZServerService.ResolveEntity(sourceLow, sourceHigh);
        EntityAI destination = TransferZServerService.ResolveEntity(destinationLow, destinationHigh);

        if (destinationIsVicinity)
            UnpackToVicinity(player, source, sourceCargoIndex);
        else
            Unpack(player, source, destination, sourceCargoIndex, destinationCargoIndex);
    }
}
