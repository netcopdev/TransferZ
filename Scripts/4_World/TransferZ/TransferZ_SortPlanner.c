class TransferZSortPlanner : TransferZMaintenanceService
{
    protected static bool SortBeforeV2(TransferZSortRecord left, TransferZSortRecord right)
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

    protected static void SortRecordsV2(notnull array<ref TransferZSortRecord> records)
    {
        for (int recordIndex = 1; recordIndex < records.Count(); recordIndex++)
        {
            ref TransferZSortRecord current = records.Get(recordIndex);
            int previousIndex = recordIndex - 1;
            while (previousIndex >= 0 && SortBeforeV2(current, records.Get(previousIndex)))
            {
                records.Set(previousIndex + 1, records.Get(previousIndex));
                previousIndex--;
            }
            records.Set(previousIndex + 1, current);
        }
    }

    protected static int CountForeignCurrentOverlapsV2(notnull array<ref TransferZSortRecord> records, int recordIndex, int targetRow, int targetCol)
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

    protected static int TargetAssignmentCostV2(notnull array<ref TransferZSortRecord> records, int recordIndex, int targetRow, int targetCol)
    {
        TransferZSortRecord record = records.Get(recordIndex);
        int foreignOverlap = CountForeignCurrentOverlapsV2(records, recordIndex, targetRow, targetCol);
        int distance = Math.AbsInt(record.row - targetRow) + Math.AbsInt(record.col - targetCol);
        return foreignOverlap * 10000 + distance;
    }

    protected static void OptimizeEquivalentTargetAssignmentsV2(notnull array<ref TransferZSortRecord> records)
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

                    int currentCost = TargetAssignmentCostV2(records, leftIndex, left.targetRow, left.targetCol) + TargetAssignmentCostV2(records, rightIndex, right.targetRow, right.targetCol);
                    int swappedCost = TargetAssignmentCostV2(records, leftIndex, right.targetRow, right.targetCol) + TargetAssignmentCostV2(records, rightIndex, left.targetRow, left.targetCol);
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

    protected static void LogTargetsV2(notnull array<ref TransferZSortRecord> records)
    {
        for (int recordIndex = 0; recordIndex < records.Count(); recordIndex++)
        {
            TransferZSortRecord record = records.Get(recordIndex);
            Print("[TransferZ] Sort planner V2 target record=" + recordIndex.ToString() + " type=" + record.item.GetType() + " from=" + record.row.ToString() + "," + record.col.ToString() + " to=" + record.targetRow.ToString() + "," + record.targetCol.ToString() + " size=" + record.width.ToString() + "x" + record.height.ToString());
        }
    }

    protected static bool EnsureRecordAtTargetV2(PlayerBase player, EntityAI source, notnull array<ref TransferZSortRecord> records, notnull array<int> currentGrid, notnull array<int> targetGrid, int cargoWidth, int cargoHeight, notnull array<ref TransferZSortMove> moves, notnull array<int> activeRecords, int recordIndex, inout int stepCount, int maxSteps)
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

            bool blockerCleared = ParkRecord(player, source, records, currentGrid, targetGrid, cargoWidth, cargoHeight, moves, blockerIndex, recordIndex, false, stepCount, maxSteps);
            if (blockerCleared)
            {
                Print("[TransferZ] Sort planner V2 parked blocker=" + blockerIndex.ToString() + " for target=" + recordIndex.ToString());
                clearGuard++;
                continue;
            }

            if (activeRecords.Get(blockerIndex) != 0)
                blockerCleared = BreakActiveCycle(player, source, records, currentGrid, targetGrid, cargoWidth, cargoHeight, moves, activeRecords, blockerIndex, recordIndex, stepCount, maxSteps);
            else
                blockerCleared = EnsureRecordAtTargetV2(player, source, records, currentGrid, targetGrid, cargoWidth, cargoHeight, moves, activeRecords, blockerIndex, stepCount, maxSteps);

            if (!blockerCleared)
            {
                Print("[TransferZ] Sort planner V2 could not relocate blocker=" + blockerIndex.ToString() + " for target=" + recordIndex.ToString());
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

    protected static bool BuildSortPlanV2(PlayerBase player, EntityAI source, notnull array<ref TransferZSortRecord> records, int cargoWidth, int cargoHeight, notnull array<ref TransferZSortMove> moves)
    {
        moves.Clear();
        if (!AssignTargets(player, source, records, cargoWidth, cargoHeight))
        {
            Print("[TransferZ] Sort planner V2 failed: target layout assignment");
            return false;
        }

        OptimizeEquivalentTargetAssignmentsV2(records);
        LogTargetsV2(records);

        ref array<int> currentGrid = new array<int>();
        ResetGrid(currentGrid, cargoWidth * cargoHeight);
        for (int currentIndex = 0; currentIndex < records.Count(); currentIndex++)
        {
            TransferZSortRecord currentRecord = records.Get(currentIndex);
            if (!RectFree(currentGrid, cargoWidth, cargoHeight, currentRecord.row, currentRecord.col, currentRecord.width, currentRecord.height))
            {
                Print("[TransferZ] Sort planner V2 failed: overlapping current cargo geometry at record=" + currentIndex.ToString());
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
        Print("[TransferZ] Sort planner V2 active records=" + records.Count().ToString() + " maxSteps=" + maxSteps.ToString());
        for (int planIndex = 0; planIndex < records.Count(); planIndex++)
        {
            if (!EnsureRecordAtTargetV2(player, source, records, currentGrid, targetGrid, cargoWidth, cargoHeight, moves, activeRecords, planIndex, stepCount, maxSteps))
            {
                moves.Clear();
                Print("[TransferZ] Sort planner V2 stopped: could not clear target for record=" + planIndex.ToString() + " steps=" + stepCount.ToString() + "/" + maxSteps.ToString());
                return false;
            }
        }

        if (!AllRecordsAtTarget(records))
        {
            moves.Clear();
            Print("[TransferZ] Sort planner V2 stopped: target layout incomplete after steps=" + stepCount.ToString());
            return false;
        }

        Print("[TransferZ] Sort planner V2 solved moves=" + moves.Count().ToString() + " steps=" + stepCount.ToString());
        return true;
    }

    override static int Sort(PlayerBase player, EntityAI source)
    {
        if (!player || !source || !TransferZServerService.IsReachable(player, source) || !source.GetInventory().GetCargo())
        {
            Print("[TransferZ] Sort V2 rejected: invalid or unreachable cargo source");
            return -1;
        }

        ref array<ref TransferZSortRecord> records = new array<ref TransferZSortRecord>();
        int cargoWidth;
        int cargoHeight;
        if (!SnapshotSortRecords(source, records, cargoWidth, cargoHeight))
        {
            Print("[TransferZ] Sort V2 rejected: cargo snapshot failed for " + source.GetType());
            return -1;
        }
        if (records.Count() < 2)
            return 0;

        SortRecordsV2(records);

        ref array<ref TransferZSortMove> moves = new array<ref TransferZSortMove>();
        if (!BuildSortPlanV2(player, source, records, cargoWidth, cargoHeight, moves))
        {
            Print("[TransferZ] Sort V2 failed: no safe in-cargo rearrangement plan for " + source.GetType());
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
                Print("[TransferZ] Sort V2 stopped after native move validation failed at move=" + moved.ToString() + "/" + moves.Count().ToString());
                return -1;
            }
            moved++;
        }

        Print("[TransferZ] Sort V2 result source=" + source.GetType() + " moves=" + moved.ToString() + "/" + moves.Count().ToString());
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
