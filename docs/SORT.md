# Sort behavior

TransferZ Sort is a server-authoritative rearrangement of a container's direct cargo children. It never deletes or recreates sorted items: the same `EntityAI` objects are moved through DayZ's native inventory system between exact cells of the source cargo until the planned layout is reached.

## Layout policy

The target layout is deterministic and rotation-aware.

- The target packer scans the cargo from the first free cell at the top-left downward.
- Sort first attempts a complete compact layout with **every item kept in its current orientation**. A horizontal saw stays horizontal; a vertical spray can stays vertical.
- If that cannot produce a complete layout, the fallback packers may rotate non-square items. Even then, an unrotated candidate is preferred at an anchor, and first-fit searches the entire cargo in the current orientation before trying the alternate orientation.
- Detachable magazines retain their vertical-orientation preference only as a secondary fallback after an all-current-orientation layout has failed.
- Rotation is therefore a packing escape hatch for tight layouts, not a normal part of tidying an already roomy container.
- Equivalent-size target assignments are optimized before movement so interchangeable shapes do not create unnecessary identity swaps.
- Smaller items fill otherwise wasted gaps in the compact target layout; there is no dedicated bottom zone for small items.
- DayZ user-reserved inventory cells are treated as unavailable.
- If the compact anchor packer cannot assign every item, Sort falls back to a deterministic rotation-aware first-fit target layout rather than failing solely because of the packing heuristic.

## Transactional execution

Sort snapshots the exact original row, column and orientation of every direct cargo child before any move. It computes the complete target layout and a bounded rearrangement plan before execution. If the target layout already matches the snapshot, Sort is a successful no-op.

Planning is entirely virtual. A cloned record set is used while the planner resolves blockers and cycles, so the authoritative original snapshot remains unchanged for rollback verification. The planner uses genuinely free cells inside the **same cargo grid** as temporary parking when a direct target move is blocked. When a target is occupied by one compatible equal-size item, TransferZ can instead plan DayZ's native atomic inventory swap; this exchanges the two cargo locations without requiring any empty intermediary cell.

Sort never uses the ground/vicinity, player inventory, or another container as hidden temporary storage. If there is not enough in-cargo workspace for the bounded planner to reach the target layout, Sort fails before moving the first item.

Execution is a sequence of exact cargo-to-cargo moves of the same original entity objects. Before each successful move, TransferZ records the item's exact current row, column and orientation as the inverse operation. Each dedicated-server move is committed synchronously through the moved item's generic `GameInventory`, so the next step validates against the state actually produced by the previous step.

If a move fails or the final target verification fails, the successfully executed operations are reversed in strict reverse order. The original snapshot is then verified before Sort returns failure. A normal failed Sort therefore restores the exact original layout; a failure of the reverse sequence is logged as a `CRITICAL rollback incomplete` invariant violation.

Because items never leave their source cargo during Sort, normal world-drop physics and vicinity/drop side effects are not part of sorting. Chamber contents, attachments and mod-defined entity state remain preserved by object identity rather than manual serialization or reconstruction.

## UI feedback

A failed Sort request is reported back to the requesting client. The Sort button's full background flashes red for 500 ms and is cleared by a scheduled GUI callback. An already-sorted container or a container with fewer than two direct cargo items is a successful no-op and does not show the failure flash.
