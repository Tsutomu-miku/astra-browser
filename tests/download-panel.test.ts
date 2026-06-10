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
const fakeStore = {
  cancelDownload: vi.fn(),
  pauseDownload: vi.fn(),
  resumeDownload: vi.fn(),
  removeDownload: vi.fn()
};

describe("downloads panel", () => {
  it("renders download rows with file actions", () => {
    const html = renderToStaticMarkup(createElement(DownloadItem, {
      download: completedDownload,
      onContextMenu: vi.fn(),
      store: fakeStore
    }));

    expect(html).toContain("archive.zip");
    expect(html).toContain("Completed");
    expect(html).toContain('title="Open download"');
    expect(html).toContain('title="Show in folder"');
  });

  it("renders pause / resume buttons for progressing and paused downloads", () => {
    const progressing = renderToStaticMarkup(createElement(DownloadItem, {
      download: { ...completedDownload, state: "progressing", receivedBytes: 40, canPause: true, savePath: "", url: "https://example.com/a.zip" },
      onContextMenu: vi.fn(),
      store: fakeStore
    }));
    expect(progressing).toContain('title="Pause download"');
    expect(progressing).not.toContain('title="Resume download"');

    const paused = renderToStaticMarkup(createElement(DownloadItem, {
      download: { ...completedDownload, state: "paused", receivedBytes: 40, savePath: "" },
      onContextMenu: vi.fn(),
      store: fakeStore
    }));
    expect(paused).toContain('title="Resume download"');
    expect(paused).toContain("is-paused");
  });

  it("renders context menu actions for download entries", () => {
    const html = renderToStaticMarkup(createElement(DownloadContextMenu, {
      download: completedDownload,
      left: 10,
      top: 20,
      onClose: vi.fn(),
      store: fakeStore
    }));

    expect(html).toContain('role="menu"');
    expect(html).toContain("Open download");
    expect(html).toContain("Show in folder");
    expect(html).toContain("Copy file path");
  });

  it("renders Pause/Resume/Cancel and Remove entries in the context menu", () => {
    const progressing = renderToStaticMarkup(createElement(DownloadContextMenu, {
      download: { ...completedDownload, state: "progressing", receivedBytes: 40, canPause: true, savePath: "" },
      left: 0,
      top: 0,
      onClose: vi.fn(),
      store: fakeStore
    }));
    expect(progressing).toContain("Pause");
    expect(progressing).toContain("Cancel");

    const terminal = renderToStaticMarkup(createElement(DownloadContextMenu, {
      download: { ...completedDownload, state: "interrupted", receivedBytes: 0 },
      left: 0,
      top: 0,
      onClose: vi.fn(),
      store: fakeStore
    }));
    expect(terminal).toContain("Remove from list");
  });

  it("styles the download context menu and pause/terminal modifiers", () => {
    expect(panelsCss).toContain(".download-context-menu");
    expect(panelsCss).toContain(".download-context-menu button:disabled");
    expect(panelsCss).toContain(".download-context-menu-separator");
    expect(panelsCss).toContain(".panel-header-actions");
    expect(panelsCss).toContain(".download-item.is-paused");
    expect(panelsCss).toContain(".download-item.is-terminal");
  });
});
