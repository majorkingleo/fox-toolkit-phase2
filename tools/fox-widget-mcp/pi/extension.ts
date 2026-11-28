// pi package entry point: register the MCP server for every session.
//
// This is the "install as a pi package" path:
//   pi install /home/martin/programs/fox2/tools/fox-widget-mcp
//
// It does the same thing as the mcp.json entry created by
//   pi mcp add fox-widgets -- node <this dir>/server.mjs
// but keeps the configuration inside the checkout instead of ~/.pi/agent.
//
// A server configured in mcp.json with the same name takes precedence.

import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import type { ExtensionAPI } from "@earendil-works/pi-coding-agent";

const here = dirname(fileURLToPath(import.meta.url));
const packageRoot = resolve(here, "..");
// tools/fox-widget-mcp -> tools -> <repo root>
const repoRoot = resolve(packageRoot, "..", "..");

export default function (pi: ExtensionAPI) {
  pi.registerMcpServer("fox-widgets", {
    command: process.execPath,
    args: [join(packageRoot, "server.mjs")],
    env: { FOX_MCP_ROOT: repoRoot },
    description:
      "FOX 1.7.95 and FOX 2 widget oracle: class hierarchy, headers, implementations, theme coverage, fox1/fox2 diffs",
    exposure: "codemode",
  });
}
