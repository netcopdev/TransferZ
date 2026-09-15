modded class TransferZHeaderControls
{
    protected int m_TransferZSeenMaintenanceResult;
    protected int m_TransferZSortFailureUntil;
    protected EntityAI m_TransferZFeedbackEntity;
    protected ImageWidget m_TransferZSortFailureBackground;

    protected ImageWidget TransferZGetSortFailureBackground()
    {
        if (!m_TransferZSortFailureBackground && m_ManageRoot)
            m_TransferZSortFailureBackground = ImageWidget.Cast(m_ManageRoot.FindAnyWidget("TransferZ_SortBase"));
        return m_TransferZSortFailureBackground;
    }

    protected void TransferZResetSortFailureVisual()
    {
        m_TransferZSortFailureUntil = 0;

        ImageWidget background = TransferZGetSortFailureBackground();
        if (background)
        {
            background.SetColor(ARGB(0, 0, 0, 0));
            background.Show(false);
        }
    }

    protected void TransferZShowSortFailureVisual()
    {
        ImageWidget background = TransferZGetSortFailureBackground();
        if (!background)
        {
            Print("[TransferZ] Sort UI failure background widget not found");
            return;
        }

        background.SetColor(ARGB(235, 142, 46, 43));
        background.Show(true);
    }

    protected void TransferZExpireSortFailureVisual()
    {
        if (m_TransferZSortFailureUntil <= 0)
            return;

        int remaining = m_TransferZSortFailureUntil - GetGame().GetTime();
        if (remaining > 0)
        {
            GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(TransferZExpireSortFailureVisual, remaining, false);
            return;
        }

        TransferZResetSortFailureVisual();
    }

    protected void TransferZTriggerSortFailureVisual()
    {
        m_TransferZSortFailureUntil = GetGame().GetTime() + 1000;
        TransferZShowSortFailureVisual();
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(TransferZExpireSortFailureVisual, 1000, false);
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

    override bool OnButtonMouseEnter(Widget w, int x, int y)
    {
        bool handled = super.OnButtonMouseEnter(w, x, y);
        if (w == m_SortButton && m_TransferZSortFailureUntil > GetGame().GetTime())
            TransferZShowSortFailureVisual();
        return handled;
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
        TransferZMaintenanceClient.RequestSort(m_Entity);

        // DayZDiag/single-player resolves synchronously; multiplayer resolves later via RPC.
        // Failure feedback comes only from the published result so it is not double-triggered.
        TransferZConsumeMaintenanceResult();
        ShowTooltip(w);
    }
}
