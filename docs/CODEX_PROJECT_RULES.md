# TransferZ project rules

## Product contract

TransferZ makes inventory routing explicit and deterministic.

### Destination

A cargo-bearing entity may be selected as the active destination. Exact-destination operations target that entity's own cargo only. They must not silently fall back to arbitrary inventory space.

### Transfer

Transfer snapshots the source container's direct cargo children and attempts to move them in source order. Cargo-bearing child containers move intact when DayZ permits the move. Failed items remain in place.

### Unpack

Unpack recursively traverses cargo. Cargo-bearing nodes remain where they are; non-container leaf items are collected and moved to the destination. Loose direct items therefore move as well. Attachments are outside the initial traversal scope.

A destination nested inside the source is rejected. Moving contents from a nested source upward to an ancestor destination is allowed.

### Links

Links are temporary client-session state. Pairing A and B gives deterministic A-to-B and B-to-A double-click routing for cargo icons. When no TransferZ route exists, vanilla double-click behavior must remain unchanged.

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
- `T` transfers direct contents.
- `U` recursively unpacks leaf items.
- `L` starts, completes, or removes a temporary link.
- `P` stores the attached cargo container's attachment-slot path as the preferred personal target.

The controls extend the vanilla inventory rather than replacing it.

## Scope boundaries for 0.1

- No item-type filtering yet.
- No persistent world-container links.
- No TransferZ-owned partial stack splitting or merging.
- No automatic relocation of empty nested containers after Unpack.
- No persistent preferred personal targets for containers nested in cargo rather than attached through slots.
- No class allowlists for container support.
- No custom replacement inventory screen.
