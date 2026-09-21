# TransferZ audit/self-test checkpoint — 2026-09-20

Branch: `cleanup/remaining-audit-findings`

Active PR stack: **#41–#48, then #50–#52**. PR #49 was closed after review and is intentionally not part of the stack.

## Completed

- Added the automated self-test foundation and isolated DayZDiag fixture.
- Preserved exact cargo-grid identity across client/server maintenance and routing paths.
- Added native authorization for hidden Sort staging.
- Sort rollback attempts exact restoration first. If that invariant itself fails, only unrestored tracked items are emergency-dropped beside the player through DayZ's native inventory path. No persistent recovery crate is created.
- Added server-side request throttling, expiry, and bounded batch/Unpack work.
- Removed the obsolete pre-V4 Sort planner and duplicate pre-sort work.
- Bounded V4 parking-search work and reused one target layout through fallback.
- Kept Sort fallback deliberately bounded to the original single fixed `TransferZ_SortBuffer`; if staging does not fit, Sort rolls back and fails.
- Closed #49 after review: guarding against a theoretical 32-bit class-hash collision was not worth additional runtime/test complexity here.
- Scoped post-drag suppression to the exact dragged/clicked subject.
- Made multi-item cargo previews conservative when joint packing is not proven.
- Cached preferred routing state for native stack splits instead of rereading JSON on each split attempt; preference load/save failures are logged.
- Removed the numeric nested-Unpack source-order filename and stale vicinity comment.
- Made `VERSION` the canonical release-version source and added `tools/version_metadata.py`.
- Fixed Sort so a single item is compacted to the deterministic top-left target instead of being treated as a no-op.
- On multiplayer/dedicated servers, Sort now abandons recursive blocker-parking as soon as no direct temporary placement exists and uses the existing bounded native buffer fallback instead. Routine planner candidate-budget exhaustion is no longer logged as an error-like event when fallback is available.

## Validation

The active stack was rebuilt from the original known-good PR trees after review, overlaying only the accepted rollback policy and removing #49. Fresh GitHub Self-test runs passed on every rewritten active head: #44 run 169, #45 run 184, #46 run 186, #47 run 188, #48 run 190, #50 run 192, #51 run 194, and #52 run 196.

A first real user-run DayZDiag compile of the audit branch exposed Enforce Script rejecting the C/C++-style empty-condition loop `for (...; ; ...)` in `TransferZ_ServerService.c`. The same construct was also found in `TransferZ_OperationPreview.c`. Both runtime occurrences were rewritten as explicit `while (true)` loops, and the repository contracts now scan all runtime/test `.c` files to prevent that parser-incompatible pattern from returning.

The next real DayZDiag run compiled successfully and started the suite. Transfer completed and preserved item identity, then execution stopped when Unpack began. The first fix made `TransferZCargo.Get()` reject a returned cargo whose native `GetOwnerCargoIndex()` does not match the requested grid index.

A subsequent DayZDiag run completed the entire suite rather than hanging. Transfer, exact-class transfer, Stack, Sort, and the emergency ground-drop primitive all passed. The three cargo-identity fixture checks also passed. The four remaining Unpack failures then exposed a test-rig mistake rather than the active UI path: the fixture was calling the obsolete duplicate `TransferZServerService.Unpack()`, whose semantics also incorrectly included direct loose cargo. Production header/drag Unpack already routes through `TransferZNestedUnpackService`. The duplicate generic server/client Unpack path has now been removed. A repository-contract failure then exposed two remaining vicinity-Unpack call sites still pointing at that obsolete API; those were switched to `TransferZNestedUnpackService` / `RequestNestedUnpack*` as well. Repository contracts now lock header, drag, and vicinity Unpack to the same production nested service, and the DayZDiag fixture exercises that production service directly with additional fixture-validity checks.

The latest user-run DayZDiag pass reached the production nested-Unpack service, but the fixture itself failed to create the nested BandageDressing and Battery9V, so the remaining Unpack failures were not evidence against production Unpack behavior. The fixture now uses DayZ's direct `CreateEntityInCargo()` API for those nested cargo items instead of the broader `CreateInInventory()` search.

The following user-run DayZDiag preflight on head `3a504a74af23fe5391920e2062c5c7071427d04d` reached `[TransferZTest] SUITE PASS`. Subsequent in-game testing found two Sort issues not covered by that fixture: a one-item cargo was skipped entirely, and a small dedicated-server sort could exhaust the 16,384-candidate recursive parking budget before falling back. The branch now includes a runtime single-item top-left regression and a multiplayer planner guard that prefers the bounded native buffer over combinatorial recursive parking.

The connector environment still cannot itself claim a complete DayZ runtime pass. The repository contains `tools/run-transferz-self-test.ps1` and the DayZDiag fixture for the exact-build runtime gate on a Windows machine with DayZ/DayZDiag and CF installed.

## Decisions retained

- `main` is not to be updated without explicit merge approval **after the user has completed in-game testing**. Automated repository tests and DayZDiag results are preflight evidence only and never authorize a merge.
- One fixed Sort buffer is sufficient by design; failure to stage within it is a normal Sort failure.
- Rollback invariant failure uses emergency ground drop, not a recovery container.
- Vehicle cargo reach remains governed by DayZ native inventory authority; issue #30 records the no-extra-distance-formula decision.

## Next concrete action

Continue DayZDiag/preflight fixes as needed, then wait for the user's in-game testing. Do not merge anything to `main` until the user explicitly approves the merge after that in-game test.
