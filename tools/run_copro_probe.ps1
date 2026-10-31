# run_copro_probe.ps1 - log geometry-coprocessor calls in the attract loop (tools\mame_copro_probe.lua), headless;
# -Name <recording> replays a recorded .inp session instead (tools\mame_replay_probe.lua, as run_state_rec.ps1 -Name).
#   .\run_copro_probe.ps1 -Out "<folder inside private>" -Funcs "0c,0f,1a" | all [-Seconds 330] [-Max 3000] [-Total n] [-Start s] [-TimeoutMinutes 25]
# Output: <Out>\copro.txt (one line per call), for tools\tgp_model.py.
param([Parameter(Mandatory)] [string]$Out, [string]$Funcs = "1f", [double]$Seconds = 330, [int]$Max = 3000, [int]$Total = 0, [double]$Start = 0, [string]$Name = "", [int]$Chapter = 0,
      [string]$Base = "nvram_base_us", [double]$TimeoutMinutes = 25)
$root = Split-Path $PSScriptRoot
$private = Join-Path $root "private"
$play = Join-Path $private "mame_play"
if ($Name -and $Base -eq "nvram_base_us") {
    $bf = Join-Path $play "inp\$Name.base.txt"
    if (Test-Path $bf) { $Base = (Get-Content $bf -TotalCount 1).Trim() }
}
$mame = Join-Path $private "mame0289"
New-Item -ItemType Directory -Force "$Out\nvram" | Out-Null
$Out = (Resolve-Path $Out).Path
Copy-Item -Recurse -Force (Join-Path $private "mame_play\$Base\*") "$Out\nvram\"
$env:HOTD_PROBE_FILE = "$Out\copro.txt"; $env:HOTD_SECONDS = "$Seconds"; $env:HOTD_PROBE_FUNCS = $Funcs; $env:HOTD_PROBE_MAX = "$Max"; $env:HOTD_PROBE_TOTAL = "$Total"; $env:HOTD_PROBE_START = "$Start"
$script = Join-Path $PSScriptRoot "mame_copro_probe.lua"
if ($Name) {
    # replay a recorded session with the same settings as replay_hotd.ps1 / run_state_rec.ps1 -Name (no dumps)
    $env:HOTD_DUMP_DIR = $Out
    $env:HOTD_START = "20"; $env:HOTD_COINS = $(if ($Base -eq "nvram_base") { "2" } else { "0" }); $env:HOTD_INFLIVES = "1"; $env:HOTD_INFAMMO = "1"; $env:HOTD_SHOOT = "0"
    if ($Chapter -eq 0) { $Chapter = $(if ($Name -match "_ch(\d)_") { [int]$Matches[1] } else { 1 }) }
    if ($Chapter -gt 1) { $env:HOTD_START_STAGE = [string]($Chapter - 1); $env:HOTD_STAGE_HOLD = "8" } else { $env:HOTD_START_STAGE = "" }
    $env:HOTD_EVERY = "1000000"; $env:HOTD_DUMP_FROM = "1000000"; $env:HOTD_TEXRAM = "0"; $env:HOTD_BUFRAM = "0"; $env:HOTD_FIFO_AT = ""
    $script = Join-Path $PSScriptRoot "mame_replay_probe.lua"
}
$mameArgs = @("hotd", "-rompath", "`"$private`"", "-autoboot_script", "`"$script`"", "-nvram_directory", "`"$Out\nvram`"",
              "-skip_gameinfo", "-keyboardprovider", "none", "-mouseprovider", "none", "-lightgunprovider", "none", "-joystickprovider", "none", "-video", "none", "-sound", "none", "-nothrottle", "-str", [string][int]($Seconds + 30))
if ($Name) { $mameArgs += @("-input_directory", "`"$play\inp`"", "-playback", "$Name.inp", "-exit_after_playback") }
. (Join-Path $PSScriptRoot "rec_header.ps1")
$env:HOTD_REC_BASE = $Base
Write-RecHeader $Out (Join-Path $mame "mame.exe") $mameArgs $script
$p = Start-Process -FilePath (Join-Path $mame "mame.exe") -WorkingDirectory $mame -ArgumentList $mameArgs -PassThru -NoNewWindow `
                   -RedirectStandardOutput "$Out\mame.log" -RedirectStandardError "$Out\mame_err.log"
if (-not $p.WaitForExit([int]($TimeoutMinutes * 60000))) { Stop-Process -Id $p.Id -Force; "TIMEOUT: MAME was stopped." }
Get-Content "$Out\mame.log", "$Out\mame_err.log" -ErrorAction SilentlyContinue | Where-Object { $_ -match "error|probe|Average" }
