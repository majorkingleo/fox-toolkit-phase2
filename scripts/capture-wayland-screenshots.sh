#!/usr/bin/env bash
#
# Store the Wayland screenshots of the tests that came after the pixel gate:
# the drag and drop test, the window icon and the same test UI at an output
# scale of 2.  Everything is captured from a dumped frame, so the images are the
# buffers the application really committed and not a compositor screenshot.
#
#   ./scripts/capture-wayland-screenshots.sh
#
# The frames go to screenshots/tests/fox2/wayland/.
#
set -uo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
BUILD="$ROOT/tmp/build-fox2-wayland"
RUN="$ROOT/tmp/wayland-shots"
SHOTS="$ROOT/screenshots/tests/fox2/wayland"

command -v sway >/dev/null 2>&1 || { echo "error: sway not found" >&2; exit 2; }
python3 -c 'import PIL' >/dev/null 2>&1 || { echo "error: python3 with Pillow is needed" >&2; exit 2; }
mkdir -p -- "$SHOTS" "$RUN"
RUN=$(cd -- "$RUN" && pwd)
rm -rf -- "$RUN"/*
mkdir -p -- "$RUN/runtime" "$RUN/home"
chmod 700 "$RUN/runtime"

export XDG_RUNTIME_DIR="$RUN/runtime"
export HOME="$RUN/home"
unset DISPLAY WAYLAND_DISPLAY DBUS_SESSION_BUS_ADDRESS DBUS_SYSTEM_BUS_ADDRESS XDG_CONFIG_HOME
export WLR_BACKENDS=headless WLR_LIBINPUT_NO_DEVICES=1 WLR_RENDERER=pixman WLR_HEADLESS_OUTPUTS=1

# Capture one frame of an application; $1 is the binary, $2 the shot name,
# $3 the output scale and $4 the size of the application window
capture(){
  local bin="$1" name="$2" scale="$3"
  local dir="$RUN/$name"
  mkdir -p -- "$dir/frames" "$dir/home"
  chmod 700 "$dir/home"
  printf 'output HEADLESS-1 resolution 1024x640 scale %s bg #101010 solid_color\n' "$scale" >"$dir/config"
  (
    export XDG_RUNTIME_DIR="$dir"
    export HOME="$dir/home"
    sway --unsupported-gpu -c "$dir/config" >"$dir/sway.log" 2>&1 &
    local sway_pid=$!
    local sock=""
    for i in $(seq 1 200); do
      sock=$(ls "$dir"/wayland-* 2>/dev/null | head -1) || true
      [ -n "$sock" ] && [ -S "$sock" ] && break
      sleep 0.05
      done
    export WAYLAND_DISPLAY=$(basename "$sock")
    FOX_WAYLAND_DUMP="$dir/frames" "$bin" --no-dialogs >"$dir/app.log" 2>&1 &
    local app_pid=$!
    sleep 2.0
    kill "$app_pid" 2>/dev/null
    wait "$app_pid" 2>/dev/null
    kill "$sway_pid" 2>/dev/null
    wait "$sway_pid" 2>/dev/null
    )
  local frame
  frame=$(ls "$dir/frames"/*.ppm 2>/dev/null | head -1)
  if [ -z "$frame" ]; then
    echo "error: no frame was dumped for $name" >&2
    return 1
    fi
  python3 - "$frame" "$SHOTS/$name.png" <<'EOF'
import sys
from PIL import Image
Image.open(sys.argv[1]).save(sys.argv[2])
EOF
  echo "saved $name.png from $(basename "$frame")"
  }

echo "==> drag and drop"
capture "$BUILD/bin/dnd" dnd 1

echo "==> window icon"
capture "$BUILD/bin/icon" icon 1

echo "==> the DPI test at an output scale of 2"
capture "$BUILD/bin/dpi" dpi-scale2 2

ls -1 "$SHOTS"
