# Native stack split routing

TransferZ does not implement its own stack-splitting semantics and does not assign any routing operation to right click. DayZ still owns whether an item can be split, the split quantity, resulting state, and the native item-manipulation protocol.

TransferZ only influences the destination chosen for a native split.

## Routing priority

The split destination is resolved in this order:

1. **Active destination (`D*`)** — when a valid transient destination is selected, it is the explicit split target. A container destination means that container's exact cargo. A `VICINITY` destination means DayZ's normal ground placement around the player.
2. **Preferred destination (`P*`)** — when no `D*` is active and a preferred destination is configured, use its exact cargo.
3. **Immediate source cargo** — only when neither `D*` nor `P*` is selected, keep a cargo stack in the same immediate container when that cargo has room for the newly created split entity.
4. **Vanilla DayZ fallback** — use DayZ's normal behavior in every remaining case.

An explicitly selected route is not silently replaced by another TransferZ route. If active `D*` cannot accept the split, TransferZ falls back to vanilla rather than trying `P*` or the source. Likewise, if `P*` is configured but cannot currently resolve or accept the split, TransferZ falls back to vanilla rather than moving the result back into the source container.

## Source-local splitting

When no `D*` or `P*` is selected and the stack is already in cargo, TransferZ asks the immediate cargo owner for a free location for the new split entity. If a valid location exists, the new stack remains beside the original stack in that same container.

This applies to normal cargo containers generally; it is not limited to containers currently held in hands. A stack itself in hands, an attachment, or an item on the ground has no immediate cargo source for this rule and therefore uses vanilla fallback when no explicit destination is selected.

## Native execution

Cargo destination checks use `FindFirstFreeLocationForNewEntity` because splitting creates a new entity. `VICINITY` uses DayZ's native ground-position resolution. The chosen destination is then passed through DayZ's native `INPUT_UDT_ITEM_MANIPULATION` split path rather than recreating an item or manually copying quantity/state.

The lower-level split hook lives in `4_World`. The mission-side `TransferZClientState` mirrors only its already-validated transient `D*` state into the split bridge whenever destination state is set, queried, cleared, or revalidated. This avoids trying to mod `ItemBase` from the Mission script module while keeping split routing consistent with the visible active destination. Preferred state continues to come from the same `$profile:TransferZ/preferences.json` data used by the normal preferred-destination feature.
