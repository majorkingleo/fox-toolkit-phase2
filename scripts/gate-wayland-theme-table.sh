#!/usr/bin/env bash
#
# Themed table gate:  the column header survives a scroll.
#
# All windows of a shell paint into one buffer on this platform, so a window has
# to keep its painting inside the area its children leave for it.  The table
# paints its cells itself, and the cell that is partly scrolled out of the
# viewport starts above the header; when the cell clip is not clamped to the
# viewport the cells paint over the column header and it disappears as soon as
# the grid scrolls (this gate was written after exactly that).
#
# The gate opens the table test with a themed registry, keeps the frame, drags
# the vertical scroll bar down and compares the column header band of the two
# frames: the band does not scroll vertically, so it has to be identical.
#
#   ./scripts/gate-wayland-theme-table.sh
#   ./scripts/gate-wayland-theme-table.sh --theme=/usr/share/themes/Adwaita
#
set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

BUILD=tmp/build-fox2-wayland
RUN=tmp/sway-theme-table
OUT=tmp/wlxinject
DUMP=
THEME=/usr/share/themes/Breeze
SRC=tools/wlxinject.c
XML=scripts/protocols/wlr-virtual-pointer-unstable-v1.xml

usage(){
  sed -n '2,20p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
  }

while [ $# -gt 0 ]; do
  case "$1" in
    --build)   shift; BUILD=$1 ;;
    --build=*) BUILD=${1#*=} ;;
    --run-dir) shift; RUN=$1 ;;
    --run-dir=*) RUN=${1#*=} ;;
    --theme)   shift; THEME=$1 ;;
    --theme=*) THEME=${1#*=} ;;
    -h|--help) usage; exit 0 ;;
    *) echo "error: unknown option '$1'" >&2; usage >&2; exit 2 ;;
    esac
  shift
  done

for tool in sway swaymsg wayland-scanner cc python3; do
  command -v "$tool" >/dev/null 2>&1 || { echo "error: $tool not found" >&2; exit 2; }
  done
[ -x "$BUILD/bin/table" ] || { echo "error: no binary at $BUILD/bin/table" >&2; exit 2; }
[ -d "$THEME" ] || { echo "error: no theme at $THEME" >&2; exit 2; }

mkdir -p -- "$OUT"
wayland-scanner private-code "$XML" "$OUT/vptr-protocol.c" || exit 2
wayland-scanner client-header "$XML" "$OUT/wlr-virtual-pointer-unstable-v1-client-protocol.h" || exit 2
cc -O1 -Wall -I"$OUT" -o "$OUT/wlxinject" "$SRC" "$OUT/vptr-protocol.c" -lwayland-client || exit 2

rm -rf -- "$RUN"
mkdir -p -- "$RUN/home" "$RUN/registry" "$RUN/dump"
RUN=$(cd -- "$RUN" && pwd)
chmod 700 "$RUN"
printf '[SETTINGS]\ntheme=%s\n[display]\nscale=100\n' "$THEME" >"$RUN/registry/fox.rc"
printf '%s\n' \
  'output HEADLESS-1 resolution 1280x800 bg #101010 solid_color' \
  'default_border pixel 0' >"$RUN/config"

export XDG_RUNTIME_DIR="$RUN"
export HOME="$RUN/home"
unset DISPLAY WAYLAND_DISPLAY DBUS_SESSION_BUS_ADDRESS XDG_CONFIG_HOME SWAYSOCK
export WLR_BACKENDS=headless WLR_LIBINPUT_NO_DEVICES=1 WLR_RENDERER=pixman WLR_HEADLESS_OUTPUTS=1
export WLXINJECT_WIDTH=1280 WLXINJECT_HEIGHT=800

sway --unsupported-gpu -c "$RUN/config" >"$RUN/sway.log" 2>&1 &
sway_pid=$!
app_pid=""
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
export SWAYSOCK=$(ls "$RUN"/sway-ipc.*.sock 2>/dev/null | head -1)

echo "==> table test, theme $THEME"
FOXDIR="$RUN/registry" FOX_WAYLAND_DUMP="$RUN/dump" "$BUILD/bin/table" >"$RUN/app.log" 2>&1 &
app_pid=$!
sleep 2.0
kill -0 "$app_pid" 2>/dev/null || { echo "error: the application did not stay up" >&2; exit 2; }

swaymsg '[title="Table Widget Test"] floating enable' >/dev/null 2>&1
swaymsg '[title="Table Widget Test"] move position 0 0' >/dev/null 2>&1
sleep 0.6

rect=$(swaymsg -t get_tree | python3 -c "
import json,sys
t=json.load(sys.stdin)
def walk(n):
    if n.get('name')=='Table Widget Test':
        r=n['rect']; print('%d %d %d %d'%(r['x'],r['y'],r['width'],r['height']))
    for c in n.get('nodes',[])+n.get('floating_nodes',[]): walk(c)
walk(t)")
[ -n "$rect" ] || { echo "error: no table window" >&2; exit 2; }
set -- $rect; wx=$1; wy=$2; ww=$3; wh=$4
echo "==> window ${ww}x${wh} at $wx,$wy"

frame=""
for i in $(seq 1 100); do
  frame=$(ls "$RUN"/dump/window-*-"${ww}x${wh}".ppm 2>/dev/null | head -1)
  [ -n "$frame" ] && break
  sleep 0.1
  done
[ -n "$frame" ] || { echo "error: the window committed no frame" >&2; exit 2; }
cp -- "$frame" "$RUN/before.ppm"

echo "==> dragging the vertical scroll bar down"
sx=$((wx+ww-10)); sy=$((wy+100))
"$OUT/wlxinject" "$sx" "$sy" move sleep:200 down sleep:200 "move:$sx,$((sy+60))" sleep:200 \
                 "move:$sx,$((sy+200))" sleep:300 up sleep:600 >/dev/null 2>&1

after=""
for i in $(seq 1 100); do
  after=$(ls -t "$RUN"/dump/window-*-"${ww}x${wh}".ppm 2>/dev/null | head -1)
  if [ -n "$after" ] && ! cmp -s -- "$frame" "$after"; then break; fi
  sleep 0.1
  done
[ -n "$after" ] || { echo "error: the window committed no frame after the scroll" >&2; exit 2; }
cp -- "$after" "$RUN/after.ppm"

python3 - "$RUN/before.ppm" "$RUN/after.ppm" <<'EOF'
import sys

# The column header band of the table test, in the coordinates of its frame:
# below the menu bar and above the first grid row, right of the row header and
# left of the vertical scroll bar
BAND_Y0, BAND_Y1 = 35, 69
BAND_X0, BAND_X1 = 130, 883
SURFACE = (0xef, 0xf0, 0xf1)

def load(path):
    with open(path,'rb') as f:
        data=f.read()
    head, pixels = data.split(b'\n',3)[:3], data.split(b'\n',3)[3]
    magic, size, maxval = head
    w,h = map(int, size.split())
    return w,h,pixels

def band(path):
    w,h,pixels = load(path)
    rows=[]
    for y in range(BAND_Y0, min(BAND_Y1,h)):
        rows.append(pixels[(y*w+BAND_X0)*3:(y*w+min(BAND_X1,w))*3])
    return w,h,rows

def surface_fraction(rows,w):
    total=0; surface=0
    for row in rows:
        for i in range(0,len(row),3):
            total+=1
            if (row[i],row[i+1],row[i+2])==SURFACE: surface+=1
    return surface/max(total,1)

w0,h0,before = band(sys.argv[1])
w1,h1,after  = band(sys.argv[2])
if (w0,h0)!=(w1,h1):
    print("FAIL: the two frames are not the same size")
    raise SystemExit(1)

fraction=surface_fraction(before,w0)
if fraction<0.60:
    print("FAIL: the column header band is not where the gate looks for it (%.0f%% surface)" % (100*fraction))
    raise SystemExit(1)

differing=0
for a,b in zip(before,after):
    differing+=sum(1 for x,y in zip(a,b) if x!=y)
if differing:
    print("FAIL: the column header changed while the grid scrolled (%d bytes)" % differing)
    raise SystemExit(1)

print("PASS: the column header survives the scroll (%d rows compared)" % len(before))
EOF
exit $?
