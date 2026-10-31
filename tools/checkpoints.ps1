# checkpoints.ps1 - playability checkpoints: plays each run of tools\checkpoints.txt (the default route, a branch
# route, a bot that dies; the window opened without focus, screenshots offscreen) once per model source and saves a
# screenshot at each moment listed there (title card .. results, CONTINUE / game over), then writes a contact sheet
# with the MAME and real-PC reference frames (tools\checkpoint_sheet.py). Output under private\ only.
#   .\tools\checkpoints.ps1 [-Build build] [-Out private\checkpoints\<stamp>] [-Sources arcade,pc] [-Runs main,die]
#                           [-TimeoutMinutes 30]
param(
    [string]$Build = "build",
    [string]$Out = "",
    [string[]]$Sources = @("arcade", "pc"),
    [string[]]$Runs = @(),
    [double]$TimeoutMinutes = 30
)
$ErrorActionPreference = "Stop"
$repo = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$priv = Join-Path $repo "private"
if (-not (Test-Path -LiteralPath $priv)) {
    # a git worktree: private\ is in the main checkout
    $common = (& git -C $repo rev-parse --git-common-dir)
    if (-not [IO.Path]::IsPathRooted($common)) { $common = Join-Path $repo $common }
    $priv = Join-Path (Split-Path (Resolve-Path -LiteralPath $common).Path) "private"
}
$root = Split-Path $priv
if ($Out -eq "") { $Out = Join-Path $priv ("checkpoints\" + (Get-Date -Format "yyyyMMdd_HHmm")) }
if (-not [IO.Path]::IsPathRooted($Build)) { $Build = Join-Path $repo $Build }
$exe = Join-Path $Build "Release\hotd.exe"
$list = Join-Path $PSScriptRoot "checkpoints.txt"
# the runs of checkpoints.txt ("run <name> <hotd options>"; checkpoints before the first run line: the default run)
$runTable = [ordered]@{}
$cur = "main"
$runTable[$cur] = @{ Args = "--autofire --lives-kept 1"; Lines = @() }
foreach ($line in Get-Content -LiteralPath $list) {
    $t = $line.Trim()
    if ($t -eq "" -or $t.StartsWith("#")) { continue }
    if ($t -match '^run\s+(\S+)\s*(.*)$') {
        $cur = $Matches[1]
        $runTable[$cur] = @{ Args = $Matches[2]; Lines = @() }
        continue
    }
    $runTable[$cur].Lines += $t
}
foreach ($s in $Sources) {
    $dir = Join-Path $Out $s
    New-Item -ItemType Directory -Force $dir | Out-Null
    foreach ($r in $runTable.Keys) {
        if ($runTable[$r].Lines.Count -eq 0) { continue }
        if ($Runs.Count -gt 0 -and $Runs -notcontains $r) { continue }
        $rl = Join-Path $dir "run_$r.txt"
        Set-Content -LiteralPath $rl -Value $runTable[$r].Lines -Encoding ascii
        $hargs = @("--root", "`"$root`"", "--mute", "--models", $s) +
                 @($runTable[$r].Args -split '\s+' | Where-Object { $_ -ne "" }) +
                 @("--checkpoints", "`"$rl`"", "--shot-dir", "`"$dir`"")
        $p = Start-Process -FilePath $exe -ArgumentList $hargs -PassThru -NoNewWindow `
                           -RedirectStandardOutput "$dir\run_$r.log" -RedirectStandardError "$dir\run_${r}_err.log"
        if (-not $p.WaitForExit([int]($TimeoutMinutes * 60000))) {
            Stop-Process -Id $p.Id -Force
            "TIMEOUT: the $s run '$r' was stopped after $TimeoutMinutes minutes."
        }
        Get-Content "$dir\run_$r.log", "$dir\run_${r}_err.log" -ErrorAction SilentlyContinue |
            Select-String "checkpoint not reached|screenshot:"
    }
}
& py (Join-Path $PSScriptRoot "checkpoint_sheet.py") $list $Out (Join-Path $priv "mame_dumps") (Join-Path $priv "screenshots\pc_look\fw2")
