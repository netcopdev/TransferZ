modded class MissionGameplay
{
    protected float m_TransferZTransientCheckAccumulator;
    protected int m_TransferZSeenMaintenanceSequence;

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);

        if (!GetGame().GetPlayer())
            return;

        // In multiplayer a maintenance RESULT only updates
        // TransferZMaintenanceResultState from 4_World; nothing refreshes the
        // headers. Closable headers are never ticked by vanilla, so without
        // this a Sort failure is not shown until some unrelated RefreshAll.
        int maintenanceSequence = TransferZMaintenanceResultState.GetSequence();
        if (maintenanceSequence != m_TransferZSeenMaintenanceSequence)
        {
            m_TransferZSeenMaintenanceSequence = maintenanceSequence;
            TransferZHeaderControls.RefreshAll();
        }

        m_TransferZTransientCheckAccumulator += timeslice;
        if (m_TransferZTransientCheckAccumulator < 0.25)
            return;

        m_TransferZTransientCheckAccumulator = 0.0;
        if (TransferZClientState.Get().ValidateTransientState())
            TransferZHeaderControls.RefreshAll();
    }
}
