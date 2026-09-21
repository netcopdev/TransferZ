param(
    [Parameter(Mandatory = $true)]
    [string]$DayZDiagPath,

    [Parameter(Mandatory = $true)]
    [string]$DependencyMods,

    [string]$AddonBuilderPath,

    [string]$KnowledgePackPath,

    [string]$ProfilesPath = (Join-Path $PSScriptRoot "..\diag-profile"),

    [int]$TimeoutSeconds = 120
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$python = Get-Command python -ErrorAction SilentlyContinue
if (-not $python) {
    throw "Python was not found on PATH."
}

Write-Host "[1/4] Running fast TransferZ repository contracts..."
& $python.Source (Join-Path $PSScriptRoot "self_test.py")
if ($LASTEXITCODE -ne 0) {
    throw "TransferZ repository contracts failed."
}

if ($KnowledgePackPath) {
    Write-Host "[2/4] Running DayZ Knowledge Pack lint..."
    $packRoot = [System.IO.Path]::GetFullPath($KnowledgePackPath)
    $validator = Join-Path $packRoot "tools\dayz-script-validator\scripts\script_validator.py"
    $uiReconcile = Join-Path $packRoot "tools\dayz-script-validator\scripts\ui_reconcile.py"

    if (-not (Test-Path -LiteralPath $validator -PathType Leaf)) {
        throw "DayZ script validator not found: $validator"
    }
    if (-not (Test-Path -LiteralPath $uiReconcile -PathType Leaf)) {
        throw "DayZ UI reconciler not found: $uiReconcile"
    }

    & $python.Source $validator $repoRoot
    $validatorExit = $LASTEXITCODE
    if ($validatorExit -eq 1 -or $validatorExit -gt 2) {
        throw "DayZ script validator failed with exit code $validatorExit."
    }

    & $python.Source $uiReconcile $repoRoot --json
    $uiExit = $LASTEXITCODE
    if ($uiExit -eq 1 -or $uiExit -gt 2) {
        throw "DayZ UI reconciliation failed with exit code $uiExit."
    }
}
else {
    Write-Host "[2/4] DayZ Knowledge Pack lint skipped; pass -KnowledgePackPath to enable it."
}

Write-Host "[3/4] Building isolated local @TransferZ candidate..."
$candidateRoot = Join-Path $repoRoot "dist\self-test\@TransferZ"
$candidateAddons = Join-Path $candidateRoot "addons"
New-Item -ItemType Directory -Force -Path $candidateAddons | Out-Null

$buildPbo = Join-Path $PSScriptRoot "build-pbo.ps1"
if ($AddonBuilderPath) {
    & $buildPbo -AddonBuilder $AddonBuilderPath -ProjectRoot $repoRoot -OutputDir $candidateAddons
}
else {
    & $buildPbo -ProjectRoot $repoRoot -OutputDir $candidateAddons
}
if ($LASTEXITCODE -ne 0) {
    throw "TransferZ PBO build failed."
}

$candidatePbo = Join-Path $candidateAddons "TransferZ.pbo"
if (-not (Test-Path -LiteralPath $candidatePbo -PathType Leaf)) {
    throw "Built TransferZ PBO was not found: $candidatePbo"
}

foreach ($metadataName in @("mod.cpp", "meta.cpp")) {
    $metadataSource = Join-Path $repoRoot $metadataName
    if (Test-Path -LiteralPath $metadataSource -PathType Leaf) {
        Copy-Item -LiteralPath $metadataSource -Destination (Join-Path $candidateRoot $metadataName) -Force
    }
}

$resolvedDependencies = new-object System.Collections.Generic.List[string]
foreach ($rawPath in $DependencyMods.Split(";")) {
    $trimmed = $rawPath.Trim()
    if (-not $trimmed) {
        continue
    }
    $fullPath = [System.IO.Path]::GetFullPath($trimmed)
    if (-not (Test-Path -LiteralPath $fullPath -PathType Container)) {
        throw "Dependency mod path not found: $fullPath"
    }
    $resolvedDependencies.Add($fullPath)
}
if ($resolvedDependencies.Count -eq 0) {
    throw "At least one dependency mod path is required (TransferZ currently requires CF)."
}

$effectiveMods = [string]::Join(";", $resolvedDependencies.ToArray()) + ";" + $candidateRoot

Write-Host "[4/4] Running automated DayZDiag smoke suite..."
$runner = Join-Path $PSScriptRoot "run-dayzdiag-self-tests.ps1"
$runnerArgs = @{
    DayZDiagPath = $DayZDiagPath
    ModList = $effectiveMods
    ProfilesPath = $ProfilesPath
    TimeoutSeconds = $TimeoutSeconds
}
& $runner @runnerArgs
exit $LASTEXITCODE
