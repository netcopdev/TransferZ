modded class TransferZClientState
{
    void ToggleDestinationSelection(EntityAI destination)
    {
        if (!destination)
            return;

        if (IsDestination(destination))
        {
            ClearDestination();
            return;
        }

        SetDestination(destination);
    }

    void ToggleVicinityDestinationSelection()
    {
        if (IsDestinationVicinity())
        {
            ClearDestination();
            return;
        }

        SetVicinityDestination();
    }
}

modded class TransferZHeaderControls
{
    override void OnDestination(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT || !m_Entity)
            return;

        TransferZClientState.Get().ToggleDestinationSelection(m_Entity);
        RefreshAll();
        ShowTooltip(w);
    }
}

modded class TransferZVicinityHeaderControls
{
    override void OnDestination(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT)
            return;

        TransferZClientState.Get().ToggleVicinityDestinationSelection();
        TransferZHeaderControls.RefreshAll();
        ShowTooltip(w);
    }
}
