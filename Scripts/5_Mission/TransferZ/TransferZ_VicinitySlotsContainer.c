class TransferZVicinityHeaderControls
{
    protected Widget m_Root;
    protected Widget m_TooltipRoot;
    protected TextWidget m_TooltipText;
    protected ButtonWidget m_TransferButton;
    protected ButtonWidget m_UnpackButton;
    protected VicinitySlotsContainer m_Source;
    protected int m_IgnoreOperationClickUntil;

    void TransferZVicinityHeaderControls(Widget parent, VicinitySlotsContainer source)
    {
        if (!parent || !source)
            return;

        m_Source = source;
        m_Root = GetGame().GetWorkspace().CreateWidgets("TransferZ/GUI/layouts/transferz_vicinity_controls.layout", parent);
        if (!m_Root)
        {
            Print("[TransferZ] Vicinity controls layout creation failed");
            return;
        }

        m_TooltipRoot = GetGame().GetWorkspace().CreateWidgets("TransferZ/GUI/layouts/transferz_header_tooltip.layout");
        if (m_TooltipRoot)
        {
            m_TooltipText = TextWidget.Cast(m_TooltipRoot.FindAnyWidget("TransferZ_TooltipText"));
            m_TooltipRoot.SetSort(10000);
            m_TooltipRoot.Show(false);
        }

        m_TransferButton = ButtonWidget.Cast(m_Root.FindAnyWidget("TransferZ_VicinityTransfer"));
        m_UnpackButton = ButtonWidget.Cast(m_Root.FindAnyWidget("TransferZ_VicinityUnpack"));

        RegisterOperationButton(m_TransferButton, "OnTransfer");
        RegisterOperationButton(m_UnpackButton, "OnUnpack");
    }

    void ~TransferZVicinityHeaderControls()
    {
        if (m_TooltipRoot)
        {
            m_TooltipRoot.Unlink();
            m_TooltipRoot = null;
        }
    }

    protected void RegisterOperationButton(ButtonWidget button, string clickFunction)
    {
        if (!button)
            return;

        button.SetFlags(WidgetFlags.DRAGGABLE);
        WidgetEventHandler handler = WidgetEventHandler.GetInstance();
        handler.RegisterOnClick(button, this, clickFunction);
        handler.RegisterOnMouseEnter(button, this, "OnButtonMouseEnter");
        handler.RegisterOnMouseLeave(button, this, "OnButtonMouseLeave");
        handler.RegisterOnDrag(button, this, "OnOperationDrag");
        handler.RegisterOnDrop(button, this, "OnOperationDrop");
    }

    protected string DisplayName(EntityAI entity)
    {
        if (!entity)
            return "none";

        string name = entity.GetDisplayName();
        if (name == "")
            name = entity.GetType();
        return name;
    }

    protected string TooltipFor(Widget w)
    {
        EntityAI destination = TransferZClientState.Get().GetDestination();

        if (w == m_TransferButton)
        {
            if (destination)
                return "Move loose vicinity items -> " + DisplayName(destination);
            return "Move loose vicinity items: select destination";
        }

        if (w == m_UnpackButton)
        {
            if (destination)
                return "Unpack vicinity containers -> " + DisplayName(destination);
            return "Unpack vicinity containers: select destination";
        }

        return "";
    }

    protected void PositionTooltip(Widget source)
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

        float tooltipW = 270.0;
        float tooltipH = 30.0;
        float tooltipX = sourceX + sourceW * 0.5 - tooltipW * 0.5;
        float tooltipY = sourceY - tooltipH - 2.0;

        if (tooltipX < 4.0)
            tooltipX = 4.0;
        if (tooltipX + tooltipW > screenW - 4.0)
            tooltipX = screenW - tooltipW - 4.0;
        if (tooltipY < 4.0)
            tooltipY = sourceY + sourceH + 2.0;

        m_TooltipRoot.SetScreenSize(tooltipW, tooltipH, false);
        m_TooltipRoot.SetScreenPos(tooltipX, tooltipY, false);
    }

    protected void ShowTooltip(Widget source)
    {
        if (!m_TooltipRoot || !m_TooltipText)
            return;

        string text = TooltipFor(source);
        if (text == "")
        {
            HideTooltip();
            return;
        }

        ItemManager itemManager = ItemManager.GetInstance();
        if (itemManager)
            itemManager.HideTooltip();

        m_TooltipText.SetText(text);
        PositionTooltip(source);
        m_TooltipRoot.Show(true);
    }

    protected void HideTooltip()
    {
        if (m_TooltipRoot)
            m_TooltipRoot.Show(false);
    }

    protected void Snapshot(notnull array<EntityAI> items)
    {
        items.Clear();
        if (m_Source)
            m_Source.TransferZSnapshotVisibleItems(items);
    }

    bool OnButtonMouseEnter(Widget w, int x, int y)
    {
        ShowTooltip(w);
        return true;
    }

    bool OnButtonMouseLeave(Widget w, Widget enter_w, int x, int y)
    {
        HideTooltip();
        return true;
    }

    void OnOperationDrag(Widget w, int x, int y)
    {
        if (!m_Source)
            return;

        ref array<EntityAI> items = new array<EntityAI>();
        Snapshot(items);
        if (items.Count() == 0)
            return;

        if (w == m_TransferButton)
            TransferZOperationDrag.BeginVicinity(TransferZOperation.TRANSFER, items);
        else if (w == m_UnpackButton)
            TransferZOperationDrag.BeginVicinity(TransferZOperation.UNPACK, items);
        else
            return;

        m_IgnoreOperationClickUntil = GetGame().GetTime() + 250;
        HideTooltip();
        TransferZHeaderControls.SetOperationDropTargetsVisible(true);
    }

    void OnOperationDrop(Widget w, int x, int y)
    {
        TransferZHeaderControls.CancelOperationDragLater();
    }

    void OnTransfer(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT || !m_Source)
            return;
        if (GetGame().GetTime() < m_IgnoreOperationClickUntil)
            return;

        ref array<EntityAI> items = new array<EntityAI>();
        Snapshot(items);
        TransferZClientState.Get().RequestVicinityTransfer(items);
        ShowTooltip(w);
    }

    void OnUnpack(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT || !m_Source)
            return;
        if (GetGame().GetTime() < m_IgnoreOperationClickUntil)
            return;

        ref array<EntityAI> items = new array<EntityAI>();
        Snapshot(items);
        TransferZClientState.Get().RequestVicinityUnpack(items);
        ShowTooltip(w);
    }
}

modded class VicinitySlotsContainer
{
    void TransferZSnapshotVisibleItems(notnull array<EntityAI> items)
    {
        items.Clear();
        if (!m_ShowedItems)
            return;

        for (int i = 0; i < m_ShowedItems.Count(); i++)
        {
            EntityAI item = m_ShowedItems.Get(i);
            if (item)
                items.Insert(item);
        }
    }

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

modded class VicinityContainer
{
    protected ref TransferZVicinityHeaderControls m_TransferZVicinityHeaderControls;

    void VicinityContainer(LayoutHolder parent, int sort = -1)
    {
        if (m_CollapsibleHeader && m_VicinityIconsContainer)
            m_TransferZVicinityHeaderControls = new TransferZVicinityHeaderControls(m_CollapsibleHeader.GetMainWidget(), m_VicinityIconsContainer);
    }
}
