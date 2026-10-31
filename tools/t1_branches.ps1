# T1: every branch path of Chapter 1 headless (hotd --route, --strict --default-route: no guards, the other branches
# held at 0). Prints one line per path (clear frame, or the event-script op it stalled on) and writes the logs and
# boundary reports to -Out. Branches (arcade 0xAABE0): 0 -> 1 / 2, 3 -> 4 / 9, 4 -> 5 / 7, 5 -> 6 / 12, 7 -> 8 / 16,
# 9 -> 10 / 11, 11 / 12 -> 15 / 17; 10 -> 14, 15 / 16 -> 18, 17 -> 13; 6, 8, 13, 14, 18 -> 19.
#   powershell -File tools\t1_branches.ps1 [-Build build-t1] [-Out <dir>] [-Frames 70000] [-Root <repo>] [-Cuts]
param([string]$Build = "build-t1", [string]$Out = "$env:TEMP\hotd_t1", [int]$Frames = 70000, [string]$Root = "",
      [switch]$Cuts)
$here = Split-Path -Parent $PSScriptRoot
if (-not $Root) {
    $Root = $here
    if (-not (Test-Path (Join-Path $Root "private"))) { $Root = (Resolve-Path (Join-Path $here "..\..\..")).Path }
}
$exe = Join-Path $here "$Build\Release\hotd.exe"
New-Item -ItemType Directory -Force $Out | Out-Null
$paths = @("1:0,1:3", "1:3,1:9", "1:3,1:9,1:11", "1:4", "1:4,1:7", "1:5", "1:5,1:12")
foreach ($p in $paths) {
    $n = "route_" + ($p -replace '[:,]', '_')
    $mode = if ($Cuts) { @() } else { @("--strict", "--default-route") }
    $a = @("--root", $Root, "--frames", "$Frames", "--autofire", "--mute", "--lives-kept", "1", "--route", $p,
           "--boundary-report", (Join-Path $Out "$n.md")) + $mode
    $lines = & $exe @a 2>&1
    $lines | Out-File (Join-Path $Out "$n.txt")
    $end = $lines | Select-String "the chapter is over" | Select-Object -First 1
    $stall = $lines | Select-String "^event script at" | Select-Object -Last 1
    $ev = ($lines | Select-String "^events played" | Select-Object -Last 1).Line -replace "events played \(scene_history\): ", "" -replace "; chapter_ended.*", ""
    $res = if ($end) { $end.Line } else { "no clear: " + $stall.Line }
    "{0,-14} events {1,-22} {2}" -f $p, $ev, $res
}
