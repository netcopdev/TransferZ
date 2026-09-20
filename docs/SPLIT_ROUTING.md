# Native stack split routing

TransferZ does not implement its own stack-splitting semantics and does not assign any routing operation to right click. DayZ still owns whether an item can be split, the split quantity, resulting state, and the native item-manipulation protocol.

TransferZ only influences the destination chosen for a native split.

## Routing priority

The split destination is resolved in this order:

1. **Active destination (`D*`)** — when a valid transient destination is selected, it is the explicit split target. A container destination means that exact `(owner entity, cargo grid index)`. A `VICINITY` destination means DayZ's normal ground placement around the player.
2. **Immediate source cargo** — when no usable `D*` route exists and the stack is already in cargo, keep the split in that same immediate cargo grid whenever it has room for the new entity.
3. **Preferred destination (`P*`)** — use the configured preferred cargo grid only when there is no usable immediate source cargo, including when the original stack is not in cargo or its source cargo has no room.
4. **Vanilla DayZ fallback** — use DayZ's normal behavior in every remaining case.

`D*`, source cargo, and `P*` are ordered preferences rather than hard stops. If `D*` cannot accept the split, routing continues to the immediate source cargo; if that cannot take it, routing continues to `P*`; if `P*` cannot resolve or accept it, TransferZ falls back to vanilla.

## Source-local splitting

When `D*` is absent or cannot accept the split and the stack is already in cargo, TransferZ searches that exact source cargo grid for a free location before considering `P*`. If a valid location exists, the new stack remains beside the original stack in that same grid.

This applies to normal cargo containers generally; it is not limited to containers currently held in hands. A stack itself in hands, an attachment, or an item on the ground has no immediate cargo source for this rule, so routing may continue to `P*` before vanilla fallback.

## Native execution

Cargo destination checks are grid-specific. TransferZ enumerates the selected grid's rows/columns in both item orientations, builds exact `InventoryLocation.SetCargo(..., cargoIndex, row, col, flip)` candidates, and accepts only candidates DayZ reports addable. This prevents an entity-level free-location search from silently choosing a different grid while still allowing rotation. `VICINITY` uses DayZ's native ground-position resolution. The chosen destination is then passed through DayZ's native `INPUT_UDT_ITEM_MANIPULATION` split path rather than recreating an item or manually copying quantity/state.

The lower-level split hook lives in `4_World`. The mission-side `TransferZClientState` mirrors the validated transient `D*` owner **and cargo index** into the split bridge whenever destination state is set, queried, cleared, or revalidated. This avoids trying to mod `ItemBase` from the Mission script module while keeping split routing consistent with the visible active destination. Preferred state continues to come from the same `$profile:TransferZ/preferences.json` data used by the normal preferred-destination feature, including its cargo-grid index.
