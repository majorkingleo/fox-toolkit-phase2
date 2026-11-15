#!/usr/bin/env bash
#
# Cursor gate: the widget under the pointer decides the pointer shape.
#
# The Wayland client shows one cursor for the whole surface, so it has to ask
# for a new shape whenever the pointer moves to another widget.  This gate runs
# the calculator on a headless sway, moves one virtual pointer from its text
# display to a button and then to a label, and reads the shapes off the wire
# with WAYLAND_DEBUG: the display field must ask for "text" (9) and the button
# and label for "default" (1).
#
# A headless sway has no input devices; scripts/wlxinject.c creates one
# virtual pointer and keeps it alive for the whole gesture, so the seat does
# not change capabilities in the middle of the run.
#
#   ./scripts/gate-wayland-cursor.sh
#   ./scripts/gate-wayland-cursor.sh --bin=FILE --build=DIR
#
# Exit status is 0 when both shapes were requested, 1 when they were not, 2
# when the run itself failed.
#
set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

BUILD=tmp/build-fox2-wayland
BIN=
RUN=tmp/sway-cursor-gate
OUT=tmp/wlxinject
SRC=scripts/wlxinject.c
XML=scripts/protocols/wlr-virtual-pointer-unstable-v1.xml

usage(){
  sed -n '2,20p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
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
for tool in sway swaymsg wayland-scanner cc; do
  command -v "$tool" >/dev/null 2>&1 || { echo "error: $tool not found" >&2; exit 2; }
  done

# The virtual pointer injector
mkdir -p -- "$OUT"
wayland-scanner private-code "$XML" "$OUT/vptr-protocol.c" || exit 2
wayland-scanner client-header "$XML" "$OUT/wlr-virtual-pointer-unstable-v1-client-protocol.h" || exit 2
cc -O1 -Wall -I"$OUT" -o "$OUT/wlxinject" "$SRC" "$OUT/vptr-protocol.c" -lwayland-client || exit 2

rm -rf -- "$RUN"
mkdir -p -- "$RUN/home"
RUN=$(cd -- "$RUN" && pwd)
chmod 700 "$RUN"
printf '%s\n' \
  'output HEADLESS-1 resolution 1024x640 bg #101010 solid_color' \
  'default_border pixel 0' >"$RUN/config"

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
WAYLAND_DEBUG=1 FOX_WAYLAND_TRACE=1 "$BIN" >"$RUN/app.log" 2>&1 &
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
echo "==> window at $wx,$wy"

# One virtual pointer, three positions: display field, AC button, label
disp_x=$((wx + 300)); disp_y=$((wy + 20))
button_x=$((wx + 378)); button_y=$((wy + 102))
label_x=$((wx + 330)); label_y=$((wy + 40))
"$OUT/wlxinject" "$disp_x" "$disp_y" move sleep:300 \
                 "move:$button_x,$button_y" sleep:300 \
                 "move:$label_x,$label_y" sleep:300 >/dev/null 2>&1
sleep 1.0

kill -0 "$app_pid" 2>/dev/null || { echo "error: the application ended" >&2; exit 1; }

shapes=$(grep -o "set_shape([0-9]*, [0-9]*)" "$RUN/app.log" | sed 's/.*, //; s/)//' | tr '\n' ' ')
echo "==> shapes requested: $shapes"
grep -q "set_shape([0-9]*, 9)" "$RUN/app.log" || { echo "FAIL: the text display did not ask for the text cursor" >&2; exit 1; }
grep -q "set_shape([0-9]*, 1)" "$RUN/app.log" || { echo "FAIL: the button or label did not ask for the default cursor" >&2; exit 1; }
echo "PASS: the text display gets the text cursor and the button the default cursor"
exit 0
