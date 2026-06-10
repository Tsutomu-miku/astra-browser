import { createElement, createRef } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { readFileSync } from "node:fs";
import { join } from "node:path";
import { describe, expect, it, vi } from "vitest";

import { buildMemoryBreakdown } from "../src/renderer/common/memory/memoryUsage";
import { createDefaultState } from "../src/renderer/domain/browser";
import { getMemorySaverState } from "../src/renderer/common/memory/memorySaverState";
import { getChromeAccent, NEUTRAL_CHROME_ACCENT } from "../src/renderer/common/theme/chromeTheme";
import { DataSettingsSection } from "../src/renderer/surfaces/panels/settings/components/DataSettingsSection";
import { GlobalSettingsSection } from "../src/renderer/surfaces/panels/settings/components/GlobalSettingsSection";
import { MemorySaverSection } from "../src/renderer/surfaces/panels/settings/components/MemorySaverSection";
import { SettingsSectionNav } from "../src/renderer/surfaces/panels/settings/components/SettingsSectionNav";
import { SpaceSettingsSection } from "../src/renderer/surfaces/panels/settings/components/SpaceSettingsSection";
import { WorkspaceManagementSection } from "../src/renderer/surfaces/panels/settings/components/WorkspaceManagementSection";
import { AutofillSettingsSection } from "../src/renderer/surfaces/panels/settings/components/autofill/AutofillSettingsSection";
import type { PasswordEntry } from "../src/renderer/domain/browser";
import { PasswordEditorDialog } from "../src/renderer/surfaces/panels/settings/components/autofill/PasswordEditorDialog";

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
    expect(html).toContain("Workspaces");
    expect(html).toContain("Appearance");
    expect(html).toContain("Autofill and passwords");
    expect(html).toContain("Privacy and security");
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
    const state = createDefaultState();
    const memoryBreakdown = buildMemoryBreakdown(state.workspaces, new Map(), null);
    const dataHtml = renderToStaticMarkup(createElement(DataSettingsSection, {
      dataSummary: "2 history · 1 downloads · 0 permissions",
      importInputRef: createRef<HTMLInputElement>(),
      importStatus: null,
      memoryBreakdown,
      memoryError: null,
      memoryHistory: [],
      memorySaver: {
        mountedWebviews: 3,
        protectedTabs: 1,
        reclaimableTabs: 2,
        sleepAfterMinutes: 30,
        sleepEnabled: true,
        sleepingTabs: 0,
        summary: "2 releasable · 0 sleeping · 1 protected"
      },
      memoryStatus: "ready",
      onClearBrowsingData: vi.fn(),
      onClearProfile: vi.fn(),
      onExportBackup: vi.fn(),
      onImportBackup: vi.fn(),
      onRefreshMemory: vi.fn(),
      onRefreshProfileStorage: vi.fn(),
      onSleepInactiveTabs: vi.fn(),
      onUpdateMemorySaver: vi.fn(),
      profileStorageEntries: [],
      profileStorageError: null,
      profileStorageStatus: "ready",
      bookmarksImportStatus: null,
      bookmarksImportInputRef: createRef<HTMLInputElement>(),
      onImportBookmarks: vi.fn()
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
    expect(dataHtml).toContain('aria-label="Memory usage"');
    expect(dataHtml).toContain("Memory usage");
    expect(workspaceHtml).toContain('aria-label="Workspace management"');
    expect(workspaceHtml).toContain("2 spaces");
  });

  it("summarizes active Space memory saver state", () => {
    const state = createDefaultState();
    const workspace = state.workspaces[0];
    workspace.tabs = workspace.tabs.filter((tab) => !tab.isFavorite);
    workspace.favoriteOrder = [];
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
    expect(panelsSettingsCss).toContain(".memory-usage-section");
    expect(panelsSettingsCss).toContain(".memory-workspace-list");
  });

  it("renders the Autofill panel with vault controls, password list, and search", () => {
    const passwords: PasswordEntry[] = [
      { id: "1", origin: "https://github.com", username: "octocat", encryptedPassword: "x.y", notes: "", usedAt: 0, createdAt: 0, updatedAt: 0 },
      { id: "2", origin: "https://example.com", username: "alice", encryptedPassword: "a.b", createdAt: 0, updatedAt: 0 }
    ];
    const html = renderToStaticMarkup(createElement(AutofillSettingsSection, {
      autofill: { passwords, addresses: [], paymentMethods: [] },
      onAddPassword: vi.fn(),
      onEditPassword: vi.fn(),
      onRevealPassword: async () => "plain",
      onRemovePassword: vi.fn(),
      passwordVaultUnlocked: true,
      onUnlockVault: vi.fn(),
      onLockVault: vi.fn(),
      passwordSearchQuery: "",
      setPasswordSearchQuery: vi.fn(),
      onAddAddress: vi.fn(),
      onRemoveAddress: vi.fn(),
      onAddPaymentMethod: vi.fn(),
      onRemovePaymentMethod: vi.fn()
    }));

    expect(html).toContain('aria-label="Autofill and passwords"');
    expect(html).toContain("Vault unlocked");
    expect(html).toContain("octocat");
    expect(html).toContain("alice");
    expect(html).toContain("Reveal password");
    expect(html).toContain("Copy password");
    expect(html).toContain("Search saved passwords…");
  });

  it("filters Autofill passwords by search query and hides vault unlock when locked", () => {
    const passwords: PasswordEntry[] = [
      { id: "1", origin: "https://github.com", username: "octocat", encryptedPassword: "x.y", createdAt: 0, updatedAt: 0 },
      { id: "2", origin: "https://mail.example.com", username: "alice", encryptedPassword: "a.b", createdAt: 0, updatedAt: 0 }
    ];
    const html = renderToStaticMarkup(createElement(AutofillSettingsSection, {
      autofill: { passwords, addresses: [], paymentMethods: [] },
      onAddPassword: vi.fn(),
      onEditPassword: vi.fn(),
      onRevealPassword: async () => null,
      onRemovePassword: vi.fn(),
      passwordVaultUnlocked: false,
      onUnlockVault: vi.fn(),
      onLockVault: vi.fn(),
      passwordSearchQuery: "github",
      setPasswordSearchQuery: vi.fn(),
      onAddAddress: vi.fn(),
      onRemoveAddress: vi.fn(),
      onAddPaymentMethod: vi.fn(),
      onRemovePaymentMethod: vi.fn()
    }));
    expect(html).toContain("Vault locked");
    expect(html).toContain("octocat");
    expect(html).not.toContain("alice");
  });

  it("renders the password editor dialog", () => {
    const html = renderToStaticMarkup(createElement(PasswordEditorDialog, {
      entry: { id: "", origin: "https://new.test", username: "", encryptedPassword: "", createdAt: 0, updatedAt: 0 },
      onClose: vi.fn(),
      onSave: async () => null
    }));
    expect(html).toContain("Add password");
    expect(html).toContain("Origin / URL");
    expect(html).toContain("Password");
    expect(html).toContain("password-editor-dialog");
  });

  it("styles the autofill + password editor", () => {
    expect(panelsSettingsCss).toContain(".autofill-list");
    expect(panelsSettingsCss).toContain(".vault-status-row");
    expect(panelsSettingsCss).toContain(".password-inline-reveal");
    expect(panelsSettingsCss).toContain(".password-editor-backdrop");
    expect(panelsSettingsCss).toContain(".password-editor-header");
  });
});
