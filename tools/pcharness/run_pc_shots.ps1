# run_pc_shots.ps1 - runs the PC harness (run_pc.ps1, one THOTD at a time) and saves PrintWindow captures of the game
# window every -Every ticks (pc_tNNNNN.png in -Out; no focus change). The capture is the window client area (scaled
# by dgVoodoo and the desktop DPI). Used for the PC look comparisons (docs/teaser_status/render.md).
#   .\run_pc_shots.ps1 -Out "<folder inside private>" [-Ticks 3300] [-Every 100] [-Script game_aim.txt] [-Timeout 400] [-Mortal]
#   -Mortal: no -InfLives (the player can die: CONTINUE / GAME OVER captures)
#   -EverySeconds N (menuart): a capture every N seconds of wall time instead (pc_sNNNN.png): the front-end menus run
#   without game ticks, so the tick trigger never fires there
param([string]$Out, [int]$Ticks = 3300, [int]$Every = 100, [string]$Script = "game_aim.txt", [int]$Timeout = 400, [switch]$Mortal,
      [int]$EverySeconds = 0)

$wt = Split-Path (Split-Path $PSScriptRoot)
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System; using System.Drawing; using System.Runtime.InteropServices;
public static class Cap {
  [DllImport("user32.dll")] static extern bool PrintWindow(IntPtr h, IntPtr dc, uint f);
  [DllImport("user32.dll")] static extern bool GetClientRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
  public struct RECT { public int l, t, r, b; }
  public static bool Save(IntPtr h, string path) {
    RECT r; GetClientRect(h, out r); int w = r.r - r.l, ht = r.b - r.t; if (w <= 0 || ht <= 0) return false;
    using (var bmp = new Bitmap(w, ht)) { using (var g = Graphics.FromImage(bmp)) { var dc = g.GetHdc(); PrintWindow(h, dc, 3); g.ReleaseHdc(dc); }
      bmp.Save(path, System.Drawing.Imaging.ImageFormat.Png); }
    return true; }
  // the game has two visible top-level windows (its own and the one dgVoodoo presents in); MainWindowHandle may be
  // the game's own, which captures white (pcpolish3, 2026-10-17): every visible window of the process is captured
  // and the one with the most varied pixels is kept
  delegate bool EnumProc(IntPtr h, IntPtr lp);
  [DllImport("user32.dll")] static extern bool EnumWindows(EnumProc f, IntPtr lp);
  [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  public static bool SaveBest(int pid, string path) {
    var hs = new System.Collections.Generic.List<IntPtr>();
    EnumWindows((h, lp) => { uint p; GetWindowThreadProcessId(h, out p); if (p == (uint)pid && IsWindowVisible(h)) hs.Add(h); return true; }, IntPtr.Zero);
    Bitmap best = null; long bestScore = -1;
    foreach (var h in hs) {
      RECT r; GetClientRect(h, out r); int w = r.r - r.l, ht = r.b - r.t; if (w <= 0 || ht <= 0) continue;
      var bmp = new Bitmap(w, ht);
      using (var g = Graphics.FromImage(bmp)) { var dc = g.GetHdc(); PrintWindow(h, dc, 3); g.ReleaseHdc(dc); }
      long score = 0; int lo = 255, hi = 0;
      for (int y = 0; y < ht; y += Math.Max(1, ht / 32)) for (int x = 0; x < w; x += Math.Max(1, w / 32)) {
        var c = bmp.GetPixel(x, y); int v = (c.R + c.G + c.B) / 3; if (v < lo) lo = v; if (v > hi) hi = v; }
      score = hi - lo;
      if (score > bestScore) { if (best != null) best.Dispose(); best = bmp; bestScore = score; } else bmp.Dispose();
    }
    if (best == null) return false;
    best.Save(path, System.Drawing.Imaging.ImageFormat.Png); best.Dispose();
    return true; }
}
'@
# the client rectangle in physical pixels: without this a scaled desktop gave a DPI-virtualized (smaller) rectangle
# and the captures lost the right / bottom of the game (the HUD) (pcpolish, 2026-10-16)
[void][Cap]::SetProcessDPIAware()
New-Item -ItemType Directory -Force $Out | Out-Null
$job = Start-Job -ScriptBlock { param($wt, $out, $t, $s, $to, $inf) & "$wt\tools\pcharness\run_pc.ps1" -Out $out -Ticks $t -InputScript "$wt\tools\pcharness\scripts\$s" -InfLives:$inf -TimeoutSeconds $to } -ArgumentList $wt, $Out, $Ticks, $Script, $Timeout, (-not $Mortal)
$next = $(if ($EverySeconds -gt 0) { $EverySeconds } else { $Every }); $log = Join-Path $Out "pc.log"; $t0 = Get-Date
while ($job.State -eq "Running" -and ((Get-Date) - $t0).TotalSeconds -lt $Timeout + 30) {
  Start-Sleep -Milliseconds 50
  if ($EverySeconds -gt 0) {
    $sec = [int]((Get-Date) - $t0).TotalSeconds
    if ($sec -ge $next) {
      $p = Get-Process THOTD -ErrorAction SilentlyContinue | Select-Object -First 1
      if ($p) { [void][Cap]::SaveBest($p.Id, (Join-Path $Out ("pc_s{0:D4}.png" -f $sec))) }
      $next = $sec + $EverySeconds
    }
    continue
  }
  if (-not (Test-Path $log)) { continue }
  $last = Get-Content $log -Tail 1 -ErrorAction SilentlyContinue
  if ($last -match '^\s*(?:tick\s*)?(\d+)') { $tick = [int]$matches[1] } else { continue }
  if ($tick -ge $next) {
    $p = Get-Process THOTD -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($p) { [void][Cap]::SaveBest($p.Id, (Join-Path $Out ("pc_t{0:D5}.png" -f $tick))) }
    $next = $tick - ($tick % $Every) + $Every
  }
}
Receive-Job $job | Select-Object -Last 3
