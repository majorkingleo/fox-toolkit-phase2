# fox-widget-mcp

A local, read-only [MCP](https://modelcontextprotocol.io) server that gives a
coding agent (pi, Claude Code, Cursor, …) knowledge about the two FOX trees in
this repository:

| variant | tree | what it is |
| --- | --- | --- |
| `fox1` | `fox-1.7.95/` | pristine FOX 1.7.95 baseline, classic palette |
| `fox2` | `fox2/` | the themed FOX 2.0 experiment in this repo |

It answers the questions widget work actually asks: *what is the base class,
what is the public API, where is it implemented, is it themed, what did the
theme change, where does this colour/role get used?*

All tools are read-only source lookups (`readOnlyHint: true`). No build step,
no dependencies — plain Node.js ≥ 20 over stdio.

## Tools

| tool | purpose |
| --- | --- |
| `list_widgets` | all `FXWindow` descendants of fox1/fox2, filter by name, base class or theme state |
| `widget_info` | header location, inheritance chain, subclasses, public API, implementation files, theme state + note, docs mentions, usages |
| `widget_source` | the class declaration and/or `lib/*.cpp` implementation, with line numbers |
| `read_source` | any repo file by line range (`fox2:lib/FXButton.cpp`) |
| `grep_source` | regex search over headers/lib/apps/tests/utils/docs of both trees |
| `diff_widget` | unified diff of a widget's header or implementation, fox1 → fox2 |
| `theme_status` | the `docs/theme-widget-checklist.md` table: DONE/PARTIAL/OPEN by priority |
| `widget_hierarchy` | inheritance tree rooted at `FXWindow` (or any class) |

Resources: `fox://widgets` (catalog JSON) and `fox://theme-checklist`.

## Install

There is no packaging convention beyond what pi already standardises, and pi
supports two of them. Use whichever fits:

### 1. `mcp.json` entry (standard for any MCP client)

```sh
pi mcp add fox-widgets -- node /home/martin/programs/fox2/tools/fox-widget-mcp/server.mjs
```

That writes a user-level entry to `~/.pi/agent/mcp.json`:

```json
{
  "mcpServers": {
    "fox-widgets": {
      "command": "node",
      "args": ["/home/martin/programs/fox2/tools/fox-widget-mcp/server.mjs"],
      "description": "FOX 1.7.95 and FOX 2 widget oracle: class hierarchy, headers, implementations, theme coverage, diffs"
    }
  }
}
```

Add `-l` to write `.pi/mcp.json` for this project instead (read only in a
trusted project). Check it with `pi mcp list`, inspect with `/mcp` inside a
session, `/reload` after editing the file by hand.

The same JSON block works unchanged in Claude Code, Cursor and other clients.

### 2. pi package (self-registering)

`package.json` declares `pi.extensions`, and `pi/extension.ts` calls
`pi.registerMcpServer()` on load:

```sh
pi install /home/martin/programs/fox2/tools/fox-widget-mcp
```

Now the server appears in every session without an `mcp.json` entry, and the
registration is not saved anywhere outside the checkout. A `mcp.json` server of
the same name wins, so both paths can coexist.

Removal: `pi mcp remove fox-widgets` or `pi remove /home/…/fox-widget-mcp`.

## Configuration

| variable | meaning |
| --- | --- |
| `FOX_MCP_ROOT` | repository root to index. Default: the repo this file lives in. Set it to reuse the server for another FOX checkout. |
| `FOX_MCP_DEBUG` | `1` logs every JSON-RPC line to stderr (`~/.pi/agent/mcp.log` when run by pi) |

Server name: `fox-widgets`; tools appear as `mcp__fox_widgets__<tool>`. Exposure
defaults to `codemode`; set `"exposure": "direct"` in `mcp.json` if the model
should see the tools without codemode/tool_search.

## Development

```sh
cd /home/martin/programs/fox2/tools/fox-widget-mcp
node server.mjs --list-tools     # tool inventory
node server.mjs --help
node --test test/                # end-to-end stdio tests
```

Layout:

```
server.mjs         JSON-RPC/stdio transport, tools/*, resources/*
src/catalog.mjs    parses include/*.h, lib/*.cpp, docs/theme-widget-checklist.md
src/tools.mjs      tool schemas and handlers
src/util.mjs       fs/text/diff helpers
pi/extension.ts    optional pi package entry (registerMcpServer)
test/              spawn-based smoke tests
```

The index is built lazily on the first tool call and cached per process, so
after changing sources in `fox2/`, restart the server (`/mcp reconnect
fox-widgets` or `/reload`).
