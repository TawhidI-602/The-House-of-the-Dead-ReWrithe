# replay_hotd.ps1 - replay a recorded session (play_hotd.ps1) headless and dump color tables, work RAM, texture RAM and screenshots.
# Uses the same starting NVRAM and the same Lua settings that affect the game (start time, coins, infinite lives and ammo, no script shooting),
# so the replay follows the recording exactly. Only the dump settings differ, and those only read memory.
#
#   .\replay_hotd.ps1 -Name <recording name without .inp> -Out "<dump folder>" [-Every 2] [-Seconds 3600] [-TexRam] [-TexRamEvery 10] [-BufRam] [-FifoAt 106 -FifoFor 0.1] [-TimeoutMinutes 40] [-Shoot 0] [-Base nvram_base_us]
# -Shoot 1 only for test recordings made with the script shooting: MAME does not record inputs set from Lua, so they must be repeated.
# Output goes to -Out (keep it inside private\).
param(
    [Parameter(Mandatory)] [string]$Name,
    [Parameter(Mandatory)] [string]$Out,
    [int]$Chapter = 0,   # 0 = take it from the recording name ("_ch3_" = Chapter 3), else 1-4; must match the recording
    [string]$Base = "",   # default: from <recording>.base.txt (written by play_hotd.ps1), else nvram_base_us; "nvram_base" = original Japanese settings
    [double]$Every = 2, [double]$DumpFrom = 0, [double]$Seconds = 3600, [switch]$TexRam, [switch]$BufRam, [double]$FifoAt = 0, [double]$FifoFor = 0.1, [double]$TexRamEvery = 0, [double]$TimeoutMinutes = 40, [int]$Shoot = 0
)
$root = Split-Path $PSScriptRoot
$private = Join-Path $root "private"
$mame = Join-Path $private "mame0289"
$play = Join-Path $private "mame_play"
if (-not $Base) {
    $bf = Join-Path $play "inp\$Name.base.txt"
    $Base = $(if (Test-Path $bf) { (Get-Content $bf -TotalCount 1).Trim() } else { "nvram_base_us" })
}
"starting state: $Base"
New-Item -ItemType Directory -Force "$Out\nvram" | Out-Null
Copy-Item -Recurse -Force "$play\$Base\*" "$Out\nvram\"

$env:HOTD_DUMP_DIR = $Out; $env:HOTD_START = "20"; $env:HOTD_COINS = $(if ($Base -eq "nvram_base") { "2" } else { "0" }); $env:HOTD_INFLIVES = "1"; $env:HOTD_INFAMMO = "1"; $env:HOTD_SHOOT = "$Shoot"
if ($Chapter -eq 0) { $Chapter = $(if ($Name -match "_ch(\d)_") { [int]$Matches[1] } else { 1 }) }
if ($Chapter -gt 1) { $env:HOTD_START_STAGE = [string]($Chapter - 1); $env:HOTD_STAGE_HOLD = "8" } else { $env:HOTD_START_STAGE = "" }
$env:HOTD_EVERY = "$Every"; $env:HOTD_DUMP_FROM = $(if ($DumpFrom -gt 0) { "$DumpFrom" } else { "$Every" }); $env:HOTD_SECONDS = "$Seconds"; $env:HOTD_TEXRAM = $(if ($TexRam) { "1" } else { "0" }); $env:HOTD_TEXRAM_EVERY = "$TexRamEvery"; $env:HOTD_BUFRAM = $(if ($BufRam) { "1" } else { "0" }); $env:HOTD_FIFO_AT = $(if ($FifoAt -gt 0) { "$FifoAt" } else { "" }); $env:HOTD_FIFO_FOR = "$FifoFor"
$script = Join-Path $PSScriptRoot "mame_dump.lua"
$mameArgs = @("hotd", "-rompath", "`"$private`"", "-autoboot_script", "`"$script`"",
              "-nvram_directory", "`"$Out\nvram`"", "-input_directory", "`"$play\inp`"", "-playback", "$Name.inp", "-exit_after_playback",
              "-snapshot_directory", "`"$Out\snap`"", "-skip_gameinfo", "-keyboardprovider", "none", "-mouseprovider", "none", "-lightgunprovider", "none", "-joystickprovider", "none", "-video", "none", "-sound", "none", "-nothrottle",
              "-str", [string][int]($Seconds + 30))
$p = Start-Process -FilePath (Join-Path $mame "mame.exe") -WorkingDirectory $mame -ArgumentList $mameArgs -PassThru -NoNewWindow `
                   -RedirectStandardOutput "$Out\mame.log" -RedirectStandardError "$Out\mame_err.log"
if (-not $p.WaitForExit([int]($TimeoutMinutes * 60000))) { Stop-Process -Id $p.Id -Force; "TIMEOUT: MAME was stopped." }
"dumps: $((Get-ChildItem $Out -Filter *_palette.bin -ErrorAction SilentlyContinue).Count)"
Get-Content "$Out\mame.log", "$Out\mame_err.log" -ErrorAction SilentlyContinue | Where-Object { $_ -match "error|Average|playback|Playback" }
