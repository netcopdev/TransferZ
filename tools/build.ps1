[CmdletBinding()]
param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [string]$OutputDir = (Join-Path (Resolve-Path (Join-Path $PSScriptRoot "..")).Path "dist"),
    [string]$ReleaseRoot = (Join-Path (Resolve-Path (Join-Path $PSScriptRoot "..")).Path "dist\release"),
    [string]$BuildConfig,
    [string]$PrivateKey,
    [string]$PublicKey,
    [string]$AddonBuilder,
    [string]$DSSignFile,
    [string]$BankRev
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-RequiredFile {
    param(
        [string]$ExplicitPath,
        [string[]]$Candidates,
        [string]$Description
    )

    if ($ExplicitPath) {
        if (-not (Test-Path -LiteralPath $ExplicitPath -PathType Leaf)) {
            throw "$Description was not found at '$ExplicitPath'."
        }
        return (Resolve-Path -LiteralPath $ExplicitPath).Path
    }

    foreach ($candidate in $Candidates) {
        if ($candidate -and (Test-Path -LiteralPath $candidate -PathType Leaf)) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    throw "$Description was not found. Pass it explicitly or configure it in the TransferZ build config."
}

function Get-ConfigString {
    param(
        [hashtable]$Config,
        [string]$Name
    )

    if ($Config.ContainsKey($Name) -and $null -ne $Config[$Name]) {
        $value = [string]$Config[$Name]
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            return $value
        }
    }

    return $null
}

function Assert-PboContents {
    param(
        [Parameter(Mandatory = $true)][string]$PboPath,
        [Parameter(Mandatory = $true)][string]$BankRevPath
    )

    $listing = @(& $BankRevPath -l $PboPath)
    if ($LASTEXITCODE -ne 0) {
        throw "BankRev failed while auditing '$PboPath' with exit code $LASTEXITCODE."
    }

    $forbiddenExtensions = @(
        '.png', '.jpg', '.jpeg', '.webp', '.bmp', '.tga',
        '.py', '.pyc', '.pyo', '.ps1', '.psm1', '.psd', '.xcf', '.svg',
        '.md', '.ttf', '.otf', '.zip', '.7z'
    )

    $forbidden = @($listing | Where-Object {
        $line = $_.Trim()
        if (-not $line) { return $false }
        $extension = [System.IO.Path]::GetExtension($line).ToLowerInvariant()
        return $forbiddenExtensions -contains $extension
    })

    if ($forbidden.Count -gt 0) {
        throw "Release PBO contains development assets:`n  $($forbidden -join "`n  ")"
    }

    $scriptCount = @($listing | Where-Object { $_ -match '(?i)\.c$' }).Count
    Write-Host "PBO content audit passed:"
    Write-Host "  Scripts      : $scriptCount"
}

$projectRootFull = (Resolve-Path -LiteralPath $ProjectRoot).Path
$outputDirFull = [System.IO.Path]::GetFullPath($OutputDir)
$releaseRootFull = [System.IO.Path]::GetFullPath($ReleaseRoot)

$buildConfigPath = $BuildConfig
if (-not $buildConfigPath -and $env:TRANSFERZ_BUILD_CONFIG) {
    $buildConfigPath = $env:TRANSFERZ_BUILD_CONFIG
}
if (-not $buildConfigPath) {
    if (-not $env:LOCALAPPDATA) {
        throw "LOCALAPPDATA is unavailable. Pass -BuildConfig or set TRANSFERZ_BUILD_CONFIG."
    }
    $buildConfigPath = Join-Path $env:LOCALAPPDATA "TransferZ\build.psd1"
}
$buildConfigPath = [System.IO.Path]::GetFullPath($buildConfigPath)

$localConfig = @{}
if (Test-Path -LiteralPath $buildConfigPath -PathType Leaf) {
    $loadedConfig = Import-PowerShellDataFile -LiteralPath $buildConfigPath
    if ($null -eq $loadedConfig) {
        throw "TransferZ build config '$buildConfigPath' did not contain a PowerShell data table."
    }
    $localConfig = $loadedConfig
}

if (-not $PrivateKey) { $PrivateKey = Get-ConfigString $localConfig 'PrivateKey' }
if (-not $PublicKey) { $PublicKey = Get-ConfigString $localConfig 'PublicKey' }
if (-not $AddonBuilder) { $AddonBuilder = Get-ConfigString $localConfig 'AddonBuilder' }
if (-not $DSSignFile) { $DSSignFile = Get-ConfigString $localConfig 'DSSignFile' }
if (-not $BankRev) { $BankRev = Get-ConfigString $localConfig 'BankRev' }

if (-not $PrivateKey -or -not $PublicKey) {
    throw "TransferZ signing paths are not configured. Create '$buildConfigPath' from tools\build-config.example.psd1, or pass -PrivateKey and -PublicKey explicitly."
}

$steamRoots = @(
    "C:\Program Files (x86)\Steam\steamapps\common",
    "C:\Program Files\Steam\steamapps\common",
    "D:\SteamLibrary\steamapps\common",
    "E:\SteamLibrary\steamapps\common"
)

$addonBuilderCandidates = $steamRoots | ForEach-Object { Join-Path $_ "DayZ Tools\Bin\AddonBuilder\AddonBuilder.exe" }
$dsSignCandidates = $steamRoots | ForEach-Object { Join-Path $_ "DayZ Tools\Bin\DsUtils\DSSignFile.exe" }
$bankRevCandidates = $steamRoots | ForEach-Object { Join-Path $_ "DayZ Tools\Bin\PboUtils\BankRev.exe" }

$addonBuilderExe = Resolve-RequiredFile $AddonBuilder $addonBuilderCandidates "AddonBuilder.exe"
$dsSignFileExe = Resolve-RequiredFile $DSSignFile $dsSignCandidates "DSSignFile.exe"
$bankRevExe = Resolve-RequiredFile $BankRev $bankRevCandidates "BankRev.exe"
$privateKeyPath = Resolve-RequiredFile $PrivateKey @() "TransferZ private signing key"
$publicKeyPath = Resolve-RequiredFile $PublicKey @() "TransferZ public signing key"

$pboBuildScript = Join-Path $PSScriptRoot "build-pbo.ps1"
if (-not (Test-Path -LiteralPath $pboBuildScript -PathType Leaf)) {
    throw "PBO build helper was not found at '$pboBuildScript'."
}

New-Item -ItemType Directory -Force -Path $outputDirFull | Out-Null
New-Item -ItemType Directory -Force -Path $releaseRootFull | Out-Null

Write-Host "TransferZ release build"
Write-Host "  Project      : $projectRootFull"
Write-Host "  Build config : $buildConfigPath"
Write-Host "  Output       : $outputDirFull"
Write-Host "  Release      : $releaseRootFull"
Write-Host ""

& $pboBuildScript -AddonBuilder $addonBuilderExe -ProjectRoot $projectRootFull -OutputDir $outputDirFull
if (-not $?) {
    throw "TransferZ PBO build failed."
}

$pboPath = Join-Path $outputDirFull "TransferZ.pbo"
if (-not (Test-Path -LiteralPath $pboPath -PathType Leaf)) {
    throw "Expected built PBO was not found at '$pboPath'."
}

Assert-PboContents -PboPath $pboPath -BankRevPath $bankRevExe
$pboInfo = Get-Item -LiteralPath $pboPath
Write-Host ("PBO size       : {0:N2} MB" -f ($pboInfo.Length / 1MB))

Get-ChildItem -LiteralPath $outputDirFull -Filter "TransferZ.pbo*.bisign" -File -ErrorAction SilentlyContinue | Remove-Item -Force

Write-Host ""
Write-Host "Signing $pboPath ..."
& $dsSignFileExe $privateKeyPath $pboPath
if ($LASTEXITCODE -ne 0) {
    throw "DSSignFile failed with exit code $LASTEXITCODE."
}

$signature = Get-ChildItem -LiteralPath $outputDirFull -Filter "TransferZ.pbo*.bisign" -File -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTimeUtc -Descending |
    Select-Object -First 1
if (-not $signature) {
    throw "DSSignFile returned success but no TransferZ .bisign file was created."
}

$releaseModRoot = Join-Path $releaseRootFull "@TransferZ"
$releaseAddons = Join-Path $releaseModRoot "addons"
$releaseKeys = Join-Path $releaseModRoot "keys"

if (Test-Path -LiteralPath $releaseModRoot) {
    Remove-Item -LiteralPath $releaseModRoot -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $releaseAddons | Out-Null
New-Item -ItemType Directory -Force -Path $releaseKeys | Out-Null

Copy-Item -LiteralPath $pboPath -Destination (Join-Path $releaseAddons "TransferZ.pbo") -Force
Copy-Item -LiteralPath $signature.FullName -Destination (Join-Path $releaseAddons $signature.Name) -Force
Copy-Item -LiteralPath $publicKeyPath -Destination (Join-Path $releaseKeys ([System.IO.Path]::GetFileName($publicKeyPath))) -Force

$modCpp = Join-Path $projectRootFull 'mod.cpp'
if (Test-Path -LiteralPath $modCpp -PathType Leaf) {
    Copy-Item -LiteralPath $modCpp -Destination (Join-Path $releaseModRoot 'mod.cpp') -Force
}

$releasePbo = Join-Path $releaseAddons "TransferZ.pbo"
$releaseBisign = Join-Path $releaseAddons $signature.Name
$releaseBikey = Join-Path $releaseKeys ([System.IO.Path]::GetFileName($publicKeyPath))
if (-not (Test-Path -LiteralPath $releasePbo -PathType Leaf) -or -not (Test-Path -LiteralPath $releaseBisign -PathType Leaf) -or -not (Test-Path -LiteralPath $releaseBikey -PathType Leaf)) {
    throw "Release package verification failed."
}

$releaseFiles = Get-ChildItem -LiteralPath $releaseModRoot -Recurse -File
$releaseBytes = ($releaseFiles | Measure-Object Length -Sum).Sum

Write-Host ""
Write-Host "Release package ready:"
Write-Host "  $releaseModRoot"
Write-Host ("  Total size   : {0:N2} MB" -f ($releaseBytes / 1MB))
Write-Host ""
Write-Host "Contents:"
Write-Host "  addons\TransferZ.pbo"
Write-Host "  addons\$($signature.Name)"
Write-Host "  keys\$([System.IO.Path]::GetFileName($publicKeyPath))"
if (Test-Path -LiteralPath (Join-Path $releaseModRoot 'mod.cpp') -PathType Leaf) {
    Write-Host "  mod.cpp"
}
Write-Host ""
Write-Host "Deploy @TransferZ to both server and client."
Write-Host "The server must also have the public .bikey in its root keys directory."
