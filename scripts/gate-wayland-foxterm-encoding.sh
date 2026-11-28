#!/usr/bin/env bash
#
# FoxTerm gate: the character encoding switch (requirement R20, plan item N2).
#
# A session decodes its output and encodes its input through one character
# encoding.  UTF-8 is the default; the context menu selects another one live,
# without restarting the child and without clearing the screen, and the profile
# selects the encoding of a new tab.  This gate checks that end to end:
#
#   1. the same byte (0xE4) drawn by the child becomes a replacement character
#      in UTF-8 and a letter in ISO-8859-1, chosen from the menu; the rows that
#      were already on the screen stay pixel-identical, so the switch neither
#      clears nor re-decodes drawn cells, and only the cell of the byte changes,
#      so a single byte encoding puts one byte in one cell;
#   2. switching back to UTF-8 renders the next line as UTF-8 again;
#   3. keys are encoded too: typing an umlaut sends three UTF-8 bytes and one
#      Latin-1 byte, read back from the child rather than from the screen;
#   4. a pasted character the encoding cannot express reaches the child as '?'
#      and the status line says so; the same paste in UTF-8 sends its UTF-8
#      bytes and says nothing;
#   5. a profile with encoding=ISO-8859-1 starts a session that renders the
#      byte like the live switch did.
#
# The status line is the witness for the menu entries: showStatus() is the only
# effect a successfully activated encoding entry has, and its bar is the only
# light plate at the bottom of the window.
#
#   ./scripts/gate-wayland-foxterm-encoding.sh
#   ./scripts/gate-wayland-foxterm-encoding.sh --bin=FILE --build=DIR
#
# Exit status is 0 when all phases pass, 1 when one fails, 2 when the harness
# could not start.
#
set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

BUILD=tmp/build-fox2-wayland
BIN=
RUN=tmp/sway-foxterm-encoding
OUT=tmp/wlxinject
SRC=tools/wlxinject.c
XML=scripts/protocols/wlr-virtual-pointer-unstable-v1.xml
CLIP=scripts/wlclip.c
DC_XML=scripts/protocols/wlr-data-control-unstable-v1.xml

# Geometry of the tiled window on the headless output: 26 rows of tab strip,
# one hairline, then the grid with 6 px padding and an 8x18 cell.
GRID=300,120             # a cell in the upper half of the grid
CASCADE=320,273          # the Encoding cascade of the menu posted at GRID
ROW0=33                  # top of the first grid row
CELLW=8
CELLH=18
COL0=6
BYTECOL=3                # the byte sits at column 3 of "AAA<0xE4>BBB"

LATIN1=1                 # menu index of ISO-8859-1
UTF8=0

usage(){
  sed -n '2,40p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
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
for tool in sway swaymsg wtype grim wayland-scanner cc python3; do
  command -v "$tool" >/dev/null 2>&1 || { echo "error: $tool not found" >&2; exit 2; }
  done
if ! python3 -c 'import PIL' >/dev/null 2>&1; then
  echo "error: python3 with Pillow is needed to measure the frames" >&2
  exit 2
  fi

# The pointer injector and the clipboard peer ------------------------------
mkdir -p -- "$OUT"
wayland-scanner private-code "$XML" "$OUT/vptr-protocol.c" || exit 2
wayland-scanner client-header "$XML" "$OUT/wlr-virtual-pointer-unstable-v1-client-protocol.h" || exit 2
wayland-scanner private-code "$DC_XML" "$OUT/datacontrol-protocol.c" || exit 2
wayland-scanner client-header "$DC_XML" "$OUT/wlr-data-control-unstable-v1-client-protocol.h" || exit 2
cc -O1 -Wall -I"$OUT" -o "$OUT/wlxinject" "$SRC" "$OUT/vptr-protocol.c" -lwayland-client || exit 2
cc -O1 -Wall -I"$OUT" -o "$OUT/wlclip" "$CLIP" "$OUT/datacontrol-protocol.c" -lwayland-client || exit 2

rm -rf -- "$RUN"
mkdir -p -- "$RUN/dump"
RUN=$(cd -- "$RUN" && pwd)
chmod 700 "$RUN"
printf '%s\n' \
  'output HEADLESS-1 resolution 1024x640 bg #101010 solid_color' \
  'default_border pixel 0' >"$RUN/config"

export XDG_RUNTIME_DIR="$RUN"
export HOME="$RUN/home"
unset DISPLAY WAYLAND_DISPLAY DBUS_SESSION_BUS_ADDRESS DBUS_SYSTEM_BUS_ADDRESS XDG_CONFIG_HOME SWAYSOCK
export WLR_BACKENDS=headless WLR_LIBINPUT_NO_DEVICES=1 WLR_RENDERER=pixman WLR_HEADLESS_OUTPUTS=1
export WLXINJECT_WIDTH=1024 WLXINJECT_HEIGHT=640
mkdir -p "$HOME"

sway --unsupported-gpu -c "$RUN/config" >"$RUN/sway.log" 2>&1 &
sway_pid=$!
app_pid=
peer_pid=
cleanup(){
  [ -n "$peer_pid" ] && kill "$peer_pid" 2>/dev/null
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

titles(){
  swaymsg -t get_tree 2>/dev/null | python3 -c "
import json,sys
try: t=json.load(sys.stdin)
except Exception: sys.exit(0)
def walk(n):
    if n.get('name'): print(n['name'])
    for c in n.get('nodes',[])+n.get('floating_nodes',[]): walk(c)
walk(t)"
  }

wait_title(){
  local pattern=$1 limit=${2:-80} i
  for i in $(seq 1 "$limit"); do
    titles | grep -q "$pattern" && return 0
    sleep 0.1
    done
  return 1
  }

alive(){
  [ -n "$app_pid" ] && kill -0 "$app_pid" 2>/dev/null
  }

fail=0
note(){
  echo "FAIL: $*" >&2
  fail=1
  }

marks(){
  wc -l <"$RUN/app.log" 2>/dev/null || echo 0
  }

click(){
  "$OUT/wlxinject" "${1%,*}" "${1#*,}" move sleep:120 down sleep:120 up sleep:250 >/dev/null 2>&1
  sleep 0.5
  }

rightclick(){
  "$OUT/wlxinject" "${1%,*}" "${1#*,}" move sleep:120 rightdown sleep:120 rightup sleep:250 >/dev/null 2>&1
  sleep 0.6
  }

# Start foxterm with a private home; the log carries the popup positions
start_app(){
  local home=$1
  shift
  mkdir -p "$home/.config"
  rm -f "$RUN/app.log"
  HOME="$home" FOX_WAYLAND_TRACE=1 FOX_WAYLAND_DUMP="$RUN/dump" "$BIN" "$@" >>"$RUN/app.log" 2>&1 &
  app_pid=$!
  if ! wait_title "FoxTerm" 80; then
    note "no FoxTerm window appeared"
    return 1
    fi
  sleep 0.6
  swaymsg '[title="FoxTerm - Terminal"] focus' >/dev/null 2>&1
  swaymsg "[title=\"FoxTerm\"] focus" >/dev/null 2>&1
  sleep 0.4
  }

stop_app(){
  [ -n "$app_pid" ] || return 0
  kill "$app_pid" 2>/dev/null
  wait "$app_pid" 2>/dev/null
  app_pid=""
  }

# Select an encoding from the context menu of the grid.  The view menu is
# posted at the pointer, the cascade posts its submenu at its own right edge,
# and both positions are in the trace log, so no coordinate has to be guessed.
switch_encoding(){
  local index=$1 name=$2 before pos x y i
  before=$(marks)
  rightclick "$GRID"
  if ! tail -n +"$before" "$RUN/app.log" | grep -q "create popup"; then
    note "no context menu appeared for the encoding switch"
    return 1
    fi
  click "$CASCADE"
  for i in $(seq 1 40); do
    [ "$(tail -n +"$before" "$RUN/app.log" | grep -c "create popup")" -ge 2 ] && break
    sleep 0.1
    done
  pos=$(tail -n +"$before" "$RUN/app.log" | grep "create popup" | tail -1 | sed -E 's/.*at \+([0-9]+)\+([0-9]+) [0-9]+x[0-9]+.*/\1 \2/')
  case "$pos" in
    *" "*) ;;
    *) note "the encoding submenu did not open"; return 1 ;;
    esac
  x=${pos% *}
  y=${pos#* }
  click "$((x+20)),$((y+10+23*index))"
  for i in $(seq 1 40); do
    [ "$(tail -n +"$before" "$RUN/app.log" | grep -c "destroy surface of window .* (popup)")" -ge 2 ] && break
    sleep 0.1
    done
  sleep 0.4
  echo "    selected $name"
  }

# The status line is the only light plate at the bottom of the window
status_ink(){
  python3 - "$1" <<'PY'
import sys
from PIL import Image
image=Image.open(sys.argv[1]).convert("RGB")
width,height=image.size
px=image.load()
ink=0
for y in range(height-24,height):
    for x in range(0,width,4):
        r,g,b=px[x,y]
        if r>60 or g>60 or b>60:
            ink+=1
print(ink)
PY
  }

expect_status(){
  local file=$1 want=$2 what=$3 ink
  ink=$(status_ink "$file" 2>/dev/null)
  if [ "$want" = yes ] && [ "${ink:-0}" -lt 20 ]; then
    note "the status line did not report anything after $what"
  elif [ "$want" = no ] && [ "${ink:-0}" -ge 20 ]; then
    note "the status line is still up after $what"
  else
    echo "    the status line is $([ "$want" = yes ] && echo up || echo quiet) after $what"
    fi
  }

# Wait until the child wrote the expected bytes
wait_bytes(){
  local file=$1 want=$2 i got
  for i in $(seq 1 60); do
    got=$(tr -d ' \n' <"$file" 2>/dev/null)
    [ "$got" = "$want" ] && return 0
    sleep 0.1
    done
  return 1
  }

type_line(){
  wtype -s 120 "$1"
  sleep 0.2
  wtype -s 120 -k Return
  sleep 0.6
  }

# --- Phase 1: the same byte under two encodings ----------------------------
# The child prints one line with the byte 0xE4 each time it reads a line, so a
# switch can be observed without restarting it.
echo "==> phase 1: output decoding through the menu"
start_app "$RUN/home-out" --execute /bin/sh -c 'while read x; do printf "AAA\344BBB\n"; done' || exit 1
type_line 'go'
grim "$RUN/frame-utf8.png" 2>>"$RUN/grim.log" || note "grim could not capture the first frame"
expect_status "$RUN/frame-utf8.png" no "the first line"
switch_encoding $LATIN1 ISO-8859-1
grim "$RUN/frame-switch.png" 2>>"$RUN/grim.log" || note "grim could not capture the switched frame"
expect_status "$RUN/frame-switch.png" yes "the encoding switch"
type_line 'go'
sleep 0.3
grim "$RUN/frame-latin1.png" 2>>"$RUN/grim.log" || note "grim could not capture the latin-1 frame"
switch_encoding $UTF8 UTF-8
grim "$RUN/frame-back.png" 2>>"$RUN/grim.log" || note "grim could not capture the switch back"
type_line 'go'
sleep 0.3
grim "$RUN/frame-utf8-again.png" 2>>"$RUN/grim.log" || note "grim could not capture the last frame"
alive || note "the application ended during the encoding switch"

# --- Phase 2: keys are encoded too ---------------------------------------
# The child echoes the byte values of the line it read, so this checks the
# inverse mapping, not the glyphs on the screen.
echo "==> phase 2: keys go out in the session encoding"
stop_app
BYTES="$RUN/typed.txt"
rm -f "$BYTES"
start_app "$RUN/home-keys" --execute /bin/sh -c "while read x; do printf '%s' \"\$x\" | od -An -tx1 > '$BYTES'; done" || exit 1
rm -f "$BYTES"
wtype -s 200 'z'
sleep 0.2
wtype -s 200 'ä'
sleep 0.2
wtype -s 120 -k Return
if wait_bytes "$BYTES" 7ac3a4; then
  echo "    in UTF-8 the umlaut is three bytes (7a c3 a4)"
else
  note "in UTF-8 the child read [$(tr -d ' \n' <"$BYTES" 2>/dev/null)] instead of 7a c3 a4"
  fi
switch_encoding $LATIN1 ISO-8859-1
rm -f "$BYTES"
wtype -s 200 'z'
sleep 0.2
wtype -s 200 'ä'
sleep 0.2
wtype -s 120 -k Return
if wait_bytes "$BYTES" 7ae4; then
  echo "    in Latin-1 the same key is one byte (7a e4)"
else
  note "in Latin-1 the child read [$(tr -d ' \n' <"$BYTES" 2>/dev/null)] instead of 7a e4"
  fi

# --- Phase 3: paste, and the report of a character the set lacks ----------
echo "==> phase 3: paste and the unmappable-character report"
rm -f "$BYTES"
"$OUT/wlclip" --write '中' 12 >>"$RUN/peer.log" 2>&1 &
peer_pid=$!
sleep 1.0
wtype -s 120 -M ctrl -M shift -k v -m shift -m ctrl
sleep 0.4
wtype -s 120 -k Return
if wait_bytes "$BYTES" 3f; then
  echo "    the Latin-1 session sent '?' (3f) for a CJK character"
else
  note "the Latin-1 session sent [$(tr -d ' \n' <"$BYTES" 2>/dev/null)] instead of 3f"
  fi
kill "$peer_pid" 2>/dev/null
wait "$peer_pid" 2>/dev/null
peer_pid=""
grim "$RUN/frame-loss.png" 2>>"$RUN/grim.log" || note "grim could not capture the loss report"
expect_status "$RUN/frame-loss.png" yes "a character the encoding cannot express"

# The same paste in UTF-8 arrives as its UTF-8 bytes and says nothing
switch_encoding $UTF8 UTF-8
sleep 4.5
grim "$RUN/frame-quiet.png" 2>>"$RUN/grim.log" || note "grim could not capture the quiet frame"
expect_status "$RUN/frame-quiet.png" no "the status timeout"
rm -f "$BYTES"
"$OUT/wlclip" --write '中' 12 >>"$RUN/peer.log" 2>&1 &
peer_pid=$!
sleep 1.0
wtype -s 120 -M ctrl -M shift -k v -m shift -m ctrl
sleep 0.4
wtype -s 120 -k Return
if wait_bytes "$BYTES" e4b8ad; then
  echo "    the UTF-8 session sent the three UTF-8 bytes (e4 b8 ad)"
else
  note "the UTF-8 session sent [$(tr -d ' \n' <"$BYTES" 2>/dev/null)] instead of e4 b8 ad"
  fi
kill "$peer_pid" 2>/dev/null
wait "$peer_pid" 2>/dev/null
peer_pid=""
grim "$RUN/frame-utf8-paste.png" 2>>"$RUN/grim.log" || note "grim could not capture the paste frame"
expect_status "$RUN/frame-utf8-paste.png" no "a paste that lost nothing"

# --- Phase 4: the profile selects the encoding of a new session -----------
echo "==> phase 4: a profile with encoding=ISO-8859-1"
stop_app
mkdir -p "$RUN/home-profile/.config"
cat >"$RUN/home-profile/.config/FoxTerm.rc" <<EOF
[SETTINGS]
activeProfile=default

[PROFILE.default]
name=Default
encoding=ISO-8859-1
term=xterm-256color
kind=1
scrollback=10000
cursor=0
closeonexit=true
confirm=false
EOF
start_app "$RUN/home-profile" --execute /bin/sh -c 'while read x; do printf "AAA\344BBB\n"; done' || exit 1
type_line 'go'
sleep 0.3
grim "$RUN/frame-profile.png" 2>>"$RUN/grim.log" || note "grim could not capture the profile frame"
alive || note "the application ended in the profile phase"

# --- Phase 5: measure the committed frames --------------------------------
echo "==> phase 5: measure the frames"
if ! python3 - "$RUN/frame-utf8.png" "$RUN/frame-latin1.png" "$RUN/frame-utf8-again.png" "$RUN/frame-profile.png" "$ROW0" "$CELLW" "$CELLH" "$COL0" "$BYTECOL" <<'PY'
import sys
from PIL import Image

frames = sys.argv[1:5]
row0, cellw, cellh, col0, bytecol = map(int, sys.argv[5:10])
fail = 0

def note(text):
    global fail
    print("FAIL: " + text)
    fail = 1

def open_frame(path):
    image = Image.open(path).convert("RGB")
    if image.size != (1024, 640):
        note("%s is %dx%d, not the tiled window" % (path, image.size[0], image.size[1]))
    return image

def band(image, row, rows=1):
    top = row0 + cellh * row
    return image.crop((0, top, image.size[0], top + cellh * rows))

def cell(image, row, col):
    x = col0 + cellw * col
    y = row0 + cellh * row
    return image.crop((x, y, x + cellw, y + cellh))

def same(a, b):
    return a.tobytes() == b.tobytes()

def ink(patch):
    data = patch.tobytes()
    return sum(1 for i in range(0, len(data), 3) if data[i] > 60 or data[i+1] > 60 or data[i+2] > 60)

utf8 = open_frame(frames[0])       # row 1: the byte in UTF-8 (replacement)
latin1 = open_frame(frames[1])     # row 3: the same byte in ISO-8859-1
again = open_frame(frames[2])      # row 5: the byte in UTF-8 again
profile = open_frame(frames[3])    # row 1: the byte from a profile session

# The rows that were already drawn must not change when the encoding switches:
# no clear, no re-decoding, no reflow
if not same(band(utf8, 0, 2), band(latin1, 0, 2)):
    note("the switch to Latin-1 changed rows that were already on the screen")
else:
    print("    the screen keeps its drawn rows across the switch")

if not same(band(utf8, 0), band(again, 0)):
    note("the first row changed during the whole sequence")
if not same(band(utf8, 1), band(latin1, 1)):
    note("the first byte line changed after the switch")

# The byte must render differently once and only in its own cell
utf8_line = band(utf8, 1)
latin1_line = band(latin1, 3)
if same(utf8_line, latin1_line):
    note("the byte 0xE4 is drawn the same in UTF-8 and Latin-1; the switch did not decode")
else:
    left = None
    right = None
    for y in range(utf8_line.size[1]):
        for x in range(utf8_line.size[0]):
            if utf8_line.getpixel((x, y)) != latin1_line.getpixel((x, y)):
                left = x if left is None else min(left, x)
                right = x if right is None else max(right, x)
    x0 = col0 + cellw * bytecol
    if left is None:
        note("no pixel differs between the two byte lines")
    elif left < x0 - 2 or x0 + cellw < right:
        note("the difference spans x %d..%d, wider than the single cell at %d" % (left, right, x0))
    else:
        print("    the encoding changed one cell (x %d..%d), as one byte must" % (left, right))

# Switching back to UTF-8 renders the next line like the first one
if not same(cell(again, 5, bytecol), cell(utf8, 1, bytecol)):
    note("after switching back to UTF-8 the byte is not drawn like the first UTF-8 line")
else:
    print("    switching back to UTF-8 draws the byte like the first line")
if same(cell(again, 5, bytecol), cell(latin1, 3, bytecol)):
    note("after switching back to UTF-8 the byte is still drawn as Latin-1")

# The profile put its encoding on the first session
if not same(cell(profile, 1, bytecol), cell(latin1, 3, bytecol)):
    note("a profile with encoding=ISO-8859-1 did not start in Latin-1")
else:
    print("    a profile with encoding=ISO-8859-1 starts in Latin-1")
if same(cell(profile, 1, bytecol), cell(utf8, 1, bytecol)):
    note("the profile session drew the byte like UTF-8")

# The letter must be drawn, not a blank cell
if ink(cell(utf8, 1, bytecol)) == 0 or ink(cell(latin1, 3, bytecol)) == 0:
    note("the byte cell is blank in one of the frames")

sys.exit(fail)
PY
then
  fail=1
  fi

# The screenshots are evidence for the review in the plan
cp -f "$RUN/frame-latin1.png" "$RUN/foxterm-latin1.png" 2>/dev/null

if [ "$fail" -ne 0 ]; then
  echo "FAIL: see $RUN/app.log" >&2
  exit 1
  fi
echo "PASS: the encoding switch decodes output, encodes input and comes from the profile"
exit 0
