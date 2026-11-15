#!/usr/bin/env bash
#
# Tool tip gate:  rest the pointer on the big button of the button test until
# its tool tip appears, and check that the tip is a popup that grabs nothing.
#
# A tool tip is a popup surface like a menu, but it must not take the pointer
# grab: the user is only moving the pointer, and a grab would swallow the click
# that is meant for the window below.  The module tells them apart by asking the
# widget what it is - an FXPopup takes the grab, a plain shell does not.
#
set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

RUN=${RUN:-tmp/sway-tooltip}
OUT=${OUT:-tmp/wlxinject}
DUMP=${DUMP:-tmp/dump-tooltip}
SRC=tools/wlxinject.c
XML=scripts/protocols/wlr-virtual-pointer-unstable-v1.xml

# Somewhere inside the button of the tool tip test
TIPPOINT=200,150

mkdir -p -- "$OUT"
wayland-scanner private-code "$XML" "$OUT/vptr-protocol.c" || exit 2
wayland-scanner client-header "$XML" "$OUT/wlr-virtual-pointer-unstable-v1-client-protocol.h" || exit 2
cc -O1 -Wall -I"$OUT" -o "$OUT/wlxinject" "$SRC" "$OUT/vptr-protocol.c" -lwayland-client || exit 2

rm -rf -- "$RUN" "$DUMP"
mkdir -p -- "$RUN/home" "$DUMP"
RUN=$(cd -- "$RUN" && pwd)
DUMP=$(cd -- "$DUMP" && pwd)
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

FOX_WAYLAND_TRACE=1 FOX_WAYLAND_DUMP="$DUMP" tmp/build-fox2-wayland/bin/tooltip >"$RUN/app.log" 2>&1 &
app_pid=$!
sleep 2.0

echo "==> rest the pointer on the big button"
# The injector has to stay alive while the tip waits:  when it exits, the
# compositor takes its virtual pointer away and the application sees the
# pointer leave the window, which cancels the tip
"$OUT/wlxinject" "${TIPPOINT%,*}" "${TIPPOINT#*,}" move sleep:3500 >/dev/null 2>&1
# The tip waits for the tool tip pause of the application, then for its timeout
sleep 3.0

failed=""
grep -q "create popup" "$RUN/app.log" || failed="the tool tip never became a popup"
grep -q "without grab" "$RUN/app.log" || failed="${failed:-the tool tip took the pointer grab, it must not}"
kill "$app_pid" 2>/dev/null
wait "$app_pid" 2>/dev/null
app_pid=""

if [ -n "$failed" ]; then
  echo "FAIL: $failed"
  grep -E "create popup|popup done|pointer enter|destroy surface" "$RUN/app.log" | tail -10
  exit 1
  fi

tip=$(ls -t "$DUMP"/window-*.ppm 2>/dev/null | head -1)
echo "==> tool tip frame: $(basename "${tip:-none}")"
grep -E "create popup" "$RUN/app.log" | tail -2
[ -n "$tip" ] && cp "$tip" "$DUMP/tooltip.ppm"
echo "PASS: the tool tip is a popup and it grabs nothing"
exit 0
