# T1 integration: the headless default-route play-throughs of Chapter 1 (docs/teaser_status/t1.md).
# Runs hotd --frames N --autofire --mute --lives-kept 1 for: arcade / PC models x 1 / 2 players (P2 joins at frame
# 2600) x teaser cuts on / --strict --default-route (guards off, route 0), and one --strict run on the route the shots choose; each with --boundary-report; prints one line per run (chapter clear frame, or the
# event-script op it stalled on) and writes the logs and reports to -Out.
#   powershell -File tools\t1_playthroughs.ps1 [-Build build-t1] [-Out <dir>] [-Frames 70000] [-Root <repo with private\>]
param(
    [string]$Build = "build-t1",
    [string]$Out = "$env:TEMP\hotd_t1",
    [int]$Frames = 70000,
    [string]$Root = ""
)
$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $PSScriptRoot
if (-not $Root) {
    $Root = $here
    if (-not (Test-Path (Join-Path $Root "private"))) {   # a git worktree: the main checkout's private\
        $Root = (Resolve-Path (Join-Path $here "..\..\..")).Path
    }
}
$exe = Join-Path $here "$Build\Release\hotd.exe"
New-Item -ItemType Directory -Force $Out | Out-Null
$runs = @(
    @{ n = "a1";  a = @() },
    @{ n = "a2";  a = @("--start-at", "2:2600") },
    @{ n = "p1";  a = @("--models", "pc") },
    @{ n = "p2";  a = @("--models", "pc", "--start-at", "2:2600") },
    @{ n = "a1s"; a = @("--strict", "--default-route") },
    @{ n = "a2s"; a = @("--strict", "--default-route", "--start-at", "2:2600") },
    @{ n = "p1s"; a = @("--models", "pc", "--strict", "--default-route") },
    @{ n = "p2s"; a = @("--models", "pc", "--strict", "--default-route", "--start-at", "2:2600") },
    @{ n = "a1o"; a = @("--strict") }   # the original's behaviour: the route the shots choose (branch events)
)
foreach ($r in $runs) {
    $log = Join-Path $Out "$($r.n).txt"
    $rep = Join-Path $Out "$($r.n)_boundaries.md"
    $hargs = @("--root", $Root, "--frames", "$Frames", "--autofire", "--mute", "--lives-kept", "1", "--boundary-report", $rep) + $r.a
    $sw = [Diagnostics.Stopwatch]::StartNew()
    & $exe @hargs > $log 2>&1
    $code = $LASTEXITCODE
    $t = [int]$sw.Elapsed.TotalSeconds
    $lines = Get-Content $log
    $end = ($lines | Select-String "the chapter is over" | Select-Object -First 1)
    $stall = ($lines | Select-String "^event script at" | Select-Object -Last 1)
    $ev = ($lines | Select-String "^events played" | Select-Object -Last 1)
    $score = ($lines | Select-String "^player \d: state" | ForEach-Object { $_.Line -replace ".*score (\-?\d+).*", '$1' }) -join "/"
    $bad = ($lines | Select-String "access outside|halted|too deep" | Measure-Object).Count
    $res = if ($end) { $end.Line } elseif ($code -ne 0) { "EXIT CODE $code (crash?)" } else { "no clear: " + $stall.Line }
    "{0,-4} {1}s  {2}  | {3} | scores {4} | warnings {5}" -f $r.n, $t, $res, $ev.Line, $score, $bad
}
