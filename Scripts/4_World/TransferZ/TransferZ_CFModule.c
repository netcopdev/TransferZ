[CF_RegisterModule(TransferZCFModule)]
class TransferZCFModule : CF_ModuleWorld
{
    override void OnInit()
    {
        super.OnInit();
        EnableRPC();
        Print("[TransferZ] BUILD MARKER SORT-DIAG-20260915-A");
    }

    override int GetRPCMin()
    {
        return TransferZRPC.REQUEST;
    }

    override int GetRPCMax()
    {
        return TransferZMaintenanceRPC.RESULT;
    }

    override void OnRPC(Class sender, CF_EventArgs args)
    {
        super.OnRPC(sender, args);

        CF_EventRPCArgs rpc = CF_EventRPCArgs.Cast(args);
        if (!rpc)
            return;

        if (!GetGame().IsServer())
        {
            if (rpc.ID == TransferZMaintenanceRPC.RESULT)
                TransferZMaintenanceResultState.HandleRPC(rpc.Context);
            return;
        }

        PlayerBase player = PlayerBase.Cast(rpc.Target);
        if (!player)
            return;

        if (rpc.ID == TransferZRPC.REQUEST)
            TransferZServerService.HandleRequest(player, rpc.Sender, rpc.Context);
        else if (rpc.ID == TransferZNestedUnpackRPC.REQUEST)
            TransferZNestedUnpackService.HandleRequest(player, rpc.Sender, rpc.Context);
        else if (rpc.ID == TransferZMaintenanceRPC.REQUEST)
            TransferZSortPlannerV5.HandleRequest(player, rpc.Sender, rpc.Context);
    }
}
