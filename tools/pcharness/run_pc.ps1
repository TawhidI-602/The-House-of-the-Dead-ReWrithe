# run_pc.ps1 - runs the PC port under the harness: a COPY of the game in private\pcharness\game (never C:\Games\HOTD)
# with the hook (private\pcharness\bin\dinput.dll, built by build.ps1) next to THOTD.EXE.
#   .\run_pc.ps1 -Out "<folder inside private>" [-Ticks 600] [-Args ""] [-TimeoutSeconds 120] [-Sound] [-InputScript <file>]
#                 [-CapFuncs "4c3d58,..."] [-CapBoundary "..."] [-CapMax 30] [-CapIf "addr:mask,..."] [-CapIfArg "off:mask"]
#                 [-CapFront 2] [-CapFrom <tick>] [-CapPorted "<ported routines for branch coverage>"] [-InfLives] [-TwoPlayers]
#                 [-Route "1:11,1:12"] [-AimSkip "4ccf20"] [-Map <state map>]   (chapter:event: route_choice = 1 there, so the chapter takes its other branch)
#   -CapFuncs: per-routine captures (hotd_hook.c "per-routine capture", docs/proof.md) into <Out>\capture.bin
# The copy is set to run windowed through dgVoodoo (FullScreenMode = false) with CD music muted; the hook keeps the
# game from taking focus or the mouse, gives it a tick-based clock, logs every tick to <Out>\pc.log and writes the
# game state after every tick to <Out>\state.rec (HOTDREC1, as tools/run_state_rec.ps1; tools/state_diff.py).
param([Parameter(Mandatory)] [string]$Out, [int]$Ticks = 600, [string]$Args = "", [int]$TimeoutSeconds = 120,
      [switch]$Sound, [string]$InputScript = "", [string]$CapFuncs = "", [string]$CapBoundary = "", [int]$CapMax = 30,
      [string]$CapIf = "", [string]$CapIfArg = "", [int]$CapFront = -1, [int]$CapFrom = 0, [string]$CapPorted = "", [switch]$InfLives, [switch]$TwoPlayers, [string]$Route = "",
      [string]$AimSkip = "", [string]$Map = "")
$root = Split-Path (Split-Path $PSScriptRoot)
$game = Join-Path $root "private\pcharness\game"
$bin = Join-Path $root "private\pcharness\bin"
# in a git worktree (no game copy of its own): the main checkout's game copy, with this worktree's hook build if any
if (-not (Test-Path (Join-Path $game "THOTD.EXE"))) {
    $main = Split-Path (Resolve-Path (& git -C $root rev-parse --git-common-dir)).Path
    $game = Join-Path $main "private\pcharness\game"
    if (-not (Test-Path (Join-Path $bin "dinput.dll"))) { $bin = Join-Path $main "private\pcharness\bin" }
}
if (-not (Test-Path (Join-Path $game "THOTD.EXE"))) { throw "no game copy in $game" }
if ((Resolve-Path $game).Path -like "C:\Games\HOTD\*" -or (Resolve-Path $game).Path -eq "C:\Games\HOTD") { throw "refusing to run in the original install" }
New-Item -ItemType Directory -Force $Out | Out-Null
$Out = (Resolve-Path $Out).Path
# Several games can run at once (the hook keeps the game's single-instance check and its settings files to each run).
# A running game keeps its dinput.dll open, so a different hook build goes in by renaming the loaded one aside
# (allowed for a mapped DLL); the old copies are removed once nothing uses them.
$hook = Join-Path $game "dinput.dll"
Get-ChildItem $game -Filter "dinput.dll.old-*" | ForEach-Object { Remove-Item -Force $_.FullName -ErrorAction SilentlyContinue }
if (-not (Test-Path $hook) -or (Get-FileHash $hook).Hash -ne (Get-FileHash (Join-Path $bin "dinput.dll")).Hash) {
    if (Test-Path $hook) { Rename-Item $hook "dinput.dll.old-$([guid]::NewGuid().ToString('N'))" -ErrorAction Stop }
    Copy-Item -Force (Join-Path $bin "dinput.dll") $hook -ErrorAction Stop
}

# windowed, no music
$conf = Join-Path $game "dgVoodoo.conf"
$wini = Join-Path $game "winmm.ini"
# rewritten only when they change (another game may be reading them as it starts)
$c = Get-Content $conf; $n = $c -replace '^FullScreenMode\s*=.*$', 'FullScreenMode                       = false'
if (($c -join "`n") -ne ($n -join "`n")) { $n | Set-Content -Encoding ascii $conf }
$c = Get-Content $wini; $n = $c -replace '^(CDDAVolume|MIDIVolume|WAVEVolume)\s*=.*$', '$1 = 0'
if (($c -join "`n") -ne ($n -join "`n")) { $n | Set-Content -Encoding ascii $wini }

# the settings files the game reads at start and writes at exit: each run gets its own copy (HOTD_PC_CFG_DIR), so
# runs never share them and every run starts from the game folder's copies
foreach ($f in "THOTD.CFG", "SCORES.CFG", "CSTATS.CFG") {
    $src = Join-Path $game $f
    if (Test-Path $src) { Copy-Item -Force $src (Join-Path $Out $f) } else { Remove-Item -Force (Join-Path $Out $f) -ErrorAction SilentlyContinue }
}
$env:HOTD_PC_CFG_DIR = $Out

# hooked games keep the "already running" atom to themselves; a crashed unhooked game can leave it behind, which
# would stop the original install from starting: cleared if no THOTD runs at all
Add-Type -Namespace Hotd -Name Atom -MemberDefinition @'
[DllImport("kernel32.dll")] public static extern ushort GlobalFindAtomA(string s);
[DllImport("kernel32.dll")] public static extern ushort GlobalDeleteAtom(ushort a);
'@
if (-not (Get-Process THOTD -ErrorAction SilentlyContinue)) {
    $a = [Hotd.Atom]::GlobalFindAtomA("TheHouseOfTheDead")
    if ($a -ne 0) { [void][Hotd.Atom]::GlobalDeleteAtom($a); "cleared a stale instance atom" }
}

$env:__COMPAT_LAYER = "!DWM8And16BitMitigation"
$env:HOTD_PC_LOG = Join-Path $Out "pc.log"
$env:HOTD_PC_TICKS = "$Ticks"
$env:HOTD_PC_REC = Join-Path $Out "state.rec"
# -Map: another state map (e.g. tools\pcharness\pccap_map.tsv: tools\state_map.tsv plus the gun records and Boss Mode's state)
$env:HOTD_PC_MAP = $(if ($Map) { (Resolve-Path $Map).Path } else { Join-Path $root "tools\state_map.tsv" })
$env:HOTD_PC_MUTE = $(if ($Sound) { "0" } else { "1" })
$env:HOTD_PC_INFLIVES = $(if ($InfLives) { "1" } else { "0" })   # player 1's lives kept at 3 (long scripted games)
$env:HOTD_PC_TWO_PLAYERS = $(if ($TwoPlayers) { "1" } else { "0" })   # with -InfLives: player 2's lives too (an input script that joins player 2: scripts\game_aim_2p.txt)
$env:HOTD_PC_AIM_SKIP = $AimSkip   # "4ccf20,...": routines the auto-aim never aims at (e.g. the researchers' frame)
$env:HOTD_PC_ROUTE = $Route   # "chapter:event,...": route_choice = 1 in those events (the other branch, route_branches)
$env:HOTD_PC_INPUT = $(if ($InputScript) { (Resolve-Path $InputScript).Path } else { "" })
$env:HOTD_PC_CAP_FILE = $(if ($CapFuncs) { Join-Path $Out "capture.bin" } else { "" })
$env:HOTD_PC_CAP_FUNCS = $CapFuncs; $env:HOTD_PC_CAP_BOUNDARY = $CapBoundary; $env:HOTD_PC_CAP_MAX = "$CapMax"
$env:HOTD_PC_CAP_IF = $CapIf; $env:HOTD_PC_CAP_IF_ARG = $CapIfArg; $env:HOTD_PC_CAP_FRONT = "$CapFront"; $env:HOTD_PC_CAP_FROM = "$CapFrom"
# branch coverage of the ported routines (-CapPorted, default -CapFuncs): tools/pc_branches.py lists their jumps
$env:HOTD_PC_CAP_BRANCHES = ""; $env:HOTD_PC_CAP_COVER = ""
$portedList = $(if ($CapPorted) { $CapPorted } else { $CapFuncs })
if ($CapFuncs -and $portedList) {
    $brFile = Join-Path $Out "branches.txt"
    & py (Join-Path $root "tools\pc_branches.py") (Join-Path $game "THOTD.EXE") $brFile $portedList | Out-Null
    $env:HOTD_PC_CAP_BRANCHES = $brFile; $env:HOTD_PC_CAP_COVER = Join-Path $Out "coverage.txt"
}
$exe = Join-Path $game "THOTD.EXE"
@("date: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss zzz')", "exe: $exe",
  "exe_sha256: $((Get-FileHash $exe).Hash)", "hook_sha256: $((Get-FileHash (Join-Path $game 'dinput.dll')).Hash)",
  "args: $Args", "ticks: $Ticks", "mute: $env:HOTD_PC_MUTE", "input: $env:HOTD_PC_INPUT", "inflives: $env:HOTD_PC_INFLIVES", "route: $Route",
  "capture: funcs $CapFuncs boundary $CapBoundary max $CapMax if $CapIf ifarg $CapIfArg front $CapFront from $CapFrom") | Set-Content (Join-Path $Out "recording.txt")
$sp = @{ FilePath = $exe; WorkingDirectory = $game; PassThru = $true }
if ($Args) { $sp.ArgumentList = $Args }
$p = Start-Process @sp
if (-not $p.WaitForExit($TimeoutSeconds * 1000)) { Stop-Process -Id $p.Id -Force; "TIMEOUT: the game was stopped." }
"exit code: $($p.ExitCode)"
Get-Content (Join-Path $Out "pc.log") -TotalCount 40
