// Tool definitions for the FOX widget MCP server.
// Every tool is read-only: these are source lookups, not code generation.

import { execFileSync } from "node:child_process";
import path from "node:path";
import {
  collapse,
  isDir,
  isFile,
  numbered,
  readTextSafe,
  sliceLines,
  splitLines,
  walkFiles,
} from "./util.mjs";
import { VARIANT_IDS, VARIANTS } from "./catalog.mjs";

const readOnly = Object.freeze({
  readOnlyHint: true,
  destructiveHint: false,
  idempotentHint: true,
  openWorldHint: false,
});

const VARIANTS_SCHEMA = { type: "string", enum: [...VARIANT_IDS, "both"] };
const SOURCE_EXT = /\.(h|hpp|cpp|cc|cxx|c|in)$/i;
const DOC_EXT = /\.(md|html|txt)$/i;

export function createTools(catalog) {
  return [
    {
      name: "list_widgets",
      title: "List FOX widgets",
      description:
        "List FOX widget classes (everything deriving from FXWindow) in the local FOX 1.7.95 baseline (variant fox1) and/or the themed FOX 2 tree (variant fox2). Use it to discover a widget name before calling widget_info.",
      inputSchema: {
        type: "object",
        properties: {
          variant: { ...VARIANTS_SCHEMA, default: "both", description: "Which source tree to list." },
          name: { type: "string", description: "Case-insensitive substring the class name must contain, e.g. \"scroll\"." },
          base: { type: "string", description: "Only widgets deriving from this class, e.g. FXScrollArea." },
          themeState: {
            type: "string",
            enum: ["DONE", "PARTIAL", "OPEN", "UNTESTED"],
            description: "Filter by theme state from docs/theme-widget-checklist.md (UNTESTED = not mentioned).",
          },
          limit: { type: "integer", minimum: 1, maximum: 2000, default: 300 },
        },
        additionalProperties: false,
      },
      annotations: readOnly,
      handler: (args) => listWidgets(catalog, args),
    },
    {
      name: "widget_info",
      title: "FOX widget facts",
      description:
        "Everything known about one FOX widget class: header location, base classes, subclasses, public API, implementation files, theme status from the checklist, docs mentions and where apps/tests use it. Accepts a plain class name such as FXButton (no .h).",
      inputSchema: {
        type: "object",
        properties: {
          name: { type: "string", description: "Widget class name, e.g. FXButton or FXScrollBar." },
          variant: { type: "string", enum: VARIANT_IDS, default: "fox2", description: "Variant that supplies API and implementation." },
          themeNotes: { type: "boolean", default: true, description: "Include the (long) checklist note." },
        },
        required: ["name"],
        additionalProperties: false,
      },
      annotations: readOnly,
      handler: (args) => widgetInfo(catalog, args),
    },
    {
      name: "widget_source",
      title: "Read a widget's header or implementation",
      description:
        "Print the class declaration from the header and/or the matching lib/*.cpp implementation of a widget, with line numbers. Use read_source afterwards to read a specific range of a large file.",
      inputSchema: {
        type: "object",
        properties: {
          name: { type: "string", description: "Widget class name, e.g. FXTable." },
          variant: { type: "string", enum: VARIANT_IDS, default: "fox2" },
          part: { type: "string", enum: ["header", "impl", "all"], default: "header" },
          maxLines: { type: "integer", minimum: 20, maximum: 2000, default: 300 },
        },
        required: ["name"],
        additionalProperties: false,
      },
      annotations: readOnly,
      handler: (args) => widgetSource(catalog, args),
    },
    {
      name: "read_source",
      title: "Read repository source",
      description:
        "Read any file of the FOX trees (headers, lib implementations, apps, tests, utils, docs) with line numbers by line range. Paths are repository-relative or variant-prefixed: fox2:lib/FXButton.cpp, fox1:include/FXButton.h.",
      inputSchema: {
        type: "object",
        properties: {
          path: { type: "string", description: "Path such as fox2:lib/FXButton.cpp, fox-1.7.95/include/FXButton.h or tests/button.cpp." },
          variant: { type: "string", enum: VARIANT_IDS, description: "Search hint when the path is ambiguous." },
          startLine: { type: "integer", minimum: 1, default: 1 },
          endLine: { type: "integer", minimum: 1 },
          maxLines: { type: "integer", minimum: 1, maximum: 2000, default: 200 },
        },
        required: ["path"],
        additionalProperties: false,
      },
      annotations: readOnly,
      handler: (args) => readSource(catalog, args),
    },
    {
      name: "grep_source",
      title: "Search both FOX trees",
      description:
        "Regular-expression search across the FOX 1 and FOX 2 sources (headers, lib, apps, tests, utils) and the project docs. Returns file:line hits. Use it to find every widget that paints a colour, uses a theme role, or to compare APIs across trees.",
      inputSchema: {
        type: "object",
        properties: {
          pattern: { type: "string", description: "JavaScript regular expression, or a literal string when fixed=true." },
          variant: { ...VARIANTS_SCHEMA, default: "both" },
          scope: {
            type: "string",
            enum: ["headers", "lib", "apps", "tests", "utils", "docs", "all"],
            default: "all",
          },
          fixed: { type: "boolean", default: false, description: "Treat pattern as a literal string." },
          ignoreCase: { type: "boolean", default: false },
          context: { type: "integer", minimum: 0, maximum: 6, default: 0, description: "Context lines around each hit." },
          maxResults: { type: "integer", minimum: 1, maximum: 500, default: 80 },
          filesOnly: { type: "boolean", default: false, description: "Only list matching files." },
        },
        required: ["pattern"],
        additionalProperties: false,
      },
      annotations: readOnly,
      handler: (args) => grepSource(catalog, args),
    },
    {
      name: "diff_widget",
      title: "Diff a widget between FOX 1 and FOX 2",
      description:
        "Unified diff of one widget's header or implementation between the FOX 1.7.95 baseline and the themed FOX 2 tree. This is how to see what theming changed and what the classic path still does.",
      inputSchema: {
        type: "object",
        properties: {
          name: { type: "string", description: "Widget class name, e.g. FXTabBar." },
          part: { type: "string", enum: ["header", "impl"], default: "impl" },
          context: { type: "integer", minimum: 0, maximum: 20, default: 3 },
          maxLines: { type: "integer", minimum: 20, maximum: 4000, default: 400 },
        },
        required: ["name"],
        additionalProperties: false,
      },
      annotations: readOnly,
      handler: (args) => diffWidget(catalog, args),
    },
    {
      name: "theme_status",
      title: "FOX 2 theme coverage",
      description:
        "Widget theming status from docs/theme-widget-checklist.md: DONE / PARTIAL / OPEN per widget, grouped by priority (covered, P0, P1, P2), including the note that says what is still missing.",
      inputSchema: {
        type: "object",
        properties: {
          widget: { type: "string", description: "Only rows whose name contains this substring." },
          state: { type: "string", enum: ["DONE", "PARTIAL", "OPEN"] },
          group: { type: "string", enum: ["covered", "P0", "P1", "P2", "all"], default: "all" },
          notes: { type: "boolean", default: false, description: "Include the notes (long)." },
          limit: { type: "integer", minimum: 1, maximum: 500, default: 200 },
        },
        additionalProperties: false,
      },
      annotations: readOnly,
      handler: (args) => themeStatus(catalog, args),
    },
    {
      name: "widget_hierarchy",
      title: "FOX class hierarchy",
      description:
        "Inheritance tree of the FOX widget classes (or of every FOX class when includeNonWidgets=true), rooted at FXWindow or at any other class. Use it to find which base class provides a behaviour.",
      inputSchema: {
        type: "object",
        properties: {
          root: { type: "string", default: "FXWindow", description: "Class name to start from, e.g. FXScrollArea." },
          variant: { type: "string", enum: VARIANT_IDS, default: "fox2" },
          includeNonWidgets: { type: "boolean", default: false },
          maxDepth: { type: "integer", minimum: 1, maximum: 12, default: 6 },
          maxLines: { type: "integer", minimum: 20, maximum: 4000, default: 400 },
        },
        additionalProperties: false,
      },
      annotations: readOnly,
      handler: (args) => widgetHierarchy(catalog, args),
    },
  ];
}

/* ------------------------------------------------------------------ tools */

function listWidgets(catalog, args) {
  const variants = (args.variant ?? "both") === "both" ? VARIANT_IDS : [args.variant];
  const limit = args.limit ?? 300;
  const wantedState = args.themeState ? String(args.themeState).toUpperCase() : null;
  const nameFilter = args.name ? String(args.name).toLowerCase() : null;
  const baseFilter = args.base ? String(args.base) : null;
  const chunks = [];
  let shown = 0;

  for (const id of variants) {
    if (!catalog.hasVariant(id)) {
      chunks.push(`${id}: source tree missing (${VARIANTS[id].dir})`);
      continue;
    }
    let widgets = catalog.widgets(id);
    if (nameFilter) widgets = widgets.filter((entry) => entry.name.toLowerCase().includes(nameFilter));
    if (baseFilter) widgets = widgets.filter((entry) => derives(catalog, id, entry, baseFilter));
    if (wantedState) {
      // The checklist describes fox2; the pristine baseline has no theme at all.
      widgets = widgets.filter((entry) => {
        if (id === "fox1") return false;
        const theme = catalog.themeFor(entry.name);
        const state = theme ? normalizeState(theme.state) : "UNTESTED";
        return state === wantedState;
      });
    }
    const total = widgets.length;
    const page = widgets.slice(0, limit);
    shown += page.length;

    const width = Math.max(4, ...page.map((entry) => entry.name.length));
    const baseWidth = Math.max(4, ...page.map((entry) => (entry.bases[0] ?? "").length));
    const lines = page.map((entry) => {
      const theme = catalog.themeFor(entry.name);
      const state = id === "fox1" ? "classic" : theme ? `${normalizeState(theme.state)}/${theme.group}` : "UNTESTED";
      return `${entry.name.padEnd(width)}  ${(entry.bases[0] ?? "-").padEnd(baseWidth)}  ${state.padEnd(16)}  ${entry.headerRel}`;
    });
    if (!page.length) continue;
    chunks.push(
      `${id} — ${VARIANTS[id].label}, version ${catalog.index(id).version}: ${total} widget${total === 1 ? "" : "s"}${total > page.length ? ` (showing ${page.length})` : ""}`,
      lines.join("\n"),
    );
  }

  if (!shown) return textResult("No widget matched those filters. Try list_widgets without filters.");
  chunks.push("Columns: class, first base class, theme state/group (fox2 checklist), header path. Use widget_info for details.");
  return textResult(chunks.join("\n\n"));
}

function widgetInfo(catalog, args) {
  const name = requireName(args.name);
  const ids = VARIANT_IDS.filter((id) => catalog.hasVariant(id));
  const present = ids.filter((id) => catalog.widget(id, name));
  if (!present.length) {
    return textResult(`No class named ${name} in ${ids.join(" or ")}. Use list_widgets with name="${name.replace(/^FX/, "")}" to find it.`);
  }
  const wanted = args.variant ?? "fox2";
  const primary = catalog.hasVariant(wanted) && catalog.widget(wanted, name) ? wanted : present[0];
  const entry = catalog.widget(primary, name);
  const parts = [`# ${name} (${primary} — ${VARIANTS[primary].label}, version ${catalog.index(primary).version})`];

  parts.push(`header: ${entry.headerRel}:${entry.declLine}-${entry.endLine}`);
  const chain = lineage(catalog, primary, name);
  parts.push(`inherits: ${chain.join(" -> ")}`);
  parts.push(`base: ${entry.bases.join(", ") || "(none)"}${entry.isWidget ? " — widget (FXWindow descendant)" : " — not an FXWindow descendant"}`);

  const subs = catalog.index(primary).subclasses.get(name) ?? [];
  parts.push(`subclasses: ${subs.length ? subs.join(", ") : "(none)"}`);
  parts.push(`also present in: ${present.filter((id) => id !== primary).map((id) => `${id}:${catalog.widget(id, name).headerRel}`).join(", ") || "(none)"}`);

  const impls = catalog.implFiles(primary, name);
  if (impls.length) {
    const sizes = impls.map((rel) => {
      const source = readTextSafe(path.join(catalog.root, rel));
      const lines = source ? splitLines(source).length : 0;
      return `${rel} (${lines} lines)`;
    });
    parts.push(`implementation: ${sizes.join(", ")}`);
  } else {
    parts.push(`implementation: (no lib file matching ${name}.cpp)`);
  }

  if (entry.isWidget) {
    const theme = catalog.themeFor(name);
    if (theme) {
      const note = args.themeNotes === false ? "" : `\n  note: ${trimNote(theme.note, 900)}`;
      parts.push(`theme: ${normalizeState(theme.state)} — group ${theme.group} (${theme.section})\n  checklist: ${catalog.themeIndex().file}${note}`);
    } else {
      parts.push(`theme: not in ${catalog.themeIndex().file ?? "the checklist"} — presumably still classic/untouched by the theme work.`);
    }
  }

  if (entry.api.length) {
    parts.push(describeApi(entry.api, name));
  }

  const docs = catalog.docsFor(name, 10);
  parts.push(`docs mentions:\n${docs.length ? docs.map((hit) => `  ${hit.file}:${hit.line}: ${hit.text}`).join("\n") : "  (none)"}`);

  const usages = catalog.usages(primary, name, 10);
  parts.push(`used in apps/tests/utils:\n${usages.length ? usages.map((hit) => `  ${hit.file}:${hit.line}: ${hit.text}`).join("\n") : "  (none)"}`);

  parts.push("Next: widget_source for the declaration or implementation, diff_widget to see what fox2 changed, grep_source to find where a behaviour lives.");
  return textResult(parts.join("\n"));
}

function widgetSource(catalog, args) {
  const name = requireName(args.name);
  const variant = requireVariant(catalog, args.variant ?? "fox2");
  const entry = catalog.widget(variant, name);
  if (!entry) return textResult(`No class ${name} in ${variant}. Use list_widgets first.`);
  const maxLines = args.maxLines ?? 300;
  const part = args.part ?? "header";
  const out = [];
  let remaining = maxLines;

  if (part === "header" || part === "all") {
    const text = readTextSafe(entry.headerAbs);
    if (text == null) return textResult(`Cannot read ${entry.headerRel}`);
    const slice = sliceLines(text, entry.declLine, entry.endLine);
    const lines = slice.lines.slice(0, remaining);
    remaining -= lines.length;
    out.push(
      `--- ${entry.headerRel}:${entry.declLine}-${entry.endLine} (class ${name}, ${slice.lines.length} lines${lines.length < slice.lines.length ? ", truncated" : ""}) ---`,
      numbered(lines, entry.declLine),
    );
  }

  if (part === "impl" || part === "all") {
    if (remaining < 20) {
      out.push(`(no budget left for the implementation — raise maxLines)`);
    } else {
      const impls = catalog.implFiles(variant, name);
      if (!impls.length) {
        out.push(`--- no lib/*.cpp for ${name} in ${variant}; try grep_source with pattern "${name}" ---`);
      } else {
        const primary = impls.find((rel) => path.basename(rel) === `${name}.cpp`) ?? impls[0];
        const text = readTextSafe(path.join(catalog.root, primary));
        const lines = splitLines(text ?? "");
        const shown = lines.slice(0, Math.min(remaining, lines.length));
        const others = impls.filter((rel) => rel !== primary);
        out.push(
          `--- ${primary} (${lines.length} lines${shown.length < lines.length ? `, showing ${shown.length}` : ""}) ---`,
          numbered(shown, 1),
          others.length ? `other implementation files: ${others.join(", ")} (use read_source ${others[0]} startLine=...)` : "",
        );
      }
    }
  }

  return textResult(out.filter(Boolean).join("\n"));
}

function readSource(catalog, args) {
  const start = args.startLine ?? 1;
  const result = catalog.readSource(args.path, {
    variant: args.variant,
    startLine: start,
    endLine: args.endLine,
    maxLines: args.maxLines ?? 200,
  });
  const header = `${result.rel} (lines ${result.startLine}-${result.endLine} of ${result.totalLines})`;
  return textResult(`${header}\n${numbered(result.lines, result.startLine)}`);
}

function grepSource(catalog, args) {
  const pattern = String(args.pattern ?? "");
  if (!pattern) return textResult("pattern is required");
  let regex;
  try {
    regex = new RegExp(args.fixed ? escapeRegExp(pattern) : pattern, args.ignoreCase ? "i" : "");
  } catch (error) {
    return textResult(`invalid regular expression: ${error.message}`);
  }

  const variants = (args.variant ?? "both") === "both" ? VARIANT_IDS : [args.variant];
  const scope = args.scope ?? "all";
  const context = args.context ?? 0;
  const maxResults = args.maxResults ?? 80;
  const filesOnly = args.filesOnly === true;
  const hits = [];
  const filesWithHits = new Map();

  for (const id of variants) {
    if (!catalog.hasVariant(id)) continue;
    const variantDir = catalog.variantDir(id);
    for (const target of scopeTargets(catalog, variantDir, scope)) {
      const absFiles = isDir(target) ? walkFiles(target, { maxFiles: 20000 }) : isFile(target) ? [target] : [];
      for (const abs of absFiles) {
        if (hits.length >= maxResults && !filesOnly) break;
        if (!isSearchable(abs, scope)) continue;
        const text = readTextSafe(abs);
        if (text == null || text.length > 4_000_000) continue;
        const lines = splitLines(text);
        const rel = catalog.rel(abs);
        for (let i = 0; i < lines.length; i++) {
          regex.lastIndex = 0;
          if (!regex.test(lines[i])) continue;
          if (!filesWithHits.has(rel)) filesWithHits.set(rel, 0);
          filesWithHits.set(rel, filesWithHits.get(rel) + 1);
          if (filesOnly) break;
          const block = [];
          for (let j = Math.max(0, i - context); j <= Math.min(lines.length - 1, i + context); j++) {
            block.push(`${rel}:${j + 1}: ${collapse(lines[j]).slice(0, 240)}`);
          }
          hits.push(block.join("\n"));
          if (hits.length >= maxResults) break;
        }
        if (hits.length >= maxResults && !filesOnly) break;
      }
    }
  }

  if (filesOnly) {
    if (!filesWithHits.size) return textResult(`No file matched /${pattern}/.`);
    const listing = [...filesWithHits.entries()]
      .sort((a, b) => b[1] - a[1])
      .map(([file, count]) => `${String(count).padStart(4)}  ${file}`);
    return textResult(`${filesWithHits.size} file(s) match /${pattern}/:\n${listing.join("\n")}`);
  }
  if (!hits.length) return textResult(`No match for /${pattern}/ in ${scope} (${variants.join(", ")}).`);
  const suffix = hits.length >= maxResults ? `\n… stopped at ${maxResults} matches — narrow with scope or a stricter pattern` : "";
  return textResult(`${hits.join(context ? "\n--\n" : "\n")}${suffix}`);
}

function diffWidget(catalog, args) {
  const name = requireName(args.name);
  for (const id of VARIANT_IDS) {
    if (!catalog.hasVariant(id)) return textResult(`Source tree ${VARIANTS[id].dir} is missing under ${catalog.root}`);
  }
  const part = args.part ?? "impl";
  const context = args.context ?? 3;
  let relA;
  let relB;

  if (part === "header") {
    const a = catalog.widget("fox1", name);
    const b = catalog.widget("fox2", name);
    if (!a || !b) {
      return textResult(`Header diff needs ${name} in both trees (fox1: ${a ? "yes" : "no"}, fox2: ${b ? "yes" : "no"}).`);
    }
    relA = a.headerRel;
    relB = b.headerRel;
  } else {
    const implA = primaryImpl(catalog, "fox1", name);
    const implB = primaryImpl(catalog, "fox2", name);
    if (!implA || !implB) {
      const available = catalog.implFiles(catalog.hasVariant("fox2") ? "fox2" : "fox1", name);
      return textResult(
        `Implementation diff needs a lib/*.cpp in both trees (fox1: ${implA ?? "none"}, fox2: ${implB ?? "none"}).\n` +
          `Known implementation files: ${available.join(", ") || "(none)"}\n` +
          `Try part="header", or read a specific file with read_source.`,
      );
    }
    relA = implA;
    relB = implB;
  }

  const diff = unifiedDiff(
    path.join(catalog.root, relA),
    path.join(catalog.root, relB),
    `${relA}`,
    `${relB}`,
    context,
  );
  if (diff.error) return textResult(diff.error);
  if (!diff.text.trim()) return textResult(`${name}: no differences in ${part} between ${relA} and ${relB}.`);

  const lines = splitLines(diff.text);
  const maxLines = args.maxLines ?? 400;
  const shown = lines.slice(0, maxLines);
  const stat = diffStat(lines);
  const suffix = lines.length > shown.length ? `\n… truncated at ${maxLines} of ${lines.length} diff lines` : "";
  return textResult(
    `# ${name} — ${part} diff, fox1 -> fox2${stat}\n${shown.join("\n")}${suffix}`,
  );
}

function themeStatus(catalog, args) {
  const { rows, file } = catalog.themeIndex();
  if (!rows.size) return textResult(`No theme checklist found (${file ?? "docs/theme-widget-checklist.md"}).`);
  const widget = args.widget ? String(args.widget) : null;
  const state = args.state ? String(args.state).toUpperCase() : null;
  const group = args.group && args.group !== "all" ? args.group : null;
  const notes = args.notes === true;
  const limit = args.limit ?? 200;

  const entries = [...rows.entries()]
    .filter(([name, row]) => (!widget || name.includes(widget)) && (!state || normalizeState(row.state) === state) && (!group || row.group === group))
    .sort((a, b) => a[0].localeCompare(b[0]))
    .slice(0, limit);

  if (!entries.length) return textResult("No checklist row matched those filters.");

  const counts = new Map();
  for (const [, row] of rows) {
    const key = normalizeState(row.state);
    counts.set(key, (counts.get(key) ?? 0) + 1);
  }
  const summary = [...counts.entries()].map(([key, value]) => `${key}=${value}`).join(" ");

  const body = entries.map(([name, row]) => {
    const line = `${normalizeState(row.state).padEnd(8)} ${row.group.padEnd(8)} ${name}`;
    return notes && row.note ? `${line}\n         ${trimNote(row.note, 700)}` : line;
  });
  return textResult(
    `# Theme coverage (${file})\n${summary}\n\n${body.join("\n")}\n\nGroups: covered = "Already covered", P0/P1/P2 = remaining work by priority. Toggle notes=true for the "what is missing" text.`,
  );
}

function widgetHierarchy(catalog, args) {
  const variant = requireVariant(catalog, args.variant ?? "fox2");
  const index = catalog.index(variant);
  const root = args.root ? requireName(args.root) : "FXWindow";
  if (!index.classes.has(root)) {
    return textResult(`No class ${root} in ${variant}. Try widget_hierarchy root="FXObject".`);
  }
  const includeNonWidgets = args.includeNonWidgets === true;
  const maxDepth = args.maxDepth ?? 6;
  const maxLines = args.maxLines ?? 400;
  const lines = [];

  const visit = (name, depth, prefix, isLast, isRoot) => {
    if (lines.length >= maxLines) return;
    const entry = index.classes.get(name);
    const marker = isRoot ? `${name}` : `${prefix}${isLast ? "└── " : "├── "}${name}`;
    lines.push(entry && !entry.isWidget && !isRoot ? `${marker}  (not a widget)` : marker);
    if (depth >= maxDepth) return;
    const children = (index.subclasses.get(name) ?? []).filter((child) => {
      if (includeNonWidgets) return true;
      const childEntry = index.classes.get(child);
      return childEntry && childEntry.isWidget;
    });
    const childPrefix = isRoot ? "" : `${prefix}${isLast ? "    " : "│   "}`;
    children.forEach((child, i) => visit(child, depth + 1, childPrefix, i === children.length - 1, false));
  };

  visit(root, 0, "", true, true);
  return textResult(
    `# ${variant} (${VARIANTS[variant].label}) class hierarchy under ${root}\n${lines.join("\n")}\n\n` +
      (includeNonWidgets ? "Non-widget classes are included." : "Only FXWindow descendants are shown; set includeNonWidgets=true for the full tree."),
  );
}

/* --------------------------------------------------------------- helpers */

function scopeTargets(catalog, variantDir, scope) {
  const map = {
    headers: [path.join(variantDir, "include")],
    lib: [path.join(variantDir, "lib")],
    apps: [path.join(variantDir, "apps")],
    tests: [path.join(variantDir, "tests")],
    utils: [path.join(variantDir, "utils")],
    docs: [path.join(catalog.root, "docs"), path.join(catalog.root, "README.md")],
  };
  if (scope === "all") return Object.values(map).flat();
  return map[scope] ?? Object.values(map).flat();
}

function isSearchable(abs, scope) {
  if (scope === "docs") return DOC_EXT.test(abs);
  if (scope === "all") return SOURCE_EXT.test(abs) || DOC_EXT.test(abs);
  return SOURCE_EXT.test(abs) || /\.(xbm|css|rc|am|ac|cmake|txt)$/i.test(abs);
}

function derives(catalog, variant, entry, target, seen = new Set()) {
  if (entry.name === target) return true;
  if (seen.has(entry.name)) return false;
  seen.add(entry.name);
  return entry.bases.some((base) => {
    const parent = catalog.widget(variant, base) ?? catalog.index(variant).classes.get(base);
    return parent ? derives(catalog, variant, parent, target, seen) : base === target;
  });
}

function lineage(catalog, variant, name) {
  const chain = [];
  let current = catalog.index(variant).classes.get(name);
  const seen = new Set();
  while (current && !seen.has(current.name)) {
    seen.add(current.name);
    chain.push(current.name);
    const base = current.bases[0];
    current = base ? catalog.index(variant).classes.get(base) : undefined;
  }
  return chain;
}

function primaryImpl(catalog, variant, name) {
  if (!catalog.hasVariant(variant)) return null;
  const impls = catalog.implFiles(variant, name);
  if (!impls.length) return null;
  return impls.find((rel) => path.basename(rel) === `${name}.cpp`) ?? impls[0];
}

function unifiedDiff(fileA, fileB, labelA, labelB, context) {
  try {
    const output = execFileSync(
      "diff",
      ["-u", `-U${context}`, "--label", labelA, "--label", labelB, fileA, fileB],
      { encoding: "utf8", maxBuffer: 32 * 1024 * 1024 },
    );
    return { text: output };
  } catch (error) {
    if (error && error.status === 1 && typeof error.stdout === "string") return { text: error.stdout };
    if (error && error.code === "ENOENT") {
      return { error: "The `diff` program is not available. Use read_source on both trees instead." };
    }
    return { error: `diff failed: ${error.message}` };
  }
}

function diffStat(lines) {
  let added = 0;
  let removed = 0;
  for (const line of lines) {
    if (line.startsWith("+") && !line.startsWith("+++")) added++;
    else if (line.startsWith("-") && !line.startsWith("---")) removed++;
  }
  return ` (+${added}/-${removed} lines)`;
}

function describeApi(api, name) {
  const isCtor = (line) => new RegExp(`(^|[^A-Za-z0-9_])~?${name}\\s*\\(`).test(line);
  const isHandler = (line) => /\bon[A-Z]\w*\s*\(/.test(line) && !isCtor(line);
  const ctors = api.filter(isCtor);
  const handlers = api.filter(isHandler);
  const methods = api.filter((line) => !isCtor(line) && !isHandler(line));
  const section = (title, lines) =>
    lines.length ? `  ${title} (${lines.length}):\n${lines.map((line) => `    ${line}`).join("\n")}` : "";
  return [`public API (${api.length} declarations from the header):`, section("constructors/destructor", ctors), section("methods", methods), section("event handlers", handlers)]
    .filter(Boolean)
    .join("\n");
}

function normalizeState(state) {
  const text = String(state ?? "").toUpperCase();
  if (text.includes("DONE")) return "DONE";
  if (text.includes("PARTIAL")) return "PARTIAL";
  if (text.includes("OPEN")) return "OPEN";
  return "UNTESTED";
}

function trimNote(note, max) {
  const clean = collapse(note ?? "");
  if (clean.length <= max) return clean;
  return `${clean.slice(0, max)}… (${clean.length} chars; read docs/theme-widget-checklist.md for the rest)`;
}

function requireName(value) {
  const name = String(value ?? "").trim();
  if (!name) throw new Error("name is required");
  return name.replace(/\.h$/, "");
}

function requireVariant(catalog, id) {
  if (!VARIANT_IDS.includes(id)) throw new Error(`unknown variant ${id}`);
  if (!catalog.hasVariant(id)) throw new Error(`source tree ${VARIANTS[id].dir} is missing under ${catalog.root}`);
  return id;
}

function escapeRegExp(text) {
  return text.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
}

function textResult(text) {
  return { content: [{ type: "text", text }] };
}
