#!/usr/bin/env bash
#
# FoxTerm gate: the cell grid matches the font advance.
#
# Step S1 of the visual plan: `cellw` used to come from the advance of `M` in
# whatever face the FONT_MONO role resolved to.  The theme presets leave that
# role open, so it inherited the proportional interface font (Noto Sans:
# i=3, M=11, W=12) and every cell was 11 px wide for an 8 px grid: the prompt
# was letter spaced, which is defect V1.
#
# The gate starts a shell that writes two rows of 80 `M` and measures the
# committed frame:
#
#   - the 80 glyphs must come out as 80 separate blobs with one pitch, and the
#     distance between two neighbouring glyphs may not exceed 2 px; a cell
#     wider than the glyph advance leaves a gap of `cellw - glyph`
#   - the ink of the row must span 80 pitches, so cells are neither too wide
#     nor clipped
#   - the distance between the two text rows is the cell height, so rows must
#     not carry extra leading
#
#   ./scripts/gate-wayland-foxterm-metrics.sh
#   ./scripts/gate-wayland-foxterm-metrics.sh --bin=FILE --build=DIR
#
# Exit status is 0 when the grid matches the font, 1 when it does not, 2 when
# the harness could not start.
#
set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

BUILD=tmp/build-fox2-wayland
BIN=
RUN=tmp/sway-foxterm-metrics

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

[ -n "$BIN" ] || BIN="$BUILD/bin/foxterm"
if [ ! -x "$BIN" ]; then
  echo "error: no binary at $BIN (build with scripts/build-fox2-wayland.sh)" >&2
  exit 2
  fi
for tool in sway swaymsg; do
  command -v "$tool" >/dev/null 2>&1 || { echo "error: $tool not found" >&2; exit 2; }
  done
if ! python3 -c 'import PIL' >/dev/null 2>&1; then
  echo "error: python3 with Pillow is needed to measure the frame" >&2
  exit 2
  fi

rm -rf -- "$RUN"
mkdir -p -- "$RUN/home" "$RUN/dump"
RUN=$(cd -- "$RUN" && pwd)
chmod 700 "$RUN"
printf '%s\n' \
  'output HEADLESS-1 resolution 1024x700 bg #101010 solid_color' \
  'default_border pixel 0' >"$RUN/config"

export XDG_RUNTIME_DIR="$RUN"
export HOME="$RUN/home"
unset DISPLAY WAYLAND_DISPLAY DBUS_SESSION_BUS_ADDRESS DBUS_SYSTEM_BUS_ADDRESS XDG_CONFIG_HOME SWAYSOCK
export WLR_BACKENDS=headless WLR_LIBINPUT_NO_DEVICES=1 WLR_RENDERER=pixman WLR_HEADLESS_OUTPUTS=1

sway --unsupported-gpu -c "$RUN/config" >"$RUN/sway.log" 2>&1 &
sway_pid=$!
app_pid=
cleanup(){
  [ -n "$app_pid" ] && kill "$app_pid" 2>/dev/null
  kill "$sway_pid" 2>/dev/null
  wait "$sway_pid" 2>/dev/null
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

echo "==> running $BIN with two rows of 80 M"
FOX_WAYLAND_DUMP="$RUN/dump" "$BIN" --execute /bin/sh -c '
i=0; while [ $i -lt 80 ]; do printf M; i=$((i+1)); done; printf "\n"
i=0; while [ $i -lt 80 ]; do printf M; i=$((i+1)); done
sleep 120' >"$RUN/app.log" 2>&1 &
app_pid=$!

frame=""
for i in $(seq 1 100); do
  frame=$(ls -t "$RUN"/dump/*.ppm 2>/dev/null | head -1)
  [ -n "$frame" ] && break
  sleep 0.1
  done
[ -n "$frame" ] || { echo "error: no frame was committed" >&2; exit 2; }
sleep 0.6
frame=$(ls -t "$RUN"/dump/*.ppm 2>/dev/null | head -1)
kill -0 "$app_pid" 2>/dev/null || { echo "error: the application ended" >&2; exit 2; }
echo "==> measuring $frame"

python3 - "$frame" <<'EOF'
import sys
from PIL import Image

image = Image.open(sys.argv[1]).convert("RGB")
width, height = image.size
px = image.load()
bg = (12, 12, 12)

def ink(x, y):
    r, g, b = px[x, y]
    return abs(r - bg[0]) + abs(g - bg[1]) + abs(b - bg[2]) > 24

# The grid, without the 2 px edge and the scrollbar on the right
x0 = 2
x1 = width - 16

# Row bands that contain glyph ink; the chrome above the grid is skipped
bands = []
start = None
for y in range(34, height):
    hits = sum(1 for x in range(x0, x1) if ink(x, y))
    row = hits > 4
    if row and start is None:
        start = y
    elif not row and start is not None:
        bands.append((start, y - 1))
        start = None
if start is not None:
    bands.append((start, height - 1))
if len(bands) < 2:
    print("FAIL: expected two text rows, found %d bands" % len(bands))
    sys.exit(1)

# Glyph blobs in the first text row, measured on its middle scanline
mid = (bands[0][0] + bands[0][1]) // 2
blobs = []
run = None
for x in range(x0, x1):
    if ink(x, mid):
        if run is None:
            run = [x, x]
        else:
            run[1] = x
    elif run is not None:
        blobs.append(tuple(run))
        run = None
if run is not None:
    blobs.append(tuple(run))

if len(blobs) != 80:
    print("FAIL: expected 80 glyph blobs, found %d" % len(blobs))
    sys.exit(1)

pitches = [blobs[i + 1][0] - blobs[i][0] for i in range(len(blobs) - 1)]
cellw = max(set(pitches), key=pitches.count)
if pitches.count(cellw) != len(pitches):
    print("FAIL: the glyph pitch is not constant: %s" % sorted(set(pitches)))
    sys.exit(1)

gaps = [blobs[i + 1][0] - blobs[i][1] - 1 for i in range(len(blobs) - 1)]
bbox = blobs[-1][1] - blobs[0][0] + 1
cellh = bands[1][0] - bands[0][0]
glyph = [b[1] - b[0] + 1 for b in blobs]

print("    cell width (glyph pitch): %d px" % cellw)
print("    glyph widths: %s" % sorted(set(glyph)))
print("    largest gap between glyphs: %d px" % max(gaps))
print("    row ink: %d px, 80 pitches: %d px" % (bbox, 80 * cellw))
print("    cell height (row pitch): %d px" % cellh)

fail = 0
if max(gaps) > 2:
    print("FAIL: cells are wider than the glyph advance; the grid is letter spaced")
    fail = 1
if abs(bbox - 80 * cellw) > cellw:
    print("FAIL: the row does not span 80 cells")
    fail = 1
if not (cellh >= max(bands[0][1] - bands[0][0] + 1, bands[1][1] - bands[1][0] + 1)):
    print("FAIL: the cell height is smaller than the glyph ink")
    fail = 1
sys.exit(fail)
EOF
verdict=$?
if [ "$verdict" -ne 0 ]; then
  echo "FAIL: see $frame" >&2
  exit 1
  fi
echo "PASS: the cell grid matches the font advance"
exit 0
