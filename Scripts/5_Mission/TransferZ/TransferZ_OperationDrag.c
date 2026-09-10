class TransferZOperationDrag
{
    protected static int s_Operation = 0;
    protected static EntityAI s_Source;
    protected static bool s_FromVicinity = false;
    protected static ref array<EntityAI> s_VicinityItems;

    static void BeginContainer(int operation, EntityAI source)
    {
        if (!source)
            return;

        s_Operation = operation;
        s_Source = source;
        s_FromVicinity = false;

        if (s_VicinityItems)
            s_VicinityItems.Clear();
    }

    static void BeginVicinity(int operation, notnull array<EntityAI> items)
    {
        s_Operation = operation;
        s_Source = null;
        s_FromVicinity = true;

        if (!s_VicinityItems)
            s_VicinityItems = new array<EntityAI>();
        else
            s_VicinityItems.Clear();

        foreach (EntityAI item : items)
        {
            if (item)
                s_VicinityItems.Insert(item);
        }
    }

    static bool IsActive()
    {
        return s_Operation == TransferZOperation.TRANSFER || s_Operation == TransferZOperation.UNPACK;
    }

    static EntityAI GetSource()
    {
        return s_Source;
    }

    static bool IsFromVicinity()
    {
        return s_FromVicinity;
    }

    static int GetOperation()
    {
        return s_Operation;
    }

    static bool Complete(EntityAI destination)
    {
        if (!IsActive() || !destination)
            return false;

        bool handled = false;
        TransferZClientState state = TransferZClientState.Get();

        if (s_FromVicinity)
        {
            if (!s_VicinityItems)
                return false;

            if (s_Operation == TransferZOperation.TRANSFER)
                handled = state.RequestVicinityTransferTo(s_VicinityItems, destination);
            else if (s_Operation == TransferZOperation.UNPACK)
                handled = state.RequestVicinityUnpackTo(s_VicinityItems, destination);
        }
        else
        {
            if (!s_Source || s_Source == destination)
                return false;

            if (s_Operation == TransferZOperation.TRANSFER)
                handled = state.RequestTransferTo(s_Source, destination);
            else if (s_Operation == TransferZOperation.UNPACK)
                handled = state.RequestUnpackTo(s_Source, destination);
        }

        Clear();
        return handled;
    }

    static void Clear()
    {
        s_Operation = 0;
        s_Source = null;
        s_FromVicinity = false;
        if (s_VicinityItems)
            s_VicinityItems.Clear();
    }
}
