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
- `Shift + Click` routes one cargo/vicinity item to the active destination; `Alt + Click` routes one cargo/vicinity item to the resolved preferred destination.
- `Shift + Left Drag` is a source-zone batch Transfer: from cargo it moves all direct cargo children; from vicinity it moves the shown eligible loose items.
- `Alt + Left Drag` is an exact-class batch move using the dragged item's exact `GetType()`: from cargo it selects matching direct cargo children; from vicinity it selects matching shown eligible loose items.
- Right-click interactions belong to vanilla DayZ. TransferZ must not assign RMB click, RMB drag, or RMB double-click gestures to routing operations.
- Native stack splitting remains DayZ-owned. When `P*` is configured and can accept the new split entity, it overrides normal placement. Otherwise a stack inside cargo below the held container stays beside the original stack when exact source cargo has room, and all remaining cases fall back to vanilla DayZ behavior.
- Container links are session-local unless a future specification explicitly makes them persistent.
- Preferred personal destinations are stored by the ordered attachment-slot path from the player to the cargo-bearing target, not by item classname.
- Nested attachment destinations such as `Belt > DumpPouch` are supported; direct worn containers are the one-hop form of the same model.
- Keep compatibility with the legacy single `preferred_slot` preference unless a deliberate migration removes it.
- Do not reinterpret cargo-nested containers as persistent preferred personal destinations unless that behavior is explicitly designed later.
- Sort and Stack are server-authoritative container-maintenance operations. Sort may reposition and rotate only direct cargo children through validated native inventory moves. Sort snapshots every original row/column/orientation, computes a complete bounded in-cargo plan before execution, may use native atomic swaps for compatible cargo items when that avoids intermediary storage, and must synchronously reverse every executed move/swap on failure so the exact original snapshot is restored. Sort must never use vicinity/ground or arbitrary player cargo as implicit staging. If no bounded in-cargo plan exists, fail without moving anything. Stack may merge only items DayZ itself reports as combinable.
- If a requested move is invalid, inaccessible, no longer current, or does not fit, fail safely. For transactional Sort this specifically means rollback and verification of the original snapshot before returning failure; a rollback invariant violation is critical and must be logged explicitly.

## CF dependency and RPC ownership

- Community Framework (CF) is a required dependency of TransferZ.
- Register TransferZ RPC handling through a CF module rather than inventing additional direct RPC dispatch hooks.
- Keep the TransferZ RPC range contiguous and owned by the registered CF module.
- RPC handlers must still validate sender identity, target player, reachability, current inventory location, and operation-specific constraints. CF routing does not replace server-side validation.

## Compatibility and performance

- Prefer small `modded` hooks over replacement inventory UI classes.
- If TransferZ does not own a click/double-click route for an item, fall back to vanilla behavior.
- Preserve vanilla unmodified left drag, right-click, and Ctrl interactions.
- Avoid per-frame inventory scans and unnecessary RPC traffic.
- Use dynamic inventory/cargo capability checks rather than allowlists of container classnames.
- Do not introduce any additional mandatory third-party dependency without explicit approval.
- Never auto-spawn diagnostic/test items from normal runtime code, including under `DIAG_DEVELOPER`. Test fixtures must be explicit, opt-in tooling invoked deliberately by the developer.
- Header placement must preserve DayZ's native title geometry and native left/right controls. TransferZ overlays its transfer controls in the available left area and its Sort/Stack controls in the available right area without redefining the title layout.
- DayZ can finalize header geometry after TransferZ's first setup call. Use bounded deferred GUI-layout correction when needed; never solve this with a permanent polling loop.

## Source organization and load order

- Use descriptive filenames for source files.
- Keep behavior for the same vanilla/modded class consolidated in one owning TransferZ source file when practical. The earlier chain of `Z`-prefixed late-extension files existed only to force lexical composition order and has been removed after consolidation.
- Do not reintroduce filename-prefix staging (`Z`, `ZZ`, numeric stage hacks, etc.) as a substitute for coherent ownership.
- Avoid direct helper calls across separate `modded class` layers when ordinary virtual/override flow or one consolidated owner can do the job.
- If a real ordering dependency is unavoidable, document it explicitly and validate it with a DayZ compile/runtime test rather than relying on filename aesthetics or assumptions.

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
5. Do not merge into `main` without explicit user approval.
