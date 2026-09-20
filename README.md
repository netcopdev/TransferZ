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
6. Use `Shift + Left Drag` for a whole source-zone transfer and `Alt + Left Drag` for an exact-class batch move.

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
| `Shift + Left Drag` | Move the source container's direct cargo as a batch. From vicinity, move the shown eligible loose items. |
| `Alt + Left Drag` | Move all eligible items of the dragged item's exact `GetType()` from the same source zone. |
| `Double Left Click` | Use link/preferred routing when TransferZ owns the route; otherwise preserve vanilla DayZ behavior. |

Unmodified left drag remains vanilla DayZ drag behavior. `Ctrl` interactions remain vanilla and are not reassigned by TransferZ.

**Right click is not assigned to TransferZ operations.** Normal DayZ right-click behavior, including stack splitting, remains available.

### Exact-class transfer with Alt drag

From a cargo container, `Alt + Left Drag` on one item selects all direct cargo items in that same source container whose exact `GetType()` matches the representative item. Drop onto another open TransferZ container to move those matches there. Dropping onto `VICINITY` moves those matching source-cargo items to the ground.

From `VICINITY`, `Alt + Left Drag` selects currently shown loose, takeable, removable items whose exact `GetType()` matches the representative item, including same-class containers (which move intact with their contents), and moves that set into the destination container. Vicinity-to-vicinity is a no-op because those items are already there.

Different classnames are never included just because they are similar items. For example, Alt-dragging one ammunition classname moves only that exact ammunition classname.

## Native stack splitting and P*

TransferZ does not replace DayZ's stack-splitting rules. Right click still invokes the native split behavior; TransferZ only chooses where the new split stack is placed, in this order:

1. the active destination (`D*`), when one is selected: that container's exact cargo, or DayZ's normal ground placement around the player for `VICINITY`;
2. otherwise, for a stack in cargo, the same immediate container when it has room for the new stack;
3. otherwise the preferred destination (`P*`), when one is configured;
4. otherwise normal DayZ behavior.

If `D*` cannot accept the split, routing continues to the original source cargo, then `P*`. If the source cannot take it, routing continues to `P*`; if `P*` also cannot accept it, normal DayZ behavior takes over. See [`docs/SPLIT_ROUTING.md`](docs/SPLIT_ROUTING.md).

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

Sort reorganizes the selected container's **direct cargo only** while preserving the original item entities.

TransferZ snapshots each direct item's exact row, column and orientation and computes a deterministic rotation-aware target layout. It first tries a bounded move sequence entirely inside the source cargo, using free cells and native atomic swaps where useful. If the final layout fits but no in-cargo path exists, TransferZ stages the same item entities through its own hidden, non-physical native cargo buffer and places them directly into their final cells. Sort never uses ground/vicinity or arbitrary player inventory as staging, and it never deletes/recreates items.

Current Sort behavior:

- current item orientation is preserved whenever a complete layout permits it;
- rotation is a fallback for tighter layouts rather than the default;
- detachable magazines prefer vertical orientation only after an all-current-orientation layout has failed;
- equivalent items already in valid final slots are kept stationary when possible;
- native DayZ move validation remains authoritative for every step;
- if no bounded in-cargo rearrangement exists, Sort uses the dedicated hidden native cargo buffer; a normal failure restores the exact original layout and never falls back to ground staging.

Sort is transactional at the TransferZ level. The in-cargo path journals inverse moves/swaps and reverses them in strict reverse order on failure. The buffer path uses the immutable original snapshot to evacuate displaced items back through the buffer and restore exact original cells. The buffer is deleted only when verified empty. A rollback invariant violation is logged as critical rather than accepted as a partial sort.

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
- `Shift + Left Drag`: move the shown eligible loose items as a transfer batch.
- `Alt + Left Drag`: move shown eligible items of the dragged item's exact class, including same-class cargo-bearing containers.

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

For vicinity, left double-click routes the selected item to the preferred destination. TransferZ does not assign double-right-click behavior.

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

The client requests operations; the server re-resolves entities and validates sender ownership, reachability, source removal, destination acceptance, exact cargo space, and DayZ inventory locations before moving anything.

Sort has an additional rollback contract: original cargo coordinates and orientation are captured before execution, and any failed transaction must restore and verify that snapshot before returning a normal failure. In-cargo execution reverses its move journal; buffered execution restores through the hidden native cargo buffer. Absolute recovery still depends on the DayZ native inventory API continuing to accept valid recovery moves; engine-level refusal or process termination cannot be made atomic purely in script.

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

Exact-class matching is available through `Alt + Left Drag`. Stack merging is available only through DayZ's own native compatibility rules.

## Development

Read `AGENTS.md` and `docs/CODEX_PROJECT_RULES.md` before modifying the project. `main` is the stable integration branch and feature work is branch-first.
