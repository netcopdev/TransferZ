class CfgPatches
{
    class TransferZ_Core
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] =
        {
            "DZ_Data"
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
