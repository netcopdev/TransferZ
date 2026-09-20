modded class Icon
{
    protected static const int TRANSFERZ_MODIFIER_NONE = 0;
    protected static const int TRANSFERZ_MODIFIER_SHIFT = 1;
    protected static const int TRANSFERZ_MODIFIER_ALT = 2;

    protected int m_TransferZModifierDragMode = TRANSFERZ_MODIFIER_NONE;
    protected bool m_TransferZModifierDragStarted;
    protected EntityAI m_TransferZModifierDragSource;
    protected int m_TransferZModifierDragSourceCargoIndex = 0;

    protected bool m_TransferZModifierClickRegistered;
    protected bool m_TransferZModifierClickPending;
    protected int m_TransferZModifierClickMode;

    override void InitEx(EntityAI obj, bool refresh = true)
    {
        super.InitEx(obj, refresh);

        if (!m_TransferZModifierClickRegistered && GetMainWidget())
        {
            WidgetEventHandler.GetInstance().RegisterOnMouseButtonUp(GetMainWidget(), this, "TransferZOnModifierMouseButtonUp");
            m_TransferZModifierClickRegistered = true;
        }
    }

    protected EntityAI TransferZGetDirectCargoSource(out int cargoIndex)
    {
        cargoIndex = 0;
        if (!m_Obj || m_HandsIcon)
            return null;

        InventoryLocation location = new InventoryLocation();
        if (!m_Obj.GetInventory().GetCurrentInventoryLocation(location) || location.GetType() != InventoryLocationType.CARGO)
            return null;

        EntityAI source = location.GetParent();
        cargoIndex = location.GetIdx();
        if (!source || !TransferZCargo.Exists(source, cargoIndex))
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

    protected bool TransferZRouteSingleDoubleClick(EntityAI source, int sourceCargoIndex)
    {
        if (!source || !m_Obj)
            return false;

        TransferZClientState state = TransferZClientState.Get();
        EntityAI linked = state.GetLinkedDestination(source, sourceCargoIndex);
        if (linked)
            return state.RequestMoveItem(m_Obj, linked, state.GetLinkedDestinationCargoIndex(source, sourceCargoIndex));

        if (TransferZIsPlayerInventorySource(source))
            return false;

        EntityAI destination = state.GetPreferredDestination();
        int destinationCargoIndex = state.GetPreferredCargoIndex();
        if (!destination || (destination == source && destinationCargoIndex == sourceCargoIndex) || !state.CanPreferredAcceptItem(m_Obj, destination, destinationCargoIndex))
            return false;
        return state.RequestMoveItem(m_Obj, destination, destinationCargoIndex);
    }

    protected int TransferZReadModifierDragMode()
    {
        if (KeyState(KeyCode.KC_LCONTROL) || KeyState(KeyCode.KC_RCONTROL))
            return TRANSFERZ_MODIFIER_NONE;

        bool shiftDown = KeyState(KeyCode.KC_LSHIFT) || KeyState(KeyCode.KC_RSHIFT);
        bool altDown = KeyState(KeyCode.KC_LMENU) || KeyState(KeyCode.KC_RMENU);
        if (shiftDown == altDown)
            return TRANSFERZ_MODIFIER_NONE;
        if (shiftDown)
            return TRANSFERZ_MODIFIER_SHIFT;
        return TRANSFERZ_MODIFIER_ALT;
    }

    protected void TransferZResetModifierDrag()
    {
        m_TransferZModifierDragMode = TRANSFERZ_MODIFIER_NONE;
        m_TransferZModifierDragStarted = false;
        m_TransferZModifierDragSource = null;
        m_TransferZModifierDragSourceCargoIndex = 0;
    }

    override void MouseClick(Widget w, int x, int y, int button)
    {
        if (button == MouseState.LEFT)
        {
            TransferZResetModifierDrag();
            m_TransferZModifierDragSource = TransferZGetDirectCargoSource(m_TransferZModifierDragSourceCargoIndex);
            if (m_TransferZModifierDragSource && m_Obj)
                m_TransferZModifierDragMode = TransferZReadModifierDragMode();
        }

        super.MouseClick(w, x, y, button);

        if (button == MouseState.LEFT)
        {
            m_TransferZModifierClickMode = m_TransferZModifierDragMode;
            m_TransferZModifierClickPending = m_TransferZModifierClickMode != TRANSFERZ_MODIFIER_NONE && m_Obj && !m_HandsIcon;
        }
    }

    override void CreateWhiteBackground()
    {
        super.CreateWhiteBackground();

        if (m_TransferZModifierDragMode == TRANSFERZ_MODIFIER_NONE || !m_Obj || !m_TransferZModifierDragSource)
            return;

        InventoryLocation location = new InventoryLocation();
        if (!m_Obj.GetInventory().GetCurrentInventoryLocation(location))
            return;
        if (!TransferZCargo.LocationMatches(location, m_TransferZModifierDragSource, m_TransferZModifierDragSourceCargoIndex))
            return;

        if (m_TransferZModifierDragMode == TRANSFERZ_MODIFIER_SHIFT)
            TransferZOperationDrag.BeginContainer(TransferZOperation.TRANSFER, m_TransferZModifierDragSource, m_TransferZModifierDragSourceCargoIndex);
        else if (m_TransferZModifierDragMode == TRANSFERZ_MODIFIER_ALT)
            TransferZOperationDrag.BeginClassTransfer(m_TransferZModifierDragSource, m_Obj, m_TransferZModifierDragSourceCargoIndex);
        else
            return;

        m_TransferZModifierClickPending = false;
        TransferZHeaderControls.SetOperationDropTargetsVisible(true);
        m_TransferZModifierDragStarted = true;
        TransferZOperationDrag.LatchModifierItemDrag();
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
        m_TransferZModifierClickMode = TRANSFERZ_MODIFIER_NONE;

        if (!pending || !m_Obj || m_HandsIcon)
            return;

        TransferZClientState state = TransferZClientState.Get();
        if (mode == TRANSFERZ_MODIFIER_SHIFT)
            state.RequestItemToDestination(m_Obj);
        else if (mode == TRANSFERZ_MODIFIER_ALT)
            state.RequestItemToPreferred(m_Obj);
    }

    override void DoubleClick(Widget w, int x, int y, int button)
    {
        if (button == MouseState.LEFT && !g_Game.IsLeftCtrlDown() && !m_HandsIcon && m_Obj)
        {
            int sourceCargoIndex;
            EntityAI source = TransferZGetDirectCargoSource(sourceCargoIndex);
            if (source && TransferZRouteSingleDoubleClick(source, sourceCargoIndex))
                return;
        }

        super.DoubleClick(w, x, y, button);
    }
}
