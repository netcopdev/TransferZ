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
                if (!TransferZPointInsideClippedWidget(controls.m_DropHost, scroll, mouseX, mouseY))
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
            bool handled = TransferZOperationDrag.Complete(best.m_Entity);
            TransferZOperationDrag.ClearWheelCapture();
            SetOperationDropTargetsVisible(false);
            RefreshAll();
            return handled;
        }

        bool vicinityHandled = TransferZVicinityHeaderControls.CompleteModifierDragAtMousePosition(mouseX, mouseY);
        TransferZOperationDrag.ClearWheelCapture();
        return vicinityHandled;
    }

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

        bool handled = TransferZOperationDrag.CompleteToVicinity();
        TransferZHeaderControls.SetOperationDropTargetsVisible(false);
        TransferZHeaderControls.RefreshAll();
        return handled;
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

    override bool OnMouseButtonUp(Widget w, int x, int y, int button)
    {
        if (button == MouseState.LEFT && TransferZOperationDrag.IsModifierItemDrag())
        {
            TransferZHeaderControls.CompleteModifierDragAtMousePosition();
            return true;
        }

        return super.OnMouseButtonUp(w, x, y, button);
    }
}
