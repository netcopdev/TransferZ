# Native stack split routing

TransferZ does not implement its own stack-splitting semantics and does not assign any routing operation to right click. DayZ still owns whether an item can be split, the split quantity, resulting state, and the native item-manipulation protocol.

TransferZ only influences the destination chosen for a native split.

## Routing priority

The split destination is resolved in this order:

1. **Active destination (`D*`)** — when a valid transient destination is selected, it is the explicit split target. A container destination means that container's exact cargo. A `VICINITY` destination means DayZ's normal ground placement around the player.
2. **Immediate source cargo** — when no usable `D*` route exists and the stack is already in cargo, keep the split in that same immediate container whenever it has room for the new entity.
3. **Preferred destination (`P*`)** — use the configured preferred cargo only when there is no usable immediate source cargo, including when the original stack is not in cargo or its source cargo has no room.
4. **Vanilla DayZ fallback** — use DayZ's normal behavior in every remaining case.

`D*`, source cargo, and `P*` are ordered preferences rather than hard stops. If `D*` cannot accept the split, routing continues to the immediate source cargo; if that cannot take it, routing continues to `P*`; if `P*` cannot resolve or accept it, TransferZ falls back to vanilla.

## Source-local splitting

When `D*` is absent or cannot accept the split and the stack is already in cargo, TransferZ asks the immediate cargo owner for a free location for the new split entity before considering `P*`. If a valid location exists, the new stack remains beside the original stack in that same container.

This applies to normal cargo containers generally; it is not limited to containers currently held in hands. A stack itself in hands, an attachment, or an item on the ground has no immediate cargo source for this rule, so routing may continue to `P*` before vanilla fallback.

## Native execution

Cargo destination checks mirror vanilla DayZ right-click splitting and use `FindFreeLocationFor(this, FindInventoryLocationType.CARGO, ...)`, preserving the native placement search including a returned rotated (`flip`) cargo location when available. `VICINITY` uses DayZ's native ground-position resolution. The chosen destination is then passed through DayZ's native `INPUT_UDT_ITEM_MANIPULATION` split path rather than recreating an item or manually copying quantity/state.

The lower-level split hook lives in `4_World`. The mission-side `TransferZClientState` mirrors only its already-validated transient `D*` state into the split bridge whenever destination state is set, queried, cleared, or revalidated. This avoids trying to mod `ItemBase` from the Mission script module while keeping split routing consistent with the visible active destination. Preferred state continues to come from the same `$profile:TransferZ/preferences.json` data used by the normal preferred-destination feature.
