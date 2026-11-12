#!/usr/bin/env bash
#
# Install the packages needed to cross-compile fox2 for Windows with
# MinGW-w64 on CachyOS/Arch Linux, and optionally Wine for smoke tests.
#
#   ./scripts/install-cachyos-mingw.sh
#
# Options:
#   --codecs     also install the optional image and compression libraries (AUR)
#   --opengl     also install Mesa for the GL examples (AUR)
#   --no-wine    do not install Wine even when it is missing
#   --print      print the package lists and exit
#   -h|--help    show this help
#
set -euo pipefail

HOST_TOOLS=(
  make
  autoconf
  automake
  libtool
  pkgconf
  patch
  )

TOOLCHAIN=(
  mingw-w64-gcc
  mingw-w64-binutils
  mingw-w64-crt
  mingw-w64-headers
  mingw-w64-winpthreads
  )

# Not in the official repositories; these come from the AUR.
CODECS=(
  mingw-w64-zlib
  mingw-w64-bzip2
  mingw-w64-xz
  mingw-w64-libjpeg-turbo
  mingw-w64-libpng
  mingw-w64-libtiff
  mingw-w64-libjxl
  )

OPENGL=(
  mingw-w64-mesa
  )

WINE=(
  wine
  )

with_codecs=no
with_opengl=no
with_wine=yes
print_only=no

usage(){
  sed -n '2,14p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
  }

while [ $# -gt 0 ]; do
  case "$1" in
    --codecs)  with_codecs=yes ;;
    --opengl)  with_opengl=yes ;;
    --no-wine) with_wine=no ;;
    --print)   print_only=yes ;;
    -h|--help) usage; exit 0 ;;
    *) echo "error: unknown option '$1'" >&2; usage >&2; exit 2 ;;
    esac
  shift
  done

if [ "$print_only" = yes ]; then
  printf 'pacman: %s\n' "${HOST_TOOLS[*]} ${TOOLCHAIN[*]}"
  [ "$with_codecs" = yes ] && printf 'aur:    %s\n' "${CODECS[*]}"
  [ "$with_opengl" = yes ] && printf 'aur:    %s\n' "${OPENGL[*]}"
  [ "$with_wine" = yes ] && printf 'pacman: %s (only when Wine is missing)\n' "${WINE[*]}"
  exit 0
  fi

if ! command -v pacman >/dev/null 2>&1; then
  echo "error: pacman not found; this script is for CachyOS/Arch Linux." >&2
  exit 1
  fi

if ! grep -qi '^ID_LIKE=.*arch' /etc/os-release 2>/dev/null &&
   ! grep -qi '^ID=.*\(arch\|cachyos\)' /etc/os-release 2>/dev/null; then
  echo "warning: /etc/os-release does not look like Arch; continuing anyway." >&2
  fi

SUDO=()
if [ "$(id -u)" -ne 0 ]; then
  command -v sudo >/dev/null 2>&1 || {
    echo "error: not root and sudo not found." >&2
    exit 1
    }
  SUDO=(sudo)
  fi

# AUR helper for the optional libraries
aur_helper=
for helper in paru yay; do
  if command -v "$helper" >/dev/null 2>&1; then
    aur_helper=$helper
    break
    fi
  done
need_aur=no
[ "$with_codecs" = yes ] && need_aur=yes
[ "$with_opengl" = yes ] && need_aur=yes
if [ "$need_aur" = yes ] && [ -z "$aur_helper" ]; then
  echo "error: --codecs/--opengl need an AUR helper (paru or yay)." >&2
  echo "       install one, or run without those options." >&2
  exit 1
  fi

echo "==> refreshing package databases"
"${SUDO[@]}" pacman -Sy --noconfirm --noprogressbar

echo "==> installing host build tools"
"${SUDO[@]}" pacman -S --needed --noconfirm --noprogressbar "${HOST_TOOLS[@]}"

echo "==> installing the MinGW-w64 cross toolchain"
"${SUDO[@]}" pacman -S --needed --noconfirm --noprogressbar "${TOOLCHAIN[@]}"

if [ "$with_codecs" = yes ]; then
  echo "==> installing the optional image and compression libraries (AUR)"
  "$aur_helper" -S --needed --noconfirm "${CODECS[@]}"
  fi

if [ "$with_opengl" = yes ]; then
  echo "==> installing Mesa for MinGW (AUR)"
  "$aur_helper" -S --needed --noconfirm "${OPENGL[@]}"
  fi

if command -v wine >/dev/null 2>&1; then
  echo "==> Wine already present: $(command -v wine)"
elif [ "$with_wine" = yes ]; then
  echo "==> installing Wine for the smoke tests"
  # wine-cachyos and wine conflict; only reached when no wine is installed
  "${SUDO[@]}" pacman -S --needed --noconfirm --noprogressbar "${WINE[@]}"
  fi

echo
echo "==> toolchain"
x86_64-w64-mingw32-g++ --version | head -n 1
x86_64-w64-mingw32-windres --version | head -n 1
command -v wine >/dev/null 2>&1 && wine --version | head -n 1 || true

echo
echo "==> target headers in /usr/x86_64-w64-mingw32/include"
missing=0
for inc in zlib.h bzlib.h lzma.h jpeglib.h png.h tiff.h jxl/decode.h GL/gl.h; do
  if [ -f "/usr/x86_64-w64-mingw32/include/$inc" ]; then
    printf '  ok      %s\n' "$inc"
  else
    printf '  missing %s\n' "$inc"
    missing=1
    fi
  done

echo
if [ "$missing" != 0 ]; then
  echo "note: missing headers are optional; they disable the matching"
  echo "      image/compression formats and OpenGL.  Install them with"
  echo "      --codecs and --opengl."
  fi

echo "done. Next: ./scripts/build-fox2-mingw-cross.sh --run"
