class TransferZSplitPreferences
{
    string preferred_slot = "";
    ref array<string> preferred_path;

    void TransferZSplitPreferences()
    {
        preferred_path = new array<string>();
    }
}

class TransferZSplitPreferenceResolver
{
    static const string PREFERENCES_PATH = "$profile:TransferZ/preferences.json";

    static EntityAI Resolve(PlayerBase player, out bool configured)
    {
        configured = false;
        if (!player || !FileExist(PREFERENCES_PATH))
            return null;

        string errorMessage;
        ref TransferZSplitPreferences preferences = new TransferZSplitPreferences();
        if (!JsonFileLoader<TransferZSplitPreferences>.LoadFile(PREFERENCES_PATH, preferences, errorMessage) || !preferences)
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

            if (current.GetInventory().GetCargo())
                return current;
            return null;
        }

        if (preferences.preferred_slot == "")
            return null;

        configured = true;
        EntityAI legacyDestination = player.FindAttachmentBySlotName(preferences.preferred_slot);
        if (!legacyDestination || !legacyDestination.GetInventory().GetCargo())
            return null;
        return legacyDestination;
    }
}

class TransferZSplitDestinationBridge
{
    protected static EntityAI s_Destination;
    protected static bool s_DestinationVicinity;

    static void Clear()
    {
        s_Destination = null;
        s_DestinationVicinity = false;
    }

    static void SetCargo(EntityAI destination)
    {
        s_Destination = destination;
        s_DestinationVicinity = false;
    }

    static void SetVicinity()
    {
        s_Destination = null;
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

    static EntityAI GetCargo()
    {
        if (s_DestinationVicinity)
            return null;
        return s_Destination;
    }
}

modded class ItemBase
{
    protected bool TransferZResolveCargoSource(out EntityAI source)
    {
        source = null;

        InventoryLocation currentLocation = new InventoryLocation();
        if (!GetInventory().GetCurrentInventoryLocation(currentLocation))
            return false;
        if (currentLocation.GetType() != InventoryLocationType.CARGO)
            return false;

        source = currentLocation.GetParent();
        if (!source || !source.GetInventory().GetCargo())
        {
            source = null;
            return false;
        }

        return true;
    }

    protected bool TransferZFindSplitLocation(EntityAI destinationEntity, bool verifyReceive, out InventoryLocation destination)
    {
        destination = new InventoryLocation();
        if (!destinationEntity || !destinationEntity.GetInventory().GetCargo())
            return false;
        if (verifyReceive && !destinationEntity.CanReceiveItemIntoCargo(this))
            return false;

        // Match vanilla DayZ's native right-click split placement search.
        // FindFreeLocationFor(this, ...) evaluates the actual item footprint and
        // may return a rotated cargo location via destination.GetFlip().
        if (!destinationEntity.GetInventory().FindFreeLocationFor(this, FindInventoryLocationType.CARGO, destination))
            return false;
        if (!destination.IsValid() || destination.GetType() != InventoryLocationType.CARGO || destination.GetParent() != destinationEntity)
            return false;

        destination.SetCargo(destinationEntity, this, destination.GetIdx(), destination.GetRow(), destination.GetCol(), destination.GetFlip());
        return true;
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

    protected bool TransferZExecuteSplitTo(EntityAI destinationEntity, bool verifyReceive)
    {
        InventoryLocation destination;
        if (!TransferZFindSplitLocation(destinationEntity, verifyReceive, destination))
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

        // The transient active D is the first routing preference. If it cannot
        // accept the split, continue through source cargo and P before giving
        // control back to vanilla DayZ.
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
                if (destination && TransferZExecuteSplitTo(destination, true))
                    return true;
            }
        }

        // If D is absent or unusable, first keep a cargo stack in its immediate source
        // container whenever there is room for the newly created entity. This is
        // the least surprising result for an ordinary right-click split: the new
        // stack stays beside the original instead of being routed elsewhere.
        EntityAI source;
        if (TransferZResolveCargoSource(source) && TransferZExecuteSplitTo(source, false))
            return true;

        // P is the fallback only when there is no usable immediate source cargo:
        // for example the stack is in hands/attachments/on the ground, or its
        // source cargo has no room for the newly created split entity.
        bool preferredConfigured;
        EntityAI preferred = TransferZSplitPreferenceResolver.Resolve(player, preferredConfigured);
        if (preferredConfigured)
        {
            if (preferred && TransferZExecuteSplitTo(preferred, true))
                return true;
            return false;
        }

        // With no D, no usable source cargo and no P, leave the operation entirely
        // to vanilla DayZ.
        return false;
    }

    override void OnRightClick()
    {
        if (TransferZRouteNativeSplit())
            return;

        super.OnRightClick();
    }
}
