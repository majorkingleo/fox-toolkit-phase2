#!/usr/bin/env bash
#
# The decoration gate of docs/wayland-plan.md: a toplevel asks for server side
# decorations and a compositor that offers them agrees.
#
# Headless weston does not offer xdg-decoration, so this gate uses KWin's
# virtual backend, which does. The application runs with the module trace on and
# the gate passes when the compositor answered with the server side mode.
#
#   ./scripts/gate-wayland-decoration.sh
#   ./scripts/gate-wayland-decoration.sh --bin=tmp/build-fox2-wayland/bin/button
#
# Options:
#   --bin=FILE     application to run (default: tmp/build-fox2-wayland/bin/hello)
#   --run-dir=DIR  runtime directory (default: tmp/wayland-decoration-gate)
#   --seconds=N    how long the application may run (default: 4)
#   -h|--help      show this help
#
# Exit status is 0 when the compositor agreed, 1 when it did not, 2 when the
# harness could not start or KWin is not installed.
#
set -uo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
BUILD="$ROOT/tmp/build-fox2-wayland"
RUN="$ROOT/tmp/wayland-decoration-gate"
BIN=""
SECONDS_TO_RUN=4

usage(){
  sed -n '2,22p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
  }

while [ $# -gt 0 ]; do
  case "$1" in
    --bin)       shift; BIN=$1 ;;
    --bin=*)     BIN=${1#*=} ;;
    --run-dir)   shift; RUN=$1 ;;
    --run-dir=*) RUN=${1#*=} ;;
    --seconds)   shift; SECONDS_TO_RUN=$1 ;;
    --seconds=*) SECONDS_TO_RUN=${1#*=} ;;
    -h|--help)   usage; exit 0 ;;
    *) echo "error: unknown option '$1'" >&2; usage >&2; exit 2 ;;
    esac
  shift
  done

[ -n "$BIN" ] || BIN="$BUILD/bin/hello"
if ! command -v kwin_wayland >/dev/null 2>&1; then
  echo "skip: kwin_wayland not installed, no compositor here offers xdg-decoration" >&2
  exit 2
  fi
[ -x "$BIN" ] || { echo "error: no binary at $BIN (build with FOX_WAYLAND_TARGET=all)" >&2; exit 2; }

rm -rf -- "$RUN"
mkdir -p -- "$RUN/home" "$RUN/dump"
RUN=$(cd -- "$RUN" && pwd)
chmod 700 "$RUN"

export XDG_RUNTIME_DIR="$RUN"
export HOME="$RUN/home"
unset DISPLAY WAYLAND_DISPLAY
export QT_QPA_PLATFORM=offscreen

echo "==> starting kwin_wayland --virtual"
kwin_wayland --virtual --socket wayland-kwin --no-lockscreen >"$RUN/kwin.log" 2>&1 &
kwin_pid=$!
cleanup(){
  kill "$kwin_pid" 2>/dev/null || true
  wait "$kwin_pid" 2>/dev/null || true
  }
trap cleanup EXIT INT TERM

for i in $(seq 1 400); do
  [ -S "$RUN/wayland-kwin" ] && break
  kill -0 "$kwin_pid" 2>/dev/null || { echo "error: kwin exited; see $RUN/kwin.log" >&2; tail -5 "$RUN/kwin.log" >&2; exit 2; }
  sleep 0.05
  done
[ -S "$RUN/wayland-kwin" ] || { echo "error: kwin did not create a socket" >&2; exit 2; }

export WAYLAND_DISPLAY=wayland-kwin
export FOX_WAYLAND_DUMP="$RUN/dump"
export FOX_WAYLAND_TRACE=1

echo "==> running $BIN for ${SECONDS_TO_RUN}s"
timeout -k 2 "$SECONDS_TO_RUN" "$BIN" --no-dialogs >"$RUN/app.log" 2>&1
rc=$?
if [ "$rc" -ne 124 ] && [ "$rc" -ne 0 ] && [ "$rc" -ne 143 ]; then
  echo "error: $BIN failed with $rc" >&2
  tail -5 "$RUN/app.log" >&2
  exit 2
  fi

if grep -q "decoration of window .* is server side" "$RUN/app.log"; then
  grep "decoration of window" "$RUN/app.log" | head -2
  echo "PASS: the compositor draws the decorations"
  exit 0
  fi

echo "FAIL: no server side decoration was agreed (trace in $RUN/app.log)" >&2
grep -E "decoration|configure" "$RUN/app.log" | tail -6 >&2
exit 1
