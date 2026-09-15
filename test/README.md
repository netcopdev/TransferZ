# TransferZ DayZDiag test mission

This is the explicit opt-in simulation fixture for TransferZ development. It is intentionally kept outside normal TransferZ runtime code and is not packed into the release PBO.

The mission uses the same dedicated Chernarus test spawn used during TransferZ development:

- player: `SurvivorM_Mirek`
- position: `2200 10 2200`

On startup it creates a deterministic test inventory and nearby world fixtures covering:

- clothing, Plate Carrier + pouches, and a Mountain Backpack;
- M4A1 and AK-74 test weapons;
- magazines, boxed ammunition, and deliberately partial 5.56/5.45 loose-ammo stacks;
- nested portable containers (`SmallProtectorCase`, `FirstAidKit`, `AmmoBox`);
- apples, bandages, batteries, and other small transfer/sort items;
- two Wooden Crates and a green Barrel nearby;
- loose vicinity items for vicinity routing tests.

## Launch

Point DayZDiag directly at this mission folder. For the established local development layout the command is equivalent to:

```powershell
& 'E:\SteamLibrary\steamapps\common\DayZ\DayZDiag_x64.exe' -mod=$mod -mission='E:\DayZDev\TransferZ\test\TransferZTest.ChernarusPlus' -profiles='E:\DayZDev\TransferZ\diag-profile' -window -nopause -filePatching
```

Set `$mod` to the normal Diag mod list with CF loaded before TransferZ.

The previous external `E:\DayZDev\TransferZTest.ChernarusPlus` mission can be retired or updated from this tracked fixture. Keeping the mission inside the repository means a normal pull updates the test loadout together with the code being tested.
