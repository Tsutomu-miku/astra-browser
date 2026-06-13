# ADR-0024: Notes Feature Architecture

- Status: Accepted
- Date: 2026-06-13
- Deciders: Architecture

## Context

Astra includes a notes feature that lets users take notes while browsing. Notes can be linked to a specific URL (per-page notes), linked to a workspace, or stand alone. Notes appear in the sidebar notes section and can be edited in a sidebar note editor.

Key architectural questions:
- Where does note data live (service layer vs. UI layer)?
- How are notes persisted?
- How do notes relate to tabs, URLs, and workspaces?
- Should notes reuse any Chromium subsystem (e.g., bookmarks, reading list)?

## Decision

Notes are implemented as a **profile-scoped Astra service** (`AstraNoteService`, a `ProfileKeyedService`) with note data stored in `PrefService`. The sidebar notes view projects note state from the service; it never owns note data.

**Service layer (AstraNoteService):**
- Profile-scoped `KeyedService` that owns all note metadata.
- Each note has: id, title, content, url, workspace_id, timestamps, color.
- Notes are stored as a list of dictionaries in `PrefService` under a dedicated pref key.
- All mutations (Add, Update, Delete) persist immediately and notify observers.
- Query methods: `GetAllNotes()`, `GetNote(id)`, `GetNotesForUrl(url)`, `GetNotesForWorkspace(id)`, `SearchNotes(query)`.
- Observer pattern for UI to react to changes.

**Data model:**
- Notes are URL-linked, not tab-linked. A note is associated with a URL pattern, not with a specific `WebContents`. Multiple tabs with the same URL share the same note.
- Notes can optionally be linked to a workspace (workspace_id filter).
- Notes have independent identity (id) from tabs or URLs.
- Note identity is not URL equality — a note is a distinct entity that can reference a URL.

**Persistence:**
- Notes are stored in `PrefService` as a list of dictionaries.
- This is simple and works for small-to-medium note counts (hundreds of notes).
- If scale becomes a concern, migration to LevelDB or a dedicated storage backend is possible.

**UI layer:**
- Sidebar notes view reads from `AstraNoteService` and displays a list of notes.
- Note editor view (`AstraNoteEditorView`) reads from the service and dispatches update commands back to the service.
- UI is never the source of truth — all state lives in the service.

**Relationship to Chromium:**
- Notes are an Astra product feature. There is no direct Chromium equivalent (bookmarks are URL-based and different purpose, reading list is for "read later").
- The service does not replace any Chromium subsystem; it is purely Astra-specific metadata.
- `PrefService` is reused for persistence.

## Consequences

Positive:
- Clean separation: service owns state, UI projects it. Standard Chromium pattern.
- `ProfileKeyedService` ensures correct lifecycle with profile creation/destruction.
- Persistence via `PrefService` means notes participate in profile lifecycle and can be controlled by policy.
- URL-linking means notes survive tab close/reopen and work across multiple tabs with the same URL.
- Observer pattern allows multiple UI surfaces (sidebar, editor, overview) to stay in sync.

Negative:
- `PrefService` list-of-dicts storage is not optimal for large note collections or full-text search. A dedicated storage backend would be more efficient at scale.
- No built-in sync for notes (unlike bookmarks). Sync would require a separate sync adapter.
- URL-linking adds complexity: matching a note to the current tab requires URL comparison, and different URLs on the same site may not match.

Neutral:
- Notes are profile-scoped, not per-workspace. A note can belong to zero or one workspace.
- Search is simple substring matching. Full-text search with indexing can be added later.

## Alternatives Considered

### Notes as a Chrome extension (chrome.storage)
Build notes as a built-in extension using `chrome.storage.local` or `chrome.storage.sync`.

- Rejected: Adds extension IPC overhead and requires bundling a built-in extension. The notes feature is core product functionality, not an extension. Using a `KeyedService` in the browser process is more efficient and integrated.

### Notes stored as bookmark annotations
Piggyback on Chrome bookmarks, using the bookmark model with extended metadata for notes.

- Rejected: Bookmarks and notes serve different user needs. Bookmarks are for URL organization; notes are for free-form text associated with pages. Mixing them would confuse both concepts and add complexity to the bookmark model.

### Reading list extension / reuse
Build notes on top of Chromium's reading list feature.

- Rejected: Reading list is for "read later" URLs with a specific workflow. Notes are free-form text annotations. Different data model and user intent.

### Per-tab notes (stored on AstraTabFeatures)
Store notes as per-tab metadata on `AstraTabFeatures` (`WebContentsUserData`).

- Rejected: Notes should survive tab close/reopen and be accessible across tabs with the same URL. Per-tab storage would lose notes when tabs close and would duplicate notes for the same URL across tabs.

## References

- **Chromium subsystems reused:** `ProfileKeyedServiceFactory`, `PrefService`, `base::Time`, `GURL`
- **Astra components:** `AstraNoteService`, `AstraNoteServiceObserver`, `AstraSidebarNotesView`, `AstraNoteEditorView`
- **Patch points:** Profile keyed service registration (`chrome/browser/profiles/profile_keyed_service_factory*.cc`)
- **Related ADRs:** ADR-0010 (Workspace as Metadata Projection), ADR-0011 (Sidebar Projection Model)
