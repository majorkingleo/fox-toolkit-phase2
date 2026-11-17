#!/usr/bin/env bash
#
# Themed close gate (X11):  closing a themed window quits the application
# cleanly.
#
# Quitting closes the display and then runs the destructor, and the destructor
# used to ask the style to free the XRender pictures it had cached for a display
# that was already closed; that aborted inside _XAllocID.  The gate opens the
# table test with a themed registry, closes the window with a WM_DELETE_WINDOW
# message the way a window manager does, and requires the process to end with
# status zero.
#
#   ./scripts/gate-x11-theme-close.sh
#   ./scripts/gate-x11-theme-close.sh --build=tmp/build-fox2-linux
#
# Needs an X display; the theme is the one given by --theme.
#
set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

BUILD=tmp/build-fox2-linux-cmake
RUN=tmp/x11-theme-close
THEME=/usr/share/themes/Breeze

usage(){
  sed -n '2,16p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
  }

while [ $# -gt 0 ]; do
  case "$1" in
    --build)   shift; BUILD=$1 ;;
    --build=*) BUILD=${1#*=} ;;
    --run-dir) shift; RUN=$1 ;;
    --run-dir=*) RUN=${1#*=} ;;
    --theme)   shift; THEME=$1 ;;
    --theme=*) THEME=${1#*=} ;;
    -h|--help) usage; exit 0 ;;
    *) echo "error: unknown option '$1'" >&2; usage >&2; exit 2 ;;
    esac
  shift
  done

[ -x "$BUILD/bin/table" ] || { echo "error: no binary at $BUILD/bin/table" >&2; exit 2; }
[ -d "$THEME" ] || { echo "error: no theme at $THEME" >&2; exit 2; }
[ -n "${DISPLAY-}" ] || { echo "error: DISPLAY is not set" >&2; exit 2; }
command -v cc >/dev/null 2>&1 || { echo "error: cc not found" >&2; exit 2; }

rm -rf -- "$RUN"
mkdir -p -- "$RUN/home" "$RUN/registry" "$RUN/cache"
RUN=$(cd -- "$RUN" && pwd)
printf '[SETTINGS]\ntheme=%s\n[display]\nscale=100\n' "$THEME" >"$RUN/registry/fox.rc"
cc -O1 -o "$RUN/xclose" tools/xclose.c -lX11 || exit 2

echo "==> table test, theme $THEME"
FOXDIR="$RUN/registry" HOME="$RUN/home" XDG_CACHE_HOME="$RUN/cache" \
  "$BUILD/bin/table" >"$RUN/app.log" 2>&1 &
app_pid=$!

# Wait for the window to exist
closed=0
for i in $(seq 1 100); do
  if "$RUN/xclose" "Table Widget Test" >/dev/null 2>&1; then closed=1; break; fi
  kill -0 "$app_pid" 2>/dev/null || break
  sleep 0.1
  done
[ "$closed" = 1 ] || { echo "FAIL: the table window never appeared" ; kill "$app_pid" 2>/dev/null; exit 1; }

echo "==> closing the window"
state=timeout
for i in $(seq 1 100); do
  if ! kill -0 "$app_pid" 2>/dev/null; then
    wait "$app_pid"
    state="exit $?"
    break
    fi
  sleep 0.1
  done
if [ "$state" = timeout ]; then
  kill "$app_pid" 2>/dev/null
  echo "FAIL: the application did not end after the window was closed"
  exit 1
  fi
if [ "$state" != "exit 0" ]; then
  echo "FAIL: the application ended with '$state' ($(tail -3 "$RUN/app.log" | tr '\n' ' '))"
  exit 1
  fi

echo "PASS: the themed window closes and the application ends with status zero"
exit 0
