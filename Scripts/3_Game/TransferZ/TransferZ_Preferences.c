class TransferZPreferences
{
    string preferred_slot = "";
    ref array<string> preferred_path;
    int preferred_cargo_index = 0;

    void TransferZPreferences()
    {
        preferred_path = new array<string>();
    }
}
