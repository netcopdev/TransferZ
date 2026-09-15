modded class TransferZHeaderControls
{
    protected int m_TransferZSeenMaintenanceResult;
    protected int m_TransferZSortFailureUntil;
    protected EntityAI m_TransferZFeedbackEntity;

    protected void TransferZResetSortFailureVisual()
    {
        m_TransferZSortFailureUntil = 0;
        if (!m_SortHover)
            return;

        m_SortHover.SetColor(ARGB(55, 255, 255, 255));
        if (m_HoveredTooltipButton != m_SortButton)
            m_SortHover.Show(false);
    }

    protected void TransferZShowSortFailureVisual()
    {
        if (!m_SortHover)
            return;

        m_SortHover.SetColor(ARGB(220, 142, 46, 43));
        m_SortHover.Show(true);
    }

    protected void TransferZTriggerSortFailureVisual()
    {
        m_TransferZSortFailureUntil = GetGame().GetTime() + 1000;
        TransferZShowSortFailureVisual();
    }

    protected void TransferZConsumeMaintenanceResult()
    {
        int sequence = TransferZMaintenanceResultState.GetSequence();
        if (sequence <= 0 || sequence == m_TransferZSeenMaintenanceResult)
            return;

        m_TransferZSeenMaintenanceResult = sequence;
        if (TransferZMaintenanceResultState.GetOperation() != TransferZMaintenanceOperation.SORT)
            return;
        if (!TransferZMaintenanceResultState.MatchesSource(m_Entity))
            return;

        if (TransferZMaintenanceResultState.WasSuccessful())
        {
            Print("[TransferZ] Sort UI result: success");
            return;
        }

        Print("[TransferZ] Sort UI result: failure");
        TransferZTriggerSortFailureVisual();
    }

    override void SetEntity(EntityAI entity)
    {
        if (entity != m_TransferZFeedbackEntity)
        {
            m_TransferZFeedbackEntity = entity;
            m_TransferZSeenMaintenanceResult = TransferZMaintenanceResultState.GetSequence();
            TransferZResetSortFailureVisual();
        }

        super.SetEntity(entity);
    }

    override void UpdateControls()
    {
        super.UpdateControls();
        TransferZConsumeMaintenanceResult();

        if (m_TransferZSortFailureUntil <= 0)
            return;

        if (GetGame().GetTime() < m_TransferZSortFailureUntil)
        {
            TransferZShowSortFailureVisual();
            return;
        }

        TransferZResetSortFailureVisual();
    }

    override bool OnButtonMouseLeave(Widget w, Widget enter_w, int x, int y)
    {
        bool handled = super.OnButtonMouseLeave(w, enter_w, x, y);
        if (w == m_SortButton && m_TransferZSortFailureUntil > GetGame().GetTime())
            TransferZShowSortFailureVisual();
        return handled;
    }

    override void OnSort(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT || !m_Entity)
            return;

        Print("[TransferZ] Sort UI click source=" + m_Entity.GetType());
        bool accepted = TransferZMaintenanceClient.RequestSort(m_Entity);

        // Single-player/DayZDiag resolves synchronously, so consume the result immediately.
        // Multiplayer resolves later through the maintenance result RPC and UpdateControls().
        TransferZConsumeMaintenanceResult();
        if (!accepted)
        {
            Print("[TransferZ] Sort UI request rejected locally");
            TransferZTriggerSortFailureVisual();
        }

        ShowTooltip(w);
    }
}
