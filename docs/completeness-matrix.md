# Completeness Matrix

This matrix tracks the implementation status of all features across service layer,
UI layer, tests, and integration.

**Status key:**
- ✅ Complete -- implemented and functional
- ⚠️ Partial -- scaffolding exists with partial implementation
- ❌ Missing -- not implemented or stub-only

---

## Core Features

| Feature | Service Layer | UI Layer | Tests | Integration | Notes |
|---------|--------------|----------|-------|-------------|-------|
| **Workspaces (Spaces)** | ⚠️ | ⚠️ | ⚠️ | ❌ | CRUD works via service, persistence via PrefService. UI sidebar defined but not wired. Tests exist but unverified. No TabStripModel projection validated. |
| **Tab metadata (AstraTabFeatures)** | ⚠️ | N/A | ⚠️ | ❌ | Rich metadata model defined. `WebContentsUserData` pattern used. Creation on tab creation not wired. Session restore persistence not implemented. |
| **Favorites / Favorite Folders** | ⚠️ | ⚠️ | ⚠️ | ❌ | Service has folder CRUD. Sidebar section views defined. No drag-and-drop implementation. Tests exist but unverified. |
| **Sidebar** | N/A | ⚠️ | ❌ | ❌ | 61 files of views infrastructure. Sections defined. Drag types defined. TabStripModel observer declared but incremental updates not implemented. No build verification. |
| **Split View** | ⚠️ | ❌ | ❌ | ❌ | Per-tab metadata exists. View + controller declared but implementation is skeleton. No actual WebContents layout. |
| **Glance / Peek** | ⚠️ | ❌ | ❌ | ❌ | Tab metadata defined. View + controller declared. No actual peek/preview implementation. Not integrated with tab hover cards. |
| **Command Palette** | ❌ | ⚠️ | ❌ | ❌ | Commands defined in delegate. UI views declared (bubble, item, model, view). Model likely doesn't actually enumerate commands or support search. |
| **Command System** | ⚠️ | N/A | ⚠️ | ❌ | Full command enum (40+ commands). ExecuteCommand switch statement. IsCommandEnabled checks. Accelerator table declared but not merged. No patch validation. |
| **Tab Stacks (named)** | ⚠️ | ❌ | ❌ | ❌ | Service declared. Sidebar stack views declared. No actual stack management implementation. |
| **Tab Stacks (hierarchical)** | ⚠️ | ❌ | ❌ | ❌ | Per-tab metadata (stack_parent_id, is_stack_collapsed). No service or controller for tree management. |

---

## Productivity Features

| Feature | Service Layer | UI Layer | Tests | Integration | Notes |
|---------|--------------|----------|-------|-------------|-------|
| **Focus Mode** | ⚠️ | ⚠️ | ⚠️ | ❌ | Service with toggle + duration + blocklist. Controller + indicator views declared. Tests exist. No actual distraction blocking or immersive mode integration. |
| **Memory Saver (tab suspend)** | ⚠️ | ❌ | ⚠️ | ❌ | Service with enabled flag + timeout + settings. Per-tab suspended state metadata. Tests exist. Not integrated with Chromium's TabManager / discard system. |
| **Picture-in-Picture (PiP)** | ⚠️ | ⚠️ | ⚠️ | ❌ | Service declared. Per-tab PiP metadata. Controls view declared. Tests exist. Not integrated with Chromium's PictureInPictureWindowController. |
| **Notes** | ⚠️ | ⚠️ | ⚠️ | ❌ | Service with note CRUD + persistence. Sidebar notes view + note editor view declared. Tests exist. Rich editor not implemented. |
| **Reading List** | ⚠️ | ⚠️ | ⚠️ | ❌ | Service projects Chromium's ReadingListModel. Sidebar reading list view + item view declared. Tests exist. Observer wiring not validated. |
| **Screenshot Capture** | ⚠️ | ⚠️ | ⚠️ | ❌ | Service with visible/full/region capture stubs. Capture bubble + region overlay views declared. Tests exist. Actual capture via WebContents not implemented. |
| **Workspace Import/Export** | ⚠️ | ⚠️ | ⚠️ | ❌ | Import/export module with JSON format. Dialog view declared. Tests exist. File dialog integration not verified. |
| **Recently Closed Tabs** | ⚠️ | ⚠️ | ❌ | ❌ | Helper wraps TabRestoreService. Sidebar view + item view declared. Restore all + restore one commands. Observer wiring not validated. |

---

## Browser Integration Features

| Feature | Service Layer | UI Layer | Tests | Integration | Notes |
|---------|--------------|----------|-------|-------------|-------|
| **DevTools Integration** | ⚠️ | ⚠️ | ⚠️ | ❌ | Helper with toggle + dock state. DevTools toolbar + workspace panel + integration coordinator declared. Unit tests exist. Patch 0015 covers dock state only; full panel integration not done. |
| **History** | ❌ | ⚠️ | ❌ | ❌ | No Astra service (correctly -- uses Chromium). Sidebar history view + item view declared. HistoryServiceObserver not wired. |
| **Downloads** | ❌ | ⚠️ | ❌ | ❌ | No Astra service (correctly -- uses Chromium). Sidebar downloads view + item view declared. DownloadManager observer not wired. |
| **Passwords** | ❌ | ⚠️ | ❌ | ❌ | No Astra service (correctly -- uses Chromium). Sidebar passwords view + item view declared. PasswordStore observer not wired. |
| **Extensions** | ❌ | ⚠️ | ❌ | ❌ | No Astra service (correctly -- uses Chromium). Sidebar extensions view + extension icon view + popup view declared. Extension registry observer not wired. |
| **Bookmarks** | ❌ | ⚠️ | ❌ | ❌ | No Astra service (correctly -- uses Chromium). Sidebar bookmarks view + bookmark item view declared. BookmarkModel observer not wired. |
| **Tab Search** | N/A | ⚠️ | ❌ | ❌ | Bubble + item view declared. No actual search/filter implementation. Uses TabStripModel data but projection not implemented. |
| **Tab Hover Preview** | N/A | ⚠️ | ❌ | ❌ | Peek controller + preview view declared. Not integrated with Chromium's TabHoverCardController. |
| **Omnibox Astra Actions** | ⚠️ | ⚠️ | ❌ | ❌ | Omnibox provider + action + manager declared. Location bar decoration view declared. Patch 0011 documented but not validated. |

---

## UI / Theming Features

| Feature | Service Layer | UI Layer | Tests | Integration | Notes |
|---------|--------------|----------|-------|-------------|-------|
| **Color System (AstraColorMixer)** | N/A | ✅ | ❌ | ❌ | Color IDs defined. Mixer implementation exists. Theme utils exist. Light/dark + accent color derivation. No unit tests. Not integrated with Chrome's ColorProvider (patch 0012). |
| **Theme Service** | ⚠️ | N/A | ❌ | ❌ | Service declared. Accent color + theme mode prefs. Uses ui/color dependency (architecture issue -- see TD-002). |
| **Accessibility Utilities** | N/A | ⚠️ | ⚠️ | N/A | Rich utility functions for accessible names, roles, states, focus management, live regions. Unit test is substantial. Not yet applied to all Astra views. |
| **Settings UI** | N/A | ⚠️ | ❌ | ❌ | Settings bubble + page view + section view + search box + search settings view declared. No actual settings editing wired to PrefService. |
| **New Tab Page (Views)** | N/A | ⚠️ | ❌ | ❌ | New tab bubble + view + shortcut view + workspace card declared. No actual most-visited or workspace data binding. |
| **New Tab Page (WebUI)** | ❌ | ⚠️ | ❌ | ❌ | WebUI config + handler + UI declared. Basic HTML/CSS/JS resources exist. No data binding to services. |
| **Profile Menu / Workspace Avatar** | N/A | ⚠️ | ❌ | ❌ | Profile menu controller + workspaces + avatar button + header/footer views declared. Not integrated with Chrome's profile menu. |
| **Workspace Overview** | N/A | ⚠️ | ❌ | ❌ | Overview controller + view + card view declared. No actual workspace card rendering or drag-and-drop. |

---

## Startup / Infrastructure

| Feature | Service Layer | UI Layer | Tests | Integration | Notes |
|---------|--------------|----------|-------|-------------|-------|
| **BrowserMainExtraParts** | N/A | N/A | ❌ | ❌ | Class declared. Lifecycle hooks (PostCreateThreads, PreProfileInit, etc.) stubs. Patch 0001 documented. Not validated in real build. |
| **ContentBrowserClient** | N/A | N/A | ❌ | ❌ | Class declared. Hook stubs for web prefs, URL policy. Patch 0013 documented. |
| **Main Delegate** | N/A | N/A | ❌ | ❌ | Class declared. Pre-sandbox hooks. Patch 0014 documented. |
| **Feature List** | ⚠️ | N/A | ❌ | ❌ | Feature flags declared (sidebar, split view, workspaces, etc.). Not integrated with Chrome's feature list system. |
| **Accelerator Table** | N/A | N/A | ❌ | ❌ | Table + registrar declared. Patch 0007 documented. Not merged into Chrome's accelerator table. |
| **Branding** | N/A | N/A | ❌ | ❌ | Brand constants + version header declared. Patch 0005 documented. Not applied. |
| **Prefs Registration** | ⚠️ | N/A | ❌ | ❌ | 20+ pref keys defined with defaults. `RegisterProfilePrefs` function exists. Not wired into Chromium's pref registration pipeline. |
| **Keyed Service Factories** | ⚠️ | N/A | ❌ | ❌ | 10+ factory classes defined. `RegisterAstraProfileKeyedServices` stub. Not wired into Chrome's factory registration. |

---

## Summary

### By Layer

| Layer | ✅ Complete | ⚠️ Partial | ❌ Missing |
|-------|-----------|-----------|-----------|
| Service layer | 0 | 19 | 5 |
| UI layer | 1 | 22 | 5 |
| Tests | 0 | 5 | 21 |
| Integration | 0 | 0 | 27 |

### Overall

- **Total features tracked:** 35
- **Complete:** 1 (color system mixer implementation)
- **Partial:** 29
- **Missing:** 5

### Key Observations

1. **No feature is fully end-to-end complete.** Every feature has at least one
   dimension (service, UI, tests, integration) that is missing or stub-level.

2. **Service layer is furthest along.** 19 of 24 service-layer features are
   partial (have implementation beyond just declarations).

3. **Integration is the biggest gap.** 27 of 35 features have no verified
   integration with Chromium. All 15 patches are "planned" status.

4. **Tests are partially defined but never executed.** 5 test targets have
   source files, but none have been compiled or run against real Chromium.

5. **Color system is the most complete single module.** It has implementation,
   follows Chromium patterns, and has clear integration points. It just lacks
   tests and the final patch application.

6. **"Helper" services are mostly stubs.** `AstraExtensionHelper`,
   `AstraPasswordHelper`, `AstraSearchEngineHelper`, `AstraRecentTabsHelper`,
   and `AstraDevToolsHelper` wrap Chromium subsystems but have minimal
   implementation beyond declarations.
