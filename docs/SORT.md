# Sort behavior

TransferZ Sort is a server-authoritative, container-local rearrangement of direct cargo children. It never recreates items and never uses another container or the ground as temporary storage.

## Layout policy

The target layout is deterministic and keeps each item's current orientation.

- Items are ordered primarily by footprint, largest first.
- The final layout is packed from the top of the cargo grid downward.
- Smaller items follow larger items immediately and fill the earliest available gaps where they fit; there is no dedicated bottom zone for small items.
- Type is used as a stable secondary ordering within equal-size items.
- DayZ user-reserved inventory cells are treated as unavailable.

## Move planning

Sort computes the complete target arrangement before executing native inventory moves. When a later, lower-priority item blocks the target of an earlier, larger item, the blocker is parked directly in safe free space inside the same cargo instead of first chasing its own final target. This clears large-item targets without creating dependency chains through smaller items. Other blockers still follow their target dependencies when that is useful, and cycles can be broken by parking a blocker or another active-chain item. Temporary parking may use lower free space, but it does not affect the final compact top-down layout. Planning has strict step limits so a pathological layout cannot stall the game thread.

Every executed move is revalidated with DayZ inventory APIs. If planning or native move validation fails, Sort reports failure rather than moving items outside the container or recreating them.

## UI feedback

A failed Sort request is reported back to the requesting client. The Sort button's full background flashes red for one second. An already-sorted container or a container with fewer than two direct cargo items is a successful no-op and does not show the failure flash.
