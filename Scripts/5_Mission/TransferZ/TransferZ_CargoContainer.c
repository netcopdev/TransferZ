class TransferZHeaderControls
{
    protected static ref array<TransferZHeaderControls> s_Instances;

    protected EntityAI m_Entity;
    protected Container m_OwnerContainer;
    protected Widget m_HeaderHost;
    protected Widget m_DropHost;
    protected Widget m_Root;
    protected Widget m_DropTarget;
    protected Widget m_HeaderLabel;
    protected float m_HeaderLabelX;
    protected float m_HeaderLabelY;
    protected float m_HeaderLabelW;
    protected float m_HeaderLabelH;
    protected Widget m_TooltipRoot;
    protected TextWidget m_TooltipText;
    protected ImageWidget m_TooltipBackground;
    protected ButtonWidget m_DestinationButton;
    protected ButtonWidget m_TransferButton;
    protected ButtonWidget m_UnpackButton;
    protected ButtonWidget m_LinkButton;
    protected ButtonWidget m_PreferredButton;
    protected ImageWidget m_TransferStatus;
    protected ImageWidget m_UnpackStatus;
    protected Widget m_HoveredOperation;
    protected int m_IgnoreOperationClickUntil;

    void TransferZHeaderControls(Widget parent, Container owner = null)
    {
        if (!parent)
            return;

        m_HeaderHost = parent;
        m_OwnerContainer = owner;
        m_DropHost = parent;
        if (m_OwnerContainer && m_OwnerContainer.GetRootWidget())
            m_DropHost = m_OwnerContainer.GetRootWidget();

        if (!s_Instances)
            s_Instances = new array<TransferZHeaderControls>();
        s_Instances.Insert(this);

        m_HeaderLabel = parent.FindAnyWidget("TextWidget0");
        if (m_HeaderLabel)
        {
            m_HeaderLabel.GetPos(m_HeaderLabelX, m_HeaderLabelY);
            m_HeaderLabel.GetSize(m_HeaderLabelW, m_HeaderLabelH);
        }

        m_Root = GetGame().GetWorkspace().CreateWidgets("TransferZ/GUI/layouts/transferz_header_controls.layout", parent);
        if (!m_Root)
        {
            Print("[TransferZ] Header controls layout creation failed");
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

        m_DestinationButton = ButtonWidget.Cast(m_Root.FindAnyWidget("TransferZ_Destination"));
        m_TransferButton = ButtonWidget.Cast(m_Root.FindAnyWidget("TransferZ_Transfer"));
        m_UnpackButton = ButtonWidget.Cast(m_Root.FindAnyWidget("TransferZ_Unpack"));
        m_LinkButton = ButtonWidget.Cast(m_Root.FindAnyWidget("TransferZ_Link"));
        m_PreferredButton = ButtonWidget.Cast(m_Root.FindAnyWidget("TransferZ_Preferred"));
        m_TransferStatus = ImageWidget.Cast(m_Root.FindAnyWidget("TransferZ_TransferStatus"));
        m_UnpackStatus = ImageWidget.Cast(m_Root.FindAnyWidget("TransferZ_UnpackStatus"));

        PrepareStatusImage(m_TransferStatus);
        PrepareStatusImage(m_UnpackStatus);

        RegisterButton(m_DestinationButton, "OnDestination");
        RegisterOperationButton(m_TransferButton, "OnTransfer");
        RegisterOperationButton(m_UnpackButton, "OnUnpack");
        RegisterButton(m_LinkButton, "OnLink");
        RegisterButton(m_PreferredButton, "OnPreferred");

        m_Root.Show(false);
    }

    void ~TransferZHeaderControls()
    {
        if (s_Instances)
        {
            int index = s_Instances.Find(this);
            if (index >= 0)
                s_Instances.Remove(index);
        }

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

    static void RefreshAll()
    {
        if (s_Instances)
        {
            for (int i = s_Instances.Count() - 1; i >= 0; i--)
            {
                TransferZHeaderControls controls = s_Instances.Get(i);
                if (!controls)
                {
                    s_Instances.Remove(i);
                    continue;
                }

                controls.UpdateControls();
            }
        }

        TransferZVicinityHeaderControls.Refresh();
    }

    static bool IsEntityOpenOrInHands(EntityAI entity)
    {
        if (!entity)
            return false;

        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (player && player.GetEntityInHands() == entity)
            return true;

        if (!s_Instances)
            return false;

        for (int i = s_Instances.Count() - 1; i >= 0; i--)
        {
            TransferZHeaderControls controls = s_Instances.Get(i);
            if (controls && controls.m_Entity == entity && controls.IsOpenTarget())
                return true;
        }

        return false;
    }

    protected bool IsOpenTarget()
    {
        if (!m_Entity || !m_HeaderHost || !m_HeaderHost.IsVisibleHierarchy())
            return false;

        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (player && player.GetEntityInHands() == m_Entity)
            return true;

        if (m_OwnerContainer && !m_OwnerContainer.IsOpened())
            return false;

        if (m_DropHost && !m_DropHost.IsVisibleHierarchy())
            return false;

        return true;
    }

    static void SetOperationDropTargetsVisible(bool show)
    {
        EntityAI source = TransferZOperationDrag.GetSource();

        if (s_Instances)
        {
            for (int i = s_Instances.Count() - 1; i >= 0; i--)
            {
                TransferZHeaderControls controls = s_Instances.Get(i);
                if (controls)
                    controls.SetDropTargetVisible(show, source);
            }
        }

        TransferZVicinityHeaderControls.SetOperationDropTargetVisible(show);
    }

    static void CancelOperationDrag()
    {
        TransferZOperationDrag.Clear();
        SetOperationDropTargetsVisible(false);
    }

    static void CancelOperationDragLater()
    {
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(CancelOperationDrag, 75, false);
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

    protected void RestoreHeaderText()
    {
        if (!m_HeaderLabel)
            return;

        m_HeaderLabel.SetPos(m_HeaderLabelX, m_HeaderLabelY, false);
        m_HeaderLabel.SetSize(m_HeaderLabelW, m_HeaderLabelH, true);
    }

    protected void ReserveHeaderText(bool preferredVisible)
    {
        if (!m_HeaderLabel)
            return;

        float targetWidth = m_HeaderLabelW - 0.17;
        float shift = 38.0;

        if (preferredVisible)
        {
            targetWidth = m_HeaderLabelW - 0.22;
            shift = 48.0;
        }

        if (targetWidth < 0.55)
            targetWidth = 0.55;

        m_HeaderLabel.SetPos(m_HeaderLabelX - shift, m_HeaderLabelY, false);
        m_HeaderLabel.SetSize(targetWidth, m_HeaderLabelH, true);
    }

    void SetEntity(EntityAI entity)
    {
        m_Entity = entity;

        if (!m_Root)
            return;

        bool show = m_Entity && m_Entity.GetInventory().GetCargo();
        m_Root.Show(show);

        if (show)
            UpdateControls();
        else
        {
            HideTooltip();
            HideOperationStatus();
            RestoreHeaderText();
            SetDropTargetVisible(false, null);
        }
    }

    protected bool CanBePreferred()
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || !m_Entity || !m_Entity.GetInventory().GetCargo())
            return false;

        EntityAI current = m_Entity;
        int depth = 0;

        while (current && current != player && depth < 16)
        {
            InventoryLocation location = new InventoryLocation();
            if (!current.GetInventory().GetCurrentInventoryLocation(location))
                return false;
            if (location.GetType() != InventoryLocationType.ATTACHMENT)
                return false;

            EntityAI parent = location.GetParent();
            if (!parent)
                return false;

            current = parent;
            depth++;
        }

        return current == player && depth > 0;
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
        if (!m_Entity)
            return "";

        TransferZClientState state = TransferZClientState.Get();
        string destinationName = DestinationName();

        if (w == m_DestinationButton)
        {
            if (state.IsDestination(m_Entity))
                return "Destination: " + DisplayName(m_Entity);
            return "Set destination: " + DisplayName(m_Entity);
        }

        if (w == m_TransferButton)
        {
            if (destinationName != "")
                return "Transfer contents -> " + destinationName;
            return "Transfer: select destination";
        }

        if (w == m_UnpackButton)
        {
            if (destinationName != "")
                return "Unpack contents -> " + destinationName;
            return "Unpack: select destination";
        }

        if (w == m_LinkButton)
        {
            EntityAI linked = state.GetLinkedDestination(m_Entity);
            if (linked)
                return "Linked -> " + DisplayName(linked) + " (double-click moves items)";
            if (state.IsLinkAnchor(m_Entity))
                return "Link anchor: choose another container";
            return "Link this container";
        }

        if (w == m_PreferredButton)
        {
            if (state.IsPreferred(m_Entity))
                return "Preferred pickup: " + DisplayName(m_Entity);
            return "Set preferred pickup: " + DisplayName(m_Entity);
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

        if (!status || !m_Entity)
            return;

        int result = TransferZOperationPreview.EvaluateContainerOperation(operation, m_Entity);
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
        if (!m_DropTarget || !m_DropHost)
            return;

        float x;
        float y;
        float w;
        float h;
        m_DropHost.GetScreenPos(x, y);
        m_DropHost.GetScreenSize(w, h);
        m_DropTarget.SetScreenPos(x, y, false);
        m_DropTarget.SetScreenSize(w, h, false);
    }

    protected void SetDropTargetVisible(bool show, EntityAI source)
    {
        if (!m_DropTarget)
            return;

        bool canShow = show && TransferZOperationDrag.IsActive() && m_Entity && m_Entity.GetInventory().GetCargo() && IsOpenTarget();
        if (canShow && source && source == m_Entity)
        {
            bool selfUnpack = TransferZOperationDrag.GetOperation() == TransferZOperation.UNPACK && !TransferZOperationDrag.IsClassTransfer() && !TransferZOperationDrag.IsFromVicinity();
            if (!selfUnpack)
                canShow = false;
        }

        if (canShow)
            UpdateDropTargetPosition();
        m_DropTarget.Show(canShow);
    }

    protected ScrollWidget FindScrollWidget()
    {
        LayoutHolder current = m_OwnerContainer;
        while (current)
        {
            Container container = Container.Cast(current);
            if (container)
            {
                ScrollWidget scroll = container.GetScrollWidget();
                if (scroll)
                    return scroll;

                scroll = container.GetSlotsScrollWidget();
                if (scroll)
                    return scroll;
            }

            current = current.GetParent();
        }

        return null;
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
        if (!m_Entity)
            return;

        if (w == m_TransferButton)
            TransferZOperationDrag.BeginContainer(TransferZOperation.TRANSFER, m_Entity);
        else if (w == m_UnpackButton)
            TransferZOperationDrag.BeginContainer(TransferZOperation.UNPACK, m_Entity);
        else
            return;

        m_IgnoreOperationClickUntil = GetGame().GetTime() + 250;
        HideTooltip();
        HideOperationStatus();
        SetOperationDropTargetsVisible(true);
    }

    void OnOperationDrop(Widget w, int x, int y)
    {
        CancelOperationDragLater();
    }

    void OnOperationDraggingOver(Widget w, int x, int y, Widget receiver)
    {
        if (TransferZOperationDrag.IsActive())
            UpdateDropTargetPosition();
    }

    void OnOperationDropReceived(Widget w, int x, int y, Widget receiver)
    {
        if (!TransferZOperationDrag.IsActive() || !m_Entity)
            return;

        TransferZOperationDrag.Complete(m_Entity);
        SetOperationDropTargetsVisible(false);
        RefreshAll();
    }

    void OnOperationMouseWheel(Widget w, int x, int y, int wheel)
    {
        ScrollWidget scroll = FindScrollWidget();
        if (scroll)
            scroll.VScrollStep(-wheel);

        if (TransferZOperationDrag.IsActive())
            UpdateDropTargetPosition();
    }

    void UpdateControls()
    {
        if (!m_Root || !m_Entity || !m_Entity.GetInventory().GetCargo())
            return;

        TransferZClientState state = TransferZClientState.Get();

        if (m_DestinationButton)
        {
            if (state.IsDestination(m_Entity))
                m_DestinationButton.SetText("D*");
            else
                m_DestinationButton.SetText("D");
        }

        if (m_LinkButton)
        {
            if (state.IsLinked(m_Entity))
                m_LinkButton.SetText("L*");
            else if (state.IsLinkAnchor(m_Entity))
                m_LinkButton.SetText("L+");
            else
                m_LinkButton.SetText("L");
        }

        bool canPrefer = CanBePreferred();
        if (m_PreferredButton)
        {
            m_PreferredButton.Show(canPrefer);
            if (canPrefer && state.IsPreferred(m_Entity))
                m_PreferredButton.SetText("P*");
            else
                m_PreferredButton.SetText("P");
        }

        if (canPrefer)
            m_Root.SetSize(103, 29, true);
        else
            m_Root.SetSize(82, 29, true);

        ReserveHeaderText(canPrefer);
        RefreshOperationStatus();

        if (TransferZOperationDrag.IsActive())
            SetDropTargetVisible(true, TransferZOperationDrag.GetSource());
        else
            SetDropTargetVisible(false, null);
    }

    void OnDestination(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT || !m_Entity)
            return;

        TransferZClientState.Get().SetDestination(m_Entity);
        RefreshAll();
        ShowTooltip(w);
    }

    void OnTransfer(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT || !m_Entity)
            return;
        if (GetGame().GetTime() < m_IgnoreOperationClickUntil)
            return;

        TransferZClientState.Get().RequestTransfer(m_Entity);
        ShowTooltip(w);
        RefreshOperationStatus();
    }

    void OnUnpack(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT || !m_Entity)
            return;
        if (GetGame().GetTime() < m_IgnoreOperationClickUntil)
            return;

        TransferZClientState.Get().RequestUnpack(m_Entity);
        ShowTooltip(w);
        RefreshOperationStatus();
    }

    void OnLink(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT || !m_Entity)
            return;

        TransferZClientState.Get().ToggleLink(m_Entity);
        RefreshAll();
        ShowTooltip(w);
    }

    void OnPreferred(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT || !m_Entity)
            return;

        TransferZClientState.Get().SetPreferred(m_Entity);
        RefreshAll();
        ShowTooltip(w);
    }
}

modded class Header
{
    protected ref TransferZHeaderControls m_TransferZHeaderControls;

    override void SetItemPreview(EntityAI entity_ai)
    {
        super.SetItemPreview(entity_ai);

        if (m_TransferZHeaderControls)
            m_TransferZHeaderControls.SetEntity(entity_ai);
    }
}

modded class ClosableHeader
{
    void ClosableHeader(LayoutHolder parent, string function_name)
    {
        m_TransferZHeaderControls = new TransferZHeaderControls(m_PanelWidget, Container.Cast(m_Parent));
    }

    override void UpdateInterval()
    {
        super.UpdateInterval();

        if (m_TransferZHeaderControls)
            m_TransferZHeaderControls.UpdateControls();
    }
}

modded class HandsHeader
{
    protected EntityAI m_TransferZLastHandsEntity;

    void HandsHeader(LayoutHolder parent, string function_name)
    {
        m_TransferZHeaderControls = new TransferZHeaderControls(m_ItemHeader, Container.Cast(m_Parent));
    }

    override void UpdateInterval()
    {
        super.UpdateInterval();

        if (!m_TransferZHeaderControls)
            return;

        PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
        if (!player)
        {
            m_TransferZHeaderControls.SetEntity(null);
            m_TransferZLastHandsEntity = null;
            return;
        }

        EntityAI currentHands = player.GetEntityInHands();
        if (m_TransferZLastHandsEntity && m_TransferZLastHandsEntity != currentHands)
        {
            if (TransferZClientState.Get().OnContainerLeftHands(m_TransferZLastHandsEntity))
                TransferZHeaderControls.RefreshAll();
        }

        m_TransferZLastHandsEntity = currentHands;
        m_TransferZHeaderControls.SetEntity(currentHands);
    }
}

modded class CargoContainer
{
    protected ref TransferZHeaderControls m_TransferZAttachmentHeaderControls;

    void CargoContainer(LayoutHolder parent, bool is_attachment = false)
    {
        if (m_IsAttachment && m_CargoHeader)
        {
            Widget attachmentHeader = m_CargoHeader.FindAnyWidget("grid_container_header");
            if (attachmentHeader)
                m_TransferZAttachmentHeaderControls = new TransferZHeaderControls(attachmentHeader, this);
        }
    }

    override void SetEntity(EntityAI item, int cargo_index = 0, bool immedUpdate = true)
    {
        super.SetEntity(item, cargo_index, immedUpdate);

        if (m_TransferZAttachmentHeaderControls)
            m_TransferZAttachmentHeaderControls.SetEntity(item);
    }

    override void UpdateInterval()
    {
        super.UpdateInterval();

        if (m_TransferZAttachmentHeaderControls)
            m_TransferZAttachmentHeaderControls.UpdateControls();
    }
}
