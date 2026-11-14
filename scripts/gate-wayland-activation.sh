#!/usr/bin/env bash
#
# Activation gate:  a Wayland client cannot raise or focus its own windows, it
# can only ask for an activation token that carries the evidence that the user
# did something, and hand it back to the compositor with the surface.
#
# The gate checks the whole round trip on the wire: the token is requested, the
# serial of the input event goes with it, the compositor answers with done(), and
# the client activates the surface with the name it got.
#
set -uo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
BUILD="$ROOT/tmp/build-fox2-wayland"
RUN="$ROOT/tmp/wayland-activation-gate"
BIN=""

while [ $# -gt 0 ]; do
  case "$1" in
    --bin)       shift; BIN=$1 ;;
    --bin=*)     BIN=${1#*=} ;;
    --run-dir)   shift; RUN=$1 ;;
    --run-dir=*) RUN=${1#*=} ;;
    -h|--help)   sed -n '2,10p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) echo "error: unknown option '$1'" >&2; exit 2 ;;
    esac
  shift
  done

[ -n "$BIN" ] || BIN="$BUILD/bin/hello"
command -v sway >/dev/null 2>&1 || { echo "error: sway not found" >&2; exit 2; }
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
  sock=$(ls "$RUN"/wayland-* 2>/dev/null | head -1) || true
  [ -n "$sock" ] && [ -S "$sock" ] && break
  kill -0 "$sway_pid" 2>/dev/null || { echo "error: sway exited; see $RUN/sway.log" >&2; exit 2; }
  sleep 0.05
  done
[ -n "$sock" ] || { echo "error: sway did not create a socket" >&2; exit 2; }
export WAYLAND_DISPLAY=$(basename "$sock")

echo "==> running $BIN"
FOX_WAYLAND_TRACE=1 WAYLAND_DEBUG=1 "$BIN" --no-dialogs >"$RUN/app.log" 2>&1 &
app_pid=$!
sleep 2.0
kill -0 "$app_pid" 2>/dev/null || { echo "FAIL: the application did not start"; tail -20 "$RUN/app.log"; exit 1; }

failed=""
grep -q "xdg_activation_v1#" "$RUN/app.log" || failed="the manager was not bound"
grep -qE "xdg_activation_v1#[0-9]+\.get_activation_token" "$RUN/app.log" || failed="${failed:-no activation token was asked for}"
grep -qE "xdg_activation_token_v1#[0-9]+\.commit" "$RUN/app.log" || failed="${failed:-the token request was never committed}"
grep -qE "xdg_activation_token_v1#[0-9]+\.done\(" "$RUN/app.log" || failed="${failed:-the compositor never answered with a token}"
grep -qE "xdg_activation_v1#[0-9]+\.activate\(" "$RUN/app.log" || failed="${failed:-the surface was never activated}"

if [ -n "$failed" ]; then
  echo "FAIL: $failed"
  grep -E "activation|activate" "$RUN/app.log" | tail -12
  exit 1
  fi

grep -E "asked for an activation token|activated surface" "$RUN/app.log" | tail -2
echo "PASS: the window asked for an activation token and activated itself with it"
exit 0
