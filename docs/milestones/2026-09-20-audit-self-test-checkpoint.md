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

The active stack was rebuilt from the original known-good PR trees after review, overlaying only the accepted rollback policy and removing #49. Fresh CI is required on the rewritten heads before merge.

No real DayZ compile/runtime is claimed for this checkpoint from the connector environment. The repository contains `tools/run-transferz-self-test.ps1` and the DayZDiag fixture for an exact-build runtime gate on a Windows machine with DayZ/DayZDiag and CF installed.

## Decisions retained

- `main` is not to be updated without explicit merge approval.
- One fixed Sort buffer is sufficient by design; failure to stage within it is a normal Sort failure.
- Rollback invariant failure uses emergency ground drop, not a recovery container.
- Vehicle cargo reach remains governed by DayZ native inventory authority; issue #30 records the no-extra-distance-formula decision.

## Next concrete action

Verify fresh CI for the rewritten active stack. If green, it is ready for explicit merge approval. Do not merge without that approval.
