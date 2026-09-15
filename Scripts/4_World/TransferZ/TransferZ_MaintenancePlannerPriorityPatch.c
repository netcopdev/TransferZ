modded class TransferZMaintenanceService
{
    override static bool EnsureRecordAtTarget(PlayerBase player, EntityAI source, notnull array<ref TransferZSortRecord> records, notnull array<int> currentGrid, notnull array<int> targetGrid, int cargoWidth, int cargoHeight, notnull array<ref TransferZSortMove> moves, notnull array<int> activeRecords, int recordIndex, inout int stepCount, int maxSteps)
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

            // The target order is largest-first. When a later/lower-priority item blocks
            // an earlier target, do not chase that smaller item's own final target first.
            // Park it directly in safe free space, clear the higher-priority target, then
            // place the parked item later when its turn comes. This avoids dependency
            // chains where small items repeatedly occupy the staging space needed by a
            // large item at the top of the cargo grid.
            if (blockerIndex > recordIndex)
            {
                blockerCleared = ParkRecord(player, source, records, currentGrid, targetGrid, cargoWidth, cargoHeight, moves, blockerIndex, recordIndex, false, stepCount, maxSteps);
                if (blockerCleared)
                    Print("[TransferZ] Sort planner priority-park blocker=" + blockerIndex.ToString() + " for target=" + recordIndex.ToString());
            }

            if (!blockerCleared)
            {
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
}
