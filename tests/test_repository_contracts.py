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
    def test_version_is_synchronized(self) -> None:
        version = read("VERSION").strip()
        config = read("config.cpp")
        mod = read("mod.cpp")

        config_match = re.search(r'\bversion\s*=\s*"([^"]+)"', config)
        mod_match = re.search(r'\bversion\s*=\s*"([^"]+)"', mod)
        self.assertIsNotNone(config_match)
        self.assertIsNotNone(mod_match)
        self.assertEqual(version, config_match.group(1))
        self.assertEqual(version, mod_match.group(1))

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

    def test_standard_rpc_throttles_before_entity_resolution(self) -> None:
        server = read("Scripts/4_World/TransferZ/TransferZ_ServerService.c")
        handle = function_body(server, "static void HandleRequest(")
        self.assertIn("TransferZRequestGuard.AcceptStandard(player)", handle)
        self.assertLess(
            handle.index("TransferZRequestGuard.AcceptStandard(player)"),
            handle.index("ResolveEntity(sourceLow, sourceHigh)"),
        )

        nested = read("Scripts/4_World/TransferZ/TransferZ_ServerService_20_NestedUnpack.c")
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

        unpack = function_body(server, "static int Unpack(")
        self.assertIn("TransferZUnpackScanBudget", unpack)
        self.assertIn("CollectUnpackLeavesForOperation", unpack)
        self.assertLess(
            unpack.index("CollectUnpackLeavesForOperation"),
            unpack.index("TryMoveToExactCargo"),
        )

        collect = function_body(server, "static bool CollectUnpackLeaves(")
        self.assertIn("depth > MAX_UNPACK_DEPTH", collect)
        self.assertIn("ConsumeUnpackScanNode", collect)
        self.assertIn("AppendUnpackLeaf", collect)

        nested = read("Scripts/4_World/TransferZ/TransferZ_ServerService_20_NestedUnpack.c")
        nested_collect = function_body(nested, "static bool CollectNestedLeaves(")
        self.assertIn("ConsumeUnpackScanNode", nested_collect)
        self.assertIn("CollectUnpackLeaves", nested_collect)

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

    def test_diag_fixture_has_machine_readable_suite_marker(self) -> None:
        fixture = read("test/TransferZTest.ChernarusPlus/init.c")
        self.assertIn("[TransferZTest] SUITE PASS", fixture)
        self.assertIn("[TransferZTest] SUITE FAIL", fixture)
        self.assertIn("TZTest_RunSelfTests", fixture)


if __name__ == "__main__":
    unittest.main()
