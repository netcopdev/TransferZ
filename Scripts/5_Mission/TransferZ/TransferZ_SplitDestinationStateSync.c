// Keep the lower-level native split hook synchronized with the validated
// mission-side transient destination state. ItemBase lives in 4_World and
// cannot be modded from the Mission script module, so synchronization belongs
// on TransferZClientState instead of a Mission-side ItemBase extension.
modded class TransferZClientState
{
    protected void TransferZSyncSplitDestinationBridge()
    {
        if (m_DestinationVicinity)
        {
            TransferZSplitDestinationBridge.SetVicinity();
            return;
        }

        if (m_Destination)
        {
            TransferZSplitDestinationBridge.SetCargo(m_Destination);
            return;
        }

        TransferZSplitDestinationBridge.Clear();
    }

    override EntityAI GetDestination()
    {
        if (m_DestinationVicinity)
            super.IsDestinationVicinity();

        EntityAI destination = super.GetDestination();
        TransferZSyncSplitDestinationBridge();
        return destination;
    }

    override bool IsDestinationVicinity()
    {
        bool active = super.IsDestinationVicinity();
        TransferZSyncSplitDestinationBridge();
        return active;
    }

    override void SetDestination(EntityAI destination)
    {
        super.SetDestination(destination);
        TransferZSyncSplitDestinationBridge();
    }

    override void SetVicinityDestination()
    {
        super.SetVicinityDestination();
        TransferZSyncSplitDestinationBridge();
    }

    override void ToggleDestinationSelection(EntityAI destination)
    {
        super.ToggleDestinationSelection(destination);
        TransferZSyncSplitDestinationBridge();
    }

    override void ToggleVicinityDestinationSelection()
    {
        super.ToggleVicinityDestinationSelection();
        TransferZSyncSplitDestinationBridge();
    }

    override bool OnContainerLeftHands(EntityAI container)
    {
        bool changed = super.OnContainerLeftHands(container);
        TransferZSyncSplitDestinationBridge();
        return changed;
    }

    override bool ValidateTransientState()
    {
        bool changed = super.ValidateTransientState();
        TransferZSyncSplitDestinationBridge();
        return changed;
    }
}
