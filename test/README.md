# TransferZ self-testing

TransferZ uses two complementary test layers.

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
