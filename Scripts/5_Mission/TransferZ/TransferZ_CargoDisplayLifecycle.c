// Some containers change cargo visibility without changing the bound inventory
// entity. Barrels are the common case: the inventory row can exist while closed,
// then DayZ unlocks/reflows its cargo when the lid opens. Re-run TransferZ's
// bounded header-layout correction after that transition so right-side controls
// use the final native header geometry on the first open.
modded class ContainerWithCargo
{
    protected EntityAI m_TransferZCargoDisplayEntity;
    protected bool m_TransferZCargoDisplayStateKnown;
    protected bool m_TransferZCargoDisplayable;

    protected void TransferZTrackCargoDisplayTransition()
    {
        if (m_TransferZCargoDisplayEntity != m_Entity)
        {
            m_TransferZCargoDisplayEntity = m_Entity;
            m_TransferZCargoDisplayStateKnown = false;
            m_TransferZCargoDisplayable = false;
        }

        if (!m_Entity)
            return;

        bool displayable = m_Entity.CanDisplayCargo();
        if (!m_TransferZCargoDisplayStateKnown)
        {
            m_TransferZCargoDisplayStateKnown = true;
            m_TransferZCargoDisplayable = displayable;
            return;
        }

        if (displayable == m_TransferZCargoDisplayable)
            return;

        m_TransferZCargoDisplayable = displayable;
        if (!displayable)
            return;

        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(TransferZHeaderControls.RefreshAll, 0, false);
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(TransferZHeaderControls.RefreshAll, 60, false);
    }

    override void UpdateInterval()
    {
        super.UpdateInterval();
        TransferZTrackCargoDisplayTransition();
    }
}
