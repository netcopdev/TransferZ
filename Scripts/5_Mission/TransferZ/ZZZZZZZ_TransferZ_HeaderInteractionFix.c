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

    override protected float TransferZNativeHandleRight()
    {
        float right = 0.0;
        float candidate;
        Widget widget;

        // ClosableHeader keeps its reorder arrows in MovePanel. The panel is
        // normally hidden until header hover, so reserve its geometry even while
        // hidden; otherwise TransferZ occupies the same pixels and the controls
        // collide as soon as the native arrows appear.
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

        // Other header variants expose their expand/collapse controls under
        // these names. Reserve them whether currently visible or not so the
        // TransferZ block does not jump when native state changes.
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
        // title and TransferZ controls appear to vanish. T/U have their own drag
        // behavior and are covered by the same guard.
        Widget hovered = GetWidgetUnderCursor();
        if (m_TransferZHeaderControls && m_TransferZHeaderControls.TransferZOwnsWidget(hovered))
            return;

        super.OnDragHeader(w, x, y);
    }
}
