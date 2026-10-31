# run_mame_title_code.ps1 - the title screen's gun-trigger codes in MAME (tools\mame_title_code.lua), headless, on a
# COPY of the US NVRAM base.
#   .\run_mame_title_code.ps1 -Out "<folder>" [-Keys "C,C,L,L,L,R,R,S"] [-Seconds 60]   (Seconds: the play after the keys)
# Limits: -str (Seconds + 120) and a wall-clock timeout of 8 minutes.
param([Parameter(Mandatory)] [string]$Out, [string]$Keys = "C,C,L,L,L,R,R,S", [double]$Seconds = 60)
$root = Split-Path $PSScriptRoot
$private = Join-Path $root "private"
if (-not (Test-Path (Join-Path $private "mame0289"))) { $private = "C:\Games\HOTD M2 RECOMPILE\M2 Source\private" }
$mame = Join-Path $private "mame0289"
New-Item -ItemType Directory -Force "$Out\nvram" | Out-Null
Copy-Item -Recurse -Force "$private\mame_play\nvram_base_us\*" "$Out\nvram\"
$env:HOTD_DUMP_DIR = $Out; $env:HOTD_KEYS = $Keys; $env:HOTD_AFTER = "$Seconds"
$a = @("hotd", "-rompath", "`"$private`"", "-autoboot_script", "`"$PSScriptRoot\mame_title_code.lua`"", "-nvram_directory", "`"$Out\nvram`"",
       "-snapshot_directory", "`"$Out\snap`"", "-skip_gameinfo", "-keyboardprovider", "none", "-mouseprovider", "none", "-lightgunprovider", "none", "-joystickprovider", "none", "-video", "none", "-sound", "none", "-nothrottle", "-str", [string][int]($Seconds + 120))
$p = Start-Process -FilePath "$mame\mame.exe" -WorkingDirectory $mame -ArgumentList $a -PassThru -NoNewWindow `
                   -RedirectStandardOutput "$Out\mame.log" -RedirectStandardError "$Out\mame_err.log"
$null = $p.Handle
if (-not $p.WaitForExit(480000)) { Stop-Process -Id $p.Id -Force; "TIMEOUT" }
"exit $($p.ExitCode); screenshots: $((Get-ChildItem "$Out\snap" -Recurse -Filter *.png -ErrorAction SilentlyContinue).Count)"
