# Sort behavior

TransferZ Sort is a server-authoritative, container-local rearrangement of direct cargo children. It never recreates items and never uses another container or the ground as temporary storage.

## Layout policy

The target layout is deterministic and keeps each item's current orientation.

- Items are ordered primarily by footprint, largest first.
- For equal footprints, wider items are ordered before taller items. Because Sort preserves orientation, this reduces early vertical fragmentation and tends to create usable staging space as large items settle.
- The final layout is packed from the top of the cargo grid downward.
- Smaller items follow larger items immediately and fill the earliest available gaps where they fit; there is no dedicated bottom zone for small items.
- Type is used as a stable tie-break after footprint and shape.
- DayZ user-reserved inventory cells are treated as unavailable.

## Move planning

Sort computes the complete target arrangement before executing native inventory moves. A blocker occupying the current target is first parked directly in safe free space inside the same cargo when possible. If no direct parking slot exists, the planner follows that blocker's own target dependency and can break cycles by parking an active-chain item. Temporary parking may use lower free space, but it does not affect the final compact top-down layout. Planning has strict step limits so a pathological layout cannot stall the game thread.

Every executed move is revalidated with DayZ inventory APIs. If planning or native move validation fails, Sort reports failure rather than moving items outside the container or recreating them.

## UI feedback

A failed Sort request is reported back to the requesting client. The Sort button's full background flashes red for 500 ms and is cleared by a scheduled GUI callback, rather than depending on a later inventory update tick. An already-sorted container or a container with fewer than two direct cargo items is a successful no-op and does not show the failure flash.
