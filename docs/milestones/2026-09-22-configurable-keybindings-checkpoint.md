# Configurable key bindings checkpoint — 2026-09-22

## State

- Repository: `netcopdev/TransferZ`
- Branch: `feature/configurable-keybindings`
- Base: `main` at `3637f6b2171f850ec0e5786a747ab840a32c7a09`
- First implementation commit: `d5805b0d6e34e6ecb5e50d4bee3586e759b5c797`
- Final implementation/refinement commit: `e6b19226aa510f4f8b6f6252e730d93f076294eb`
- GitHub Self-test for first implementation: run `35686899114` — passed.
- GitHub Self-test for final implementation/refinement: run `35687069921` — passed.
- Merge status: **not merged**. Do not merge until the feature is confirmed in game.
- Real DayZ compile/in-game validation: **started**. The first launch confirmed that `inputs.xml` and all bindings load, but DayZ displayed raw `STR_TRANSFERZ_*` localization keys. The stringtable was then changed from the minimal 3-column CSV to the conservative full DayZ language schema with trailing commas. This localization fix still needs an in-game retest.

## Implemented behavior

TransferZ now registers a stock DayZ **TransferZ** controls section through `inputs.xml`.

Default gesture actions:

- Destination / Transfer modifier: Left Shift or Right Shift.
- Preferred / Exact-class modifier: Left Alt or Right Alt.
- Unpack container drag modifier: U.

The physical keys are no longer hardcoded by the TransferZ modifier click/drag code. More than one TransferZ modifier held at the same time fails safe and does not claim the gesture.

The Unload modifier uses the dragged cargo-bearing container itself as a direct Transfer source. The container does not move; its direct cargo children move to the release target. Dropping onto VICINITY transfers those direct children to the ground. Holding the Unload modifier on a non-container does not claim the drag, and an Unload-modifier click without an actual drag has no TransferZ click action. This is intentionally different from the existing recursive/nested Unpack command.

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

## First in-game finding

The initial Controls screen showed the TransferZ group and bindings, proving that `config.cpp -> inputs.xml` registration and default key bindings were working. All labels appeared as raw `STR_TRANSFERZ_*` keys, which isolated the failure to stringtable parsing/loading. The original table used a minimal `Language,original,english` schema. It has been replaced with DayZ's full standard language header and a trailing comma on every row, matching conservative production-mod stringtable practice.

Rebuild the PBO before continuing the matrix; the next check is simply that the same Controls screen now renders human-readable labels.

## Second in-game finding

The container modifier initially did not work for worn/attached cargo-bearing containers. Root cause: DayZ represents those items with `SlotsIcon`, the same class used for VICINITY, but TransferZ's new `SlotsIcon.OnIconDrag()` required a `VicinitySlotsContainer` before it considered any modifier. That made U-drag unreachable for backpacks, vests, pouches, and other cargo-bearing attachment-slot items.

The fix evaluates container-Unpack first and requires only that the dragged `m_Obj` has cargo. Transfer/Exact-class batching still requires an actual VICINITY parent.

## Final Unpack semantic clarification

There is one Unpack operation and no separate Unload variant.

For any selected/dragged source container:

- the source/root container stays where it is;
- direct loose cargo moves to the target;
- nested containers are recursively emptied;
- nested contents move to the target;
- each now-empty nested container is then moved to the target;
- successful completion leaves the source cargo empty and no cargo recursively nested under it.

The header `U`, configurable Unpack command, and `U + drag container` all invoke this same server-side Unpack operation.

## Required in-game validation

Do not merge before these checks are completed:

1. Open **Settings > Controls** and confirm a **TransferZ** section exists with all ten actions and readable labels.
2. Confirm default Shift click and Shift drag behavior still matches the previous release.
3. Rebind Destination / Transfer modifier away from Shift. Confirm old Shift stops invoking TransferZ and the new binding works for both single-item click and source-zone drag.
4. Confirm default Alt click and exact-class drag behavior still matches the previous release.
5. Rebind Preferred / Exact-class modifier away from Alt. Confirm old Alt stops invoking TransferZ and the new binding works for both click and exact-class drag.
6. Put two soda cans and a First Aid Kit containing a bandage inside a Protective Case in a backpack. U + drag the Protective Case onto an external container. Confirm the Protective Case stays in the backpack; the soda cans, bandage, and now-empty First Aid Kit all become direct cargo of the external container. Repeat from worn/attached and VICINITY source representations.
7. U + drag a nested cargo-bearing container onto VICINITY. Confirm the source/root container stays in place, all cargo levels are emptied to the ground, and emptied nested containers also move to the ground.
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

The final implementation/refinement Self-test is green. Build and deploy the feature branch for the in-game matrix above; do not merge until those checks are confirmed.
