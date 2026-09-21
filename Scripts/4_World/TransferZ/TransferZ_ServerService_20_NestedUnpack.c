class TransferZNestedUnpackService
{
    static void CollectNestedLeaves(EntityAI source, int sourceCargoIndex, EntityAI destination, notnull array<EntityAI> leaves)
    {
        if (!source)
            return;

        CargoBase cargo = TransferZCargo.Get(source, sourceCargoIndex);
        if (!cargo)
            return;

        for (int i = 0; i < cargo.GetItemCount(); i++)
        {
            EntityAI child = cargo.GetItem(i);
            if (!child || child == destination || !TransferZCargo.Exists(child, 0))
                continue;

            TransferZServerService.CollectUnpackLeaves(child, destination, leaves);
        }
    }

    static int Unpack(PlayerBase player, EntityAI source, EntityAI destination, int sourceCargoIndex = 0, int destinationCargoIndex = 0)
    {
        if (!player || !source || !destination)
            return 0;

        if (!TransferZServerService.IsReachable(player, source) || !TransferZServerService.IsReachable(player, destination))
            return 0;

        if (!TransferZCargo.Exists(source, sourceCargoIndex) || !TransferZCargo.Exists(destination, destinationCargoIndex))
            return 0;

        if (destination != source && TransferZServerService.IsDescendantOf(destination, source))
            return 0;

        ref array<EntityAI> leaves = new array<EntityAI>();
        CollectNestedLeaves(source, sourceCargoIndex, destination, leaves);

        int moved = 0;
        foreach (EntityAI item : leaves)
        {
            if (TransferZServerService.TryMoveToExactCargo(player, item, destination, destinationCargoIndex))
                moved++;
        }
        return moved;
    }

    static int UnpackToVicinity(PlayerBase player, EntityAI source, int sourceCargoIndex = 0)
    {
        if (!player || !source || !TransferZServerService.IsReachable(player, source) || !TransferZCargo.Exists(source, sourceCargoIndex))
            return 0;

        ref array<EntityAI> leaves = new array<EntityAI>();
        CollectNestedLeaves(source, sourceCargoIndex, null, leaves);

        int moved = 0;
        foreach (EntityAI item : leaves)
        {
            if (TransferZServerService.TryMoveToVicinity(player, item))
                moved++;
        }
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
        int sourceCargoIndex;
        int destinationLow;
        int destinationHigh;
        int destinationCargoIndex;
        bool destinationIsVicinity;

        if (!ctx.Read(sourceLow))
            return;
        if (!ctx.Read(sourceHigh))
            return;
        if (!ctx.Read(sourceCargoIndex))
            return;
        if (!ctx.Read(destinationLow))
            return;
        if (!ctx.Read(destinationHigh))
            return;
        if (!ctx.Read(destinationCargoIndex))
            return;
        if (!ctx.Read(destinationIsVicinity))
            return;

        if (sourceCargoIndex < 0 || destinationCargoIndex < 0 || !TransferZServerService.CanPlayerManipulate(player))
            return;

        EntityAI source = TransferZServerService.ResolveEntity(sourceLow, sourceHigh);
        EntityAI destination = TransferZServerService.ResolveEntity(destinationLow, destinationHigh);

        if (destinationIsVicinity)
            UnpackToVicinity(player, source, sourceCargoIndex);
        else
            Unpack(player, source, destination, sourceCargoIndex, destinationCargoIndex);
    }
}
