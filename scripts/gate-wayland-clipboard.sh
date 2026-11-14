#!/usr/bin/env bash
#
# Clipboard gate:  copy in FOX and read the text with scripts/wlclip, then write
# the clipboard with wlclip and paste it in FOX.
#
# Both directions matter.  Copying is not "put the text in the board": the
# client takes the selection and then has to answer a peer that asks for a mime
# type on a pipe, asynchronously, while its event loop keeps running.  Pasting is
# the reverse, and it is the only way to see whether the offer of a peer is kept
# and read correctly.
#
# wlclip is a plain wl_data_device client, so the peer is never FOX: that is the
# acceptance of phase 8 ("between FOX and a non-FOX Wayland client").
#
set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

RUN=${RUN:-tmp/sway-clipboard}
OUT=${OUT:-tmp/wlxinject}
SRC=scripts/wlxinject.c
XML=scripts/protocols/wlr-virtual-pointer-unstable-v1.xml
CLIP=scripts/wlclip.c
DC_XML=scripts/protocols/wlr-data-control-unstable-v1.xml

COPY_BUTTON=500,60
PASTE_BUTTON=500,100
COPY_SELECTION_BUTTON=500,140
PASTE_SELECTION_BUTTON=500,180
FOXTEXT="fox text for the peer"
PEERTEXT="peer text for fox"
FOXSELECTION="fox selection for the peer"
PEERSELECTION="peer selection for fox"

# The pointer injector and the peer client --------------------------------
mkdir -p -- "$OUT"
wayland-scanner private-code "$XML" "$OUT/vptr-protocol.c" || exit 2
wayland-scanner client-header "$XML" "$OUT/wlr-virtual-pointer-unstable-v1-client-protocol.h" || exit 2
wayland-scanner private-code "$DC_XML" "$OUT/datacontrol-protocol.c" || exit 2
wayland-scanner client-header "$DC_XML" "$OUT/wlr-data-control-unstable-v1-client-protocol.h" || exit 2
cc -O1 -Wall -I"$OUT" -o "$OUT/wlxinject" "$SRC" "$OUT/vptr-protocol.c" -lwayland-client || exit 2
cc -O1 -Wall -I"$OUT" -o "$OUT/wlclip" "$CLIP" "$OUT/datacontrol-protocol.c" -lwayland-client || exit 2

# Start sway ---------------------------------------------------------------
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
  kill "${app_pid:-}" "${peer_pid:-}" 2>/dev/null || true
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

fail(){
  echo "FAIL: $1"
  echo "== application"
  tail -12 "$RUN/app.log" 2>/dev/null
  echo "== peer"
  tail -8 "$RUN/peer.log" 2>/dev/null
  exit 1
  }

click(){
  "$OUT/wlxinject" "${1%,*}" "${1#*,}" move sleep:120 down sleep:120 up sleep:250 >/dev/null 2>&1
  sleep 0.5
  }

# FOX application ----------------------------------------------------------
FOX_CLIPBOARD_TEXT="$FOXTEXT" FOX_SELECTION_TEXT="$FOXSELECTION" FOX_WAYLAND_TRACE=1 tmp/build-fox2-wayland/bin/clipboard >>"$RUN/app.log" 2>&1 &
app_pid=$!
sleep 2.0
kill -0 "$app_pid" 2>/dev/null || fail "the application did not start"

# Direction one: FOX copies, the peer reads -------------------------------
echo "==> copy in FOX, read with wlclip"
click "$COPY_BUTTON"
grep -q "clipboard taken" "$RUN/app.log" || fail "FOX did not take the clipboard"
got=$("$OUT/wlclip" --read 2>"$RUN/peer.log")
[ "$got" = "$FOXTEXT" ] || fail "the peer read \"$got\" instead of \"$FOXTEXT\""
echo "==> peer read: $got"

# Direction two: the peer writes, FOX pastes ------------------------------
echo "==> write with wlclip, paste in FOX"
"$OUT/wlclip" --write "$PEERTEXT" 6 >>"$RUN/peer.log" 2>&1 &
peer_pid=$!
sleep 1.0
click "$PASTE_BUTTON"
sleep 0.5
grep -q "pasted \"$PEERTEXT\"" "$RUN/app.log" || fail "FOX did not paste the text of the peer"
echo "==> FOX pasted: $(grep 'pasted' "$RUN/app.log" | tail -1)"

kill "$peer_pid" 2>/dev/null
wait "$peer_pid" 2>/dev/null
peer_pid=""

# Direction three: FOX copies to the primary selection, the peer reads -------
echo "==> copy to the selection in FOX, read with wlclip"
click "$COPY_SELECTION_BUTTON"
grep -q "selection taken" "$RUN/app.log" || fail "FOX did not take the primary selection"
got=$("$OUT/wlclip" --read-primary 2>>"$RUN/peer.log")
[ "$got" = "$FOXSELECTION" ] || fail "the peer read \"$got\" from the selection instead of \"$FOXSELECTION\""
echo "==> peer read from the selection: $got"

# Direction four: the peer writes the selection, FOX pastes -----------------
echo "==> write the selection with wlclip, paste in FOX"
"$OUT/wlclip" --write-primary "$PEERSELECTION" 6 >>"$RUN/peer.log" 2>&1 &
peer_pid=$!
sleep 1.0
click "$PASTE_SELECTION_BUTTON"
sleep 0.5
grep -q "selection pasted \"$PEERSELECTION\"" "$RUN/app.log" || fail "FOX did not paste the selection of the peer"
echo "==> FOX pasted: $(grep 'selection pasted' "$RUN/app.log" | tail -1)"
kill "$peer_pid" 2>/dev/null
wait "$peer_pid" 2>/dev/null
peer_pid=""

echo "PASS: clipboard and primary selection work in both directions with a client that is not FOX"
exit 0
