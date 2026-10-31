# run_mame_dump.ps1 - run House of the Dead in MAME with tools\mame_dump.lua, headless.
# - No window and no Windows key presses: all inputs (coin, start, trigger, gun) come from the Lua script.
# - -video none makes MAME skip its start-up warning ("THIS SYSTEM DOESN'T WORK ... Press any key"), see MAME ui.cpp.
# - Two safety limits so a run can't hang: MAME's -str (quits after Seconds + 30 emulated seconds) and a wall-clock timeout
#   (default 10 minutes) after which MAME is stopped.
#
#   .\run_mame_dump.ps1 -Mame "<folder with mame.exe>" -RomPath "<folder with hotd.zip>" -Out "<dump folder>"
#                       [-Start 40] [-Every 4] [-Seconds 280] [-Shoot 1] [-TimeoutMinutes 10] [-TexRam] [-InfiniteLives] [-Coins 2]
# Output goes to -Out (keep it inside private\). MAME's console output is saved as mame.log there.
param(
    [Parameter(Mandatory)] [string]$Mame,
    [Parameter(Mandatory)] [string]$RomPath,
    [Parameter(Mandatory)] [string]$Out,
    [double]$Start = 40, [double]$Every = 4, [double]$Seconds = 280, [int]$Shoot = 1, [double]$TimeoutMinutes = 10, [switch]$TexRam, [switch]$InfiniteLives, [int]$Coins = 2
)
New-Item -ItemType Directory -Force $Out | Out-Null
$env:HOTD_DUMP_DIR = $Out; $env:HOTD_START = "$Start"; $env:HOTD_EVERY = "$Every"; $env:HOTD_SECONDS = "$Seconds"; $env:HOTD_SHOOT = "$Shoot"; $env:HOTD_TEXRAM = $(if ($TexRam) { "1" } else { "0" }); $env:HOTD_INFLIVES = $(if ($InfiniteLives) { "1" } else { "0" }); $env:HOTD_COINS = "$Coins"
$script = Join-Path $PSScriptRoot "mame_dump.lua"
$mameArgs = @("hotd", "-rompath", "`"$RomPath`"", "-autoboot_script", "`"$script`"", "-snapshot_directory", "`"$Out\snap`"",
          "-skip_gameinfo", "-keyboardprovider", "none", "-mouseprovider", "none", "-lightgunprovider", "none", "-joystickprovider", "none", "-video", "none", "-sound", "none", "-nothrottle", "-str", [string][int]($Seconds + 30))
$p = Start-Process -FilePath (Join-Path $Mame "mame.exe") -WorkingDirectory $Mame -ArgumentList $mameArgs -PassThru -NoNewWindow `
                   -RedirectStandardOutput "$Out\mame.log" -RedirectStandardError "$Out\mame_err.log"
if (-not $p.WaitForExit([int]($TimeoutMinutes * 60000))) {
    Stop-Process -Id $p.Id -Force
    "TIMEOUT: MAME was still running after $TimeoutMinutes minutes and was stopped."
}
"dumps: $((Get-ChildItem $Out -Filter *_palette.bin -ErrorAction SilentlyContinue).Count), snapshots: $((Get-ChildItem "$Out\snap" -Recurse -Filter *.png -ErrorAction SilentlyContinue).Count)"
Get-Content "$Out\mame.log", "$Out\mame_err.log" -ErrorAction SilentlyContinue | Where-Object { $_ -match "error|Average" }
