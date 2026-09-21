// Explicit TransferZ DayZDiag test fixture.
// This mission is deliberately outside the mod runtime/PBO. Launch it only when testing.

ItemBase TZTest_CreateStack(EntityAI owner, string typeName, int quantity)
{
    if (!owner)
        return NULL;

    ItemBase item = ItemBase.Cast(owner.GetInventory().CreateInInventory(typeName));

    // Ammo piles store their quantity as magazine rounds rather than ItemBase
    // quantity. Keep the generic quantity path for ordinary stackable items.
    Magazine magazine = Magazine.Cast(item);
    if (magazine)
        magazine.ServerSetAmmoCount(quantity);
    else if (item)
        item.SetQuantity(quantity);

    return item;
}

EntityAI TZTest_CreateItem(EntityAI owner, string typeName)
{
    if (!owner)
        return NULL;

    return owner.GetInventory().CreateInInventory(typeName);
}

EntityAI TZTest_CreateCargoItem(EntityAI owner, string typeName)
{
    if (!owner)
        return NULL;

    return owner.GetInventory().CreateEntityInCargo(typeName);
}

EntityAI TZTest_CreateCargoItemAt(EntityAI owner, string typeName, int cargoIndex, int row, int col, bool flip)
{
    if (!owner)
        return NULL;

    return owner.GetInventory().CreateEntityInCargoEx(typeName, cargoIndex, row, col, flip);
}

EntityAI TZTest_CreateWorldItem(string typeName, vector position)
{
    return EntityAI.Cast(GetGame().CreateObjectEx(typeName, position, ECE_PLACE_ON_SURFACE));
}

void TZTest_FillPlayer(PlayerBase player)
{
    player.RemoveAllItems();

    player.GetInventory().CreateAttachment("HikingJacket_Black");
    player.GetInventory().CreateAttachment("CargoPants_Black");
    player.GetInventory().CreateAttachment("CombatBoots_Black");
    player.GetInventory().CreateAttachment("TacticalGloves_Black");

    EntityAI vest = player.GetInventory().CreateAttachment("PlateCarrierVest");
    EntityAI pouches;
    if (vest)
        pouches = vest.GetInventory().CreateAttachment("PlateCarrierPouches");

    EntityAI backpack = player.GetInventory().CreateAttachment("MountainBag_Blue");

    // Two long guns exercise large-item sorting and normal player attachment/cargo routing.
    TZTest_CreateItem(player, "M4A1");
    TZTest_CreateItem(player, "AK74");

    // Magazines and boxed ammo give varied item sizes/types for Sort/Transfer tests.
    TZTest_CreateItem(player, "Mag_STANAG_30Rnd");
    TZTest_CreateItem(player, "Mag_STANAG_30Rnd");
    TZTest_CreateItem(player, "Mag_AK74_30Rnd");
    TZTest_CreateItem(player, "Mag_AK74_30Rnd");
    TZTest_CreateItem(player, "AmmoBox_556x45_20Rnd");
    TZTest_CreateItem(player, "AmmoBox_545x39_20Rnd");

    // Keep several deliberately partial same-class stacks for Stack and native split testing.
    EntityAI stackOwner = pouches;
    if (!stackOwner)
        stackOwner = backpack;
    if (!stackOwner)
        stackOwner = player;

    TZTest_CreateStack(stackOwner, "Ammo_556x45", 3);
    TZTest_CreateStack(stackOwner, "Ammo_556x45", 5);
    TZTest_CreateStack(stackOwner, "Ammo_556x45", 7);
    TZTest_CreateStack(stackOwner, "Ammo_556x45", 9);
    TZTest_CreateStack(stackOwner, "Ammo_556x45", 11);
    TZTest_CreateStack(stackOwner, "Ammo_545x39", 4);
    TZTest_CreateStack(stackOwner, "Ammo_545x39", 8);
    TZTest_CreateStack(stackOwner, "Ammo_545x39", 12);

    if (backpack)
    {
        TZTest_CreateItem(backpack, "Apple");
        TZTest_CreateItem(backpack, "Apple");
        TZTest_CreateItem(backpack, "BandageDressing");

        EntityAI protector = TZTest_CreateItem(backpack, "SmallProtectorCase");
        if (protector)
        {
            TZTest_CreateItem(protector, "Battery9V");
            TZTest_CreateItem(protector, "BandageDressing");
            TZTest_CreateItem(protector, "Apple");
        }

        EntityAI firstAid = TZTest_CreateItem(backpack, "FirstAidKit");
        if (firstAid)
        {
            TZTest_CreateItem(firstAid, "BandageDressing");
            TZTest_CreateItem(firstAid, "DisinfectantAlcohol");
        }
    }

    if (pouches)
        TZTest_CreateItem(pouches, "Apple");
}

void TZTest_SpawnWorldFixtures(PlayerBase player)
{
    vector basePos = player.GetPosition();

    EntityAI crateA = TZTest_CreateWorldItem("WoodenCrate", basePos + "1.5 0 0");
    if (crateA)
    {
        TZTest_CreateItem(crateA, "Apple");
        TZTest_CreateItem(crateA, "Apple");
        TZTest_CreateItem(crateA, "Apple");
        TZTest_CreateStack(crateA, "Ammo_556x45", 6);
        TZTest_CreateStack(crateA, "Ammo_556x45", 10);
        TZTest_CreateStack(crateA, "Ammo_556x45", 14);
        TZTest_CreateItem(crateA, "SmallProtectorCase");
    }

    EntityAI crateB = TZTest_CreateWorldItem("WoodenCrate", basePos + "-1.5 0 0");
    if (crateB)
    {
        TZTest_CreateItem(crateB, "M4A1");
        TZTest_CreateItem(crateB, "AK74");
        TZTest_CreateItem(crateB, "Mag_STANAG_30Rnd");
        TZTest_CreateItem(crateB, "Mag_AK74_30Rnd");
        TZTest_CreateItem(crateB, "AmmoBox");
    }

    EntityAI barrel = TZTest_CreateWorldItem("Barrel_Green", basePos + "0 0 2.5");
    if (barrel)
    {
        TZTest_CreateItem(barrel, "Apple");
        TZTest_CreateItem(barrel, "BandageDressing");
        EntityAI firstAid = TZTest_CreateItem(barrel, "FirstAidKit");
        if (firstAid)
        {
            TZTest_CreateItem(firstAid, "BandageDressing");
            TZTest_CreateItem(firstAid, "Battery9V");
        }
    }

    // Loose vicinity items preserve the original quick routing fixtures.
    TZTest_CreateWorldItem("Apple", basePos + "0.8 0 1.2");
    TZTest_CreateWorldItem("Apple", basePos + "1.1 0 1.2");
    TZTest_CreateWorldItem("BandageDressing", basePos + "0.8 0 1.6");
    TZTest_CreateWorldItem("Battery9V", basePos + "1.1 0 1.6");
    TZTest_CreateStack(TZTest_CreateWorldItem("AmmoBox", basePos + "-0.8 0 1.4"), "Ammo_556x45", 13);
}


int g_TZTestFailures = 0;

void TZTest_Check(bool condition, string name)
{
    if (condition)
    {
        Print("[TransferZTest] PASS " + name);
        return;
    }

    g_TZTestFailures++;
    Print("[TransferZTest] FAIL " + name);
}

bool TZTest_IsDirectCargoChild(EntityAI owner, EntityAI item)
{
    if (!owner || !item)
        return false;

    InventoryLocation location = new InventoryLocation();
    if (!item.GetInventory().GetCurrentInventoryLocation(location))
        return false;

    return location.GetType() == InventoryLocationType.CARGO && location.GetParent() == owner;
}

int TZTest_SumAmmo(EntityAI owner, string typeName)
{
    if (!owner)
        return 0;

    CargoBase cargo = owner.GetInventory().GetCargo();
    if (!cargo)
        return 0;

    int total = 0;
    for (int i = 0; i < cargo.GetItemCount(); i++)
    {
        EntityAI entity = cargo.GetItem(i);
        if (!entity || entity.GetType() != typeName)
            continue;

        Magazine magazine = Magazine.Cast(entity);
        if (magazine && !magazine.IsSetForDeletion())
            total += magazine.GetAmmoCount();
    }
    return total;
}

void TZTest_DeleteFixture(EntityAI entity)
{
    if (entity)
        GetGame().ObjectDelete(entity);
}

void TZTest_RunTransferSelfTest(PlayerBase player)
{
    vector basePos = player.GetPosition();
    EntityAI source = TZTest_CreateWorldItem("WoodenCrate", basePos + "1.2 0 -0.8");
    EntityAI destination = TZTest_CreateWorldItem("WoodenCrate", basePos + "-1.2 0 -0.8");
    EntityAI apple = TZTest_CreateItem(source, "Apple");
    EntityAI bandage = TZTest_CreateItem(source, "BandageDressing");

    int moved = TransferZServerService.Transfer(player, source, destination);
    TZTest_Check(moved == 2, "transfer moved all direct cargo");
    TZTest_Check(TZTest_IsDirectCargoChild(destination, apple), "transfer preserved apple identity");
    TZTest_Check(TZTest_IsDirectCargoChild(destination, bandage), "transfer preserved bandage identity");

    TZTest_DeleteFixture(source);
    TZTest_DeleteFixture(destination);
}

void TZTest_RunUnpackSelfTest(PlayerBase player)
{
    vector basePos = player.GetPosition();
    EntityAI source = TZTest_CreateWorldItem("WoodenCrate", basePos + "1.2 0 -0.8");
    EntityAI destination = TZTest_CreateWorldItem("WoodenCrate", basePos + "-1.2 0 -0.8");
    EntityAI directApple = TZTest_CreateItem(source, "Apple");
    EntityAI nested = TZTest_CreateItem(source, "SmallProtectorCase");
    EntityAI nestedBandage = TZTest_CreateCargoItem(nested, "BandageDressing");
    EntityAI nestedBattery = TZTest_CreateCargoItem(nested, "Battery9V");

    TZTest_Check(directApple != null, "unpack fixture direct loose item created");
    TZTest_Check(nestedBandage != null, "unpack fixture nested bandage created");
    TZTest_Check(nestedBattery != null, "unpack fixture nested battery created");
    TZTest_Check(!TransferZCargo.Exists(directApple, 0), "unpack fixture loose item has no cargo grid");
    TZTest_Check(TransferZCargo.Exists(nested, 0), "unpack fixture nested container cargo resolves");
    TZTest_Check(!TransferZCargo.Exists(nested, 1), "unpack fixture invalid cargo grid is rejected");
    TZTest_Check(!TransferZCargo.Exists(nestedBandage, 0), "unpack fixture nested bandage has no cargo grid");
    TZTest_Check(!TransferZCargo.Exists(nestedBattery, 0), "unpack fixture nested battery has no cargo grid");

    int moved = TransferZNestedUnpackService.Unpack(player, source, destination);
    TZTest_Check(moved == 2, "unpack moved nested leaves only");
    TZTest_Check(TZTest_IsDirectCargoChild(source, directApple), "unpack kept direct loose cargo");
    TZTest_Check(TZTest_IsDirectCargoChild(source, nested), "unpack kept nested container");
    TZTest_Check(TZTest_IsDirectCargoChild(destination, nestedBandage), "unpack moved nested bandage");
    TZTest_Check(TZTest_IsDirectCargoChild(destination, nestedBattery), "unpack moved nested battery");

    TZTest_DeleteFixture(source);
    TZTest_DeleteFixture(destination);
}

void TZTest_RunVicinityBatchSelfTest(PlayerBase player)
{
    vector basePos = player.GetPosition();
    EntityAI destination = TZTest_CreateWorldItem("WoodenCrate", basePos + "-1.2 0 -0.8");
    EntityAI appleA = TZTest_CreateWorldItem("Apple", basePos + "0.4 0 -0.4");
    EntityAI appleB = TZTest_CreateWorldItem("Apple", basePos + "0.6 0 -0.4");
    EntityAI appleC = TZTest_CreateWorldItem("Apple", basePos + "0.8 0 -0.4");

    ref array<EntityAI> items = new array<EntityAI>();
    items.Insert(appleA);
    items.Insert(appleB);
    items.Insert(appleC);

    int moved = TransferZServerService.MoveItemsFromVicinity(player, items, destination);
    TZTest_Check(moved == 3, "vicinity batch moved all ground items in one operation");
    TZTest_Check(TZTest_IsDirectCargoChild(destination, appleA), "vicinity batch moved first item");
    TZTest_Check(TZTest_IsDirectCargoChild(destination, appleB), "vicinity batch moved second item");
    TZTest_Check(TZTest_IsDirectCargoChild(destination, appleC), "vicinity batch moved third item");

    TZTest_DeleteFixture(destination);
}

void TZTest_RunVicinityUnpackBatchSelfTest(PlayerBase player)
{
    vector basePos = player.GetPosition();
    EntityAI destination = TZTest_CreateWorldItem("WoodenCrate", basePos + "-1.2 0 -0.8");
    EntityAI sourceA = TZTest_CreateWorldItem("SmallProtectorCase", basePos + "0.4 0 -0.5");
    EntityAI sourceB = TZTest_CreateWorldItem("SmallProtectorCase", basePos + "0.8 0 -0.5");
    EntityAI nestedA = TZTest_CreateCargoItem(sourceA, "FirstAidKit");
    EntityAI nestedB = TZTest_CreateCargoItem(sourceB, "FirstAidKit");
    EntityAI bandageA = TZTest_CreateCargoItem(nestedA, "BandageDressing");
    EntityAI bandageB = TZTest_CreateCargoItem(nestedB, "BandageDressing");

    ref array<EntityAI> sources = new array<EntityAI>();
    sources.Insert(sourceA);
    sources.Insert(sourceB);

    int moved = TransferZNestedUnpackService.UnpackMany(player, sources, destination, 0, false);
    TZTest_Check(moved == 2, "vicinity unpack batch moved leaves from all containers");
    TZTest_Check(TZTest_IsDirectCargoChild(destination, bandageA), "vicinity unpack batch moved first nested leaf");
    TZTest_Check(TZTest_IsDirectCargoChild(destination, bandageB), "vicinity unpack batch moved second nested leaf");

    TZTest_DeleteFixture(sourceA);
    TZTest_DeleteFixture(sourceB);
    TZTest_DeleteFixture(destination);
}

void TZTest_RunClassTransferSelfTest(PlayerBase player)
{
    vector basePos = player.GetPosition();
    EntityAI source = TZTest_CreateWorldItem("WoodenCrate", basePos + "1.2 0 -0.8");
    EntityAI destination = TZTest_CreateWorldItem("WoodenCrate", basePos + "-1.2 0 -0.8");
    EntityAI appleA = TZTest_CreateItem(source, "Apple");
    EntityAI appleB = TZTest_CreateItem(source, "Apple");
    EntityAI bandage = TZTest_CreateItem(source, "BandageDressing");

    int moved = TransferZServerService.TransferClass(player, source, destination, appleA);
    TZTest_Check(moved == 2, "class transfer moved exact class batch");
    TZTest_Check(TZTest_IsDirectCargoChild(destination, appleA) && TZTest_IsDirectCargoChild(destination, appleB), "class transfer moved both exact-class items");
    TZTest_Check(TZTest_IsDirectCargoChild(source, bandage), "class transfer left other class in source");

    TZTest_DeleteFixture(source);
    TZTest_DeleteFixture(destination);
}

void TZTest_RunStackSelfTest(PlayerBase player)
{
    vector basePos = player.GetPosition();
    EntityAI source = TZTest_CreateWorldItem("WoodenCrate", basePos + "1.2 0 -0.8");
    TZTest_CreateStack(source, "Ammo_556x45", 3);
    TZTest_CreateStack(source, "Ammo_556x45", 5);

    int before = TZTest_SumAmmo(source, "Ammo_556x45");
    int combined = TransferZMaintenanceService.Stack(player, source);
    int after = TZTest_SumAmmo(source, "Ammo_556x45");

    TZTest_Check(before == 8, "stack fixture quantity initialized");
    TZTest_Check(combined > 0, "stack combined compatible partial stacks");
    TZTest_Check(after == before, "stack preserved total quantity");

    TZTest_DeleteFixture(source);
}

void TZTest_RunSingleItemSortSelfTest(PlayerBase player)
{
    vector basePos = player.GetPosition();
    EntityAI source = TZTest_CreateWorldItem("WoodenCrate", basePos + "1.2 0 -0.8");
    EntityAI apple = TZTest_CreateCargoItemAt(source, "Apple", 0, 2, 3, false);
    TZTest_Check(apple != null, "single-item sort fixture created away from origin");

    int result = TransferZTransactionalSortPlanner.Sort(player, source);
    InventoryLocation location = new InventoryLocation();
    bool located = apple && apple.GetInventory().GetCurrentInventoryLocation(location);
    TZTest_Check(result > 0, "single-item sort performs a move");
    TZTest_Check(located && location.GetType() == InventoryLocationType.CARGO && location.GetParent() == source && location.GetIdx() == 0 && location.GetRow() == 0 && location.GetCol() == 0, "single-item sort compacts to top-left");

    TZTest_DeleteFixture(source);
}

void TZTest_RunSortSelfTest(PlayerBase player)
{
    vector basePos = player.GetPosition();
    EntityAI source = TZTest_CreateWorldItem("WoodenCrate", basePos + "1.2 0 -0.8");
    EntityAI apple = TZTest_CreateItem(source, "Apple");
    EntityAI bandage = TZTest_CreateItem(source, "BandageDressing");
    EntityAI magazine = TZTest_CreateItem(source, "Mag_STANAG_30Rnd");
    EntityAI protector = TZTest_CreateItem(source, "SmallProtectorCase");

    int result = TransferZTransactionalSortPlanner.Sort(player, source);
    TZTest_Check(result >= 0, "sort completed transactionally");
    TZTest_Check(TZTest_IsDirectCargoChild(source, apple), "sort preserved apple identity");
    TZTest_Check(TZTest_IsDirectCargoChild(source, bandage), "sort preserved bandage identity");
    TZTest_Check(TZTest_IsDirectCargoChild(source, magazine), "sort preserved magazine identity");
    TZTest_Check(TZTest_IsDirectCargoChild(source, protector), "sort preserved container identity");

    TZTest_DeleteFixture(source);
}

void TZTest_RunSortEmergencyDropSelfTest(PlayerBase player)
{
    vector basePos = player.GetPosition();
    TransferZ_SortBuffer buffer = TransferZ_SortBuffer.Cast(GetGame().CreateObjectEx("TransferZ_SortBuffer", basePos + "2.2 0 -1.2", ECE_SETUP | ECE_KEEPHEIGHT | ECE_NOLIFETIME | ECE_NOPERSISTENCY_WORLD | ECE_NOPERSISTENCY_CHAR));
    TZTest_Check(buffer != null, "sort emergency-drop buffer fixture created");
    if (!buffer)
        return;
    EntityAI stranded = TZTest_CreateItem(buffer, "Apple");
    TZTest_Check(stranded != null, "sort emergency-drop fixture accepts staged item");
    InventoryMode moveMode = InventoryMode.SERVER;
    if (!GetGame().IsMultiplayer())
        moveMode = InventoryMode.LOCAL;
    bool dropped = false;
    if (stranded)
        dropped = stranded.GetInventory().DropEntity(moveMode, player, stranded);
    InventoryLocation location = new InventoryLocation();
    bool onGround = stranded && stranded.GetInventory().GetCurrentInventoryLocation(location) && location.GetType() == InventoryLocationType.GROUND;
    TZTest_Check(dropped && onGround, "sort emergency-drop uses native ground move");
    TZTest_DeleteFixture(buffer);
    TZTest_DeleteFixture(stranded);
}


void TZTest_RunSelfTests(PlayerBase player)
{
    g_TZTestFailures = 0;
    Print("[TransferZTest] SUITE START");

    Print("[TransferZTest] RUN transfer");
    TZTest_RunTransferSelfTest(player);
    Print("[TransferZTest] RUN unpack");
    TZTest_RunUnpackSelfTest(player);
    Print("[TransferZTest] RUN vicinity-batch");
    TZTest_RunVicinityBatchSelfTest(player);
    Print("[TransferZTest] RUN vicinity-unpack-batch");
    TZTest_RunVicinityUnpackBatchSelfTest(player);
    Print("[TransferZTest] RUN class-transfer");
    TZTest_RunClassTransferSelfTest(player);
    Print("[TransferZTest] RUN stack");
    TZTest_RunStackSelfTest(player);
    Print("[TransferZTest] RUN single-item-sort");
    TZTest_RunSingleItemSortSelfTest(player);
    Print("[TransferZTest] RUN sort");
    TZTest_RunSortSelfTest(player);
    Print("[TransferZTest] RUN sort-emergency-drop");
    TZTest_RunSortEmergencyDropSelfTest(player);

    if (g_TZTestFailures == 0)
        Print("[TransferZTest] SUITE PASS");
    else
        Print("[TransferZTest] SUITE FAIL count=" + g_TZTestFailures.ToString());
}

void main()
{
    vector spawnPos = "2200 10 2200";
    PlayerBase player = PlayerBase.Cast(GetGame().CreatePlayer(NULL, "SurvivorM_Mirek", spawnPos, 0, "NONE"));
    if (!player)
        return;

    TZTest_RunSelfTests(player);
    TZTest_FillPlayer(player);
    TZTest_SpawnWorldFixtures(player);
    GetGame().SelectPlayer(NULL, player);
}

Mission CreateCustomMission(string path)
{
    return new MissionGameplay();
}
