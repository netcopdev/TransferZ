// Explicit TransferZ DayZDiag test fixture.
// This mission is deliberately outside the mod runtime/PBO. Launch it only when testing.

ItemBase TZTest_CreateStack(EntityAI owner, string typeName, int quantity)
{
    if (!owner)
        return NULL;

    ItemBase item = ItemBase.Cast(owner.GetInventory().CreateInInventory(typeName));

    // Ammunition piles keep their count in the magazine ammo state, not in
    // the ItemBase quantity; SetQuantity does not change the round count.
    Magazine pile = Magazine.Cast(item);
    if (pile)
        pile.ServerSetAmmoCount(quantity);
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

void main()
{
    vector spawnPos = "2200 10 2200";
    PlayerBase player = PlayerBase.Cast(GetGame().CreatePlayer(NULL, "SurvivorM_Mirek", spawnPos, 0, "NONE"));
    if (!player)
        return;

    TZTest_FillPlayer(player);
    TZTest_SpawnWorldFixtures(player);
    GetGame().SelectPlayer(NULL, player);
}

Mission CreateCustomMission(string path)
{
    return new MissionGameplay();
}
