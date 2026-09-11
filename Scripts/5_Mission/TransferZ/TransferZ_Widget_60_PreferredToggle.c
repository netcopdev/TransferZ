modded class TransferZClientState
{
    bool ClearPreferred()
    {
        if (!m_Preferences)
            m_Preferences = new TransferZPreferences();

        if (!m_Preferences.preferred_path)
            m_Preferences.preferred_path = new array<string>();

        bool changed = m_Preferences.preferred_path.Count() > 0 || m_Preferences.preferred_slot != "";
        m_Preferences.preferred_path.Clear();
        m_Preferences.preferred_slot = "";

        // Clearing P* is itself a persisted preference state. A later restart
        // must not resurrect the previously selected attachment path.
        SavePreferences();
        return changed;
    }

    bool TogglePreferred(EntityAI container)
    {
        if (!container)
            return false;

        if (IsPreferred(container))
            return ClearPreferred();

        return SetPreferred(container);
    }
}

modded class TransferZHeaderControls
{
    override void OnPreferred(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT || !m_Entity)
            return;

        TransferZClientState.Get().TogglePreferred(m_Entity);
        RefreshAll();
        ShowTooltip(w);
    }
}
