// Index of the two FOX source trees in this repository.
//
// fox1 = the pristine FOX 1.7.95 baseline (fox-1.7.95/)
// fox2 = the themed FOX 2.0 experiment (fox2/)
//
// The index is built lazily and cached for the lifetime of the process.

import path from "node:path";
import {
  collapse,
  isDir,
  isFile,
  lineNumberAt,
  lineStarts,
  listDir,
  matchBrace,
  readText,
  readTextSafe,
  sliceLines,
  statFile,
  splitLines,
  walkFiles,
} from "./util.mjs";

export const VARIANTS = {
  fox1: { dir: "fox-1.7.95", label: "FOX 1.7.95 (baseline)" },
  fox2: { dir: "fox2", label: "FOX 2.0 (themed experiment)" },
};

export const VARIANT_IDS = Object.keys(VARIANTS);

const CLASS_DECL =
  /(?:^|\n)[ \t]*(?:class|struct)[ \t]+((?:[A-Z][A-Z0-9_]*[ \t]+)*)(FX[A-Za-z0-9_]+)[ \t]*((?::[^{;]*)?)\{/g;

const HEADER_EXT = /\.h$/;

export function createCatalog(root) {
  return new Catalog(path.resolve(root));
}

class Catalog {
  constructor(root) {
    this.root = root;
    this.indexes = new Map();
    this.theme = null;
    this.themeMtime = 0;
  }

  variantDir(id) {
    const variant = VARIANTS[id];
    if (!variant) {
      throw new Error(`unknown variant "${id}" (expected ${VARIANT_IDS.join(" or ")})`);
    }
    return path.join(this.root, variant.dir);
  }

  hasVariant(id) {
    return Boolean(VARIANTS[id]) && isDir(this.variantDir(id));
  }

  index(id) {
    if (!VARIANTS[id]) throw new Error(`unknown variant "${id}" (expected ${VARIANT_IDS.join(" or ")})`);
    if (this.indexes.has(id)) return this.indexes.get(id);
    const dir = this.variantDir(id);
    if (!isDir(dir)) throw new Error(`missing source tree ${path.relative(this.root, dir)} under ${this.root}`);
    const index = this.buildIndex(id, dir);
    this.indexes.set(id, index);
    return index;
  }

  buildIndex(id, dir) {
    const includeDir = path.join(dir, "include");
    const headers = new Map(); // class name -> entry
    const classes = new Map(); // class name -> class entry
    const headerFiles = new Map(); // relative path -> parsed header

    for (const entry of listDir(includeDir)) {
      if (!entry.isFile() || !HEADER_EXT.test(entry.name)) continue;
      const abs = path.join(includeDir, entry.name);
      const text = readTextSafe(abs);
      if (text == null) continue;
      const rel = path.relative(this.root, abs);
      const parsed = { rel, abs, name: entry.name, classes: [] };

      const starts = lineStarts(text);
      const consumed = [];
      CLASS_DECL.lastIndex = 0;
      let match;
      while ((match = CLASS_DECL.exec(text))) {
        const openIndex = match.index + match[0].length - 1;
        if (consumed.some(([start, end]) => match.index > start && match.index < end)) continue;
        const closeIndex = matchBrace(text, openIndex);
        if (closeIndex < 0) continue;
        consumed.push([openIndex, closeIndex]);

        const name = match[2];
        const bases = parseBases(match[3]);
        const body = text.slice(openIndex + 1, closeIndex);
        const classEntry = {
          name,
          bases,
          headerRel: rel,
          headerAbs: abs,
          headerName: entry.name,
          declLine: lineNumberAt(starts, match.index),
          startLine: lineNumberAt(starts, openIndex),
          endLine: lineNumberAt(starts, closeIndex),
          api: extractPublicApi(body),
          isWidget: false,
        };
        parsed.classes.push(classEntry);
        if (!classes.has(name)) classes.set(name, classEntry);
      }

      headerFiles.set(rel, parsed);
      for (const classEntry of parsed.classes) {
        if (!headers.has(classEntry.name)) headers.set(classEntry.name, classEntry);
      }
    }

    // Resolve widgetness and subclasses now that every base class is known.
    const subclasses = new Map();
    for (const classEntry of classes.values()) {
      classEntry.isWidget = derivesFrom(classes, classEntry.name, "FXWindow");
      for (const base of classEntry.bases) {
        if (!subclasses.has(base)) subclasses.set(base, []);
        subclasses.get(base).push(classEntry.name);
      }
    }
    for (const list of subclasses.values()) list.sort();

    const libAbs = path.join(dir, "lib");
    const libFiles = isDir(libAbs)
      ? walkFiles(libAbs).map((abs) => path.relative(this.root, abs))
      : [];
    const libByName = new Map();
    for (const rel of libFiles) {
      const base = path.basename(rel).replace(/\.(cpp|cc|cxx|c)$/i, "");
      if (!libByName.has(base)) libByName.set(base, []);
      libByName.get(base).push(rel);
    }

    return {
      id,
      dir,
      label: VARIANTS[id].label,
      version: detectVersion(dir),
      headers,
      classes,
      headerFiles,
      subclasses,
      libFiles,
      libByName,
    };
  }

  /** All widget class names of a variant, sorted. */
  widgets(id) {
    return [...this.index(id).classes.values()]
      .filter((entry) => entry.isWidget)
      .sort((a, b) => a.name.localeCompare(b.name));
  }

  widget(id, name) {
    const classEntry = this.index(id).classes.get(name);
    if (!classEntry) return null;
    return classEntry;
  }

  /** theme checklist rows: class name -> { state, group, section, note } */
  themeIndex() {
    const file = path.join(this.root, "docs", "theme-widget-checklist.md");
    const text = readTextSafe(file);
    if (text == null) return { file: null, rows: new Map() };
    if (this.theme && this.themeMtime === fileMtime(file)) return this.theme;

    const rows = new Map();
    let section = "";
    for (const raw of splitLines(text)) {
      const line = raw.trimEnd();
      if (line.startsWith("## ")) {
        section = line.slice(3).trim();
        continue;
      }
      if (!line.startsWith("|")) continue;
      const cells = line
        .split("|")
        .slice(1, -1)
        .map((cell) => cell.trim());
      if (cells.length < 2) continue;
      if (/^-+$/.test(cells[0].replace(/[\s:]/g, ""))) continue;
      const names = [...cells[0].matchAll(/`(FX[A-Za-z0-9_]+)`/g)].map((m) => m[1]);
      if (!names.length) continue;
      const stateCell = cells[1] ?? "";
      if (!/DONE|PARTIAL|OPEN/.test(stateCell)) continue;
      const note = cells
        .slice(2)
        .join(" | ")
        .replace(/\s*\|\s*$/, "")
        .trim();
      const group = groupForSection(section);
      for (const name of names) {
        rows.set(name, { state: stateCell, group, section, note });
      }
    }
    this.theme = { file: path.relative(this.root, file), rows };
    this.themeMtime = fileMtime(file);
    return this.theme;
  }

  themeFor(name) {
    return this.themeIndex().rows.get(name) ?? null;
  }

  /** Resolve a user supplied path inside the repository. */
  resolvePath(input, variantHint) {
    if (typeof input !== "string" || !input.trim()) throw new Error("path must be a non-empty string");
    const cleaned = input.trim().replace(/^\.\//, "");
    const prefixed = cleaned.match(/^(fox1|fox2):(.*)$/);
    if (prefixed) {
      const abs = path.join(this.variantDir(prefixed[1]), prefixed[2]);
      return this.withinRoot(abs, input);
    }
    const candidates = [];
    candidates.push(path.join(this.root, cleaned));
    if (variantHint && VARIANTS[variantHint]) candidates.push(path.join(this.variantDir(variantHint), cleaned));
    for (const id of VARIANT_IDS) {
      if (!this.hasVariant(id)) continue;
      candidates.push(path.join(this.variantDir(id), cleaned));
      candidates.push(path.join(this.variantDir(id), "include", cleaned));
      candidates.push(path.join(this.variantDir(id), "lib", cleaned));
    }
    for (const candidate of candidates) {
      if (isFile(candidate)) return this.withinRoot(candidate, input);
    }
    throw new Error(`no such source file: ${input} (try variant:path, e.g. fox2:lib/FXButton.cpp)`);
  }

  withinRoot(abs, label) {
    const resolved = path.resolve(abs);
    const prefix = this.root.endsWith(path.sep) ? this.root : this.root + path.sep;
    if (!resolved.startsWith(prefix)) throw new Error(`path escapes the repository: ${label}`);
    if (resolved.includes(`${path.sep}.git${path.sep}`)) throw new Error(`refusing to read inside .git: ${label}`);
    return resolved;
  }

  rel(abs) {
    return path.relative(this.root, abs).split(path.sep).join("/");
  }

  /** Source files that implement a class, best candidates first. */
  implFiles(id, name) {
    const index = this.index(id);
    const exact = index.libByName.get(name) ?? [];
    const prefixed = [...index.libByName.entries()]
      .filter(([base]) => base.startsWith(`${name}_`))
      .flatMap(([, files]) => files)
      .sort();
    const out = [...new Set([...exact, ...prefixed])];
    if (out.length) return out;
    // Fall back to files that pull in the header.
    const needle = `"${name}.h"`;
    const hits = [];
    for (const rel of index.libFiles) {
      if (!/\.(cpp|cc|cxx|c)$/i.test(rel)) continue;
      const text = readTextSafe(path.join(this.root, rel));
      if (text && text.includes(needle)) hits.push(rel);
    }
    return hits;
  }

  /** Read a source file with line numbers. */
  readSource(input, options = {}) {
    const abs = this.resolvePath(input, options.variant);
    const text = readText(abs);
    const total = splitLines(text).length;
    const start = Math.max(1, options.startLine ?? 1);
    const end = options.endLine ?? Math.min(total, start + (options.maxLines ?? 200) - 1);
    const slice = sliceLines(text, start, end);
    return { abs, rel: this.rel(abs), ...slice, text: slice.lines.join("\n") };
  }

  /** The full class declaration of a widget, header included. */
  declaration(entry) {
    const text = readText(entry.headerAbs);
    const slice = sliceLines(text, entry.declLine, entry.endLine);
    return slice.lines.join("\n");
  }

  docsFor(needle, max = 12) {
    const docsDir = path.join(this.root, "docs");
    const files = isDir(docsDir)
      ? listDir(docsDir)
          .filter((entry) => entry.isFile() && entry.name.endsWith(".md"))
          .map((entry) => path.join(docsDir, entry.name))
      : [];
    const readme = path.join(this.root, "README.md");
    if (isFile(readme)) files.push(readme);
    const hits = [];
    for (const abs of files) {
      const text = readTextSafe(abs);
      if (!text) continue;
      let line = 0;
      for (const raw of splitLines(text)) {
        line++;
        if (raw.includes(needle)) {
          hits.push({ file: this.rel(abs), line, text: collapse(raw).slice(0, 200) });
          if (hits.length >= max) return hits;
        }
      }
    }
    return hits;
  }

  /** Where a class is used in apps/tests/utils of a variant. */
  usages(id, name, max = 12) {
    const dir = this.variantDir(id);
    const roots = ["apps", "tests", "utils"]
      .map((sub) => path.join(dir, sub))
      .filter((abs) => isDir(abs));
    const word = new RegExp(`\\b${name}\\b`);
    const hits = [];
    for (const root of roots) {
      for (const abs of walkFiles(root, { maxFiles: 4000 })) {
        if (!/\.(cpp|cc|cxx|h|hpp)$/i.test(abs)) continue;
        const text = readTextSafe(abs);
        if (!text || !word.test(text)) continue;
        const lines = splitLines(text);
        const first = lines.findIndex((line) => word.test(line));
        hits.push({ file: this.rel(abs), line: first + 1, text: collapse(lines[first]).slice(0, 160) });
        if (hits.length >= max) return hits;
      }
    }
    return hits;
  }
}

function parseBases(text) {
  const raw = (text ?? "").replace(/^\s*:/, "").trim();
  if (!raw) return [];
  return raw
    .split(",")
    .map((part) => part.trim())
    .map((part) => {
      const named = part.match(/([A-Za-z_]\w*)\s*(?:<[^>]*>)?\s*$/);
      return named ? named[1] : null;
    })
    .filter(Boolean);
}

function derivesFrom(classes, name, target, seen = new Set()) {
  if (name === target) return true;
  if (seen.has(name)) return false;
  seen.add(name);
  const entry = classes.get(name);
  if (!entry) return false;
  return entry.bases.some((base) => derivesFrom(classes, base, target, seen));
}

function extractPublicApi(body) {
  const api = [];
  let access = "private";
  let pending = "";
  let i = 0;

  const flush = (statement) => {
    let text = collapse(statement);
    if (!text) return;
    let specifier = text.match(/^(public|protected|private)\s*:\s*/);
    if (specifier) {
      access = specifier[1];
      text = text.slice(specifier[0].length);
      if (!text) return;
    }
    if (access !== "public") return;
    if (/^(FXDECLARE|FXIMPLEMENT|FXRTTI|friend|typedef|using)\b/.test(text)) return;
    if (!text.includes("(")) return;
    if (text.length > 400) return;
    api.push(text.replace(/\s*:\s*$/, ""));
  };

  while (i < body.length) {
    const c = body[i];
    if (c === "/" && body[i + 1] === "/") {
      const eol = body.indexOf("\n", i);
      i = eol < 0 ? body.length : eol + 1;
      pending += " ";
      continue;
    }
    if (c === "/" && body[i + 1] === "*") {
      const end = body.indexOf("*/", i + 2);
      i = end < 0 ? body.length : end + 2;
      pending += " ";
      continue;
    }
    if (c === "#") {
      const eol = body.indexOf("\n", i);
      i = eol < 0 ? body.length : eol + 1;
      continue;
    }
    if (c === "{") {
      // Inline function body or enum block: not part of the signature.
      flush(pending);
      pending = "";
      const end = matchBrace(body, i);
      i = end < 0 ? body.length : end + 1;
      continue;
    }
    if (c === ";") {
      flush(pending);
      pending = "";
      i++;
      continue;
    }
    pending += c;
    i++;
  }
  return [...new Set(api)];
}

function detectVersion(dir) {
  const fxver = path.join(dir, "include", "fxver.h");
  const text = readTextSafe(fxver);
  if (text) {
    const major = text.match(/FOX_MAJOR\s+(\d+)/);
    const minor = text.match(/FOX_MINOR\s+(\d+)/);
    const level = text.match(/FOX_LEVEL\s+(\d+)/);
    if (major && minor && level) return `${major[1]}.${minor[1]}.${level[1]}`;
  }
  for (const candidate of [path.join(dir, "CMakeLists.txt"), path.join(dir, "fox2", "CMakeLists.txt")]) {
    const cmake = readTextSafe(candidate);
    if (!cmake) continue;
    const match = cmake.match(/project\([^)]*VERSION\s+([0-9.]+)/i);
    if (match) return match[1];
  }
  return "unknown";
}

function groupForSection(section) {
  const lower = section.toLowerCase();
  if (lower.includes("already covered")) return "covered";
  const priority = section.match(/\bP([0-2])\b/);
  if (priority) return `P${priority[1]}`;
  return section || "other";
}

function fileMtime(file) {
  const stat = statFile(file);
  return stat ? stat.mtimeMs : 0;
}
