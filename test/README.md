# TransferZ self-testing

TransferZ uses two complementary test layers.

## One-command local validation

The preferred local command builds and tests an isolated candidate directly from the checkout:

```powershell
.\tools\run-transferz-self-test.ps1 `
    -DayZDiagPath 'E:\SteamLibrary\steamapps\common\DayZ\DayZDiag_x64.exe' `
    -DependencyMods 'E:\SteamLibrary\steamapps\common\DayZ\!Workshop\@CF' `
    -KnowledgePackPath 'C:\src\DayZ-Modding-Knowledge-Pack'
```

This performs, in order:

1. fast TransferZ repository contracts;
2. the DayZ Modding Knowledge Pack script validator and UI reconciler when `-KnowledgePackPath` is supplied;
3. a fresh unsigned local `TransferZ.pbo` build into `dist\self-test\@TransferZ\addons`;
4. the automated DayZDiag smoke mission against CF plus that freshly built candidate.

`-AddonBuilderPath` can be supplied when DayZ Tools is not in one of the standard paths already handled by `build-pbo.ps1`. The candidate is local-only and is not copied into a production server or client mod directory.

The low-level DayZDiag launcher uses a temporary `.bat` plus `cmd.exe` rather than passing DayZ arguments through Windows PowerShell 5.1 `Start-Process -ArgumentList`; this avoids argument-quoting failures on paths containing spaces. It also fails early when the Steam client session is not active.

GitHub Actions runs the fast contracts plus the Knowledge Pack validator/reconciler on every push and pull request. The external validator is pinned to commit `9727ae83e65ac26a1cd386c64821b00159f8f933` so CI does not silently change when that repository changes.

## 1. Fast repository contracts

Run this after every code change:

```powershell
python tools/self_test.py
```

These tests use only the Python standard library. They validate repository and architecture invariants that can be checked without DayZ, including version synchronization, packaging boundaries, CF ownership, native inventory validation on core move paths, transactional Sort verification/rollback hooks, and the DayZDiag suite marker.

The same command runs automatically in GitHub Actions on every push and pull request.

These are structural tests, not a substitute for an actual DayZ script compile.

## 2. Automated DayZDiag smoke suite

The tracked mission in `test/TransferZTest.ChernarusPlus` now executes an assertion-based smoke suite before creating the normal interactive fixture. It directly exercises the real TransferZ runtime services for:

- direct cargo Transfer;
- nested Unpack semantics;
- exact-class Transfer;
- Stack quantity preservation;
- transactional Sort identity preservation.

Each assertion prints a machine-readable `[TransferZTest] PASS ...` or `[TransferZTest] FAIL ...` line. The mission finishes the automated portion with exactly one of:

- `[TransferZTest] SUITE PASS`
- `[TransferZTest] SUITE FAIL count=N`

After the automated suite, the existing interactive inventory/world fixture is still created for manual UI testing.

### One-command launcher

On a Windows machine with DayZ Tools/DayZDiag installed:

```powershell
.\tools\run-dayzdiag-self-tests.ps1 `
    -DayZDiagPath 'E:\SteamLibrary\steamapps\common\DayZ\DayZDiag_x64.exe' `
    -ModList 'E:\DayZDev\@CF;E:\DayZDev\@TransferZ'
```

Use this lower-level launcher only when the PBO/mod folders are already prepared. For normal development, `run-transferz-self-test.ps1` above is the intended entry point.

The launcher starts the tracked mission, watches the profile logs for the suite marker, terminates DayZDiag after the marker appears, and returns:

- exit code `0` on suite pass;
- exit code `1` on suite failure;
- exit code `2` when no suite result appears before the timeout.

Use `-ProfilesPath` and `-TimeoutSeconds` when the defaults do not fit the local setup.

## Interactive fixture

The mission still creates the deterministic inventory used during TransferZ development:

- player: `SurvivorM_Mirek`
- position: `2200 10 2200`
- clothing, Plate Carrier + pouches, and a Mountain Backpack;
- M4A1 and AK-74 test weapons;
- magazines, boxed ammunition, and deliberately partial 5.56/5.45 loose-ammo stacks;
- nested portable containers (`SmallProtectorCase`, `FirstAidKit`, `AmmoBox`);
- apples, bandages, batteries, and other small transfer/sort items;
- two Wooden Crates and a green Barrel nearby;
- loose vicinity items for routing tests.

The fixture stays outside normal TransferZ runtime code and is not packed into the release PBO.
