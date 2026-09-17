# Sort behavior

TransferZ Sort is a server-authoritative, container-local rearrangement of direct cargo children. It never recreates items and never uses another container or the ground as temporary storage.

## Layout policy

The target layout is deterministic and rotation-aware.

- The target packer scans the cargo from the first free cell at the top-left downward.
- At each free anchor it evaluates both valid orientations for every non-square remaining item and selects the largest candidate that fits best there.
- Equal-quality choices prefer the item's current orientation so Sort does not rotate items without a packing benefit.
- This lets long weapons, cases, bandages, magazines, and other rectangular items use either orientation when that improves fit and overall packing.
- Equivalent-size target assignments are optimized before movement so interchangeable shapes do not create unnecessary identity swaps.
- Final target execution prioritizes larger target rectangles before small fillers. This preserves free working space while the difficult moves are settled and reduces fragmentation during rearrangement.
- Smaller items still fill otherwise wasted gaps in the compact target layout; there is no dedicated bottom zone for small items.
- DayZ user-reserved inventory cells are treated as unavailable.
- If the compact anchor packer cannot assign every item, Sort falls back to a deterministic rotation-aware first-fit target layout rather than failing solely because of the packing heuristic.

## Move planning

Sort first computes the complete final layout, then finds a legal in-container sequence to reach it. Once an item reaches a completed final target it is locked and is not disturbed again.

When a final target is occupied, the blocking item is evacuated inside the same cargo. The planner first tries direct free space, biased toward lower cargo rows and considering both orientations. If no suitable rectangle is currently free, it recursively clears a temporary rectangle by moving its blockers first. Candidate temporary rectangles with fewer blockers are explored first.

Recursive evacuation is bounded by move, search, and depth limits. Failed search branches restore the virtual cargo state before another candidate is tried, so speculative planning does not leak partial moves. The V5 planner memoizes failed recursive parking states, preventing equivalent cargo arrangements from being rediscovered repeatedly. Speculative parking branches are not individually logged; only useful planner summaries and failures are emitted, avoiding diagnostic logging itself becoming a performance cost.

Every planned move is executed later through DayZ native inventory APIs and revalidated at execution time. If bounded planning or native move validation fails, Sort reports failure rather than moving items outside the container, recreating items, or dropping them.

## UI feedback

A failed Sort request is reported back to the requesting client. The Sort button's full background flashes red for 500 ms and is cleared by a scheduled GUI callback. An already-sorted container or a container with fewer than two direct cargo items is a successful no-op and does not show the failure flash.
