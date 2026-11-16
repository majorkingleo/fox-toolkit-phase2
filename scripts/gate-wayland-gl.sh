#!/usr/bin/env bash
#
# Gate: an FXGLCanvas on Wayland is an EGL window on a wl_subsurface, and
# drawing into it reaches the compositor.
#
# tests/glcanvas clears a red frame, reads the pixel back with glReadPixels and
# exits 0 only when it is red.  A headless sway with the GL renderer (Mesa
# llvmpipe) supplies EGL and dmabuf support; the pixman renderer of the other
# gates cannot do client side GL.
#
#   ./scripts/gate-wayland-gl.sh
#   ./scripts/gate-wayland-gl.sh --build=tmp/build-fox2-wayland-gl
#
# Exit status is 0 on pass, 1 on a failed check, 2 when the run itself failed.
#
set -uo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
BUILD="$ROOT/tmp/build-fox2-wayland"
RUN="$ROOT/tmp/wayland-gl-gate"

while [ $# -gt 0 ]; do
  case "$1" in
    --build)     shift; BUILD=$1 ;;
    --build=*)   BUILD=${1#*=} ;;
    --run-dir)   shift; RUN=$1 ;;
    --run-dir=*) RUN=${1#*=} ;;
    -h|--help)   sed -n '2,14p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) echo "error: unknown option '$1'" >&2; exit 2 ;;
    esac
  shift
  done

BIN="$BUILD/bin/glcanvas"
[ -x "$BIN" ] || { echo "error: no glcanvas at $BIN (build with scripts/build-fox2-wayland.sh --opengl)" >&2; exit 2; }
command -v sway >/dev/null 2>&1 || { echo "error: sway not found" >&2; exit 2; }
for pc in egl wayland-egl gl; do
  pkg-config --exists "$pc" 2>/dev/null || { echo "error: $pc not found" >&2; exit 2; }
  done

rm -rf -- "$RUN"
mkdir -p -- "$RUN/runtime" "$RUN/home"
chmod 700 "$RUN/runtime"
printf 'output HEADLESS-1 resolution 640x480 bg #101010 solid_color\n' >"$RUN/config"

export XDG_RUNTIME_DIR="$RUN/runtime" HOME="$RUN/home"
unset DISPLAY WAYLAND_DISPLAY DBUS_SESSION_BUS_ADDRESS DBUS_SYSTEM_BUS_ADDRESS XDG_CONFIG_HOME
export WLR_BACKENDS=headless WLR_LIBINPUT_NO_DEVICES=1 WLR_HEADLESS_OUTPUTS=1
export WLR_RENDERER=gl LIBGL_ALWAYS_SOFTWARE=1

sway --unsupported-gpu -c "$RUN/config" >"$RUN/sway.log" 2>&1 &
sway_pid=$!
cleanup(){
  kill "$sway_pid" 2>/dev/null || true
  wait "$sway_pid" 2>/dev/null || true
  }
trap cleanup EXIT INT TERM

sock=""
for i in $(seq 1 200); do
  sock=$(ls "$RUN/runtime"/wayland-* 2>/dev/null | head -1)
  [ -n "$sock" ] && [ -S "$sock" ] && break
  sleep 0.05
  done
[ -n "$sock" ] || { echo "error: no sway socket; see $RUN/sway.log" >&2; exit 2; }
export WAYLAND_DISPLAY=$(basename "$sock")

echo "==> running glcanvas"
FOX_WAYLAND_TRACE=1 timeout 60 "$BIN" >"$RUN/app.log" 2>&1
rc=$?
echo "==> application exit: $rc"
grep -E "EGL [0-9]|GL canvas|glcanvas:" "$RUN/app.log" | sed 's/^/    /'
if [ "$rc" -eq 0 ] && grep -q "glcanvas: GL canvas draws" "$RUN/app.log"; then
  echo "PASS: the EGL canvas on its wl_subsurface drew the pixel"
  exit 0
  fi
if [ "$rc" -eq 2 ]; then
  echo "FAIL: the build has no OpenGL; build with scripts/build-fox2-wayland.sh --opengl"
  exit 1
  fi
echo "FAIL: the GL canvas did not draw (exit $rc)"
echo "      see $RUN/app.log and $RUN/sway.log"
exit 1
