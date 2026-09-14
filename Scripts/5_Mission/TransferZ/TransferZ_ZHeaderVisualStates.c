modded class TransferZHeaderControls
{
    protected int TransferZVisualGreen()
    {
        return ARGB(195, 38, 164, 62);
    }

    protected int TransferZVisualAmber()
    {
        return ARGB(200, 205, 137, 24);
    }

    protected int TransferZVisualRed()
    {
        return ARGB(200, 190, 48, 45);
    }

    protected int TransferZVisualPreviewColor(int result)
    {
        if (result == TransferZOperationPreviewResult.READY)
            return TransferZVisualGreen();
        if (result == TransferZOperationPreviewResult.PARTIAL)
            return TransferZVisualAmber();
        return TransferZVisualRed();
    }

    protected void TransferZRefreshOperationHover(Widget w)
    {
        if (!m_Entity)
            return;

        ImageWidget status;
        int operation;
        if (w == m_TransferButton)
        {
            status = m_TransferStatus;
            operation = TransferZOperation.TRANSFER;
        }
        else if (w == m_UnpackButton)
        {
            status = m_UnpackStatus;
            operation = TransferZOperation.UNPACK;
        }
        else
            return;

        ImageWidget hover = HoverImageFor(w);
        if (hover)
            hover.Show(false);

        if (!status)
            return;

        int result = TransferZOperationPreview.EvaluateContainerOperation(operation, m_Entity);
        status.SetColor(TransferZVisualPreviewColor(result));
        status.Show(true);
    }

    protected void TransferZRefreshActionHover(Widget w)
    {
        ImageWidget hover = HoverImageFor(w);
        if (!hover)
            return;

        if (w == m_TransferButton || w == m_UnpackButton)
        {
            hover.Show(false);
            TransferZRefreshOperationHover(w);
            return;
        }

        int color = TransferZVisualGreen();
        TransferZClientState state = TransferZClientState.Get();

        if (w == m_DestinationButton && state.IsDestination(m_Entity))
            color = TransferZVisualRed();
        else if (w == m_LinkButton)
        {
            if (state.IsLinked(m_Entity) || state.IsLinkAnchor(m_Entity))
                color = TransferZVisualRed();
            else
                color = TransferZVisualAmber();
        }
        else if (w == m_PreferredButton && state.IsPreferred(m_Entity))
            color = TransferZVisualRed();

        hover.SetColor(color);
        hover.Show(true);
    }

    override bool OnButtonMouseEnter(Widget w, int x, int y)
    {
        bool handled = super.OnButtonMouseEnter(w, x, y);
        TransferZRefreshActionHover(w);
        return handled;
    }

    override void UpdateControls()
    {
        super.UpdateControls();

        if (!m_Entity)
            return;

        TransferZClientState state = TransferZClientState.Get();

        if (m_DestinationState)
            m_DestinationState.SetColor(TransferZVisualGreen());

        if (m_LinkState)
        {
            if (state.IsLinkAnchor(m_Entity))
                m_LinkState.SetColor(TransferZVisualAmber());
            else
                m_LinkState.SetColor(TransferZVisualGreen());
        }

        if (m_PreferredState)
            m_PreferredState.SetColor(TransferZVisualGreen());

        if (m_HoveredTooltipButton)
            TransferZRefreshActionHover(m_HoveredTooltipButton);
    }

    override void OnTransfer(Widget w, int x, int y, int button)
    {
        super.OnTransfer(w, x, y, button);
        if (m_HoveredTooltipButton == w)
            TransferZRefreshOperationHover(w);
    }

    override void OnUnpack(Widget w, int x, int y, int button)
    {
        super.OnUnpack(w, x, y, button);
        if (m_HoveredTooltipButton == w)
            TransferZRefreshOperationHover(w);
    }
}

modded class TransferZVicinityHeaderControls
{
    protected int TransferZVisualGreen()
    {
        return ARGB(195, 38, 164, 62);
    }

    protected int TransferZVisualAmber()
    {
        return ARGB(200, 205, 137, 24);
    }

    protected int TransferZVisualRed()
    {
        return ARGB(200, 190, 48, 45);
    }

    protected int TransferZVisualPreviewColor(int result)
    {
        if (result == TransferZOperationPreviewResult.READY)
            return TransferZVisualGreen();
        if (result == TransferZOperationPreviewResult.PARTIAL)
            return TransferZVisualAmber();
        return TransferZVisualRed();
    }

    protected void TransferZRefreshOperationHover(Widget w)
    {
        if (!m_Source)
            return;

        ImageWidget status;
        int operation;
        if (w == m_TransferButton)
        {
            status = m_TransferStatus;
            operation = TransferZOperation.TRANSFER;
        }
        else if (w == m_UnpackButton)
        {
            status = m_UnpackStatus;
            operation = TransferZOperation.UNPACK;
        }
        else
            return;

        ImageWidget hover = HoverImageFor(w);
        if (hover)
            hover.Show(false);

        if (!status)
            return;

        ref array<EntityAI> items = new array<EntityAI>();
        Snapshot(items);
        int result = TransferZOperationPreview.EvaluateVicinityOperation(operation, items);
        status.SetColor(TransferZVisualPreviewColor(result));
        status.Show(true);
    }

    protected void TransferZRefreshActionHover(Widget w)
    {
        ImageWidget hover = HoverImageFor(w);
        if (!hover)
            return;

        if (w == m_TransferButton || w == m_UnpackButton)
        {
            hover.Show(false);
            TransferZRefreshOperationHover(w);
            return;
        }

        int color = TransferZVisualGreen();
        if (w == m_DestinationButton && TransferZClientState.Get().IsDestinationVicinity())
            color = TransferZVisualRed();

        hover.SetColor(color);
        hover.Show(true);
    }

    override bool OnButtonMouseEnter(Widget w, int x, int y)
    {
        bool handled = super.OnButtonMouseEnter(w, x, y);
        TransferZRefreshActionHover(w);
        return handled;
    }

    override void UpdateControls()
    {
        super.UpdateControls();

        if (m_DestinationState)
            m_DestinationState.SetColor(TransferZVisualGreen());

        if (m_HoveredTooltipButton)
            TransferZRefreshActionHover(m_HoveredTooltipButton);
    }

    override void OnTransfer(Widget w, int x, int y, int button)
    {
        super.OnTransfer(w, x, y, button);
        if (m_HoveredTooltipButton == w)
            TransferZRefreshOperationHover(w);
    }

    override void OnUnpack(Widget w, int x, int y, int button)
    {
        super.OnUnpack(w, x, y, button);
        if (m_HoveredTooltipButton == w)
            TransferZRefreshOperationHover(w);
    }
}
