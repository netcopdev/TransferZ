# TransferZ

TransferZ is a DayZ inventory-routing mod focused on deterministic, low-friction movement between containers.

## Status

Current development target: **0.1 prototype** on `feature/core-transfer-routing`.

The prototype currently includes exact destination selection, direct bulk transfer, nested-content unpacking, draggable transfer/unpack handles, exact-class right-drag transfer, temporary container links, linked-container double-click routing, a persistent preferred personal destination, and vicinity-wide transfer/unpack actions.

## Core behavior

### Select a destination

Every supported cargo header gets compact TransferZ controls. Press `D` on a cargo-bearing item or world container to select that exact entity as the active destination. `VICINITY` also exposes `D` and may be selected as the destination.

A container destination always targets that entity's own cargo. It does not fall back to arbitrary player inventory or cargo nested inside the destination. A `VICINITY` destination means accessible ground space around the player.

Container destinations are transient and valid only while their inventory entry is open or the container is in the player's hands. If a selected container is closed, stowed from the hands, becomes unreachable, or otherwise stops being a valid cargo participant, TransferZ clears it. A vicinity destination is cleared when the vicinity panel is closed.

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

For a one-off direct transfer, drag the source header's `T` handle onto the destination container field. This does not replace or change the currently selected `D` destination. The destination's full visible container field is a drop target, not only its header, and mouse-wheel scrolling remains available while an operation handle is dragged.

When `VICINITY` is the destination, Transfer moves the source container's direct cargo children to the ground around the player.

### Unpack

Press `U` on a source container to collect non-container leaf items from its cargo tree and move them into the selected destination.

In the example above, `Unpack -> Barrel` attempts to move `ammo`, `bandage`, and `Knife`. The Ammo Box and Medical Pouch remain where they are.

Empty nested containers are not moved automatically. Attachments are not traversed by Unpack in 0.1.

For a one-off direct unpack, drag the source header's `U` handle onto the destination container field. `U` may also target its **own source container**: in that case TransferZ flattens nested cargo leaves into the source while leaving direct loose items alone.

When `VICINITY` is the destination, Unpack moves leaf items from the source cargo tree to the ground around the player.

### Exact-class right-drag

Normal left-button item drag remains vanilla DayZ behavior.

Right-button dragging an item from cargo onto a TransferZ destination performs an exact-class bulk move. TransferZ snapshots the item's immediate source cargo and attempts to move every direct child whose `GetType()` exactly matches the dragged representative item.

For example, right-dragging one 5.56 ammo pile moves the other piles of that exact DayZ class from the same source. It does **not** move all ammunition categories and it does not recurse into nested containers. Failed or non-fitting matches remain in place.

### Vicinity batch actions

The `VICINITY` header exposes `D`, `T`, and `U` controls.

- `D` makes vicinity/ground the active destination.
- `T` moves the currently shown loose, takeable vicinity items that are **not cargo containers** into a selected container destination.
- `U` unpacks the contents of the currently shown vicinity cargo containers into the selected destination while leaving the containers themselves in place.
- If the selected container destination is itself in vicinity, TransferZ skips it as a source and allows it to receive the other items.
- If vicinity itself is the destination, vicinity `T` is a no-op because loose items are already there, while vicinity `U` empties leaf contents from the shown containers onto the ground.

The vicinity `T` and `U` handles can also be dragged directly onto a destination container field for a one-off batch operation without changing the selected `D` destination.

### Link two containers

Press `L` on the first container, then `L` on the second. TransferZ keeps a single temporary pair for the current client session.

While linked, double-clicking an item in one container requests an exact move to the other container. Clicking `L` on either participant removes that pair. If a pair already exists and `L` is clicked on another container, the old pair is dropped and that container becomes the new pending `L+` anchor. A pending first link click can be cancelled by clicking `L` on the same container again.

Link participants are transient and must remain open or in the player's hands. Closing, stowing, losing reach of, or otherwise invalidating either participant clears the link. If TransferZ has no linked route for a cargo icon, vanilla double-click behavior is left unchanged.

### Preferred personal destination

Press `P` on a cargo-bearing item attached anywhere in the player's attachment hierarchy. This includes direct worn storage such as a backpack or vest and nested cargo-bearing attachments such as a dump pouch hanging from a belt.

TransferZ stores the **ordered attachment-slot path from the player to the destination**, not item classnames. Examples:

```text
Back
Belt > DumpPouch
Vest > Pouch
Back > AttachedPouch
```

At runtime TransferZ walks that slot path through the currently equipped attachment tree. Replacing gear with another item that exposes the same attachment path therefore keeps the preference valid. If any path element is missing or the final attachment has no cargo, TransferZ does not guess another destination and vanilla behavior is used.

Loose vicinity items that are double-clicked are routed to the resolved preferred destination.

The preference is stored in:

```text
$profile:TransferZ/preferences.json
```

Older profiles containing only the original single `preferred_slot` value remain readable as a compatibility fallback.

## Header controls

| Control | Meaning |
| --- | --- |
| `D` | Select exact destination; on `VICINITY`, select ground/vicinity |
| `T` | Transfer direct contents to selected destination; drag onto another destination field for a direct one-off transfer |
| `U` | Unpack nested leaf items to selected destination; drag onto another destination field for a direct one-off unpack |
| `L` | Start, complete, replace, or remove the temporary container link |
| `P` | Set this attached cargo container's slot path as preferred personal destination |

`D*`, `L*`, `L+`, and `P*` indicate current state. Hovering a control shows a compact dark tooltip close to the button.

While hovering `T` or `U`, its background previews the immediate operation result using subdued translucent colors:

- green: the operation is expected to execute normally;
- yellow: partial execution is likely, usually because available destination space is insufficient or only some items currently qualify;
- red: the operation is currently impossible.

This is an advisory client-side preview. The server still revalidates every actual move against current DayZ inventory state.

## Inventory safety

TransferZ does not delete and recreate items. Requests are resolved and executed on the server using DayZ inventory locations and normal move validation.

For every move, the server re-resolves the entities, checks reachability, rejects other-player inventory roots, checks cargo release/receive conditions, requires free space in the exact selected cargo owner, validates the source/destination locations, and only then performs the inventory move. Vicinity moves use DayZ's normal inventory drop path.

Items that no longer qualify or do not fit remain where they are.

## Deliberate 0.1 limitations

- No arbitrary item-category filtering yet; right-drag bulk matching is exact class only.
- No partial stack splitting or TransferZ-owned stack merging.
- Links are not persistent.
- Unpack traverses cargo only, not attachments.
- Preferred personal destinations follow attachment-slot paths only; cargo-nested containers are not persisted as personal destinations.
- UI controls are intentionally compact while interaction behavior is being validated.
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
