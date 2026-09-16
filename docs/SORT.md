# Sort behavior

TransferZ Sort is a server-authoritative, container-local rearrangement of direct cargo children. It never recreates items and never uses another container or the ground as temporary storage.

## Layout policy

The target layout is deterministic and keeps each item's current orientation.

- Items are ordered primarily by footprint, largest first.
- For equal footprints, wider items are ordered before taller items.
- For items with identical dimensions, current spatial order is preserved before the type tie-break.
- Equivalent-size target assignments are optimized before movement so interchangeable shapes do not create unnecessary identity swaps.
- The final layout is packed from the top of the cargo grid downward.
- Smaller items follow larger items immediately and fill the earliest available gaps where they fit; there is no dedicated bottom zone for small items.
- DayZ user-reserved inventory cells are treated as unavailable.

## Move planning

Sort first computes the compact final layout, then settles it from the top downward. Once an item reaches a completed final target it is locked and is not disturbed again.

When a final target is occupied, the blocking item is evacuated inside the same cargo. The planner first tries direct free space, biased toward lower cargo rows. If no suitable rectangle is currently free, it recursively clears a temporary rectangle by moving its blockers first. This means smaller and medium items can be pushed into available lower/free areas to create workspace for larger items, instead of requiring a pre-existing full-size staging slot.

Recursive evacuation is bounded by move, search, and depth limits. Failed search branches restore the virtual cargo state before another candidate is tried, so speculative planning does not leak partial moves. No string-based exhaustive state search is used.

Every planned move is executed later through DayZ native inventory APIs and revalidated at execution time. If bounded planning or native move validation fails, Sort reports failure rather than moving items outside the container, recreating items, or dropping them.

## UI feedback

A failed Sort request is reported back to the requesting client. The Sort button's full background flashes red for 500 ms and is cleared by a scheduled GUI callback. An already-sorted container or a container with fewer than two direct cargo items is a successful no-op and does not show the failure flash.
