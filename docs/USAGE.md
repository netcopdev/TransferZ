# TransferZ usage

This guide describes the current TransferZ 0.1.0 controls and routing behavior.

## Core idea

TransferZ separates **where items should go** from **what operation should be performed**.

- `D*` is the active destination.
- `T` transfers a source zone's direct contents.
- `U` unpacks nested cargo.
- `L` creates one temporary bidirectional container link.
- `P*` is the persistent preferred personal cargo target.
- Item gestures provide single-item, zone-batch, and exact-class batch shortcuts.

A destination always means that exact container's own cargo. TransferZ does not silently search through other player inventory space when the chosen destination cannot accept an item.

## Header controls

### `D` — destination

Click `D` on an open cargo container to make it the active destination. The active container shows `D*`.

Clicking `D` on another container replaces the previous destination. `VICINITY` also has `D`; selecting it makes the ground/vicinity zone the destination.

Container destinations are intentionally temporary. TransferZ clears them when the selected container is no longer a valid open/in-hand reachable participant. A vicinity destination is cleared when the vicinity panel closes.

### `T` — transfer

Click `T` to move the source container's **direct cargo children** to `D*`.

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

`T -> Barrel` attempts to move:

```text
Ammo Box
Medical Pouch
Knife
```

The `ammo` and `bandage` stay inside their respective containers.

You can also drag `T` directly onto another open container to perform a one-off transfer without changing `D*`.

### `U` — unpack

Click `U` to move non-container leaf items found **inside cargo-bearing direct child containers** to `D*`.

Using the same example, `U -> Barrel` attempts to move:

```text
ammo
bandage
```

The `Ammo Box`, `Medical Pouch`, and direct loose `Knife` remain in the Backpack.

Dragging `U` back onto its own source container flattens nested cargo into that source. Attachments are not traversed by `U` in 0.1.0.

### `L` — temporary link

Click `L` on one container, then `L` on another.

- `L+` means the first container is waiting for its partner.
- `L*` means the container is part of the active link pair.
- Clicking `L` on a linked participant removes the link.
- Starting another link replaces the old pair.

Only one link pair exists at a time. Links are session-local and require both containers to remain valid/open/in-hand and reachable.

### `P` — preferred personal destination

`P` appears on cargo-bearing items in the player's attachment hierarchy.

Selecting `P` stores that attachment path as the preferred personal destination and displays `P*` when it resolves. Selecting another valid `P` replaces the stored path.

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

## Item gestures

### Normal left drag

Unmodified left drag remains standard DayZ inventory dragging.

### `Shift + Click`

Moves the clicked item to `D*`.

This works for cargo items and shown vicinity items. If no valid destination exists or the item cannot be accepted, it stays where it is.

### `Alt + Click`

Moves the clicked item to `P*`.

If no valid preferred target resolves, nothing is moved.

### `Shift + Left Drag`

Runs the source zone's `T` operation and lets you drop that operation onto another container.

- From a cargo item, the source zone is the item's immediate cargo owner.
- From a vicinity item, the source zone is the currently shown vicinity list.

### `Alt + Left Drag`

Runs the source zone's `U` operation and lets you drop that operation onto another container.

- From a cargo item, `U` operates on nested cargo under the item's immediate cargo owner.
- From vicinity, it behaves like vicinity `U` and unpacks the shown vicinity containers.

### Right drag — exact-class batch move

Right-dragging an item selects its **exact classname** as the batch selector.

From a cargo container:

- the source is that item's immediate cargo owner;
- all direct source-cargo items with the same exact `GetType()` are selected;
- drop onto another open container to move those matches there;
- drop onto `VICINITY` to move those matches to the ground.

From vicinity:

- the source is the currently shown loose vicinity list;
- only shown items with the same exact `GetType()` are selected;
- drop onto an open container to move those matches there;
- vicinity-to-vicinity is a no-op because the items are already there.

A right drag begins only after actual mouse movement. A normal right click therefore remains available for DayZ/TransferZ click behavior.

Exact class means exact class. Similar ammunition, magazines, food variants, or other related items are not grouped unless they share the same actual `GetType()`.

### `Ctrl`

TransferZ does not claim Ctrl click/drag gestures. Vanilla DayZ behavior remains in control.

## Double-click routing

### Cargo: double-left-click

TransferZ routing priority is:

1. linked partner, when the immediate source container is linked;
2. otherwise `P*` for external or in-hand containers;
3. otherwise vanilla DayZ behavior.

Items in normal worn/attached player inventory therefore retain familiar vanilla double-click-to-hands behavior when no TransferZ route owns the action.

### Cargo: double-right-click

Double-right-click is the exact-class batch form of the same route resolution.

TransferZ attempts to move matching direct source-cargo items with the same exact `GetType()` to:

1. the linked partner, if the source is linked;
2. otherwise `P*`.

### Vicinity double-click

- Double-left-click routes the selected shown item to `P*`.
- Double-right-click routes the currently shown items of that exact class to `P*`.

## Vicinity controls

The `VICINITY` header has `D`, `T`, and `U`.

### Vicinity `D`

Selects ground/vicinity as `D*`.

### Vicinity `T`

Moves currently shown loose, takeable vicinity items into the selected container destination. Cargo-bearing containers are excluded from this loose-item transfer.

If the selected destination container is itself shown in vicinity, it is skipped as a source and may still receive the other items.

With `VICINITY` itself selected as the destination, vicinity `T` is a no-op.

### Vicinity `U`

Treats the currently shown cargo-bearing vicinity containers as sources, unpacks their nested contents into the selected destination, and leaves those source containers in place.

With `VICINITY` selected as the destination, vicinity `U` empties the shown containers onto the ground.

Both vicinity `T` and `U` can be dragged directly onto an open container for a one-off action.

## Operation previews

Hovering `T` or `U` shows an advisory status color:

- green — expected normal/full execution;
- yellow — likely partial execution;
- red — currently impossible.

This is only a preview. The server performs the authoritative move validation when the operation is requested.

## Failure behavior

TransferZ fails conservatively. If an item is no longer where expected, cannot be removed, cannot fit, is out of reach, or the chosen destination is no longer valid, the item remains where it is.

TransferZ does not delete and recreate items to simulate movement.

## Common examples

### Move all loose contents of a crate to a barrel

1. Click `D` on the Barrel.
2. Click `T` on the Crate.

### Empty pouches inside a backpack but leave the pouches in the backpack

1. Click `D` on the receiving container.
2. Click `U` on the Backpack.

### Move every item of one exact ammo classname from a crate

Right-drag one representative round/stack of that exact classname from the Crate onto the destination container.

### Pick up all shown loose vicinity items

1. Select a receiving container with `D`.
2. Click vicinity `T`.

### Move all shown vicinity items of one exact class

Right-drag one representative vicinity item of that class onto the destination container.

### Create a fast two-container workflow

1. Click `L` on container A.
2. Click `L` on container B.
3. Double-left-click items in either container to route them to the other.
4. Double-right-click an item to route all exact-class matches in that source container.
