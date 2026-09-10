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

    static bool IsReachable(PlayerBase player, EntityAI entity)
    {
        if (!player || !entity)
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

    static bool TryMoveToExactCargo(PlayerBase player, EntityAI item, EntityAI destination)
    {
        if (!player || !item || !destination || item == destination)
            return false;

        if (!IsReachable(player, item) || !IsReachable(player, destination))
            return false;

        CargoBase destinationCargo = destination.GetInventory().GetCargo();
        if (!destinationCargo)
            return false;

        if (!item.GetInventory().CanRemoveEntity())
            return false;

        InventoryLocation src = new InventoryLocation();
        if (!item.GetInventory().GetCurrentInventoryLocation(src))
            return false;

        EntityAI sourceParent = src.GetParent();
        if (sourceParent && src.GetType() == InventoryLocationType.CARGO && !sourceParent.CanReleaseCargo(item))
            return false;

        if (!destination.CanReceiveItemIntoCargo(item))
            return false;

        InventoryLocation dst = new InventoryLocation();
        if (!destination.GetInventory().FindFreeLocationFor(item, FindInventoryLocationType.CARGO, dst))
            return false;

        if (!dst.IsValid() || dst.GetType() != InventoryLocationType.CARGO || dst.GetParent() != destination)
            return false;

        if (!GameInventory.CheckMoveToDstRequest(player, src, dst, GameInventory.c_MaxItemDistanceRadius))
            return false;

        if (!GameInventory.LocationCanMoveEntity(src, dst))
            return false;

        return player.GetInventory().TakeToDst(InventoryMode.SERVER, src, dst);
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
        if (!container || container == destination)
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

    static int Unpack(PlayerBase player, EntityAI source, EntityAI destination)
    {
        if (!player || !source || !destination || source == destination)
            return 0;

        if (!IsReachable(player, source) || !IsReachable(player, destination))
            return 0;

        if (!source.GetInventory().GetCargo() || !destination.GetInventory().GetCargo())
            return 0;

        if (IsDescendantOf(destination, source))
            return 0;

        ref array<EntityAI> leaves = new array<EntityAI>();
        CollectUnpackLeaves(source, destination, leaves);

        int moved = 0;
        foreach (EntityAI item : leaves)
        {
            if (TryMoveToExactCargo(player, item, destination))
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

        return TryMoveToExactCargo(player, item, destination);
    }

    static void HandleRequest(PlayerBase player, PlayerIdentity sender, ParamsReadContext ctx)
    {
        if (!player || !sender || !player.GetIdentity())
            return;

        if (sender.GetId() != player.GetIdentity().GetId())
            return;

        int operation;
        int sourceLow;
        int sourceHigh;
        int destinationLow;
        int destinationHigh;
        int itemLow;
        int itemHigh;

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

        EntityAI source = ResolveEntity(sourceLow, sourceHigh);
        EntityAI destination = ResolveEntity(destinationLow, destinationHigh);
        EntityAI item = ResolveEntity(itemLow, itemHigh);

        if (operation == TransferZOperation.TRANSFER)
            Transfer(player, source, destination);
        else if (operation == TransferZOperation.UNPACK)
            Unpack(player, source, destination);
        else if (operation == TransferZOperation.MOVE_ITEM)
            MoveItem(player, item, destination);
    }
}

modded class PlayerBase
{
    override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
    {
        super.OnRPC(sender, rpc_type, ctx);

        if (rpc_type != TransferZRPC.REQUEST || !GetGame().IsServer())
            return;

        TransferZServerService.HandleRequest(this, sender, ctx);
    }
}
