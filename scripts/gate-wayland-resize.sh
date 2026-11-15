#!/usr/bin/env bash
#
# Resize gate: the first frame after a resize already has the new layout.
#
# The xdg_surface configure used to expose the window before the core had
# seen the new size, and the configure dispatch exposed the tree before the
# delayed layout chore had run.  The first frame committed at the new size
# therefore showed the widgets at their old places, and that frame stayed on
# screen until a later expose happened (a second configure, a pointer move, or
# the layout chore a moment later), which is what made a maximized calculator
# look wrong for a while.
#
# The gate resizes the calculator, keeps every frame that is committed at the
# new size, and requires the first one to match the settled layout (the
# committed reference) and the last one.  Copying beats the second commit
# because the dump only ever holds the latest frame.  The module trace is on:
# its output slows the event loop enough that the old ordering commits the
# wrong first frame every time, instead of only on a slow compositor.
#
#   ./scripts/gate-wayland-resize.sh
#   ./scripts/gate-wayland-resize.sh --bin=FILE --build=DIR
#
# Exit status is 0 when the first frame is right, 1 when it is not, 2 when the
# harness could not start.
#
set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

BUILD=tmp/build-fox2-wayland
BIN=
RUN=tmp/sway-resize-gate
REF=screenshots/tests/fox2/wayland/calculator-800x520.png

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
    --ref)     shift; REF=$1 ;;
    --ref=*)   REF=${1#*=} ;;
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
[ -f "$REF" ] || { echo "error: reference image $REF is missing" >&2; exit 2; }
for tool in sway swaymsg; do
  command -v "$tool" >/dev/null 2>&1 || { echo "error: $tool not found" >&2; exit 2; }
  done
if ! python3 -c 'import PIL' >/dev/null 2>&1; then
  echo "error: python3 with Pillow is needed to check the frames" >&2
  exit 2
  fi

rm -rf -- "$RUN"
mkdir -p -- "$RUN/home" "$RUN/dump" "$RUN/frames"
RUN=$(cd -- "$RUN" && pwd)
chmod 700 "$RUN"
printf '%s\n' \
  'output HEADLESS-1 resolution 1024x800 bg #101010 solid_color' \
  'default_border pixel 0' >"$RUN/config"

export XDG_RUNTIME_DIR="$RUN"
export HOME="$RUN/home"
unset DISPLAY WAYLAND_DISPLAY DBUS_SESSION_BUS_ADDRESS DBUS_SYSTEM_BUS_ADDRESS XDG_CONFIG_HOME SWAYSOCK
export WLR_BACKENDS=headless WLR_LIBINPUT_NO_DEVICES=1 WLR_RENDERER=pixman WLR_HEADLESS_OUTPUTS=1

sway --unsupported-gpu -c "$RUN/config" >"$RUN/sway.log" 2>&1 &
sway_pid=$!
cleanup(){
  kill "${watcher:-}" 2>/dev/null || true
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
export SWAYSOCK=$(ls "$RUN"/sway-ipc.*.sock 2>/dev/null | head -1)

echo "==> running $BIN"
FOX_WAYLAND_TRACE=1 FOX_WAYLAND_DUMP="$RUN/dump" "$BIN" >"$RUN/app.log" 2>&1 &
app_pid=$!
sleep 2
kill -0 "$app_pid" 2>/dev/null || { echo "error: the application did not stay up" >&2; exit 2; }

swaymsg '[title="FOX Calculator"] floating enable' >/dev/null 2>&1
swaymsg '[title="FOX Calculator"] move position 0 0' >/dev/null 2>&1
sleep 0.6
[ -f "$RUN/dump/window-2-400x260.ppm" ] || { echo "error: no initial frame" >&2; exit 2; }

# Copy each committed version of the file the moment it appears
cat >"$RUN/watch.py" <<'PY'
import os,shutil,sys,time
from PIL import Image
path,out,want=sys.argv[1],sys.argv[2],int(sys.argv[3])
last=None
count=0
deadline=time.time()+8.0
while time.time()<deadline and count<want:
    try:
        st=os.stat(path)
    except OSError:
        time.sleep(0.001)
        continue
    key=(st.st_mtime_ns,st.st_size)
    if key!=last:
        dst='%s/%d.ppm'%(out,count)
        # The dump writes the file in one go, but a copy can still catch it
        # half-way; keep trying until Pillow reads it whole
        for attempt in range(200):
            try:
                shutil.copyfile(path,dst)
                im=Image.open(dst)
                im.load()
                break
            except (OSError,IOError):
                time.sleep(0.001)
        else:
            time.sleep(0.001)
            continue
        last=key
        count+=1
        if count>=want:
            break
    time.sleep(0.001)
print(count)
PY

echo "==> resizing to 800x520"
python3 "$RUN/watch.py" "$RUN/dump/window-2-800x520.ppm" "$RUN/frames" 2 >"$RUN/watch.out" 2>&1 &
watcher=$!
sleep 0.2
swaymsg '[title="FOX Calculator"] resize set 800 520' >/dev/null 2>&1
wait "$watcher" 2>/dev/null
frames=$(cat "$RUN/watch.out" 2>/dev/null)
[ -n "$frames" ] || frames=0

# If only one frame was committed, force another repaint by resizing again
if [ "$frames" -lt 2 ]; then
  python3 "$RUN/watch.py" "$RUN/dump/window-2-800x520.ppm" "$RUN/frames" 1 >"$RUN/watch2.out" 2>&1 &
  watcher=$!
  sleep 0.2
  swaymsg '[title="FOX Calculator"] resize set 800 520' >/dev/null 2>&1
  wait "$watcher" 2>/dev/null
  if [ -f "$RUN/frames/1.ppm" ]; then
    frames=$((frames+1))
  fi
  fi
[ -f "$RUN/frames/0.ppm" ] || { echo "error: the resized window committed no frame" >&2; exit 2; }
if [ ! -f "$RUN/frames/1.ppm" ]; then
  sleep 0.5
  cp "$RUN/dump/window-2-800x520.ppm" "$RUN/frames/1.ppm"
  fi

echo "==> comparing the first frame with the reference and the later frame"
python3 - "$RUN/frames/0.ppm" "$RUN/frames/1.ppm" "$REF" <<'PY'
import sys
from PIL import Image
a=Image.open(sys.argv[1]).convert("RGB")
b=Image.open(sys.argv[2]).convert("RGB")
r=Image.open(sys.argv[3]).convert("RGB")
for name,im in (("first",a),("later",b),("reference",r)):
    if im.size!=(800,520):
        print("FAIL: the %s frame is %dx%d, not 800x520"%(name,im.width,im.height))
        sys.exit(1)
def differ(x,y):
    return sum(1 for px,py in zip(x.getdata(),y.getdata()) if px!=py)
d1=differ(a,r)
d2=differ(a,b)
if d1:
    print("FAIL: the first frame at the new size does not match the settled layout (%d pixels differ)"%d1)
    sys.exit(1)
if d2:
    print("FAIL: the first frame at the new size differs from the later one (%d pixels differ)"%d2)
    sys.exit(1)
print("the first frame already has the new layout")
sys.exit(0)
PY
verdict=$?
if [ "$verdict" -ne 0 ]; then
  exit 1
  fi

kill -0 "$app_pid" 2>/dev/null || { echo "FAIL: the application ended" >&2; exit 1; }
echo "PASS: the first frame after a resize is laid out correctly"
exit 0
