param(
    [Parameter(Mandatory = $true)]
    [string]$DayZDiagPath,

    [Parameter(Mandatory = $true)]
    [string]$ModList,

    [string]$ProfilesPath = (Join-Path $PSScriptRoot "..\diag-profile"),

    [int]$TimeoutSeconds = 120
)

$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$missionPath = Join-Path $repoRoot "test\TransferZTest.ChernarusPlus"
$profilesFullPath = [System.IO.Path]::GetFullPath($ProfilesPath)

if (-not (Test-Path -LiteralPath $DayZDiagPath)) {
    throw "DayZDiag executable not found: $DayZDiagPath"
}
if (-not (Test-Path -LiteralPath $missionPath)) {
    throw "TransferZ test mission not found: $missionPath"
}

New-Item -ItemType Directory -Force -Path $profilesFullPath | Out-Null
$startedAt = Get-Date

$arguments = @(
    "-mod=`"$ModList`"",
    "-mission=`"$missionPath`"",
    "-profiles=`"$profilesFullPath`"",
    "-window",
    "-nopause",
    "-filePatching"
)

Write-Host "Launching TransferZ DayZDiag self-test mission..."
$process = Start-Process -FilePath $DayZDiagPath -ArgumentList $arguments -PassThru

$deadline = (Get-Date).AddSeconds($TimeoutSeconds)
$passMarker = "[TransferZTest] SUITE PASS"
$failMarker = "[TransferZTest] SUITE FAIL"
$matchedFile = $null
$matchedLine = $null

try {
    while ((Get-Date) -lt $deadline) {
        $candidateFiles = Get-ChildItem -LiteralPath $profilesFullPath -Recurse -File -ErrorAction SilentlyContinue |
            Where-Object { $_.LastWriteTime -ge $startedAt.AddSeconds(-2) -and ($_.Extension -ieq ".log" -or $_.Extension -ieq ".rpt") }

        foreach ($file in $candidateFiles) {
            $matches = Select-String -LiteralPath $file.FullName -SimpleMatch -Pattern $passMarker, $failMarker -ErrorAction SilentlyContinue
            if ($matches) {
                $lastMatch = $matches | Select-Object -Last 1
                $matchedFile = $file.FullName
                $matchedLine = $lastMatch.Line
                break
            }
        }

        if ($matchedLine) {
            break
        }

        if ($process.HasExited) {
            break
        }

        Start-Sleep -Milliseconds 500
        $process.Refresh()
    }
}
finally {
    if (-not $process.HasExited) {
        Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
    }
}

if (-not $matchedLine) {
    Write-Error "No TransferZ self-test suite marker was found within $TimeoutSeconds seconds. Inspect logs under $profilesFullPath."
    exit 2
}

Write-Host $matchedLine
Write-Host "Log: $matchedFile"

if ($matchedLine.Contains($failMarker)) {
    exit 1
}

exit 0
