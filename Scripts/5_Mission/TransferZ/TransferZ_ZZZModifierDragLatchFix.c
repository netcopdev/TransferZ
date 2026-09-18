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
        // Wheel scrolling can make DayZ emit a synthetic/native drop while the
        // player is still physically holding LMB. No TransferZ operation may
        // commit from that event: final destination selection belongs to the
        // actual button release. This also protects the rare case where the
        // modifier latch is temporarily unavailable but the operation is alive.
        if (TransferZOperationDrag.IsActive() && (GetMouseState(MouseState.LEFT) & MB_PRESSED_MASK) != 0)
        {
            int mouseX;
            int mouseY;
            GetMousePos(mouseX, mouseY);

            string receiverName = "<none>";
            if (receiver)
                receiverName = receiver.GetName();

            Widget hovered = GetWidgetUnderCursor();
            string hoveredName = "<none>";
            if (hovered)
                hoveredName = hovered.GetName();

            Print("[TransferZ][DragWheelGuard] ignored premature drop while LMB held receiver=" + receiverName + " hovered=" + hoveredName + " mouse=" + mouseX.ToString() + "," + mouseY.ToString());
            SetOperationDropTargetsVisible(true);
            return;
        }

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
