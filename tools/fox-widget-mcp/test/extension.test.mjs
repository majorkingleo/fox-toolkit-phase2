// Checks the optional pi-package entry point: it must register one server whose
// command really exists and whose env points at a repository root that has both
// FOX trees. Needs Node's TypeScript type stripping (Node >= 22.6).

import assert from "node:assert/strict";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { test } from "node:test";
import { isDir, isFile } from "../src/util.mjs";

const here = path.dirname(fileURLToPath(import.meta.url));

test("pi/extension.ts registers the fox-widgets server with valid paths", async (t) => {
  let extension;
  try {
    extension = (await import("../pi/extension.ts")).default;
  } catch (error) {
    t.skip(`TypeScript type stripping unavailable: ${error.code ?? error.message}`);
    return;
  }
  assert.equal(typeof extension, "function");

  const registrations = [];
  extension({ registerMcpServer: (name, config) => registrations.push({ name, config }) });

  assert.equal(registrations.length, 1);
  const [{ name, config }] = registrations;
  assert.equal(name, "fox-widgets");
  assert.equal(config.exposure, "codemode");
  assert.ok(config.description.length > 20);
  assert.ok(isFile(config.command), `command not executable: ${config.command}`);
  assert.equal(config.args.length, 1);
  assert.ok(isFile(config.args[0]), `server script missing: ${config.args[0]}`);
  assert.ok(isDir(path.join(config.env.FOX_MCP_ROOT, "fox-1.7.95")));
  assert.ok(isDir(path.join(config.env.FOX_MCP_ROOT, "fox2")));
  assert.ok(config.args[0].startsWith(path.resolve(here, "..")), "server script should come from this package");
});
