class TransferZExternalStagingSortPlanner : TransferZSortPlanner
{
    protected static bool RecordAtCargoLocation(EntityAI source, TransferZSortRecord record, int row, int col, bool flip)
    {
        if (!source || !record || !record.item)
            return false;

        InventoryLocation current = new InventoryLocation();
        if (!record.item.GetInventory().GetCurrentInventoryLocation(current))
            return false;

        return current.GetType() == InventoryLocationType.CARGO && current.GetParent() == source && current.GetRow() == row && current.GetCol() == col && current.GetFlip() == flip;
    }

    protected static bool RecordAtOriginalLocation(EntityAI source, TransferZSortRecord record)
    {
        return RecordAtCargoLocation(source, record, record.row, record.col, record.flip);
    }

    protected static bool RecordAtTargetLocation(EntityAI source, TransferZSortRecord record, int recordIndex, notnull array<int> targetFlips)
    {
        bool targetFlip = targetFlips.Get(recordIndex) != 0;
        return RecordAtCargoLocation(source, record, record.targetRow, record.targetCol, targetFlip);
    }

    protected static bool VerifyOriginalLayout(EntityAI source, notnull array<ref TransferZSortRecord> records)
    {
        for (int recordIndex = 0; recordIndex < records.Count(); recordIndex++)
        {
            if (!RecordAtOriginalLocation(source, records.Get(recordIndex)))
                return false;
        }
        return true;
    }

    protected static bool VerifyTargetLayout(EntityAI source, notnull array<ref TransferZSortRecord> records, notnull array<int> targetFlips)
    {
        for (int recordIndex = 0; recordIndex < records.Count(); recordIndex++)
        {
            if (!RecordAtTargetLocation(source, records.Get(recordIndex), recordIndex, targetFlips))
                return false;
        }
        return true;
    }

    protected static bool ValidateExactCargoMove(PlayerBase player, EntityAI source, TransferZSortRecord record, int row, int col, bool flip, out InventoryLocation src, out InventoryLocation dst)
    {
        src = null;
        dst = null;

        if (!player || !source || !record || !record.item)
            return false;

        EntityAI item = record.item;
        if (!TransferZServerService.IsReachable(player, source) || !TransferZServerService.IsReachable(player, item))
            return false;
        if (!item.GetInventory().CanRemoveEntity())
            return false;
        if (!source.CanReceiveItemIntoCargo(item))
            return false;

        src = new InventoryLocation();
        if (!item.GetInventory().GetCurrentInventoryLocation(src))
            return false;

        EntityAI sourceParent = src.GetParent();
        if (sourceParent && src.GetType() == InventoryLocationType.CARGO && !sourceParent.CanReleaseCargo(item))
            return false;

        dst = new InventoryLocation();
        dst.SetCargo(source, item, record.cargoIndex, row, col, flip);

        HumanInventory humanInventory = player.GetHumanInventory();
        if (humanInventory && humanInventory.FindCollidingUserReservedLocationIndex(item, dst) >= 0)
            return false;
        if (!GameInventory.CheckMoveToDstRequest(player, src, dst, GameInventory.c_MaxItemDistanceRadius))
            return false;
        if (!GameInventory.LocationCanMoveEntity(src, dst))
            return false;

        return true;
    }

    protected static bool CanMoveToExactCargoLocation(PlayerBase player, EntityAI source, TransferZSortRecord record, int row, int col, bool flip)
    {
        InventoryLocation src;
        InventoryLocation dst;
        return ValidateExactCargoMove(player, source, record, row, col, flip, src, dst);
    }

    protected static bool TryMoveToExactCargoLocation(PlayerBase player, EntityAI source, TransferZSortRecord record, int row, int col, bool flip, string phase)
    {
        InventoryLocation src;
        InventoryLocation dst;
        if (!ValidateExactCargoMove(player, source, record, row, col, flip, src, dst))
        {
            Print("[TransferZ] Sort transactional " + phase + " validation failed item=" + record.item.GetType() + " dst=" + row.ToString() + "," + col.ToString() + " flip=" + flip.ToString());
            return false;
        }

        InventoryMode moveMode = InventoryMode.SERVER;
        if (!GetGame().IsMultiplayer())
            moveMode = InventoryMode.LOCAL;

        bool moved = player.GetInventory().TakeToDst(moveMode, src, dst);
        if (!moved)
            Print("[TransferZ] Sort transactional " + phase + " TakeToDst failed item=" + record.item.GetType() + " dst=" + row.ToString() + "," + col.ToString() + " flip=" + flip.ToString());
        return moved;
    }

    protected static bool StageToVicinity(PlayerBase player, EntityAI item, string phase)
    {
        string itemName = "<null>";
        if (item)
            itemName = item.GetType();

        if (!TransferZServerService.TryMoveToVicinity(player, item))
        {
            Print("[TransferZ] Sort transactional " + phase + " vicinity staging failed item=" + itemName);
            return false;
        }

        InventoryLocation stagedLocation = new InventoryLocation();
        if (!item || !item.GetInventory().GetCurrentInventoryLocation(stagedLocation) || stagedLocation.GetType() != InventoryLocationType.GROUND)
        {
            Print("[TransferZ] Sort transactional " + phase + " staging verification failed item=" + itemName);
            return false;
        }
        if (!TransferZServerService.IsReachable(player, item))
        {
            Print("[TransferZ] Sort transactional " + phase + " staged item became unreachable item=" + itemName);
            return false;
        }

        return true;
    }

    protected static bool RestoreOriginalLayout(PlayerBase player, EntityAI source, notnull array<ref TransferZSortRecord> records)
    {
        Print("[TransferZ] Sort transactional rollback begin items=" + records.Count().ToString());

        int passLimit = 8;
        for (int passIndex = 0; passIndex < passLimit; passIndex++)
        {
            if (VerifyOriginalLayout(source, records))
            {
                Print("[TransferZ] Sort transactional rollback verified pass=" + passIndex.ToString());
                return true;
            }

            bool progress = false;

            for (int clearIndex = 0; clearIndex < records.Count(); clearIndex++)
            {
                TransferZSortRecord clearRecord = records.Get(clearIndex);
                if (RecordAtOriginalLocation(source, clearRecord))
                    continue;
                if (!IsDirectCargoItem(source, clearRecord.item))
                    continue;

                if (StageToVicinity(player, clearRecord.item, "rollback-clear"))
                    progress = true;
            }

            for (int restoreIndex = 0; restoreIndex < records.Count(); restoreIndex++)
            {
                TransferZSortRecord restoreRecord = records.Get(restoreIndex);
                if (RecordAtOriginalLocation(source, restoreRecord))
                    continue;

                if (TryMoveToExactCargoLocation(player, source, restoreRecord, restoreRecord.row, restoreRecord.col, restoreRecord.flip, "rollback-restore"))
                    progress = true;
            }

            if (VerifyOriginalLayout(source, records))
            {
                Print("[TransferZ] Sort transactional rollback verified pass=" + (passIndex + 1).ToString());
                return true;
            }

            if (!progress)
                break;
        }

        int contained = 0;
        for (int containIndex = 0; containIndex < records.Count(); containIndex++)
        {
            TransferZSortRecord containRecord = records.Get(containIndex);
            if (IsDirectCargoItem(source, containRecord.item))
            {
                contained++;
                continue;
            }

            if (TransferZServerService.TryMoveToExactCargo(player, containRecord.item, source))
                contained++;
        }

        bool exact = VerifyOriginalLayout(source, records);
        Print("[TransferZ] Sort transactional CRITICAL rollback incomplete exact=" + exact.ToString() + " contained=" + contained.ToString() + "/" + records.Count().ToString());
        return exact;
    }

    protected static bool BuildTransactionalTargetLayout(PlayerBase player, EntityAI source, notnull array<ref TransferZSortRecord> records, int cargoWidth, int cargoHeight, notnull array<int> targetWidths, notnull array<int> targetHeights, notnull array<int> targetFlips)
    {
        bool assigned = AssignCompactTargetsV4(player, source, records, cargoWidth, cargoHeight, targetWidths, targetHeights, targetFlips, true);
        if (!assigned)
            assigned = AssignFirstFitTargetsV4(player, source, records, cargoWidth, cargoHeight, targetWidths, targetHeights, targetFlips, true);
        if (!assigned)
            assigned = AssignCompactTargetsV4(player, source, records, cargoWidth, cargoHeight, targetWidths, targetHeights, targetFlips);
        if (!assigned)
            assigned = AssignFirstFitTargetsV4(player, source, records, cargoWidth, cargoHeight, targetWidths, targetHeights, targetFlips);

        if (!assigned)
        {
            Print("[TransferZ] Sort transactional failed: target layout assignment");
            return false;
        }

        OptimizeEquivalentTargetAssignmentsV4(records, targetWidths, targetHeights, targetFlips);
        SortRecordsByTargetV4(records, targetWidths, targetHeights, targetFlips);
        return true;
    }

    protected static bool SourceCargoEmpty(EntityAI source)
    {
        if (!source)
            return false;

        CargoBase cargo = source.GetInventory().GetCargo();
        return cargo && cargo.GetItemCount() == 0;
    }

    protected static bool PreflightOriginalMoves(PlayerBase player, EntityAI source, notnull array<ref TransferZSortRecord> records)
    {
        if (!SourceCargoEmpty(source))
        {
            Print("[TransferZ] Sort transactional rollback preflight failed: source cargo not empty");
            return false;
        }

        for (int recordIndex = 0; recordIndex < records.Count(); recordIndex++)
        {
            TransferZSortRecord record = records.Get(recordIndex);
            if (!CanMoveToExactCargoLocation(player, source, record, record.row, record.col, record.flip))
            {
                Print("[TransferZ] Sort transactional rollback preflight failed item=" + record.item.GetType() + " original=" + record.row.ToString() + "," + record.col.ToString() + " flip=" + record.flip.ToString());
                return false;
            }
        }

        return true;
    }

    protected static bool PreflightTargetMoves(PlayerBase player, EntityAI source, notnull array<ref TransferZSortRecord> records, notnull array<int> targetFlips)
    {
        if (!SourceCargoEmpty(source))
        {
            Print("[TransferZ] Sort transactional target preflight failed: source cargo not empty");
            return false;
        }

        for (int recordIndex = 0; recordIndex < records.Count(); recordIndex++)
        {
            TransferZSortRecord record = records.Get(recordIndex);
            bool targetFlip = targetFlips.Get(recordIndex) != 0;
            if (!CanMoveToExactCargoLocation(player, source, record, record.targetRow, record.targetCol, targetFlip))
            {
                Print("[TransferZ] Sort transactional target preflight failed item=" + record.item.GetType() + " target=" + record.targetRow.ToString() + "," + record.targetCol.ToString() + " flip=" + targetFlip.ToString());
                return false;
            }
        }

        return true;
    }

    override static int Sort(PlayerBase player, EntityAI source)
    {
        if (!player || !source || !TransferZServerService.IsReachable(player, source) || !source.GetInventory().GetCargo())
        {
            Print("[TransferZ] Sort transactional rejected: invalid or unreachable cargo source");
            return -1;
        }

        ref array<ref TransferZSortRecord> records = new array<ref TransferZSortRecord>();
        int cargoWidth;
        int cargoHeight;
        if (!SnapshotSortRecords(source, records, cargoWidth, cargoHeight))
        {
            Print("[TransferZ] Sort transactional rejected: cargo snapshot failed for " + source.GetType());
            return -1;
        }
        if (records.Count() < 2)
            return 0;

        SortRecordsV4(records);

        ref array<int> targetWidths = new array<int>();
        ref array<int> targetHeights = new array<int>();
        ref array<int> targetFlips = new array<int>();
        if (!BuildTransactionalTargetLayout(player, source, records, cargoWidth, cargoHeight, targetWidths, targetHeights, targetFlips))
            return -1;

        if (VerifyTargetLayout(source, records, targetFlips))
            return 0;

        int stagedCount = 0;
        for (int stageIndex = 0; stageIndex < records.Count(); stageIndex++)
        {
            TransferZSortRecord stageRecord = records.Get(stageIndex);
            if (!StageToVicinity(player, stageRecord.item, "stage"))
            {
                bool rolledBackAfterStageFailure = RestoreOriginalLayout(player, source, records);
                Print("[TransferZ] Sort transactional failed during staging staged=" + stagedCount.ToString() + "/" + records.Count().ToString() + " rollback=" + rolledBackAfterStageFailure.ToString());
                return -1;
            }
            stagedCount++;
        }

        if (!SourceCargoEmpty(source))
        {
            bool rolledBackAfterEmptyCheck = RestoreOriginalLayout(player, source, records);
            Print("[TransferZ] Sort transactional failed: source did not empty after staging rollback=" + rolledBackAfterEmptyCheck.ToString());
            return -1;
        }

        // Fail closed unless DayZ validates the complete exact original layout from
        // the current staged state. This establishes a tested rollback path before
        // any target placement is allowed to begin.
        if (!PreflightOriginalMoves(player, source, records))
        {
            bool rolledBackAfterRollbackPreflightFailure = RestoreOriginalLayout(player, source, records);
            Print("[TransferZ] Sort transactional failed during rollback preflight rollback=" + rolledBackAfterRollbackPreflightFailure.ToString());
            return -1;
        }

        if (!PreflightTargetMoves(player, source, records, targetFlips))
        {
            bool rolledBackAfterTargetPreflightFailure = RestoreOriginalLayout(player, source, records);
            Print("[TransferZ] Sort transactional failed during target preflight rollback=" + rolledBackAfterTargetPreflightFailure.ToString());
            return -1;
        }

        int placedCount = 0;
        for (int targetIndex = 0; targetIndex < records.Count(); targetIndex++)
        {
            TransferZSortRecord targetRecord = records.Get(targetIndex);
            bool targetFlip = targetFlips.Get(targetIndex) != 0;
            if (!TryMoveToExactCargoLocation(player, source, targetRecord, targetRecord.targetRow, targetRecord.targetCol, targetFlip, "commit"))
            {
                bool rolledBackAfterCommitFailure = RestoreOriginalLayout(player, source, records);
                Print("[TransferZ] Sort transactional failed during commit placed=" + placedCount.ToString() + "/" + records.Count().ToString() + " rollback=" + rolledBackAfterCommitFailure.ToString());
                return -1;
            }
            placedCount++;
        }

        if (!VerifyTargetLayout(source, records, targetFlips))
        {
            bool rolledBackAfterVerifyFailure = RestoreOriginalLayout(player, source, records);
            Print("[TransferZ] Sort transactional failed target verification rollback=" + rolledBackAfterVerifyFailure.ToString());
            return -1;
        }
        return placedCount;
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
