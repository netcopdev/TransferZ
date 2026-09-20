class TransferZRequestGuard
{
    protected static const int STANDARD_REQUEST_THROTTLE_MS = 100;
    protected static const int MAINTENANCE_REQUEST_THROTTLE_MS = 250;
    protected static const int REQUEST_ENTRY_TTL_MS = 300000;
    protected static const int REQUEST_CLEANUP_INTERVAL_MS = 60000;

    protected static ref map<string, int> s_LastStandardRequestTime = new map<string, int>();
    protected static ref map<string, int> s_LastMaintenanceRequestTime = new map<string, int>();
    protected static int s_LastCleanupTime;

    protected static void CleanupRequestMap(map<string, int> requestTimes, int now)
    {
        if (!requestTimes)
            return;

        for (int index = requestTimes.Count() - 1; index >= 0; index--)
        {
            int last = requestTimes.GetElement(index);
            int age = now - last;
            if (age < 0 || age > REQUEST_ENTRY_TTL_MS)
            {
                string key = requestTimes.GetKey(index);
                requestTimes.Remove(key);
            }
        }
    }

    protected static void CleanupExpiredEntries(int now)
    {
        int elapsed = now - s_LastCleanupTime;
        if (elapsed >= 0 && elapsed < REQUEST_CLEANUP_INTERVAL_MS)
            return;

        CleanupRequestMap(s_LastStandardRequestTime, now);
        CleanupRequestMap(s_LastMaintenanceRequestTime, now);
        s_LastCleanupTime = now;
    }

    protected static bool Accept(PlayerBase player, map<string, int> requestTimes, int throttleMs)
    {
        if (!GetGame().IsMultiplayer())
            return true;
        if (!player || !requestTimes)
            return false;

        PlayerIdentity identity = player.GetIdentity();
        if (!identity)
            return false;

        string playerId = identity.GetId();
        if (playerId == "")
            return false;

        int now = GetGame().GetTime();
        CleanupExpiredEntries(now);

        if (requestTimes.Contains(playerId))
        {
            int last = requestTimes.Get(playerId);
            int elapsed = now - last;
            if (elapsed >= 0 && elapsed < throttleMs)
                return false;
        }

        requestTimes.Set(playerId, now);
        return true;
    }

    static bool AcceptStandard(PlayerBase player)
    {
        return Accept(player, s_LastStandardRequestTime, STANDARD_REQUEST_THROTTLE_MS);
    }

    static bool AcceptMaintenance(PlayerBase player)
    {
        return Accept(player, s_LastMaintenanceRequestTime, MAINTENANCE_REQUEST_THROTTLE_MS);
    }
}
