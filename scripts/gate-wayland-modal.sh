#!/usr/bin/env bash
#
# Modal gate: a modal dialog blocks the window that opened it.
#
# The preferences dialog runs with FXDialogBox::execute(), which is a modal
# invocation.  The X11 and Win32 backends dispatch pointer and key events only
# to windows that the modal invocation allows; the Wayland backend dispatched
# everything, so a click on the calculator behind the dialog reached a button
# and changed its display, and a key typed there was handled by the calculator
# instead of being beeped away.
#
# The gate first proves that the click and the key work with no dialog open,
# then opens the dialog and repeats them: the display must stay the same and
# the dialog must stay up.  A click inside the dialog is made as well, to show
# that the filter does not block the dialog itself.
#
#   ./scripts/gate-wayland-modal.sh
#   ./scripts/gate-wayland-modal.sh --bin=FILE --build=DIR
#
# Exit status is 0 when the main window is blocked, 1 when it is not, 2 when
# the harness could not start.
#
set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

BUILD=tmp/build-fox2-wayland
BIN=
RUN=tmp/sway-modal-gate
OUT=tmp/wlxinject
SRC=tools/wlxinject.c
XML=scripts/protocols/wlr-virtual-pointer-unstable-v1.xml

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

[ -n "$BIN" ] || BIN="$BUILD/bin/calculator"
if [ ! -x "$BIN" ]; then
  echo "error: no binary at $BIN (build with scripts/build-fox2-wayland.sh)" >&2
  exit 2
  fi
for tool in sway swaymsg wayland-scanner cc wtype tesseract; do
  command -v "$tool" >/dev/null 2>&1 || { echo "error: $tool not found" >&2; exit 2; }
  done
if ! python3 -c 'import PIL' >/dev/null 2>&1; then
  echo "error: python3 with Pillow is needed to read the frames" >&2
  exit 2
  fi

mkdir -p -- "$OUT"
wayland-scanner private-code "$XML" "$OUT/vptr-protocol.c" || exit 2
wayland-scanner client-header "$XML" "$OUT/wlr-virtual-pointer-unstable-v1-client-protocol.h" || exit 2
cc -O1 -Wall -I"$OUT" -o "$OUT/wlxinject" "$SRC" "$OUT/vptr-protocol.c" -lwayland-client || exit 2

rm -rf -- "$RUN"
mkdir -p -- "$RUN/home" "$RUN/dump"
RUN=$(cd -- "$RUN" && pwd)
chmod 700 "$RUN"
printf '%s\n' \
  'output HEADLESS-1 resolution 1280x720 bg #101010 solid_color' \
  'default_border pixel 0' \
  'for_window [title="Calculator Preferences"] floating enable' >"$RUN/config"

export XDG_RUNTIME_DIR="$RUN"
export HOME="$RUN/home"
unset DISPLAY WAYLAND_DISPLAY DBUS_SESSION_BUS_ADDRESS DBUS_SYSTEM_BUS_ADDRESS XDG_CONFIG_HOME SWAYSOCK
export WLR_BACKENDS=headless WLR_LIBINPUT_NO_DEVICES=1 WLR_RENDERER=pixman WLR_HEADLESS_OUTPUTS=1
export WLXINJECT_WIDTH=1280 WLXINJECT_HEIGHT=720

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
export SWAYSOCK=$(ls "$RUN"/sway-ipc.*.sock 2>/dev/null | head -1)

# Title, geometry and focus of a toplevel in the sway tree
tree_info(){
  swaymsg -t get_tree | python3 -c "
import json,sys
t=json.load(sys.stdin)
def walk(n):
    if n.get('name')==sys.argv[1]:
        r=n['rect']; print('%d %d %d %d %d'%(r['x'],r['y'],r['width'],r['height'],1 if n.get('focused') else 0))
    for c in n.get('nodes',[])+n.get('floating_nodes',[]): walk(c)
walk(t)" "$1"
  }

# Integer value of the display field, from the last committed frame of the
# main window; the argument is a PPM file
display_value(){
  magick "$1" -crop 230x18+172+8 +repage -resize 500% "$RUN/ocr.png" || return 1
  tesseract "$RUN/ocr.png" - --psm 7 2>/dev/null |
    tr -d '| \n' | sed 's/S/5/g; s/O/0/g; s/l/1/g; s/I/1/g'
  }

echo "==> running $BIN"
FOX_WAYLAND_DUMP="$RUN/dump" FOX_WAYLAND_TRACE=1 "$BIN" >"$RUN/app.log" 2>&1 &
app_pid=$!
sleep 2
kill -0 "$app_pid" 2>/dev/null || { echo "error: the application did not stay up" >&2; exit 2; }

swaymsg '[title="FOX Calculator"] floating enable' >/dev/null 2>&1
swaymsg '[title="FOX Calculator"] move position 0 0' >/dev/null 2>&1
sleep 0.6
main=$(tree_info "FOX Calculator")
[ -n "$main" ] || { echo "error: no calculator window" >&2; exit 2; }
set -- $main; wx=$1; wy=$2; ww=$3; wh=$4
[ "$ww" -eq 400 ] && [ "$wh" -eq 260 ] || { echo "error: window is ${ww}x${wh}" >&2; exit 2; }

frame="$RUN/dump/window-2-400x260.ppm"
for i in $(seq 1 100); do
  [ -f "$frame" ] && break
  sleep 0.05
  done
[ -f "$frame" ] || { echo "error: no frame was committed" >&2; exit 2; }

# The click and the key paths work at all: type 7, press AC, type 7 again
echo "==> checking the click and the key with no dialog"
wtype -s 120 -k 7
sleep 0.5
cp "$frame" "$RUN/zero.ppm"
got=$(display_value "$RUN/zero.ppm")
[ "$got" = "7" ] || { echo "FAIL: the display shows '$got' after typing 7" >&2; exit 1; }

"$OUT/wlxinject" "$((wx+100))" "$((wy+100))" move sleep:300 \
                 "move:$((wx+378)),$((wy+102))" sleep:300 down sleep:300 up sleep:600 >/dev/null 2>&1
cp "$frame" "$RUN/cleared.ppm"
got=$(display_value "$RUN/cleared.ppm")
[ "$got" = "0" ] || { echo "FAIL: AC did not clear the display, it shows '$got'" >&2; exit 1; }

wtype -s 120 -k 7
sleep 0.5
cp "$frame" "$RUN/seeded.ppm"
got=$(display_value "$RUN/seeded.ppm")
[ "$got" = "7" ] || { echo "FAIL: the display shows '$got' after typing 7 again" >&2; exit 1; }

echo "==> opening the preferences"
"$OUT/wlxinject" "$((wx+300))" "$((wy+20))" move sleep:300 \
                 "move:$((wx+60)),$((wy+25))" sleep:300 down sleep:300 up sleep:600 >/dev/null 2>&1
prefs=""
for i in $(seq 1 100); do
  prefs=$(ls "$RUN"/dump/window-*-661x295.ppm 2>/dev/null | head -1)
  [ -n "$prefs" ] && break
  sleep 0.1
  done
[ -n "$prefs" ] || { echo "error: the preferences dialog committed no frame" >&2; exit 2; }

# Put the dialog next to the window it blocks, so both can be clicked
swaymsg '[title="Calculator Preferences"] move position 600 20' >/dev/null 2>&1
sleep 0.6
prect=$(tree_info "Calculator Preferences")
[ -n "$prect" ] || { echo "error: the preferences dialog is gone" >&2; exit 2; }
set -- $prect; px=$1; py=$2

# The dialog itself must still take a click: toggle "Beep on error"
echo "==> checking that the dialog still works"
cp "$prefs" "$RUN/dialog-before.ppm"
"$OUT/wlxinject" "$((px+500))" "$((py+250))" move sleep:300 \
                 "move:$((px+253)),$((py+192))" sleep:300 down sleep:300 up sleep:600 >/dev/null 2>&1
cp "$prefs" "$RUN/dialog-after.ppm"
if cmp -s "$RUN/dialog-before.ppm" "$RUN/dialog-after.ppm"; then
  echo "FAIL: the click on the dialog did nothing" >&2
  exit 1
  fi
echo "the dialog repainted after a click inside it"

# Now the main window is behind a modal dialog: AC must do nothing
echo "==> clicking AC on the blocked window"
cp "$frame" "$RUN/main-before.ppm"
got=$(display_value "$RUN/main-before.ppm")
[ "$got" = "7" ] || { echo "FAIL: the display shows '$got' before the blocked click" >&2; exit 1; }
"$OUT/wlxinject" "$((wx+100))" "$((wy+100))" move sleep:300 \
                 "move:$((wx+378)),$((wy+102))" sleep:300 down sleep:300 up sleep:800 >/dev/null 2>&1
cp "$frame" "$RUN/main-after.ppm"
got=$(display_value "$RUN/main-after.ppm")
[ "$got" = "7" ] || { echo "FAIL: the blocked click reached the calculator, the display shows '$got'" >&2; exit 1; }
kill -0 "$app_pid" 2>/dev/null || { echo "FAIL: the application ended" >&2; exit 1; }
main=$(tree_info "FOX Calculator")
[ -n "$main" ] || { echo "FAIL: the calculator window is gone" >&2; exit 1; }
echo "the click on the blocked window was ignored"

# The compositor moved the keyboard focus to the blocked window on that click,
# so one more key proves that the key filter is in place too
set -- $main; focused=$5
[ "$focused" -eq 1 ] || { echo "FAIL: the click did not give the calculator the keyboard focus" >&2; exit 1; }
echo "==> typing 8 into the blocked window"
wtype -s 120 -k 8
sleep 0.6
cp "$frame" "$RUN/main-key.ppm"
got=$(display_value "$RUN/main-key.ppm")
[ "$got" = "7" ] || { echo "FAIL: the blocked key reached the calculator, the display shows '$got'" >&2; exit 1; }
echo "the key on the blocked window was ignored"

echo "PASS: a modal dialog blocks the clicks and keys of the window behind it"
exit 0
