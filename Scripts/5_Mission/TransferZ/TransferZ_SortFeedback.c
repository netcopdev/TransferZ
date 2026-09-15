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

        m_SortHover.SetColor(ARGB(190, 142, 46, 43));
        m_SortHover.Show(true);
    }

    protected void TransferZConsumeMaintenanceResult()
    {
        int sequence = TransferZMaintenanceResultState.GetSequence();
        if (sequence <= 0 || sequence == m_TransferZSeenMaintenanceResult)
            return;

        m_TransferZSeenMaintenanceResult = sequence;
        if (TransferZMaintenanceResultState.GetOperation() != TransferZMaintenanceOperation.SORT)
            return;
        if (TransferZMaintenanceResultState.WasSuccessful())
            return;
        if (!TransferZMaintenanceResultState.MatchesSource(m_Entity))
            return;

        m_TransferZSortFailureUntil = GetGame().GetTime() + 1000;
        TransferZShowSortFailureVisual();
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
}
