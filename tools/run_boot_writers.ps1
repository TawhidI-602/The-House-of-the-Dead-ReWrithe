# run_boot_writers.ps1 - MAME from power-on to Chapter 1's chapter_load with tools\mame_boot_writers.lua (headless):
# who writes each 16 bytes of work RAM, and the work RAM / texture RAM / palette at the chapter load.
# NVRAM: a copy of private\mame_play\<Base> (default nvram_base_us: US region, free play, as the recordings) in
# -Out (MAME's own nvram folder is not touched); -Base "" = a cold EEPROM (the factory settings: Japan).
# -str and a wall-clock timeout bound the run. No Windows key presses: inputs come from the Lua script.
#   .\run_boot_writers.ps1 -Out private\boot_writers [-Mame private\mame0289] [-RomPath private] [-Start 20] [-Coins 0]
param(
    [Parameter(Mandatory)] [string]$Out,
    [string]$Mame = (Join-Path $PSScriptRoot "..\private\mame0289"),
    [string]$RomPath = (Join-Path $PSScriptRoot "..\private"),
    [string]$Base = "nvram_base_us",
    [double]$Start = 20, [int]$Coins = 0, [double]$Seconds = 60, [double]$TimeoutMinutes = 20
)
New-Item -ItemType Directory -Force $Out, "$Out\nvram", "$Out\cfg" | Out-Null
$Out = (Resolve-Path $Out).Path
if ($Base) { Copy-Item -Recurse -Force (Join-Path $RomPath "mame_play\$Base\*") "$Out\nvram\" }
$env:HOTD_DUMP_DIR = $Out; $env:HOTD_START = "$Start"; $env:HOTD_COINS = "$Coins"; $env:HOTD_SECONDS = "$Seconds"
$script = Join-Path $PSScriptRoot "mame_boot_writers.lua"
$mameArgs = @("hotd", "-rompath", "`"$((Resolve-Path $RomPath).Path)`"", "-autoboot_script", "`"$script`"",
              "-nvram_directory", "`"$Out\nvram`"", "-cfg_directory", "`"$Out\cfg`"",
              "-skip_gameinfo", "-keyboardprovider", "none", "-mouseprovider", "none", "-lightgunprovider", "none", "-joystickprovider", "none", "-video", "none", "-sound", "none", "-nothrottle", "-str", [string][int]($Seconds + 30))
$p = Start-Process -FilePath (Join-Path $Mame "mame.exe") -WorkingDirectory $Mame -ArgumentList $mameArgs -PassThru -NoNewWindow `
                   -RedirectStandardOutput "$Out\mame.log" -RedirectStandardError "$Out\mame_err.log"
if (-not $p.WaitForExit([int]($TimeoutMinutes * 60000))) {
    Stop-Process -Id $p.Id -Force
    "TIMEOUT: MAME was still running after $TimeoutMinutes minutes and was stopped."
}
Get-Content "$Out\mame.log", "$Out\mame_err.log" -ErrorAction SilentlyContinue | Select-Object -Last 5
