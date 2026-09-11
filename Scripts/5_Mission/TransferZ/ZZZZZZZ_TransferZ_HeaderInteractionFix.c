modded class TransferZHeaderControls
{
    protected Widget TransferZFindNativeHeaderWidget(string name)
    {
        Widget scope = m_HeaderHost;
        int depth = 0;

        while (scope && depth < 6)
        {
            Widget found = scope.FindAnyWidget(name);
            if (found && found != m_Root)
                return found;

            scope = scope.GetParent();
            depth++;
        }

        return null;
    }

    protected float TransferZReservedWidgetRight(Widget widget)
    {
        if (!widget)
            return 0.0;

        float x;
        float y;
        float w;
        float h;
        widget.GetScreenPos(x, y);
        widget.GetScreenSize(w, h);

        if (w <= 0.0 || h <= 0.0)
            return 0.0;

        return x + w;
    }

    protected float TransferZNativeReservedRight()
    {
        float right = 0.0;
        float candidate;
        Widget widget;

        // ClosableHeader keeps its reorder arrows in MovePanel. The panel is
        // normally hidden until header hover, so reserve its geometry even when
        // hidden. This is the important difference from IsVisibleHierarchy().
        widget = TransferZFindNativeHeaderWidget("MovePanel");
        candidate = TransferZReservedWidgetRight(widget);
        if (candidate > right)
            right = candidate;

        widget = TransferZFindNativeHeaderWidget("MoveUp");
        candidate = TransferZReservedWidgetRight(widget);
        if (candidate > right)
            right = candidate;

        widget = TransferZFindNativeHeaderWidget("MoveDown");
        candidate = TransferZReservedWidgetRight(widget);
        if (candidate > right)
            right = candidate;

        widget = TransferZFindNativeHeaderWidget("opened");
        candidate = TransferZReservedWidgetRight(widget);
        if (candidate > right)
            right = candidate;

        widget = TransferZFindNativeHeaderWidget("closed");
        candidate = TransferZReservedWidgetRight(widget);
        if (candidate > right)
            right = candidate;

        widget = TransferZFindNativeHeaderWidget("collapse_button");
        candidate = TransferZReservedWidgetRight(widget);
        if (candidate > right)
            right = candidate;

        return right;
    }

    protected void TransferZApplyNativeSafePlacement(bool preferredVisible)
    {
        if (!m_Root || !m_HeaderLabel)
            return;

        float blockWidth = 86.0;
        if (preferredVisible)
            blockWidth = 107.0;

        // Always start from the geometry captured before TransferZ changed the
        // title. This makes D/L/P refreshes idempotent.
        RestoreHeaderText();

        float labelX;
        float labelY;
        float labelW;
        float labelH;
        m_HeaderLabel.GetScreenPos(labelX, labelY);
        m_HeaderLabel.GetScreenSize(labelW, labelH);

        float blockX = labelX;
        float nativeRight = TransferZNativeReservedRight();
        const float nativeGap = 4.0;
        if (nativeRight > 0.0 && nativeRight + nativeGap > blockX)
            blockX = nativeRight + nativeGap;

        float blockY = labelY + (labelH - 29.0) * 0.5;
        m_Root.SetScreenPos(blockX, blockY, false);
        m_Root.SetScreenSize(blockWidth, 29.0, false);

        const float titleGap = 5.0;
        float titleX = blockX + blockWidth + titleGap;
        float originalRight = labelX + labelW;
        float targetWidth = originalRight - titleX;
        if (targetWidth < 20.0)
            targetWidth = 20.0;

        m_HeaderLabel.SetScreenPos(titleX, labelY, false);
        m_HeaderLabel.SetScreenSize(targetWidth, labelH, false);
    }

    override void UpdateControls()
    {
        // Run the existing behavior/styling first, then make one final placement
        // pass that reserves hidden native controls. UpdateControls exists on the
        // original TransferZHeaderControls class, so this is a real override.
        super.UpdateControls();

        if (!m_Root || !m_Entity || !m_Entity.GetInventory().GetCargo())
            return;

        TransferZApplyNativeSafePlacement(CanBePreferred());
    }

    bool TransferZOwnsWidget(Widget widget)
    {
        while (widget)
        {
            if (widget == m_Root)
                return true;

            widget = widget.GetParent();
        }

        return false;
    }
}

modded class ClosableHeader
{
    override void OnDragHeader(Widget w, int x, int y)
    {
        // The native ClosableHeader itself is draggable. D/L/P are ordinary
        // buttons, so a small mouse movement while clicking them can otherwise
        // start the native header drag, which hides m_PanelWidget and makes the
        // title and TransferZ controls appear to vanish.
        Widget hovered = GetWidgetUnderCursor();
        if (m_TransferZHeaderControls && m_TransferZHeaderControls.TransferZOwnsWidget(hovered))
            return;

        super.OnDragHeader(w, x, y);
    }
}
