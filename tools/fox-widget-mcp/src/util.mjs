// Small filesystem / text helpers shared by the catalog and the tools.
// No dependencies outside node: builtins.

import fs from "node:fs";
import path from "node:path";

export const SKIP_DIRS = new Set([".git", "node_modules", "build", "tmp", "__pycache__"]);

export function statFile(file) {
  try {
    return fs.statSync(file);
  } catch {
    return null;
  }
}

export function isFile(file) {
  const stat = statFile(file);
  return Boolean(stat && stat.isFile());
}

export function isDir(file) {
  const stat = statFile(file);
  return Boolean(stat && stat.isDirectory());
}

export function readText(file) {
  return fs.readFileSync(file, "utf8");
}

export function readTextSafe(file) {
  try {
    return readText(file);
  } catch {
    return null;
  }
}

export function listDir(dir) {
  try {
    return fs.readdirSync(dir, { withFileTypes: true });
  } catch {
    return [];
  }
}

/** All regular files under dir, relative to dir, skipping build/vcs directories. */
export function walkFiles(dir, options = {}) {
  const out = [];
  const maxFiles = options.maxFiles ?? 20000;
  const stack = [dir];
  while (stack.length && out.length < maxFiles) {
    const current = stack.pop();
    for (const entry of listDir(current)) {
      if (entry.name.startsWith(".") && entry.name !== ".") continue;
      const abs = path.join(current, entry.name);
      if (entry.isDirectory()) {
        if (SKIP_DIRS.has(entry.name)) continue;
        stack.push(abs);
      } else if (entry.isFile()) {
        out.push(abs);
      }
    }
  }
  return out;
}

/** Offsets of every line start, so index -> line number is a binary search. */
export function lineStarts(text) {
  const starts = [0];
  for (let i = 0; i < text.length; i++) {
    if (text.charCodeAt(i) === 10) starts.push(i + 1);
  }
  return starts;
}

export function lineNumberAt(starts, index) {
  let lo = 0;
  let hi = starts.length - 1;
  while (lo < hi) {
    const mid = (lo + hi + 1) >> 1;
    if (starts[mid] <= index) lo = mid;
    else hi = mid - 1;
  }
  return lo + 1;
}

export function splitLines(text) {
  return text.split(/\r?\n/);
}

export function numbered(lines, firstLine) {
  const width = String(firstLine + Math.max(0, lines.length - 1)).length;
  return lines
    .map((line, i) => `${String(firstLine + i).padStart(width)}  ${line}`)
    .join("\n");
}

export function sliceLines(text, startLine, endLine) {
  const all = splitLines(text);
  const from = Math.max(1, startLine ?? 1);
  const to = Math.min(all.length, endLine ?? all.length);
  return { lines: all.slice(from - 1, to), startLine: from, totalLines: all.length, endLine: to };
}

function skipQuoted(text, index) {
  const quote = text[index];
  let i = index + 1;
  while (i < text.length) {
    const c = text[i];
    if (c === "\\") {
      i += 2;
      continue;
    }
    if (c === quote) return i + 1;
    if (c === "\n" && quote !== "`") return i; // unterminated: bail out at EOL
    i++;
  }
  return i;
}

/** Index of the `}` matching the `{` at openIndex, skipping comments and strings. */
export function matchBrace(text, openIndex) {
  let depth = 0;
  let i = openIndex;
  while (i < text.length) {
    const c = text[i];
    if (c === "/" && text[i + 1] === "/") {
      const eol = text.indexOf("\n", i);
      if (eol < 0) return -1;
      i = eol + 1;
      continue;
    }
    if (c === "/" && text[i + 1] === "*") {
      const end = text.indexOf("*/", i + 2);
      if (end < 0) return -1;
      i = end + 2;
      continue;
    }
    if (c === '"' || c === "'") {
      i = skipQuoted(text, i);
      continue;
    }
    if (c === "{") depth++;
    else if (c === "}") {
      depth--;
      if (depth === 0) return i;
    }
    i++;
  }
  return -1;
}

export function collapse(text) {
  return text.replace(/\s+/g, " ").trim();
}

export function truncate(text, maxChars) {
  if (text.length <= maxChars) return text;
  return `${text.slice(0, maxChars)}\n… truncated ${text.length - maxChars} of ${text.length} chars — narrow the query`;
}

export function mkdirp(dir) {
  fs.mkdirSync(dir, { recursive: true });
}
