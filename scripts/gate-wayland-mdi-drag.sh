#!/usr/bin/env bash
#
# MDI drag gate: dragging a sub-window must not leave anything behind, and the
# frame that ends the drag must not depend on how many steps the drag took.
#
# Moving a child of the shared store is the hard case of the Wayland backend:
# the window that moves has no surface of its own, and the place it leaves has to
# be painted from the window below it.  A paint that is handed to a window and
# then covered by a window that the same frame paints later leaves a strip of the
# wrong window on screen; a frame that is committed while paints are still queued
# shows the parent without the window that is being dragged.  Both are visible as
# a frame that keeps the edges of the places the window was in before, so a drag
# that arrived in many steps ends on a different frame than one that arrived in
# two.
#
# The gate runs mditest twice, drags the same child to the same place (once in
# many steps, once in two) and compares the final frames.  It also reads the
# trace to check that the drag committed a frame for every step.
#
#   ./scripts/gate-wayland-mdi-drag.sh
#
set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

RUN=${RUN:-tmp/sway-mdi-drag}
OUT=${OUT:-tmp/wlxinject}
SRC=tools/wlxinject.c
XML=scripts/protocols/wlr-virtual-pointer-unstable-v1.xml
BIN=tmp/build-fox2-wayland/bin/mditest

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

# Drag the first MDI child from its title bar to a fixed pointer position in
# $1 steps, keep the frame the drag ends on and the log
drag(){
  local steps="$1" tag="$2" frame="$3" log="$4"
  local dump="$RUN/dump-$tag"
  mkdir -p -- "$dump"
  FOX_WAYLAND_TRACE=1 FOX_WAYLAND_DUMP="$dump" "$BIN" --no-dialogs >"$log" 2>&1 &
  app_pid=$!
  sleep 2.0
  kill -0 "$app_pid" 2>/dev/null || { echo "error: mditest did not stay up; see $log" >&2; exit 2; }

  # Leave the window and come back to the middle of the output, so the enter
  # event carries a known pointer position and gives the origin of the surface
  "$OUT/wlxinject" 1 1 move sleep:150 move:512,320 sleep:300 >/dev/null 2>&1
  sleep 0.3
  local enter lx ly ox oy
  enter=$(grep "pointer enter window 2 at" "$log" | tail -1 | sed 's/.*at //')
  [ -n "$enter" ] || { echo "error: the pointer did not reach mditest; see $log" >&2; exit 2; }
  lx=${enter%,*}; ly=${enter#*,}
  ox=$((512-lx)); oy=$((320-ly))

  # Title bar of the first MDI child: 10,10 in the client, the client starts
  # below the menu bar (26) and the bar is grabbed 17 pixels down
  local gx=$((ox+10+100)) gy=$((oy+26+10+17))
  local tx=$((gx+192)) ty=$((gy+96))
  local args=("move:$gx,$gy" sleep:300 down sleep:100)
  local i dx dy
  for i in $(seq 1 "$steps"); do
    dx=$(( (tx-gx)*i/steps )); dy=$(( (ty-gy)*i/steps ))
    args+=("move:$((gx+dx)),$((gy+dy))" sleep:25)
    done
  args+=(up sleep:400)
  "$OUT/wlxinject" 512 320 "${args[@]}" >/dev/null 2>&1
  sleep 0.8
  kill -0 "$app_pid" 2>/dev/null || { echo "FAIL: mditest ended during the drag; see $log" >&2; exit 1; }

  local newest
  newest=$(ls -t "$dump"/*.ppm 2>/dev/null | head -1)
  [ -n "$newest" ] && cp -- "$newest" "$frame"
  [ -s "$frame" ] || { echo "FAIL: the drag produced no frame" >&2; exit 1; }
  kill "$app_pid" 2>/dev/null
  wait "$app_pid" 2>/dev/null
  app_pid=
  }

echo "==> dragging in 24 steps"
drag 24 many "$RUN/many.ppm" "$RUN/many.log"
echo "==> dragging in 2 steps"
drag 2 few "$RUN/few.ppm" "$RUN/few.log"

# The final frame of a drag must not depend on the path the pointer took
if cmp -s "$RUN/many.ppm" "$RUN/few.ppm"; then
  echo "the two drags ended on the same frame"
else
  echo "FAIL: the 24 step drag left a different frame than the 2 step drag" >&2
  echo "      $RUN/many.ppm vs $RUN/few.ppm" >&2
  exit 1
fi

# Every motion of the many-step drag has to reach the compositor
python3 - "$RUN/many.log" 24 <<'PY' || exit 1
import sys
steps=int(sys.argv[2])
lines=open(sys.argv[1],errors="replace").read().splitlines()
commits=sum(1 for l in lines if 'commit window 2 ' in l)
# A few frames go out before the drag (create, configure, first paint), then one
# per step; a frame for every step is the point of the gate, and more than one
# per step means the frame of a step is torn over two commits
if commits < steps:
    print("FAIL: %d drag steps produced only %d frames"%(steps,commits))
    sys.exit(1)
if commits > steps+16:
    print("FAIL: %d drag steps produced %d frames; a step is committed more than once"%(steps,commits))
    sys.exit(1)
print("%d drag steps, %d frames committed"%(steps,commits))
PY

echo "PASS: a dragged MDI sub-window leaves the same frame however it was dragged"
exit 0
