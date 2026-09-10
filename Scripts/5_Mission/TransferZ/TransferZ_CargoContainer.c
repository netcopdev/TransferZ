modded class CargoContainer
{
    protected ButtonWidget m_TransferZDestinationButton;
    protected ButtonWidget m_TransferZTransferButton;
    protected ButtonWidget m_TransferZUnpackButton;
    protected ButtonWidget m_TransferZLinkButton;
    protected ButtonWidget m_TransferZPreferredButton;
    protected Widget m_TransferZControlHost;
    protected Widget m_TransferZControlsRoot;
    protected bool m_TransferZLoggedHostFailure;
    protected bool m_TransferZLoggedLayout;

    override void SetEntity(EntityAI item, int cargo_index = 0, bool immedUpdate = true)
    {
        Print("[TransferZ] CargoContainer.SetEntity ENTER");
        super.SetEntity(item, cargo_index, immedUpdate);

        if (item)
            Print("[TransferZ] CargoContainer.SetEntity entity=" + item.GetType());
        else
            Print("[TransferZ] CargoContainer.SetEntity entity=<null>");

        TransferZ_InitControls();
        TransferZ_LayoutControls();
        TransferZ_UpdateControls();
    }

    override void UpdateInterval()
    {
        super.UpdateInterval();

        if (!m_TransferZDestinationButton)
            TransferZ_InitControls();

        TransferZ_LayoutControls();
        TransferZ_UpdateControls();
    }

    protected Widget TransferZ_GetControlHost()
    {
        if (m_IsAttachment && m_CargoHeader)
        {
            Widget attachmentHeader = m_CargoHeader.FindAnyWidget("grid_container_header");
            if (attachmentHeader)
                return attachmentHeader;
            return m_CargoHeader;
        }

        Container parentContainer = Container.Cast(GetParent());
        if (!parentContainer)
            return null;

        Header header = parentContainer.GetHeader();
        if (!header)
            return null;

        Widget headerWidget = header.GetMainWidget();
        if (!headerWidget)
            return null;

        Widget panelWidget = headerWidget.FindAnyWidget("PanelWidget");
        if (panelWidget)
            return panelWidget;

        return headerWidget;
    }

    protected Widget TransferZ_CreateControlsRoot(Widget parent)
    {
        int flags = WidgetFlags.VISIBLE | WidgetFlags.EXACTPOS | WidgetFlags.EXACTSIZE;
        Widget root = GetGame().GetWorkspace().CreateWidget(FrameWidgetTypeID, 0, 0, 132, 22, flags, 0, 1000, parent);
        if (!root)
            return null;

        root.SetName("TransferZ_ControlsRoot");
        root.Show(false);
        return root;
    }

    protected ButtonWidget TransferZ_CreateButton(Widget parent, string name, string text, float x)
    {
        int flags = WidgetFlags.VISIBLE | WidgetFlags.EXACTPOS | WidgetFlags.EXACTSIZE | WidgetFlags.SOURCEALPHA | WidgetFlags.BLEND;
        Widget raw = GetGame().GetWorkspace().CreateWidget(ButtonWidgetTypeID, x, 0, 24, 22, flags, ARGB(220, 35, 35, 35), 1001, parent);
        ButtonWidget button = ButtonWidget.Cast(raw);
        if (!button)
            return null;

        button.SetName(name);
        button.SetText(text);
        button.SetTextColor(ARGB(255, 230, 230, 230));
        button.SetTextProportion(0.7);
        return button;
    }

    protected void TransferZ_RegisterButton(ButtonWidget button, string functionName)
    {
        if (button)
            WidgetEventHandler.GetInstance().RegisterOnClick(button, this, functionName);
    }

    protected void TransferZ_InitControls()
    {
        if (m_TransferZDestinationButton)
            return;

        Widget headerWidget = TransferZ_GetControlHost();
        if (!headerWidget)
        {
            if (!m_TransferZLoggedHostFailure)
            {
                Print("[TransferZ] TransferZ_InitControls: no visible header host");
                m_TransferZLoggedHostFailure = true;
            }
            return;
        }

        m_TransferZControlHost = headerWidget;
        Print("[TransferZ] TransferZ_InitControls: host=" + headerWidget.GetName() + " type=" + headerWidget.GetTypeName());

        m_TransferZControlsRoot = TransferZ_CreateControlsRoot(headerWidget);
        if (!m_TransferZControlsRoot)
        {
            Print("[TransferZ] TransferZ_InitControls: controls root creation failed");
            return;
        }

        m_TransferZDestinationButton = TransferZ_CreateButton(m_TransferZControlsRoot, "TransferZ_Destination", "D", 0);
        m_TransferZTransferButton = TransferZ_CreateButton(m_TransferZControlsRoot, "TransferZ_Transfer", "T", 27);
        m_TransferZUnpackButton = TransferZ_CreateButton(m_TransferZControlsRoot, "TransferZ_Unpack", "U", 54);
        m_TransferZLinkButton = TransferZ_CreateButton(m_TransferZControlsRoot, "TransferZ_Link", "L", 81);
        m_TransferZPreferredButton = TransferZ_CreateButton(m_TransferZControlsRoot, "TransferZ_Preferred", "P", 108);

        TransferZ_RegisterButton(m_TransferZDestinationButton, "TransferZ_OnDestination");
        TransferZ_RegisterButton(m_TransferZTransferButton, "TransferZ_OnTransfer");
        TransferZ_RegisterButton(m_TransferZUnpackButton, "TransferZ_OnUnpack");
        TransferZ_RegisterButton(m_TransferZLinkButton, "TransferZ_OnLink");
        TransferZ_RegisterButton(m_TransferZPreferredButton, "TransferZ_OnPreferred");

        Print("[TransferZ] TransferZ controls created");
    }

    protected void TransferZ_LayoutControls()
    {
        if (!m_TransferZControlHost || !m_TransferZControlsRoot || !m_TransferZDestinationButton)
            return;

        float hostW;
        float hostH;
        float ignoredX;
        float ignoredY;
        m_TransferZControlHost.GetScreenSize(hostW, hostH);
        m_TransferZControlHost.GetScreenPos(ignoredX, ignoredY);

        float controlsW = 132.0;
        float controlsH = 22.0;
        float rightReserve = 32.0;

        if (hostW < controlsW + rightReserve || hostH <= 0)
        {
            m_TransferZControlsRoot.Show(false);
            return;
        }

        float startX = hostW - rightReserve - controlsW;
        float startY = (hostH - controlsH) * 0.5;
        if (startX < 0)
            startX = 0;
        if (startY < 0)
            startY = 0;

        m_TransferZControlsRoot.SetPos(startX, startY, false);
        m_TransferZControlsRoot.SetSize(controlsW, controlsH, true);
        m_TransferZControlsRoot.Show(true);

        if (!m_TransferZLoggedLayout)
        {
            if (m_Entity)
                Print("[TransferZ] TransferZ controls laid out for " + m_Entity.GetType());
            else
                Print("[TransferZ] TransferZ controls laid out");
            m_TransferZLoggedLayout = true;
        }
    }

    protected bool TransferZ_CanBePreferred()
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

    protected void TransferZ_UpdateControls()
    {
        if (!m_TransferZDestinationButton || !m_Entity)
            return;

        TransferZClientState state = TransferZClientState.Get();

        if (state.IsDestination(m_Entity))
            m_TransferZDestinationButton.SetText("D*");
        else
            m_TransferZDestinationButton.SetText("D");

        if (m_TransferZLinkButton)
        {
            if (state.IsLinked(m_Entity))
                m_TransferZLinkButton.SetText("L*");
            else if (state.IsLinkAnchor(m_Entity))
                m_TransferZLinkButton.SetText("L+");
            else
                m_TransferZLinkButton.SetText("L");
        }

        if (m_TransferZPreferredButton)
        {
            bool canPrefer = TransferZ_CanBePreferred();
            m_TransferZPreferredButton.Show(canPrefer);
            if (canPrefer && state.IsPreferred(m_Entity))
                m_TransferZPreferredButton.SetText("P*");
            else
                m_TransferZPreferredButton.SetText("P");
        }
    }

    void TransferZ_OnDestination(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT || !m_Entity)
            return;
        TransferZClientState.Get().SetDestination(m_Entity);
        TransferZ_UpdateControls();
    }

    void TransferZ_OnTransfer(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT || !m_Entity)
            return;
        TransferZClientState.Get().RequestTransfer(m_Entity);
    }

    void TransferZ_OnUnpack(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT || !m_Entity)
            return;
        TransferZClientState.Get().RequestUnpack(m_Entity);
    }

    void TransferZ_OnLink(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT || !m_Entity)
            return;
        TransferZClientState.Get().ToggleLink(m_Entity);
        TransferZ_UpdateControls();
    }

    void TransferZ_OnPreferred(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT || !m_Entity)
            return;
        TransferZClientState.Get().SetPreferred(m_Entity);
        TransferZ_UpdateControls();
    }
}
