#!/usr/bin/env bash
#
# Scale gate:  run the same application on a headless sway twice, once with an
# output at scale 1 and once at scale 2, and compare what the application says
# about its display scale and how big its window came out.
#
# A Wayland compositor has no DPI: it announces a scale per output, and a client
# that draws its own pixels has to follow it with its metrics and its fonts,
# while telling the compositor through wp_viewport how many logical pixels the
# buffer covers.  So the two things worth checking are the display scale the
# application reports and that the window really grew in device pixels.
#
set -uo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
BUILD="$ROOT/tmp/build-fox2-wayland"
RUN="$ROOT/tmp/wayland-scale-gate"

rm -rf -- "$RUN"
mkdir -p -- "$RUN/home"
RUN=$(cd -- "$RUN" && pwd)
chmod 700 "$RUN"

for tool in sway; do
  command -v "$tool" >/dev/null 2>&1 || { echo "error: $tool not found" >&2; exit 2; }
  done

# One run of the application at the given output scale; prints the theme scale it
# reported and the name of the frame it dumped
run_at_scale(){
  local scale="$1"
  local dir="$RUN/scale$scale"
  local sock
  mkdir -p -- "$dir/home" "$dir/frames"
  chmod 700 "$dir" "$dir/home"
  printf 'output HEADLESS-1 resolution 1024x640 scale %s bg #101010 solid_color\n' "$scale" >"$dir/config"
  (
    export XDG_RUNTIME_DIR="$dir"
    export HOME="$dir/home"
    unset DISPLAY WAYLAND_DISPLAY
    export WLR_BACKENDS=headless WLR_LIBINPUT_NO_DEVICES=1 WLR_RENDERER=pixman WLR_HEADLESS_OUTPUTS=1
    sway --unsupported-gpu -c "$dir/config" >"$dir/sway.log" 2>&1 &
    local sway_pid=$!
    for i in $(seq 1 200); do
      sock=$(ls "$dir"/wayland-* 2>/dev/null | head -1)
      [ -n "$sock" ] && [ -S "$sock" ] && break
      sleep 0.05
      done
    if [ -z "$sock" ]; then echo "no socket" >&2; kill "$sway_pid" 2>/dev/null; exit 2; fi
    export WAYLAND_DISPLAY=$(basename "$sock")
    FOX_WAYLAND_TRACE=1 FOX_WAYLAND_DUMP="$dir/frames" "$BUILD/bin/dpi" >"$dir/app.out" 2>&1 &
    local app_pid=$!
    sleep 2.5
    kill "$app_pid" 2>/dev/null
    wait "$app_pid" 2>/dev/null
    kill "$sway_pid" 2>/dev/null
    wait "$sway_pid" 2>/dev/null
    )
  }

echo "==> application at output scale 1"
run_at_scale 1
echo "==> application at output scale 2"
run_at_scale 2

one=$(grep -m1 "theme scale" "$RUN/scale1/app.out" | awk '{print $NF}')
two=$(grep -m1 "theme scale" "$RUN/scale2/app.out" | awk '{print $NF}')
grep -q "compositor scale 1.000" "$RUN/scale1/app.out" || { echo "FAIL: scale 1 was not seen"; tail -5 "$RUN/scale1/app.out"; exit 1; }
grep -q "compositor scale 2.000" "$RUN/scale2/app.out" || { echo "FAIL: scale 2 was not seen"; tail -5 "$RUN/scale2/app.out"; exit 1; }

# The frame of the scaled run is the same window in more device pixels
frame1=$(ls "$RUN/scale1/frames"/*.ppm 2>/dev/null | head -1)
frame2=$(ls "$RUN/scale2/frames"/*.ppm 2>/dev/null | head -1)
[ -n "$frame1" ] && [ -n "$frame2" ] || { echo "FAIL: no frames were dumped"; exit 1; }
size1=$(sed -n 2p "$frame1")
size2=$(sed -n 2p "$frame2")
w1=${size1% *}
w2=${size2% *}

echo "==> scale 1: theme scale $one, frame $size1"
echo "==> scale 2: theme scale $two, frame $size2"

[ "$one" = "1.00" ] || { echo "FAIL: the theme scale at output scale 1 is $one"; exit 1; }
[ "$two" = "2.00" ] || { echo "FAIL: the theme scale at output scale 2 is $two"; exit 1; }
[ "$w2" -gt "$w1" ] || { echo "FAIL: the window did not grow with the scale ($w1 then $w2)"; exit 1; }

echo "PASS: the output scale reaches the application and its window grows with it"
exit 0
