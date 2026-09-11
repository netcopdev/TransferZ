# TransferZ project rules

## Product contract

TransferZ makes inventory routing explicit and deterministic.

### Destination

A cargo-bearing entity may be selected as the active destination. Exact-container operations target that entity's own cargo only. They must not silently fall back to arbitrary inventory space.

`VICINITY` is also a valid destination. For that synthetic target, movement means DayZ's normal ground/vicinity drop path around the requesting player.

A selected container destination is transient. It is valid only while the corresponding inventory container is open or the entity is in the player's hands. Clear it when the entity is closed, stowed from the hands, no longer exists, no longer has cargo, moves into another player's inventory, or is no longer within normal inventory-manipulation reach. Clear a vicinity destination when the vicinity panel is closed.

### Transfer

Transfer snapshots the source container's direct cargo children and attempts to move them in source order. Cargo-bearing child containers move intact when DayZ permits the move. Failed items remain in place.

The `T` control may also be dragged from a source onto another visible cargo container field to perform a one-off direct transfer without changing the selected `D` destination. Preserve normal mouse-wheel inventory scrolling while an operation control is being dragged.

When vicinity is the destination, Transfer drops those direct source children through DayZ's normal inventory drop path.

### Unpack

Container Unpack traverses only through cargo-bearing direct children of the source. The source's direct loose cargo remains in place. Cargo-bearing child containers also remain in place, while non-container leaf items found inside those child containers are collected recursively and moved to the destination. Attachments are outside the initial traversal scope.

A destination nested inside a different source is rejected. Moving contents from a nested source upward to an ancestor destination is allowed.

The source itself is a valid Unpack destination. Self-unpack flattens leaf cargo from nested child containers into the source while leaving the source's existing direct loose items untouched.

The `U` control may be dragged from a source onto another visible cargo container field, or back onto the source itself, for a one-off direct unpack without changing the selected `D` destination.

When vicinity is the destination, a normal container Unpack drops only leaf items originating inside its nested cargo-bearing child containers through DayZ's normal inventory drop path.

Vicinity `U` is intentionally zone-oriented rather than equivalent to applying normal container `U` to the vicinity list itself: it selects the shown cargo-bearing vicinity containers as sources, ignores loose vicinity items, and unpacks their contents into the destination.

### Modifier item drags

Normal unmodified item drag remains vanilla behavior.

TransferZ owns two left-button modifier drags:

- `Shift + Drag`: equivalent to dragging the source zone's `T` handle.
- `Alt + Drag`: equivalent to dragging the source zone's `U` handle.

For a direct cargo child, the source zone is its immediate cargo owner. Shift transfers that container's direct cargo children; Alt extracts only cargo contained inside that container's nested cargo-bearing children and leaves its direct loose cargo untouched.

For an item shown in `VICINITY`, the source zone is the current vicinity list. Shift behaves like vicinity `T`; Alt behaves like vicinity `U`. These operations must work from vicinity to a container, from a container to vicinity, and between normal container targets. Shift from vicinity to vicinity is a no-op because loose vicinity items are already at that destination; Alt from vicinity to vicinity unpacks shown vicinity containers onto the ground.

Do not assign `Ctrl + Drag` to TransferZ. Stock DayZ owns Ctrl+click as immediate drop-to-ground and TransferZ must not compete with or suppress that interaction. Right-button drag also remains outside TransferZ.

### Modifier item clicks

A modifier click is a single-item route, distinct from the source-zone batch behavior of the same modifier followed by an actual drag.

- `Shift + Click`: move the clicked cargo/vicinity item to the active `D*` destination.
- `Alt + Click`: move the clicked cargo/vicinity item to the resolved preferred `P*` destination.

`D*` may be a cargo container or `VICINITY`. If the requested destination is missing, invalid, already owns the item in the requested location, or cannot accept it, the item stays where it is. `Alt + Click` does nothing when no valid `P*` resolves.

`Ctrl + Click` remains vanilla DayZ behavior and must not be intercepted by TransferZ.

### Vicinity batch actions

The `VICINITY` header exposes `D`, `T`, and `U` controls.

- Vicinity `D` selects vicinity/ground as the active destination.
- Vicinity `T` moves currently shown loose, takeable vicinity items except cargo-bearing containers into a selected container destination.
- Vicinity `U` unpacks currently shown vicinity cargo-bearing containers into the destination while leaving the containers themselves in place.
- If a selected container destination is itself in vicinity, skip it as a source and allow it to receive the other items.
- With vicinity itself selected, vicinity `T` is a no-op because those loose items are already there; vicinity `U` unpacks shown containers onto the ground.
- Vicinity `T` and `U` may also be dragged onto a visible cargo container field for a one-off direct batch action.

### Links and double-click routing

Links are temporary client-session state. Pairing A and B gives deterministic A-to-B and B-to-A double-click routing for cargo icons. TransferZ keeps one active pair at a time.

Clicking `L` on either participant removes the pair. Clicking `L` on a different container while a pair exists removes the old pair and makes the clicked container the new pending anchor. A pending anchor is cancelled by clicking `L` on it again.

A pending anchor or linked participant must remain open or in the player's hands. Clear a pending anchor or active pair when a participant is closed, stowed from the hands, no longer exists, no longer has cargo, moves into another player's inventory, or is no longer within normal inventory-manipulation reach.

Double-left-click routing precedence for a cargo item is:

1. If the immediate source container has an active link, route exactly to the linked container.
2. Otherwise, if the item is in an external container or a container in hands, route it to the resolved preferred `P*` destination when available.
3. Otherwise, if the item is in the player's worn/attached inventory, leave vanilla DayZ double-click behavior in control so the item is taken/swapped to hands.
4. If no TransferZ route applies, preserve vanilla behavior.

A source at or below the entity currently held in hands counts as an in-hand container for rule 2, even though its hierarchy root is the player.

Double-right-click remains the exact-class batch variant of the same TransferZ destination resolution: exact `GetType()` matches in the immediate source cargo are moved to an active link destination first, otherwise to the preferred destination. This is separate from dragging; right-button drag itself has no TransferZ bulk behavior.

For vicinity, left double-click routes the selected item to `P*`. Right double-click routes all currently shown vicinity items of that exact class to `P*`.

### Preferred personal destination

Store the preferred personal target as the ordered attachment-slot path from the player to the cargo-bearing destination, never by item classname.

Direct worn containers are one-hop paths such as `Back`. Nested attachment containers are multi-hop paths such as `Belt > DumpPouch` or `Vest > Pouch`. Replacing gear should preserve the preference when the currently equipped attachment hierarchy exposes the same slot path and the final entity has cargo.

If any path element cannot be resolved, or the final entity no longer has cargo, do not guess a replacement destination; leave vanilla routing in control.

Continue reading the legacy single `preferred_slot` preference as a compatibility fallback. Cargo-nested containers are not persistent preferred personal destinations in 0.1.

## Move validation

Move requests are executed on the server. Before moving an item, resolve current entities and locations again, verify that the sending player can reach the entities involved, respect cargo release/receive conditions, require free space in the exact selected cargo owner, and use DayZ inventory-location validation before the synchronized move.

Vicinity/ground destinations use DayZ's standard inventory drop path. Do not invent arbitrary world placement when a native inventory drop operation exists.

Do not delete and recreate items to simulate transfer.

## UI contract

Cargo headers use compact controls:

- `D` selects the destination.
- `T` transfers direct contents and is also a draggable direct-transfer handle.
- `U` extracts leaf cargo from nested child containers while leaving direct loose cargo in place; it is also a draggable direct-unpack handle.
- `L` starts, completes, replaces, or removes the single temporary link pair.
- `P` stores the attached cargo container's attachment-slot path as the preferred personal target.

Vicinity exposes `D`, `T`, and `U`.

The TransferZ button block sits on the left side of the header **after DayZ's native left-side preview/move controls and before the title**. The native hover-only move/reorder panel must have reserved space even while hidden. Do not let TransferZ controls overlap native handles.

DayZ may finish sizing header previews and cargo widgets after TransferZ's first setup call. Initial placement therefore uses bounded deferred GUI-layout correction after the immediate pass. Keep this initialization-only; do not introduce per-frame layout polling.

T/U drag targets cover the visible destination container field rather than only the header. The temporary drag overlay must forward mouse-wheel scrolling to the appropriate native inventory scroller.

Hover tooltips must stay close to the hovered control, use a dark mostly-opaque background, and wrap onto additional lines instead of clipping longer messages. If state changes while a control remains hovered, rebuild both tooltip text and calculated geometry immediately rather than requiring mouse-out/mouse-in. Do not use the word `recursive` in player-facing tooltip text.

The button block has a subdued common background with individual hover highlights. While `T` or `U` is hovered, preview the immediate expected result with a subdued translucent background:

- green: expected normal/full execution;
- yellow: likely partial execution, such as insufficient destination capacity or only some currently eligible items;
- red: currently impossible.

The preview is advisory only. It must never replace authoritative server-side move validation.

TransferZ extends the vanilla inventory rather than replacing it. Native header dragging/reordering must remain available outside the TransferZ button rectangle; clicking `D`, `L`, or `P` must not accidentally initiate native header dragging.

## Source layout and extension staging

Use descriptive filenames. Do not add `Z`, `ZZ`, `ZZZZ`, or similar alphabetical load-order prefixes.

The existing late mission-UI extension layers use the explicit pattern:

```text
TransferZ_Widget_<stage>_<purpose>.c
```

The numeric stages document intentional extension order after the base TransferZ UI hooks. Keep the numbering sparse and preserve established order when a new stage is truly required. Prefer moving behavior into the owning subsystem over adding new stages.

Avoid direct helper calls across separate `modded class` layers when normal override flow can perform the work. This previously caused Enforce compile failures even when filename ordering appeared correct.

## Scope boundaries for 0.1

- No arbitrary item-category filtering; exact-class matching is available through double-right-click routing only.
- No persistent world-container links.
- No TransferZ-owned partial stack splitting or merging.
- No automatic relocation of empty nested containers after Unpack.
- No persistent preferred personal targets for containers nested in cargo rather than attached through slots.
- No class allowlists for container support.
- No custom replacement inventory screen.
