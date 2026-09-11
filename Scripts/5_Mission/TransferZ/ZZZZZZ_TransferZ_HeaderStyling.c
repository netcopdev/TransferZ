modded class TransferZHeaderControls
{
    protected ImageWidget m_TransferZBlockBackground;
    protected ImageWidget m_TransferZDestinationHover;
    protected ImageWidget m_TransferZTransferHover;
    protected ImageWidget m_TransferZUnpackHover;
    protected ImageWidget m_TransferZLinkHover;
    protected ImageWidget m_TransferZPreferredHover;
    protected bool m_TransferZStyleReady;

    protected void TransferZPrepareStyleImage(ImageWidget image, int color, bool show)
    {
        if (!image)
            return;

        image.LoadImageFile(0, "#(argb,8,8,3)color(1,1,1,1,ca)");
        image.SetImage(0);
        image.SetColor(color);
        image.Show(show);
    }

    protected void TransferZInitializeHeaderStyle()
    {
        if (m_TransferZStyleReady || !m_Root)
            return;

        m_TransferZBlockBackground = ImageWidget.Cast(m_Root.FindAnyWidget("TransferZ_BlockBackground"));
        m_TransferZDestinationHover = ImageWidget.Cast(m_Root.FindAnyWidget("TransferZ_DestinationHover"));
        m_TransferZTransferHover = ImageWidget.Cast(m_Root.FindAnyWidget("TransferZ_TransferHover"));
        m_TransferZUnpackHover = ImageWidget.Cast(m_Root.FindAnyWidget("TransferZ_UnpackHover"));
        m_TransferZLinkHover = ImageWidget.Cast(m_Root.FindAnyWidget("TransferZ_LinkHover"));
        m_TransferZPreferredHover = ImageWidget.Cast(m_Root.FindAnyWidget("TransferZ_PreferredHover"));

        TransferZPrepareStyleImage(m_TransferZBlockBackground, ARGB(105, 0, 0, 0), true);
        TransferZPrepareStyleImage(m_TransferZDestinationHover, ARGB(55, 255, 255, 255), false);
        TransferZPrepareStyleImage(m_TransferZTransferHover, ARGB(45, 255, 255, 255), false);
        TransferZPrepareStyleImage(m_TransferZUnpackHover, ARGB(45, 255, 255, 255), false);
        TransferZPrepareStyleImage(m_TransferZLinkHover, ARGB(55, 255, 255, 255), false);
        TransferZPrepareStyleImage(m_TransferZPreferredHover, ARGB(55, 255, 255, 255), false);

        m_TransferZStyleReady = true;
    }

    protected ImageWidget TransferZHoverImageFor(Widget w)
    {
        if (w == m_DestinationButton)
            return m_TransferZDestinationHover;
        if (w == m_TransferButton)
            return m_TransferZTransferHover;
        if (w == m_UnpackButton)
            return m_TransferZUnpackHover;
        if (w == m_LinkButton)
            return m_TransferZLinkHover;
        if (w == m_PreferredButton)
            return m_TransferZPreferredHover;
        return null;
    }

    protected void TransferZPlaceControlsBeforeTitle(bool preferredVisible)
    {
        if (!m_Root || !m_HeaderLabel)
            return;

        float blockWidth = 86.0;
        if (preferredVisible)
            blockWidth = 107.0;

        m_Root.SetPos(m_HeaderLabelX, 0, false);
        m_Root.SetSize(blockWidth, 29, true);

        if (m_TransferZBlockBackground)
            m_TransferZBlockBackground.SetSize(blockWidth, 27, false);

        float gap = 5.0;
        float targetWidth = m_HeaderLabelW;
        float hostW;
        float hostH;
        if (m_HeaderHost)
            m_HeaderHost.GetScreenSize(hostW, hostH);

        if (m_HeaderLabelW <= 2.0 && hostW > 0.0)
        {
            targetWidth -= (blockWidth + gap) / hostW;
            if (targetWidth < 0.20)
                targetWidth = 0.20;
        }
        else
        {
            targetWidth -= blockWidth + gap;
            if (targetWidth < 40.0)
                targetWidth = 40.0;
        }

        m_HeaderLabel.SetPos(m_HeaderLabelX + blockWidth + gap, m_HeaderLabelY, false);
        m_HeaderLabel.SetSize(targetWidth, m_HeaderLabelH, true);
    }

    override bool OnButtonMouseEnter(Widget w, int x, int y)
    {
        bool handled = super.OnButtonMouseEnter(w, x, y);
        TransferZInitializeHeaderStyle();

        ImageWidget hover = TransferZHoverImageFor(w);
        if (hover)
            hover.Show(true);

        return handled;
    }

    override bool OnButtonMouseLeave(Widget w, Widget enter_w, int x, int y)
    {
        bool handled = super.OnButtonMouseLeave(w, enter_w, x, y);

        ImageWidget hover = TransferZHoverImageFor(w);
        if (hover)
            hover.Show(false);

        return handled;
    }

    override void UpdateControls()
    {
        super.UpdateControls();
        if (!m_Root || !m_Entity || !m_Entity.GetInventory().GetCargo())
            return;

        TransferZInitializeHeaderStyle();
        bool preferredVisible = CanBePreferred();

        if (m_TransferZPreferredHover && !preferredVisible)
            m_TransferZPreferredHover.Show(false);

        TransferZPlaceControlsBeforeTitle(preferredVisible);
    }
}

modded class TransferZVicinityHeaderControls
{
    protected Widget m_TransferZHeaderLabel;
    protected float m_TransferZHeaderLabelX;
    protected float m_TransferZHeaderLabelY;
    protected float m_TransferZHeaderLabelW;
    protected float m_TransferZHeaderLabelH;
    protected ImageWidget m_TransferZBlockBackground;
    protected ImageWidget m_TransferZDestinationHover;
    protected ImageWidget m_TransferZTransferHover;
    protected ImageWidget m_TransferZUnpackHover;
    protected bool m_TransferZStyleReady;

    protected void TransferZPrepareVicinityStyleImage(ImageWidget image, int color, bool show)
    {
        if (!image)
            return;

        image.LoadImageFile(0, "#(argb,8,8,3)color(1,1,1,1,ca)");
        image.SetImage(0);
        image.SetColor(color);
        image.Show(show);
    }

    protected void TransferZInitializeVicinityStyle()
    {
        if (m_TransferZStyleReady || !m_Root)
            return;

        Widget headerHost = m_Root.GetParent();
        if (headerHost)
        {
            m_TransferZHeaderLabel = headerHost.FindAnyWidget("TextWidget0");
            if (m_TransferZHeaderLabel)
            {
                m_TransferZHeaderLabel.GetPos(m_TransferZHeaderLabelX, m_TransferZHeaderLabelY);
                m_TransferZHeaderLabel.GetSize(m_TransferZHeaderLabelW, m_TransferZHeaderLabelH);
            }
        }

        m_TransferZBlockBackground = ImageWidget.Cast(m_Root.FindAnyWidget("TransferZ_VicinityBlockBackground"));
        m_TransferZDestinationHover = ImageWidget.Cast(m_Root.FindAnyWidget("TransferZ_VicinityDestinationHover"));
        m_TransferZTransferHover = ImageWidget.Cast(m_Root.FindAnyWidget("TransferZ_VicinityTransferHover"));
        m_TransferZUnpackHover = ImageWidget.Cast(m_Root.FindAnyWidget("TransferZ_VicinityUnpackHover"));

        TransferZPrepareVicinityStyleImage(m_TransferZBlockBackground, ARGB(105, 0, 0, 0), true);
        TransferZPrepareVicinityStyleImage(m_TransferZDestinationHover, ARGB(55, 255, 255, 255), false);
        TransferZPrepareVicinityStyleImage(m_TransferZTransferHover, ARGB(45, 255, 255, 255), false);
        TransferZPrepareVicinityStyleImage(m_TransferZUnpackHover, ARGB(45, 255, 255, 255), false);

        m_TransferZStyleReady = true;
    }

    protected ImageWidget TransferZVicinityHoverImageFor(Widget w)
    {
        if (w == m_DestinationButton)
            return m_TransferZDestinationHover;
        if (w == m_TransferButton)
            return m_TransferZTransferHover;
        if (w == m_UnpackButton)
            return m_TransferZUnpackHover;
        return null;
    }

    protected void TransferZPlaceVicinityControlsBeforeTitle()
    {
        if (!m_Root || !m_TransferZHeaderLabel)
            return;

        const float blockWidth = 65.0;
        const float gap = 5.0;
        m_Root.SetPos(m_TransferZHeaderLabelX, 0, false);
        m_Root.SetSize(blockWidth, 29, true);

        if (m_TransferZBlockBackground)
            m_TransferZBlockBackground.SetSize(blockWidth, 27, false);

        float targetWidth = m_TransferZHeaderLabelW;
        float hostW;
        float hostH;
        Widget host = m_Root.GetParent();
        if (host)
            host.GetScreenSize(hostW, hostH);

        if (m_TransferZHeaderLabelW <= 2.0 && hostW > 0.0)
        {
            targetWidth -= (blockWidth + gap) / hostW;
            if (targetWidth < 0.20)
                targetWidth = 0.20;
        }
        else
        {
            targetWidth -= blockWidth + gap;
            if (targetWidth < 40.0)
                targetWidth = 40.0;
        }

        m_TransferZHeaderLabel.SetPos(m_TransferZHeaderLabelX + blockWidth + gap, m_TransferZHeaderLabelY, false);
        m_TransferZHeaderLabel.SetSize(targetWidth, m_TransferZHeaderLabelH, true);
    }

    override bool OnButtonMouseEnter(Widget w, int x, int y)
    {
        bool handled = super.OnButtonMouseEnter(w, x, y);
        TransferZInitializeVicinityStyle();

        ImageWidget hover = TransferZVicinityHoverImageFor(w);
        if (hover)
            hover.Show(true);

        return handled;
    }

    override bool OnButtonMouseLeave(Widget w, Widget enter_w, int x, int y)
    {
        bool handled = super.OnButtonMouseLeave(w, enter_w, x, y);

        ImageWidget hover = TransferZVicinityHoverImageFor(w);
        if (hover)
            hover.Show(false);

        return handled;
    }

    override void UpdateControls()
    {
        super.UpdateControls();
        if (!m_Root)
            return;

        TransferZInitializeVicinityStyle();
        TransferZPlaceVicinityControlsBeforeTitle();
    }
}
