# ADR-0014: Favorite Folders as Tab Metadata

- Status: Accepted
- Date: 2026-06-12
- Deciders: Architecture

## Context

Astra's sidebar features a favorites section with folders, similar to Arc Browser's favorites bar. The key architectural question is whether favorites and their folder hierarchy should be a separate data store (like Chrome bookmarks) or whether they are metadata attached to tabs.

Chrome bookmarks are a full Chromium subsystem with their own model, storage, sync, and UI. Favorites in Astra serve a different purpose: they mark which tabs appear in the sidebar's favorites section, organized into folders for quick access. Favorites are inherently tab-oriented -- a favorite is a tab, not a URL bookmark that can exist independently.

The design must answer:
- Where does "is this tab a favorite?" live?
- Where does folder hierarchy (name, order, parent) live?
- How does this relate to Chrome bookmarks (if at all)?
- How do favorites survive session restore?

## Decision

Favorites are **tab metadata** stored on `AstraTabFeatures` (`WebContentsUserData`), with folder hierarchy managed by `AstraFavoriteService` (a `ProfileKeyedService`). There are two layers:

**Per-tab favorite state (AstraTabFeatures):**
- `is_favorite` -- whether the tab appears in the favorites section.
- `favorite_folder_id` -- which folder the tab belongs to ("root" for top-level).
- `favorite_order_index` -- position within the folder.
- This state travels with the tab and is restored via Chromium's session restore pipeline (patch point: `chrome/browser/sessions/session_service.cc`).

**Folder hierarchy (AstraFavoriteService):**
- Profile-scoped keyed service that owns folder definitions (id, name, parent_id, order_index, expanded state).
- Folders form a tree rooted at an always-present "root" folder.
- Persisted via Chromium's `PrefService` (not a custom file).
- The service is the source of truth for folder structure; it never owns WebContents.

**Projection model:**
- The sidebar projects favorites by iterating `TabStripModel`, checking `AstraTabFeatures::is_favorite()`, and grouping by `favorite_folder_id`.
- Favorite state changes are observed indirectly via `TabStripModelObserver` and `AstraFavoriteServiceObserver`.

**Relationship to Chrome bookmarks:**
- Favorites and bookmarks are entirely separate systems.
- Chrome bookmarks are reused as-is (bookmark bar, bookmark manager, sync).
- A tab can be both a favorite (Astra) and have its URL bookmarked (Chromium) -- these are independent concepts.
- No sync between favorites and bookmarks; they serve different user needs.

## Consequences

Positive:

- Lightweight: favorite state is a few booleans and strings on each tab. No separate data store to maintain.
- Tab identity is preserved: favorites are tabs, not URLs. Closing a tab removes it from favorites naturally.
- Session restore works out of the box (once the session restore patch is in place) -- favorite state travels with the tab.
- Folders are simple metadata that can be persisted via PrefService, reusing Chromium's profile lifecycle.
- Clear separation from Chrome bookmarks -- no confusion, no sync complexity.

Negative:

- Favorite state is per-tab, not per-URL. If you open the same URL in two tabs, they have independent favorite states. This is intentional but differs from bookmarks.
- Querying "which tabs are in folder X" requires iterating all tabs in the profile (via `BrowserList` + `TabStripModel`). Acceptable for typical tab counts.
- Favorite state does not survive if a tab is closed and reopened from history -- only session restore preserves it.
- No built-in sync for favorite folders (would need a separate sync adapter, unlike bookmarks which sync natively).

Neutral:

- Folders are profile-scoped, not per-workspace. A tab's favorite folder membership is the same regardless of which workspace the tab is in.

## Alternatives Considered

### Favorites as Chrome bookmarks with a special "Favorites Bar" folder
Use Chrome's bookmark model, treating the favorites bar as a special folder.

- Rejected: Bookmarks are URL-based, not tab-based. A favorite tab has state (position, split view, workspace) that bookmarks do not track. The bookmark model would need to be extended with tab metadata, which is the wrong layer. Also, the sidebar's visual design and interactions (drag-to-favorite, folder grouping) differ from Chrome's bookmark bar UX.

### Separate Astra favorite data store with its own persistence
Build a full Astra favorite model similar to bookmarks but for tabs.

- Rejected: Overengineering. The favorite state is just a few fields per tab. Putting it in its own store with its own persistence, observers, and sync would duplicate infrastructure that WebContentsUserData + session restore already provides.

## References

- **Chromium subsystems reused:** `TabStripModel`, `WebContentsUserData`, `PrefService`, `ProfileKeyedServiceFactory`, `SessionService`
- **Astra components:** `AstraFavoriteService`, `AstraTabFeatures`, `AstraSidebarView` (projection)
- **Patch points:** Session restore metadata (`chrome/browser/sessions/session_service.cc`), profile keyed service registration (`chrome/browser/profiles/profile_keyed_service_factory*.cc`)
