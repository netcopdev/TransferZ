modded class TransferZHeaderControls
{
    protected Widget m_TransferZHoveredTooltipButton;

    override protected int StatusColor(int result)
    {
        if (result == TransferZOperationPreviewResult.READY)
            return ARGB(165, 48, 122, 62);
        if (result == TransferZOperationPreviewResult.PARTIAL)
            return ARGB(170, 154, 118, 34);
        return ARGB(170, 142, 46, 43);
    }

    override protected void PositionTooltip(Widget source, float tooltipH)
    {
        if (!m_TooltipRoot || !source)
            return;

        float sourceX;
        float sourceY;
        float sourceW;
        float sourceH;
        source.GetScreenPos(sourceX, sourceY);
        source.GetScreenSize(sourceW, sourceH);

        int screenW;
        int screenH;
        GetScreenSize(screenW, screenH);

        float tooltipW = 260.0;
        float tooltipX = sourceX + sourceW * 0.5 - tooltipW * 0.5;
        float tooltipY = sourceY - tooltipH - 2.0;

        if (tooltipX < 4.0)
            tooltipX = 4.0;
        if (tooltipX + tooltipW > screenW - 4.0)
            tooltipX = screenW - tooltipW - 4.0;
        if (tooltipY < 4.0)
            tooltipY = sourceY + sourceH + 2.0;

        // Use immediate geometry updates. Without this, changed wrapped text can
        // repaint while the old root height survives until the next hover cycle.
        m_TooltipRoot.SetScreenSize(tooltipW, tooltipH, true);
        if (m_TooltipBackground)
            m_TooltipBackground.SetSize(tooltipW, tooltipH, true);
        if (m_TooltipText)
            m_TooltipText.SetSize(tooltipW - 16.0, tooltipH - 10.0, true);
        m_TooltipRoot.SetScreenPos(tooltipX, tooltipY, true);
        m_TooltipRoot.Update();
    }

    override protected void ShowTooltip(Widget source)
    {
        m_TransferZHoveredTooltipButton = source;

        if (m_TooltipBackground)
            m_TooltipBackground.SetColor(ARGB(232, 0, 0, 0));

        super.ShowTooltip(source);
    }

    override protected void HideTooltip()
    {
        m_TransferZHoveredTooltipButton = null;
        super.HideTooltip();
    }

    override bool OnButtonMouseEnter(Widget w, int x, int y)
    {
        m_TransferZHoveredTooltipButton = w;
        return super.OnButtonMouseEnter(w, x, y);
    }

    override bool OnButtonMouseLeave(Widget w, Widget enter_w, int x, int y)
    {
        m_TransferZHoveredTooltipButton = null;
        return super.OnButtonMouseLeave(w, enter_w, x, y);
    }

    override void UpdateControls()
    {
        super.UpdateControls();

        // State text can change while the pointer remains over the same control
        // (for example L -> L+ -> L*). Rebuild wrapped text and geometry now.
        if (m_TransferZHoveredTooltipButton)
            ShowTooltip(m_TransferZHoveredTooltipButton);
    }
}

modded class TransferZVicinityHeaderControls
{
    protected Widget m_TransferZHoveredTooltipButton;

    override protected int StatusColor(int result)
    {
        if (result == TransferZOperationPreviewResult.READY)
            return ARGB(165, 48, 122, 62);
        if (result == TransferZOperationPreviewResult.PARTIAL)
            return ARGB(170, 154, 118, 34);
        return ARGB(170, 142, 46, 43);
    }

    override protected void PositionTooltip(Widget source, float tooltipH)
    {
        if (!m_TooltipRoot || !source)
            return;

        float sourceX;
        float sourceY;
        float sourceW;
        float sourceH;
        source.GetScreenPos(sourceX, sourceY);
        source.GetScreenSize(sourceW, sourceH);

        int screenW;
        int screenH;
        GetScreenSize(screenW, screenH);

        float tooltipW = 260.0;
        float tooltipX = sourceX + sourceW * 0.5 - tooltipW * 0.5;
        float tooltipY = sourceY - tooltipH - 2.0;

        if (tooltipX < 4.0)
            tooltipX = 4.0;
        if (tooltipX + tooltipW > screenW - 4.0)
            tooltipX = screenW - tooltipW - 4.0;
        if (tooltipY < 4.0)
            tooltipY = sourceY + sourceH + 2.0;

        m_TooltipRoot.SetScreenSize(tooltipW, tooltipH, true);
        if (m_TooltipBackground)
            m_TooltipBackground.SetSize(tooltipW, tooltipH, true);
        if (m_TooltipText)
            m_TooltipText.SetSize(tooltipW - 16.0, tooltipH - 10.0, true);
        m_TooltipRoot.SetScreenPos(tooltipX, tooltipY, true);
        m_TooltipRoot.Update();
    }

    override protected void ShowTooltip(Widget source)
    {
        m_TransferZHoveredTooltipButton = source;

        if (m_TooltipBackground)
            m_TooltipBackground.SetColor(ARGB(232, 0, 0, 0));

        super.ShowTooltip(source);
    }

    override protected void HideTooltip()
    {
        m_TransferZHoveredTooltipButton = null;
        super.HideTooltip();
    }

    override bool OnButtonMouseEnter(Widget w, int x, int y)
    {
        m_TransferZHoveredTooltipButton = w;
        return super.OnButtonMouseEnter(w, x, y);
    }

    override bool OnButtonMouseLeave(Widget w, Widget enter_w, int x, int y)
    {
        m_TransferZHoveredTooltipButton = null;
        return super.OnButtonMouseLeave(w, enter_w, x, y);
    }

    override void UpdateControls()
    {
        super.UpdateControls();

        if (m_TransferZHoveredTooltipButton)
            ShowTooltip(m_TransferZHoveredTooltipButton);
    }
}

modded class VicinitySlotsContainer
{
    protected ItemBase TransferZResolveDoubleClickItem(Widget w)
    {
        if (!w)
            return null;

        ItemPreviewWidget preview = ItemPreviewWidget.Cast(w.FindAnyWidget("Render"));
        if (!preview)
        {
            string renderName = w.GetName();
            renderName.Replace("PanelWidget", "Render");
            preview = ItemPreviewWidget.Cast(w.FindAnyWidget(renderName));
        }
        if (!preview)
            preview = ItemPreviewWidget.Cast(w);
        if (!preview)
            return null;

        return ItemBase.Cast(preview.GetItem());
    }

    protected bool TransferZRouteVicinityClassDoubleClick(ItemBase representative)
    {
        if (!representative || !m_ShowedItems)
            return false;

        TransferZClientState state = TransferZClientState.Get();
        EntityAI destination = state.GetPreferredDestination();
        if (!destination)
            return false;

        string className = representative.GetType();
        if (className == "")
            return false;

        bool requested = false;
        for (int i = 0; i < m_ShowedItems.Count(); i++)
        {
            EntityAI item = m_ShowedItems.Get(i);
            if (!item || item.GetType() != className)
                continue;

            ItemBase itemBase = ItemBase.Cast(item);
            if (!itemBase || !itemBase.IsTakeable() || !item.GetInventory().CanRemoveEntity())
                continue;

            if (state.RequestMoveItem(item, destination))
                requested = true;
        }

        return requested;
    }

    override void DoubleClick(Widget w, int x, int y, int button)
    {
        if (button == MouseState.RIGHT && !g_Game.IsLeftCtrlDown())
        {
            ItemBase item = TransferZResolveDoubleClickItem(w);
            if (item && item.IsTakeable() && item.GetInventory().CanRemoveEntity())
            {
                if (TransferZRouteVicinityClassDoubleClick(item))
                    return;
            }
        }

        super.DoubleClick(w, x, y, button);
    }
}
