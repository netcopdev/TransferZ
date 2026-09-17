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

## Execution and temporary staging

Sort computes the complete final layout before executing the first move. Items already at their final row, column and orientation remain in place.

Every other direct cargo child is temporarily staged before final placement. For an external source container, TransferZ first tries free cargo space in the requesting player's inventory. If no suitable player cargo location is available, or the source itself is inside the player's inventory hierarchy, TransferZ uses DayZ's normal vicinity/ground drop path around the requesting player.

After staging, the same item entities are moved back into the source cargo at their exact final target locations and orientations through validated native inventory moves. Because staging removes the need to solve cycles inside a nearly full cargo grid, execution is linear rather than an exponentially branching rearrangement search.

Temporary staging can trigger normal DayZ inventory enter/exit or drop callbacks because the items genuinely move through those locations. It does not copy item state or reconstruct weapons, magazines, nested containers, attachments, chamber contents, or mod-defined variables.

If staging or final placement fails, Sort reports failure and makes a best-effort attempt to return any still-staged items to free space in the source cargo. No item is deliberately deleted, recreated, or converted into a replacement entity.

## UI feedback

A failed Sort request is reported back to the requesting client. The Sort button's full background flashes red for 500 ms and is cleared by a scheduled GUI callback. An already-sorted container or a container with fewer than two direct cargo items is a successful no-op and does not show the failure flash.
