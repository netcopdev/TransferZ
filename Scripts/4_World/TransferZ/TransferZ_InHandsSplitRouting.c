class TransferZSplitPreferences
{
    string preferred_slot = "";
    ref array<string> preferred_path;
    int preferred_cargo_index = 0;

    void TransferZSplitPreferences()
    {
        preferred_path = new array<string>();
    }
}

class TransferZSplitPreferenceResolver
{
    static const string PREFERENCES_PATH = "$profile:TransferZ/preferences.json";

    static EntityAI Resolve(PlayerBase player, out bool configured, out int cargoIndex)
    {
        configured = false;
        cargoIndex = 0;
        if (!player || !FileExist(PREFERENCES_PATH))
            return null;

        string errorMessage;
        ref TransferZSplitPreferences preferences = new TransferZSplitPreferences();
        if (!JsonFileLoader<TransferZSplitPreferences>.LoadFile(PREFERENCES_PATH, preferences, errorMessage) || !preferences)
            return null;

        cargoIndex = preferences.preferred_cargo_index;
        if (cargoIndex < 0)
            return null;

        if (preferences.preferred_path && preferences.preferred_path.Count() > 0)
        {
            configured = true;
            EntityAI current = player;
            for (int i = 0; i < preferences.preferred_path.Count(); i++)
            {
                string slotName = preferences.preferred_path.Get(i);
                if (slotName == "")
                    return null;

                current = current.FindAttachmentBySlotName(slotName);
                if (!current)
                    return null;
            }

            if (TransferZCargo.Exists(current, cargoIndex))
                return current;
            return null;
        }

        if (preferences.preferred_slot == "")
            return null;

        configured = true;
        EntityAI legacyDestination = player.FindAttachmentBySlotName(preferences.preferred_slot);
        if (!legacyDestination || !TransferZCargo.Exists(legacyDestination, cargoIndex))
            return null;
        return legacyDestination;
    }
}

class TransferZSplitDestinationBridge
{
    protected static EntityAI s_Destination;
    protected static int s_DestinationCargoIndex = 0;
    protected static bool s_DestinationVicinity;

    static void Clear()
    {
        s_Destination = null;
        s_DestinationCargoIndex = 0;
        s_DestinationVicinity = false;
    }

    static void SetCargo(EntityAI destination, int cargoIndex = 0)
    {
        s_Destination = destination;
        s_DestinationCargoIndex = cargoIndex;
        s_DestinationVicinity = false;
    }

    static void SetVicinity()
    {
        s_Destination = null;
        s_DestinationCargoIndex = 0;
        s_DestinationVicinity = true;
    }

    static bool HasDestination()
    {
        return s_DestinationVicinity || s_Destination != null;
    }

    static bool IsVicinity()
    {
        return s_DestinationVicinity;
    }

    static int GetCargoIndex()
    {
        if (s_DestinationVicinity || !s_Destination)
            return 0;
        return s_DestinationCargoIndex;
    }

    static EntityAI GetCargo()
    {
        if (s_DestinationVicinity)
            return null;
        return s_Destination;
    }
}

modded class ItemBase
{
    protected bool TransferZResolveCargoSource(out EntityAI source, out int cargoIndex)
    {
        source = null;
        cargoIndex = 0;

        InventoryLocation currentLocation = new InventoryLocation();
        if (!GetInventory().GetCurrentInventoryLocation(currentLocation))
            return false;
        if (currentLocation.GetType() != InventoryLocationType.CARGO)
            return false;

        source = currentLocation.GetParent();
        cargoIndex = currentLocation.GetIdx();
        if (!source || !TransferZCargo.Exists(source, cargoIndex))
        {
            source = null;
            cargoIndex = 0;
            return false;
        }

        return true;
    }

    protected bool TransferZFindSplitLocation(EntityAI destinationEntity, int destinationCargoIndex, bool verifyReceive, out InventoryLocation destination)
    {
        destination = new InventoryLocation();
        if (!destinationEntity || !TransferZCargo.Exists(destinationEntity, destinationCargoIndex))
            return false;
        if (verifyReceive && !destinationEntity.CanReceiveItemIntoCargo(this))
            return false;

        if (!TransferZCargo.FindFreeLocation(destinationEntity, destinationCargoIndex, this, destination))
            return false;
        return TransferZCargo.LocationMatches(destination, destinationEntity, destinationCargoIndex);
    }

    protected bool TransferZExecuteSplitAtLocation(notnull InventoryLocation destination)
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || !destination.IsValid())
            return false;

        if (g_Game.IsClient())
        {
            if (!ScriptInputUserData.CanStoreInputUserData())
                return false;
            if (player.GetInventory().HasInventoryReservation(this, destination))
                return false;

            player.GetInventory().AddInventoryReservationEx(null, destination, GameInventory.c_InventoryReservationTimeoutShortMS);

            ScriptInputUserData ctx = new ScriptInputUserData();
            ctx.Write(INPUT_UDT_ITEM_MANIPULATION);
            ctx.Write(4);
            ItemBase splitSource = this;
            ctx.Write(splitSource);
            destination.WriteToContext(ctx);
            ctx.Write(true);
            ctx.Send();
            return true;
        }

        if (!g_Game.IsMultiplayer())
        {
            SplitItemToInventoryLocation(destination);
            return true;
        }

        return false;
    }

    protected bool TransferZExecuteSplitTo(EntityAI destinationEntity, int destinationCargoIndex, bool verifyReceive)
    {
        InventoryLocation destination;
        if (!TransferZFindSplitLocation(destinationEntity, destinationCargoIndex, verifyReceive, destination))
            return false;
        return TransferZExecuteSplitAtLocation(destination);
    }

    protected bool TransferZExecuteSplitToVicinity()
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
            return false;

        InventoryLocation destination = new InventoryLocation();
        if (!GameInventory.SetGroundPosByOwner(player, this, destination))
            return false;
        if (!destination.IsValid() || destination.GetType() != InventoryLocationType.GROUND)
            return false;

        return TransferZExecuteSplitAtLocation(destination);
    }

    protected bool TransferZRouteNativeSplit()
    {
        if (!CanBeSplit() || GetDayZGame().IsLeftCtrlDown())
            return false;

        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || player.GetInventory().HasInventoryReservation(this, null))
            return false;

        if (TransferZSplitDestinationBridge.HasDestination())
        {
            if (TransferZSplitDestinationBridge.IsVicinity())
            {
                if (TransferZExecuteSplitToVicinity())
                    return true;
            }
            else
            {
                EntityAI destination = TransferZSplitDestinationBridge.GetCargo();
                int destinationCargoIndex = TransferZSplitDestinationBridge.GetCargoIndex();
                if (destination && TransferZExecuteSplitTo(destination, destinationCargoIndex, true))
                    return true;
            }
        }

        EntityAI source;
        int sourceCargoIndex;
        if (TransferZResolveCargoSource(source, sourceCargoIndex) && TransferZExecuteSplitTo(source, sourceCargoIndex, false))
            return true;

        bool preferredConfigured;
        int preferredCargoIndex;
        EntityAI preferred = TransferZSplitPreferenceResolver.Resolve(player, preferredConfigured, preferredCargoIndex);
        if (preferredConfigured)
        {
            if (preferred && TransferZExecuteSplitTo(preferred, preferredCargoIndex, true))
                return true;
            return false;
        }

        return false;
    }

    override void OnRightClick()
    {
        if (TransferZRouteNativeSplit())
            return;

        super.OnRightClick();
    }
}
