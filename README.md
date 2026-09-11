# TransferZ

TransferZ is a DayZ inventory-routing mod for moving items between containers quickly while keeping the destination explicit and predictable.

Current version: **0.1.0**.

## Quick user guide

1. **Choose where items should go** with `D`.
   - `D*` marks the active destination.
   - Click `D*` again to clear it.
   - `D` on `VICINITY` means the ground around you.
2. **Move a container's direct contents** with `T`.
   - Nested containers move intact when they fit.
3. **Empty nested containers** with `U`.
   - `U` moves items found inside nested cargo containers.
   - Loose items directly in the source stay where they are.
4. **For one-off moves**, drag `T` or `U` onto another container instead of changing `D*`.
5. **Fast item shortcuts**:
   - `Shift + Click` -> move that item to `D*`.
   - `Alt + Click` -> move that item to `P*`.
   - `Shift + Drag` -> batch `T` from that item's source container/zone.
   - `Alt + Drag` -> batch `U` from that item's source container/zone.
6. **Link two containers** with `L` on the first and `L` on the second. Double-clicking items then routes between the linked pair.
7. **Set a preferred personal container** with `P` on worn/attached cargo. `P*` is persistent and is used by preferred-routing shortcuts.

Hover the buttons for a short explanation. `T` and `U` also show a green/yellow/red preview for ready/partial/impossible operations.

## Header controls

| Control | Meaning |
| --- | --- |
| `D` | Select exact destination. `D*` is active. |
| `T` | Transfer direct cargo children. Can also be dragged to a destination. |
| `U` | Move leaf items from nested cargo containers while leaving direct loose cargo in place. Can also be dragged. |
| `L` | Start, complete, replace, or remove a temporary container link. |
| `P` | Set or clear the persistent preferred attached-container destination. |

The controls are placed between DayZ's native left-side header controls and the header title. Native move/reorder handles keep their own reserved area.

## Destination behavior

A selected container always means that container's own cargo. TransferZ does not silently fall back to arbitrary player inventory or cargo nested inside the destination.

Container destinations are temporary. They are cleared when the selected container is no longer a valid open/in-hand participant, becomes unreachable, or otherwise stops being usable. A `VICINITY` destination is cleared when the vicinity panel is closed.

## Transfer (`T`)

`T` moves the source container's **direct cargo children** to the selected destination.

Example:

```text
Backpack
  Ammo Box
    ammo
  Medical Pouch
    bandage
  Knife
```

`T -> Barrel` attempts to move `Ammo Box`, `Medical Pouch`, and `Knife`. The nested containers keep their contents.

When `VICINITY` is the destination, direct source items are dropped using DayZ's normal ground inventory path.

## Unpack (`U`)

`U` moves non-container leaf items found **inside nested cargo-bearing child containers**. Direct loose cargo in the source is not moved.

Using the example above, `U -> Barrel` attempts to move `ammo` and `bandage`. `Knife`, `Ammo Box`, and `Medical Pouch` stay in the Backpack.

Dragging `U` back onto its own source container flattens nested cargo into that source while still leaving existing direct loose cargo alone.

Attachments are not traversed by `U` in 0.1.0.

## Vicinity actions

`VICINITY` has `D`, `T`, and `U`.

- `D`: use vicinity/ground as `D*`.
- `T`: move shown loose takeable vicinity items, excluding cargo containers, into the selected container.
- `U`: unpack shown vicinity cargo containers into the selected destination while leaving those containers in place.
- With `VICINITY` itself selected, vicinity `T` is a no-op and vicinity `U` empties shown containers onto the ground.

The vicinity `T` and `U` buttons can also be dragged directly onto a container.

## Modifier shortcuts

Unmodified drag remains normal DayZ behavior.

- `Shift + Click`: move one cargo/vicinity item to `D*`.
- `Alt + Click`: move one cargo/vicinity item to `P*`.
- `Shift + Drag`: perform source-zone `T`.
- `Alt + Drag`: perform source-zone `U`.

For cargo items, the source zone is the item's immediate cargo owner. For vicinity items, the source zone is the visible vicinity list.

`Ctrl + Click` remains vanilla DayZ behavior. TransferZ does not assign a special right-button drag action.

## Links and double-click routing

Press `L` on one container and then `L` on another to create one temporary link pair.

- `L+` = waiting for the second container.
- `L*` = linked participant.
- Clicking `L` on a linked participant removes the link.
- Starting a new link drops the old pair.
- Link participants must remain valid/open/in-hand participants.

Double-left-click routing priority for cargo items is:

1. linked partner, when the source container is linked;
2. otherwise `P*` for external or in-hand containers;
3. otherwise vanilla DayZ behavior.

Double-right-click is the exact-class batch version of the same route: matching direct source-cargo items with the same exact `GetType()` are attempted against the linked destination first, otherwise `P*`.

For vicinity, left double-click routes the selected item to `P*`, while right double-click routes shown items of that exact class to `P*`.

## Preferred destination (`P`)

`P` is available on cargo-bearing items in the player's attachment hierarchy, including nested attachments such as a pouch attached to a belt or vest.

TransferZ stores the ordered attachment-slot path, for example:

```text
Back
Belt > DumpPouch
Vest > Pouch
```

The preferred path survives restarts and can resolve through replacement gear when the same attachment path exists. If the path cannot be resolved, TransferZ does not guess another destination.

Preferences are stored at:

```text
$profile:TransferZ/preferences.json
```

The older single `preferred_slot` format remains readable as a compatibility fallback.

## Safety

TransferZ never deletes and recreates items to simulate movement.

The client requests operations; the server re-resolves entities and validates reachability, source removal, destination acceptance, exact cargo space, and DayZ inventory locations before moving anything. Items that no longer qualify or do not fit stay where they are.

## Build and sign

TransferZ uses a local build configuration so machine-specific paths are not committed.

1. Copy `tools/build-config.example.psd1` to `%LOCALAPPDATA%\TransferZ\build.psd1`.
2. Set `PrivateKey` and `PublicKey`. Tool paths may be left blank when DayZ Tools is in a standard Steam library.
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

Load `@TransferZ` on both client and server, and copy the public `.bikey` into the server root `keys` directory.

## Current scope

TransferZ 0.1.0 intentionally does not provide arbitrary item-category filters, partial stack splitting/merging, persistent world-container links, attachment traversal during `U`, or persistent preferred targets for containers nested in cargo rather than attached through slots.

## Development

Read `AGENTS.md` and `docs/CODEX_PROJECT_RULES.md` before modifying the project. `main` is the stable integration branch and feature work is branch-first.
