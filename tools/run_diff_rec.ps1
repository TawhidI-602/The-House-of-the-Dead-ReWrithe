# run_diff_rec.ps1 - the arcade side of the whole-game differential (tools\mame_diff_rec.lua, docs/proof.md): replays a
# recorded session (.inp from play_hotd.ps1) headless as run_state_rec.ps1 -Name does and writes, from a start frame on,
# the work RAM snapshot, the per-frame state record and the per-frame external writes (inputs.bin) the runtime's
# --diff-replay needs. Compare with tools\diff_replay.ps1.
#
#   .\run_diff_rec.ps1 -Out <folder inside private> -Name <recording> [-Chapter 1-4 | -Start <frame>] [-Seconds 1900]
#                      [-Frames n] [-FullEvery 600] [-Root <main checkout>] [-TimeoutMinutes 90]
#   Without -Name: a fresh game played by mame_dump.lua's auto-aim (HOTD_SHOOT 3, inputs from MAME's Lua only), started
#   in chapter -StartChapter (1-4); -TwoPlayers: player 2 joins and stands there (HOTD_START2, its lives kept);
#   -AimSkip "73010": routines the auto-aim never shoots (73010: the Magician's fireballs, so that they hit)
param(
    [Parameter(Mandatory)] [string]$Out, [string]$Name = "", [int]$StartChapter = 1, [switch]$TwoPlayers, [string]$AimSkip = "",
    [int]$Chapter = 0, [int]$Start = -1, [int]$Delay = 30, [double]$Seconds = 1900, [int]$Frames = -1,
    [int]$FullEvery = 600, [string]$Root = "", [double]$TimeoutMinutes = 90
)
if (-not $Root) { $Root = Split-Path $PSScriptRoot }
$private = Join-Path $Root "private"
$mame = Join-Path $private "mame0289"
$play = Join-Path $private "mame_play"
$bf = Join-Path $play "inp\$Name.base.txt"
$Base = $(if (Test-Path $bf) { (Get-Content $bf -TotalCount 1).Trim() } else { "nvram_base_us" })
New-Item -ItemType Directory -Force "$Out\nvram" | Out-Null
$Out = (Resolve-Path $Out).Path
Copy-Item -Recurse -Force "$play\$Base\*" "$Out\nvram\"
$env:HOTD_DIFF_DIR = $Out; $env:HOTD_DUMP_DIR = $Out
$env:HOTD_DIFF_CHAPTER = $(if ($Chapter -gt 0) { [string]($Chapter - 1) } else { "" })
$env:HOTD_DIFF_START = $(if ($Start -ge 0) { [string]$Start } else { "" })
$env:HOTD_DIFF_DELAY = "$Delay"; $env:HOTD_DIFF_FULL_EVERY = "$FullEvery"
$env:HOTD_DIFF_FRAMES = $(if ($Frames -ge 0) { [string]$Frames } else { "" })
# the same session settings as run_state_rec.ps1 -Name / replay_hotd.ps1
$env:HOTD_SECONDS = "$Seconds"
$env:HOTD_START = "20"; $env:HOTD_COINS = $(if ($Base -eq "nvram_base") { "2" } else { "0" }); $env:HOTD_INFLIVES = "1"; $env:HOTD_INFAMMO = "1"; $env:HOTD_SHOOT = "0"
$env:HOTD_START_STAGE = ""
$env:HOTD_START2 = $(if ($TwoPlayers) { "1" } else { "0" }); $env:HOTD_AIM_SKIP = $AimSkip
if (-not $Name) {   # a fresh game (as run_capture.ps1 without -Name)
    $env:HOTD_START = "40"; $env:HOTD_COINS = "2"; $env:HOTD_SHOOT = "3"; $env:HOTD_INFAMMO = "0"
    if ($StartChapter -gt 1) { $env:HOTD_START_STAGE = [string]($StartChapter - 1); $env:HOTD_STAGE_HOLD = "8" }
}
$env:HOTD_EVERY = "1000000"; $env:HOTD_DUMP_FROM = "1000000"; $env:HOTD_TEXRAM = "0"; $env:HOTD_BUFRAM = "0"; $env:HOTD_FIFO_AT = ""
$script = Join-Path $PSScriptRoot "mame_diff_rec.lua"
"starting state: $Base"
$mameArgs = @("hotd", "-rompath", "`"$private`"", "-autoboot_script", "`"$script`"", "-nvram_directory", "`"$Out\nvram`"",
              "-skip_gameinfo", "-keyboardprovider", "none", "-mouseprovider", "none", "-lightgunprovider", "none", "-joystickprovider", "none",
              "-video", "none", "-sound", "none", "-nothrottle", "-str", [string][int]($Seconds + 30), "-snapshot_directory", "`"$Out\snap`"")
if ($Name) { $mameArgs += @("-input_directory", "`"$play\inp`"", "-playback", "$Name.inp", "-exit_after_playback") }
. (Join-Path $PSScriptRoot "rec_header.ps1")
$env:HOTD_REC_BASE = $Base
Write-RecHeader $Out (Join-Path $mame "mame.exe") $mameArgs $script
$t0 = Get-Date
$p = Start-Process -FilePath (Join-Path $mame "mame.exe") -WorkingDirectory $mame -ArgumentList $mameArgs -PassThru -NoNewWindow `
                   -RedirectStandardOutput "$Out\mame.log" -RedirectStandardError "$Out\mame_err.log"
if (-not $p.WaitForExit([int]($TimeoutMinutes * 60000))) { Stop-Process -Id $p.Id -Force; "TIMEOUT: MAME was stopped." }
"wall time: $([int]((Get-Date) - $t0).TotalSeconds) s"
Get-Content "$Out\diff_rec.log", "$Out\mame.log", "$Out\mame_err.log" -ErrorAction SilentlyContinue | Where-Object { $_ -match "error|diff_rec|start|end|stopped|playback|Playback" }
