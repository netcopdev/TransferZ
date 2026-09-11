modded class TransferZHeaderControls
{
    protected Widget m_TransferZHandsGrid;
    protected Widget m_TransferZHandsLeftSpacer;
    protected float m_TransferZHandsLeftSpacerW;
    protected float m_TransferZHandsLeftSpacerH;
    protected bool m_TransferZHandsLayoutCaptured;

    protected bool TransferZCaptureHandsLayout()
    {
        if (m_TransferZHandsLayoutCaptured)
            return true;
        if (!m_HeaderHost || !m_HeaderLabel)
            return false;

        Widget grid = m_HeaderHost.FindAnyWidget("hih_cont");
        Widget leftSpacer = m_HeaderHost.FindAnyWidget("FrameWidget0");
        if (!grid || !leftSpacer || m_HeaderLabel.GetParent() != grid)
            return false;

        float spacerW;
        float spacerH;
        leftSpacer.GetSize(spacerW, spacerH);
        if (spacerW <= 0.0 || spacerH <= 0.0)
            return false;

        m_TransferZHandsGrid = grid;
        m_TransferZHandsLeftSpacer = leftSpacer;
        m_TransferZHandsLeftSpacerW = spacerW;
        m_TransferZHandsLeftSpacerH = spacerH;
        m_TransferZHandsLayoutCaptured = true;
        return true;
    }

    protected void TransferZRestoreHandsLayout()
    {
        if (!TransferZCaptureHandsLayout())
            return;

        m_TransferZHandsLeftSpacer.SetSize(m_TransferZHandsLeftSpacerW, m_TransferZHandsLeftSpacerH, false);

        TextWidget title = TextWidget.Cast(m_HeaderLabel);
        if (title)
            title.SetTextHorizontalAlignment(ALIGN_CENTER);

        m_TransferZHandsGrid.Update();
        m_HeaderLabel.Update();
    }

    protected void TransferZApplyHandsLayout()
    {
        if (!m_Root || !TransferZCaptureHandsLayout())
            return;

        float blockWidth = 86.0;
        if (CanBePreferred())
            blockWidth = 107.0;

        const float titleGap = 5.0;

        float spacerX;
        float spacerY;
        m_TransferZHandsLeftSpacer.GetScreenPos(spacerX, spacerY);

        float reservedWidth = m_TransferZHandsLeftSpacerW + blockWidth + titleGap;
        m_TransferZHandsLeftSpacer.SetSize(reservedWidth, m_TransferZHandsLeftSpacerH, false);
        m_TransferZHandsGrid.Update();
        m_HeaderLabel.Update();

        TextWidget title = TextWidget.Cast(m_HeaderLabel);
        if (title)
            title.SetTextHorizontalAlignment(ALIGN_LEFT);

        float labelX;
        float labelY;
        float labelW;
        float labelH;
        m_HeaderLabel.GetScreenPos(labelX, labelY);
        m_HeaderLabel.GetScreenSize(labelW, labelH);

        float blockX = spacerX + m_TransferZHandsLeftSpacerW;
        float blockY = labelY + (labelH - 29.0) * 0.5;
        m_Root.SetScreenPos(blockX, blockY, false);
        m_Root.SetScreenSize(blockWidth, 29.0, false);

        ImageWidget background = ImageWidget.Cast(m_Root.FindAnyWidget("TransferZ_BlockBackground"));
        if (background)
            background.SetSize(blockWidth, 27, false);
    }

    override void SetEntity(EntityAI entity)
    {
        bool hasCargo = entity && entity.GetInventory().GetCargo();
        if (!hasCargo)
            TransferZRestoreHandsLayout();

        super.SetEntity(entity);

        if (hasCargo)
            TransferZApplyHandsLayout();
    }

    override void UpdateControls()
    {
        super.UpdateControls();

        if (!m_Root || !m_Entity || !m_Entity.GetInventory().GetCargo())
        {
            TransferZRestoreHandsLayout();
            return;
        }

        TransferZApplyHandsLayout();
    }
}
