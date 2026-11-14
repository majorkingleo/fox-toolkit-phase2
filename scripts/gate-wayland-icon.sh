#!/usr/bin/env bash
#
# Toplevel icon gate:  a window that carries an icon hands it to a compositor
# that offers xdg_toplevel_icon_v1, and keeps working with one that does not.
#
# Headless sway knows no toplevel icons, KWin does, so the gate runs KWin's
# virtual backend, like the decoration gate, and checks the wire: the manager is
# bound, an icon object is made, the pixels go into a shared memory buffer, and
# set_icon() names the toplevel.  Then it runs the same application against
# headless sway and expects the fallback to be quiet.
#
set -uo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
BUILD="$ROOT/tmp/build-fox2-wayland"
RUN="$ROOT/tmp/wayland-icon-gate"
BIN="$BUILD/bin/icon"

[ -x "$BIN" ] || { echo "error: no binary at $BIN" >&2; exit 2; }

rm -rf -- "$RUN"
mkdir -p -- "$RUN/kwin/home" "$RUN/kwin/runtime" "$RUN/sway/home"
chmod 700 "$RUN/kwin/runtime" "$RUN/sway/home"
RUN=$(cd -- "$RUN" && pwd)

echo "==> starting kwin_wayland --virtual"
(
  export XDG_RUNTIME_DIR="$RUN/kwin/runtime"
  export HOME="$RUN/kwin/home"
  unset DISPLAY WAYLAND_DISPLAY DBUS_SESSION_BUS_ADDRESS DBUS_SYSTEM_BUS_ADDRESS
  kwin_wayland --virtual --socket wayland-kwin --no-lockscreen >"$RUN/kwin/kwin.log" 2>&1 &
  kwin_pid=$!
  for i in $(seq 1 200); do
    [ -S "$RUN/kwin/runtime/wayland-kwin" ] && break
    sleep 0.05
    done
  if [ ! -S "$RUN/kwin/runtime/wayland-kwin" ]; then
    echo "FAIL: kwin did not start"; tail -5 "$RUN/kwin/kwin.log"; kill "$kwin_pid" 2>/dev/null; exit 1
    fi
  export WAYLAND_DISPLAY=wayland-kwin
  FOX_WAYLAND_TRACE=1 WAYLAND_DEBUG=1 "$BIN" --no-dialogs >"$RUN/kwin/app.log" 2>&1 &
  app_pid=$!
  sleep 2.0
  kill "$app_pid" 2>/dev/null
  wait "$app_pid" 2>/dev/null
  kill "$kwin_pid" 2>/dev/null
  wait "$kwin_pid" 2>/dev/null
  )

failed=""
grep -qE "xdg_toplevel_icon_manager_v1#[0-9]+\.set_icon" "$RUN/kwin/app.log" || failed="the icon never reached the compositor"
grep -qE "xdg_toplevel_icon_v1#[0-9]+\.add_buffer" "$RUN/kwin/app.log" || failed="${failed:-no icon buffer was offered}"
grep -q "icon 16x16 set on window" "$RUN/kwin/app.log" || failed="${failed:-the application did not set an icon}"

if [ -n "$failed" ]; then
  echo "FAIL: $failed"
  grep -E "icon|toplevel_icon" "$RUN/kwin/app.log" | tail -12
  exit 1
  fi

echo "==> the same application where the protocol is missing"
(
  export XDG_RUNTIME_DIR="$RUN/sway/runtime"
  export HOME="$RUN/sway/home"
  mkdir -p -- "$RUN/sway/runtime"
  chmod 700 "$RUN/sway/runtime"
  unset DISPLAY WAYLAND_DISPLAY DBUS_SESSION_BUS_ADDRESS DBUS_SYSTEM_BUS_ADDRESS
  export WLR_BACKENDS=headless WLR_LIBINPUT_NO_DEVICES=1 WLR_RENDERER=pixman WLR_HEADLESS_OUTPUTS=1
  echo 'output HEADLESS-1 resolution 1024x640 bg #101010 solid_color' >"$RUN/sway/config"
  sway --unsupported-gpu -c "$RUN/sway/config" >"$RUN/sway/sway.log" 2>&1 &
  sway_pid=$!
  sock=""
  for i in $(seq 1 200); do
    sock=$(ls "$RUN/sway/runtime"/wayland-* 2>/dev/null | head -1) || true
    [ -n "$sock" ] && [ -S "$sock" ] && break
    sleep 0.05
    done
  export WAYLAND_DISPLAY=$(basename "$sock")
  FOX_WAYLAND_TRACE=1 "$BIN" --no-dialogs >"$RUN/sway/app.log" 2>&1 &
  app_pid=$!
  sleep 1.5
  kill "$app_pid" 2>/dev/null
  wait "$app_pid" 2>/dev/null
  kill "$sway_pid" 2>/dev/null
  wait "$sway_pid" 2>/dev/null
  )

grep -q "no toplevel icon protocol" "$RUN/sway/app.log" || { echo "FAIL: the fallback was not taken"; tail -5 "$RUN/sway/app.log"; exit 1; }

grep -E "icon .* set on window|toplevel_icon_v1#[0-9]+\.add_buffer" "$RUN/kwin/app.log" | tail -2
echo "PASS: the icon reaches a compositor that wants it, and the rest keep working"
exit 0
