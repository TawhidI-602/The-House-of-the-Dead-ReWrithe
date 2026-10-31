# Netplay test, up to four PCs (docs/netplay.md "Four PCs"): a headless host (--net-pcs 4) and three headless joiners on
# 127.0.0.1, each its own HOTD_PREF_DIR; -LateMs N starts the third joiner N ms after the others (it joins the running
# game). e.g. tools\netplay4_test.ps1 -Root <checkout with private\> -Name sim -HostArgs "--net-sim 5,15" -ClientArgs "--net-sim 5,15"
# -LeaveMs N: a seat leaves the waiting room before Start: after c1, a joiner "leaver" comes in (seat 3) and is closed N ms
# later; 6.5 s on (the host frees a silent seat after 5 s) c2 and c3 join: c2 must get the freed seat 3, c3 seat 4, c1's
# roster must show seat 3 free and then taken again, and the game runs with all four. Prints PASS / FAIL lines.
# Protocol 15 (PCs with one or two seats): -Joiners N (1-3 joiners), -C1Args "--net-seats 2" (c1 asks for two seats; the
# host's own: -HostArgs "--net-seats 2"); -KillMs N: c1 killed N ms after the start (no goodbye: a crash / pulled cable);
# -C3Args: c3 only (e.g. the late joiner's PC Mode pick "--net-pc-char 6 --start-at 4:20000");
# with -RejoinMs M a new c1 ("c1b", the same arguments) joins M ms after that and gets its seat(s) back in the running
# game; without it c1 is gone for good and the others play on. The last line: SYNC PASS / FAIL (every PC that played to
# the end: result=ok and the same final hash).
# -QuitAt F (2026-10-30): c1 leaves cleanly at frame F (HOTD_NET_QUIT_AT: BYE "leaving") instead of a kill; with
# -RejoinMs as above. The host's choice when a PC left: -HostArgs "--net-drop wait" (the game waits for c1b) or
# "--net-drop 3" (on without it after 3 s); the joiners print "the other PC shows: P2 left: ..." meanwhile.
param([string]$Exe = "$PSScriptRoot\..\build\Release\hotd.exe", [string]$Root = "$PSScriptRoot\..", [string]$Scratch = "$env:TEMP\hotd_netplay4_test", [string]$Name = "q", [string]$HostArgs = "", [string]$ClientArgs = "", [string]$Common = "--frames 40000 --autofire --mute --lives-kept 1", [int]$Port = 47910, [string]$Code = "TEST-CODE", [int]$LateMs = 0, [int]$LeaveMs = 0,
      [int]$Joiners = 3, [string]$C1Args = "", [string]$C3Args = "", [int]$KillMs = 0, [int]$RejoinMs = 0, [int]$QuitAt = 0)
$s = $Scratch; New-Item -ItemType Directory -Force $s | Out-Null
$wait = if ($LateMs -gt 0) { $Joiners - 1 } else { $Joiners }
$sides = @("host") + (@("c1", "c2", "c3") | Select-Object -First $Joiners)
$all = if ($LeaveMs -gt 0) { @("host", "c1", "leaver", "c2", "c3") } else { $sides }
foreach ($side in ($all + @("c1b"))) { New-Item -ItemType Directory -Force "$s\pref_${Name}_$side" | Out-Null }
$procs = @{}
function JoinCmd($side) {
    $extra = if ($side -eq "c1" -or $side -eq "c1b") { " $C1Args" } elseif ($side -eq "c3") { " $C3Args" } else { "" }
    return "--root `"$Root`" $Common --join 127.0.0.1:$Port --code $Code --net-name $side $ClientArgs$extra"
}
function Launch($side, $cmd) {
    $env:HOTD_PREF_DIR = "$s\pref_${Name}_$side"
    if ($QuitAt -gt 0 -and $side -eq "c1") { $env:HOTD_NET_QUIT_AT = "$QuitAt" } else { Remove-Item Env:HOTD_NET_QUIT_AT -ErrorAction SilentlyContinue }
    $p = Start-Process -FilePath $Exe -ArgumentList $cmd -NoNewWindow -PassThru -RedirectStandardOutput "$s\${Name}_$side.txt" -RedirectStandardError "$s\${Name}_$side.err"
    $null = $p.Handle
    return $p
}
foreach ($side in $all) {
    if ($side -eq "host") { $cmd = "--root `"$Root`" $Common --host 127.0.0.1:$Port --code $Code --net-pcs 4 --net-wait $wait --no-lan --upnp dry-run $HostArgs" }
    else { $cmd = JoinCmd $side }
    if ($side -eq "c3" -and $LateMs -gt 0) { Start-Sleep -Milliseconds $LateMs }
    if ($side -eq "c2" -and $LeaveMs -gt 0) {   # the leaver closes (killed: no goodbye, as a crash or a pulled cable)
        Start-Sleep -Milliseconds $LeaveMs
        $procs["leaver"].Kill(); $procs["leaver"].WaitForExit(5000) | Out-Null
        Start-Sleep -Milliseconds 6500
    }
    $procs[$side] = Launch $side $cmd
    if ($side -eq "host") { Start-Sleep -Milliseconds 500 } else { Start-Sleep -Milliseconds 300 }
}
$ends = $sides
if ($KillMs -gt 0 -or $QuitAt -gt 0) {   # c1's PC drops out mid-game (killed, or it leaves); with -RejoinMs it comes back as c1b
    if ($QuitAt -gt 0) {
        $procs["c1"].WaitForExit(300000) | Out-Null
        "c1 left: " + ((Select-String -Path "$s\${Name}_c1.txt" -Pattern "QUIT_AT|NET:" | % Line) -join " / ")
    } else {
        Start-Sleep -Milliseconds $KillMs
        $procs["c1"].Kill(); $procs["c1"].WaitForExit(5000) | Out-Null
        "c1 killed after $KillMs ms"
    }
    Remove-Item Env:HOTD_NET_QUIT_AT -ErrorAction SilentlyContinue
    $ends = $sides | Where-Object { $_ -ne "c1" }
    if ($RejoinMs -gt 0) {
        Start-Sleep -Milliseconds $RejoinMs
        $procs["c1b"] = Launch "c1b" (JoinCmd "c1b")
        $ends += "c1b"
        $all += "c1b"
    }
}
$t0 = Get-Date
foreach ($side in $ends) { if (-not $procs[$side].WaitForExit(600000)) { $procs[$side].Kill(); "$side killed" } }
"{0}: {1:N1} s, exit {2}" -f $Name, ((Get-Date) - $t0).TotalSeconds, (($ends | % { $procs[$_].ExitCode }) -join " ")
foreach ($side in $all) {
    "--- $side"
    Select-String -Path "$s\${Name}_$side.txt" -Pattern "^\[net\]|NET:|ended|netplay stopped|game over|character is now|PC MODE|other PC shows" | Select-Object -First 30 | % Line
    Select-String -Path "$s\${Name}_$side.err" -Pattern "netplay|rror|waiting room|seats|joining|joined|left" -ErrorAction SilentlyContinue | Select-Object -First 12 | % Line
}
if ($LeaveMs -gt 0) {
    $txt = @{}
    foreach ($side in $all) { $txt[$side] = (Get-Content "$s\${Name}_$side.txt", "$s\${Name}_$side.err" -ErrorAction SilentlyContinue) -join "`n" }
    $checks = @(
        @("the host frees the leaver's seat 3", ($txt["host"] -match "player 3 left the waiting room: the seat is free again")),
        @("c1's roster: seat 3 in, then free, then in again", ($txt["c1"] -match "(?s)seats: 1 2 \(this PC\) 3\s.*seats: 1 2 \(this PC\)\s*\n.*seats: 1 2 \(this PC\) 3")),
        @("c2 gets the freed seat 3", ($txt["c2"] -match "seats: 1 2 3 \(this PC\)")),
        @("c3 gets seat 4", ($txt["c3"] -match "seats: 1 2 3 4 \(this PC\)")),
        @("all four end in sync", (($sides | % { $txt[$_] -match "NET: result=ok" }) -notcontains $false))
    )
    foreach ($c in $checks) { "{0}: {1}" -f $(if ($c[1]) { "PASS" } else { "FAIL" }), $c[0] }
}
$hashes = @()
$okAll = $true
foreach ($side in $ends) {
    $l = Select-String -Path "$s\${Name}_$side.txt" -Pattern "NET: result=" | Select-Object -Last 1 | % Line
    if (-not $l -or $l -notmatch "result=ok") { $okAll = $false }
    if ($l -match " hash=([0-9a-f]+)") { $hashes += $Matches[1] }
}
$same = ($hashes | Select-Object -Unique).Count -eq 1 -and $hashes.Count -eq $ends.Count
"SYNC {0}: {1} PCs to the end, result=ok on all: {2}, final hashes: {3}" -f $(if ($okAll -and $same) { "PASS" } else { "FAIL" }), $ends.Count, $okAll, (($hashes | Select-Object -Unique) -join " ")
