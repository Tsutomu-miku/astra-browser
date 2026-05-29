import { createElement, createRef } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { readFileSync } from "node:fs";
import { join } from "node:path";
import { describe, expect, it, vi } from "vitest";

import { createDefaultState } from "../src/renderer/domain/browser";
import { getMemorySaverState } from "../src/renderer/common/memory/memorySaverState";
import { getChromeAccent, NEUTRAL_CHROME_ACCENT } from "../src/renderer/common/theme/chromeTheme";
import { DataSettingsSection } from "../src/renderer/surfaces/panels/settings/components/DataSettingsSection";
import { GlobalSettingsSection } from "../src/renderer/surfaces/panels/settings/components/GlobalSettingsSection";
import { MemorySaverSection } from "../src/renderer/surfaces/panels/settings/components/MemorySaverSection";
import { SettingsSectionNav } from "../src/renderer/surfaces/panels/settings/components/SettingsSectionNav";
import { SpaceSettingsSection } from "../src/renderer/surfaces/panels/settings/components/SpaceSettingsSection";
import { WorkspaceManagementSection } from "../src/renderer/surfaces/panels/settings/components/WorkspaceManagementSection";

const panelsSettingsCss = readFileSync(join(__dirname, "../src/renderer/styles/panels-settings.css"), "utf8");

describe("settings panel sections", () => {
  it("renders section navigation for global, Space, data, and Spaces controls", () => {
    const html = renderToStaticMarkup(createElement(SettingsSectionNav, {
      activeSection: "global",
      onSelect: vi.fn()
    }));

    expect(html).toContain("Global");
    expect(html).toContain("Space");
    expect(html).toContain("Data");
    expect(html).toContain("Spaces");
    expect(html).toContain("Appearance, homepage, search");
  });

  it("renders global and Space settings as separate panes", () => {
    const state = createDefaultState();
    const globalHtml = renderToStaticMarkup(createElement(GlobalSettingsSection, {
      settings: state.settings,
      onUpdateSettings: vi.fn()
    }));
    const spaceHtml = renderToStaticMarkup(createElement(SpaceSettingsSection, {
      workspace: state.workspaces[0],
      searchEngine: state.settings.searchEngine,
      onUpdateWorkspace: vi.fn()
    }));

    expect(globalHtml).toContain('aria-label="Global settings"');
    expect(globalHtml).toContain("Chrome color");
    expect(globalHtml).toContain("Match current Space");
    expect(globalHtml).toContain("Search engine");
    expect(spaceHtml).toContain('aria-label="Space settings"');
    expect(spaceHtml).toContain("Workspace accent");
  });

  it("resolves neutral or Space-matched chrome accent", () => {
    const state = createDefaultState();
    const workspace = { ...state.workspaces[0], accent: "#7dd3fc" };

    expect(getChromeAccent(state.settings, workspace)).toBe(NEUTRAL_CHROME_ACCENT);
    expect(getChromeAccent({ ...state.settings, chromeAccentMode: "space" }, workspace)).toBe("#7dd3fc");
  });

  it("renders data and workspace management panes", () => {
    const dataHtml = renderToStaticMarkup(createElement(DataSettingsSection, {
      dataSummary: "2 history · 1 downloads · 0 permissions",
      importInputRef: createRef<HTMLInputElement>(),
      importStatus: null,
      memorySaver: {
        mountedWebviews: 3,
        protectedTabs: 1,
        reclaimableTabs: 2,
        sleepAfterMinutes: 30,
        sleepEnabled: true,
        sleepingTabs: 0,
        summary: "2 releasable · 0 sleeping · 1 protected"
      },
      onClearBrowsingData: vi.fn(),
      onClearProfile: vi.fn(),
      onExportBackup: vi.fn(),
      onImportBackup: vi.fn(),
      onRefreshProfileStorage: vi.fn(),
      onSleepInactiveTabs: vi.fn(),
      onUpdateMemorySaver: vi.fn(),
      profileStorageEntries: [],
      profileStorageError: null,
      profileStorageStatus: "ready"
    }));
    const workspaceHtml = renderToStaticMarkup(createElement(WorkspaceManagementSection, {
      activeWorkspaceId: "personal",
      onAddWorkspace: vi.fn(),
      onDeleteWorkspace: vi.fn(),
      workspaceCount: 2
    }));

    expect(dataHtml).toContain('aria-label="Data settings"');
    expect(dataHtml).toContain("Memory Saver");
    expect(dataHtml).toContain("Sleep inactive tabs");
    expect(dataHtml).toContain("Browser backup");
    expect(workspaceHtml).toContain('aria-label="Workspace management"');
    expect(workspaceHtml).toContain("2 spaces");
  });

  it("summarizes active Space memory saver state", () => {
    const state = createDefaultState();
    const workspace = state.workspaces[0];
    workspace.activeTabId = workspace.tabs[0].id;
    workspace.tabs.push({
      ...workspace.tabs[0],
      id: "background",
      title: "Background",
      url: "https://background.example",
      isPinned: false,
      isSleeping: false
    }, {
      ...workspace.tabs[0],
      id: "pinned",
      title: "Pinned",
      url: "https://pinned.example",
      isPinned: true,
      isSleeping: false
    }, {
      ...workspace.tabs[0],
      id: "sleeping",
      title: "Sleeping",
      url: "https://sleeping.example",
      isPinned: false,
      isSleeping: true
    });
    state.splitTabIds = ["pinned"];
    state.splitTabId = "pinned";
    state.splitMode = true;

    expect(getMemorySaverState(workspace, state)).toEqual({
      mountedWebviews: 3,
      protectedTabs: 2,
      reclaimableTabs: 1,
      sleepAfterMinutes: 30,
      sleepEnabled: true,
      sleepingTabs: 1,
      summary: "1 releasable · 1 sleeping · 2 protected"
    });
  });

  it("renders memory saver controls with releasable tab counts", () => {
    const onSleepInactiveTabs = vi.fn();
    const html = renderToStaticMarkup(createElement(MemorySaverSection, {
      memorySaver: {
        mountedWebviews: 4,
        protectedTabs: 2,
        reclaimableTabs: 1,
        sleepAfterMinutes: 15,
        sleepEnabled: true,
        sleepingTabs: 1,
        summary: "1 releasable · 1 sleeping · 2 protected"
      },
      onSleepInactiveTabs,
      onUpdateMemorySaver: vi.fn()
    }));

    expect(html).toContain('aria-label="Memory saver"');
    expect(html).toContain("Auto sleep after 15 min");
    expect(html).toContain("1 releasable · 1 sleeping · 2 protected");
    expect(html).toContain("Automatically sleep inactive tabs");
    expect(html).toContain('aria-label="Memory saver auto sleep delay"');
    expect(html).toContain("15m");
    expect(html).toContain('aria-pressed="true"');
    expect(html).toContain("<strong>4</strong>");
    expect(html).not.toContain("disabled");
  });

  it("styles the settings section navigation", () => {
    expect(panelsSettingsCss).toContain(".settings-section-nav");
    expect(panelsSettingsCss).toContain(".settings-section-tab[aria-pressed=\"true\"]");
    expect(panelsSettingsCss).toContain(".settings-pane");
    expect(panelsSettingsCss).toContain(".memory-saver-metrics");
    expect(panelsSettingsCss).toContain(".memory-saver-delay-options button[aria-pressed=\"true\"]");
  });
});
