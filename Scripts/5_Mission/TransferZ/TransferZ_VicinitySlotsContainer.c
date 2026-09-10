modded class VicinitySlotsContainer
{
    override void DoubleClick(Widget w, int x, int y, int button)
    {
        if (button == MouseState.LEFT && !g_Game.IsLeftCtrlDown() && w)
        {
            ItemPreviewWidget preview = ItemPreviewWidget.Cast(w.FindAnyWidget("Render"));
            if (!preview)
            {
                string renderName = w.GetName();
                renderName.Replace("PanelWidget", "Render");
                preview = ItemPreviewWidget.Cast(w.FindAnyWidget(renderName));
            }
            if (!preview)
                preview = ItemPreviewWidget.Cast(w);

            if (preview)
            {
                ItemBase item = ItemBase.Cast(preview.GetItem());
                if (item && item.IsTakeable() && item.GetInventory().CanRemoveEntity())
                {
                    if (TransferZClientState.Get().TryRouteVicinityDoubleClick(item))
                        return;
                }
            }
        }

        super.DoubleClick(w, x, y, button);
    }
}
