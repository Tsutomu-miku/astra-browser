import { existsSync, readdirSync, readFileSync, statSync } from "node:fs";
import { join, relative } from "node:path";
import { fileURLToPath } from "node:url";

const root = fileURLToPath(new URL("..", import.meta.url));

const forbiddenPaths = [
  "CMakeLists.txt",
  "browser/chromium",
  "browser/core",
  "native",
  "scripts/build-native.sh",
  "scripts/setup-cef.sh"
];

const activeRequiredFiles = [
  "AGENTS.md",
  "README.md",
  "docs/ARCHITECTURE.md",
  "docs/CODE_STRUCTURE.md",
  "docs/ENGINEERING_STANDARDS.md",
  "docs/ROADMAP.md",
  "docs/CHROMIUM_DIRECT_REFACTOR_PLAN.md",
  "docs/adr/0009-direct-chromium-architecture.md",
  "chromium/astra/BUILD.gn"
];

const activeDocFiles = [
  "README.md",
  "AGENTS.md",
  "docs/ARCHITECTURE.md",
  "docs/CODE_STRUCTURE.md",
  "docs/ENGINEERING_STANDARDS.md",
  "docs/ROADMAP.md",
  "docs/CHROMIUM_DIRECT_REFACTOR_PLAN.md",
  "docs/adr/README.md",
  "docs/adr/0009-direct-chromium-architecture.md"
];

const activeDocForbiddenPhrases = [
  "Electron is acceptable",
  "Chromium 原生 CEF",
  "CEF 架构",
  "CEF 工程",
  "setup-cef",
  "build-native",
  "electron-builder artifacts"
];

const astraSourceForbidden = [
  /\bCEF\b/i,
  /\bCef[A-Z_a-z0-9]*/,
  /\bElectron\b/i,
  /\bAppKit\b/,
  /\bCMake\b/i,
  /\blibcef\b/i,
  /\belectron-builder\b/i
];

const duplicateChromiumServiceNames = [
  "ProfileManager",
  "DownloadManager",
  "PermissionManager",
  "HistoryService",
  "ExtensionService",
  "PasswordManager",
  "AutofillService",
  "SafeBrowsingService",
  "DevToolsService"
];

const errors = [];

function assert(condition, message) {
  if (!condition) {
    errors.push(message);
  }
}

function read(path) {
  return readFileSync(join(root, path), "utf8");
}

function walk(dir) {
  const fullDir = join(root, dir);
  if (!existsSync(fullDir)) {
    return [];
  }
  return readdirSync(fullDir).flatMap((entry) => {
    const fullPath = join(fullDir, entry);
    const stat = statSync(fullPath);
    return stat.isDirectory()
      ? walk(relative(root, fullPath))
      : [relative(root, fullPath)];
  });
}

for (const path of forbiddenPaths) {
  assert(!existsSync(join(root, path)), `Forbidden legacy path exists: ${path}`);
}

for (const path of activeRequiredFiles) {
  assert(existsSync(join(root, path)), `Missing active architecture file: ${path}`);
}

for (const path of activeDocFiles) {
  if (!existsSync(join(root, path))) continue;
  const contents = read(path);
  for (const phrase of activeDocForbiddenPhrases) {
    assert(!contents.includes(phrase),
           `Active doc ${path} contains stale phrase: ${phrase}`);
  }
}

for (const file of walk("chromium/astra")) {
  if (!/\.(cc|h|gn|gni|md)$/.test(file)) continue;
  const contents = read(file);

  for (const pattern of astraSourceForbidden) {
    assert(!pattern.test(contents),
           `${file} references forbidden runtime pattern ${pattern}`);
  }

  for (const serviceName of duplicateChromiumServiceNames) {
    assert(!contents.includes(`class ${serviceName}`),
           `${file} declares Chromium-owned service ${serviceName}`);
  }

  const vagueTodos = [...contents.matchAll(/TODO(?!\(astra\))/g)];
  assert(vagueTodos.length === 0,
         `${file} contains TODO without TODO(astra) owner`);
}

if (errors.length > 0) {
  console.error("Architecture guard failed:");
  for (const error of errors) {
    console.error(`- ${error}`);
  }
  process.exit(1);
}

console.log("Architecture guard passed.");
