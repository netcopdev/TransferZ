modded class Icon
{
    protected bool m_TransferZRightDragArmed;
    protected bool m_TransferZClassDragStarted;
    protected EntityAI m_TransferZClassDragSource;

    void Icon(LayoutHolder parent, bool hands_icon = false)
    {
        Widget mainWidget = GetMainWidget();
        if (mainWidget)
            WidgetEventHandler.GetInstance().RegisterOnMouseButtonUp(mainWidget, this, "TransferZMouseButtonUp");
    }

    protected EntityAI TransferZGetDirectCargoSource()
    {
        if (!m_Obj || m_HandsIcon)
            return null;

        InventoryLocation location = new InventoryLocation();
        if (!m_Obj.GetInventory().GetCurrentInventoryLocation(location))
            return null;
        if (location.GetType() != InventoryLocationType.CARGO)
            return null;

        EntityAI source = location.GetParent();
        if (!source || !source.GetInventory().GetCargo())
            return null;
        return source;
    }

    override void MouseClick(Widget w, int x, int y, int button)
    {
        if (button == MouseState.RIGHT)
        {
            m_TransferZRightDragArmed = false;
            m_TransferZClassDragStarted = false;
            m_TransferZClassDragSource = TransferZGetDirectCargoSource();
            if (m_TransferZClassDragSource && m_Obj)
                m_TransferZRightDragArmed = true;
        }

        super.MouseClick(w, x, y, button);
    }

    override void CreateWhiteBackground()
    {
        super.CreateWhiteBackground();

        if (!m_TransferZRightDragArmed || !m_Obj || !m_TransferZClassDragSource)
            return;

        InventoryLocation location = new InventoryLocation();
        if (!m_Obj.GetInventory().GetCurrentInventoryLocation(location))
            return;
        if (location.GetType() != InventoryLocationType.CARGO || location.GetParent() != m_TransferZClassDragSource)
            return;

        TransferZOperationDrag.BeginClassTransfer(m_TransferZClassDragSource, m_Obj);
        TransferZHeaderControls.SetOperationDropTargetsVisible(true);
        m_TransferZClassDragStarted = true;
        m_TransferZRightDragArmed = false;
    }

    override void DestroyWhiteBackground()
    {
        super.DestroyWhiteBackground();

        if (m_TransferZClassDragStarted)
        {
            TransferZHeaderControls.CancelOperationDragLater();
            m_TransferZClassDragStarted = false;
        }

        m_TransferZRightDragArmed = false;
        m_TransferZClassDragSource = null;
    }

    void TransferZMouseButtonUp(Widget w, int x, int y, int button)
    {
        if (button != MouseState.RIGHT)
            return;

        if (!m_TransferZClassDragStarted)
        {
            m_TransferZRightDragArmed = false;
            m_TransferZClassDragSource = null;
        }
    }

    override void DoubleClick(Widget w, int x, int y, int button)
    {
        if (button == MouseState.LEFT && !g_Game.IsLeftCtrlDown() && !m_HandsIcon && m_Obj)
        {
            CargoContainer sourceContainer = CargoContainer.Cast(m_Parent);
            if (sourceContainer)
            {
                EntityAI source = sourceContainer.GetEntity();
                if (TransferZClientState.Get().TryRouteCargoDoubleClick(source, m_Obj))
                    return;
            }
        }

        super.DoubleClick(w, x, y, button);
    }
}
