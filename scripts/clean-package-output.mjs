import { readdirSync, rmSync } from "node:fs";
import { join } from "node:path";
import { fileURLToPath } from "node:url";

const root = fileURLToPath(new URL("..", import.meta.url));
const releaseDir = join(root, "release");
const stagingDirectoryNames = [/^mac(?:-.+)?$/, /^win(?:-.+)?-unpacked$/, /^linux(?:-.+)?-unpacked$/];
const removableFiles = [/^builder-debug\.yml$/, /^latest(?:-.+)?\.yml$/, /^.+\.blockmap$/];

let removedCount = 0;

try {
  for (const entry of readdirSync(releaseDir, { withFileTypes: true })) {
    const shouldRemove = entry.isDirectory()
      ? stagingDirectoryNames.some((pattern) => pattern.test(entry.name))
      : removableFiles.some((pattern) => pattern.test(entry.name));

    if (!shouldRemove) {
      continue;
    }

    rmSync(join(releaseDir, entry.name), { force: true, recursive: true });
    removedCount += 1;
  }
} catch (error) {
  if (error?.code !== "ENOENT") {
    throw error;
  }
}

console.log(`Cleaned ${removedCount} package staging artifact${removedCount === 1 ? "" : "s"}.`);
