#!/usr/bin/env node
// fox-widget-mcp — a local read-only MCP server for the FOX toolkit sources
// in this repository: the FOX 1.7.95 baseline and the themed FOX 2 tree.
//
// Transport: stdio, one JSON-RPC 2.0 message per line (MCP 2025-11-25).
// No runtime dependencies; run it with `node server.mjs`.
//
// Environment:
//   FOX_MCP_ROOT  repository root to index (default: the repo this file lives in)
//   FOX_MCP_DEBUG 1 to log every request to stderr

import path from "node:path";
import { fileURLToPath } from "node:url";
import { readTextSafe, truncate } from "./src/util.mjs";
import { createCatalog, VARIANTS, VARIANT_IDS } from "./src/catalog.mjs";
import { createTools } from "./src/tools.mjs";

const SUPPORTED_PROTOCOLS = ["2025-11-25", "2025-06-18", "2025-03-26", "2024-11-05"];
const SERVER_INFO = { name: "fox-widgets", title: "FOX widget oracle", version: "0.1.0" };
const MAX_RESULT_CHARS = 18_000;

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(process.env.FOX_MCP_ROOT ?? path.join(here, "..", ".."));
const debug = process.env.FOX_MCP_DEBUG === "1";

const catalog = createCatalog(root);
const tools = new Map(createTools(catalog).map((tool) => [tool.name, tool]));

const INSTRUCTIONS = [
  "FOX toolkit source oracle for the local checkout at " + root + ".",
  "Two variants: fox1 = pristine FOX 1.7.95 baseline (fox-1.7.95/), fox2 = the themed FOX 2 experiment (fox2/).",
  "Workflow: list_widgets to find a class, widget_info for base class/API/implementation/theme status,",
  "widget_source or read_source to read the code, grep_source to search both trees,",
  "diff_widget to see exactly what the theming changed, theme_status for the widget theming checklist.",
  "Every tool is read-only. Prefer widget_info/diff_widget over grepping by hand.",
].join(" ");

if (process.argv.includes("--help") || process.argv.includes("-h")) {
  process.stdout.write(
    `fox-widget-mcp — read-only MCP server for the FOX sources in ${root}\n\n` +
      `Usage: node server.mjs [--help] [--list-tools]\n\n` +
      `Environment:\n  FOX_MCP_ROOT   repository root (default ${root})\n  FOX_MCP_DEBUG  1 = log JSON-RPC traffic to stderr\n\n` +
      `Register with pi:\n  pi mcp add fox-widgets -- node ${path.join(here, "server.mjs")}\n`,
  );
  process.exit(0);
}

if (process.argv.includes("--list-tools")) {
  for (const tool of tools.values()) {
    process.stdout.write(`${tool.name}\n    ${tool.description}\n`);
  }
  process.exit(0);
}

/* ------------------------------------------------------------- transport */

let buffer = "";
process.stdin.setEncoding("utf8");
process.stdin.on("data", (chunk) => {
  buffer += chunk;
  let newline;
  while ((newline = buffer.indexOf("\n")) >= 0) {
    const line = buffer.slice(0, newline).trim();
    buffer = buffer.slice(newline + 1);
    if (!line) continue;
    handleLine(line);
  }
});
process.stdin.on("end", () => process.exit(0));
process.on("SIGTERM", () => process.exit(0));
process.on("SIGINT", () => process.exit(0));
process.on("uncaughtException", (error) => log(`uncaught: ${error?.stack ?? error}`));

function send(message) {
  if (debug) log(`-> ${JSON.stringify(message).slice(0, 400)}`);
  process.stdout.write(`${JSON.stringify(message)}\n`);
}

function reply(id, result) {
  send({ jsonrpc: "2.0", id, result });
}

function fail(id, code, message, data) {
  send({ jsonrpc: "2.0", id: id ?? null, error: { code, message, ...(data === undefined ? {} : { data }) } });
}

function log(text) {
  process.stderr.write(`[fox-widgets] ${text}\n`);
}

async function handleLine(line) {
  let message;
  try {
    message = JSON.parse(line);
  } catch (error) {
    fail(null, -32700, `parse error: ${error.message}`);
    return;
  }
  if (debug) log(`<- ${line.slice(0, 400)}`);

  if (Array.isArray(message)) {
    fail(null, -32600, "batch requests are not supported by this server");
    return;
  }
  if (!message || typeof message !== "object" || message.jsonrpc !== "2.0") {
    fail(message?.id, -32600, "invalid JSON-RPC 2.0 message");
    return;
  }

  const isNotification = message.id === undefined || message.id === null;
  if (isNotification) return; // initialized, cancelled, progress, ... — nothing to do

  const { id, method, params } = message;
  try {
    switch (method) {
      case "initialize":
        return reply(id, initializeResult(params));
      case "ping":
        return reply(id, {});
      case "tools/list":
        return reply(id, { tools: [...tools.values()].map(publicTool) });
      case "tools/call":
        return reply(id, await callTool(params));
      case "resources/list":
        return reply(id, { resources: listResources() });
      case "resources/read":
        return reply(id, readResource(params));
      case "prompts/list":
        return reply(id, { prompts: [] });
      case "logging/setLevel":
        return reply(id, {});
      default:
        return fail(id, -32601, `method not found: ${method}`);
    }
  } catch (error) {
    log(`error in ${method}: ${error?.stack ?? error}`);
    fail(id, -32603, `internal error: ${error?.message ?? error}`);
  }
}

function initializeResult(params) {
  const requested = params?.protocolVersion;
  const protocolVersion = SUPPORTED_PROTOCOLS.includes(requested) ? requested : SUPPORTED_PROTOCOLS[0];
  return {
    protocolVersion,
    capabilities: {
      tools: { listChanged: false },
      resources: { subscribe: false, listChanged: false },
      prompts: {},
    },
    serverInfo: SERVER_INFO,
    instructions: INSTRUCTIONS,
  };
}

/* ----------------------------------------------------------------- tools */

function publicTool(tool) {
  return {
    name: tool.name,
    title: tool.title,
    description: tool.description,
    inputSchema: tool.inputSchema,
    annotations: tool.annotations,
  };
}

async function callTool(params) {
  const name = params?.name;
  const tool = tools.get(name);
  if (!tool) {
    return { content: [{ type: "text", text: `Unknown tool "${name}". Available: ${[...tools.keys()].join(", ")}` }], isError: true };
  }
  const args = params?.arguments ?? {};
  if (typeof args !== "object" || Array.isArray(args)) {
    return { content: [{ type: "text", text: "arguments must be an object" }], isError: true };
  }
  let result;
  try {
    result = await tool.handler(args);
  } catch (error) {
    return { content: [{ type: "text", text: `${tool.name}: ${error?.message ?? error}` }], isError: true };
  }
  const content = (result?.content ?? []).map((item) =>
    item.type === "text" ? { ...item, text: truncate(item.text ?? "", MAX_RESULT_CHARS) } : item,
  );
  return { content, ...(result?.isError ? { isError: true } : {}) };
}

/* ------------------------------------------------------------- resources */

function listResources() {
  return [
    {
      uri: "fox://widgets",
      name: "FOX widget catalog",
      description: "Widget counts, versions and tree locations for the indexed FOX variants.",
      mimeType: "application/json",
    },
    {
      uri: "fox://theme-checklist",
      name: "FOX 2 theme widget checklist",
      description: "The themed-widget status table (DONE/PARTIAL/OPEN) from docs/theme-widget-checklist.md.",
      mimeType: "text/markdown",
    },
  ];
}

function readResource(params) {
  const uri = params?.uri;
  if (uri === "fox://widgets") {
    const variants = VARIANT_IDS.filter((id) => catalog.hasVariant(id)).map((id) => {
      const index = catalog.index(id);
      return {
        id,
        label: VARIANTS[id].label,
        version: index.version,
        dir: VARIANTS[id].dir,
        widgets: catalog.widgets(id).length,
        classes: index.classes.size,
      };
    });
    const body = JSON.stringify({ root, variants }, null, 2);
    return { contents: [{ uri, mimeType: "application/json", text: body }] };
  }
  if (uri === "fox://theme-checklist") {
    const file = path.join(root, "docs", "theme-widget-checklist.md");
    const text = readTextSafe(file);
    if (text == null) throw new Error(`missing ${file}`);
    return { contents: [{ uri, mimeType: "text/markdown", text }] };
  }
  throw new Error(`unknown resource ${uri}`);
}

if (debug) log(`serving ${root} with ${tools.size} tools`);
