# Sort behavior

TransferZ Sort is a server-authoritative rearrangement of a container's direct cargo children. It never deletes or recreates sorted items: the same `EntityAI` objects are moved through DayZ's native inventory system from their current location to temporary staging and then back into the source cargo.

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

Sort snapshots the exact original row, column and orientation of every direct cargo child before any move. It computes the complete target layout before execution and is a no-op when that layout already matches the snapshot.

When movement is required, Sort temporarily stages every tracked direct cargo item through DayZ's normal vicinity/ground drop path. Each staging move is immediately verified as a reachable ground location. Staging the complete set deliberately empties the source cargo, which removes in-cargo move cycles and makes both final placement and rollback deterministic. Player inventory is not used as an implicit staging area.

With the source empty, Sort first preflights every exact **original** move from the current staged state. This proves the normal rollback path before any target placement is allowed to begin. It then preflights every exact target move. Only when both complete layouts pass native DayZ validation does Sort start committing target placements. The same original item entities are then moved back into the source at their exact target rows, columns and orientations. No weapon, magazine, container or other item is copied or reconstructed.

Success is reported only after every tracked item is verified at its target location. If staging, rollback preflight, target preflight, target placement, or final verification fails, Sort enters synchronous rollback:

- any tracked item currently in the source at a non-original location is evacuated again;
- every tracked item is restored to its exact original row, column and orientation from the snapshot;
- rollback is verified before the failed operation returns;
- no tracked item is intentionally left in vicinity;
- if DayZ unexpectedly refuses an exact rollback move after repeated recovery passes, TransferZ performs a final containment attempt back into the source and emits a `CRITICAL rollback incomplete` diagnostic. This is treated as a hard invariant violation, not a successful or acceptable partial sort.

The rollback design means a normal failed Sort should leave the source exactly as it was before the button was pressed. Absolute recovery still depends on DayZ's native inventory API continuing to accept valid reverse moves and cannot survive process/server termination mid-operation; TransferZ does not bypass or corrupt native inventory state to force a move.

Temporary staging can trigger normal DayZ inventory/drop callbacks because the same entities genuinely move through vicinity. Chamber contents, attachments and mod-defined entity state are preserved by object identity rather than manually serialized and recreated.

## UI feedback

A failed Sort request is reported back to the requesting client. The Sort button's full background flashes red for 500 ms and is cleared by a scheduled GUI callback. An already-sorted container or a container with fewer than two direct cargo items is a successful no-op and does not show the failure flash.
