# Installs this folder's Ghidra additions into the private Ghidra copy (idempotent):
#  - x86watcom.cspec: Watcom register convention as a compiler spec "watcom" for x86:LE:32:default (THOTD.EXE)
#  - i960\: the i960 processor module (SLEIGH), compiled with Ghidra's sleigh tool (arcade program)
param([string]$Ghidra = "")
$ErrorActionPreference = "Stop"
$repo = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
if ($Ghidra -eq "") { $Ghidra = Join-Path $repo "private\tools\ghidra_12.1.4_PUBLIC" }
$env:JAVA_HOME = Join-Path $repo "private\tools\jdk-21.0.12.1+1"

$x86 = Join-Path $Ghidra "Ghidra\Processors\x86\data\languages"
Copy-Item -LiteralPath (Join-Path $PSScriptRoot "x86watcom.cspec") -Destination $x86 -Force
$ldefs = Join-Path $x86 "x86.ldefs"
$text = Get-Content -LiteralPath $ldefs -Raw
if ($text -notmatch 'id="watcom"') {
    # first language block = x86:LE:32:default
    $text = $text -replace '(<compiler name="golang" spec="x86-32-golang.cspec" id="golang"/>)', "`$1`r`n    <compiler name=`"Watcom`" spec=`"x86watcom.cspec`" id=`"watcom`"/>"
    Set-Content -LiteralPath $ldefs -Value $text -NoNewline -Encoding utf8
}
"x86 watcom spec installed"

$src = Join-Path $PSScriptRoot "i960"
if (Test-Path -LiteralPath $src) {
    $dst = Join-Path $Ghidra "Ghidra\Processors\i960"
    if (Test-Path -LiteralPath $dst) { Remove-Item -LiteralPath $dst -Recurse -Force }
    Copy-Item -LiteralPath $src -Destination $dst -Recurse
    & (Join-Path $Ghidra "support\sleigh.bat") (Join-Path $dst "data\languages\i960.slaspec")
    if ($LASTEXITCODE -ne 0) { throw "sleigh failed" }
    "i960 module installed"
}
