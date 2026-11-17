#!/usr/bin/env bash
#
# Idle inhibit gate:  a window that asks the system to stay awake gets an
# inhibitor from the compositor's zwp_idle_inhibit_manager_v1, tied to its
# surface, and destroys it when the request is given up.
#
# Headless sway offers the protocol, so the gate runs the idle test against it
# and checks the wire:  the manager is bound, create_inhibitor() names the
# surface, and the inhibitor is destroyed again before the program ends.
#
set -uo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
BUILD="$ROOT/tmp/build-fox2-wayland"
RUN="$ROOT/tmp/wayland-idle-gate"
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

[ -n "$BIN" ] || BIN="$BUILD/bin/idle"
command -v sway >/dev/null 2>&1 || { echo "error: sway not found" >&2; exit 2; }
[ -x "$BIN" ] || { echo "error: no binary at $BIN" >&2; exit 2; }

rm -rf -- "$RUN"
mkdir -p -- "$RUN/home"
RUN=$(cd -- "$RUN" && pwd)
chmod 700 "$RUN"
echo 'output HEADLESS-1 resolution 1024x640 bg #101010 solid_color' >"$RUN/config"

export XDG_RUNTIME_DIR="$RUN"
export HOME="$RUN/home"
unset DISPLAY WAYLAND_DISPLAY DBUS_SESSION_BUS_ADDRESS DBUS_SYSTEM_BUS_ADDRESS
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
  sock=$(ls "$RUN"/wayland-* 2>/dev/null | head -1) || true
  [ -n "$sock" ] && [ -S "$sock" ] && break
  kill -0 "$sway_pid" 2>/dev/null || { echo "error: sway exited; see $RUN/sway.log" >&2; exit 2; }
  sleep 0.05
  done
[ -n "$sock" ] || { echo "error: sway did not create a socket" >&2; exit 2; }
export WAYLAND_DISPLAY=$(basename "$sock")

echo "==> running $BIN"
FOX_WAYLAND_TRACE=1 WAYLAND_DEBUG=1 timeout 10 "$BIN" --no-dialogs >"$RUN/app.log" 2>&1
status=$?

failed=""
[ "$status" = 0 ] || failed="the application did not finish by itself (status $status)"
grep -q "zwp_idle_inhibit_manager_v1#" "$RUN/app.log" || failed="${failed:-the manager was not bound}"
grep -qE "zwp_idle_inhibit_manager_v1#[0-9]+\.create_inhibitor" "$RUN/app.log" || failed="${failed:-no inhibitor was created}"
grep -qE "zwp_idle_inhibitor_v1#[0-9]+\.destroy" "$RUN/app.log" || failed="${failed:-the inhibitor was never destroyed}"
grep -q "idle inhibited for window" "$RUN/app.log" || failed="${failed:-the library did not report the inhibit}"
grep -q "idle inhibit released for window" "$RUN/app.log" || failed="${failed:-the library did not report the release}"

if [ -n "$failed" ]; then
  echo "FAIL: $failed"
  grep -E "idle" "$RUN/app.log" | tail -12
  exit 1
  fi

grep -E "idle inhibit" "$RUN/app.log" | tail -4
echo "PASS: the window held the screen on through an inhibitor and let go of it"
exit 0
