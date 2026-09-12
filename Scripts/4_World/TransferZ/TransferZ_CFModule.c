[CF_RegisterModule(TransferZCFModule)]
class TransferZCFModule : CF_ModuleWorld
{
    override void OnInit()
    {
        super.OnInit();
        EnableRPC();
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
}
