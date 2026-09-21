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

    def test_sort_keeps_transactional_verification_and_rollback(self) -> None:
        source = read("Scripts/4_World/TransferZ/TransferZ_TransactionalSortPlanner.c")
        body = function_body(source, "static int Sort(")
        self.assertIn("VerifyLayout", body)
        self.assertIn("RollbackExecutedMoves", body)
        self.assertIn("SortWithNativeBuffer", body)

    def test_sort_buffer_is_hidden_and_nonpersistent(self) -> None:
        buffer_source = read("Scripts/4_World/TransferZ/TransferZ_SortBuffer.c")
        planner = read("Scripts/4_World/TransferZ/TransferZ_TransactionalSortPlanner.c")
        create_body = function_body(planner, "protected static TransferZ_SortBuffer CreateSortBuffer(")

        self.assertIn("IsInventoryVisible", buffer_source)
        self.assertIn("return false;", buffer_source)
        self.assertIn("ECE_NOPERSISTENCY_WORLD", create_body)
        self.assertIn("ECE_NOPERSISTENCY_CHAR", create_body)

    def test_rpc_entry_checks_player_state_and_service_rechecks_sender(self) -> None:
        dispatcher = read("Scripts/4_World/TransferZ/TransferZ_CFModule.c")
        dispatcher_body = function_body(dispatcher, "override void OnRPC(")
        self.assertIn("TransferZServerService.CanPlayerManipulate(player)", dispatcher_body)

        service = read("Scripts/4_World/TransferZ/TransferZ_ServerService.c")
        service_body = function_body(service, "static void HandleRequest(")
        self.assertIn("sender.GetId()", service_body)
        self.assertIn("playerIdentity.GetId()", service_body)

    def test_diag_fixture_has_machine_readable_suite_marker(self) -> None:
        fixture = read("test/TransferZTest.ChernarusPlus/init.c")
        self.assertIn("[TransferZTest] SUITE PASS", fixture)
        self.assertIn("[TransferZTest] SUITE FAIL", fixture)
        self.assertIn("TZTest_RunSelfTests", fixture)


if __name__ == "__main__":
    unittest.main()
