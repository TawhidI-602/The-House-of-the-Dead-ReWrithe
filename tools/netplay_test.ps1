# Netplay test (docs/netplay.md): two headless hotd instances on 127.0.0.1, e.g.
#   tools\netplay_test.ps1 -Root <checkout with private\> -Name sim -HostArgs "--net-sim 10,20 --net-delay 8" -ClientArgs "--net-sim 10,20"
# (a 2-player game needs -HostArgs "--players 2"; without it player 1 plays alone and chapter 1 ends at 16140, not 15810)
param([string]$Exe = "$PSScriptRoot\..\build\Release\hotd.exe", [string]$Root = "$PSScriptRoot\..", [string]$Scratch = "$env:TEMP\hotd_netplay_test", [string]$Name = "t", [string]$HostArgs = "", [string]$ClientArgs = "", [string]$Common = "--frames 40000 --autofire --mute --lives-kept 1", [int]$Port = 47900, [string]$Code = "TEST-CODE", [string]$ClientCode = "", [string]$ClientDesync = "")
# two headless hotd instances on 127.0.0.1 (no window), each its own HOTD_PREF_DIR; prints the key lines of both
$exe = $Exe
$root = $Root
$s = $Scratch; New-Item -ItemType Directory -Force $s | Out-Null
if (-not $ClientCode) { $ClientCode = $Code }
foreach ($side in "host", "client") { New-Item -ItemType Directory -Force "$s\pref_${Name}_$side" | Out-Null }
$hostCmd = "--root `"$root`" $Common --host 127.0.0.1:$Port --code $Code $HostArgs"
$cliCmd = "--root `"$root`" $Common --join 127.0.0.1:$Port --code $ClientCode $ClientArgs"
$env:HOTD_PREF_DIR = "$s\pref_${Name}_host"
$h = Start-Process -FilePath $exe -ArgumentList $hostCmd -NoNewWindow -PassThru -RedirectStandardOutput "$s\${Name}_host.txt" -RedirectStandardError "$s\${Name}_host.err"
Start-Sleep -Milliseconds 500
$env:HOTD_PREF_DIR = "$s\pref_${Name}_client"
if ($ClientDesync) { $env:HOTD_NET_DESYNC_AT = $ClientDesync }
$c = Start-Process -FilePath $exe -ArgumentList $cliCmd -NoNewWindow -PassThru -RedirectStandardOutput "$s\${Name}_client.txt" -RedirectStandardError "$s\${Name}_client.err"
$null = $h.Handle; $null = $c.Handle   # (keeps the exit codes readable)
$t0 = Get-Date
$c.WaitForExit(); $h.WaitForExit(30000) | Out-Null
if (-not $h.HasExited) { $h.Kill(); "host killed (still running)" }
"{0}: {1:N1} s, exit host {2} client {3}" -f $Name, ((Get-Date) - $t0).TotalSeconds, $h.ExitCode, $c.ExitCode
foreach ($side in "host", "client") {
    "--- $side"
    Select-String -Path "$s\${Name}_$side.txt" -Pattern "^\[net\]|NET:|ended|netplay stopped|game over" | Select-Object -First 25 | % Line
    Select-String -Path "$s\${Name}_$side.err" -Pattern "netplay|rror" -ErrorAction SilentlyContinue | Select-Object -First 5 | % Line
}

