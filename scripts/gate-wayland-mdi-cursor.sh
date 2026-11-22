#!/usr/bin/env bash
#
# MDI cursor gate: the cursor of an MDI sub-window follows the pointer.
#
# An MDI sub-window has one widget for its title bar and its resize border, and
# it changes its own cursor as the pointer moves inside it: an arrow over the
# title bar, the resize cursor over the border, and the move cursor while it is
# dragged.  The compositor has one cursor for the surface, so the client has to
# ask again whenever the *cursor* changes and not only when the widget under the
# pointer does.  The gate runs mditest on a headless sway, moves the virtual
# pointer over the content, the border and the title bar of the first sub-window
# and drags it, and reads the requests off the wire with WAYLAND_DEBUG:
#
#   - the resize border and the drag use image cursors (wl_pointer.set_cursor),
#     so at least two of those have to be on the wire,
#   - one of them has to come after the press, which is the drag cursor,
#   - and an arrow (shape 1) has to be asked for again after the last of them,
#     which is the title bar and the end of the drag.
#
#   ./scripts/gate-wayland-mdi-cursor.sh
#   ./scripts/gate-wayland-mdi-cursor.sh --bin=FILE --run-dir=DIR
#
# Exit status is 0 when the cursor followed the pointer, 1 when it did not, 2
# when the harness could not start.
#
set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

RUN=${RUN:-tmp/sway-mdi-cursor}
OUT=${OUT:-tmp/wlxinject}
SRC=tools/wlxinject.c
XML=scripts/protocols/wlr-virtual-pointer-unstable-v1.xml
BIN=tmp/build-fox2-wayland/bin/mditest

while [ $# -gt 0 ]; do
  case "$1" in
    --bin)       shift; BIN=$1 ;;
    --bin=*)     BIN=${1#*=} ;;
    --run-dir)   shift; RUN=$1 ;;
    --run-dir=*) RUN=${1#*=} ;;
    -h|--help)   sed -n '2,27p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) echo "error: unknown option '$1'" >&2; exit 2 ;;
    esac
  shift
  done

[ -x "$BIN" ] || { echo "error: no mditest at $BIN (build with scripts/build-fox2-wayland.sh)" >&2; exit 2; }

mkdir -p -- "$OUT"
wayland-scanner private-code "$XML" "$OUT/vptr-protocol.c" || exit 2
wayland-scanner client-header "$XML" "$OUT/wlr-virtual-pointer-unstable-v1-client-protocol.h" || exit 2
cc -O1 -Wall -I"$OUT" -o "$OUT/wlxinject" "$SRC" "$OUT/vptr-protocol.c" -lwayland-client || exit 2

rm -rf -- "$RUN"
mkdir -p -- "$RUN/home"
RUN=$(cd -- "$RUN" && pwd)
chmod 700 "$RUN"
echo 'output HEADLESS-1 resolution 1024x640 bg #101010 solid_color' >"$RUN/config"

export XDG_RUNTIME_DIR="$RUN"
export HOME="$RUN/home"
unset DISPLAY WAYLAND_DISPLAY
export WLR_BACKENDS=headless WLR_LIBINPUT_NO_DEVICES=1 WLR_RENDERER=pixman WLR_HEADLESS_OUTPUTS=1
export WLXINJECT_WIDTH=1024 WLXINJECT_HEIGHT=640

sway --unsupported-gpu -c "$RUN/config" >"$RUN/sway.log" 2>&1 &
sway_pid=$!
cleanup(){ kill "${app_pid:-}" 2>/dev/null || true; kill "$sway_pid" 2>/dev/null || true; wait "$sway_pid" 2>/dev/null || true; }
trap cleanup EXIT INT TERM

sock=""
for i in $(seq 1 200); do
  sock=$(ls "$RUN"/wayland-* 2>/dev/null | head -1)
  [ -n "$sock" ] && [ -S "$sock" ] && break
  sleep 0.05
  done
[ -n "$sock" ] || { echo "error: no sway socket" >&2; exit 2; }
export WAYLAND_DISPLAY=$(basename "$sock")

WAYLAND_DEBUG=1 FOX_WAYLAND_TRACE=1 "$BIN" --no-dialogs >"$RUN/app.log" 2>&1 &
app_pid=$!
sleep 2.5
kill -0 "$app_pid" 2>/dev/null || { echo "error: mditest did not stay up; see $RUN/app.log" >&2; exit 2; }

# The place of the first sub-window in the store, from its bind line
cx=0
for i in $(seq 1 100); do
  cx=$(grep "bind window 39 store" "$RUN/app.log" | grep "visible 400x300" | tail -1 | sed 's/.*store [0-9]*x[0-9]* origin +\([0-9-]*\)+\([0-9-]*\) visible.*/\1/')
  [ -n "$cx" ] && [ "$cx" != "0" ] && break
  sleep 0.05
  done
cy=$(grep "bind window 39 store" "$RUN/app.log" | grep "visible 400x300" | tail -1 | sed 's/.*store [0-9]*x[0-9]* origin +\([0-9-]*\)+\([0-9-]*\) visible.*/\2/')
[ -n "$cx" ] && [ -n "$cy" ] || { echo "error: no sub-window in the trace; see $RUN/app.log" >&2; exit 2; }

# Origin of the surface from an enter event
"$OUT/wlxinject" 1 1 move sleep:150 move:512,320 sleep:300 >/dev/null 2>&1
sleep 0.3
enter=$(grep "pointer enter window 2 at" "$RUN/app.log" | tail -1 | sed 's/.*at //')
[ -n "$enter" ] || { echo "error: the pointer did not reach mditest; see $RUN/app.log" >&2; exit 2; }
lx=${enter%,*}; ly=${enter#*,}
ox=$((512-lx)); oy=$((320-ly))

bodyx=$((ox+cx+100)); bodyy=$((oy+cy+120))
edgex=$((ox+cx+200)); edgey=$((oy+cy+1))
titlex=$((ox+cx+100)); titley=$((oy+cy+10))

# Content, then the resize border above it, then the title bar, then drag it
"$OUT/wlxinject" 512 320 "move:$bodyx,$bodyy" sleep:300 "move:$edgex,$edgey" sleep:300 \
  "move:$titlex,$titley" sleep:300 down sleep:200 \
  "move:$((titlex+60)),$((titley+40))" sleep:200 "move:$((titlex+120)),$((titley+80))" sleep:200 \
  up sleep:400 >/dev/null 2>&1
sleep 0.6
kill -0 "$app_pid" 2>/dev/null || { echo "FAIL: mditest ended during the gesture; see $RUN/app.log" >&2; exit 1; }

python3 - "$RUN/app.log" <<'WIRE' || exit 1
import re
import sys

lines = open(sys.argv[1], errors="replace").read().splitlines()
press = None
image = []
shape_after_image = False
for i, line in enumerate(lines):
    if "button 1 down to FXMDIChild" in line:
        press = i
    if re.search(r"wl_pointer#\d+\.set_cursor\(", line):
        image.append(i)
    if image and re.search(r"wp_cursor_shape_device_v1#\d+\.set_shape\(\d+, 1\)", line):
        shape_after_image = True

if len(image) < 2:
    print("FAIL: only %d image cursors were asked for; the resize border and the"
          " drag both need one" % len(image))
    sys.exit(1)
if press is None:
    print("FAIL: the press never reached a sub-window; see the log")
    sys.exit(1)
if not any(i > press for i in image):
    print("FAIL: no cursor was asked for after the press; the drag cursor never"
          " reached the compositor")
    sys.exit(1)
if not shape_after_image:
    print("FAIL: the pointer never went back to the arrow after an image cursor")
    sys.exit(1)
print("%d image cursors, one of them while dragging, and the arrow came back"
      % len(image))
WIRE

echo "PASS: the cursor of an MDI sub-window follows the title bar, the border and the drag"
exit 0
