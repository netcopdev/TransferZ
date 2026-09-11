# AGENTS.md — TransferZ

## Read first

Before modifying this repository, read this file and `docs/CODEX_PROJECT_RULES.md`.

## Git workflow

- Start feature, fix, refactor, tooling, or documentation work on a dedicated branch by default.
- `main` is the stable integration branch. Do not merge into `main` merely because implementation is complete.
- Merging/integration requires explicit user approval.
- Keep commits and changes focused and reviewable. Avoid unrelated cleanup.
- Documentation must evolve with implementation when behavior, controls, build steps, or architecture change.

## TransferZ architecture

- TransferZ is an inventory-routing layer. Do not delete/recreate items to simulate moves.
- The client may request an operation; the server owns validation and execution.
- Never trust client-supplied entity identifiers without resolving and revalidating them on the server.
- Exact-destination operations must target the selected container's own cargo. Do not silently fall back to arbitrary player inventory or nested destination containers.
- `Transfer` moves direct cargo children and preserves nested container structure.
- Container `Unpack` moves non-container leaf items found inside cargo-bearing child containers while leaving the source's direct loose cargo and the nested containers themselves in place.
- Vicinity `Unpack` operates on the shown cargo-bearing vicinity containers and ignores loose vicinity items.
- `Shift + Click` routes one cargo/vicinity item to the active `D*`; `Alt + Click` routes one cargo/vicinity item to the resolved `P*`.
- `Shift + Left Drag` and `Alt + Left Drag` remain source-zone batch operations for `T` and `U` respectively.
- `Right Drag` is an exact-class batch move. From cargo, use the immediate cargo owner as the source and exact `GetType()` matches among its direct cargo children. From vicinity, use the currently shown loose vicinity items and filter by exact `GetType()`.
- A cargo-source right drag may target another container or `VICINITY`. A vicinity-source right drag targets a container; vicinity-to-vicinity is a no-op.
- Container links are session-local unless a future specification explicitly makes them persistent.
- Preferred personal destinations are stored by the ordered attachment-slot path from the player to the cargo-bearing target, not by item classname.
- Nested attachment destinations such as `Belt > DumpPouch` are supported; direct worn containers are the one-hop form of the same model.
- Keep compatibility with the legacy single `preferred_slot` preference unless a deliberate migration removes it.
- Do not reinterpret cargo-nested containers as persistent preferred personal destinations unless that behavior is explicitly designed later.
- If a requested move is invalid, inaccessible, no longer current, or does not fit, fail safely and leave the item where it is.

## Compatibility and performance

- Prefer small `modded` hooks over replacement inventory UI classes.
- If TransferZ does not own a click/double-click route for an item, fall back to vanilla behavior.
- Preserve vanilla unmodified left drag and Ctrl interactions.
- RMB exact-class drag must begin only after real pointer movement so a normal RMB click remains available.
- Avoid per-frame inventory scans and unnecessary RPC traffic. Short-lived GUI polling used only while an RMB gesture is actively being resolved is acceptable; permanent inventory polling is not.
- Use dynamic inventory/cargo capability checks rather than allowlists of container classnames.
- Do not introduce mandatory third-party dependencies without explicit approval.
- Header placement must preserve DayZ's native title geometry and native left/right controls. TransferZ overlays its compact control block without redefining the title layout.
- DayZ can finalize header geometry after TransferZ's first setup call. Use bounded deferred GUI-layout correction when needed; never solve this with a permanent polling loop.

## Source organization and load order

- Use descriptive filenames for new ordinary source files.
- **Do not rename existing late UI extension files solely to make their names prettier.** The current `Z`-prefixed files encode a proven Enforce compilation/extension order and are part of the working implementation.
- A previous cleanup that replaced those names with numeric `TransferZ_Widget_<stage>_<purpose>.c` filenames changed behavior and produced compile/runtime regressions even though the source bodies were effectively unchanged.
- Any future staging cleanup must first prove equivalent Enforce visibility/override order with a real DayZ compile and runtime test. Treat filename order as behaviorally significant until that work is deliberately redesigned.
- Prefer consolidating behavior into the owning subsystem when practical instead of adding another late extension layer.
- Do not call helper methods across separate `modded class` layers when ordinary virtual/override flow can do the job; Enforce may not resolve those helpers as expected.

## Enforce Script safety

- For nontrivial DayZ API use, inspect the current `BohemiaInteractive/DayZ-Script-Diff` implementation and mirror proven syntax.
- Keep function/method invocations on one physical line, especially calls with four or more arguments or `out`/`inout` parameters.
- Compute long arguments into local variables instead of formatting calls across multiple lines.
- Do not place comments inside argument lists or chained calls.
- Prefer simple, explicit control flow over clever abstractions.
- Before considering script work complete, inspect `.c` files for calls broken immediately after `(` and rewrite them to parser-safe form.
- If a real DayZ compile has not been run, state that explicitly; generic syntax checks are not a substitute.

## Completion checks

Before handing work over:

1. Inspect the branch diff and repository status.
2. Verify the documented semantics still match the implementation.
3. Run available static/build checks.
4. If DayZ Tools or a DayZ server compile is unavailable, say so rather than claiming compile validation.
5. Do not perform cosmetic source-file renames or staging cleanup during integration unless that exact load-order change has been explicitly designed and runtime-tested.
