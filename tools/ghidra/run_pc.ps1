# Imports THOTD.EXE.orig (the unpatched executable) into a Ghidra project with the Watcom convention, analyses it and
# exports every function's decompiled C: private\decomp\pc\thotd.c + thotd_index.tsv. Run install_specs.ps1 first.
param([string]$Exe = "C:\Games\HOTD\THOTD.EXE.orig", [switch]$Reexport)
$ErrorActionPreference = "Stop"
$repo = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$env:JAVA_HOME = Join-Path $repo "private\tools\jdk-21.0.12.1+1"
$headless = Join-Path $repo "private\tools\ghidra_12.1.4_PUBLIC\support\analyzeHeadless.bat"
$proj = Join-Path $repo "private\ghidra"
$out = Join-Path $repo "private\decomp\pc"
New-Item -ItemType Directory -Force $proj, $out | Out-Null
$common = @("-scriptPath", $PSScriptRoot, "-postScript", "ExportDecompiled.java", (Join-Path $out "thotd"))
if ($Reexport) {
    # existing project: only the export (plus any scripts given in -Pre, e.g. renaming passes)
    & $headless $proj hotd_pc -process "THOTD.EXE.orig" -noanalysis -preScript FixNoReturn.java -preScript FixSplits.java (Join-Path $out "splits") -preScript ApplyNames.java pc -preScript ApplyTypes.java pc @common
} else {
    & $headless $proj hotd_pc -import $Exe -overwrite -processor "x86:LE:32:default" -cspec watcom `
        -preScript PreAnalysisPc.java -postScript FindFunctionsPc.java -postScript FixNoReturn.java -postScript FixSplits.java (Join-Path $out "splits") -postScript ApplyNames.java pc -postScript ApplyTypes.java pc @common
}
