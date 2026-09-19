class TransferZSortPlannerState
{
    PlayerBase player;
    EntityAI source;
    ref array<ref TransferZSortRecord> records;
    ref array<int> currentGrid;
    ref array<int> targetGrid;
    ref array<int> targetWidths;
    ref array<int> targetHeights;
    ref array<int> targetFlips;
    ref array<int> lockedRecords;
    ref array<int> activeParking;
    ref array<ref TransferZSortMove> moves;
    int cargoWidth;
    int cargoHeight;
    int maxDepth;
    int maxSteps;
    int maxSearch;
    int stepCount;
    int searchCount;
}

class TransferZSortPlanner : TransferZMaintenanceService
{
    protected static int LongSideV4(TransferZSortRecord record)
    {
        if (record.width > record.height)
            return record.width;
        return record.height;
    }

    protected static int ShortSideV4(TransferZSortRecord record)
    {
        if (record.width < record.height)
            return record.width;
        return record.height;
    }

    protected static bool SortBeforeV4(TransferZSortRecord left, TransferZSortRecord right)
    {
        int leftArea = left.width * left.height;
        int rightArea = right.width * right.height;
        if (leftArea != rightArea)
            return leftArea > rightArea;

        int leftLong = LongSideV4(left);
        int rightLong = LongSideV4(right);
        if (leftLong != rightLong)
            return leftLong > rightLong;

        int leftShort = ShortSideV4(left);
        int rightShort = ShortSideV4(right);
        if (leftShort != rightShort)
            return leftShort > rightShort;

        if (left.row != right.row)
            return left.row < right.row;
        if (left.col != right.col)
            return left.col < right.col;
        return left.typeHash < right.typeHash;
    }

    protected static void SortRecordsV4(notnull array<ref TransferZSortRecord> records)
    {
        for (int recordIndex = 1; recordIndex < records.Count(); recordIndex++)
        {
            ref TransferZSortRecord current = records.Get(recordIndex);
            int previousIndex = recordIndex - 1;
            while (previousIndex >= 0 && SortBeforeV4(current, records.Get(previousIndex)))
            {
                records.Set(previousIndex + 1, records.Get(previousIndex));
                previousIndex--;
            }
            records.Set(previousIndex + 1, current);
        }
    }

    protected static int OrientationCountV4(TransferZSortRecord record)
    {
        if (!record || record.width == record.height)
            return 1;
        return 2;
    }

    protected static void GetOrientationV4(TransferZSortRecord record, int orientationIndex, out int width, out int height, out bool flip)
    {
        width = record.width;
        height = record.height;
        flip = record.flip;

        if (orientationIndex == 1 && record.width != record.height)
        {
            width = record.height;
            height = record.width;
            flip = !record.flip;
        }
    }

    protected static bool CollidesWithUserReservationV4(PlayerBase player, EntityAI source, TransferZSortRecord record, int row, int col, bool flip)
    {
        if (!player || !source || !record || !record.item)
            return true;

        HumanInventory humanInventory = player.GetHumanInventory();
        if (!humanInventory)
            return false;

        InventoryLocation destination = new InventoryLocation();
        destination.SetCargo(source, record.item, record.cargoIndex, row, col, flip);
        return humanInventory.FindCollidingUserReservedLocationIndex(record.item, destination) >= 0;
    }

    protected static int FindFirstTargetAnchorV4(notnull array<int> targetGrid, int cargoWidth, int cargoHeight)
    {
        for (int row = 0; row < cargoHeight; row++)
        {
            for (int col = 0; col < cargoWidth; col++)
            {
                if (targetGrid.Get(row * cargoWidth + col) == 0)
                    return row * cargoWidth + col;
            }
        }
        return -1;
    }

    protected static int AnchorFitWasteV4(notnull array<int> targetGrid, int cargoWidth, int cargoHeight, int row, int col, int itemWidth, int itemHeight)
    {
        int horizontalRun = 0;
        for (int scanCol = col; scanCol < cargoWidth; scanCol++)
        {
            if (targetGrid.Get(row * cargoWidth + scanCol) != 0)
                break;
            horizontalRun++;
        }

        int verticalRun = 0;
        for (int scanRow = row; scanRow < cargoHeight; scanRow++)
        {
            if (targetGrid.Get(scanRow * cargoWidth + col) != 0)
                break;
            verticalRun++;
        }

        int horizontalWaste = horizontalRun - itemWidth;
        int verticalWaste = verticalRun - itemHeight;
        return horizontalWaste * (cargoHeight + 1) + verticalWaste;
    }

    protected static bool BetterAnchorCandidateV4(TransferZSortRecord candidate, int candidateWidth, int candidateHeight, int candidateOrientation, int candidateWaste, TransferZSortRecord best, int bestWidth, int bestHeight, int bestOrientation, int bestWaste)
    {
        if (!best)
            return true;

        int candidateArea = candidateWidth * candidateHeight;
        int bestArea = bestWidth * bestHeight;
        if (candidateArea != bestArea)
            return candidateArea > bestArea;
        if (candidateWaste != bestWaste)
            return candidateWaste < bestWaste;
        if (candidateOrientation != bestOrientation)
            return candidateOrientation == 0;
        if (candidateWidth != bestWidth)
            return candidateWidth > bestWidth;
        if (candidateHeight != bestHeight)
            return candidateHeight > bestHeight;
        if (candidate.row != best.row)
            return candidate.row < best.row;
        if (candidate.col != best.col)
            return candidate.col < best.col;
        return candidate.typeHash < best.typeHash;
    }

    protected static bool AssignCompactTargetsV4(PlayerBase player, EntityAI source, notnull array<ref TransferZSortRecord> records, int cargoWidth, int cargoHeight, notnull array<int> targetWidths, notnull array<int> targetHeights, notnull array<int> targetFlips)
    {
        ref array<int> targetGrid = new array<int>();
        ref array<int> assigned = new array<int>();
        ResetGrid(targetGrid, cargoWidth * cargoHeight);
        ResetGrid(assigned, records.Count());
        ResetGrid(targetWidths, records.Count());
        ResetGrid(targetHeights, records.Count());
        ResetGrid(targetFlips, records.Count());

        int assignedCount = 0;
        int skippedCells = 0;
        while (assignedCount < records.Count())
        {
            int anchor = FindFirstTargetAnchorV4(targetGrid, cargoWidth, cargoHeight);
            if (anchor < 0)
                return false;

            int anchorRow = anchor / cargoWidth;
            int anchorCol = anchor - anchorRow * cargoWidth;
            int bestIndex = -1;
            int bestWidth = 0;
            int bestHeight = 0;
            int bestOrientation = 0;
            int bestWaste = 0;
            bool bestFlip = false;
            TransferZSortRecord bestRecord;

            for (int candidateIndex = 0; candidateIndex < records.Count(); candidateIndex++)
            {
                if (assigned.Get(candidateIndex) != 0)
                    continue;

                TransferZSortRecord candidate = records.Get(candidateIndex);
                int orientationCount = OrientationCountV4(candidate);
                for (int orientationIndex = 0; orientationIndex < orientationCount; orientationIndex++)
                {
                    int candidateWidth;
                    int candidateHeight;
                    bool candidateFlip;
                    GetOrientationV4(candidate, orientationIndex, candidateWidth, candidateHeight, candidateFlip);

                    if (!RectFree(targetGrid, cargoWidth, cargoHeight, anchorRow, anchorCol, candidateWidth, candidateHeight))
                        continue;
                    if (CollidesWithUserReservationV4(player, source, candidate, anchorRow, anchorCol, candidateFlip))
                        continue;

                    int candidateWaste = AnchorFitWasteV4(targetGrid, cargoWidth, cargoHeight, anchorRow, anchorCol, candidateWidth, candidateHeight);
                    if (!BetterAnchorCandidateV4(candidate, candidateWidth, candidateHeight, orientationIndex, candidateWaste, bestRecord, bestWidth, bestHeight, bestOrientation, bestWaste))
                        continue;

                    bestIndex = candidateIndex;
                    bestWidth = candidateWidth;
                    bestHeight = candidateHeight;
                    bestOrientation = orientationIndex;
                    bestWaste = candidateWaste;
                    bestFlip = candidateFlip;
                    bestRecord = candidate;
                }
            }

            if (bestIndex < 0)
            {
                targetGrid.Set(anchor, -1);
                skippedCells++;
                if (skippedCells > cargoWidth * cargoHeight)
                    return false;
                continue;
            }

            bestRecord.targetRow = anchorRow;
            bestRecord.targetCol = anchorCol;
            targetWidths.Set(bestIndex, bestWidth);
            targetHeights.Set(bestIndex, bestHeight);
            if (bestFlip)
                targetFlips.Set(bestIndex, 1);
            MarkRect(targetGrid, cargoWidth, anchorRow, anchorCol, bestWidth, bestHeight, bestIndex + 1);
            assigned.Set(bestIndex, 1);
            assignedCount++;
        }
        return true;
    }

    protected static bool AssignFirstFitTargetsV4(PlayerBase player, EntityAI source, notnull array<ref TransferZSortRecord> records, int cargoWidth, int cargoHeight, notnull array<int> targetWidths, notnull array<int> targetHeights, notnull array<int> targetFlips)
    {
        ref array<int> targetGrid = new array<int>();
        ResetGrid(targetGrid, cargoWidth * cargoHeight);
        ResetGrid(targetWidths, records.Count());
        ResetGrid(targetHeights, records.Count());
        ResetGrid(targetFlips, records.Count());

        for (int targetIndex = 0; targetIndex < records.Count(); targetIndex++)
        {
            TransferZSortRecord record = records.Get(targetIndex);
            bool placed = false;

            for (int targetRow = 0; targetRow < cargoHeight && !placed; targetRow++)
            {
                for (int targetCol = 0; targetCol < cargoWidth && !placed; targetCol++)
                {
                    int orientationCount = OrientationCountV4(record);
                    for (int orientationIndex = 0; orientationIndex < orientationCount; orientationIndex++)
                    {
                        int targetWidth;
                        int targetHeight;
                        bool targetFlip;
                        GetOrientationV4(record, orientationIndex, targetWidth, targetHeight, targetFlip);

                        if (!RectFree(targetGrid, cargoWidth, cargoHeight, targetRow, targetCol, targetWidth, targetHeight))
                            continue;
                        if (CollidesWithUserReservationV4(player, source, record, targetRow, targetCol, targetFlip))
                            continue;

                        record.targetRow = targetRow;
                        record.targetCol = targetCol;
                        targetWidths.Set(targetIndex, targetWidth);
                        targetHeights.Set(targetIndex, targetHeight);
                        if (targetFlip)
                            targetFlips.Set(targetIndex, 1);
                        MarkRect(targetGrid, cargoWidth, targetRow, targetCol, targetWidth, targetHeight, targetIndex + 1);
                        placed = true;
                        break;
                    }
                }
            }

            if (!placed)
                return false;
        }

        return true;
    }

    protected static bool TargetBeforeV4(TransferZSortRecord left, int leftWidth, int leftHeight, TransferZSortRecord right, int rightWidth, int rightHeight)
    {
        if (left.targetRow != right.targetRow)
            return left.targetRow < right.targetRow;
        if (left.targetCol != right.targetCol)
            return left.targetCol < right.targetCol;

        int leftArea = leftWidth * leftHeight;
        int rightArea = rightWidth * rightHeight;
        if (leftArea != rightArea)
            return leftArea > rightArea;
        if (leftWidth != rightWidth)
            return leftWidth > rightWidth;
        return left.typeHash < right.typeHash;
    }

    protected static void SortRecordsByTargetV4(notnull array<ref TransferZSortRecord> records, notnull array<int> targetWidths, notnull array<int> targetHeights, notnull array<int> targetFlips)
    {
        for (int recordIndex = 1; recordIndex < records.Count(); recordIndex++)
        {
            ref TransferZSortRecord current = records.Get(recordIndex);
            int currentWidth = targetWidths.Get(recordIndex);
            int currentHeight = targetHeights.Get(recordIndex);
            int currentFlip = targetFlips.Get(recordIndex);
            int previousIndex = recordIndex - 1;

            while (previousIndex >= 0 && TargetBeforeV4(current, currentWidth, currentHeight, records.Get(previousIndex), targetWidths.Get(previousIndex), targetHeights.Get(previousIndex)))
            {
                records.Set(previousIndex + 1, records.Get(previousIndex));
                targetWidths.Set(previousIndex + 1, targetWidths.Get(previousIndex));
                targetHeights.Set(previousIndex + 1, targetHeights.Get(previousIndex));
                targetFlips.Set(previousIndex + 1, targetFlips.Get(previousIndex));
                previousIndex--;
            }

            records.Set(previousIndex + 1, current);
            targetWidths.Set(previousIndex + 1, currentWidth);
            targetHeights.Set(previousIndex + 1, currentHeight);
            targetFlips.Set(previousIndex + 1, currentFlip);
        }
    }

    protected static int CountForeignCurrentOverlapsV4(notnull array<ref TransferZSortRecord> records, notnull array<int> targetWidths, notnull array<int> targetHeights, int recordIndex, int targetRow, int targetCol)
    {
        int overlapCount = 0;
        int targetWidth = targetWidths.Get(recordIndex);
        int targetHeight = targetHeights.Get(recordIndex);

        for (int otherIndex = 0; otherIndex < records.Count(); otherIndex++)
        {
            if (otherIndex == recordIndex)
                continue;

            TransferZSortRecord other = records.Get(otherIndex);
            if (RectOverlaps(targetRow, targetCol, targetWidth, targetHeight, other.row, other.col, other.width, other.height))
                overlapCount++;
        }

        return overlapCount;
    }

    protected static int TargetAssignmentCostV4(notnull array<ref TransferZSortRecord> records, notnull array<int> targetWidths, notnull array<int> targetHeights, notnull array<int> targetFlips, int recordIndex, int targetRow, int targetCol)
    {
        TransferZSortRecord record = records.Get(recordIndex);
        int foreignOverlap = CountForeignCurrentOverlapsV4(records, targetWidths, targetHeights, recordIndex, targetRow, targetCol);
        int distance = Math.AbsInt(record.row - targetRow) + Math.AbsInt(record.col - targetCol);
        int rotationPenalty = 0;
        bool targetFlip = targetFlips.Get(recordIndex) != 0;
        if (record.flip != targetFlip)
            rotationPenalty = 1;
        return foreignOverlap * 10000 + distance * 2 + rotationPenalty;
    }

    protected static void OptimizeEquivalentTargetAssignmentsV4(notnull array<ref TransferZSortRecord> records, notnull array<int> targetWidths, notnull array<int> targetHeights, notnull array<int> targetFlips)
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
                    if (targetWidths.Get(leftIndex) != targetWidths.Get(rightIndex) || targetHeights.Get(leftIndex) != targetHeights.Get(rightIndex))
                        continue;

                    int currentCost = TargetAssignmentCostV4(records, targetWidths, targetHeights, targetFlips, leftIndex, left.targetRow, left.targetCol) + TargetAssignmentCostV4(records, targetWidths, targetHeights, targetFlips, rightIndex, right.targetRow, right.targetCol);
                    int swappedCost = TargetAssignmentCostV4(records, targetWidths, targetHeights, targetFlips, leftIndex, right.targetRow, right.targetCol) + TargetAssignmentCostV4(records, targetWidths, targetHeights, targetFlips, rightIndex, left.targetRow, left.targetCol);
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

    protected static void BuildTargetGridV4(notnull array<ref TransferZSortRecord> records, notnull array<int> targetWidths, notnull array<int> targetHeights, int cargoWidth, int cargoHeight, notnull array<int> targetGrid)
    {
        ResetGrid(targetGrid, cargoWidth * cargoHeight);
        for (int targetIndex = 0; targetIndex < records.Count(); targetIndex++)
        {
            TransferZSortRecord record = records.Get(targetIndex);
            MarkRect(targetGrid, cargoWidth, record.targetRow, record.targetCol, targetWidths.Get(targetIndex), targetHeights.Get(targetIndex), targetIndex + 1);
        }
    }

    protected static bool RecordAtTargetV4(TransferZSortRecord record, int recordIndex, notnull array<int> targetFlips)
    {
        bool targetFlip = targetFlips.Get(recordIndex) != 0;
        return record.row == record.targetRow && record.col == record.targetCol && record.flip == targetFlip;
    }

    protected static bool AllRecordsAtTargetV4(notnull array<ref TransferZSortRecord> records, notnull array<int> targetFlips)
    {
        for (int recordIndex = 0; recordIndex < records.Count(); recordIndex++)
        {
            if (!RecordAtTargetV4(records.Get(recordIndex), recordIndex, targetFlips))
                return false;
        }
        return true;
    }

    protected static int FindBlockerForTargetV4(notnull array<ref TransferZSortRecord> records, notnull array<int> currentGrid, notnull array<int> targetWidths, notnull array<int> targetHeights, int cargoWidth, int targetIndex)
    {
        TransferZSortRecord pending = records.Get(targetIndex);
        int targetWidth = targetWidths.Get(targetIndex);
        int targetHeight = targetHeights.Get(targetIndex);

        for (int targetRow = pending.targetRow; targetRow < pending.targetRow + targetHeight; targetRow++)
        {
            for (int targetCol = pending.targetCol; targetCol < pending.targetCol + targetWidth; targetCol++)
            {
                int value = currentGrid.Get(targetRow * cargoWidth + targetCol);
                if (value != 0 && value != targetIndex + 1)
                    return value - 1;
            }
        }
        return -1;
    }

    protected static int CountTargetOverlapV4(notnull array<int> targetGrid, int cargoWidth, int row, int col, int itemWidth, int itemHeight)
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

    protected static bool FindTemporaryPlacementV4(notnull TransferZSortPlannerState state, int recordIndex, int protectedTargetIndex, bool requireTargetOverlapImprovement, out int bestRow, out int bestCol, out int bestWidth, out int bestHeight, out bool bestFlip)
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

    protected static void AddPlannedMoveV4(notnull array<ref TransferZSortMove> moves, TransferZSortRecord record, int row, int col, bool flip)
    {
        ref TransferZSortMove move = new TransferZSortMove();
        move.item = record.item;
        move.row = row;
        move.col = col;
        move.flip = flip;
        moves.Insert(move);
    }

    protected static void MoveRecordInGridV4(notnull array<int> currentGrid, int cargoWidth, TransferZSortRecord record, int recordValue, int row, int col, int newWidth, int newHeight, bool newFlip)
    {
        MarkRect(currentGrid, cargoWidth, record.row, record.col, record.width, record.height, 0);
        MarkRect(currentGrid, cargoWidth, row, col, newWidth, newHeight, recordValue);
        record.row = row;
        record.col = col;
        record.width = newWidth;
        record.height = newHeight;
        record.flip = newFlip;
    }

    protected static void CapturePlannerStateV4(notnull array<ref TransferZSortRecord> records, notnull array<int> rows, notnull array<int> cols, notnull array<int> widths, notnull array<int> heights, notnull array<int> flips)
    {
        rows.Resize(records.Count());
        cols.Resize(records.Count());
        widths.Resize(records.Count());
        heights.Resize(records.Count());
        flips.Resize(records.Count());

        for (int recordIndex = 0; recordIndex < records.Count(); recordIndex++)
        {
            TransferZSortRecord record = records.Get(recordIndex);
            rows.Set(recordIndex, record.row);
            cols.Set(recordIndex, record.col);
            widths.Set(recordIndex, record.width);
            heights.Set(recordIndex, record.height);
            if (record.flip)
                flips.Set(recordIndex, 1);
            else
                flips.Set(recordIndex, 0);
        }
    }

    protected static void RestorePlannerStateV4(notnull array<ref TransferZSortRecord> records, notnull array<int> currentGrid, int cargoWidth, int cargoHeight, notnull array<ref TransferZSortMove> moves, int moveCount, notnull array<int> rows, notnull array<int> cols, notnull array<int> widths, notnull array<int> heights, notnull array<int> flips)
    {
        for (int recordIndex = 0; recordIndex < records.Count(); recordIndex++)
        {
            TransferZSortRecord record = records.Get(recordIndex);
            record.row = rows.Get(recordIndex);
            record.col = cols.Get(recordIndex);
            record.width = widths.Get(recordIndex);
            record.height = heights.Get(recordIndex);
            record.flip = flips.Get(recordIndex) != 0;
        }

        moves.Resize(moveCount);
        ResetGrid(currentGrid, cargoWidth * cargoHeight);
        for (int gridIndex = 0; gridIndex < records.Count(); gridIndex++)
        {
            TransferZSortRecord gridRecord = records.Get(gridIndex);
            MarkRect(currentGrid, cargoWidth, gridRecord.row, gridRecord.col, gridRecord.width, gridRecord.height, gridIndex + 1);
        }
    }

    protected static bool CollectBlockersV4(notnull array<int> currentGrid, int cargoWidth, int row, int col, int itemWidth, int itemHeight, int ownValue, notnull array<int> blockers)
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

    protected static bool ParkRecordRecursiveV4(notnull TransferZSortPlannerState state, int recordIndex, int protectedTargetIndex, int depth)
    {
        if (recordIndex < 0 || recordIndex >= state.records.Count())
            return false;
        if (state.lockedRecords.Get(recordIndex) != 0 || state.activeParking.Get(recordIndex) != 0)
            return false;
        if (depth > state.maxDepth || state.stepCount >= state.maxSteps || state.searchCount >= state.maxSearch)
            return false;

        state.searchCount++;
        state.activeParking.Set(recordIndex, 1);

        int directRow;
        int directCol;
        int directWidth;
        int directHeight;
        bool directFlip;
        if (FindTemporaryPlacementV4(state, recordIndex, protectedTargetIndex, false, directRow, directCol, directWidth, directHeight, directFlip))
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
                    if (CollidesWithUserReservationV4(state.player, state.source, record, candidateRow, candidateCol, candidateFlip))
                        continue;

                    ref array<int> blockers = new array<int>();
                    CollectBlockersV4(state.currentGrid, state.cargoWidth, candidateRow, candidateCol, candidateWidth, candidateHeight, recordIndex + 1, blockers);
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

                    bool cleared = true;
                    foreach (int recursiveBlocker : blockers)
                    {
                        if (!ParkRecordRecursiveV4(state, recursiveBlocker, protectedTargetIndex, depth + 1))
                        {
                            cleared = false;
                            break;
                        }
                    }

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

        state.activeParking.Set(recordIndex, 0);
        return false;
    }

    protected static bool EnsureRecordAtTargetV4(notnull TransferZSortPlannerState state, int recordIndex)
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

            if (!ParkRecordRecursiveV4(state, blockerIndex, recordIndex, 0))
            {
                Print("[TransferZ] Sort planner V4 could not evacuate blocker=" + blockerIndex.ToString() + " for target=" + recordIndex.ToString());
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

    protected static bool BuildSortPlanV4(PlayerBase player, EntityAI source, notnull array<ref TransferZSortRecord> records, int cargoWidth, int cargoHeight, notnull array<ref TransferZSortMove> moves)
    {
        moves.Clear();

        ref array<int> targetWidths = new array<int>();
        ref array<int> targetHeights = new array<int>();
        ref array<int> targetFlips = new array<int>();

        if (!AssignCompactTargetsV4(player, source, records, cargoWidth, cargoHeight, targetWidths, targetHeights, targetFlips))
        {
            if (!AssignFirstFitTargetsV4(player, source, records, cargoWidth, cargoHeight, targetWidths, targetHeights, targetFlips))
            {
                Print("[TransferZ] Sort planner V4 failed: target layout assignment");
                return false;
            }
        }

        OptimizeEquivalentTargetAssignmentsV4(records, targetWidths, targetHeights, targetFlips);
        SortRecordsByTargetV4(records, targetWidths, targetHeights, targetFlips);

        ref TransferZSortPlannerState state = new TransferZSortPlannerState();
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
        state.moves = moves;
        state.cargoWidth = cargoWidth;
        state.cargoHeight = cargoHeight;

        ResetGrid(state.currentGrid, cargoWidth * cargoHeight);
        for (int currentIndex = 0; currentIndex < records.Count(); currentIndex++)
        {
            TransferZSortRecord currentRecord = records.Get(currentIndex);
            if (!RectFree(state.currentGrid, cargoWidth, cargoHeight, currentRecord.row, currentRecord.col, currentRecord.width, currentRecord.height))
            {
                Print("[TransferZ] Sort planner V4 failed: overlapping current cargo geometry at record=" + currentIndex.ToString());
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

        for (int planIndex = 0; planIndex < records.Count(); planIndex++)
        {
            if (!EnsureRecordAtTargetV4(state, planIndex))
            {
                moves.Clear();
                Print("[TransferZ] Sort planner V4 stopped: target=" + planIndex.ToString() + " steps=" + state.stepCount.ToString() + "/" + state.maxSteps.ToString() + " search=" + state.searchCount.ToString() + "/" + state.maxSearch.ToString());
                return false;
            }
        }

        if (!AllRecordsAtTargetV4(records, targetFlips))
        {
            moves.Clear();
            Print("[TransferZ] Sort planner V4 stopped: target layout incomplete");
            return false;
        }
        return true;
    }

    override static int Sort(PlayerBase player, EntityAI source)
    {
        if (!player || !source || !TransferZServerService.IsReachable(player, source) || !source.GetInventory().GetCargo())
        {
            Print("[TransferZ] Sort V4 rejected: invalid or unreachable cargo source");
            return -1;
        }

        ref array<ref TransferZSortRecord> records = new array<ref TransferZSortRecord>();
        int cargoWidth;
        int cargoHeight;
        if (!SnapshotSortRecords(source, records, cargoWidth, cargoHeight))
        {
            Print("[TransferZ] Sort V4 rejected: cargo snapshot failed for " + source.GetType());
            return -1;
        }
        if (records.Count() < 2)
            return 0;

        SortRecordsV4(records);

        ref array<ref TransferZSortMove> moves = new array<ref TransferZSortMove>();
        if (!BuildSortPlanV4(player, source, records, cargoWidth, cargoHeight, moves))
        {
            Print("[TransferZ] Sort V4 failed: no bounded in-cargo rearrangement plan for " + source.GetType());
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
                Print("[TransferZ] Sort V4 stopped after native move validation failed at move=" + moved.ToString() + "/" + moves.Count().ToString());
                return -1;
            }
            moved++;
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
