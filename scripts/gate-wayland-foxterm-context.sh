#!/usr/bin/env bash
#
# FoxTerm context menu gate: the right-click menus of requirement R19.
#
# The terminal view and the tab strip each post a menu; the menu itself belongs
# to the application, so the widget has to decide only whether the right button
# belongs to the child or to the application.  This gate checks that decision
# and both menus end to end:
#
#   - a right click in the grid posts a menu that becomes a grabbing popup, and
#     a click outside it dismisses the popup without ending the application;
#   - an entry of that menu acts: Select All from the keyboard, Copy from the
#     menu, and the text is read back with scripts/wlclip, a client that is not
#     FOX;
#   - with mouse reporting active the right button goes to the child as button
#     two and no menu appears; Shift is the escape hatch that still opens it;
#   - a right click on the tab strip posts the tab menu, and its Duplicate
#     entry really adds a tab (the detach shortcut then opens a second window,
#     which it cannot do with a single tab);
#   - an entry that cannot act is drawn disabled, never hidden: Copy without a
#     selection, Clear Scrollback without retained rows, every tab entry on the
#     empty part of the strip, Close Others with a single tab.  The entries come
#     back enabled when they can act, and Clear Scrollback really empties the
#     scrollback.  The state is read from a grim screenshot of the popup.
#
# Positions are layout coordinates of the tiled window on the headless output:
# the strip occupies the top rows, the grid starts under the hairline.
#
#   ./scripts/gate-wayland-foxterm-context.sh
#   ./scripts/gate-wayland-foxterm-context.sh --bin=FILE --build=DIR
#
# Exit status is 0 when all phases pass, 1 when one fails, 2 when the harness
# could not start.
#
set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

BUILD=tmp/build-fox2-wayland
BIN=
RUN=tmp/sway-foxterm-context
OUT=tmp/wlxinject
SRC=tools/wlxinject.c
XML=scripts/protocols/wlr-virtual-pointer-unstable-v1.xml
CLIP=scripts/wlclip.c
DC_XML=scripts/protocols/wlr-data-control-unstable-v1.xml

GRID=300,300             # a cell in the middle of the grid
FIRST_ENTRY=340,310      # first entry of the menu posted at GRID
FIRST_ENTRY_CLEAR=340,407 # Clear Scrollback entry of that menu
OUTSIDE=800,600          # empty desktop, well outside any popup
TAB=24,12                # title of the first tab
TAB_ENTRY=64,22          # first entry of the menu posted at TAB
STRIP_END=600,12         # empty right end of the strip

# Entry text sits 20 px right of the point a menu was posted at.  Offsets of
# the entry rows below that point, as used by phase 6:
#   view menu: Copy 13, Paste 36, Select All 59, Find 82, Clear Scrollback 107,
#              Reset 130, Encoding 153, New Tab 178, Detach 201, Close 224
#   tab menu:  Duplicate 13, Rename 36, Detach 59, Close 84, Close Others 107

MARK=context-menu-mark
REPORT=""

usage(){
  sed -n '2,33p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
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
for tool in sway swaymsg wtype grim wayland-scanner cc; do
  command -v "$tool" >/dev/null 2>&1 || { echo "error: $tool not found" >&2; exit 2; }
  done

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

window_count(){
  titles | grep -c '^FoxTerm' || true
  }

wait_title(){
  local pattern=$1 limit=${2:-60} i
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

popups(){
  grep -c "create popup for window" "$RUN/app.log" 2>/dev/null || true
  }

# Number of popups created since a mark
popups_since(){
  tail -n +"$1" "$RUN/app.log" | grep -c "create popup"
  }

# A popup is dismissed when its surface goes away or the compositor says so
dismissed_since(){
  tail -n +"$1" "$RUN/app.log" | grep -Eq "destroy surface of window .* \(popup\)|popup done"
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

# Is the entry at (px,py)+offset drawn enabled?  Menu text is black and a
# disabled entry is drawn gray, so the darkest pixel of the text box tells.
shot(){
  grim "$1" 2>>"$RUN/grim.log" || note "grim could not capture the popup"
  }

enabled(){   # file px py xoff yoff
  python3 - "$@" <<'PY'
import sys
from PIL import Image
f,px,py,xo,yo=sys.argv[1],*map(int,sys.argv[2:6])
box=(px+xo,py+yo-7,px+xo+90,py+yo+7)
im=Image.open(f).convert("L").crop(box)
print("yes" if min(im.getdata())<60 else "no")
PY
  }

expect_entry(){   # file px py name offset want
  local got
  got=$(enabled "$1" "$2" "$3" 20 "$5" 2>/dev/null)
  [ "$got" = "$6" ] && return 0
  note "$4 entry at +$5 is $([ "$got" = yes ] && echo enabled || echo disabled), expected $([ "$6" = yes ] && echo enabled || echo disabled)"
  }

start_app(){
  rm -f "$RUN/app.log"
  FOX_WAYLAND_TRACE=1 FOX_WAYLAND_DUMP="$RUN/dump" "$BIN" "$@" >>"$RUN/app.log" 2>&1 &
  app_pid=$!
  if ! wait_title "FoxTerm" 80; then
    note "no FoxTerm window appeared"
    return 1
    fi
  sleep 0.6
  }

focus_app(){
  swaymsg '[title="FoxTerm"] focus' >/dev/null 2>&1
  swaymsg '[title="FoxTerm - Terminal"] focus' >/dev/null 2>&1
  sleep 0.3
  }

# --- Phase 1: a right click in the grid posts a grabbing popup --------------
echo "==> phase 1: right click in the grid"
start_app --execute /bin/sh -c "printf '$MARK\n'; sleep 300"
before=$(marks)
rightclick "$GRID"
[ "$(popups)" -eq 1 ] && echo "    the menu became a popup" || note "no popup after a right click in the grid"
grep -q "create popup for window [0-9]* under [0-9]* at .* with grab" "$RUN/app.log" \
  || note "the menu popup did not take the pointer grab"

# --- Phase 2: a click outside dismisses the popup --------------------------
echo "==> phase 2: click outside dismisses it"
click "$OUTSIDE"
dismissed_since "$before" && echo "    the popup went away" || note "the popup was not dismissed by the click outside it"
alive && echo "    the application is still up" || note "the application ended with the menu"

# --- Phase 3: an entry of the menu acts -----------------------------------
# Select All comes from the keyboard, Copy from the menu; the text is read
# back with a client that is not FOX, so the entry really copied the screen.
echo "==> phase 3: Select All and Copy from the menu"
before=$(marks)
focus_app
wtype -s 250 -M ctrl -M shift -k a -m shift -m ctrl
sleep 0.3
rightclick "$GRID"
click "$FIRST_ENTRY"
got=$("$OUT/wlclip" --read 2>>"$RUN/peer.log")
case "$got" in
  *"$MARK"*) echo "    the peer read the marked line from the clipboard" ;;
  *) note "the peer read \"$(printf '%s' "$got" | head -c 60)\" instead of the marked line" ;;
  esac
grep -q "clipboard taken" "$RUN/app.log" || note "FOX never took the clipboard"

# --- Phase 4: mouse reporting owns the right button -----------------------
# The child turns button reporting on, so a plain right click must reach it as
# button two; Shift is the escape hatch that still opens the menu.  The child
# puts its terminal in non-canonical mode first, or the report would wait in
# the line discipline for a newline that never comes.
echo "==> phase 4: mouse reporting takes the right button"
kill "$app_pid" 2>/dev/null; wait "$app_pid" 2>/dev/null; app_pid=""
REPORT="$RUN/report.txt"
rm -f "$REPORT"
start_app --execute /bin/sh -c "stty -icanon min 1 time 0; printf '\033[?1000h'; head -c 6 > '$REPORT'; sleep 300"
before=$(marks)
focus_app
rightclick "$GRID"
for i in $(seq 1 40); do [ -s "$REPORT" ] && break; sleep 0.1; done
if [ -s "$REPORT" ]; then
  hex=$(od -An -tu1 "$REPORT" | tr -s ' ' | sed 's/^ //')
  case "$hex" in
    27\ 91\ 77\ 34\ *) echo "    the child got the right button report ($hex)" ;;
    *) note "the child got \"$hex\", not an ESC [ M button report" ;;
    esac
else
  note "the child received nothing for a reported right click"
  fi
[ "$(popups)" -eq 0 ] && echo "    no menu appeared while reporting is on" \
  || note "the menu opened although the child owns the right button"

echo "    Shift+right click still opens the menu"
wtype -M shift -s 1200 >/dev/null 2>&1 &
shift_pid=$!
sleep 0.4
"$OUT/wlxinject" "${GRID%,*}" "${GRID#*,}" move sleep:100 rightdown sleep:120 rightup >/dev/null 2>&1
wait "$shift_pid" 2>/dev/null
sleep 0.6
[ "$(popups)" -eq 1 ] && echo "    the menu opened with Shift held" \
  || note "Shift+right click did not open the menu while reporting is on"
click "$OUTSIDE"

# --- Phase 5: the tab strip menu -----------------------------------------
# Duplicate from the menu has to really add a tab: detaching then produces a
# second window, which detaching a single tab cannot do.
echo "==> phase 5: the tab strip menu"
kill "$app_pid" 2>/dev/null; wait "$app_pid" 2>/dev/null; app_pid=""
start_app --execute /bin/sh -c "printf 'tabs\n'; sleep 300"
before=$(marks)
rightclick "$TAB"
[ "$(popups)" -eq 1 ] && echo "    a right click on the tab posted its menu" \
  || note "no tab menu after a right click on the tab"
click "$TAB_ENTRY"
sleep 0.8
focus_app
wtype -s 250 -M ctrl -M shift -k m -m shift -m ctrl
sleep 1.2
if [ "$(window_count)" = 2 ]; then
  echo "    Duplicate added a tab, detaching it opened a second window"
else
  note "expected two windows after Duplicate and detach, found $(window_count)"
  fi

echo "    a right click on the empty strip posts the same menu"
kill "$app_pid" 2>/dev/null; wait "$app_pid" 2>/dev/null; app_pid=""
start_app --execute /bin/sh -c "printf 'strip\n'; sleep 300"
before=$(marks)
rightclick "$STRIP_END"
[ "$(popups)" -eq 1 ] && echo "    the strip menu came up over the empty end" \
  || note "no menu on the empty part of the strip"
click "$OUTSIDE"
alive || note "the application ended with the strip menu"

# --- Phase 6: entries that cannot act are disabled, not hidden --------------
# Pixels are read from a screenshot of the popup, so a handler that only
# returns a value and never enables or disables its sender fails here.
echo "==> phase 6: unavailable entries are disabled"
kill "$app_pid" 2>/dev/null; wait "$app_pid" 2>/dev/null; app_pid=""
start_app --execute /bin/sh -c "printf 'plain\n'; sleep 300"
focus_app
gx=${GRID%,*}; gy=${GRID#*,}

rightclick "$GRID"
shot "$RUN/view-none.png"
click "$OUTSIDE"
expect_entry "$RUN/view-none.png" "$gx" "$gy" view 13 no      # Copy: nothing selected
expect_entry "$RUN/view-none.png" "$gx" "$gy" view 36 yes     # Paste: child runs
expect_entry "$RUN/view-none.png" "$gx" "$gy" view 59 yes     # Select All
expect_entry "$RUN/view-none.png" "$gx" "$gy" view 82 yes     # Find
expect_entry "$RUN/view-none.png" "$gx" "$gy" view 107 no     # Clear Scrollback: none retained
expect_entry "$RUN/view-none.png" "$gx" "$gy" view 130 yes    # Reset Terminal
expect_entry "$RUN/view-none.png" "$gx" "$gy" view 178 yes    # New Tab
expect_entry "$RUN/view-none.png" "$gx" "$gy" view 201 no     # Detach Tab: a single tab stays
expect_entry "$RUN/view-none.png" "$gx" "$gy" view 224 yes    # Close Tab
echo "    view menu without selection or scrollback: Copy, Clear Scrollback and Detach gray"

focus_app
wtype -s 250 -M ctrl -M shift -k a -m shift -m ctrl
sleep 0.3
rightclick "$GRID"
shot "$RUN/view-sel.png"
click "$OUTSIDE"
expect_entry "$RUN/view-sel.png" "$gx" "$gy" view 13 yes      # Copy: selection exists
echo "    Copy comes back enabled with a selection"

# Scrollback: a child that prints more rows than the grid holds
kill "$app_pid" 2>/dev/null; wait "$app_pid" 2>/dev/null; app_pid=""
start_app --execute /bin/sh -c "seq 1 300; sleep 300"
sleep 0.8
focus_app
rightclick "$GRID"
shot "$RUN/view-scroll.png"
expect_entry "$RUN/view-scroll.png" "$gx" "$gy" view 107 yes  # Clear Scrollback: rows retained
click "$FIRST_ENTRY_CLEAR"
sleep 0.5
rightclick "$GRID"
shot "$RUN/view-cleared.png"
click "$OUTSIDE"
expect_entry "$RUN/view-cleared.png" "$gx" "$gy" view 107 no  # and none left afterwards
echo "    Clear Scrollback is enabled with rows and empties them"

# The tab strip menu on the empty part: every entry gray
kill "$app_pid" 2>/dev/null; wait "$app_pid" 2>/dev/null; app_pid=""
start_app --execute /bin/sh -c "printf 'strip\n'; sleep 300"
sx=${STRIP_END%,*}; sy=${STRIP_END#*,}
rightclick "$STRIP_END"
shot "$RUN/strip-empty.png"
click "$OUTSIDE"
for off in 13 36 59 84 107; do
  expect_entry "$RUN/strip-empty.png" "$sx" "$sy" tab "$off" no
  done
echo "    strip menu on the empty strip: all five entries gray"

# On the only tab: Detach and Close Others need a second tab
tx=${TAB%,*}; ty=${TAB#*,}
rightclick "$TAB"
shot "$RUN/strip-tab.png"
click "$OUTSIDE"
for off in 13 36 84; do
  expect_entry "$RUN/strip-tab.png" "$tx" "$ty" tab "$off" yes
  done
expect_entry "$RUN/strip-tab.png" "$tx" "$ty" tab 59 no   # Detach: a single tab stays
expect_entry "$RUN/strip-tab.png" "$tx" "$ty" tab 107 no  # Close Others
echo "    strip menu on the only tab: Detach and Close Others gray"

# With a second tab both Detach entries act, and the detached window posts its
# own menus: the view is retargeted to the window it moves into, and a window
# built after the application is up has to create its menu panes itself.
echo "==> phase 7: Detach needs and gets a second tab"
wtype -s 250 -M ctrl -M shift -k t -m shift -m ctrl
sleep 1.0
focus_app
rightclick "$GRID"
shot "$RUN/view-two.png"
click "$OUTSIDE"
expect_entry "$RUN/view-two.png" "$gx" "$gy" view 201 yes  # Detach Tab with a second tab
rightclick "$TAB"
shot "$RUN/strip-two.png"
click "$OUTSIDE"
expect_entry "$RUN/strip-two.png" "$tx" "$ty" tab 59 yes  # Detach with a second tab
echo "    both Detach entries are enabled with two tabs"

wtype -s 250 -M ctrl -M shift -k m -m shift -m ctrl
sleep 1.5
if [ "$(window_count)" = 2 ]; then
  echo "    Ctrl+Shift+M detached the tab into a second window"
else
  note "expected two windows after the detach, found $(window_count)"
  fi
alive || note "the application ended with the detach"

# The detached window is the focused one; its menu must post and must act on
# that window.  Right after the detach the pointer may still report its old
# place, so the click is repeated once if nothing happened.
dx=800; dy=120
before=$(marks)
rightclick "$dx,$dy"
if [ "$(popups_since "$before")" -eq 0 ]; then
  rightclick "$dx,$dy"
  fi
if [ "$(popups_since "$before")" -ge 1 ]; then
  echo "    the detached window posts its own context menu"
else
  note "the detached window did not post a context menu"
  fi
shot "$RUN/detached-menu.png"
alive || note "the application crashed on the menu of the detached window"
click "$OUTSIDE"
alive || note "the application crashed after the menu of the detached window"

if [ "$fail" -ne 0 ]; then
  echo "FAIL: see $RUN/app.log" >&2
  exit 1
  fi
echo "PASS: context menus post, act, and yield the button to the child"
exit 0
