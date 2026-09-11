modded class TransferZHeaderControls
{
    protected EntityAI m_TransferZPlacementEntity;
    protected bool m_TransferZInitialPlacementQueued;

    protected void TransferZForcePlacementAfterLayout()
    {
        if (!m_Root || !m_HeaderLabel || !m_Entity || !m_Entity.GetInventory().GetCargo())
            return;

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

        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(TransferZDeferredPlacementPassOne, 0, false);
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(TransferZDeferredPlacementPassTwo, 60, false);
    }

    override void UpdateControls()
    {
        super.UpdateControls();

        if (!m_Root || !m_Entity || !m_Entity.GetInventory().GetCargo())
            return;

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
        if (m_TransferZHeaderControls && m_TransferZHeaderControls.TransferZMouseInsideControls())
            return;

        super.OnDragHeader(w, x, y);
    }
}
