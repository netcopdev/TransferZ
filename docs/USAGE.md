# TransferZ usage

This guide describes the current TransferZ 0.1.0 controls and routing behavior.

## Core idea

TransferZ separates **where items should go** from **what operation should be performed**.

Cargo headers show two compact groups:

- **left:** `D` Destination, `T` Transfer, `U` Unpack, `L` Link, `P` Preferred;
- **right:** Sort, Stack.

`VICINITY` shows Destination, Transfer, and Unpack on the left only.

A destination always means the exact `(owner entity, cargo grid index)` represented by that header. TransferZ does not silently search another grid on the same entity or other player inventory space when the chosen grid cannot accept an item.

Open vehicle cargo is a supported source and destination for TransferZ routing and maintenance operations. Vehicle access is not judged from the vehicle model origin; actual item moves remain subject to DayZ's native source/destination access and distance validation.

## Header controls

### Destination (`D`)

Click `D` on an open cargo grid to make that exact grid the active destination. The active destination receives a subdued state highlight.

Clicking `D` on another cargo grid replaces the previous destination. Clicking `D` on the active grid again clears it. `VICINITY` also has a `D` control; selecting it makes the ground/vicinity zone the destination.

Container destinations are intentionally temporary. TransferZ clears them when the selected container is no longer a valid open/in-hand reachable participant. A vicinity destination is cleared when the vicinity panel closes.

### Transfer (`T`)

Click `T` to move the source container's **direct cargo children** to the active destination.

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

### Unpack (`U`)

Click `U` to move non-container leaf items found **inside cargo-bearing direct child containers** to the active destination.

Using the same example, Unpack to a Barrel attempts to move:

```text
ammo
bandage
```

The `Ammo Box`, `Medical Pouch`, and direct loose `Knife` remain in the Backpack.

Dragging Unpack back onto its own source container flattens nested cargo into that source. Attachments are not traversed by Unpack in 0.1.0.

### Link (`L`)

Click `L` on one container, then `L` on another.

- The first participant receives a pending-link state highlight.
- Both participants receive the linked-state highlight after the pair is completed.
- Clicking Link on a linked participant removes the link.
- Starting another link replaces the old pair.

Only one link pair exists at a time. Links are session-local and bind exact cargo grids; two grids on the same owning entity are distinct participants. Both participants must remain valid/open/in-hand and reachable.

### Preferred (`P`)

`P` appears on cargo-bearing items in the player's attachment hierarchy.

Selecting it stores that attachment path **and cargo-grid index** as the preferred personal destination and highlights it when resolved. Selecting the currently preferred target again clears the preference. Selecting another valid target replaces the stored path.

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

Sort reorganizes the selected **cargo grid only** while preserving the original item entities. Other cargo grids on the same entity are not part of that transaction.

TransferZ first snapshots every direct cargo item's exact row, column and orientation and calculates a deterministic compacted rotation-aware target layout. If the target already matches the snapshot, Sort is a successful no-op.

When movement is required, TransferZ first computes a complete bounded **in-cargo move sequence**. Free cells inside that same cargo grid may clear blockers or break cycles. Single-player may use DayZ's synchronous native atomic swap for compatible equal-size blockers; multiplayer does not plan direct swaps because the native server swap is asynchronous and cannot be safely verified as one immediate transactional step. If the final layout fits but no bounded in-cargo path exists, TransferZ uses its own hidden native `TransferZ_SortBuffer` as temporary cargo workspace. The same item entities move source → buffer → exact final cells; Sort never uses ground/vicinity, arbitrary player inventory, or arbitrary containers as staging.

Current packing behavior:

- larger items are placed first and smaller items fill gaps;
- Sort first tries to keep every item's current orientation, so already-horizontal and already-vertical items normally stay that way;
- rotation is introduced only when a complete layout cannot otherwise be produced;
- detachable magazines prefer vertical orientation only as a secondary fallback once rotation is actually needed;
- equivalent-size items already occupying valid final slots are kept there when possible to avoid pointless identity swaps;
- DayZ user-reserved cells are treated as unavailable;
- the same original `EntityAI` objects are moved; Sort does not recreate weapons, magazines, nested containers or their state.

Each authoritative step is an exact cargo-to-cargo move. Before a successful step, TransferZ records its inverse. If any move or final verification fails, those successful moves are reversed in strict reverse order and the exact original snapshot is verified before failure is returned.

If the bounded in-cargo planner cannot reach the requested compact layout, Sort falls back to the hidden native sort buffer. A buffer failure restores the exact original snapshot before a normal failure is returned; the buffer is deleted only when verified empty. There is no ground-staging fallback.

### Stack — right-side maintenance control

Stack scans only the selected cargo grid and asks DayZ whether pairs can be combined.

Only pairs accepted by DayZ's own `CanBeCombined` logic are passed to DayZ's native `CombineItems` behavior. This means TransferZ does not invent compatibility between related ammo types, different magazines, different food variants, or any other classes that DayZ itself refuses to combine.

Stack does not:

- merge through nested containers;
- split stacks;
- move stacks to another container;
- recreate items or manually copy quantity/state.

## Key configuration

TransferZ registers a **TransferZ** section in DayZ's stock **Settings > Controls** interface.

Default gesture bindings:

- **Destination / Transfer modifier** — Left Shift or Right Shift.
- **Preferred / Exact-class modifier** — Left Alt or Right Alt.
- **Unpack container drag modifier** — U.

The modifier actions can be rebound like normal DayZ controls. Destination, Transfer, Unpack, Link, Preferred, Sort, and Stack are also exposed as configurable keyboard commands, but have no default binding. These discrete commands are evaluated only while the inventory menu is open. A command acts on the open cargo grid under the mouse; Destination, Transfer, and Unpack also work on the visible `VICINITY` field.

If more than one mutually exclusive TransferZ modifier is held, TransferZ does not claim the click/drag. Likewise, if conflicting TransferZ command actions fire on the same frame, no command is executed.

## Item gestures

### Normal left drag

Unmodified left drag remains standard DayZ inventory dragging.

### Right click

Right click, right drag, and double-right-click are not assigned to TransferZ routing operations. DayZ retains its native right-click behavior, including stack splitting.

### **Destination / Transfer modifier** (default `Shift`) + Click

Moves the clicked item to the active destination.

This works for cargo items and shown vicinity items. If no valid destination exists or the item cannot be accepted, it stays where it is.

### **Preferred / Exact-class modifier** (default `Alt`) + Click

Moves the clicked item to the preferred personal destination.

If no valid preferred target resolves, nothing is moved.

### **Destination / Transfer modifier** (default `Shift`) + Left Drag

Moves the source zone as a Transfer batch and lets you drop that batch onto another container.

- From a cargo item, the source zone is the item's immediate cargo owner and all direct cargo children are selected.
- From a loose vicinity item, the currently shown eligible non-container vicinity items are selected.
- From a cargo-bearing ground container, only that dragged container is selected; its contents remain inside it.
- From a cargo source, dropping onto `VICINITY` moves the batch to the ground through DayZ's normal drop path.

### **Preferred / Exact-class modifier** (default `Alt`) + Left Drag

Selects an exact-class batch using the dragged item as the representative.

From a cargo container:

- the source is the dragged item's immediate cargo owner;
- all direct source-cargo items with the same exact `GetType()` are selected;
- drop onto another open container to move those matches there;
- drop onto `VICINITY` to move those matches to the ground.

From vicinity:

- the source is the currently shown eligible vicinity items;
- shown takeable, removable items with the same exact `GetType()` are selected, including same-class cargo-bearing containers, which move intact with their contents;
- drop onto an open container to move those matches there;
- vicinity-to-vicinity is a no-op because those items are already there.

Exact class means exact class. Similar ammunition, magazines, food variants, or other related items are not grouped unless they share the same actual `GetType()`.

### Unpack container modifier + Left Drag

Hold **Unpack container drag modifier** (default `U`) and left-drag a cargo-bearing container. This invokes the same Unpack operation as the header `U`: the dragged/source container stays where it is, while all of its cargo is flattened into the drop target.

Nested containers are processed contents-first. Their cargo is moved out recursively, then the now-empty nested containers themselves are moved to the target. Dropping on `VICINITY` applies the same flattening to the ground. On a non-container item, this modifier does not claim the drag.

Example: a Protective Case in your backpack contains soda cans and a First Aid Kit containing a bandage. `U + drag` the Protective Case onto a barrel: the case stays in the backpack, while the soda cans, bandage, and now-empty First Aid Kit become direct cargo of the barrel.

### Ctrl and conflicting bindings

TransferZ does not bind Ctrl by default. All TransferZ actions are configurable in the stock DayZ Controls interface; assigning a key or combination that conflicts with a vanilla action is an explicit user choice.

## Native stack splitting and preferred destination

TransferZ does not implement its own stack split. DayZ still decides whether an item can be split, the split amount, resulting item state, and the item-manipulation protocol.

TransferZ only influences where the new split stack is placed, in this order:

1. the active destination (`D*`), when one is selected: that exact cargo grid, or DayZ's normal ground placement around the player for `VICINITY`;
2. otherwise, for a stack in cargo, the same immediate cargo grid when it has room for the new stack;
3. otherwise the preferred destination (`P*`), when one is configured;
4. otherwise normal DayZ behavior.

If `D*` cannot accept the split, routing continues to the original source cargo, then `P*`. If the source cannot take it, routing continues to `P*`; if `P*` also cannot accept it, normal DayZ behavior takes over. See [`SPLIT_ROUTING.md`](SPLIT_ROUTING.md).

This preserves native right-click behavior while letting `D*` and `P*` receive split stacks.

## Double-click routing

### Cargo: double-left-click

TransferZ routing priority is:

1. linked partner grid, when the immediate source cargo grid is linked;
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

- green — a single-item cargo placement is individually proven, or the complete vicinity action is currently eligible;
- yellow — partial execution is known **or** a multi-item cargo batch is geometrically uncertain;
- red — currently impossible.

For container batches, TransferZ deliberately does not infer a guaranteed joint fit from aggregate free area. Several items may each fit individually and still fragment the grid when moved together, so those previews stay yellow until the server performs the authoritative sequence.

### Server work limits

TransferZ bounds expensive server-authored inventory work so a modified client cannot turn one RPC into unbounded traversal or repeated large batches:

- ordinary Transfer/Unpack/Class/MoveItem RPCs: at most one accepted request per player per 100 ms;
- direct Transfer and exact-class source grids: maximum 1024 direct items;
- Unpack traversal: maximum 2048 scanned cargo nodes, 32 nested cargo levels, and 1024 collected leaves;
- Sort/Stack maintenance RPCs retain their 250 ms throttle.

These are fail-before-mutation limits. If a request exceeds a limit, TransferZ rejects that operation rather than moving a prefix and leaving a surprise partial result.

## Failure behavior

TransferZ fails conservatively. Normal routing failures leave items where DayZ left them rather than deleting/recreating them.

Sort has the stronger transactional rule described above: after the snapshot is taken, an ordinary failure must restore and verify the original cargo layout before returning failure. TransferZ does not accept a partially sorted container as a successful or normal failed result.

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

Click Sort on the right side of that container's header. TransferZ computes a rotation-aware compact layout and first tries a bounded in-cargo sequence. If that geometry has no safe intermediary path, it uses the hidden native sort buffer, then verifies the complete final layout. Any normal failure restores and verifies the exact original snapshot.

### Consolidate partial stacks

Click Stack on the right side of that container's header. Only pairs DayZ considers natively combinable are merged.

### Pick up all shown loose vicinity items

1. Select a receiving container with Destination.
2. Click vicinity Transfer, or hold `Shift` and left-drag one loose non-container vicinity item onto the destination.

### Move all shown vicinity items of one exact class

Hold `Alt` and left-drag one representative vicinity item of that class onto the destination container.

### Create a fast two-container workflow

1. Click Link on container A.
2. Click Link on container B.
3. Double-left-click items in either container to route them to the other.
4. Use **Preferred / Exact-class modifier** (default `Alt`) + Left Drag when you want to move every exact-class match as a batch.

## Dependency and Workshop

TransferZ Workshop: https://steamcommunity.com/sharedfiles/filedetails/?id=3799485223

TransferZ requires Community Framework (CF) on both client and server. Load CF before TransferZ.

CF Workshop: https://steamcommunity.com/sharedfiles/filedetails/?id=1559212036
