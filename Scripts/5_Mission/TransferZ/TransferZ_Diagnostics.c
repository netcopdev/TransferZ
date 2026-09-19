modded class MissionGameplay
{
    protected float m_TransferZTransientCheckAccumulator;

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);

        if (!GetGame().GetPlayer())
            return;

        m_TransferZTransientCheckAccumulator += timeslice;
        if (m_TransferZTransientCheckAccumulator < 0.25)
            return;

        m_TransferZTransientCheckAccumulator = 0.0;
        if (TransferZClientState.Get().ValidateTransientState())
            TransferZHeaderControls.RefreshAll();
    }
}
