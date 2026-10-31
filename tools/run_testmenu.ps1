# run_testmenu.ps1 - run mame_testmenu.lua headless on a COPY of an NVRAM folder and collect the screenshots.
#   .\run_testmenu.ps1 -Out "<folder>" -Keys "T,S,S,T" [-NvramFrom "<nvram folder>"]
# The copy (with whatever the menu changed) ends up in <Out>\nvram. Time limits: -str 120 and 3 minutes wall clock.
param([Parameter(Mandatory)] [string]$Out, [Parameter(Mandatory)] [string]$Keys, [string]$NvramFrom = "")
$root = Split-Path $PSScriptRoot
$private = Join-Path $root "private"
$mame = Join-Path $private "mame0289"
if (-not $NvramFrom) { $NvramFrom = Join-Path $private "mame_play\nvram_base" }
New-Item -ItemType Directory -Force "$Out\nvram" | Out-Null
Copy-Item -Recurse -Force "$NvramFrom\*" "$Out\nvram\"
$env:HOTD_DUMP_DIR = $Out; $env:HOTD_KEYS = $Keys
$a = @("hotd", "-rompath", "`"$private`"", "-autoboot_script", "`"$PSScriptRoot\mame_testmenu.lua`"", "-nvram_directory", "`"$Out\nvram`"",
       "-snapshot_directory", "`"$Out\snap`"", "-skip_gameinfo", "-keyboardprovider", "none", "-mouseprovider", "none", "-lightgunprovider", "none", "-joystickprovider", "none", "-video", "none", "-sound", "none", "-nothrottle", "-str", "120")
$p = Start-Process -FilePath "$mame\mame.exe" -WorkingDirectory $mame -ArgumentList $a -PassThru -NoNewWindow `
                   -RedirectStandardOutput "$Out\mame.log" -RedirectStandardError "$Out\mame_err.log"
$null = $p.Handle
if (-not $p.WaitForExit(180000)) { Stop-Process -Id $p.Id -Force; "TIMEOUT" }
"exit $($p.ExitCode); screenshots: $((Get-ChildItem "$Out\snap" -Recurse -Filter *.png -ErrorAction SilentlyContinue).Count)"
