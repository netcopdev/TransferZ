[CF_RegisterModule(TransferZCFModule)]
class TransferZCFModule : CF_ModuleWorld
{
#ifdef DIAG_DEVELOPER
    protected ref array<PlayerBase> m_TransferZDiagSeededPlayers;
    protected bool m_TransferZDiagOfflineSeeded;
#endif

    override void OnInit()
    {
        super.OnInit();
        EnableRPC();

#ifdef DIAG_DEVELOPER
        m_TransferZDiagSeededPlayers = new array<PlayerBase>();
        EnableClientReady();
        EnableUpdate();
#endif
    }

    override int GetRPCMin()
    {
        return TransferZRPC.REQUEST;
    }

    override int GetRPCMax()
    {
        return TransferZMaintenanceRPC.REQUEST + 1;
    }

    override void OnRPC(Class sender, CF_EventArgs args)
    {
        super.OnRPC(sender, args);

        if (!GetGame().IsServer())
            return;

        CF_EventRPCArgs rpc = CF_EventRPCArgs.Cast(args);
        if (!rpc)
            return;

        PlayerBase player = PlayerBase.Cast(rpc.Target);
        if (!player)
            return;

        if (rpc.ID == TransferZRPC.REQUEST)
            TransferZServerService.HandleRequest(player, rpc.Sender, rpc.Context);
        else if (rpc.ID == TransferZNestedUnpackRPC.REQUEST)
            TransferZNestedUnpackService.HandleRequest(player, rpc.Sender, rpc.Context);
        else if (rpc.ID == TransferZMaintenanceRPC.REQUEST)
            TransferZMaintenanceService.HandleRequest(player, rpc.Sender, rpc.Context);
    }

#ifdef DIAG_DEVELOPER
    override void OnUpdate(Class sender, CF_EventArgs args)
    {
        super.OnUpdate(sender, args);

        if (m_TransferZDiagOfflineSeeded || GetGame().IsMultiplayer())
            return;

        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
            return;

        m_TransferZDiagOfflineSeeded = true;
        SeedDiagnosticAmmo(player);
    }

    override void OnClientReady(Class sender, CF_EventArgs args)
    {
        super.OnClientReady(sender, args);

        if (!GetGame().IsServer())
            return;

        CF_EventPlayerArgs playerArgs = CF_EventPlayerArgs.Cast(args);
        if (!playerArgs || !playerArgs.Player)
            return;

        PlayerBase player = playerArgs.Player;
        if (m_TransferZDiagSeededPlayers && m_TransferZDiagSeededPlayers.Find(player) >= 0)
            return;

        if (!m_TransferZDiagSeededPlayers)
            m_TransferZDiagSeededPlayers = new array<PlayerBase>();
        m_TransferZDiagSeededPlayers.Insert(player);

        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(SeedDiagnosticAmmo, 750, false, player);
    }

    protected bool CreateDiagnosticAmmoStack(PlayerBase player, int quantity)
    {
        if (!player)
            return false;

        ItemBase ammo = ItemBase.Cast(player.GetInventory().CreateInInventory("Ammo_556x45"));
        if (!ammo)
            ammo = ItemBase.Cast(GetGame().CreateObjectEx("Ammo_556x45", player.GetPosition(), ECE_PLACE_ON_SURFACE));
        if (!ammo)
            return false;

        ammo.SetQuantity(quantity);
        return true;
    }

    protected void SeedDiagnosticAmmo(PlayerBase player)
    {
        if (!player)
            return;

        int created = 0;
        if (CreateDiagnosticAmmoStack(player, 3))
            created++;
        if (CreateDiagnosticAmmoStack(player, 5))
            created++;
        if (CreateDiagnosticAmmoStack(player, 7))
            created++;
        if (CreateDiagnosticAmmoStack(player, 9))
            created++;
        if (CreateDiagnosticAmmoStack(player, 11))
            created++;

        player.UpdateInventoryMenu();
        Print("[TransferZ] DIAG: seeded " + created.ToString() + "/5 partial Ammo_556x45 stacks for Stack testing");
    }
#endif
}
