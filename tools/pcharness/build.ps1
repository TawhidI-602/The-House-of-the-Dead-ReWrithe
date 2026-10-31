# build.ps1 - builds the PC harness hook (a proxy dinput.dll, 32-bit) with the VS Build Tools into private\pcharness\bin.
$root = Split-Path (Split-Path $PSScriptRoot)
$out = Join-Path $root "private\pcharness\bin"
New-Item -ItemType Directory -Force $out | Out-Null
$vc = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat"
$src = Join-Path $PSScriptRoot "hotd_hook.c"
$def = Join-Path $PSScriptRoot "dinput.def"
cmd /c "`"$vc`" >nul 2>nul && cd /d `"$out`" && cl /nologo /O2 /W3 /LD /MT `"$src`" /link /DEF:`"$def`" /OUT:dinput.dll user32.lib"
