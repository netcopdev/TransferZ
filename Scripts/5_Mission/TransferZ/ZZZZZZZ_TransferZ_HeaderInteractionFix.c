modded class TransferZHeaderControls
{
    bool TransferZMouseInsideControls()
    {
        if (!m_Root || !m_Root.IsVisibleHierarchy())
            return false;

        int mouseX;
        int mouseY;
        GetMousePos(mouseX, mouseY);

        float x;
        float y;
        float w;
        float h;
        m_Root.GetScreenPos(x, y);
        m_Root.GetScreenSize(w, h);

        return mouseX >= x && mouseX <= x + w && mouseY >= y && mouseY <= y + h;
    }
}

modded class ClosableHeader
{
    override void OnDragHeader(Widget w, int x, int y)
    {
        // ClosableHeader itself is draggable. TransferZ buttons live inside its
        // panel, so a small mouse movement during a D/L/P click can otherwise
        // start the vanilla header drag and hide the panel. Test the actual
        // TransferZ screen rectangle instead of widget ancestry because DayZ can
        // report the draggable header as the widget under the cursor once drag
        // detection starts.
        if (m_TransferZHeaderControls && m_TransferZHeaderControls.TransferZMouseInsideControls())
            return;

        super.OnDragHeader(w, x, y);
    }
}
