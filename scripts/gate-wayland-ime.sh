#!/usr/bin/env bash
#
# Text input gate:  the client must bind zwp_text_input_manager_v3, enable the
# text input of the seat while a window of its own has the keyboard focus, and
# tell the compositor where the caret is.
#
# The protocol is checked on the wire, with WAYLAND_DEBUG=1, because the
# interesting part is what the compositor is told and not what our own trace
# says: enable, set_cursor_rectangle and commit have to be sent after the focus
# arrives, and the commit is what makes them take effect.
#
# No input method runs in this gate: a committed string can only come from an
# input method client, and sway has none.  What is checked here is the half that
# belongs to the toolkit.
#
set -uo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
BUILD="$ROOT/tmp/build-fox2-wayland"
RUN="$ROOT/tmp/wayland-ime-gate"
BIN=""

while [ $# -gt 0 ]; do
  case "$1" in
    --bin)       shift; BIN=$1 ;;
    --bin=*)     BIN=${1#*=} ;;
    --run-dir)   shift; RUN=$1 ;;
    --run-dir=*) RUN=${1#*=} ;;
    -h|--help)   sed -n '2,15p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) echo "error: unknown option '$1'" >&2; exit 2 ;;
    esac
  shift
  done

[ -n "$BIN" ] || BIN="$BUILD/bin/dnd"
for tool in sway wtype; do
  command -v "$tool" >/dev/null 2>&1 || { echo "error: $tool not found" >&2; exit 2; }
  done
[ -x "$BIN" ] || { echo "error: no binary at $BIN" >&2; exit 2; }

rm -rf -- "$RUN"
mkdir -p -- "$RUN/home"
RUN=$(cd -- "$RUN" && pwd)
chmod 700 "$RUN"
echo 'output HEADLESS-1 resolution 1024x640 bg #101010 solid_color' >"$RUN/config"

export XDG_RUNTIME_DIR="$RUN"
export HOME="$RUN/home"
unset DISPLAY WAYLAND_DISPLAY
export WLR_BACKENDS=headless
export WLR_LIBINPUT_NO_DEVICES=1
export WLR_RENDERER=pixman
export WLR_HEADLESS_OUTPUTS=1

echo "==> starting headless sway"
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
  kill -0 "$sway_pid" 2>/dev/null || { echo "error: sway exited; see $RUN/sway.log" >&2; exit 2; }
  sleep 0.05
  done
[ -n "$sock" ] || { echo "error: sway did not create a socket" >&2; exit 2; }
export WAYLAND_DISPLAY=$(basename "$sock")

echo "==> running $BIN"
FOX_WAYLAND_TRACE=1 WAYLAND_DEBUG=1 "$BIN" --no-dialogs >"$RUN/app.log" 2>&1 &
app_pid=$!
sleep 1.5
kill -0 "$app_pid" 2>/dev/null || { echo "FAIL: the application did not start"; tail -20 "$RUN/app.log"; exit 1; }

# The keyboard focus is what makes the client enable the text input; wtype is the
# only thing in this gate that can give it the focus
echo "==> typing a key"
wtype -s 100 -k Shift_L >>"$RUN/input.log" 2>&1
sleep 0.8

failed=""
grep -q "zwp_text_input_manager_v3" "$RUN/app.log" || failed="the manager was not bound"
grep -q "fxwayland: text input created" "$RUN/app.log" || failed="${failed:-the text input of the seat was not created}"
grep -qE "zwp_text_input_v3#[0-9]+\.enable" "$RUN/app.log" || failed="${failed:-the text input was never enabled}"
grep -qE "zwp_text_input_v3#[0-9]+\.set_cursor_rectangle" "$RUN/app.log" || failed="${failed:-the caret rectangle was never reported}"
grep -qE "zwp_text_input_v3#[0-9]+\.commit" "$RUN/app.log" || failed="${failed:-the text input state was never committed}"

if [ -n "$failed" ]; then
  echo "FAIL: $failed"
  grep -E "text input|zwp_text_input" "$RUN/app.log" | tail -12
  exit 1
  fi

grep -E "text input created|text input enabled|zwp_text_input_v3#[0-9]+\.enable" "$RUN/app.log" | tail -4
echo "PASS: the text input is enabled on focus and the caret is reported"
exit 0
