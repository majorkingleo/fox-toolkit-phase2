#!/usr/bin/env bash
# Register the fox-widgets MCP server with pi (or print the config for any
# other MCP client).
#
#   ./install.sh                 global  -> ~/.pi/agent/mcp.json   (all projects)
#   ./install.sh --local         project -> <repo>/.pi/mcp.json    (this checkout only)
#   ./install.sh --print         only print the JSON block, change nothing
#   ./install.sh --uninstall     remove the entry again
#   ./install.sh --root DIR      index another FOX checkout (sets FOX_MCP_ROOT)
#   ./install.sh --test          also run the server's test suite
#
# Idempotent: an existing "fox-widgets" entry is replaced.

set -euo pipefail

SERVER="fox-widgets"
HERE="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd -- "$HERE/../.." && pwd)"
SCRIPT="$HERE/server.mjs"
NODE="$(command -v node || true)"

LOCAL=0
PRINT_ONLY=0
UNINSTALL=0
RUN_TESTS=0
ROOT=""

usage() {
  awk 'NR>1 && /^#/ {sub(/^# ?/,""); print; next} NR>1 {exit}' "${BASH_SOURCE[0]}"
}

while [ $# -gt 0 ]; do
  case "$1" in
    -l|--local) LOCAL=1 ;;
    --print|--dry-run) PRINT_ONLY=1 ;;
    --uninstall|--remove) UNINSTALL=1 ;;
    --test) RUN_TESTS=1 ;;
    --root)
      shift
      [ $# -gt 0 ] || { echo "error: --root needs a directory" >&2; exit 2; }
      ROOT="$(cd -- "$1" && pwd)"
      ;;
    -h|--help) usage; exit 0 ;;
    *) echo "error: unknown argument $1 (try --help)" >&2; exit 2 ;;
  esac
  shift
done

if [ -z "$NODE" ]; then
  echo "error: node not found in PATH (the server needs Node.js >= 20)" >&2
  exit 1
fi

NODE_MAJOR="$("$NODE" -p 'process.versions.node.split(".")[0]')"
if [ "$NODE_MAJOR" -lt 20 ]; then
  echo "error: node $("$NODE" -v) is too old (need >= 20)" >&2
  exit 1
fi

if [ ! -f "$SCRIPT" ]; then
  echo "error: $SCRIPT is missing — is this a full checkout?" >&2
  exit 1
fi
if [ ! -d "$REPO/fox2" ] || [ ! -d "$REPO/fox-1.7.95" ]; then
  echo "error: $REPO does not look like the fox-toolkit-phase2 checkout (fox2/ and fox-1.7.95/ are required)" >&2
  exit 1
fi

json_config() {
  cat <<JSON
{
  "mcpServers": {
    "$SERVER": {
      "command": "$NODE",
      "args": ["$SCRIPT"]$( [ -n "$ROOT" ] && printf ',\n      "env": { "FOX_MCP_ROOT": "%s" }' "$ROOT" )
    }
  }
}
JSON
}

if [ "$PRINT_ONLY" -eq 1 ]; then
  json_config
  exit 0
fi

if ! command -v pi >/dev/null 2>&1; then
  cat >&2 <<EOF
pi is not on PATH, so nothing was registered. Add this to your MCP client
configuration by hand (pi: ~/.pi/agent/mcp.json, or .pi/mcp.json in a project):

$(json_config)

The server is a plain stdio MCP server, so the same block works in Claude Code,
Cursor, Codex and other clients (stdio transport, no arguments besides the path).
EOF
  exit 1
fi

if [ "$UNINSTALL" -eq 1 ]; then
  if [ "$LOCAL" -eq 1 ]; then
    ( cd "$REPO" && pi mcp remove "$SERVER" -l )
    # pi leaves an empty {"mcpServers":{}} behind; drop it so the checkout stays clean.
    if [ -f "$REPO/.pi/mcp.json" ] && "$NODE" -e '
      const fs=require("fs");
      const file=process.argv[1];
      const data=JSON.parse(fs.readFileSync(file,"utf8"));
      process.exit(Object.keys(data.mcpServers??{}).length===0?0:1);
    ' "$REPO/.pi/mcp.json"; then
      rm -f "$REPO/.pi/mcp.json"
      echo "Removed the empty $REPO/.pi/mcp.json"
    fi
  else
    pi mcp remove "$SERVER"
  fi
  exit 0
fi

SCOPE_ARGS=()
SCOPE_LABEL="global (~/.pi/agent/mcp.json)"
if [ "$LOCAL" -eq 1 ]; then
  SCOPE_ARGS=(-l)
  SCOPE_LABEL="project ($REPO/.pi/mcp.json)"
fi

ENV_ARGS=()
if [ -n "$ROOT" ]; then
  ENV_ARGS=(--env "FOX_MCP_ROOT=$ROOT")
fi

# Replace an existing entry so the command path is always current.
if [ "$LOCAL" -eq 1 ]; then
  ( cd "$REPO" && pi mcp remove "$SERVER" ${SCOPE_ARGS[@]+"${SCOPE_ARGS[@]}"} ) >/dev/null 2>&1 || true
  ( cd "$REPO" && pi mcp add "$SERVER" ${SCOPE_ARGS[@]+"${SCOPE_ARGS[@]}"} ${ENV_ARGS[@]+"${ENV_ARGS[@]}"} -- "$NODE" "$SCRIPT" )
else
  pi mcp remove "$SERVER" >/dev/null 2>&1 || true
  pi mcp add "$SERVER" ${ENV_ARGS[@]+"${ENV_ARGS[@]}"} -- "$NODE" "$SCRIPT"
fi

echo
echo "Registered \"$SERVER\" as $SCOPE_LABEL:"
echo "  $NODE $SCRIPT"
if [ -n "$ROOT" ]; then
  echo "  FOX_MCP_ROOT=$ROOT"
else
  echo "  indexes $REPO (fox-1.7.95/ + fox2/)"
fi

echo
echo "Verifying..."
"$NODE" "$SCRIPT" --list-tools >/dev/null
echo "  server answers on stdio: ok"
if pi mcp list >/dev/null 2>&1; then
  echo "  pi connects: ok"
else
  echo "  pi connects: FAILED (run 'pi mcp list' for details)" >&2
  exit 1
fi

if [ "$RUN_TESTS" -eq 1 ]; then
  echo
  ( cd "$HERE" && "$NODE" --test test/ )
fi

cat <<EOF

Next:
  start a new pi session .................. the server is connected there
  existing session ........................ /mcp reconnect fox-widgets  (or /reload)
  inspect tools ........................... /mcp
  remove .................................. $HERE/install.sh --uninstall

Tool names reach the model as mcp__fox_widgets__<tool>; exposure is the default
"codemode". See AGENTS.md next to this script for the tool reference.
EOF
