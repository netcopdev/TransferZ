enum TransferZInputModifier
{
    NONE = 0,
    TRANSFER = 1,
    EXACT_CLASS = 2,
    UNLOAD = 3
}

enum TransferZInputCommand
{
    NONE = 0,
    DESTINATION = 1,
    TRANSFER = 2,
    UNPACK = 3,
    LINK = 4,
    PREFERRED = 5,
    SORT = 6,
    STACK = 7
}

class TransferZInput
{
    static const string ACTION_TRANSFER_MODIFIER = "UATransferZTransferModifier";
    static const string ACTION_CLASS_MODIFIER = "UATransferZClassModifier";
    static const string ACTION_UNLOAD_MODIFIER = "UATransferZUnloadModifier";
    static const string ACTION_DESTINATION = "UATransferZDestination";
    static const string ACTION_TRANSFER = "UATransferZTransfer";
    static const string ACTION_UNPACK = "UATransferZUnpack";
    static const string ACTION_LINK = "UATransferZLink";
    static const string ACTION_PREFERRED = "UATransferZPreferred";
    static const string ACTION_SORT = "UATransferZSort";
    static const string ACTION_STACK = "UATransferZStack";

    protected static UAInput s_TransferModifier;
    protected static UAInput s_ClassModifier;
    protected static UAInput s_UnloadModifier;
    protected static UAInput s_Destination;
    protected static UAInput s_Transfer;
    protected static UAInput s_Unpack;
    protected static UAInput s_Link;
    protected static UAInput s_Preferred;
    protected static UAInput s_Sort;
    protected static UAInput s_Stack;

    protected static void Resolve()
    {
        UAInputAPI api = GetUApi();
        if (!api)
            return;

        if (!s_TransferModifier)
            s_TransferModifier = api.GetInputByName(ACTION_TRANSFER_MODIFIER);
        if (!s_ClassModifier)
            s_ClassModifier = api.GetInputByName(ACTION_CLASS_MODIFIER);
        if (!s_UnloadModifier)
            s_UnloadModifier = api.GetInputByName(ACTION_UNLOAD_MODIFIER);
        if (!s_Destination)
            s_Destination = api.GetInputByName(ACTION_DESTINATION);
        if (!s_Transfer)
            s_Transfer = api.GetInputByName(ACTION_TRANSFER);
        if (!s_Unpack)
            s_Unpack = api.GetInputByName(ACTION_UNPACK);
        if (!s_Link)
            s_Link = api.GetInputByName(ACTION_LINK);
        if (!s_Preferred)
            s_Preferred = api.GetInputByName(ACTION_PREFERRED);
        if (!s_Sort)
            s_Sort = api.GetInputByName(ACTION_SORT);
        if (!s_Stack)
            s_Stack = api.GetInputByName(ACTION_STACK);
    }

    protected static bool IsHeld(UAInput input)
    {
        return input && input.LocalValue() > 0.5;
    }

    protected static bool WasPressed(UAInput input)
    {
        return input && input.LocalPress();
    }

    static int ModifierMode()
    {
        Resolve();

        bool transfer = IsHeld(s_TransferModifier);
        bool exactClass = IsHeld(s_ClassModifier);
        bool unload = IsHeld(s_UnloadModifier);
        int active = 0;

        if (transfer)
            active++;
        if (exactClass)
            active++;
        if (unload)
            active++;

        if (active != 1)
            return TransferZInputModifier.NONE;
        if (transfer)
            return TransferZInputModifier.TRANSFER;
        if (exactClass)
            return TransferZInputModifier.EXACT_CLASS;
        return TransferZInputModifier.UNLOAD;
    }

    static int PressedCommand()
    {
        Resolve();

        int command = TransferZInputCommand.NONE;
        int pressed = 0;

        if (WasPressed(s_Destination))
        {
            command = TransferZInputCommand.DESTINATION;
            pressed++;
        }
        if (WasPressed(s_Transfer))
        {
            command = TransferZInputCommand.TRANSFER;
            pressed++;
        }
        if (WasPressed(s_Unpack))
        {
            command = TransferZInputCommand.UNPACK;
            pressed++;
        }
        if (WasPressed(s_Link))
        {
            command = TransferZInputCommand.LINK;
            pressed++;
        }
        if (WasPressed(s_Preferred))
        {
            command = TransferZInputCommand.PREFERRED;
            pressed++;
        }
        if (WasPressed(s_Sort))
        {
            command = TransferZInputCommand.SORT;
            pressed++;
        }
        if (WasPressed(s_Stack))
        {
            command = TransferZInputCommand.STACK;
            pressed++;
        }

        if (pressed != 1)
            return TransferZInputCommand.NONE;
        return command;
    }
}
