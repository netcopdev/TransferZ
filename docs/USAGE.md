# TransferZ usage

This guide describes the current TransferZ 0.1.0 controls and routing behavior.

## Core idea

TransferZ separates **where items should go** from **what operation should be performed**.

Cargo headers show two compact groups:

- **left:** Destination, Transfer, Unpack, Link, Preferred;
- **right:** Sort, Stack.

`VICINITY` shows Destination, Transfer, and Unpack on the left only.

A destination always means that exact container's own cargo. TransferZ does not silently search through other player inventory space when the chosen destination cannot accept an item.

## Header controls

### Destination — target icon

Click the target icon on an open cargo container to make it the active destination. The active target receives a subdued state highlight.

Clicking it on another container replaces the previous destination. Clicking the active destination again clears it. `VICINITY` also has the target icon; selecting it makes the ground/vicinity zone the destination.

Container destinations are intentionally temporary. TransferZ clears them when the selected container is no longer a valid open/in-hand reachable participant. A vicinity destination is cleared when the vicinity panel closes.

### Transfer — arrow icon

Click Transfer to move the source container's **direct cargo children** to the active destination.

Nested containers are treated as direct items and move intact when they fit.

Example:

```text
Backpack
  Ammo Box
    ammo
  Medical Pouch
    bandage
  Knife
```

Transfer to a Barrel attempts to move:

```text
Ammo Box
Medical Pouch
Knife
```

The `ammo` and `bandage` stay inside their respective containers.

You can also drag the Transfer control directly onto another open container to perform a one-off transfer without changing the active destination.

### Unpack

Click Unpack to move non-container leaf items found **inside cargo-bearing direct child containers** to the active destination.

Using the same example, Unpack to a Barrel attempts to move:

```text
ammo
bandage
```

The `Ammo Box`, `Medical Pouch`, and direct loose `Knife` remain in the Backpack.

Dragging Unpack back onto its own source container flattens nested cargo into that source. Attachments are not traversed by Unpack in 0.1.0.

### Link

Click Link on one container, then Link on another.

- The first participant receives a pending-link state highlight.
- Both participants receive the linked-state highlight after the pair is completed.
- Clicking Link on a linked participant removes the link.
- Starting another link replaces the old pair.

Only one link pair exists at a time. Links are session-local and require both containers to remain valid/open/in-hand and reachable.

### Preferred — pin icon

Preferred appears on cargo-bearing items in the player's attachment hierarchy.

Selecting it stores that attachment path as the preferred personal destination and highlights it when resolved. Selecting the currently preferred target again clears the preference. Selecting another valid target replaces the stored path.

Examples of stored paths:

```text
Back
Belt > DumpPouch
Vest > Pouch
```

The path survives restarts and can resolve through replacement equipment when the same attachment path exists. TransferZ does not guess a different target if the stored path cannot be resolved.

Preferences are stored in:

```text
$profile:TransferZ/preferences.json
```

### Sort — right-side maintenance control

Sort reorganizes the selected container's **direct cargo grid only**.

TransferZ first snapshots the direct cargo, calculates a deterministic compacted target layout, and then builds a move plan before changing anything. The plan uses the container's own grid only.

Current behavior:

- larger direct cargo items are placed first;
- item type contributes to deterministic grouping after size priority;
- each item's existing orientation is preserved;
- nested containers are treated as ordinary direct cargo items and stay intact;
- DayZ user-reserved cells, including the placeholder for an item currently held in hands, are treated as occupied;
- all movement uses native DayZ inventory locations within the same cargo owner;
- if no safe in-container move plan exists, the operation is skipped rather than moving items to the ground or another temporary container.

Sort never deletes/recreates items.

### Stack — right-side maintenance control

Stack scans the selected container's direct cargo and asks DayZ whether pairs can be combined.

Only pairs accepted by DayZ's own `CanBeCombined` logic are passed to DayZ's native `CombineItems` behavior. This means TransferZ does not invent compatibility between related ammo types, different magazines, different food variants, or any other classes that DayZ itself refuses to combine.

Stack does not:

- merge through nested containers;
- split stacks;
- move stacks to another container;
- recreate items or manually copy quantity/state.

## Item gestures

### Normal left drag

Unmodified left drag remains standard DayZ inventory dragging.

### Right click

Right click, right drag, and double-right-click are not assigned to TransferZ routing operations. DayZ retains its native right-click behavior, including stack splitting.

### `Shift + Click`

Moves the clicked item to the active destination.

This works for cargo items and shown vicinity items. If no valid destination exists or the item cannot be accepted, it stays where it is.

### `Alt + Click`

Moves the clicked item to the preferred personal destination.

If no valid preferred target resolves, nothing is moved.

### `Shift + Left Drag`

Moves the source zone as a Transfer batch and lets you drop that batch onto another container.

- From a cargo item, the source zone is the item's immediate cargo owner and all direct cargo children are selected.
- From a vicinity item, the currently shown eligible loose vicinity items are selected.
- From a cargo source, dropping onto `VICINITY` moves the batch to the ground through DayZ's normal drop path.

### `Alt + Left Drag`

Selects an exact-class batch using the dragged item as the representative.

From a cargo container:

- the source is the dragged item's immediate cargo owner;
- all direct source-cargo items with the same exact `GetType()` are selected;
- drop onto another open container to move those matches there;
- drop onto `VICINITY` to move those matches to the ground.

From vicinity:

- the source is the currently shown loose vicinity list;
- only shown loose, takeable, removable non-container items with the same exact `GetType()` are selected;
- drop onto an open container to move those matches there;
- vicinity-to-vicinity is a no-op because those items are already there.

Exact class means exact class. Similar ammunition, magazines, food variants, or other related items are not grouped unless they share the same actual `GetType()`.

### `Ctrl`

TransferZ does not claim Ctrl click/drag gestures. Vanilla DayZ behavior remains in control.

## Native stack splitting and preferred destination

TransferZ does not implement its own stack split. DayZ still decides whether an item can be split, the split amount, resulting item state, and the item-manipulation protocol.

TransferZ only influences destination selection in the following cases:

- If the stack itself is currently in hands, the split result first tries the resolved preferred destination (`P*`) when that exact cargo has room. If `P*` cannot accept it, DayZ's normal fallback handles the split.
- If the stack is in cargo at or below a container currently held in hands, the split result first tries the exact immediate source cargo, then `P*`, then DayZ's normal fallback.

This preserves native right-click behavior while making `P*` useful for hand-held splitting.

## Double-click routing

### Cargo: double-left-click

TransferZ routing priority is:

1. linked partner, when the immediate source container is linked;
2. otherwise the preferred target for external or in-hand containers;
3. otherwise vanilla DayZ behavior.

Items in normal worn/attached player inventory therefore retain familiar vanilla double-click-to-hands behavior when no TransferZ route owns the action.

### Vicinity: double-left-click

Double-left-click on a shown vicinity item routes it to the preferred target when available. Otherwise vanilla behavior remains in control.

TransferZ does not assign double-right-click routing.

## Vicinity controls

The `VICINITY` header has Destination, Transfer, and Unpack.

### Vicinity Destination

Selects ground/vicinity as the active destination. Selecting it again clears it.

### Vicinity Transfer

Moves currently shown loose, takeable vicinity items into the selected container destination. Cargo-bearing containers are excluded from this loose-item transfer.

If the selected destination container is itself shown in vicinity, it is skipped as a source and may still receive the other items.

With `VICINITY` itself selected as the destination, vicinity Transfer is a no-op.

### Vicinity Unpack

Treats the currently shown cargo-bearing vicinity containers as sources, unpacks their nested contents into the selected destination, and leaves those source containers in place.

With `VICINITY` selected as the destination, vicinity Unpack empties the shown containers onto the ground.

Both vicinity Transfer and Unpack can be dragged directly onto an open container for a one-off action.

Vicinity does not expose Sort or Stack because it is a list of world items rather than one cargo grid.

## Operation previews

Hovering Transfer or Unpack shows an advisory status color:

- green — expected normal/full execution;
- yellow — likely partial execution;
- red — currently impossible.

This is only a preview. The server performs the authoritative move validation when the operation is requested.

## Failure behavior

TransferZ fails conservatively. If an item is no longer where expected, cannot be removed, cannot fit, is out of reach, or the chosen destination is no longer valid, the item remains where it is.

TransferZ does not delete and recreate items to simulate movement, sorting, or stacking.

## Common examples

### Move all direct contents of a crate to a barrel

Either:

1. Select the Barrel with Destination.
2. Click Transfer on the Crate.

Or hold `Shift` and left-drag any item from the Crate onto the Barrel.

### Move every item of one exact ammo classname from a crate

Hold `Alt` and left-drag one representative stack of that exact classname from the Crate onto the destination container.

### Empty pouches inside a backpack but leave the pouches in the backpack

1. Select the receiving container with Destination.
2. Click Unpack on the Backpack.

### Repack a messy container

Click Sort on the right side of that container's header. TransferZ attempts a deterministic in-place compaction using only that container's cargo grid and preserves active DayZ reserved locations.

### Consolidate partial stacks

Click Stack on the right side of that container's header. Only pairs DayZ considers natively combinable are merged.

### Pick up all shown loose vicinity items

1. Select a receiving container with Destination.
2. Click vicinity Transfer, or hold `Shift` and left-drag one vicinity item onto the destination.

### Move all shown vicinity items of one exact class

Hold `Alt` and left-drag one representative vicinity item of that class onto the destination container.

### Create a fast two-container workflow

1. Click Link on container A.
2. Click Link on container B.
3. Double-left-click items in either container to route them to the other.
4. Use `Alt + Left Drag` when you want to move every exact-class match as a batch.

## Dependency

TransferZ requires Community Framework (CF) on both client and server. Load CF before TransferZ.

Steam Workshop: https://steamcommunity.com/sharedfiles/filedetails/?id=1559212036
