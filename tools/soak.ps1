# soak.ps1: the crash / hang soak (docs/soak.md). Headless, muted runs of hotd.exe across the game's modes and options,
# each with a wall-clock timeout (a hang is killed by its own PID, never by name), several at once; every run's log is
# scanned for crashes, hangs, GL errors, the watchdog, memory faults, NaN draws and the expected end line. Writes
# <Out>\summary.md (a Markdown table) and <Out>\<case>.log; the exit code is the number of failed runs.
#
#   powershell -ExecutionPolicy Bypass -File tools\soak.ps1 -Root "<main checkout>" [-Set quick|full] [-Jobs 4]
#       [-Exe build\Release\hotd.exe] [-Out build\soak] [-Only <regex>] [-Timeout 900]
#       [-PcDir <the PC game's folder, only read: the menus>]
#
# -Root: the folder with private\ (the game data). Every run gets its own HOTD_PREF_DIR under <Out>\pref\<case>.
param(
    [string]$Root = "",
    [string]$Exe = "",
    [string]$Out = "",
    [string]$Set = "quick",
    [int]$Jobs = 4,
    [string]$Only = "",
    [int]$Timeout = 900,
    [string]$PcDir = "C:\Games\HOTD"
)
$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $PSScriptRoot
if (-not $Exe) { $Exe = Join-Path $here "build\Release\hotd.exe" }
if (-not $Out) { $Out = Join-Path $here "build\soak" }
if (-not $Root) { $Root = $here }
New-Item -ItemType Directory -Force $Out | Out-Null
New-Item -ItemType Directory -Force (Join-Path $Out "pref") | Out-Null

# a case: name, arguments, the line that must appear (regex; "" = none), frames
$cases = New-Object System.Collections.ArrayList
function Add-Case([string]$name, [string]$argLine, [string]$expect) {
    [void]$cases.Add([pscustomobject]@{ Name = $name; Args = $argLine; Expect = $expect })
}
$c1 = "--frames 40000 --autofire --mute --lives-kept 1"
$end1 = "chapter 1 starts \(after the clear\)|the chapter is over|chapter 2 starts"
$ending = "the ending is over"

# 1. the arcade game: every chapter, both models (the proof runs)
foreach ($m in @("arcade", "pc")) {
    Add-Case "arcade-c1-$m" "$c1 --models $m" "chapter 2 starts"
    foreach ($ch in 2, 3) { Add-Case "arcade-c$ch-$m" "--chapter $ch --frames 70000 --autofire --mute --lives-kept 1 --models $m" "chapter $($ch + 1) starts" }
    Add-Case "arcade-c4-strict-$m" "--chapter 4 --strict --autofire --lives-kept 1 --mute --frames 60000 --default-route --models $m" $ending
    # misc17: the arcade's name entry after the ending (was window-only): --ranking-low makes the score place,
    # --name-keys types the name (one key every 8 frames, '#' END)
    Add-Case "name-entry-c4-$m" "--chapter 4 --strict --autofire --lives-kept 1 --mute --frames 60000 --default-route --models $m --name-entry --ranking-low --name-keys ABC#" 'ranking: P1 place 1 "ABC"'
}
Add-Case "name-entry-c4-2p" "--chapter 4 --autofire --lives-kept 1 --mute --frames 60000 --default-route --start-at 2:600 --name-entry --ranking-low --name-keys AB#" 'ranking: P[12] place 1 "AB '
Add-Case "name-entry-c4-4p" "--chapter 4 --autofire --lives-kept 1 --mute --frames 60000 --default-route --players 4 --name-entry --ranking-low --name-keys XY#" 'ranking: players 3 / 4'
Add-Case "name-entry-c4-empty" "--chapter 4 --autofire --lives-kept 1 --mute --frames 60000 --default-route --name-entry --ranking-low --name-keys #" 'ranking: P1 place 1'
# 2. drawing: the offscreen renderer (GL errors) across Draw Depth / Aspect / models / Ragdoll / language
foreach ($m in @("arcade", "pc")) {
    foreach ($dd in @("sort", "buffer")) {
        foreach ($asp in @("16:9", "16:10", "4:3")) {
            $fr = if ($Set -eq "full") { 20000 } else { 6000 }
            Add-Case "render-$m-$dd-$($asp.Replace(':', 'x'))" "--render --frames $fr --autofire --mute --lives-kept 1 --models $m --draw-depth $dd --aspect $asp --size 640x360" ""
        }
    }
    Add-Case "render-$m-ragdoll" "--render --frames 8000 --autofire --mute --lives-kept 1 --models $m --mod ragdoll --size 640x360" ""
}
foreach ($l in @("ko", "ar", "en")) { Add-Case "render-lang-$l" "--render --frames 4000 --autofire --mute --lives-kept 1 --lang $l --size 640x360" "" }
# 3. players: 2P / 4P, CPU teammates, joins
Add-Case "2p-start" "$c1 --start-at 2:600" "chapter 2 starts"
Add-Case "2p-cpu" "$c1 --players 2 --seat-bot 2:2 --start-at 2:600" "chapter 2 starts"
Add-Case "4p-c1" "$c1 --players 4" "chapter 2 starts"
Add-Case "4p-cpu" "$c1 --players 4 --seat-bot 2:1 --seat-bot 3:2 --seat-bot 4:3" "chapter 2 starts"
Add-Case "3p-join" "$c1 --players 3 --start-at 4:3000" "chapter 2 starts"
Add-Case "4p-render-pc" "--render --frames 8000 --autofire --mute --lives-kept 1 --players 4 --models pc --size 640x360" ""
# 4. difficulty
foreach ($d in 0, 4) { Add-Case "diff$d" "$c1 --difficulty $d" "chapter 2 starts" }
# 5. lives out: continue countdown, GAME OVER, the restart (bots: game overs restart the chapter; the watchdog)
Add-Case "livesout-1p" "--frames 6000 --autofire --mute --lives-out 2000" "game over done"
Add-Case "livesout-4p" "--frames 6000 --autofire --mute --players 4 --lives-out 2000" "game over done"
foreach ($s in 1, 2, 3) { Add-Case "bot-$s" "--frames 30000 --mute --bot $s --bot-gameovers 2" "ROBUST: result=(clear|gameover|frames)" }
Add-Case "bot-2p" "--frames 30000 --mute --bot 7 --bot-players 2 --bot-gameovers 2" "ROBUST: result=(clear|gameover|frames)"
foreach ($ch in 2, 3, 4) { Add-Case "bot-c$ch" "--chapter $ch --frames 30000 --mute --bot $ch --bot-gameovers 2" "ROBUST: result=(clear|gameover|frames)" }
Add-Case "bot-4p" "--frames 30000 --mute --players 4 --bot 8 --bot-gameovers 2" "ROBUST: result=(clear|gameover|frames)"
Add-Case "bot-pcmode" "--frames 30000 --mute --pc-char 4 --bot 9 --bot-gameovers 2" "ROBUST: result=(clear|gameover|frames)"
# the continue / GAME OVER screens drawn (both models), and a 2P game where only one player runs out
foreach ($m in @("arcade", "pc")) {
    Add-Case "render-livesout-$m" "--render --frames 6000 --autofire --mute --lives-out 2000 --models $m --size 640x360" "game over done"
    Add-Case "render-bot-$m" "--render --frames 20000 --mute --bot 5 --bot-players 2 --bot-gameovers 2 --models $m --size 640x360" "ROBUST: result=(clear|gameover|frames)"
}
Add-Case "render-livesout-4p" "--render --frames 6000 --autofire --mute --players 4 --lives-out 2000 --models pc --size 640x360" "game over done"
Add-Case "render-livesout-boss" "--render --frames 6000 --autofire --mute --boss-mode chariot --lives-out 1500 --size 640x360" ""
# 6. PC Mode: every character (P1), a 2P pair, 4P
foreach ($pc in 0..6) { Add-Case "pcmode-char$pc" "$c1 --pc-char $pc" "chapter 2 starts" }
Add-Case "pcmode-2p" "$c1 --pc-char 2,0 --start-at 2:600" "chapter 2 starts"
Add-Case "pcmode-4p" "$c1 --players 4 --pc-char 5,0,3,6" "chapter 2 starts"
Add-Case "pcmode-c4" "--chapter 4 --frames 60000 --autofire --mute --lives-kept 1 --default-route --pc-char 5,6" $ending
Add-Case "pcmode-livesout" "--frames 6000 --autofire --mute --pc-char 1 --lives-out 2000" "game over done"
# 7. Boss Mode
foreach ($b in @("chariot", "hangedman", "hermit", "magician", "all")) {
    Add-Case "boss-$b" "--frames 40000 --autofire --mute --lives-kept 1 --boss-mode $b" "Boss Mode: "
}
Add-Case "boss-all-4p" "--frames 40000 --autofire --mute --lives-kept 1 --players 4 --boss-mode all" "Boss Mode: "
Add-Case "boss-render-pc" "--render --frames 8000 --autofire --mute --lives-kept 1 --models pc --boss-mode hermit --size 640x360" ""
# 8. Sonic Mode
foreach ($s in @("frenzy", "overkill", "nightmare")) { Add-Case "sonic-$s" "$c1 --sonic-mode $s" "chapter 2 starts" }
# 9. Chapter Select: every chapter drawn (both models), 4P and PC Mode per chapter
foreach ($ch in 1..4) {
    foreach ($m in @("arcade", "pc")) {
        Add-Case "render-c$ch-$m" "--render --chapter $ch --frames 20000 --autofire --mute --lives-kept 1 --default-route --models $m --size 640x360" ""
    }
    $next = if ($ch -lt 4) { "chapter $($ch + 1) starts" } else { $ending }
    Add-Case "4p-ch$ch" "--chapter $ch --frames 70000 --autofire --mute --lives-kept 1 --default-route --players 4" $next
    Add-Case "pcmode-c$ch-2p" "--chapter $ch --frames 70000 --autofire --mute --lives-kept 1 --default-route --pc-char 3,4 --start-at 2:600" $next
}
# 11. the menus: every screen --frontend-shot draws, both styles, three languages (a crash or an error: a failure)
$shots = Join-Path $Out "shots"
New-Item -ItemType Directory -Force $shots | Out-Null
$screens = if ($Set -eq "full") { @(0..36) + @(119, 121) } else { @(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 15, 16, 17, 19, 21, 22, 23, 32, 119, 121) }
foreach ($st in @("classic", "remake")) {
    foreach ($l in @("en", "ko", "ar")) {
        foreach ($sc in $screens) {
            Add-Case "menu-$st-$l-$sc" "--pc `"$PcDir`" --frontend-shot `"$shots\$st-$l-$sc.png`" --frontend-screen $sc --menu-style $st --lang $l --size 960x540" ""
        }
    }
}
# 10. the whole game in one go (full set)
if ($Set -eq "full") {
    Add-Case "whole-arcade" "--frames 140000 --autofire --mute --lives-kept 1 --default-route --strict" $ending
    Add-Case "whole-4p" "--frames 140000 --autofire --mute --lives-kept 1 --default-route --players 4" $ending
    Add-Case "whole-pcmode" "--frames 140000 --autofire --mute --lives-kept 1 --default-route --pc-char 3,4" $ending
    Add-Case "whole-sonic" "--frames 140000 --autofire --mute --lives-kept 1 --default-route --sonic-mode frenzy" $ending
    Add-Case "whole-render-pc" "--render --frames 140000 --autofire --mute --lives-kept 1 --default-route --models pc --size 640x360" $ending
    foreach ($s in 10..19) { Add-Case "bot-long-$s" "--frames 80000 --mute --bot $s --bot-gameovers 4" "ROBUST: result=(clear|gameover|frames)" }
    foreach ($s in 20..23) { Add-Case "bot-4p-$s" "--frames 60000 --mute --players 4 --bot $s --bot-gameovers 3" "ROBUST: result=(clear|gameover|frames)" }
}
if ($Only) { $cases = @($cases | Where-Object { $_.Name -match $Only }) }

# problems in a log (each: a short word and the regex)
$bad = @(
    @("gl-error", "GL error 0x"),
    @("watchdog", "watchdog: |ROBUST: result=problem|ROBUST: result=restart-failed"),
    @("bad-access", "access outside the mapped memory"),
    @("nan", "nan_draws=[1-9]"),
    @("assert", "(?i)assertion|abort\(\)|terminate called"),
    @("render-fail", "render: (framebuffer incomplete|OpenGL)"),
    @("gl-errors", "gl_errors=[1-9]")
)

$rows = New-Object System.Collections.ArrayList
$queue = New-Object System.Collections.Queue
foreach ($c in $cases) { $queue.Enqueue($c) }
$running = @{}
$t0 = Get-Date
Write-Host ("soak: {0} runs, {1} at once, timeout {2} s" -f $cases.Count, $Jobs, $Timeout)
function Finish($c, $p, [bool]$timedOut, [double]$secs) {
    $log = Join-Path $Out "$($c.Name).log"
    $text = ""
    foreach ($f in @("$log.out", "$log.err")) { if (Test-Path $f) { $text += [IO.File]::ReadAllText($f) + "`n" } }
    [IO.File]::WriteAllText($log, "hotd $($c.Args)`n`n" + $text)
    Remove-Item -Force "$log.out", "$log.err" -ErrorAction SilentlyContinue
    $probs = @()
    $code = if ($timedOut) { "-" } else { $p.ExitCode }
    if ($timedOut) { $probs += "HANG (timeout)" }
    elseif ($code -ne 0) { $probs += ("exit 0x{0:X}" -f ([int64]$code -band 0xFFFFFFFF)) }
    foreach ($b in $bad) { if ($text -match $b[1]) { $probs += $b[0] } }
    if ($c.Expect -and -not ($text -match $c.Expect)) { $probs += "no '$($c.Expect)'" }
    $last = ""
    foreach ($l in ($text -split "`n")) {
        if ($l -match "^frame (\d+): (chapter \d starts|the chapter is over|game over done)" -or $l -match "the ending is over" -or
            $l -match "^ROBUST: result=\w+" -or $l -match "Boss Mode: .*(time|done|over)") { $last = $l.Trim() }
    }
    if ($last.Length -gt 90) { $last = $last.Substring(0, 90) }
    $ok = $probs.Count -eq 0
    [void]$rows.Add([pscustomobject]@{ Name = $c.Name; Ok = $ok; Secs = [int]$secs; Problems = ($probs -join "; "); Last = $last; Args = $c.Args })
    Write-Host ("{0,-28} {1,-4} {2,5} s  {3}" -f $c.Name, $(if ($ok) { "ok" } else { "FAIL" }), [int]$secs, $(if ($ok) { $last } else { $probs -join "; " }))
}
while ($queue.Count -gt 0 -or $running.Count -gt 0) {
    while ($queue.Count -gt 0 -and $running.Count -lt $Jobs) {
        $c = $queue.Dequeue()
        $pref = Join-Path $Out "pref\$($c.Name)"
        New-Item -ItemType Directory -Force $pref | Out-Null
        $env:HOTD_PREF_DIR = $pref
        $log = Join-Path $Out "$($c.Name).log"
        $argLine = "--root `"$Root`" " + $c.Args
        $p = Start-Process -FilePath $Exe -ArgumentList $argLine -NoNewWindow -PassThru -RedirectStandardOutput "$log.out" -RedirectStandardError "$log.err"
        $null = $p.Handle   # (PowerShell 5.1: without the handle cached, ExitCode stays empty)
        $running[$c.Name] = @{ C = $c; P = $p; T = Get-Date }
    }
    Start-Sleep -Milliseconds 500
    foreach ($k in @($running.Keys)) {
        $r = $running[$k]
        $secs = ((Get-Date) - $r.T).TotalSeconds
        if ($r.P.HasExited) {
            $r.P.WaitForExit()
            Finish $r.C $r.P $false $secs
            $running.Remove($k)
        } elseif ($secs -gt $Timeout) {
            try { $r.P.Kill() } catch {}
            try { $r.P.WaitForExit(10000) | Out-Null } catch {}
            Finish $r.C $r.P $true $secs
            $running.Remove($k)
        }
    }
}
$fails = @($rows | Where-Object { -not $_.Ok })
$md = New-Object System.Text.StringBuilder
[void]$md.AppendLine("# Soak $(Get-Date -Format 'yyyy-MM-dd HH:mm') ($Set): $($rows.Count) runs, $($fails.Count) failed, $([int]((Get-Date) - $t0).TotalMinutes) min")
[void]$md.AppendLine("")
[void]$md.AppendLine("| run | result | s | last line / problems |")
[void]$md.AppendLine("|---|---|---|---|")
foreach ($r in ($rows | Sort-Object Name)) {
    [void]$md.AppendLine("| $($r.Name) | $(if ($r.Ok) { 'ok' } else { '**FAIL**' }) | $($r.Secs) | $(if ($r.Ok) { $r.Last } else { $r.Problems }) |")
}
[IO.File]::WriteAllText((Join-Path $Out "summary.md"), $md.ToString())
Write-Host ("soak: {0} runs, {1} failed (summary: {2})" -f $rows.Count, $fails.Count, (Join-Path $Out "summary.md"))
exit $fails.Count
