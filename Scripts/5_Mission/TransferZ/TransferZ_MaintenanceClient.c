class TransferZMaintenanceClient
{
    protected static int s_LastRequestTime = -1000;
    protected static int s_LastOperation = -1;
    protected static EntityAI s_LastSource;

    protected static bool IsReachableContainer(EntityAI source)
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || !source || !source.GetInventory().GetCargo())
            return false;

        EntityAI root = source.GetHierarchyRoot();
        if (!root)
            root = source;
        if (root == player)
            return true;
        if (root.IsMan())
            return false;

        return GameInventory.CheckManipulatedObjectsDistances(source, player, GameInventory.c_MaxItemDistanceRadius);
    }

    protected static bool IsDuplicate(int operation, EntityAI source)
    {
        int now = GetGame().GetTime();
        bool duplicate = operation == s_LastOperation && source == s_LastSource && now - s_LastRequestTime >= 0 && now - s_LastRequestTime < 250;
        s_LastRequestTime = now;
        s_LastOperation = operation;
        s_LastSource = source;
        return duplicate;
    }

    protected static bool Request(int operation, EntityAI source)
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || !IsReachableContainer(source))
        {
            if (operation == TransferZMaintenanceOperation.SORT)
            {
                Print("[TransferZ] Sort client rejected: source is not reachable cargo");
                TransferZMaintenanceResultState.PublishLocal(operation, source, false);
            }
            return false;
        }
        if (IsDuplicate(operation, source))
        {
            if (operation == TransferZMaintenanceOperation.SORT)
                Print("[TransferZ] Sort client rejected: duplicate request debounce");
            return false;
        }

        if (!GetGame().IsMultiplayer())
        {
            if (operation == TransferZMaintenanceOperation.SORT)
            {
                Print("[TransferZ] Sort client executing local DayZDiag/single-player request");
                int sortResult = TransferZMaintenanceService.Sort(player, source);
                Print("[TransferZ] Sort client local result=" + sortResult.ToString());
                TransferZMaintenanceResultState.PublishLocal(operation, source, sortResult >= 0);
                return sortResult >= 0;
            }
            if (operation == TransferZMaintenanceOperation.STACK)
            {
                TransferZMaintenanceService.Stack(player, source);
                player.UpdateInventoryMenu();
                return true;
            }
            return false;
        }

        int sourceLow;
        int sourceHigh;
        TransferZNet.GetEntityNetworkId(source, sourceLow, sourceHigh);

        if (operation == TransferZMaintenanceOperation.SORT)
            Print("[TransferZ] Sort client sending RPC source=" + source.GetType() + " net=" + sourceLow.ToString() + ":" + sourceHigh.ToString());

        ScriptRPC rpc = new ScriptRPC();
        rpc.Write(operation);
        rpc.Write(sourceLow);
        rpc.Write(sourceHigh);
        rpc.Send(player, TransferZMaintenanceRPC.REQUEST, true, player.GetIdentity());
        return true;
    }

    static bool RequestSort(EntityAI source)
    {
        return Request(TransferZMaintenanceOperation.SORT, source);
    }

    static bool RequestStack(EntityAI source)
    {
        return Request(TransferZMaintenanceOperation.STACK, source);
    }
}
