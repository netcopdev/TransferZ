class TransferZMaintenanceResultState
{
    protected static int s_Sequence;
    protected static int s_Operation;
    protected static int s_SourceLow;
    protected static int s_SourceHigh;
    protected static int s_SourceCargoIndex;
    protected static bool s_Success;

    static void Publish(int operation, int sourceLow, int sourceHigh, int sourceCargoIndex, bool success)
    {
        s_Operation = operation;
        s_SourceLow = sourceLow;
        s_SourceHigh = sourceHigh;
        s_SourceCargoIndex = sourceCargoIndex;
        s_Success = success;
        s_Sequence++;
    }

    static void PublishLocal(int operation, EntityAI source, int sourceCargoIndex, bool success)
    {
        int sourceLow;
        int sourceHigh;
        TransferZNet.GetEntityNetworkId(source, sourceLow, sourceHigh);
        Publish(operation, sourceLow, sourceHigh, sourceCargoIndex, success);
    }

    static void HandleRPC(ParamsReadContext ctx)
    {
        int operation;
        int sourceLow;
        int sourceHigh;
        int sourceCargoIndex;
        bool success;
        if (!ctx.Read(operation) || !ctx.Read(sourceLow) || !ctx.Read(sourceHigh) || !ctx.Read(sourceCargoIndex) || !ctx.Read(success))
            return;
        if (sourceCargoIndex < 0)
            return;

        Publish(operation, sourceLow, sourceHigh, sourceCargoIndex, success);
    }

    static int GetSequence()
    {
        return s_Sequence;
    }

    static int GetOperation()
    {
        return s_Operation;
    }

    static bool WasSuccessful()
    {
        return s_Success;
    }

    static bool MatchesSource(EntityAI source, int sourceCargoIndex = 0)
    {
        if (!source || sourceCargoIndex < 0)
            return false;

        int sourceLow;
        int sourceHigh;
        TransferZNet.GetEntityNetworkId(source, sourceLow, sourceHigh);
        return sourceLow == s_SourceLow && sourceHigh == s_SourceHigh && sourceCargoIndex == s_SourceCargoIndex;
    }
}
