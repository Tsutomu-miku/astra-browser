# 0006 — Session restore metadata bridge

**Files patched:**
- `chrome/browser/sessions/base_session_service.cc` (tab save)
- `chrome/browser/sessions/session_restore.cc` (tab restore)
- `chrome/browser/sessions/tab_restore_service.cc` (closed tab restore)

**Size estimate:** ~20-30 lines total across 3 files

**Status:** planned

**Astra component:** `astra/browser/astra_session_restore_helper.*`,
`astra/browser/astra_session_metadata.*`

## Context

Chromium's session service is responsible for persisting browser state (windows,
tabs, navigation history) to disk and restoring it on startup or after a crash.
The key components are:

- **`sessions::BaseSessionService`** — base class that handles serializing
  session data (windows, tabs, navigations) to disk.  Both `SessionService`
  (full session save/restore) and `TabRestoreService` (recently closed tabs)
  derive from this.

- **`sessions::SessionRestore`** — orchestrates creating Browser windows and
  WebContents from saved session data on startup.

- **`sessions::TabRestoreService`** — manages the "reopen closed tab"
  (Ctrl+Shift+T / Cmd+Shift+T) stack, including restoring full windows.

- **`sessions::SessionTab`** and **`sessions::SerializedNavigationEntry`** —
  data structures that hold per-tab and per-navigation state for serialization.

Astra adds product-specific metadata to each tab (workspace ID, favorite state,
split view configuration, sidebar presentation).  This metadata must survive
browser restarts and tab restore operations.

**We do NOT replace or wrap Chromium's session service.**  Astra metadata is a
small extra dictionary that rides alongside Chromium's existing per-tab session
data.  The patch attaches this dictionary when saving and applies it when
restoring.

## Three tiers of Astra persistence

It is important to be clear about where each piece of Astra state lives:

### 1. Profile-level — stored in PrefService
Survives all sessions; shared across all windows of a profile.

- Workspace definitions (id, name, color, icon, order, created time).
- User preferences (default sidebar width, default split view orientation).
- Favorite folder definitions (if we ever have named folders beyond "root").

**Owner:** `AstraWorkspaceService` (ProfileKeyedService), `AstraFavoriteService`.
**Persistence path:** `PrefService` → profile `Preferences` file.

### 2. Window-level — stored in session window data
Per-window state that is restored for each browser window.

- Active workspace ID (which workspace the sidebar shows for this window).
- Sidebar visibility state (shown / hidden).
- Sidebar width (pinned width for this window).
- (Future: window-specific split view defaults.)

**Owner:** Astra window state (not yet implemented — currently in profile prefs).
**Persistence path:** Session service → `SessionWindow` extra data.
**TODO(astra):** Implement window-level session metadata.
**Patch point:** `BaseSessionService::SaveWindow()` / `SessionRestore` where
Browser objects are created.

### 3. Tab-level — stored in session tab data
Per-tab state that travels with each tab across save/restore cycles.

- Workspace ID (which workspace this tab belongs to).
- Favorite flag and favorite folder ID.
- Favorite order index.
- Sidebar pinned flag.
- Split view: is_in_split_view, partner_id, ratio, orientation.

**Owner:** `AstraTabFeatures` (WebContentsUserData).
**Persistence path:** Session service → `SessionTab` extra data.
**This patch implements tab-level metadata bridges.**

## Change — Tab save path

**File:** `chrome/browser/sessions/base_session_service.cc`
(or wherever `SessionTab` is populated from a `WebContents`/`NavigationController`)

**Where:** In the function that builds a `sessions::SessionTab` from a live
tab, or in the `TabRestoreService` function that captures tab state for the
"recently closed" stack.

### Before

```cpp
// Somewhere in BaseSessionService or TabRestoreService, when serializing a tab.
std::unique_ptr<sessions::SessionTab> tab =
    std::make_unique<sessions::SessionTab>();
tab->tab_id = ...;
tab->current_navigation_index = ...;
// ... navigations, pinned state, etc.
```

### After

```cpp
#include "astra/browser/astra_session_restore_helper.h"  // at top, guarded

// ... when building the SessionTab from WebContents ...

#if BUILDFLAG(IS_ASTRA_BRANDED)
  base::Value::Dict astra_metadata =
      astra::AstraSessionRestoreHelper::OnWillSaveTab(web_contents);
  if (!astra_metadata.empty()) {
    // Attach to the tab's extra_data or extended_data field.
    // TODO(astra): Use the correct field name on SessionTab.  If SessionTab
    // does not have a generic extra_data dict, we may need to add one via a
    // separate patch to sessions/session_tab.h, or piggyback on
    // SerializedNavigationEntry's extended_info map.
    tab->extra_data.Merge(std::move(astra_metadata));
  }
#endif
```

**Alternative save point:** If `SessionTab` does not support a generic extra
data dict, we can attach metadata to each `SerializedNavigationEntry` using
`SetExtendedInfoKey()`, which already exists for extension data.  The first
navigation entry (or the current one) would carry the Astra metadata.

## Change — Tab restore path

**File:** `chrome/browser/sessions/session_restore.cc`

**Where:** In the function that creates a WebContents for a restored tab and
inserts it into TabStripModel.  This is typically inside
`SessionRestore::RestoredTab` creation or the `RestoreTab` helper.

### Before

```cpp
// Creating WebContents for a restored tab.
content::WebContents::CreateParams create_params(profile);
create_params.initially_hidden = true;
std::unique_ptr<content::WebContents> web_contents =
    content::WebContents::Create(create_params);
// ... restore navigation state ...
tab_strip_model->AddWebContents(std::move(web_contents), ...);
```

### After

```cpp
// ... after WebContents is created but before it's inserted into the tab strip ...

#if BUILDFLAG(IS_ASTRA_BRANDED)
  // Apply Astra metadata from the session tab's extra data.
  // TODO(astra): Pull extra_data from the correct SessionTab field.
  const base::Value::Dict& astra_metadata = restored_tab.extra_data;
  if (astra::HasAstraMetadata(astra_metadata)) {
    astra::AstraSessionRestoreHelper::OnWillRestoreTab(
        profile, web_contents.get(), astra_metadata);
  }
#endif
```

## Change — Closed tab restore path

**File:** `chrome/browser/sessions/tab_restore_service.cc`

**Where:** In `TabRestoreService::RestoreEntryById()` or the internal helper
that creates WebContents from a `TabRestoreService::Tab` entry.

The pattern is identical to session restore: call
`AstraSessionRestoreHelper::OnWillRestoreTab()` with the metadata from the
saved tab entry.

```cpp
#if BUILDFLAG(IS_ASTRA_BRANDED)
  astra::AstraSessionRestoreHelper::OnWillRestoreTab(
      profile, web_contents.get(), entry->extra_data);
#endif
```

## Rationale

### Why patch the session service instead of building our own persistence?

1. **Correctness:** Chromium's session service handles crash recovery, lazy
   tab loading, tab discarding, incognito behavior, and multi-window
   coordination.  Reimplementing this would be a huge effort and inevitably
   have bugs.

2. **Lifecycle alignment:** Astra metadata describes tabs.  Tab lifecycle is
   owned by Chromium.  Metadata should travel with the tab's session entry,
   not be stored in a separate file that could get out of sync.

3. **Extension precedent:** Chromium already supports `SerializedNavigationEntry`
   extended info for extension data.  Our approach follows the same pattern.

### Why use base::Value::Dict as the data format?

- It's the standard flexible data container in Chromium.
- It serializes to JSON/Pickle trivially (matching how session data is stored).
- It's forward-compatible: we can add new fields without changing the patch.
- Unknown fields are ignored on read, supporting forward/backward compatibility.

### Why is AstraSessionRestoreHelper a static-method-only class?

- The helper has no state of its own — it just bridges between Chromium's
  session types and Astra's metadata types.
- Chromium patch points call into it as a plain function, which is simpler
  than finding or injecting a service instance.
- It is thin by design — all real logic is in `AstraTabFeatures` and the
  serialization functions in `astra_session_metadata.cc`.

## Build Flag

- Gate: `BUILDFLAG(IS_ASTRA_BRANDED)`
- All includes and calls guarded by `#if BUILDFLAG(IS_ASTRA_BRANDED)`
- Non-Astra builds are completely unaffected.

## Alternatives Considered

### 1. Store Astra tab metadata in PrefService
Store a map of tab_id → metadata in profile preferences.

- **Rejected:** Tab IDs are session-scoped and not stable across restarts
  (SessionTab IDs are assigned during session restore, not persisted).
  PrefService would require a separate stable identity scheme for tabs.
  Also, prefs don't participate in "reopen closed tab" naturally — the
  metadata would be lost when a tab is closed and then restored.

### 2. Use Tab Groups metadata as a storage vehicle
Piggyback on Chromium's tab groups feature, using group metadata fields to
store Astra workspace IDs.

- **Rejected:** Tab groups have their own semantics (visual grouping within
  a strip).  Using them as a generic metadata store would be an abuse and
  would conflict with actual tab group usage.  It also wouldn't cover all
  our fields (favorites, split view, etc.).

### 3. Store per-tab metadata in localStorage or IndexedDB in the renderer
Use the web platform's storage APIs per tab.

- **Rejected:** Renderer storage is per-origin, not per-tab.  Tabs navigate
  between origins, so the metadata would be lost.  Also, security boundaries
  make this unreliable.

### 4. Build a parallel AstraSessionService
Create our own service that watches TabStripModelObserver and persists
metadata to a custom file.

- **Rejected:** Duplicates session restore logic.  Timing issues during
  restore (Chromium creates tabs, then we apply metadata asynchronously)
  would cause UI flicker.  Also, crash recovery would be inconsistent —
  Chromium restores tabs but our service might not have saved the latest
  metadata.

## Risks & Rebase Concerns

- **SessionTab data model:** If `sessions::SessionTab` does not have a generic
  `extra_data` dictionary field, we may need a small additional patch to
  `sessions/session_tab.h` to add one.  This is still minimal (a single
  `base::Value::Dict` member and serialization code).

- **Alternative: SerializedNavigationEntry extended_info.**  This is a more
  stable attachment point because `SetExtendedInfoKey()` is a public API.
  The trade-off is that metadata is attached to a navigation entry rather
  than the tab itself, so we'd need to copy it to the current entry on each
  navigation.  **Recommendation:** use SessionTab extra_data if possible,
  fall back to SerializedNavigationEntry extended_info.

- **Stability:** The session service code in Chromium is relatively stable.
  Major refactors are rare, but the exact function signatures for tab save
  and restore do change occasionally.

- **Graceful degradation:** If the patch fails to apply, Astra metadata simply
  won't persist across restarts.  Tabs still restore correctly via Chromium's
  session service; only Astra-specific decorations (workspace, favorite, split
  view) are lost.

## Legacy session migration

For notes about migrating session data from the legacy prototype build to the
direct Chromium build, see `docs/session-restore-migration.md`.

Key principle: **session restore is a Chromium feature.**  Astra adds metadata
only.  We never store session data ourselves or build a parallel session
service.

## Related

- ADR: `docs/adr/0010-workspace-as-metadata-projection.md`
- Related patches: 0001 (browser main extra parts) — startup hook where
  session restore happens.
- Astra source:
  - `astra/browser/astra_session_metadata.*` — serialization functions.
  - `astra/browser/astra_session_restore_helper.*` — patch point bridge.
  - `astra/browser/astra_tab_features.*` — per-tab metadata storage.
  - `astra/browser/astra_workspace_service.*` — profile-level workspace
    definitions (persisted via PrefService, not session restore).
