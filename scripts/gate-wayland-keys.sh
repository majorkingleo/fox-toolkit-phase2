#!/usr/bin/env bash
#
# Keyboard gate: keys reach the shell, not just the focused widget.
#
# The Wayland backend has to dispatch a key to the shell of the focus window,
# because the shell bounces it to the focus widget and then checks the
# accelerator table.  Sending it straight to the focus widget skips the
# accelerators, which is why the calculator's digit and operator hot keys did
# nothing.
#
# The gate runs the calculator on a headless sway, types "6/2=" with wtype and
# passes when the trace shows the keys arriving at the Calculator (the shell)
# and the display area of the committed frame changed from "0" to the result.
#
#   ./scripts/gate-wayland-keys.sh
#   ./scripts/gate-wayland-keys.sh --bin=FILE --build=DIR
#
# Exit status is 0 when the keys reached the shell and changed the display, 1
# when they did not, 2 when the harness could not start.
#
set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

BUILD=tmp/build-fox2-wayland
BIN=
RUN=tmp/sway-keys-gate

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
for tool in sway wtype; do
  command -v "$tool" >/dev/null 2>&1 || { echo "error: $tool not found" >&2; exit 2; }
  done
if ! python3 -c 'import PIL' >/dev/null 2>&1; then
  echo "error: python3 with Pillow is needed to compare the frames" >&2
  exit 2
  fi

rm -rf -- "$RUN"
mkdir -p -- "$RUN/home" "$RUN/dump"
RUN=$(cd -- "$RUN" && pwd)
chmod 700 "$RUN"
printf '%s\n' \
  'output HEADLESS-1 resolution 1024x640 bg #101010 solid_color' \
  'default_border pixel 0' >"$RUN/config"

export XDG_RUNTIME_DIR="$RUN"
export HOME="$RUN/home"
unset DISPLAY WAYLAND_DISPLAY DBUS_SESSION_BUS_ADDRESS DBUS_SYSTEM_BUS_ADDRESS XDG_CONFIG_HOME SWAYSOCK
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
export SWAYSOCK=$(ls "$RUN"/sway-ipc.*.sock 2>/dev/null | head -1)

echo "==> running $BIN"
FOX_WAYLAND_TRACE=1 FOX_WAYLAND_DUMP="$RUN/dump" "$BIN" >"$RUN/app.log" 2>&1 &
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
[ "$3" -eq 400 ] && [ "$4" -eq 260 ] || { echo "error: window is ${3}x${4}" >&2; exit 2; }

frame="$RUN/dump/window-2-400x260.ppm"
for i in $(seq 1 40); do
  [ -f "$frame" ] && break
  sleep 0.05
  done
[ -f "$frame" ] || { echo "error: no frame was committed" >&2; exit 2; }
cp "$frame" "$RUN/before.ppm"

echo "==> typing 6/2="
wtype -s 120 -k 6
sleep 0.25
wtype -s 120 -k slash
sleep 0.25
wtype -s 120 -k 2
sleep 0.25
wtype -s 120 -k equal
sleep 0.8

kill -0 "$app_pid" 2>/dev/null || { echo "error: the application ended" >&2; exit 1; }
cp "$frame" "$RUN/after.ppm"

grep -q "key press to Calculator " "$RUN/app.log" || {
  echo "FAIL: the keys did not reach the shell (the accelerators were skipped)" >&2
  grep -E "key press to " "$RUN/app.log" | tail -4 >&2
  exit 1
  }

grep -q "key press to FXButton " "$RUN/app.log" && {
  echo "FAIL: a key was still dispatched straight to a widget" >&2
  exit 1
  }

python3 - "$RUN/before.ppm" "$RUN/after.ppm" <<'EOF'
import sys
from PIL import Image
before = Image.open(sys.argv[1]).convert("RGB")
after  = Image.open(sys.argv[2]).convert("RGB")
# The display field of the calculator
box = (172, 8, 172 + 230, 8 + 18)
a = before.crop(box)
b = after.crop(box)
if a.tobytes() == b.tobytes():
    print("FAIL: the display did not change, the keys had no effect")
    sys.exit(1)
print("the display changed after 6/2=")
sys.exit(0)
EOF
verdict=$?
if [ "$verdict" -ne 0 ]; then
  exit 1
  fi
echo "PASS: the keys reached the shell and the accelerator table acted on them"
exit 0
