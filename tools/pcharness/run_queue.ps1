# run_queue.ps1 - PC proof captures one after another (docs/proof.md "Capture tools"): a queue file lists
# run_pc.ps1 argument lines and this runs them in order, then `build\Release\hotd_proof.exe pccapture <Out>` for each.
#   .\run_queue.ps1 <queue file> [-DefaultTimeoutSeconds 3600] [-WaitMinutes 240] [-MaxGames 2] [-DryRun]
# Queue file: one run_pc.ps1 argument line per run (blank lines and # comments skipped), e.g.
#   -Out private\pcharness\runs\dog_pa -Ticks 90000 -InputScript tools\pcharness\scripts\game_aim.txt -InfLives -CapMax 3000 -CapFuncs "4dd5c4,..." -CapBoundary "..."
# Every run gets -TimeoutSeconds (DefaultTimeoutSeconds when the line has none). Before each run the script waits
# (up to -WaitMinutes) while -MaxGames THOTD.EXE or more run (other sessions' captures: hooked games can run side by
# side, each one slower). Per run: run_pc.ps1's output in
# <Out>\queue_run.txt, the proof's in <Out>\proof.txt, and one summary line appended to
# private\pcharness\queue_log.txt (captures, differ, dropped, boundary calls, per-routine counts).
param([Parameter(Mandatory)] [string]$QueueFile, [int]$DefaultTimeoutSeconds = 3600, [int]$WaitMinutes = 240,
      [int]$MaxGames = 2, [switch]$DryRun)
$root = Split-Path (Split-Path $PSScriptRoot)
$runPc = Join-Path $PSScriptRoot "run_pc.ps1"
$proof = Join-Path $root "build\Release\hotd_proof.exe"
$log = Join-Path $root "private\pcharness\queue_log.txt"
$QueueFile = (Resolve-Path $QueueFile).Path
Push-Location $root
try {
    $lines = @(Get-Content $QueueFile | Where-Object { $_.Trim() -and -not $_.Trim().StartsWith("#") })
    "queue: $($lines.Count) runs from $QueueFile"
    $n = 0
    foreach ($line in $lines) {
        $n++
        $line = $line.Trim()
        if ($line -notmatch '(?:^|\s)-Out\s+(?:"([^"]+)"|(\S+))') { "run ${n}: no -Out, skipped: $line"; continue }
        $out = if ($Matches[1]) { $Matches[1] } else { $Matches[2] }
        if ($line -notmatch '(?:^|\s)-TimeoutSeconds\s') { $line += " -TimeoutSeconds $DefaultTimeoutSeconds" }
        "run ${n}/$($lines.Count): $out"
        if ($DryRun) { "  & run_pc.ps1 $line"; continue }

        # at most -MaxGames games at once: wait while other captures fill the slots
        $deadline = (Get-Date).AddMinutes($WaitMinutes)
        while (@(Get-Process THOTD -ErrorAction SilentlyContinue).Count -ge $MaxGames) {
            if ((Get-Date) -gt $deadline) { break }
            Start-Sleep -Seconds 30
        }
        if (@(Get-Process THOTD -ErrorAction SilentlyContinue).Count -ge $MaxGames) {
            $msg = "$(Get-Date -Format 'yyyy-MM-dd HH:mm') $out : NOT RUN ($MaxGames THOTD still running after $WaitMinutes min)"
            Add-Content -Path $log -Value $msg; $msg; continue
        }

        $t0 = Get-Date
        $runOut = Invoke-Expression "& `"$runPc`" $line" 2>&1 | Out-String
        $mins = [math]::Round(((Get-Date) - $t0).TotalMinutes, 1)
        New-Item -ItemType Directory -Force $out | Out-Null
        Set-Content -Path (Join-Path $out "queue_run.txt") -Value $runOut
        $timeout = if ($runOut -match "TIMEOUT") { " TIMEOUT" } else { "" }

        $summary = ""
        if (-not (Test-Path (Join-Path $out "capture.bin"))) {
            $summary = "no capture.bin"
        } elseif (-not (Test-Path $proof)) {
            $summary = "no $proof (build hotd_proof)"
        } else {
            $proofOut = & $proof pccapture $out 2>&1 | Out-String
            $code = $LASTEXITCODE
            Set-Content -Path (Join-Path $out "proof.txt") -Value $proofOut
            if ($proofOut -match "pc captures: (\d+) checked \((\d+) boundary calls replayed\), (\d+) differ, (\d+) dropped") {
                $summary = "captures $($Matches[1]), differ $($Matches[3]), dropped $($Matches[4]), boundary calls $($Matches[2])"
            } else {
                $summary = "proof exit $code, no summary line"
            }
            $per = [regex]::Matches($proofOut, '(?m)^\s+([0-9A-Fa-f]{6,8}): (\d+)\s*$') | ForEach-Object { "$($_.Groups[1].Value):$($_.Groups[2].Value)" }
            if ($per) { $summary += "; per routine " + ($per -join " ") }
            $first = [regex]::Match($proofOut, '(?m)^capture .*differ.*$')
            if ($first.Success) { $summary += "; first difference: " + $first.Value.Trim() }
        }
        $msg = "$(Get-Date -Format 'yyyy-MM-dd HH:mm') $out ($mins min$timeout): $summary"
        Add-Content -Path $log -Value $msg
        $msg
    }
} finally {
    Pop-Location
}
