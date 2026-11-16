#!/usr/bin/env bash
#
# Gate: a double click in the file list of the ported PathFinder opens the
# "Unknown File Type" dialog, the way it does on X11.
#
# This is the regression test for the click counter of the Wayland backend: it
# carries click_time, click_button and moved from one event to the next, so a
# second press within the double click time has to arrive as click_count 2. A
# backend that hands every raw event a fresh FXEvent loses that state and the
# list only ever sees single clicks.
#
# The application is started on a directory with one text file, whose first row
# is double clicked through a virtual pointer. A second window of the client is
# the dialog; the gate fails when only the main window remains.
#
#   ./scripts/gate-wayland-doubleclick.sh
#
# Options:
#   --build=DIR    build directory (default: tmp/build-fox2-wayland)
#   --run-dir=DIR  runtime directory (default: tmp/wayland-doubleclick)
#   -h|--help      show this help
#
# Exit status is 0 on pass, 1 on a failed check, 2 when the run itself failed.
#
set -uo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
BUILD="$ROOT/tmp/build-fox2-wayland"
RUN="$ROOT/tmp/wayland-doubleclick"
BIN=""
OUT="$ROOT/tools/build"

usage(){
  sed -n '2,22p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
  }

while [ $# -gt 0 ]; do
  case "$1" in
    --build)     shift; BUILD=$1 ;;
    --build=*)   BUILD=${1#*=} ;;
    --run-dir)   shift; RUN=$1 ;;
    --run-dir=*) RUN=${1#*=} ;;
    -h|--help)   usage; exit 0 ;;
    *) echo "error: unknown option '$1'" >&2; usage >&2; exit 2 ;;
    esac
  shift
  done

BIN="$BUILD/bin/PathFinder"
[ -x "$BIN" ] || { echo "error: no PathFinder at $BIN (build with scripts/build-fox2-wayland.sh)" >&2; exit 2; }
command -v sway >/dev/null 2>&1 || { echo "error: sway not found" >&2; exit 2; }
command -v swaymsg >/dev/null 2>&1 || { echo "error: swaymsg not found" >&2; exit 2; }

# The virtual pointer injector, the same one the click gate uses
if [ ! -x "$OUT/wlxinject" ]; then
  make -C "$ROOT/tools" wayland >/dev/null 2>&1 || {
    echo "error: cannot build tools/build/wlxinject" >&2
    exit 2
    }
  fi

rm -rf -- "$RUN"
mkdir -p -- "$RUN/runtime" "$RUN/home" "$RUN/demo" "$RUN/frames"
chmod 700 "$RUN/runtime"
printf 'hello\n' >"$RUN/demo/demo.txt"

# A fresh HOME keeps the default 800x600 geometry and the classic theme
printf 'output HEADLESS-1 resolution 1024x768 bg #101010 solid_color\n' >"$RUN/config"
printf 'default_border pixel 0\n' >>"$RUN/config"
printf 'for_window [title="PathFinder.*"] floating enable\n' >>"$RUN/config"
printf 'for_window [title="Unknown.*"] floating enable\n' >>"$RUN/config"

export XDG_RUNTIME_DIR="$RUN/runtime" HOME="$RUN/home"
unset DISPLAY WAYLAND_DISPLAY DBUS_SESSION_BUS_ADDRESS DBUS_SYSTEM_BUS_ADDRESS XDG_CONFIG_HOME
export WLR_BACKENDS=headless WLR_LIBINPUT_NO_DEVICES=1 WLR_RENDERER=pixman WLR_HEADLESS_OUTPUTS=1

sway --unsupported-gpu -c "$RUN/config" >"$RUN/sway.log" 2>&1 &
sway_pid=$!
app_pid=
cleanup(){
  [ -n "$app_pid" ] && { kill "$app_pid" 2>/dev/null || true; wait "$app_pid" 2>/dev/null || true; }
  kill "$sway_pid" 2>/dev/null || true
  wait "$sway_pid" 2>/dev/null || true
  }
trap cleanup EXIT INT TERM

sock=""
for i in $(seq 1 200); do
  sock=$(ls "$RUN/runtime"/wayland-* 2>/dev/null | head -1)
  [ -n "$sock" ] && [ -S "$sock" ] && break
  sleep 0.05
  done
[ -n "$sock" ] || { echo "error: no sway socket" >&2; exit 2; }
export WAYLAND_DISPLAY=$(basename "$sock")
export SWAYSOCK=$(ls "$RUN/runtime"/sway-ipc.*.sock 2>/dev/null | head -1)

FOX_WAYLAND_DUMP="$RUN/frames" "$BIN" "$RUN/demo" >"$RUN/app.log" 2>&1 &
app_pid=$!

# Wait for the main window and read its place from the compositor
rect=""
for i in $(seq 1 200); do
  rect=$(swaymsg -t get_tree 2>/dev/null | python3 -c '
import json,sys
tree=json.load(sys.stdin)
found=[]
def walk(n):
    if n.get("name","").startswith("PathFinder") and n.get("visible"):
        r=n.get("rect",{}); found.append((r.get("x",0),r.get("y",0),r.get("width",0),r.get("height",0)))
    for c in n.get("nodes",[])+n.get("floating_nodes",[]): walk(c)
walk(tree)
print(" ".join(str(v) for v in found[0]) if found else "")' 2>/dev/null)
  [ -n "$rect" ] && break
  sleep 0.05
  done
[ -n "$rect" ] || { echo "error: PathFinder window did not appear; see $RUN/app.log" >&2; exit 2; }
set -- $rect
win_x=$1; win_y=$2; win_w=$3; win_h=$4
echo "==> PathFinder at $win_x,$win_y ${win_w}x$win_h"
[ "$win_w" -ge 800 ] || echo "warning: window is ${win_w}px wide, not the default 800" >&2

# The file list sits at 209,89 in the window (menu, tool bar and location bar
# above it); the first row is 22 px high, so its centre is at 50,13
click_x=$((win_x+209+50))
click_y=$((win_y+89+13))
echo "==> double click the first row at $click_x,$click_y"
WLXINJECT_WIDTH=1024 WLXINJECT_HEIGHT=768 "$OUT/wlxinject" 700 400 move sleep:200 \
  "move:$click_x,$click_y" sleep:400 down sleep:80 up sleep:150 down sleep:80 up sleep:1200 \
  >>"$RUN/input.log" 2>&1
windows=""
for i in $(seq 1 40); do
  windows=$(swaymsg -t get_tree | python3 -c '
import json,sys
tree=json.load(sys.stdin)
names=[]
def walk(n):
    if n.get("name") and n.get("visible"): names.append(n["name"])
    for c in n.get("nodes",[])+n.get("floating_nodes",[]): walk(c)
walk(tree)
print("\n".join(names))')
  printf '%s\n' "$windows" | grep -q 'Unknown' && break
  sleep 0.05
  done

echo "==> visible windows:"
printf '%s\n' "$windows" | sed 's/^/    /'
if printf '%s\n' "$windows" | grep -q 'Unknown'; then
  echo "PASS: the double click opened the unknown-file-type dialog"
  exit 0
  fi
echo "FAIL: no second window after the double click"
echo "      see $RUN/app.log and $RUN/frames"
exit 1
