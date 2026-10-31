# run_capture.ps1 - routine captures for the proof harness (docs/proof.md): replays a recorded .inp session headless
# (as run_copro_probe.ps1 -Name), logs every coprocessor call from boot and captures the given routines at entry / ret
# (tools\mame_capture.lua, MAME's debugger without a window).
#   .\run_capture.ps1 -Out "<folder inside private>" -Name rectest_us -Funcs "454e0" [-If "51f288:2,51f354:2"] [-IfArg "58:6"] [-Boundary "30ef0"]
#                     [-Ported "684b0,68010"] [-MinCalls n] [-Max 50]
#   -Ported: the ported routines whose call sites mark a boundary's return (default: -Funcs)
#                     [-Seconds 60] [-TimeoutMinutes 40] [-Route "1:11,1:12"] [-AimSkip "6dbd0"] [-TwoPlayers]
#                     [-Skip n] [-Every n] [-Always "addr:mask,..."]: sampling (the first n calls of each routine not
#                     captured, then every n-th, and between them the calls where one of the words & mask != 0)
#   -Mortal [-Shoot 0]: lives not kept (a lost game: game over); -Shoot overrides HOTD_SHOOT (0 = never fire)
#   -TwoPlayers: a fresh game (no -Name) started by both players; player 2 only stands there (mame_dump.lua HOTD_START2)
# Output: <Out>\copro.txt, <Out>\capture.bin, recording.txt.
param([Parameter(Mandatory)] [string]$Out, [string]$Name = "", [Parameter(Mandatory)] [string]$Funcs,
      [string]$If = "", [string]$IfArg = "", [string]$Ported = "", [string]$Boundary = "", [int]$MinCalls = 0, [int]$Max = 50, [double]$Seconds = 60, [int]$Chapter = 0, [string]$Base = "nvram_base_us",
      [double]$TimeoutMinutes = 40, [string]$Route = "", [string]$AimSkip = "", [int]$Skip = 0, [int]$Every = 1,
      [string]$Always = "", [switch]$TwoPlayers, [string]$AimPrefer = "", [string]$XRam = "", [switch]$Mortal, [string]$Shoot = "",
      [switch]$FiniteLives, [string]$IfNe = "", [string]$Root = "", [int]$Continue = 0, [switch]$Attract, [switch]$NoSkip, [string]$HoldFire = "")
#   -FiniteLives: the life counter is not kept at 3 (lives run out: continue countdown, game over); same as -Mortal
$env:HOTD_CAP_IF_NE = $IfNe   # "a:b,...": capture only when the u32 at a != the u32 at b (mame_capture.lua)
$env:HOTD_CAP_XRAM = $XRam   # "1802000:800,...": memory outside work RAM recorded at each capture's entry (mame_capture.lua)
$env:HOTD_CAP_SKIP = "$Skip"; $env:HOTD_CAP_EVERY = "$Every"; $env:HOTD_CAP_ALWAYS = $Always
$env:HOTD_CONTINUE = "$Continue"   # -Continue n: player 1 continues up to n times (mame_dump.lua)
$env:HOTD_START2 = $(if ($TwoPlayers) { "1" } else { "0" })
$env:HOTD_AIM_PREFER = $AimPrefer   # "565b0,...": the auto-aim aims at these routines first when on screen
$env:HOTD_AIM_SKIP = $AimSkip   # "6dbd0,...": routines the auto-aim (no -Name) never aims at
$env:HOTD_NOSKIP = $(if ($NoSkip) { "1" } else { "0" })   # -NoSkip (proofs-11): cut-scenes not skipped by the auto-aim
$env:HOTD_HOLDFIRE = $HoldFire   # -HoldFire "c:e:seconds,..." (proofs-11): no auto-aim fire for that long after entering the event
$env:HOTD_ROUTE = $Route   # "chapter:event,...": route_choice = 1 in those events (mame_dump.lua; the replay then leaves the recorded route)
$root = $(if ($Root) { $Root } else { Split-Path $PSScriptRoot })   # -Root: the checkout with private\ (a worktree's tools on the main checkout's)
$private = Join-Path $root "private"
$play = Join-Path $private "mame_play"
if ($Base -eq "nvram_base_us") {
    $bf = Join-Path $play "inp\$Name.base.txt"
    if (Test-Path $bf) { $Base = (Get-Content $bf -TotalCount 1).Trim() }
}
$mame = Join-Path $private "mame0289"
New-Item -ItemType Directory -Force "$Out\nvram" | Out-Null
$Out = (Resolve-Path $Out).Path
Copy-Item -Recurse -Force (Join-Path $private "mame_play\$Base\*") "$Out\nvram\"
$env:HOTD_PROBE_FILE = "$Out\copro.txt"; $env:HOTD_SECONDS = "$Seconds"; $env:HOTD_PROBE_FUNCS = "all"; $env:HOTD_PROBE_MAX = "100000000"
$env:HOTD_PROBE_TOTAL = "0"; $env:HOTD_PROBE_START = "0"
$env:HOTD_CAP_FILE = "$Out\capture.bin"; $env:HOTD_CAP_FUNCS = $Funcs; $env:HOTD_CAP_MAX = "$Max"; $env:HOTD_CAP_IF = $If
$env:HOTD_CAP_BOUNDARY = $Boundary; $env:HOTD_CAP_IF_ARG = $IfArg; $env:HOTD_CAP_PORTED = $(if ($Ported) { $Ported } else { $Funcs }); $env:HOTD_CAP_MINCALLS = "$MinCalls"
$env:HOTD_CAP_INDEX = Join-Path $private "decomp\arcade\maincpu_index.tsv"
$env:HOTD_CAP_COVER = "$Out\coverage.txt"   # branch coverage of the -Ported routines (tools/branch_coverage.py)
# the session's inputs exactly as replay_hotd.ps1 / run_state_rec.ps1 -Name (no dumps)
$env:HOTD_DUMP_DIR = $Out
$env:HOTD_START = "20"; $env:HOTD_COINS = $(if ($Base -eq "nvram_base") { "2" } else { "0" }); $env:HOTD_INFLIVES = "1"; $env:HOTD_INFAMMO = "1"; $env:HOTD_SHOOT = "0"
if (-not $Name) { $env:HOTD_START = "40"; $env:HOTD_COINS = "2"; $env:HOTD_SHOOT = "3"; $env:HOTD_INFAMMO = "0" }   # no recording: a fresh game, auto-aim (mame_dump.lua)
if ($Attract) { $env:HOTD_START = "0" }   # -Attract: no coin, no start: the attract loop and its demos (mame_dump.lua)
if ($Mortal -or $FiniteLives) { $env:HOTD_INFLIVES = "0" }   # -Mortal / -FiniteLives: lives run out (game over); with -Shoot 0 the player never fires
if ($Shoot) { $env:HOTD_SHOOT = $Shoot }   # mame_dump.lua HOTD_SHOOT: 0 none, 1 sweep, 3 auto-aim
if ($Chapter -eq 0) { $Chapter = $(if ($Name -match "_ch(\d)_") { [int]$Matches[1] } else { 1 }) }
if ($Chapter -gt 1) { $env:HOTD_START_STAGE = [string]($Chapter - 1); $env:HOTD_STAGE_HOLD = "8" } else { $env:HOTD_START_STAGE = "" }
$env:HOTD_EVERY = "1000000"; $env:HOTD_DUMP_FROM = "1000000"; $env:HOTD_TEXRAM = "0"; $env:HOTD_BUFRAM = "0"; $env:HOTD_FIFO_AT = ""
$script = Join-Path $PSScriptRoot "mame_replay_capture.lua"
$mameArgs = @("hotd", "-rompath", "`"$private`"", "-autoboot_script", "`"$script`"", "-nvram_directory", "`"$Out\nvram`"",
              "-skip_gameinfo", "-keyboardprovider", "none", "-mouseprovider", "none", "-lightgunprovider", "none", "-joystickprovider", "none", "-video", "none", "-sound", "none", "-nothrottle", "-debug", "-debugger", "none",
              "-str", [string][int]($Seconds + 30))
if ($Name) { $mameArgs += @("-input_directory", "`"$play\inp`"", "-playback", "$Name.inp", "-exit_after_playback") }
. (Join-Path $PSScriptRoot "rec_header.ps1")
$env:HOTD_REC_BASE = $Base
Write-RecHeader $Out (Join-Path $mame "mame.exe") $mameArgs $script
$p = Start-Process -FilePath (Join-Path $mame "mame.exe") -WorkingDirectory $mame -ArgumentList $mameArgs -PassThru -NoNewWindow `
                   -RedirectStandardOutput "$Out\mame.log" -RedirectStandardError "$Out\mame_err.log"
if (-not $p.WaitForExit([int]($TimeoutMinutes * 60000))) { Stop-Process -Id $p.Id -Force; "TIMEOUT: MAME was stopped." }
Get-Content "$Out\mame.log", "$Out\mame_err.log" -ErrorAction SilentlyContinue | Where-Object { $_ -match "error|probe|capture|Average" }
