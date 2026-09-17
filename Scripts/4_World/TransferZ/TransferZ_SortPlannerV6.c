class TransferZSortPlannerStateV6 : TransferZSortPlannerStateV5
{
    ref array<int> parkingGuardGrid;
}

class TransferZSortPlannerV6 : TransferZSortPlannerV5
{
    protected static bool GuardRectFreeV6(notnull array<int> guardGrid, int cargoWidth, int cargoHeight, int row, int col, int itemWidth, int itemHeight)
    {
        if (row < 0 || col < 0 || row + itemHeight > cargoHeight || col + itemWidth > cargoWidth)
            return false;

        for (int guardRow = row; guardRow < row + itemHeight; guardRow++)
        {
            for (int guardCol = col; guardCol < col + itemWidth; guardCol++)
            {
                if (guardGrid.Get(guardRow * cargoWidth + guardCol) != 0)
                    return false;
            }
        }
        return true;
    }

    protected static void AdjustGuardRectV6(notnull array<int> guardGrid, int cargoWidth, int row, int col, int itemWidth, int itemHeight, int delta)
    {
        for (int guardRow = row; guardRow < row + itemHeight; guardRow++)
        {
            for (int guardCol = col; guardCol < col + itemWidth; guardCol++)
            {
                int index = guardRow * cargoWidth + guardCol;
                guardGrid.Set(index, guardGrid.Get(index) + delta);
            }
        }
    }

    protected static string ParkingStateKeyV6(notnull TransferZSortPlannerStateV6 state, int recordIndex, int protectedTargetIndex)
    {
        string key = ParkingStateKeyV5(state, recordIndex, protectedTargetIndex) + ":g:";
        for (int gridIndex = 0; gridIndex < state.parkingGuardGrid.Count(); gridIndex++)
        {
            if (state.parkingGuardGrid.Get(gridIndex) != 0)
                key += gridIndex.ToString() + ",";
        }
        return key;
    }

    protected static bool FindTemporaryPlacementV6(notnull TransferZSortPlannerStateV6 state, int recordIndex, int protectedTargetIndex, bool requireTargetOverlapImprovement, out int bestRow, out int bestCol, out int bestWidth, out int bestHeight, out bool bestFlip)
    {
        bestRow = -1;
        bestCol = -1;
        bestWidth = 0;
        bestHeight = 0;
        bestFlip = false;

        TransferZSortRecord record = state.records.Get(recordIndex);
        int currentOverlap = CountTargetOverlapV4(state.targetGrid, state.cargoWidth, record.row, record.col, record.width, record.height);
        int bestOverlap = 1000000;

        TransferZSortRecord protectedTarget;
        int protectedWidth = 0;
        int protectedHeight = 0;
        if (protectedTargetIndex >= 0)
        {
            protectedTarget = state.records.Get(protectedTargetIndex);
            protectedWidth = state.targetWidths.Get(protectedTargetIndex);
            protectedHeight = state.targetHeights.Get(protectedTargetIndex);
        }

        int orientationCount = OrientationCountV4(record);
        for (int orientationIndex = 0; orientationIndex < orientationCount; orientationIndex++)
        {
            int itemWidth;
            int itemHeight;
            bool itemFlip;
            GetOrientationV4(record, orientationIndex, itemWidth, itemHeight, itemFlip);

            for (int tempRow = state.cargoHeight - itemHeight; tempRow >= 0; tempRow--)
            {
                for (int tempCol = state.cargoWidth - itemWidth; tempCol >= 0; tempCol--)
                {
                    if (tempRow == record.row && tempCol == record.col && itemFlip == record.flip)
                        continue;
                    if (tempRow == record.targetRow && tempCol == record.targetCol)
                        continue;
                    if (protectedTarget && RectOverlaps(tempRow, tempCol, itemWidth, itemHeight, protectedTarget.targetRow, protectedTarget.targetCol, protectedWidth, protectedHeight))
                        continue;
                    if (!GuardRectFreeV6(state.parkingGuardGrid, state.cargoWidth, state.cargoHeight, tempRow, tempCol, itemWidth, itemHeight))
                        continue;
                    if (!RectFree(state.currentGrid, state.cargoWidth, state.cargoHeight, tempRow, tempCol, itemWidth, itemHeight, recordIndex + 1))
                        continue;

                    int targetOverlap = CountTargetOverlapV4(state.targetGrid, state.cargoWidth, tempRow, tempCol, itemWidth, itemHeight);
                    if (requireTargetOverlapImprovement && targetOverlap >= currentOverlap)
                        continue;
                    if (targetOverlap > bestOverlap)
                        continue;
                    if (targetOverlap == bestOverlap && bestRow >= 0 && itemFlip != record.flip)
                        continue;
                    if (CollidesWithUserReservationV4(state.player, state.source, record, tempRow, tempCol, itemFlip))
                        continue;

                    bestOverlap = targetOverlap;
                    bestRow = tempRow;
                    bestCol = tempCol;
                    bestWidth = itemWidth;
                    bestHeight = itemHeight;
                    bestFlip = itemFlip;
                    if (targetOverlap == 0 && itemFlip == record.flip)
                        return true;
                }
            }
        }

        return bestRow >= 0;
    }

    protected static bool ParkRecordRecursiveV6(notnull TransferZSortPlannerStateV6 state, int recordIndex, int protectedTargetIndex, int depth)
    {
        if (recordIndex < 0 || recordIndex >= state.records.Count())
            return false;
        if (state.lockedRecords.Get(recordIndex) != 0 || state.activeParking.Get(recordIndex) != 0)
            return false;
        if (depth > state.maxDepth || state.stepCount >= state.maxSteps || state.searchCount >= state.maxSearch)
            return false;

        string entryKey = ParkingStateKeyV6(state, recordIndex, protectedTargetIndex);
        if (IsDeadParkingStateV5(state, entryKey))
            return false;

        state.searchCount++;
        state.activeParking.Set(recordIndex, 1);

        int directRow;
        int directCol;
        int directWidth;
        int directHeight;
        bool directFlip;
        if (FindTemporaryPlacementV6(state, recordIndex, protectedTargetIndex, false, directRow, directCol, directWidth, directHeight, directFlip))
        {
            TransferZSortRecord directRecord = state.records.Get(recordIndex);
            AddPlannedMoveV4(state.moves, directRecord, directRow, directCol, directFlip);
            MoveRecordInGridV4(state.currentGrid, state.cargoWidth, directRecord, recordIndex + 1, directRow, directCol, directWidth, directHeight, directFlip);
            state.stepCount++;
            state.activeParking.Set(recordIndex, 0);
            return true;
        }

        TransferZSortRecord record = state.records.Get(recordIndex);
        TransferZSortRecord protectedTarget;
        int protectedWidth = 0;
        int protectedHeight = 0;
        if (protectedTargetIndex >= 0)
        {
            protectedTarget = state.records.Get(protectedTargetIndex);
            protectedWidth = state.targetWidths.Get(protectedTargetIndex);
            protectedHeight = state.targetHeights.Get(protectedTargetIndex);
        }

        int orientationCount = OrientationCountV4(record);
        for (int blockerLimit = 1; blockerLimit <= state.records.Count(); blockerLimit++)
        {
            for (int orientationIndex = 0; orientationIndex < orientationCount; orientationIndex++)
            {
                int candidateWidth;
                int candidateHeight;
                bool candidateFlip;
                GetOrientationV4(record, orientationIndex, candidateWidth, candidateHeight, candidateFlip);

                for (int candidateRow = state.cargoHeight - candidateHeight; candidateRow >= 0; candidateRow--)
                {
                    for (int candidateCol = state.cargoWidth - candidateWidth; candidateCol >= 0; candidateCol--)
                    {
                        if (state.searchCount >= state.maxSearch || state.stepCount >= state.maxSteps)
                            break;
                        if (candidateRow == record.row && candidateCol == record.col && candidateFlip == record.flip)
                            continue;
                        if (candidateRow == record.targetRow && candidateCol == record.targetCol)
                            continue;
                        if (protectedTarget && RectOverlaps(candidateRow, candidateCol, candidateWidth, candidateHeight, protectedTarget.targetRow, protectedTarget.targetCol, protectedWidth, protectedHeight))
                            continue;
                        if (!GuardRectFreeV6(state.parkingGuardGrid, state.cargoWidth, state.cargoHeight, candidateRow, candidateCol, candidateWidth, candidateHeight))
                            continue;
                        if (CollidesWithUserReservationV4(state.player, state.source, record, candidateRow, candidateCol, candidateFlip))
                            continue;

                        ref array<int> blockers = new array<int>();
                        CollectBlockersV4(state.currentGrid, state.cargoWidth, candidateRow, candidateCol, candidateWidth, candidateHeight, recordIndex + 1, blockers);
                        if (blockers.Count() != blockerLimit)
                            continue;

                        bool candidateBlockedByLocked = false;
                        bool candidateBlockedByActive = false;
                        foreach (int blockerIndex : blockers)
                        {
                            if (state.lockedRecords.Get(blockerIndex) != 0)
                                candidateBlockedByLocked = true;
                            if (state.activeParking.Get(blockerIndex) != 0)
                                candidateBlockedByActive = true;
                        }
                        if (candidateBlockedByLocked || candidateBlockedByActive)
                            continue;

                        int savedMoveCount = state.moves.Count();
                        int savedStepCount = state.stepCount;
                        ref array<int> savedRows = new array<int>();
                        ref array<int> savedCols = new array<int>();
                        ref array<int> savedWidths = new array<int>();
                        ref array<int> savedHeights = new array<int>();
                        ref array<int> savedFlips = new array<int>();
                        CapturePlannerStateV4(state.records, savedRows, savedCols, savedWidths, savedHeights, savedFlips);

                        AdjustGuardRectV6(state.parkingGuardGrid, state.cargoWidth, candidateRow, candidateCol, candidateWidth, candidateHeight, 1);
                        bool cleared = true;
                        foreach (int recursiveBlocker : blockers)
                        {
                            if (!ParkRecordRecursiveV6(state, recursiveBlocker, protectedTargetIndex, depth + 1))
                            {
                                cleared = false;
                                break;
                            }
                        }
                        AdjustGuardRectV6(state.parkingGuardGrid, state.cargoWidth, candidateRow, candidateCol, candidateWidth, candidateHeight, -1);

                        if (cleared && RectFree(state.currentGrid, state.cargoWidth, state.cargoHeight, candidateRow, candidateCol, candidateWidth, candidateHeight, recordIndex + 1))
                        {
                            AddPlannedMoveV4(state.moves, record, candidateRow, candidateCol, candidateFlip);
                            MoveRecordInGridV4(state.currentGrid, state.cargoWidth, record, recordIndex + 1, candidateRow, candidateCol, candidateWidth, candidateHeight, candidateFlip);
                            state.stepCount++;
                            state.activeParking.Set(recordIndex, 0);
                            return true;
                        }

                        RestorePlannerStateV4(state.records, state.currentGrid, state.cargoWidth, state.cargoHeight, state.moves, savedMoveCount, savedRows, savedCols, savedWidths, savedHeights, savedFlips);
                        state.stepCount = savedStepCount;
                    }
                }
            }
        }

        state.activeParking.Set(recordIndex, 0);
        RememberDeadParkingStateV5(state, entryKey);
        return false;
    }

    protected static bool EnsureRecordAtTargetV6(notnull TransferZSortPlannerStateV6 state, int recordIndex)
    {
        TransferZSortRecord record = state.records.Get(recordIndex);
        if (RecordAtTargetV4(record, recordIndex, state.targetFlips))
        {
            state.lockedRecords.Set(recordIndex, 1);
            return true;
        }

        int targetWidth = state.targetWidths.Get(recordIndex);
        int targetHeight = state.targetHeights.Get(recordIndex);
        bool targetFlip = state.targetFlips.Get(recordIndex) != 0;

        int clearGuard = 0;
        int clearGuardLimit = state.records.Count() * 8 + 16;
        while (!RectFree(state.currentGrid, state.cargoWidth, state.cargoHeight, record.targetRow, record.targetCol, targetWidth, targetHeight, recordIndex + 1))
        {
            if (clearGuard >= clearGuardLimit || state.stepCount >= state.maxSteps || state.searchCount >= state.maxSearch)
                return false;

            int blockerIndex = FindBlockerForTargetV4(state.records, state.currentGrid, state.targetWidths, state.targetHeights, state.cargoWidth, recordIndex);
            if (blockerIndex < 0 || state.lockedRecords.Get(blockerIndex) != 0)
                return false;

            if (!ParkRecordRecursiveV6(state, blockerIndex, recordIndex, 0))
            {
                Print("[TransferZ] Sort planner V6 could not evacuate blocker=" + blockerIndex.ToString() + " for target=" + recordIndex.ToString());
                return false;
            }
            clearGuard++;
        }

        if (CollidesWithUserReservationV4(state.player, state.source, record, record.targetRow, record.targetCol, targetFlip))
            return false;

        AddPlannedMoveV4(state.moves, record, record.targetRow, record.targetCol, targetFlip);
        MoveRecordInGridV4(state.currentGrid, state.cargoWidth, record, recordIndex + 1, record.targetRow, record.targetCol, targetWidth, targetHeight, targetFlip);
        state.stepCount++;
        state.lockedRecords.Set(recordIndex, 1);
        return true;
    }

    protected static bool BuildSortPlanV6(PlayerBase player, EntityAI source, notnull array<ref TransferZSortRecord> records, int cargoWidth, int cargoHeight, notnull array<ref TransferZSortMove> moves)
    {
        moves.Clear();

        ref array<int> targetWidths = new array<int>();
        ref array<int> targetHeights = new array<int>();
        ref array<int> targetFlips = new array<int>();

        if (!AssignCompactTargetsV4(player, source, records, cargoWidth, cargoHeight, targetWidths, targetHeights, targetFlips))
        {
            Print("[TransferZ] Sort planner V6 compact target packing failed; retrying rotation-aware first-fit targets");
            if (!AssignFirstFitTargetsV4(player, source, records, cargoWidth, cargoHeight, targetWidths, targetHeights, targetFlips))
            {
                Print("[TransferZ] Sort planner V6 failed: target layout assignment");
                return false;
            }
        }

        OptimizeEquivalentTargetAssignmentsV4(records, targetWidths, targetHeights, targetFlips);
        SortRecordsForPlanningV5(records, targetWidths, targetHeights, targetFlips);
        LogTargetsV4(records, targetWidths, targetHeights, targetFlips);

        ref TransferZSortPlannerStateV6 state = new TransferZSortPlannerStateV6();
        state.player = player;
        state.source = source;
        state.records = records;
        state.currentGrid = new array<int>();
        state.targetGrid = new array<int>();
        state.targetWidths = targetWidths;
        state.targetHeights = targetHeights;
        state.targetFlips = targetFlips;
        state.lockedRecords = new array<int>();
        state.activeParking = new array<int>();
        state.deadParkingStates = new array<string>();
        state.parkingGuardGrid = new array<int>();
        state.moves = moves;
        state.cargoWidth = cargoWidth;
        state.cargoHeight = cargoHeight;

        ResetGrid(state.currentGrid, cargoWidth * cargoHeight);
        ResetGrid(state.parkingGuardGrid, cargoWidth * cargoHeight);
        for (int currentIndex = 0; currentIndex < records.Count(); currentIndex++)
        {
            TransferZSortRecord currentRecord = records.Get(currentIndex);
            if (!RectFree(state.currentGrid, cargoWidth, cargoHeight, currentRecord.row, currentRecord.col, currentRecord.width, currentRecord.height))
            {
                Print("[TransferZ] Sort planner V6 failed: overlapping current cargo geometry at record=" + currentIndex.ToString());
                return false;
            }
            MarkRect(state.currentGrid, cargoWidth, currentRecord.row, currentRecord.col, currentRecord.width, currentRecord.height, currentIndex + 1);
        }

        if (AllRecordsAtTargetV4(records, targetFlips))
            return true;

        BuildTargetGridV4(records, targetWidths, targetHeights, cargoWidth, cargoHeight, state.targetGrid);
        ResetGrid(state.lockedRecords, records.Count());
        ResetGrid(state.activeParking, records.Count());

        state.maxSteps = records.Count() * 32 + 128;
        if (state.maxSteps < 192)
            state.maxSteps = 192;
        if (state.maxSteps > 1024)
            state.maxSteps = 1024;

        state.maxSearch = records.Count() * records.Count() * 8 + 64;
        if (state.maxSearch < 256)
            state.maxSearch = 256;
        if (state.maxSearch > 2048)
            state.maxSearch = 2048;

        state.maxDepth = records.Count() + 4;
        state.stepCount = 0;
        state.searchCount = 0;
        Print("[TransferZ] Sort planner V6 active records=" + records.Count().ToString() + " maxSteps=" + state.maxSteps.ToString() + " maxSearch=" + state.maxSearch.ToString());

        for (int planIndex = 0; planIndex < records.Count(); planIndex++)
        {
            if (!EnsureRecordAtTargetV6(state, planIndex))
            {
                moves.Clear();
                Print("[TransferZ] Sort planner V6 stopped: target=" + planIndex.ToString() + " steps=" + state.stepCount.ToString() + "/" + state.maxSteps.ToString() + " search=" + state.searchCount.ToString() + "/" + state.maxSearch.ToString() + " deadStates=" + state.deadParkingStates.Count().ToString());
                return false;
            }
        }

        if (!AllRecordsAtTargetV4(records, targetFlips))
        {
            moves.Clear();
            Print("[TransferZ] Sort planner V6 stopped: target layout incomplete");
            return false;
        }

        Print("[TransferZ] Sort planner V6 solved moves=" + moves.Count().ToString() + " steps=" + state.stepCount.ToString() + " search=" + state.searchCount.ToString() + " deadStates=" + state.deadParkingStates.Count().ToString());
        return true;
    }

    override static int Sort(PlayerBase player, EntityAI source)
    {
        if (!player || !source || !TransferZServerService.IsReachable(player, source) || !source.GetInventory().GetCargo())
        {
            Print("[TransferZ] Sort V6 rejected: invalid or unreachable cargo source");
            return -1;
        }

        ref array<ref TransferZSortRecord> records = new array<ref TransferZSortRecord>();
        int cargoWidth;
        int cargoHeight;
        if (!SnapshotSortRecords(source, records, cargoWidth, cargoHeight))
        {
            Print("[TransferZ] Sort V6 rejected: cargo snapshot failed for " + source.GetType());
            return -1;
        }
        if (records.Count() < 2)
            return 0;

        SortRecordsV4(records);

        ref array<ref TransferZSortMove> moves = new array<ref TransferZSortMove>();
        if (!BuildSortPlanV6(player, source, records, cargoWidth, cargoHeight, moves))
        {
            Print("[TransferZ] Sort V6 failed: no bounded in-cargo rearrangement plan for " + source.GetType());
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
                Print("[TransferZ] Sort V6 stopped after native move validation failed at move=" + moved.ToString() + "/" + moves.Count().ToString());
                return -1;
            }
            moved++;
        }

        Print("[TransferZ] Sort V6 result source=" + source.GetType() + " moves=" + moved.ToString() + "/" + moves.Count().ToString());
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
