class TransferZOperationDrag
{
    protected static int s_Operation = 0;
    protected static EntityAI s_Source;
    protected static int s_SourceCargoIndex = 0;
    protected static EntityAI s_RepresentativeItem;
    protected static bool s_ClassTransfer = false;
    protected static bool s_FromVicinity = false;
    protected static bool s_ModifierItemDrag = false;
    protected static bool s_ModifierReleaseWatchQueued = false;
    protected static bool s_ModifierReleaseCancelQueued = false;
    protected static int s_TransferZSuppressNativeDropUntil;
    protected static ref array<EntityAI> s_VicinityItems;

    protected static bool LeftMousePressed()
    {
        return (GetMouseState(MouseState.LEFT) & MB_PRESSED_MASK) != 0;
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

    protected static void StartModifierReleaseWatch()
    {
        if (!s_ModifierItemDrag || s_ModifierReleaseWatchQueued)
            return;

        s_ModifierReleaseWatchQueued = true;
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(WatchModifierRelease, 25, false);
    }

    protected static void WatchModifierRelease()
    {
        if (!IsActive() || !s_ModifierItemDrag)
        {
            s_ModifierReleaseWatchQueued = false;
            s_ModifierReleaseCancelQueued = false;
            return;
        }

        if (LeftMousePressed())
        {
            GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(WatchModifierRelease, 25, false);
            return;
        }

        s_ModifierReleaseWatchQueued = false;
        if (!s_ModifierReleaseCancelQueued)
        {
            s_ModifierReleaseCancelQueued = true;
            // Give the widget mouse-up event one GUI turn to complete a valid
            // drop. If no target handled the release, cancel the batch session.
            GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(CancelReleasedModifierDrag, 50, false);
        }
    }

    protected static void CancelReleasedModifierDrag()
    {
        s_ModifierReleaseCancelQueued = false;
        if (!IsActive() || !s_ModifierItemDrag || LeftMousePressed())
            return;

        ForceClear();
        TransferZHeaderControls.SetOperationDropTargetsVisible(false);
        TransferZHeaderControls.RefreshAll();
    }

    protected static void RestoreModifierDropTargets()
    {
        if (!IsActive() || !s_ModifierItemDrag || !LeftMousePressed())
            return;

        TransferZHeaderControls.SetOperationDropTargetsVisible(true);
    }

    protected static void SetModifierItemDrag(bool modifierItemDrag)
    {
        s_ModifierItemDrag = modifierItemDrag;
        if (s_ModifierItemDrag)
            StartModifierReleaseWatch();
    }

    static void LatchModifierItemDrag()
    {
        if (!IsActive())
            return;

        SetModifierItemDrag(true);
    }

    static void BeginContainer(int operation, EntityAI source, int sourceCargoIndex = 0)
    {
        if (!source || !TransferZCargo.Exists(source, sourceCargoIndex))
            return;

        s_Operation = operation;
        s_Source = source;
        s_SourceCargoIndex = sourceCargoIndex;
        s_RepresentativeItem = null;
        s_ClassTransfer = false;
        s_FromVicinity = false;

        if (s_VicinityItems)
            s_VicinityItems.Clear();

        SetModifierItemDrag(false);
    }

    static void BeginClassTransfer(EntityAI source, EntityAI representative, int sourceCargoIndex = 0)
    {
        if (!source || !representative || !TransferZCargo.Exists(source, sourceCargoIndex))
            return;

        s_Operation = TransferZOperation.TRANSFER_CLASS;
        s_Source = source;
        s_SourceCargoIndex = sourceCargoIndex;
        s_RepresentativeItem = representative;
        s_ClassTransfer = true;
        s_FromVicinity = false;

        if (s_VicinityItems)
            s_VicinityItems.Clear();

        SetModifierItemDrag(true);
    }

    static void BeginVicinity(int operation, notnull array<EntityAI> items)
    {
        s_Operation = operation;
        s_Source = null;
        s_SourceCargoIndex = 0;
        s_RepresentativeItem = null;
        s_ClassTransfer = false;
        s_FromVicinity = true;

        if (!s_VicinityItems)
            s_VicinityItems = new array<EntityAI>();
        else
            s_VicinityItems.Clear();

        foreach (EntityAI item : items)
        {
            if (item)
                s_VicinityItems.Insert(item);
        }

        SetModifierItemDrag(false);
    }

    static bool IsActive()
    {
        return s_Operation == TransferZOperation.TRANSFER || s_Operation == TransferZOperation.UNPACK || s_Operation == TransferZOperation.TRANSFER_CLASS;
    }

    static bool IsModifierItemDrag()
    {
        return s_ModifierItemDrag && IsActive();
    }

    static bool IsClassTransfer()
    {
        return s_ClassTransfer && s_Operation == TransferZOperation.TRANSFER_CLASS;
    }

    static EntityAI GetSource()
    {
        return s_Source;
    }

    static int GetSourceCargoIndex()
    {
        return s_SourceCargoIndex;
    }

    static EntityAI GetRepresentativeItem()
    {
        return s_RepresentativeItem;
    }

    static bool IsFromVicinity()
    {
        return s_FromVicinity;
    }

    static int GetOperation()
    {
        return s_Operation;
    }

    static bool Complete(EntityAI destination, int destinationCargoIndex = 0)
    {
        if (!IsActive() || !destination || !TransferZCargo.Exists(destination, destinationCargoIndex))
            return false;

        bool handled = false;
        TransferZClientState state = TransferZClientState.Get();

        if (s_ClassTransfer)
        {
            if (s_Source && s_RepresentativeItem && (s_Source != destination || s_SourceCargoIndex != destinationCargoIndex))
                handled = state.RequestClassTransferTo(s_Source, destination, s_RepresentativeItem, s_SourceCargoIndex, destinationCargoIndex);
        }
        else if (s_FromVicinity)
        {
            if (s_VicinityItems)
            {
                if (s_Operation == TransferZOperation.TRANSFER)
                    handled = state.RequestVicinityTransferTo(s_VicinityItems, destination, destinationCargoIndex);
                else if (s_Operation == TransferZOperation.UNPACK)
                    handled = state.RequestVicinityUnpackTo(s_VicinityItems, destination, destinationCargoIndex);
            }
        }
        else if (s_Source)
        {
            if (s_Operation == TransferZOperation.TRANSFER)
            {
                if (s_Source != destination || s_SourceCargoIndex != destinationCargoIndex)
                    handled = state.RequestTransferTo(s_Source, destination, s_SourceCargoIndex, destinationCargoIndex);
            }
            else if (s_Operation == TransferZOperation.UNPACK)
            {
                handled = state.RequestNestedUnpackTo(s_Source, destination, s_SourceCargoIndex, destinationCargoIndex);
            }
        }

        ForceClear();
        return handled;
    }

    static bool CompleteToVicinity()
    {
        if (!IsActive())
            return false;

        bool handled = false;
        TransferZClientState state = TransferZClientState.Get();

        if (s_ClassTransfer)
        {
            if (s_Source && s_RepresentativeItem)
                handled = state.RequestClassTransferToVicinity(s_Source, s_RepresentativeItem, s_SourceCargoIndex);
        }
        else if (s_FromVicinity)
        {
            if (s_VicinityItems && s_Operation == TransferZOperation.UNPACK)
                handled = state.RequestVicinityUnpackToVicinity(s_VicinityItems);
        }
        else if (s_Source)
        {
            if (s_Operation == TransferZOperation.TRANSFER)
                handled = state.RequestTransferToVicinity(s_Source, s_SourceCargoIndex);
            else if (s_Operation == TransferZOperation.UNPACK)
                handled = state.RequestNestedUnpackToVicinity(s_Source, s_SourceCargoIndex);
        }

        ForceClear();
        return handled;
    }

    static void Clear()
    {
        // Vanilla Icon.DestroyWhiteBackground() can run while an item drag is
        // still physically held when its scroller moves. The old delayed cancel
        // path reaches here. Do not let that visual teardown cancel a modifier
        // batch; restore the overlays after the caller hides them instead.
        if (IsModifierItemDrag() && LeftMousePressed())
        {
            GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(RestoreModifierDropTargets, 0, false);
            GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(RestoreModifierDropTargets, 30, false);
            return;
        }

        ForceClear();
    }

    // UI teardown must not preserve a modifier drag merely because LMB is
    // still physically held. The owning inventory widgets are disappearing.
    static void CancelForUiTeardown()
    {
        ForceClear();
    }

    protected static void ForceClear()
    {
        s_Operation = 0;
        s_Source = null;
        s_SourceCargoIndex = 0;
        s_RepresentativeItem = null;
        s_ClassTransfer = false;
        s_FromVicinity = false;
        s_ModifierItemDrag = false;
        s_ModifierReleaseWatchQueued = false;
        s_ModifierReleaseCancelQueued = false;
        if (s_VicinityItems)
            s_VicinityItems.Clear();
    }
}

modded class WidgetEventHandler
{
    protected void TransferZRefreshDropTargetsAfterScroll()
    {
        if (TransferZOperationDrag.IsActive())
            TransferZHeaderControls.SetOperationDropTargetsVisible(true);
    }

    override bool OnMouseWheel(Widget w, int x, int y, int wheel)
    {
        if (w && TransferZOperationDrag.IsActive() && w.GetName() == "TransferZHeaderDropTarget")
        {

            // TransferZ's overlay handlers negate the wheel value before
            // calling VScrollStep. Feed the opposite value here so the resulting
            // scroll direction matches vanilla DayZ.
            bool handled = super.OnMouseWheel(w, x, y, -wheel);

            TransferZRefreshDropTargetsAfterScroll();
            GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(TransferZRefreshDropTargetsAfterScroll, 0, false);
            GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(TransferZRefreshDropTargetsAfterScroll, 30, false);
            return handled;
        }

        return super.OnMouseWheel(w, x, y, wheel);
    }

    override bool OnDropReceived(Widget w, int x, int y, Widget reciever)
    {
        bool activeModifierDrag = TransferZOperationDrag.IsModifierItemDrag();
        bool trailingNativeDrop = TransferZOperationDrag.ShouldSuppressNativeDrop();
        if (activeModifierDrag || trailingNativeDrop)
        {

            if (activeModifierDrag)
                TransferZRefreshDropTargetsAfterScroll();
            return true;
        }

        return super.OnDropReceived(w, x, y, reciever);
    }

    override bool OnMouseButtonUp(Widget w, int x, int y, int button)
    {
        if (button == MouseState.LEFT && TransferZOperationDrag.IsModifierItemDrag())
        {
            // TransferZ owns modifier-item release. End the native widget drag
            // before moving the batch so DayZ cannot enqueue a stale
            // PredictiveTakeToDst for the representative item.
            TransferZOperationDrag.ArmNativeDropSuppression();

            Widget nativeDrag = GetDragWidget();
            ItemManager itemManager = ItemManager.GetInstance();
            if (itemManager)
            {
                Icon draggedIcon = itemManager.GetDraggedIcon();
                if (draggedIcon)
                {
                    draggedIcon.DestroyWhiteBackground();
                }
                else if (nativeDrag)
                {
                    SlotsIcon draggedSlotsIcon;
                    nativeDrag.GetUserData(draggedSlotsIcon);
                    if (draggedSlotsIcon)
                        draggedSlotsIcon.OnIconDrop(nativeDrag);
                }
            }

            if (nativeDrag)
                CancelWidgetDragging();

            if (itemManager)
            {
                itemManager.HideDropzones();
                itemManager.SetIsDragging(false);
            }

            TransferZHeaderControls.CompleteModifierDragAtMousePosition();
            return true;
        }

        if (button == MouseState.LEFT && TransferZOperationDrag.IsActive())
        {
            // Header T/U drags use a native draggable button only as the mouse
            // transport. Always terminate that native widget drag on release,
            // then let TransferZ resolve the target from live screen geometry.
            // Without this, an invalid drop can leave native mouse capture
            // active and freeze all TransferZ hover/click feedback until the
            // inventory UI is rebuilt.
            Widget operationDrag = GetDragWidget();
            if (operationDrag)
                CancelWidgetDragging();

            TransferZHeaderControls.CompleteModifierDragAtMousePosition();
            if (TransferZOperationDrag.IsActive())
            {
                TransferZHeaderControls.CancelOperationDrag();
                TransferZHeaderControls.RefreshAll();
            }
            return true;
        }

        return super.OnMouseButtonUp(w, x, y, button);
    }
}

