class TransferZServerService
{
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

    static bool TryMoveToExactCargo(PlayerBase player, EntityAI item, EntityAI destination)
    {
        if (!player || !item || !destination || item == destination)
            return MoveFailure("invalid arguments", item, destination);

        if (!IsReachable(player, item))
            return MoveFailure("item not reachable", item, destination);

        if (!IsReachable(player, destination))
            return MoveFailure("destination not reachable", item, destination);

        CargoBase destinationCargo = destination.GetInventory().GetCargo();
        if (!destinationCargo)
            return MoveFailure("destination has no cargo", item, destination);
        if (!destination.CanDisplayCargo())
            return MoveFailure("destination cargo is not accessible", item, destination);

        if (!item.GetInventory().CanRemoveEntity())
            return MoveFailure("item cannot be removed", item, destination);

        InventoryLocation src = new InventoryLocation();
        if (!item.GetInventory().GetCurrentInventoryLocation(src))
            return MoveFailure("source location unavailable", item, destination);

        // A client-predicted representative drag can reach the requested cargo
        // before its TransferZ batch RPC is processed. Treat that as an already
        // completed member instead of relocating it inside the same cargo.
        if (src.GetType() == InventoryLocationType.CARGO && src.GetParent() == destination)
            return true;

        // TransferZ routes cargo children and loose vicinity items only. Hands
        // and attachment sources remain under vanilla state-machine handling.
        int srcType = src.GetType();
        if (srcType != InventoryLocationType.CARGO && srcType != InventoryLocationType.GROUND)
            return MoveFailure("source is not cargo or ground", item, destination);

        EntityAI sourceParent = src.GetParent();
        if (sourceParent && src.GetType() == InventoryLocationType.CARGO && !sourceParent.CanReleaseCargo(item))
            return MoveFailure("source cargo refuses release", item, destination);

        if (!destination.CanReceiveItemIntoCargo(item))
            return MoveFailure("destination refuses cargo item", item, destination);

        InventoryLocation dst = new InventoryLocation();
        // Running out of suitable cells is an expected batch-transfer outcome:
        // move everything that fits, then stop accepting individual members.
        // Do not flood the server log for normal capacity misses.
        if (!destination.GetInventory().FindFreeLocationFor(item, FindInventoryLocationType.CARGO, dst))
            return false;

        // Modded cargo implementations can occasionally decline to resolve an
        // exact cargo location even after reporting a candidate. Treat that the
        // same as an ordinary no-fit result; native validation still guards every
        // location that does reach the mutation path below.
        if (!dst.IsValid() || dst.GetType() != InventoryLocationType.CARGO || dst.GetParent() != destination)
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

        // Do not route server-authored batch moves through DayZPlayerInventory.
        // Its SERVER path queues a sync juncture for the remote player and does
        // not update the authoritative location immediately. A TransferZ batch
        // then plans every following item against stale cargo state and usually
        // only the first move survives. The item's GameInventory SERVER path
        // performs LocationSyncMoveEntity immediately and emits the server move
        // to clients, so each next batch step sees the committed state.
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

    static void SnapshotDirectCargo(EntityAI source, notnull array<EntityAI> items)
    {
        CargoBase cargo = source.GetInventory().GetCargo();
        if (!cargo)
            return;

        for (int i = 0; i < cargo.GetItemCount(); i++)
        {
            EntityAI item = cargo.GetItem(i);
            if (item)
                items.Insert(item);
        }
    }

    static void CollectUnpackLeaves(EntityAI container, EntityAI destination, notnull array<EntityAI> leaves)
    {
        if (!container)
            return;

        CargoBase cargo = container.GetInventory().GetCargo();
        if (!cargo)
            return;

        for (int i = 0; i < cargo.GetItemCount(); i++)
        {
            EntityAI item = cargo.GetItem(i);
            if (!item || item == destination)
                continue;

            CargoBase nestedCargo = item.GetInventory().GetCargo();
            if (nestedCargo)
                CollectUnpackLeaves(item, destination, leaves);
            else
                leaves.Insert(item);
        }
    }

    static void CollectUnpackLeavesForOperation(EntityAI source, EntityAI destination, notnull array<EntityAI> leaves)
    {
        if (!source)
            return;

        if (source != destination)
        {
            CollectUnpackLeaves(source, destination, leaves);
            return;
        }

        CargoBase sourceCargo = source.GetInventory().GetCargo();
        if (!sourceCargo)
            return;

        // Self-unpack flattens nested cargo into the source. Direct loose items
        // are already in the requested destination and must not be moved.
        for (int i = 0; i < sourceCargo.GetItemCount(); i++)
        {
            EntityAI child = sourceCargo.GetItem(i);
            if (child && child.GetInventory().GetCargo())
                CollectUnpackLeaves(child, source, leaves);
        }
    }

    static int Transfer(PlayerBase player, EntityAI source, EntityAI destination)
    {
        if (!player || !source || !destination || source == destination)
            return 0;

        if (!IsReachable(player, source) || !IsReachable(player, destination))
            return 0;

        if (!source.GetInventory().GetCargo() || !destination.GetInventory().GetCargo())
            return 0;

        if (IsDescendantOf(destination, source))
            return 0;

        ref array<EntityAI> items = new array<EntityAI>();
        SnapshotDirectCargo(source, items);

        int moved = 0;
        foreach (EntityAI item : items)
        {
            if (TryMoveToExactCargo(player, item, destination))
                moved++;
        }
        return moved;
    }

    static int TransferToVicinity(PlayerBase player, EntityAI source)
    {
        if (!player || !source || !IsReachable(player, source) || !source.GetInventory().GetCargo())
            return 0;

        ref array<EntityAI> items = new array<EntityAI>();
        SnapshotDirectCargo(source, items);

        int moved = 0;
        foreach (EntityAI item : items)
        {
            if (TryMoveToVicinity(player, item))
                moved++;
        }
        return moved;
    }

    static int TransferClass(PlayerBase player, EntityAI source, EntityAI destination, EntityAI representative)
    {
        if (!player || !source || !destination || !representative || source == destination)
            return 0;

        if (!IsReachable(player, source) || !IsReachable(player, destination) || !IsReachable(player, representative))
            return 0;

        if (!source.GetInventory().GetCargo() || !destination.GetInventory().GetCargo())
            return 0;

        if (IsDescendantOf(destination, source))
            return 0;

        InventoryLocation representativeLocation = new InventoryLocation();
        if (!representative.GetInventory().GetCurrentInventoryLocation(representativeLocation))
            return 0;

        bool representativeInSource = representativeLocation.GetType() == InventoryLocationType.CARGO && representativeLocation.GetParent() == source;
        bool representativeAtDestination = representativeLocation.GetType() == InventoryLocationType.CARGO && representativeLocation.GetParent() == destination;
        if (!representativeInSource && !representativeAtDestination)
            return 0;

        string className = representative.GetType();
        if (className == "")
            return 0;

        ref array<EntityAI> items = new array<EntityAI>();
        SnapshotDirectCargo(source, items);

        int matched = 0;
        int moved = 0;
        foreach (EntityAI item : items)
        {
            if (!item || item.GetType() != className)
                continue;

            matched++;
            if (TryMoveToExactCargo(player, item, destination))
                moved++;
        }
        return moved;
    }

    static int TransferClassToVicinity(PlayerBase player, EntityAI source, EntityAI representative)
    {
        if (!player || !source || !representative)
            return 0;

        if (!IsReachable(player, source) || !IsReachable(player, representative) || !source.GetInventory().GetCargo())
            return 0;

        InventoryLocation representativeLocation = new InventoryLocation();
        if (!representative.GetInventory().GetCurrentInventoryLocation(representativeLocation))
            return 0;

        bool representativeInSource = representativeLocation.GetType() == InventoryLocationType.CARGO && representativeLocation.GetParent() == source;
        bool representativeAlreadyGround = representativeLocation.GetType() == InventoryLocationType.GROUND;
        if (!representativeInSource && !representativeAlreadyGround)
            return 0;

        string className = representative.GetType();
        if (className == "")
            return 0;

        ref array<EntityAI> items = new array<EntityAI>();
        SnapshotDirectCargo(source, items);

        int matched = 0;
        int moved = 0;
        foreach (EntityAI item : items)
        {
            if (!item || item.GetType() != className)
                continue;

            matched++;
            if (TryMoveToVicinity(player, item))
                moved++;
        }
        return moved;
    }

    static int Unpack(PlayerBase player, EntityAI source, EntityAI destination)
    {
        if (!player || !source || !destination)
            return 0;

        if (!IsReachable(player, source) || !IsReachable(player, destination))
            return 0;

        if (!source.GetInventory().GetCargo() || !destination.GetInventory().GetCargo())
            return 0;

        if (destination != source && IsDescendantOf(destination, source))
            return 0;

        ref array<EntityAI> leaves = new array<EntityAI>();
        CollectUnpackLeavesForOperation(source, destination, leaves);

        int moved = 0;
        foreach (EntityAI item : leaves)
        {
            if (TryMoveToExactCargo(player, item, destination))
                moved++;
        }
        return moved;
    }

    static int UnpackToVicinity(PlayerBase player, EntityAI source)
    {
        if (!player || !source || !IsReachable(player, source) || !source.GetInventory().GetCargo())
            return 0;

        ref array<EntityAI> leaves = new array<EntityAI>();
        CollectUnpackLeaves(source, null, leaves);

        int moved = 0;
        foreach (EntityAI item : leaves)
        {
            if (TryMoveToVicinity(player, item))
                moved++;
        }
        return moved;
    }

    static bool MoveItem(PlayerBase player, EntityAI item, EntityAI destination)
    {
        if (!player || !item || !destination)
            return false;

        if (IsDescendantOf(destination, item))
            return false;

        bool moved = TryMoveToExactCargo(player, item, destination);
        string movedText = "false";
        if (moved)
            movedText = "true";
        return moved;
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
        int destinationLow;
        int destinationHigh;
        int itemLow;
        int itemHigh;
        bool destinationIsVicinity;

        if (!ctx.Read(operation))
            return;
        if (!ctx.Read(sourceLow))
            return;
        if (!ctx.Read(sourceHigh))
            return;
        if (!ctx.Read(destinationLow))
            return;
        if (!ctx.Read(destinationHigh))
            return;
        if (!ctx.Read(itemLow))
            return;
        if (!ctx.Read(itemHigh))
            return;
        if (!ctx.Read(destinationIsVicinity))
            return;

        EntityAI source = ResolveEntity(sourceLow, sourceHigh);
        EntityAI destination = ResolveEntity(destinationLow, destinationHigh);
        EntityAI item = ResolveEntity(itemLow, itemHigh);

        if (destinationIsVicinity)
        {
            if (operation == TransferZOperation.TRANSFER)
                TransferToVicinity(player, source);
            else if (operation == TransferZOperation.UNPACK)
                UnpackToVicinity(player, source);
            else if (operation == TransferZOperation.MOVE_ITEM)
                MoveItemToVicinity(player, item);
            else if (operation == TransferZOperation.TRANSFER_CLASS)
                TransferClassToVicinity(player, source, item);
            return;
        }

        if (operation == TransferZOperation.TRANSFER)
            Transfer(player, source, destination);
        else if (operation == TransferZOperation.UNPACK)
            Unpack(player, source, destination);
        else if (operation == TransferZOperation.MOVE_ITEM)
            MoveItem(player, item, destination);
        else if (operation == TransferZOperation.TRANSFER_CLASS)
            TransferClass(player, source, destination, item);
    }
}
