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

## Validation

The active stack was rebuilt from the original known-good PR trees after review, overlaying only the accepted rollback policy and removing #49. Fresh GitHub Self-test runs passed on every rewritten active head: #44 run 169, #45 run 184, #46 run 186, #47 run 188, #48 run 190, #50 run 192, #51 run 194, and #52 run 196.

A first real user-run DayZDiag compile of the audit branch exposed Enforce Script rejecting the C/C++-style empty-condition loop `for (...; ; ...)` in `TransferZ_ServerService.c`. The same construct was also found in `TransferZ_OperationPreview.c`. Both runtime occurrences were rewritten as explicit `while (true)` loops, and the repository contracts now scan all runtime/test `.c` files to prevent that parser-incompatible pattern from returning.

The next real DayZDiag run compiled successfully and started the suite. Transfer completed and preserved item identity, then execution stopped when Unpack began. The first fix made `TransferZCargo.Get()` reject a returned cargo whose native `GetOwnerCargoIndex()` does not match the requested grid index.

A subsequent DayZDiag run completed the entire suite rather than hanging. Transfer, exact-class transfer, Stack, Sort, and the emergency ground-drop primitive all passed. Unpack alone failed four assertions: moved-leaf count, preservation of the direct loose Apple, and both nested-leaf destination checks. This narrowed the remaining defect to cargo-bearing classification during recursive Unpack. The native `CargoBase` API exposes both `GetCargoOwner()` and `GetOwnerCargoIndex()`; exact cargo identity therefore now validates both owner entity and grid index. The Unpack fixture also asserts that a loose Apple exposes no cargo, the nested protector does expose cargo 0, and cargo 1 is rejected before the operation runs.

The connector environment still cannot itself claim a complete DayZ runtime pass. The repository contains `tools/run-transferz-self-test.ps1` and the DayZDiag fixture for the exact-build runtime gate on a Windows machine with DayZ/DayZDiag and CF installed.

## Decisions retained

- `main` is not to be updated without explicit merge approval.
- One fixed Sort buffer is sufficient by design; failure to stage within it is a normal Sort failure.
- Rollback invariant failure uses emergency ground drop, not a recovery container.
- Vehicle cargo reach remains governed by DayZ native inventory authority; issue #30 records the no-extra-distance-formula decision.

## Next concrete action

The rewritten active stack is green and ready for explicit merge approval. Do not merge without that approval.
