import { createElement } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { readFileSync } from "node:fs";
import { join } from "node:path";
import { describe, expect, it, vi } from "vitest";

import type { HistoryEntry } from "../src/renderer/domain/browser";
import { HistoryEntryContextMenu } from "../src/renderer/surfaces/panels/history/components/HistoryEntryContextMenu";
import { HistoryItem } from "../src/renderer/surfaces/panels/history/components/HistoryItem";
import { filterHistory } from "../src/renderer/surfaces/panels/history/model/historyFilters";

const panelsHistoryCss = readFileSync(join(__dirname, "../src/renderer/styles/panels-history.css"), "utf8");
const entry: HistoryEntry = {
  id: "history_docs",
  title: "Docs",
  url: "https://docs.example",
  visitedAt: 1,
  workspaceId: "personal"
};

describe("history panel", () => {
  it("renders history entries with split and preview action hints", () => {
    const html = renderToStaticMarkup(createElement(HistoryItem, {
      entry,
      onContextMenu: vi.fn(),
      onOpen: vi.fn(),
      onOpenInSplit: vi.fn(),
      onPreview: vi.fn(),
      onRemove: vi.fn()
    }));

    expect(html).toContain('class="history-open"');
    expect(html).toContain("Alt Glance");
    expect(html).toContain("Shift Split");
  });

  it("renders context menu actions for history entries", () => {
    const html = renderToStaticMarkup(createElement(HistoryEntryContextMenu, {
      entry,
      left: 10,
      top: 20,
      onClose: vi.fn(),
      onOpen: vi.fn(),
      onOpenInSplit: vi.fn(),
      onPreview: vi.fn(),
      onRemove: vi.fn()
    }));

    expect(html).toContain('role="menu"');
    expect(html).toContain("Open");
    expect(html).toContain("Preview in Glance");
    expect(html).toContain("Open in split view");
    expect(html).toContain("Remove History");
  });

  it("filters history by title and URL", () => {
    const matches = filterHistory([
      entry,
      { ...entry, id: "mail", title: "Mail", url: "https://mail.example" }
    ], "docs");

    expect(matches).toEqual([entry]);
  });

  it("styles the history context menu and action hints", () => {
    expect(panelsHistoryCss).toContain(".history-action-hints");
    expect(panelsHistoryCss).toContain(".history-context-menu");
    expect(panelsHistoryCss).toContain(".history-context-menu button.danger");
  });
});
