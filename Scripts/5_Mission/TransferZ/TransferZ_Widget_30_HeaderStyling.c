modded class TransferZHeaderControls
{
    protected ImageWidget m_TransferZBlockBackground;
    protected ImageWidget m_TransferZDestinationHover;
    protected ImageWidget m_TransferZTransferHover;
    protected ImageWidget m_TransferZUnpackHover;
    protected ImageWidget m_TransferZLinkHover;
    protected ImageWidget m_TransferZPreferredHover;
    protected bool m_TransferZStyleReady;
    protected bool m_TransferZNativeHeaderGeometryCaptured;

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

    protected bool TransferZCaptureNativeHeaderGeometry()
    {
        if (m_TransferZNativeHeaderGeometryCaptured)
            return true;
        if (!m_HeaderHost || !m_HeaderLabel || !m_HeaderHost.IsVisibleHierarchy())
            return false;

        m_HeaderHost.Update();
        m_HeaderLabel.Update();

        float screenW;
        float screenH;
        m_HeaderLabel.GetScreenSize(screenW, screenH);
        if (screenW <= 20.0 || screenH <= 0.0)
            return false;

        m_HeaderLabel.GetPos(m_HeaderLabelX, m_HeaderLabelY);
        m_HeaderLabel.GetSize(m_HeaderLabelW, m_HeaderLabelH);
        m_TransferZNativeHeaderGeometryCaptured = true;
        return true;
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

    protected float TransferZWidgetRight(Widget widget)
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

    protected bool TransferZUsesNativeMoveControls()
    {
        if (!m_OwnerContainer)
            return false;

        LayoutHolder parent = m_OwnerContainer.GetParent();
        if (!parent)
            return false;

        parent = parent.GetParent();
        return parent && parent.IsInherited(RightArea);
    }

    protected float TransferZNativeLeftReservedRight()
    {
        if (!m_HeaderHost)
            return 0.0;

        float right = 0.0;
        float candidate;

        candidate = TransferZWidgetRight(m_HeaderHost.FindAnyWidget("LeftSpacer"));
        if (candidate > right)
            right = candidate;

        candidate = TransferZWidgetRight(m_HeaderHost.FindAnyWidget("Render"));
        if (candidate > right)
            right = candidate;

        if (TransferZUsesNativeMoveControls())
        {
            float moveRight = 0.0;

            candidate = TransferZWidgetRight(m_HeaderHost.FindAnyWidget("MoveUp"));
            if (candidate > moveRight)
                moveRight = candidate;

            candidate = TransferZWidgetRight(m_HeaderHost.FindAnyWidget("MoveDown"));
            if (candidate > moveRight)
                moveRight = candidate;

            if (moveRight <= 0.0)
                moveRight = TransferZWidgetRight(m_HeaderHost.FindAnyWidget("MovePanel"));

            if (moveRight > right)
                right = moveRight;
        }

        return right;
    }

    protected void TransferZPlaceControlsBeforeTitle(bool preferredVisible)
    {
        if (!m_Root || !m_HeaderLabel)
            return;

        float blockWidth = 86.0;
        if (preferredVisible)
            blockWidth = 107.0;

        RestoreHeaderText();

        float labelX;
        float labelY;
        float labelW;
        float labelH;
        m_HeaderLabel.GetScreenPos(labelX, labelY);
        m_HeaderLabel.GetScreenSize(labelW, labelH);

        float blockX = labelX;
        float nativeRight = TransferZNativeLeftReservedRight();
        const float nativeGap = 4.0;
        if (nativeRight > 0.0 && nativeRight + nativeGap > blockX)
            blockX = nativeRight + nativeGap;

        float blockY = labelY + (labelH - 29.0) * 0.5;
        m_Root.SetScreenPos(blockX, blockY, false);
        m_Root.SetScreenSize(blockWidth, 29.0, false);

        if (m_TransferZBlockBackground)
            m_TransferZBlockBackground.SetSize(blockWidth, 27, false);

        const float titleGap = 5.0;
        float titleX = blockX + blockWidth + titleGap;
        float originalRight = labelX + labelW;
        float targetWidth = originalRight - titleX;
        if (targetWidth < 20.0)
            targetWidth = 20.0;

        m_HeaderLabel.SetScreenPos(titleX, labelY, false);
        m_HeaderLabel.SetScreenSize(targetWidth, labelH, false);
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
        bool hasEntity = m_Root && m_Entity && m_Entity.GetInventory().GetCargo();
        if (hasEntity && !TransferZCaptureNativeHeaderGeometry())
        {
            m_Root.Show(false);
            return;
        }

        super.UpdateControls();
        if (!hasEntity)
            return;

        m_Root.Show(true);
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
        const float titleGap = 5.0;

        m_TransferZHeaderLabel.SetPos(m_TransferZHeaderLabelX, m_TransferZHeaderLabelY, false);
        m_TransferZHeaderLabel.SetSize(m_TransferZHeaderLabelW, m_TransferZHeaderLabelH, true);

        float labelX;
        float labelY;
        float labelW;
        float labelH;
        m_TransferZHeaderLabel.GetScreenPos(labelX, labelY);
        m_TransferZHeaderLabel.GetScreenSize(labelW, labelH);

        float blockX = labelX;
        float blockY = labelY + (labelH - 29.0) * 0.5;
        m_Root.SetScreenPos(blockX, blockY, false);
        m_Root.SetScreenSize(blockWidth, 29.0, false);

        if (m_TransferZBlockBackground)
            m_TransferZBlockBackground.SetSize(blockWidth, 27, false);

        float titleX = blockX + blockWidth + titleGap;
        float originalRight = labelX + labelW;
        float targetWidth = originalRight - titleX;
        if (targetWidth < 20.0)
            targetWidth = 20.0;

        m_TransferZHeaderLabel.SetScreenPos(titleX, labelY, false);
        m_TransferZHeaderLabel.SetScreenSize(targetWidth, labelH, false);
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
