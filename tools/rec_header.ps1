# rec_header.ps1 - dot-source, then call Write-RecHeader before starting MAME: writes <Out>\recording.txt with the MAME
# version, the full command line, the HOTD_* settings, the Lua scripts' hashes and the date, so later comparisons of two
# recordings can check they were made the same way (user, 2026-10-13: the Oct 7 replay dumps no longer match).
function Write-RecHeader([string]$Out, [string]$MameExe, [string[]]$MameArgs, [string]$Script) {
    $ver = (& $MameExe -version 2>$null | Select-Object -First 1); $global:LASTEXITCODE = 0   # mame -version exits non-zero
    $lines = @("date: $((Get-Date).ToString('yyyy-MM-dd HH:mm:ss zzz'))",
               "mame: $ver",
               "mame_exe: $MameExe",
               "command: `"$MameExe`" $($MameArgs -join ' ')")
    foreach ($s in @($Script, (Join-Path (Split-Path $Script) "mame_dump.lua"))) {
        if (Test-Path $s) { $lines += "script: $(Split-Path $s -Leaf) sha256 $((Get-FileHash $s -Algorithm SHA256).Hash)" }
    }
    $lines += Get-ChildItem env: | Where-Object { $_.Name -like "HOTD_*" } | Sort-Object Name | ForEach-Object { "env: $($_.Name)=$($_.Value)" }
    $lines | Set-Content -Encoding utf8 (Join-Path $Out "recording.txt")
}
