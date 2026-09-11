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
- `Shift + Click` routes one cargo/vicinity item to the active `D*`; `Alt + Click` routes one cargo/vicinity item to the resolved `P*`. Modifier drags remain source-zone batch operations.
- Container links are session-local unless a future specification explicitly makes them persistent.
- Preferred personal destinations are stored by the ordered attachment-slot path from the player to the cargo-bearing target, not by item classname.
- Nested attachment destinations such as `Belt > DumpPouch` are supported; direct worn containers are the one-hop form of the same model.
- Keep compatibility with the legacy single `preferred_slot` preference unless a deliberate migration removes it.
- Do not reinterpret cargo-nested containers as persistent preferred personal destinations unless that behavior is explicitly designed later.
- If a requested move is invalid, inaccessible, no longer current, or does not fit, fail safely and leave the item where it is.

## Compatibility and performance

- Prefer small `modded` hooks over replacement inventory UI classes.
- If TransferZ does not own a double-click route for an item, fall back to vanilla behavior.
- Avoid per-frame inventory scans and unnecessary RPC traffic.
- Use dynamic inventory/cargo capability checks rather than allowlists of container classnames.
- Do not introduce mandatory third-party dependencies without explicit approval.

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
