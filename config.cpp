class CfgPatches
{
    class TransferZ_Core
    {
        units[] = {"TransferZ_SortBuffer"};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] =
        {
            "DZ_Data",
            "DZ_Scripts",
            "DZ_Gear_Containers",
            "DZ_Gear_Camping",
            "JM_CF_Scripts"
        };
    };
};

class CfgMods
{
    class TransferZ
    {
        dir = "TransferZ";
        name = "TransferZ";
        author = "TransferZ contributors";
        version = "0.1.0";
        type = "mod";
        dependencies[] =
        {
            "Game",
            "World",
            "Mission"
        };

        class defs
        {
            class gameScriptModule
            {
                value = "";
                files[] =
                {
                    "TransferZ/Scripts/3_Game"
                };
            };

            class worldScriptModule
            {
                value = "";
                files[] =
                {
                    "TransferZ/Scripts/4_World"
                };
            };

            class missionScriptModule
            {
                value = "";
                files[] =
                {
                    "TransferZ/Scripts/5_Mission"
                };
            };
        };
    };
};


class CfgVehicles
{
    class Container_Base;

    // Internal, model-less transaction workspace. TransferZ creates this only
    // while a server-authoritative Sort fallback is active.
    class TransferZ_SortBuffer : Container_Base
    {
        scope = 1;
        displayName = "TransferZ Recovery Crate";
        descriptionShort = "Emergency recovery container created only when a TransferZ Sort rollback cannot return every item to its original cargo layout.";
        model = "\\dz\\gear\\camping\\wooden_case.p3d";
        weight = 0;
        itemSize[] = {1,1};
        itemsCargoSize[] = {20,500};
        rotationFlags = 17;
        canBeDigged = 0;
    };
};
