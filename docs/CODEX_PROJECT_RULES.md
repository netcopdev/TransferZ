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

Store the preferred personal target by the player's attachment slot, not by item classname. Replacement gear in the same slot should inherit the preference if it has cargo.

## Move validation

Move requests are executed on the server. Before moving an item, resolve current entities and locations again, verify that the sending player can reach the entities involved, respect cargo release/receive conditions, require free space in the exact selected cargo owner, and use DayZ inventory-location validation before the synchronized move.

Do not delete and recreate items to simulate transfer.

## Initial UI

Cargo headers use compact controls:

- `D` selects the destination.
- `T` transfers direct contents.
- `U` recursively unpacks leaf items.
- `L` starts, completes, or removes a temporary link.
- `P` stores a worn cargo container's attachment slot as the preferred personal target.

The controls extend the vanilla inventory rather than replacing it.

## Scope boundaries for 0.1

- No item-type filtering yet.
- No persistent world-container links.
- No TransferZ-owned partial stack splitting or merging.
- No automatic relocation of empty nested containers after Unpack.
- No class allowlists for container support.
- No custom replacement inventory screen.
