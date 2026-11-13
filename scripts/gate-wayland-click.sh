#!/usr/bin/env bash
#
# Pointer gate:  start a headless sway, run the click application, put a
# virtual pointer on its button, press and release, and expect the program to
# end with 0.
#
# A headless sway has no input devices at all:  the seat reports no
# capabilities, and neither swaymsg nor wlrctl can click, because wlrctl
# destroys its virtual pointer again before the compositor delivers anything.
# scripts/wlxinject.c therefore creates one virtual pointer, keeps it alive for
# the whole gesture and then moves, presses and releases it.
#
# The presses sweep a few points inside the window because the compositor
# decides where the toplevel ends up and how much decoration it adds.
#
set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

RUN=${RUN:-tmp/sway-click}
OUT=${OUT:-tmp/wlxinject}
SRC=scripts/wlxinject.c
XML=scripts/protocols/wlr-virtual-pointer-unstable-v1.xml

# Build the injector -------------------------------------------------------
mkdir -p -- "$OUT"
wayland-scanner private-code "$XML" "$OUT/vptr-protocol.c" || exit 2
wayland-scanner client-header "$XML" "$OUT/wlr-virtual-pointer-unstable-v1-client-protocol.h" || exit 2
cc -O1 -Wall -I"$OUT" -o "$OUT/wlxinject" "$SRC" "$OUT/vptr-protocol.c" -lwayland-client || exit 2

# Start sway ---------------------------------------------------------------
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

# Run the application ------------------------------------------------------
export FOX_WAYLAND_TRACE=1
tmp/build-fox2-wayland/bin/click >"$RUN/app.log" 2>&1 &
app_pid=$!
sleep 1.5

clicked=no
for point in 512,320 400,240 300,180 512,400 200,140 620,420; do
  x=${point%,*}
  y=${point#*,}
  echo "==> press at $x,$y"
  "$OUT/wlxinject" "$x" "$y" move sleep:120 down sleep:120 up sleep:200 >>"$RUN/input.log" 2>&1
  sleep 0.3
  if ! kill -0 "$app_pid" 2>/dev/null; then
    clicked=$point
    break
    fi
  done

state=timeout
for i in $(seq 1 40); do
  if ! kill -0 "$app_pid" 2>/dev/null; then
    wait "$app_pid"
    state="exit $?"
    break
    fi
  sleep 0.05
  done
if [ "$state" = timeout ]; then
  kill "$app_pid" 2>/dev/null
  wait "$app_pid" 2>/dev/null
  fi

echo "==> application: $state"
grep -E "pointer button|button .* to |^press$|^release$|^command" "$RUN/app.log" | head -8
[ "$state" = "exit 0" ] && { echo "PASS: the pointer press reached the button (at $clicked)"; exit 0; }
echo "FAIL: the application did not act on the pointer press"
exit 1
