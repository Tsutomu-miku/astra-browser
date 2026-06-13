# ADR-0018: Workspace Import / Export Format

- Status: Accepted
- Date: 2026-06-12
- Deciders: Architecture

## Context

Workspaces are a core Astra concept. Users may want to share workspaces, back them up, or transfer them between profiles or devices. This requires a serialization format for workspace data.

The format must capture:
- Workspace metadata: name, accent color, order index.
- Tab list: URLs, titles, pinned state, favorite state.
- Version number for future format evolution.
- Timestamp for reference.

Key decisions:
- What serialization format (JSON, binary, Protocol Buffers)?
- What data is included (only metadata vs. full tab state)?
- How are IDs handled on import (preserve vs. regenerate)?
- Security: what URL schemes are allowed in imports?

## Decision

Workspace data is serialized as **human-readable JSON** with a versioned schema. The format includes workspace metadata and tab URLs (not full tab session state). Import creates new tabs from URLs using Chromium's standard tab opening mechanisms.

**Schema (version 1):**

```json
{
  "version": 1,
  "exported_at": "2026-06-12T10:00:00Z",
  "workspaces": [
    {
      "id": "workspace-guid",
      "name": "Work",
      "accent_color": "#5B8FF9",
      "order_index": 0,
      "tabs": [
        {
          "title": "GitHub",
          "url": "https://github.com",
          "pinned": false,
          "favorite": true
        }
      ]
    }
  ]
}
```

**Export behavior:**
- `AstraWorkspaceImportExport::ExportWorkspacesToJson(profile)` iterates all browsers for the profile.
- Collects workspace list from `AstraWorkspaceService`.
- Collects tab data from each browser's `TabStripModel`, grouped by `workspace_id` from `AstraTabFeatures`.
- Returns a JSON string.
- Export is read-only -- never modifies state.

**Import behavior:**
- JSON is validated against the schema before any state changes.
- Imported workspaces receive **fresh IDs** (via `base::GenerateGUID`) to avoid collisions.
- The `replace_existing` flag controls whether existing non-default workspaces are deleted first.
- Tabs are opened using Chromium's standard `Browser::OpenURL` / `NavigateParams` path.
- Per-tab Astra metadata (`workspace_id`, favorite state) is set on each new tab's `AstraTabFeatures`.
- Invalid URLs are skipped; dangerous schemes (javascript:, data:, etc.) are rejected.
- Caps: max 50 workspaces, max 200 tabs per workspace.

**Validation:**
- `ValidateWorkspaceJson()` checks JSON structure, version, required fields, URL validity, and URL scheme safety.
- Validation is a separate step from import; only valid JSON proceeds to state mutation.

## Consequences

Positive:

- Human-readable: users can inspect, edit, and diff workspace files with standard tools.
- Portable: JSON works everywhere, no special tools needed.
- Secure: validation happens before any state changes; dangerous URL schemes are rejected.
- Simple: relies on `base::JSONReader` / `base::JSONWriter` from Chromium, no new dependencies.
- Versioned: schema version allows future evolution with backward compatibility.
- URL-based: avoids the complexity of serializing full session state (navigation history, form data, etc.).

Negative:

- Only URLs and basic metadata, not full tab state. Imported tabs lose back/forward history, scroll position, form data, and other session state. This is acceptable for a workspace "template" model but not for full session backup.
- Opening many tabs at once can be slow. A lazy-loading approach (like session restore) would improve performance for large imports.
- JSON is verbose compared to binary formats. Not a concern for typical workspace sizes (tens of tabs).
- No built-in compression. Large exports could optionally use gzip, but the default is plain JSON for readability.

Neutral:

- IDs are regenerated on import. This means re-importing the same file creates duplicate workspaces, not updates. A future "sync" mode could match by name.

## Alternatives Considered

### Full session restore format
Use Chromium's `SessionService` snapshot format (protobuf-like binary) for full tab state export.

- Rejected: Session service format is complex, version-specific to Chromium, and not designed for user exchange. Full session state (cookies, form data, post data) also has security and privacy implications for sharing. URL-based export is simpler, safer, and more portable.

### Binary format (Protocol Buffers / FlatBuffers)
Use a binary serialization format for efficiency.

- Rejected: Premature optimization. Workspace files are small (kilobytes, not megabytes). Human readability and toolability are more valuable than byte savings.

### Bookmark HTML format
Export as Netscape bookmark HTML (the standard bookmarks interchange format).

- Rejected: Bookmark HTML is designed for bookmarks, not workspaces with tab metadata. It does not capture workspace concept, accent color, tab order, or pinned state. JSON is more flexible and extensible.

## References

- **Chromium subsystems reused:** `base::JSONReader`, `base::JSONWriter`, `GURL`, `BrowserList`, `TabStripModel`, `Browser::OpenURL`, `NavigateParams`
- **Astra components:** `AstraWorkspaceImportExport`, `AstraWorkspaceService`, `AstraTabFeatures`
- **Patch points:** None (uses public Chromium APIs). Future lazy loading may integrate with `sessions::SessionRestore`.
