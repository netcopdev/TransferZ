modded class SlotsIcon
{
    protected bool m_TransferZVicinityModifierDragStarted;

    protected VicinitySlotsContainer TransferZFindVicinitySource()
    {
        LayoutHolder current = m_Parent;
        while (current)
        {
            VicinitySlotsContainer vicinity = VicinitySlotsContainer.Cast(current);
            if (vicinity)
                return vicinity;

            current = current.GetParent();
        }

        return null;
    }

    protected int TransferZReadVicinityModifierOperation()
    {
        // Ctrl stays entirely vanilla: DayZ owns Ctrl+click as drop-to-ground.
        if (KeyState(KeyCode.KC_LCONTROL) || KeyState(KeyCode.KC_RCONTROL))
            return 0;

        bool shiftDown = KeyState(KeyCode.KC_LSHIFT) || KeyState(KeyCode.KC_RSHIFT);
        bool altDown = KeyState(KeyCode.KC_LMENU) || KeyState(KeyCode.KC_RMENU);

        if (shiftDown == altDown)
            return 0;

        if (shiftDown)
            return TransferZOperation.TRANSFER;
        return TransferZOperation.UNPACK;
    }

    override void OnIconDrag(Widget w)
    {
        super.OnIconDrag(w);

        m_TransferZVicinityModifierDragStarted = false;

        VicinitySlotsContainer vicinity = TransferZFindVicinitySource();
        if (!vicinity || !m_Obj)
            return;

        int operation = TransferZReadVicinityModifierOperation();
        if (operation == 0)
            return;

        ref array<EntityAI> items = new array<EntityAI>();
        vicinity.TransferZSnapshotVisibleItems(items);
        if (items.Count() == 0)
            return;

        // Modifier drags from a vicinity item operate on the vicinity zone:
        // Shift = vicinity T, Alt = vicinity U.
        TransferZOperationDrag.BeginVicinity(operation, items);
        TransferZHeaderControls.SetOperationDropTargetsVisible(true);
        m_TransferZVicinityModifierDragStarted = true;
    }

    override void OnIconDrop(Widget w)
    {
        super.OnIconDrop(w);

        if (m_TransferZVicinityModifierDragStarted)
            TransferZHeaderControls.CancelOperationDragLater();

        m_TransferZVicinityModifierDragStarted = false;
    }
}
