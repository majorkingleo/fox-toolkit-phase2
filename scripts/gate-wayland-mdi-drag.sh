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
# trace to check that the drag committed a frame for every step, and it checks
# every frame of the many-step drag on its own: the windows the moving one
# uncovers do not change, so a pixel outside the moving window in two frames of
# a drag has to be the same byte.  Comparing only the frame a drag ends on
# misses a frame in between that keeps a strip of an earlier place, which is
# what a real drag of an MDI sub-window looked like.
#
#   ./scripts/gate-wayland-mdi-drag.sh
#   ./scripts/gate-wayland-mdi-drag.sh --bin=FILE
#
# Options:
#   --bin=FILE   application to run (default: tmp/build-fox2-wayland/bin/mditest)
#   --run-dir=DIR  scratch directory (default: tmp/sway-mdi-drag)
#
set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

RUN=${RUN:-tmp/sway-mdi-drag}
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
    -h|--help)   sed -n '2,31p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit 0 ;;
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
  FOX_WAYLAND_TRACE=1 FOX_WAYLAND_DUMP="$dump" FOX_WAYLAND_DUMP_FRAMES=1 "$BIN" --no-dialogs >"$log" 2>&1 &
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

# Every frame of the many-step drag, not only the one it ends on: the windows
# the moving one uncovers keep their content, so a pixel outside the moving
# window in a frame and in the last frame has to be the same byte.  The moving
# window is the one the trace names as moved, and its rectangle in a frame is
# the last place the table bound it to before that frame was committed
echo "==> checking every frame of the many-step drag"
python3 - "$RUN/dump-many" "$RUN/many.log" <<'PY' || exit 1
import collections
import glob
import os
import re
import sys

dump, logpath = sys.argv[1], sys.argv[2]
lines = open(logpath, errors="replace").read().splitlines()

moved = collections.Counter()
for line in lines:
    m = re.search(r"moved window (\d+) left", line)
    if m:
        moved[int(m.group(1))] += 1
if not moved:
    print("FAIL: the trace names no moving window; was the drag a move?")
    sys.exit(1)
wid = moved.most_common(1)[0][0]

# The rectangle of the moving window at the time each frame was committed
frames = []
last = None
for line in lines:
    # The frame of a moving window that hangs out of the store is excluded by
    # the part of it that was painted: the origin says where the window sits in
    # the store and the visible rectangle where that part is in the window
    m = re.search(r"bind window %d store \d+x\d+ origin ([+-]\d+)([+-]\d+) visible (\d+)x(\d+) at ([+-]\d+)([+-]\d+)" % wid, line)
    if m:
        ox, oy, vw, vh, vx, vy = (int(v) for v in m.groups())
        last = (ox + vx, oy + vy, vw, vh)
        continue
    if "commit window 2 " in line:
        frames.append(last)

files = sorted(glob.glob(os.path.join(dump, "*.ppm")),
               key=lambda p: int(re.search(r"-(\d+)\.ppm$", p).group(1)))
if len(files) < 4:
    print("FAIL: the drag produced %d frames, too few to check" % len(files))
    sys.exit(1)


def readppm(path):
    data = open(path, "rb").read()
    head = data.split(b"\n", 3)
    w, h = map(int, head[1].split())
    return w, h, head[3]


def inside(r, x, y):
    return r is not None and r[0] <= x < r[0] + r[2] and r[1] <= y < r[1] + r[3]


w, h, ref = readppm(files[-1])
refrect = frames[-1]
badframes = 0
for i, path in enumerate(files):
    w2, h2, pix = readppm(path)
    if (w2, h2) != (w, h) or i >= len(frames):
        continue
    r = frames[i]
    bad = 0
    for y in range(26, h):
        row = y * w * 3
        for x in range(w):
            if inside(r, x, y) or inside(refrect, x, y):
                continue
            o = row + x * 3
            if pix[o:o + 3] != ref[o:o + 3]:
                bad += 1
    if bad:
        badframes += 1
        print("frame %d (%s) keeps %d pixels of an earlier state" % (i, os.path.basename(path), bad))
if badframes:
    print("FAIL: %d of %d frames differ from the frame the drag ends on, outside"
          " the moving window" % (badframes, len(files)))
    sys.exit(1)
print("all %d frames keep the content of the windows the drag uncovers" % len(files))
PY

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
echo "PASS: no frame of the drag keeps a strip of a place the window had been in"
exit 0
