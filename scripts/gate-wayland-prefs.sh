#!/usr/bin/env bash
#
# Preferences gate: the first frame of the preferences dialog is complete.
#
# The dialog is a switcher with three pages.  A repaint that was queued while a
# page was still visible used to be dispatched after the page was hidden, which
# painted the colour page on top of the settings page in the very first frame;
# only a later repaint made it right.  The gate opens the dialog, keeps the
# first committed frame of its surface, and compares its content area with the
# X11 reference: the colour wells of the other page would be thousands of
# differing pixels.  The small budget is the text antialiasing difference
# between the two compositors.
#
#   ./scripts/gate-wayland-prefs.sh
#   ./scripts/gate-wayland-prefs.sh --bin=FILE --build=DIR
#
# Exit status is 0 when the first frame is complete, 1 when it is not, 2 when
# the harness could not start.
#
set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

BUILD=tmp/build-fox2-wayland
BIN=
RUN=tmp/sway-prefs-gate
OUT=tmp/wlxinject
SRC=tools/wlxinject.c
XML=scripts/protocols/wlr-virtual-pointer-unstable-v1.xml
REF=screenshots/tests/fox1/x11/calculator-prefs-calculator.png
BUDGET=200

usage(){
  sed -n '2,21p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
  }

while [ $# -gt 0 ]; do
  case "$1" in
    --bin)     shift; BIN=$1 ;;
    --bin=*)   BIN=${1#*=} ;;
    --build)   shift; BUILD=$1 ;;
    --build=*) BUILD=${1#*=} ;;
    --run-dir) shift; RUN=$1 ;;
    --run-dir=*) RUN=${1#*=} ;;
    --ref)     shift; REF=$1 ;;
    --ref=*)   REF=${1#*=} ;;
    -h|--help) usage; exit 0 ;;
    *) echo "error: unknown option '$1'" >&2; usage >&2; exit 2 ;;
    esac
  shift
  done

[ -n "$BIN" ] || BIN="$BUILD/bin/calculator"
if [ ! -x "$BIN" ]; then
  echo "error: no binary at $BIN (build with scripts/build-fox2-wayland.sh)" >&2
  exit 2
  fi
for tool in sway swaymsg wayland-scanner cc; do
  command -v "$tool" >/dev/null 2>&1 || { echo "error: $tool not found" >&2; exit 2; }
  done
if ! python3 -c 'import PIL' >/dev/null 2>&1; then
  echo "error: python3 with Pillow is needed to compare the frames" >&2
  exit 2
  fi
[ -f "$REF" ] || { echo "error: reference image $REF is missing" >&2; exit 2; }

mkdir -p -- "$OUT"
wayland-scanner private-code "$XML" "$OUT/vptr-protocol.c" || exit 2
wayland-scanner client-header "$XML" "$OUT/wlr-virtual-pointer-unstable-v1-client-protocol.h" || exit 2
cc -O1 -Wall -I"$OUT" -o "$OUT/wlxinject" "$SRC" "$OUT/vptr-protocol.c" -lwayland-client || exit 2

rm -rf -- "$RUN"
mkdir -p -- "$RUN/home" "$RUN/dump"
RUN=$(cd -- "$RUN" && pwd)
chmod 700 "$RUN"
printf '%s\n' \
  'output HEADLESS-1 resolution 1024x640 bg #101010 solid_color' \
  'default_border pixel 0' \
  'for_window [title="Calculator Preferences"] floating enable' >"$RUN/config"

export XDG_RUNTIME_DIR="$RUN"
export HOME="$RUN/home"
unset DISPLAY WAYLAND_DISPLAY DBUS_SESSION_BUS_ADDRESS DBUS_SYSTEM_BUS_ADDRESS XDG_CONFIG_HOME SWAYSOCK
export WLR_BACKENDS=headless WLR_LIBINPUT_NO_DEVICES=1 WLR_RENDERER=pixman WLR_HEADLESS_OUTPUTS=1
export WLXINJECT_WIDTH=1024 WLXINJECT_HEIGHT=640

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
export SWAYSOCK=$(ls "$RUN"/sway-ipc.*.sock 2>/dev/null | head -1)

echo "==> running $BIN"
FOX_WAYLAND_DUMP="$RUN/dump" "$BIN" >"$RUN/app.log" 2>&1 &
app_pid=$!
sleep 2
kill -0 "$app_pid" 2>/dev/null || { echo "error: the application did not stay up" >&2; exit 2; }

swaymsg '[title="FOX Calculator"] floating enable' >/dev/null 2>&1
sleep 0.6

rect=$(swaymsg -t get_tree | python3 -c "
import json,sys
t=json.load(sys.stdin)
def walk(n):
    if n.get('name')=='FOX Calculator':
        r=n['rect']; print('%d %d %d %d'%(r['x'],r['y'],r['width'],r['height']))
    for c in n.get('nodes',[])+n.get('floating_nodes',[]): walk(c)
walk(t)")
[ -n "$rect" ] || { echo "error: no calculator window" >&2; exit 2; }
set -- $rect
wx=$1; wy=$2

echo "==> opening the preferences"
"$OUT/wlxinject" "$((wx+300))" "$((wy+20))" move sleep:300 \
                 "move:$((wx+60)),$((wy+25))" sleep:300 down sleep:300 up sleep:600 >/dev/null 2>&1

# Keep the first committed frame of the dialog surface
frame=""
last=""
for i in $(seq 1 200); do
  frame=$(ls "$RUN"/dump/window-*-661x295.ppm 2>/dev/null | head -1)
  if [ -n "$frame" ]; then
    cur=$(stat -c %y "$frame" 2>/dev/null || true)
    if [ -n "$cur" ] && [ "$cur" != "$last" ]; then
      last="$cur"
      cp "$frame" "$RUN/first.ppm"
      break
    fi
  fi
  sleep 0.02
  done
[ -s "$RUN/first.ppm" ] || { echo "error: the preferences dialog committed no frame" >&2; exit 2; }

kill -0 "$app_pid" 2>/dev/null || { echo "error: the application ended" >&2; exit 1; }

echo "==> comparing the first frame with $REF"
python3 - "$RUN/first.ppm" "$REF" "$BUDGET" <<'EOF'
import sys
from PIL import Image
frame, ref, budget = sys.argv[1], sys.argv[2], int(sys.argv[3])
a = Image.open(frame).convert("RGB")
b = Image.open(ref).convert("RGB")
if a.size != b.size:
    print(f"FAIL: frame {a.width}x{a.height}, reference {b.width}x{b.height}")
    sys.exit(1)
# The content area: skip the left button column (hover) and the Close button
# (focus ring), both of which depend on where the pointer is
box = (70, 40, 661, 245)
pa, pb = a.crop(box).load(), b.crop(box).load()
w, h = box[2]-box[0], box[3]-box[1]
differing = 0
for y in range(h):
    for x in range(w):
        if pa[x, y] != pb[x, y]:
            differing += 1
print(f"differing {differing} of {w*h} content pixels (budget {budget})")
sys.exit(0 if differing <= budget else 1)
EOF
verdict=$?
if [ "$verdict" -ne 0 ]; then
  echo "FAIL: the first frame is not complete (a hidden page was painted over it)" >&2
  exit 1
  fi
echo "PASS: the first preferences frame shows only the settings page"
exit 0
