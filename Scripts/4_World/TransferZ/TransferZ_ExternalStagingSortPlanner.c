class TransferZExternalStagingSortPlanner : TransferZSortPlanner
{
    protected static bool TryStageInPlayerCargo(PlayerBase player, EntityAI source, EntityAI item)
    {
        if (!player || !source || !item)
            return false;

        EntityAI sourceRoot = source.GetHierarchyRoot();
        if (!sourceRoot)
            sourceRoot = source;

        // When sorting a container already inside the player's hierarchy,
        // FindFreeLocationFor may simply resolve back into the source itself.
        // Use vicinity staging for that case until explicit alternate-container enumeration exists.
        if (sourceRoot == player)
            return false;

        if (!TransferZServerService.IsReachable(player, item))
            return false;
        if (!item.GetInventory().CanRemoveEntity())
            return false;

        InventoryLocation src = new InventoryLocation();
        if (!item.GetInventory().GetCurrentInventoryLocation(src))
            return false;

        EntityAI sourceParent = src.GetParent();
        if (sourceParent && src.GetType() == InventoryLocationType.CARGO && !sourceParent.CanReleaseCargo(item))
            return false;

        InventoryLocation dst = new InventoryLocation();
        if (!player.GetInventory().FindFreeLocationFor(item, FindInventoryLocationType.CARGO, dst))
            return false;
        if (!dst.IsValid() || dst.GetType() != InventoryLocationType.CARGO)
            return false;

        EntityAI destinationParent = dst.GetParent();
        if (!destinationParent)
            return false;
        if (destinationParent == source || TransferZServerService.IsDescendantOf(destinationParent, source))
            return false;

        EntityAI destinationRoot = destinationParent.GetHierarchyRoot();
        if (!destinationRoot)
            destinationRoot = destinationParent;
        if (destinationRoot != player)
            return false;

        if (!destinationParent.CanReceiveItemIntoCargo(item))
            return false;

        HumanInventory humanInventory = player.GetHumanInventory();
        if (humanInventory && humanInventory.FindCollidingUserReservedLocationIndex(item, dst) >= 0)
            return false;

        if (!GameInventory.CheckMoveToDstRequest(player, src, dst, GameInventory.c_MaxItemDistanceRadius))
            return false;
        if (!GameInventory.LocationCanMoveEntity(src, dst))
            return false;

        InventoryMode moveMode = InventoryMode.SERVER;
        if (!GetGame().IsMultiplayer())
            moveMode = InventoryMode.LOCAL;

        return player.GetInventory().TakeToDst(moveMode, src, dst);
    }

    protected static bool TryStageExternally(PlayerBase player, EntityAI source, EntityAI item, out bool usedPlayerCargo)
    {
        usedPlayerCargo = false;

        if (TryStageInPlayerCargo(player, source, item))
        {
            usedPlayerCargo = true;
            return true;
        }

        return TransferZServerService.TryMoveToVicinity(player, item);
    }

    protected static bool TryMoveToExactSortTarget(PlayerBase player, EntityAI source, TransferZSortRecord record, int row, int col, bool flip)
    {
        if (!player || !source || !record || !record.item)
            return false;

        EntityAI item = record.item;
        if (!TransferZServerService.IsReachable(player, source) || !TransferZServerService.IsReachable(player, item))
        {
            Print("[TransferZ] Sort external target rejected: unreachable item=" + item.GetType());
            return false;
        }
        if (!item.GetInventory().CanRemoveEntity())
        {
            Print("[TransferZ] Sort external target rejected: CanRemoveEntity=false item=" + item.GetType());
            return false;
        }
        if (!source.CanReceiveItemIntoCargo(item))
        {
            Print("[TransferZ] Sort external target rejected: source refuses cargo item=" + item.GetType());
            return false;
        }

        InventoryLocation src = new InventoryLocation();
        if (!item.GetInventory().GetCurrentInventoryLocation(src))
        {
            Print("[TransferZ] Sort external target rejected: source location unavailable item=" + item.GetType());
            return false;
        }

        EntityAI sourceParent = src.GetParent();
        if (sourceParent && src.GetType() == InventoryLocationType.CARGO && !sourceParent.CanReleaseCargo(item))
        {
            Print("[TransferZ] Sort external target rejected: current cargo refuses release item=" + item.GetType());
            return false;
        }

        InventoryLocation dst = new InventoryLocation();
        dst.SetCargo(source, item, record.cargoIndex, row, col, flip);

        HumanInventory humanInventory = player.GetHumanInventory();
        if (humanInventory && humanInventory.FindCollidingUserReservedLocationIndex(item, dst) >= 0)
        {
            Print("[TransferZ] Sort external target rejected: destination reserved item=" + item.GetType());
            return false;
        }

        if (!GameInventory.CheckMoveToDstRequest(player, src, dst, GameInventory.c_MaxItemDistanceRadius))
        {
            Print("[TransferZ] Sort external target rejected: CheckMoveToDstRequest=false item=" + item.GetType());
            return false;
        }
        if (!GameInventory.LocationCanMoveEntity(src, dst))
        {
            Print("[TransferZ] Sort external target rejected: LocationCanMoveEntity=false item=" + item.GetType());
            return false;
        }

        InventoryMode moveMode = InventoryMode.SERVER;
        if (!GetGame().IsMultiplayer())
            moveMode = InventoryMode.LOCAL;

        bool moved = player.GetInventory().TakeToDst(moveMode, src, dst);
        if (!moved)
            Print("[TransferZ] Sort external target rejected: TakeToDst=false item=" + item.GetType());
        return moved;
    }

    protected static int RecoverStagedItems(PlayerBase player, EntityAI source, notnull array<ref TransferZSortRecord> records, notnull array<int> staged)
    {
        int recovered = 0;
        for (int recordIndex = 0; recordIndex < records.Count(); recordIndex++)
        {
            if (staged.Get(recordIndex) == 0)
                continue;

            TransferZSortRecord record = records.Get(recordIndex);
            if (!record || !record.item)
                continue;

            if (IsDirectCargoItem(source, record.item))
            {
                staged.Set(recordIndex, 0);
                recovered++;
                continue;
            }

            if (TransferZServerService.TryMoveToExactCargo(player, record.item, source))
            {
                staged.Set(recordIndex, 0);
                recovered++;
            }
        }
        return recovered;
    }

    protected static bool BuildExternalTargetLayout(PlayerBase player, EntityAI source, notnull array<ref TransferZSortRecord> records, int cargoWidth, int cargoHeight, notnull array<int> targetWidths, notnull array<int> targetHeights, notnull array<int> targetFlips)
    {
        if (!AssignCompactTargetsV4(player, source, records, cargoWidth, cargoHeight, targetWidths, targetHeights, targetFlips))
        {
            Print("[TransferZ] Sort external compact target packing failed; retrying rotation-aware first-fit targets");
            if (!AssignFirstFitTargetsV4(player, source, records, cargoWidth, cargoHeight, targetWidths, targetHeights, targetFlips))
            {
                Print("[TransferZ] Sort external failed: target layout assignment");
                return false;
            }
        }

        OptimizeEquivalentTargetAssignmentsV4(records, targetWidths, targetHeights, targetFlips);
        SortRecordsByTargetV4(records, targetWidths, targetHeights, targetFlips);
        LogTargetsV4(records, targetWidths, targetHeights, targetFlips);
        return true;
    }

    override static int Sort(PlayerBase player, EntityAI source)
    {
        if (!player || !source || !TransferZServerService.IsReachable(player, source) || !source.GetInventory().GetCargo())
        {
            Print("[TransferZ] Sort external rejected: invalid or unreachable cargo source");
            return -1;
        }

        ref array<ref TransferZSortRecord> records = new array<ref TransferZSortRecord>();
        int cargoWidth;
        int cargoHeight;
        if (!SnapshotSortRecords(source, records, cargoWidth, cargoHeight))
        {
            Print("[TransferZ] Sort external rejected: cargo snapshot failed for " + source.GetType());
            return -1;
        }
        if (records.Count() < 2)
            return 0;

        SortRecordsV4(records);

        ref array<int> targetWidths = new array<int>();
        ref array<int> targetHeights = new array<int>();
        ref array<int> targetFlips = new array<int>();
        if (!BuildExternalTargetLayout(player, source, records, cargoWidth, cargoHeight, targetWidths, targetHeights, targetFlips))
            return -1;

        if (AllRecordsAtTargetV4(records, targetFlips))
            return 0;

        ref array<int> staged = new array<int>();
        ResetGrid(staged, records.Count());

        int stagedPlayer = 0;
        int stagedGround = 0;

        // Clear every item that needs to move before placing any target. This makes
        // execution linear and removes recursive in-cargo cycle breaking while
        // preserving each original EntityAI instance.
        for (int recordIndex = 0; recordIndex < records.Count(); recordIndex++)
        {
            TransferZSortRecord record = records.Get(recordIndex);
            if (RecordAtTargetV4(record, recordIndex, targetFlips))
                continue;

            bool usedPlayerCargo;
            if (!TryStageExternally(player, source, record.item, usedPlayerCargo))
            {
                int recovered = RecoverStagedItems(player, source, records, staged);
                Print("[TransferZ] Sort external failed: could not stage item=" + record.item.GetType() + " recovered=" + recovered.ToString());
                return -1;
            }

            staged.Set(recordIndex, 1);
            if (usedPlayerCargo)
                stagedPlayer++;
            else
                stagedGround++;
        }

        Print("[TransferZ] Sort external staged playerCargo=" + stagedPlayer.ToString() + " ground=" + stagedGround.ToString());

        int moved = 0;
        for (int targetIndex = 0; targetIndex < records.Count(); targetIndex++)
        {
            if (staged.Get(targetIndex) == 0)
                continue;

            TransferZSortRecord targetRecord = records.Get(targetIndex);
            bool targetFlip = targetFlips.Get(targetIndex) != 0;
            if (!TryMoveToExactSortTarget(player, source, targetRecord, targetRecord.targetRow, targetRecord.targetCol, targetFlip))
            {
                int recoveredAfterFailure = RecoverStagedItems(player, source, records, staged);
                Print("[TransferZ] Sort external failed: target placement item=" + targetRecord.item.GetType() + " recovered=" + recoveredAfterFailure.ToString());
                return -1;
            }

            staged.Set(targetIndex, 0);
            moved++;
        }

        Print("[TransferZ] Sort external result source=" + source.GetType() + " repositioned=" + moved.ToString() + " stagedPlayer=" + stagedPlayer.ToString() + " stagedGround=" + stagedGround.ToString());
        return moved;
    }

    override static void HandleRequest(PlayerBase player, PlayerIdentity sender, ParamsReadContext ctx)
    {
        if (!player)
            return;

        if (GetGame().IsMultiplayer())
        {
            PlayerIdentity playerIdentity = player.GetIdentity();
            if (!sender || !playerIdentity || sender.GetId() != playerIdentity.GetId())
            {
                Print("[TransferZ] Maintenance RPC rejected: sender does not own player");
                return;
            }
        }

        int operation;
        int sourceLow;
        int sourceHigh;
        if (!ctx.Read(operation) || !ctx.Read(sourceLow) || !ctx.Read(sourceHigh))
            return;

        EntityAI source = TransferZServerService.ResolveEntity(sourceLow, sourceHigh);
        if (!source)
        {
            if (operation == TransferZMaintenanceOperation.SORT)
                SendResult(player, operation, sourceLow, sourceHigh, false);
            return;
        }

        if (operation == TransferZMaintenanceOperation.SORT)
        {
            int sortResult = Sort(player, source);
            SendResult(player, operation, sourceLow, sourceHigh, sortResult >= 0);
            player.UpdateInventoryMenu();
        }
        else if (operation == TransferZMaintenanceOperation.STACK)
        {
            Stack(player, source);
            player.UpdateInventoryMenu();
        }
    }
}
