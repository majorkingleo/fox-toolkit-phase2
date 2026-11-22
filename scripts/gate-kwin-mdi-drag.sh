#!/usr/bin/env bash
#
# MDI drag gate on KWin: drag a sub-window of mditest and check every frame.
#
# The sub-windows of an MDI client share one buffer, so a window that moves has
# to be painted out of the place it left by the windows below it.  Sway hides
# the faults of that: it coalesces its input, so a drag arrives in fewer, larger
# steps.  KWin delivers every motion, and a drag there kept thin strips of the
# places a sub-window had been in, and once the frame of the window below ended
# up on top of the moving one.  KWin also exposes org_kde_kwin_fake_input, so
# tools/kwininject can press the title bar and move it like a user would.
#
# The gate runs mditest under a private KWin virtual backend, drags the first
# sub-window to the same place in many steps and in two, and requires:
#
#   - the same frame at the end of both drags,
#   - one committed frame per step,
#   - every frame of the many-step drag to keep the content of the windows the
#     moving one uncovers: a pixel outside the moving window in a frame and in
#     the frame the drag ends on has to be the same byte.
#
#   ./scripts/gate-kwin-mdi-drag.sh
#
# Exit status is 0 when the drag is clean, 1 when a frame keeps an earlier
# state, 2 when the harness could not start (no KWin, no fake input).
#
set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

BUILD=tmp/build-fox2-wayland
BIN=
RUN=tmp/kwin-mdi-drag-gate
OUT=tmp/kwininject
XML=scripts/protocols/org-kde-kwin-fake-input.xml
STEPS=10

while [ $# -gt 0 ]; do
  case "$1" in
    --bin)      shift; BIN=$1 ;;
    --bin=*)    BIN=${1#*=} ;;
    --build)    shift; BUILD=$1 ;;
    --build=*)  BUILD=${1#*=} ;;
    --run-dir)  shift; RUN=$1 ;;
    --run-dir=*) RUN=${1#*=} ;;
    --steps)    shift; STEPS=$1 ;;
    --steps=*)  STEPS=${1#*=} ;;
    -h|--help)  sed -n '2,29p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) echo "error: unknown option '$1'" >&2; exit 2 ;;
    esac
  shift
  done

[ -n "$BIN" ] || BIN="$BUILD/bin/mditest"
[ -x "$BIN" ] || { echo "error: no mditest at $BIN (build with scripts/build-fox2-wayland.sh)" >&2; exit 2; }
command -v kwin_wayland >/dev/null 2>&1 || { echo "skip: kwin_wayland is not installed" >&2; exit 2; }
for tool in wayland-scanner cc python3; do
  command -v "$tool" >/dev/null 2>&1 || { echo "error: $tool not found" >&2; exit 2; }
  done

mkdir -p -- "$OUT"
wayland-scanner client-header "$XML" "$OUT/org-kde-kwin-fake-input-client-protocol.h" || exit 2
wayland-scanner private-code "$XML" "$OUT/fake-input-protocol.c" || exit 2
cc -O1 -Wall -I"$OUT" -o "$OUT/kwininject" tools/kwininject.c "$OUT/fake-input-protocol.c" -lwayland-client || exit 2

rm -rf -- "$RUN"
mkdir -p -- "$RUN/home" "$RUN/config"
RUN=$(cd -- "$RUN" && pwd)
chmod 700 "$RUN"

export XDG_RUNTIME_DIR="$RUN"
export HOME="$RUN/home"
# A registry of its own, so the sub-windows start at their default places every
# run and the two drags are comparable
export XDG_CONFIG_HOME="$RUN/config"
unset DISPLAY WAYLAND_DISPLAY DBUS_SESSION_BUS_ADDRESS DBUS_SYSTEM_BUS_ADDRESS
export QT_QPA_PLATFORM=offscreen

echo "==> starting kwin_wayland --virtual"
kwin_wayland --virtual --socket wayland-kwin --no-lockscreen >"$RUN/kwin.log" 2>&1 &
kwin_pid=$!
cleanup(){
  kill "${app_pid:-}" 2>/dev/null || true
  kill "$kwin_pid" 2>/dev/null || true
  wait "$kwin_pid" 2>/dev/null || true
  }
trap cleanup EXIT INT TERM

for i in $(seq 1 400); do
  [ -S "$RUN/wayland-kwin" ] && break
  kill -0 "$kwin_pid" 2>/dev/null || { echo "error: kwin exited; see $RUN/kwin.log" >&2; exit 2; }
  sleep 0.05
  done
[ -S "$RUN/wayland-kwin" ] || { echo "error: kwin did not create a socket" >&2; exit 2; }
export WAYLAND_DISPLAY=wayland-kwin

# Drag the first sub-window's title bar by $3,$4 pixels in $1 steps; keep the
# frames, the last one and the log
drag(){
  local steps="$1" tag="$2" movex="${3:-192}" movey="${4:-96}"
  local dump="$RUN/dump-$tag" log="$RUN/$tag.log"
  mkdir -p -- "$dump"
  # A home of its own per run: mditest saves the places of its sub-windows in
  # its registry when it exits, and the second run has to start from the same
  # places as the first one did
  export HOME="$RUN/home-$tag"
  export XDG_CONFIG_HOME="$RUN/config-$tag"
  mkdir -p -- "$HOME" "$XDG_CONFIG_HOME"
  FOX_WAYLAND_TRACE=1 FOX_WAYLAND_DUMP="$dump" FOX_WAYLAND_DUMP_FRAMES=1 "$BIN" --no-dialogs >"$log" 2>&1 &
  app_pid=$!
  sleep 2.5
  kill -0 "$app_pid" 2>/dev/null || { echo "error: mditest did not stay up; see $log" >&2; exit 2; }

  local screen sw sh cx cy
  screen=$(grep -m1 "connected, screen" "$log" | sed 's/.*screen //')
  sw=${screen%x*}; sh=${screen#*x}
  cx=$((sw/2)); cy=$((sh/2))

  # The pointer goes to a corner first, which is outside the window, and then to
  # the middle of the output; the enter event of that move carries the position
  # inside the surface and gives the origin of the client area.  A window KWin
  # is still placing would otherwise be measured at the place it had before
  "$OUT/kwininject" 2 2 move sleep:200 >/dev/null 2>&1
  sleep 0.2
  local before after enter lx ly ox oy
  before=$(grep -c "pointer enter window 2 at" "$log")
  "$OUT/kwininject" "$cx" "$cy" move sleep:400 >/dev/null 2>&1
  sleep 0.3
  after=$(grep -c "pointer enter window 2 at" "$log")
  [ "$after" -gt "$before" ] || { echo "error: the pointer did not reach mditest; see $log" >&2; exit 2; }
  enter=$(grep "pointer enter window 2 at" "$log" | tail -1 | sed 's/.*at //')
  [ -n "$enter" ] || { echo "error: the pointer did not reach mditest; see $log" >&2; exit 2; }
  lx=${enter%,*}; ly=${enter#*,}
  ox=$((cx-lx)); oy=$((cy-ly))

  # Title bar of the first sub-window: 10,10 in the client, the client starts
  # below the menu bar (28) and the bar is grabbed 10 pixels down
  local gx=$((ox+10+100)) gy=$((oy+28+10+10))
  local tx=$((gx+movex)) ty=$((gy+movey))
  local args=("move:$gx,$gy" sleep:300 down sleep:150)
  local i dx dy
  for i in $(seq 1 "$steps"); do
    dx=$(( (tx-gx)*i/steps )); dy=$(( (ty-gy)*i/steps ))
    args+=("move:$((gx+dx)),$((gy+dy))" sleep:130)
    done
  args+=(up sleep:500)
  "$OUT/kwininject" "$cx" "$cy" "${args[@]}" >/dev/null 2>&1
  sleep 0.8
  kill -0 "$app_pid" 2>/dev/null || { echo "FAIL: mditest ended during the drag; see $log" >&2; exit 1; }

  grep -q "button 1 down to FXMDIChild" "$log" || { echo "FAIL: the press missed the sub-window's title bar; see $log" >&2; exit 1; }
  local newest
  newest=$(ls -t "$dump"/*.ppm 2>/dev/null | head -1)
  [ -s "$newest" ] || { echo "FAIL: the drag produced no frame" >&2; exit 1; }
  cp -- "$newest" "$RUN/$tag.ppm"

  kill "$app_pid" 2>/dev/null
  wait "$app_pid" 2>/dev/null
  app_pid=
  }

echo "==> dragging in $STEPS steps"
drag "$STEPS" many
echo "==> dragging in 2 steps"
drag 2 few
echo "==> dragging to the top left corner"
drag 6 corner -320 -200

# A sub-window dragged to the corner hangs out of the store to the left and
# above.  A window whose origin is negative used to be handed a buffer that
# started at the corner instead, which moved its drawing origin and dropped the
# clip against the window above it, so its scrolled content grew over the whole
# client.  Everything below and right of the three sub-windows is client
# background, and it has to stay background in every frame of that drag
echo "==> checking the corner drag"
python3 - "$RUN/dump-corner" "$RUN/corner.log" <<'CORNER' || exit 1
import glob
import os
import re
import sys

dump, logpath = sys.argv[1], sys.argv[2]
lines = open(logpath, errors="replace").read().splitlines()
if not any("moved window" in line for line in lines):
    print("FAIL: the trace names no moving window; was the drag a move?")
    sys.exit(1)
if not any(re.search(r"origin -\d+", line) for line in lines):
    print("FAIL: the sub-window never reached a negative origin; see the log")
    sys.exit(1)


def readppm(path):
    data = open(path, "rb").read()
    head = data.split(b"\n", 3)
    w, h = map(int, head[1].split())
    return w, h, head[3]


files = sorted(glob.glob(os.path.join(dump, "*.ppm")),
               key=lambda p: int(re.search(r"-(\d+)\.ppm$", p).group(1)))
if len(files) < 3:
    print("FAIL: the corner drag produced %d frames, too few to check" % len(files))
    sys.exit(1)
w, h, ref = readppm(files[0])

# The sub-windows are 400x300 at 10,10 to 30,30, so all of the client below
# y=400 and right of x=460 is background
right = 460
below = 400
top = 28
bottom = h - 40
badframes = 0
for i, path in enumerate(files):
    w2, h2, pix = readppm(path)
    if (w2, h2) != (w, h):
        continue
    bad = 0
    for y in range(top, bottom):
        row = y * w * 3
        for x in range(4, w - 4):
            if x < right and y < below:
                continue
            o = row + x * 3
            if pix[o:o + 3] != ref[o:o + 3]:
                bad += 1
    if bad:
        badframes += 1
        print("frame %d (%s) has %d pixels of content outside the sub-windows"
              % (i, os.path.basename(path), bad))
if badframes:
    print("FAIL: %d of %d frames of the corner drag painted outside the sub-window"
          % (badframes, len(files)))
    sys.exit(1)
print("the client outside the sub-windows stays background in all %d frames" % len(files))
CORNER

if cmp -s "$RUN/many.ppm" "$RUN/few.ppm"; then
  echo "the two drags ended on the same frame"
else
  echo "FAIL: the $STEPS step drag left a different frame than the 2 step drag" >&2
  echo "      $RUN/many.ppm vs $RUN/few.ppm" >&2
  exit 1
fi

# Every frame of the many-step drag, not only the one it ends on
echo "==> checking every frame of the $STEPS step drag"
python3 - "$RUN/dump-many" "$RUN/many.log" "$STEPS" <<'PY' || exit 1
import collections
import glob
import os
import re
import sys

dump, logpath, steps = sys.argv[1], sys.argv[2], int(sys.argv[3])
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
commits = sum(1 for line in lines if "commit window 2 " in line)
if commits < steps:
    print("FAIL: %d drag steps produced only %d frames" % (steps, commits))
    sys.exit(1)
if commits > steps + 16:
    print("FAIL: %d drag steps produced %d frames; a step is committed more than once" % (steps, commits))
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
    for y in range(28, h):
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
print("%d drag steps, %d frames committed, all of them clean" % (steps, commits))
PY

echo "PASS: an MDI sub-window drag leaves no strip of an earlier place in any frame"
echo "PASS: a sub-window dragged to the corner keeps its content inside itself"
exit 0
