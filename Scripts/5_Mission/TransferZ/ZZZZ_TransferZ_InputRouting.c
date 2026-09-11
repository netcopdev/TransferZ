modded class TransferZClientState
{
    protected bool TransferZItemAlreadyInCargo(EntityAI item, EntityAI destination)
    {
        if (!item || !destination)
            return false;

        InventoryLocation location = new InventoryLocation();
        if (!item.GetInventory().GetCurrentInventoryLocation(location))
            return false;

        return location.GetType() == InventoryLocationType.CARGO && location.GetParent() == destination;
    }

    protected bool TransferZItemAlreadyInVicinity(EntityAI item)
    {
        if (!item)
            return false;

        InventoryLocation location = new InventoryLocation();
        if (!item.GetInventory().GetCurrentInventoryLocation(location))
            return false;

        return location.GetType() == InventoryLocationType.GROUND;
    }

    bool RequestItemToDestination(EntityAI item)
    {
        if (!item)
            return false;

        if (IsDestinationVicinity())
        {
            if (TransferZItemAlreadyInVicinity(item))
                return false;
            return RequestMoveItemToVicinity(item);
        }

        EntityAI destination = GetDestination();
        if (!destination || item == destination || TransferZItemAlreadyInCargo(item, destination))
            return false;

        return RequestMoveItem(item, destination);
    }

    bool RequestItemToPreferred(EntityAI item)
    {
        if (!item)
            return false;

        EntityAI destination = GetPreferredDestination();
        if (!destination || item == destination || TransferZItemAlreadyInCargo(item, destination))
            return false;

        return RequestMoveItem(item, destination);
    }

    protected void SendNestedUnpackRequest(EntityAI source, EntityAI destination, bool destinationIsVicinity)
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || !source || !source.GetInventory().GetCargo())
            return;

        if (!destinationIsVicinity && !IsParticipantReachable(destination))
            return;

        if (IsDuplicateRequest(TransferZNestedUnpackRPC.CLIENT_OPERATION, source, destination, null, destinationIsVicinity))
            return;

        if (!GetGame().IsMultiplayer())
        {
            if (destinationIsVicinity)
                TransferZNestedUnpackService.UnpackToVicinity(player, source);
            else
                TransferZNestedUnpackService.Unpack(player, source, destination);
            player.UpdateInventoryMenu();
            return;
        }

        int sourceLow;
        int sourceHigh;
        int destinationLow;
        int destinationHigh;
        TransferZNet.GetEntityNetworkId(source, sourceLow, sourceHigh);
        TransferZNet.GetEntityNetworkId(destination, destinationLow, destinationHigh);

        ScriptRPC rpc = new ScriptRPC();
        rpc.Write(sourceLow);
        rpc.Write(sourceHigh);
        rpc.Write(destinationLow);
        rpc.Write(destinationHigh);
        rpc.Write(destinationIsVicinity);
        rpc.Send(player, TransferZNestedUnpackRPC.REQUEST, true, player.GetIdentity());
    }

    bool RequestNestedUnpackTo(EntityAI source, EntityAI destination)
    {
        if (!source || !source.GetInventory().GetCargo() || !IsParticipantReachable(destination))
            return false;

        SendNestedUnpackRequest(source, destination, false);
        return true;
    }

    bool RequestNestedUnpackToVicinity(EntityAI source)
    {
        if (!source || !source.GetInventory().GetCargo())
            return false;

        SendNestedUnpackRequest(source, null, true);
        return true;
    }

    bool RequestNestedUnpack(EntityAI source)
    {
        if (IsDestinationVicinity())
            return RequestNestedUnpackToVicinity(source);
        return RequestNestedUnpackTo(source, GetDestination());
    }
}

modded class TransferZHeaderControls
{
    override void OnUnpack(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT || !m_Entity)
            return;
        if (GetGame().GetTime() < m_IgnoreOperationClickUntil)
            return;

        TransferZClientState.Get().RequestNestedUnpack(m_Entity);
        ShowTooltip(w);
        RefreshOperationStatus();
    }
}

modded class Icon
{
    protected bool m_TransferZModifierClickRegistered;
    protected bool m_TransferZModifierClickPending;
    protected int m_TransferZModifierClickMode;

    override void InitEx(EntityAI obj, bool refresh = true)
    {
        super.InitEx(obj, refresh);

        if (!m_TransferZModifierClickRegistered && GetMainWidget())
        {
            WidgetEventHandler.GetInstance().RegisterOnMouseButtonUp(GetMainWidget(), this, "TransferZOnModifierMouseButtonUp");
            m_TransferZModifierClickRegistered = true;
        }
    }

    override void MouseClick(Widget w, int x, int y, int button)
    {
        super.MouseClick(w, x, y, button);

        if (button != MouseState.LEFT)
            return;

        m_TransferZModifierClickMode = m_TransferZModifierDragMode;
        m_TransferZModifierClickPending = m_TransferZModifierClickMode != TRANSFERZ_DRAG_NONE && m_Obj && !m_HandsIcon;
    }

    override void CreateWhiteBackground()
    {
        super.CreateWhiteBackground();

        if (m_TransferZModifierDragStarted)
            m_TransferZModifierClickPending = false;
    }

    override void DestroyWhiteBackground()
    {
        if (m_TransferZModifierDragStarted)
            m_TransferZModifierClickPending = false;

        super.DestroyWhiteBackground();
    }

    void TransferZOnModifierMouseButtonUp(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT)
            return;

        bool pending = m_TransferZModifierClickPending;
        int mode = m_TransferZModifierClickMode;
        m_TransferZModifierClickPending = false;
        m_TransferZModifierClickMode = TRANSFERZ_DRAG_NONE;

        if (!pending || !m_Obj || m_HandsIcon)
            return;

        TransferZClientState state = TransferZClientState.Get();
        if (mode == TRANSFERZ_DRAG_TRANSFER)
            state.RequestItemToDestination(m_Obj);
        else if (mode == TRANSFERZ_DRAG_UNPACK)
            state.RequestItemToPreferred(m_Obj);
    }
}

modded class SlotsIcon
{
    protected static int s_TransferZModifierClickSuppressUntil;

    override void OnIconDrag(Widget w)
    {
        super.OnIconDrag(w);

        if (m_TransferZVicinityModifierDragStarted)
            s_TransferZModifierClickSuppressUntil = GetGame().GetTime() + 300;
    }

    static bool TransferZSuppressModifierClick()
    {
        return GetGame().GetTime() < s_TransferZModifierClickSuppressUntil;
    }
}

modded class VicinitySlotsContainer
{
    protected EntityAI TransferZResolveClickedVicinityItem(Widget w)
    {
        if (!w)
            return null;

        string name = w.GetName();
        name.Replace("PanelWidget", "Render");
        ItemPreviewWidget preview = ItemPreviewWidget.Cast(w.FindAnyWidget(name));
        if (!preview)
            preview = ItemPreviewWidget.Cast(w.FindAnyWidget("Render"));
        if (!preview)
            preview = ItemPreviewWidget.Cast(w);
        if (!preview)
            return null;

        return preview.GetItem();
    }

    override void MouseClick(Widget w, int x, int y, int button)
    {
        if (button == MouseState.LEFT && SlotsIcon.TransferZSuppressModifierClick())
            return;

        if (button == MouseState.LEFT && !KeyState(KeyCode.KC_LCONTROL) && !KeyState(KeyCode.KC_RCONTROL))
        {
            bool shiftDown = KeyState(KeyCode.KC_LSHIFT) || KeyState(KeyCode.KC_RSHIFT);
            bool altDown = KeyState(KeyCode.KC_LMENU) || KeyState(KeyCode.KC_RMENU);

            if (shiftDown != altDown)
            {
                EntityAI item = TransferZResolveClickedVicinityItem(w);
                ItemBase itemBase = ItemBase.Cast(item);
                if (itemBase && itemBase.IsTakeable() && item.GetInventory().CanRemoveEntity())
                {
                    if (shiftDown)
                        TransferZClientState.Get().RequestItemToDestination(item);
                    else
                        TransferZClientState.Get().RequestItemToPreferred(item);
                }
                return;
            }
        }

        super.MouseClick(w, x, y, button);
    }
}
