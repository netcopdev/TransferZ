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
            cargo.GetItemSize(i, itemWidth, itemHeight);

            bool itemFlip = location.GetFlip();
            if (itemFlip)
            {
                int orientationSwap = itemWidth;
                itemWidth = itemHeight;
                itemHeight = orientationSwap;
            }

            if (row < 0 || col < 0 || itemWidth <= 0 || itemHeight <= 0)
            {
                Print("[TransferZ] Sort snapshot failed: invalid geometry index=" + i.ToString() + " type=" + item.GetType() + " row=" + row.ToString() + " col=" + col.ToString() + " size=" + itemWidth.ToString() + "x" + itemHeight.ToString() + " flip=" + itemFlip.ToString());
                return false;
            }

            ref TransferZSortRecord record = new TransferZSortRecord();
            record.item = item;
            record.row = row;
            record.col = col;
            record.width = itemWidth;
            record.height = itemHeight;
            record.flip = itemFlip;
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

    protected static bool AllRecordsAtTarget(notnull array<ref TransferZSortRecord> records)
    {
        for (int i = 0; i < records.Count(); i++)
        {
            if (!RecordAtTarget(records.Get(i)))
                return false;
        }
        return true;
    }

    protected static string BuildSortStateKey(notnull array<ref TransferZSortRecord> records)
    {
        string key = "";
        for (int i = 0; i < records.Count(); i++)
        {
            TransferZSortRecord record = records.Get(i);
            key += record.row.ToString() + "," + record.col.ToString() + ";";
        }
        return key;
    }

    protected static int FindPriorityBlocker(notnull array<ref TransferZSortRecord> records, notnull array<int> currentGrid, int cargoWidth)
    {
        for (int pendingIndex = 0; pendingIndex < records.Count(); pendingIndex++)
        {
            TransferZSortRecord pending = records.Get(pendingIndex);
            if (RecordAtTarget(pending))
                continue;

            for (int y = pending.targetRow; y < pending.targetRow + pending.height; y++)
            {
                for (int x = pending.targetCol; x < pending.targetCol + pending.width; x++)
                {
                    int value = currentGrid.Get(y * cargoWidth + x);
                    if (value != 0 && value != pendingIndex + 1)
                        return value - 1;
                }
            }
        }
        return -1;
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

    protected static bool SearchSortPlan(notnull array<ref TransferZSortRecord> records, notnull array<int> currentGrid, int cargoWidth, int cargoHeight, notnull array<ref TransferZSortMove> moves, notnull array<string> visitedStates, notnull array<int> visitedDepths, int depth, int maxDepth, inout int stateCount, int maxStates)
    {
        if (AllRecordsAtTarget(records))
            return true;
        if (depth >= maxDepth || stateCount >= maxStates)
            return false;

        string stateKey = BuildSortStateKey(records);
        int seenIndex = visitedStates.Find(stateKey);
        if (seenIndex >= 0)
        {
            if (visitedDepths.Get(seenIndex) <= depth)
                return false;
            visitedDepths.Set(seenIndex, depth);
        }
        else
        {
            visitedStates.Insert(stateKey);
            visitedDepths.Insert(depth);
        }
        stateCount++;

        for (int targetIndex = 0; targetIndex < records.Count(); targetIndex++)
        {
            TransferZSortRecord targetRecord = records.Get(targetIndex);
            if (RecordAtTarget(targetRecord))
                continue;
            if (!RectFree(currentGrid, cargoWidth, cargoHeight, targetRecord.targetRow, targetRecord.targetCol, targetRecord.width, targetRecord.height, targetIndex + 1))
                continue;

            int oldRow = targetRecord.row;
            int oldCol = targetRecord.col;
            AddPlannedMove(moves, targetRecord, targetRecord.targetRow, targetRecord.targetCol);
            MoveRecordInGrid(currentGrid, cargoWidth, targetRecord, targetIndex + 1, targetRecord.targetRow, targetRecord.targetCol);

            if (SearchSortPlan(records, currentGrid, cargoWidth, cargoHeight, moves, visitedStates, visitedDepths, depth + 1, maxDepth, stateCount, maxStates))
                return true;

            MoveRecordInGrid(currentGrid, cargoWidth, targetRecord, targetIndex + 1, oldRow, oldCol);
            moves.Remove(moves.Count() - 1);
        }

        int priorityBlocker = FindPriorityBlocker(records, currentGrid, cargoWidth);
        for (int pass = 0; pass < 2; pass++)
        {
            for (int recordIndex = 0; recordIndex < records.Count(); recordIndex++)
            {
                if (pass == 0 && recordIndex != priorityBlocker)
                    continue;
                if (pass == 1 && recordIndex == priorityBlocker)
                    continue;

                TransferZSortRecord record = records.Get(recordIndex);
                if (RecordAtTarget(record))
                    continue;

                for (int tempRow = cargoHeight - record.height; tempRow >= 0; tempRow--)
                {
                    for (int tempCol = cargoWidth - record.width; tempCol >= 0; tempCol--)
                    {
                        if (tempRow == record.row && tempCol == record.col)
                            continue;
                        if (tempRow == record.targetRow && tempCol == record.targetCol)
                            continue;
                        if (!RectFree(currentGrid, cargoWidth, cargoHeight, tempRow, tempCol, record.width, record.height, recordIndex + 1))
                            continue;

                        int tempOldRow = record.row;
                        int tempOldCol = record.col;
                        AddPlannedMove(moves, record, tempRow, tempCol);
                        MoveRecordInGrid(currentGrid, cargoWidth, record, recordIndex + 1, tempRow, tempCol);

                        if (SearchSortPlan(records, currentGrid, cargoWidth, cargoHeight, moves, visitedStates, visitedDepths, depth + 1, maxDepth, stateCount, maxStates))
                            return true;

                        MoveRecordInGrid(currentGrid, cargoWidth, record, recordIndex + 1, tempOldRow, tempOldCol);
                        moves.Remove(moves.Count() - 1);

                        if (stateCount >= maxStates)
                            return false;
                    }
                }
            }
        }

        return false;
    }

    protected static bool BuildSortPlan(notnull array<ref TransferZSortRecord> records, int cargoWidth, int cargoHeight, notnull array<ref TransferZSortMove> moves)
    {
        moves.Clear();
        if (!AssignTargets(records, cargoWidth, cargoHeight))
        {
            Print("[TransferZ] Sort planner failed: deterministic target layout could not be assigned");
            return false;
        }

        ref array<int> currentGrid = new array<int>();
        ResetGrid(currentGrid, cargoWidth * cargoHeight);
        for (int i = 0; i < records.Count(); i++)
        {
            TransferZSortRecord record = records.Get(i);
            if (!RectFree(currentGrid, cargoWidth, cargoHeight, record.row, record.col, record.width, record.height))
            {
                Print("[TransferZ] Sort planner failed: current cargo geometry overlaps at record=" + i.ToString() + " type=" + record.item.GetType() + " row=" + record.row.ToString() + " col=" + record.col.ToString() + " size=" + record.width.ToString() + "x" + record.height.ToString() + " flip=" + record.flip.ToString());
                return false;
            }
            MarkRect(currentGrid, cargoWidth, record.row, record.col, record.width, record.height, i + 1);
        }

        if (AllRecordsAtTarget(records))
            return true;

        int maxDepth = records.Count() * 4 + 12;
        int maxStates = records.Count() * records.Count() * 32;
        if (maxStates < 4096)
            maxStates = 4096;
        if (maxStates > 16000)
            maxStates = 16000;

        ref array<string> visitedStates = new array<string>();
        ref array<int> visitedDepths = new array<int>();
        int stateCount = 0;

        if (SearchSortPlan(records, currentGrid, cargoWidth, cargoHeight, moves, visitedStates, visitedDepths, 0, maxDepth, stateCount, maxStates))
        {
            Print("[TransferZ] Sort planner solved states=" + stateCount.ToString() + " moves=" + moves.Count().ToString());
            return true;
        }

        moves.Clear();
        Print("[TransferZ] Sort planner exhausted search states=" + stateCount.ToString() + "/" + maxStates.ToString() + " depth=" + maxDepth.ToString());
        return false;
    }

    protected static bool TryMoveWithinCargo(PlayerBase player, EntityAI source, EntityAI item, int row, int col, bool flip)
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
        if (!IsDirectCargoItem(source, item))
        {
            Print("[TransferZ] Sort move rejected: item no longer direct cargo item=" + item.GetType());
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
        if (!item.GetInventory().GetCurrentInventoryLocation(src))
        {
            Print("[TransferZ] Sort move rejected: current inventory location unavailable item=" + item.GetType());
            return false;
        }

        InventoryLocation dst = new InventoryLocation();
        dst.SetCargo(source, item, src.GetIdx(), row, col, flip);
        if (!GameInventory.CheckMoveToDstRequest(player, src, dst, GameInventory.c_MaxItemDistanceRadius))
        {
            Print("[TransferZ] Sort move rejected: CheckMoveToDstRequest=false item=" + item.GetType() + " src=" + src.GetRow().ToString() + "," + src.GetCol().ToString() + " dst=" + row.ToString() + "," + col.ToString() + " flip=" + flip.ToString());
            return false;
        }
        if (!GameInventory.LocationCanMoveEntity(src, dst))
        {
            Print("[TransferZ] Sort move rejected: LocationCanMoveEntity=false item=" + item.GetType() + " src=" + src.GetRow().ToString() + "," + src.GetCol().ToString() + " dst=" + row.ToString() + "," + col.ToString() + " flip=" + flip.ToString());
            return false;
        }

        bool moved;
        if (GetGame().IsMultiplayer())
            moved = source.ServerTakeToDst(src, dst);
        else
            moved = source.LocalTakeToDst(src, dst);

        if (!moved)
            Print("[TransferZ] Sort move rejected: native TakeToDst returned false item=" + item.GetType() + " src=" + src.GetRow().ToString() + "," + src.GetCol().ToString() + " dst=" + row.ToString() + "," + col.ToString() + " flip=" + flip.ToString());
        return moved;
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
