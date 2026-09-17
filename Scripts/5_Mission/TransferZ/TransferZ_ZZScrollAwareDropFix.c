// Widget.GetScreenPos() already reports the current on-screen position after a
// ScrollWidget has moved its content. Detached TransferZ drop overlays therefore
// copy that live screen rectangle and only clip it to the owning scroll viewport;
// applying GetVScrollPos() again shifts targets upward into the next container.
modded class TransferZHeaderControls
{
    override static bool TransferZPointInsideClippedWidget(Widget widget, ScrollWidget scroll, int mouseX, int mouseY)
    {
        if (!widget || !widget.IsVisibleHierarchy())
            return false;

        float x;
        float y;
        float w;
        float h;
        widget.GetScreenPos(x, y);
        widget.GetScreenSize(w, h);
        if (w <= 0.0 || h <= 0.0)
            return false;

        float left = x;
        float top = y;
        float right = x + w;
        float bottom = y + h;

        if (scroll && scroll.IsVisibleHierarchy())
        {
            float sx;
            float sy;
            float sw;
            float sh;
            scroll.GetScreenPos(sx, sy);
            scroll.GetScreenSize(sw, sh);

            if (sx > left)
                left = sx;
            if (sy > top)
                top = sy;
            if (sx + sw < right)
                right = sx + sw;
            if (sy + sh < bottom)
                bottom = sy + sh;
        }

        if (right <= left || bottom <= top)
            return false;

        return mouseX >= left && mouseX < right && mouseY >= top && mouseY < bottom;
    }

    override void UpdateDropTargetPosition()
    {
        if (!m_DropTarget || !m_DropHost)
            return;

        float x;
        float y;
        float w;
        float h;
        m_DropHost.GetScreenPos(x, y);
        m_DropHost.GetScreenSize(w, h);

        ScrollWidget scroll = FindScrollWidget();
        if (scroll && scroll.IsVisibleHierarchy())
        {
            float sx;
            float sy;
            float sw;
            float sh;
            scroll.GetScreenPos(sx, sy);
            scroll.GetScreenSize(sw, sh);

            float left = x;
            float top = y;
            float right = x + w;
            float bottom = y + h;

            if (sx > left)
                left = sx;
            if (sy > top)
                top = sy;
            if (sx + sw < right)
                right = sx + sw;
            if (sy + sh < bottom)
                bottom = sy + sh;

            if (right <= left || bottom <= top)
            {
                m_DropTarget.SetScreenPos(0.0, 0.0, false);
                m_DropTarget.SetScreenSize(0.0, 0.0, false);
                return;
            }

            x = left;
            y = top;
            w = right - left;
            h = bottom - top;
        }

        m_DropTarget.SetScreenPos(x, y, false);
        m_DropTarget.SetScreenSize(w, h, false);
    }
}
