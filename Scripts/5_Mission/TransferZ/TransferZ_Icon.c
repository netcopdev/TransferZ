modded class Icon
{
    override void DoubleClick(Widget w, int x, int y, int button)
    {
        if (button == MouseState.LEFT && !g_Game.IsLeftCtrlDown() && !m_HandsIcon && m_Obj)
        {
            CargoContainer sourceContainer = CargoContainer.Cast(m_Parent);
            if (sourceContainer)
            {
                EntityAI source = sourceContainer.GetEntity();
                if (TransferZClientState.Get().TryRouteCargoDoubleClick(source, m_Obj))
                    return;
            }
        }

        super.DoubleClick(w, x, y, button);
    }
}
