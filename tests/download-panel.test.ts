import { createElement } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { readFileSync } from "node:fs";
import { join } from "node:path";
import { describe, expect, it, vi } from "vitest";

import type { DownloadEntry } from "../src/renderer/domain/browser";
import { DownloadContextMenu } from "../src/renderer/surfaces/panels/downloads/components/DownloadContextMenu";
import { DownloadItem } from "../src/renderer/surfaces/panels/downloads/components/DownloadItem";

const panelsCss = readFileSync(join(__dirname, "../src/renderer/styles/panels.css"), "utf8");
const completedDownload: DownloadEntry = {
  filename: "archive.zip",
  id: "download",
  receivedBytes: 100,
  savePath: "/tmp/archive.zip",
  startedAt: 1,
  state: "completed",
  totalBytes: 100
};

describe("downloads panel", () => {
  it("renders download rows with file actions", () => {
    const html = renderToStaticMarkup(createElement(DownloadItem, {
      download: completedDownload,
      onContextMenu: vi.fn()
    }));

    expect(html).toContain("archive.zip");
    expect(html).toContain("Completed");
    expect(html).toContain("Open download");
    expect(html).toContain("Show in folder");
  });

  it("renders context menu actions for download entries", () => {
    const html = renderToStaticMarkup(createElement(DownloadContextMenu, {
      download: completedDownload,
      left: 10,
      top: 20,
      onClose: vi.fn()
    }));

    expect(html).toContain('role="menu"');
    expect(html).toContain("Open download");
    expect(html).toContain("Show in folder");
    expect(html).toContain("Copy file path");
  });

  it("styles the download context menu", () => {
    expect(panelsCss).toContain(".download-context-menu");
    expect(panelsCss).toContain(".download-context-menu button:disabled");
  });
});
