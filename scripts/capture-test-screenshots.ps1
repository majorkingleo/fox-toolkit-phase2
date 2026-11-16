# Store one screenshot per GUI test program on Windows.
#
#   .\scripts\capture-test-screenshots.ps1                    # all programs
#   .\scripts\capture-test-screenshots.ps1 -Programs hello,button
#   .\scripts\capture-test-screenshots.ps1 -Delay 3           # slower start
#
# Each program is started with --no-dialogs, given -Delay seconds to open and
# paint its first window, captured with PrintWindow (the window is read from
# the compositor, so other windows on top do not end up in the image) and
# stopped again. The images go to screenshots/tests/fox2/win32/<program>.png.
# A program that shows no window within -Timeout seconds is reported and
# skipped, and the script returns the number of such programs. The smoke tests
# (bmpicon, textspin, combo, colordlg, fontdlg) end at once and are not listed.
param(
  [string[]]$Programs = @('hello','button','dpi','popup','click','menus','tooltip','clipboard','dnd','icon','hello2','layout','ratio','header','splitter','foursplit','gaugetest','switcher','tabbook','groupbox','dialog','wizard','datatarget','calendar','dirlist','iconlist','table','image','imageviewer','bitmapviewer','scribble','dctest','mditest','shutter'),
  [string]$BuildDir,
  [string]$OutDir,
  [double]$Delay = 1.5,
  [double]$Timeout = 10,
  [string[]]$Click = @(),
  [string]$Suffix = ''
  )

$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
if (-not $BuildDir) { $BuildDir = Join-Path $root 'tmp\build-fox2-win-cmake' }
if (-not $OutDir) { $OutDir = Join-Path $root 'screenshots\tests\fox2\win32' }
$bin = Join-Path $BuildDir 'bin'
$run = Join-Path $root 'tmp\shots-run'
New-Item -ItemType Directory -Path $OutDir,$run -Force | Out-Null

Add-Type -AssemblyName System.Drawing
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class ShotWin {
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
  [DllImport("user32.dll")] public static extern bool SetProcessDpiAwarenessContext(IntPtr ctx);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint flags);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
  [DllImport("user32.dll")] public static extern bool ScreenToClient(IntPtr h, ref POINT p);
  [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
  [DllImport("user32.dll")] public static extern IntPtr ChildWindowFromPointEx(IntPtr h, POINT p, uint flags);
  [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr h, uint msg, IntPtr w, IntPtr l);
  // Press and release the left button at screen point (x,y) by sending the
  // messages to the deepest child window there; works without an input desktop
  public static void Click(IntPtr top, int x, int y) {
    IntPtr h = top;
    POINT p; p.X = x; p.Y = y;
    for (;;) {
      POINT c = p; ScreenToClient(h, ref c);
      IntPtr child = ChildWindowFromPointEx(h, c, 0);
      if (child == IntPtr.Zero || child == h) break;
      h = child;
      }
    POINT q = p; ScreenToClient(h, ref q);
    IntPtr l = (IntPtr)((q.Y << 16) | (q.X & 0xFFFF));
    SendMessage(h, 0x200, IntPtr.Zero, l);              // WM_MOUSEMOVE
    SendMessage(h, 0x201, (IntPtr)1, l);                // WM_LBUTTONDOWN
    SendMessage(h, 0x202, IntPtr.Zero, l);              // WM_LBUTTONUP
    }
  [DllImport("dwmapi.dll")] public static extern int DwmGetWindowAttribute(IntPtr h, int attr, out RECT r, int size);
  }
'@
# per-monitor v2, so the window rectangle and the DWM bounds use the same pixels on every monitor
[void][ShotWin]::SetProcessDpiAwarenessContext([IntPtr]-4)

# Save the window h as a PNG; returns false when the window has no size
function Save-Window([IntPtr]$h,[string]$file){
  $r = New-Object ShotWin+RECT
  [void][ShotWin]::GetWindowRect($h,[ref]$r)
  $w = $r.Right - $r.Left
  $hh = $r.Bottom - $r.Top
  if ($w -le 0 -or $hh -le 0) { return $false }
  $bmp = New-Object System.Drawing.Bitmap $w,$hh
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $dc = $g.GetHdc()
  $ok = [ShotWin]::PrintWindow($h,$dc,2)     # PW_RENDERFULLCONTENT
  $g.ReleaseHdc($dc)
  $g.Dispose()
  if ($ok) {
    # The window rectangle includes the invisible resize frame of Windows 10
    # and 11; cut it off with the visible bounds
    $v = New-Object ShotWin+RECT
    $rect = New-Object System.Drawing.Rectangle 0,0,$w,$hh
    if ([ShotWin]::DwmGetWindowAttribute($h,9,[ref]$v,16) -eq 0) {   # DWMWA_EXTENDED_FRAME_BOUNDS
      $rect = New-Object System.Drawing.Rectangle ($v.Left-$r.Left),($v.Top-$r.Top),($v.Right-$v.Left),($v.Bottom-$v.Top)
      }
    $crop = $bmp.Clone($rect,$bmp.PixelFormat)
    $crop.Save($file,[System.Drawing.Imaging.ImageFormat]::Png)
    $crop.Dispose()
    }
  $bmp.Dispose()
  return $ok
  }

$env:PATH = $bin + ';' + $env:PATH
$failed = 0
foreach ($prog in $Programs) {
  $exe = Join-Path $bin "$prog.exe"
  if (-not (Test-Path -LiteralPath $exe)) {
    Write-Host "skip  $prog (no $exe)"
    $failed++
    continue
    }
  $p = Start-Process -FilePath $exe -ArgumentList '--no-dialogs' -WorkingDirectory $run -PassThru
  $h = [IntPtr]::Zero
  $limit = (Get-Date).AddSeconds($Timeout)
  while ((Get-Date) -lt $limit -and -not $p.HasExited) {
    $p.Refresh()
    $h = $p.MainWindowHandle
    if ($h -ne [IntPtr]::Zero -and [ShotWin]::IsWindowVisible($h)) { break }
    Start-Sleep -Milliseconds 100
    }
  if ($h -eq [IntPtr]::Zero -or $p.HasExited) {
    Write-Host "FAIL  $prog (no window, exit=$(if ($p.HasExited) { $p.ExitCode } else { 'running' }))"
    if (-not $p.HasExited) { $p.Kill() }
    $failed++
    continue
    }
  Start-Sleep -Seconds $Delay
  $p.Refresh()
  $h = $p.MainWindowHandle
  # -Click 'x,y' presses the left button at x,y of the captured picture (one point per entry, in order)
  foreach ($c in $Click) {
    $xy = $c.Split(',')
    $v = New-Object ShotWin+RECT
    [void][ShotWin]::DwmGetWindowAttribute($h,9,[ref]$v,16)
    [void][ShotWin]::SetForegroundWindow($h)
    [ShotWin]::Click($h,$v.Left + [int]$xy[0],$v.Top + [int]$xy[1])
    Start-Sleep -Milliseconds 800
    }
  $file = Join-Path $OutDir "$prog$Suffix.png"
  if (Save-Window $h $file) {
    Write-Host "saved $prog$Suffix.png"
    }
  else {
    Write-Host "FAIL  $prog (capture)"
    $failed++
    }
  if (-not $p.HasExited) { $p.Kill() }
  $p.WaitForExit()
  }
exit $failed
