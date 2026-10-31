# DISABLED at the user's request (2026-10-08): the resulting calibration made aiming worse. Not wired to any .bat; MAME aiming
# stays as it is, proper mouse aiming is a phase 7 feature of the rewrite. Kept only for reference.
param([switch]$IReallyWantTo)
if (-not $IReallyWantTo) { "calibrate_gun.ps1 is disabled (it made aiming worse)."; exit 1 }
# calibrate_gun.ps1 - measure how the Windows cursor maps to the game's light-gun values in the play setup.
# MAME starts exactly like play_hotd.ps1 (fullscreen, 4:3 mode, Windows cursor as gun), but no game is played or recorded.
# While it runs, this script only READS the cursor position and the left mouse button (no input is sent anywhere).
# The Lua script logs the gun X/Y the game receives at each trigger press. Clicks and presses are paired in order.
# Output: private\calibration\<time>\clicks.txt (cursor), calib_ports.txt (gun values).
$Resolution = "1920x1440"; $Clicks = 9
Add-Type -TypeDefinition @'
using System; using System.Runtime.InteropServices;
public static class Cur {
  [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
  [DllImport("user32.dll")] public static extern bool GetCursorPos(out POINT p);
  [DllImport("user32.dll")] public static extern short GetAsyncKeyState(int vk);
  [DllImport("user32.dll")] public static extern int GetSystemMetrics(int i);
  [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
}
'@
[void][Cur]::SetProcessDPIAware()   # physical pixels, not Windows-scaled ones
$root = Split-Path $PSScriptRoot
$private = Join-Path $root "private"
$mame = Join-Path $private "mame0289"
$out = Join-Path $private ("calibration\" + (Get-Date -Format "yyyyMMdd_HHmmss"))
New-Item -ItemType Directory -Force "$out\nvram" | Out-Null
Copy-Item -Recurse -Force "$private\mame_play\nvram_base_us\*" "$out\nvram\"
$env:HOTD_DUMP_DIR = $out; $env:HOTD_CALIB_CLICKS = "$Clicks"
$a = @("hotd", "-rompath", "`"$private`"", "-autoboot_script", "`"$PSScriptRoot\mame_calib.lua`"", "-nvram_directory", "`"$out\nvram`"",
       "-skip_gameinfo", "-switchres", "-resolution", $Resolution, "-sound", "none",
       "-lightgun", "-lightgun_device", "lightgun", "-lightgunprovider", "win32", "-str", "330")
""
"CALIBRATION: click these $Clicks points exactly on the edge of the GAME PICTURE (not the black bars, if any), in this order:"
"  1 center   2 top-left corner   3 top edge middle   4 top-right corner   5 right edge middle"
"  6 bottom-right corner   7 bottom edge middle   8 bottom-left corner   9 left edge middle"
"At MAME's red warning press SPACE (not the mouse). MAME closes by itself after the 9th click (or press Esc)."
$p = Start-Process -FilePath "$mame\mame.exe" -WorkingDirectory $mame -ArgumentList $a -PassThru -NoNewWindow `
                   -RedirectStandardOutput "$out\mame_out.log" -RedirectStandardError "$out\mame_err.log"
$null = $p.Handle
$log = New-Object System.Collections.Generic.List[string]
$down = $false; $t0 = Get-Date
while (-not $p.HasExited -and ((Get-Date) - $t0).TotalMinutes -lt 6) {
    $isDown = ([Cur]::GetAsyncKeyState(0x01) -band 0x8000) -ne 0
    if ($isDown -and -not $down) {
        $pt = New-Object Cur+POINT; [void][Cur]::GetCursorPos([ref]$pt)
        $log.Add(("{0} {1} {2} {3} {4}" -f ((Get-Date) - $t0).TotalSeconds.ToString("0.000"), $pt.X, $pt.Y, [Cur]::GetSystemMetrics(0), [Cur]::GetSystemMetrics(1)))
    }
    $down = $isDown
    Start-Sleep -Milliseconds 5
}
if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force; "Time limit reached, MAME was stopped." }
$log | Set-Content -Encoding ascii "$out\clicks.txt"
"Clicks logged: $($log.Count). Gun presses logged: $((Get-Content "$out\calib_ports.txt" -ErrorAction SilentlyContinue).Count). Data: $out"

# --- compute the calibration and store it in the game's own settings (GUN SETTING > PLAYER1 GUN ADJUSTMENT, run headless) ---
$res = py "$PSScriptRoot\apply_gun_calibration.py" "$out"
$res | Where-Object { $_ -notmatch "^KEYS=" }
$keys = ($res | Where-Object { $_ -match "^KEYS=" }) -replace "^KEYS=", ""
if (-not $keys) { ""; "Calibration NOT applied (see above). Please run calibrate_gun.bat again."; exit 1 }
$apply = Join-Path $out "apply"
& "$PSScriptRoot\run_testmenu.ps1" -Out $apply -Keys $keys -NvramFrom "$private\mame_play\nvram_base_us" | Out-Null
$shots = [regex]::Matches($keys, "G(\d+):(\d+)") | ForEach-Object { [int]$_.Groups[1].Value, [int]$_.Groups[2].Value }
$e = [IO.File]::ReadAllBytes("$apply\nvram\hotd\eeprom")
$stored = 0..3 | ForEach-Object { [BitConverter]::ToUInt16($e, 0x20 + 2 * $_) }
if (($stored -join ",") -ne ($shots -join ",")) { ""; "Calibration NOT stored as expected (game has $($stored -join ','), wanted $($shots -join ','))."; exit 1 }
$dest = Join-Path $private "mame_play\nvram_base_us_cal"
New-Item -ItemType Directory -Force $dest | Out-Null
Copy-Item -Recurse -Force "$apply\nvram\*" "$dest\"
""
"Calibration stored in the game's settings ($($stored -join ', ')) and saved as the new starting state: $dest"
"play_hotd.bat and play_hotd_chapter.bat use it from now on."
