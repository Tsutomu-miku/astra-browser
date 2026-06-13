# 0009 — Profile menu workspace section

**Patch ID:** 0009
**File:** `chrome/browser/ui/views/profiles/profile_menu_view.cc`
**Size estimate:** ~15 lines
**Status:** planned
**Astra component:** `astra/ui/views/profiles/`

## Context

Chrome's profile menu (top-right, avatar button) is the natural place for
user/identity-oriented controls.  Astra adds workspace switching in this
area since workspaces are user-level organization that fits alongside
profile selection, sync status, and sign-in state.

We patch `ProfileMenuView` to insert an Astra workspace section between
the profile identity header and the rest of the menu items.  The section
shows:
- Current workspace name and accent color
- List of all workspaces (click to switch)
- "New workspace" action

This is a projection-only UI — all workspace state lives in
`AstraWorkspaceService` (a `ProfileKeyedService`).  The patch itself just
adds the view; all logic is in `//astra`.

**Chromium class:** `ProfileMenuView`
  (`chrome/browser/ui/views/profiles/profile_menu_view.h`)
  - The main profile menu bubble view.
  - Builds menu sections in `BuildBody()`.

**Related Chromium classes:**
- `ProfileMenuViewController` — controller for the profile menu
- `AvatarToolbarButton` — the toolbar button that opens the menu

## Change

### Files to patch

**Primary: `chrome/browser/ui/views/profiles/profile_menu_view.cc`**

Add an include at the top of the file:

```cpp
#if BUILDFLAG(IS_ASTRA_BRANDED)
#include "astra/ui/views/profiles/astra_profile_menu_controller.h"
#include "astra/ui/views/profiles/astra_profile_menu_workspaces.h"
#endif
```

### Before (in ProfileMenuView::BuildBody or similar)

```cpp
void ProfileMenuView::BuildBody() {
  // ... profile identity header ...

  // Sync info section.
  AddSyncInfo();

  // Menu items (Manage your Google Account, Passwords, ...)
  AddMenuItems();

  // ... rest of menu ...
}
```

### After

```cpp
void ProfileMenuView::BuildBody() {
  // ... profile identity header ...

#if BUILDFLAG(IS_ASTRA_BRANDED)
  // Astra workspace section.
  // The workspace controller is obtained from the browser's AstraBrowserView
  // or created per-menu-instance.  The view is added as a child of this menu.
  // TODO(astra): Obtain the controller from BrowserView's
  //   profile_menu_controller() instead of creating a local one.
  auto workspace_controller =
      std::make_unique<astra::AstraProfileMenuController>(browser());
  AstraProfileMenuWorkspaces* workspaces_view =
      workspace_controller->GetWorkspacesView();
  AddChildView(workspace_controller.release()->GetWorkspacesView());
  // Note: ownership transfer needs careful handling.  In practice, the
  // controller should live on AstraBrowserView and we just get the view.
#endif

  // Sync info section.
  AddSyncInfo();

  // Menu items (Manage your Google Account, Passwords, ...)
  AddMenuItems();

  // ... rest of menu ...
}
```

**Alternative (simpler):** Add the workspace section after the sync info
and before the main menu items.  The exact insertion point depends on the
desired visual hierarchy — workspaces are product-level, so they should
sit near the top of the menu.

### Secondary: `chrome/browser/ui/views/toolbar/avatar_toolbar_button.cc`

An alternative or complementary patch adds a workspace accent color badge
to the avatar button itself.  This gives a persistent visual indicator
of the current workspace without opening the menu.

```cpp
#if BUILDFLAG(IS_ASTRA_BRANDED)
#include "astra/ui/views/profiles/astra_workspace_avatar_button.h"
#endif
```

In the avatar button's paint or layout, overlay a small accent color
ring or badge.  Or, replace the avatar button entirely with
`AstraWorkspaceAvatarButton` (which wraps the avatar and adds the badge).

## Rationale

**Why the profile menu?**
- Workspaces are a user/identity concept — they fit naturally with
  profile switching and account management.
- The profile menu is already a discoverable, well-understood UI surface.
- Adding the workspace section here keeps the toolbar uncluttered
  (no extra button needed).

**Why not a separate toolbar button?**
- An extra button adds visual noise to the toolbar.
- The profile menu is already the "user identity" entry point.
- However, a separate `AstraWorkspaceAvatarButton` is also available as
  an alternative (see secondary patch above) for stronger visual
  differentiation between workspaces.

**Astra code this delegates to:**
- `astra/ui/views/profiles/astra_profile_menu_controller.h` — controller
  that manages workspace UI state and observes `AstraWorkspaceService`.
- `astra/ui/views/profiles/astra_profile_menu_workspaces.h` — the view
  showing the workspace list and "new workspace" action.
- `astra/browser/astra_workspace_service.h` — truth source for all
  workspace state (ProfileKeyedService).

## Build Flag

- **Gate:** `BUILDFLAG(IS_ASTRA_BRANDED)`
- **Build flag defined in:** `build/config/chrome_build.gni` (see patch 0003)
- **Astra sources built by:** `//astra/ui/views:views` (see BUILD.gn)

## Alternatives Considered

1. **Separate workspace button in the toolbar**
   - Pros: More discoverable, dedicated UI surface.
   - Cons: Clutters the toolbar, duplicates the profile menu concept.
   - Verdict: We provide `AstraWorkspaceAvatarButton` as an optional
     alternative, but the primary integration is the profile menu.

2. **Sidebar-only workspace switching**
   - Pros: Already implemented (AstraWorkspaceSwitcherView in sidebar).
   - Cons: Not discoverable when sidebar is hidden; doesn't fit the
     "user identity" mental model as well.
   - Verdict: Keep sidebar switcher as a secondary surface; profile menu
     is the primary entry point.

3. **Replace the profile menu entirely with an Astra version**
   - Pros: Full control over layout and behavior.
   - Cons: Massive patch, high rebase cost, loses all Chromium profile
     menu features (sync, sign-in, etc.).
   - Verdict: Rejected — we augment, don't replace.

4. **Use a bubble anchored to the avatar button (not embedded)**
   - Pros: Simpler patch — just show a separate bubble on click.
   - Cons: Two bubbles stacking visually, inconsistent UX.
   - Verdict: Use the embedded approach if possible; fall back to a
     separate bubble if embedding is too complex.

## Risks & Rebase Concerns

- **Profile menu layout changes:** The profile menu has undergone
  several redesigns (avatar menu, profile menu, etc.).  The exact
  method names and structure of `ProfileMenuView::BuildBody()` may
  change between Chromium versions.
  - Mitigation: Keep the patch minimal — just insert one view.
    The exact line may shift but the insertion concept stays the same.

- **`ProfileMenuViewController` ownership:** The controller pattern
  may change.  We should prefer getting the controller from
  `AstraBrowserView` rather than creating one per menu instance.
  - Mitigation: Document the preferred integration pattern and use
    the simpler approach first.

- **Theme/styling mismatches:** If Chromium changes the profile menu
  visual style, our embedded section may look out of place.
  - Mitigation: Use Chromium color IDs and layout constants; avoid
    hardcoded Astra-specific styling values in the view.

## Related

- ADR: (none yet — consider adding an ADR for workspace UX)
- Related patches: 0002 (BrowserView install), 0006 (command forwarding)
- Astra source:
  - `astra/ui/views/profiles/astra_profile_menu_controller.cc`
  - `astra/ui/views/profiles/astra_profile_menu_workspaces.cc`
  - `astra/ui/views/profiles/astra_workspace_avatar_button.cc`
  - `astra/browser/astra_workspace_service.cc`
