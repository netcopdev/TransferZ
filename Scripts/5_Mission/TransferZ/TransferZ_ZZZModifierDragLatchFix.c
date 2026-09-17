// Shift modifier drags are started from Icon.CreateWhiteBackground(). At that
// point ItemManager.IsDragging() is not a reliable indication that the native
// drag has entered its active state yet. The modifier nature is already known
// by Icon, so latch it explicitly instead of inferring it from ItemManager.
modded class TransferZOperationDrag
{
    static void LatchModifierItemDrag()
    {
        if (!IsActive())
            return;

        SetModifierItemDrag(true);
    }
}

modded class Icon
{
    override void CreateWhiteBackground()
    {
        super.CreateWhiteBackground();

        // TransferZ_Icon.c sets this only after a Shift/Alt item drag has
        // successfully created a TransferZ batch operation. Do not depend on
        // ItemManager.IsDragging() timing to classify that operation.
        if (m_TransferZModifierDragStarted && TransferZOperationDrag.IsActive())
            TransferZOperationDrag.LatchModifierItemDrag();
    }
}

modded class TransferZHeaderControls
{
    override void OnOperationDropReceived(Widget w, int x, int y, Widget receiver)
    {
        // Registered drop callbacks are what DayZ actually delivers after the
        // scroll/capture sequence. For modifier item drags, never trust the
        // callback owner's cached entity; resolve against live mouse geometry.
        if (TransferZOperationDrag.IsModifierItemDrag())
        {
            CompleteModifierDragAtMousePosition();
            return;
        }

        super.OnOperationDropReceived(w, x, y, receiver);
    }
}
