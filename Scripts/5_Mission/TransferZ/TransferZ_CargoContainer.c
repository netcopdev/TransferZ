class TransferZHeaderControls
{
    protected EntityAI m_Entity;
    protected Widget m_Root;
    protected Widget m_HeaderLabel;
    protected float m_HeaderLabelX;
    protected float m_HeaderLabelY;
    protected float m_HeaderLabelW;
    protected float m_HeaderLabelH;
    protected ButtonWidget m_DestinationButton;
    protected ButtonWidget m_TransferButton;
    protected ButtonWidget m_UnpackButton;
    protected ButtonWidget m_LinkButton;
    protected ButtonWidget m_PreferredButton;

    void TransferZHeaderControls(Widget parent)
    {
        if (!parent)
            return;

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
            RestoreHeaderText();
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

modded class HandsHeader
{
    void HandsHeader(LayoutHolder parent, string function_name)
    {
        m_TransferZHeaderControls = new TransferZHeaderControls(m_ItemHeader);
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
            return;
        }

        m_TransferZHeaderControls.SetEntity(player.GetEntityInHands());
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
