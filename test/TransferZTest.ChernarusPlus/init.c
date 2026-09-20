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
    EntityAI nestedBandage = TZTest_CreateItem(nested, "BandageDressing");
    EntityAI nestedBattery = TZTest_CreateItem(nested, "Battery9V");

    int moved = TransferZServerService.Unpack(player, source, destination);
    TZTest_Check(moved == 2, "unpack moved nested leaves only");
    TZTest_Check(TZTest_IsDirectCargoChild(source, directApple), "unpack kept direct loose cargo");
    TZTest_Check(TZTest_IsDirectCargoChild(source, nested), "unpack kept nested container");
    TZTest_Check(TZTest_IsDirectCargoChild(destination, nestedBandage), "unpack moved nested bandage");
    TZTest_Check(TZTest_IsDirectCargoChild(destination, nestedBattery), "unpack moved nested battery");

    TZTest_DeleteFixture(source);
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

void TZTest_RunRecoveryBufferSelfTest(PlayerBase player)
{
    vector basePos = player.GetPosition();
    TransferZ_SortBuffer buffer = TransferZ_SortBuffer.Cast(GetGame().CreateObjectEx("TransferZ_SortBuffer", basePos + "2.2 0 -1.2", ECE_SETUP | ECE_KEEPHEIGHT | ECE_NOLIFETIME));
    TZTest_Check(buffer != null, "recovery buffer fixture created");
    if (!buffer)
        return;

    EntityAI stranded = TZTest_CreateItem(buffer, "Apple");
    TZTest_Check(stranded != null, "recovery buffer accepts staging cargo before recovery");
    TZTest_Check(!buffer.IsRecoveryMode(), "recovery buffer starts in staging mode");
    TZTest_Check(!buffer.CanDisplayCargo(), "staging buffer cargo is hidden");
    TZTest_Check(!buffer.IsInventoryVisible(), "staging buffer inventory is hidden");

    buffer.EnableRecoveryMode();

    TZTest_Check(buffer.IsRecoveryMode(), "recovery buffer switches to recovery mode");
    TZTest_Check(buffer.CanDisplayCargo(), "recovery buffer cargo becomes visible");
    TZTest_Check(buffer.IsInventoryVisible(), "recovery buffer inventory becomes visible");
    TZTest_Check(!buffer.CanReceiveItemIntoCargo(stranded), "recovery buffer rejects new cargo");
    TZTest_Check(!buffer.IsTakeable(), "recovery buffer remains non-takeable");
    TZTest_Check(TZTest_IsDirectCargoChild(buffer, stranded), "recovery buffer preserves stranded item identity");

    TZTest_DeleteFixture(buffer);
}


void TZTest_RunSelfTests(PlayerBase player)
{
    g_TZTestFailures = 0;
    Print("[TransferZTest] SUITE START");

    TZTest_RunTransferSelfTest(player);
    TZTest_RunUnpackSelfTest(player);
    TZTest_RunClassTransferSelfTest(player);
    TZTest_RunStackSelfTest(player);
    TZTest_RunSortSelfTest(player);
    TZTest_RunRecoveryBufferSelfTest(player);

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
