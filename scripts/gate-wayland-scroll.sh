#!/usr/bin/env bash
#
# Scroll gate: the wheel moves the document the way it does on X11.
#
# wl_pointer's vertical axis is positive towards the end of the document, while
# FOX counts a positive wheel as a scroll towards the top (Button4 on X11,
# a positive WM_MOUSEWHEEL delta on Win32).  The Wayland seat did not flip the
# sign, so the help window scrolled the wrong way.
#
# The gate opens the calculator's help window, which starts at the top of the
# document, and rolls the wheel down three notches: the frame must change (the
# document moves on) and, after three notches up, must be identical to the
# frame it started with.  With the sign inverted, the down scroll is clamped at
# the top and does nothing, and the up scroll moves the document instead.
#
#   ./scripts/gate-wayland-scroll.sh
#   ./scripts/gate-wayland-scroll.sh --bin=FILE --build=DIR
#
# Exit status is 0 when the wheel scrolls the right way, 1 when it does not,
# 2 when the harness could not start.
#
set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

BUILD=tmp/build-fox2-wayland
BIN=
RUN=tmp/sway-scroll-gate
OUT=tmp/wlxinject-scroll
SRC=tools/wlxinject.c
XML=scripts/protocols/wlr-virtual-pointer-unstable-v1.xml

usage(){
  sed -n '2,22p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
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

mkdir -p -- "$OUT"
wayland-scanner private-code "$XML" "$OUT/vptr-protocol.c" || exit 2
wayland-scanner client-header "$XML" "$OUT/wlr-virtual-pointer-unstable-v1-client-protocol.h" || exit 2
cc -O1 -Wall -I"$OUT" -o "$OUT/wlxinject" "$SRC" "$OUT/vptr-protocol.c" -lwayland-client || exit 2

rm -rf -- "$RUN"
mkdir -p -- "$RUN/home" "$RUN/dump"
RUN=$(cd -- "$RUN" && pwd)
chmod 700 "$RUN"
printf '%s\n' \
  'output HEADLESS-1 resolution 1024x1100 bg #101010 solid_color' \
  'default_border pixel 0' \
  'for_window [title="Calculator Help"] floating enable' >"$RUN/config"

export XDG_RUNTIME_DIR="$RUN"
export HOME="$RUN/home"
unset DISPLAY WAYLAND_DISPLAY DBUS_SESSION_BUS_ADDRESS DBUS_SYSTEM_BUS_ADDRESS XDG_CONFIG_HOME SWAYSOCK
export WLR_BACKENDS=headless WLR_LIBINPUT_NO_DEVICES=1 WLR_RENDERER=pixman WLR_HEADLESS_OUTPUTS=1
export WLXINJECT_WIDTH=1024 WLXINJECT_HEIGHT=1100

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
swaymsg '[title="FOX Calculator"] move position 0 0' >/dev/null 2>&1
sleep 0.6
main=$(swaymsg -t get_tree | python3 -c "
import json,sys
t=json.load(sys.stdin)
def walk(n):
    if n.get('name')=='FOX Calculator':
        r=n['rect']; print('%d %d %d %d'%(r['x'],r['y'],r['width'],r['height']))
    for c in n.get('nodes',[])+n.get('floating_nodes',[]): walk(c)
walk(t)")
[ -n "$main" ] || { echo "error: no calculator window" >&2; exit 2; }
set -- $main; wx=$1; wy=$2; ww=$3; wh=$4
[ "$ww" -eq 400 ] && [ "$wh" -eq 260 ] || { echo "error: window is ${ww}x${wh}" >&2; exit 2; }

# The question mark opens the help window
echo "==> opening the help window"
"$OUT/wlxinject" "$((wx+300))" "$((wy+20))" move sleep:300 \
                 "move:$((wx+153)),$((wy+20))" sleep:300 down sleep:300 up sleep:800 >/dev/null 2>&1
frame=""
for i in $(seq 1 100); do
  frame=$(ls "$RUN"/dump/window-*-652x964.ppm 2>/dev/null | head -1)
  [ -n "$frame" ] && break
  sleep 0.1
  done
[ -n "$frame" ] || { echo "error: the help window committed no frame" >&2; exit 2; }

rect=$(swaymsg -t get_tree | python3 -c "
import json,sys
t=json.load(sys.stdin)
def walk(n):
    if n.get('name')=='Calculator Help':
        r=n['rect']; print('%d %d %d %d'%(r['x'],r['y'],r['width'],r['height']))
    for c in n.get('nodes',[])+n.get('floating_nodes',[]): walk(c)
walk(t)")
[ -n "$rect" ] || { echo "error: the help window is gone" >&2; exit 2; }
set -- $rect; hx=$1; hy=$2

# Let the first frame settle, then keep it as the top of the document
sleep 0.5
cp "$frame" "$RUN/top.ppm"

echo "==> wheel down three notches"
"$OUT/wlxinject" "$((hx+320))" "$((hy+300))" move sleep:300 wheel:3 sleep:900 >/dev/null 2>&1
cp "$frame" "$RUN/down.ppm"
grep -q "wheel " "$RUN/app.log" || { echo "FAIL: the wheel did not reach the application at all" >&2; exit 1; }
if cmp -s "$RUN/top.ppm" "$RUN/down.ppm"; then
  echo "FAIL: rolling the wheel down did not move the document (the sign is inverted)" >&2
  grep "wheel " "$RUN/app.log" | tail -2 >&2
  exit 1
  fi
echo "the document moved on"

echo "==> wheel up three notches"
"$OUT/wlxinject" "$((hx+320))" "$((hy+300))" move sleep:300 wheel:-3 sleep:900 >/dev/null 2>&1
cp "$frame" "$RUN/up.ppm"
if ! cmp -s "$RUN/top.ppm" "$RUN/up.ppm"; then
  echo "FAIL: rolling the wheel up did not return to the top of the document" >&2
  exit 1
  fi
echo "the document came back to the top"

echo "PASS: the wheel scrolls the help window the same way as on X11"
exit 0
