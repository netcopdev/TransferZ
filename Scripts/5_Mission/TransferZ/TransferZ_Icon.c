modded class Icon
{
    protected static const int TRANSFERZ_DRAG_NONE = 0;
    protected static const int TRANSFERZ_DRAG_TRANSFER = 1;
    protected static const int TRANSFERZ_DRAG_UNPACK = 2;

    protected int m_TransferZModifierDragMode = TRANSFERZ_DRAG_NONE;
    protected bool m_TransferZModifierDragStarted;
    protected EntityAI m_TransferZModifierDragSource;

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

        // Contents of a container currently held in hands keep TransferZ's
        // external/in-hand routing semantics rather than vanilla worn-inventory
        // double-click-to-hands behavior.
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

        // An explicit link always wins over the preferred personal target.
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

        // Normal double-left-click from worn/player inventory must remain the
        // familiar DayZ action: take/swap the item to hands. Returning false
        // here lets vanilla Icon.DoubleClick perform exactly that action.
        if (TransferZIsPlayerInventorySource(source))
            return false;

        EntityAI destination = state.GetPreferredDestination();
        if (!destination || destination == source)
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

        // Exact GetType() matching is performed and revalidated by the server.
        return TransferZClientState.Get().RequestClassTransferTo(source, destination, m_Obj);
    }

    protected int TransferZReadModifierDragMode()
    {
        bool shiftDown = KeyState(KeyCode.KC_LSHIFT) || KeyState(KeyCode.KC_RSHIFT);
        bool altDown = KeyState(KeyCode.KC_LMENU) || KeyState(KeyCode.KC_RMENU);

        // Ctrl+click/drag is intentionally not owned by TransferZ. DayZ uses
        // Ctrl+click as an immediate drop-to-ground gesture, so TransferZ must
        // not compete with that stock interaction.
        if (KeyState(KeyCode.KC_LCONTROL) || KeyState(KeyCode.KC_RCONTROL))
            return TRANSFERZ_DRAG_NONE;

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

    override void MouseClick(Widget w, int x, int y, int button)
    {
        if (button == MouseState.LEFT)
        {
            TransferZResetModifierDrag();
            m_TransferZModifierDragSource = TransferZGetDirectCargoSource();
            if (m_TransferZModifierDragSource && m_Obj)
                m_TransferZModifierDragMode = TransferZReadModifierDragMode();
        }

        super.MouseClick(w, x, y, button);
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

        TransferZHeaderControls.SetOperationDropTargetsVisible(true);
        m_TransferZModifierDragStarted = true;
    }

    override void DestroyWhiteBackground()
    {
        super.DestroyWhiteBackground();

        if (m_TransferZModifierDragStarted)
            TransferZHeaderControls.CancelOperationDragLater();

        TransferZResetModifierDrag();
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
