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
}

class TransferZMaintenanceService
{
    protected static bool IsDirectCargoItem(EntityAI source, EntityAI item)
    {
        if (!source || !item)
            return false;

        InventoryLocation location = new InventoryLocation();
        if (!item.GetInventory().GetCurrentInventoryLocation(location))
            return false;

        return location.GetType() == InventoryLocationType.CARGO && location.GetParent() == source;
    }

    protected static bool SortBefore(TransferZSortRecord left, TransferZSortRecord right)
    {
        int leftArea = left.width * left.height;
        int rightArea = right.width * right.height;
        if (leftArea != rightArea)
            return leftArea > rightArea;
        if (left.typeHash != right.typeHash)
            return left.typeHash < right.typeHash;
        if (left.row != right.row)
            return left.row < right.row;
        return left.col < right.col;
    }

    protected static void SortRecords(notnull array<ref TransferZSortRecord> records)
    {
        for (int i = 1; i < records.Count(); i++)
        {
            ref TransferZSortRecord current = records.Get(i);
            int j = i - 1;
            while (j >= 0 && SortBefore(current, records.Get(j)))
            {
                records.Set(j + 1, records.Get(j));
                j--;
            }
            records.Set(j + 1, current);
        }
    }

    protected static bool SnapshotSortRecords(EntityAI source, notnull array<ref TransferZSortRecord> records, out int cargoWidth, out int cargoHeight)
    {
        records.Clear();
        cargoWidth = 0;
        cargoHeight = 0;

        if (!source)
            return false;

        CargoBase cargo = source.GetInventory().GetCargo();
        if (!cargo)
            return false;

        cargoWidth = cargo.GetWidth();
        cargoHeight = cargo.GetHeight();
        if (cargoWidth <= 0 || cargoHeight <= 0)
        {
            Print("[TransferZ] Sort snapshot failed: invalid cargo dimensions for " + source.GetType());
            return false;
        }

        for (int i = 0; i < cargo.GetItemCount(); i++)
        {
            EntityAI item = cargo.GetItem(i);
            if (!item)
                continue;

            InventoryLocation location = new InventoryLocation();
            if (!item.GetInventory().GetCurrentInventoryLocation(location))
            {
                Print("[TransferZ] Sort snapshot failed: no inventory location for item index=" + i.ToString() + " type=" + item.GetType());
                return false;
            }
            if (location.GetType() != InventoryLocationType.CARGO || location.GetParent() != source)
            {
                Print("[TransferZ] Sort snapshot failed: item is not direct cargo index=" + i.ToString() + " type=" + item.GetType());
                return false;
            }

            int row = location.GetRow();
            int col = location.GetCol();
            int itemWidth;
            int itemHeight;

            // Vanilla CargoContainer uses GetItemSize for its output values without
            // treating the bool return as a validity gate. Position is already
            // authoritative in InventoryLocation, so use that directly as well.
            cargo.GetItemSize(i, itemWidth, itemHeight);

            if (row < 0 || col < 0 || itemWidth <= 0 || itemHeight <= 0)
            {
                Print("[TransferZ] Sort snapshot failed: invalid geometry index=" + i.ToString() + " type=" + item.GetType() + " row=" + row.ToString() + " col=" + col.ToString() + " size=" + itemWidth.ToString() + "x" + itemHeight.ToString());
                return false;
            }

            ref TransferZSortRecord record = new TransferZSortRecord();
            record.item = item;
            record.row = row;
            record.col = col;
            record.width = itemWidth;
            record.height = itemHeight;
            record.flip = location.GetFlip();
            record.cargoIndex = location.GetIdx();

            string typeName = item.GetType();
            typeName.ToLower();
            record.typeHash = typeName.Hash();
            records.Insert(record);
        }

        SortRecords(records);
        return true;
    }

    protected static void ResetGrid(notnull array<int> grid, int cellCount)
    {
        grid.Resize(cellCount);
        for (int i = 0; i < cellCount; i++)
            grid.Set(i, 0);
    }

    protected static bool RectInside(int cargoWidth, int cargoHeight, int row, int col, int itemWidth, int itemHeight)
    {
        return row >= 0 && col >= 0 && row + itemHeight <= cargoHeight && col + itemWidth <= cargoWidth;
    }

    protected static bool RectFree(notnull array<int> grid, int cargoWidth, int cargoHeight, int row, int col, int itemWidth, int itemHeight, int allowedValue = 0)
    {
        if (!RectInside(cargoWidth, cargoHeight, row, col, itemWidth, itemHeight))
            return false;

        for (int y = row; y < row + itemHeight; y++)
        {
            for (int x = col; x < col + itemWidth; x++)
            {
                int value = grid.Get(y * cargoWidth + x);
                if (value != 0 && value != allowedValue)
                    return false;
            }
        }
        return true;
    }

    protected static void MarkRect(notnull array<int> grid, int cargoWidth, int row, int col, int itemWidth, int itemHeight, int value)
    {
        for (int y = row; y < row + itemHeight; y++)
        {
            for (int x = col; x < col + itemWidth; x++)
                grid.Set(y * cargoWidth + x, value);
        }
    }

    protected static bool AssignTargets(notnull array<ref TransferZSortRecord> records, int cargoWidth, int cargoHeight)
    {
        ref array<int> targetGrid = new array<int>();
        ResetGrid(targetGrid, cargoWidth * cargoHeight);

        for (int i = 0; i < records.Count(); i++)
        {
            TransferZSortRecord record = records.Get(i);
            bool placed = false;
            for (int row = 0; row < cargoHeight && !placed; row++)
            {
                for (int col = 0; col < cargoWidth; col++)
                {
                    if (!RectFree(targetGrid, cargoWidth, cargoHeight, row, col, record.width, record.height))
                        continue;

                    record.targetRow = row;
                    record.targetCol = col;
                    MarkRect(targetGrid, cargoWidth, row, col, record.width, record.height, i + 1);
                    placed = true;
                    break;
                }
            }

            if (!placed)
                return false;
        }

        return true;
    }

    protected static bool RecordAtTarget(TransferZSortRecord record)
    {
        return record.row == record.targetRow && record.col == record.targetCol;
    }

    protected static void AddPlannedMove(notnull array<ref TransferZSortMove> moves, TransferZSortRecord record, int row, int col)
    {
        ref TransferZSortMove move = new TransferZSortMove();
        move.item = record.item;
        move.row = row;
        move.col = col;
        move.flip = record.flip;
        moves.Insert(move);
    }

    protected static void MoveRecordInGrid(notnull array<int> currentGrid, int cargoWidth, TransferZSortRecord record, int recordValue, int row, int col)
    {
        MarkRect(currentGrid, cargoWidth, record.row, record.col, record.width, record.height, 0);
        MarkRect(currentGrid, cargoWidth, row, col, record.width, record.height, recordValue);
        record.row = row;
        record.col = col;
    }

    protected static bool BuildSortPlan(notnull array<ref TransferZSortRecord> records, int cargoWidth, int cargoHeight, notnull array<ref TransferZSortMove> moves)
    {
        moves.Clear();
        if (!AssignTargets(records, cargoWidth, cargoHeight))
            return false;

        ref array<int> currentGrid = new array<int>();
        ResetGrid(currentGrid, cargoWidth * cargoHeight);
        for (int i = 0; i < records.Count(); i++)
        {
            TransferZSortRecord record = records.Get(i);
            if (!RectFree(currentGrid, cargoWidth, cargoHeight, record.row, record.col, record.width, record.height))
                return false;
            MarkRect(currentGrid, cargoWidth, record.row, record.col, record.width, record.height, i + 1);
        }

        int moveLimit = Math.Max(32, records.Count() * records.Count() * 6);
        while (moves.Count() < moveLimit)
        {
            bool allDone = true;
            bool progressed = false;

            for (int targetIndex = 0; targetIndex < records.Count(); targetIndex++)
            {
                TransferZSortRecord targetRecord = records.Get(targetIndex);
                if (RecordAtTarget(targetRecord))
                    continue;

                allDone = false;
                if (!RectFree(currentGrid, cargoWidth, cargoHeight, targetRecord.targetRow, targetRecord.targetCol, targetRecord.width, targetRecord.height, targetIndex + 1))
                    continue;

                AddPlannedMove(moves, targetRecord, targetRecord.targetRow, targetRecord.targetCol);
                MoveRecordInGrid(currentGrid, cargoWidth, targetRecord, targetIndex + 1, targetRecord.targetRow, targetRecord.targetCol);
                progressed = true;
            }

            if (allDone)
                return true;
            if (progressed)
                continue;

            int blockedIndex = -1;
            int blockerIndex = -1;
            for (int pendingIndex = 0; pendingIndex < records.Count() && blockerIndex < 0; pendingIndex++)
            {
                TransferZSortRecord pending = records.Get(pendingIndex);
                if (RecordAtTarget(pending))
                    continue;

                blockedIndex = pendingIndex;
                for (int y = pending.targetRow; y < pending.targetRow + pending.height && blockerIndex < 0; y++)
                {
                    for (int x = pending.targetCol; x < pending.targetCol + pending.width; x++)
                    {
                        int value = currentGrid.Get(y * cargoWidth + x);
                        if (value != 0 && value != pendingIndex + 1)
                        {
                            blockerIndex = value - 1;
                            break;
                        }
                    }
                }
            }

            if (blockedIndex < 0 || blockerIndex < 0)
                return false;

            TransferZSortRecord blocker = records.Get(blockerIndex);
            bool tempFound = false;
            for (int tempRow = 0; tempRow < cargoHeight && !tempFound; tempRow++)
            {
                for (int tempCol = 0; tempCol < cargoWidth; tempCol++)
                {
                    if (!RectFree(currentGrid, cargoWidth, cargoHeight, tempRow, tempCol, blocker.width, blocker.height, blockerIndex + 1))
                        continue;
                    if (tempRow == blocker.row && tempCol == blocker.col)
                        continue;
                    if (tempRow == blocker.targetRow && tempCol == blocker.targetCol)
                        continue;

                    // A pending target is valid temporary parking. The previous
                    // planner forbade this and could deadlock after the user moved
                    // an already-sorted item because every useful free rectangle
                    // was also somebody's future target. The planner is still
                    // bounded and validates every physical move before execution.
                    AddPlannedMove(moves, blocker, tempRow, tempCol);
                    MoveRecordInGrid(currentGrid, cargoWidth, blocker, blockerIndex + 1, tempRow, tempCol);
                    tempFound = true;
                    break;
                }
            }

            if (!tempFound)
                return false;
        }

        return false;
    }

    protected static bool TryMoveWithinCargo(PlayerBase player, EntityAI source, EntityAI item, int row, int col, bool flip)
    {
        if (!player || !source || !item || !TransferZServerService.IsReachable(player, source) || !TransferZServerService.IsReachable(player, item))
            return false;
        if (!IsDirectCargoItem(source, item) || !item.GetInventory().CanRemoveEntity() || !source.CanReleaseCargo(item) || !source.CanReceiveItemIntoCargo(item))
            return false;

        InventoryLocation src = new InventoryLocation();
        if (!item.GetInventory().GetCurrentInventoryLocation(src))
            return false;

        InventoryLocation dst = new InventoryLocation();
        dst.SetCargo(source, item, src.GetIdx(), row, col, flip);
        if (!GameInventory.CheckMoveToDstRequest(player, src, dst, GameInventory.c_MaxItemDistanceRadius))
            return false;
        if (!GameInventory.LocationCanMoveEntity(src, dst))
            return false;

        // Sorting is a server-authoritative rearrangement inside one cargo grid.
        // Use the container's own GameInventory here. EntityAI::ServerTakeToDst
        // performs the synchronous location move and sends the inventory sync
        // command; routing this through DayZPlayerInventory can defer the move,
        // which breaks a multi-step sort plan that depends on each prior move
        // having completed before the next one is validated.
        if (GetGame().IsMultiplayer())
            return source.ServerTakeToDst(src, dst);
        return source.LocalTakeToDst(src, dst);
    }

    static int Sort(PlayerBase player, EntityAI source)
    {
        if (!player || !source || !TransferZServerService.IsReachable(player, source) || !source.GetInventory().GetCargo())
        {
            Print("[TransferZ] Sort rejected: invalid or unreachable cargo source");
            return 0;
        }

        ref array<ref TransferZSortRecord> records = new array<ref TransferZSortRecord>();
        int cargoWidth;
        int cargoHeight;
        if (!SnapshotSortRecords(source, records, cargoWidth, cargoHeight))
        {
            Print("[TransferZ] Sort rejected: cargo snapshot failed for " + source.GetType());
            return 0;
        }
        if (records.Count() < 2)
        {
            Print("[TransferZ] Sort skipped: fewer than two direct cargo items in " + source.GetType());
            return 0;
        }

        ref array<ref TransferZSortMove> moves = new array<ref TransferZSortMove>();
        if (!BuildSortPlan(records, cargoWidth, cargoHeight, moves))
        {
            Print("[TransferZ] Sort skipped: no safe in-cargo rearrangement plan for " + source.GetType());
            return 0;
        }
        if (moves.Count() == 0)
        {
            Print("[TransferZ] Sort skipped: cargo already matches target order for " + source.GetType());
            return 0;
        }

        int moved = 0;
        foreach (TransferZSortMove move : moves)
        {
            if (!move || !move.item)
                continue;
            if (!TryMoveWithinCargo(player, source, move.item, move.row, move.col, move.flip))
            {
                Print("[TransferZ] Sort stopped after native move validation failed for " + source.GetType() + " at move " + moved.ToString() + "/" + moves.Count().ToString());
                break;
            }
            moved++;
        }

        Print("[TransferZ] Sort result source=" + source.GetType() + " moves=" + moved.ToString() + "/" + moves.Count().ToString());
        return moved;
    }

    static int Stack(PlayerBase player, EntityAI source)
    {
        if (!player || !source || !TransferZServerService.IsReachable(player, source))
            return 0;

        CargoBase cargo = source.GetInventory().GetCargo();
        if (!cargo)
            return 0;

        ref array<EntityAI> items = new array<EntityAI>();
        for (int i = 0; i < cargo.GetItemCount(); i++)
        {
            EntityAI entity = cargo.GetItem(i);
            if (entity)
                items.Insert(entity);
        }

        int combined = 0;
        for (int targetIndex = 0; targetIndex < items.Count(); targetIndex++)
        {
            ItemBase target = ItemBase.Cast(items.Get(targetIndex));
            if (!target || !IsDirectCargoItem(source, target))
                continue;

            for (int sourceIndex = targetIndex + 1; sourceIndex < items.Count(); sourceIndex++)
            {
                ItemBase donor = ItemBase.Cast(items.Get(sourceIndex));
                if (!donor || !IsDirectCargoItem(source, donor))
                    continue;
                if (!target.CanBeCombined(donor, false, false))
                    continue;

                target.CombineItems(donor, true);
                combined++;

                if (!IsDirectCargoItem(source, target))
                    break;
                if (target.IsFullQuantity())
                    break;
            }
        }

        Print("[TransferZ] Stack result source=" + source.GetType() + " combines=" + combined.ToString());
        return combined;
    }

    static void HandleRequest(PlayerBase player, PlayerIdentity sender, ParamsReadContext ctx)
    {
        if (!player)
            return;

        if (GetGame().IsMultiplayer())
        {
            PlayerIdentity playerIdentity = player.GetIdentity();
            if (!sender || !playerIdentity || sender.GetId() != playerIdentity.GetId())
            {
                Print("[TransferZ] Maintenance RPC rejected: sender does not own player");
                return;
            }
        }

        int operation;
        int sourceLow;
        int sourceHigh;
        if (!ctx.Read(operation) || !ctx.Read(sourceLow) || !ctx.Read(sourceHigh))
            return;

        EntityAI source = TransferZServerService.ResolveEntity(sourceLow, sourceHigh);
        if (!source)
            return;

        if (operation == TransferZMaintenanceOperation.SORT)
            Sort(player, source);
        else if (operation == TransferZMaintenanceOperation.STACK)
            Stack(player, source);

        player.UpdateInventoryMenu();
    }
}
