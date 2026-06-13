# Session Restore Migration Notes

## Context

The legacy Electron-based prototype had its own session storage system, built
on top of Electron's `session` API plus custom JSON files stored in userData.

The direct Chromium build replaces this entirely with Chromium's native
session service.  Astra only adds per-tab and per-window metadata that rides
alongside Chromium's session data.

## Migration Decision: No automatic migration

**We do NOT migrate session data from the legacy prototype to the Chromium build.**

Rationale:

1. **Different profile formats.**  Electron stores session state in a
   completely different format from Chromium's `SessionService`.  There is
   no straightforward mapping.

2. **Fresh start.**  The direct Chromium build is a new product with a new
   profile.  Users start with clean profiles, matching what they'd get from
   any Chromium-based browser on first launch.

3. **Scope control.**  Building a migration tool for what is essentially a
   prototype-to-production transition is not worth the effort.  The legacy
   prototype was for exploration and validation; real users start fresh.

## If migration is ever needed

If a future product requirement demands importing tabs/workspaces from the
legacy prototype, the approach would be:

1. **Standalone import utility**, not part of the browser runtime.
   A one-time tool or first-run wizard step.

2. **What would be imported:**
   - Tab URLs and titles → create new tabs with navigations.
   - Workspace definitions → write to `AstraWorkspaceService` prefs.
   - Tab-to-workspace membership → write Astra metadata per tab.
   - Favorites → write favorite metadata per tab.

3. **What would NOT be imported:**
   - Back/forward history beyond the current URL (too complex).
   - Cached resources, cookies, storage (profile isolation).
   - Saved passwords, form data (use Chromium's import from other browsers
     instead, if needed).
   - Split view state (would need both tabs to exist and be re-linked).

4. **Implementation sketch:**
   - Read the legacy prototype's session JSON file.
   - For each workspace, create an `AstraWorkspace` entry in the new profile.
   - For each tab, restore the URL as a new navigation entry and attach
     Astra metadata via the session restore bridge.
   - Write everything into the Chromium profile's session directory.

This is explicitly out of scope for the initial direct Chromium build.

## Architecture principle

Session restore is a **Chromium feature**.  Astra adds metadata.

- Chromium owns: `SessionService`, `TabRestoreService`, session persistence,
  session restore orchestration, crash recovery, lazy tab loading.
- Astra owns: extra per-tab metadata (workspace, favorite, split view, etc.),
  extra per-window metadata (active workspace, sidebar state), serialization
  format, metadata application logic.

We never reimplement session restore, never store session data in custom
files, and never bypass Chromium's session pipeline.
