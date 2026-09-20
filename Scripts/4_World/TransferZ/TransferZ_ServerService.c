class TransferZUnpackScanBudget
{
    int scannedNodes;
    bool exceeded;
}

class TransferZServerService
{
    static const int MAX_BATCH_ITEMS = 512;
    static const int MAX_UNPACK_SCAN_NODES = 2048;
    static const int MAX_UNPACK_DEPTH = 32;

    static EntityAI ResolveEntity(int low, int high)
    {
        Object obj = GetGame().GetObjectByNetworkId(low, high);
        return EntityAI.Cast(obj);
    }

    static bool IsDescendantOf(EntityAI entity, EntityAI ancestor)
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

    static bool CanPlayerManipulate(PlayerBase player)
    {
        if (!player || !player.IsAlive())
            return false;
        if (player.IsUnconscious() || player.IsRestrained())
            return false;
        if (player.GetInventory().IsInventoryLocked())
            return false;
        return true;
    }

    // DayZ owns inventory contention. TransferZ never acquires a parallel lock;
    // it simply refuses to mutate an item that is already covered by a native
    // inventory juncture.
    static bool HasNativeInventoryJuncture(EntityAI item)
    {
        return item && GetGame().HasInventoryJunctureItem(item);
    }

    // Validate every cargo hop, not only the direct source/destination. This
    // prevents an RPC from reaching through closed/hidden nested containers.
    static bool IsCargoChainAccessible(EntityAI entity)
    {
        if (!entity)
            return false;

        InventoryLocation hop = new InventoryLocation();
        EntityAI child = entity;
        EntityAI parent = entity.GetHierarchyParent();
        while (parent)
        {
            if (!child.GetInventory().GetCurrentInventoryLocation(hop))
                return false;

            int hopType = hop.GetType();
            if (hopType == InventoryLocationType.CARGO || hopType == InventoryLocationType.PROXYCARGO)
            {
                if (!parent.CanDisplayCargo() || !parent.CanReleaseCargo(child))
                    return false;
            }

            child = parent;
            parent = parent.GetHierarchyParent();
        }
        return true;
    }

    static bool IsReachable(PlayerBase player, EntityAI entity)
    {
        if (!player || !entity)
            return false;

        if (!IsCargoChainAccessible(entity))
            return false;

        EntityAI root = entity.GetHierarchyRoot();
        if (!root)
            root = entity;

        if (root == player)
            return true;

        if (root.IsMan())
            return false;

        // Vehicle inventory access is not represented by distance to the model
        // origin. Large vehicles can expose valid cargo several metres from that
        // origin, so accept displayable vehicle cargo at this coarse layer. Every
        // mutating move is still checked by DayZ's native request validators.
        if (root.IsInherited(Transport))
            return root.CanDisplayCargo();

        return GameInventory.CheckManipulatedObjectsDistances(entity, player, GameInventory.c_MaxItemDistanceRadius);
    }

    static bool MoveFailure(string reason, EntityAI item, EntityAI destination)
    {
        string itemName = "<null>";
        string destinationName = "<null>";
        if (item)
            itemName = item.GetType();
        if (destination)
            destinationName = destination.GetType();

        Print("[TransferZ] Move rejected: " + reason + " item=" + itemName + " destination=" + destinationName);
        return false;
    }

    static bool DirectBatchWithinBudget(EntityAI source, int sourceCargoIndex, string operation)
    {
        CargoBase cargo = TransferZCargo.Get(source, sourceCargoIndex);
        if (!cargo)
            return false;

        int itemCount = cargo.GetItemCount();
        if (itemCount <= MAX_BATCH_ITEMS)
            return true;

        Print("[TransferZ] " + operation + " rejected: direct cargo item count " + itemCount.ToString() + " exceeds limit " + MAX_BATCH_ITEMS.ToString());
        return false;
    }

    static bool ConsumeUnpackScanNode(TransferZUnpackScanBudget budget)
    {
        if (!budget || budget.exceeded)
            return false;

        if (budget.scannedNodes >= MAX_UNPACK_SCAN_NODES)
        {
            budget.exceeded = true;
            return false;
        }

        budget.scannedNodes++;
        return true;
    }

    static bool AppendUnpackLeaf(EntityAI item, notnull array<EntityAI> leaves, TransferZUnpackScanBudget budget)
    {
        if (!item || !budget || budget.exceeded)
            return false;

        if (leaves.Count() >= MAX_BATCH_ITEMS)
        {
            budget.exceeded = true;
            return false;
        }

        leaves.Insert(item);
        return true;
    }


    static bool TryMoveToExactCargo(PlayerBase player, EntityAI item, EntityAI destination, int destinationCargoIndex = 0)
    {
        if (!player || !item || !destination || item == destination)
            return MoveFailure("invalid arguments", item, destination);

        if (!IsReachable(player, item))
            return MoveFailure("item not reachable", item, destination);

        if (!IsReachable(player, destination))
            return MoveFailure("destination not reachable", item, destination);

        CargoBase destinationCargo = TransferZCargo.Get(destination, destinationCargoIndex);
        if (!destinationCargo)
            return MoveFailure("destination cargo grid unavailable", item, destination);
        if (!destination.CanDisplayCargo())
            return MoveFailure("destination cargo is not accessible", item, destination);

        if (!item.GetInventory().CanRemoveEntity())
            return MoveFailure("item cannot be removed", item, destination);

        InventoryLocation src = new InventoryLocation();
        if (!item.GetInventory().GetCurrentInventoryLocation(src))
            return MoveFailure("source location unavailable", item, destination);

        // A client-predicted representative drag can reach the requested cargo
        // before its TransferZ batch RPC is processed. Treat that exact grid as
        // already completed, but never collapse two cargo grids on one owner.
        if (TransferZCargo.LocationMatches(src, destination, destinationCargoIndex))
            return true;

        int srcType = src.GetType();
        if (srcType != InventoryLocationType.CARGO && srcType != InventoryLocationType.GROUND)
            return MoveFailure("source is not cargo or ground", item, destination);

        EntityAI sourceParent = src.GetParent();
        if (sourceParent && srcType == InventoryLocationType.CARGO && !sourceParent.CanReleaseCargo(item))
            return MoveFailure("source cargo refuses release", item, destination);

        if (!destination.CanReceiveItemIntoCargo(item))
            return MoveFailure("destination refuses cargo item", item, destination);

        InventoryLocation dst;
        if (!TransferZCargo.FindFreeLocation(destination, destinationCargoIndex, item, dst))
            return false;
        if (!TransferZCargo.LocationMatches(dst, destination, destinationCargoIndex))
            return false;

        if (!GameInventory.CheckMoveToDstRequest(player, src, dst, GameInventory.c_MaxItemDistanceRadius))
            return MoveFailure("native move request validation failed", item, destination);

        if (!GameInventory.LocationCanMoveEntity(src, dst))
            return MoveFailure("native location move validation failed", item, destination);

        if (HasNativeInventoryJuncture(item))
            return MoveFailure("native inventory juncture active", item, destination);

        InventoryMode moveMode = InventoryMode.SERVER;
        if (!GetGame().IsMultiplayer())
            moveMode = InventoryMode.LOCAL;

        if (!item.GetInventory().TakeToDst(moveMode, src, dst))
            return MoveFailure("TakeToDst failed", item, destination);

        return true;
    }

    static bool TryMoveToVicinity(PlayerBase player, EntityAI item)
    {
        if (!player || !item)
            return MoveFailure("invalid vicinity arguments", item, null);

        if (!IsReachable(player, item))
            return MoveFailure("item not reachable", item, null);

        if (!item.GetInventory().CanRemoveEntity())
            return MoveFailure("item cannot be removed", item, null);

        InventoryLocation src = new InventoryLocation();
        if (!item.GetInventory().GetCurrentInventoryLocation(src))
            return MoveFailure("source location unavailable", item, null);

        // Transfer-to-vicinity is defined only for cargo children. Ground is
        // already vicinity; hands/attachments remain vanilla-owned.
        if (src.GetType() != InventoryLocationType.CARGO)
            return MoveFailure("vicinity move requires a cargo source", item, null);

        EntityAI sourceParent = src.GetParent();
        if (sourceParent && src.GetType() == InventoryLocationType.CARGO && !sourceParent.CanReleaseCargo(item))
            return MoveFailure("source cargo refuses release", item, null);

        // Vehicle reachability above deliberately avoids distance to the model
        // origin. Validate the exact source location before any server-authored
        // drop so remote/inaccessible cargo cannot be manipulated.
        if (!GameInventory.CheckDropRequest(player, src, GameInventory.c_MaxItemDistanceRadius))
            return MoveFailure("CheckDropRequest failed", item, null);

        if (HasNativeInventoryJuncture(item))
            return MoveFailure("native inventory juncture active", item, null);

        InventoryMode moveMode = InventoryMode.SERVER;
        if (!GetGame().IsMultiplayer())
            moveMode = InventoryMode.LOCAL;

        // Drop relative to the requesting player so VICINITY consistently means
        // the accessible ground area around that player, irrespective of where
        // the source cargo sits in the UI hierarchy. Use the item's inventory
        // for the same immediate authoritative semantics as exact-cargo moves.
        if (!item.GetInventory().DropEntity(moveMode, player, item))
            return MoveFailure("DropEntity failed", item, null);

        return true;
    }

    static void SnapshotDirectCargo(EntityAI source, int sourceCargoIndex, notnull array<EntityAI> items)
    {
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

    static bool CollectUnpackLeaves(EntityAI container, EntityAI destination, notnull array<EntityAI> leaves, TransferZUnpackScanBudget budget, int depth)
    {
        if (!container || !budget || budget.exceeded)
            return false;

        if (depth > MAX_UNPACK_DEPTH)
        {
            budget.exceeded = true;
            return false;
        }

        for (int cargoIndex = 0; ; cargoIndex++)
        {
            CargoBase cargo = TransferZCargo.Get(container, cargoIndex);
            if (!cargo)
                break;

            for (int i = 0; i < cargo.GetItemCount(); i++)
            {
                if (!ConsumeUnpackScanNode(budget))
                    return false;

                EntityAI item = cargo.GetItem(i);
                if (!item || item == destination)
                    continue;

                if (TransferZCargo.Exists(item, 0))
                {
                    if (!CollectUnpackLeaves(item, destination, leaves, budget, depth + 1))
                        return false;
                }
                else if (!AppendUnpackLeaf(item, leaves, budget))
                {
                    return false;
                }
            }
        }

        return true;
    }

    static bool CollectUnpackLeavesForOperation(EntityAI source, int sourceCargoIndex, EntityAI destination, bool nestedOnly, notnull array<EntityAI> leaves, TransferZUnpackScanBudget budget)
    {
        if (!source || !budget || budget.exceeded)
            return false;

        CargoBase sourceCargo = TransferZCargo.Get(source, sourceCargoIndex);
        if (!sourceCargo)
            return false;

        for (int i = 0; i < sourceCargo.GetItemCount(); i++)
        {
            if (!ConsumeUnpackScanNode(budget))
                return false;

            EntityAI child = sourceCargo.GetItem(i);
            if (!child || child == destination)
                continue;

            if (TransferZCargo.Exists(child, 0))
            {
                if (!CollectUnpackLeaves(child, destination, leaves, budget, 1))
                    return false;
            }
            else if (!nestedOnly && !AppendUnpackLeaf(child, leaves, budget))
            {
                return false;
            }
        }

        return true;
    }

    static int Transfer(PlayerBase player, EntityAI source, EntityAI destination, int sourceCargoIndex = 0, int destinationCargoIndex = 0)
    {
        if (!player || !source || !destination)
            return 0;
        if (source == destination && sourceCargoIndex == destinationCargoIndex)
            return 0;

        if (!IsReachable(player, source) || !IsReachable(player, destination))
            return 0;

        if (!TransferZCargo.Exists(source, sourceCargoIndex) || !TransferZCargo.Exists(destination, destinationCargoIndex))
            return 0;

        if (!DirectBatchWithinBudget(source, sourceCargoIndex, "Transfer"))
            return 0;

        if (destination != source && IsDescendantOf(destination, source))
            return 0;

        ref array<EntityAI> items = new array<EntityAI>();
        SnapshotDirectCargo(source, sourceCargoIndex, items);

        int moved = 0;
        foreach (EntityAI item : items)
        {
            if (TryMoveToExactCargo(player, item, destination, destinationCargoIndex))
                moved++;
        }
        return moved;
    }

    static int TransferToVicinity(PlayerBase player, EntityAI source, int sourceCargoIndex = 0)
    {
        if (!player || !source || !IsReachable(player, source) || !TransferZCargo.Exists(source, sourceCargoIndex))
            return 0;

        if (!DirectBatchWithinBudget(source, sourceCargoIndex, "TransferToVicinity"))
            return 0;

        ref array<EntityAI> items = new array<EntityAI>();
        SnapshotDirectCargo(source, sourceCargoIndex, items);

        int moved = 0;
        foreach (EntityAI item : items)
        {
            if (TryMoveToVicinity(player, item))
                moved++;
        }
        return moved;
    }

    static int TransferClass(PlayerBase player, EntityAI source, EntityAI destination, EntityAI representative, int sourceCargoIndex = 0, int destinationCargoIndex = 0)
    {
        if (!player || !source || !destination || !representative)
            return 0;
        if (source == destination && sourceCargoIndex == destinationCargoIndex)
            return 0;

        if (!IsReachable(player, source) || !IsReachable(player, destination) || !IsReachable(player, representative))
            return 0;

        if (!TransferZCargo.Exists(source, sourceCargoIndex) || !TransferZCargo.Exists(destination, destinationCargoIndex))
            return 0;

        if (!DirectBatchWithinBudget(source, sourceCargoIndex, "TransferClass"))
            return 0;

        if (destination != source && IsDescendantOf(destination, source))
            return 0;

        InventoryLocation representativeLocation = new InventoryLocation();
        if (!representative.GetInventory().GetCurrentInventoryLocation(representativeLocation))
            return 0;

        bool representativeInSource = TransferZCargo.LocationMatches(representativeLocation, source, sourceCargoIndex);
        bool representativeAtDestination = TransferZCargo.LocationMatches(representativeLocation, destination, destinationCargoIndex);
        if (!representativeInSource && !representativeAtDestination)
            return 0;

        string className = representative.GetType();
        if (className == "")
            return 0;

        ref array<EntityAI> items = new array<EntityAI>();
        SnapshotDirectCargo(source, sourceCargoIndex, items);

        int moved = 0;
        foreach (EntityAI item : items)
        {
            if (!item || item.GetType() != className)
                continue;

            if (TryMoveToExactCargo(player, item, destination, destinationCargoIndex))
                moved++;
        }
        return moved;
    }

    static int TransferClassToVicinity(PlayerBase player, EntityAI source, EntityAI representative, int sourceCargoIndex = 0)
    {
        if (!player || !source || !representative)
            return 0;

        if (!IsReachable(player, source) || !IsReachable(player, representative) || !TransferZCargo.Exists(source, sourceCargoIndex))
            return 0;

        if (!DirectBatchWithinBudget(source, sourceCargoIndex, "TransferClassToVicinity"))
            return 0;

        InventoryLocation representativeLocation = new InventoryLocation();
        if (!representative.GetInventory().GetCurrentInventoryLocation(representativeLocation))
            return 0;

        bool representativeInSource = TransferZCargo.LocationMatches(representativeLocation, source, sourceCargoIndex);
        bool representativeAlreadyGround = representativeLocation.GetType() == InventoryLocationType.GROUND;
        if (!representativeInSource && !representativeAlreadyGround)
            return 0;

        string className = representative.GetType();
        if (className == "")
            return 0;

        ref array<EntityAI> items = new array<EntityAI>();
        SnapshotDirectCargo(source, sourceCargoIndex, items);

        int moved = 0;
        foreach (EntityAI item : items)
        {
            if (!item || item.GetType() != className)
                continue;

            if (TryMoveToVicinity(player, item))
                moved++;
        }
        return moved;
    }

    static int Unpack(PlayerBase player, EntityAI source, EntityAI destination, int sourceCargoIndex = 0, int destinationCargoIndex = 0)
    {
        if (!player || !source || !destination)
            return 0;

        if (!IsReachable(player, source) || !IsReachable(player, destination))
            return 0;

        if (!TransferZCargo.Exists(source, sourceCargoIndex) || !TransferZCargo.Exists(destination, destinationCargoIndex))
            return 0;

        if (destination != source && IsDescendantOf(destination, source))
            return 0;

        ref array<EntityAI> leaves = new array<EntityAI>();
        TransferZUnpackScanBudget budget = new TransferZUnpackScanBudget();
        if (!CollectUnpackLeavesForOperation(source, sourceCargoIndex, destination, source == destination && sourceCargoIndex == destinationCargoIndex, leaves, budget))
        {
            Print("[TransferZ] Unpack rejected: traversal budget exceeded scanned=" + budget.scannedNodes.ToString() + " leaves=" + leaves.Count().ToString());
            return 0;
        }

        int moved = 0;
        foreach (EntityAI item : leaves)
        {
            if (TryMoveToExactCargo(player, item, destination, destinationCargoIndex))
                moved++;
        }
        return moved;
    }

    static int UnpackToVicinity(PlayerBase player, EntityAI source, int sourceCargoIndex = 0)
    {
        if (!player || !source || !IsReachable(player, source) || !TransferZCargo.Exists(source, sourceCargoIndex))
            return 0;

        ref array<EntityAI> leaves = new array<EntityAI>();
        TransferZUnpackScanBudget budget = new TransferZUnpackScanBudget();
        if (!CollectUnpackLeavesForOperation(source, sourceCargoIndex, null, false, leaves, budget))
        {
            Print("[TransferZ] UnpackToVicinity rejected: traversal budget exceeded scanned=" + budget.scannedNodes.ToString() + " leaves=" + leaves.Count().ToString());
            return 0;
        }

        int moved = 0;
        foreach (EntityAI item : leaves)
        {
            if (TryMoveToVicinity(player, item))
                moved++;
        }
        return moved;
    }

    static bool MoveItem(PlayerBase player, EntityAI item, EntityAI destination, int destinationCargoIndex = 0)
    {
        if (!player || !item || !destination)
            return false;

        if (IsDescendantOf(destination, item))
            return false;

        return TryMoveToExactCargo(player, item, destination, destinationCargoIndex);
    }

    static bool MoveItemToVicinity(PlayerBase player, EntityAI item)
    {
        if (!player || !item)
            return false;

        bool moved = TryMoveToVicinity(player, item);
        string movedText = "false";
        if (moved)
            movedText = "true";
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
                Print("[TransferZ] RPC rejected: missing multiplayer identity");
                return;
            }

            if (sender.GetId() != playerIdentity.GetId())
            {
                Print("[TransferZ] RPC rejected: sender does not own player");
                return;
            }
        }

        int operation;
        int sourceLow;
        int sourceHigh;
        int sourceCargoIndex;
        int destinationLow;
        int destinationHigh;
        int destinationCargoIndex;
        int itemLow;
        int itemHigh;
        bool destinationIsVicinity;

        if (!ctx.Read(operation))
            return;
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
        if (!ctx.Read(itemLow))
            return;
        if (!ctx.Read(itemHigh))
            return;
        if (!ctx.Read(destinationIsVicinity))
            return;

        if (sourceCargoIndex < 0 || destinationCargoIndex < 0)
            return;

        EntityAI source = ResolveEntity(sourceLow, sourceHigh);
        EntityAI destination = ResolveEntity(destinationLow, destinationHigh);
        EntityAI item = ResolveEntity(itemLow, itemHigh);

        if (!CanPlayerManipulate(player))
            return;

        if (!TransferZRequestGuard.AcceptStandard(player))
        {
            Print("[TransferZ] RPC throttled");
            return;
        }

        if (destinationIsVicinity)
        {
            if (operation == TransferZOperation.TRANSFER)
                TransferToVicinity(player, source, sourceCargoIndex);
            else if (operation == TransferZOperation.UNPACK)
                UnpackToVicinity(player, source, sourceCargoIndex);
            else if (operation == TransferZOperation.MOVE_ITEM)
                MoveItemToVicinity(player, item);
            else if (operation == TransferZOperation.TRANSFER_CLASS)
                TransferClassToVicinity(player, source, item, sourceCargoIndex);
            return;
        }

        if (operation == TransferZOperation.TRANSFER)
            Transfer(player, source, destination, sourceCargoIndex, destinationCargoIndex);
        else if (operation == TransferZOperation.UNPACK)
            Unpack(player, source, destination, sourceCargoIndex, destinationCargoIndex);
        else if (operation == TransferZOperation.MOVE_ITEM)
            MoveItem(player, item, destination, destinationCargoIndex);
        else if (operation == TransferZOperation.TRANSFER_CLASS)
            TransferClass(player, source, destination, item, sourceCargoIndex, destinationCargoIndex);
    }
}
