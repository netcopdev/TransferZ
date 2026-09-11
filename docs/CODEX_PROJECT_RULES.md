# TransferZ project rules

## Product contract

TransferZ makes inventory routing explicit and deterministic.

### Destination

A cargo-bearing entity may be selected as the active destination. Exact-destination operations target that entity's own cargo only. They must not silently fall back to arbitrary inventory space.

The selected destination is transient. Clear it when the entity no longer exists, no longer has cargo, moves into another player's inventory, or is no longer within normal inventory-manipulation reach.

### Transfer

Transfer snapshots the source container's direct cargo children and attempts to move them in source order. Cargo-bearing child containers move intact when DayZ permits the move. Failed items remain in place.

The `T` control may also be dragged from a source onto another cargo header to perform a one-off direct transfer to that destination without changing the selected `D` destination. Preserve normal mouse-wheel inventory scrolling while an operation control is being dragged.

Right-button dragging an item from cargo onto another cargo header performs an exact-class bulk transfer. The server must verify that the representative dragged item is still a direct cargo child of the source, derive the match from its server-side `GetType()`, snapshot only the source's direct cargo children, and attempt to move only exact type matches. Do not recurse into nested containers and do not broaden the match to inheritance or semantic categories.

### Unpack

Unpack traverses cargo. Cargo-bearing nodes remain where they are; non-container leaf items are collected and moved to the destination. Loose direct items therefore move as well. Attachments are outside the initial traversal scope.

A destination nested inside the source is rejected. Moving contents from a nested source upward to an ancestor destination is allowed.

The `U` control may also be dragged from a source onto another cargo header to perform a one-off direct unpack without changing the selected `D` destination.

### Vicinity batch actions

The `VICINITY` header exposes `T` and `U` controls.

- Vicinity `T` moves currently shown loose, takeable vicinity items except cargo-bearing containers into the destination.
- Vicinity `U` unpacks currently shown vicinity cargo-bearing containers into the destination while leaving the containers themselves in place.
- If the destination is itself in vicinity, skip it as a source and allow it to receive the other items.
- Vicinity `T` and `U` may also be dragged onto a cargo header for a one-off direct batch action.

### Links

Links are temporary client-session state. Pairing A and B gives deterministic A-to-B and B-to-A double-click routing for cargo icons. TransferZ keeps one active pair at a time.

Clicking `L` on either participant removes the pair. Clicking `L` on a different container while a pair exists removes the old pair and makes the clicked container the new pending anchor. A pending anchor is cancelled by clicking `L` on it again.

Clear a pending anchor or active pair when a participant no longer exists, no longer has cargo, moves into another player's inventory, or is no longer within normal inventory-manipulation reach. When no TransferZ route exists, vanilla double-click behavior must remain unchanged.

### Preferred personal destination

Store the preferred personal target as the ordered attachment-slot path from the player to the cargo-bearing destination, never by item classname.

Direct worn containers are one-hop paths such as `Back`. Nested attachment containers are multi-hop paths such as `Belt > DumpPouch` or `Vest > Pouch`. Replacing gear should preserve the preference when the currently equipped attachment hierarchy exposes the same slot path and the final entity has cargo.

If any path element cannot be resolved, or the final entity no longer has cargo, do not guess a replacement destination; leave vanilla routing in control.

Continue reading the legacy single `preferred_slot` preference as a compatibility fallback. Cargo-nested containers are not persistent preferred personal destinations in 0.1.

## Move validation

Move requests are executed on the server. Before moving an item, resolve current entities and locations again, verify that the sending player can reach the entities involved, respect cargo release/receive conditions, require free space in the exact selected cargo owner, and use DayZ inventory-location validation before the synchronized move.

Do not delete and recreate items to simulate transfer.

## Initial UI

Cargo headers use compact controls:

- `D` selects the destination.
- `T` transfers direct contents and is also a draggable direct-transfer handle.
- `U` unpacks nested leaf items and is also a draggable direct-unpack handle.
- `L` starts, completes, or removes the single temporary link pair.
- `P` stores the attached cargo container's attachment-slot path as the preferred personal target.

Hover tooltips must stay compact, use a dark semi-transparent background, remain close to the hovered control, and describe the currently relevant action or destination.

The controls extend the vanilla inventory rather than replacing it.

## Scope boundaries for 0.1

- No item-type filtering yet.
- No persistent world-container links.
- No TransferZ-owned partial stack splitting or merging.
- No automatic relocation of empty nested containers after Unpack.
- No persistent preferred personal targets for containers nested in cargo rather than attached through slots.
- No class allowlists for container support.
- No custom replacement inventory screen.
