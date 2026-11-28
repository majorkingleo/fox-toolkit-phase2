import assert from "node:assert/strict";
import { after, before, describe, test } from "node:test";
import { connect, textOf } from "./client.mjs";

let server;

before(async () => {
  server = await connect();
});

after(() => {
  server?.close();
});

const call = async (name, args = {}) => textOf(await server.request("tools/call", { name, arguments: args }));

describe("protocol", () => {
  test("initialize negotiates a supported protocol and advertises tools", () => {
    assert.equal(server.init.protocolVersion, "2025-11-25");
    assert.equal(server.init.serverInfo.name, "fox-widgets");
    assert.match(server.init.instructions, /fox1/);
    assert.ok(server.init.capabilities.tools);
    assert.ok(server.init.capabilities.resources);
  });

  test("tools/list exposes the documented tools with read-only annotations", async () => {
    const { tools } = await server.request("tools/list");
    const names = tools.map((tool) => tool.name).sort();
    assert.deepEqual(names, [
      "diff_widget",
      "grep_source",
      "list_widgets",
      "read_source",
      "theme_status",
      "widget_hierarchy",
      "widget_info",
      "widget_source",
    ]);
    for (const tool of tools) {
      assert.equal(tool.annotations.readOnlyHint, true, tool.name);
      assert.equal(tool.inputSchema.type, "object");
      assert.ok(tool.description.length > 40, tool.name);
    }
  });

  test("unknown methods and tools fail cleanly", async () => {
    await assert.rejects(server.request("nope/nothing"), /method not found/);
    const text = await call("no_such_tool");
    assert.match(text, /Unknown tool/);
  });
});

describe("catalog", () => {
  test("list_widgets lists both trees", async () => {
    const text = await call("list_widgets", { limit: 400 });
    assert.match(text, /fox1 — FOX 1\.7\.95/);
    assert.match(text, /fox2 — FOX 2\.0/);
    assert.match(text, /FXButton/);
    assert.match(text, /FXScrollBar/);
  });

  test("list_widgets filters by name, base and theme state", async () => {
    const byName = await call("list_widgets", { variant: "fox2", name: "scroll" });
    assert.match(byName, /FXScrollBar/);
    assert.match(byName, /FXScrollArea/);
    assert.doesNotMatch(byName, /FXButton/);

    const byBase = await call("list_widgets", { variant: "fox2", base: "FXScrollArea" });
    assert.match(byBase, /FXScrollWindow/);
    assert.doesNotMatch(byBase, /FXButton/);

    const open = await call("list_widgets", { variant: "fox2", themeState: "OPEN" });
    assert.match(open, /FXDial|FXKnob/);
    assert.doesNotMatch(open, /DONE\//);
  });

  test("widget_info reports bases, API, implementation and theme state", async () => {
    const text = await call("widget_info", { name: "FXScrollBar" });
    assert.match(text, /# FXScrollBar \(fox2/);
    assert.match(text, /inherits: FXScrollBar -> FXWindow -> FXDrawable -> FXId -> FXObject/);
    assert.match(text, /implementation: fox2\/lib\/FXScrollBar\.cpp/);
    assert.match(text, /theme: PARTIAL/);
    assert.match(text, /subclasses: /);
    assert.match(text, /public API/);
  });

  test("widget_info explains an unknown class", async () => {
    const text = await call("widget_info", { name: "FXNoSuchWidget" });
    assert.match(text, /No class named FXNoSuchWidget/);
  });

  test("widget_hierarchy renders the FXWindow tree", async () => {
    const text = await call("widget_hierarchy", { root: "FXWindow", maxDepth: 2 });
    assert.match(text, /^# fox2 .*class hierarchy under FXWindow/m);
    assert.match(text, /├── FXComposite|└── FXComposite/);
    assert.match(text, /FXShell/);
  });

  test("theme_status returns checklist rows and groups", async () => {
    const all = await call("theme_status", { limit: 500 });
    assert.match(all, /DONE=\d+/);
    assert.match(all, /PARTIAL=\d+/);
    assert.match(all, /OPEN=\d+/);

    const p0 = await call("theme_status", { group: "P0" });
    assert.match(p0, /FXScrollBar/);

    const done = await call("theme_status", { state: "DONE", widget: "FXButton" });
    assert.match(done, /DONE\s+covered\s+FXButton/);
  });
});

describe("source access", () => {
  test("widget_source prints the class declaration", async () => {
    const text = await call("widget_source", { name: "FXButton", part: "header" });
    assert.match(text, /--- fox2\/include\/FXButton\.h:\d+-\d+ \(class FXButton/);
    assert.match(text, /class FXAPI FXButton : public FXLabel \{/);
    assert.match(text, /^\s*70 {2}class/m);
  });

  test("widget_source prints implementation and names the others", async () => {
    const text = await call("widget_source", { name: "FXWindow", part: "impl", maxLines: 40 });
    assert.match(text, /--- fox2\/lib\/FXWindow\.cpp/);
    assert.match(text, /other implementation files: .*FXWindow_x11\.cpp/);
  });

  test("read_source accepts variant-prefixed paths and ranges", async () => {
    const text = await call("read_source", { path: "fox1:include/FXButton.h", startLine: 70, maxLines: 3 });
    assert.match(text, /fox-1\.7\.95\/include\/FXButton\.h \(lines 70-72 of \d+\)/);
    assert.match(text, /70 {2}class FXAPI FXButton/);
  });

  test("read_source refuses to escape the repository", async () => {
    const text = await call("read_source", { path: "../../etc/passwd" });
    assert.match(text, /no such source file|escapes the repository/);
  });

  test("grep_source searches both trees", async () => {
    const text = await call("grep_source", { pattern: "ROLE_BUTTON", scope: "lib", maxResults: 5 });
    assert.match(text, /fox2\/lib\//);
    assert.doesNotMatch(text, /fox-1\.7\.95/);
  });

  test("grep_source filesOnly counts files", async () => {
    const text = await call("grep_source", { pattern: "applyTheme", scope: "lib", filesOnly: true });
    assert.match(text, /\d+ file\(s\) match \/applyTheme\//);
    assert.match(text, /fox2\/lib\//);
  });

  test("diff_widget shows what fox2 changed", async () => {
    const text = await call("diff_widget", { name: "FXButton", part: "header", maxLines: 60 });
    assert.match(text, /# FXButton — header diff, fox1 -> fox2 \(\+\d+\/-\d+ lines\)/);
    assert.match(text, /^--- fox-1\.7\.95\/include\/FXButton\.h$/m);
    assert.match(text, /^\+\+\+ fox2\/include\/FXButton\.h$/m);
  });

  test("resources expose the catalog and the checklist", async () => {
    const { resources } = await server.request("resources/list");
    assert.deepEqual(
      resources.map((resource) => resource.uri).sort(),
      ["fox://theme-checklist", "fox://widgets"],
    );

    const catalog = await server.request("resources/read", { uri: "fox://widgets" });
    const payload = JSON.parse(catalog.contents[0].text);
    assert.equal(payload.variants.length, 2);
    assert.ok(payload.variants.every((variant) => variant.widgets > 50));

    const checklist = await server.request("resources/read", { uri: "fox://theme-checklist" });
    assert.match(checklist.contents[0].text, /Widget theming checklist/);
  });
});
