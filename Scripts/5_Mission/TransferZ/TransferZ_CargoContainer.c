class TransferZHeaderControls
{
    protected EntityAI m_Entity;
    protected Widget m_Root;
    protected ButtonWidget m_DestinationButton;
    protected ButtonWidget m_TransferButton;
    protected ButtonWidget m_UnpackButton;
    protected ButtonWidget m_LinkButton;
    protected ButtonWidget m_PreferredButton;

    void TransferZHeaderControls(Widget parent)
    {
        if (!parent)
            return;

        m_Root = GetGame().GetWorkspace().CreateWidgets("TransferZ/GUI/layouts/transferz_header_controls.layout", parent);
        if (!m_Root)
        {
            Print("[TransferZ] Header controls layout creation failed");
            return;
        }

        m_DestinationButton = ButtonWidget.Cast(m_Root.FindAnyWidget("TransferZ_Destination"));
        m_TransferButton = ButtonWidget.Cast(m_Root.FindAnyWidget("TransferZ_Transfer"));
        m_UnpackButton = ButtonWidget.Cast(m_Root.FindAnyWidget("TransferZ_Unpack"));
        m_LinkButton = ButtonWidget.Cast(m_Root.FindAnyWidget("TransferZ_Link"));
        m_PreferredButton = ButtonWidget.Cast(m_Root.FindAnyWidget("TransferZ_Preferred"));

        WidgetEventHandler.GetInstance().RegisterOnClick(m_DestinationButton, this, "OnDestination");
        WidgetEventHandler.GetInstance().RegisterOnClick(m_TransferButton, this, "OnTransfer");
        WidgetEventHandler.GetInstance().RegisterOnClick(m_UnpackButton, this, "OnUnpack");
        WidgetEventHandler.GetInstance().RegisterOnClick(m_LinkButton, this, "OnLink");
        WidgetEventHandler.GetInstance().RegisterOnClick(m_PreferredButton, this, "OnPreferred");

        m_Root.Show(false);
        Print("[TransferZ] Header controls attached to " + parent.GetName());
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

        if (m_PreferredButton)
        {
            bool canPrefer = CanBePreferred();
            m_PreferredButton.Show(canPrefer);
            if (canPrefer && state.IsPreferred(m_Entity))
                m_PreferredButton.SetText("P*");
            else
                m_PreferredButton.SetText("P");
        }
    }

    void OnDestination(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT || !m_Entity)
            return;

        TransferZClientState.Get().SetDestination(m_Entity);
        UpdateControls();
    }

    void OnTransfer(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT || !m_Entity)
            return;

        TransferZClientState.Get().RequestTransfer(m_Entity);
    }

    void OnUnpack(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT || !m_Entity)
            return;

        TransferZClientState.Get().RequestUnpack(m_Entity);
    }

    void OnLink(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT || !m_Entity)
            return;

        TransferZClientState.Get().ToggleLink(m_Entity);
        UpdateControls();
    }

    void OnPreferred(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT || !m_Entity)
            return;

        TransferZClientState.Get().SetPreferred(m_Entity);
        UpdateControls();
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
        m_TransferZHeaderControls = new TransferZHeaderControls(m_PanelWidget);
    }

    override void UpdateInterval()
    {
        super.UpdateInterval();

        if (m_TransferZHeaderControls)
            m_TransferZHeaderControls.UpdateControls();
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
                m_TransferZAttachmentHeaderControls = new TransferZHeaderControls(attachmentHeader);
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
