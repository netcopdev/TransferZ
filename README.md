# TransferZ

TransferZ is a DayZ inventory-routing mod for moving, organizing, and consolidating loot between open containers, worn cargo, hands containers, and vicinity without replacing the vanilla inventory screen.

Current version: **0.1.0**.

For the complete control reference and examples, see [`docs/USAGE.md`](docs/USAGE.md).

## Quick start

1. Use the **Destination** target icon on the container that should receive items. The active destination is highlighted. The same control on `VICINITY` selects the ground/vicinity zone instead.
2. Use the **Transfer** arrow on a source container to move its direct cargo children to the active destination.
3. Use **Unpack** on a source container to extract leaf items from nested cargo containers while leaving the nested containers and the source's existing direct loose cargo in place.
4. Drag Transfer or Unpack directly onto another open container for a one-off operation without changing the active destination.
5. Use the **Sort** and **Stack** controls on the right side of a cargo header to organize that container in place.
6. Use the item shortcuts below for single items, source-zone batch operations, and exact-class batch moves.

## Header controls

Transfer/navigation controls remain on the **left** side of cargo headers. Container-maintenance controls are on the **right**.

| Control | Meaning |
| --- | --- |
| Destination target | Select or clear this container as the active destination. |
| Transfer arrow | Move the source container's direct cargo children to the active destination. Also draggable. |
| Unpack | Move leaf items out of nested cargo containers while leaving direct loose cargo and the nested containers in place. Also draggable. |
| Link | Start, complete, replace, or remove the current temporary container link pair. |
| Preferred pin | Store or clear this worn/attached cargo container as the persistent preferred personal destination. |
| Sort | Compact/reorder this container's direct cargo in place. |
| Stack | Merge compatible partial stacks in this container using DayZ's own combine rules. |

`VICINITY` exposes Destination, Transfer, and Unpack only.

Hover a control for a short explanation. Transfer and Unpack also show a subdued green/yellow/red preview for ready/partial/impossible operations.

## Item shortcuts

| Gesture | Action |
| --- | --- |
| `Shift + Click` | Move the clicked item to the active destination. |
| `Alt + Click` | Move the clicked item to the preferred personal destination. |
| `Shift + Left Drag` | Run the source zone's Transfer operation and drop it onto a destination. |
| `Alt + Left Drag` | Run the source zone's Unpack operation and drop it onto a destination. |
| `Right Drag` | Move all currently eligible items with the same exact `GetType()` as the dragged item to the drop target. |
| `Double Left Click` | Use link/preferred routing when TransferZ owns the route; otherwise preserve vanilla DayZ behavior. |
| `Double Right Click` | Exact-class batch version of TransferZ double-click routing. |

Unmodified left drag remains vanilla DayZ drag behavior. `Ctrl` interactions remain vanilla and are not reassigned by TransferZ.

### Right-drag exact-class transfer

Right-drag is an explicit same-class batch move.

From a cargo container, dragging one item selects all direct cargo items in that same source container whose exact `GetType()` matches the representative item. Drop onto another open TransferZ container to move those matches there. Dropping onto `VICINITY` moves those matching source-cargo items to the ground.

From `VICINITY`, right-drag selects currently shown loose, takeable, removable non-container items whose exact `GetType()` matches the representative item and moves that set into the destination container. Vicinity-to-vicinity is a no-op because those items are already there.

Different classnames are never included just because they are similar items. For example, right-dragging one ammunition classname moves only that exact ammunition classname.

## Destination behavior

A selected container always means that container's own cargo. TransferZ does not silently fall back to arbitrary player inventory or cargo nested inside the destination.

Selecting another destination replaces the current one. Container destinations are transient and are cleared automatically when the selected participant is no longer a valid open/in-hand reachable container. A `VICINITY` destination is cleared when the vicinity panel is closed.

## Transfer

Transfer snapshots the source container's **direct cargo children** and attempts to move them in source order. Cargo-bearing child containers move intact with their contents when DayZ accepts the move.

Example:

```text
Backpack
  Ammo Box
    ammo
  Medical Pouch
    bandage
  Knife
```

Transfer to a Barrel attempts to move `Ammo Box`, `Medical Pouch`, and `Knife` to the Barrel. The nested containers keep their contents.

When `VICINITY` is the destination, direct source items are dropped through DayZ's normal ground-inventory path.

## Unpack

Unpack traverses cargo-bearing direct children of the source and moves non-container leaf cargo found inside them. The source's direct loose cargo and the nested cargo containers themselves stay where they are.

Using the example above, Unpack to a Barrel attempts to move `ammo` and `bandage`. `Knife`, `Ammo Box`, and `Medical Pouch` remain in the Backpack.

Dragging Unpack back onto its own source container flattens nested cargo into that source while leaving its existing direct loose cargo alone.

Attachments are not traversed by Unpack in 0.1.0.

## Sort

Sort reorganizes the selected container's **direct cargo only**.

TransferZ first builds a complete deterministic layout plan using the container's native cargo grid. It then performs only validated in-cargo DayZ inventory moves. Items are not dropped, spawned, or recreated as temporary storage.

Current Sort behavior:

- larger direct items are placed first;
- like item types are kept deterministic/grouped after size priority;
- existing item orientation is preserved;
- nested containers are treated as ordinary direct cargo items and move intact;
- if no safe all-in-cargo rearrangement plan exists, Sort leaves the cargo alone rather than using an unsafe fallback.

## Stack

Stack scans the selected container's direct cargo and asks DayZ whether pairs can be combined. Only pairs accepted by DayZ's own `CanBeCombined` logic are merged with the native `CombineItems` behavior.

TransferZ does not define its own ammo-family/category matching, does not merge through nested containers, and does not split stacks to manufacture a merge.

## Vicinity

`VICINITY` behaves as a synthetic inventory zone:

- Destination: select ground/vicinity as the active destination.
- Transfer: move currently shown loose takeable vicinity items, excluding cargo-bearing containers, into the selected container.
- Unpack: unpack currently shown vicinity cargo containers into the destination while leaving those containers in place.
- `Shift + Click`: move one shown item to the active destination.
- `Alt + Click`: move one shown item to the preferred personal destination.
- `Shift + Left Drag`: run vicinity Transfer and drop onto a container.
- `Alt + Left Drag`: run vicinity Unpack and drop onto a container.
- `Right Drag`: move shown loose items of the dragged item's exact class into the drop target.

With `VICINITY` itself selected, vicinity Transfer is a no-op and vicinity Unpack empties shown cargo containers onto the ground.

## Links and double-click routing

Use Link on one container and then Link on another to create one temporary link pair.

- A pending first participant is highlighted separately from an active pair.
- Clicking Link on a linked participant removes the pair.
- Starting a new link drops the old pair.
- Link participants must remain valid/open/in-hand and reachable.

For cargo items, double-left-click routing priority is:

1. linked partner, when the immediate source container is linked;
2. otherwise the preferred destination for external or in-hand containers;
3. otherwise vanilla DayZ behavior.

Double-right-click uses the same destination resolution but sends exact `GetType()` matches from the immediate source cargo as a batch.

For vicinity, left double-click routes the selected item to the preferred destination; right double-click routes currently shown items of that exact class to the preferred destination.

## Preferred destination

The Preferred control is available on cargo-bearing items in the player's attachment hierarchy, including nested attachments such as a pouch attached to a belt or vest.

TransferZ stores the ordered attachment-slot path, for example:

```text
Back
Belt > DumpPouch
Vest > Pouch
```

The preferred path survives restarts and can resolve through replacement gear when the same attachment path exists. Selecting the currently preferred container again clears the preference. If the path cannot be resolved, TransferZ does not guess another destination.

Preferences are stored at:

```text
$profile:TransferZ/preferences.json
```

The older single `preferred_slot` format remains readable as a compatibility fallback.

## Safety

TransferZ never deletes and recreates items to simulate movement, sorting, or stacking.

The client requests operations; the server re-resolves entities and validates sender ownership, reachability, source removal, destination acceptance, exact cargo space, and DayZ inventory locations before moving anything. Items that no longer qualify or do not fit stay where they are.

Sort is deliberately conservative: if TransferZ cannot plan and execute the rearrangement using only the container's own cargo grid and native DayZ moves, it stops rather than using an external temporary location.

## Install

TransferZ requires **Community Framework (CF)** on both client and server:

- Steam Workshop: https://steamcommunity.com/sharedfiles/filedetails/?id=1559212036
- Load CF before TransferZ, for example: `-mod=@CF;@TransferZ`

Load `@TransferZ` on both client and server. Copy the supplied public `.bikey` into the server root `keys` directory.

TransferZ uses CF for registered RPC dispatch; CF is therefore a mandatory runtime dependency, not an optional admin-tool dependency.

## Build and sign

1. Copy `tools/build-config.example.psd1` to `%LOCALAPPDATA%\TransferZ\build.psd1`.
2. Set `PrivateKey` and `PublicKey`. Tool paths may be left blank when DayZ Tools is installed in a standard Steam library.
3. From the repository root run:

```powershell
.\tools\build.ps1
```

The signed package is created under:

```text
dist\release\@TransferZ\
  addons\
  keys\
  mod.cpp
```

## Current scope

TransferZ 0.1.0 intentionally does not provide arbitrary category filters, TransferZ-owned stack splitting, persistent world-container links, attachment traversal during Unpack, or persistent preferred targets for containers nested in cargo rather than attached through slots.

Exact-class matching is available through right-drag and right-double-click routing. Stack merging is available only through DayZ's own native compatibility rules.

## Development

Read `AGENTS.md` and `docs/CODEX_PROJECT_RULES.md` before modifying the project. `main` is the stable integration branch and feature work is branch-first.
