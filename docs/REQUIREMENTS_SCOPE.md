# Astra Browser — Requirements & Scope Definition

> **Core principle:** Astra is a productivity layer on top of Chromium.
> We never reimplement what Chromium already provides.
> We only build what Chromium doesn't have and what makes Astra uniquely valuable.

This document defines the product scope: what we build, what we reuse from Chromium,
and what we explicitly do NOT build from scratch.

---

## 1. Architecture Summary

```
┌─────────────────────────────────────────────────────┐
│  Astra UI Layer (Views)                              │
│  ├── Astra-only surfaces (sidebar, split view, ...)  │
│  └── Projection surfaces (history, downloads, ...)   │
├─────────────────────────────────────────────────────┤
│  Astra Service Layer (ProfileKeyedService)           │
│  ├── Astra-only metadata (workspaces, favorites, ...)│
│  └── Projection helpers (wrap Chromium services)     │
├─────────────────────────────────────────────────────┤
│  Chromium (chrome / content / components / ui/views) │
│  Everything browser-standard: tabs, navigation,      │
│  history, bookmarks, downloads, passwords, extensions│
│  DevTools, WebUI, autofill, safe browsing, updates   │
└─────────────────────────────────────────────────────┘
```

**Projection pattern:** For Chromium-owned data (history, bookmarks, downloads,
passwords, extensions), Astra provides alternative UI surfaces but never owns
the data or the business logic. Data always flows from Chromium → Astra projection,
never the reverse.

---

## 2. Chromium-Native Features (REUSE, don't rebuild)

These features are built into Chromium. Astra does NOT reimplement their backend logic.
Astra may provide alternative UI "projections" (e.g. a sidebar panel for bookmarks)
but the data source, state management, and core functionality remain Chromium-owned.

### 2.1 Core Browser Infrastructure

| Feature | Chromium Owner | Astra Role |
|---------|---------------|------------|
| TabStripModel / WebContents ownership | `chrome/browser/ui/tabs` | Add metadata via `WebContentsUserData` |
| NavigationController | `content/browser` | None — use as-is |
| Profile management | `chrome/browser/profiles` | Add ProfileKeyedServices |
| Session restore | `chrome/browser/sessions` | Hook to persist Astra metadata |
| Window management | `chrome/browser/ui/browser.h` | Install Astra views into BrowserView |

### 2.2 User Data

| Feature | Chromium Owner | Astra Role |
|---------|---------------|------------|
| Bookmarks | `components/bookmarks` | Sidebar projection (read-only + write via BookmarkModel API) |
| History | `components/history` | Sidebar projection (read-only) |
| Downloads | `chrome/browser/download` | Sidebar + shelf projection (read + control via DownloadItem API) |
| Password Manager | `components/password_manager` | Sidebar projection (read-only) |
| Extensions | `extensions/browser` | Sidebar projection + popup host |
| Reading List | `components/reading_list` | Sidebar projection (use ReadingListModel API) |
| Autofill | `components/autofill` | None — use as-is |
| Form data / saved addresses | `components/autofill` | None — use as-is |

### 2.3 UI Surfaces (Chromium Built-in)

These UI surfaces are built into Chromium and should NOT be reimplemented by Astra.
Astra may augment with additional features but the base surface stays Chromium.

| Surface | Chromium Location | Astra Role |
|---------|-------------------|------------|
| Main toolbar | `chrome/browser/ui/views/toolbar` | None — use as-is (may add Astra decoration) |
| Tab strip | `chrome/browser/ui/views/tabs` | None — use as-is (may add Astra metadata indicators) |
| Omnibox / location bar | `chrome/browser/ui/views/location_bar` | Add Astra action chip (patch 0011) |
| Tab search | `chrome/browser/ui/views/tab_search` | **Do not reimplement.** May add Astra-specific filters. |
| Find bar | `chrome/browser/ui/views/find_bar` | **Do not reimplement.** |
| Info bars | `chrome/browser/ui/views/infobars` | **Do not reimplement.** |
| Permission prompts | `chrome/browser/permissions` | **Do not reimplement.** |
| App menu / 3-dot menu | `chrome/browser/ui/views/toolbar/app_menu` | Add Astra menu items (patch 0003) |
| Page info bubble | `chrome/browser/ui/views/page_info` | **Do not reimplement.** |
| Downloads bubble | `chrome/browser/ui/views/download/bubble` | May add Astra-style refresh, but keep Chromium logic |
| Settings page | `chrome/browser/ui/webui/settings` | **Do not reimplement.** Astra settings live in own panel. |
| History page | `chrome/browser/ui/webui/history` | **Do not reimplement.** Sidebar projection is enough. |
| Bookmarks manager | `chrome/browser/ui/webui/bookmarks` | **Do not reimplement.** Sidebar projection is enough. |
| DevTools | `third_party/devtools-frontend` | Add Astra workspace panel (patch 0015). |
| Context menus | `chrome/browser/renderer_context_menu` | Add Astra menu items. |
| Zoom controls | `chrome/browser/ui` | **Do not reimplement.** |
| Status bar | `chrome/browser/ui/views/status_icons` | May style but don't reimplement. |
| Autofill popup | `chrome/browser/ui/views/autofill` | **Do not reimplement.** |

### 2.4 Platform Features

| Feature | Chromium Owner | Astra Role |
|---------|---------------|------------|
| Safe Browsing | `components/safe_browsing` | None — use as-is |
| Extensions (MV3) | `extensions/browser` | None — use as-is |
| Update / auto-update | `chrome/browser/upgrade` | None — use as-is |
| Policy / enterprise | `components/policy` | None — use as-is |
| Spell check | `components/spellcheck` | None — use as-is |
| Printing | `chrome/browser/printing` | None — use as-is |
| Media / WebRTC | `content/browser` | None — use as-is |
| Picture-in-Picture | `chrome/browser/picture_in_picture` | May extend with Astra controls, but base PiP is Chromium. |

---

## 3. Astra-Unique Features (BUILD — these are our value add)

These features do not exist in Chromium. They are the reason Astra exists.
All of these are fair game for building out.

### 3.1 Workspace & Tab Organization (Core Value)

**Priority: P0 — highest**

| Feature | Description | Chromium Equivalent |
|---------|-------------|---------------------|
| **Workspaces (Spaces)** | Color-coded workspace containers. Tabs belong to workspaces. Switching workspace filters the tab strip (projection, not reparenting). | None (Chrome has profiles but different semantics) |
| **Workspace Switcher** | Quick switcher at top of sidebar / keyboard shortcut | None |
| **Workspace Overview** | Grid view of all workspaces, drag tabs between them | None (Arc-style) |
| **Workspace Templates** | Pre-configured workspace templates (e.g. "Coding", "Research") | None |
| **Favorite Folders** | Pin important tabs in a Favorites section. Folders inside favorites. | Bookmarks are close but different: favorites are live tabs, not links |
| **Tab Stacks (Arc-style)** | Group tabs into named stacks in the sidebar. Expand/collapse. | Tab groups are similar, but stacks are sidebar-first and hierarchical |
| **Tab Ancestry / Tree** | Visualize parent-child tab relationships (who opened whom) | None (Chrome has tab groups but not tree) |
| **Smart Grouping** | Auto-group tabs by domain, topic, or workflow | None |

### 3.2 Layout & Productivity (Core Value)

**Priority: P0 — highest**

| Feature | Description | Chromium Equivalent |
|---------|-------------|---------------------|
| **Split View** | Split the content area into two WebContents side by side. Multiple layouts (50/50, 70/30, vertical, horizontal). | None (Chrome has tab groups but not split content area) |
| **Split Layout Picker** | Quick UI to choose split layout | None |
| **Glance / Peek** | Hover or shortcut to temporarily preview a tab/link in a side panel without switching tabs. | None (Arc-style peek) |
| **Focus Mode** | Distraction-free browsing: hides toolbar, tabs, sidebar. Site blocklist. Pomodoro-style timer. | None (Chrome has fullscreen but different purpose) |
| **Focus Session History** | Track focus sessions: duration, sites visited, productivity score. | None |
| **Command Palette** | Quick command search (Ctrl+K / Cmd+K). Searches Chrome commands + Astra commands + tabs + bookmarks. | Chrome has tab search (Ctrl+Shift+A) but not general command palette |
| **Sidebar** | Left sidebar with tabs grouped by workspace. Primary Astra UI surface. | Chrome has side panel but it's single-panel and limited |
| **Sidebar Stack View** | Collapsible tab stacks within sidebar | None |

### 3.3 Tab Metadata & Annotations

**Priority: P1 — high**

| Feature | Description | Chromium Equivalent |
|---------|-------------|---------------------|
| **Tab Notes** | Attach notes to tabs. Notes persist across sessions. | None |
| **Tab Labels / Tags** | Color-coded labels on tabs. Filter by label. | None (tab groups are close but different UX) |
| **Tab Features Metadata** | Per-tab Astra state: workspace_id, is_favorite, stack_id, split_config, etc. | None (Chrome has TabUserData but it's for internal use) |
| **Reading Progress** | Track pages you've read, reading streaks, weekly stats. | None (reading list exists but not progress tracking) |
| **Tab Rules / Auto Actions** | Auto-apply actions when a tab matches rules (e.g. auto-move to workspace, auto-hibernate, auto-add to favorites). | None |

### 3.4 Tab Lifecycle Management

**Priority: P1 — high**

| Feature | Description | Chromium Equivalent |
|---------|-------------|---------------------|
| **Tab Sleep / Hibernation** | Auto-suspend inactive tabs to save memory. Per-tab suspended state. | Chrome has memory saver (discards tabs) but less visible/controllable |
| **Tab Duplicate Finder** | Find and merge duplicate tabs. | None |
| **Tab Snooze** | Snooze a tab — it closes and reopens at a scheduled time. | None (similar to read-later but timed) |
| **Tab Suggestions** | Smart tab suggestions: continue where you left off, related tabs, morning routine, etc. | None (Chrome has "resume" but not smart suggestions) |
| **Session Manager / Snapshots** | Save and restore window sessions (workspace snapshots). | Chrome has session restore but not manual snapshots |
| **Tab Sharing** | Share tabs via various channels (email, SMS, social, workspace). | None (Chrome has "send to your devices" but limited) |

### 3.5 Content & Reading

**Priority: P2 — medium**

| Feature | Description | Chromium Equivalent |
|---------|-------------|---------------------|
| **Reader Mode** | Distraction-free reading view. Themes, font size, focus mode. | Chrome has reader mode (Distill page) but less feature-rich |
| **Screenshot Capture** | Visible area / full page / region capture. Annotation. | Chrome has partial screenshot (partial capture in share menu) |
| **Quick Actions** | Context-aware quick actions on pages (translate, read later, screenshot, share). | Chrome has some in share menu |

### 3.6 Astra System Features

**Priority: P1 — high**

| Feature | Description | Chromium Equivalent |
|---------|-------------|---------------------|
| **Astra Command System** | Unified command ID system for all Astra actions. Accelerator table. | None (extends Chrome command system) |
| **Astra Color System** | AstraColorMixer with Astra-specific color IDs, accent color, light/dark mode. | Extends Chrome ColorProvider |
| **Astra Branding** | Product name, icons, about dialog. | Chromium branding replaced |
| **Astra Keyed Services** | Profile-scoped service factory registration. | Extends Chrome's KeyedService framework |
| **Preferences / Settings** | Astra-specific settings. | Extends Chrome PrefService |

---

## 4. Explicit "Do Not Build" List

To prevent scope creep and duplicated work, here is what we explicitly
**do not build from scratch**:

### ❌ Do NOT reimplement these UIs

These exist in Chromium and we use them as-is (or with light styling):

- ❌ **Full settings page** — use `chrome://settings`
- ❌ **Full history page** — use `chrome://history`, sidebar projection is enough
- ❌ **Full bookmarks manager** — use `chrome://bookmarks`, sidebar projection is enough
- ❌ **Full downloads page** — use `chrome://downloads`, sidebar/bubble projection is enough
- ❌ **Full extensions page** — use `chrome://extensions`, sidebar projection is enough
- ❌ **Full passwords page** — use `chrome://password-manager`, sidebar projection is enough
- ❌ **Tab search bubble** — use Chromium's built-in tab search
- ❌ **Find bar** — use Chromium's built-in find
- ❌ **Zoom controls** — use Chromium's built-in zoom
- ❌ **Info bars** — use Chromium's built-in info bars
- ❌ **Permission prompts** — use Chromium's built-in prompts
- ❌ **Autofill popup** — use Chromium's built-in autofill
- ❌ **Page info bubble** — use Chromium's built-in page info
- ❌ **App menu (3-dot)** — add Astra items, don't replace
- ❌ **Main toolbar** — add Astra decorations, don't replace
- ❌ **Tab strip** — add Astra indicators, don't replace
- ❌ **Omnibox** — add Astra actions, don't replace
- ❌ **Context menu** — add Astra items, don't replace

### ❌ Do NOT reimplement these services

- ❌ **Bookmark storage** — use `BookmarkModel`
- ❌ **History storage** — use `HistoryService`
- ❌ **Download storage** — use `DownloadManager`
- ❌ **Password storage** — use `PasswordStore`
- ❌ **Cookie storage** — use `CookieStore` / `StoragePartition`
- ❌ **Extensions registry** — use `ExtensionRegistry`
- ❌ **Tab model** — use `TabStripModel`
- ❌ **Profile system** — use `Profile`
- ❌ **Navigation** — use `NavigationController`
- ❌ **Safe browsing** — use `SafeBrowsingService`
- ❌ **Auto-update** — use `UpgradeDetector`
- ❌ **Policy engine** — use `PolicyService`

---

## 5. Priority Roadmap (Astra-Unique Only)

### Phase 0: Foundation ✅ Complete
- Direct Chromium architecture decision
- Overlay directory structure
- Patch queue definition

### Phase 1: Core Astra Experience (in progress)
1. **Workspace system** — service + metadata + persistence
2. **Sidebar** — primary UI surface with tab projection
3. **Command system** — all Astra commands + accelerators
4. **Color system** — AstraColorMixer integration
5. **BrowserView integration** — install sidebar into Chrome window

### Phase 2: Productivity Features
1. **Split View** — dual WebContents layout
2. **Command Palette** — unified command + tab search
3. **Focus Mode** — distraction-free browsing
4. **Tab Stacks** — named tab groups in sidebar
5. **Favorite Folders** — pinned tab folders

### Phase 3: Tab Intelligence
1. **Tab Notes** — per-tab annotations
2. **Tab Rules** — auto-actions on tab patterns
3. **Smart Grouping** — auto-organize tabs
4. **Tab Suggestions** — AI-assisted tab suggestions
5. **Tab Ancestry** — tab tree visualization

### Phase 4: Content Features
1. **Glance / Peek** — tab preview
2. **Reader Mode** — distraction-free reading
3. **Screenshot Capture** — full-page + region
4. **Reading Progress** — reading tracking
5. **Quick Actions** — context-aware actions

### Phase 5: Tab Lifecycle
1. **Tab Sleep** — memory management
2. **Session Manager** — save/restore snapshots
3. **Tab Snooze** — timed tab reopening
4. **Tab Duplicates** — find and merge
5. **Tab Sharing** — share tabs across channels

---

## 6. Validation: How We Know We're On Track

To ensure we're not duplicating Chromium work:

1. **Architecture check** — `pnpm check:architecture` runs on every PR
2. **Patch size rule** — All Chromium patches are < 50 lines. Logic lives in `//astra`.
3. **Service ownership rule** — If a feature's data lives in Chromium, we don't
   create an Astra "service" for it — we create a "helper" or "projection".
4. **UI projection rule** — If Chromium has a full WebUI page for something,
   we may add a compact sidebar projection but we don't rebuild the full page.

---

## 7. Current State Audit

As of current development (35 batches of views):

**✅ Correctly Astra-unique (building these is right):**
- Workspaces / spaces
- Sidebar (as projection surface)
- Split view
- Glance / peek
- Focus mode
- Command palette
- Tab stacks
- Favorite folders
- Tab notes
- Tab rules
- Tab labels
- Tab ancestry
- Smart grouping
- Tab suggestions
- Tab sleep / memory saver
- Session manager / snapshots
- Tab snooze
- Tab duplicates
- Tab sharing
- Reader mode (enhanced, beyond Chrome's basic distill)
- Reading progress
- Screenshot capture (enhanced)
- Quick actions
- Color system (Astra extensions)
- Appearance panel (Astra theme + density)
- Keyboard shortcuts panel (Astra-specific reference)
- Tab containers (multi-account style identity isolation)

**⚠️ Projection surfaces (OK to build, but must wrap Chromium services):**
- History sidebar view — wraps `HistoryService`
- Bookmarks sidebar view — wraps `BookmarkModel`
- Downloads sidebar + shelf — wraps `DownloadManager`
- Passwords sidebar view — wraps `PasswordStore`
- Extensions sidebar view — wraps `ExtensionRegistry`
- Reading list sidebar view — wraps `ReadingListModel`
- Recently closed tabs — wraps `TabRestoreService`
- Profile menu / avatar — wraps `Profile`
- DevTools integration panel — wraps DevTools frontend

**❌ Potentially duplicating Chromium (should review):**
- Full settings page — Chromium has `chrome://settings`
  - _Current status: `astra_settings_page_view` exists. Should this exist?_
  - _Recommendation: Keep only Astra-specific settings panel. Delegate Chrome
    settings to `chrome://settings`._
- History page view — Chromium has `chrome://history`
  - _Current status: `astra_history_page_view` exists._
  - _Recommendation: Remove full page view. Sidebar projection is sufficient._
- Bookmarks manager page — Chromium has `chrome://bookmarks`
  - _Current status: `astra_bookmarks_manager_view` exists._
  - _Recommendation: Remove full page view. Sidebar projection is sufficient._
- Downloads page — Chromium has `chrome://downloads`
  - _Current status: `astra_downloads_page_view` exists._
  - _Recommendation: Remove full page view. Sidebar + bubble is sufficient._
- Extensions page — Chromium has `chrome://extensions`
  - _Current status: `astra_extensions_page_view` exists._
  - _Recommendation: Remove full page view. Sidebar projection is sufficient._
- Password manager page — Chromium has `chrome://password-manager`
  - _Current status: `astra_password_manager_view` exists._
  - _Recommendation: Remove full page view. Sidebar projection is sufficient._
- Tab search — Chromium has native tab search
  - _Current status: `astra_tab_search_bubble` exists._
  - _Recommendation: Keep but add Astra-specific filters (by workspace, label,
    note content). Don't replace the Chrome tab search._
- Find bar — Chromium has built-in find
  - _Current status: `astra_find_bar_view` exists._
  - _Recommendation: Remove. Use Chromium's find bar._
- Info bar — Chromium has native info bars
  - _Current status: `astra_info_bar_view` exists._
  - _Recommendation: Remove. Use Chromium's info bar system._
- Zoom button / controls — Chromium has native zoom
  - _Current status: `astra_zoom_button` exists._
  - _Recommendation: Remove. Use Chromium's zoom controls._
- Permission prompt — Chromium has native prompts
  - _Current status: `astra_permission_prompt_view` exists._
  - _Recommendation: Remove. Use Chromium's permission system._
- Page info bubble — Chromium has native page info
  - _Current status: `astra_page_info_bubble` exists._
  - _Recommendation: Remove. Use Chromium's page info._
- Site settings page — Chromium has site settings
  - _Current status: `astra_site_settings_view` + model exists._
  - _Recommendation: Remove. Use Chromium's site settings._
- Omnibox popup — Chromium has omnibox
  - _Current status: `astra_omnibox_popup_view` exists._
  - _Recommendation: Remove. Use Chromium's omnibox. Add Astra actions via
    OmniboxProvider._
- Autofill popup — Chromium has autofill
  - _Current status: `astra_autofill_popup_view` exists._
  - _Recommendation: Remove. Use Chromium's autofill._
- App menu — Chromium has app menu
  - _Current status: `astra_app_menu_view` exists._
  - _Recommendation: Remove full replacement. Add Astra items via menu model._
- Status bar — Chromium has status bar
  - _Current status: `astra_status_bar_view` exists._
  - _Recommendation: Remove or restyle only. Don't reimplement logic._
- Title bar — Chromium has title bar
  - _Current status: `astra_title_bar_view` exists._
  - _Recommendation: Remove or restyle only. Don't reimplement logic._
- Tab context menu — Chromium has context menus
  - _Current status: `astra_tab_context_menu_view` exists._
  - _Recommendation: Don't replace. Add Astra items via menu model._
- Reading list page — Chromium has reading list
  - _Current status: `astra_reading_list_page_view` exists._
  - _Recommendation: Remove full page. Sidebar projection is sufficient._
- Clear browsing data — Chromium has clear browsing data dialog
  - _Current status: `astra_clear_browsing_data_views_unittest.cc` (test only)._
  - _Recommendation: Remove. Use Chromium's dialog._
- New tab page (WebUI + Views) — Chromium has NTP
  - _Current status: Both WebUI and Views NTP exist._
  - _Recommendation: Keep ONE NTP implementation (Views) with Astra-specific
    content (workspaces, shortcuts). Don't rebuild the whole thing._
- Notes page — this is Astra-unique content
  - _Current status: `astra_notes_page_view` exists._
  - _Recommendation: Keep. This is Astra-unique metadata._
- Tab groups page — Chromium has tab groups
  - _Current status: `astra_tab_groups_page_view` exists._
  - _Recommendation: Remove. Use Chromium's tab groups. Our sidebar projection
    is enough._
- Performance dashboard — Chromium has task manager
  - _Current status: `astra_performance_dashboard_view` exists._
  - _Recommendation: Repurpose as Astra-specific performance (tab sleep stats,
    memory saved). Don't replace task manager._
- Health dashboard — somewhat duplicative of performance
  - _Current status: `astra_browser_health_view` exists._
  - _Recommendation: Merge with performance dashboard. Keep as Astra-specific
    health score (memory, tabs, focus time)._
- Browser health page — duplicate of health dashboard
  - _Recommendation: Consolidate._

**Summary of files to consider removing:**
- ~15 view files that duplicate Chromium WebUI pages
- ~5 view files that duplicate Chromium UI surfaces
- Potential to remove ~20% of views code while keeping all Astra-unique value

---

## 8. Next Actions

Based on this audit:

1. **Stop building** full-page replacements for Chromium WebUI pages
2. **Focus on** Astra-unique features (workspaces, split view, focus, tab stacks, notes, etc.)
3. **Audit and remove** duplicative view files (optional — low priority, but good hygiene)
4. **Align all batches** with this requirements document
5. **Prioritize integration** over more views — get the existing unique features wired into Chromium

---

_This document is the source of truth for "what Astra builds vs what Chromium provides."
When in doubt, check this document. If a feature isn't listed in §3 (Astra-Unique),
it probably shouldn't be built from scratch._
