# Netplay from the game's own menu (docs/netplay.md "Playing online"): Online Co-op > Host a Game / Join a Game, driven
# by the HOTD_ONLINE_TEST hooks in hidden windows (never shown, no focus, no keys: HOTD_WINDOW_DISPLAY=99), against
# each other and against the command line, on 127.0.0.1. Each pair plays -Frames frames (--autofire) and must end with
# "NET: result=ok" and the same hash on both PCs.
#   -Mode menu-cli:  menu host (Start once player 2 is in)  + command-line joiner (--join address --code)
#         cli-menu:  command-line host                      + menu joiner (address and password in the Join box)
#         menu-menu: menu host                              + menu joiner (address and password)
#         code:      menu host (a random code)              + menu joiner (the host's join code, typed in the box)
#         lan:       menu host (a random code, --lobby)     + menu joiner (the LAN list's game clicked, the code typed)
#         lan-open:  menu host (--lobby)                    + menu joiner (the LAN list's game clicked: no code typed)
#         rejoin-close: menu host + menu joiner (the join code) whose window closes at frame -QuitAt (HOTD_NET_QUIT_AT: a
#                    player closing the window, not a crash); a new menu joiner with the same code joins the running
#                    game; the host and the new joiner must end ok with the same hash
#         rejoin-kill: the same, the first joiner killed hard (a crash) 3 s into play; the new joiner 2 s later from the
#                    LAN list (no code: the host lists itself as joinable while it waits)
#         all:       the five first modes, one after another (default); "rejoin": lan-open + both rejoin modes
# Every window keeps the boot (splash; the attract would follow): the menu host hosts as the splash ends, so this
# also checks the attract never takes the screen while hosting / joining (the waiting room unanswered: menuhost).
# Prints PASS / FAIL per mode; the exit code is the number of failed modes.
param([string]$Root = "$PSScriptRoot\..", [string]$Exe = "$PSScriptRoot\..\build\Release\hotd.exe",
      [string]$Scratch = "$env:TEMP\hotd_netplay_menu_test", [ValidateSet("all", "rejoin", "menu-cli", "cli-menu", "menu-menu", "code", "lan", "lan-open", "rejoin-close", "rejoin-kill")]
      [string]$Mode = "all", [int]$Port = 47970, [int]$QuitAt = 1200,
      [int]$CodePort = 47803,   # code / lan: a short join code needs the default ports 47800-47803
      [int]$Frames = 3000, [int]$TimeoutS = 240,
      [string]$Arcade = "", [string]$Pc = "")   # (a fresh settings folder: --arcade / --pc, as the input test)
$ErrorActionPreference = "Stop"
$Root = (Resolve-Path $Root).Path
$files = $(if ($Arcade) { " --arcade `"$Arcade`"" } else { "" }) + $(if ($Pc) { " --pc `"$Pc`"" } else { "" })
$common = "--root `"$Root`" --autofire --mute --lives-kept 1$files"

function Start-Hotd($name, $argList, $onlineTest, $quitAt = 0) {
    New-Item -ItemType Directory -Force "$Scratch\pref_$name" | Out-Null
    $env:HOTD_PREF_DIR = "$Scratch\pref_$name"
    $env:HOTD_WINDOW_DISPLAY = "99"   # (no such display: the window stays hidden)
    if ($onlineTest) { $env:HOTD_ONLINE_TEST = $onlineTest } else { Remove-Item Env:HOTD_ONLINE_TEST -ErrorAction SilentlyContinue }
    if ($quitAt -gt 0) { $env:HOTD_NET_QUIT_AT = "$quitAt" } else { Remove-Item Env:HOTD_NET_QUIT_AT -ErrorAction SilentlyContinue }
    Remove-Item "$Scratch\$name.txt", "$Scratch\$name.err" -ErrorAction SilentlyContinue
    $p = Start-Process $Exe -ArgumentList $argList -NoNewWindow -PassThru -RedirectStandardOutput "$Scratch\$name.txt" -RedirectStandardError "$Scratch\$name.err"
    $null = $p.Handle
    Remove-Item Env:HOTD_ONLINE_TEST, Env:HOTD_NET_QUIT_AT -ErrorAction SilentlyContinue
    return $p
}
# waits until the log matches (or the process ended / timed out): true when it matched
function Wait-Log($name, $p, $pattern, $seconds) {
    for ($i = 0; $i -lt $seconds * 10; $i++) {
        if ((Log-Of $name) -match $pattern) { return $true }
        if ($p.HasExited) { return $false }
        Start-Sleep -Milliseconds 100
    }
    return $false
}
function Log-Of($name) { (Get-Content "$Scratch\$name.txt", "$Scratch\$name.err" -ErrorAction SilentlyContinue) -join "`n" }
# the host's join code from its log ("hosting on UDP ...; join code XXXX-XXXX-XXXX-XXXX"), waited for
function Wait-Code($name, $p) {
    for ($i = 0; $i -lt 600; $i++) {
        if ((Log-Of $name) -match "join code ([A-Z0-9-]+)") { return $Matches[1] }
        if ($p.HasExited) { break }
        Start-Sleep -Milliseconds 100
    }
    return $null
}
function Run-Pair($m) {
    $port = $(if ($m -eq "code" -or $m -like "lan*" -or $m -like "rejoin*") { $CodePort } else { $Port })
    $lobby = $Port + 1
    $menuHost = "$common --stop-at $Frames --net-bind 127.0.0.1 --net-port $port --upnp off"
    $cliJoin = "$common --frames $Frames --net-timeout 60"
    $menuJoin = "$common --stop-at $Frames --net-timeout 60"
    $h = $null; $c = $null; $c1 = $null
    switch ($m) {
        "menu-cli" {
            $h = Start-Hotd "h_$m" "$menuHost --code MENUPAIR1 --no-lan" "host"
            if (-not (Wait-Code "h_$m" $h)) { break }
            $c = Start-Hotd "c_$m" "$cliJoin --join 127.0.0.1:$port --code MENUPAIR1" $null
        }
        "cli-menu" {
            $h = Start-Hotd "h_$m" "$common --frames $Frames --host 127.0.0.1:$port --code MENUPAIR2 --no-lan --net-timeout 120" $null
            if (-not (Wait-Code "h_$m" $h)) { break }
            $c = Start-Hotd "c_$m" $menuJoin "join:127.0.0.1:$port|MENUPAIR2"
        }
        "menu-menu" {
            $h = Start-Hotd "h_$m" "$menuHost --code MENUPAIR3 --no-lan" "host"
            if (-not (Wait-Code "h_$m" $h)) { break }
            $c = Start-Hotd "c_$m" $menuJoin "join:127.0.0.1:$port|MENUPAIR3"
        }
        "code" {
            $h = Start-Hotd "h_$m" "$menuHost --no-lan" "host"
            $code = Wait-Code "h_$m" $h
            if (-not $code) { break }
            "  join code $code"
            $c = Start-Hotd "c_$m" $menuJoin "join:$($code.ToLower().Replace('-', ''))"   # (typed lower case, no dashes)
        }
        "lan" {
            $h = Start-Hotd "h_$m" "$menuHost --lobby 127.0.0.1:$lobby" "host"
            $code = Wait-Code "h_$m" $h
            if (-not $code) { break }
            "  join code $code"
            $c = Start-Hotd "c_$m" "$menuJoin --lobby 127.0.0.1:$lobby" "join-lan:$code"
        }
        "lan-open" {
            $h = Start-Hotd "h_$m" "$menuHost --lobby 127.0.0.1:$lobby" "host"
            if (-not (Wait-Code "h_$m" $h)) { break }
            $c = Start-Hotd "c_$m" "$menuJoin --lobby 127.0.0.1:$lobby" "join-lan-open"
        }
        { $_ -eq "rejoin-close" -or $_ -eq "rejoin-kill" } {
            $h = Start-Hotd "h_$m" "$menuHost --lobby 127.0.0.1:$lobby" "host"
            $code = Wait-Code "h_$m" $h
            if (-not $code) { break }
            "  join code $code"
            if ($m -eq "rejoin-close") {
                $c1 = Start-Hotd "c1_$m" "$menuJoin --lobby 127.0.0.1:$lobby" "join:$code" $QuitAt
                $c1.WaitForExit(180000) | Out-Null
                if (-not $c1.HasExited) { $c1.Kill(); "  c1 killed (it did not close)" }
                "  c1 closed: " + (((Log-Of "c1_$m") -split "`n" | Where-Object { $_ -match "QUIT_AT|left the online game" }) -join " / ")
                Start-Sleep -Milliseconds 1500
                $c = Start-Hotd "c_$m" "$menuJoin --lobby 127.0.0.1:$lobby" "join:$code"
            } else {
                $c1 = Start-Hotd "c1_$m" "$menuJoin --lobby 127.0.0.1:$lobby" "join:$code"
                if (-not (Wait-Log "c1_$m" $c1 "both games started the same" 120)) { "  c1 never started"; break }
                Start-Sleep -Seconds 3
                $c1.Kill(); $c1.WaitForExit(5000) | Out-Null
                "  c1 killed (a crash)"
                Start-Sleep -Seconds 2
                $c = Start-Hotd "c_$m" "$menuJoin --lobby 127.0.0.1:$lobby" "join-lan-open"
            }
        }
    }
    $deadline = (Get-Date).AddSeconds($TimeoutS)
    foreach ($p in @($c, $h)) {
        if (-not $p) { continue }
        $left = [int][Math]::Max(1000, ($deadline - (Get-Date)).TotalMilliseconds)
        $p.WaitForExit($left) | Out-Null
    }
    foreach ($p in @($h, $c, $c1)) { if ($p -and -not $p.HasExited) { $p.Kill(); "  killed $($p.Id) (timed out)" } }
    $res = @()
    foreach ($n in "h_$m", "c_$m") {
        $line = (Log-Of $n) -split "`n" | Where-Object { $_ -match "^NET: result=" } | Select-Object -First 1
        if (-not $line) { $line = "(no NET: line)" }
        $hash = $(if ($line -match " hash=([0-9a-f]+)") { $Matches[1] } else { "" })
        $ok = $line -match "^NET: result=ok"
        $res += [pscustomobject]@{ Name = $n; Ok = $ok; Hash = $hash; Line = $line }
        "  $n`: " + $line.Substring(0, [Math]::Min(150, $line.Length))
        (Log-Of $n) -split "`n" | Where-Object { $_ -match "\[net\] (hosting on|a player|player 2 joined|connected|online|the LAN list|joining|player 2 left|player 2 lost|goes on|left the online)" } |
            Select-Object -First 10 | ForEach-Object { "    " + $_.Substring(0, [Math]::Min(150, $_.Length)) }
    }
    $pass = $res.Count -eq 2 -and $res[0].Ok -and $res[1].Ok -and $res[0].Hash -ne "" -and $res[0].Hash -eq $res[1].Hash
    "$(if ($pass) { 'PASS' } else { 'FAIL' }) $m $(if ($pass) { $res[0].Hash })"
    return $pass
}

New-Item -ItemType Directory -Force $Scratch | Out-Null
$modes = $(if ($Mode -eq "all") { @("menu-cli", "cli-menu", "menu-menu", "code", "lan") } elseif ($Mode -eq "rejoin") { @("lan-open", "rejoin-close", "rejoin-kill") } else { @($Mode) })
$failed = 0
foreach ($m in $modes) {
    "--- $m"
    $out = Run-Pair $m
    $out | Select-Object -SkipLast 1 | ForEach-Object { $_ }
    if (-not ($out | Select-Object -Last 1)) { $failed++ }
}
"menu netplay: $($modes.Count - $failed) / $($modes.Count) passed"
exit $failed
