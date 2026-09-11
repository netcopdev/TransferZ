modded class TransferZHeaderControls
{
    protected static bool TransferZWidgetIsWithin(Widget widget, Widget root)
    {
        Widget current = widget;
        while (current)
        {
            if (current == root)
                return true;
            current = current.GetParent();
        }
        return false;
    }

    static bool TransferZCompleteClassDropAtWidget(Widget widget)
    {
        if (!widget || !TransferZOperationDrag.IsClassTransfer())
            return false;

        if (s_Instances)
        {
            for (int i = s_Instances.Count() - 1; i >= 0; i--)
            {
                TransferZHeaderControls controls = s_Instances.Get(i);
                if (!controls || !controls.m_DropTarget || !controls.m_DropTarget.IsVisibleHierarchy())
                    continue;

                if (TransferZWidgetIsWithin(widget, controls.m_DropTarget))
                {
                    bool handled = TransferZOperationDrag.Complete(controls.m_Entity);
                    SetOperationDropTargetsVisible(false);
                    RefreshAll();
                    return handled;
                }
            }
        }

        if (TransferZVicinityHeaderControls.TransferZCompleteClassDropAtWidget(widget))
        {
            SetOperationDropTargetsVisible(false);
            RefreshAll();
            return true;
        }

        return false;
    }
}

modded class TransferZVicinityHeaderControls
{
    static bool TransferZCompleteClassDropAtWidget(Widget widget)
    {
        if (!widget || !s_Instance || !s_Instance.m_DropTarget || !s_Instance.m_DropTarget.IsVisibleHierarchy())
            return false;

        Widget current = widget;
        while (current)
        {
            if (current == s_Instance.m_DropTarget)
                return TransferZOperationDrag.CompleteToVicinity();
            current = current.GetParent();
        }

        return false;
    }
}

modded class Icon
{
    protected bool m_TransferZRightDragTracking;
    protected bool m_TransferZRightDragStarted;
    protected int m_TransferZRightDragStartX;
    protected int m_TransferZRightDragStartY;
    protected EntityAI m_TransferZRightDragSource;
    protected EntityAI m_TransferZRightDragRepresentative;

    protected void TransferZResetRightDragTracking()
    {
        m_TransferZRightDragTracking = false;
        m_TransferZRightDragStarted = false;
        m_TransferZRightDragSource = null;
        m_TransferZRightDragRepresentative = null;
    }

    protected bool TransferZRightDragSourceStillValid()
    {
        if (!m_TransferZRightDragSource || !m_TransferZRightDragRepresentative)
            return false;

        InventoryLocation location = new InventoryLocation();
        if (!m_TransferZRightDragRepresentative.GetInventory().GetCurrentInventoryLocation(location))
            return false;

        return location.GetType() == InventoryLocationType.CARGO && location.GetParent() == m_TransferZRightDragSource;
    }

    protected void TransferZStartRightDragTracking()
    {
        TransferZResetRightDragTracking();

        if (!m_Obj || m_HandsIcon)
            return;
        if (KeyState(KeyCode.KC_LCONTROL) || KeyState(KeyCode.KC_RCONTROL))
            return;

        EntityAI source = TransferZGetDirectCargoSource();
        if (!source)
            return;

        m_TransferZRightDragSource = source;
        m_TransferZRightDragRepresentative = m_Obj;
        GetMousePos(m_TransferZRightDragStartX, m_TransferZRightDragStartY);
        m_TransferZRightDragTracking = true;
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.TransferZPollRightDrag, 16, false);
    }

    protected void TransferZPollRightDrag()
    {
        if (!m_TransferZRightDragTracking)
            return;

        bool rightDown = (GetMouseState(MouseState.RIGHT) & 0x80000000) != 0;
        int mouseX;
        int mouseY;
        GetMousePos(mouseX, mouseY);

        if (rightDown)
        {
            if (!m_TransferZRightDragStarted)
            {
                int dx = mouseX - m_TransferZRightDragStartX;
                int dy = mouseY - m_TransferZRightDragStartY;
                if (dx > 5 || dx < -5 || dy > 5 || dy < -5)
                {
                    if (!TransferZRightDragSourceStillValid())
                    {
                        TransferZResetRightDragTracking();
                        return;
                    }

                    TransferZOperationDrag.BeginClassTransfer(m_TransferZRightDragSource, m_TransferZRightDragRepresentative);
                    TransferZHeaderControls.SetOperationDropTargetsVisible(true);
                    m_TransferZRightDragStarted = true;
                }
            }

            GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.TransferZPollRightDrag, 16, false);
            return;
        }

        if (m_TransferZRightDragStarted)
        {
            Widget hovered = GetWidgetUnderCursor();
            if (!TransferZHeaderControls.TransferZCompleteClassDropAtWidget(hovered))
                TransferZHeaderControls.CancelOperationDrag();
        }

        TransferZResetRightDragTracking();
    }

    override void MouseClick(Widget w, int x, int y, int button)
    {
        super.MouseClick(w, x, y, button);

        if (button == MouseState.RIGHT)
            TransferZStartRightDragTracking();
        else if (button == MouseState.LEFT)
            TransferZResetRightDragTracking();
    }
}
