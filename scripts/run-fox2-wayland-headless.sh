#!/usr/bin/env bash
#
# Run a command against a private headless Weston compositor.
#
# The Wayland gate of docs/wayland-plan.md must not depend on the desktop
# session, so this script starts weston with the headless backend on a private
# socket in its own runtime directory, runs the command with that socket in
# WAYLAND_DISPLAY, and stops the compositor again.
#
#   ./scripts/run-fox2-wayland-headless.sh wayland-info
#   ./scripts/run-fox2-wayland-headless.sh --run-dir=tmp/x tmp/build-fox2-wayland/bin/hello
#
# Options:
#   --run-dir=DIR  socket and log directory (default: tmp/wayland-headless)
#   --socket=NAME  compositor socket name (default: wayland-fox)
#   --keep         leave the compositor running after the command
#   -h|--help      show this help
#
# Exit status is the exit status of the command, or 2 when the compositor could
# not be started.
#
set -euo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
RUNDIR="$ROOT/tmp/wayland-headless"
SOCKET="wayland-fox"
keep=no

while [ $# -gt 0 ]; do
  case "$1" in
    --run-dir)   shift; RUNDIR=$1 ;;
    --run-dir=*) RUNDIR=${1#*=} ;;
    --socket)    shift; SOCKET=$1 ;;
    --socket=*)  SOCKET=${1#*=} ;;
    --keep)      keep=yes ;;
    -h|--help)   sed -n '2,20p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit 0 ;;
    --)          shift; break ;;
    *)           break ;;
    esac
  shift
  done

if [ $# -eq 0 ]; then
  echo "error: no command given" >&2
  sed -n '2,20p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//' >&2
  exit 2
  fi

if ! command -v weston >/dev/null 2>&1; then
  echo "error: weston not found; install weston (headless compositor)" >&2
  exit 2
  fi

mkdir -p -- "$RUNDIR"
RUNDIR=$(cd -- "$RUNDIR" && pwd)
chmod 700 "$RUNDIR"
log="$RUNDIR/weston.log"
pidfile="$RUNDIR/weston.pid"

# A private runtime directory keeps the desktop session out of this
export XDG_RUNTIME_DIR="$RUNDIR"
unset DISPLAY WAYLAND_DISPLAY

if [ -S "$RUNDIR/$SOCKET" ]; then
  echo "error: '$RUNDIR/$SOCKET' already exists; run with --socket=NAME or remove it" >&2
  exit 2
  fi

echo "==> starting headless weston on $SOCKET"
weston --backend=headless-backend.so --socket="$SOCKET" --no-config --idle-time=0 --log="$log" &
weston_pid=$!
echo "$weston_pid" >"$pidfile"

cleanup(){
  if [ "$keep" = no ] && kill -0 "$weston_pid" 2>/dev/null; then
    kill "$weston_pid" 2>/dev/null || true
    wait "$weston_pid" 2>/dev/null || true
    fi
  }
trap cleanup EXIT INT TERM

# Wait for the socket
for i in $(seq 1 100); do
  [ -S "$RUNDIR/$SOCKET" ] && break
  kill -0 "$weston_pid" 2>/dev/null || { echo "error: weston exited; see $log" >&2; tail -n 20 "$log" >&2; exit 2; }
  sleep 0.05
  done
if [ ! -S "$RUNDIR/$SOCKET" ]; then
  echo "error: weston did not create $RUNDIR/$SOCKET; see $log" >&2
  exit 2
  fi

echo "==> WAYLAND_DISPLAY=$SOCKET (XDG_RUNTIME_DIR=$RUNDIR)"
export WAYLAND_DISPLAY="$SOCKET"

rc=0
"$@" || rc=$?
echo "==> command exited with $rc"
exit "$rc"
