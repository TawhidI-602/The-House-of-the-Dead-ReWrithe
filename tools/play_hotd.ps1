# play_hotd.ps1 - play House of the Dead (arcade) in MAME with the mouse as light gun, infinite lives and ammo, automatic start
# (free play), and record the inputs so they can be replayed later for dumps (replay_hotd.ps1).
#
# Started by play_hotd.bat (double-click). What happens:
#  - MAME opens fullscreen. It first shows a red "THIS SYSTEM DOESN'T WORK" warning: press any key (e.g. Space) once.
#  - About 20 seconds later the script presses Start for you (free play; US region, English text, red blood). Then just play:
#    the Windows mouse cursor is the gun, LEFT click = shoot. Lives and bullets never run out (no reloading needed).
#  - Press Esc to quit when you are done. The recording is saved when MAME closes.
# Safety limits: MAME stops by itself after 60 emulated minutes (-str); this script stops MAME after 70 real minutes.
# Every session starts from the same saved MAME settings (NVRAM, -Base), which is needed for an exact replay later.
# (This MAME build has no -offscreen_reload, so there's no right-click reload; infinite ammo replaces it.)
param(
    [int]$Chapter = 1,                   # 1-4: start the game directly in this chapter (2-4 via the stage byte, see mame_dump.lua)
    [string]$Name = "",
    [string]$Base = "nvram_base_us",   # US region, red blood, free play; "nvram_base" = the original Japanese settings
    [int]$Minutes = 60,
    [string]$Resolution = "1920x1440"   # a 4:3 mode the monitor offers (checked: 1920x1440, 1600x1200, 1440x1080, ...)
)
if (-not $Name) { $Name = "play_us_" + $(if ($Chapter -gt 1) { "ch$Chapter`_" } else { "" }) + (Get-Date -Format "yyyyMMdd_HHmm") }
$root = Split-Path $PSScriptRoot
$private = Join-Path $root "private"
$mame = Join-Path $private "mame0289"
$play = Join-Path $private "mame_play"
$session = Join-Path $play $Name
New-Item -ItemType Directory -Force "$play\inp", "$session\nvram" | Out-Null

# Fixed starting state: copy the chosen base NVRAM in for every session (the replay must use the same base).
# (PowerShell variable names ignore case: don't name the path "$base", it would overwrite $Base.)
$basePath = Join-Path $play $Base
if (-not (Test-Path $basePath)) { "Missing $basePath"; exit 1 }
Set-Content -Encoding ascii (Join-Path $play "inp\$Name.base.txt") $Base   # the replay needs the same starting state
Copy-Item -Recurse -Force "$basePath\*" "$session\nvram\"

# The same Lua script and settings are used for playback, so the game sees identical script actions.
$env:HOTD_DUMP_DIR = $session; $env:HOTD_START = "20"; $env:HOTD_COINS = $(if ($Base -eq "nvram_base") { "2" } else { "0" }); $env:HOTD_INFLIVES = "1"; $env:HOTD_INFAMMO = "1"
if ($Chapter -gt 1) { $env:HOTD_START_STAGE = [string]($Chapter - 1); $env:HOTD_STAGE_HOLD = "8" } else { $env:HOTD_START_STAGE = "" }
$env:HOTD_SHOOT = "0"; $env:HOTD_EVERY = "30"; $env:HOTD_DUMP_FROM = "30"; $env:HOTD_SECONDS = [string]($Minutes * 60); $env:HOTD_TEXRAM = "0"
$script = Join-Path $PSScriptRoot "mame_dump.lua"
$mameArgs = @("hotd", "-rompath", "`"$private`"", "-autoboot_script", "`"$script`"",
              "-nvram_directory", "`"$session\nvram`"", "-input_directory", "`"$play\inp`"", "-record", "$Name.inp",
              "-snapshot_directory", "`"$session\snap`"", "-skip_gameinfo",
              # Fullscreen in a 4:3 display mode. The Win32 light gun (Windows cursor) spans the whole monitor, so the monitor must be
              # exactly the game picture: at the desktop's 2560x1440 the 4:3 picture has black bars and shots drift toward the center
              # horizontally. Windows restores the normal display mode when MAME closes.
              "-switchres", "-resolution", $Resolution,
              "-lightgun", "-lightgun_device", "lightgun", "-lightgunprovider", "win32",
              "-str", [string]($Minutes * 60 + 30))
"Starting MAME. Press any key once at the red warning, then wait about 20 seconds for the game to start."
"Aim with the mouse, left click = shoot (no reloading needed), Esc = quit."
$p = Start-Process -FilePath (Join-Path $mame "mame.exe") -WorkingDirectory $mame -ArgumentList $mameArgs -PassThru -NoNewWindow `
                   -RedirectStandardOutput "$session\mame_out.log" -RedirectStandardError "$session\mame_err.log"
$null = $p.Handle   # needed so ExitCode is available after the process ends
if (-not $p.WaitForExit(($Minutes + 10) * 60000)) { Stop-Process -Id $p.Id -Force; "Time limit reached, MAME was stopped." }
$inp = Join-Path $play "inp\$Name.inp"
$size = if (Test-Path $inp) { (Get-Item $inp).Length } else { 0 }
if ($p.ExitCode -ne 0 -or $size -le 64) {
    ""
    "MAME DID NOT FINISH NORMALLY (exit code $($p.ExitCode), recording $size bytes). MAME's error output:"
    Get-Content "$session\mame_err.log" -TotalCount 15 -ErrorAction SilentlyContinue
    ""
    "Full logs: $session"
    exit 1
}
"Recording saved: $inp ($size bytes)"
