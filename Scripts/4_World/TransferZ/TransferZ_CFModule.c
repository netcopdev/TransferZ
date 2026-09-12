[CF_RegisterModule(TransferZCFModule)]
class TransferZCFModule : CF_ModuleWorld
{
#ifdef DIAG_DEVELOPER
    protected ref array<PlayerBase> m_TransferZDiagSeededPlayers;
#endif

    override void OnInit()
    {
        super.OnInit();
        EnableRPC();

#ifdef DIAG_DEVELOPER
        m_TransferZDiagSeededPlayers = new array<PlayerBase>();
        EnableClientReady();
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

    protected void CreateDiagnosticAmmoStack(PlayerBase player, int quantity)
    {
        if (!player)
            return;

        ItemBase ammo = ItemBase.Cast(player.GetInventory().CreateInInventory("Ammo_556x45"));
        if (ammo)
            ammo.SetQuantity(quantity);
    }

    protected void SeedDiagnosticAmmo(PlayerBase player)
    {
        if (!player)
            return;

        CreateDiagnosticAmmoStack(player, 3);
        CreateDiagnosticAmmoStack(player, 5);
        CreateDiagnosticAmmoStack(player, 7);
        CreateDiagnosticAmmoStack(player, 9);
        CreateDiagnosticAmmoStack(player, 11);

        player.UpdateInventoryMenu();
        Print("[TransferZ] DIAG: seeded five partial Ammo_556x45 stacks for Stack testing");
    }
#endif
}
