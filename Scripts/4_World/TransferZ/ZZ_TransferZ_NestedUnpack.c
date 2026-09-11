class TransferZNestedUnpackService
{
    static void CollectNestedLeaves(EntityAI source, EntityAI destination, notnull array<EntityAI> leaves)
    {
        if (!source)
            return;

        CargoBase cargo = source.GetInventory().GetCargo();
        if (!cargo)
            return;

        for (int i = 0; i < cargo.GetItemCount(); i++)
        {
            EntityAI child = cargo.GetItem(i);
            if (!child || child == destination || !child.GetInventory().GetCargo())
                continue;

            TransferZServerService.CollectUnpackLeaves(child, destination, leaves);
        }
    }

    static int Unpack(PlayerBase player, EntityAI source, EntityAI destination)
    {
        if (!player || !source || !destination)
            return 0;

        if (!TransferZServerService.IsReachable(player, source) || !TransferZServerService.IsReachable(player, destination))
            return 0;

        if (!source.GetInventory().GetCargo() || !destination.GetInventory().GetCargo())
            return 0;

        if (destination != source && TransferZServerService.IsDescendantOf(destination, source))
            return 0;

        ref array<EntityAI> leaves = new array<EntityAI>();
        CollectNestedLeaves(source, destination, leaves);

        int moved = 0;
        foreach (EntityAI item : leaves)
        {
            if (TransferZServerService.TryMoveToExactCargo(player, item, destination))
                moved++;
        }

        Print("[TransferZ] Nested unpack result source=" + source.GetType() + " destination=" + destination.GetType() + " moved=" + moved.ToString() + "/" + leaves.Count().ToString());
        return moved;
    }

    static int UnpackToVicinity(PlayerBase player, EntityAI source)
    {
        if (!player || !source || !TransferZServerService.IsReachable(player, source) || !source.GetInventory().GetCargo())
            return 0;

        ref array<EntityAI> leaves = new array<EntityAI>();
        CollectNestedLeaves(source, null, leaves);

        int moved = 0;
        foreach (EntityAI item : leaves)
        {
            if (TransferZServerService.TryMoveToVicinity(player, item))
                moved++;
        }

        Print("[TransferZ] Nested unpack result source=" + source.GetType() + " destination=VICINITY moved=" + moved.ToString() + "/" + leaves.Count().ToString());
        return moved;
    }

    static void HandleRequest(PlayerBase player, PlayerIdentity sender, ParamsReadContext ctx)
    {
        if (!player)
            return;

        if (GetGame().IsMultiplayer())
        {
            PlayerIdentity playerIdentity = player.GetIdentity();
            if (!sender || !playerIdentity)
            {
                Print("[TransferZ] Nested unpack RPC rejected: missing multiplayer identity");
                return;
            }

            if (sender.GetId() != playerIdentity.GetId())
            {
                Print("[TransferZ] Nested unpack RPC rejected: sender does not own player");
                return;
            }
        }

        int sourceLow;
        int sourceHigh;
        int destinationLow;
        int destinationHigh;
        bool destinationIsVicinity;

        if (!ctx.Read(sourceLow))
            return;
        if (!ctx.Read(sourceHigh))
            return;
        if (!ctx.Read(destinationLow))
            return;
        if (!ctx.Read(destinationHigh))
            return;
        if (!ctx.Read(destinationIsVicinity))
            return;

        EntityAI source = TransferZServerService.ResolveEntity(sourceLow, sourceHigh);
        EntityAI destination = TransferZServerService.ResolveEntity(destinationLow, destinationHigh);

        if (destinationIsVicinity)
            UnpackToVicinity(player, source);
        else
            Unpack(player, source, destination);
    }
}

modded class PlayerBase
{
    override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
    {
        super.OnRPC(sender, rpc_type, ctx);

        if (rpc_type != TransferZNestedUnpackRPC.REQUEST || !GetGame().IsServer())
            return;

        TransferZNestedUnpackService.HandleRequest(this, sender, ctx);
    }
}
