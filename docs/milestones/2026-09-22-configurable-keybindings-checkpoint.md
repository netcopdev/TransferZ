# Configurable key bindings checkpoint — 2026-09-22

## State

- Repository: `netcopdev/TransferZ`
- Branch: `feature/configurable-keybindings`
- Base: `main` at `3637f6b2171f850ec0e5786a747ab840a32c7a09`
- First implementation commit: `d5805b0d6e34e6ecb5e50d4bee3586e759b5c797`
- GitHub Self-test for first implementation: run `35686899114` — passed.
- Merge status: **not merged**. Do not merge until the feature is confirmed in game.
- Real DayZ compile/in-game validation: **not yet performed**.

## Implemented behavior

TransferZ now registers a stock DayZ **TransferZ** controls section through `inputs.xml`.

Default gesture actions:

- Destination / Transfer modifier: Left Shift or Right Shift.
- Preferred / Exact-class modifier: Left Alt or Right Alt.
- Unpack container drag modifier: U.

The physical keys are no longer hardcoded by the TransferZ modifier click/drag code. More than one TransferZ modifier held at the same time fails safe and does not claim the gesture.

The Unpack modifier uses the dragged cargo-bearing container itself as the Unpack source. The container does not move; the release target selects the one-off Unpack destination. Dropping onto VICINITY uses the existing ground unpack path. Holding the Unpack modifier on a non-container does not claim the drag, and an Unpack-modifier click without an actual drag has no TransferZ click action.

The following discrete stock DayZ user actions are exposed but deliberately have no default key:

- Destination
- Transfer
- Unpack
- Link
- Preferred
- Sort
- Stack

Discrete commands are evaluated only while the inventory menu is open. Cargo commands target the live open cargo grid under the mouse. Destination, Transfer, and Unpack also operate on the visible VICINITY field.

## Packaging

`config.cpp` registers `TransferZ/inputs.xml`.

`tools/build-pbo.ps1` stages `inputs.xml` and `stringtable.csv` as runtime assets. The release PBO audit requires both assets to be present.

## Required in-game validation

Do not merge before these checks are completed:

1. Open **Settings > Controls** and confirm a **TransferZ** section exists with all ten actions and readable labels.
2. Confirm default Shift click and Shift drag behavior still matches the previous release.
3. Rebind Destination / Transfer modifier away from Shift. Confirm old Shift stops invoking TransferZ and the new binding works for both single-item click and source-zone drag.
4. Confirm default Alt click and exact-class drag behavior still matches the previous release.
5. Rebind Preferred / Exact-class modifier away from Alt. Confirm old Alt stops invoking TransferZ and the new binding works for both click and exact-class drag.
6. U + drag a cargo-bearing container onto another open cargo grid. Confirm the source container stays in place and only eligible nested leaf cargo is unpacked to the target.
7. U + drag a cargo-bearing container onto VICINITY. Confirm the source container stays in place and eligible nested leaf cargo goes to the ground.
8. U + click without dragging a container. Confirm no TransferZ Unpack operation is triggered.
9. U + drag a non-container item. Confirm TransferZ does not claim the drag and normal DayZ behavior remains available.
10. Bind and exercise Destination, Transfer, Unpack, Link, Preferred, Sort, and Stack. Confirm each acts on the intended open cargo grid under the mouse.
11. With the mouse over VICINITY, confirm bound Destination, Transfer, and Unpack commands operate on VICINITY; Link/Preferred/Sort/Stack should not fabricate a vicinity equivalent.
12. Bind two TransferZ modifier actions to conflicting keys/combinations and confirm ambiguous simultaneous modifiers fail safe instead of selecting an arbitrary TransferZ operation.
13. Verify normal unmodified left drag, right click, native stack splitting, double-click routing, and inventory scrolling during modifier drags still behave correctly.
14. Close inventory and press bound discrete TransferZ command keys. Confirm TransferZ performs no inventory operation.

## Build/test commands

Repository/static gate:

```powershell
py -3.14 tools/self_test.py
```

Build a signed test release using the normal local build configuration:

```powershell
.\tools\build.ps1
```

If the build configuration is not in the default `%LOCALAPPDATA%\TransferZ\build.psd1`:

```powershell
.\tools\build.ps1 -BuildConfig 'C:\path\to\build.psd1'
```

After the final refinement commit, verify its GitHub Self-test is green before deploying the branch build for the in-game matrix above.
