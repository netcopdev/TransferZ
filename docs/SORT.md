# Sort behavior

TransferZ Sort is a server-authoritative rearrangement of one selected cargo grid. It never deletes or recreates sorted items: the same `EntityAI` objects are moved through DayZ's native inventory system between exact cells of that grid until the planned layout is reached.

## Layout policy

The target layout is deterministic and rotation-aware.

- The target packer scans the cargo from the first free cell at the top-left downward.
- Sort first attempts a complete compact layout with **every item kept in its current orientation**. A horizontal saw stays horizontal; a vertical spray can stays vertical.
- If that cannot produce a complete layout, the fallback packers may rotate non-square items. Even then, an unrotated candidate is preferred at an anchor, and first-fit searches the entire cargo in the current orientation before trying the alternate orientation.
- Detachable magazines retain their vertical-orientation preference only as a secondary fallback after an all-current-orientation layout has failed.
- Rotation is therefore a packing escape hatch for tight layouts, not a normal part of tidying an already roomy container.
- When packing geometry and orientation are equivalent, identical item classes are kept in contiguous runs where possible instead of being interleaved with other same-footprint items.
- Equivalent-size target assignments are optimized before movement only within the same item class, so movement minimization does not undo that grouping.
- Smaller items fill otherwise wasted gaps in the compact target layout; there is no dedicated bottom zone for small items.
- DayZ user-reserved inventory cells are treated as unavailable.
- If the compact anchor packer cannot assign every item, Sort falls back to a deterministic rotation-aware first-fit target layout rather than failing solely because of the packing heuristic.

## Transactional execution

Sort snapshots the exact original cargo index, row, column and orientation of every item in the selected grid before any move. It computes the complete target layout and a bounded rearrangement plan before execution. If the target layout already matches the snapshot, Sort is a successful no-op.

Planning is entirely virtual. A cloned record set is used while the planner resolves blockers and cycles, so the authoritative original snapshot remains unchanged for rollback verification. The planner uses genuinely free cells inside the **same cargo grid** as temporary parking when a direct target move is blocked. In single-player, when a target is occupied by one compatible equal-size item of the same class, TransferZ can instead plan DayZ's synchronous native atomic inventory swap. Multiplayer/dedicated-server Sort deliberately does not plan direct swaps because DayZ's server swap command is asynchronous while the transactional executor verifies each step immediately. Multiplayer therefore resolves blockers through ordinary in-cargo evacuation moves and falls back to the transactional hidden buffer when no bounded in-cargo path exists.

TransferZ first tries that bounded in-cargo plan. If the final layout is valid but the source grid cannot provide enough intermediary workspace, Sort falls back to a dedicated `TransferZ_SortBuffer`. The buffer is a hidden, non-interactive, non-physical native cargo entity created server-authoritatively beside the player for only that transaction. Non-stationary items are moved into its cargo, then moved directly into their exact final source-cargo cells. Ground/vicinity and arbitrary player/world containers are never used as Sort workspace.

Before the buffer is created, every tracked item must still be in the requested source grid and must pass DayZ's native `GameInventory.CheckRequestSrc` anti-cheat/source-authorization check. The same native source check is repeated immediately before each source → buffer stage. The hidden buffer itself is **not** treated as a player destination: it is server-internal workspace and deliberately has no visible/interactive cargo. Applying a player destination request check to that internal endpoint would model the wrong trust boundary and can reject valid fallback Sorts. Buffer → source placement instead revalidates the real source owner as reachable and still requires normal `CanRemoveEntity`, `CanReleaseCargo`/`CanReceiveItemIntoCargo`, reservation, juncture and `LocationCanMoveEntity` checks.

Both paths move the same original entity objects through DayZ's native inventory system. The buffer path therefore preserves chambers, attachments, nested cargo, quantities and mod-defined entity state by object identity rather than serialization/reconstruction. The buffer is networked so clients observe a valid native inventory parent while an item is staged; it is not a local-only phantom parent.

If buffered staging or placement fails, TransferZ uses the immutable original snapshot to evacuate displaced tracked items back into the buffer as necessary and restore exact original row/column/orientation. The buffer is deleted only after the original or final layout has been verified and its cargo is empty. A non-empty buffer is never deleted; failure to restore the exact snapshot is logged as `CRITICAL rollback incomplete`.

Normal world-drop physics and vicinity/drop callbacks are therefore not part of sorting. A hard server-process termination during the short buffered transaction cannot be made fully atomic in script, but normal execution never deletes/recreates the player's items.

## UI feedback

A failed Sort request is reported back to the requesting client together with the owning entity and cargo-grid index. The matching Sort button's full background flashes red for 500 ms and is cleared by a scheduled GUI callback. An already-sorted container or a container with fewer than two direct cargo items is a successful no-op and does not show the failure flash.
