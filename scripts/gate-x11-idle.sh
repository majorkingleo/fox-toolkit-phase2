#!/usr/bin/env bash
#
# Idle inhibit gate (X11):  a window that asks the system to stay awake runs
# "xdg-screensaver suspend <window>" once it exists and "xdg-screensaver resume
# <window>" when it gives the request up, naming the same window both times.
#
# A stand-in for xdg-screensaver logs its arguments, so no desktop is needed;
# the gate only needs an X display.
#
set -uo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
BUILD="${FOX_BUILD_DIR:-$HOME/build/fox2-cmake}"
BIN="$BUILD/bin/idle"
RUN="$ROOT/tmp/x11-idle-gate"

[ -x "$BIN" ] || { echo "error: no binary at $BIN" >&2; exit 2; }
[ -n "${DISPLAY-}" ] || { echo "error: DISPLAY is not set" >&2; exit 2; }

rm -rf -- "$RUN"
mkdir -p -- "$RUN/bin"
cat >"$RUN/bin/xdg-screensaver" <<EOF
#!/bin/sh
echo "\$@" >>"$RUN/calls.log"
EOF
chmod +x "$RUN/bin/xdg-screensaver"

echo "==> running the idle test"
PATH="$RUN/bin:$PATH" LD_LIBRARY_PATH="$BUILD/lib:${LD_LIBRARY_PATH-}" "$BIN" >"$RUN/app.log" 2>&1
sleep 0.3

failed=""
[ -f "$RUN/calls.log" ] || failed="xdg-screensaver was never run"
if [ -z "$failed" ]; then
  first=$(sed -n 1p "$RUN/calls.log")
  second=$(sed -n 2p "$RUN/calls.log")
  case "$first" in
    "suspend "[0-9]*) ;;
    *) failed="the first call was '$first', not a suspend" ;;
    esac
  [ -n "$failed" ] || [ "${second%% *}" = "resume" ] || failed="the second call was '$second', not a resume"
  [ -n "$failed" ] || [ "${first#suspend }" = "${second#resume }" ] || failed="suspend and resume named different windows"
  [ -n "$failed" ] || [ "$(wc -l <"$RUN/calls.log")" = 2 ] || failed="expected exactly two calls"
  fi

if [ -n "$failed" ]; then
  echo "FAIL: $failed"
  cat "$RUN/calls.log" 2>/dev/null
  cat "$RUN/app.log"
  exit 1
  fi

echo "PASS: $(sed -n 1p "$RUN/calls.log"), then $(sed -n 2p "$RUN/calls.log")"
