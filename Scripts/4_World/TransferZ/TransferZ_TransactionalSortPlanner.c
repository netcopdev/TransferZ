class TransferZSortRollbackMove
{
    EntityAI item;
    int row;
    int col;
    bool flip;
    int forwardRow;
    int forwardCol;
    bool forwardFlip;

    EntityAI swapItem;
    int swapRow;
    int swapCol;
    bool swapFlip;
    int swapForwardRow;
    int swapForwardCol;
    bool swapForwardFlip;
}

class TransferZTransactionalSortPlanner : TransferZSortPlanner
{
    protected static TransferZSortRecord CloneSortRecord(TransferZSortRecord sourceRecord)
    {
        if (!sourceRecord)
            return null;

        ref TransferZSortRecord clone = new TransferZSortRecord();
        clone.item = sourceRecord.item;
        clone.row = sourceRecord.row;
        clone.col = sourceRecord.col;
        clone.width = sourceRecord.width;
        clone.height = sourceRecord.height;
        clone.flip = sourceRecord.flip;
        clone.cargoIndex = sourceRecord.cargoIndex;
        clone.typeHash = sourceRecord.typeHash;
        clone.targetRow = sourceRecord.targetRow;
        clone.targetCol = sourceRecord.targetCol;
        return clone;
    }

    protected static void CloneSortRecords(notnull array<ref TransferZSortRecord> sourceRecords, notnull array<ref TransferZSortRecord> destinationRecords)
    {
        destinationRecords.Clear();
        foreach (TransferZSortRecord sourceRecord : sourceRecords)
        {
            TransferZSortRecord clone = CloneSortRecord(sourceRecord);
            if (clone)
                destinationRecords.Insert(clone);
        }
    }

    protected static bool RecordAtCargoLocation(EntityAI source, TransferZSortRecord record)
    {
        if (!source || !record || !record.item)
            return false;

        InventoryLocation current = new InventoryLocation();
        if (!record.item.GetInventory().GetCurrentInventoryLocation(current))
            return false;

        return current.GetType() == InventoryLocationType.CARGO && current.GetParent() == source && current.GetRow() == record.row && current.GetCol() == record.col && current.GetFlip() == record.flip;
    }

    protected static bool VerifyLayout(EntityAI source, notnull array<ref TransferZSortRecord> expectedRecords)
    {
        if (!source)
            return false;

        CargoBase cargo = source.GetInventory().GetCargo();
        if (!cargo || cargo.GetItemCount() != expectedRecords.Count())
            return false;

        foreach (TransferZSortRecord expectedRecord : expectedRecords)
        {
            if (!RecordAtCargoLocation(source, expectedRecord))
                return false;
        }
        return true;
    }

    protected static bool CaptureRollbackMove(EntityAI source, EntityAI item, out TransferZSortRollbackMove rollbackMove)
    {
        rollbackMove = null;
        if (!source || !item)
            return false;

        InventoryLocation current = new InventoryLocation();
        if (!item.GetInventory().GetCurrentInventoryLocation(current))
            return false;
        if (current.GetType() != InventoryLocationType.CARGO || current.GetParent() != source)
            return false;

        rollbackMove = new TransferZSortRollbackMove();
        rollbackMove.item = item;
        rollbackMove.row = current.GetRow();
        rollbackMove.col = current.GetCol();
        rollbackMove.flip = current.GetFlip();
        return true;
    }

    protected static bool ItemAtCargoCoordinates(EntityAI source, EntityAI item, int row, int col, bool flip)
    {
        if (!source || !item)
            return false;

        InventoryLocation current = new InventoryLocation();
        if (!item.GetInventory().GetCurrentInventoryLocation(current))
            return false;

        return current.GetType() == InventoryLocationType.CARGO && current.GetParent() == source && current.GetRow() == row && current.GetCol() == col && current.GetFlip() == flip;
    }

    protected static bool ItemAtRollbackLocation(EntityAI source, TransferZSortRollbackMove rollbackMove)
    {
        if (!rollbackMove)
            return false;
        return ItemAtCargoCoordinates(source, rollbackMove.item, rollbackMove.row, rollbackMove.col, rollbackMove.flip);
    }

    protected static bool ItemAtForwardLocation(EntityAI source, TransferZSortRollbackMove rollbackMove)
    {
        if (!rollbackMove)
            return false;
        return ItemAtCargoCoordinates(source, rollbackMove.item, rollbackMove.forwardRow, rollbackMove.forwardCol, rollbackMove.forwardFlip);
    }

    protected static bool SwapAtRollbackLocations(EntityAI source, TransferZSortRollbackMove rollbackMove)
    {
        if (!rollbackMove || !rollbackMove.swapItem)
            return false;
        return ItemAtCargoCoordinates(source, rollbackMove.item, rollbackMove.row, rollbackMove.col, rollbackMove.flip) && ItemAtCargoCoordinates(source, rollbackMove.swapItem, rollbackMove.swapRow, rollbackMove.swapCol, rollbackMove.swapFlip);
    }

    protected static bool SwapAtForwardLocations(EntityAI source, TransferZSortRollbackMove rollbackMove)
    {
        if (!rollbackMove || !rollbackMove.swapItem)
            return false;
        return ItemAtCargoCoordinates(source, rollbackMove.item, rollbackMove.forwardRow, rollbackMove.forwardCol, rollbackMove.forwardFlip) && ItemAtCargoCoordinates(source, rollbackMove.swapItem, rollbackMove.swapForwardRow, rollbackMove.swapForwardCol, rollbackMove.swapForwardFlip);
    }

    protected static bool CaptureSwapRollbackMove(EntityAI source, EntityAI item1, EntityAI item2, out TransferZSortRollbackMove rollbackMove)
    {
        rollbackMove = null;
        if (!source || !item1 || !item2)
            return false;

        InventoryLocation first = new InventoryLocation();
        InventoryLocation second = new InventoryLocation();
        if (!item1.GetInventory().GetCurrentInventoryLocation(first) || !item2.GetInventory().GetCurrentInventoryLocation(second))
            return false;
        if (first.GetType() != InventoryLocationType.CARGO || second.GetType() != InventoryLocationType.CARGO)
            return false;
        if (first.GetParent() != source || second.GetParent() != source)
            return false;

        rollbackMove = new TransferZSortRollbackMove();
        rollbackMove.item = item1;
        rollbackMove.row = first.GetRow();
        rollbackMove.col = first.GetCol();
        rollbackMove.flip = first.GetFlip();
        rollbackMove.swapItem = item2;
        rollbackMove.swapRow = second.GetRow();
        rollbackMove.swapCol = second.GetCol();
        rollbackMove.swapFlip = second.GetFlip();

        // Native ordinary swap exchanges locations while preserving each item's
        // own cargo orientation.
        rollbackMove.forwardRow = rollbackMove.swapRow;
        rollbackMove.forwardCol = rollbackMove.swapCol;
        rollbackMove.forwardFlip = rollbackMove.flip;
        rollbackMove.swapForwardRow = rollbackMove.row;
        rollbackMove.swapForwardCol = rollbackMove.col;
        rollbackMove.swapForwardFlip = rollbackMove.swapFlip;
        return true;
    }

    protected static bool RollbackExecutedMoves(PlayerBase player, EntityAI source, notnull array<ref TransferZSortRollbackMove> rollbackMoves, notnull array<ref TransferZSortRecord> originalRecords)
    {
        bool reverseMovesAccepted = true;

        for (int rollbackIndex = rollbackMoves.Count() - 1; rollbackIndex >= 0; rollbackIndex--)
        {
            TransferZSortRollbackMove rollbackMove = rollbackMoves.Get(rollbackIndex);
            if (!rollbackMove || !rollbackMove.item)
            {
                reverseMovesAccepted = false;
                continue;
            }

            if (rollbackMove.swapItem)
            {
                if (SwapAtRollbackLocations(source, rollbackMove))
                    continue;
                if (!SwapAtForwardLocations(source, rollbackMove))
                {
                    reverseMovesAccepted = false;
                    continue;
                }

                if (!TrySwapWithinCargo(player, source, rollbackMove.item, rollbackMove.swapItem) || !SwapAtRollbackLocations(source, rollbackMove))
                    reverseMovesAccepted = false;
                continue;
            }

            if (ItemAtRollbackLocation(source, rollbackMove))
                continue;

            // Reverse only the exact forward state this entry produced. If the
            // item is somewhere unexpected, fail closed rather than moving an
            // unknown state and making recovery less deterministic.
            if (!ItemAtForwardLocation(source, rollbackMove))
            {
                reverseMovesAccepted = false;
                continue;
            }

            if (!TryMoveWithinCargo(player, source, rollbackMove.item, rollbackMove.row, rollbackMove.col, rollbackMove.flip))
                reverseMovesAccepted = false;
        }

        bool exact = VerifyLayout(source, originalRecords);
        if (!exact)
            Print("[TransferZ] Sort transactional CRITICAL rollback incomplete reverseAccepted=" + reverseMovesAccepted.ToString() + " moves=" + rollbackMoves.Count().ToString());
        return exact;
    }

    protected static bool ExecutePlannedMove(PlayerBase player, EntityAI source, TransferZSortMove move, notnull array<ref TransferZSortRollbackMove> rollbackMoves)
    {
        if (!move || !move.item)
            return false;

        if (move.swapItem)
        {
            TransferZSortRollbackMove swapRollback;
            if (!CaptureSwapRollbackMove(source, move.item, move.swapItem, swapRollback))
                return false;

            bool swapped = TrySwapWithinCargo(player, source, move.item, move.swapItem);
            if (!swapped)
            {
                if (SwapAtForwardLocations(source, swapRollback))
                    rollbackMoves.Insert(swapRollback);
                return false;
            }

            rollbackMoves.Insert(swapRollback);
            return SwapAtForwardLocations(source, swapRollback);
        }

        TransferZSortRollbackMove rollbackMove;
        if (!CaptureRollbackMove(source, move.item, rollbackMove))
            return false;

        rollbackMove.forwardRow = move.row;
        rollbackMove.forwardCol = move.col;
        rollbackMove.forwardFlip = move.flip;

        bool moved = TryMoveWithinCargo(player, source, move.item, move.row, move.col, move.flip);
        if (!moved)
        {
            // Native false should mean no mutation. If the exact destination did
            // commit anyway, retain the inverse so the caller can still unwind it.
            if (ItemAtForwardLocation(source, rollbackMove))
                rollbackMoves.Insert(rollbackMove);
            return false;
        }

        rollbackMoves.Insert(rollbackMove);
        return ItemAtForwardLocation(source, rollbackMove);
    }

    override static int Sort(PlayerBase player, EntityAI source)
    {
        if (!player || !source || !TransferZServerService.IsReachable(player, source) || !source.GetInventory().GetCargo())
        {
            Print("[TransferZ] Sort transactional rejected: invalid or unreachable cargo source");
            return -1;
        }

        ref array<ref TransferZSortRecord> originalRecords = new array<ref TransferZSortRecord>();
        int cargoWidth;
        int cargoHeight;
        if (!SnapshotSortRecords(source, originalRecords, cargoWidth, cargoHeight))
        {
            Print("[TransferZ] Sort transactional rejected: cargo snapshot failed for " + source.GetType());
            return -1;
        }
        if (originalRecords.Count() < 2)
            return 0;

        SortRecordsV4(originalRecords);

        // Planning operates only on clones. BuildSortPlanV4 mutates its virtual
        // record geometry while solving blockers/cycles, so the authoritative
        // original snapshot remains untouched for rollback verification.
        ref array<ref TransferZSortRecord> plannedRecords = new array<ref TransferZSortRecord>();
        CloneSortRecords(originalRecords, plannedRecords);
        if (plannedRecords.Count() != originalRecords.Count())
        {
            Print("[TransferZ] Sort transactional rejected: planning snapshot clone failed");
            return -1;
        }

        ref array<ref TransferZSortMove> moves = new array<ref TransferZSortMove>();
        if (!BuildSortPlanV4(player, source, plannedRecords, cargoWidth, cargoHeight, moves))
        {
            Print("[TransferZ] Sort transactional failed: no bounded in-cargo rearrangement plan for " + source.GetType());
            return -1;
        }
        if (moves.Count() == 0)
            return 0;

        // The planner is purely virtual. Refuse to execute if anything changed
        // after the original snapshot was taken.
        if (!VerifyLayout(source, originalRecords))
        {
            Print("[TransferZ] Sort transactional rejected: source changed before execution");
            return -1;
        }

        ref array<ref TransferZSortRollbackMove> rollbackMoves = new array<ref TransferZSortRollbackMove>();
        int moved = 0;
        foreach (TransferZSortMove move : moves)
        {
            if (!ExecutePlannedMove(player, source, move, rollbackMoves))
            {
                bool rolledBackAfterMoveFailure = RollbackExecutedMoves(player, source, rollbackMoves, originalRecords);
                Print("[TransferZ] Sort transactional failed during in-cargo execution moved=" + moved.ToString() + "/" + moves.Count().ToString() + " rollback=" + rolledBackAfterMoveFailure.ToString());
                return -1;
            }
            moved++;
        }

        if (!VerifyLayout(source, plannedRecords))
        {
            bool rolledBackAfterVerifyFailure = RollbackExecutedMoves(player, source, rollbackMoves, originalRecords);
            Print("[TransferZ] Sort transactional failed target verification rollback=" + rolledBackAfterVerifyFailure.ToString());
            return -1;
        }

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
