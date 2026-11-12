# Run the FOX DPI test app with the library on PATH and its own session log.
#
#   .\scripts\run-dpi-app.ps1                               # new timestamped log dir
#   .\scripts\run-dpi-app.ps1 -LogDir .\tmp\dpi-session     # fixed log dir
#
# The app appends to dpi-log.txt in its working directory; this script makes
# that directory $LogDir and prints it when the app exits.
param(
  [string]$LogDir
)

$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$build = Join-Path $root 'tmp\build-fox2-win'
$exe = Join-Path $build 'tests\dpi.exe'
if (-not (Test-Path -LiteralPath $exe)) {
  throw "dpi.exe not found; build first: .\scripts\build-fox2-mingw.ps1 -Fast"
  }
if (-not $LogDir) {
  $LogDir = Join-Path $root ('tmp\dpi-session-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
  }
New-Item -ItemType Directory -Path $LogDir -Force | Out-Null
$LogDir = (Resolve-Path -LiteralPath $LogDir).Path
$logFile = Join-Path $LogDir 'dpi-log.txt'

$env:PATH = (Join-Path $build 'lib\.libs') + ';C:\msys64\mingw64\bin;' + $env:PATH

Write-Host "FOX DPI test app"
Write-Host "  log file  : $logFile"
Write-Host "  press Snapshot in the app to mark each monitor state"

$proc = Start-Process -FilePath $exe -WorkingDirectory $LogDir -PassThru -Wait
Write-Host "  exit code : $($proc.ExitCode)"
if (Test-Path -LiteralPath $logFile) {
  Write-Host "  log lines : $((Get-Content -LiteralPath $logFile).Count)"
  }
