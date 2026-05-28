import { createElement, createRef } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { readFileSync } from "node:fs";
import { join } from "node:path";
import { describe, expect, it, vi } from "vitest";

import { createDefaultState } from "../src/renderer/domain/browser";
import { DataSettingsSection } from "../src/renderer/surfaces/panels/settings/components/DataSettingsSection";
import { GlobalSettingsSection } from "../src/renderer/surfaces/panels/settings/components/GlobalSettingsSection";
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
    expect(html).toContain("Homepage, search, startup");
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
    expect(globalHtml).toContain("Search engine");
    expect(spaceHtml).toContain('aria-label="Space settings"');
    expect(spaceHtml).toContain("Workspace accent");
  });

  it("renders data and workspace management panes", () => {
    const dataHtml = renderToStaticMarkup(createElement(DataSettingsSection, {
      dataSummary: "2 history · 1 downloads · 0 permissions",
      importInputRef: createRef<HTMLInputElement>(),
      importStatus: null,
      onClearBrowsingData: vi.fn(),
      onClearProfile: vi.fn(),
      onExportBackup: vi.fn(),
      onImportBackup: vi.fn(),
      onRefreshProfileStorage: vi.fn(),
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
    expect(dataHtml).toContain("Browser backup");
    expect(workspaceHtml).toContain('aria-label="Workspace management"');
    expect(workspaceHtml).toContain("2 spaces");
  });

  it("styles the settings section navigation", () => {
    expect(panelsSettingsCss).toContain(".settings-section-nav");
    expect(panelsSettingsCss).toContain(".settings-section-tab[aria-pressed=\"true\"]");
    expect(panelsSettingsCss).toContain(".settings-pane");
  });
});
