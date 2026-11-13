#!/usr/bin/env bash
#
# The input gate of docs/wayland-plan.md: a key typed into the application has
# to reach the widget under the focus and act on it.
#
# A private headless sway with a virtual keyboard is the compositor; wtype types
# a space into the hello button, which the button turns into a command that
# quits the application. The gate passes when the application exits by itself
# with status 0 instead of being stopped.
#
#   ./scripts/gate-wayland-input.sh
#   ./scripts/gate-wayland-input.sh --bin=tmp/build-fox2-wayland/bin/button
#
# Options:
#   --bin=FILE     application to run (default: tmp/build-fox2-wayland/bin/hello)
#   --run-dir=DIR  runtime directory (default: tmp/wayland-input-gate)
#   -h|--help      show this help
#
# Exit status is 0 when the key reached the widget, 1 when the application had to
# be stopped, and 2 when the harness could not start.
#
set -uo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
BUILD="$ROOT/tmp/build-fox2-wayland"
RUN="$ROOT/tmp/wayland-input-gate"
BIN=""

usage(){
  sed -n '2,22p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
  }

while [ $# -gt 0 ]; do
  case "$1" in
    --bin)       shift; BIN=$1 ;;
    --bin=*)     BIN=${1#*=} ;;
    --run-dir)   shift; RUN=$1 ;;
    --run-dir=*) RUN=${1#*=} ;;
    -h|--help)   usage; exit 0 ;;
    *) echo "error: unknown option '$1'" >&2; usage >&2; exit 2 ;;
    esac
  shift
  done

[ -n "$BIN" ] || BIN="$BUILD/bin/hello"
for tool in sway wtype; do
  command -v "$tool" >/dev/null 2>&1 || { echo "error: $tool not found" >&2; exit 2; }
  done
[ -x "$BIN" ] || { echo "error: no binary at $BIN" >&2; exit 2; }

rm -rf -- "$RUN"
mkdir -p -- "$RUN/home"
RUN=$(cd -- "$RUN" && pwd)
chmod 700 "$RUN"

# The window fills the screen, so there is room for the pointer to land on it;
# sway draws its own title bar, which is why the pointer goes below it
cat >"$RUN/config" <<'EOF'
output HEADLESS-1 resolution 1024x640 bg #101010 solid_color
EOF

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

export FOX_WAYLAND_TRACE=1
echo "==> running $BIN"
"$BIN" --no-dialogs >"$RUN/app.log" 2>&1 &
app_pid=$!
sleep 1.5

echo "==> typing a space"
# The press and the release need to be at least one compositor frame apart:
# sway drops the press when both arrive in the same one, which made this gate
# fail intermittently
wtype -s 200 -k space >>"$RUN/input.log" 2>&1
sleep 0.8

state=timeout
for i in $(seq 1 60); do
  if ! kill -0 "$app_pid" 2>/dev/null; then
    wait "$app_pid"
    state="exit $?"
    break
    fi
  sleep 0.05
  done
if [ "$state" = timeout ]; then
  kill "$app_pid" 2>/dev/null
  wait "$app_pid" 2>/dev/null
  fi

echo "==> application: $state"
grep -E "focus in|key press|key release" "$RUN/app.log" | tail -6
if [ "$state" = "exit 0" ]; then
  echo "PASS: the key reached the widget and the window closed"
  exit 0
  fi
echo "FAIL: the key did not reach the widget (trace in $RUN/app.log)" >&2
exit 1
