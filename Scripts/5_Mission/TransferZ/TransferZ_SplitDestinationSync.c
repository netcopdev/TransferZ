// Transfer the currently validated mission-side destination into the lower-level
// native split hook only for the duration of this right-click call. This avoids
// keeping a second persistent copy of transient D state in 4_World.
modded class ItemBase
{
    override void OnRightClick()
    {
        TransferZSplitDestinationBridge.Clear();

        TransferZClientState state = TransferZClientState.Get();
        if (state)
        {
            if (state.IsDestinationVicinity())
            {
                TransferZSplitDestinationBridge.SetVicinity();
            }
            else
            {
                EntityAI destination = state.GetDestination();
                if (destination)
                    TransferZSplitDestinationBridge.SetCargo(destination);
            }
        }

        super.OnRightClick();
        TransferZSplitDestinationBridge.Clear();
    }
}
