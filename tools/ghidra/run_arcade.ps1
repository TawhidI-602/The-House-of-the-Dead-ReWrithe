# Imports the arcade program (maincpu.bin, i960) into the Ghidra project with our i960 module, adds the memory map,
# finds the functions and exports every function's decompiled C: private\decomp\arcade\maincpu.c + maincpu_index.tsv.
# Run install_specs.ps1 first.
param([switch]$Reexport)
$ErrorActionPreference = "Stop"
$repo = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$env:JAVA_HOME = Join-Path $repo "private\tools\jdk-21.0.12.1+1"
$headless = Join-Path $repo "private\tools\ghidra_12.1.4_PUBLIC\support\analyzeHeadless.bat"
$proj = Join-Path $repo "private\ghidra"
$regions = Join-Path $repo "private\arcade_exports\regions"
$out = Join-Path $repo "private\decomp\arcade"
New-Item -ItemType Directory -Force $proj, $out | Out-Null
$export = @("-postScript", "ExportDecompiled.java", (Join-Path $out "maincpu"))
if ($Reexport) {
    & $headless $proj hotd_arcade -process "maincpu.bin" -noanalysis -preScript FixNoReturn.java -preScript FixSplits.java (Join-Path $out "splits") -preScript ApplyNames.java arcade -preScript ApplyTypes.java arcade -scriptPath $PSScriptRoot @export
} else {
    & $headless $proj hotd_arcade -import (Join-Path $regions "maincpu.bin") -overwrite -loader BinaryLoader `
        -loader-baseAddr 0x0 -processor "i960:LE:32:default" -cspec default -scriptPath $PSScriptRoot `
        -preScript PreAnalysisArcade.java $regions -postScript FindFunctionsArcade.java -postScript FixNoReturn.java -postScript FixSplits.java (Join-Path $out "splits") -postScript ApplyNames.java arcade -postScript ApplyTypes.java arcade @export
}
