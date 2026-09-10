@{
    # Keep local machine paths out of the repository. Copy this file to:
    #   $env:LOCALAPPDATA\TransferZ\build.psd1
    # or point TRANSFERZ_BUILD_CONFIG at another local file.

    PrivateKey   = 'C:\path\to\TransferZ.biprivatekey'
    PublicKey    = 'C:\path\to\TransferZ.bikey'

    # Optional overrides. Leave blank to use normal Steam-library discovery.
    AddonBuilder = ''
    DSSignFile   = ''
    BankRev      = ''
}
