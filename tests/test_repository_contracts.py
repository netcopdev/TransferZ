from __future__ import annotations

import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def function_body(source: str, signature_fragment: str) -> str:
    start = source.find(signature_fragment)
    if start < 0:
        raise AssertionError(f"missing function signature: {signature_fragment}")
    brace = source.find("{", start)
    if brace < 0:
        raise AssertionError(f"missing function body: {signature_fragment}")

    depth = 0
    for index in range(brace, len(source)):
        char = source[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[brace : index + 1]
    raise AssertionError(f"unterminated function body: {signature_fragment}")


class RepositoryContracts(unittest.TestCase):
    def test_enforce_for_loops_do_not_use_empty_conditions(self) -> None:
        pattern = re.compile(r"for\s*\([^;\n]*;\s*;")
        offenders: list[str] = []

        for root_name in ("Scripts", "test"):
            root = ROOT / root_name
            if not root.exists():
                continue
            for path in sorted(root.rglob("*.c")):
                relative = path.relative_to(ROOT)
                for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
                    if pattern.search(line):
                        offenders.append(f"{relative}:{line_number}: {line.strip()}")

        self.assertEqual(
            [],
            offenders,
            "Enforce Script rejects C/C++-style for loops with an empty condition:\n" + "\n".join(offenders),
        )

    def test_version_is_synchronized(self) -> None:
        version = read("VERSION").strip()
        config = read("config.cpp")
        mod = read("mod.cpp")
        readme = read("README.md")

        config_match = re.search(r'\bversion\s*=\s*"([^"]+)"', config)
        mod_match = re.search(r'\bversion\s*=\s*"([^"]+)"', mod)
        readme_match = re.search(r"Current version:\s*\*\*([^*]+)\*\*\.", readme)
        self.assertIsNotNone(config_match)
        self.assertIsNotNone(mod_match)
        self.assertIsNotNone(readme_match)
        self.assertEqual(version, config_match.group(1))
        self.assertEqual(version, mod_match.group(1))
        self.assertEqual(version, readme_match.group(1))
        self.assertTrue((ROOT / "tools/version_metadata.py").is_file())

    def test_runtime_script_roots_do_not_pack_test_fixture(self) -> None:
        config = read("config.cpp")
        self.assertIn('"TransferZ/Scripts/3_Game"', config)
        self.assertIn('"TransferZ/Scripts/4_World"', config)
        self.assertIn('"TransferZ/Scripts/5_Mission"', config)
        self.assertNotIn("test/TransferZTest", config)

    def test_cf_remains_a_required_dependency(self) -> None:
        self.assertIn('"JM_CF_Scripts"', read("config.cpp"))

    def test_exact_cargo_move_keeps_native_request_validation(self) -> None:
        source = read("Scripts/4_World/TransferZ/TransferZ_ServerService.c")
        body = function_body(source, "static bool TryMoveToExactCargo(")
        self.assertIn("GameInventory.CheckMoveToDstRequest", body)
        self.assertIn("GameInventory.LocationCanMoveEntity", body)
        self.assertIn("item.GetInventory().TakeToDst", body)
        self.assertNotIn("player.GetInventory().TakeToDst", body)

    def test_vicinity_move_keeps_native_drop_validation(self) -> None:
        source = read("Scripts/4_World/TransferZ/TransferZ_ServerService.c")
        body = function_body(source, "static bool TryMoveToVicinity(")
        self.assertIn("GameInventory.CheckDropRequest", body)
        self.assertIn("item.GetInventory().DropEntity", body)

    def test_stack_keeps_native_source_validation(self) -> None:
        source = read("Scripts/4_World/TransferZ/TransferZ_MaintenanceService.c")
        body = function_body(source, "static int Stack(")
        self.assertIn("GameInventory.CheckRequestSrc", body)
        self.assertIn("CombineItems", body)

    def test_only_v4_sort_planner_remains_active(self) -> None:
        maintenance = read("Scripts/4_World/TransferZ/TransferZ_MaintenanceService.c")
        snapshot = function_body(maintenance, "protected static bool SnapshotSortRecords(")

        obsolete = (
            "protected static bool SortBefore(",
            "protected static void SortRecords(",
            "protected static bool AssignTargets(",
            "protected static void BuildTargetGrid(",
            "protected static bool FindTemporaryPlacement(",
            "protected static bool BuildSortPlan(",
        )
        for declaration in obsolete:
            self.assertNotIn(declaration, maintenance)
        self.assertNotIn("\n        SortRecords(records);", snapshot)

        planner = read("Scripts/4_World/TransferZ/TransferZ_SortPlanner.c")
        transactional = read("Scripts/4_World/TransferZ/TransferZ_TransactionalSortPlanner.c")
        self.assertIn("SortRecordsV4", planner)
        self.assertIn("BuildSortPlanFromTargetsV4", planner)
        self.assertIn("SortRecordsV4(originalRecords)", transactional)
        self.assertIn("BuildSortPlanFromTargetsV4", transactional)

    def test_native_split_preferences_are_session_cached(self) -> None:
        shared = read("Scripts/3_Game/TransferZ/TransferZ_Preferences.c")
        client = read("Scripts/5_Mission/TransferZ/TransferZ_ClientState.c")
        split = read("Scripts/4_World/TransferZ/TransferZ_InHandsSplitRouting.c")

        self.assertIn("class TransferZPreferences", shared)
        self.assertNotIn("class TransferZPreferences", client)
        self.assertNotIn("class TransferZSplitPreferences", split)

        load = function_body(client, "protected void LoadPreferences(")
        save = function_body(client, "protected void SavePreferences(")
        sync = function_body(client, "protected void SyncSplitPreferences(")
        ensure = function_body(split, "protected static void EnsureLoaded(")
        resolve = function_body(split, "static EntityAI Resolve(")

        self.assertIn("Failed to load preferences", load)
        self.assertIn("Failed to save preferences", save)
        self.assertIn("TransferZSplitPreferenceResolver.Update", sync)
        self.assertIn("s_LoadAttempted", ensure)
        self.assertIn("JsonFileLoader<TransferZPreferences>.LoadFile", ensure)
        self.assertNotIn("JsonFileLoader", resolve)
        self.assertIn("EnsureLoaded()", resolve)

    def test_batch_preview_does_not_claim_joint_fit_from_area_alone(self) -> None:
        preview = read("Scripts/5_Mission/TransferZ/TransferZ_OperationPreview.c")
        evaluate = function_body(preview, "protected static int EvaluateCandidates(")

        self.assertIn("requiredArea > freeArea", evaluate)
        self.assertIn("if (candidates.Count() > 1)", evaluate)
        self.assertIn("return TransferZOperationPreviewResult.PARTIAL;", evaluate)
        self.assertLess(
            evaluate.index("if (candidates.Count() > 1)"),
            evaluate.rindex("return TransferZOperationPreviewResult.READY;"),
        )

    def test_ui_suppression_is_scoped_to_exact_drag_subject(self) -> None:
        drag = read("Scripts/5_Mission/TransferZ/TransferZ_OperationDrag.c")
        arm = function_body(drag, "static void ArmNativeDropSuppression(")
        consume = function_body(drag, "static bool ConsumeNativeDropSuppression(")
        drop = function_body(drag, "override bool OnDropReceived(")

        self.assertIn("s_TransferZSuppressedNativeDropWidget = draggedWidget", arm)
        self.assertIn("draggedWidget != s_TransferZSuppressedNativeDropWidget", consume)
        self.assertIn("s_TransferZSuppressedNativeDropWidget = null", consume)
        self.assertIn("ConsumeNativeDropSuppression(w)", drop)
        self.assertNotIn("ShouldSuppressNativeDrop()", drag)

        vicinity = read("Scripts/5_Mission/TransferZ/TransferZ_VicinitySlotsContainer.c")
        suppress = function_body(vicinity, "static bool TransferZSuppressModifierClick(")
        click = function_body(vicinity, "override void MouseClick(")

        self.assertIn("clickedItem != s_TransferZModifierClickSuppressItem", suppress)
        self.assertIn("s_TransferZModifierClickSuppressItem = null", suppress)
        self.assertIn("TransferZSuppressModifierClick(clickedItem)", click)
        self.assertNotIn("TransferZSuppressModifierClick()", vicinity)

    def test_single_item_sort_is_not_skipped(self) -> None:
        transactional = read("Scripts/4_World/TransferZ/TransferZ_TransactionalSortPlanner.c")
        sort = function_body(transactional, "static int Sort(")
        self.assertIn("originalRecords.Count() < 1", sort)
        self.assertNotIn("originalRecords.Count() < 2", sort)

        fixture = read("test/TransferZTest.ChernarusPlus/init.c")
        self.assertIn("TZTest_RunSingleItemSortSelfTest(player)", fixture)
        self.assertIn("CreateEntityInCargoEx(typeName, cargoIndex, row, col, flip)", fixture)
        self.assertIn("location.GetRow() == 0 && location.GetCol() == 0", fixture)

    def test_multiplayer_sort_avoids_recursive_parking_explosion(self) -> None:
        planner = read("Scripts/4_World/TransferZ/TransferZ_SortPlanner.c")
        park = function_body(planner, "protected static bool ParkRecordRecursiveV4(")
        direct = park.index("FindTemporaryPlacementV4")
        multiplayer = park.index("if (GetGame().IsMultiplayer())")
        recursive_candidates = park.index("for (int orientationIndex = 0; orientationIndex < orientationCount; orientationIndex++)")
        self.assertLess(direct, multiplayer)
        self.assertLess(multiplayer, recursive_candidates)
        self.assertIn("state.activeParking.Set(recordIndex, 0);", park)
        self.assertNotIn("candidate work budget exhausted", planner)

    def test_sort_reuses_one_target_layout_and_bounds_parking_search(self) -> None:
        planner = read("Scripts/4_World/TransferZ/TransferZ_SortPlanner.c")
        transactional = read("Scripts/4_World/TransferZ/TransferZ_TransactionalSortPlanner.c")
        sort = function_body(transactional, "static int Sort(")
        buffer_fallback = function_body(transactional, "protected static int SortWithNativeBuffer(")
        plan = function_body(planner, "protected static bool BuildSortPlanFromTargetsV4(")
        temporary = function_body(planner, "protected static bool FindTemporaryPlacementV4(")
        recursive = function_body(planner, "protected static bool ParkRecordRecursiveV4(")

        self.assertEqual(sort.count("BuildTargetLayoutV4("), 1)
        self.assertNotIn("BuildTargetLayoutV4(", buffer_fallback)
        self.assertIn("BuildSortPlanFromTargetsV4", sort)
        self.assertIn("layoutRecords, targetWidths, targetHeights, targetFlips", sort)

        self.assertIn("maxCandidateChecks", plan)
        self.assertIn("131072", plan)
        self.assertIn("ConsumePlannerCandidateV4(state)", temporary)
        self.assertIn("ConsumePlannerCandidateV4(state)", recursive)
        self.assertIn("candidateBudgetExceeded", planner)

    def test_sort_keeps_transactional_verification_and_rollback(self) -> None:
        source = read("Scripts/4_World/TransferZ/TransferZ_TransactionalSortPlanner.c")
        body = function_body(source, "static int Sort(")
        self.assertIn("VerifyLayout", body)
        self.assertIn("RollbackExecutedMoves", body)
        self.assertIn("SortWithNativeBuffer", body)

    def test_sort_rollback_emergency_drops_only_unrestored_items(self) -> None:
        planner = read("Scripts/4_World/TransferZ/TransferZ_TransactionalSortPlanner.c")
        emergency = function_body(planner, "protected static bool EmergencyDropUnrestoredItems(")
        recover = function_body(planner, "protected static bool RecoverBufferedSort(")
        fixture = read("test/TransferZTest.ChernarusPlus/init.c")
        self.assertIn("RecordAtCargoLocation(source, record)", emergency)
        self.assertIn("currentParent != source && currentParent != buffer", emergency)
        self.assertIn("record.item.GetInventory().DropEntity(moveMode, player, record.item)", emergency)
        self.assertIn('EmergencyDropUnrestoredItems(player, source, buffer, originalRecords, "rollback-incomplete")', recover)
        self.assertNotIn("EnableRecoveryMode", planner)
        self.assertNotIn("PreserveSortBufferForRecovery", planner)
        self.assertIn("TZTest_RunSortEmergencyDropSelfTest(player)", fixture)

    def test_sort_buffer_is_hidden_and_nonpersistent(self) -> None:
        buffer_source = read("Scripts/4_World/TransferZ/TransferZ_SortBuffer.c")
        planner = read("Scripts/4_World/TransferZ/TransferZ_TransactionalSortPlanner.c")
        create_body = function_body(planner, "protected static TransferZ_SortBuffer CreateSortBuffer(")
        self.assertIn("IsInventoryVisible", buffer_source)
        self.assertIn("return false;", buffer_source)
        self.assertIn("ECE_NOPERSISTENCY_WORLD", create_body)
        self.assertIn("ECE_NOPERSISTENCY_CHAR", create_body)

    def test_sort_buffer_requires_native_source_authorization(self) -> None:
        planner = read("Scripts/4_World/TransferZ/TransferZ_TransactionalSortPlanner.c")
        authorize = function_body(planner, "protected static bool ValidateBufferedSortAuthorization(")
        stage = function_body(planner, "protected static bool TryMoveToSortBuffer(")
        restore = function_body(planner, "protected static bool TryMoveFromSortBuffer(")
        fallback = function_body(planner, "protected static int SortWithNativeBuffer(")

        self.assertIn("GameInventory.CheckRequestSrc", authorize)
        self.assertIn("GameInventory.CheckRequestSrc", stage)
        self.assertIn("TransferZServerService.IsReachable(player, source)", restore)
        self.assertIn("ValidateBufferedSortAuthorization(player, source, originalRecords)", fallback)
        self.assertLess(
            fallback.index("ValidateBufferedSortAuthorization(player, source, originalRecords)"),
            fallback.index("CreateSortBuffer(player)"),
        )

    def test_sort_buffer_capacity_is_a_single_hard_limit(self) -> None:
        planner = read("Scripts/4_World/TransferZ/TransferZ_TransactionalSortPlanner.c")
        config = read("config.cpp")
        fallback = function_body(planner, "protected static int SortWithNativeBuffer(")

        self.assertNotIn("TransferZ_SortBufferWide", config)
        self.assertNotIn("MAX_SORT_BUFFER_COUNT", planner)
        self.assertNotIn("TryMoveToSortBufferPool", planner)
        self.assertEqual(fallback.count("CreateSortBuffer(player)"), 1)
        self.assertIn("if (!TryMoveToSortBuffer(player, source, layoutRecord.cargoIndex, buffer, layoutRecord.item))", fallback)
        self.assertIn("Sort buffer fallback failed during staging", fallback)
        self.assertIn("return -1;", fallback)

    def test_server_request_guard_throttles_and_expires_identity_entries(self) -> None:
        guard = read("Scripts/4_World/TransferZ/TransferZ_RequestGuard.c")
        standard = function_body(guard, "static bool AcceptStandard(")
        maintenance = function_body(guard, "static bool AcceptMaintenance(")
        cleanup = function_body(guard, "protected static void CleanupRequestMap(")

        self.assertIn("STANDARD_REQUEST_THROTTLE_MS = 100", guard)
        self.assertIn("MAINTENANCE_REQUEST_THROTTLE_MS = 250", guard)
        self.assertIn("REQUEST_ENTRY_TTL_MS = 300000", guard)
        self.assertIn("Accept(player, s_LastStandardRequestTime", standard)
        self.assertIn("Accept(player, s_LastMaintenanceRequestTime", maintenance)
        self.assertIn("requestTimes.Remove(key)", cleanup)

        maintenance_service = read("Scripts/4_World/TransferZ/TransferZ_MaintenanceService.c")
        accept_maintenance = function_body(maintenance_service, "protected static bool AcceptServerMaintenanceRequest(")
        self.assertIn("TransferZRequestGuard.AcceptMaintenance(player)", accept_maintenance)
        self.assertNotIn("s_LastMaintenanceRequestTime", maintenance_service)

    def test_vicinity_modifier_batch_uses_one_throttled_rpc(self) -> None:
        constants = read("Scripts/3_Game/TransferZ/TransferZ_Constants.c")
        client = read("Scripts/5_Mission/TransferZ/TransferZ_ClientState.c")
        server = read("Scripts/4_World/TransferZ/TransferZ_ServerService.c")
        fixture = read("test/TransferZTest.ChernarusPlus/init.c")

        self.assertIn("VICINITY_BATCH = 5", constants)

        request = function_body(client, "bool RequestVicinityTransferTo(")
        send_batch = function_body(client, "protected bool SendVicinityBatchRequest(")
        self.assertIn("SendVicinityBatchRequest(candidates, destination, destinationCargoIndex)", request)
        self.assertNotIn("RequestMoveItem(", request)
        self.assertEqual(send_batch.count("rpc.Send("), 1)
        self.assertIn("rpc.Write(items.Count())", send_batch)
        self.assertIn("TransferZOperation.VICINITY_BATCH", send_batch)

        handle = function_body(server, "static void HandleRequest(")
        batch = function_body(server, "static int MoveItemsFromVicinity(")
        self.assertLess(
            handle.index("TransferZRequestGuard.AcceptStandard(player)"),
            handle.index("operation == TransferZOperation.VICINITY_BATCH"),
        )
        self.assertIn("batchCount > MAX_BATCH_ITEMS", handle)
        self.assertIn("batchLocation.GetType() != InventoryLocationType.GROUND", batch)
        self.assertIn("MoveItem(player, batchItem, destination, destinationCargoIndex)", batch)

        self.assertIn("TZTest_RunVicinityBatchSelfTest(player)", fixture)
        self.assertIn("MoveItemsFromVicinity(player, items, destination)", fixture)

    def test_vicinity_unpack_batch_uses_one_throttled_rpc(self) -> None:
        constants = read("Scripts/3_Game/TransferZ/TransferZ_Constants.c")
        client = read("Scripts/5_Mission/TransferZ/TransferZ_ClientState.c")
        server = read("Scripts/4_World/TransferZ/TransferZ_ServerService.c")
        nested = read("Scripts/4_World/TransferZ/TransferZ_NestedUnpackService.c")
        fixture = read("test/TransferZTest.ChernarusPlus/init.c")

        self.assertIn("VICINITY_UNPACK_BATCH = 6", constants)
        send_batch = function_body(client, "protected bool SendVicinityUnpackBatchRequest(")
        self.assertEqual(send_batch.count("rpc.Send("), 1)
        self.assertIn("rpc.Write(sources.Count())", send_batch)

        unpack_to = function_body(client, "bool RequestVicinityUnpackTo(")
        unpack_vicinity = function_body(client, "bool RequestVicinityUnpackToVicinity(")
        self.assertIn("SendVicinityUnpackBatchRequest(sources, destination, destinationCargoIndex, false)", unpack_to)
        self.assertIn("SendVicinityUnpackBatchRequest(sources, null, 0, true)", unpack_vicinity)
        self.assertNotIn("RequestNestedUnpackTo(", unpack_to)
        self.assertNotIn("RequestNestedUnpackToVicinity(", unpack_vicinity)

        handle = function_body(server, "static void HandleRequest(")
        self.assertIn("operation == TransferZOperation.VICINITY_UNPACK_BATCH", handle)
        self.assertIn("unpackBatchCount > MAX_BATCH_ITEMS", handle)
        self.assertIn("TransferZNestedUnpackService.UnpackMany", handle)

        unpack_many = function_body(nested, "static int UnpackMany(")
        self.assertIn("TransferZUnpackScanBudget", unpack_many)
        self.assertIn("CollectNestedLeaves", unpack_many)
        self.assertIn("TryMoveToExactCargo", unpack_many)
        self.assertIn("TryMoveToVicinity", unpack_many)

        self.assertIn("TZTest_RunVicinityUnpackBatchSelfTest(player)", fixture)
        self.assertIn("UnpackMany(player, sources, destination, 0, false)", fixture)

    def test_standard_rpc_throttles_before_entity_resolution(self) -> None:
        server = read("Scripts/4_World/TransferZ/TransferZ_ServerService.c")
        handle = function_body(server, "static void HandleRequest(")
        self.assertIn("TransferZRequestGuard.AcceptStandard(player)", handle)
        self.assertLess(
            handle.index("TransferZRequestGuard.AcceptStandard(player)"),
            handle.index("ResolveEntity(sourceLow, sourceHigh)"),
        )

        nested = read("Scripts/4_World/TransferZ/TransferZ_NestedUnpackService.c")
        nested_handle = function_body(nested, "static void HandleRequest(")
        self.assertIn("TransferZRequestGuard.AcceptStandard(player)", nested_handle)
        self.assertLess(
            nested_handle.index("TransferZRequestGuard.AcceptStandard(player)"),
            nested_handle.index("ResolveEntity(sourceLow, sourceHigh)"),
        )

    def test_batch_and_unpack_work_are_bounded_before_mutation(self) -> None:
        server = read("Scripts/4_World/TransferZ/TransferZ_ServerService.c")
        self.assertIn("MAX_BATCH_ITEMS = 1024", server)
        self.assertIn("MAX_UNPACK_SCAN_NODES = 2048", server)
        self.assertIn("MAX_UNPACK_DEPTH = 32", server)

        transfer = function_body(server, "static int Transfer(")
        self.assertIn("DirectBatchWithinBudget", transfer)
        self.assertLess(
            transfer.index("DirectBatchWithinBudget"),
            transfer.index("SnapshotDirectCargo"),
        )

        self.assertNotIn("static int Unpack(", server)
        self.assertNotIn("CollectUnpackLeavesForOperation", server)

        collect = function_body(server, "static bool CollectUnpackLeaves(")
        self.assertIn("depth > MAX_UNPACK_DEPTH", collect)
        self.assertIn("ConsumeUnpackScanNode", collect)
        self.assertIn("AppendUnpackLeaf", collect)

        nested = read("Scripts/4_World/TransferZ/TransferZ_NestedUnpackService.c")
        nested_collect = function_body(nested, "static bool CollectNestedLeaves(")
        nested_unpack = function_body(nested, "static int Unpack(")
        self.assertIn("ConsumeUnpackScanNode", nested_collect)
        self.assertIn("CollectUnpackLeaves", nested_collect)
        self.assertIn("TransferZUnpackScanBudget", nested_unpack)
        self.assertIn("CollectNestedLeaves", nested_unpack)
        self.assertLess(
            nested_unpack.index("CollectNestedLeaves"),
            nested_unpack.index("TryMoveToExactCargo"),
        )

    def test_rpc_entry_checks_player_state_and_service_rechecks_sender(self) -> None:
        dispatcher = read("Scripts/4_World/TransferZ/TransferZ_CFModule.c")
        dispatcher_body = function_body(dispatcher, "override void OnRPC(")
        self.assertIn("TransferZServerService.CanPlayerManipulate(player)", dispatcher_body)

        service = read("Scripts/4_World/TransferZ/TransferZ_ServerService.c")
        service_body = function_body(service, "static void HandleRequest(")
        self.assertIn("sender.GetId()", service_body)
        self.assertIn("playerIdentity.GetId()", service_body)

    def test_cargo_identity_uses_owner_and_grid_index(self) -> None:
        cargo = read("Scripts/3_Game/TransferZ/TransferZ_Cargo.c")
        self.assertIn("GetCargoFromIndex(cargoIndex)", cargo)
        self.assertIn("cargo.GetCargoOwner() != owner", cargo)
        self.assertIn("cargo.GetOwnerCargoIndex() != cargoIndex", cargo)
        self.assertIn("location.GetIdx() == cargoIndex", cargo)
        self.assertIn("candidate.SetCargo(owner, item, cargoIndex", cargo)

    def test_header_preserves_vanilla_cargo_index(self) -> None:
        source = read("Scripts/5_Mission/TransferZ/TransferZ_CargoContainer.c")
        body = function_body(source, "override void SetEntity(EntityAI item, int cargo_index")
        self.assertIn("SetEntity(item, cargo_index)", body)

    def test_transfer_rpc_carries_both_cargo_indices(self) -> None:
        client = read("Scripts/5_Mission/TransferZ/TransferZ_ClientState.c")
        send = function_body(client, "protected void SendRequest(")
        self.assertIn("rpc.Write(sourceCargoIndex)", send)
        self.assertIn("rpc.Write(destinationCargoIndex)", send)

        server = read("Scripts/4_World/TransferZ/TransferZ_ServerService.c")
        handle = function_body(server, "static void HandleRequest(")
        self.assertIn("ctx.Read(sourceCargoIndex)", handle)
        self.assertIn("ctx.Read(destinationCargoIndex)", handle)

    def test_sort_and_stack_are_scoped_to_requested_grid(self) -> None:
        maintenance = read("Scripts/4_World/TransferZ/TransferZ_MaintenanceService.c")
        snapshot = function_body(maintenance, "protected static bool SnapshotSortRecords(")
        stack = function_body(maintenance, "static int Stack(")
        self.assertIn("TransferZCargo.Get(source, sourceCargoIndex)", snapshot)
        self.assertIn("TransferZCargo.LocationMatches", snapshot)
        self.assertIn("TransferZCargo.Get(source, sourceCargoIndex)", stack)

        planner = read("Scripts/4_World/TransferZ/TransferZ_TransactionalSortPlanner.c")
        sort = function_body(planner, "static int Sort(")
        self.assertIn("SnapshotSortRecords(source, sourceCargoIndex", sort)

    def test_maintenance_result_feedback_is_grid_specific(self) -> None:
        service = read("Scripts/4_World/TransferZ/TransferZ_MaintenanceService.c")
        send = function_body(service, "protected static void SendResult(")
        self.assertIn("rpc.Write(sourceCargoIndex)", send)

        state = read("Scripts/4_World/TransferZ/TransferZ_MaintenanceResultState.c")
        handle = function_body(state, "static void HandleRPC(")
        self.assertIn("ctx.Read(sourceCargoIndex)", handle)
        self.assertIn("sourceCargoIndex == s_SourceCargoIndex", state)

        feedback = read("Scripts/5_Mission/TransferZ/TransferZ_SortFeedback.c")
        self.assertIn("MatchesSource(m_Entity, m_CargoIndex)", feedback)
        self.assertIn("RequestSort(m_Entity, m_CargoIndex)", feedback)
        self.assertIn("override void SetEntity(EntityAI entity, int cargoIndex = 0)", feedback)

    def test_native_split_routing_carries_cargo_index(self) -> None:
        source = read("Scripts/4_World/TransferZ/TransferZ_InHandsSplitRouting.c")
        route = function_body(source, "protected bool TransferZRouteNativeSplit(")
        self.assertIn("GetCargoIndex()", route)
        self.assertIn("sourceCargoIndex", route)
        self.assertIn("preferredCargoIndex", route)

    def test_nested_unpack_uses_descriptive_source_filename(self) -> None:
        self.assertTrue((ROOT / "Scripts/4_World/TransferZ/TransferZ_NestedUnpackService.c").is_file())
        self.assertFalse((ROOT / "Scripts/4_World/TransferZ/TransferZ_ServerService_20_NestedUnpack.c").exists())

    def test_ui_unpack_uses_only_nested_unpack_service(self) -> None:
        client = read("Scripts/5_Mission/TransferZ/TransferZ_ClientState.c")
        server = read("Scripts/4_World/TransferZ/TransferZ_ServerService.c")
        cargo_ui = read("Scripts/5_Mission/TransferZ/TransferZ_CargoContainer.c")
        drag = read("Scripts/5_Mission/TransferZ/TransferZ_OperationDrag.c")
        fixture = read("test/TransferZTest.ChernarusPlus/init.c")

        self.assertNotIn("bool RequestUnpackTo(", client)
        self.assertNotIn("bool RequestUnpackToVicinity(", client)
        self.assertNotIn("bool RequestUnpack(", client)
        self.assertNotIn("TransferZServerService.Unpack", client)
        self.assertNotIn("TransferZOperation.UNPACK", function_body(server, "static void HandleRequest("))
        self.assertIn("RequestNestedUnpack(m_Entity, m_CargoIndex)", cargo_ui)
        self.assertIn("RequestNestedUnpackTo(", drag)
        self.assertIn("RequestNestedUnpackToVicinity(", drag)
        self.assertIn("TransferZNestedUnpackService.UnpackMany(player, sources, destination", client)
        self.assertIn("TransferZNestedUnpackService.UnpackMany(player, sources, null, 0, true)", client)
        self.assertIn("SendVicinityUnpackBatchRequest(sources, destination", client)
        self.assertIn("SendVicinityUnpackBatchRequest(sources, null, 0, true)", client)
        self.assertIn("TransferZNestedUnpackService.Unpack(player, source, destination)", fixture)

    def test_configurable_inputs_are_packaged_and_runtime_code_is_keycode_free(self) -> None:
        config = read("config.cpp")
        inputs = read("inputs.xml")
        stringtable = read("stringtable.csv")
        build = read("tools/build-pbo.ps1")
        release_build = read("tools/build.ps1")
        input_source = read("Scripts/3_Game/TransferZ/TransferZ_Input.c")
        mission_input = read("Scripts/5_Mission/TransferZ/TransferZ_InputCommands.c")
        icon = read("Scripts/5_Mission/TransferZ/TransferZ_Icon.c")
        vicinity = read("Scripts/5_Mission/TransferZ/TransferZ_VicinitySlotsContainer.c")
        cargo = read("Scripts/5_Mission/TransferZ/TransferZ_CargoContainer.c")

        self.assertIn('inputs = "TransferZ/inputs.xml";', config)
        self.assertIn('<sorting name="transferz" loc="STR_TRANSFERZ_INPUT_GROUP">', inputs)
        for action in (
            "UATransferZTransferModifier",
            "UATransferZClassModifier",
            "UATransferZUnpackModifier",
            "UATransferZDestination",
            "UATransferZTransfer",
            "UATransferZUnpack",
            "UATransferZLink",
            "UATransferZPreferred",
            "UATransferZSort",
            "UATransferZStack",
        ):
            self.assertIn(f'name="{action}"', inputs)
            self.assertIn(action, input_source)

        self.assertIn('<btn name="kLShift" />', inputs)
        self.assertIn('<btn name="kRShift" />', inputs)
        self.assertIn('<btn name="kLMenu" />', inputs)
        self.assertIn('<btn name="kRMenu" />', inputs)
        self.assertIn('<btn name="kU" />', inputs)
        self.assertIn("STR_TRANSFERZ_INPUT_GROUP", stringtable)
        self.assertIn("'inputs.xml'", build)
        self.assertIn("'stringtable.csv'", build)
        self.assertIn("'.xml'", build)
        self.assertIn("'.csv'", build)
        self.assertIn("@('inputs.xml', 'stringtable.csv')", release_build)
        self.assertIn("Release PBO is missing required runtime asset", release_build)

        self.assertNotIn("KC_LSHIFT", icon)
        self.assertNotIn("KC_RSHIFT", icon)
        self.assertNotIn("KC_LMENU", icon)
        self.assertNotIn("KC_RMENU", icon)
        self.assertNotIn("KC_LSHIFT", vicinity)
        self.assertNotIn("KC_RSHIFT", vicinity)
        self.assertNotIn("KC_LMENU", vicinity)
        self.assertNotIn("KC_RMENU", vicinity)
        self.assertIn("TransferZInput.ModifierMode()", icon)
        self.assertIn("TransferZInput.ModifierMode()", vicinity)
        self.assertIn("TransferZOperation.UNPACK", icon)
        self.assertIn("TransferZOperation.UNPACK, m_Obj, 0", vicinity)
        self.assertIn("InventoryMenu.Cast(g_Game.GetUIManager().FindMenu(MENU_INVENTORY))", mission_input)
        self.assertIn("TransferZInput.PressedCommand()", mission_input)
        self.assertLess(mission_input.index("FindMenu(MENU_INVENTORY)"), mission_input.index("TransferZInput.PressedCommand()"))
        self.assertIn("ExecuteInputCommandAtMousePosition(command)", mission_input)
        self.assertIn("TransferZInputCommand.SORT", cargo)
        self.assertIn("TransferZInputCommand.UNPACK", vicinity)

    def test_diag_fixture_has_machine_readable_suite_marker(self) -> None:
        fixture = read("test/TransferZTest.ChernarusPlus/init.c")
        self.assertIn("[TransferZTest] SUITE PASS", fixture)
        self.assertIn("[TransferZTest] SUITE FAIL", fixture)
        self.assertIn("[TransferZTest] RUN unpack", fixture)
        self.assertIn("CreateEntityInCargo(typeName)", fixture)
        self.assertIn('TZTest_CreateCargoItem(nested, "BandageDressing")', fixture)
        self.assertIn('TZTest_CreateCargoItem(nested, "Battery9V")', fixture)
        self.assertIn("TZTest_RunSelfTests", fixture)


if __name__ == "__main__":
    unittest.main()
