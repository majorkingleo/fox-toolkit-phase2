#!/usr/bin/env bash
#
# Install GTK themes into the Wine prefix, so the Win32 build imports the same
# themes the Linux desktop uses.
#
# The themes are copied from the system theme directory into
# <prefix>/drive_c/ProgramData/themes, which is where a GTK installation on
# Windows keeps them.  The ControlPanel lists that folder, and the FOX importer
# reads gtk-3.0/gtk.css, gtk-4.0/gtk.css or gtk-2.0/gtkrc from there and caches
# the resolved theme in the prefix.
#
#   ./scripts/install-wine-gtk-themes.sh [theme ...]
#
# With no arguments Breeze and Breeze-Dark are installed.  To use one, write the
# theme into fox.ini, next to the folder that $FOXDIR points at:
#
#   [SETTINGS]
#   theme=C:/ProgramData/themes/Breeze
#
# Forward slashes work as they are.  With backslashes the value has to be
# quoted and doubled (theme="C:\\ProgramData\\themes\\Breeze"), because the
# settings parser reads a single backslash as an escape; a value written by the
# ControlPanel is quoted and escaped that way already.
#
# Environment:
#   WINEPREFIX        target prefix (default: tmp/wineprefix)
#   THEME_SOURCE_DIR  system theme directory (default: /usr/share/themes)
#
set -euo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
PFX=${WINEPREFIX:-$ROOT/tmp/wineprefix}
SRC=${THEME_SOURCE_DIR:-/usr/share/themes}
DEST="$PFX/drive_c/ProgramData/themes"

usage(){
  sed -n '2,27p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
  }

while [ $# -gt 0 ]; do
  case "$1" in
    -h|--help) usage; exit 0 ;;
    --)        shift; break ;;
    -*)        echo "error: unknown option '$1'" >&2; usage >&2; exit 2 ;;
    *)         break ;;
    esac
  done

[ $# -gt 0 ] || set -- Breeze Breeze-Dark

[ -d "$PFX" ] || {
  echo "error: $PFX is not a Wine prefix; run wineboot --init first." >&2
  exit 1
  }
[ -d "$SRC" ] || {
  echo "error: $SRC does not exist; set THEME_SOURCE_DIR." >&2
  exit 1
  }

mkdir -p -- "$DEST"

missing=0
for theme in "$@"; do
  if [ ! -d "$SRC/$theme" ]; then
    echo "warning: no theme '$theme' in $SRC" >&2
    missing=1
    continue
    fi
  rm -rf -- "${DEST:?}/$theme"
  cp -r --no-target-directory "$SRC/$theme" "$DEST/$theme"
  if [ -f "$DEST/$theme/gtk-3.0/gtk.css" ]; then
    echo "==> installed $theme (gtk-3.0/gtk.css)"
  elif [ -f "$DEST/$theme/gtk-4.0/gtk.css" ]; then
    echo "==> installed $theme (gtk-4.0/gtk.css)"
  elif [ -f "$DEST/$theme/gtk-2.0/gtkrc" ]; then
    echo "==> installed $theme (gtk-2.0/gtkrc)"
  else
    echo "warning: $theme has no gtk-3.0/gtk.css, gtk-4.0/gtk.css or gtk-2.0/gtkrc" >&2
    missing=1
    fi
  echo "    theme=C:/ProgramData/themes/$theme"
  done

echo
echo "themes in $DEST:"
ls -1 "$DEST" 2>/dev/null | sed 's/^/  /'
exit "$missing"
