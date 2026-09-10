param(
    [string]$AddonBuilder,
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [string]$OutputDir = (Join-Path (Resolve-Path (Join-Path $PSScriptRoot "..")).Path "dist")
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Copy-RuntimeFile {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$RelativeDestination,
        [Parameter(Mandatory = $true)][string]$StageRoot
    )

    if (-not (Test-Path -LiteralPath $Source -PathType Leaf)) {
        throw "Required runtime file was not found: '$Source'."
    }

    $destination = Join-Path $StageRoot $RelativeDestination
    $destinationDirectory = Split-Path -Parent $destination
    New-Item -ItemType Directory -Force -Path $destinationDirectory | Out-Null
    Copy-Item -LiteralPath $Source -Destination $destination -Force
}

function Copy-RuntimeTree {
    param(
        [Parameter(Mandatory = $true)][string]$SourceRoot,
        [Parameter(Mandatory = $true)][string]$RelativeDestination,
        [Parameter(Mandatory = $true)][string]$StageRoot
    )

    if (-not (Test-Path -LiteralPath $SourceRoot -PathType Container)) {
        throw "Required runtime directory was not found: '$SourceRoot'."
    }

    foreach ($file in Get-ChildItem -LiteralPath $SourceRoot -Recurse -File -Filter '*.c') {
        $relative = $file.FullName.Substring($SourceRoot.Length).TrimStart('\', '/')
        Copy-RuntimeFile -Source $file.FullName -RelativeDestination (Join-Path $RelativeDestination $relative) -StageRoot $StageRoot
    }
}

if (-not $AddonBuilder) {
    $candidates = @(
        "C:\Program Files (x86)\Steam\steamapps\common\DayZ Tools\Bin\AddonBuilder\AddonBuilder.exe",
        "C:\Program Files\Steam\steamapps\common\DayZ Tools\Bin\AddonBuilder\AddonBuilder.exe",
        "D:\SteamLibrary\steamapps\common\DayZ Tools\Bin\AddonBuilder\AddonBuilder.exe",
        "E:\SteamLibrary\steamapps\common\DayZ Tools\Bin\AddonBuilder\AddonBuilder.exe"
    )
    $AddonBuilder = $candidates | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } | Select-Object -First 1
}
if (-not $AddonBuilder -or -not (Test-Path -LiteralPath $AddonBuilder -PathType Leaf)) {
    throw "AddonBuilder.exe not found. Pass -AddonBuilder '<path-to-AddonBuilder.exe>'."
}

$projectRootFull = (Resolve-Path -LiteralPath $ProjectRoot).Path
$outputDirFull = [System.IO.Path]::GetFullPath($OutputDir)
New-Item -ItemType Directory -Force -Path $outputDirFull | Out-Null

$projectName = Split-Path -Leaf $projectRootFull.TrimEnd('\')
$stagingParent = Join-Path ([System.IO.Path]::GetTempPath()) ("TransferZ-Core-" + [guid]::NewGuid().ToString("N"))
$stagedProjectRoot = Join-Path $stagingParent $projectName
$expectedPbo = Join-Path $outputDirFull "$projectName.pbo"
$previousPboWriteTime = $null
if (Test-Path -LiteralPath $expectedPbo -PathType Leaf) {
    $previousPboWriteTime = (Get-Item -LiteralPath $expectedPbo).LastWriteTimeUtc
}

New-Item -ItemType Directory -Force -Path $stagedProjectRoot | Out-Null

try {
    Copy-RuntimeFile -Source (Join-Path $projectRootFull 'config.cpp') -RelativeDestination 'config.cpp' -StageRoot $stagedProjectRoot
    Copy-RuntimeTree -SourceRoot (Join-Path $projectRootFull 'Scripts') -RelativeDestination 'Scripts' -StageRoot $stagedProjectRoot

    $unexpected = @(Get-ChildItem -LiteralPath $stagedProjectRoot -Recurse -File | Where-Object { @('.cpp', '.c') -notcontains $_.Extension.ToLowerInvariant() })
    if ($unexpected.Count -gt 0) {
        throw "TransferZ runtime staging contains unexpected development files."
    }

    $stageFiles = @(Get-ChildItem -LiteralPath $stagedProjectRoot -Recurse -File)
    $stageBytes = ($stageFiles | Measure-Object Length -Sum).Sum
    Write-Host ("Runtime staging: {0} files, {1:N2} MB" -f $stageFiles.Count, ($stageBytes / 1MB))

    & $AddonBuilder $stagedProjectRoot $outputDirFull -clear -packonly
    if ($LASTEXITCODE -ne 0) {
        throw "AddonBuilder failed with exit code $LASTEXITCODE."
    }
    if (-not (Test-Path -LiteralPath $expectedPbo -PathType Leaf)) {
        throw "AddonBuilder did not create '$expectedPbo'."
    }

    $builtPbo = Get-Item -LiteralPath $expectedPbo
    if ($previousPboWriteTime -and $builtPbo.LastWriteTimeUtc -eq $previousPboWriteTime) {
        throw "AddonBuilder did not refresh '$expectedPbo'."
    }

    Write-Host ("Built TransferZ core PBO: {0:N2} MB" -f ($builtPbo.Length / 1MB))
}
finally {
    $tempRoot = [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath()).TrimEnd('\') + '\'
    $stagingParentFull = [System.IO.Path]::GetFullPath($stagingParent)
    if ($stagingParentFull.StartsWith($tempRoot, [System.StringComparison]::OrdinalIgnoreCase) -and (Test-Path -LiteralPath $stagingParentFull)) {
        Remove-Item -LiteralPath $stagingParentFull -Recurse -Force
    }
}
