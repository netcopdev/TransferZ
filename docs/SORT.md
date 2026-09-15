# Sort behavior

TransferZ Sort is a server-authoritative, container-local rearrangement of direct cargo children. It never recreates items and never uses another container or the ground as temporary storage.

## Layout policy

The target layout is deterministic and keeps each item's current orientation.

- Larger items are assigned first and packed toward the top of the cargo grid.
- Small items (up to four cargo cells) are assigned toward the lower part of the grid so they do not fragment the space needed by larger items.
- Type is used as a stable secondary ordering within equal-size items.
- DayZ user-reserved inventory cells are treated as unavailable.
- If the size-banded layout itself cannot fit, Sort retries a compact top-down target layout before giving up.

## Move planning

Sort computes the complete target arrangement before executing native inventory moves. The planner follows blocking dependencies: an item occupying another item's target is first moved to its own final target when possible. Cycles are broken by parking a blocker, or another active-chain item, in safe free space inside the same cargo. Planning has strict step limits so a pathological layout cannot stall the game thread.

Every executed move is revalidated with DayZ inventory APIs. If planning or native move validation fails, Sort reports failure rather than moving items outside the container or recreating them.

## UI feedback

A failed Sort request is reported back to the requesting client. The Sort button background flashes red for one second. An already-sorted container or a container with fewer than two direct cargo items is a successful no-op and does not show the failure flash.
