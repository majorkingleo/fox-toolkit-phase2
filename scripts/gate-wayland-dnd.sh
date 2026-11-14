#!/usr/bin/env bash
#
# Drag and drop gate:  press in the upper half of tests/dnd, drag into the lower
# half and release, and expect the target to print the text it received.
#
# This exercises both sides of the protocol at once: the press starts a
# wl_data_device.start_drag with the source of the window, the compositor sends
# enter, motion and drop to the target surface, and the target reads the data
# back through the pipe the source writes into.
#
set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

RUN=${RUN:-tmp/sway-dnd}
OUT=${OUT:-tmp/wlxinject}
SRC=scripts/wlxinject.c
XML=scripts/protocols/wlr-virtual-pointer-unstable-v1.xml

SOURCE=500,60
TARGET=500,140
DRAGTEXT="dragged text for the target"

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
[ -n "$sock" ] || { echo "error: no sway socket"; exit 2; }
export WAYLAND_DISPLAY=$(basename "$sock")

echo "==> start the application"
FOX_DRAG_TEXT="$DRAGTEXT" FOX_WAYLAND_TRACE=1 tmp/build-fox2-wayland/bin/dnd >"$RUN/app.log" 2>&1 &
app_pid=$!
sleep 2.0
kill -0 "$app_pid" 2>/dev/null || { echo "FAIL: the application did not start"; tail -10 "$RUN/app.log"; exit 1; }

# Press on the source, move it over the target in steps, then release.  The moves
# in between matter: the compositor only starts delivering the drag once the
# pointer has left the surface of the source.
echo "==> drag from $SOURCE to $TARGET"
"$OUT/wlxinject" "${SOURCE%,*}" "${SOURCE#*,}" move sleep:200 down sleep:200 \
                 move:500,200 sleep:200 \
                 move:"${TARGET%,*}","${TARGET#*,}" sleep:300 \
                 up sleep:400 >"$RUN/input.log" 2>&1
sleep 1.0

failed=""
grep -q "^drag of \"$DRAGTEXT\" started$" "$RUN/app.log" || failed="the source did not start a drag"
grep -q "drag entered the target" "$RUN/app.log" || failed="${failed:-the drag never reached the target}"
grep -q "^dropped \"$DRAGTEXT\"$" "$RUN/app.log" || failed="${failed:-the target did not receive the text}"

if [ -n "$failed" ]; then
  echo "FAIL: $failed"
  grep -E "drag|drop|DnD" "$RUN/app.log" | tail -16
  exit 1
  fi

grep -E "^drag of|^drag ended|drag entered|^dropped" "$RUN/app.log" | tail -6
echo "PASS: the text was dragged from one widget to the other"
exit 0
