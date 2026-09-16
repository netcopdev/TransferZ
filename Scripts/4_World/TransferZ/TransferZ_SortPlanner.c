class TransferZSortPlanner : TransferZMaintenanceService
{
    protected static bool SortBeforeV3(TransferZSortRecord left, TransferZSortRecord right)
    {
        int leftArea = left.width * left.height;
        int rightArea = right.width * right.height;
        if (leftArea != rightArea)
            return leftArea > rightArea;
        if (left.width != right.width)
            return left.width > right.width;
        if (left.height != right.height)
            return left.height > right.height;
        if (left.row != right.row)
            return left.row < right.row;
        if (left.col != right.col)
            return left.col < right.col;
        return left.typeHash < right.typeHash;
    }

    protected static void SortRecordsV3(notnull array<ref TransferZSortRecord> records)
    {
        for (int recordIndex = 1; recordIndex < records.Count(); recordIndex++)
        {
            ref TransferZSortRecord current = records.Get(recordIndex);
            int previousIndex = recordIndex - 1;
            while (previousIndex >= 0 && SortBeforeV3(current, records.Get(previousIndex)))
            {
                records.Set(previousIndex + 1, records.Get(previousIndex));
                previousIndex--;
            }
            records.Set(previousIndex + 1, current);
        }
    }

    protected static int CountForeignCurrentOverlapsV3(notnull array<ref TransferZSortRecord> records, int recordIndex, int targetRow, int targetCol)
    {
        TransferZSortRecord record = records.Get(recordIndex);
        int overlapCount = 0;

        for (int otherIndex = 0; otherIndex < records.Count(); otherIndex++)
        {
            if (otherIndex == recordIndex)
                continue;

            TransferZSortRecord other = records.Get(otherIndex);
            if (RectOverlaps(targetRow, targetCol, record.width, record.height, other.row, other.col, other.width, other.height))
                overlapCount++;
        }

        return overlapCount;
    }

    protected static int TargetAssignmentCostV3(notnull array<ref TransferZSortRecord> records, int recordIndex, int targetRow, int targetCol)
    {
        TransferZSortRecord record = records.Get(recordIndex);
        int foreignOverlap = CountForeignCurrentOverlapsV3(records, recordIndex, targetRow, targetCol);
        int distance = Math.AbsInt(record.row - targetRow) + Math.AbsInt(record.col - targetCol);
        return foreignOverlap * 10000 + distance;
    }

    protected static void OptimizeEquivalentTargetAssignmentsV3(notnull array<ref TransferZSortRecord> records)
    {
        bool changed = true;
        int passCount = 0;
        int passLimit = records.Count() * records.Count() + 1;

        while (changed && passCount < passLimit)
        {
            changed = false;
            passCount++;

            for (int leftIndex = 0; leftIndex < records.Count(); leftIndex++)
            {
                TransferZSortRecord left = records.Get(leftIndex);
                for (int rightIndex = leftIndex + 1; rightIndex < records.Count(); rightIndex++)
                {
                    TransferZSortRecord right = records.Get(rightIndex);
                    if (left.width != right.width || left.height != right.height)
                        continue;

                    int currentCost = TargetAssignmentCostV3(records, leftIndex, left.targetRow, left.targetCol) + TargetAssignmentCostV3(records, rightIndex, right.targetRow, right.targetCol);
                    int swappedCost = TargetAssignmentCostV3(records, leftIndex, right.targetRow, right.targetCol) + TargetAssignmentCostV3(records, rightIndex, left.targetRow, left.targetCol);
                    if (swappedCost >= currentCost)
                        continue;

                    int swapRow = left.targetRow;
                    int swapCol = left.targetCol;
                    left.targetRow = right.targetRow;
                    left.targetCol = right.targetCol;
                    right.targetRow = swapRow;
                    right.targetCol = swapCol;
                    changed = true;
                }
            }
        }
    }

    protected static void LogTargetsV3(notnull array<ref TransferZSortRecord> records)
    {
        for (int recordIndex = 0; recordIndex < records.Count(); recordIndex++)
        {
            TransferZSortRecord record = records.Get(recordIndex);
            Print("[TransferZ] Sort planner V3 target record=" + recordIndex.ToString() + " type=" + record.item.GetType() + " from=" + record.row.ToString() + "," + record.col.ToString() + " to=" + record.targetRow.ToString() + "," + record.targetCol.ToString() + " size=" + record.width.ToString() + "x" + record.height.ToString());
        }
    }

    protected static void CapturePositionsV3(notnull array<ref TransferZSortRecord> records, notnull array<int> rows, notnull array<int> cols)
    {
        rows.Resize(records.Count());
        cols.Resize(records.Count());
        for (int recordIndex = 0; recordIndex < records.Count(); recordIndex++)
        {
            TransferZSortRecord record = records.Get(recordIndex);
            rows.Set(recordIndex, record.row);
            cols.Set(recordIndex, record.col);
        }
    }

    protected static void RestorePlannerStateV3(notnull array<ref TransferZSortRecord> records, notnull array<int> currentGrid, int cargoWidth, int cargoHeight, notnull array<ref TransferZSortMove> moves, int moveCount, notnull array<int> rows, notnull array<int> cols)
    {
        for (int recordIndex = 0; recordIndex < records.Count(); recordIndex++)
        {
            TransferZSortRecord record = records.Get(recordIndex);
            record.row = rows.Get(recordIndex);
            record.col = cols.Get(recordIndex);
        }

        moves.Resize(moveCount);
        ResetGrid(currentGrid, cargoWidth * cargoHeight);
        for (int gridIndex = 0; gridIndex < records.Count(); gridIndex++)
        {
            TransferZSortRecord gridRecord = records.Get(gridIndex);
            MarkRect(currentGrid, cargoWidth, gridRecord.row, gridRecord.col, gridRecord.width, gridRecord.height, gridIndex + 1);
        }
    }

    protected static bool CollectBlockersV3(notnull array<int> currentGrid, int cargoWidth, int row, int col, int itemWidth, int itemHeight, int ownValue, notnull array<int> blockers)
    {
        blockers.Clear();
        for (int scanRow = row; scanRow < row + itemHeight; scanRow++)
        {
            for (int scanCol = col; scanCol < col + itemWidth; scanCol++)
            {
                int value = currentGrid.Get(scanRow * cargoWidth + scanCol);
                if (value == 0 || value == ownValue)
                    continue;

                int blockerIndex = value - 1;
                if (blockers.Find(blockerIndex) < 0)
                    blockers.Insert(blockerIndex);
            }
        }
        return blockers.Count() > 0;
    }

    protected static bool ParkRecordRecursiveV3(PlayerBase player, EntityAI source, notnull array<ref TransferZSortRecord> records, notnull array<int> currentGrid, notnull array<int> targetGrid, notnull array<int> lockedRecords, notnull array<int> activeParking, int cargoWidth, int cargoHeight, notnull array<ref TransferZSortMove> moves, int recordIndex, int protectedTargetIndex, int depth, int maxDepth, inout int stepCount, int maxSteps, inout int searchCount, int maxSearch)
    {
        if (recordIndex < 0 || recordIndex >= records.Count())
            return false;
        if (lockedRecords.Get(recordIndex) != 0 || activeParking.Get(recordIndex) != 0)
            return false;
        if (depth > maxDepth || stepCount >= maxSteps || searchCount >= maxSearch)
            return false;

        searchCount++;
        activeParking.Set(recordIndex, 1);

        int directRow;
        int directCol;
        if (FindTemporaryPlacement(player, source, records, currentGrid, targetGrid, cargoWidth, cargoHeight, recordIndex, protectedTargetIndex, false, directRow, directCol))
        {
            TransferZSortRecord directRecord = records.Get(recordIndex);
            AddPlannedMove(moves, directRecord, directRow, directCol);
            MoveRecordInGrid(currentGrid, cargoWidth, directRecord, recordIndex + 1, directRow, directCol);
            stepCount++;
            activeParking.Set(recordIndex, 0);
            Print("[TransferZ] Sort planner V3 parked record=" + recordIndex.ToString() + " direct=" + directRow.ToString() + "," + directCol.ToString());
            return true;
        }

        TransferZSortRecord record = records.Get(recordIndex);
        TransferZSortRecord protectedTarget;
        if (protectedTargetIndex >= 0)
            protectedTarget = records.Get(protectedTargetIndex);

        for (int candidateRow = cargoHeight - record.height; candidateRow >= 0; candidateRow--)
        {
            for (int candidateCol = cargoWidth - record.width; candidateCol >= 0; candidateCol--)
            {
                if (searchCount >= maxSearch || stepCount >= maxSteps)
                    break;
                if (candidateRow == record.row && candidateCol == record.col)
                    continue;
                if (candidateRow == record.targetRow && candidateCol == record.targetCol)
                    continue;
                if (protectedTarget && RectOverlaps(candidateRow, candidateCol, record.width, record.height, protectedTarget.targetRow, protectedTarget.targetCol, protectedTarget.width, protectedTarget.height))
                    continue;
                if (CollidesWithUserReservation(player, source, record, candidateRow, candidateCol))
                    continue;

                ref array<int> blockers = new array<int>();
                CollectBlockersV3(currentGrid, cargoWidth, candidateRow, candidateCol, record.width, record.height, recordIndex + 1, blockers);
                bool candidateBlockedByLocked = false;
                bool candidateBlockedByActive = false;
                foreach (int blockerIndex : blockers)
                {
                    if (lockedRecords.Get(blockerIndex) != 0)
                        candidateBlockedByLocked = true;
                    if (activeParking.Get(blockerIndex) != 0)
                        candidateBlockedByActive = true;
                }
                if (candidateBlockedByLocked || candidateBlockedByActive)
                    continue;

                int savedMoveCount = moves.Count();
                int savedStepCount = stepCount;
                ref array<int> savedRows = new array<int>();
                ref array<int> savedCols = new array<int>();
                CapturePositionsV3(records, savedRows, savedCols);

                bool cleared = true;
                foreach (int recursiveBlocker : blockers)
                {
                    if (!ParkRecordRecursiveV3(player, source, records, currentGrid, targetGrid, lockedRecords, activeParking, cargoWidth, cargoHeight, moves, recursiveBlocker, protectedTargetIndex, depth + 1, maxDepth, stepCount, maxSteps, searchCount, maxSearch))
                    {
                        cleared = false;
                        break;
                    }
                }

                if (cleared && RectFree(currentGrid, cargoWidth, cargoHeight, candidateRow, candidateCol, record.width, record.height, recordIndex + 1))
                {
                    AddPlannedMove(moves, record, candidateRow, candidateCol);
                    MoveRecordInGrid(currentGrid, cargoWidth, record, recordIndex + 1, candidateRow, candidateCol);
                    stepCount++;
                    activeParking.Set(recordIndex, 0);
                    Print("[TransferZ] Sort planner V3 recursively parked record=" + recordIndex.ToString() + " at=" + candidateRow.ToString() + "," + candidateCol.ToString() + " depth=" + depth.ToString());
                    return true;
                }

                RestorePlannerStateV3(records, currentGrid, cargoWidth, cargoHeight, moves, savedMoveCount, savedRows, savedCols);
                stepCount = savedStepCount;
            }
        }

        activeParking.Set(recordIndex, 0);
        return false;
    }

    protected static bool EnsureRecordAtTargetV3(PlayerBase player, EntityAI source, notnull array<ref TransferZSortRecord> records, notnull array<int> currentGrid, notnull array<int> targetGrid, notnull array<int> lockedRecords, notnull array<int> activeParking, int cargoWidth, int cargoHeight, notnull array<ref TransferZSortMove> moves, int recordIndex, int maxDepth, inout int stepCount, int maxSteps, inout int searchCount, int maxSearch)
    {
        TransferZSortRecord record = records.Get(recordIndex);
        if (RecordAtTarget(record))
        {
            lockedRecords.Set(recordIndex, 1);
            return true;
        }

        int clearGuard = 0;
        int clearGuardLimit = records.Count() * 8 + 16;
        while (!RectFree(currentGrid, cargoWidth, cargoHeight, record.targetRow, record.targetCol, record.width, record.height, recordIndex + 1))
        {
            if (clearGuard >= clearGuardLimit || stepCount >= maxSteps || searchCount >= maxSearch)
                return false;

            int blockerIndex = FindBlockerForTarget(records, currentGrid, cargoWidth, recordIndex);
            if (blockerIndex < 0 || lockedRecords.Get(blockerIndex) != 0)
                return false;

            if (!ParkRecordRecursiveV3(player, source, records, currentGrid, targetGrid, lockedRecords, activeParking, cargoWidth, cargoHeight, moves, blockerIndex, recordIndex, 0, maxDepth, stepCount, maxSteps, searchCount, maxSearch))
            {
                Print("[TransferZ] Sort planner V3 could not evacuate blocker=" + blockerIndex.ToString() + " for target=" + recordIndex.ToString());
                return false;
            }
            clearGuard++;
        }

        if (CollidesWithUserReservation(player, source, record, record.targetRow, record.targetCol))
            return false;

        AddPlannedMove(moves, record, record.targetRow, record.targetCol);
        MoveRecordInGrid(currentGrid, cargoWidth, record, recordIndex + 1, record.targetRow, record.targetCol);
        stepCount++;
        lockedRecords.Set(recordIndex, 1);
        Print("[TransferZ] Sort planner V3 locked record=" + recordIndex.ToString() + " at=" + record.targetRow.ToString() + "," + record.targetCol.ToString());
        return true;
    }

    protected static bool BuildSortPlanV3(PlayerBase player, EntityAI source, notnull array<ref TransferZSortRecord> records, int cargoWidth, int cargoHeight, notnull array<ref TransferZSortMove> moves)
    {
        moves.Clear();
        if (!AssignTargets(player, source, records, cargoWidth, cargoHeight))
        {
            Print("[TransferZ] Sort planner V3 failed: compact target layout assignment");
            return false;
        }

        OptimizeEquivalentTargetAssignmentsV3(records);
        LogTargetsV3(records);

        ref array<int> currentGrid = new array<int>();
        ResetGrid(currentGrid, cargoWidth * cargoHeight);
        for (int currentIndex = 0; currentIndex < records.Count(); currentIndex++)
        {
            TransferZSortRecord currentRecord = records.Get(currentIndex);
            if (!RectFree(currentGrid, cargoWidth, cargoHeight, currentRecord.row, currentRecord.col, currentRecord.width, currentRecord.height))
            {
                Print("[TransferZ] Sort planner V3 failed: overlapping current cargo geometry at record=" + currentIndex.ToString());
                return false;
            }
            MarkRect(currentGrid, cargoWidth, currentRecord.row, currentRecord.col, currentRecord.width, currentRecord.height, currentIndex + 1);
        }

        if (AllRecordsAtTarget(records))
            return true;

        ref array<int> targetGrid = new array<int>();
        BuildTargetGrid(records, cargoWidth, cargoHeight, targetGrid);

        ref array<int> lockedRecords = new array<int>();
        ref array<int> activeParking = new array<int>();
        ResetGrid(lockedRecords, records.Count());
        ResetGrid(activeParking, records.Count());

        int maxSteps = records.Count() * 32 + 128;
        if (maxSteps < 192)
            maxSteps = 192;
        if (maxSteps > 1024)
            maxSteps = 1024;

        int maxSearch = records.Count() * records.Count() * 8 + 64;
        if (maxSearch < 256)
            maxSearch = 256;
        if (maxSearch > 2048)
            maxSearch = 2048;

        int maxDepth = records.Count() + 4;
        int stepCount = 0;
        int searchCount = 0;
        Print("[TransferZ] Sort planner V3 active records=" + records.Count().ToString() + " maxSteps=" + maxSteps.ToString() + " maxSearch=" + maxSearch.ToString());

        for (int planIndex = 0; planIndex < records.Count(); planIndex++)
        {
            if (!EnsureRecordAtTargetV3(player, source, records, currentGrid, targetGrid, lockedRecords, activeParking, cargoWidth, cargoHeight, moves, planIndex, maxDepth, stepCount, maxSteps, searchCount, maxSearch))
            {
                moves.Clear();
                Print("[TransferZ] Sort planner V3 stopped: target=" + planIndex.ToString() + " steps=" + stepCount.ToString() + "/" + maxSteps.ToString() + " search=" + searchCount.ToString() + "/" + maxSearch.ToString());
                return false;
            }
        }

        if (!AllRecordsAtTarget(records))
        {
            moves.Clear();
            Print("[TransferZ] Sort planner V3 stopped: target layout incomplete");
            return false;
        }

        Print("[TransferZ] Sort planner V3 solved moves=" + moves.Count().ToString() + " steps=" + stepCount.ToString() + " search=" + searchCount.ToString());
        return true;
    }

    override static int Sort(PlayerBase player, EntityAI source)
    {
        if (!player || !source || !TransferZServerService.IsReachable(player, source) || !source.GetInventory().GetCargo())
        {
            Print("[TransferZ] Sort V3 rejected: invalid or unreachable cargo source");
            return -1;
        }

        ref array<ref TransferZSortRecord> records = new array<ref TransferZSortRecord>();
        int cargoWidth;
        int cargoHeight;
        if (!SnapshotSortRecords(source, records, cargoWidth, cargoHeight))
        {
            Print("[TransferZ] Sort V3 rejected: cargo snapshot failed for " + source.GetType());
            return -1;
        }
        if (records.Count() < 2)
            return 0;

        SortRecordsV3(records);

        ref array<ref TransferZSortMove> moves = new array<ref TransferZSortMove>();
        if (!BuildSortPlanV3(player, source, records, cargoWidth, cargoHeight, moves))
        {
            Print("[TransferZ] Sort V3 failed: no bounded in-cargo rearrangement plan for " + source.GetType());
            return -1;
        }
        if (moves.Count() == 0)
            return 0;

        int moved = 0;
        foreach (TransferZSortMove move : moves)
        {
            if (!move || !move.item)
                continue;
            if (!TryMoveWithinCargo(player, source, move.item, move.row, move.col, move.flip))
            {
                Print("[TransferZ] Sort V3 stopped after native move validation failed at move=" + moved.ToString() + "/" + moves.Count().ToString());
                return -1;
            }
            moved++;
        }

        Print("[TransferZ] Sort V3 result source=" + source.GetType() + " moves=" + moved.ToString() + "/" + moves.Count().ToString());
        return moved;
    }

    override static void HandleRequest(PlayerBase player, PlayerIdentity sender, ParamsReadContext ctx)
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
        {
            if (operation == TransferZMaintenanceOperation.SORT)
                SendResult(player, operation, sourceLow, sourceHigh, false);
            return;
        }

        if (operation == TransferZMaintenanceOperation.SORT)
        {
            int sortResult = Sort(player, source);
            SendResult(player, operation, sourceLow, sourceHigh, sortResult >= 0);
            player.UpdateInventoryMenu();
        }
        else if (operation == TransferZMaintenanceOperation.STACK)
        {
            Stack(player, source);
            player.UpdateInventoryMenu();
        }
    }
}
