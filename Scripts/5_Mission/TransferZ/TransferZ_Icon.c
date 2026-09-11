modded class Icon
{
    protected static const int TRANSFERZ_DRAG_NONE = 0;
    protected static const int TRANSFERZ_DRAG_TRANSFER = 1;
    protected static const int TRANSFERZ_DRAG_UNPACK = 2;

    protected int m_TransferZModifierDragMode = TRANSFERZ_DRAG_NONE;
    protected bool m_TransferZModifierDragStarted;
    protected EntityAI m_TransferZModifierDragSource;

    protected bool m_TransferZModifierClickRegistered;
    protected bool m_TransferZModifierClickPending;
    protected int m_TransferZModifierClickMode;

    protected bool m_TransferZRightDragTracking;
    protected bool m_TransferZRightDragStarted;
    protected int m_TransferZRightDragStartX;
    protected int m_TransferZRightDragStartY;
    protected EntityAI m_TransferZRightDragSource;
    protected EntityAI m_TransferZRightDragRepresentative;

    override void InitEx(EntityAI obj, bool refresh = true)
    {
        super.InitEx(obj, refresh);

        if (!m_TransferZModifierClickRegistered && GetMainWidget())
        {
            WidgetEventHandler.GetInstance().RegisterOnMouseButtonUp(GetMainWidget(), this, "TransferZOnModifierMouseButtonUp");
            m_TransferZModifierClickRegistered = true;
        }
    }

    protected EntityAI TransferZGetDirectCargoSource()
    {
        if (!m_Obj || m_HandsIcon)
            return null;

        InventoryLocation location = new InventoryLocation();
        if (!m_Obj.GetInventory().GetCurrentInventoryLocation(location) || location.GetType() != InventoryLocationType.CARGO)
            return null;

        EntityAI source = location.GetParent();
        if (!source || !source.GetInventory().GetCargo())
            return null;
        return source;
    }

    protected bool TransferZIsDescendantOf(EntityAI entity, EntityAI ancestor)
    {
        if (!entity || !ancestor)
            return false;

        EntityAI current = entity;
        while (current)
        {
            if (current == ancestor)
                return true;
            current = current.GetHierarchyParent();
        }
        return false;
    }

    protected bool TransferZIsPlayerInventorySource(EntityAI source)
    {
        PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
        if (!player || !source || source.GetHierarchyRoot() != player)
            return false;

        EntityAI hands = player.GetEntityInHands();
        if (hands && TransferZIsDescendantOf(source, hands))
            return false;
        return true;
    }

    protected EntityAI TransferZResolveDoubleClickDestination(EntityAI source)
    {
        if (!source)
            return null;

        TransferZClientState state = TransferZClientState.Get();
        EntityAI destination = state.GetLinkedDestination(source);
        if (destination)
            return destination;

        destination = state.GetPreferredDestination();
        if (!destination || destination == source)
            return null;
        return destination;
    }

    protected bool TransferZRouteSingleDoubleClick(EntityAI source)
    {
        if (!source || !m_Obj)
            return false;

        TransferZClientState state = TransferZClientState.Get();
        EntityAI linked = state.GetLinkedDestination(source);
        if (linked)
            return state.RequestMoveItem(m_Obj, linked);

        if (TransferZIsPlayerInventorySource(source))
            return false;

        EntityAI destination = state.GetPreferredDestination();
        if (!destination || destination == source || !state.CanPreferredAcceptItem(m_Obj, destination))
            return false;
        return state.RequestMoveItem(m_Obj, destination);
    }

    protected bool TransferZRouteClassDoubleClick(EntityAI source)
    {
        if (!source || !m_Obj)
            return false;

        EntityAI destination = TransferZResolveDoubleClickDestination(source);
        if (!destination)
            return false;
        return TransferZClientState.Get().RequestClassTransferTo(source, destination, m_Obj);
    }

    protected int TransferZReadModifierDragMode()
    {
        if (KeyState(KeyCode.KC_LCONTROL) || KeyState(KeyCode.KC_RCONTROL))
            return TRANSFERZ_DRAG_NONE;

        bool shiftDown = KeyState(KeyCode.KC_LSHIFT) || KeyState(KeyCode.KC_RSHIFT);
        bool altDown = KeyState(KeyCode.KC_LMENU) || KeyState(KeyCode.KC_RMENU);
        if (shiftDown == altDown)
            return TRANSFERZ_DRAG_NONE;
        if (shiftDown)
            return TRANSFERZ_DRAG_TRANSFER;
        return TRANSFERZ_DRAG_UNPACK;
    }

    protected void TransferZResetModifierDrag()
    {
        m_TransferZModifierDragMode = TRANSFERZ_DRAG_NONE;
        m_TransferZModifierDragStarted = false;
        m_TransferZModifierDragSource = null;
    }

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
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(TransferZPollRightDrag, 16, false);
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

            GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(TransferZPollRightDrag, 16, false);
            return;
        }

        if (m_TransferZRightDragStarted)
        {
            Widget hovered = GetWidgetUnderCursor();
            if (!TransferZHeaderControls.CompleteRightDragAtWidget(hovered))
                TransferZHeaderControls.CancelOperationDrag();
        }

        TransferZResetRightDragTracking();
    }

    override void MouseClick(Widget w, int x, int y, int button)
    {
        if (button == MouseState.LEFT)
        {
            TransferZResetModifierDrag();
            TransferZResetRightDragTracking();
            m_TransferZModifierDragSource = TransferZGetDirectCargoSource();
            if (m_TransferZModifierDragSource && m_Obj)
                m_TransferZModifierDragMode = TransferZReadModifierDragMode();
        }

        super.MouseClick(w, x, y, button);

        if (button == MouseState.LEFT)
        {
            m_TransferZModifierClickMode = m_TransferZModifierDragMode;
            m_TransferZModifierClickPending = m_TransferZModifierClickMode != TRANSFERZ_DRAG_NONE && m_Obj && !m_HandsIcon;
        }
        else if (button == MouseState.RIGHT)
        {
            TransferZStartRightDragTracking();
        }
    }

    override void CreateWhiteBackground()
    {
        super.CreateWhiteBackground();

        if (m_TransferZModifierDragMode == TRANSFERZ_DRAG_NONE || !m_Obj || !m_TransferZModifierDragSource)
            return;

        InventoryLocation location = new InventoryLocation();
        if (!m_Obj.GetInventory().GetCurrentInventoryLocation(location))
            return;
        if (location.GetType() != InventoryLocationType.CARGO || location.GetParent() != m_TransferZModifierDragSource)
            return;

        if (m_TransferZModifierDragMode == TRANSFERZ_DRAG_TRANSFER)
            TransferZOperationDrag.BeginContainer(TransferZOperation.TRANSFER, m_TransferZModifierDragSource);
        else if (m_TransferZModifierDragMode == TRANSFERZ_DRAG_UNPACK)
            TransferZOperationDrag.BeginContainer(TransferZOperation.UNPACK, m_TransferZModifierDragSource);
        else
            return;

        m_TransferZModifierClickPending = false;
        TransferZHeaderControls.SetOperationDropTargetsVisible(true);
        m_TransferZModifierDragStarted = true;
    }

    override void DestroyWhiteBackground()
    {
        if (m_TransferZModifierDragStarted)
        {
            m_TransferZModifierClickPending = false;
            TransferZHeaderControls.CancelOperationDragLater();
        }

        super.DestroyWhiteBackground();
        TransferZResetModifierDrag();
    }

    void TransferZOnModifierMouseButtonUp(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT)
            return;

        bool pending = m_TransferZModifierClickPending;
        int mode = m_TransferZModifierClickMode;
        m_TransferZModifierClickPending = false;
        m_TransferZModifierClickMode = TRANSFERZ_DRAG_NONE;

        if (!pending || !m_Obj || m_HandsIcon)
            return;

        TransferZClientState state = TransferZClientState.Get();
        if (mode == TRANSFERZ_DRAG_TRANSFER)
            state.RequestItemToDestination(m_Obj);
        else if (mode == TRANSFERZ_DRAG_UNPACK)
            state.RequestItemToPreferred(m_Obj);
    }

    override void DoubleClick(Widget w, int x, int y, int button)
    {
        if (!g_Game.IsLeftCtrlDown() && !m_HandsIcon && m_Obj)
        {
            EntityAI source = TransferZGetDirectCargoSource();
            if (source)
            {
                if (button == MouseState.LEFT && TransferZRouteSingleDoubleClick(source))
                    return;
                if (button == MouseState.RIGHT && TransferZRouteClassDoubleClick(source))
                    return;
            }
        }

        super.DoubleClick(w, x, y, button);
    }
}
