#!/usr/bin/env bash
#
# Install the MSYS2 packages required to build FOX 1.7.95 (and later fox2)
# on Windows with the MinGW-w64 toolchain.
#
# Run from the MSYS2 MINGW64 environment, for example:
#
#   C:\msys64\mingw64.exe
#   cd /e/progs/fox-toolkit-phase2
#   ./scripts/install-msys2-deps.sh
#
# Options:
#   --opengl   also install Mesa (GL/gl.h, GL/glu.h) for the GL examples
#   --upgrade  run a full "pacman -Syu" first (may close this shell)
#   --print    print the package list and exit
#   -h|--help  show this help
#
set -euo pipefail

MSYS_PACKAGES=(
  make
  autoconf-wrapper
  autoconf2.73
  automake-wrapper
  automake1.18
  libtool
  diffutils
  pkgconf
  )

MINGW_PACKAGES=(
  mingw-w64-x86_64-gcc
  mingw-w64-x86_64-binutils
  mingw-w64-x86_64-crt
  mingw-w64-x86_64-libwinpthread
  mingw-w64-x86_64-zlib
  mingw-w64-x86_64-bzip2
  mingw-w64-x86_64-xz
  mingw-w64-x86_64-libjpeg-turbo
  mingw-w64-x86_64-libpng
  mingw-w64-x86_64-libtiff
  mingw-w64-x86_64-libwebp
  mingw-w64-x86_64-openjpeg2
  mingw-w64-x86_64-libjxl
  )

OPENGL_PACKAGES=(
  mingw-w64-x86_64-mesa
  )

with_opengl=no
print_only=no
do_upgrade=no

usage(){
  sed -n '2,22p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
  }

while [ $# -gt 0 ]; do
  case "$1" in
    --opengl)  with_opengl=yes ;;
    --upgrade) do_upgrade=yes ;;
    --print)   print_only=yes ;;
    -h|--help) usage; exit 0 ;;
    *) echo "error: unknown option '$1'" >&2; usage >&2; exit 2 ;;
    esac
  shift
  done

packages=("${MSYS_PACKAGES[@]}" "${MINGW_PACKAGES[@]}")
if [ "$with_opengl" = yes ]; then
  packages+=("${OPENGL_PACKAGES[@]}")
  fi

if [ "$print_only" = yes ]; then
  printf '%s\n' "${packages[@]}"
  exit 0
  fi

if [ "${MSYSTEM-}" != "MINGW64" ]; then
  echo "error: run this script in the MSYS2 MINGW64 environment." >&2
  echo "       start C:\\msys64\\mingw64.exe, or from PowerShell run:" >&2
  echo "       .\\scripts\\build-fox1-mingw.ps1 -InstallDeps" >&2
  exit 1
  fi

if ! command -v pacman >/dev/null 2>&1; then
  echo "error: pacman not found; is this really an MSYS2 shell?" >&2
  exit 1
  fi

echo "==> refreshing package databases"
pacman -Sy --noconfirm --noprogressbar

if [ "$do_upgrade" = yes ]; then
  echo "==> full system upgrade (the shell may be closed, then rerun this script)"
  pacman -Syu --noconfirm --noprogressbar
  fi

pending=$(pacman -Qu 2>/dev/null | wc -l) || true
if [ "$pending" -gt 0 ]; then
  echo "error: $pending installed packages are out of date; installing new" >&2
  echo "       packages now would create a partial upgrade." >&2
  echo "       Run 'pacman -Syu' in the MSYS2 MINGW64 shell, reopen the shell," >&2
  echo "       then run this script again (or use --upgrade)." >&2
  exit 1
  fi

echo "==> installing ${#packages[@]} packages"
pacman -S --needed --noconfirm --noprogressbar "${packages[@]}"

echo
echo "==> toolchain"
g++ --version | head -n 1
make --version | head -n 1
autoconf --version | head -n 1
automake --version | head -n 1
libtool --version | head -n 1

echo
echo "==> library headers"
missing=0
has_header(){
  if [ -f "/mingw64/include/$1" ]; then
    return 0
    fi
  # openjpeg installs into a versioned subdirectory
  if [ "$1" = openjpeg.h ]; then
    ls /mingw64/include/openjpeg-*/openjpeg.h >/dev/null 2>&1 && return 0
    fi
  return 1
  }
for inc in zlib.h bzlib.h lzma.h jpeglib.h png.h tiff.h webp/decode.h openjpeg.h jxl/decode.h; do
  if has_header "$inc"; then
    printf '  ok      %s\n' "$inc"
  else
    printf '  missing %s\n' "$inc"
    missing=1
    fi
  done
if [ "$with_opengl" = yes ]; then
  for inc in GL/gl.h GL/glu.h; do
    if [ -f "/mingw64/include/$inc" ]; then
      printf '  ok      %s\n' "$inc"
    else
      printf '  missing %s\n' "$inc"
      missing=1
      fi
    done
  fi

echo
if [ "$missing" != 0 ]; then
  echo "warning: some headers are missing; check the pacman output above." >&2
  exit 1
  fi

echo "done. Next: ./scripts/build-fox1-mingw.sh"
