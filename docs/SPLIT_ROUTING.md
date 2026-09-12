# Native stack split routing

TransferZ does not implement its own stack-splitting semantics and does not assign any routing operation to right click. DayZ still owns whether an item can be split, the split quantity, resulting state, and the native item-manipulation protocol.

TransferZ only influences destination selection for native splits in two hand-related cases.

## Stack itself in hands

When the stack being split is the entity currently held in hands:

1. **Preferred destination (`P*`)** — resolve the player's preferred attachment-path destination and place the split there when its exact cargo can accept the new stack.
2. **Vanilla DayZ fallback** — if `P*` is missing, invalid, or has no suitable free location, do not consume the split action; DayZ chooses its normal fallback location.

## Stack inside a held container

When the stack is a direct cargo item whose source is at or below the container currently held in hands:

1. **Exact source cargo** — keep the newly created split stack in the same immediate source container when that cargo has a valid free location.
2. **Preferred destination (`P*`)** — when the source cargo has no usable location, resolve the preferred attachment-path destination and place the split there when its exact cargo can accept the new stack.
3. **Vanilla DayZ fallback** — when neither TransferZ destination is usable, leave normal DayZ split fallback in control.

The source and preferred checks use `FindFirstFreeLocationForNewEntity` because a split creates a new entity. The selected destination is then passed through DayZ's native `INPUT_UDT_ITEM_MANIPULATION` split path rather than recreating or manually copying stack state.

The preferred path is read from the same `$profile:TransferZ/preferences.json` data used by the mission-side preferred-destination feature. No arbitrary player-inventory fallback is introduced by TransferZ.
