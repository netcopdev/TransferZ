# TransferZ project rules

## Product contract

TransferZ makes inventory routing explicit and deterministic.

### Destination

A cargo-bearing entity may be selected as the active destination. Exact-container operations target that entity's own cargo only. They must not silently fall back to arbitrary inventory space.

`VICINITY` is also a valid destination. For that synthetic target, movement means DayZ's normal ground/vicinity drop path around the requesting player.

A selected container destination is transient. It is valid only while the corresponding inventory container is open or the entity is in the player's hands. Clear it when the entity is closed, stowed from the hands, no longer exists, no longer has cargo, moves into another player's inventory, or is no longer within normal inventory-manipulation reach. Clear a vicinity destination when the vicinity panel is closed.

Selecting another valid destination replaces the current destination.

### Transfer

Transfer snapshots the source container's direct cargo children and attempts to move them in source order. Cargo-bearing child containers move intact when DayZ permits the move. Failed items remain in place.

The Transfer control may also be dragged from a source onto another visible cargo container field to perform a one-off direct transfer without changing the selected destination. Preserve normal mouse-wheel inventory scrolling while an operation control is being dragged.

When vicinity is the destination, Transfer drops those direct source children through DayZ's normal inventory drop path.

### Unpack

Container Unpack traverses only through cargo-bearing direct children of the source. The source's direct loose cargo remains in place. Cargo-bearing child containers also remain in place, while non-container leaf items found inside those child containers are collected recursively and moved to the destination. Attachments are outside the initial traversal scope.

A destination nested inside a different source is rejected. Moving contents from a nested source upward to an ancestor destination is allowed.

The source itself is a valid Unpack destination. Self-unpack flattens leaf cargo from nested child containers into the source while leaving the source's existing direct loose items untouched.

The Unpack control may be dragged from a source onto another visible cargo container field, or back onto the source itself, for a one-off direct unpack without changing the selected destination.

When vicinity is the destination, a normal container Unpack drops only leaf items originating inside its nested cargo-bearing child containers through DayZ's normal inventory drop path.

Vicinity Unpack is intentionally zone-oriented rather than equivalent to applying normal container Unpack to the vicinity list itself: it selects the shown cargo-bearing vicinity containers as sources, ignores loose vicinity items, and unpacks their contents into the destination.

### Sort

Sort is a server-authoritative maintenance operation over the source container's direct cargo children. It never deletes or recreates sorted items and must preserve the identity of every original `EntityAI`.

- Snapshot every direct cargo child's exact original row, column and orientation before the first move.
- Build a complete deterministic rotation-aware target layout before execution.
- Use DayZ cargo dimensions, native inventory locations and native move validation.
- Treat DayZ user-reserved inventory locations as unavailable.
- Consider both valid cargo orientations for non-square items. Prefer the current orientation when fit quality is otherwise equivalent.
- If the computed target layout already matches the snapshot, Sort is a successful no-op.
- When movement is required, temporarily stage the same tracked entities through DayZ's normal vicinity/ground path so the source cargo becomes empty. Do not use arbitrary player cargo as an implicit staging area.
- Preflight every exact target move while the source is empty before committing the first target placement.
- Report success only after every tracked item is verified at its exact target row, column and orientation.
- Any staging, preflight, commit or final-verification failure starts synchronous rollback. Clear partial target placements, restore every tracked item to its exact original row, column and orientation, and verify the original snapshot before returning failure.
- No ordinary failed Sort may intentionally leave a tracked item in vicinity or leave a partially sorted source. A rollback invariant violation is critical, must be logged explicitly, and must trigger a final containment attempt back into the source rather than being treated as an acceptable partial result.
- Do not serialize/reconstruct weapon, magazine, attachment, chamber, quantity or mod-defined state. Preservation comes from moving the same entity objects.
- Keep equivalent-target assignment bounded for large cargo. Preserve records already occupying valid equivalent target slots first, then assign remaining records directly using cached slot-overlap data. Do not use iterative all-pairs improvement passes; assignment optimization must remain quadratic and must never compromise target-layout validity or transactional rollback.
- Transactional Sort SHOULD leave records already at their exact target row/column/orientation in cargo and stage only records that actually need movement. Preflight and rollback must treat those stationary records as immutable occupancy and fail closed if unexpected cargo remains.

Sort is not a replacement for Transfer and does not move nested contents independently of their direct container item.

### Stack

Stack is a container-local merge operation.

- Consider only direct cargo children of the selected container.
- Use DayZ's own `CanBeCombined`/`CombineItems` semantics; TransferZ must not invent compatibility rules.
- Do not merge across different containers or through nested cargo.
- Do not split stacks to make a merge possible.
- Do not recreate items or manually copy quantity/state.
- If DayZ reports two items as non-combinable, leave them alone.

### Modifier item drags

Normal unmodified left-button item drag remains vanilla behavior.

TransferZ owns two left-button modifier drags:

- `Shift + Left Drag`: move the source zone as a Transfer batch.
- `Alt + Left Drag`: move the exact-class batch selected by the dragged representative item.

For a direct cargo child, the source zone is its immediate cargo owner. Shift selects all direct cargo children of that source. Alt selects only direct cargo children whose exact `GetType()` matches the dragged item.

For an item shown in `VICINITY`, Shift selects the currently shown eligible loose vicinity items. Alt selects only shown eligible loose items whose exact `GetType()` matches the dragged item. Cargo-bearing vicinity containers are excluded from the exact-class loose-item batch.

Both modifier drags may target another visible cargo container. A cargo-source Shift or Alt drag may also target `VICINITY`; a vicinity-source Shift or Alt drag to vicinity is a no-op because those loose items are already there.

While a Shift/Alt drag is active, TransferZ owns final release and destination resolution. Native DayZ drop callbacks from the representative dragged icon must be consumed and must never execute a second predictive item move. On actual LMB release, cancel the native widget drag before committing the TransferZ batch, and suppress only the immediately trailing native drop event window needed to discard already-queued callbacks.

After scroll/capture churn, `GetWidgetUnderCursor()` is not sufficient proof of the destination. A hovered TransferZ drop overlay MUST also contain the current mouse point inside its owning container's live clipped drop-host rectangle; otherwise treat it as stale capture and continue with live geometry resolution.

Every modifier-item completion path (global mouse-up, cargo registered drop, vicinity registered drop, and legacy widget completion helper) MUST delegate to `CompleteModifierDragAtMousePosition()`. No modifier path may commit directly from the callback receiver or cached entity.

Critical modifier-drag event handling MUST live in the primary TransferZ implementation files. Do not split mouse-up, native-drop suppression, latching, scroll clipping, or destination resolution across filename-ordered `Z`/`ZZ`/`ZZZ` patch layers; Enforce Script modded-class ordering is not a valid correctness dependency.

Do not broaden exact-class matching into category matching, inheritance matching, ammo-family matching, or fuzzy similarity without an explicit new specification.

Do not assign `Ctrl + Drag` to TransferZ. Stock DayZ owns Ctrl-related inventory interactions and TransferZ must not compete with or suppress them.

### Right-click and native stack splitting

TransferZ does not assign any routing operation to right click, right drag, or double-right-click. Those gestures remain vanilla DayZ behavior.

In particular, normal right-click stack splitting must remain available without TransferZ gesture detection competing for the same button.

TransferZ may influence only the destination chosen for the native split in these established cases, while DayZ still owns the split quantity/state and item-manipulation protocol:

- If a resolved preferred destination (`P*`) exists and its exact cargo can accept the new split entity, `P*` overrides normal placement.
- If the stack is in cargo at or below a container currently held in hands and `P*` is not usable, keep the result in the exact immediate source cargo when space is available.
- Otherwise leave normal DayZ fallback behavior in control.

This destination selection is not a new RMB gesture and must not change whether or how DayZ decides that an item can be split.

### Modifier item clicks

A modifier click is a single-item route, distinct from the source-zone batch behavior of the same modifier followed by an actual left drag.

- `Shift + Click`: move the clicked cargo/vicinity item to the active destination.
- `Alt + Click`: move the clicked cargo/vicinity item to the resolved preferred destination.

The active destination may be a cargo container or `VICINITY`. If the requested destination is missing, invalid, already owns the item in the requested location, or cannot accept it, the item stays where it is. `Alt + Click` does nothing when no valid preferred target resolves.

`Ctrl + Click` remains vanilla DayZ behavior and must not be intercepted by TransferZ.

### Vicinity batch actions

The `VICINITY` header exposes Destination, Transfer, and Unpack controls.

- Vicinity Destination selects vicinity/ground as the active destination.
- Vicinity Transfer moves currently shown loose, takeable vicinity items except cargo-bearing containers into a selected container destination.
- Vicinity Unpack unpacks currently shown vicinity cargo-bearing containers into the destination while leaving the containers themselves in place.
- If a selected container destination is itself in vicinity, skip it as a source and allow it to receive the other items.
- With vicinity itself selected, vicinity Transfer is a no-op because those loose items are already there; vicinity Unpack unpacks shown containers onto the ground.
- Vicinity Transfer and Unpack may also be dragged onto a visible cargo container field for a one-off direct batch action.
- `Shift + Left Drag` from a vicinity item selects the shown eligible loose items for transfer.
- `Alt + Left Drag` from a vicinity item selects shown eligible loose items of that exact class.

### Links and double-click routing

Links are temporary client-session state. Pairing A and B gives deterministic A-to-B and B-to-A double-left-click routing for cargo icons. TransferZ keeps one active pair at a time.

Clicking Link on either participant removes the pair. Clicking Link on a different container while a pair exists removes the old pair and makes the clicked container the new pending anchor. A pending anchor is cancelled by clicking Link on it again.

A pending anchor or linked participant must remain open or in the player's hands. Clear a pending anchor or active pair when a participant is closed, stowed from the hands, no longer exists, no longer has cargo, moves into another player's inventory, or is no longer within normal inventory-manipulation reach.

Double-left-click routing precedence for a cargo item is:

1. If the immediate source container has an active link, route exactly to the linked container.
2. Otherwise, if the item is in an external container or a container in hands, route it to the resolved preferred destination when available.
3. Otherwise, if the item is in the player's worn/attached inventory, leave vanilla DayZ double-click behavior in control so the item is taken/swapped to hands.
4. If no TransferZ route applies, preserve vanilla behavior.

A source at or below the entity currently held in hands counts as an in-hand container for rule 2, even though its hierarchy root is the player.

For vicinity, left double-click routes the selected item to the preferred destination. Right double-click is not owned by TransferZ.

### Preferred personal destination

Store the preferred personal target as the ordered attachment-slot path from the player to the cargo-bearing destination, never by item classname.

Direct worn containers are one-hop paths such as `Back`. Nested attachment containers are multi-hop paths such as `Belt > DumpPouch` or `Vest > Pouch`. Replacing gear should preserve the preference when the currently equipped attachment hierarchy exposes the same slot path and the final entity has cargo.

Selecting another valid Preferred control replaces the stored preferred path. If any path element cannot be resolved, or the final entity no longer has cargo, do not guess a replacement destination; leave vanilla routing in control.

Continue reading the legacy single `preferred_slot` preference as a compatibility fallback. Cargo-nested containers are not persistent preferred personal destinations in 0.1.

## RPC architecture

Community Framework (CF) is a mandatory TransferZ dependency.

- Register TransferZ RPC ownership through one CF module.
- Keep TransferZ's RPC identifiers in one contiguous range owned by that module.
- Do not add new direct `PlayerBase.OnRPC` layers for TransferZ operations.
- CF dispatch is transport/routing only; operation handlers must still validate sender identity and all current server-side inventory state.
- Client-side request debouncing is advisory only and never a security boundary.

## Move validation

Move requests are executed on the server. Before moving an item, resolve current entities and locations again, verify that the sending player can reach the entities involved, respect cargo release/receive conditions, require free space in the exact selected cargo owner, and use DayZ inventory-location validation before the synchronized move.

Vicinity/ground destinations use DayZ's standard inventory drop path. Do not invent arbitrary world placement when a native inventory drop operation exists.

Do not delete and recreate items to simulate transfer, sorting, or stacking.

## UI contract

Cargo headers use compact controls. The transfer/navigation group remains on the left side of the header, after DayZ's native left-side preview/move controls. The maintenance group remains on the right side, before DayZ's native right-side controls.

Left group:

- `D`: select/toggle this container as the active Destination.
- `T`: Transfer direct cargo to the active destination; also draggable.
- `U`: Unpack leaf cargo from nested child containers while leaving direct loose cargo and nested containers in place; also draggable.
- `L`: start, complete, replace, or remove the single temporary Link pair.
- `P`: store/toggle the attached cargo container's attachment-slot path as the Preferred personal target.

Right group:

- Sort: three descending-width horizontal bars, with no arrow; compact/reorder this container's direct cargo in place.
- Stack: three equal horizontal dashes; merge compatible partial stacks in this container using DayZ's native combination rules.

`VICINITY` exposes only `D`, `T`, and `U` on the left because it has no single cargo grid to sort or stack.

TransferZ must preserve DayZ's native title geometry. Do not move or shrink the native header title to make room for TransferZ controls. Overlay TransferZ controls in available left/right header space while respecting DayZ's native preview, move, collapse, and other header controls.

DayZ may finish sizing header previews and cargo widgets after TransferZ's first setup call. Initial placement therefore uses bounded deferred GUI-layout correction after the immediate pass. Keep this initialization-only; do not introduce permanent per-frame layout polling.

Transfer/Unpack drag targets cover the visible destination container field rather than only the header. The same destination overlays are reused by `Shift + Left Drag` and `Alt + Left Drag` item batches. The temporary drag overlay must forward mouse-wheel scrolling to the appropriate native inventory scroller.

Hover tooltips must stay close to the hovered control, use a dark mostly-opaque background, and wrap onto additional lines instead of clipping longer messages. If state changes while a control remains hovered, rebuild both tooltip text and calculated geometry immediately rather than requiring mouse-out/mouse-in. Do not use the word `recursive` in player-facing tooltip text.

The control blocks have a subdued common background with individually visible button cells and brighter hover highlights. Active Destination, Link, and Preferred state is shown visually without changing the letter labels. While Transfer or Unpack is hovered, preview the immediate expected result with a subdued translucent background:

- green: expected normal/full execution;
- yellow: likely partial execution, such as insufficient destination capacity or only some currently eligible items;
- red: currently impossible.

The preview is advisory only. It must never replace authoritative server-side move validation.

TransferZ extends the vanilla inventory rather than replacing it. Native header dragging/reordering must remain available outside the TransferZ control rectangles; clicking a TransferZ control must not accidentally initiate native header dragging.

## Source layout and extension staging

Use descriptive filenames for source files and keep one clear owner for each TransferZ subsystem.

The earlier implementation accumulated several `Z`-prefixed mission-UI files solely to force Enforce lexical composition order. That staging was fragile: changing filenames changed compile/runtime behavior. The current architecture deliberately consolidates those layers into the owning files instead.

Therefore:

- do not reintroduce `Z`/`ZZ`/numeric filename staging to control modded-class resolution;
- when several TransferZ behaviors touch the same vanilla class, consolidate them into one owning TransferZ source file where practical;
- avoid direct helper calls across separate `modded class` layers when ordinary override flow or consolidation can perform the work;
- if a true ordering dependency cannot be avoided, document it and validate it with a real DayZ compile/runtime test.

## Scope boundaries for 0.1

- No arbitrary item-category filtering; exact-class matching is available through `Alt + Left Drag`.
- No persistent world-container links.
- No TransferZ-owned stack splitting; DayZ owns split quantity/state and TransferZ only applies the documented preferred/source destination selection around native splits.
- No automatic relocation of empty nested containers after Unpack.
- No persistent preferred personal targets for containers nested in cargo rather than attached through slots.
- No class allowlists for container support.
- No custom replacement inventory screen.

- Modifier item drags own their native drag teardown: before TransferZ commits the batch, the native `Icon` / `SlotsIcon` visual drag state MUST be explicitly reset, then widget dragging may be cancelled. `CancelWidgetDragging()` alone is not sufficient because it does not run the registered native drop cleanup and can leave colored cursor borders behind.
- VICINITY modifier-drop hit testing uses the visible vicinity slots root directly. Do not clip that root against `VicinityContainer`'s cargo scroller; current DayZ reparents vicinity slots into a separate LeftArea slots area.

- VICINITY has no implicit ownership boundary. Shift-dragging a cargo-bearing ground container moves only that dragged container. Shift-dragging loose ground loot batches eligible loose items but excludes cargo-bearing ground containers. Alt-drag remains the explicit homogeneous batch operation and includes same-class cargo-bearing siblings. Container contents remain inside moved containers; unpack is a separate operation.

- Sort orientation preference: detachable magazines (`MagazineStorage`) prefer vertical placement (height >= width). The planner first attempts a complete layout with that preference enforced for every magazine, then falls back to unrestricted rotation only when a complete preferred layout is impossible. Ammunition piles are not treated as magazines for this rule.

- Dedicated-server inventory batches MUST NOT execute sequential server-authored cargo/ground moves through `player.GetInventory()` / `DayZPlayerInventory`. Its `InventoryMode.SERVER` path queues remote-player sync junctures and leaves authoritative locations stale until later simulation processing. TransferZ batch/transaction code uses the moved item's `GameInventory` with `InventoryMode.SERVER`, which commits the location immediately and emits the server move to clients. This is required for multi-item Transfer/Class Transfer and transactional Sort correctness on dedicated servers.
- CF module RPC ranges use an exclusive upper bound. `GetRPCMax()` must therefore return one past the highest TransferZ RPC ID that the module handles.

- Vehicle cargo reachability MUST NOT be decided solely by `CheckManipulatedObjectsDistances(vehicle, player, c_MaxItemDistanceRadius)`. Large vehicles can expose valid cargo access points farther from the model origin. Client/UI gating may accept displayable `Transport` cargo, while every server mutation MUST still use DayZ's exact native inventory request validation (`CheckMoveToDstRequest`, `CheckDropRequest`, or `CheckRequestSrc` as appropriate).
