# Build the fox2 library on Windows through MSYS2/MinGW-w64.
#
#   .\scripts\build-fox2-mingw.ps1                  # build only
#   .\scripts\build-fox2-mingw.ps1 -InstallDeps     # install packages first
#   .\scripts\build-fox2-mingw.ps1 -OpenGL          # enable GL (needs Mesa)
#   .\scripts\build-fox2-mingw.ps1 -Clean -Run      # clean, build, run examples
#
# If script execution is blocked by the PowerShell policy, run:
#   powershell -ExecutionPolicy Bypass -File .\scripts\build-fox2-mingw.ps1
#
param(
  [switch]$InstallDeps,
  [switch]$OpenGL,
  [switch]$Debug,
  [switch]$Release,
  [switch]$Clean,
  [switch]$Run,
  [string]$Jobs
)

$ErrorActionPreference = 'Stop'

$msysRoot = if ($env:MSYS2_ROOT) { $env:MSYS2_ROOT } else { 'C:\msys64' }
$bash = Join-Path $msysRoot 'usr\bin\bash.exe'
if (-not (Test-Path -LiteralPath $bash)) {
  throw "MSYS2 bash not found at '$bash'. Set MSYS2_ROOT if MSYS2 lives elsewhere."
}

$root = Split-Path -Parent $PSScriptRoot

function ConvertTo-MsysPath {
  param([string]$Path)
  $full = [System.IO.Path]::GetFullPath($Path).Replace('\', '/')
  if ($full -match '^([A-Za-z]):/(.*)$') {
    return '/' + $matches[1].ToLowerInvariant() + '/' + $matches[2]
  }
  return $full
}

$rootMsys = ConvertTo-MsysPath $root

$env:MSYSTEM = 'MINGW64'
$env:CHERE_INVOKING = '1'

$installArgs = @()
$buildArgs = @()
if ($OpenGL) { $installArgs += '--opengl'; $buildArgs += '--opengl' }
if ($Debug) { $buildArgs += '--debug' }
if ($Release) { $buildArgs += '--release' }
if ($Clean) { $buildArgs += '--clean' }
if ($Run) { $buildArgs += '--run' }
if ($Jobs) { $buildArgs += "--jobs=$Jobs" }

if ($InstallDeps) {
  $line = "$rootMsys/scripts/install-msys2-deps.sh " + ($installArgs -join ' ')
  & $bash -lc $line
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

$line = "$rootMsys/scripts/build-fox2-mingw.sh " + ($buildArgs -join ' ')
& $bash -lc $line
exit $LASTEXITCODE
