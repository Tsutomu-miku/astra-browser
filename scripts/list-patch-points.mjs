// ============================================================================
// Patch Point Scanner
// ============================================================================
// Scans all source files under chromium/astra/ for patch point comments and
// generates a consolidated report.
//
// Patch point comment formats detected:
//   // Patch point: <description>
//   // Chromium patch point: <description>
//   // Patch points: <description>
//   // Chromium patch points: <description>
//
// The report lists every patch point with file path, line number, and
// description.  It also groups patch points by category and layer.
//
// Usage:
//   node scripts/list-patch-points.mjs [--json] [--by-file|--by-layer|--by-category]
//
// Options:
//   --json       Output as JSON instead of human-readable text
//   --by-file    Group results by source file (default)
//   --by-layer   Group results by architectural layer
//   --by-category  Group results by patch point category (best-effort)
// ============================================================================

import { readdirSync, readFileSync, statSync, existsSync } from "node:fs";
import { join, relative, basename, dirname } from "node:path";
import { fileURLToPath } from "node:url";

const root = fileURLToPath(new URL("..", import.meta.url));
const astraDir = join(root, "chromium", "astra");

// ============================================================================
// Helpers
// ============================================================================

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

// Extract architectural layer from a file path.
function getLayer(filePath) {
  const relFromAstra = relative(astraDir, join(root, filePath));
  const parts = relFromAstra.split("/");
  if (parts.length === 0) return "root";

  const top = parts[0];
  if (top === "app") return "app";
  if (top === "browser") return "browser";
  if (top === "common") return "common";
  if (top === "build") return "build";
  if (top === "ui") {
    if (parts[1] === "views") return "ui/views";
    if (parts[1] === "color") return "ui/color";
    if (parts[1] === "accessibility") return "ui/accessibility";
    return "ui";
  }
  if (top === "patches") return "patches";
  if (top === "test") return "test";
  return "root";
}

// Categorize a patch point description.
// Best-effort heuristic based on keywords.
function getCategory(description) {
  const desc = description.toLowerCase();

  if (/\b(browser_main|main_extra_parts|main_delegate|startup|bootstrap)/.test(desc)) {
    return "Startup / BrowserMain";
  }
  if (/\b(browser_view|toolbar|view|widget|ui|views)/.test(desc)) {
    return "UI / BrowserView";
  }
  if (/\b(command|accelerator|shortcut|keybinding)/.test(desc)) {
    return "Commands / Accelerators";
  }
  if (/\b(profile|keyed_service|factory|prefs)/.test(desc)) {
    return "Profile / Keyed Services";
  }
  if (/\b(tab|tabstrip|webcontents)/.test(desc)) {
    return "Tab / WebContents";
  }
  if (/\b(build|gn|buildflag|config)/.test(desc)) {
    return "Build / GN";
  }
  if (/\b(test|unittest|browsertest|test_suite)/.test(desc)) {
    return "Tests";
  }
  if (/\b(branding|version|brand|constants)/.test(desc)) {
    return "Branding / Constants";
  }
  if (/\b(sessions?|restore|session_restore)/.test(desc)) {
    return "Session / Restore";
  }
  if (/\b(devtools?|dev_tools?)/.test(desc)) {
    return "DevTools";
  }
  if (/\b(extension)/.test(desc)) {
    return "Extensions";
  }
  if (/\b(download)/.test(desc)) {
    return "Downloads";
  }
  if (/\b(password|autofill)/.test(desc)) {
    return "Password / Autofill";
  }
  if (/\b(history|bookmark)/.test(desc)) {
    return "History / Bookmarks";
  }
  if (/\b(pip|picture_in_picture)/.test(desc)) {
    return "PiP";
  }
  if (/\b(file|dialog|picker|save|open)/.test(desc)) {
    return "File Dialogs";
  }
  if (/none needed|no patch needed/.test(desc)) {
    return "No Patch Needed";
  }
  return "Other";
}

// ============================================================================
// Scanner
// ============================================================================

function scanPatchPoints() {
  const astraFiles = walk("chromium/astra");
  const allPatchPoints = [];

  // Source file extensions to scan.
  const sourceExts = /\.(cc|h|mm|gn|gni|md)$/;

  // Match patch point comments.
  // Pattern: // optional prefix "Chromium " then "Patch point(s):" then description.
  // The description may span multiple lines if subsequent lines are also
  // // comments (indented continuation).
  const patchPointPattern =
    /^\s*\/\/\s*(?:Chromium\s+)?[Pp]atch\s+points?:\s*(.*)$/m;

  for (const file of astraFiles) {
    if (!sourceExts.test(file)) continue;

    const contents = readFileSync(join(root, file), "utf8");
    const lines = contents.split("\n");

    for (let i = 0; i < lines.length; i++) {
      const match = lines[i].match(patchPointPattern);
      if (!match) continue;

      let description = match[1].trim();
      const startLine = i + 1; // 1-indexed

      // Check for continuation lines (subsequent // comments).
      // A continuation line starts with // and is part of the same comment block.
      let j = i + 1;
      while (j < lines.length) {
        const contMatch = lines[j].match(/^\s*\/\/\s*(.*)$/);
        if (!contMatch) break;

        const contText = contMatch[1].trim();

        // Stop if the continuation line looks like a new patch point or
        // a different kind of annotation (TODO, NOTE, etc.).
        if (/^(?:Chromium\s+)?[Pp]atch\s+points?:/.test(contText)) break;
        if (/^TODO\b/.test(contText)) break;
        if (/^NOTE\b/.test(contText)) break;

        if (contText.length === 0) {
          // Empty comment line — probably end of the comment block.
          break;
        }

        // Skip separator lines (lines made mostly of =, -, *, etc.).
        if (/^[=*\-]{3,}$/.test(contText)) {
          break;
        }

        description += " " + contText;
        j++;
      }

      // Skip "Patch point: None needed" or similar entries that indicate
      // no actual patch is required. These are informational, not patch points.
      // We still include them but mark them as informational.
      const isInformational = /none\s+needed|no\s+patch\s+needed/i.test(description);

      const layer = getLayer(file);
      const category = getCategory(description);

      allPatchPoints.push({
        file,
        line: startLine,
        description,
        layer,
        category,
        isInformational,
      });

      i = j - 1; // Skip past continuation lines
    }
  }

  return allPatchPoints;
}

// ============================================================================
// Output formatters
// ============================================================================

function formatByFile(patchPoints) {
  // Group by file.
  const byFile = new Map();
  for (const pp of patchPoints) {
    if (!byFile.has(pp.file)) {
      byFile.set(pp.file, []);
    }
    byFile.get(pp.file).push(pp);
  }

  const files = [...byFile.keys()].sort();
  let output = "";

  output += "=".repeat(72) + "\n";
  output += "Astra Chromium Patch Points — By File\n";
  output += "=".repeat(72) + "\n";
  output += "\n";
  output += `Total patch points: ${patchPoints.length}\n`;
  output += `Files with patch points: ${files.length}\n`;
  output += "\n";

  for (const file of files) {
    const points = byFile.get(file);
    output += `\n${file} (${points.length})\n`;
    output += "-".repeat(file.length + 6 + String(points.length).length) + "\n";
    for (const pp of points) {
      const marker = pp.isInformational ? "i" : "•";
      const lineStr = String(pp.line).padStart(4);
      output += `  ${marker} L${lineStr}  ${pp.description}\n`;
    }
  }

  return output;
}

function formatByLayer(patchPoints) {
  // Group by layer.
  const byLayer = new Map();
  for (const pp of patchPoints) {
    if (!byLayer.has(pp.layer)) {
      byLayer.set(pp.layer, []);
    }
    byLayer.get(pp.layer).push(pp);
  }

  // Layer order for display.
  const layerOrder = [
    "build",
    "common",
    "browser",
    "app",
    "ui/views",
    "ui/color",
    "ui/accessibility",
    "ui",
    "patches",
    "test",
    "root",
  ];

  let output = "";

  output += "=".repeat(72) + "\n";
  output += "Astra Chromium Patch Points — By Layer\n";
  output += "=".repeat(72) + "\n";
  output += "\n";
  output += `Total patch points: ${patchPoints.length}\n`;
  output += `Layers: ${byLayer.size}\n`;
  output += "\n";

  for (const layer of layerOrder) {
    if (!byLayer.has(layer)) continue;
    const points = byLayer.get(layer);
    const informational = points.filter((p) => p.isInformational).length;

    output += `\n${layer} (${points.length}${informational > 0 ? `, ${informational} informational` : ""})\n`;
    output += "=".repeat(layer.length + 6 + String(points.length).length + (informational > 0 ? 18 + String(informational).length : 0)) + "\n";

    // Sort by file, then line.
    points.sort((a, b) => {
      if (a.file !== b.file) return a.file < b.file ? -1 : 1;
      return a.line - b.line;
    });

    for (const pp of points) {
      const marker = pp.isInformational ? "i" : "•";
      const relFile = relative(astraDir, join(root, pp.file));
      output += `  ${marker} ${relFile}:${pp.line}\n`;
      output += `    ${pp.description}\n`;
    }
  }

  return output;
}

function formatByCategory(patchPoints) {
  // Group by category.
  const byCategory = new Map();
  for (const pp of patchPoints) {
    if (!byCategory.has(pp.category)) {
      byCategory.set(pp.category, []);
    }
    byCategory.get(pp.category).push(pp);
  }

  // Category order: most common first.
  const categories = [...byCategory.entries()]
    .sort((a, b) => b[1].length - a[1].length)
    .map(([cat]) => cat);

  let output = "";

  output += "=".repeat(72) + "\n";
  output += "Astra Chromium Patch Points — By Category\n";
  output += "=".repeat(72) + "\n";
  output += "\n";
  output += `Total patch points: ${patchPoints.length}\n`;
  output += `Categories: ${categories.length}\n`;
  output += "\n";

  for (const category of categories) {
    const points = byCategory.get(category);
    const informational = points.filter((p) => p.isInformational).length;

    output += `\n${category} (${points.length}${informational > 0 ? `, ${informational} informational` : ""})\n`;
    output += "-".repeat(Math.max(category.length, 40)) + "\n";

    // Sort by file, then line.
    points.sort((a, b) => {
      if (a.file !== b.file) return a.file < b.file ? -1 : 1;
      return a.line - b.line;
    });

    for (const pp of points) {
      const marker = pp.isInformational ? "i" : "•";
      const relFile = relative(astraDir, join(root, pp.file));
      output += `  ${marker} ${relFile}:${pp.line}\n`;
      output += `    ${pp.description}\n`;
    }
  }

  return output;
}

function formatJson(patchPoints) {
  return JSON.stringify(
    {
      total: patchPoints.length,
      generatedAt: new Date().toISOString(),
      patchPoints: patchPoints.map((pp) => ({
        file: pp.file,
        line: pp.line,
        description: pp.description,
        layer: pp.layer,
        category: pp.category,
        isInformational: pp.isInformational,
      })),
    },
    null,
    2
  );
}

// ============================================================================
// Main
// ============================================================================

function main() {
  const args = process.argv.slice(2);
  const asJson = args.includes("--json");
  const byLayer = args.includes("--by-layer");
  const byCategory = args.includes("--by-category");
  const byFile = args.includes("--by-file") || (!byLayer && !byCategory);

  if (!existsSync(astraDir)) {
    console.error("chromium/astra/ directory not found.");
    process.exit(1);
  }

  const patchPoints = scanPatchPoints();

  if (asJson) {
    console.log(formatJson(patchPoints));
  } else if (byCategory) {
    console.log(formatByCategory(patchPoints));
  } else if (byLayer) {
    console.log(formatByLayer(patchPoints));
  } else {
    console.log(formatByFile(patchPoints));
  }
}

main();
