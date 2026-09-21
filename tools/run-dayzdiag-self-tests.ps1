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

$steam = Get-ItemProperty -Path "HKCU:\\Software\\Valve\\Steam\\ActiveProcess" -ErrorAction SilentlyContinue
if (-not $steam -or [int64]$steam.pid -eq 0 -or [int64]$steam.ActiveUser -eq 0) {
    throw "Steam client session is not active. Restart Steam and wait until ActiveProcess has non-zero pid and ActiveUser."
}

New-Item -ItemType Directory -Force -Path $profilesFullPath | Out-Null
$startedAt = Get-Date

$launchDir = Join-Path $repoRoot "dist\\self-test"
New-Item -ItemType Directory -Force -Path $launchDir | Out-Null
$launchBat = Join-Path $launchDir ("launch-" + [guid]::NewGuid().ToString("N") + ".bat")

if ($DayZDiagPath.Contains('"') -or $ModList.Contains('"') -or $missionPath.Contains('"') -or $profilesFullPath.Contains('"')) {
    throw "DayZDiag self-test paths/mod list may not contain a double quote."
}

$batchLines = @(
    "@echo off",
    "start `"`" /wait `"$DayZDiagPath`" `"-mod=$ModList`" `"-mission=$missionPath`" `"-profiles=$profilesFullPath`" -window -nopause -filePatching",
    "exit /b %ERRORLEVEL%"
)
Set-Content -LiteralPath $launchBat -Value $batchLines -Encoding ASCII

Write-Host "Launching TransferZ DayZDiag self-test mission..."
$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = $env:ComSpec
$psi.Arguments = "/d /s /c `"`"$launchBat`"`""
$psi.UseShellExecute = $false
$process = [System.Diagnostics.Process]::Start($psi)
if (-not $process) {
    throw "Failed to launch DayZDiag wrapper process."
}

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
    if ($process -and -not $process.HasExited) {
        $taskKill = Join-Path $env:SystemRoot "System32\\taskkill.exe"
        if (Test-Path -LiteralPath $taskKill -PathType Leaf) {
            & $taskKill /PID $process.Id /T /F | Out-Null
        }
        else {
            Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
        }
    }

    if ($launchBat -and (Test-Path -LiteralPath $launchBat -PathType Leaf)) {
        Remove-Item -LiteralPath $launchBat -Force -ErrorAction SilentlyContinue
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
