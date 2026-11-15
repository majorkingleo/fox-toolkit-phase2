#!/usr/bin/env bash
#
# Resize drag gate: drag a window border on KWin and watch the frame pacing.
#
# The calculator bugs around resizing (stale first frame, a CPU storm, a
# backlog of configures, commits that waited for the drag to pause) were all
# found by dragging the window on a real compositor; sway cannot reproduce
# them because it coalesces its own configure stream.  KWin does not, and it
# exposes org_kde_kwin_fake_input, so tools/kwininject can press the bottom
# right border and move it like a user would.
#
# The gate runs the calculator under a private KWin virtual backend, drags the
# border out and requires that the window grew, that every applied configure
# is committed (a couple of frames of slack), and that no gap between two
# commits is longer than 150 ms.  Before the frames were flushed while the
# event queue was still busy, a drag could paint for hundreds of milliseconds
# without the compositor ever seeing the result.
#
#   ./scripts/gate-wayland-resize-drag.sh
#   ./scripts/gate-wayland-resize-drag.sh --bin=FILE --build=DIR
#
# Exit status is 0 when the drag is paced, 1 when it is not, 2 when the harness
# could not start (no KWin, no fake input).
#
set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

BUILD=tmp/build-fox2-wayland
BIN=
RUN=tmp/kwin-resize-drag-gate
OUT=tmp/kwininject
XML=scripts/protocols/org-kde-kwin-fake-input.xml
MAXGAP=0.150

usage(){
  sed -n '2,25p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
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
if ! command -v kwin_wayland >/dev/null 2>&1; then
  echo "skip: kwin_wayland is not installed" >&2
  exit 2
  fi
for tool in wayland-scanner cc; do
  command -v "$tool" >/dev/null 2>&1 || { echo "error: $tool not found" >&2; exit 2; }
  done

mkdir -p -- "$OUT"
wayland-scanner client-header "$XML" "$OUT/org-kde-kwin-fake-input-client-protocol.h" || exit 2
wayland-scanner private-code "$XML" "$OUT/fake-input-protocol.c" || exit 2
cc -O1 -Wall -I"$OUT" -o "$OUT/kwininject" tools/kwininject.c "$OUT/fake-input-protocol.c" -lwayland-client || exit 2

rm -rf -- "$RUN"
mkdir -p -- "$RUN/home" "$RUN/dump"
RUN=$(cd -- "$RUN" && pwd)
chmod 700 "$RUN"

export XDG_RUNTIME_DIR="$RUN"
export HOME="$RUN/home"
unset DISPLAY WAYLAND_DISPLAY DBUS_SESSION_BUS_ADDRESS DBUS_SYSTEM_BUS_ADDRESS XDG_CONFIG_HOME
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

echo "==> running $BIN"
FOX_WAYLAND_TRACE=1 FOX_WAYLAND_DUMP="$RUN/dump" "$BIN" >"$RUN/app.log" 2>&1 &
app_pid=$!
sleep 2
kill -0 "$app_pid" 2>/dev/null || { echo "error: the application did not stay up" >&2; exit 2; }

screen=$(grep -m1 "connected, screen" "$RUN/app.log" | sed 's/.*screen //')
[ -n "$screen" ] || { echo "error: the application reported no screen size" >&2; exit 2; }
sw=${screen%x*}; sh=${screen#*x}
cx=$((sw/2)); cy=$((sh/2))

# KWin centres the window; a pointer move to the middle of the output lands in
# it, and the enter event carries the position inside the surface, which gives
# the origin of the client area
echo "==> finding the window"
"$OUT/kwininject" "$cx" "$cy" move sleep:300 >/dev/null 2>&1
sleep 0.3
enter=$(grep "pointer enter window 2 at" "$RUN/app.log" | tail -1 | sed 's/.*at //')
[ -n "$enter" ] || { echo "error: the pointer did not reach the calculator; see $RUN/app.log" >&2; exit 2; }
lx=${enter%,*}; ly=${enter#*,}
ox=$((cx-lx)); oy=$((cy-ly))
echo "window origin $ox,$oy on ${sw}x${sh}"

# Drag the bottom right border (the client area plus a couple of border pixels)
echo "==> dragging the border"
args=("move:$((ox+400+2)),$((oy+260+2))" sleep:300 down sleep:100)
for i in $(seq 1 20); do
  args+=("move:$((ox+400+2+i*12)),$((oy+260+2+i*9))" sleep:15)
done
args+=(up sleep:800)
"$OUT/kwininject" "$cx" "$cy" "${args[@]}" >/dev/null 2>&1
sleep 0.5

kill -0 "$app_pid" 2>/dev/null || { echo "FAIL: the application ended during the drag" >&2; exit 1; }

python3 - "$RUN/app.log" "$MAXGAP" <<'PY'
import re,sys
log,gapmax=sys.argv[1],float(sys.argv[2])
lines=open(log,errors="replace").read().splitlines()
def stamp(line):
    m=re.search(r'\[(\d+\.\d+)\]',line)
    return float(m.group(1)) if m else None
def size(line):
    m=re.search(r'configure window 2 (\d+)x(\d+)',line)
    return (int(m.group(1)),int(m.group(2))) if m else None
# The drag starts at the first configure that is not the 400x260 window
start=None
for line in lines:
    if 'toplevel configure 2' in line:
        m=re.search(r'toplevel configure 2 (\d+)x(\d+)',line)
        w,h=int(m.group(1)),int(m.group(2))
        if m and w>0 and h>0 and (w,h)!=(400,260):
            start=stamp(line)
            break
if start is None:
    print("FAIL: the drag produced no configure (the border was not grabbed)")
    sys.exit(1)
disp=[stamp(l) for l in lines if 'dispatch configure window 2' in l and stamp(l) and stamp(l)>=start]
comm=[stamp(l) for l in lines if 'commit window 2' in l and stamp(l) and stamp(l)>=start]
if not disp:
    print("FAIL: no configure was applied")
    sys.exit(1)
if len(comm)+2 < len(disp):
    print("FAIL: %d configures applied but only %d frames committed"%(len(disp),len(comm)))
    sys.exit(1)
times=[start]+comm
gap=max(b-a for a,b in zip(times,times[1:])) if len(times)>1 else float('inf')
if gap>gapmax:
    print("FAIL: %.0f ms passed between two commits during the drag (limit %.0f ms)"%(gap*1000,gapmax*1000))
    sys.exit(1)
# The last committed frame must be the larger window
last=None
for line in lines:
    m=re.search(r'commit window 2 (\d+)x(\d+)',line)
    if m:
        last=(int(m.group(1)),int(m.group(2)))
if not last or (last[0]<=400 and last[1]<=260):
    print("FAIL: the window did not grow (last commit %s)"%(last,))
    sys.exit(1)
print("%d configures applied, %d frames committed, largest commit gap %.0f ms"%(len(disp),len(comm),gap*1000))
sys.exit(0)
PY
verdict=$?
if [ "$verdict" -ne 0 ]; then
  echo "FAIL: the drag is not paced" >&2
  exit 1
  fi

echo "PASS: a border drag on KWin is laid out and committed in pace"
exit 0
