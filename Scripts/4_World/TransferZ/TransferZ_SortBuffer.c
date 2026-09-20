class TransferZ_SortBuffer : Container_Base
{
    protected bool m_RecoveryMode;

    void TransferZ_SortBuffer()
    {
        RegisterNetSyncVariableBool("m_RecoveryMode");
    }

    protected void ApplyTransferZBufferMode()
    {
        SetAllowDamage(false);

        if (m_RecoveryMode)
        {
            DisableSimulation(false);
            SetInvisible(false);
            OnInvisibleSet(false);
            return;
        }

        DisableSimulation(true);
        SetInvisible(true);
        OnInvisibleSet(true);
    }

    override void EEInit()
    {
        super.EEInit();
        ApplyTransferZBufferMode();
    }

    override void OnVariablesSynchronized()
    {
        super.OnVariablesSynchronized();
        ApplyTransferZBufferMode();
    }

    override void AfterStoreLoad()
    {
        super.AfterStoreLoad();

        if (!GetGame().IsServer())
            return;

        CargoBase cargo = GetInventory().GetCargo();
        if (cargo && cargo.GetItemCount() > 0)
        {
            EnableRecoveryMode();
            Print("[TransferZ] Recovered persisted Sort buffer with stranded items count=" + cargo.GetItemCount().ToString());
            return;
        }

        // A persisted empty buffer has no recovery value.
        Delete();
    }

    void EnableRecoveryMode()
    {
        if (m_RecoveryMode)
            return;

        m_RecoveryMode = true;
        ApplyTransferZBufferMode();
        SetSynchDirty();
    }

    bool IsRecoveryMode()
    {
        return m_RecoveryMode;
    }

    override bool IsInventoryVisible()
    {
        return m_RecoveryMode;
    }

    override bool CanDisplayCargo()
    {
        return m_RecoveryMode;
    }

    override bool CanReceiveItemIntoCargo(EntityAI item)
    {
        if (m_RecoveryMode)
            return false;
        return super.CanReceiveItemIntoCargo(item);
    }

    override bool IsTakeable()
    {
        return false;
    }

    override bool CanPutIntoHands(EntityAI parent)
    {
        return false;
    }

    override bool CanPutInCargo(EntityAI parent)
    {
        return false;
    }

    override bool IsActionTargetVisible()
    {
        return m_RecoveryMode;
    }

    override bool CanBeActionTarget()
    {
        return m_RecoveryMode;
    }
}
