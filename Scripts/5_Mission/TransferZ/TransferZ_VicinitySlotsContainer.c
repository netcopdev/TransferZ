class TransferZVicinityHeaderControls
{
    protected static TransferZVicinityHeaderControls s_Instance;

    protected Widget m_Root;
    protected Widget m_TooltipRoot;
    protected TextWidget m_TooltipText;
    protected ImageWidget m_TooltipBackground;
    protected Widget m_DropTarget;
    protected ButtonWidget m_DestinationButton;
    protected ButtonWidget m_TransferButton;
    protected ButtonWidget m_UnpackButton;
    protected ImageWidget m_TransferStatus;
    protected ImageWidget m_UnpackStatus;
    protected Widget m_HoveredOperation;
    protected VicinitySlotsContainer m_Source;
    protected VicinityContainer m_Owner;
    protected int m_IgnoreOperationClickUntil;

    void TransferZVicinityHeaderControls(Widget parent, VicinitySlotsContainer source, VicinityContainer owner)
    {
        if (!parent || !source || !owner)
            return;

        s_Instance = this;
        m_Source = source;
        m_Owner = owner;

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
            m_TooltipBackground = ImageWidget.Cast(m_TooltipRoot.FindAnyWidget("TransferZ_TooltipBackground"));
            if (m_TooltipBackground)
            {
                m_TooltipBackground.LoadImageFile(0, "#(argb,8,8,3)color(1,1,1,1,ca)");
                m_TooltipBackground.SetImage(0);
                m_TooltipBackground.SetColor(ARGB(210, 0, 0, 0));
            }
            m_TooltipRoot.SetSort(10000);
            m_TooltipRoot.Show(false);
        }

        m_DropTarget = GetGame().GetWorkspace().CreateWidgets("TransferZ/GUI/layouts/transferz_header_drop_target.layout");
        if (m_DropTarget)
        {
            m_DropTarget.SetSort(9990);
            WidgetEventHandler handler = WidgetEventHandler.GetInstance();
            handler.RegisterOnDropReceived(m_DropTarget, this, "OnOperationDropReceived");
            handler.RegisterOnDraggingOver(m_DropTarget, this, "OnOperationDraggingOver");
            handler.RegisterOnMouseWheel(m_DropTarget, this, "OnOperationMouseWheel");
            m_DropTarget.Show(false);
        }

        m_DestinationButton = ButtonWidget.Cast(m_Root.FindAnyWidget("TransferZ_VicinityDestination"));
        m_TransferButton = ButtonWidget.Cast(m_Root.FindAnyWidget("TransferZ_VicinityTransfer"));
        m_UnpackButton = ButtonWidget.Cast(m_Root.FindAnyWidget("TransferZ_VicinityUnpack"));
        m_TransferStatus = ImageWidget.Cast(m_Root.FindAnyWidget("TransferZ_VicinityTransferStatus"));
        m_UnpackStatus = ImageWidget.Cast(m_Root.FindAnyWidget("TransferZ_VicinityUnpackStatus"));

        PrepareStatusImage(m_TransferStatus);
        PrepareStatusImage(m_UnpackStatus);

        RegisterButton(m_DestinationButton, "OnDestination");
        RegisterOperationButton(m_TransferButton, "OnTransfer");
        RegisterOperationButton(m_UnpackButton, "OnUnpack");
        UpdateControls();
    }

    void ~TransferZVicinityHeaderControls()
    {
        if (s_Instance == this)
            s_Instance = null;

        WidgetEventHandler handler = WidgetEventHandler.GetInstance();
        if (handler && m_DropTarget)
            handler.UnregisterWidget(m_DropTarget);

        if (m_TooltipRoot)
        {
            m_TooltipRoot.Unlink();
            m_TooltipRoot = null;
        }

        if (m_DropTarget)
        {
            m_DropTarget.Unlink();
            m_DropTarget = null;
        }
    }

    protected void PrepareStatusImage(ImageWidget image)
    {
        if (!image)
            return;

        image.LoadImageFile(0, "#(argb,8,8,3)color(1,1,1,1,ca)");
        image.SetImage(0);
        image.Show(false);
    }

    static void Refresh()
    {
        if (s_Instance)
            s_Instance.UpdateControls();
    }

    static bool IsVicinityOpen()
    {
        if (!s_Instance || !s_Instance.m_Source || !s_Instance.m_Owner)
            return false;

        Widget root = s_Instance.m_Source.GetRootWidget();
        if (!root || !root.IsVisibleHierarchy())
            return false;

        return !s_Instance.m_Owner.IsHidden();
    }

    static void SetOperationDropTargetVisible(bool show)
    {
        if (s_Instance)
            s_Instance.SetDropTargetVisible(show);
    }

    protected void RegisterButton(ButtonWidget button, string clickFunction)
    {
        if (!button)
            return;

        WidgetEventHandler handler = WidgetEventHandler.GetInstance();
        handler.RegisterOnClick(button, this, clickFunction);
        handler.RegisterOnMouseEnter(button, this, "OnButtonMouseEnter");
        handler.RegisterOnMouseLeave(button, this, "OnButtonMouseLeave");
    }

    protected void RegisterOperationButton(ButtonWidget button, string clickFunction)
    {
        RegisterButton(button, clickFunction);
        if (!button)
            return;

        button.SetFlags(WidgetFlags.DRAGGABLE);
        WidgetEventHandler handler = WidgetEventHandler.GetInstance();
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

    protected string DestinationName()
    {
        TransferZClientState state = TransferZClientState.Get();
        if (state.IsDestinationVicinity())
            return "VICINITY";

        EntityAI destination = state.GetDestination();
        if (destination)
            return DisplayName(destination);

        return "";
    }

    protected string TooltipFor(Widget w)
    {
        TransferZClientState state = TransferZClientState.Get();
        string destinationName = DestinationName();

        if (w == m_DestinationButton)
        {
            if (state.IsDestinationVicinity())
                return "Destination: VICINITY";
            return "Set destination: VICINITY";
        }

        if (w == m_TransferButton)
        {
            if (state.IsDestinationVicinity())
                return "Loose items are already in VICINITY";
            if (destinationName != "")
                return "Move loose vicinity items -> " + destinationName;
            return "Move loose vicinity items: select destination";
        }

        if (w == m_UnpackButton)
        {
            if (destinationName != "")
                return "Unpack vicinity containers -> " + destinationName;
            return "Unpack vicinity containers: select destination";
        }

        return "";
    }

    protected string WrapTooltipText(string text, out int lineCount)
    {
        lineCount = 1;
        if (text == "")
            return text;

        ref array<string> words = new array<string>();
        text.Split(" ", words);

        const int MAX_LINE_CHARS = 34;
        string result = "";
        string line = "";

        foreach (string word : words)
        {
            if (word == "")
                continue;

            if (line == "")
            {
                line = word;
                continue;
            }

            if (line.Length() + 1 + word.Length() <= MAX_LINE_CHARS)
            {
                line += " " + word;
            }
            else
            {
                if (result != "")
                    result += "\n";
                result += line;
                line = word;
                lineCount++;
            }
        }

        if (line != "")
        {
            if (result != "")
                result += "\n";
            result += line;
        }

        return result;
    }

    protected void PositionTooltip(Widget source, float tooltipH)
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

        m_TooltipRoot.SetScreenSize(tooltipW, tooltipH, false);
        if (m_TooltipBackground)
            m_TooltipBackground.SetSize(tooltipW, tooltipH, false);
        if (m_TooltipText)
            m_TooltipText.SetSize(tooltipW - 16.0, tooltipH - 10.0, false);
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

        int lineCount;
        string wrapped = WrapTooltipText(text, lineCount);
        float tooltipH = 10.0 + lineCount * 18.0;
        if (tooltipH < 34.0)
            tooltipH = 34.0;

        m_TooltipText.SetText(wrapped);
        PositionTooltip(source, tooltipH);
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

    protected int StatusColor(int result)
    {
        if (result == TransferZOperationPreviewResult.READY)
            return ARGB(105, 68, 96, 72);
        if (result == TransferZOperationPreviewResult.PARTIAL)
            return ARGB(110, 125, 103, 48);
        return ARGB(110, 112, 58, 55);
    }

    protected void ShowOperationStatus(Widget button)
    {
        if (!m_Source)
            return;

        ImageWidget status;
        int operation;
        if (button == m_TransferButton)
        {
            status = m_TransferStatus;
            operation = TransferZOperation.TRANSFER;
        }
        else if (button == m_UnpackButton)
        {
            status = m_UnpackStatus;
            operation = TransferZOperation.UNPACK;
        }
        else
        {
            return;
        }

        if (!status)
            return;

        ref array<EntityAI> items = new array<EntityAI>();
        Snapshot(items);
        int result = TransferZOperationPreview.EvaluateVicinityOperation(operation, items);
        status.SetColor(StatusColor(result));
        status.Show(true);
        m_HoveredOperation = button;
    }

    protected void HideOperationStatus()
    {
        if (m_TransferStatus)
            m_TransferStatus.Show(false);
        if (m_UnpackStatus)
            m_UnpackStatus.Show(false);
        m_HoveredOperation = null;
    }

    protected void RefreshOperationStatus()
    {
        if (!m_HoveredOperation)
            return;

        Widget hovered = m_HoveredOperation;
        if (m_TransferStatus)
            m_TransferStatus.Show(false);
        if (m_UnpackStatus)
            m_UnpackStatus.Show(false);
        ShowOperationStatus(hovered);
    }

    protected void UpdateDropTargetPosition()
    {
        if (!m_DropTarget || !m_Source || !m_Source.GetRootWidget())
            return;

        float x;
        float y;
        float w;
        float h;
        m_Source.GetRootWidget().GetScreenPos(x, y);
        m_Source.GetRootWidget().GetScreenSize(w, h);
        m_DropTarget.SetScreenPos(x, y, false);
        m_DropTarget.SetScreenSize(w, h, false);
    }

    protected void SetDropTargetVisible(bool show)
    {
        if (!m_DropTarget)
            return;

        bool canShow = show && TransferZOperationDrag.IsActive() && IsVicinityOpen();
        if (canShow && TransferZOperationDrag.IsFromVicinity() && TransferZOperationDrag.GetOperation() == TransferZOperation.TRANSFER)
            canShow = false;

        if (canShow)
            UpdateDropTargetPosition();
        m_DropTarget.Show(canShow);
    }

    bool OnButtonMouseEnter(Widget w, int x, int y)
    {
        ShowTooltip(w);
        if (w == m_TransferButton || w == m_UnpackButton)
            ShowOperationStatus(w);
        return true;
    }

    bool OnButtonMouseLeave(Widget w, Widget enter_w, int x, int y)
    {
        HideTooltip();
        if (w == m_TransferButton || w == m_UnpackButton)
            HideOperationStatus();
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
        HideOperationStatus();
        TransferZHeaderControls.SetOperationDropTargetsVisible(true);
    }

    void OnOperationDrop(Widget w, int x, int y)
    {
        TransferZHeaderControls.CancelOperationDragLater();
    }

    void OnOperationDraggingOver(Widget w, int x, int y, Widget receiver)
    {
        if (TransferZOperationDrag.IsActive())
            UpdateDropTargetPosition();
    }

    void OnOperationDropReceived(Widget w, int x, int y, Widget receiver)
    {
        if (!TransferZOperationDrag.IsActive())
            return;

        TransferZOperationDrag.CompleteToVicinity();
        TransferZHeaderControls.SetOperationDropTargetsVisible(false);
        TransferZHeaderControls.RefreshAll();
    }

    void OnOperationMouseWheel(Widget w, int x, int y, int wheel)
    {
        if (m_Owner)
        {
            ScrollWidget scroll = m_Owner.TransferZGetScrollWidget();
            if (scroll)
                scroll.VScrollStep(-wheel);
        }

        if (TransferZOperationDrag.IsActive())
            UpdateDropTargetPosition();
    }

    void UpdateControls()
    {
        if (!m_Root)
            return;

        TransferZClientState state = TransferZClientState.Get();
        if (m_DestinationButton)
        {
            if (state.IsDestinationVicinity())
                m_DestinationButton.SetText("D*");
            else
                m_DestinationButton.SetText("D");
        }

        RefreshOperationStatus();

        if (TransferZOperationDrag.IsActive())
            SetDropTargetVisible(true);
        else
            SetDropTargetVisible(false);
    }

    void OnDestination(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT)
            return;

        TransferZClientState.Get().SetVicinityDestination();
        TransferZHeaderControls.RefreshAll();
        ShowTooltip(w);
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
        RefreshOperationStatus();
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
        RefreshOperationStatus();
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
            m_TransferZVicinityHeaderControls = new TransferZVicinityHeaderControls(m_CollapsibleHeader.GetMainWidget(), m_VicinityIconsContainer, this);
    }

    ScrollWidget TransferZGetScrollWidget()
    {
        #ifndef PLATFORM_CONSOLE
        return m_CargoScrollWidget;
        #else
        return null;
        #endif
    }
}
