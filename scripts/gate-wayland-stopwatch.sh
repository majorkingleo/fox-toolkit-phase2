#!/usr/bin/env bash
#
# Stopwatch gate: the desktop clock runs on the headless compositor, a virtual
# pointer switches the mode, starts and stops the stopwatch, opens the history
# popup and toggles the analog face.  The frames are dumped by the Wayland
# backend (FOX_WAYLAND_DUMP) and stored under screenshots/tests/fox2/wayland.
#
#   ./scripts/gate-wayland-stopwatch.sh
#   ./scripts/gate-wayland-stopwatch.sh --save
#   ./scripts/gate-wayland-stopwatch.sh --bin=FILE
#
# Options:
#   --bin=FILE     application to run (default: tmp/build-fox2-wayland/bin/stopwatch)
#   --build=DIR    build directory (default: tmp/build-fox2-wayland)
#   --run-dir=DIR  compositor runtime directory (default: tmp/sway-stopwatch)
#   --dump=DIR     where the frames are written (default: RUN/dump)
#   --save         copy the frames to screenshots/tests/fox2/wayland
#   -h|--help      show this help
#
# Exit status is 0 when every step passed, 2 when the run itself failed.
#
set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

ROOT=$(pwd)
BUILD=tmp/build-fox2-wayland
BIN=
RUN=tmp/sway-stopwatch
DUMP=
OUT=tmp/wlxinject
SRC=tools/wlxinject.c
XML=scripts/protocols/wlr-virtual-pointer-unstable-v1.xml
save=no

usage(){
  sed -n '2,22p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
  }

while [ $# -gt 0 ]; do
  case "$1" in
    --bin)       shift; BIN=$1 ;;
    --bin=*)     BIN=${1#*=} ;;
    --build)     shift; BUILD=$1 ;;
    --build=*)   BUILD=${1#*=} ;;
    --run-dir)   shift; RUN=$1 ;;
    --run-dir=*) RUN=${1#*=} ;;
    --dump)      shift; DUMP=$1 ;;
    --dump=*)    DUMP=${1#*=} ;;
    --save)      save=yes ;;
    -h|--help)   usage; exit 0 ;;
    *) echo "error: unknown option '$1'" >&2; usage >&2; exit 2 ;;
    esac
  shift
  done

[ -n "$BIN" ] || BIN="$BUILD/bin/stopwatch"
[ -n "$DUMP" ] || DUMP="$RUN/dump"

if [ ! -x "$BIN" ]; then
  echo "error: no binary at $BIN (build with scripts/build-fox2-wayland.sh)" >&2
  exit 2
  fi
for tool in sway swaymsg wayland-scanner cc; do
  command -v "$tool" >/dev/null 2>&1 || { echo "error: $tool not found" >&2; exit 2; }
  done
if ! python3 -c 'import PIL' >/dev/null 2>&1; then
  echo "error: python3 with Pillow is needed to store the frames" >&2
  exit 2
  fi

# The virtual pointer injector
mkdir -p -- "$OUT"
wayland-scanner private-code "$XML" "$OUT/vptr-protocol.c" || exit 2
wayland-scanner client-header "$XML" "$OUT/wlr-virtual-pointer-unstable-v1-client-protocol.h" || exit 2
cc -O1 -Wall -I"$OUT" -o "$OUT/wlxinject" "$SRC" "$OUT/vptr-protocol.c" -lwayland-client || exit 2

rm -rf -- "$RUN" "$DUMP"
mkdir -p -- "$RUN/home" "$DUMP"
RUN=$(cd -- "$RUN" && pwd)
DUMP=$(cd -- "$DUMP" && pwd)
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
[ -n "$sock" ] || { echo "error: no sway socket; see $RUN/sway.log" >&2; exit 2; }
export WAYLAND_DISPLAY=$(basename "$sock")
export SWAYSOCK=$(ls "$RUN"/sway-ipc.*.sock 2>/dev/null | head -1)

fail(){
  echo "FAIL: $1" >&2
  grep -E "create surface|toplevel configure|commit window|error" "$RUN/app.log" | tail -10
  exit 1
  }

alive(){
  kill -0 "$app_pid" 2>/dev/null
  }

# Print "x y w h" of a top level window with the given title
window_rect(){
  swaymsg -t get_tree | python3 -c "
import json,sys
title=sys.argv[1]
t=json.load(sys.stdin)
def walk(n):
    if n.get('name')==title:
        r=n['rect']; print('%d %d %d %d'%(r['x'],r['y'],r['width'],r['height']))
    for c in n.get('nodes',[])+n.get('floating_nodes',[]): walk(c)
walk(t)" "$1"
  }

newest_frame(){
  ls -t "$DUMP"/window-*.ppm 2>/dev/null | head -1
  }

click(){
  "$OUT/wlxinject" "$1" "$2" move sleep:120 down sleep:120 up sleep:500 >/dev/null 2>&1
  sleep 0.4
  }

right_click(){
  "$OUT/wlxinject" "$1" "$2" move sleep:120 rightdown sleep:120 rightup sleep:500 >/dev/null 2>&1
  sleep 0.4
  }

mkdir -p -- "$ROOT/tmp/stopwatch-frames"

capture(){
  frame=$(newest_frame)
  [ -n "$frame" ] || fail "no frame was committed for '$1'"
  python3 - "$frame" "$ROOT/tmp/stopwatch-frames/$1.png" <<'EOF'
import sys, re
from PIL import Image
frame, out = sys.argv[1], sys.argv[2]
image = Image.open(frame).convert("RGB")
m = re.search(r"-bar(\d+)", frame)
if m:
    image = image.crop((0, int(m.group(1)), image.width, image.height))
image.save(out)
print("captured", out, image.width, image.height)
EOF
  }

echo "==> running $BIN"
FOX_WAYLAND_TRACE=1 FOX_WAYLAND_DUMP="$DUMP" "$BIN" >"$RUN/app.log" 2>&1 &
app_pid=$!
sleep 2
alive || fail "the application did not stay up"

swaymsg '[title="Stopwatch"] floating enable' >/dev/null 2>&1
sleep 0.6

rect=$(window_rect Stopwatch)
[ -n "$rect" ] || fail "the stopwatch window did not appear"
set -- $rect
wx=$1; wy=$2; ww=$3; wh=$4
echo "==> window at $wx,$wy ${ww}x${wh}"
[ "$ww" -eq 260 ] && [ "$wh" -eq 200 ] || fail "the window is ${ww}x${wh}, expected 260x200"
capture clock-digital

echo "==> switch to the stopwatch"
click $((wx + 97)) $((wy + 180))
alive || fail "the application ended on the mode switch"
capture stopwatch-idle

echo "==> start, wait, stop"
click $((wx + 65)) $((wy + 141))
sleep 2
alive || fail "the application ended on start"
capture stopwatch-running
click $((wx + 65)) $((wy + 141))
capture stopwatch-stopped
elapsed1=$(sed -n 's/^elapsed=\([0-9]*\)/\1/p' "$HOME/.config/Stopwatch.rc" | tail -1)
echo "==> first stop after ${elapsed1}ns"

echo "==> right click opens the history popup"
right_click $((wx + 130)) $((wy + 60))
alive || fail "the application ended on the popup"
surfaces=$(ls "$DUMP"/window-*.ppm 2>/dev/null | wc -l)
[ "$surfaces" -ge 2 ] || fail "no popup surface was committed"
capture history-popup

echo "==> close the popup and toggle the face"
"$OUT/wlxinject" 5 5 move sleep:120 down sleep:120 up sleep:300 >/dev/null 2>&1
sleep 0.4
click $((wx + 227)) $((wy + 180))
alive || fail "the application ended on the style toggle"
capture stopwatch-analog

echo "==> clock mode, analog face, then digital again"
click $((wx + 32)) $((wy + 180))
sleep 1.2
alive || fail "the application ended in clock mode"
capture clock-analog
click $((wx + 227)) $((wy + 180))
capture clock-digital-again

# R10: the stopwatch keeps running while another mode is shown
echo "==> start, hide behind the clock, come back and stop"
click $((wx + 97)) $((wy + 180))
click $((wx + 65)) $((wy + 141))
sleep 1
click $((wx + 32)) $((wy + 180))
sleep 2
alive || fail "the application ended while the clock was shown"
click $((wx + 97)) $((wy + 180))
click $((wx + 65)) $((wy + 141))
sleep 0.5
alive || fail "the application ended on the second stop"

settings="$HOME/.config/Stopwatch.rc"
[ -f "$settings" ] || fail "no settings file at $settings"
count=$(sed -n 's/^count=//p' "$settings" | tail -1)
elapsed2=$(sed -n 's/^elapsed=\([0-9]*\)/\1/p' "$settings" | tail -1)
echo "==> settings: count=$count elapsed=${elapsed2}ns"
[ "$count" = 2 ] || fail "the history has $count entries, expected 2"
python3 - "$elapsed1" "$elapsed2" <<'PY' || fail "the stopwatch did not keep running behind the clock"
import sys
first, second = int(sys.argv[1]), int(sys.argv[2])
run = second - first
if not (2500000000 <= run <= 15000000000):
    print("FAIL: the hidden stopwatch ran %.2f s, expected at least 2.5 s" % (run/1e9))
    sys.exit(1)
print("PASS: the stopwatch kept running behind the clock (%.2f s)" % (run/1e9))
PY

if [ "$save" = yes ]; then
  dest="$ROOT/screenshots/tests/fox2/wayland"
  mkdir -p -- "$dest"
  for name in clock-digital clock-analog history-popup stopwatch-idle stopwatch-running stopwatch-stopped stopwatch-analog; do
    case "$name" in
      clock-*)   out="stopwatch-$name" ;;
      history-*) out="stopwatch-history" ;;
      *)         out="$name" ;;
      esac
    cp -f -- "$ROOT/tmp/stopwatch-frames/$name.png" "$dest/$out.png" || fail "cannot store $name"
    done
  echo "saved frames to $dest"
  fi

echo "PASS: the stopwatch ran, switched modes, opened the history and toggled the face"
