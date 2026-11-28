# AGENTS.md — fox-widgets MCP server

Instructions for an AI coding agent that has to install, verify, or use this
MCP server. Humans can read it too; `README.md` is the long version.

**What it is:** a local, read-only MCP server (stdio, Node.js ≥ 20, no
dependencies) that answers questions about the two FOX sources in this checkout:
`fox-1.7.95/` (classic baseline) and `fox2/` (the themed FOX 2 tree). It never
writes files and never touches the network.

## 1. Install (one command)

```sh
/home/martin/programs/mcp/fox-toolkit-phase2/tools/fox-widget-mcp/install.sh
```

It checks node, registers the server with pi, and verifies both that the server
answers and that pi connects. Idempotent — running it again just refreshes the
command path.

| flag | effect |
| --- | --- |
| (none) | global: `~/.pi/agent/mcp.json`, available in every project |
| `--local` | project: `<repo>/.pi/mcp.json`, only inside this trusted project |
| `--root DIR` | index a different FOX checkout (writes `FOX_MCP_ROOT=DIR`) |
| `--print` | print the JSON config for another client, change nothing |
| `--uninstall` | remove the entry again |
| `--test` | also run the server's 18 tests |

No `pi` on the machine? Add the `--print` output to the client's MCP config by
hand; the block is standard stdio and works in Claude Code, Cursor, Codex, etc.

Manual equivalent of the default install:

```sh
pi mcp add fox-widgets -- node /home/martin/programs/mcp/fox-toolkit-phase2/tools/fox-widget-mcp/server.mjs
```

## 2. Verify

```sh
pi mcp list                      # expect: connected, 8 tools, offers resources
node .../tools/fox-widget-mcp/server.mjs --list-tools
node --test .../tools/fox-widget-mcp/test/
```

Inside a running pi session, `/mcp` shows the tools; `/mcp reconnect
fox-widgets` picks up a new server build, `/reload` re-reads config.

## 3. Use

Tool names reach the model as `mcp__fox_widgets__<tool>`. Exposure is the
default `codemode`, so call them from codemode scripts or find them with
`searchTools()`/`describeTool()`; set `"exposure": "direct"` in `mcp.json` if
they should be declared to the model directly.

| tool | use it when |
| --- | --- |
| `list_widgets` | you need the name of a widget. Filters: `name`, `base`, `themeState`, `variant` (`fox1`, `fox2`, `both`) |
| `widget_info` | first call for any widget: base class chain, subclasses, public API, implementation files, theme state, docs mentions, usages |
| `widget_source` | you want the class declaration (`part:"header"`) or `lib/*.cpp` (`part:"impl"`) |
| `read_source` | you know the file; reads a line range with line numbers. Paths: `fox2:lib/FXButton.cpp`, `fox-1.7.95/include/FXButton.h`, `tests/button.cpp` |
| `grep_source` | "where is this colour / role / macro used", or compare APIs across both trees |
| `diff_widget` | before editing a widget, to see what the theming already changed |
| `theme_status` | which widgets are DONE / PARTIAL / OPEN, grouped P0/P1/P2 |
| `widget_hierarchy` | you need the tree under `FXWindow` or any other base class |

Resources: `fox://widgets` (variant/version/widget counts), `fox://theme-checklist`
(the raw checklist).

Typical sequence for "make `FXKnob` themed":

1. `theme_status` with `widget:"FXKnob"` → state and what is missing.
2. `widget_info` `{name:"FXKnob"}` → base class (`FXFrame`), API, `lib/FXKnob.cpp`.
3. `diff_widget` `{name:"FXButton", part:"impl"}` → copy the pattern a finished
   widget uses (`applyTheme`, `FXStyle::drawButton()`).
4. `grep_source` `{pattern:"METRIC_CORNER_RADIUS", scope:"lib"}` → other users of
   the role you want.
5. `widget_source`/`read_source` on the exact lines, then edit the real files
   with the normal editing tools.

## 4. Configuration and layout

| variable | meaning |
| --- | --- |
| `FOX_MCP_ROOT` | repository root to index. Default: resolved from the server's own location, i.e. `…/fox-toolkit-phase2`. |
| `FOX_MCP_DEBUG` | `1` logs every JSON-RPC line to stderr (pi appends it to `~/.pi/agent/mcp.log`) |

```
tools/fox-widget-mcp/
  server.mjs        transport + tools/list + resources
  src/catalog.mjs   header/class parser, theme checklist parser, file index
  src/tools.mjs     the 8 tools
  src/util.mjs      fs/text/diff helpers
  pi/extension.ts   optional pi package entry (pi.registerMcpServer)
  install.sh        this installer
  AGENTS.md         this file
  test/             end-to-end stdio tests
```

## 5. Gotchas

- The source index is built lazily on the first tool call and cached per
  process. After editing `fox2/` files, `/mcp reconnect fox-widgets` (new
  process) — otherwise tool output is stale.
- The server is read-only by design. Do not add write tools; edits belong to the
  agent's normal file tools so they show up in the transcript.
- `theme_status` reads `docs/theme-widget-checklist.md`; if that file moves, the
  theme column degrades to `UNTESTED` (the rest keeps working).
- `diff_widget` shells out to `diff(1)`. Missing `diff` produces a clear error,
  not a crash.
- Do not commit `.pi/mcp.json` unless the project-scope install is intended; the
  global entry is the default and needs no file in the repo.
