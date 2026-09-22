modded class MissionGameplay
{
    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);

        if (TransferZOperationDrag.IsActive())
            return;

        int command = TransferZInput.PressedCommand();
        if (command != TransferZInputCommand.NONE)
            TransferZHeaderControls.ExecuteInputCommandAtMousePosition(command);
    }
}
