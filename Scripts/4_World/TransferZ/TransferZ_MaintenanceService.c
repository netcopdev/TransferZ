class TransferZSortRecord
{
    EntityAI item;
    int row;
    int col;
    int width;
    int height;
    bool flip;
    int cargoIndex;
    int typeHash;
    int targetRow;
    int targetCol;
}

class TransferZSortMove
{
    EntityAI item;
    int row;
    int col;
    bool flip;

    // Non-null for an atomic native swap. Ordinary moves leave this null.
    EntityAI swapItem;
}

class TransferZMaintenanceService
{
    // This is server-load protection only. Inventory validity remains governed
    // by DayZ's native request/location checks and juncture state.
    protected static bool AcceptServerMaintenanceRequest(PlayerBase player)
    {
        return TransferZRequestGuard.AcceptMaintenance(player);
    }

    protected static bool IsDirectCargoItem(EntityAI source, EntityAI item, int sourceCargoIndex)
    {
        if (!source || !item)
            return false;

        InventoryLocation location = new InventoryLocation();
        if (!item.GetInventory().GetCurrentInventoryLocation(location))
            return false;

        return TransferZCargo.LocationMatches(location, source, sourceCargoIndex);
    }

    // CombineItems can synchronously mark an emptied donor for deletion while
    // the object remains in cargo until DayZ processes that deletion. Never use
    // such a donor again during this same Stack pass: CanBeCombined does not
    // reject IsSetForDeletion(), and refilling it would put live quantity into
    // an entity that is still going to disappear.
    protected static bool IsLiveStackItem(ItemBase item)
    {
        return item && !item.IsSetForDeletion();
    }

    protected static bool SnapshotSortRecords(EntityAI source, int sourceCargoIndex, notnull array<ref TransferZSortRecord> records, out int cargoWidth, out int cargoHeight)
    {
        records.Clear();
        cargoWidth = 0;
        cargoHeight = 0;

        if (!source)
            return false;

        CargoBase cargo = TransferZCargo.Get(source, sourceCargoIndex);
        if (!cargo)
            return false;

        cargoWidth = cargo.GetWidth();
        cargoHeight = cargo.GetHeight();
        if (cargoWidth <= 0 || cargoHeight <= 0)
        {
            Print("[TransferZ] Sort snapshot failed: invalid cargo dimensions for " + source.GetType() + " grid=" + sourceCargoIndex.ToString());
            return false;
        }

        for (int cargoItemIndex = 0; cargoItemIndex < cargo.GetItemCount(); cargoItemIndex++)
        {
            EntityAI item = cargo.GetItem(cargoItemIndex);
            if (!item)
                continue;

            InventoryLocation location = new InventoryLocation();
            if (!item.GetInventory().GetCurrentInventoryLocation(location))
            {
                Print("[TransferZ] Sort snapshot failed: no inventory location for item index=" + cargoItemIndex.ToString() + " type=" + item.GetType());
                return false;
            }
            if (!TransferZCargo.LocationMatches(location, source, sourceCargoIndex))
            {
                Print("[TransferZ] Sort snapshot failed: item left requested cargo grid index=" + cargoItemIndex.ToString() + " type=" + item.GetType());
                return false;
            }

            int row = location.GetRow();
            int col = location.GetCol();
            int itemWidth;
            int itemHeight;
            cargo.GetItemSize(cargoItemIndex, itemWidth, itemHeight);

            bool itemFlip = location.GetFlip();
            if (itemFlip)
            {
                int orientationSwap = itemWidth;
                itemWidth = itemHeight;
                itemHeight = orientationSwap;
            }

            if (row < 0 || col < 0 || itemWidth <= 0 || itemHeight <= 0)
            {
                Print("[TransferZ] Sort snapshot failed: invalid geometry index=" + cargoItemIndex.ToString() + " type=" + item.GetType() + " row=" + row.ToString() + " col=" + col.ToString() + " size=" + itemWidth.ToString() + "x" + itemHeight.ToString() + " flip=" + itemFlip.ToString());
                return false;
            }

            ref TransferZSortRecord record = new TransferZSortRecord();
            record.item = item;
            record.row = row;
            record.col = col;
            record.width = itemWidth;
            record.height = itemHeight;
            record.flip = itemFlip;
            record.cargoIndex = sourceCargoIndex;

            string typeName = item.GetType();
            typeName.ToLower();
            record.typeHash = typeName.Hash();
            records.Insert(record);
        }

        return true;
    }

    protected static void ResetGrid(notnull array<int> grid, int cellCount)
    {
        grid.Resize(cellCount);
        for (int cellIndex = 0; cellIndex < cellCount; cellIndex++)
            grid.Set(cellIndex, 0);
    }

    protected static bool RectInside(int cargoWidth, int cargoHeight, int row, int col, int itemWidth, int itemHeight)
    {
        return row >= 0 && col >= 0 && row + itemHeight <= cargoHeight && col + itemWidth <= cargoWidth;
    }

    protected static bool RectFree(notnull array<int> grid, int cargoWidth, int cargoHeight, int row, int col, int itemWidth, int itemHeight, int allowedValue = 0)
    {
        if (!RectInside(cargoWidth, cargoHeight, row, col, itemWidth, itemHeight))
            return false;

        for (int checkRow = row; checkRow < row + itemHeight; checkRow++)
        {
            for (int checkCol = col; checkCol < col + itemWidth; checkCol++)
            {
                int value = grid.Get(checkRow * cargoWidth + checkCol);
                if (value != 0 && value != allowedValue)
                    return false;
            }
        }
        return true;
    }

    protected static void MarkRect(notnull array<int> grid, int cargoWidth, int row, int col, int itemWidth, int itemHeight, int value)
    {
        for (int markRow = row; markRow < row + itemHeight; markRow++)
        {
            for (int markCol = col; markCol < col + itemWidth; markCol++)
                grid.Set(markRow * cargoWidth + markCol, value);
        }
    }

    protected static bool RectOverlaps(int firstRow, int firstCol, int firstWidth, int firstHeight, int secondRow, int secondCol, int secondWidth, int secondHeight)
    {
        if (firstCol + firstWidth <= secondCol || secondCol + secondWidth <= firstCol)
            return false;
        if (firstRow + firstHeight <= secondRow || secondRow + secondHeight <= firstRow)
            return false;
        return true;
    }

    protected static bool TryMoveWithinCargo(PlayerBase player, EntityAI source, int sourceCargoIndex, EntityAI item, int row, int col, bool flip)
    {
        if (!player || !source || !item)
        {
            Print("[TransferZ] Sort move rejected: null player/source/item");
            return false;
        }
        if (!TransferZServerService.IsReachable(player, source) || !TransferZServerService.IsReachable(player, item))
        {
            Print("[TransferZ] Sort move rejected: unreachable item=" + item.GetType());
            return false;
        }
        if (!IsDirectCargoItem(source, item, sourceCargoIndex))
        {
            Print("[TransferZ] Sort move rejected: item no longer in requested cargo grid item=" + item.GetType());
            return false;
        }
        if (!item.GetInventory().CanRemoveEntity())
        {
            Print("[TransferZ] Sort move rejected: CanRemoveEntity=false item=" + item.GetType());
            return false;
        }
        if (!source.CanReleaseCargo(item))
        {
            Print("[TransferZ] Sort move rejected: CanReleaseCargo=false item=" + item.GetType());
            return false;
        }
        if (!source.CanReceiveItemIntoCargo(item))
        {
            Print("[TransferZ] Sort move rejected: CanReceiveItemIntoCargo=false item=" + item.GetType());
            return false;
        }

        InventoryLocation src = new InventoryLocation();
        if (!item.GetInventory().GetCurrentInventoryLocation(src) || !TransferZCargo.LocationMatches(src, source, sourceCargoIndex))
        {
            Print("[TransferZ] Sort move rejected: current cargo grid changed item=" + item.GetType());
            return false;
        }

        InventoryLocation dst = new InventoryLocation();
        dst.SetCargo(source, item, sourceCargoIndex, row, col, flip);

        HumanInventory humanInventory = player.GetHumanInventory();
        if (humanInventory && humanInventory.GetUserReservedLocationCount() > 0 && humanInventory.FindCollidingUserReservedLocationIndex(item, dst) >= 0)
        {
            Print("[TransferZ] Sort move rejected: destination collides with DayZ user-reserved location item=" + item.GetType() + " dst=" + row.ToString() + "," + col.ToString());
            return false;
        }

        if (!GameInventory.CheckMoveToDstRequest(player, src, dst, GameInventory.c_MaxItemDistanceRadius))
        {
            Print("[TransferZ] Sort move rejected: CheckMoveToDstRequest=false item=" + item.GetType());
            return false;
        }
        if (!GameInventory.LocationCanMoveEntity(src, dst))
        {
            Print("[TransferZ] Sort move rejected: LocationCanMoveEntity=false item=" + item.GetType());
            return false;
        }

        if (TransferZServerService.HasNativeInventoryJuncture(item))
        {
            Print("[TransferZ] Sort move rejected: native inventory juncture active item=" + item.GetType());
            return false;
        }

        InventoryMode moveMode = InventoryMode.SERVER;
        if (!GetGame().IsMultiplayer())
            moveMode = InventoryMode.LOCAL;

        bool moved = item.GetInventory().TakeToDst(moveMode, src, dst);
        if (!moved)
            Print("[TransferZ] Sort move rejected: native TakeToDst returned false item=" + item.GetType());
        return moved;
    }

    static int Stack(PlayerBase player, EntityAI source, int sourceCargoIndex = 0)
    {
        if (!player || !source || !TransferZServerService.IsReachable(player, source))
            return 0;

        CargoBase cargo = TransferZCargo.Get(source, sourceCargoIndex);
        if (!cargo)
            return 0;

        ref array<EntityAI> items = new array<EntityAI>();
        for (int cargoItemIndex = 0; cargoItemIndex < cargo.GetItemCount(); cargoItemIndex++)
        {
            EntityAI entity = cargo.GetItem(cargoItemIndex);
            if (entity)
                items.Insert(entity);
        }

        int combined = 0;
        for (int targetIndex = 0; targetIndex < items.Count(); targetIndex++)
        {
            ItemBase target = ItemBase.Cast(items.Get(targetIndex));
            if (!IsLiveStackItem(target))
                continue;

            bool targetValidated = false;

            for (int sourceIndex = targetIndex + 1; sourceIndex < items.Count(); sourceIndex++)
            {
                ItemBase donor = ItemBase.Cast(items.Get(sourceIndex));
                if (!IsLiveStackItem(donor))
                    continue;
                if (!target.CanBeCombined(donor, false, false))
                    continue;

                if (!targetValidated)
                {
                    InventoryLocation targetLocation = new InventoryLocation();
                    if (!target.GetInventory().GetCurrentInventoryLocation(targetLocation))
                        break;
                    if (!TransferZCargo.LocationMatches(targetLocation, source, sourceCargoIndex))
                        break;
                    if (!GameInventory.CheckRequestSrc(player, targetLocation, GameInventory.c_MaxItemDistanceRadius))
                        break;
                    targetValidated = true;
                }

                InventoryLocation donorLocation = new InventoryLocation();
                if (!donor.GetInventory().GetCurrentInventoryLocation(donorLocation))
                    continue;
                if (!TransferZCargo.LocationMatches(donorLocation, source, sourceCargoIndex))
                    continue;
                if (!GameInventory.CheckRequestSrc(player, donorLocation, GameInventory.c_MaxItemDistanceRadius))
                    continue;

                if (TransferZServerService.HasNativeInventoryJuncture(target) || TransferZServerService.HasNativeInventoryJuncture(donor))
                    continue;

                target.CombineItems(donor, true);
                combined++;

                if (!IsDirectCargoItem(source, target, sourceCargoIndex))
                    break;
                if (target.IsFullQuantity())
                    break;
            }
        }
        return combined;
    }

    protected static bool TrySwapWithinCargo(PlayerBase player, EntityAI source, int sourceCargoIndex, EntityAI item1, EntityAI item2)
    {
        if (!player || !source || !item1 || !item2 || item1 == item2)
            return false;
        if (!IsDirectCargoItem(source, item1, sourceCargoIndex) || !IsDirectCargoItem(source, item2, sourceCargoIndex))
            return false;
        if (!item1.GetInventory().CanRemoveEntity() || !item2.GetInventory().CanRemoveEntity())
            return false;
        if (!GameInventory.CanSwapEntitiesEx(item1, item2))
            return false;

        InventoryLocation src1;
        InventoryLocation src2;
        InventoryLocation dst1;
        InventoryLocation dst2;
        if (!GameInventory.MakeSrcAndDstForSwap(item1, item2, src1, src2, dst1, dst2))
            return false;
        if (!src1 || !src2 || !dst1 || !dst2)
            return false;
        if (!TransferZCargo.LocationMatches(src1, source, sourceCargoIndex) || !TransferZCargo.LocationMatches(src2, source, sourceCargoIndex))
            return false;
        if (!TransferZCargo.LocationMatches(dst1, source, sourceCargoIndex) || !TransferZCargo.LocationMatches(dst2, source, sourceCargoIndex))
            return false;

        HumanInventory humanInventory = player.GetHumanInventory();
        if (humanInventory && humanInventory.GetUserReservedLocationCount() > 0)
        {
            if (humanInventory.FindCollidingUserReservedLocationIndex(item1, dst1) >= 0)
                return false;
            if (humanInventory.FindCollidingUserReservedLocationIndex(item2, dst2) >= 0)
                return false;
        }

        if (TransferZServerService.HasNativeInventoryJuncture(item1) || TransferZServerService.HasNativeInventoryJuncture(item2))
            return false;

        if (GetGame().IsMultiplayer())
        {
            InventoryInputUserData.SendServerSwap(src1, src2, dst1, dst2);
            return true;
        }

        return GameInventory.LocationSwap(src1, src2, dst1, dst2);
    }

    protected static void SendResult(PlayerBase player, int operation, int sourceLow, int sourceHigh, int sourceCargoIndex, bool success)
    {
        if (!player || !GetGame().IsMultiplayer())
            return;

        PlayerIdentity identity = player.GetIdentity();
        if (!identity)
            return;

        ScriptRPC rpc = new ScriptRPC();
        rpc.Write(operation);
        rpc.Write(sourceLow);
        rpc.Write(sourceHigh);
        rpc.Write(sourceCargoIndex);
        rpc.Write(success);
        rpc.Send(player, TransferZMaintenanceRPC.RESULT, true, identity);
    }

}
