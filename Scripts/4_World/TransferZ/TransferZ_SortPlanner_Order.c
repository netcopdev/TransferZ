modded class TransferZSortPlanner
{
    override static bool SortBeforeV2(TransferZSortRecord left, TransferZSortRecord right)
    {
        int leftArea = left.width * left.height;
        int rightArea = right.width * right.height;
        if (leftArea != rightArea)
            return leftArea > rightArea;
        if (left.width != right.width)
            return left.width > right.width;
        if (left.height != right.height)
            return left.height > right.height;

        // Identical-shape items keep their current spatial order before type tie-breaks.
        // This avoids forcing same-size cargo into swap cycles when no same-size staging
        // rectangle exists, while preserving the final largest-first compact layout.
        if (left.row != right.row)
            return left.row < right.row;
        if (left.col != right.col)
            return left.col < right.col;
        return left.typeHash < right.typeHash;
    }
}
