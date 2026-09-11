enum TransferZOperation
{
    TRANSFER = 1,
    UNPACK = 2,
    MOVE_ITEM = 3,
    TRANSFER_CLASS = 4
}

class TransferZRPC
{
    static const int REQUEST = 84627101;
}

class TransferZNet
{
    static void GetEntityNetworkId(EntityAI entity, out int low, out int high)
    {
        low = 0;
        high = 0;
        if (entity)
            entity.GetNetworkID(low, high);
    }
}
