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

    static bool TransferZCompleteRightDragAtWidget(Widget widget)
    {
        if (!widget || !TransferZOperationDrag.IsActive())
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

        if (TransferZVicinityHeaderControls.TransferZCompleteRightDragAtWidget(widget))
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
    static bool TransferZCompleteRightDragAtWidget(Widget widget)
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
            if (!TransferZHeaderControls.TransferZCompleteRightDragAtWidget(hovered))
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

modded class VicinitySlotsContainer
{
    protected bool m_TransferZVicinityRightDragTracking;
    protected bool m_TransferZVicinityRightDragStarted;
    protected int m_TransferZVicinityRightDragStartX;
    protected int m_TransferZVicinityRightDragStartY;
    protected EntityAI m_TransferZVicinityRightDragRepresentative;

    protected void TransferZResetVicinityRightDrag()
    {
        m_TransferZVicinityRightDragTracking = false;
        m_TransferZVicinityRightDragStarted = false;
        m_TransferZVicinityRightDragRepresentative = null;
    }

    protected EntityAI TransferZResolveVicinityItemFromWidget(Widget w)
    {
        if (!w)
            return null;

        string name = w.GetName();
        name.Replace("PanelWidget", "Render");

        ItemPreviewWidget preview = ItemPreviewWidget.Cast(w.FindAnyWidget(name));
        if (!preview)
            preview = ItemPreviewWidget.Cast(w.FindAnyWidget("Render"));
        if (!preview)
            preview = ItemPreviewWidget.Cast(w);

        if (!preview)
            return null;

        return preview.GetItem();
    }

    protected bool TransferZBuildVicinityClassItems(EntityAI representative, notnull array<EntityAI> matches)
    {
        matches.Clear();
        if (!representative)
            return false;

        string className = representative.GetType();
        ref array<EntityAI> visible = new array<EntityAI>();
        TransferZSnapshotVisibleItems(visible);

        bool representativeStillVisible = false;
        foreach (EntityAI item : visible)
        {
            if (!item)
                continue;

            if (item == representative)
                representativeStillVisible = true;

            if (item.GetType() == className)
                matches.Insert(item);
        }

        return representativeStillVisible && matches.Count() > 0;
    }

    protected void TransferZStartVicinityRightDrag(Widget w)
    {
        TransferZResetVicinityRightDrag();

        if (KeyState(KeyCode.KC_LCONTROL) || KeyState(KeyCode.KC_RCONTROL))
            return;

        EntityAI representative = TransferZResolveVicinityItemFromWidget(w);
        if (!representative)
            return;

        m_TransferZVicinityRightDragRepresentative = representative;
        GetMousePos(m_TransferZVicinityRightDragStartX, m_TransferZVicinityRightDragStartY);
        m_TransferZVicinityRightDragTracking = true;
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.TransferZPollVicinityRightDrag, 16, false);
    }

    protected void TransferZPollVicinityRightDrag()
    {
        if (!m_TransferZVicinityRightDragTracking)
            return;

        bool rightDown = (GetMouseState(MouseState.RIGHT) & 0x80000000) != 0;
        int mouseX;
        int mouseY;
        GetMousePos(mouseX, mouseY);

        if (rightDown)
        {
            if (!m_TransferZVicinityRightDragStarted)
            {
                int dx = mouseX - m_TransferZVicinityRightDragStartX;
                int dy = mouseY - m_TransferZVicinityRightDragStartY;
                if (dx > 5 || dx < -5 || dy > 5 || dy < -5)
                {
                    ref array<EntityAI> matches = new array<EntityAI>();
                    if (!TransferZBuildVicinityClassItems(m_TransferZVicinityRightDragRepresentative, matches))
                    {
                        TransferZResetVicinityRightDrag();
                        return;
                    }

                    // For vicinity the source is a filtered snapshot rather than a
                    // cargo container: only loose visible items whose exact GetType()
                    // matches the representative are handed to the existing vicinity
                    // transfer path.
                    TransferZOperationDrag.BeginVicinity(TransferZOperation.TRANSFER, matches);
                    TransferZHeaderControls.SetOperationDropTargetsVisible(true);
                    m_TransferZVicinityRightDragStarted = true;
                }
            }

            GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.TransferZPollVicinityRightDrag, 16, false);
            return;
        }

        if (m_TransferZVicinityRightDragStarted)
        {
            Widget hovered = GetWidgetUnderCursor();
            if (!TransferZHeaderControls.TransferZCompleteRightDragAtWidget(hovered))
                TransferZHeaderControls.CancelOperationDrag();
        }

        TransferZResetVicinityRightDrag();
    }

    override void MouseButtonDown(Widget w, int x, int y, int button)
    {
        super.MouseButtonDown(w, x, y, button);

        if (button == MouseState.RIGHT)
            TransferZStartVicinityRightDrag(w);
        else if (button == MouseState.LEFT)
            TransferZResetVicinityRightDrag();
    }

    override void MouseClick(Widget w, int x, int y, int button)
    {
        // Once RMB movement crossed the drag threshold, do not also execute
        // DayZ's normal RMB-up action. The polling callback completes the
        // TransferZ drop on the next GUI tick.
        if (button == MouseState.RIGHT && m_TransferZVicinityRightDragStarted)
            return;

        super.MouseClick(w, x, y, button);
    }
}
