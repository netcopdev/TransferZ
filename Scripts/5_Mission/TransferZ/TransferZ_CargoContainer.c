modded class CargoContainer
{
    protected ButtonWidget m_TransferZDestinationButton;
    protected ButtonWidget m_TransferZTransferButton;
    protected ButtonWidget m_TransferZUnpackButton;
    protected ButtonWidget m_TransferZLinkButton;
    protected ButtonWidget m_TransferZPreferredButton;
    protected Widget m_TransferZControlHost;

    override void SetEntity(EntityAI item, int cargo_index = 0, bool immedUpdate = true)
    {
        super.SetEntity(item, cargo_index, immedUpdate);
        TransferZ_InitControls();
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
            return m_CargoHeader;

        Container parentContainer = Container.Cast(GetParent());
        if (!parentContainer)
            return null;

        Header header = parentContainer.GetHeader();
        if (!header)
            return null;

        return header.GetRootWidget();
    }

    protected ButtonWidget TransferZ_CreateButton(Widget parent, string name, string text)
    {
        int flags = WidgetFlags.VISIBLE | WidgetFlags.EXACTPOS | WidgetFlags.EXACTSIZE | WidgetFlags.SOURCEALPHA | WidgetFlags.BLEND;
        Widget raw = GetGame().GetWorkspace().CreateWidget(ButtonWidgetTypeID, 0, 0, 24, 20, flags, ARGB(235, 25, 25, 25), 1000, parent);
        ButtonWidget button = ButtonWidget.Cast(raw);
        if (!button)
            return null;

        button.SetName(name);
        button.SetText(text);
        button.SetTextColor(ARGB(255, 235, 235, 235));
        button.SetTextProportion(0.72);
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

        m_TransferZControlHost = TransferZ_GetControlHost();
        if (!m_TransferZControlHost)
            return;

        m_TransferZDestinationButton = TransferZ_CreateButton(m_TransferZControlHost, "TransferZ_Destination", "D");
        m_TransferZTransferButton = TransferZ_CreateButton(m_TransferZControlHost, "TransferZ_Transfer", "T");
        m_TransferZUnpackButton = TransferZ_CreateButton(m_TransferZControlHost, "TransferZ_Unpack", "U");
        m_TransferZLinkButton = TransferZ_CreateButton(m_TransferZControlHost, "TransferZ_Link", "L");
        m_TransferZPreferredButton = TransferZ_CreateButton(m_TransferZControlHost, "TransferZ_Preferred", "P");

        TransferZ_RegisterButton(m_TransferZDestinationButton, "TransferZ_OnDestination");
        TransferZ_RegisterButton(m_TransferZTransferButton, "TransferZ_OnTransfer");
        TransferZ_RegisterButton(m_TransferZUnpackButton, "TransferZ_OnUnpack");
        TransferZ_RegisterButton(m_TransferZLinkButton, "TransferZ_OnLink");
        TransferZ_RegisterButton(m_TransferZPreferredButton, "TransferZ_OnPreferred");

        TransferZ_LayoutControls();

        if (m_Entity)
            Print("[TransferZ] cargo controls attached: " + m_Entity.GetType() + " host=" + m_TransferZControlHost.GetName());
    }

    protected void TransferZ_LayoutControls()
    {
        if (!m_TransferZControlHost || !m_TransferZDestinationButton)
            return;

        float hostWidth;
        float hostHeight;
        m_TransferZControlHost.GetScreenSize(hostWidth, hostHeight);
        if (hostWidth <= 0 || hostHeight <= 0)
            return;

        float buttonWidth = 24.0;
        float buttonHeight = 20.0;
        float gap = 2.0;
        float rightMargin = 32.0;
        float totalWidth = buttonWidth * 5.0 + gap * 4.0;
        float startX = hostWidth - rightMargin - totalWidth;
        float y = (hostHeight - buttonHeight) * 0.5;

        if (startX < 0)
            startX = 0;
        if (y < 0)
            y = 0;

        m_TransferZDestinationButton.SetPos(startX, y);
        m_TransferZTransferButton.SetPos(startX + buttonWidth + gap, y);
        m_TransferZUnpackButton.SetPos(startX + (buttonWidth + gap) * 2.0, y);
        m_TransferZLinkButton.SetPos(startX + (buttonWidth + gap) * 3.0, y);
        m_TransferZPreferredButton.SetPos(startX + (buttonWidth + gap) * 4.0, y);
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
