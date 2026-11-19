---
name: build-win32
description: Build the fox2 Win32 target on Windows with MSYS2/MinGW-w64 installed at D:\msys64, using the scripts in scripts/. Use when asked to build, rebuild, clean-build or run the Win32 library, tests and tools, or when a Win32 build script exits silently or complains about MSYSTEM.
---

# Build the Win32 target (MSYS2 MinGW64 at D:\msys64)

Use the scripts in `scripts/`; do not call cmake, make or g++ by hand.

| Script | Purpose |
|---|---|
| `scripts/build-fox2-mingw.ps1` | PowerShell entry point: starts MSYS2 bash with the right environment |
| `scripts/build-fox2-mingw.sh`  | The real build driver (Autotools or CMake+Ninja) |
| `scripts/install-msys2-deps.sh` | Installs the MinGW packages (run via `-InstallDeps`) |

## Run it from PowerShell (the supported way)

MSYS2 is not in the default `C:\msys64` location, so point the wrapper to it
first. The wrapper sets `MSYSTEM=MINGW64` and `CHERE_INVOKING=1`, which the
`.sh` script requires.

```powershell
$env:MSYS2_ROOT='D:\msys64'
.\scripts\build-fox2-mingw.ps1 -Release -CMake
```

Common switches (all combinable): `-CMake` (CMake+Ninja, no configure),
`-Release` / `-Debug`, `-Clean`, `-Fast`, `-Trace` (not with `-Release`),
`-OpenGL`, `-Run` (starts hello and the DPI diagnostic), `-InstallDeps`,
`-Install -Prefix /usr/local -Destdir DIR`, `-Jobs N`.

First time on a machine: add `-InstallDeps`.

If the PowerShell policy blocks the script:
`powershell -ExecutionPolicy Bypass -File .\scripts\build-fox2-mingw.ps1 ...`

## Output

| Mode | Build dir | Log |
|---|---|---|
| `-CMake` | `tmp\build-fox2-win-cmake` (`bin\*.exe`, `bin\libFOX-2.0.dll`, `lib\libFOX-2.0.dll.a`) | `build.log` in that dir |
| Autotools (default) | `tmp\build-fox2-win` (`lib\.libs`, `tests\.libs`; builds hello, button, dpi only) | `build.log` in that dir |

The CMake mode also copies the needed MinGW runtime DLLs into `bin\`, so the
programs start without MSYS2 on the PATH. Success ends with
`done. Build log: ...`; verify the exe files exist afterwards.

## Pitfalls

- **Do not run `D:\msys64\mingw64.exe -c "./scripts/..."` from the agent shell.**
  It opens a separate mintty window, returns exit code 0 immediately, gives no
  output and builds nothing. Use the `.ps1` wrapper, or call bash directly:
  ```powershell
  $env:MSYSTEM='MINGW64'; $env:CHERE_INVOKING='1'
  D:\msys64\usr\bin\bash.exe -lc "./scripts/build-fox2-mingw.sh --release --cmake"
  ```
- Running the `.sh` without `MSYSTEM=MINGW64` fails with
  "run this script in the MSYS2 MINGW64 environment".
- `g++`/`cmake`/`ninja`/`make` not found: rerun with `-InstallDeps`.
- `--trace` and `--release` cannot be combined.
- Build output goes only to `tmp\`; do not commit it. Do not modify
  `fox-1.7.95`; all changes belong in `fox2`.
- For a Win32 build of the FOX 1.x origin use `scripts/build-fox1-mingw.ps1`
  the same way (with `MSYS2_ROOT` set).
- Cross-compiling from Linux is a different script: `scripts/build-fox2-mingw-cross.sh`.
