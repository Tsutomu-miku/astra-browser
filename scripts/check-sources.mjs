import { readFileSync, readdirSync, statSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { join } from "node:path";
import { spawnSync } from "node:child_process";

const root = fileURLToPath(new URL("..", import.meta.url));
const requiredFiles = [
  "package.json",
  "tsconfig.json",
  "vite.config.ts",
  "src/main/main.js",
  "src/main/preload.js",
  "src/renderer/index.html",
  "src/renderer/main.tsx",
  "src/renderer/app/App.tsx",
  "src/renderer/domain/browser/index.ts",
  "src/renderer/styles.css",
  "docs/PROJECT_SPEC.md"
];
const maxSourceLines = 300;
const oversizedFiles = [];

const missing = requiredFiles.filter((file) => {
  try {
    statSync(join(root, file));
    return false;
  } catch {
    return true;
  }
});

if (missing.length > 0) {
  throw new Error(`Missing required files: ${missing.join(", ")}`);
}

function walk(dir) {
  return readdirSync(dir).flatMap((entry) => {
    const path = join(dir, entry);
    const stat = statSync(path);
    return stat.isDirectory() ? walk(path) : [path];
  });
}

const sourceFiles = walk(join(root, "src")).filter((file) =>
  /\.(html|css|js|mjs|ts|tsx)$/.test(file)
);
const jsFiles = sourceFiles.filter((file) => /\.(js|mjs)$/.test(file));

for (const file of sourceFiles) {
  const contents = readFileSync(file, "utf8");
  if (contents.includes("TODO")) {
    throw new Error(`Unexpected TODO in ${file}`);
  }

  const lineCount = contents.split(/\r?\n/).length;
  if (lineCount > maxSourceLines) {
    oversizedFiles.push(`${file} (${lineCount})`);
  }
}

for (const file of jsFiles) {
  const result = spawnSync(process.execPath, ["--check", file], {
    encoding: "utf8"
  });

  if (result.status !== 0) {
    throw new Error(`Syntax check failed for ${file}\n${result.stderr}`);
  }
}

const renderer = readFileSync(join(root, "src/renderer/app/App.tsx"), "utf8");
const html = readFileSync(join(root, "src/renderer/index.html"), "utf8");
const referencedIds = [...renderer.matchAll(/document\.querySelector\("#([^"]+)"\)/g)]
  .map((match) => match[1])
  .sort();
const htmlIds = new Set([...html.matchAll(/\sid="([^"]+)"/g)].map((match) => match[1]));
const missingIds = referencedIds.filter((id) => !htmlIds.has(id));

if (missingIds.length > 0) {
  throw new Error(`Renderer references missing HTML ids: ${missingIds.join(", ")}`);
}

if (oversizedFiles.length > 0) {
  console.warn(`Review large source files over ${maxSourceLines} lines:\n${oversizedFiles.join("\n")}`);
}

console.log(`Checked ${sourceFiles.length} source files.`);
