class TransferZOperationDrag
{
    protected static int s_Operation = 0;
    protected static EntityAI s_Source;
    protected static EntityAI s_RepresentativeItem;
    protected static bool s_ClassTransfer = false;
    protected static bool s_FromVicinity = false;
    protected static ref array<EntityAI> s_VicinityItems;

    static void BeginContainer(int operation, EntityAI source)
    {
        if (!source)
            return;

        s_Operation = operation;
        s_Source = source;
        s_RepresentativeItem = null;
        s_ClassTransfer = false;
        s_FromVicinity = false;

        if (s_VicinityItems)
            s_VicinityItems.Clear();
    }

    static void BeginClassTransfer(EntityAI source, EntityAI representative)
    {
        if (!source || !representative)
            return;

        s_Operation = TransferZOperation.TRANSFER_CLASS;
        s_Source = source;
        s_RepresentativeItem = representative;
        s_ClassTransfer = true;
        s_FromVicinity = false;

        if (s_VicinityItems)
            s_VicinityItems.Clear();
    }

    static void BeginVicinity(int operation, notnull array<EntityAI> items)
    {
        s_Operation = operation;
        s_Source = null;
        s_RepresentativeItem = null;
        s_ClassTransfer = false;
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
        return s_Operation == TransferZOperation.TRANSFER || s_Operation == TransferZOperation.UNPACK || s_Operation == TransferZOperation.TRANSFER_CLASS;
    }

    static bool IsClassTransfer()
    {
        return s_ClassTransfer && s_Operation == TransferZOperation.TRANSFER_CLASS;
    }

    static EntityAI GetSource()
    {
        return s_Source;
    }

    static EntityAI GetRepresentativeItem()
    {
        return s_RepresentativeItem;
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

        if (s_ClassTransfer)
        {
            if (s_Source && s_RepresentativeItem && s_Source != destination)
                handled = state.RequestClassTransferTo(s_Source, destination, s_RepresentativeItem);
        }
        else if (s_FromVicinity)
        {
            if (s_VicinityItems)
            {
                if (s_Operation == TransferZOperation.TRANSFER)
                    handled = state.RequestVicinityTransferTo(s_VicinityItems, destination);
                else if (s_Operation == TransferZOperation.UNPACK)
                    handled = state.RequestVicinityUnpackTo(s_VicinityItems, destination);
            }
        }
        else if (s_Source)
        {
            if (s_Operation == TransferZOperation.TRANSFER)
            {
                if (s_Source != destination)
                    handled = state.RequestTransferTo(s_Source, destination);
            }
            else if (s_Operation == TransferZOperation.UNPACK)
            {
                // U may be dropped back onto its own source to flatten nested
                // cargo into that source container.
                handled = state.RequestUnpackTo(s_Source, destination);
            }
        }

        Clear();
        return handled;
    }

    static bool CompleteToVicinity()
    {
        if (!IsActive())
            return false;

        bool handled = false;
        TransferZClientState state = TransferZClientState.Get();

        if (s_ClassTransfer)
        {
            if (s_Source && s_RepresentativeItem)
                handled = state.RequestClassTransferToVicinity(s_Source, s_RepresentativeItem);
        }
        else if (s_FromVicinity)
        {
            if (s_VicinityItems && s_Operation == TransferZOperation.UNPACK)
                handled = state.RequestVicinityUnpackToVicinity(s_VicinityItems);
            // Vicinity T -> Vicinity is deliberately a no-op: those loose
            // items are already in the requested destination.
        }
        else if (s_Source)
        {
            if (s_Operation == TransferZOperation.TRANSFER)
                handled = state.RequestTransferToVicinity(s_Source);
            else if (s_Operation == TransferZOperation.UNPACK)
                handled = state.RequestUnpackToVicinity(s_Source);
        }

        Clear();
        return handled;
    }

    static void Clear()
    {
        s_Operation = 0;
        s_Source = null;
        s_RepresentativeItem = null;
        s_ClassTransfer = false;
        s_FromVicinity = false;
        if (s_VicinityItems)
            s_VicinityItems.Clear();
    }
}
