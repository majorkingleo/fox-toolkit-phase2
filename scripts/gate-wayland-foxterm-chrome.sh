#!/usr/bin/env bash
#
# FoxTerm gate: grid padding, the hairline and the transient status line.
#
# Step S2 of the visual plan: the terminal grid gets the theme padding on all
# four sides, the sunken frame around the switcher is gone and one hairline in
# ROLE_BORDER separates the tab strip from the grid, and the status line is no
# longer a permanent bar - it appears on child exit, on an error and while the
# find bar is open, and hides itself again.
#
# The gate measures committed frames:
#
#   1. the strip, then exactly one hairline row, then the grid; the four pixel
#      gaps the default FXPacker spacing used to leave must be gone
#   2. the grid background reaches the left and bottom window edge, and the
#      first glyph starts inside the padding, not at the edge
#   3. with closeonexit=false a child that exits brings the status line in at
#      the bottom, and it is gone again after its timeout
#
#   ./scripts/gate-wayland-foxterm-chrome.sh
#   ./scripts/gate-wayland-foxterm-chrome.sh --bin=FILE --build=DIR
#
# Exit status is 0 when the chrome matches the plan, 1 when it does not, 2 when
# the harness could not start.
#
set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

BUILD=tmp/build-fox2-wayland
BIN=
RUN=tmp/sway-foxterm-chrome

usage(){
  sed -n '2,24p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
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
mkdir -p -- "$RUN/dump"
RUN=$(cd -- "$RUN" && pwd)
chmod 700 "$RUN"
printf '%s\n' \
  'output HEADLESS-1 resolution 1000x620 bg #404040 solid_color' \
  'default_border pixel 0' >"$RUN/config"

export XDG_RUNTIME_DIR="$RUN"
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

# Keep the newest committed frame under a stable name
grab(){
  local name=$1 i frame=""
  for i in $(seq 1 100); do
    frame=$(ls -t "$RUN"/dump/*.ppm 2>/dev/null | head -1)
    [ -n "$frame" ] && break
    sleep 0.1
    done
  [ -n "$frame" ] || return 1
  sleep 0.3
  frame=$(ls -t "$RUN"/dump/*.ppm 2>/dev/null | head -1)
  cp "$frame" "$RUN/$name.ppm"
  }

seed_profile(){
  local home=$1 closeonexit=$2
  mkdir -p "$home/.config"
  cat >"$home/.config/FoxTerm.rc" <<EOF
[SETTINGS]
activeProfile=default

[PROFILE.default]
name=Default
shell=/bin/sh
term=xterm-256color
kind=1
scrollback=10000
cursor=0
closeonexit=$closeonexit
confirm=true
EOF
  }

fail=0
note(){
  echo "FAIL: $*" >&2
  fail=1
  }

# --- Phase 1: strip, hairline, grid at the window edge ----------------------
echo "==> phase 1: padding, hairline and no frame spacing"
seed_profile "$RUN/home-grid" true
HOME="$RUN/home-grid" FOX_WAYLAND_DUMP="$RUN/dump" "$BIN" --execute /bin/sh -c '
printf "sh-5.3$ echo hello\n"
printf "hello\n"
sleep 120' >"$RUN/app-grid.log" 2>&1 &
app_pid=$!
if grab grid1; then
  python3 - "$RUN/grid1.ppm" <<'EOF'
import sys
from PIL import Image

image = Image.open(sys.argv[1]).convert("RGB")
width, height = image.size
px = image.load()
grid = (12, 12, 12)

def columns(y):
    colors = []
    x = 0
    while x < width:
        c = px[x, y]
        x2 = x
        while x2 + 1 < width and px[x2 + 1, y] == c:
            x2 += 1
        colors.append((x, x2, c))
        x = x2 + 1
    return colors

# The grid starts where the center column turns into terminal background for
# good; above it must be exactly one hairline and then the strip
gridstart = None
for y in range(0, height - 5):
    if all(px[width // 2, y + k] == grid for k in range(0, 5)):
        gridstart = y
        break
if gridstart is None:
    print("FAIL: no grid background found at the center column")
    sys.exit(1)

line = gridstart - 1
strip = line
linecolor = px[width // 2, line]
stripcolor = px[width // 2, strip - 1]
print("    strip rows: %d, separator rows: 1, first grid row: %d" % (strip, gridstart))

fail = 0
if strip < 16 or 40 < strip:
    print("FAIL: the tab strip is %d rows high" % strip)
    fail = 1
if linecolor == grid or linecolor == stripcolor:
    print("FAIL: no hairline between the strip and the grid")
    fail = 1
if stripcolor == grid or px[width // 2, strip - 2] != stripcolor:
    print("FAIL: the band above the hairline is not one strip colour")
    fail = 1
if px[width // 2, 0] != stripcolor:
    print("FAIL: the strip does not start at the window edge")
    fail = 1

# The grid background must reach the bottom and the left edge
bottom = [px[x, height - 1] for x in range(width)]
if any(c != grid for c in bottom):
    print("FAIL: the last row is not terminal background; a status bar is still reserved")
    fail = 1
left = [px[x, gridstart + 4] for x in range(0, 3)]
if any(c != grid for c in left):
    print("FAIL: the padding on the left is not terminal background")
    fail = 1

# First glyph ink: inside the padding, not at the edge
def ink(x, y):
    r, g, b = px[x, y]
    return abs(r - grid[0]) + abs(g - grid[1]) + abs(b - grid[2]) > 24

first_x = None
first_y = None
for y in range(gridstart + 1, min(height, gridstart + 60)):
    for x in range(width):
        if ink(x, y):
            first_x = x
            first_y = y - gridstart
            break
    if first_x is not None:
        break
print("    first glyph at x=%s, %s rows below the separator" % (first_x, first_y))
if first_x is None or first_x < 6:
    print("FAIL: the grid has no padding on the left")
    fail = 1
if first_y is None or first_y < 6 or 20 < first_y:
    print("FAIL: the grid has no padding below the separator")
    fail = 1
sys.exit(fail)
EOF
  [ $? -eq 0 ] || fail=1
else
  note "no frame was committed"
  fi
# Selection fills cells only; the padding stays terminal background
echo "    select all: the padding must stay background"
swaymsg '[title="FoxTerm - Terminal"] focus' >/dev/null 2>&1
sleep 0.4
if command -v wtype >/dev/null 2>&1; then
  wtype -s 250 -M ctrl -M shift -k a -m shift -m ctrl
  sleep 0.8
  if grab grid2; then
    python3 - "$RUN/grid2.ppm" <<'EOF'
import sys
from PIL import Image

image = Image.open(sys.argv[1]).convert("RGB")
width, height = image.size
px = image.load()
grid = (12, 12, 12)

# The leftmost padding columns must still be terminal background, a few rows
# into the grid where a selection would be painted
gridstart = None
for y in range(0, height - 5):
    if all(px[width // 2, y + k] == grid for k in range(0, 5)):
        gridstart = y
        break
row = gridstart + 12
pad = [px[x, row] for x in range(0, 6)]
selected = px[40, row] != grid
print("    padding row: %s, cell selected: %s" % (pad, selected))
fail = 0
if any(c != grid for c in pad):
    print("FAIL: the selection paints into the padding")
    fail = 1
if not selected:
    print("FAIL: select all did not reach the grid")
    fail = 1
sys.exit(fail)
EOF
    [ $? -eq 0 ] || fail=1
  else
    note "no frame after select all"
    fi
else
  echo "    wtype not installed, selection check skipped"
  fi

kill "$app_pid" 2>/dev/null
wait "$app_pid" 2>/dev/null
app_pid=

# --- Phase 2: the status line is transient -------------------------------
echo "==> phase 2: status line on exit, then gone"
seed_profile "$RUN/home-status" false
HOME="$RUN/home-status" FOX_WAYLAND_DUMP="$RUN/dump" "$BIN" --execute /bin/sh -c '
printf "before exit\n"; sleep 1; exit 3' >"$RUN/app-status.log" 2>&1 &
app_pid=$!
sleep 2.6
if grab status_on; then
  shown=$(python3 - "$RUN/status_on.ppm" <<'EOF'
import sys
from PIL import Image
image = Image.open(sys.argv[1]).convert("RGB")
width, height = image.size
px = image.load()
grid = (12, 12, 12)
print(1 if px[width // 2, height - 1] != grid else 0)
EOF
)
  if [ "$shown" = 1 ]; then
    echo "    the status line appeared after the child exited"
  else
    note "no status line after the child exited"
    fi
else
  note "no frame after the child exited"
  fi
sleep 4.5
if grab status_off; then
  hidden=$(python3 - "$RUN/status_off.ppm" <<'EOF'
import sys
from PIL import Image
image = Image.open(sys.argv[1]).convert("RGB")
width, height = image.size
px = image.load()
grid = (12, 12, 12)
print(1 if px[width // 2, height - 1] == grid else 0)
EOF
)
  if [ "$hidden" = 1 ]; then
    echo "    the status line hid itself again"
  else
    note "the status line stayed on screen"
    fi
else
  note "no frame after the status timeout"
  fi
kill "$app_pid" 2>/dev/null
wait "$app_pid" 2>/dev/null
app_pid=

if [ "$fail" -ne 0 ]; then
  echo "FAIL: see $RUN" >&2
  exit 1
  fi
echo "PASS: the grid padding, the hairline and the transient status line are correct"
exit 0
