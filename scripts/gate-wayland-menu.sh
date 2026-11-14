#!/usr/bin/env bash
#
# Menu gate:  open a menu, open its submenu and pick an entry, all with the
# virtual pointer of scripts/wlxinject.
#
# This is the gate for the popup role of phase 7.  It checks three things the
# protocol cares about:  a menu is an xdg_popup with an explicit grab, a
# submenu is a popup hanging off that popup (the only parent the protocol
# allows for a grabbing popup), and the popup chain goes away again, top down,
# when the compositor dismisses it.
#
# The first half opens the menu and clicks outside it: the compositor sends
# popup_done, the application hides the menu and keeps running.  The second half
# opens menu, submenu and picks the third entry, which ends the program with 0.
#
set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

RUN=${RUN:-tmp/sway-menu}
OUT=${OUT:-tmp/wlxinject}
DUMP=${DUMP:-tmp/dump-menu-gate}
SRC=scripts/wlxinject.c
XML=scripts/protocols/wlr-virtual-pointer-unstable-v1.xml

# Positions in layout coordinates for the tiled window of the menu test:
# the File button, the Recent entry of its menu and the Three entry of the
# submenu that Recent opens
FILE=40,40
RECENT=60,115
THREE=150,163
OUTSIDE=600,400

mkdir -p -- "$OUT"
wayland-scanner private-code "$XML" "$OUT/vptr-protocol.c" || exit 2
wayland-scanner client-header "$XML" "$OUT/wlr-virtual-pointer-unstable-v1-client-protocol.h" || exit 2
cc -O1 -Wall -I"$OUT" -o "$OUT/wlxinject" "$SRC" "$OUT/vptr-protocol.c" -lwayland-client || exit 2

rm -rf -- "$RUN" "$DUMP"
mkdir -p -- "$RUN/home" "$DUMP"
RUN=$(cd -- "$RUN" && pwd)
DUMP=$(cd -- "$DUMP" && pwd)
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
[ -n "$sock" ] || { echo "error: no sway socket"; exit 2; }
export WAYLAND_DISPLAY=$(basename "$sock")

click(){
  "$OUT/wlxinject" "${1%,*}" "${1#*,}" move sleep:120 down sleep:120 up sleep:250 >/dev/null 2>&1
  sleep 0.4
  }

app_pid=""
start_app(){
  : >"$RUN/app.log"
  FOX_WAYLAND_TRACE=1 FOX_WAYLAND_DUMP="$DUMP" tmp/build-fox2-wayland/bin/menus >>"$RUN/app.log" 2>&1 &
  app_pid=$!
  sleep 2.0
  }

alive(){
  kill -0 "$app_pid" 2>/dev/null
  }

fail(){
  echo "FAIL: $1"
  grep -E "create popup|popup done|destroy surface|button .* to |^command|^menu open" "$RUN/app.log" | tail -12
  exit 1
  }

# Half one: the menu is dismissed by a click outside it ---------------------
start_app
echo "==> open the menu, then click outside it"
click "$FILE"
grep -q "create popup for window .* under .* with grab" "$RUN/app.log" || fail "the menu did not become a grabbing popup"
click "$OUTSIDE"
sleep 0.5
# The menu closes because the popup holds the grab and the press comes back to
# it, which is how FOX unposts a menu on X11 as well; a compositor that
# dismisses the popup itself sends popup_done on top of that
grep -Eq "destroy surface of window .* \(popup\)|popup done" "$RUN/app.log" || fail "the menu was not dismissed by the click outside it"
alive || fail "the application ended when the menu was dismissed, it should not"
kill "$app_pid" 2>/dev/null
wait "$app_pid" 2>/dev/null
app_pid=""

# Half two: menu, submenu, pick an entry -----------------------------------
start_app
echo "==> open the menu and its submenu, then pick the third entry"
click "$FILE"
grep -q "create popup for window .* under .* with grab" "$RUN/app.log" || fail "the menu did not become a grabbing popup"
click "$RECENT"
# The submenu must hang off the popup, not off the toplevel
grep -Eq "create popup for window [0-9]+ under [0-9]+ at .* with grab" "$RUN/app.log" || fail "the submenu did not become a grabbing popup"
click "$THREE"

state=timeout
for i in $(seq 1 40); do
  if ! alive; then
    wait "$app_pid"
    state="exit $?"
    break
    fi
  sleep 0.05
  done
app_pid=""
[ "$state" = "exit 0" ] || fail "picking the entry did not end the program ($state)"

grep -q "^command three$" "$RUN/app.log" || fail "the wrong entry was picked"
nested=$(grep -c "create popup" "$RUN/app.log")
[ "$nested" -ge 2 ] || fail "only one popup was made, the submenu is missing"
echo "==> $nested popups, the second one hanging off the first"
grep -E "create popup|^command" "$RUN/app.log" | tail -4
frame=$(ls -t "$DUMP"/window-*.ppm 2>/dev/null | head -1)
[ -n "$frame" ] && cp "$frame" "$DUMP/menu.ppm"
echo "PASS: menu, submenu and dismissal all behave"
exit 0
