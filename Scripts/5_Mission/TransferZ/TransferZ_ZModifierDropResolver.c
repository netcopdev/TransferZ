modded class TransferZHeaderControls
{
    override void OnOperationDropReceived(Widget w, int x, int y, Widget receiver)
    {
        if (TransferZOperationDrag.ShouldResolveWheelCapturedDropAtMouse())
        {
            CompleteModifierDragAtMousePosition();
            return;
        }

        super.OnOperationDropReceived(w, x, y, receiver);
    }
}

// Loaded after TransferZ_OperationDrag.c. Keep modifier batch state alive across
// native Icon teardown and resolve the final destination from live screen geometry.
modded class WidgetEventHandler
{
    override bool OnMouseWheel(Widget w, int x, int y, int wheel)
    {
        if (w && TransferZOperationDrag.IsActive() && w.GetName() == "TransferZHeaderDropTarget")
            TransferZOperationDrag.MarkWheelCapture();

        return super.OnMouseWheel(w, x, y, wheel);
    }

    override bool OnDropReceived(Widget w, int x, int y, Widget reciever)
    {
        bool activeModifierDrag = TransferZOperationDrag.IsModifierItemDrag();
        bool trailingNativeDrop = TransferZOperationDrag.ShouldSuppressNativeDrop();
        if (activeModifierDrag || trailingNativeDrop)
        {
            string receiverName = "<none>";
            if (reciever)
                receiverName = reciever.GetName();

            string draggedName = "<none>";
            if (w)
                draggedName = w.GetName();

            string phase = "active";
            if (!activeModifierDrag)
                phase = "post-release";

            Print("[TransferZ][DragNativeGuard] suppressed native drop phase=" + phase + " dragged=" + draggedName + " receiver=" + receiverName);

            if (activeModifierDrag)
                TransferZHeaderControls.SetOperationDropTargetsVisible(true);
            return true;
        }

        return super.OnDropReceived(w, x, y, reciever);
    }

    override bool OnMouseButtonUp(Widget w, int x, int y, int button)
    {
        if (button == MouseState.LEFT && TransferZOperationDrag.IsModifierItemDrag())
        {
            // TransferZ owns the modifier drag. Cancel DayZ's native widget drag
            // before moving the batch so the representative icon cannot enqueue
            // a second predictive move against its now-stale source location.
            TransferZOperationDrag.ArmNativeDropSuppression();
            Widget nativeDrag = GetDragWidget();
            if (nativeDrag)
                CancelWidgetDragging();

            ItemManager itemManager = ItemManager.GetInstance();
            if (itemManager)
            {
                itemManager.HideDropzones();
                itemManager.SetIsDragging(false);
            }

            TransferZHeaderControls.CompleteModifierDragAtMousePosition();
            return true;
        }

        return super.OnMouseButtonUp(w, x, y, button);
    }
}
