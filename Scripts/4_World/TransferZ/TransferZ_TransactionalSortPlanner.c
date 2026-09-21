class TransferZSortRollbackMove
{
    EntityAI item;
    int row;
    int col;
    bool flip;
    int cargoIndex;
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

        return TransferZCargo.LocationMatches(current, source, record.cargoIndex) && current.GetRow() == record.row && current.GetCol() == record.col && current.GetFlip() == record.flip;
    }

    protected static bool VerifyLayout(EntityAI source, notnull array<ref TransferZSortRecord> expectedRecords)
    {
        if (!source)
            return false;
        if (expectedRecords.Count() == 0)
            return true;

        TransferZSortRecord firstRecord = expectedRecords.Get(0);
        if (!firstRecord)
            return false;

        int cargoIndex = firstRecord.cargoIndex;
        CargoBase cargo = TransferZCargo.Get(source, cargoIndex);
        if (!cargo || cargo.GetItemCount() != expectedRecords.Count())
            return false;

        foreach (TransferZSortRecord expectedRecord : expectedRecords)
        {
            if (!expectedRecord || expectedRecord.cargoIndex != cargoIndex || !RecordAtCargoLocation(source, expectedRecord))
                return false;
        }
        return true;
    }

    protected static bool CaptureRollbackMove(EntityAI source, int sourceCargoIndex, EntityAI item, out TransferZSortRollbackMove rollbackMove)
    {
        rollbackMove = null;
        if (!source || !item)
            return false;

        InventoryLocation current = new InventoryLocation();
        if (!item.GetInventory().GetCurrentInventoryLocation(current))
            return false;
        if (!TransferZCargo.LocationMatches(current, source, sourceCargoIndex))
            return false;

        rollbackMove = new TransferZSortRollbackMove();
        rollbackMove.item = item;
        rollbackMove.row = current.GetRow();
        rollbackMove.col = current.GetCol();
        rollbackMove.flip = current.GetFlip();
        rollbackMove.cargoIndex = sourceCargoIndex;
        return true;
    }

    protected static bool ItemAtCargoCoordinates(EntityAI source, int cargoIndex, EntityAI item, int row, int col, bool flip)
    {
        if (!source || !item)
            return false;

        InventoryLocation current = new InventoryLocation();
        if (!item.GetInventory().GetCurrentInventoryLocation(current))
            return false;

        return TransferZCargo.LocationMatches(current, source, cargoIndex) && current.GetRow() == row && current.GetCol() == col && current.GetFlip() == flip;
    }

    protected static bool ItemAtRollbackLocation(EntityAI source, TransferZSortRollbackMove rollbackMove)
    {
        if (!rollbackMove)
            return false;
        return ItemAtCargoCoordinates(source, rollbackMove.cargoIndex, rollbackMove.item, rollbackMove.row, rollbackMove.col, rollbackMove.flip);
    }

    protected static bool ItemAtForwardLocation(EntityAI source, TransferZSortRollbackMove rollbackMove)
    {
        if (!rollbackMove)
            return false;
        return ItemAtCargoCoordinates(source, rollbackMove.cargoIndex, rollbackMove.item, rollbackMove.forwardRow, rollbackMove.forwardCol, rollbackMove.forwardFlip);
    }

    protected static bool SwapAtRollbackLocations(EntityAI source, TransferZSortRollbackMove rollbackMove)
    {
        if (!rollbackMove || !rollbackMove.swapItem)
            return false;
        return ItemAtCargoCoordinates(source, rollbackMove.cargoIndex, rollbackMove.item, rollbackMove.row, rollbackMove.col, rollbackMove.flip) && ItemAtCargoCoordinates(source, rollbackMove.cargoIndex, rollbackMove.swapItem, rollbackMove.swapRow, rollbackMove.swapCol, rollbackMove.swapFlip);
    }

    protected static bool SwapAtForwardLocations(EntityAI source, TransferZSortRollbackMove rollbackMove)
    {
        if (!rollbackMove || !rollbackMove.swapItem)
            return false;
        return ItemAtCargoCoordinates(source, rollbackMove.cargoIndex, rollbackMove.item, rollbackMove.forwardRow, rollbackMove.forwardCol, rollbackMove.forwardFlip) && ItemAtCargoCoordinates(source, rollbackMove.cargoIndex, rollbackMove.swapItem, rollbackMove.swapForwardRow, rollbackMove.swapForwardCol, rollbackMove.swapForwardFlip);
    }

    protected static bool CaptureSwapRollbackMove(EntityAI source, int sourceCargoIndex, EntityAI item1, EntityAI item2, out TransferZSortRollbackMove rollbackMove)
    {
        rollbackMove = null;
        if (!source || !item1 || !item2)
            return false;

        InventoryLocation first = new InventoryLocation();
        InventoryLocation second = new InventoryLocation();
        if (!item1.GetInventory().GetCurrentInventoryLocation(first) || !item2.GetInventory().GetCurrentInventoryLocation(second))
            return false;
        if (!TransferZCargo.LocationMatches(first, source, sourceCargoIndex) || !TransferZCargo.LocationMatches(second, source, sourceCargoIndex))
            return false;

        rollbackMove = new TransferZSortRollbackMove();
        rollbackMove.item = item1;
        rollbackMove.row = first.GetRow();
        rollbackMove.col = first.GetCol();
        rollbackMove.flip = first.GetFlip();
        rollbackMove.cargoIndex = sourceCargoIndex;
        rollbackMove.swapItem = item2;
        rollbackMove.swapRow = second.GetRow();
        rollbackMove.swapCol = second.GetCol();
        rollbackMove.swapFlip = second.GetFlip();

        rollbackMove.forwardRow = rollbackMove.swapRow;
        rollbackMove.forwardCol = rollbackMove.swapCol;
        rollbackMove.forwardFlip = rollbackMove.flip;
        rollbackMove.swapForwardRow = rollbackMove.row;
        rollbackMove.swapForwardCol = rollbackMove.col;
        rollbackMove.swapForwardFlip = rollbackMove.swapFlip;
        return true;
    }

    protected static bool RollbackExecutedMoves(PlayerBase player, EntityAI source, int sourceCargoIndex, notnull array<ref TransferZSortRollbackMove> rollbackMoves, notnull array<ref TransferZSortRecord> originalRecords)
    {
        bool reverseMovesAccepted = true;

        for (int rollbackIndex = rollbackMoves.Count() - 1; rollbackIndex >= 0; rollbackIndex--)
        {
            TransferZSortRollbackMove rollbackMove = rollbackMoves.Get(rollbackIndex);
            if (!rollbackMove || !rollbackMove.item || rollbackMove.cargoIndex != sourceCargoIndex)
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

                if (!TrySwapWithinCargo(player, source, sourceCargoIndex, rollbackMove.item, rollbackMove.swapItem) || !SwapAtRollbackLocations(source, rollbackMove))
                    reverseMovesAccepted = false;
                continue;
            }

            if (ItemAtRollbackLocation(source, rollbackMove))
                continue;

            if (!ItemAtForwardLocation(source, rollbackMove))
            {
                reverseMovesAccepted = false;
                continue;
            }

            if (!TryMoveWithinCargo(player, source, sourceCargoIndex, rollbackMove.item, rollbackMove.row, rollbackMove.col, rollbackMove.flip))
                reverseMovesAccepted = false;
        }

        bool exact = VerifyLayout(source, originalRecords);
        if (!exact)
            Print("[TransferZ] Sort transactional CRITICAL rollback incomplete reverseAccepted=" + reverseMovesAccepted.ToString() + " moves=" + rollbackMoves.Count().ToString());
        return exact;
    }

    protected static bool ExecutePlannedMove(PlayerBase player, EntityAI source, int sourceCargoIndex, TransferZSortMove move, notnull array<ref TransferZSortRollbackMove> rollbackMoves)
    {
        if (!move || !move.item)
            return false;

        if (move.swapItem)
        {
            TransferZSortRollbackMove swapRollback;
            if (!CaptureSwapRollbackMove(source, sourceCargoIndex, move.item, move.swapItem, swapRollback))
                return false;

            bool swapped = TrySwapWithinCargo(player, source, sourceCargoIndex, move.item, move.swapItem);
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
        if (!CaptureRollbackMove(source, sourceCargoIndex, move.item, rollbackMove))
            return false;

        rollbackMove.forwardRow = move.row;
        rollbackMove.forwardCol = move.col;
        rollbackMove.forwardFlip = move.flip;

        bool moved = TryMoveWithinCargo(player, source, sourceCargoIndex, move.item, move.row, move.col, move.flip);
        if (!moved)
        {
            if (ItemAtForwardLocation(source, rollbackMove))
                rollbackMoves.Insert(rollbackMove);
            return false;
        }

        rollbackMoves.Insert(rollbackMove);
        return ItemAtForwardLocation(source, rollbackMove);
    }

    protected static TransferZ_SortBuffer CreateSortBuffer(PlayerBase player)
    {
        if (!player)
            return null;

        int flags = ECE_SETUP | ECE_KEEPHEIGHT | ECE_NOLIFETIME | ECE_NOPERSISTENCY_WORLD | ECE_NOPERSISTENCY_CHAR;
        TransferZ_SortBuffer buffer = TransferZ_SortBuffer.Cast(GetGame().CreateObjectEx("TransferZ_SortBuffer", player.GetPosition(), flags, RF_IGNORE));
        if (!buffer)
            return null;

        buffer.SetPosition(player.GetPosition());
        buffer.SetAllowDamage(false);
        buffer.DisableSimulation(true);

        CargoBase cargo = buffer.GetInventory().GetCargo();
        if (!cargo)
        {
            buffer.Delete();
            return null;
        }

        return buffer;
    }

    protected static bool DeleteSortBufferIfEmpty(TransferZ_SortBuffer buffer)
    {
        if (!buffer)
            return true;

        CargoBase cargo = buffer.GetInventory().GetCargo();
        if (!cargo || cargo.GetItemCount() != 0)
            return false;

        buffer.Delete();
        return true;
    }

    protected static bool EmergencyDropUnrestoredItems(PlayerBase player, EntityAI source, TransferZ_SortBuffer buffer, notnull array<ref TransferZSortRecord> originalRecords, string reason)
    {
        if (!player || !source || !buffer)
            return false;
        InventoryMode moveMode = InventoryMode.SERVER;
        if (!GetGame().IsMultiplayer())
            moveMode = InventoryMode.LOCAL;
        bool allDropped = true;
        int dropped = 0;
        foreach (TransferZSortRecord record : originalRecords)
        {
            if (!record || !record.item)
            {
                allDropped = false;
                continue;
            }
            if (RecordAtCargoLocation(source, record))
                continue;
            InventoryLocation current = new InventoryLocation();
            if (!record.item.GetInventory().GetCurrentInventoryLocation(current))
            {
                allDropped = false;
                continue;
            }
            if (current.GetType() != InventoryLocationType.CARGO)
            {
                allDropped = false;
                continue;
            }
            EntityAI currentParent = current.GetParent();
            if (currentParent != source && currentParent != buffer)
            {
                allDropped = false;
                continue;
            }
            if (!record.item.GetInventory().DropEntity(moveMode, player, record.item))
            {
                allDropped = false;
                continue;
            }
            dropped++;
        }
        CargoBase bufferCargo = buffer.GetInventory().GetCargo();
        bool bufferEmpty = bufferCargo && bufferCargo.GetItemCount() == 0;
        if (bufferEmpty)
            buffer.Delete();
        Print("[TransferZ] Sort EMERGENCY ground drop reason=" + reason + " dropped=" + dropped.ToString() + " complete=" + allDropped.ToString() + " bufferEmpty=" + bufferEmpty.ToString());
        return allDropped && bufferEmpty;
    }

    protected static bool ItemInSortBuffer(TransferZ_SortBuffer buffer, EntityAI item)
    {
        if (!buffer || !item)
            return false;

        InventoryLocation current = new InventoryLocation();
        if (!item.GetInventory().GetCurrentInventoryLocation(current))
            return false;

        return current.GetType() == InventoryLocationType.CARGO && current.GetParent() == buffer;
    }

    protected static bool ValidateBufferedSortAuthorization(PlayerBase player, EntityAI source, notnull array<ref TransferZSortRecord> records)
    {
        if (!player || !source || !TransferZServerService.IsReachable(player, source))
            return false;

        foreach (TransferZSortRecord record : records)
        {
            if (!record || !record.item)
                return false;

            InventoryLocation current = new InventoryLocation();
            if (!record.item.GetInventory().GetCurrentInventoryLocation(current))
                return false;
            if (!TransferZCargo.LocationMatches(current, source, record.cargoIndex))
                return false;
            if (!record.item.GetInventory().CanRemoveEntity())
                return false;
            if (!source.CanReleaseCargo(record.item))
                return false;
            if (!GameInventory.CheckRequestSrc(player, current, GameInventory.c_MaxItemDistanceRadius))
                return false;
            if (TransferZServerService.HasNativeInventoryJuncture(record.item))
                return false;
        }

        return true;
    }

    protected static bool TryMoveToSortBuffer(PlayerBase player, EntityAI source, int sourceCargoIndex, TransferZ_SortBuffer buffer, EntityAI item)
    {
        if (!player || !source || !buffer || !item || !IsDirectCargoItem(source, item, sourceCargoIndex))
            return false;
        if (!item.GetInventory().CanRemoveEntity() || !source.CanReleaseCargo(item) || !buffer.CanReceiveItemIntoCargo(item))
            return false;

        InventoryLocation src = new InventoryLocation();
        if (!item.GetInventory().GetCurrentInventoryLocation(src) || !TransferZCargo.LocationMatches(src, source, sourceCargoIndex))
            return false;
        if (!GameInventory.CheckRequestSrc(player, src, GameInventory.c_MaxItemDistanceRadius))
            return false;

        InventoryLocation dst = new InventoryLocation();
        if (!buffer.GetInventory().FindFreeLocationFor(item, FindInventoryLocationType.CARGO, dst))
            return false;
        if (dst.GetType() != InventoryLocationType.CARGO || dst.GetParent() != buffer)
            return false;
        if (!GameInventory.LocationCanMoveEntity(src, dst))
            return false;
        if (TransferZServerService.HasNativeInventoryJuncture(item))
            return false;

        InventoryMode moveMode = InventoryMode.SERVER;
        if (!GetGame().IsMultiplayer())
            moveMode = InventoryMode.LOCAL;

        if (!item.GetInventory().TakeToDst(moveMode, src, dst))
            return false;
        return ItemInSortBuffer(buffer, item);
    }

    protected static bool TryMoveFromSortBuffer(PlayerBase player, EntityAI source, TransferZ_SortBuffer buffer, EntityAI item, int cargoIndex, int row, int col, bool flip)
    {
        if (!player || !source || !buffer || !item || !TransferZCargo.Exists(source, cargoIndex) || !ItemInSortBuffer(buffer, item))
            return false;
        if (!TransferZServerService.IsReachable(player, source))
            return false;
        if (!item.GetInventory().CanRemoveEntity() || !buffer.CanReleaseCargo(item) || !source.CanReceiveItemIntoCargo(item))
            return false;

        InventoryLocation src = new InventoryLocation();
        if (!item.GetInventory().GetCurrentInventoryLocation(src))
            return false;

        InventoryLocation dst = new InventoryLocation();
        dst.SetCargo(source, item, cargoIndex, row, col, flip);

        HumanInventory humanInventory = player.GetHumanInventory();
        if (humanInventory && humanInventory.GetUserReservedLocationCount() > 0 && humanInventory.FindCollidingUserReservedLocationIndex(item, dst) >= 0)
            return false;
        if (!GameInventory.LocationCanMoveEntity(src, dst))
            return false;
        if (TransferZServerService.HasNativeInventoryJuncture(item))
            return false;

        InventoryMode moveMode = InventoryMode.SERVER;
        if (!GetGame().IsMultiplayer())
            moveMode = InventoryMode.LOCAL;

        if (!item.GetInventory().TakeToDst(moveMode, src, dst))
            return false;
        return ItemAtCargoCoordinates(source, cargoIndex, item, row, col, flip);
    }

    protected static void BuildExpectedTargetRecords(notnull array<ref TransferZSortRecord> layoutRecords, notnull array<int> targetWidths, notnull array<int> targetHeights, notnull array<int> targetFlips, notnull array<ref TransferZSortRecord> expectedRecords)
    {
        expectedRecords.Clear();
        for (int recordIndex = 0; recordIndex < layoutRecords.Count(); recordIndex++)
        {
            TransferZSortRecord layoutRecord = layoutRecords.Get(recordIndex);
            TransferZSortRecord expectedRecord = CloneSortRecord(layoutRecord);
            if (!expectedRecord)
                continue;

            expectedRecord.row = layoutRecord.targetRow;
            expectedRecord.col = layoutRecord.targetCol;
            expectedRecord.width = targetWidths.Get(recordIndex);
            expectedRecord.height = targetHeights.Get(recordIndex);
            expectedRecord.flip = targetFlips.Get(recordIndex) != 0;
            expectedRecords.Insert(expectedRecord);
        }
    }

    protected static bool VerifyBufferedStaging(EntityAI source, TransferZ_SortBuffer buffer, notnull array<ref TransferZSortRecord> layoutRecords, notnull array<int> targetFlips, int expectedStaged)
    {
        if (!source || !buffer)
            return false;

        int stationary = 0;
        for (int recordIndex = 0; recordIndex < layoutRecords.Count(); recordIndex++)
        {
            TransferZSortRecord record = layoutRecords.Get(recordIndex);
            if (!record || !record.item)
                return false;

            if (RecordAtTargetV4(record, recordIndex, targetFlips))
            {
                stationary++;
                if (!RecordAtCargoLocation(source, record))
                    return false;
            }
            else if (!ItemInSortBuffer(buffer, record.item))
            {
                return false;
            }
        }

        int sourceCargoIndex = 0;
        if (layoutRecords.Count() > 0 && layoutRecords.Get(0))
            sourceCargoIndex = layoutRecords.Get(0).cargoIndex;
        CargoBase sourceCargo = TransferZCargo.Get(source, sourceCargoIndex);
        CargoBase bufferCargo = buffer.GetInventory().GetCargo();
        return sourceCargo && bufferCargo && sourceCargo.GetItemCount() == stationary && bufferCargo.GetItemCount() == expectedStaged;
    }

    protected static bool RecoverBufferedSort(PlayerBase player, EntityAI source, TransferZ_SortBuffer buffer, notnull array<ref TransferZSortRecord> originalRecords)
    {
        if (!player || !source || !buffer)
            return false;

        bool accepted = true;

        // First evacuate every tracked item that is in the source but no longer
        // at its exact original coordinates. This recreates the empty geometry
        // needed to restore the original snapshot deterministically.
        foreach (TransferZSortRecord originalRecord : originalRecords)
        {
            if (!originalRecord || !originalRecord.item)
            {
                accepted = false;
                continue;
            }
            if (RecordAtCargoLocation(source, originalRecord))
                continue;
            if (ItemInSortBuffer(buffer, originalRecord.item))
                continue;

            InventoryLocation current = new InventoryLocation();
            if (!originalRecord.item.GetInventory().GetCurrentInventoryLocation(current))
            {
                accepted = false;
                continue;
            }
            if (current.GetType() != InventoryLocationType.CARGO || current.GetParent() != source)
            {
                accepted = false;
                continue;
            }
            if (!TryMoveToSortBuffer(player, source, originalRecord.cargoIndex, buffer, originalRecord.item))
                accepted = false;
        }

        // Restore exact row/column/orientation from the immutable snapshot.
        foreach (TransferZSortRecord restoreRecord : originalRecords)
        {
            if (!restoreRecord || !restoreRecord.item)
            {
                accepted = false;
                continue;
            }
            if (RecordAtCargoLocation(source, restoreRecord))
                continue;
            if (!ItemInSortBuffer(buffer, restoreRecord.item))
            {
                accepted = false;
                continue;
            }
            if (!TryMoveFromSortBuffer(player, source, buffer, restoreRecord.item, restoreRecord.cargoIndex, restoreRecord.row, restoreRecord.col, restoreRecord.flip))
                accepted = false;
        }

        bool exact = VerifyLayout(source, originalRecords);
        CargoBase bufferCargo = buffer.GetInventory().GetCargo();
        bool bufferEmpty = bufferCargo && bufferCargo.GetItemCount() == 0;
        if (exact && bufferEmpty)
        {
            DeleteSortBufferIfEmpty(buffer);
            return true;
        }
        bool emergencyDropped = EmergencyDropUnrestoredItems(player, source, buffer, originalRecords, "rollback-incomplete");
        Print("[TransferZ] Sort buffer CRITICAL rollback incomplete accepted=" + accepted.ToString() + " exact=" + exact.ToString() + " bufferEmpty=" + bufferEmpty.ToString() + " emergencyDropped=" + emergencyDropped.ToString());
        return false;
    }

    protected static int SortWithNativeBuffer(PlayerBase player, EntityAI source, notnull array<ref TransferZSortRecord> originalRecords, notnull array<ref TransferZSortRecord> layoutRecords, notnull array<int> targetWidths, notnull array<int> targetHeights, notnull array<int> targetFlips)
    {
        if (layoutRecords.Count() != originalRecords.Count())
            return -1;
        if (targetWidths.Count() != layoutRecords.Count() || targetHeights.Count() != layoutRecords.Count() || targetFlips.Count() != layoutRecords.Count())
            return -1;
        if (AllRecordsAtTargetV4(layoutRecords, targetFlips))
            return 0;

        ref array<ref TransferZSortRecord> expectedRecords = new array<ref TransferZSortRecord>();
        BuildExpectedTargetRecords(layoutRecords, targetWidths, targetHeights, targetFlips, expectedRecords);
        if (expectedRecords.Count() != originalRecords.Count())
            return -1;

        if (!VerifyLayout(source, originalRecords))
            return -1;

        // The hidden buffer is server-internal workspace, not a player-accessible
        // destination. Authorize the actual player-visible source through DayZ's
        // native anti-cheat gate before any item leaves that source. Each staging
        // move repeats CheckRequestSrc against its current authoritative location.
        if (!ValidateBufferedSortAuthorization(player, source, originalRecords))
        {
            Print("[TransferZ] Sort buffer fallback rejected: native source authorization failed");
            return -1;
        }

        TransferZ_SortBuffer buffer = CreateSortBuffer(player);
        if (!buffer)
        {
            Print("[TransferZ] Sort buffer fallback failed: could not create internal cargo buffer");
            return -1;
        }

        int staged = 0;
        for (int stageIndex = 0; stageIndex < layoutRecords.Count(); stageIndex++)
        {
            TransferZSortRecord layoutRecord = layoutRecords.Get(stageIndex);
            if (RecordAtTargetV4(layoutRecord, stageIndex, targetFlips))
                continue;

            if (!TryMoveToSortBuffer(player, source, layoutRecord.cargoIndex, buffer, layoutRecord.item))
            {
                bool stageRollback = RecoverBufferedSort(player, source, buffer, originalRecords);
                Print("[TransferZ] Sort buffer fallback failed during staging staged=" + staged.ToString() + " rollback=" + stageRollback.ToString());
                return -1;
            }
            staged++;
        }

        if (!VerifyBufferedStaging(source, buffer, layoutRecords, targetFlips, staged))
        {
            bool stagingVerifyRollback = RecoverBufferedSort(player, source, buffer, originalRecords);
            Print("[TransferZ] Sort buffer fallback failed staging verification rollback=" + stagingVerifyRollback.ToString());
            return -1;
        }

        int placed = 0;
        for (int placeIndex = 0; placeIndex < layoutRecords.Count(); placeIndex++)
        {
            TransferZSortRecord placeRecord = layoutRecords.Get(placeIndex);
            if (RecordAtTargetV4(placeRecord, placeIndex, targetFlips))
                continue;

            bool targetFlip = targetFlips.Get(placeIndex) != 0;
            if (!TryMoveFromSortBuffer(player, source, buffer, placeRecord.item, placeRecord.cargoIndex, placeRecord.targetRow, placeRecord.targetCol, targetFlip))
            {
                bool placeRollback = RecoverBufferedSort(player, source, buffer, originalRecords);
                Print("[TransferZ] Sort buffer fallback failed during placement placed=" + placed.ToString() + "/" + staged.ToString() + " rollback=" + placeRollback.ToString());
                return -1;
            }
            placed++;
        }

        if (!VerifyLayout(source, expectedRecords))
        {
            bool verifyRollback = RecoverBufferedSort(player, source, buffer, originalRecords);
            Print("[TransferZ] Sort buffer fallback failed target verification rollback=" + verifyRollback.ToString());
            return -1;
        }

        if (!DeleteSortBufferIfEmpty(buffer))
        {
            bool cleanupRollback = RecoverBufferedSort(player, source, buffer, originalRecords);
            Print("[TransferZ] Sort buffer fallback failed cleanup rollback=" + cleanupRollback.ToString());
            return -1;
        }

        return staged + placed;
    }

    static int Sort(PlayerBase player, EntityAI source, int sourceCargoIndex = 0)
    {
        if (!player || !source || !TransferZServerService.IsReachable(player, source) || !TransferZCargo.Exists(source, sourceCargoIndex))
        {
            Print("[TransferZ] Sort transactional rejected: invalid or unreachable cargo source");
            return -1;
        }

        ref array<ref TransferZSortRecord> originalRecords = new array<ref TransferZSortRecord>();
        int cargoWidth;
        int cargoHeight;
        if (!SnapshotSortRecords(source, sourceCargoIndex, originalRecords, cargoWidth, cargoHeight))
        {
            Print("[TransferZ] Sort transactional rejected: cargo snapshot failed for " + source.GetType());
            return -1;
        }
        if (originalRecords.Count() < 1)
            return 0;

        SortRecordsV4(originalRecords);

        // Compute the deterministic target layout exactly once. Both execution
        // strategies use the same immutable layout, so falling back from the
        // bounded in-cargo planner does not repeat the expensive packing passes.
        ref array<ref TransferZSortRecord> layoutRecords = new array<ref TransferZSortRecord>();
        CloneSortRecords(originalRecords, layoutRecords);
        if (layoutRecords.Count() != originalRecords.Count())
        {
            Print("[TransferZ] Sort transactional rejected: layout snapshot clone failed");
            return -1;
        }

        ref array<int> targetWidths = new array<int>();
        ref array<int> targetHeights = new array<int>();
        ref array<int> targetFlips = new array<int>();
        if (!BuildTargetLayoutV4(player, source, layoutRecords, cargoWidth, cargoHeight, targetWidths, targetHeights, targetFlips))
        {
            Print("[TransferZ] Sort transactional rejected: target layout failed for " + source.GetType());
            return -1;
        }
        if (AllRecordsAtTargetV4(layoutRecords, targetFlips))
            return 0;

        ref array<ref TransferZSortRecord> plannedRecords = new array<ref TransferZSortRecord>();
        CloneSortRecords(layoutRecords, plannedRecords);
        if (plannedRecords.Count() != layoutRecords.Count())
        {
            Print("[TransferZ] Sort transactional rejected: planning snapshot clone failed");
            return -1;
        }

        ref array<ref TransferZSortMove> moves = new array<ref TransferZSortMove>();
        if (!BuildSortPlanFromTargetsV4(player, source, plannedRecords, cargoWidth, cargoHeight, targetWidths, targetHeights, targetFlips, moves))
        {
            int bufferedResult = SortWithNativeBuffer(player, source, originalRecords, layoutRecords, targetWidths, targetHeights, targetFlips);
            if (bufferedResult < 0)
                Print("[TransferZ] Sort transactional failed: bounded in-cargo plan and native buffer fallback both failed for " + source.GetType());
            return bufferedResult;
        }
        if (moves.Count() == 0)
            return 0;

        if (!VerifyLayout(source, originalRecords))
        {
            Print("[TransferZ] Sort transactional rejected: source changed before execution");
            return -1;
        }

        ref array<ref TransferZSortRollbackMove> rollbackMoves = new array<ref TransferZSortRollbackMove>();
        int moved = 0;
        foreach (TransferZSortMove move : moves)
        {
            if (!ExecutePlannedMove(player, source, sourceCargoIndex, move, rollbackMoves))
            {
                bool rolledBackAfterMoveFailure = RollbackExecutedMoves(player, source, sourceCargoIndex, rollbackMoves, originalRecords);
                string failedKind = "move";
                if (move.swapItem)
                    failedKind = "swap";
                string failedItem = "null";
                if (move.item)
                    failedItem = move.item.GetType();
                Print("[TransferZ] Sort transactional failed during in-cargo execution moved=" + moved.ToString() + "/" + moves.Count().ToString() + " kind=" + failedKind + " item=" + failedItem + " target=" + move.row.ToString() + "," + move.col.ToString() + " flip=" + move.flip.ToString() + " rollback=" + rolledBackAfterMoveFailure.ToString());
                return -1;
            }
            moved++;
        }

        if (!VerifyLayout(source, plannedRecords))
        {
            bool rolledBackAfterVerifyFailure = RollbackExecutedMoves(player, source, sourceCargoIndex, rollbackMoves, originalRecords);
            Print("[TransferZ] Sort transactional failed target verification rollback=" + rolledBackAfterVerifyFailure.ToString());
            return -1;
        }

        return moved;
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
        int sourceCargoIndex;
        if (!ctx.Read(operation))
            return;
        if (!ctx.Read(sourceLow))
            return;
        if (!ctx.Read(sourceHigh))
            return;
        if (!ctx.Read(sourceCargoIndex))
            return;

        if (sourceCargoIndex < 0)
            return;
        if (operation != TransferZMaintenanceOperation.SORT && operation != TransferZMaintenanceOperation.STACK)
            return;
        if (!TransferZServerService.CanPlayerManipulate(player))
            return;

        if (!AcceptServerMaintenanceRequest(player))
        {
            Print("[TransferZ] Maintenance RPC throttled for player=" + player.GetIdentity().GetId());
            return;
        }

        EntityAI source = TransferZServerService.ResolveEntity(sourceLow, sourceHigh);
        if (!source)
        {
            if (operation == TransferZMaintenanceOperation.SORT)
                SendResult(player, operation, sourceLow, sourceHigh, sourceCargoIndex, false);
            return;
        }

        if (operation == TransferZMaintenanceOperation.SORT)
        {
            int sortResult = Sort(player, source, sourceCargoIndex);
            SendResult(player, operation, sourceLow, sourceHigh, sourceCargoIndex, sortResult >= 0);
            player.UpdateInventoryMenu();
        }
        else if (operation == TransferZMaintenanceOperation.STACK)
        {
            Stack(player, source, sourceCargoIndex);
            player.UpdateInventoryMenu();
        }
    }

}
