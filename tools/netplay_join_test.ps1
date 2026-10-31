# Netplay: joining a game already running (docs/netplay.md), two headless hotd instances on 127.0.0.1.
#   -Mode join:   the host plays alone (--net-alone, held to --net-solo-fps 1000); a client joins after 4 s, catches up
#                 from the host's log and presses start at frame 9000
#   -Mode rejoin: host and client play (2 players, 15 ms each way); the client is killed after 7 s; the host waits; a
#                 new client joins 9 s later and catches up
#   -Mode leave:  the same, but the client leaves cleanly at frame -QuitAt (HOTD_NET_QUIT_AT: a player closing the
#                 game, BYE "leaving", 2026-10-30): the host keeps the seat; the new client joins -RejoinMs later
#   -Mode continue: the client leaves at -QuitAt; the host (--net-drop 3) waits 3 s, then goes on alone to the end
#                 (no rejoin): its log must say it went on, and its run ends at --frames
# Both runs end with "NET: result=ok" on both sides when the final states agree (continue: the host alone, "nobye").
param([string]$Root = "$PSScriptRoot\..", [string]$Exe = "$PSScriptRoot\..\build\Release\hotd.exe",
      [string]$Scratch = "$env:TEMP\hotd_netplay_test", [ValidateSet("join", "rejoin", "leave", "continue")] [string]$Mode = "join", [int]$Port = 47950,
      [int]$RejoinMs = 9000,   # rejoin: the new client this long after the kill (2000: before the host's 6 s silence ran out)
      [int]$QuitAt = 3000,     # leave / continue: the client's frame of leaving
      [int]$Players = 2,   # 4: the 4-seat game (docs/coop4.md phase 7, each PC two seats; join: P3 starts at 500, P2 / P4 at 9000)
      [string]$HostArgs = "", [string]$ClientArgs = "")   # extra options (PC Mode: -HostArgs "--pc-mode --net-pc-char 5" -ClientArgs "--net-pc-char 6")
New-Item -ItemType Directory -Force $Scratch, "$Scratch\pref_jh", "$Scratch\pref_jc" | Out-Null
$common = "--root `"$Root`" --frames 20000 --autofire --mute --lives-kept 1"
$hostExtra = $(if ($Players -eq 4) { " --players 4 --start-at 3:500" } else { "" }) + " $HostArgs"
$clientExtra = $(if ($Players -eq 4) { " --start-at 4:9000" } else { "" }) + " $ClientArgs"
function Run($pref, $argList, $out, $quitAt = 0) {
    $env:HOTD_PREF_DIR = "$Scratch\$pref"
    if ($quitAt -gt 0) { $env:HOTD_NET_QUIT_AT = "$quitAt" } else { Remove-Item Env:HOTD_NET_QUIT_AT -ErrorAction SilentlyContinue }
    $p = Start-Process $Exe -ArgumentList $argList -NoNewWindow -PassThru -RedirectStandardOutput "$Scratch\$out.txt" -RedirectStandardError "$Scratch\$out.err"
    $null = $p.Handle
    Remove-Item Env:HOTD_NET_QUIT_AT -ErrorAction SilentlyContinue
    return $p
}
$c1 = $null
if ($Mode -eq "join") {
    $h = Run "pref_jh" "$common --host 127.0.0.1:$Port --code JOINTEST --no-lan --net-alone --net-solo-fps 1000 --net-timeout 30$hostExtra" "jh"
    Start-Sleep 4
    $c = Run "pref_jc" "$common --join 127.0.0.1:$Port --code JOINTEST --start-at 2:9000$clientExtra" "jc"
} else {
    $drop = $(if ($Mode -eq "continue") { " --net-drop 3" } else { "" })
    $h = Run "pref_jh" "$common --players $Players --host 127.0.0.1:$Port --code REJOIN99 --no-lan --net-sim 0,15 --net-timeout 60$drop $HostArgs" "jh"
    Start-Sleep -Milliseconds 500
    if ($Mode -eq "rejoin") {
        $c1 = Run "pref_jc" "$common --join 127.0.0.1:$Port --code REJOIN99 --net-sim 0,15 $ClientArgs" "jc1"
        Start-Sleep 7; $c1.Kill()
    } else {
        $c1 = Run "pref_jc" "$common --join 127.0.0.1:$Port --code REJOIN99 --net-sim 0,15 $ClientArgs" "jc1" $QuitAt
        $c1.WaitForExit(120000) | Out-Null
        Select-String -Path "$Scratch\jc1.txt" -Pattern "QUIT_AT|NET:" | ForEach-Object { "jc1: " + $_.Line.Substring(0, [Math]::Min(120, $_.Line.Length)) }
    }
    if ($Mode -ne "continue") {
        Start-Sleep -Milliseconds $RejoinMs
        $c = Run "pref_jc" "$common --join 127.0.0.1:$Port --code REJOIN99 --net-sim 0,15 $ClientArgs" "jc"
    }
}
if ($c) { $c.WaitForExit(300000) | Out-Null }
$h.WaitForExit(300000) | Out-Null
foreach ($p in $h, $c, $c1) { if ($p -and -not $p.HasExited) { $p.Kill(); "killed" } }
foreach ($n in $(if ($c) { "jh", "jc" } else { "jh" })) {
    "--- $n"
    Select-String -Path "$Scratch\$n.txt" -Pattern "\[net\]|NET:|chapter 1 ended|stopped|character is now|PC MODE" | Select-Object -First 20 | ForEach-Object { $_.Line.Substring(0, [Math]::Min(170, $_.Line.Length)) }
}
