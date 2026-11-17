#!/usr/bin/env bash
#
# Themed palette gate:  nothing is left over from the classic palette.
#
# Every widget that still paints from its constructor-time colours instead of the
# theme shows up as a patch of the classic Win95-style palette inside an
# otherwise flat window: #D4D0C8 for a surface, #8E8B86 for a shadow and #E2E0DA
# for a highlight.  This gate runs a few of the layouts that exercise most
# controls with a themed registry and requires those three colours to be absent
# from the frames.
#
# The widgets found this way so far: packers that drew their own bevel, the
# status bar separator, the shutter item content and the console.
#
#   ./scripts/gate-wayland-theme-palette.sh
#   ./scripts/gate-wayland-theme-palette.sh --theme=/usr/share/themes/Adwaita
#
set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

BUILD=tmp/build-fox2-wayland
RUN=tmp/sway-theme-palette
THEME=/usr/share/themes/Breeze
# The colour a themed surface uses for shadows and edges; a large area of it
# means a widget filled with it where a hairline belongs
SHADOW=${SHADOW:-A0A1A1}
SHADOW_LIMIT=${SHADOW_LIMIT:-5000}
APPS=${APPS:-"header table splitter shutter ControlPanel button tabbook switcher dialog colordemo rulerdemo calendar"}

usage(){
  sed -n '2,18p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
  }

while [ $# -gt 0 ]; do
  case "$1" in
    --build)   shift; BUILD=$1 ;;
    --build=*) BUILD=${1#*=} ;;
    --run-dir) shift; RUN=$1 ;;
    --run-dir=*) RUN=${1#*=} ;;
    --theme)   shift; THEME=$1 ;;
    --theme=*) THEME=${1#*=} ;;
    --shadow)  shift; SHADOW=$1 ;;
    --shadow=*) SHADOW=${1#*=} ;;
    --shadow-limit) shift; SHADOW_LIMIT=$1 ;;
    --shadow-limit=*) SHADOW_LIMIT=${1#*=} ;;
    -h|--help) usage; exit 0 ;;
    *) echo "error: unknown option '$1'" >&2; usage >&2; exit 2 ;;
    esac
  shift
  done

for tool in sway wayland-scanner python3; do
  command -v "$tool" >/dev/null 2>&1 || { echo "error: $tool not found" >&2; exit 2; }
  done
[ -d "$THEME" ] || { echo "error: no theme at $THEME" >&2; exit 2; }

rm -rf -- "$RUN"
mkdir -p -- "$RUN/home" "$RUN/registry"
RUN=$(cd -- "$RUN" && pwd)
chmod 700 "$RUN"
printf '[SETTINGS]\ntheme=%s\n[display]\nscale=100\n' "$THEME" >"$RUN/registry/fox.rc"
echo 'output HEADLESS-1 resolution 1280x800 bg #101010 solid_color' >"$RUN/config"

export XDG_RUNTIME_DIR="$RUN"
export HOME="$RUN/home"
unset DISPLAY WAYLAND_DISPLAY DBUS_SESSION_BUS_ADDRESS XDG_CONFIG_HOME SWAYSOCK
export WLR_BACKENDS=headless WLR_LIBINPUT_NO_DEVICES=1 WLR_RENDERER=pixman WLR_HEADLESS_OUTPUTS=1

sway --unsupported-gpu -c "$RUN/config" >"$RUN/sway.log" 2>&1 &
sway_pid=$!
cleanup(){
  kill "${app_pid:-}" 2>/dev/null || true
  kill "$sway_pid" 2>/dev/null || true
  wait "$sway_pid" 2>/dev/null || true
  }
trap cleanup EXIT INT TERM

sock=""
for i in $(seq 1 200); do
  sock=$(ls "$RUN"/wayland-* 2>/dev/null | head -1)
  [ -n "$sock" ] && [ -S "$sock" ] && break
  sleep 0.05
  done
[ -n "$sock" ] || { echo "error: no sway socket" >&2; exit 2; }
export WAYLAND_DISPLAY=$(basename "$sock")

failed=""
for app in $APPS; do
  bin="$BUILD/bin/$app"
  if [ ! -x "$bin" ]; then
    echo "error: no binary at $bin" >&2
    exit 2
    fi
  dump="$RUN/$app"
  mkdir -p -- "$dump"
  echo "==> $app with theme $THEME"
  FOXDIR="$RUN/registry" FOX_WAYLAND_DUMP="$dump" "$bin" >"$RUN/$app.log" 2>&1 &
  app_pid=$!
  sleep 2.0
  if ! kill -0 "$app_pid" 2>/dev/null; then
    echo "error: $app did not stay up" >&2
    app_pid=""
    exit 2
    fi
  sleep 0.5
  kill "$app_pid" 2>/dev/null
  wait "$app_pid" 2>/dev/null
  app_pid=""

  frame=$(ls -S "$dump"/window-*.ppm 2>/dev/null | head -1)
  [ -n "$frame" ] || { echo "error: $app committed no frame" >&2; exit 2; }

  counts=$(python3 - "$frame" "$SHADOW" <<'EOF'
import sys

# The three colours of the classic palette: surface, shadow and highlight, plus
# the surface shadow of the theme that is being imported
CLASSIC=('D4D0C8','8E8B86','E2E0DA',sys.argv[2].upper())

with open(sys.argv[1],'rb') as f:
    data=f.read()
# P6 width height maxval, then three bytes per pixel
parts=data.split(b'\n',3)
pixels=parts[3]
found={}
for i in range(0,len(pixels)-2,3):
    key='%02X%02X%02X'%(pixels[i],pixels[i+1],pixels[i+2])
    if key in CLASSIC:
        found[key]=found.get(key,0)+1
print(' '.join('%s=%d'%(k,found.get(k,0)) for k in CLASSIC))
EOF
)
  echo "    classic pixels: $counts"
  classic=$(printf '%s\n' "$counts" | tr ' ' '\n' | grep -v "^${SHADOW}=" | sed 's/.*=//' | paste -sd+ | bc)
  shadow=$(printf '%s\n' "$counts" | tr ' ' '\n' | sed -n "s/^${SHADOW}=//p")
  [ "${classic:-0}" = 0 ] || failed="$failed $app($counts)"
  [ "${shadow:-0}" -le "$SHADOW_LIMIT" ] || failed="$failed $app($counts, more than $SHADOW_LIMIT of ${SHADOW})"
  done

if [ -n "$failed" ]; then
  echo "FAIL: the classic palette is still painted in:$failed"
  exit 1
  fi
echo "PASS: no classic palette colour appears in the themed frames"
exit 0
