# run_state_rec.ps1 - record the arcade game state every frame (tools\mame_state_rec.lua) headless, for tools\state_diff.py.
# Same starting state as replay_hotd.ps1 (a copy of an NVRAM base); no inputs by default = the attract loop with its demos
# (seed 0, recorded gun input: the same every run).
# -Name replays a recorded session (.inp from play_hotd.ps1) exactly as replay_hotd.ps1 does: tools\mame_replay_rec.lua
# runs tools\mame_dump.lua (the session's Lua inputs: coin / start at 20 s, infinite lives and ammo, start stage; no dumps)
# with the recorder's taps. The base NVRAM then comes from <recording>.base.txt.
#
#   .\run_state_rec.ps1 -Out "<folder inside private>" [-Seconds 300] [-Modes "3"] [-Name <recording>] [-TimeoutMinutes 30]
param(
    [Parameter(Mandatory)] [string]$Out,
    [double]$Seconds = 300, [string]$Modes = "", [string]$Name = "", [string]$Base = "",
    [int]$From = 0, [int]$Chapter = 0, [double]$TimeoutMinutes = 30
)
$root = Split-Path $PSScriptRoot
$private = Join-Path $root "private"
$mame = Join-Path $private "mame0289"
$play = Join-Path $private "mame_play"
if (-not $Base) {
    $bf = Join-Path $play "inp\$Name.base.txt"
    $Base = $(if ($Name -and (Test-Path $bf)) { (Get-Content $bf -TotalCount 1).Trim() } else { "nvram_base_us" })
}
New-Item -ItemType Directory -Force "$Out\nvram" | Out-Null
$Out = (Resolve-Path $Out).Path   # MAME runs in its own folder
Copy-Item -Recurse -Force "$play\$Base\*" "$Out\nvram\"
$env:HOTD_REC_FILE = Join-Path $Out "state.rec"; $env:HOTD_SECONDS = "$Seconds"; $env:HOTD_REC_MODES = $Modes; $env:HOTD_REC_FROM = "$From"
if ($Name) {
    # the same settings as replay_hotd.ps1 / play_hotd.ps1 (only the dump settings differ: no dumps)
    $env:HOTD_REC_TAPS_ONLY = "1"; $env:HOTD_DUMP_DIR = $Out
    $env:HOTD_START = "20"; $env:HOTD_COINS = $(if ($Base -eq "nvram_base") { "2" } else { "0" }); $env:HOTD_INFLIVES = "1"; $env:HOTD_INFAMMO = "1"; $env:HOTD_SHOOT = "0"
    if ($Chapter -eq 0) { $Chapter = $(if ($Name -match "_ch(\d)_") { [int]$Matches[1] } else { 1 }) }
    if ($Chapter -gt 1) { $env:HOTD_START_STAGE = [string]($Chapter - 1); $env:HOTD_STAGE_HOLD = "8" } else { $env:HOTD_START_STAGE = "" }
    $env:HOTD_EVERY = "1000000"; $env:HOTD_DUMP_FROM = "1000000"; $env:HOTD_TEXRAM = "0"; $env:HOTD_BUFRAM = "0"; $env:HOTD_FIFO_AT = ""
    $script = Join-Path $PSScriptRoot "mame_replay_rec.lua"
} else {
    $env:HOTD_REC_TAPS_ONLY = ""; $env:HOTD_START = "0"; $env:HOTD_COINS = "0"
    $script = Join-Path $PSScriptRoot "mame_state_rec.lua"
}
"starting state: $Base"
$mameArgs = @("hotd", "-rompath", "`"$private`"", "-autoboot_script", "`"$script`"", "-nvram_directory", "`"$Out\nvram`"",
              "-skip_gameinfo", "-keyboardprovider", "none", "-mouseprovider", "none", "-lightgunprovider", "none", "-joystickprovider", "none", "-video", "none", "-sound", "none", "-nothrottle", "-str", [string][int]($Seconds + 30))
if ($Name) { $mameArgs += @("-input_directory", "`"$play\inp`"", "-playback", "$Name.inp", "-exit_after_playback", "-snapshot_directory", "`"$Out\snap`"") }
. (Join-Path $PSScriptRoot "rec_header.ps1")
$env:HOTD_REC_BASE = $Base
Write-RecHeader $Out (Join-Path $mame "mame.exe") $mameArgs $script
$t0 = Get-Date
$p = Start-Process -FilePath (Join-Path $mame "mame.exe") -WorkingDirectory $mame -ArgumentList $mameArgs -PassThru -NoNewWindow `
                   -RedirectStandardOutput "$Out\mame.log" -RedirectStandardError "$Out\mame_err.log"
if (-not $p.WaitForExit([int]($TimeoutMinutes * 60000))) { Stop-Process -Id $p.Id -Force; "TIMEOUT: MAME was stopped." }
"wall time: $([int]((Get-Date) - $t0).TotalSeconds) s, state.rec: $([int]((Get-Item $env:HOTD_REC_FILE -ErrorAction SilentlyContinue).Length / 1MB)) MB"
Get-Content "$Out\mame.log", "$Out\mame_err.log" -ErrorAction SilentlyContinue | Where-Object { $_ -match "error|state_rec|Average|playback|Playback" }
