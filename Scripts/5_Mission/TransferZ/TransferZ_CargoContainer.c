modded class CargoContainer
{
    protected ButtonWidget m_TransferZDestinationButton;
    protected ButtonWidget m_TransferZTransferButton;
    protected ButtonWidget m_TransferZUnpackButton;
    protected ButtonWidget m_TransferZLinkButton;
    protected ButtonWidget m_TransferZPreferredButton;

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

    protected ButtonWidget TransferZ_CreateButton(Widget parent, string name, string text, float x)
    {
        Widget raw = GetGame().GetWorkspace().CreateWidget(ButtonWidgetTypeID, 0, 0, 1, 1, WidgetFlags.VISIBLE | WidgetFlags.SOURCEALPHA | WidgetFlags.BLEND, ARGB(220, 35, 35, 35), 1000, parent);
        ButtonWidget button = ButtonWidget.Cast(raw);
        if (!button)
            return null;

        button.SetName(name);
        button.SetPos(x, 0.08);
        button.SetSize(0.05, 0.84);
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
            return;

        m_TransferZDestinationButton = TransferZ_CreateButton(headerWidget, "TransferZ_Destination", "D", 0.650);
        m_TransferZTransferButton = TransferZ_CreateButton(headerWidget, "TransferZ_Transfer", "T", 0.705);
        m_TransferZUnpackButton = TransferZ_CreateButton(headerWidget, "TransferZ_Unpack", "U", 0.760);
        m_TransferZLinkButton = TransferZ_CreateButton(headerWidget, "TransferZ_Link", "L", 0.815);
        m_TransferZPreferredButton = TransferZ_CreateButton(headerWidget, "TransferZ_Preferred", "P", 0.870);

        TransferZ_RegisterButton(m_TransferZDestinationButton, "TransferZ_OnDestination");
        TransferZ_RegisterButton(m_TransferZTransferButton, "TransferZ_OnTransfer");
        TransferZ_RegisterButton(m_TransferZUnpackButton, "TransferZ_OnUnpack");
        TransferZ_RegisterButton(m_TransferZLinkButton, "TransferZ_OnLink");
        TransferZ_RegisterButton(m_TransferZPreferredButton, "TransferZ_OnPreferred");
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
