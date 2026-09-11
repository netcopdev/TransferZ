modded class TransferZHeaderControls
{
    protected EntityAI m_TransferZPlacementEntity;
    protected bool m_TransferZInitialPlacementQueued;

    protected void TransferZForcePlacementAfterLayout()
    {
        if (!m_Root || !m_HeaderLabel || !m_Entity || !m_Entity.GetInventory().GetCargo())
            return;

        // First inventory creation can run TransferZ placement before DayZ has
        // finished sizing/positioning the native preview and header widgets.
        // Force those widgets through a layout update, then re-run the normal
        // TransferZ UpdateControls chain. The styling layer in that chain owns
        // the actual placement helper, so we do not call across modded-class
        // layers directly.
        RestoreHeaderText();
        if (m_HeaderHost)
            m_HeaderHost.Update();
        m_HeaderLabel.Update();
        m_Root.Update();

        UpdateControls();
    }

    protected void TransferZDeferredPlacementPassOne()
    {
        TransferZForcePlacementAfterLayout();
    }

    protected void TransferZDeferredPlacementPassTwo()
    {
        TransferZForcePlacementAfterLayout();
        m_TransferZInitialPlacementQueued = false;
    }

    protected void TransferZQueueInitialPlacement()
    {
        if (m_TransferZInitialPlacementQueued)
            return;

        m_TransferZInitialPlacementQueued = true;

        // One GUI-queue turn handles the normal first layout pass. The short
        // second pass covers headers whose geometry changes once previews/cargo
        // have populated. This is initialization-only, not a polling loop.
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(TransferZDeferredPlacementPassOne, 0, false);
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(TransferZDeferredPlacementPassTwo, 60, false);
    }

    override void UpdateControls()
    {
        super.UpdateControls();

        if (!m_Root || !m_Entity || !m_Entity.GetInventory().GetCargo())
            return;

        // Queue the deferred geometry correction once for each entity shown by
        // this control object. Deferred UpdateControls() calls see the same
        // entity, so they do not queue themselves again.
        if (m_TransferZPlacementEntity != m_Entity)
        {
            m_TransferZPlacementEntity = m_Entity;
            m_TransferZInitialPlacementQueued = false;
            TransferZQueueInitialPlacement();
        }
    }

    bool TransferZMouseInsideControls()
    {
        if (!m_Root || !m_Root.IsVisibleHierarchy())
            return false;

        int mouseX;
        int mouseY;
        GetMousePos(mouseX, mouseY);

        float x;
        float y;
        float w;
        float h;
        m_Root.GetScreenPos(x, y);
        m_Root.GetScreenSize(w, h);

        return mouseX >= x && mouseX <= x + w && mouseY >= y && mouseY <= y + h;
    }
}

modded class ClosableHeader
{
    override void OnDragHeader(Widget w, int x, int y)
    {
        // ClosableHeader itself is draggable. TransferZ buttons live inside its
        // panel, so a small mouse movement during a D/L/P click can otherwise
        // start the vanilla header drag and hide the panel. Test the actual
        // TransferZ screen rectangle instead of widget ancestry because DayZ can
        // report the draggable header as the widget under the cursor once drag
        // detection starts.
        if (m_TransferZHeaderControls && m_TransferZHeaderControls.TransferZMouseInsideControls())
            return;

        super.OnDragHeader(w, x, y);
    }
}
