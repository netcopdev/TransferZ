# Native stack split routing

TransferZ does not implement its own stack-splitting semantics and does not assign any routing operation to right click. DayZ still owns whether an item can be split, the split quantity, resulting state, and the native item-manipulation protocol.

TransferZ only influences destination selection for native splits in hand-related cases.

## Preferred destination override

When a valid preferred destination (`P*`) is configured and its exact cargo has room for the newly created split entity, that destination takes priority over normal same-source placement.

If `P*` is missing, invalid, or cannot accept the split, TransferZ does not force it.

## Stack itself in hands

When the stack being split is the entity currently held in hands:

1. **Preferred destination (`P*`)** — use it when it can accept the new split stack.
2. **Vanilla DayZ fallback** — otherwise leave the split operation entirely to DayZ.

## Stack inside a held container

When the stack is a cargo item whose source is at or below the container currently held in hands:

1. **Preferred destination (`P*`)** — when configured and usable, it overrides normal placement.
2. **Exact source cargo** — otherwise keep the newly created split stack beside the original stack when that immediate source cargo has a valid free location.
3. **Vanilla DayZ fallback** — when neither TransferZ destination is usable, leave normal DayZ split fallback in control.

The source and preferred checks use `FindFirstFreeLocationForNewEntity` because a split creates a new entity. The selected destination is then passed through DayZ's native `INPUT_UDT_ITEM_MANIPULATION` split path rather than recreating or manually copying stack state.

The preferred path is read from the same `$profile:TransferZ/preferences.json` data used by the mission-side preferred-destination feature. No arbitrary player-inventory fallback is introduced by TransferZ.
