class TransferZ_SortBuffer : Container_Base
{
    override void EEInit()
    {
        super.EEInit();
        SetAllowDamage(false);
        DisableSimulation(true);
        SetInvisible(true);
        OnInvisibleSet(true);
    }

    override bool IsInventoryVisible()
    {
        return false;
    }

    override bool CanDisplayCargo()
    {
        return false;
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
        return false;
    }

    override bool CanBeActionTarget()
    {
        return false;
    }
}
