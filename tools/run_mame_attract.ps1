# run_mame_attract.ps1 - the attract loop in MAME (tools\mame_attract.lua), headless, on a COPY of the US NVRAM base.
#   .\run_mame_attract.ps1 -Out "<folder inside private\>" [-Seconds 240] [-Every 1]
# Limits: -str (Seconds + 30) and a wall-clock timeout of 8 minutes.
param([Parameter(Mandatory)] [string]$Out, [double]$Seconds = 240, [double]$Every = 1)
$root = Split-Path $PSScriptRoot
$private = Join-Path $root "private"
if (-not (Test-Path (Join-Path $private "mame0289"))) { $private = "C:\Games\HOTD M2 RECOMPILE\M2 Source\private" }
$mame = Join-Path $private "mame0289"
New-Item -ItemType Directory -Force "$Out\nvram" | Out-Null
Copy-Item -Recurse -Force "$private\mame_play\nvram_base_us\*" "$Out\nvram\"
$env:HOTD_DUMP_DIR = $Out; $env:HOTD_SECONDS = "$Seconds"; $env:HOTD_EVERY = "$Every"
$a = @("hotd", "-rompath", "`"$private`"", "-autoboot_script", "`"$PSScriptRoot\mame_attract.lua`"", "-nvram_directory", "`"$Out\nvram`"",
       "-snapshot_directory", "`"$Out\snap`"", "-skip_gameinfo", "-keyboardprovider", "none", "-mouseprovider", "none", "-lightgunprovider", "none", "-joystickprovider", "none", "-video", "none", "-sound", "none", "-nothrottle", "-str", [string][int]($Seconds + 30))
$p = Start-Process -FilePath "$mame\mame.exe" -WorkingDirectory $mame -ArgumentList $a -PassThru -NoNewWindow `
                   -RedirectStandardOutput "$Out\mame.log" -RedirectStandardError "$Out\mame_err.log"
$null = $p.Handle
if (-not $p.WaitForExit(480000)) { Stop-Process -Id $p.Id -Force; "TIMEOUT" }
"exit $($p.ExitCode); screenshots: $((Get-ChildItem "$Out\snap" -Recurse -Filter *.png -ErrorAction SilentlyContinue).Count)"
