class TransferZMaintenanceResultState
{
    protected static int s_Sequence;
    protected static int s_Operation;
    protected static int s_SourceLow;
    protected static int s_SourceHigh;
    protected static bool s_Success;

    static void Publish(int operation, int sourceLow, int sourceHigh, bool success)
    {
        s_Operation = operation;
        s_SourceLow = sourceLow;
        s_SourceHigh = sourceHigh;
        s_Success = success;
        s_Sequence++;
    }

    static void PublishLocal(int operation, EntityAI source, bool success)
    {
        int sourceLow;
        int sourceHigh;
        TransferZNet.GetEntityNetworkId(source, sourceLow, sourceHigh);
        Publish(operation, sourceLow, sourceHigh, success);
    }

    static void HandleRPC(ParamsReadContext ctx)
    {
        int operation;
        int sourceLow;
        int sourceHigh;
        bool success;
        if (!ctx.Read(operation) || !ctx.Read(sourceLow) || !ctx.Read(sourceHigh) || !ctx.Read(success))
            return;

        Publish(operation, sourceLow, sourceHigh, success);
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

    static bool MatchesSource(EntityAI source)
    {
        if (!source)
            return false;

        int sourceLow;
        int sourceHigh;
        TransferZNet.GetEntityNetworkId(source, sourceLow, sourceHigh);
        return sourceLow == s_SourceLow && sourceHigh == s_SourceHigh;
    }
}
