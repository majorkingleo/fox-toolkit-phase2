#!/usr/bin/env bash
#
# Repaint gate:  the same picture reached in one step and in three must be the
# same pixels.
#
# A widget that moves or resizes leaves the pixels it painted behind, and on
# X11 the server exposes the uncovered area.  Here every window shares one
# buffer, so the module has to repaint the parent itself; when it does not, the
# frame keeps ghosts of the widgets that moved (this gate was written after
# exactly that).
#
# The gate needs no reference image:  it drives the button test to the same
# state twice, once in one click and once in three clicks, and compares the two
# dumped frames.  Stale pixels make the three click frame differ from the one
# click frame, while a correct repaint makes them identical.
#
set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

RUN=${RUN:-tmp/sway-repaint}
OUT=${OUT:-tmp/wlxinject}
SRC=scripts/wlxinject.c
XML=scripts/protocols/wlr-virtual-pointer-unstable-v1.xml

mkdir -p -- "$OUT"
wayland-scanner private-code "$XML" "$OUT/vptr-protocol.c" || exit 2
wayland-scanner client-header "$XML" "$OUT/wlr-virtual-pointer-unstable-v1-client-protocol.h" || exit 2
cc -O1 -Wall -I"$OUT" -o "$OUT/wlxinject" "$SRC" "$OUT/vptr-protocol.c" -lwayland-client || exit 2

rm -rf -- "$RUN"
mkdir -p -- "$RUN/home"
RUN=$(cd -- "$RUN" && pwd)
chmod 700 "$RUN"
echo 'output HEADLESS-1 resolution 1024x640 bg #101010 solid_color' >"$RUN/config"

export XDG_RUNTIME_DIR="$RUN"
export HOME="$RUN/home"
unset DISPLAY WAYLAND_DISPLAY
export WLR_BACKENDS=headless WLR_LIBINPUT_NO_DEVICES=1 WLR_RENDERER=pixman WLR_HEADLESS_OUTPUTS=1
export WLXINJECT_WIDTH=1024 WLXINJECT_HEIGHT=640

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
  sleep 0.05
  done
[ -n "$sock" ] || { echo "error: no sway socket"; exit 2; }
export WAYLAND_DISPLAY=$(basename "$sock")

# The radio buttons of the button test, in the coordinates of the layout
AFTER_TEXT=886,149
CENTERED=890,177

# Run the application and click a list of points, then keep the last frame
shoot(){
  local dump="$1"; shift
  local point
  rm -rf -- "$dump"
  mkdir -p -- "$dump"
  FOX_WAYLAND_DUMP="$dump" tmp/build-fox2-wayland/bin/button --no-dialogs >"$RUN/app.log" 2>&1 &
  local pid=$!
  sleep 2.0
  for point in "$@"; do
    "$OUT/wlxinject" "${point%,*}" "${point#*,}" move sleep:150 down sleep:150 up >/dev/null 2>&1
    sleep 0.6
    done
  sleep 1.0
  kill "$pid" 2>/dev/null
  wait "$pid" 2>/dev/null
  ls -t "$dump"/*.ppm 2>/dev/null | head -1
  }

echo "==> after text, in one click"
one=$(shoot "$RUN/one" "$AFTER_TEXT")
echo "==> after text, again after two other settings"
three=$(shoot "$RUN/three" "$AFTER_TEXT" "$CENTERED" "$AFTER_TEXT")

echo "==> $one"
echo "==> $three"
[ -n "$one" ] && [ -n "$three" ] || { echo "FAIL: no frame was dumped"; exit 1; }

if cmp -s "$one" "$three"; then
  echo "PASS: the frame does not keep pixels of the widgets that moved"
  exit 0
  fi
changed=$(python3 - "$one" "$three" <<'EOF'
import sys
def load(path):
    with open(path,'rb') as f:
        data=f.read()
    parts=data.split(b'\n',3)
    return parts[3]
a=load(sys.argv[1])
b=load(sys.argv[2])
if len(a)!=len(b):
    print("the two frames are not even the same size")
    raise SystemExit
print("%d of %d bytes differ" % (sum(1 for x,y in zip(a,b) if x!=y),len(a)))
EOF
)
echo "FAIL: stale pixels remain ($changed)"
exit 1
