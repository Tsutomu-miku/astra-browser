# Code Structure

## Active Directories

```text
chromium/astra/
  BUILD.gn
    Single entry for Astra direct Chromium build targets.

  app/
    Startup hooks and Chromium patch helpers:
    astra_browser_main_extra_parts.*   — BrowserMainParts integration
    astra_content_browser_client.*     — ContentBrowserClient override
    astra_main_delegate.*              — ChromeMainDelegate integration
    astra_feature_list.*               — Astra feature flag definitions
    astra_brand.*                      — Product branding strings
    astra_accelerator_table.*          — Astra keybinding table
    astra_accelerator_registrar.*      — Accelerator registration
    astra_version.h                    — Version constants

  common/
    Shared types, enums, constants, and lightweight utilities.
    Bottom of the Astra dependency graph. Depends only on //base, //skia, //ui/gfx.

      astra_workspace_types.*        — Workspace ID, info struct, accent color enum
      astra_tab_types.h              — Split view state, tab feature flags
      astra_command_constants.h      — Command ID range, accelerator string IDs
      astra_ui_constants.h           — Sidebar dimensions, spacing, radii, icon sizes

  browser/
    Product services and metadata that Chromium does not own.
    All services use ProfileKeyedService for profile-scoped state,
    or WebContentsUserData for per-tab metadata.

    Core product services:
      astra_workspace_service.*        — Spaces / workspace metadata
      astra_workspace_window_manager.* — Per-window workspace state
      astra_workspace_import_export.*  — JSON workspace import/export
      astra_favorite_service.*         — Favorite folder hierarchy
      astra_tab_features.*             — Per-tab Astra metadata (WebContentsUserData)
      astra_tab_stack_service.*        — Tab stack / tree organization
      astra_command_delegate.*         — Astra-only command dispatch
      astra_window_features.*          — Per-window Astra metadata

    Productivity features:
      astra_note_service.*             — Notes (URL-linked, workspace-linked)
      astra_reading_list_service.*     — Reading list projection
      astra_focus_mode_service.*       — Focus / distraction-free mode
      astra_memory_saver_service.*     — Tab suspend / memory saver policy
      astra_pip_service.*              — Picture-in-Picture state tracking
      astra_screenshot_service.*       — Screenshot capture + metadata
      astra_new_tab_page_service.*     — New tab page content

    Chromium helper services (projection / integration):
      astra_session_metadata.*         — Session restore metadata format
      astra_session_restore_helper.*   — Session restore bridge
      astra_omnibox_provider.*         — Astra omnibox AutocompleteProvider
      astra_omnibox_manager.*          — Omnibox action coordinator
      astra_omnibox_action.*           — Omnibox action definitions
      astra_accessibility_service.*    — Accessibility service integration
      astra_incognito_handler.*        — Incognito mode Astra behavior
      astra_prefs.*                    — Shared pref key registration
      astra_devtools_helper.*          — DevTools integration helper
      astra_extension_helper.*         — Extension integration helper
      astra_password_helper.*          — Password manager integration helper
      astra_recent_tabs_helper.*       — Recent tabs integration helper
      astra_search_engine_helper.*     — Search engine integration helper

  ui/views/
    Chromium Views UI additions. All UI code reads models and dispatches
    commands; it is never the source of truth for product state.

    astra_browser_view.*              — BrowserView augmentation controller
    astra_focus_mode_controller.*     — Focus mode UI controller
    astra_focus_mode_indicator.*      — Focus mode indicator view

    sidebar/
      astra_sidebar_view.*             — Main sidebar container
      astra_sidebar_item_view.*        — Base sidebar item view
      astra_sidebar_section_view.*     — Collapsible sidebar section
      astra_sidebar_bookmarks_view.*   — Bookmarks sidebar section
      astra_sidebar_history_view.*     — History sidebar section
      astra_sidebar_downloads_view.*   — Downloads sidebar section
      astra_sidebar_extensions_view.*  — Extensions sidebar section
      astra_sidebar_passwords_view.*   — Passwords sidebar section
      astra_sidebar_notes_view.*       — Notes sidebar section
      astra_sidebar_reading_list_view.* — Reading list sidebar section
      astra_sidebar_recently_closed_view.* — Recently closed sidebar section
      astra_sidebar_tab_groups_view.*  — Tab groups sidebar section
      astra_sidebar_stack_child_view.* — Stack child item view
      astra_sidebar_stack_header_view.* — Stack header view
      astra_sidebar_drag_types.h       — Sidebar drag-and-drop types
      astra_sidebar_drop_indicator_view.* — Sidebar drop indicator
      astra_workspace_switcher_view.*  — Workspace switcher in sidebar

    split_view/
      astra_split_view.*               — Split view container
      astra_split_view_controller.*    — Split view layout controller

    glance/
      astra_glance_view.*              — Glance / peek bubble view
      astra_glance_view_controller.*   — Glance lifecycle controller

    command_palette/
      astra_command_palette_view.*     — Command palette main view
      astra_command_palette_bubble.*   — Command palette bubble delegate
      astra_command_palette_model.*    — Command search model
      astra_command_palette_item_view.* — Command palette result item

    workspace/
      astra_workspace_card_view.*      — Workspace card (overview)
      astra_workspace_overview_view.*  — Workspace overview container
      astra_workspace_overview_controller.* — Overview controller
      astra_workspace_import_export_dialog.* — Import/export dialog

    newtab/
      astra_new_tab_view.*             — New tab page view
      astra_new_tab_bubble.*           — New tab bubble/shortcut
      astra_ntp_shortcut_view.*        — NTP shortcut tile
      astra_ntp_workspace_card.*       — NTP workspace card

    omnibox/
      astra_location_bar_decoration.*  — Location bar Astra decoration

    profiles/
      astra_profile_menu_controller.*  — Profile menu workspace integration
      astra_profile_menu_workspaces.*  — Workspace entries in profile menu
      astra_workspace_avatar_button.*  — Workspace avatar button

    tab_search/
      astra_tab_search_bubble.*        — Tab search bubble
      astra_tab_search_item_view.*     — Tab search result item

    tab_hover/
      astra_tab_hover_preview_view.*   — Tab hover preview
      astra_tab_hover_peek_controller.* — Tab hover peek controller

    screenshot/
      astra_screenshot_capture_bubble.* — Screenshot capture bubble
      astra_screenshot_region_overlay.* — Screenshot region selection overlay

    pip/
      astra_pip_controls_view.*        — PiP custom controls view

    settings/
      astra_settings_bubble.*          — Settings bubble
      astra_settings_page_view.*       — Settings page view
      astra_search_settings_view.*     — Search settings view

    devtools/
      astra_devtools_integration.*     — DevTools extension coordinator
      astra_devtools_toolbar.*         — Astra DevTools toolbar buttons
      astra_devtools_workspace_panel.* — Workspace inspector panel

  ui/color/
    Astra ColorProvider mixer and color IDs. Extends Chromium's color system.

    astra_color_ids.h              — Astra-specific ColorId constants
    astra_color_mixer.*            — Color mixer function (light/dark + accent)

  ui/accessibility/
    Accessibility utility functions for Astra views:
    astra_accessibility_utils.*       — AX helpers, focus management, keyboard nav

  build/
    Build configuration:
    buildflags.gni                     — Astra build flag definitions
    astra_buildflags.h                 — Build flag header

  patches/
    Human-readable patch queue notes.
    Each patch is tiny and delegates to //astra immediately.
    See patches/README.md for the full patch table.

  test/
    Test infrastructure documentation.
    Test files live alongside source (*_unittest.cc, *_browsertest.cc).

docs/
  Active direct Chromium docs, ADRs, standards, and roadmap.

docs/adr/
  Architecture Decision Records for active decisions.

docs/legacy/electron-prototype/
  Historical Electron prototype docs only. Not active architecture.

scripts/
  Bootstrap, build, and architecture check scripts:
  check-architecture.mjs              — Architecture guardrail script

src/
  Legacy Electron prototype. No new direct Chromium architecture here.
```

## Placement Rules

- Put Chromium startup hooks in `chromium/astra/app`.
- Put Profile-keyed product metadata in `chromium/astra/browser`.
- Put per-tab product metadata on `content::WebContentsUserData`.
- Put UI widgets in `chromium/astra/ui/views`.
- Put Chromium source patch notes in `chromium/astra/patches`.
- Put durable decisions in `docs/adr`.
- Put agent-facing constraints in `AGENTS.md` and
  `docs/ENGINEERING_STANDARDS.md`.

## Naming Rules

- Prefix Astra C++ classes with `Astra`.
- Prefix files with `astra_`.
- Use Chromium naming and ownership conventions in C++.
- Use Chromium TODO style: `TODO(astra): ...`.
- Never use TODOs as vague placeholders. Name the Chromium component or patch
  point that should own the future work.

## File Size Rules

- Keep Astra overlay files small enough to review in one sitting.
- Split by Chromium ownership boundary, not by arbitrary helper extraction.
- If a file starts mirroring a Chromium subsystem, stop and route to the
  Chromium subsystem instead.

## Review Checklist

Before merging a change, answer:

- Does this reuse Chromium's existing service instead of replacing it?
- Is the state truth source in Chromium or in an Astra product metadata object?
- Is UI code only rendering or delegating commands?
- Is the Chromium patch point minimal?
- Can another agent identify the owner directory from the file path alone?
