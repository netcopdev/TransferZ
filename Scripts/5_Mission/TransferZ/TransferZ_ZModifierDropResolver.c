modded class TransferZOperationDrag
{
    protected static int s_TransferZLastWheelCaptureAt;

    static void MarkWheelCapture()
    {
        if (IsActive())
            s_TransferZLastWheelCaptureAt = GetGame().GetTime();
    }

    static bool ShouldResolveWheelCapturedDropAtMouse()
    {
        if (!IsActive() || s_TransferZLastWheelCaptureAt <= 0)
            return false;

        return GetGame().GetTime() - s_TransferZLastWheelCaptureAt < 10000;
    }

    static void ClearWheelCapture()
    {
        s_TransferZLastWheelCaptureAt = 0;
    }
}

modded class TransferZHeaderControls
{
    protected static bool TransferZPointInsideClippedWidget(Widget widget, ScrollWidget scroll, int mouseX, int mouseY)
    {
        if (!widget || !widget.IsVisibleHierarchy())
            return false;

        float x;
        float y;
        float w;
        float h;
        widget.GetScreenPos(x, y);
        widget.GetScreenSize(w, h);
        if (w <= 0.0 || h <= 0.0)
            return false;

        float left = x;
        float top = y;
        float right = x + w;
        float bottom = y + h;

        if (scroll && scroll.IsVisibleHierarchy())
        {
            float sx;
            float sy;
            float sw;
            float sh;
            scroll.GetScreenPos(sx, sy);
            scroll.GetScreenSize(sw, sh);

            if (sx > left)
                left = sx;
            if (sy > top)
                top = sy;
            if (sx + sw < right)
                right = sx + sw;
            if (sy + sh < bottom)
                bottom = sy + sh;
        }

        if (right <= left || bottom <= top)
            return false;

        return mouseX >= left && mouseX <= right && mouseY >= top && mouseY <= bottom;
    }

    static bool CompleteModifierDragAtMousePosition()
    {
        if (!TransferZOperationDrag.IsActive())
            return false;

        int mouseX;
        int mouseY;
        GetMousePos(mouseX, mouseY);
        Print("[TransferZ][DragDiag] RESOLVE mouse=" + mouseX.ToString() + "," + mouseY.ToString());

        EntityAI source = TransferZOperationDrag.GetSource();
        TransferZHeaderControls best;
        float bestArea = 999999999.0;

        if (s_Instances)
        {
            for (int i = s_Instances.Count() - 1; i >= 0; i--)
            {
                TransferZHeaderControls controls = s_Instances.Get(i);
                if (!controls || !controls.m_Entity || !controls.m_DropHost || !controls.IsOpenTarget())
                    continue;
                if (!controls.m_Entity.GetInventory().GetCargo())
                    continue;

                if (source && source == controls.m_Entity)
                {
                    bool selfUnpack = TransferZOperationDrag.GetOperation() == TransferZOperation.UNPACK && !TransferZOperationDrag.IsClassTransfer() && !TransferZOperationDrag.IsFromVicinity();
                    if (!selfUnpack)
                        continue;
                }

                float x;
                float y;
                float w;
                float h;
                controls.m_DropHost.GetScreenPos(x, y);
                controls.m_DropHost.GetScreenSize(w, h);

                ScrollWidget scroll = controls.FindScrollWidget();
                bool inside = TransferZPointInsideClippedWidget(controls.m_DropHost, scroll, mouseX, mouseY);
                Print("[TransferZ][DragDiag] CANDIDATE entity=" + controls.m_Entity.GetType() + " pos=" + x.ToString() + "," + y.ToString() + " size=" + w.ToString() + "x" + h.ToString() + " inside=" + inside.ToString());
                if (!inside)
                    continue;

                float area = w * h;
                if (!best || area < bestArea)
                {
                    best = controls;
                    bestArea = area;
                }
            }
        }

        if (best)
        {
            Print("[TransferZ][DragDiag] RESOLVE_CHOSEN cargo=" + best.m_Entity.GetType());
            bool handled = TransferZOperationDrag.Complete(best.m_Entity);
            TransferZOperationDrag.ClearWheelCapture();
            SetOperationDropTargetsVisible(false);
            RefreshAll();
            return handled;
        }

        Print("[TransferZ][DragDiag] RESOLVE_NO_CARGO trying vicinity");
        bool vicinityHandled = TransferZVicinityHeaderControls.CompleteModifierDragAtMousePosition(mouseX, mouseY);
        TransferZOperationDrag.ClearWheelCapture();
        return vicinityHandled;
    }

    override void OnOperationDropReceived(Widget w, int x, int y, Widget receiver)
    {
        if (TransferZOperationDrag.ShouldResolveWheelCapturedDropAtMouse())
        {
            string captured = "<none>";
            if (m_Entity)
                captured = m_Entity.GetType();
            int mouseX;
            int mouseY;
            GetMousePos(mouseX, mouseY);
            Print("[TransferZ][DragDiag] REGISTERED_DROP captured=" + captured + " event=" + x.ToString() + "," + y.ToString() + " actual=" + mouseX.ToString() + "," + mouseY.ToString());
            CompleteModifierDragAtMousePosition();
            return;
        }

        super.OnOperationDropReceived(w, x, y, receiver);
    }
}

modded class TransferZVicinityHeaderControls
{
    static bool CompleteModifierDragAtMousePosition(int mouseX, int mouseY)
    {
        if (!TransferZOperationDrag.IsActive() || !s_Instance || !s_Instance.m_Source || !IsVicinityOpen())
            return false;

        if (TransferZOperationDrag.IsFromVicinity() && TransferZOperationDrag.GetOperation() == TransferZOperation.TRANSFER)
            return false;

        Widget root = s_Instance.m_Source.GetRootWidget();
        if (!root || !root.IsVisibleHierarchy())
            return false;

        float x;
        float y;
        float w;
        float h;
        root.GetScreenPos(x, y);
        root.GetScreenSize(w, h);

        ScrollWidget scroll;
        if (s_Instance.m_Owner)
            scroll = s_Instance.m_Owner.TransferZGetScrollWidget();

        float left = x;
        float top = y;
        float right = x + w;
        float bottom = y + h;

        if (scroll && scroll.IsVisibleHierarchy())
        {
            float sx;
            float sy;
            float sw;
            float sh;
            scroll.GetScreenPos(sx, sy);
            scroll.GetScreenSize(sw, sh);
            if (sx > left)
                left = sx;
            if (sy > top)
                top = sy;
            if (sx + sw < right)
                right = sx + sw;
            if (sy + sh < bottom)
                bottom = sy + sh;
        }

        if (right <= left || bottom <= top)
            return false;
        if (mouseX < left || mouseX > right || mouseY < top || mouseY > bottom)
            return false;

        Print("[TransferZ][DragDiag] RESOLVE_CHOSEN vicinity");
        bool handled = TransferZOperationDrag.CompleteToVicinity();
        TransferZHeaderControls.SetOperationDropTargetsVisible(false);
        TransferZHeaderControls.RefreshAll();
        return handled;
    }
}

// This file intentionally sorts after TransferZ_OperationDrag.c. The earlier
// WidgetEventHandler layer keeps the modifier session alive across native Icon
// teardown. This outer layer resolves the final destination from live screen
// geometry and also intercepts the registered drop callback when mouse capture
// remains stuck on the first overlay that handled the wheel.
modded class WidgetEventHandler
{
    override bool OnMouseWheel(Widget w, int x, int y, int wheel)
    {
        if (w && TransferZOperationDrag.IsActive() && w.GetName() == "TransferZHeaderDropTarget")
            TransferZOperationDrag.MarkWheelCapture();

        return super.OnMouseWheel(w, x, y, wheel);
    }

    override bool OnMouseButtonUp(Widget w, int x, int y, int button)
    {
        if (button == MouseState.LEFT && TransferZOperationDrag.IsModifierItemDrag())
        {
            string widgetName = "<null>";
            if (w)
                widgetName = w.GetName();
            int mouseX;
            int mouseY;
            GetMousePos(mouseX, mouseY);
            Print("[TransferZ][DragDiag] MOUSE_UP widget=" + widgetName + " event=" + x.ToString() + "," + y.ToString() + " actual=" + mouseX.ToString() + "," + mouseY.ToString());
            TransferZHeaderControls.CompleteModifierDragAtMousePosition();
            return true;
        }

        return super.OnMouseButtonUp(w, x, y, button);
    }
}
