// Managed: instances are CallLater targets and WidgetEventHandler handlers,
// and the header that owns them can be destroyed while a call is queued
// (for example a vicinity container leaving range). Managed references are
// nulled when the object is deleted instead of dangling.
class TransferZHeaderControls : Managed
{
    protected static ref array<TransferZHeaderControls> s_Instances;

    protected EntityAI m_Entity;
    protected int m_CargoIndex = 0;
    protected Container m_OwnerContainer;
    protected Widget m_HeaderHost;
    protected Widget m_DropHost;
    protected Widget m_Root;
    protected Widget m_ManageRoot;
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
    protected ButtonWidget m_SortButton;
    protected ButtonWidget m_StackButton;

    protected ImageWidget m_TransferStatus;
    protected ImageWidget m_UnpackStatus;
    protected Widget m_HoveredOperation;
    protected Widget m_HoveredTooltipButton;
    protected int m_IgnoreOperationClickUntil;
    protected int m_LastFullRefreshTime;
    protected static const int INTERVAL_REFRESH_MS = 250;

    protected ImageWidget m_DestinationState;
    protected ImageWidget m_LinkState;
    protected ImageWidget m_PreferredState;

    protected EntityAI m_PlacementEntity;
    protected int m_PlacementCargoIndex = -1;
    protected bool m_InitialPlacementQueued;

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
        m_ManageRoot = GetGame().GetWorkspace().CreateWidgets("TransferZ/GUI/layouts/transferz_header_manage_controls.layout", parent);
        if (!m_Root || !m_ManageRoot)
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
                PrepareSolidImage(m_TooltipBackground, ARGB(232, 0, 0, 0), true);
            }
            m_TooltipRoot.SetSort(10000);
            m_TooltipRoot.Show(false);
        }

        m_DropTarget = GetGame().GetWorkspace().CreateWidgets("TransferZ/GUI/layouts/transferz_header_drop_target.layout");
        if (m_DropTarget)
        {
            m_DropTarget.SetSort(9990);
            WidgetEventHandler dropHandler = WidgetEventHandler.GetInstance();
            dropHandler.RegisterOnDropReceived(m_DropTarget, this, "OnOperationDropReceived");
            dropHandler.RegisterOnDraggingOver(m_DropTarget, this, "OnOperationDraggingOver");
            dropHandler.RegisterOnMouseWheel(m_DropTarget, this, "OnOperationMouseWheel");
            m_DropTarget.Show(false);
        }

        m_DestinationButton = ButtonWidget.Cast(m_Root.FindAnyWidget("TransferZ_Destination"));
        m_TransferButton = ButtonWidget.Cast(m_Root.FindAnyWidget("TransferZ_Transfer"));
        m_UnpackButton = ButtonWidget.Cast(m_Root.FindAnyWidget("TransferZ_Unpack"));
        m_LinkButton = ButtonWidget.Cast(m_Root.FindAnyWidget("TransferZ_Link"));
        m_PreferredButton = ButtonWidget.Cast(m_Root.FindAnyWidget("TransferZ_Preferred"));
        m_SortButton = ButtonWidget.Cast(m_ManageRoot.FindAnyWidget("TransferZ_Sort"));
        m_StackButton = ButtonWidget.Cast(m_ManageRoot.FindAnyWidget("TransferZ_Stack"));

        m_TransferStatus = ImageWidget.Cast(m_Root.FindAnyWidget("TransferZ_TransferStatus"));
        m_UnpackStatus = ImageWidget.Cast(m_Root.FindAnyWidget("TransferZ_UnpackStatus"));
        m_DestinationState = ImageWidget.Cast(m_Root.FindAnyWidget("TransferZ_DestinationState"));
        m_LinkState = ImageWidget.Cast(m_Root.FindAnyWidget("TransferZ_LinkState"));
        m_PreferredState = ImageWidget.Cast(m_Root.FindAnyWidget("TransferZ_PreferredState"));

        PrepareSolidImage(m_DestinationState, ARGB(115, 48, 122, 62), false);
        PrepareSolidImage(m_LinkState, ARGB(115, 48, 122, 62), false);
        PrepareSolidImage(m_PreferredState, ARGB(115, 48, 122, 62), false);
        PrepareStatusImage(m_TransferStatus);
        PrepareStatusImage(m_UnpackStatus);

        RegisterButton(m_DestinationButton, "OnDestination");
        RegisterOperationButton(m_TransferButton, "OnTransfer");
        RegisterOperationButton(m_UnpackButton, "OnUnpack");
        RegisterButton(m_LinkButton, "OnLink");
        RegisterButton(m_PreferredButton, "OnPreferred");
        RegisterButton(m_SortButton, "OnSort");
        RegisterButton(m_StackButton, "OnStack");

        m_Root.Show(false);
        m_ManageRoot.Show(false);
    }

    void ~TransferZHeaderControls()
    {
        if (GetGame())
        {
            ScriptCallQueue guiQueue = GetGame().GetCallQueue(CALL_CATEGORY_GUI);
            if (guiQueue)
            {
                guiQueue.Remove(DeferredPlacementPassOne);
                guiQueue.Remove(DeferredPlacementPassTwo);
            }
        }

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

    protected void PrepareSolidImage(ImageWidget image, int color, bool show)
    {
        if (!image)
            return;

        image.LoadImageFile(0, "#(argb,8,8,3)color(1,1,1,1,ca)");
        image.SetImage(0);
        image.SetColor(color);
        image.Show(show);
    }

    protected void PrepareStatusImage(ImageWidget image)
    {
        if (!image)
            return;

        PrepareSolidImage(image, ARGB(0, 0, 0, 0), false);
    }

    static void OnInventoryClosed()
    {
        if (TransferZOperationDrag.IsActive())
        {
            Widget nativeDrag = GetDragWidget();
            if (nativeDrag)
                CancelWidgetDragging();

            TransferZOperationDrag.CancelForUiTeardown();
            SetOperationDropTargetsVisible(false);
        }

        if (s_Instances)
        {
            for (int i = s_Instances.Count() - 1; i >= 0; i--)
            {
                TransferZHeaderControls controls = s_Instances.Get(i);
                if (!controls)
                    continue;

                controls.HideTooltip();
                controls.HideOperationStatus();
                if (controls.m_DropTarget)
                    controls.m_DropTarget.Show(false);
            }
        }

        TransferZVicinityHeaderControls.OnInventoryClosed();
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

    protected static bool WidgetIsWithin(Widget widget, Widget root)
    {
        Widget current = widget;
        while (current)
        {
            if (current == root)
                return true;
            current = current.GetParent();
        }
        return false;
    }

    protected static bool TransferZPointInsideClippedWidget(Widget widget, ScrollWidget scroll, int mouseX, int mouseY)
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

    static bool CompleteModifierDragAtMousePosition()
    {
        if (!TransferZOperationDrag.IsActive())
            return false;

        int mouseX;
        int mouseY;
        GetMousePos(mouseX, mouseY);

        EntityAI source = TransferZOperationDrag.GetSource();

        Widget hovered = GetWidgetUnderCursor();

        // Prefer the live drop-target widget beneath the cursor, but never trust
        // widget ancestry alone after scrolling. Mouse capture can leave
        // GetWidgetUnderCursor() pointing at the previously captured overlay.
        // The current mouse point must also lie inside that container's live,
        // clipped drop-host rectangle before the hovered widget may win.
        if (hovered && s_Instances)
        {
            for (int hoverIndex = s_Instances.Count() - 1; hoverIndex >= 0; hoverIndex--)
            {
                TransferZHeaderControls hoveredControls = s_Instances.Get(hoverIndex);
                if (!hoveredControls || !hoveredControls.m_Entity || !hoveredControls.m_DropTarget || !hoveredControls.m_DropTarget.IsVisibleHierarchy() || !hoveredControls.IsOpenTarget())
                    continue;
                if (!TransferZCargo.Exists(hoveredControls.m_Entity, hoveredControls.m_CargoIndex))
                    continue;

                if (source && source == hoveredControls.m_Entity && TransferZOperationDrag.GetSourceCargoIndex() == hoveredControls.m_CargoIndex)
                {
                    bool hoveredSelfUnpack = TransferZOperationDrag.GetOperation() == TransferZOperation.UNPACK && !TransferZOperationDrag.IsClassTransfer() && !TransferZOperationDrag.IsFromVicinity();
                    if (!hoveredSelfUnpack)
                        continue;
                }

                if (!WidgetIsWithin(hovered, hoveredControls.m_DropTarget))
                    continue;

                ScrollWidget hoveredScroll = hoveredControls.FindScrollWidget();
                if (!TransferZPointInsideClippedWidget(hoveredControls.m_DropHost, hoveredScroll, mouseX, mouseY))
                {
                    continue;
                }
                bool hoveredHandled = TransferZOperationDrag.Complete(hoveredControls.m_Entity, hoveredControls.m_CargoIndex);
                SetOperationDropTargetsVisible(false);
                RefreshAll();
                return hoveredHandled;
            }
        }

        TransferZHeaderControls best;
        float bestArea = 999999999.0;

        // Mouse capture can occasionally prevent GetWidgetUnderCursor() from
        // exposing the overlay. Fall back to live container screen geometry.
        if (s_Instances)
        {
            for (int i = s_Instances.Count() - 1; i >= 0; i--)
            {
                TransferZHeaderControls controls = s_Instances.Get(i);
                if (!controls || !controls.m_Entity || !controls.m_DropHost || !controls.IsOpenTarget())
                    continue;
                if (!TransferZCargo.Exists(controls.m_Entity, controls.m_CargoIndex))
                    continue;

                if (source && source == controls.m_Entity && TransferZOperationDrag.GetSourceCargoIndex() == controls.m_CargoIndex)
                {
                    bool selfUnpack = TransferZOperationDrag.GetOperation() == TransferZOperation.UNPACK && !TransferZOperationDrag.IsClassTransfer() && !TransferZOperationDrag.IsFromVicinity();
                    if (!selfUnpack)
                        continue;
                }

                float x;
                float y;
                float w;
                float h;
                controls.m_DropHost.GetScreenPos(x, y);
                controls.m_DropHost.GetScreenSize(w, h);

                ScrollWidget scroll = controls.FindScrollWidget();
                if (!TransferZPointInsideClippedWidget(controls.m_DropHost, scroll, mouseX, mouseY))
                    continue;

                float area = w * h;
                if (!best || area < bestArea)
                {
                    best = controls;
                    bestArea = area;
                }
            }
        }

        if (best)
        {
            bool handled = TransferZOperationDrag.Complete(best.m_Entity, best.m_CargoIndex);
            SetOperationDropTargetsVisible(false);
            RefreshAll();
            return handled;
        }

        bool vicinityHandled = TransferZVicinityHeaderControls.CompleteModifierDragAtMousePosition(mouseX, mouseY);
        return vicinityHandled;
    }


    static bool ExecuteInputCommandAtMousePosition(int command)
    {
        if (command == TransferZInputCommand.NONE || TransferZOperationDrag.IsActive())
            return false;

        int mouseX;
        int mouseY;
        GetMousePos(mouseX, mouseY);

        TransferZHeaderControls best;
        float bestArea = 999999999.0;

        if (s_Instances)
        {
            for (int i = s_Instances.Count() - 1; i >= 0; i--)
            {
                TransferZHeaderControls controls = s_Instances.Get(i);
                if (!controls || !controls.m_Entity || !controls.m_DropHost || !controls.IsOpenTarget())
                    continue;
                if (!TransferZCargo.Exists(controls.m_Entity, controls.m_CargoIndex))
                    continue;

                ScrollWidget scroll = controls.FindScrollWidget();
                if (!TransferZPointInsideClippedWidget(controls.m_DropHost, scroll, mouseX, mouseY))
                    continue;

                float x;
                float y;
                float w;
                float h;
                controls.m_DropHost.GetScreenPos(x, y);
                controls.m_DropHost.GetScreenSize(w, h);
                float area = w * h;
                if (!best || area < bestArea)
                {
                    best = controls;
                    bestArea = area;
                }
            }
        }

        if (best && best.ExecuteInputCommand(command))
            return true;

        return TransferZVicinityHeaderControls.ExecuteInputCommandAtMousePosition(mouseX, mouseY, command);
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

    void SetEntity(EntityAI entity, int cargoIndex = 0)
    {
        m_Entity = entity;
        m_CargoIndex = cargoIndex;
        bool show = m_Entity && TransferZCargo.Exists(m_Entity, m_CargoIndex);

        if (m_Root)
            m_Root.Show(show);
        if (m_ManageRoot)
            m_ManageRoot.Show(show);

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
        if (!player || !m_Entity || !TransferZCargo.Exists(m_Entity, m_CargoIndex))
            return false;

        EntityAI current = m_Entity;
        int depth = 0;
        while (current && current != player && depth < 16)
        {
            InventoryLocation location = new InventoryLocation();
            if (!current.GetInventory().GetCurrentInventoryLocation(location) || location.GetType() != InventoryLocationType.ATTACHMENT)
                return false;

            current = location.GetParent();
            if (!current)
                return false;
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
            if (state.IsDestination(m_Entity, m_CargoIndex))
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
                return "Unpack nested contents -> " + destinationName;
            return "Unpack nested contents: select destination";
        }
        if (w == m_LinkButton)
        {
            EntityAI linked = state.GetLinkedDestination(m_Entity, m_CargoIndex);
            if (linked)
                return "Linked -> " + DisplayName(linked);
            if (state.IsLinkAnchor(m_Entity, m_CargoIndex))
                return "Link anchor: choose another container";

            if (s_Instances)
            {
                for (int i = s_Instances.Count() - 1; i >= 0; i--)
                {
                    TransferZHeaderControls controls = s_Instances.Get(i);
                    if (controls && controls.m_Entity && (controls.m_Entity != m_Entity || controls.m_CargoIndex != m_CargoIndex) && state.IsLinkAnchor(controls.m_Entity, controls.m_CargoIndex))
                        return "Link to " + DisplayName(controls.m_Entity);
                }
            }
            return "Link this container";
        }
        if (w == m_PreferredButton)
        {
            if (state.IsPreferred(m_Entity, m_CargoIndex))
                return "Preferred pickup: " + DisplayName(m_Entity);
            return "Set preferred pickup: " + DisplayName(m_Entity);
        }
        if (w == m_SortButton)
            return "Sort and compact direct cargo by size and type";
        if (w == m_StackButton)
            return "Merge compatible partial stacks in this container";
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
                line += " " + word;
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

        m_TooltipRoot.SetScreenSize(tooltipW, tooltipH, true);
        if (m_TooltipBackground)
            m_TooltipBackground.SetSize(tooltipW, tooltipH, true);
        if (m_TooltipText)
            m_TooltipText.SetSize(tooltipW - 16.0, tooltipH - 10.0, true);
        m_TooltipRoot.SetScreenPos(tooltipX, tooltipY, true);
        m_TooltipRoot.Update();
    }

    protected void ShowTooltip(Widget source)
    {
        m_HoveredTooltipButton = source;
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
        m_HoveredTooltipButton = null;
        if (m_TooltipRoot)
            m_TooltipRoot.Show(false);
    }

    protected int StatusColor(int result)
    {
        if (result == TransferZOperationPreviewResult.READY)
            return ARGB(165, 48, 122, 62);
        if (result == TransferZOperationPreviewResult.PARTIAL)
            return ARGB(170, 154, 118, 34);
        return ARGB(170, 142, 46, 43);
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
            return;

        if (!status || !m_Entity)
            return;

        int result = TransferZOperationPreview.EvaluateContainerOperation(operation, m_Entity, m_CargoIndex);
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

    protected float WidgetRight(Widget widget)
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

    protected float NativeLeftReservedRight()
    {
        if (!m_HeaderHost)
            return 0.0;

        float right = 0.0;
        float candidate = WidgetRight(m_HeaderHost.FindAnyWidget("LeftSpacer"));
        if (candidate > right)
            right = candidate;
        candidate = WidgetRight(m_HeaderHost.FindAnyWidget("Render"));
        if (candidate > right)
            right = candidate;
        candidate = WidgetRight(m_HeaderHost.FindAnyWidget("MovePanel"));
        if (candidate > right)
            right = candidate;
        return right;
    }

    protected float NativeRightReservedLeft(float fallbackRight)
    {
        if (!m_HeaderHost)
            return fallbackRight;

        float left = fallbackRight;
        Widget candidateWidget = m_HeaderHost.FindAnyWidget("RightSpacer");
        if (candidateWidget)
        {
            float x;
            float y;
            candidateWidget.GetScreenPos(x, y);
            if (x > 0.0 && x < left)
                left = x;
        }

        candidateWidget = m_HeaderHost.FindAnyWidget("FrameWidget1");
        if (candidateWidget)
        {
            float fx;
            float fy;
            candidateWidget.GetScreenPos(fx, fy);
            if (fx > 0.0 && fx < left)
                left = fx;
        }
        return left;
    }

    protected void PlaceControls()
    {
        if (!m_Root || !m_ManageRoot || !m_HeaderHost)
            return;

        RestoreHeaderText();

        float hostX;
        float hostY;
        float hostW;
        float hostH;
        m_HeaderHost.GetScreenPos(hostX, hostY);
        m_HeaderHost.GetScreenSize(hostW, hostH);

        float labelX = hostX;
        float labelY = hostY;
        float labelH = hostH;
        if (m_HeaderLabel)
        {
            float labelW;
            m_HeaderLabel.GetScreenPos(labelX, labelY);
            m_HeaderLabel.GetScreenSize(labelW, labelH);
        }

        bool preferredVisible = CanBePreferred();
        float leftWidth = 86.0;
        if (preferredVisible)
            leftWidth = 107.0;

        float leftX = labelX;
        float nativeLeftRight = NativeLeftReservedRight();
        if (nativeLeftRight > 0.0 && nativeLeftRight + 4.0 > leftX)
            leftX = nativeLeftRight + 4.0;

        float blockY = labelY + (labelH - 29.0) * 0.5;
        m_Root.SetScreenPos(leftX, blockY, false);
        m_Root.SetScreenSize(leftWidth, 29.0, false);

        float nativeRightLeft = NativeRightReservedLeft(hostX + hostW);
        float rightX = nativeRightLeft - 48.0;
        if (rightX < hostX)
            rightX = hostX;
        m_ManageRoot.SetScreenPos(rightX, blockY, false);
        m_ManageRoot.SetScreenSize(44.0, 29.0, false);
    }

    protected void ForcePlacementAfterLayout()
    {
        if (!m_Root || !m_ManageRoot || !m_Entity || !TransferZCargo.Exists(m_Entity, m_CargoIndex))
            return;

        RestoreHeaderText();
        if (m_HeaderHost)
            m_HeaderHost.Update();
        if (m_HeaderLabel)
            m_HeaderLabel.Update();
        m_Root.Update();
        m_ManageRoot.Update();
        PlaceControls();
    }

    protected void DeferredPlacementPassOne()
    {
        ForcePlacementAfterLayout();
    }

    protected void DeferredPlacementPassTwo()
    {
        ForcePlacementAfterLayout();
        m_InitialPlacementQueued = false;
    }

    protected void QueueInitialPlacement()
    {
        if (m_InitialPlacementQueued)
            return;

        m_InitialPlacementQueued = true;
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(DeferredPlacementPassOne, 0, false);
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(DeferredPlacementPassTwo, 60, false);
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

    protected void SetDropTargetVisible(bool show, EntityAI source)
    {
        if (!m_DropTarget)
            return;

        bool canShow = show && TransferZOperationDrag.IsActive() && m_Entity && TransferZCargo.Exists(m_Entity, m_CargoIndex) && IsOpenTarget();
        if (canShow && source && source == m_Entity && TransferZOperationDrag.GetSourceCargoIndex() == m_CargoIndex)
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
            TransferZOperationDrag.BeginContainer(TransferZOperation.TRANSFER, m_Entity, m_CargoIndex);
        else if (w == m_UnpackButton)
            TransferZOperationDrag.BeginContainer(TransferZOperation.UNPACK, m_Entity, m_CargoIndex);
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

        // Scrolling can cause DayZ to emit a registered drop while LMB is still
        // physically held. Modifier batches never commit from that synthetic
        // event; actual mouse-up is authoritative.
        if ((GetMouseState(MouseState.LEFT) & MB_PRESSED_MASK) != 0)
        {
            SetOperationDropTargetsVisible(true);
            return;
        }

        if (TransferZOperationDrag.IsModifierItemDrag())
        {
            CompleteModifierDragAtMousePosition();
            return;
        }

        TransferZOperationDrag.Complete(m_Entity, m_CargoIndex);
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

    // Vanilla interval paths can run every frame while the inventory is open.
    // Event-driven callers still use UpdateControls() directly and remain
    // immediate; interval callers are throttled separately.
    void UpdateControlsFromInterval()
    {
        int now = GetGame().GetTime();
        if (m_LastFullRefreshTime > 0 && now >= m_LastFullRefreshTime && now - m_LastFullRefreshTime < INTERVAL_REFRESH_MS)
            return;

        UpdateControls();
    }

    void UpdateControls()
    {
        m_LastFullRefreshTime = GetGame().GetTime();

        if (!m_Root || !m_ManageRoot || !m_Entity || !TransferZCargo.Exists(m_Entity, m_CargoIndex))
            return;

        TransferZClientState state = TransferZClientState.Get();
        bool destinationActive = state.IsDestination(m_Entity, m_CargoIndex);
        bool linked = state.IsLinked(m_Entity, m_CargoIndex);
        bool linkAnchor = !linked && state.IsLinkAnchor(m_Entity, m_CargoIndex);
        bool canPrefer = CanBePreferred();
        bool preferred = canPrefer && state.IsPreferred(m_Entity, m_CargoIndex);

        if (m_DestinationState)
            m_DestinationState.Show(destinationActive);
        if (m_LinkState)
        {
            if (linkAnchor)
                m_LinkState.SetColor(ARGB(125, 154, 118, 34));
            else
                m_LinkState.SetColor(ARGB(115, 48, 122, 62));
            m_LinkState.Show(linked || linkAnchor);
        }
        if (m_PreferredButton)
            m_PreferredButton.Show(canPrefer);
        if (m_PreferredState)
            m_PreferredState.Show(preferred);

        PlaceControls();
        RefreshOperationStatus();

        if (m_HoveredTooltipButton)
            ShowTooltip(m_HoveredTooltipButton);

        if (m_PlacementEntity != m_Entity || m_PlacementCargoIndex != m_CargoIndex)
        {
            m_PlacementEntity = m_Entity;
            m_PlacementCargoIndex = m_CargoIndex;
            m_InitialPlacementQueued = false;
            QueueInitialPlacement();
        }

        if (TransferZOperationDrag.IsActive())
            SetDropTargetVisible(true, TransferZOperationDrag.GetSource());
        else
            SetDropTargetVisible(false, null);
    }

    void OnDestination(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT || !m_Entity)
            return;

        TransferZClientState.Get().ToggleDestinationSelection(m_Entity, m_CargoIndex);
        RefreshAll();
        ShowTooltip(w);
    }

    void OnTransfer(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT || !m_Entity || GetGame().GetTime() < m_IgnoreOperationClickUntil)
            return;

        TransferZClientState.Get().RequestTransfer(m_Entity, m_CargoIndex);
        ShowTooltip(w);
        RefreshOperationStatus();
    }

    void OnUnpack(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT || !m_Entity || GetGame().GetTime() < m_IgnoreOperationClickUntil)
            return;

        TransferZClientState.Get().RequestNestedUnpack(m_Entity, m_CargoIndex);
        ShowTooltip(w);
        RefreshOperationStatus();
    }

    void OnLink(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT || !m_Entity)
            return;

        TransferZClientState.Get().ToggleLink(m_Entity, m_CargoIndex);
        RefreshAll();
        ShowTooltip(w);
    }

    void OnPreferred(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT || !m_Entity)
            return;

        TransferZClientState.Get().TogglePreferred(m_Entity, m_CargoIndex);
        RefreshAll();
        ShowTooltip(w);
    }

    void OnSort(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT || !m_Entity)
            return;

        TransferZMaintenanceClient.RequestSort(m_Entity, m_CargoIndex);
        ShowTooltip(w);
    }

    void OnStack(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT || !m_Entity)
            return;

        TransferZMaintenanceClient.RequestStack(m_Entity, m_CargoIndex);
        ShowTooltip(w);
    }

    protected bool ExecuteInputCommand(int command)
    {
        if (!m_Entity || !IsOpenTarget() || !TransferZCargo.Exists(m_Entity, m_CargoIndex))
            return false;

        TransferZClientState state = TransferZClientState.Get();
        if (command == TransferZInputCommand.DESTINATION)
        {
            state.ToggleDestinationSelection(m_Entity, m_CargoIndex);
            RefreshAll();
            return true;
        }
        if (command == TransferZInputCommand.TRANSFER)
        {
            state.RequestTransfer(m_Entity, m_CargoIndex);
            RefreshOperationStatus();
            return true;
        }
        if (command == TransferZInputCommand.UNPACK)
        {
            state.RequestNestedUnpack(m_Entity, m_CargoIndex);
            RefreshOperationStatus();
            return true;
        }
        if (command == TransferZInputCommand.LINK)
        {
            state.ToggleLink(m_Entity, m_CargoIndex);
            RefreshAll();
            return true;
        }
        if (command == TransferZInputCommand.PREFERRED)
        {
            if (!CanBePreferred())
                return false;
            state.TogglePreferred(m_Entity, m_CargoIndex);
            RefreshAll();
            return true;
        }
        if (command == TransferZInputCommand.SORT)
        {
            TransferZMaintenanceClient.RequestSort(m_Entity, m_CargoIndex);
            return true;
        }
        if (command == TransferZInputCommand.STACK)
        {
            TransferZMaintenanceClient.RequestStack(m_Entity, m_CargoIndex);
            return true;
        }
        return false;
    }

    protected bool MouseInsideWidget(Widget widget, int mouseX, int mouseY)
    {
        if (!widget || !widget.IsVisibleHierarchy())
            return false;
        float x;
        float y;
        float w;
        float h;
        widget.GetScreenPos(x, y);
        widget.GetScreenSize(w, h);
        return mouseX >= x && mouseX <= x + w && mouseY >= y && mouseY <= y + h;
    }

    bool TransferZMouseInsideControls()
    {
        int mouseX;
        int mouseY;
        GetMousePos(mouseX, mouseY);
        return MouseInsideWidget(m_Root, mouseX, mouseY) || MouseInsideWidget(m_ManageRoot, mouseX, mouseY);
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

    override void OnDragHeader(Widget w, int x, int y)
    {
        if (m_TransferZHeaderControls && m_TransferZHeaderControls.TransferZMouseInsideControls())
            return;
        super.OnDragHeader(w, x, y);
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

        bool handsChanged = currentHands != m_TransferZLastHandsEntity;
        m_TransferZLastHandsEntity = currentHands;

        if (handsChanged)
            m_TransferZHeaderControls.SetEntity(currentHands);
        else if (currentHands && currentHands.GetInventory().GetCargo())
            m_TransferZHeaderControls.UpdateControlsFromInterval();
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
            m_TransferZAttachmentHeaderControls.SetEntity(item, cargo_index);
    }

    override void UpdateInterval()
    {
        super.UpdateInterval();
        if (m_TransferZAttachmentHeaderControls)
            m_TransferZAttachmentHeaderControls.UpdateControlsFromInterval();
    }
}
