# ADR-0026: Reading List Integration

- Status: Accepted
- Date: 2026-06-13
- Deciders: Architecture

## Context

Astra includes a reading list feature in the sidebar that shows pages the user has saved to read later. Chromium already has a built-in reading list feature (`components/reading_list/core/`) with its own model, storage, and sync support.

Key architectural questions:
- Does Astra build its own reading list or reuse Chromium's?
- Where does the reading list live in the architecture (service layer, UI projection)?
- How does the sidebar reading list view relate to Chromium's reading list model?
- Should reading list entries be tied to tabs or be independent URL entries?

## Decision

Astra **reuses Chromium's reading list model** (`components/reading_list/core/`) and projects it in the sidebar via `AstraReadListService`, which is a thin Astra service that adapts the Chromium reading list model for Astra's sidebar UI. Astra does not reimplement reading list storage or sync.

**Chromium subsystems reused:**
- `ReadingListModel` — the core reading list model with entries, storage, and sync.
- `ReadingListSyncService` — sync integration (if applicable).
- `PrefService` — reading list preferences.

**Astra service layer (AstraReadingListService):**
- Profile-scoped service that wraps `ReadingListModel`.
- Provides a simplified interface for the sidebar reading list view.
- Translates between Chromium reading list entry types and Astra's view model.
- Observes `ReadingListModel` for changes and notifies Astra observers.
- Does not own reading list data — it delegates to Chromium's model.
- Adds Astra-specific presentation metadata (e.g., sidebar display order, section grouping).

**Data model:**
- Reading list entries are URL-based (not tab-based), matching Chromium's model.
- Each entry has: URL, title, addition date, last update date, read status, estimated read time.
- Entries are independent of tabs — a reading list entry exists even if no tab is open for that URL.
- Adding the current page to reading list reads the active tab's URL and adds it to the Chromium model.

**UI projection (sidebar reading list view):**
- The sidebar reading list view reads from `AstraReadListService` (which reads from `ReadingListModel`).
- It shows entries grouped by read/unread status, or by date, or in a flat list.
- Clicking an entry opens the URL in a new tab (standard Chromium behavior).
- The view dispatches commands (add to reading list, mark as read, delete) through the service, which delegates to Chromium's model.
- UI is never the source of truth — all state lives in Chromium's `ReadingListModel`.

**Persistence and sync:**
- Reading list data is stored and synced by Chromium's reading list subsystem.
- Astra does not add custom persistence.
- If the user signs into Chrome, reading list entries sync across devices via Chromium sync.

## Consequences

Positive:
- Full reuse of Chromium's reading list infrastructure: storage, sync, model, change notifications.
- Reading list entries sync across devices automatically (via Chromium sync).
- No reimplementation of storage, CRUD operations, or sync logic.
- Consistent with the overall Astra philosophy: reuse Chromium, project in UI.
- The service layer is thin — mostly an adapter and observer bridge.

Negative:
- The reading list is URL-based, not tab-based. There is no direct link between a reading list entry and an open tab. If a user opens a reading list entry and the tab already has that URL, there is no automatic association.
- Custom Astra presentation metadata (if any) cannot be stored in the Chromium reading list model and would need separate storage.
- The reading list model API may change between Chromium versions, requiring rebase work on the adapter service.
- Astra-specific reading list features (e.g., workspaces) would need to be layered on top rather than modifying the core model.

Neutral:
- Reading list entries are independent of workspaces. A reading list entry is visible across all workspaces, matching Chromium's profile-scoped model.
- The sidebar reading list view is similar in structure to the bookmarks view — both project Chromium data.

## Alternatives Considered

### Build a custom Astra reading list
Implement a full Astra reading list service with its own storage and model.

- Rejected: Duplicates Chromium's entire reading list subsystem. Chromium already has a well-tested reading list with sync, storage, and UI patterns. Building a custom version would be a large effort with no product benefit.

### Reading list as tab metadata
Make reading list entries be a flag on tabs (like favorites), so only open tabs can be "read later" items.

- Rejected: Reading list is inherently about saving pages to read later, independent of whether the tab is open. Tabs come and go; reading list entries persist. A tab-based model would lose entries when tabs close, defeating the purpose of a reading list.

### Use Chrome bookmarks with a "Read Later" folder
Implement reading list on top of Chrome bookmarks, using a special folder.

- Rejected: Bookmarks and reading list serve different purposes. Bookmarks are for permanent organization; reading list is for temporary "read later" items with read/unread status. Chromium already has a dedicated reading list feature — we should use it.

### WebUI reading list page
Use Chromium's existing `chrome://reading-list` WebUI page instead of building a sidebar view.

- Considered: Chromium does have a reading list WebUI. However, the sidebar is the primary Astra UI surface for accessing features, and a sidebar reading list view provides quicker access and better integration with the overall sidebar UX. The WebUI page can still be accessible as a full-page view.

## References

- **Chromium subsystems reused:** `ReadingListModel`, `ReadingListSyncService`, `components/reading_list/core/`, `PrefService`
- **Astra components:** `AstraReadingListService` (adapter), `AstraSidebarReadingListView` (UI projection), `AstraReadingListItemView`
- **Patch points:** Profile keyed service registration (to hook into reading list model creation)
- **Related ADRs:** ADR-0011 (Sidebar Projection Model), ADR-0014 (Favorite Folders as Tab Metadata)
