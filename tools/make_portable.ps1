# Builds the viewer in Release and copies everything it needs into one self-contained folder (for a flash drive).
# The originals are only read, never moved or changed.
#
#   powershell -ExecutionPolicy Bypass -File tools\make_portable.ps1 [-Out <folder>] [-Pc <PC install>]
#
# Default output: <repo>\private\portable\HOTD Viewer (inside private\, so it is never committed).
# The folder contains game data: personal use only, never share it.
param(
    [string]$Out = "",
    [string]$Pc = "C:\Games\HOTD"
)
$ErrorActionPreference = "Stop"
$repo = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
if ($Out -eq "") { $Out = Join-Path $repo "private\portable\HOTD Viewer" }
$priv = Join-Path $repo "private"

# 1. Release build
$cmake = (Get-Command cmake -ErrorAction SilentlyContinue).Source
if (-not $cmake) { $cmake = "C:\Program Files\CMake\bin\cmake.exe" }
& $cmake --build (Join-Path $repo "build") --config Release --target hotd_viewer -- /v:minimal /nologo
if ($LASTEXITCODE -ne 0) { throw "build failed" }

if (Test-Path -LiteralPath $Out) { Remove-Item -LiteralPath $Out -Recurse -Force }
New-Item -ItemType Directory -Force $Out | Out-Null

function Copy-One([string]$src, [string]$dstRel) {
    if (-not (Test-Path -LiteralPath $src)) { throw "missing: $src" }
    $dst = Join-Path $Out $dstRel
    New-Item -ItemType Directory -Force (Split-Path $dst) | Out-Null
    Copy-Item -LiteralPath $src -Destination $dst
}

# 2. Program: exe, SDL3.dll, MSVC runtime (the shaders are built into the exe)
$rel = Join-Path $repo "build\Release"
Copy-One (Join-Path $rel "hotd_viewer.exe") "bin\hotd_viewer.exe"
Copy-One (Join-Path $rel "SDL3.dll") "bin\SDL3.dll"
$vs = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Redist\MSVC"
$crt = Get-ChildItem -LiteralPath $vs -Directory | Sort-Object Name -Descending |
    ForEach-Object { Join-Path $_.FullName "x64\Microsoft.VC143.CRT" } | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if (-not $crt) { throw "MSVC runtime (VC143.CRT) not found under $vs" }
foreach ($d in "msvcp140.dll", "msvcp140_1.dll", "msvcp140_2.dll", "vcruntime140.dll", "vcruntime140_1.dll") {
    Copy-One (Join-Path $crt $d) "bin\$d"
}

# 3. PC game data the viewer reads (as installed, i.e. the patched files)
$pcFiles = @("ART\STAGE1\WORLD\WORLD1.RBH", "ART\STAGE2\WORLD\WORLD2.RBH", "ART\STAGE3\WORLD\WORLD3.RBH",
             "ART\STAGE4\WORLD\WORLD4.RBH", "ART\STAGE1\CREATURE\CREAT1.RBH", "ART\STAGE1\CREATURE\CREAT2.RBH",
             "ART\STAGE1\CREATURE\BIGGER.VMC", "EVT\ALLSTAGE\EVTALL.RBH", "CAM\ALLSTAGE\CAMALL.RBH")
foreach ($f in $pcFiles) { Copy-One (Join-Path $Pc $f) "pc\$f" }

# 4. Arcade: the dump zip, the extracted regions the viewer reads, the derived files
Copy-One (Join-Path $priv "hotd.zip") "private\hotd.zip"
foreach ($r in "maincpu.bin", "main_data.bin", "polygons.bin", "textures.bin") {
    Copy-One (Join-Path $priv "arcade_exports\regions\$r") "private\arcade_exports\regions\$r"
}
foreach ($f in "arcade_texsets.txt", "creature_poses.txt", "arcade_states.txt", "arcade_props.txt") {
    Copy-One (Join-Path $priv $f) "private\$f"
}

# 5. MAME dump files: only the color dumps the texture-set file names ("sets" lines: rom:<area>/<tag>)
$dumps = Join-Path $priv "mame_dumps\fullgame_bufram"
Copy-One (Join-Path $dumps "index.txt") "private\mame_dumps\fullgame_bufram\index.txt"
$tags = @()
foreach ($line in Get-Content -LiteralPath (Join-Path $priv "arcade_texsets.txt")) {
    if ($line -match '^sets\s') {
        foreach ($t in ($line -split '\s+' | Select-Object -Skip 2)) {
            foreach ($part in $t -split '/') { if ($part -notlike 'rom:*' -and $part -ne '') { $tags += $part } }
        }
    }
}
$texsetsAllRom = $true
foreach ($line in Get-Content -LiteralPath (Join-Path $priv "arcade_texsets.txt")) {
    if ($line -match '^sets\s') {
        foreach ($t in ($line -split '\s+' | Select-Object -Skip 2)) { if ($t.Split('/')[0] -notlike 'rom:*') { $texsetsAllRom = $false } }
    }
}
foreach ($tag in ($tags | Sort-Object -Unique)) {
    foreach ($suffix in "_palette.bin", "_colorxlat.bin") {
        Copy-One (Join-Path $dumps "$tag$suffix") "private\mame_dumps\fullgame_bufram\$tag$suffix"
    }
    if (-not $texsetsAllRom) { Copy-One (Join-Path $dumps "${tag}_texram.bin") "private\mame_dumps\fullgame_bufram\${tag}_texram.bin" }
}

# 6. Launcher and README
@'
@echo off
rem Starts the portable HOTD viewer. Only paths relative to this file: works from any drive letter.
rem Extra arguments are passed on, e.g.  "Run Viewer.bat" --chapter 2 --source pc
setlocal
cd /d "%~dp0"
start "" "%~dp0bin\hotd_viewer.exe" --root "%~dp0." --pc "%~dp0pc" %*
'@ | Set-Content -LiteralPath (Join-Path $Out "Run Viewer.bat") -Encoding ascii

@'
HOTD Viewer, portable copy
==========================

PERSONAL USE ONLY. This folder contains game data from the owner's own copies of
The House of the Dead (the 1998 PC port and the arcade ROM dump). Do not share or upload it.

How to run
----------
Double-click "Run Viewer.bat". It uses only paths relative to its own folder, so the folder
can sit on any drive or path. No install needed: Windows 10/11, 64-bit, OpenGL 3.3.
Press H in the viewer for the controls. F12 screenshots go to private\screenshots\ here.

What's in it
------------
bin\        hotd_viewer.exe, SDL3.dll and the Microsoft C++ runtime DLLs
pc\         the PC game files the viewer reads (worlds, creatures, textures, event scripts, camera rails)
private\    hotd.zip (arcade ROM dump), the ROM regions extracted from it (arcade_exports\regions),
            files derived from MAME replays (arcade_texsets.txt, creature_poses.txt, arcade_states.txt,
            arcade_props.txt) and the few MAME dump color files they refer to (mame_dumps\fullgame_bufram)

Made by tools\make_portable.ps1 in the rewrite's repository; rerun it to refresh this copy.
'@ | Set-Content -LiteralPath (Join-Path $Out "README.txt") -Encoding ascii

$bytes = (Get-ChildItem -LiteralPath $Out -Recurse -File | Measure-Object Length -Sum).Sum
$files = (Get-ChildItem -LiteralPath $Out -Recurse -File).Count
"Portable folder: $Out"
"{0} files, {1:N1} MB" -f $files, ($bytes / 1MB)
