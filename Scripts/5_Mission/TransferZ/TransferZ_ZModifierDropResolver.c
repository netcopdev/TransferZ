modded class TransferZOperationDrag
{
    protected static int s_TransferZLastWheelCaptureAt;
    protected static int s_TransferZSuppressNativeDropUntil;

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

    static void ArmNativeDropSuppression()
    {
        // DayZ can queue the native dragged icon's drop just before/after the
        // mouse-up callback. TransferZ owns modifier drags, so swallow only
        // that short trailing event window after the authoritative release.
        s_TransferZSuppressNativeDropUntil = GetGame().GetTime() + 250;
    }

    static bool ShouldSuppressNativeDrop()
    {
        return s_TransferZSuppressNativeDropUntil > 0 && GetGame().GetTime() <= s_TransferZSuppressNativeDropUntil;
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

        return mouseX >= left && mouseX < right && mouseY >= top && mouseY < bottom;
    }

    static bool CompleteModifierDragAtMousePosition()
    {
        if (!TransferZOperationDrag.IsActive())
            return false;

        int mouseX;
        int mouseY;
        GetMousePos(mouseX, mouseY);

        EntityAI source = TransferZOperationDrag.GetSource();
        string sourceName = "<vicinity>";
        if (source)
            sourceName = source.GetType();

        Widget hovered = GetWidgetUnderCursor();
        string hoveredName = "<none>";
        if (hovered)
            hoveredName = hovered.GetName();

        bool wheelRecent = TransferZOperationDrag.ShouldResolveWheelCapturedDropAtMouse();
        string wheelState = "no";
        if (wheelRecent)
            wheelState = "yes";

        // Prefer the live drop-target widget beneath the cursor, but never trust
        // widget ancestry alone after scrolling. Mouse capture can leave
        // GetWidgetUnderCursor() pointing at the previously captured overlay.
        // The current mouse point must also lie inside that container's live,
        // clipped drop-host rectangle before the hovered widget may win.
        if (hovered && s_Instances)
        {
            for (int hoverIndex = s_Instances.Count() - 1; hoverIndex >= 0; hoverIndex--)
            {
                TransferZHeaderControls hoveredControls = s_Instances.Get(hoverIndex);
                if (!hoveredControls || !hoveredControls.m_Entity || !hoveredControls.m_DropTarget || !hoveredControls.m_DropTarget.IsVisibleHierarchy() || !hoveredControls.IsOpenTarget())
                    continue;
                if (!hoveredControls.m_Entity.GetInventory().GetCargo())
                    continue;

                if (source && source == hoveredControls.m_Entity)
                {
                    bool hoveredSelfUnpack = TransferZOperationDrag.GetOperation() == TransferZOperation.UNPACK && !TransferZOperationDrag.IsClassTransfer() && !TransferZOperationDrag.IsFromVicinity();
                    if (!hoveredSelfUnpack)
                        continue;
                }

                if (!WidgetIsWithin(hovered, hoveredControls.m_DropTarget))
                    continue;

                ScrollWidget hoveredScroll = hoveredControls.FindScrollWidget();
                if (!TransferZPointInsideClippedWidget(hoveredControls.m_DropHost, hoveredScroll, mouseX, mouseY))
                {
                    Print("[TransferZ][DragResolve] rejected stale hovered candidate=" + hoveredControls.m_Entity.GetType() + " hovered=" + hoveredName + " mouse=" + mouseX.ToString() + "," + mouseY.ToString() + " wheelRecent=" + wheelState);
                    continue;
                }

                Print("[TransferZ][DragResolve] path=hovered source=" + sourceName + " destination=" + hoveredControls.m_Entity.GetType() + " hovered=" + hoveredName + " mouse=" + mouseX.ToString() + "," + mouseY.ToString() + " wheelRecent=" + wheelState);
                bool hoveredHandled = TransferZOperationDrag.Complete(hoveredControls.m_Entity);
                TransferZOperationDrag.ClearWheelCapture();
                SetOperationDropTargetsVisible(false);
                RefreshAll();
                return hoveredHandled;
            }
        }

        TransferZHeaderControls best;
        float bestArea = 999999999.0;

        // Mouse capture can occasionally prevent GetWidgetUnderCursor() from
        // exposing the overlay. Fall back to live container screen geometry.
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
            Print("[TransferZ][DragResolve] path=geometry source=" + sourceName + " destination=" + best.m_Entity.GetType() + " hovered=" + hoveredName + " mouse=" + mouseX.ToString() + "," + mouseY.ToString() + " wheelRecent=" + wheelState);
            bool handled = TransferZOperationDrag.Complete(best.m_Entity);
            TransferZOperationDrag.ClearWheelCapture();
            SetOperationDropTargetsVisible(false);
            RefreshAll();
            return handled;
        }

        bool vicinityHandled = TransferZVicinityHeaderControls.CompleteModifierDragAtMousePosition(mouseX, mouseY);
        if (!vicinityHandled && TransferZOperationDrag.IsActive())
            Print("[TransferZ][DragResolve] path=no-target source=" + sourceName + " hovered=" + hoveredName + " mouse=" + mouseX.ToString() + "," + mouseY.ToString() + " wheelRecent=" + wheelState);

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
        if (mouseX < left || mouseX >= right || mouseY < top || mouseY >= bottom)
            return false;

        EntityAI source = TransferZOperationDrag.GetSource();
        string sourceName = "<vicinity>";
        if (source)
            sourceName = source.GetType();

        Print("[TransferZ][DragResolve] path=vicinity source=" + sourceName + " mouse=" + mouseX.ToString() + "," + mouseY.ToString());
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
