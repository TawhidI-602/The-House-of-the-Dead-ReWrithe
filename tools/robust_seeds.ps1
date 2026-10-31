# Robustness runs (worker "robust", docs/teaser_status/robust.md): many seeded bot play-throughs (hotd --bot <seed>)
# in parallel, headless and muted, no lives kept (real game overs, continues, player 2 joining / leaving), both model
# sources, 1P and 2P. Each run has a wall-clock timeout (a hang inside a frame). Writes <out>\results.tsv and one log
# per run.
#   tools\robust_seeds.ps1 -Exe build-robust\Release\hotd.exe -Seeds 1..40 [-Parallel 8] [-Out <folder>] [-Frames 150000]
param(
    [string]$Exe = "build-robust\Release\hotd.exe",
    [int[]]$Seeds = (1..32),
    [int]$Parallel = 8,
    [string]$Out = "build-robust\seeds",
    [int]$Frames = 150000,
    [int]$TimeoutSec = 600,
    [string]$Root = "C:\Games\HOTD M2 RECOMPILE\M2 Source",
    [string[]]$Extra = @()
)
$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force $Out | Out-Null
# a copy of the exe (the build can relink while the runs go on)
$bin = Join-Path $Out "bin"
New-Item -ItemType Directory -Force $bin | Out-Null
Copy-Item (Resolve-Path $Exe).Path $bin -Force
Copy-Item (Join-Path (Split-Path (Resolve-Path $Exe).Path) "SDL3.dll") $bin -Force
$exePath = (Resolve-Path (Join-Path $bin (Split-Path $Exe -Leaf))).Path
$jobs = @()
foreach ($s in $Seeds) {
    # the seed picks the configuration too: models (even / odd), players (seed / 2 even / odd), lives setting
    $models = if ($s % 2 -eq 0) { "arcade" } else { "pc" }
    $players = if ([math]::Floor($s / 2) % 2 -eq 0) { 1 } else { 2 }
    $lives = @(9, 0, 3, 9)[[math]::Floor($s / 4) % 4]
    $start2 = $players -eq 2 -and ($s % 3 -eq 0)          # both players from the start (else player 2 joins later)
    $strict = ($s % 5 -eq 4)                                # teaser cuts off (no guards) now and then
    $jobs += [pscustomobject]@{ Seed = $s; Models = $models; Players = $players; Lives = $lives; Start2 = $start2; Strict = $strict }
}
$running = @()
$results = @()
function Finish($r) {
    $log = Get-Content $r.Log -ErrorAction SilentlyContinue
    $line = ($log | Where-Object { $_ -like "ROBUST:*" } | Select-Object -Last 1)
    $exit = if ($r.TimedOut) { "timeout" } else { $r.Proc.ExitCode }
    if (-not $line) { $line = "ROBUST: result=none" }
    $rline = ($log | Where-Object { $_ -like "RENDER:*" } | Select-Object -Last 1)   # with -Extra --render (robust-2)
    $glerr = @($log | Where-Object { $_ -like "render: GL error*" -or $_ -like "render: draw-call spike*" }).Count
    $script:results += [pscustomobject]@{ Seed = $r.Job.Seed; Models = $r.Job.Models; Players = $r.Job.Players; Lives = $r.Job.Lives; Start2 = $r.Job.Start2; Strict = $r.Job.Strict;
        Exit = $exit; Secs = [math]::Round(((Get-Date) - $r.Start).TotalSeconds, 1); Robust = $line; Render = $rline; RenderLines = $glerr }
    Write-Host ("seed {0} {1} {2}P start2 {6} strict {7} lives {3}: exit {4} {5}" -f $r.Job.Seed, $r.Job.Models, $r.Job.Players, $r.Job.Lives, $exit, $line, $r.Job.Start2, $r.Job.Strict)
}
foreach ($j in $jobs) {
    while ($running.Count -ge $Parallel) {
        Start-Sleep -Milliseconds 200
        $still = @()
        foreach ($r in $running) {
            if ($r.Proc.HasExited) { Finish $r }
            elseif (((Get-Date) - $r.Start).TotalSeconds -gt $TimeoutSec) { $r.Proc.Kill(); $r.TimedOut = $true; Finish $r }
            else { $still += $r }
        }
        $running = $still
    }
    $log = Join-Path $Out ("seed{0}.log" -f $j.Seed)
    $argv = @("--root", "`"$Root`"", "--frames", $Frames, "--bot", $j.Seed, "--bot-players", $j.Players, "--mute",
              "--models", $j.Models, "--lives", $j.Lives, "--players", $(if ($j.Start2) { "2" } else { "1" })) + $Extra
    if ($j.Strict) { $argv += "--strict" }
    $p = Start-Process -FilePath $exePath -ArgumentList $argv -NoNewWindow -PassThru -RedirectStandardOutput $log `
                       -RedirectStandardError ($log + ".err")
    $null = $p.Handle   # keeps the exit code readable after the process ends
    $running += [pscustomobject]@{ Job = $j; Proc = $p; Start = Get-Date; Log = $log; TimedOut = $false }
}
while ($running.Count -gt 0) {
    Start-Sleep -Milliseconds 200
    $still = @()
    foreach ($r in $running) {
        if ($r.Proc.HasExited) { Finish $r }
        elseif (((Get-Date) - $r.Start).TotalSeconds -gt $TimeoutSec) { $r.Proc.Kill(); $r.TimedOut = $true; Finish $r }
        else { $still += $r }
    }
    $running = $still
}
$results | Sort-Object Seed | Export-Csv -Path (Join-Path $Out "results.tsv") -Delimiter "`t" -NoTypeInformation
