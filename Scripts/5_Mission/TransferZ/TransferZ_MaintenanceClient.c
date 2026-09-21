class TransferZMaintenanceClient
{
    protected static int s_LastRequestTime = -1000;
    protected static int s_LastOperation = -1;
    protected static EntityAI s_LastSource;
    protected static int s_LastSourceCargoIndex = 0;

    protected static bool IsReachableContainer(EntityAI source, int sourceCargoIndex)
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || !source || !TransferZCargo.Exists(source, sourceCargoIndex))
            return false;

        EntityAI root = source.GetHierarchyRoot();
        if (!root)
            root = source;
        if (root == player)
            return true;
        if (root.IsMan())
            return false;

        if (root.IsInherited(Transport))
            return root.CanDisplayCargo();

        return GameInventory.CheckManipulatedObjectsDistances(source, player, GameInventory.c_MaxItemDistanceRadius);
    }

    protected static bool IsDuplicate(int operation, EntityAI source, int sourceCargoIndex)
    {
        int now = GetGame().GetTime();
        bool duplicate = operation == s_LastOperation && source == s_LastSource && sourceCargoIndex == s_LastSourceCargoIndex && now - s_LastRequestTime >= 0 && now - s_LastRequestTime < 250;
        s_LastRequestTime = now;
        s_LastOperation = operation;
        s_LastSource = source;
        s_LastSourceCargoIndex = sourceCargoIndex;
        return duplicate;
    }

    protected static bool Request(int operation, EntityAI source, int sourceCargoIndex)
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || !IsReachableContainer(source, sourceCargoIndex))
        {
            if (operation == TransferZMaintenanceOperation.SORT)
                TransferZMaintenanceResultState.PublishLocal(operation, source, sourceCargoIndex, false);
            return false;
        }
        if (IsDuplicate(operation, source, sourceCargoIndex))
            return false;

        if (!GetGame().IsMultiplayer())
        {
            if (operation == TransferZMaintenanceOperation.SORT)
            {
                int sortResult = TransferZTransactionalSortPlanner.Sort(player, source, sourceCargoIndex);
                TransferZMaintenanceResultState.PublishLocal(operation, source, sourceCargoIndex, sortResult >= 0);
                return sortResult >= 0;
            }
            if (operation == TransferZMaintenanceOperation.STACK)
            {
                TransferZMaintenanceService.Stack(player, source, sourceCargoIndex);
                player.UpdateInventoryMenu();
                return true;
            }
            return false;
        }

        int sourceLow;
        int sourceHigh;
        TransferZNet.GetEntityNetworkId(source, sourceLow, sourceHigh);

        ScriptRPC rpc = new ScriptRPC();
        rpc.Write(operation);
        rpc.Write(sourceLow);
        rpc.Write(sourceHigh);
        rpc.Write(sourceCargoIndex);
        rpc.Send(player, TransferZMaintenanceRPC.REQUEST, true, player.GetIdentity());
        return true;
    }

    static bool RequestSort(EntityAI source, int sourceCargoIndex = 0)
    {
        return Request(TransferZMaintenanceOperation.SORT, source, sourceCargoIndex);
    }

    static bool RequestStack(EntityAI source, int sourceCargoIndex = 0)
    {
        return Request(TransferZMaintenanceOperation.STACK, source, sourceCargoIndex);
    }
}
