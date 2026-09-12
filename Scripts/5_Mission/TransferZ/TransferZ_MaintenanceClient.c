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
        if (!player || !IsReachableContainer(source) || IsDuplicate(operation, source))
            return false;

        if (!GetGame().IsMultiplayer())
        {
            if (operation == TransferZMaintenanceOperation.SORT)
            {
                TransferZMaintenanceService.Sort(player, source);
            }
            else if (operation == TransferZMaintenanceOperation.STACK)
            {
                TransferZMaintenanceService.Stack(player, source);
                player.UpdateInventoryMenu();
            }
            else
            {
                return false;
            }

            return true;
        }

        int sourceLow;
        int sourceHigh;
        TransferZNet.GetEntityNetworkId(source, sourceLow, sourceHigh);

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
