# Sort behavior

TransferZ Sort is a server-authoritative rearrangement of a container's direct cargo children. It never deletes or recreates sorted items: the same `EntityAI` objects are moved through DayZ's native inventory system from their current location to temporary staging and then back into the source cargo.

## Layout policy

The target layout is deterministic and rotation-aware.

- The target packer scans the cargo from the first free cell at the top-left downward.
- At each free anchor it evaluates both valid orientations for every non-square remaining item and selects the largest candidate that fits best there.
- Equal-quality choices prefer the item's current orientation so Sort does not rotate items without a packing benefit.
- This lets long weapons, cases, bandages, magazines, and other rectangular items use either orientation when that improves fit and overall packing.
- Equivalent-size target assignments are optimized before movement so interchangeable shapes do not create unnecessary identity swaps.
- Smaller items fill otherwise wasted gaps in the compact target layout; there is no dedicated bottom zone for small items.
- DayZ user-reserved inventory cells are treated as unavailable.
- If the compact anchor packer cannot assign every item, Sort falls back to a deterministic rotation-aware first-fit target layout rather than failing solely because of the packing heuristic.

## Transactional execution

Sort snapshots the exact original row, column and orientation of every direct cargo child before any move. It computes the complete target layout before execution and is a no-op when that layout already matches the snapshot.

When movement is required, Sort temporarily stages every tracked direct cargo item through DayZ's normal vicinity/ground drop path. Staging the complete set deliberately empties the source cargo, which removes in-cargo move cycles and makes both final placement and rollback deterministic. Player inventory is not used as an implicit staging area.

With the source empty, Sort preflights every exact target move before committing the first target placement. The same original item entities are then moved back into the source at their exact target rows, columns and orientations through validated native inventory moves. No weapon, magazine, container or other item is copied or reconstructed.

Success is reported only after every tracked item is verified at its target location. If staging, target preflight, target placement, or final verification fails, Sort enters synchronous rollback:

- any tracked item currently in the source at a non-original location is evacuated again;
- every tracked item is restored to its exact original row, column and orientation from the snapshot;
- rollback is verified before the failed operation returns;
- no tracked item is intentionally left in vicinity;
- if DayZ unexpectedly refuses an exact rollback move after repeated recovery passes, TransferZ performs a final containment attempt back into the source and emits a `CRITICAL rollback incomplete` diagnostic. This is treated as a hard invariant violation, not a successful or acceptable partial sort.

The rollback design means a normal failed Sort should leave the source exactly as it was before the button was pressed. Absolute recovery still depends on DayZ's native inventory API accepting the reverse moves; TransferZ does not bypass or corrupt native inventory state to force a move.

Temporary staging can trigger normal DayZ inventory/drop callbacks because the same entities genuinely move through vicinity. Chamber contents, attachments and mod-defined entity state are preserved by object identity rather than manually serialized and recreated.

## UI feedback

A failed Sort request is reported back to the requesting client. The Sort button's full background flashes red for 500 ms and is cleared by a scheduled GUI callback. An already-sorted container or a container with fewer than two direct cargo items is a successful no-op and does not show the failure flash.
