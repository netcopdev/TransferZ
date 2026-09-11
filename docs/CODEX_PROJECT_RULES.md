# TransferZ project rules

## Product contract

TransferZ makes inventory routing explicit and deterministic.

### Destination

A cargo-bearing entity may be selected as the active destination. Exact-container operations target that entity's own cargo only. They must not silently fall back to arbitrary inventory space.

`VICINITY` is also a valid destination. For that synthetic target, movement means DayZ's normal ground/vicinity drop path around the requesting player.

A selected container destination is transient. It is valid only while the corresponding inventory container is open or the entity is in the player's hands. Clear it when the entity is closed, stowed from the hands, no longer exists, no longer has cargo, moves into another player's inventory, or is no longer within normal inventory-manipulation reach. Clear a vicinity destination when the vicinity panel is closed.

### Transfer

Transfer snapshots the source container's direct cargo children and attempts to move them in source order. Cargo-bearing child containers move intact when DayZ permits the move. Failed items remain in place.

The `T` control may also be dragged from a source onto another visible cargo container field to perform a one-off direct transfer to that destination without changing the selected `D` destination. Preserve normal mouse-wheel inventory scrolling while an operation control is being dragged.

When vicinity is the destination, Transfer drops those direct source children through DayZ's normal inventory drop path.

### Unpack

Unpack traverses cargo. Cargo-bearing nodes remain where they are; non-container leaf items are collected and moved to the destination. Loose direct items therefore move as well. Attachments are outside the initial traversal scope.

A destination nested inside a different source is rejected. Moving contents from a nested source upward to an ancestor destination is allowed.

The source itself is a valid Unpack destination. Self-unpack flattens nested cargo leaves into the source while leaving direct loose items, which are already at the destination, untouched.

The `U` control may be dragged from a source onto another visible cargo container field, or back onto the source itself, for a one-off direct unpack without changing the selected `D` destination.

When vicinity is the destination, Unpack drops leaf items through DayZ's normal inventory drop path.

### Modifier item drags

Normal unmodified item drag remains vanilla behavior.

For an item that is a direct cargo child, latch exactly one held modifier when the left-button drag starts:

- `Ctrl + Drag`: exact-class bulk transfer. Resolve the dragged representative's immediate cargo owner, snapshot that owner's direct cargo children, and move only children whose `GetType()` exactly equals the representative's `GetType()`.
- `Shift + Drag`: equivalent to dragging the immediate source container's `T` handle. Transfer all direct cargo children from that source.
- `Alt + Drag`: equivalent to dragging the immediate source container's `U` handle. Unpack that source container into the drop destination.

Do not broaden exact-class matching to inheritance or arbitrary categories and do not recurse into nested cargo for the class filter. Failed or non-fitting matches remain in place.

The modifier is latched at drag start; releasing it during movement or mouse-wheel scrolling must not change the operation. If more than one of Ctrl/Shift/Alt is held at drag start, do not guess an operation; leave the drag to vanilla behavior.

TransferZ does not assign a bulk drag operation to the right mouse button.

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

## Initial UI

Cargo headers use compact controls:

- `D` selects the destination.
- `T` transfers direct contents and is also a draggable direct-transfer handle.
- `U` unpacks nested leaf items and is also a draggable direct-unpack handle.
- `L` starts, completes, replaces, or removes the single temporary link pair.
- `P` stores the attached cargo container's attachment-slot path as the preferred personal target.

Vicinity exposes `D`, `T`, and `U`.

T/U drag targets cover the visible destination container field rather than only the header. The temporary drag overlay must forward mouse-wheel scrolling to the appropriate native inventory scroller.

Hover tooltips must stay close to the hovered control, use a dark mostly-opaque background, and wrap onto additional lines instead of clipping longer messages. If state changes while a control remains hovered, rebuild both tooltip text and calculated geometry immediately rather than requiring mouse-out/mouse-in. Do not use the word `recursive` in player-facing tooltip text.

While `T` or `U` is hovered, preview the immediate expected result with a subdued translucent background:

- green: expected normal/full execution;
- yellow: likely partial execution, such as insufficient destination capacity or only some currently eligible items;
- red: currently impossible.

The status colors should remain subdued, but sufficiently saturated and opaque to read clearly behind the button text.

The preview is advisory only. It must never replace authoritative server-side move validation.

The controls extend the vanilla inventory rather than replacing it.

## Scope boundaries for 0.1

- No arbitrary item-category filtering; exact-class matching is the only bulk class filter.
- No persistent world-container links.
- No TransferZ-owned partial stack splitting or merging.
- No automatic relocation of empty nested containers after Unpack.
- No persistent preferred personal targets for containers nested in cargo rather than attached through slots.
- No class allowlists for container support.
- No custom replacement inventory screen.
