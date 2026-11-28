#!/usr/bin/env bash
#
# FoxTerm gate: close-on-exit and close confirmation.
#
# The tab strip promised two policies that the window never applied (plan
# sections 10.3 and 10.5, review bugs B1 and B2):
#
#   - closeOnExit: a tab whose child exited must close under the default
#     policy, and closing the last tab of the last window ends the process;
#     with closeonexit=false the tab stays and keeps the final screen with an
#     "[exited]" suffix;
#   - confirmClose: closing a tab whose child is still running must ask first,
#     and Cancel must leave the session alone.
#
# The gate also covers the shortcut path the policies are reached through: the
# terminal widget handed Ctrl+Shift+W to the child as 0x17, so the window
# accelerators never acted.  Every phase therefore uses the keyboard, not a
# private hook.
#
#   ./scripts/gate-wayland-foxterm.sh
#   ./scripts/gate-wayland-foxterm.sh --bin=FILE --build=DIR
#
# Exit status is 0 when all phases pass, 1 when one fails, 2 when the harness
# could not start.
#
set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

BUILD=tmp/build-fox2-wayland
BIN=
RUN=tmp/sway-foxterm-gate

usage(){
  sed -n '2,23p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
  }

while [ $# -gt 0 ]; do
  case "$1" in
    --bin)     shift; BIN=$1 ;;
    --bin=*)   BIN=${1#*=} ;;
    --build)   shift; BUILD=$1 ;;
    --build=*) BUILD=${1#*=} ;;
    --run-dir) shift; RUN=$1 ;;
    --run-dir=*) RUN=${1#*=} ;;
    -h|--help) usage; exit 0 ;;
    *) echo "error: unknown option '$1'" >&2; usage >&2; exit 2 ;;
    esac
  shift
  done

[ -n "$BIN" ] || BIN="$BUILD/bin/foxterm"
if [ ! -x "$BIN" ]; then
  echo "error: no binary at $BIN (build with scripts/build-fox2-wayland.sh)" >&2
  exit 2
  fi
for tool in sway swaymsg wtype; do
  command -v "$tool" >/dev/null 2>&1 || { echo "error: $tool not found" >&2; exit 2; }
  done

rm -rf -- "$RUN"
mkdir -p -- "$RUN/dump"
RUN=$(cd -- "$RUN" && pwd)
chmod 700 "$RUN"
printf '%s\n' \
  'output HEADLESS-1 resolution 1024x640 bg #101010 solid_color' \
  'default_border pixel 0' >"$RUN/config"

run_home="$RUN/home"
mkdir -p "$run_home"
export XDG_RUNTIME_DIR="$RUN"
export HOME="$run_home"
unset DISPLAY WAYLAND_DISPLAY DBUS_SESSION_BUS_ADDRESS DBUS_SYSTEM_BUS_ADDRESS XDG_CONFIG_HOME SWAYSOCK
export WLR_BACKENDS=headless WLR_LIBINPUT_NO_DEVICES=1 WLR_RENDERER=pixman WLR_HEADLESS_OUTPUTS=1

sway --unsupported-gpu -c "$RUN/config" >"$RUN/sway.log" 2>&1 &
sway_pid=$!
app_pid=
cleanup(){
  [ -n "$app_pid" ] && kill "$app_pid" 2>/dev/null
  kill "$sway_pid" 2>/dev/null
  wait "$sway_pid" 2>/dev/null
  }
trap cleanup EXIT INT TERM

sock=""
for i in $(seq 1 200); do
  sock=$(ls "$RUN"/wayland-* 2>/dev/null | head -1)
  [ -n "$sock" ] && [ -S "$sock" ] && break
  sleep 0.05
  done
[ -n "$sock" ] || { echo "error: no sway socket" >&2; exit 2; }
export WAYLAND_DISPLAY=$(basename "$sock")
export SWAYSOCK=$(ls "$RUN"/sway-ipc.*.sock 2>/dev/null | head -1)

# Print the title of every mapped Wayland window of the compositor
titles(){
  swaymsg -t get_tree 2>/dev/null | python3 -c "
import json,sys
try: t=json.load(sys.stdin)
except Exception: sys.exit(0)
def walk(n):
    if n.get('name'): print(n['name'])
    for c in n.get('nodes',[])+n.get('floating_nodes',[]): walk(c)
walk(t)"
  }

# Number of mapped FoxTerm windows
window_count(){
  titles | grep -c '^FoxTerm' || true
  }

# Wait until a window title matches, 0 when it showed up, 1 on timeout
wait_title(){
  local pattern=$1 limit=${2:-60} i
  for i in $(seq 1 "$limit"); do
    titles | grep -q "$pattern" && return 0
    sleep 0.1
    done
  return 1
  }

# Wait until no window title matches
wait_no_title(){
  local pattern=$1 limit=${2:-60} i
  for i in $(seq 1 "$limit"); do
    titles | grep -q "$pattern" || return 0
    sleep 0.1
    done
  return 1
  }

# Wait for the application process to end; prints the wait status
wait_exit(){
  local limit=${1:-100} i
  for i in $(seq 1 "$limit"); do
    kill -0 "$app_pid" 2>/dev/null || break
    sleep 0.1
    done
  if kill -0 "$app_pid" 2>/dev/null; then
    return 1
    fi
  wait "$app_pid"
  local status=$?
  app_pid=
  return "$status"
  }

alive(){
  [ -n "$app_pid" ] && kill -0 "$app_pid" 2>/dev/null
  }

# Start foxterm with a private home so every phase begins with a known registry
start_app(){
  local home=$1
  shift
  export HOME="$home"
  FOX_WAYLAND_DUMP="$RUN/dump" "$BIN" "$@" >>"$RUN/app.log" 2>&1 &
  app_pid=$!
  }

# Seed a registry with a close-on-exit and a close-confirmation policy
seed_profile(){
  local home=$1 closeonexit=$2 confirm=$3
  mkdir -p "$home/.config"
  cat >"$home/.config/FoxTerm.rc" <<EOF
[SETTINGS]
activeProfile=default

[PROFILE.default]
name=Default
shell=/bin/sh
term=xterm-256color
kind=1
scrollback=10000
cursor=0
closeonexit=$closeonexit
confirm=$confirm
EOF
  }

fail=0
note(){
  echo "FAIL: $*" >&2
  fail=1
  }

# --- Phase 1: the default policy closes the tab and the last window ---------
echo "==> phase 1: closeOnExit=true (default)"
seed_profile "$RUN/home-close" true true
start_app "$RUN/home-close" --execute /bin/sh -c "printf 'one\n'; exit 0"
if wait_exit 100; then
  echo "    the process ended after the child exited (status 0)"
else
  fail=1
  echo "FAIL: the process stayed up with closeonexit=true" >&2
  fi

# --- Phase 2: closeonexit=false keeps the final screen ---------------------
echo "==> phase 2: closeOnExit=false"
seed_profile "$RUN/home-keep" false true
start_app "$RUN/home-keep" --execute /bin/sh -c "printf 'kept\n'; exit 3"
if wait_title "FoxTerm - Terminal \[exited\]" 80; then
  echo "    the tab stayed and its title gained [exited]"
else
  fail=1
  echo "FAIL: the tab did not stay with an [exited] title" >&2
  titles >&2
  fi
if alive; then
  echo "    the process is still up"
else
  fail=1
  echo "FAIL: the process ended with closeonexit=false" >&2
  fi

# A termination signal must quit at once, without a confirmation dialog
echo "==> phase 3: SIGINT quits a kept tab without a dialog"
kill -INT "$app_pid" 2>/dev/null
if wait_exit 80; then
  echo "    the process quit on the signal"
else
  fail=1
  echo "FAIL: SIGINT left the process running" >&2
  fi

# --- Phase 4: confirmClose on a live session ------------------------------
echo "==> phase 4: confirmClose=true with a live child"
seed_profile "$RUN/home-confirm" true true
start_app "$RUN/home-confirm" --execute /bin/sh -c "printf 'live\n'; sleep 300"
if wait_title "FoxTerm - Terminal" 80; then
  :
else
  fail=1
  echo "FAIL: no terminal window appeared" >&2
  titles >&2
  fi
swaymsg '[title="FoxTerm - Terminal"] focus' >/dev/null 2>&1
sleep 0.4

echo "    Ctrl+Shift+W on a running session"
wtype -s 250 -M ctrl -M shift -k w -m shift -m ctrl
if wait_title "Close Tab" 60; then
  echo "    the confirmation dialog appeared"
else
  fail=1
  echo "FAIL: no confirmation dialog for a running session" >&2
  titles >&2
  fi

echo "    Escape cancels the close"
wtype -s 250 -k Escape
if wait_no_title "Close Tab" 60; then
  echo "    the dialog closed"
else
  fail=1
  echo "FAIL: the confirmation dialog did not close" >&2
  fi
if alive && titles | grep -q "FoxTerm - Terminal"; then
  echo "    the session is still open"
else
  fail=1
  echo "FAIL: Cancel closed the tab anyway" >&2
  titles >&2
  fi

echo "    Ctrl+Shift+W and OK closes the last tab"
wtype -s 250 -M ctrl -M shift -k w -m shift -m ctrl
if wait_title "Close Tab" 60; then
  wtype -s 250 -k Return
else
  fail=1
  echo "FAIL: the second confirmation did not appear" >&2
  fi
if wait_exit 100; then
  echo "    the tab closed and the last window ended the process"
else
  fail=1
  echo "FAIL: closing the last tab left the process running" >&2
  fi

# --- Phase 5: ordinary keys still reach the child -------------------------
# The window accelerators are consulted before the child sees a key; a plain
# typing key has no accelerator and must still arrive at the session.
echo "==> phase 5: typed text reaches the child"
seed_profile "$RUN/home-typing" true true
rm -f -- "$RUN/typed.txt"
export FOXTERM_GATE_OUT="$RUN/typed.txt"
start_app "$RUN/home-typing" --execute /bin/sh -c 'read line; printf "%s\n" "$line" > "$FOXTERM_GATE_OUT"'
if wait_title "FoxTerm - Terminal" 80; then
  :
else
  fail=1
  echo "FAIL: no terminal window for the typing phase" >&2
  fi
swaymsg '[title="FoxTerm - Terminal"] focus' >/dev/null 2>&1
sleep 0.4
wtype -s 120 'hello-terminal'
sleep 0.3
wtype -s 120 -k Return
for i in $(seq 1 60); do
  [ -s "$RUN/typed.txt" ] && break
  sleep 0.1
  done
if [ "$(cat "$RUN/typed.txt" 2>/dev/null)" = "hello-terminal" ]; then
  echo "    the child received the typed line"
else
  fail=1
  echo "FAIL: the child did not receive the typed line" >&2
  fi
if wait_exit 100; then
  echo "    the child exited and the process ended"
else
  fail=1
  echo "FAIL: the process stayed up after the child exited" >&2
  fi

# --- Phase 6: a detached window closes without ending the application -----
# Closing the last tab of one window must delete that window only.  The menu
# panes of a deleted window used to outlive it, so the next update pass walked
# into their dangling targets and crashed.
echo "==> phase 6: closing the last tab of a detached window"
seed_profile "$RUN/home-detach" true true
start_app "$RUN/home-detach" --execute /bin/sh -c "printf 'split\n'; sleep 300"
if wait_title "FoxTerm - Terminal" 80; then
  :
else
  fail=1
  echo "FAIL: no terminal window for the detach phase" >&2
  fi
swaymsg '[title="FoxTerm - Terminal"] focus' >/dev/null 2>&1
sleep 0.4

echo "    Ctrl+Shift+T adds a tab"
wtype -s 250 -M ctrl -M shift -k t -m shift -m ctrl
sleep 0.8
echo "    Ctrl+Shift+M detaches it"
wtype -s 250 -M ctrl -M shift -k m -m shift -m ctrl
sleep 1.2
if [ "$(window_count)" = 2 ]; then
  echo "    the detached tab opened a second window"
else
  fail=1
  echo "FAIL: expected two windows, found $(window_count)" >&2
  fi

echo "    closing the tab of the detached window"
wtype -s 250 -M ctrl -M shift -k w -m shift -m ctrl
if wait_title "Close Tab" 60; then
  wtype -s 250 -k Return
else
  fail=1
  echo "FAIL: no confirmation in the detached window" >&2
  fi
sleep 1.2
if alive; then
  echo "    the process is still up"
else
  fail=1
  echo "FAIL: closing one window ended the application" >&2
  fi
if [ "$(window_count)" = 1 ]; then
  echo "    only the detached window closed"
else
  fail=1
  echo "FAIL: expected one window left, found $(window_count)" >&2
  fi

echo "    Ctrl+Q quits the remaining window"
wtype -s 250 -M ctrl -k q -m ctrl
if wait_title "Quit FoxTerm" 60; then
  wtype -s 250 -k Return
else
  fail=1
  echo "FAIL: no quit confirmation" >&2
  fi
if wait_exit 100; then
  echo "    the process quit"
else
  fail=1
  echo "FAIL: the process did not quit" >&2
  fi

# --- Phase 7: a window keeps at least one tab -----------------------------
# Detaching the only tab of a window used to move the session to a new window
# and delete the old one, and the moved view kept reporting to the window that
# was gone, so the next right click or title change crashed.  Detach is now
# refused (and disabled in both menus) while the window has a single tab.
echo "==> phase 7: the last tab cannot be detached"
rm -f -- "$RUN/typed.txt"
export FOXTERM_GATE_OUT="$RUN/typed.txt"
start_app "$RUN/home-keep" --execute /bin/sh -c 'read line; sleep 300'
if wait_title "FoxTerm - Terminal" 80; then
  :
else
  fail=1
  echo "FAIL: no terminal window for the detach phase" >&2
  fi
swaymsg '[title="FoxTerm - Terminal"] focus' >/dev/null 2>&1
sleep 0.4
before=$(window_count)
wtype -s 250 -M ctrl -M shift -k m -m shift -m ctrl
sleep 1.2
wtype -s 250 -M ctrl -M shift -k m -m shift -m ctrl
sleep 1.2
if [ "$(window_count)" = "$before" ]; then
  echo "    Ctrl+Shift+M on the last tab leaves one window"
else
  fail=1
  echo "FAIL: detaching the last tab produced $(window_count) windows" >&2
  fi
if alive; then
  echo "    the process is still up"
else
  fail=1
  echo "FAIL: the process ended on the refused detach" >&2
  fi

echo "    Ctrl+Q quits it"
wtype -s 250 -M ctrl -k q -m ctrl
if wait_title "Quit FoxTerm" 60; then
  wtype -s 250 -k Return
else
  fail=1
  echo "FAIL: no quit confirmation for the kept window" >&2
  fi
if wait_exit 100; then
  echo "    the process quit"
else
  fail=1
  echo "FAIL: the process did not quit after the detach phase" >&2
  fi

if [ "$fail" -ne 0 ]; then
  echo "FAIL: see $RUN/app.log" >&2
  exit 1
  fi
echo "PASS: close-on-exit and close confirmation act as documented"
exit 0
