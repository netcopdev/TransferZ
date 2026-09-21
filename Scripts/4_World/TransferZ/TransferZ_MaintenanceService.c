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
    protected static const int SERVER_MAINTENANCE_THROTTLE_MS = 250;
    protected static ref map<string, int> s_LastMaintenanceRequestTime = new map<string, int>();

    // This is server-load protection only. Inventory validity remains governed
    // by DayZ's native request/location checks and juncture state.
    protected static bool AcceptServerMaintenanceRequest(PlayerBase player)
    {
        if (!GetGame().IsMultiplayer())
            return true;
        if (!player)
            return false;

        PlayerIdentity identity = player.GetIdentity();
        if (!identity)
            return false;

        string playerId = identity.GetId();
        int now = GetGame().GetTime();
        if (s_LastMaintenanceRequestTime.Contains(playerId))
        {
            int last = s_LastMaintenanceRequestTime.Get(playerId);
            int elapsed = now - last;
            if (elapsed >= 0 && elapsed < SERVER_MAINTENANCE_THROTTLE_MS)
                return false;
        }

        s_LastMaintenanceRequestTime.Set(playerId, now);
        return true;
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
        for (int recordIndex = 1; recordIndex < records.Count(); recordIndex++)
        {
            ref TransferZSortRecord current = records.Get(recordIndex);
            int previousIndex = recordIndex - 1;
            while (previousIndex >= 0 && SortBefore(current, records.Get(previousIndex)))
            {
                records.Set(previousIndex + 1, records.Get(previousIndex));
                previousIndex--;
            }
            records.Set(previousIndex + 1, current);
        }
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

        SortRecords(records);
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

    protected static bool CollidesWithUserReservation(PlayerBase player, EntityAI source, TransferZSortRecord record, int row, int col)
    {
        if (!player || !source || !record || !record.item)
            return true;

        HumanInventory humanInventory = player.GetHumanInventory();
        if (!humanInventory)
            return false;

        InventoryLocation destination = new InventoryLocation();
        destination.SetCargo(source, record.item, record.cargoIndex, row, col, record.flip);
        return humanInventory.FindCollidingUserReservedLocationIndex(record.item, destination) >= 0;
    }

    protected static bool AssignTargets(PlayerBase player, EntityAI source, notnull array<ref TransferZSortRecord> records, int cargoWidth, int cargoHeight)
    {
        ref array<int> targetGrid = new array<int>();
        ResetGrid(targetGrid, cargoWidth * cargoHeight);

        for (int targetIndex = 0; targetIndex < records.Count(); targetIndex++)
        {
            TransferZSortRecord record = records.Get(targetIndex);
            bool placed = false;

            for (int targetRow = 0; targetRow < cargoHeight && !placed; targetRow++)
            {
                for (int targetCol = 0; targetCol < cargoWidth; targetCol++)
                {
                    if (!RectFree(targetGrid, cargoWidth, cargoHeight, targetRow, targetCol, record.width, record.height))
                        continue;
                    if (CollidesWithUserReservation(player, source, record, targetRow, targetCol))
                        continue;

                    record.targetRow = targetRow;
                    record.targetCol = targetCol;
                    MarkRect(targetGrid, cargoWidth, targetRow, targetCol, record.width, record.height, targetIndex + 1);
                    placed = true;
                    break;
                }
            }

            if (!placed)
                return false;
        }

        return true;
    }

    protected static void BuildTargetGrid(notnull array<ref TransferZSortRecord> records, int cargoWidth, int cargoHeight, notnull array<int> targetGrid)
    {
        ResetGrid(targetGrid, cargoWidth * cargoHeight);
        for (int targetIndex = 0; targetIndex < records.Count(); targetIndex++)
        {
            TransferZSortRecord record = records.Get(targetIndex);
            MarkRect(targetGrid, cargoWidth, record.targetRow, record.targetCol, record.width, record.height, targetIndex + 1);
        }
    }

    protected static bool RecordAtTarget(TransferZSortRecord record)
    {
        return record.row == record.targetRow && record.col == record.targetCol;
    }

    protected static bool AllRecordsAtTarget(notnull array<ref TransferZSortRecord> records)
    {
        for (int recordIndex = 0; recordIndex < records.Count(); recordIndex++)
        {
            if (!RecordAtTarget(records.Get(recordIndex)))
                return false;
        }
        return true;
    }

    protected static int FindBlockerForTarget(notnull array<ref TransferZSortRecord> records, notnull array<int> currentGrid, int cargoWidth, int targetIndex)
    {
        TransferZSortRecord pending = records.Get(targetIndex);
        for (int targetRow = pending.targetRow; targetRow < pending.targetRow + pending.height; targetRow++)
        {
            for (int targetCol = pending.targetCol; targetCol < pending.targetCol + pending.width; targetCol++)
            {
                int value = currentGrid.Get(targetRow * cargoWidth + targetCol);
                if (value != 0 && value != targetIndex + 1)
                    return value - 1;
            }
        }
        return -1;
    }

    protected static int CountTargetOverlap(notnull array<int> targetGrid, int cargoWidth, int row, int col, int itemWidth, int itemHeight)
    {
        int overlap = 0;
        for (int overlapRow = row; overlapRow < row + itemHeight; overlapRow++)
        {
            for (int overlapCol = col; overlapCol < col + itemWidth; overlapCol++)
            {
                if (targetGrid.Get(overlapRow * cargoWidth + overlapCol) != 0)
                    overlap++;
            }
        }
        return overlap;
    }

    protected static bool FindTemporaryPlacement(PlayerBase player, EntityAI source, notnull array<ref TransferZSortRecord> records, notnull array<int> currentGrid, notnull array<int> targetGrid, int cargoWidth, int cargoHeight, int recordIndex, int protectedTargetIndex, bool requireTargetOverlapImprovement, out int bestRow, out int bestCol)
    {
        bestRow = -1;
        bestCol = -1;

        TransferZSortRecord record = records.Get(recordIndex);
        int currentOverlap = CountTargetOverlap(targetGrid, cargoWidth, record.row, record.col, record.width, record.height);
        int bestOverlap = 1000000;
        TransferZSortRecord protectedTarget;
        if (protectedTargetIndex >= 0)
            protectedTarget = records.Get(protectedTargetIndex);

        for (int tempRow = cargoHeight - record.height; tempRow >= 0; tempRow--)
        {
            for (int tempCol = cargoWidth - record.width; tempCol >= 0; tempCol--)
            {
                if (tempRow == record.row && tempCol == record.col)
                    continue;
                if (tempRow == record.targetRow && tempCol == record.targetCol)
                    continue;
                if (protectedTarget && RectOverlaps(tempRow, tempCol, record.width, record.height, protectedTarget.targetRow, protectedTarget.targetCol, protectedTarget.width, protectedTarget.height))
                    continue;
                if (!RectFree(currentGrid, cargoWidth, cargoHeight, tempRow, tempCol, record.width, record.height, recordIndex + 1))
                    continue;

                int targetOverlap = CountTargetOverlap(targetGrid, cargoWidth, tempRow, tempCol, record.width, record.height);
                if (requireTargetOverlapImprovement && targetOverlap >= currentOverlap)
                    continue;
                if (targetOverlap >= bestOverlap)
                    continue;
                if (CollidesWithUserReservation(player, source, record, tempRow, tempCol))
                    continue;

                bestOverlap = targetOverlap;
                bestRow = tempRow;
                bestCol = tempCol;
                if (targetOverlap == 0)
                    return true;
            }
        }

        return bestRow >= 0;
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

    protected static bool ParkRecord(PlayerBase player, EntityAI source, notnull array<ref TransferZSortRecord> records, notnull array<int> currentGrid, notnull array<int> targetGrid, int cargoWidth, int cargoHeight, notnull array<ref TransferZSortMove> moves, int recordIndex, int protectedTargetIndex, bool requireTargetOverlapImprovement, inout int stepCount, int maxSteps)
    {
        if (stepCount >= maxSteps)
            return false;

        int tempRow;
        int tempCol;
        if (!FindTemporaryPlacement(player, source, records, currentGrid, targetGrid, cargoWidth, cargoHeight, recordIndex, protectedTargetIndex, requireTargetOverlapImprovement, tempRow, tempCol))
            return false;

        TransferZSortRecord record = records.Get(recordIndex);
        AddPlannedMove(moves, record, tempRow, tempCol);
        MoveRecordInGrid(currentGrid, cargoWidth, record, recordIndex + 1, tempRow, tempCol);
        stepCount++;
        return true;
    }

    protected static bool BreakActiveCycle(PlayerBase player, EntityAI source, notnull array<ref TransferZSortRecord> records, notnull array<int> currentGrid, notnull array<int> targetGrid, int cargoWidth, int cargoHeight, notnull array<ref TransferZSortMove> moves, notnull array<int> activeRecords, int blockerIndex, int protectedTargetIndex, inout int stepCount, int maxSteps)
    {
        if (ParkRecord(player, source, records, currentGrid, targetGrid, cargoWidth, cargoHeight, moves, blockerIndex, protectedTargetIndex, false, stepCount, maxSteps))
            return true;

        for (int activeIndex = records.Count() - 1; activeIndex >= 0; activeIndex--)
        {
            if (activeIndex == blockerIndex || activeRecords.Get(activeIndex) == 0)
                continue;
            if (ParkRecord(player, source, records, currentGrid, targetGrid, cargoWidth, cargoHeight, moves, activeIndex, protectedTargetIndex, true, stepCount, maxSteps))
                return true;
        }

        return false;
    }

    protected static bool EnsureRecordAtTarget(PlayerBase player, EntityAI source, notnull array<ref TransferZSortRecord> records, notnull array<int> currentGrid, notnull array<int> targetGrid, int cargoWidth, int cargoHeight, notnull array<ref TransferZSortMove> moves, notnull array<int> activeRecords, int recordIndex, inout int stepCount, int maxSteps)
    {
        TransferZSortRecord record = records.Get(recordIndex);
        if (RecordAtTarget(record))
            return true;
        if (activeRecords.Get(recordIndex) != 0 || stepCount >= maxSteps)
            return false;

        activeRecords.Set(recordIndex, 1);
        int clearGuard = 0;
        int clearGuardLimit = records.Count() * 6 + 16;

        while (!RectFree(currentGrid, cargoWidth, cargoHeight, record.targetRow, record.targetCol, record.width, record.height, recordIndex + 1))
        {
            if (stepCount >= maxSteps || clearGuard >= clearGuardLimit)
            {
                activeRecords.Set(recordIndex, 0);
                return false;
            }

            int blockerIndex = FindBlockerForTarget(records, currentGrid, cargoWidth, recordIndex);
            if (blockerIndex < 0)
            {
                activeRecords.Set(recordIndex, 0);
                return false;
            }

            bool blockerCleared = false;
            if (activeRecords.Get(blockerIndex) != 0)
            {
                blockerCleared = BreakActiveCycle(player, source, records, currentGrid, targetGrid, cargoWidth, cargoHeight, moves, activeRecords, blockerIndex, recordIndex, stepCount, maxSteps);
            }
            else
            {
                blockerCleared = EnsureRecordAtTarget(player, source, records, currentGrid, targetGrid, cargoWidth, cargoHeight, moves, activeRecords, blockerIndex, stepCount, maxSteps);
                if (!blockerCleared)
                    blockerCleared = ParkRecord(player, source, records, currentGrid, targetGrid, cargoWidth, cargoHeight, moves, blockerIndex, recordIndex, false, stepCount, maxSteps);
            }

            if (!blockerCleared)
            {
                activeRecords.Set(recordIndex, 0);
                return false;
            }

            clearGuard++;
        }

        if (!RecordAtTarget(record))
        {
            if (stepCount >= maxSteps || CollidesWithUserReservation(player, source, record, record.targetRow, record.targetCol))
            {
                activeRecords.Set(recordIndex, 0);
                return false;
            }

            AddPlannedMove(moves, record, record.targetRow, record.targetCol);
            MoveRecordInGrid(currentGrid, cargoWidth, record, recordIndex + 1, record.targetRow, record.targetCol);
            stepCount++;
        }

        activeRecords.Set(recordIndex, 0);
        return true;
    }

    protected static bool BuildSortPlan(PlayerBase player, EntityAI source, notnull array<ref TransferZSortRecord> records, int cargoWidth, int cargoHeight, notnull array<ref TransferZSortMove> moves)
    {
        moves.Clear();
        if (!AssignTargets(player, source, records, cargoWidth, cargoHeight))
        {
            Print("[TransferZ] Sort planner failed: deterministic target layout could not be assigned around active inventory reservations");
            return false;
        }

        ref array<int> currentGrid = new array<int>();
        ResetGrid(currentGrid, cargoWidth * cargoHeight);
        for (int currentIndex = 0; currentIndex < records.Count(); currentIndex++)
        {
            TransferZSortRecord currentRecord = records.Get(currentIndex);
            if (!RectFree(currentGrid, cargoWidth, cargoHeight, currentRecord.row, currentRecord.col, currentRecord.width, currentRecord.height))
            {
                Print("[TransferZ] Sort planner failed: current cargo geometry overlaps at record=" + currentIndex.ToString() + " type=" + currentRecord.item.GetType() + " row=" + currentRecord.row.ToString() + " col=" + currentRecord.col.ToString() + " size=" + currentRecord.width.ToString() + "x" + currentRecord.height.ToString() + " flip=" + currentRecord.flip.ToString());
                return false;
            }
            MarkRect(currentGrid, cargoWidth, currentRecord.row, currentRecord.col, currentRecord.width, currentRecord.height, currentIndex + 1);
        }

        if (AllRecordsAtTarget(records))
            return true;

        ref array<int> targetGrid = new array<int>();
        BuildTargetGrid(records, cargoWidth, cargoHeight, targetGrid);

        ref array<int> activeRecords = new array<int>();
        ResetGrid(activeRecords, records.Count());

        int maxSteps = records.Count() * 16 + 64;
        if (maxSteps < 128)
            maxSteps = 128;
        if (maxSteps > 768)
            maxSteps = 768;

        int stepCount = 0;
        for (int planIndex = 0; planIndex < records.Count(); planIndex++)
        {
            if (!EnsureRecordAtTarget(player, source, records, currentGrid, targetGrid, cargoWidth, cargoHeight, moves, activeRecords, planIndex, stepCount, maxSteps))
            {
                moves.Clear();
                Print("[TransferZ] Sort planner stopped: could not clear target for record=" + planIndex.ToString() + " steps=" + stepCount.ToString() + "/" + maxSteps.ToString());
                return false;
            }
        }

        if (!AllRecordsAtTarget(records))
        {
            moves.Clear();
            Print("[TransferZ] Sort planner stopped: target layout incomplete after steps=" + stepCount.ToString());
            return false;
        }
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
