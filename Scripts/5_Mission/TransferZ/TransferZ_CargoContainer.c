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

    protected ButtonWidget TransferZ_CreateButton(Widget parent, string name, string text, float x)
    {
        Widget raw = GetGame().GetWorkspace().CreateWidget(ButtonWidgetTypeID, 0, 0, 1, 1, WidgetFlags.VISIBLE, ARGB(210, 35, 35, 35), 1000, parent);
        ButtonWidget button = ButtonWidget.Cast(raw);
        if (!button)
            return null;

        button.SetName(name);
        button.SetPos(x, 0.08);
        button.SetSize(0.07, 0.84);
        button.SetText(text);
        button.SetTextColor(ARGB(255, 230, 230, 230));
        return button;
    }

    protected void TransferZ_InitControls()
    {
        if (m_TransferZDestinationButton)
            return;

        Container parentContainer = Container.Cast(GetParent());
        if (!parentContainer)
            return;

        Header header = parentContainer.GetHeader();
        if (!header)
            return;

        Widget headerWidget = header.GetMainWidget();
        if (!headerWidget)
            return;

        m_TransferZDestinationButton = TransferZ_CreateButton(headerWidget, "TransferZ_Destination", "D", 0.625);
        m_TransferZTransferButton = TransferZ_CreateButton(headerWidget, "TransferZ_Transfer", "T", 0.700);
        m_TransferZUnpackButton = TransferZ_CreateButton(headerWidget, "TransferZ_Unpack", "U", 0.775);
        m_TransferZLinkButton = TransferZ_CreateButton(headerWidget, "TransferZ_Link", "L", 0.850);
        m_TransferZPreferredButton = TransferZ_CreateButton(headerWidget, "TransferZ_Preferred", "P", 0.925);

        WidgetEventHandler.GetInstance().RegisterOnClick(m_TransferZDestinationButton, this, "TransferZ_OnDestination");
        WidgetEventHandler.GetInstance().RegisterOnClick(m_TransferZTransferButton, this, "TransferZ_OnTransfer");
        WidgetEventHandler.GetInstance().RegisterOnClick(m_TransferZUnpackButton, this, "TransferZ_OnUnpack");
        WidgetEventHandler.GetInstance().RegisterOnClick(m_TransferZLinkButton, this, "TransferZ_OnLink");
        WidgetEventHandler.GetInstance().RegisterOnClick(m_TransferZPreferredButton, this, "TransferZ_OnPreferred");
    }

    protected bool TransferZ_CanBePreferred()
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || !m_Entity)
            return false;

        InventoryLocation location = new InventoryLocation();
        if (!m_Entity.GetInventory().GetCurrentInventoryLocation(location))
            return false;

        return location.GetType() == InventoryLocationType.ATTACHMENT && location.GetParent() == player;
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

        if (state.IsLinked(m_Entity))
            m_TransferZLinkButton.SetText("L*");
        else if (state.IsLinkAnchor(m_Entity))
            m_TransferZLinkButton.SetText("L+");
        else
            m_TransferZLinkButton.SetText("L");

        bool canPrefer = TransferZ_CanBePreferred();
        m_TransferZPreferredButton.Show(canPrefer);
        if (canPrefer && state.IsPreferred(m_Entity))
            m_TransferZPreferredButton.SetText("P*");
        else
            m_TransferZPreferredButton.SetText("P");
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
