# Stack split routing

TransferZ does not implement its own stack-splitting semantics. Quantity, item state, and server-side split execution remain DayZ-native.

For a normal DayZ stack split initiated on an item whose immediate cargo source is inside (or below) the container currently held in the player's hands, TransferZ only influences destination selection.

Routing order is:

1. **Exact source cargo** — keep the newly created split stack in the same immediate source container when that cargo has a valid free location.
2. **Preferred destination (`P*`)** — when the source cargo has no usable location, resolve the player's saved preferred attachment path and place the split there when its exact cargo can accept the new stack.
3. **Vanilla DayZ fallback** — when neither TransferZ destination is usable, do not consume the action; DayZ's normal split behavior chooses the fallback location.

The source and preferred checks use `FindFirstFreeLocationForNewEntity` because a split creates a new entity. The selected destination is then passed through DayZ's native `INPUT_UDT_ITEM_MANIPULATION` split path rather than recreating or manually copying stack state.

The `ItemBase` hook lives in `Scripts/4_World`, the same script layer as `ItemBase`. It resolves `P*` from the preference data persisted at `$profile:TransferZ/preferences.json`, avoiding a dependency from World code on the Mission-layer `TransferZClientState`.

This routing applies only to stack splits originating from cargo under the item currently in hands. Other DayZ split behavior remains untouched.