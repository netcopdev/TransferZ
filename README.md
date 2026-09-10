# TransferZ

TransferZ is a DayZ inventory-routing mod focused on deterministic, low-friction movement between containers.

## Status

Current development target: **0.1 prototype** on `feature/core-transfer-routing`.

The first implementation adds exact destination selection, direct bulk transfer, recursive unpacking, temporary container links, linked-container double-click routing, and a persistent preferred personal destination.

## Core behavior

### Select a destination

Every supported cargo header gets compact TransferZ controls. Press `D` on a cargo-bearing item or world container to select that exact entity as the active destination.

TransferZ intentionally targets the selected entity's own cargo. It does not fall back to arbitrary player inventory or to cargo nested inside the selected destination.

### Transfer

Press `T` on a source container to move its **direct cargo children** to the selected destination.

Nested cargo-bearing containers are transferred as containers when DayZ permits the move. Their contents are not flattened.

Example:

```text
Backpack
  Ammo Box
    ammo
  Medical Pouch
    bandage
  Knife
```

`Transfer -> Barrel` attempts to move `Ammo Box`, `Medical Pouch`, and `Knife`.

### Unpack

Press `U` on a source container to recursively collect non-container leaf items and move them into the selected destination.

In the example above, `Unpack -> Barrel` attempts to move `ammo`, `bandage`, and `Knife`. The Ammo Box and Medical Pouch remain where they are.

Empty nested containers are not moved automatically. Attachments are not traversed by Unpack in 0.1.

### Link two containers

Press `L` on the first container, then `L` on the second. The pair is held only for the current client session.

While linked, double-clicking an item in one container requests an exact move to the other container. Clicking `L` on an already linked container removes that pair. A pending first link click can be cancelled by clicking `L` on the same container again.

If TransferZ has no linked route for a cargo icon, vanilla double-click behavior is left unchanged.

### Preferred personal destination

Press `P` on a cargo-bearing item worn directly in a player attachment slot, such as a backpack or vest. TransferZ stores the **slot name**, not the item's classname.

When a loose vicinity item is double-clicked, TransferZ routes it to the currently equipped cargo-bearing item in that preferred slot. If there is no usable preferred destination, vanilla behavior is used.

The preference is stored in:

```text
$profile:TransferZ/preferences.json
```

## Header controls

| Control | Meaning |
| --- | --- |
| `D` | Select exact destination |
| `T` | Transfer direct contents to selected destination |
| `U` | Recursively unpack leaf items to selected destination |
| `L` | Start/complete/remove a temporary container link |
| `P` | Set this worn container's attachment slot as preferred personal destination |

`D*`, `L*`, `L+`, and `P*` indicate state on a header that has refreshed since the corresponding action.

## Inventory safety

TransferZ does not delete and recreate items. Requests are resolved and executed on the server using DayZ inventory locations and normal move validation.

For every move, the server re-resolves the entities, checks reachability, rejects other-player inventory roots, checks cargo release/receive conditions, requires free space in the exact selected cargo owner, validates the source/destination locations, and only then performs a server inventory move.

Items that no longer qualify or do not fit remain where they are.

## Deliberate 0.1 limitations

- No type/category filtering yet.
- No partial stack splitting or TransferZ-owned stack merging.
- Links are not persistent.
- Unpack traverses cargo only, not attachments.
- UI controls are intentionally minimal while interaction behavior is being validated.
- The current scripts still require a real DayZ/DayZ Tools compile and in-game validation before the prototype should be treated as release-ready.

## Build and sign

TransferZ follows the PaintZ-style local build configuration: machine-specific paths stay outside Git.

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

Load `@TransferZ` on both client and server, and copy the public `.bikey` to the server root `keys` directory.

## Development rules

Read `AGENTS.md` and `docs/CODEX_PROJECT_RULES.md` before modifying the project. Work is branch-first; `main` is not merged without explicit approval.
