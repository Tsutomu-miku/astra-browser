#ifndef ASTRA_BROWSER_ASTRA_PREFS_H_
#define ASTRA_BROWSER_ASTRA_PREFS_H_

class PrefRegistrySimple;

namespace astra {
namespace prefs {

// Registers all Astra profile-scoped preferences with the given registry.
//
// All Astra persistence goes through the profile's PrefService — no custom
// file I/O, no JSON files, no localStorage for browser state. This ensures
// Astra metadata participates in Chromium's profile lifecycle, sync (if
// registered as syncable), policy, and migration framework.
//
// Chromium component: PrefService / PrefRegistry.
// Patch point: called from AstraWorkspaceServiceFactory::RegisterProfilePrefs
// and other Astra keyed service factories.  The top-level registration hook
// into Chromium's profile pref pipeline is still TODO(astra).
void RegisterProfilePrefs(PrefRegistrySimple* registry);

// -- Workspace pref keys (exposed for service-layer use) --------------------
//
// These are declared here (rather than in an anonymous namespace in the .cc)
// because AstraWorkspaceService needs to read/write them by name.
//
// Format follows Chromium pref conventions: dot-separated path under the
// "astra" namespace.

// List of workspace dictionaries. Each dict contains:
//   id            (string)   - unique workspace identifier
//   name          (string)   - user-visible display name
//   accent_color  (string)   - hex color string, e.g. "#5B8FF9"
//   created_time  (double)   - microseconds since Windows epoch (base::Time)
//   is_default    (bool)     - whether this is the default workspace
//   order_index   (int)      - sort position in the workspace list
//   icon          (string, optional) - icon identifier
inline constexpr char kPrefWorkspaces[] = "astra.workspaces.list";

// ID of the currently active workspace (string).
inline constexpr char kPrefActiveWorkspaceId[] = "astra.workspaces.active";

// -- Workspace overview pref keys ------------------------------------------
//
// Presentation settings for the workspace overview UI.
// These control how workspaces are displayed in the overview.
//
// Workspace data is owned by AstraWorkspaceService.
// These prefs control only the presentation (view mode, card size, etc.).
//
// Chromium component: PrefService (persistence).
// The overview view reads these through the controller.

// Default view mode for the workspace overview (int).
// 0 = grid, 1 = list.
// Default: 0 = grid view.
inline constexpr char kPrefWorkspaceOverviewViewMode[] =
    "astra.workspaces.overview.view_mode";

// Default card size for the workspace overview (int).
// 0 = small, 1 = medium, 2 = large.
// Default: 1 = medium.
inline constexpr char kPrefWorkspaceOverviewCardSize[] =
    "astra.workspaces.overview.card_size";

// Whether to show statistics on workspace cards (bool).
// When true, cards show last used time and other stats.
// Default: true — statistics provide useful context.
inline constexpr char kPrefWorkspaceOverviewShowStatistics[] =
    "astra.workspaces.overview.show_statistics";

// Whether to show hibernation indicator on cards (bool).
// When true, hibernated workspaces show a visual badge.
// Default: true — hibernation state is important context.
inline constexpr char kPrefWorkspaceOverviewShowHibernation[] =
    "astra.workspaces.overview.show_hibernation";

// Whether to show the "New Workspace" quick-add card (bool).
// Default: true — quick workspace creation is a core feature.
inline constexpr char kPrefWorkspaceOverviewShowQuickAdd[] =
    "astra.workspaces.overview.show_quick_add";

// Default sort order for workspaces in the overview (string).
// Values: "manual", "name", "last_used", "created".
// Default: "manual" — user-defined order.
inline constexpr char kPrefWorkspaceOverviewSortOrder[] =
    "astra.workspaces.overview.sort_order";

// Bulk delete confirmation threshold (int).
// Number of workspaces above which deletion requires confirmation.
// Default: 1 — always confirm for safety.
inline constexpr char kPrefWorkspaceOverviewBulkDeleteThreshold[] =
    "astra.workspaces.overview.bulk_delete_threshold";

// Whether to include hibernated workspaces in the overview (bool).
// When false, hibernated workspaces are hidden from the main view.
// Default: true — show all workspaces.
inline constexpr char kPrefWorkspaceOverviewIncludeHibernated[] =
    "astra.workspaces.overview.include_hibernated";

// -- Workspace import/export pref keys ------------------------------------
//
// Default settings for workspace import/export operations.
// These control the default behavior when no explicit options are provided.
//
// Import/export operates on workspace metadata (from AstraWorkspaceService)
// and tab data (from Chromium's TabStripModel).  Settings persist via
// PrefService so user preferences survive restarts.
//
// Chromium owner: base::JSONReader / base::JSONWriter for serialization.
// Actual workspace state is owned by AstraWorkspaceService + Chromium.

// Whether to include tabs in workspace exports by default (bool).
// Default: true — exports include tab URLs and titles.
inline constexpr char kPrefWorkspaceExportIncludeTabs[] =
    "astra.workspaces.export.include_tabs";

// Whether to include workspace settings in exports (bool).
// Settings include accent color, icon, description, order index.
// Default: true — full workspace metadata is exported.
inline constexpr char kPrefWorkspaceExportIncludeSettings[] =
    "astra.workspaces.export.include_settings";

// Whether to include favorite state in exported tabs (bool).
// Default: true — favorite markers are exported with tabs.
inline constexpr char kPrefWorkspaceExportIncludeFavorites[] =
    "astra.workspaces.export.include_favorites";

// Whether to include advanced metadata in exports (bool).
// Metadata includes creation time, last used time, hibernation state.
// Default: false — basic export doesn't include internal timestamps.
inline constexpr char kPrefWorkspaceExportIncludeMetadata[] =
    "astra.workspaces.export.include_metadata";

// Default import mode (int).
// 0 = merge (add imported workspaces to existing ones),
// 1 = replace (delete existing non-default workspaces before import).
// Default: 0 = merge — safer default, preserves user's existing workspaces.
inline constexpr char kPrefWorkspaceImportMode[] =
    "astra.workspaces.import.mode";

// Default conflict resolution strategy for import (int).
// 0 = rename (appends " (2)", " (3)", etc. to duplicate names),
// 1 = skip (skips workspaces with duplicate names).
// Default: 0 = rename — all workspaces get imported, names are adjusted.
inline constexpr char kPrefWorkspaceImportConflictResolution[] =
    "astra.workspaces.import.conflict_resolution";

// Whether to open tabs for imported workspaces by default (bool).
// Default: true — imported workspaces open with their tabs.
inline constexpr char kPrefWorkspaceImportOpenTabs[] =
    "astra.workspaces.import.open_tabs";

// Whether to apply favorite state from imported tabs (bool).
// Default: true — imported favorite tabs retain their favorite status.
inline constexpr char kPrefWorkspaceImportApplyFavorites[] =
    "astra.workspaces.import.apply_favorites";

// -- Workspace window pref keys --------------------------------------------
//
// Window behavior preferences for workspaces.  These control how Astra
// manages browser windows within workspaces.
//
// Chromium owns actual window management (BrowserList, BrowserWindow).
// These prefs control only Astra-specific window behavior.

// Whether to remember and restore window placement per workspace (bool).
// When true, each workspace's windows remember their screen positions and
// are restored to those positions when switching workspaces.
// Default: true — workspace window recall is a core Arc-style feature.
inline constexpr char kPrefWorkspaceWindowRememberPlacement[] =
    "astra.workspaces.window.remember_placement";

// Whether new browser windows open in the active workspace (bool).
// When true, windows created via Ctrl+N or menu open in the current
// active workspace.  When false, new windows open in the default workspace.
// Default: true — new windows belong to the current context.
inline constexpr char kPrefWorkspaceWindowNewInActiveWorkspace[] =
    "astra.workspaces.window.new_in_active_workspace";

// Whether to auto-tile windows in a workspace (bool).
// When true, windows in a workspace are automatically arranged side-by-side
// (tiled) when the workspace is activated.
// Default: false — windows retain their individual positions by default.
inline constexpr char kPrefWorkspaceWindowAutoTile[] =
    "astra.workspaces.window.auto_tile";

// Default number of windows to create for new workspaces (int).
// When a new workspace is created, this many browser windows are opened
// automatically.  0 means no windows are created automatically.
// Default: 1 — new workspaces start with one browser window.
inline constexpr char kPrefWorkspaceWindowDefaultWindowCount[] =
    "astra.workspaces.window.default_window_count";

// -- Sidebar pref keys ------------------------------------------------------

// Whether the Astra sidebar is visible (bool).
inline constexpr char kPrefSidebarVisible[] = "astra.sidebar.visible";

// Width of the Astra sidebar in pixels (int).
inline constexpr char kPrefSidebarWidth[] = "astra.sidebar.width";

// Whether the Astra sidebar is pinned open (bool). When unpinned, the sidebar
// slides in as an overlay on hover or command.
inline constexpr char kPrefSidebarPinned[] = "astra.sidebar.pinned";

// Sidebar position: "left" or "right" (string).
// Controls which side of the window the sidebar appears on.
inline constexpr char kPrefSidebarPosition[] = "astra.sidebar.position";

// Whether the sidebar auto-hides when the user interacts with the page (bool).
// When true, the sidebar collapses after a short delay when the cursor
// leaves the sidebar area.
inline constexpr char kPrefSidebarAutoHide[] = "astra.sidebar.auto_hide";

// List of pinned section IDs in the sidebar (list of strings).
// Controls which sections (favorites, workspaces, tab stacks, etc.) are
// shown in the sidebar's pinned area and in what order.
inline constexpr char kPrefSidebarPinnedSections[] =
    "astra.sidebar.pinned_sections";

// Whether section icons are shown in the sidebar (bool).
// When true, each section header displays an icon next to the label.
// Default: true — icons improve scannability of the sidebar.
inline constexpr char kPrefSidebarShowSectionIcons[] =
    "astra.sidebar.show_section_icons";

// Whether section labels are shown in the sidebar (bool).
// When false, only icons are shown (icon-only mode).
// Default: true — labels provide clear section identification.
inline constexpr char kPrefSidebarShowSectionLabels[] =
    "astra.sidebar.show_section_labels";

// Whether compact mode is enabled (bool).
// Compact mode reduces padding and item height for denser information.
// Default: false — comfortable spacing by default.
inline constexpr char kPrefSidebarCompactMode[] =
    "astra.sidebar.compact_mode";

// Whether the sidebar auto-hides when a tab is clicked (bool).
// When true, clicking a tab in the sidebar closes the sidebar automatically.
// Default: false — sidebar stays open after tab selection.
inline constexpr char kPrefSidebarAutoHideOnTabClick[] =
    "astra.sidebar.auto_hide_on_tab_click";

// Whether tab count badges are shown on section headers (bool).
// When true, each section shows a badge with the number of items inside.
// Default: true — item counts provide useful context.
inline constexpr char kPrefSidebarShowTabCountBadges[] =
    "astra.sidebar.show_tab_count_badges";

// Whether the workspace badge is shown in the sidebar header (bool).
// When true, the active workspace name and badge are displayed prominently.
// Default: true — workspace context is important for multi-workspace users.
inline constexpr char kPrefSidebarShowWorkspaceBadge[] =
    "astra.sidebar.show_workspace_badge";

// Whether the tab groups section is shown (bool).
// Controls visibility of the tab groups section in the sidebar.
// Default: true — tab groups are a core organization feature.
inline constexpr char kPrefSidebarShowTabGroupsSection[] =
    "astra.sidebar.show_tab_groups_section";

// Whether the history section is shown (bool).
// Controls visibility of the history section in the sidebar.
// Default: true — quick history access is a key sidebar feature.
inline constexpr char kPrefSidebarShowHistorySection[] =
    "astra.sidebar.show_history_section";

// Whether the recently closed tabs section is shown (bool).
// Controls visibility of the recently closed section in the sidebar.
// Default: true — recently closed tabs are frequently accessed.
inline constexpr char kPrefSidebarShowRecentlyClosedSection[] =
    "astra.sidebar.show_recently_closed_section";

// Whether the reading list section is shown (bool).
// Controls visibility of the reading list section in the sidebar.
// Default: true — reading list is a common sidebar feature.
inline constexpr char kPrefSidebarShowReadingListSection[] =
    "astra.sidebar.show_reading_list_section";

// Whether the notes section is shown (bool).
// Controls visibility of the notes section in the sidebar.
// Default: true — notes are a core Astra productivity feature.
inline constexpr char kPrefSidebarShowNotesSection[] =
    "astra.sidebar.show_notes_section";

// Whether the downloads section is shown (bool).
// Controls visibility of the downloads section in the sidebar.
// Default: true — download progress is important to see at a glance.
inline constexpr char kPrefSidebarShowDownloadsSection[] =
    "astra.sidebar.show_downloads_section";

// Whether the passwords section is shown (bool).
// Controls visibility of the passwords section in the sidebar.
// Default: true — quick password access is useful for many users.
inline constexpr char kPrefSidebarShowPasswordsSection[] =
    "astra.sidebar.show_passwords_section";

// Whether the extensions section is shown (bool).
// Controls visibility of the extensions section in the sidebar.
// Default: true — extensions are a key part of the browser experience.
inline constexpr char kPrefSidebarShowExtensionsSection[] =
    "astra.sidebar.show_extensions_section";

// Whether UI animations are enabled for the sidebar (bool).
// When false, all sidebar transitions (expand/collapse, show/hide) are instant.
// Default: true — smooth animations provide better UX.
inline constexpr char kPrefSidebarAnimationEnabled[] =
    "astra.sidebar.animation_enabled";

// Whether to remember the last active section between sessions (bool).
// When true, the sidebar restores the previously active section on startup.
// Default: true — returning to the last used section is convenient.
inline constexpr char kPrefSidebarRememberLastSection[] =
    "astra.sidebar.remember_last_section";

// The last active section ID (string).
// Persisted when remember_last_section is true so the sidebar can restore
// the previous session's active section.
inline constexpr char kPrefSidebarLastActiveSection[] =
    "astra.sidebar.last_active_section";

// The default active section ID (string).
// Used on first run or when remember_last_section is false.
// Default: "open_tabs" — the most commonly used section.
inline constexpr char kPrefSidebarDefaultActiveSection[] =
    "astra.sidebar.default_active_section";

// Ordered list of section IDs (list of strings).
// Defines the display order of sidebar sections.
// Sections not in this list use their default position.
inline constexpr char kPrefSidebarSectionOrder[] =
    "astra.sidebar.section_order";

// List of collapsed section IDs (list of strings).
// Sections in this list are shown in collapsed (header-only) state.
inline constexpr char kPrefSidebarCollapsedSections[] =
    "astra.sidebar.collapsed_sections";

// -- Split view pref keys ---------------------------------------------------

// Default orientation for new split view pairs (string).
// Values: "horizontal" or "vertical".
inline constexpr char kPrefSplitViewDefaultOrientation[] =
    "astra.split_view.default_orientation";

// Default split view ratio (double).
// Controls the default size ratio when creating a new split view pair.
// 0.5 means equal split; values range from 0.1 to 0.9.
inline constexpr char kPrefSplitViewDefaultRatio[] =
    "astra.split_view.default_ratio";

// Whether split view is enabled (bool).
// When false, split view commands are disabled and UI is hidden.
inline constexpr char kPrefSplitViewEnabled[] = "astra.split_view.enabled";

// Whether split views snap to preset ratios when dragged near them (bool).
// When true, the divider snaps to common ratios (50/50, 70/30, etc.)
// when the user drags close to one.
inline constexpr char kPrefSplitViewSnapToPresets[] =
    "astra.split_view.snap_to_presets";

// Whether the divider is visible (bool).
// When false, the divider is hidden but split view still works programmatically.
// This is useful for presentation mode or minimalist setups.
inline constexpr char kPrefSplitViewDividerVisible[] =
    "astra.split_view.divider_visible";

// Whether to remember the last used split ratio for each tab pair (bool).
// When true, the ratio from a previous split view session is restored
// when the same two tabs are split again.
inline constexpr char kPrefSplitViewRememberRatio[] =
    "astra.split_view.remember_ratio";

// Whether the mini map / thumbnail view is enabled (bool).
// When true, a small schematic overview of the split layout is shown
// on hover or via keyboard shortcut.
//
// TODO(astra): Implement real thumbnail rendering using WebContents
//   capture APIs.  For now, the minimap shows a schematic representation.
//   Chromium owner: content::WebContents::CopyFromSurface()
inline constexpr char kPrefSplitViewMinimapEnabled[] =
    "astra.split_view.minimap_enabled";

// -- Notes pref keys --------------------------------------------------------

// List of note dictionaries. Each dict contains:
//   id             (string)   - unique note identifier
//   title          (string)   - note title
//   content        (string)   - note body text (plain text)
//   url            (string, optional) - associated URL for per-page notes
//   workspace_id   (string, optional) - associated workspace
//   created_time   (double)   - microseconds since Windows epoch (base::Time)
//   modified_time  (double)   - microseconds since Windows epoch (base::Time)
//   color          (string, optional) - hex color string, e.g. "#FFD93D"
inline constexpr char kPrefNotes[] = "astra.notes.list";

// Default sort order for note lists (int, maps to NoteSortOrder enum).
// Default: 0 = kDateDescending (most recent first).
//
// This is a presentation preference — it doesn't affect which notes exist,
// only the order in which they're displayed.
//
// Chromium component: PrefService (persistence).
inline constexpr char kPrefNoteSortOrder[] = "astra.notes.sort_order";
inline constexpr int kDefaultNoteSortOrder = 0;

// -- Focus mode pref keys ---------------------------------------------------

// Default focus mode session duration in minutes (int).
// Pomodoro-style default of 25 minutes.
inline constexpr char kPrefFocusModeDefaultDuration[] =
    "astra.focus_mode.default_duration";

// List of distraction site URL patterns (list of strings).
// Used by focus mode to block or warn about distracting websites.
inline constexpr char kPrefFocusModeBlocklist[] =
    "astra.focus_mode.blocklist";

// Whether focus mode should auto-start based on configured conditions
// (e.g., work hours, certain days).
inline constexpr char kPrefFocusModeAutoStart[] =
    "astra.focus_mode.auto_start";

// Short break duration in minutes for pomodoro mode (int).
// Short breaks occur between work sessions.
inline constexpr char kPrefFocusModeShortBreakDuration[] =
    "astra.focus_mode.short_break_duration";

// Long break duration in minutes for pomodoro mode (int).
// Long breaks occur every N work sessions (configurable interval).
inline constexpr char kPrefFocusModeLongBreakDuration[] =
    "astra.focus_mode.long_break_duration";

// Number of work sessions before a long break (int).
// Pomodoro classic: every 4 work sessions = long break.
inline constexpr char kPrefFocusModeLongBreakInterval[] =
    "astra.focus_mode.long_break_interval";

// Whether to auto-start the next phase in pomodoro mode (bool).
// When true, work sessions auto-start after breaks and vice versa.
// When false, user must manually start each phase.
inline constexpr char kPrefFocusModeAutoStartNextPhase[] =
    "astra.focus_mode.auto_start_next_phase";

// Whitelist of sites that are always allowed during focus mode (list of strings).
// Sites on this list bypass the distraction blocklist.
// Patterns follow the same format as the blocklist: exact host, subdomain, wildcard.
inline constexpr char kPrefFocusModeWhitelist[] =
    "astra.focus_mode.whitelist";

// Total accumulated focus time in seconds (int).
// Cumulative stat: sum of all completed focus sessions.
// Persisted across browser restarts.
inline constexpr char kPrefFocusModeTotalFocusSeconds[] =
    "astra.focus_mode.total_focus_seconds";

// Total number of completed focus sessions (int).
// Cumulative stat: count of sessions that ran to completion or were
// manually ended.  Used for productivity tracking.
inline constexpr char kPrefFocusModeSessionsCompleted[] =
    "astra.focus_mode.sessions_completed";

// Total number of completed pomodoro cycles (int).
// A cycle = N work sessions + a long break (where N = long break interval).
inline constexpr char kPrefFocusModeCyclesCompleted[] =
    "astra.focus_mode.cycles_completed";

// Whether distraction warnings are enabled (bool).
// When true, navigating to a blocked site during focus mode triggers
// a distraction warning notification.
inline constexpr char kPrefFocusModeWarningsEnabled[] =
    "astra.focus_mode.warnings_enabled";

// Auto-start start time (string, "HH:MM" 24-hour format).
// Focus mode automatically starts at this time on configured days.
inline constexpr char kPrefFocusModeAutoStartTime[] =
    "astra.focus_mode.auto_start_time";

// Auto-start end time (string, "HH:MM" 24-hour format).
// Focus mode automatically ends at this time on configured days.
inline constexpr char kPrefFocusModeAutoEndTime[] =
    "astra.focus_mode.auto_end_time";

// Auto-start days of week (list of ints).
// 0 = Sunday, 1 = Monday, ..., 6 = Saturday.
// Default: weekdays only (Mon-Fri).
inline constexpr char kPrefFocusModeAutoStartDays[] =
    "astra.focus_mode.auto_start_days";

// Session presets (list of dicts).
// Each preset is a named duration configuration that the user can
// quickly select.  Each dict contains:
//   id       (string)  - unique preset identifier
//   name     (string)  - user-visible display name
//   duration (int)     - focus duration in minutes
//   break_duration (int, optional) - associated break duration in minutes
inline constexpr char kPrefFocusModePresets[] =
    "astra.focus_mode.presets";

// Whether the focus session is currently paused (bool).
// Persisted so that pause state survives browser restart during a
// long focus session.
inline constexpr char kPrefFocusModePaused[] =
    "astra.focus_mode.paused";

// Remaining seconds in the current paused session (int).
// Only meaningful when kPrefFocusModePaused is true.
// Persisted alongside paused state for session resume after restart.
inline constexpr char kPrefFocusModePausedRemainingSeconds[] =
    "astra.focus_mode.paused_remaining_seconds";

// -- Focus mode presentation / UI prefs -----------------------------------
//
// These prefs control how the focus mode UI is presented.
// They are read by AstraFocusModeModel (//astra/ui/views/focus_mode/)
// and applied by the indicator / menu bubble views.
//
// Truth source: AstraFocusModeModel (views layer).
// Chromium owns: nothing — these are pure Astra presentation settings.

// Whether the floating focus indicator is shown during focus mode (bool).
// When false, no indicator widget is displayed; focus mode is still active
// but has no visible UI chrome.
// Default: true — the indicator is the primary way users see focus state.
inline constexpr char kPrefFocusModeShowIndicator[] =
    "astra.focus_mode.show_indicator";

// Position of the focus mode indicator on screen (int, enum).
// 0 = top_left, 1 = top_right, 2 = bottom_left, 3 = bottom_right.
// Default: 1 = top_right — matches common indicator placement.
inline constexpr char kPrefFocusModeIndicatorPosition[] =
    "astra.focus_mode.indicator_position";

// Visual style of the focus mode indicator (int, enum).
// 0 = minimal (small dot/badge), 1 = full (label + timer), 2 = badge (icon only).
// Default: 1 = full — shows label and timer for maximum clarity.
inline constexpr char kPrefFocusModeIndicatorStyle[] =
    "astra.focus_mode.indicator_style";

// Whether to show the timer countdown in the indicator (bool).
// When false, the indicator shows only the focus mode label.
// Default: true — timer is a core part of the focus mode UX.
inline constexpr char kPrefFocusModeShowTimerInIndicator[] =
    "astra.focus_mode.show_timer_in_indicator";

// Whether to show session statistics in the focus mode menu (bool).
// Stats include today's focus minutes, sessions completed, streak.
// Default: true — stats provide motivation and context.
inline constexpr char kPrefFocusModeShowSessionStats[] =
    "astra.focus_mode.show_session_stats";

// Whether distraction site blocking is active during focus mode (bool).
// This is the master toggle for the blocklist feature.
// When false, the blocklist is ignored even if sites are configured.
// Default: false — user opts in to distraction blocking.
inline constexpr char kPrefFocusModeBlockDistractingSites[] =
    "astra.focus_mode.block_distracting_sites";

// Whether notification sounds play for focus mode events (bool).
// Events include: session start, session end, break reminders, distraction warnings.
// Default: true — audio feedback helps users stay aware without looking.
inline constexpr char kPrefFocusModeNotificationSound[] =
    "astra.focus_mode.notification_sound";

// Whether break reminders are enabled (bool).
// When enabled, the user is reminded to take breaks at configured intervals.
// This is separate from pomodoro mode's structured work-break cycles.
// Default: false — break reminders are opt-in.
inline constexpr char kPrefFocusModeBreakReminders[] =
    "astra.focus_mode.break_reminders";

// Break reminder interval in minutes (int).
// How often the user is reminded to take a break during focus sessions.
// Default: 25 — matches Pomodoro work session length.
inline constexpr char kPrefFocusModeBreakIntervalMinutes[] =
    "astra.focus_mode.break_interval_minutes";

// Suggested break duration in minutes (int).
// How long a break the user is encouraged to take.
// Default: 5 — matches Pomodoro short break length.
inline constexpr char kPrefFocusModeBreakDurationMinutes[] =
    "astra.focus_mode.break_duration_minutes";

// Whether non-focus tabs are dimmed during focus mode (bool).
// When true, tabs that aren't part of the current focus context are
// visually dimmed to reduce distraction.
// Default: false — dimmed tabs can be confusing for some users.
inline constexpr char kPrefFocusModeDimNonFocusTabs[] =
    "astra.focus_mode.dim_non_focus_tabs";

// Whether the sidebar is automatically hidden during focus mode (bool).
// When true, the sidebar collapses when focus mode starts and restores
// when focus mode ends.
// Default: true — hiding the sidebar is a core focus mode feature.
inline constexpr char kPrefFocusModeHideSidebar[] =
    "astra.focus_mode.hide_sidebar";

// -- Memory saver pref keys --------------------------------------------------

// Whether the memory saver (auto-suspend inactive tabs) is enabled (bool).
// Default: true — tabs are automatically suspended after inactivity timeout.
inline constexpr char kPrefMemorySaverEnabled[] = "astra.memory_saver.enabled";

// Auto-suspend timeout in minutes (int). Tabs inactive for this long are
// eligible for suspension. Default: 5 minutes.
inline constexpr char kPrefMemorySaverTimeoutMinutes[] =
    "astra.memory_saver.timeout_minutes";

// Whether tabs in the active workspace can be suspended (bool).
// When false (default), only tabs in non-active workspaces are auto-suspended,
// preserving the user's current workspace context.
// When true, all inactive tabs are eligible regardless of workspace.
inline constexpr char kPrefMemorySaverSuspendActiveWorkspace[] =
    "astra.memory_saver.suspend_active_workspace";

// -- Picture-in-Picture (PiP) pref keys --------------------------------------

// Default PiP window size preset (string).
// Values: "small", "medium", "large".
// Controls the default size of PiP windows when they are first created.
//
// The actual PiP window management is owned by Chromium's
// PictureInPictureWindowController.  This pref controls the default size
// that Astra applies when creating or restoring a PiP window.
//
// Chromium owner: PictureInPictureWindowController
//   (chrome/browser/picture_in_picture/picture_in_picture_window_controller.h)
inline constexpr char kPrefPiPDefaultSize[] = "astra.pip.default_size";

// Whether PiP windows are always-on-top by default (bool).
// When true, PiP windows stay above other browser windows.
//
// Chromium owner: views::Widget::SetZOrderLevel()
//   (ui/views/widget/widget.h)
// Chromium owner: PictureInPictureWindowViews
//   (chrome/browser/ui/views/picture_in_picture/picture_in_picture_window_views.h)
inline constexpr char kPrefPiPAlwaysOnTop[] = "astra.pip.always_on_top";

// Default snap position for PiP windows (string).
// Values: "top_left", "top_right", "bottom_left", "bottom_right", "free_floating".
// Controls where new PiP windows appear on the screen.
//
// Chromium owner: views::Widget (window positioning)
inline constexpr char kPrefPiPSnapPosition[] = "astra.pip.snap_position";

// Whether snap-to-corner is enabled for PiP windows (bool).
// When true, PiP windows snap to the nearest screen corner when dragged.
inline constexpr char kPrefPiPSnapToCornerEnabled[] =
    "astra.pip.snap_to_corner_enabled";

// PiP window opacity (double).
// Range: 0.2 to 1.0.  Default: 1.0 (fully opaque).
// Controls the transparency of PiP windows.
//
// Chromium owner: views::Widget::SetOpacity()
inline constexpr char kPrefPiPOpacity[] = "astra.pip.opacity";

// Whether auto-PiP on tab switch is enabled (bool).
// When true, switching away from a tab playing video automatically
// sends it to PiP mode.
inline constexpr char kPrefPiPAutoPipOnTabSwitch[] =
    "astra.pip.auto_pip_on_tab_switch";

// Maximum number of concurrent PiP windows (int).
// Default: 3.  0 means no limit.
inline constexpr char kPrefPiPMaxWindows[] = "astra.pip.max_windows";

// -- PiP controls presentation pref keys -------------------------------------
//
// These prefs control how the Astra PiP controls overlay is presented.
// They are read by AstraPipControlsModel and applied by the controls view.
//
// The actual PiP window is owned by Chromium.  These prefs control only
// the Astra-specific controls overlay presentation.
//
// Chromium owner: PictureInPictureWindowViews (chrome/browser/ui/views/picture_in_picture/)
// Patch point: The controls view is added as an overlay child of the
//   PiP window's client view.

// Whether PiP controls auto-hide after inactivity (bool).
// When true, the controls fade out after the auto-hide delay and the
// mouse leaves the PiP window.
// Default: true — auto-hide keeps the video unobstructed.
inline constexpr char kPrefPiPControlsAutoHide[] =
    "astra.pip.controls.auto_hide";

// Auto-hide delay in milliseconds (int).
// How long after the mouse leaves the controls area before they hide.
// Default: 3000 ms (3 seconds).
inline constexpr char kPrefPiPControlsAutoHideDelayMs[] =
    "astra.pip.controls.auto_hide_delay_ms";

// Whether the top bar (title, close button) is shown (bool).
// Default: true — top bar provides essential window controls.
inline constexpr char kPrefPiPControlsShowTopBar[] =
    "astra.pip.controls.show_top_bar";

// Whether the bottom bar (playback controls) is shown (bool).
// Default: true — bottom bar provides playback interaction.
inline constexpr char kPrefPiPControlsShowBottomBar[] =
    "astra.pip.controls.show_bottom_bar";

// Whether the resize handle is shown in the bottom-right corner (bool).
// Default: true — resize handle provides visual affordance for resizing.
inline constexpr char kPrefPiPControlsShowResizeHandle[] =
    "astra.pip.controls.show_resize_handle";

// Opacity of the controls overlay (double).
// Range: 0.3 to 1.0.  Default: 1.0 (fully opaque).
// Controls the transparency of the control bars, not the PiP window itself.
inline constexpr char kPrefPiPControlsOpacity[] =
    "astra.pip.controls.opacity";

// Default size preset for PiP controls (string).
// Values: "small", "medium", "large".
// Controls the default size preset applied to new PiP windows.
// Default: "medium".
inline constexpr char kPrefPiPControlsDefaultSizePreset[] =
    "astra.pip.controls.default_size_preset";

// Whether the always-on-top button is shown in the controls (bool).
// Default: true — always-on-top is a core PiP feature.
inline constexpr char kPrefPiPControlsShowAlwaysOnTopButton[] =
    "astra.pip.controls.show_always_on_top_button";

// Whether playback controls (play/pause, etc.) are shown (bool).
// Default: true — playback controls are essential for video PiP.
inline constexpr char kPrefPiPControlsShowPlaybackControls[] =
    "astra.pip.controls.show_playback_controls";

// Whether skip forward/backward buttons are shown (bool).
// Default: true — skip buttons provide quick navigation.
inline constexpr char kPrefPiPControlsShowSkipButtons[] =
    "astra.pip.controls.show_skip_buttons";

// Skip duration in seconds (int).
// How far forward/backward the skip buttons jump.
// Default: 10 seconds.
inline constexpr char kPrefPiPControlsSkipDurationSeconds[] =
    "astra.pip.controls.skip_duration_seconds";

// Playback rate presets (list of doubles).
// List of playback speed options shown in the rate selector.
// Default: [0.5, 1.0, 1.25, 1.5, 2.0].
inline constexpr char kPrefPiPControlsPlaybackRatePresets[] =
    "astra.pip.controls.playback_rate_presets";

// -- Reading list pref keys -------------------------------------------------
//
// Reading list entries are Astra-projected metadata for articles the user
// wants to read later.  In production, the canonical data source is
// Chromium's ReadingListModel (components/reading_list/core/).  For the
// overlay skeleton, entries are stored as Astra metadata via PrefService.
//
// Each entry dictionary contains:
//   url             (string)  - URL of the reading list item
//   title           (string)  - Title of the page
//   is_read         (bool)    - Whether the entry has been marked as read
//   added_time      (double)  - microseconds since Windows epoch (base::Time)
//   update_time     (double)  - microseconds since Windows epoch (base::Time)
//   estimated_read_time_minutes (int) - Estimated read time in minutes, -1 if unknown
//   category        (string, optional) - Category/folder name for the entry
inline constexpr char kPrefReadingListEntries[] = "astra.reading_list.entries";

// Default sort order for reading list entries (string).
// Values: "newest", "oldest", "title", "read_status".
// Default: "newest" — newest entries first.
inline constexpr char kPrefReadingListSortOrder[] =
    "astra.reading_list.sort_order";
inline constexpr char kDefaultReadingListSortOrder[] = "newest";

// -- Favorite folders pref keys ---------------------------------------------
//
// Favorite folders organize favorite tabs into a hierarchical structure.
// Folder metadata (name, parent, order) is persisted via PrefService.
// Per-tab favorite folder membership is stored on AstraTabFeatures
// (WebContentsUserData) and travels with tabs through session restore.
//
// Chromium analog: Bookmarks model (components/bookmarks/browser/).
//   Favorites differ from bookmarks in that they are lightweight tab
//   references rather than permanent URL entries.

// List of favorite folder dictionaries. Each dict contains:
//   id            (string)  - unique folder identifier
//   name          (string)  - user-visible folder name
//   parent_id     (string)  - parent folder ID, empty for root-level
//   order_index   (int)     - sort position within the parent
//   color         (string, optional) - accent color for the folder
//   icon          (string, optional) - icon identifier
inline constexpr char kPrefFavoriteFolders[] = "astra.favorites.folders";

// -- Tab stacks pref keys -------------------------------------------------
//
// Tab stacks are Astra-native named groups of tabs (like Vivaldi tab stacks
// or Arc tab orphans).  Stacks have their own identity (name, color, order)
// and tabs are members of a stack by ID reference.
//
// Stack metadata (name, color, order, collapsed state) is persisted via
// PrefService.  Per-tab stack membership is stored on AstraTabFeatures
// (WebContentsUserData) and travels with tabs through session restore.
//
// Chromium analog: TabGroupModel + TabGroup (chrome/browser/ui/tabs/)
//   Stacks differ from tab groups in that stacks are Astra-native, have
//   richer metadata, and are projected in the sidebar as collapsible sections.

// List of tab stack dictionaries.  Each dict contains:
//   id            (string)  - unique stack identifier
//   name          (string)  - user-visible stack name
//   color         (string)  - hex color string, e.g. "#5B8FF9"
//   order_index   (int)     - sort position in the stack list
//   collapsed     (bool)    - whether the stack is collapsed in the sidebar
inline constexpr char kPrefTabStacks[] = "astra.tab_stacks.list";

// -- Search engine pref keys ----------------------------------------------
//
// IMPORTANT: Search engine state and management is fully owned by Chromium's
// TemplateURLService (components/search_engines/template_url_service.h).
// Astra does NOT store or manage search engines — it only projects
// TemplateURLService state through AstraSearchEngineHelper.
//
// Chromium owner: TemplateURLService / SearchEngineTabHelper
//   (components/search_engines/, chrome/browser/search_engines/)
//
// The Astra-specific search prefs below control only presentation and UX
// behavior — never the actual search engine data itself.

// Whether the default search engine is shown as a prominent shortcut in
// the Astra new tab page or sidebar (bool).
// Default: true — the default engine is shown as a quick access item.
inline constexpr char kPrefSearchShowDefaultEngine[] =
    "astra.search.show_default_engine";

// Whether to show the "Other search engines" list in the Astra sidebar
// search section (bool).
// Default: false — only the default engine is shown by default to keep
// the sidebar uncluttered.
inline constexpr char kPrefSearchShowOtherEngines[] =
    "astra.search.show_other_engines";

// Whether search suggestions are enabled in Astra search UI (bool).
// Default: true — suggestions are shown by default.
//
// This controls whether Astra surfaces (sidebar search, command palette)
// show search suggestions.  The actual suggestion data comes from
// Chromium's search suggest service.
//
// Chromium owner: SearchSuggestService / Omnibox suggest providers
//   (components/omnibox/browser/)
inline constexpr char kPrefSearchSuggestionsEnabled[] =
    "astra.search.suggestions_enabled";

// List of recent search queries (list of strings).
// Most recent first.  Capped at kPrefSearchMaxRecentQueries.
//
// Recent queries are an Astra-level presentation feature — they let
// the user quickly re-run a recent search.  They are not the same as
// Chromium's search history (which is managed by the HistoryService).
//
// Chromium owner: HistoryService (components/history/core/browser/)
inline constexpr char kPrefSearchRecentQueries[] =
    "astra.search.recent_queries";

// Maximum number of recent search queries to remember (int).
// Default: 10.
inline constexpr char kPrefSearchMaxRecentQueries[] =
    "astra.search.max_recent_queries";

// Hard upper limit for recent queries (int).
// Used to clamp user-set values to a reasonable maximum.
inline constexpr int kMaxSearchRecentQueriesLimit = 100;

// -- Accessibility pref keys -------------------------------------------------
//
// Accessibility settings for Astra UI surfaces.  These complement Chromium's
// built-in accessibility settings (high contrast, reduced motion, font size)
// with Astra-specific presentation control.
//
// Chromium subsystems reused:
//   - AccessibilityManager (chrome/browser/accessibility/)
//   - NativeTheme (ui/native_theme/) — system-level high contrast and
//     reduced motion detection
//   - PrefService (components/prefs/) — persistence

// Whether high contrast mode is enabled for Astra UI (bool).
// When enabled, Astra surfaces use high-contrast color schemes.
// The system-level high contrast setting also triggers this automatically.
//
// Chromium owner: ui/native_theme/native_theme.h (UsesHighContrastColors)
// Chromium owner: AccessibilityManager (chrome/browser/accessibility/)
inline constexpr char kPrefHighContrastMode[] =
    "astra.accessibility.high_contrast";

// Whether reduced motion mode is enabled for Astra UI (bool).
// When enabled, animations and transitions are minimized or disabled.
// The system-level prefers-reduced-motion setting also triggers this.
//
// Chromium owner: ui/native_theme/native_theme.h (prefers_reduced_transitions)
inline constexpr char kPrefReducedMotion[] =
    "astra.accessibility.reduced_motion";

// Accessibility font scale factor (double).
// Scales text in Astra UI surfaces.  1.0 = default, 1.5 = 50% larger, etc.
// Independent of Chromium's page-level text zoom.
//
// Chromium owner: AccessibilityManager / SpokenFeedback font sizes,
//   or ui/views style/font lists.
inline constexpr char kPrefAccessibilityFontScale[] =
    "astra.accessibility.font_scale";

// Whether caret browsing is enabled for Astra UI (bool).
// When enabled, a movable caret appears in web content for keyboard navigation.
//
// Chromium owner: AccessibilityManager::IsCaretBrowsingEnabled()
inline constexpr char kPrefCaretBrowsingEnabled[] =
    "astra.accessibility.caret_browsing.enabled";

// Whether sticky keys is enabled (bool).
// When enabled, modifier keys (Shift, Ctrl, Alt, Cmd) stay active after
// being pressed, allowing one-finger key combinations.
//
// Chromium owner: AccessibilityManager / StickyKeysController
inline constexpr char kPrefStickyKeysEnabled[] =
    "astra.accessibility.sticky_keys.enabled";

// Whether slow keys is enabled (bool).
// When enabled, a key must be held for a minimum duration before it registers.
//
// Chromium owner: AccessibilityManager / SlowKeys
inline constexpr char kPrefSlowKeysEnabled[] =
    "astra.accessibility.slow_keys.enabled";

// Slow keys acceptance delay in milliseconds (int).
// The amount of time a key must be held before it is accepted.
// Default: 500ms.
inline constexpr char kPrefSlowKeysDelayMs[] =
    "astra.accessibility.slow_keys.delay_ms";

// Whether mouse keys is enabled (bool).
// When enabled, the user can control the mouse cursor with the keyboard.
//
// Chromium owner: AccessibilityManager / MouseKeys
inline constexpr char kPrefMouseKeysEnabled[] =
    "astra.accessibility.mouse_keys.enabled";

// Whether large cursor is enabled (bool).
// When enabled, the mouse cursor is displayed at an enlarged size.
//
// Chromium owner: AccessibilityManager / LargeCursor
inline constexpr char kPrefLargeCursorEnabled[] =
    "astra.accessibility.large_cursor.enabled";

// Large cursor size (int).
// Controls the size of the large cursor when enabled.
// Range: 1 (smallest) to 5 (largest).  Default: 3.
inline constexpr char kPrefLargeCursorSize[] =
    "astra.accessibility.large_cursor.size";

// Whether screen magnifier is enabled (bool).
// When enabled, a portion of the screen is magnified for better visibility.
//
// Chromium owner: AccessibilityManager / MagnificationController
inline constexpr char kPrefMagnifierEnabled[] =
    "astra.accessibility.magnifier.enabled";

// Magnifier scale factor (double).
// Controls the zoom level of the magnifier.
// Range: 1.0 (no zoom) to 10.0 (10x zoom).  Default: 2.0.
inline constexpr char kPrefMagnifierScale[] =
    "astra.accessibility.magnifier.scale";

// Magnifier type (int).
// Controls whether the magnifier is docked or fullscreen.
// 0 = docked, 1 = fullscreen.  Default: 0 (docked).
inline constexpr char kPrefMagnifierType[] =
    "astra.accessibility.magnifier.type";

// Whether select-to-speak is enabled (bool).
// When enabled, selected text is read aloud using text-to-speech.
//
// Chromium owner: AccessibilityManager / SelectToSpeak
inline constexpr char kPrefSelectToSpeakEnabled[] =
    "astra.accessibility.select_to_speak.enabled";

// Whether dictation is enabled (bool).
// When enabled, the user can dictate text using their voice.
//
// Chromium owner: AccessibilityManager / Dictation
inline constexpr char kPrefDictationEnabled[] =
    "astra.accessibility.dictation.enabled";

// Whether the on-screen virtual keyboard is enabled (bool).
// When enabled, a virtual keyboard appears for text input.
//
// Chromium owner: AccessibilityManager / VirtualKeyboard
inline constexpr char kPrefVirtualKeyboardEnabled[] =
    "astra.accessibility.virtual_keyboard.enabled";

// Minimum font size in pixels (int).
// The smallest font size that will be used in Astra UI surfaces.
// Range: 0 (no minimum) to 72.  Default: 0.
inline constexpr char kPrefMinimumFontSize[] =
    "astra.accessibility.minimum_font_size";

// Font weight adjustment (int).
// Adjusts the boldness of text in Astra UI.
// Range: -100 (lighter) to 300 (bolder).  Default: 0 (no adjustment).
inline constexpr char kPrefFontWeightAdjustment[] =
    "astra.accessibility.font_weight_adjustment";

// Letter spacing multiplier (double).
// Multiplies the default letter spacing in Astra UI text.
// Range: 0.5 (tight) to 3.0 (very wide).  Default: 1.0 (normal).
inline constexpr char kPrefLetterSpacing[] =
    "astra.accessibility.letter_spacing";

// Line height multiplier (double).
// Multiplies the default line height in Astra UI text.
// Range: 0.8 (tight) to 3.0 (very loose).  Default: 1.0 (normal).
inline constexpr char kPrefLineHeight[] =
    "astra.accessibility.line_height";

// Contrast level (int).
// Controls the contrast level of Astra UI.
// 0 = normal, 1 = increased, 2 = high.  Default: 0 (normal).
inline constexpr char kPrefContrastLevel[] =
    "astra.accessibility.contrast_level";

// Whether night light / warm colors is enabled (bool).
// When enabled, the color temperature is shifted toward warmer tones.
//
// Chromium owner: NightLightController
inline constexpr char kPrefNightLightEnabled[] =
    "astra.accessibility.night_light.enabled";

// Color temperature (int).
// Controls the warmth of the color temperature when night light is enabled.
// Range: 0 (coolest) to 100 (warmest).  Default: 50.
inline constexpr char kPrefColorTemperature[] =
    "astra.accessibility.color_temperature";

// Whether color inversion is enabled (bool).
// When enabled, all colors are inverted on screen.
//
// Chromium owner: AccessibilityManager / ColorInversion
inline constexpr char kPrefColorInversionEnabled[] =
    "astra.accessibility.color_inversion.enabled";

// Animation reduction level (int).
// Controls how much animation is reduced in Astra UI.
// 0 = off (full animations), 1 = some (reduce non-essential), 2 = max (no animations).
// Default: 0 (off).
inline constexpr char kPrefAnimationReductionLevel[] =
    "astra.accessibility.animation_reduction";

// Whether auto-scroll is enabled (bool).
// When enabled, pages scroll automatically at a configurable speed.
inline constexpr char kPrefAutoScrollEnabled[] =
    "astra.accessibility.auto_scroll.enabled";

// Scroll speed multiplier (double).
// Controls the speed of auto-scroll and general scroll acceleration.
// Range: 0.25 (slow) to 5.0 (fast).  Default: 1.0 (normal).
inline constexpr char kPrefScrollSpeed[] =
    "astra.accessibility.scroll_speed";

// -- Theme pref keys -------------------------------------------------------
//
// Theme settings for Astra UI surfaces.  These complement Chromium's
// built-in theme system with Astra-specific customization.
//
// Chromium subsystems reused:
//   - ThemeService (chrome/browser/themes/) — Chromium's theme system
//   - NativeTheme (ui/native_theme/) — system-level theme detection
//   - PrefService (components/prefs/) — persistence

// Accent color for Astra UI (string).
// Hex color string, e.g. "#5B8FF9".  Used for highlights, active states,
// and emphasis across Astra surfaces.
//
// When empty, the accent color follows the system accent color or
// Chromium's theme color.
//
// Chromium owner: ThemeService / NativeTheme
inline constexpr char kPrefThemeAccentColor[] = "astra.theme.accent_color";

// Theme mode (string).
// Values: "light", "dark", "system".
// Controls whether Astra UI uses light mode, dark mode, or follows the
// system setting.
//
// Chromium owner: ThemeService (chrome/browser/themes/theme_service.h)
inline constexpr char kPrefThemeMode[] = "astra.theme.mode";

// Active named color scheme (string).
// Empty string means no named scheme is active.
// When set, contains the machine-readable name of the active scheme
// (e.g. "ocean", "forest", "sunset").
inline constexpr char kPrefThemeScheme[] = "astra.theme.scheme";

// Whether to use workspace accent for theming (bool).
// When true, the accent color follows the active workspace.
// When false, the custom accent color is used.
// Default: true.
inline constexpr char kPrefThemeUseWorkspaceAccent[] =
    "astra.theme.use_workspace_accent";

// Custom accent color (int, stored as ARGB).
// Used when use_workspace_accent is false.
// Default: Google Blue 500.
inline constexpr char kPrefThemeCustomAccentColor[] =
    "astra.theme.custom_accent_color";

// Whether high contrast mode is enabled for Astra UI (bool).
// When enabled, UI surfaces use higher-contrast colors and thicker borders.
// Default: false — standard contrast.
inline constexpr char kPrefThemeHighContrast[] = "astra.theme.high_contrast";

// Whether to show accent color on tab strips (bool).
// Default: true.
inline constexpr char kPrefThemeShowAccentOnTabs[] =
    "astra.theme.show_accent_on_tabs";

// Whether to show accent color on the sidebar (bool).
// Default: true.
inline constexpr char kPrefThemeShowAccentOnSidebar[] =
    "astra.theme.show_accent_on_sidebar";

// Accent color intensity (double).
// Controls how prominent the accent color appears.
// Range: 0.5 (subtle) to 2.0 (vibrant).
// Default: 1.0 — normal intensity.
inline constexpr char kPrefThemeAccentIntensity[] =
    "astra.theme.accent_intensity";

// Whether auto light/dark theme scheduling is enabled (bool).
// When enabled, the theme automatically switches between light and
// dark modes at configured times of day.
// Default: false — no auto switching.
inline constexpr char kPrefThemeUseAutoSchedule[] =
    "astra.theme.use_auto_schedule";

// Auto theme light mode start time (string, HH:MM 24h format).
// Default: "07:00".
inline constexpr char kPrefThemeAutoLightStart[] =
    "astra.theme.auto_light_start";

// Auto theme dark mode start time (string, HH:MM 24h format).
// Default: "19:00".
inline constexpr char kPrefThemeAutoDarkStart[] =
    "astra.theme.auto_dark_start";

// -- Command palette pref keys ----------------------------------------------
//
// Preferences for the Astra command palette (quick actions, Ctrl+K).
//
// Chromium analog: Omnibox / QuickActionSearchProvider
//   (chrome/browser/ui/omnibox/quick_action_search_provider.h)

// Number of recent commands to show in the command palette (int).
// Controls how many recently executed commands are shown at the top of
// the command palette.
inline constexpr char kPrefCommandPaletteRecentCount[] =
    "astra.command_palette.recent_count";

// Whether to show workspace commands in the command palette (bool).
inline constexpr char kPrefCommandPaletteShowWorkspaces[] =
    "astra.command_palette.show_workspaces";

// Maximum number of visible command results (int).
// Controls how many commands are shown in the command palette results list.
// Default: 20.  Clamped between 1 and kMaxResults (20).
inline constexpr char kPrefCommandPaletteMaxVisible[] =
    "astra.command_palette.max_visible";

// Whether to show command descriptions in the palette (bool).
// When false, only the command name and shortcut are shown.
// Default: true.
inline constexpr char kPrefCommandPaletteShowDescriptions[] =
    "astra.command_palette.show_descriptions";

// Whether to show keyboard shortcut hints in the palette (bool).
// When false, shortcut text is hidden from result items.
// Default: true.
inline constexpr char kPrefCommandPaletteShowShortcuts[] =
    "astra.command_palette.show_shortcuts";

// Whether to show the "recent commands" section on empty query (bool).
// When false, recent commands are not boosted or shown as a separate
// section when the search field is empty.
// Default: true.
inline constexpr char kPrefCommandPaletteShowRecentSection[] =
    "astra.command_palette.show_recent_section";

// -- Command delegate pref keys ---------------------------------------------
//
// Preferences for the Astra command delegate service.
//
// These prefs track command execution history and user-defined command
// aliases.  The command delegate is the entry point for all Astra-specific
// commands (ID range 60000+).
//
// Chromium analog: BrowserCommandController + CommandUpdater
//   (chrome/browser/ui/browser_command_controller.h)
//   (chrome/browser/command_updater.h)

// List of recently executed command IDs (list of ints).
// Most recent first, capped at kPrefCommandRecentMax.
// Persisted per-profile via PrefService.
inline constexpr char kPrefCommandRecentList[] = "astra.commands.recent";

// Maximum number of recent commands to remember (int).
// Default: 20.  The list is truncated when it exceeds this size.
inline constexpr char kPrefCommandRecentMax[] = "astra.commands.recent_max";

// Dictionary of command aliases (dict: alias string -> command_id int).
// Users can define alternate names for commands for faster access from
// the command palette.
inline constexpr char kPrefCommandAliases[] = "astra.commands.aliases";

// -- Session restore pref keys ------------------------------------------------
//
// Session restore settings that control how Astra participates in Chromium's
// session restore pipeline.  The actual session restore engine is fully owned
// by Chromium's SessionService and SessionRestore — these prefs control only
// Astra-specific behavior and presentation.
//
// Chromium owner: SessionService (chrome/browser/sessions/session_service.h)
// Chromium owner: SessionRestore (chrome/browser/sessions/session_restore.h)
// Chromium owner: StartupBrowserCreator (chrome/browser/ui/startup/)

// Session restore mode: controls how many sessions are restored on startup.
// Values: "all" (restore all windows/tabs), "last" (restore last active window),
// "none" (start with a blank new tab page).
//
// Note: This is Astra's presentation of the restore mode.  The actual
// restoration behavior is owned by Chromium's session restore settings
// (chrome.policy.session_restore, etc.).  Astra uses its own pref for
// UI-level control that maps to Chromium's restore policies.
//
// TODO(astra): Map Astra's restore mode to Chromium's session restore policies.
// Patch point: chrome/browser/prefs/session_startup_pref.cc or
//   chrome/browser/ui/startup/startup_browser_creator.cc.
inline constexpr char kPrefSessionRestoreMode[] = "astra.session_restore.mode";

// Whether session restore is enabled for Astra metadata.
// When false, Astra does not attach metadata to session entries and does not
// apply metadata on restore.  Chromium's session restore still runs normally.
// Default: true — Astra metadata participates in session restore.
inline constexpr char kPrefSessionRestoreEnabled[] =
    "astra.session_restore.enabled";

// Timestamp of the last session save (double, microseconds since Windows epoch).
// Recorded each time Chromium's SessionService persists a session snapshot.
// Used by UI to show "last saved" information.
//
// This is Astra's copy of the last save time — Chromium's SessionService
// maintains its own internal timestamps.  We mirror it here for easy access
// by Astra UI surfaces without needing to query SessionService directly.
//
// Chromium owner: BaseSessionService::current_session_time_()
//   (chrome/browser/sessions/base_session_service.h)
inline constexpr char kPrefSessionRestoreLastSaveTime[] =
    "astra.session_restore.last_save_time";

// Number of tabs in the last saved session (int).
// Recorded alongside last_save_time for UI display.
// Used by the "restore session" UI to show how many tabs would be restored.
inline constexpr char kPrefSessionRestoreLastTabCount[] =
    "astra.session_restore.last_tab_count";

// Number of windows in the last saved session (int).
inline constexpr char kPrefSessionRestoreLastWindowCount[] =
    "astra.session_restore.last_window_count";

// Number of workspaces represented in the last saved session (int).
// Derived from the unique workspace_ids across all restored tabs/windows.
inline constexpr char kPrefSessionRestoreLastWorkspaceCount[] =
    "astra.session_restore.last_workspace_count";

// Whether to restore tabs lazily (bool).
// When true (default), tabs are not loaded until they are first activated.
// This matches Chromium's default lazy session restore behavior.
//
// TODO(astra): This pref should map to Chromium's
//   kShouldUseLazySessionRestore pref (chrome/browser/prefs/session_startup_pref.cc).
//   For now it's Astra-level to control UI hints about lazy restore.
inline constexpr char kPrefSessionRestoreLazyLoading[] =
    "astra.session_restore.lazy_loading";

// Whether to show the "restore session" prompt on startup (bool).
// When true, the startup page shows a banner with session restore options.
// When false, restore happens automatically based on the mode setting.
inline constexpr char kPrefSessionRestoreShowPrompt[] =
    "astra.session_restore.show_prompt";

// Restore load mode (string): "full", "lazy", "smart", or "minimal".
// Controls how aggressively tabs are loaded during session restore.
// Default: "smart".
//
// Chromium owner: SessionRestore (chrome/browser/sessions/session_restore.h)
inline constexpr char kPrefSessionRestoreLoadMode[] =
    "astra.session_restore.load_mode";

// Whether to restore the previous session on startup (bool).
// Default: true.
inline constexpr char kPrefSessionRestoreOnStartup[] =
    "astra.session_restore.on_startup";

// Maximum number of tabs to restore per workspace (int).
// 0 means no limit.
// Default: 0 (no limit).
inline constexpr char kPrefSessionRestoreMaxTabsPerWorkspace[] =
    "astra.session_restore.max_tabs_per_workspace";

// Whether to restore workspaces one at a time instead of all at once (bool).
// Default: false.
inline constexpr char kPrefSessionRestoreWorkspacesIndividually[] =
    "astra.session_restore.workspaces_individually";

// Auto-save interval in minutes (int).
// 0 means auto-save is disabled.
// Default: 5 minutes.
inline constexpr char kPrefSessionRestoreAutoSaveInterval[] =
    "astra.session_restore.auto_save_interval";

// Maximum number of named saved sessions to keep (int).
// Default: 10.
inline constexpr char kPrefSessionRestoreMaxSavedSessions[] =
    "astra.session_restore.max_saved_sessions";

// Dictionary of named saved sessions (dict of session_name -> session_data).
// Default: empty dict.
//
// Each entry contains a serialized AstraSessionMetadata dict.
inline constexpr char kPrefSessionRestoreSavedSessions[] =
    "astra.session_restore.saved_sessions";

// -- Password helper pref keys ----------------------------------------------
//
// IMPORTANT: Password data and management is fully owned by Chromium's
// Password Manager (components/password_manager/).  Astra does NOT store
// or manage passwords — it only projects Password Manager state through
// AstraPasswordHelper and adds presentation preferences.
//
// Chromium owner: Password Manager (components/password_manager/)
// Chromium owner: PasswordManagerClient (chrome/browser/password_manager/)
//
// The Astra-specific password prefs below control only presentation and UX
// behavior — never the actual password data itself.

// Whether password suggestions are shown in Astra UI surfaces (e.g. sidebar
// login panel, form fill suggestions).
//
// This is a presentation preference — it never affects whether Chromium's
// password manager auto-fills or saves passwords.
inline constexpr char kPrefPasswordShowSuggestions[] =
    "astra.password.show_suggestions";

// Whether auto-fill is shown as enabled in Astra UI.
//
// This is a presentation preference for the Astra UI.  The actual
// auto-fill behavior is controlled by Chromium's password manager settings.
inline constexpr char kPrefPasswordAutoFillEnabled[] =
    "astra.password.auto_fill_enabled";

// Whether the password manager shortcut is shown in the Astra sidebar
// or toolbar.
//
// This controls whether the password manager entry point appears in
// the Astra UI.  The actual password manager is always available
// through Chrome settings.
inline constexpr char kPrefPasswordManagerShortcut[] =
    "astra.password.manager_shortcut";

// Whether the password health summary is shown in the password sidebar
// section.
inline constexpr char kPrefPasswordShowHealth[] =
    "astra.password.show_health";

// Whether breach alerts are shown in Astra UI.
//
// When enabled, the Astra UI shows a warning indicator when compromised
// passwords are detected.  The actual breach detection is done by
// Chromium's password manager.
inline constexpr char kPrefPasswordBreachAlertsEnabled[] =
    "astra.password.breach_alerts_enabled";

// Maximum number of passwords to show in the sidebar password section.
// Default: 20.
inline constexpr char kPrefPasswordMaxSidebarPasswords[] =
    "astra.password.max_sidebar_passwords";

// Whether a password breach has been detected since the user last
// acknowledged it.
//
// This is an Astra-level presentation flag — it tracks whether the user
// has seen and dismissed the breach warning UI.  The actual breach state
// is owned by Chromium's PasswordHealthChecker.
inline constexpr char kPrefPasswordBreachDetected[] =
    "astra.password.breach_detected";

// -- Safety helper pref keys -----------------------------------------------
//
// IMPORTANT: Safe browsing state and security decisions are fully owned by
// Chromium's SafeBrowsingService and SecurityStateModel.  Astra does NOT
// implement safe browsing or make security decisions — it only projects
// safety state through AstraSafetyHelper and adds presentation preferences.
//
// Chromium owner: SafeBrowsingService
//   (components/safe_browsing/core/browser/safe_browsing_service.h)
// Chromium owner: SecurityStateModel
//   (components/security_state/core/security_state.h)
//
// The Astra-specific safety prefs below control only presentation and UX
// behavior — never the actual safe browsing engine or security decisions.

// Whether safe browsing is enabled in Astra UI.
// This is a presentation flag — Chromium's actual safe browsing pref is
// safebrowsing.safe_browsing_enabled.
// Default: true — safe browsing is on by default.
inline constexpr char kPrefSafeBrowsingEnabled[] =
    "astra.safety.safe_browsing_enabled";

// Safe browsing protection level.
// 0 = off, 1 = standard, 2 = enhanced.
// Default: 1 = standard protection.
inline constexpr char kPrefSafeBrowsingLevel[] =
    "astra.safety.safe_browsing_level";

// Whether enhanced protection is shown as enabled.
// Convenience pref for the UI toggle; actual level is kPrefSafeBrowsingLevel.
// Default: false — standard protection by default.
inline constexpr char kPrefEnhancedProtection[] =
    "astra.safety.enhanced_protection";

// Whether password reuse warnings are shown.
// Default: true — warn users about password reuse for security.
inline constexpr char kPrefWarnOnPasswordReuse[] =
    "astra.safety.warn_on_password_reuse";

// Password protection level.
// 0 = off, 1 = standard, 2 = enhanced.
// Default: 1 = standard protection.
inline constexpr char kPrefPasswordProtectionLevel[] =
    "astra.safety.password_protection_level";

// Whether the security button is shown in the toolbar.
// Default: true — security info is easily accessible.
inline constexpr char kPrefShowSecurityButton[] =
    "astra.safety.show_security_button";

// Whether threat notifications are shown.
// Default: true — users should be aware of detected threats.
inline constexpr char kPrefShowThreatNotifications[] =
    "astra.safety.show_threat_notifications";

// Whether dangerous downloads are blocked.
// Default: true — block dangerous downloads for safety.
inline constexpr char kPrefBlockDangerousDownloads[] =
    "astra.safety.block_dangerous_downloads";

// Whether to warn on dangerous downloads.
// Default: true — warn users about potentially harmful downloads.
inline constexpr char kPrefWarnOnDangerousDownloads[] =
    "astra.safety.warn_on_dangerous_downloads";

// Whether to auto-report safety issues.
// Default: false — opt-in for privacy.
inline constexpr char kPrefAutoReportSafetyIssues[] =
    "astra.safety.auto_report_safety_issues";

// Whether to show mixed content warnings.
// Default: true — mixed content is a security concern.
inline constexpr char kPrefMixedContentWarning[] =
    "astra.safety.mixed_content_warning";

// Whether the site info button is shown.
// Default: true — site info is easily accessible.
inline constexpr char kPrefShowSiteInfoButton[] =
    "astra.safety.show_site_info_button";

// Whether safety check reminders are shown.
// Default: true — encourage regular safety checks.
inline constexpr char kPrefSafetyCheckReminders[] =
    "astra.safety.safety_check_reminders";

// Whether third-party cookie protection is enabled.
// Default: true — cookie protection is a privacy feature.
inline constexpr char kPrefCookieProtection[] =
    "astra.safety.cookie_protection";

// -- Extension helper pref keys --------------------------------------------
//
// IMPORTANT: Extension data and management is fully owned by Chromium's
// extensions system.  Astra does NOT store or manage extensions — it only
// projects extension state through AstraExtensionHelper and adds
// presentation preferences.
//
// Chromium owner: ExtensionRegistry (extensions/browser/extension_registry.h)
// Chromium owner: ExtensionService (chrome/browser/extensions/extension_service.h)
//
// The Astra-specific extension prefs below control only presentation and UX
// behavior — never the actual extension data or state itself.

// Whether the extension toolbar (browser action area) is shown in Astra UI.
//
// This is a presentation preference — it never affects whether extensions
// are installed, enabled, or functional in Chromium.
inline constexpr char kPrefExtensionShowToolbar[] = "astra.extensions.show_toolbar";

// Whether extensions are shown in the Astra sidebar section.
//
// This controls whether the extensions section appears in the sidebar.
// The actual extensions still work normally in Chromium.
inline constexpr char kPrefExtensionShowInSidebar[] =
    "astra.extensions.show_in_sidebar";

// Whether the extension manager shortcut is shown in the Astra sidebar
// or toolbar.
//
// This controls whether the extension manager entry point appears in
// the Astra UI.  The actual extension management is always available
// through Chrome settings.
inline constexpr char kPrefExtensionManagerShortcut[] =
    "astra.extensions.manager_shortcut";

// Whether recommended extensions are shown in the extension sidebar section.
//
// Recommended extensions are curated by Astra and shown as suggestions.
// They are not installed automatically — the user must opt in.
inline constexpr char kPrefExtensionShowRecommended[] =
    "astra.extensions.show_recommended";

// Maximum number of extensions to show in the sidebar extensions section.
//
// Default: 10.
inline constexpr char kPrefExtensionMaxSidebarCount[] =
    "astra.extensions.max_sidebar_count";

// Default sort order for extension lists.
// Values: "name", "install_date", "category".
// Default: "name".
//
// This is a presentation preference — it controls the default display
// order of extensions in the sidebar and extension management UI.
inline constexpr char kPrefExtensionSortOrder[] = "astra.extensions.sort_order";

// -- Downloads helper pref keys --------------------------------------------
//
// IMPORTANT: Download data and management is fully owned by Chromium's
// DownloadManager.  Astra does NOT store or manage downloads — it only
// projects DownloadManager state through AstraDownloadsHelper and adds
// presentation preferences.
//
// Chromium owner: DownloadManager (content/public/browser/download_manager.h)
// Chromium owner: DownloadItem (components/download/public/common/download_item.h)
//
// The Astra-specific download prefs below control only presentation and UX
// behavior — never the actual download data itself.

// Whether the downloads section is shown in the sidebar.
//
// This is a presentation preference — it never affects whether downloads
// are actually performed or tracked by Chromium.
inline constexpr char kPrefDownloadsShowInSidebar[] =
    "astra.downloads.show_in_sidebar";

// Whether download notifications are shown.
//
// This controls Astra-specific download notifications.  Chromium's
// native download notifications are managed separately.
inline constexpr char kPrefDownloadsShowNotifications[] =
    "astra.downloads.show_notifications";

// Whether downloads auto-open when complete.
//
// This is an Astra-level auto-open preference.  The actual auto-open
// behavior may also be controlled by Chromium's download settings.
inline constexpr char kPrefDownloadsAutoOpen[] =
    "astra.downloads.auto_open";

// Sort order for downloads lists.
// Values: "newest_first" or "oldest_first".
// Default: "newest_first".
inline constexpr char kPrefDownloadsSortOrder[] =
    "astra.downloads.sort_order";

// Maximum number of recent downloads to show in the sidebar.
// Default: 20.
inline constexpr char kPrefDownloadsMaxRecent[] =
    "astra.downloads.max_recent";

// Whether download speed is shown in the UI.
// Default: true.
inline constexpr char kPrefDownloadsShowSpeed[] =
    "astra.downloads.show_speed";

// Whether file size is shown in the UI.
// Default: true.
inline constexpr char kPrefDownloadsShowFileSize[] =
    "astra.downloads.show_file_size";

// Whether download progress bar is shown in the UI.
// Default: true.
inline constexpr char kPrefDownloadsShowProgress[] =
    "astra.downloads.show_progress";

// Display mode for download items.
// Values: "list" or "compact".
// Default: "list".
inline constexpr char kPrefDownloadsDisplayMode[] =
    "astra.downloads.display_mode";

// Whether to prompt the user for download location before saving.
// Default: true.
inline constexpr char kPrefDownloadsPromptForLocation[] =
    "astra.downloads.prompt_for_location";

// Whether to show safe browsing warnings for dangerous downloads.
// Default: true.
//
// This controls whether Astra UI surfaces show danger warnings.
// The actual safe browsing detection is done by Chromium's Safe Browsing.
inline constexpr char kPrefDownloadsSafeBrowsingWarnings[] =
    "astra.downloads.safe_browsing_warnings";

// -- Incognito pref keys ---------------------------------------------------
//
// Astra-specific incognito settings.  These complement Chromium's built-in
// incognito behavior with Astra UI and presentation preferences.
//
// IMPORTANT: Actual incognito privacy features (history deletion, cookie
// isolation, etc.) are fully owned by Chromium's OTR profile mechanism.
// Astra does NOT store or manage incognito privacy state — it only adds
// UI presentation and session tracking on top.
//
// Chromium subsystems reused:
//   - Profile (chrome/browser/profiles/profile.h) — OTR profile management
//   - Profile::IsOffTheRecord() — incognito detection
//   - BrowserList — window tracking
//   - PrefService — persistence for Astra-level incognito settings

// Whether the sidebar shows an incognito badge/indicator (bool).
// When true, Astra UI surfaces show a visual reminder that the user
// is in incognito mode.
//
// Chromium owns the actual incognito visual treatment (title bar color,
// incognito icon).  This pref controls only Astra-specific UI elements.
inline constexpr char kPrefIncognitoShowSidebarBadge[] =
    "astra.incognito.show_sidebar_badge";

// Whether to confirm before closing all incognito windows (bool).
// When true, the "Close All Incognito" command shows a confirmation dialog.
inline constexpr char kPrefIncognitoConfirmCloseAll[] =
    "astra.incognito.confirm_close_all";

// Whether to warn when external links open in incognito (bool).
// When true, clicking external links that would open in incognito shows
// a warning dialog.
inline constexpr char kPrefIncognitoWarnOnExternalOpen[] =
    "astra.incognito.warn_on_external_open";

// Default workspace ID for new incognito windows (string).
// Controls which workspace incognito windows start on.
// The active workspace in incognito is local to each window and does not
// affect the original profile's active workspace.
inline constexpr char kPrefIncognitoDefaultWorkspace[] =
    "astra.incognito.default_workspace";

// -- Window feature pref keys ---------------------------------------------
//
// Default window behavior preferences that persist via PrefService.
// Per-window runtime state (workspace, sidebar visibility per window, etc.)
// does NOT persist through PrefService — it travels with the window via
// session restore metadata.  These prefs control default BEHAVIORS for
// new windows and global window settings.
//
// Chromium owner: Browser / BrowserList for actual window management.
//   Astra only adds presentation preferences and convenience defaults.

// Whether new browser windows open maximized (bool).
// Default: false — new windows open at their default size.
//
// The actual window state on creation is owned by Chromium's Browser
// and BrowserWindow.  This pref controls Astra's default suggestion.
inline constexpr char kPrefWindowDefaultMaximized[] =
    "astra.window.default_maximized";

// Whether new windows open in the currently active workspace (bool).
// Default: true — new windows inherit the active workspace.
// When false, new windows always open in the default workspace.
inline constexpr char kPrefWindowNewWindowsInActiveWorkspace[] =
    "astra.window.new_windows_in_active_workspace";

// Whether window size and position are remembered across restarts (bool).
// Default: true — windows restore to their previous positions.
// When false, windows always open at default size/position.
//
// Chromium owns session restore and window positioning.  This pref
// controls whether Astra participates in saving/restoring window
// placement metadata.
inline constexpr char kPrefWindowRememberSizeAndPosition[] =
    "astra.window.remember_size_and_position";

// Default window width in pixels (int).
// Default: 1280.  Used when remember_size_and_position is false or for
// the first window on a fresh profile.
inline constexpr char kPrefWindowDefaultWidth[] =
    "astra.window.default_width";

// Default window height in pixels (int).
// Default: 800.
inline constexpr char kPrefWindowDefaultHeight[] =
    "astra.window.default_height";

// -- DevTools pref keys ----------------------------------------------------
//
// IMPORTANT: DevTools is fully owned by Chromium's DevTools subsystem.
// Astra does NOT store or manage DevTools engine state — it only adds
// presentation preferences and custom panel registration on top.
//
// These Astra-specific prefs control only presentation and UX behavior —
// never the actual DevTools engine or debugging state.
//
// Chromium subsystems reused:
//   - DevToolsWindow (chrome/browser/devtools/devtools_window.h)
//   - DevToolsManager (content/browser/devtools/devtools_manager.h)
//   - chrome.devtools.panels extension API
//   - PrefService — persistence for Astra-level DevTools settings

// Default dock state for new DevTools windows (string).
// Values: "bottom", "right", "left", "undocked", "minimized".
// Default: "bottom" — matches Chromium's default dock side.
//
// This controls the default dock position when DevTools is opened for the
// first time or when there's no previously saved dock state.
// The actual docking behavior is fully owned by Chromium's DevToolsWindow.
inline constexpr char kPrefDevToolsDefaultDockState[] =
    "astra.dev_tools.default_dock_state";

// Default panel to show when DevTools opens (string).
// Default: empty string — uses Chromium's default panel (Elements).
//
// This controls which panel is activated by default when DevTools opens.
// Can be set to an Astra panel ID (e.g. "workspace-inspector") or
// a standard Chromium DevTools panel name.
inline constexpr char kPrefDevToolsDefaultPanel[] =
    "astra.dev_tools.default_panel";

// Whether DevTools auto-opens for new tabs (bool).
// Default: false — DevTools must be manually opened.
//
// When true, Astra automatically opens DevTools for newly created tabs.
// Useful for development workflows.
inline constexpr char kPrefDevToolsAutoOpen[] =
    "astra.dev_tools.auto_open";

// Whether the Astra DevTools panel is visible/enabled (bool).
// Default: true — the Astra panel tab is shown in DevTools.
//
// This controls whether the Astra panel tab appears in the DevTools tab strip.
// The actual panel content and rendering is owned by Chromium's DevTools
// framework (extension or WebUI panel).
inline constexpr char kPrefDevToolsAstraPanelVisible[] =
    "astra.dev_tools.astra_panel_visible";

// Side of DevTools where the Astra panel sidebar appears (string).
// Values: "left", "right", "bottom".
// Default: "right".
//
// This controls which edge of the DevTools window the Astra panel tabs
// are displayed on.  It's a presentation preference only.
inline constexpr char kPrefDevToolsPanelSide[] =
    "astra.dev_tools.panel_side";

// Whether to show icons on Astra DevTools panel tabs (bool).
// Default: true — icons help with quick identification.
inline constexpr char kPrefDevToolsShowPanelIcons[] =
    "astra.dev_tools.show_panel_icons";

// Whether to show text labels on Astra DevTools panel tabs (bool).
// Default: true — labels make tabs readable.
inline constexpr char kPrefDevToolsShowPanelLabels[] =
    "astra.dev_tools.show_panel_labels";

// Width of the Astra DevTools panel sidebar in pixels (int).
// Clamped between kMinPanelWidth and kMaxPanelWidth.
// Default: 240.
inline constexpr char kPrefDevToolsPanelWidth[] =
    "astra.dev_tools.panel_width";

// Whether experimental Astra DevTools features are enabled (bool).
// Default: false — experiments are opt-in.
//
// Experimental features are work-in-progress panels or behaviors that
// are not yet ready for general use.
inline constexpr char kPrefDevToolsExperimentsEnabled[] =
    "astra.dev_tools.experiments_enabled";

// Whether the workspace panel auto-expands when DevTools opens (bool).
// Default: true — workspace information is the most commonly used.
inline constexpr char kPrefDevToolsAutoExpandWorkspacePanel[] =
    "astra.dev_tools.auto_expand_workspace_panel";

// Whether the Astra DevTools panel toolbar is shown (bool).
// Default: true — the toolbar provides panel-switching and actions.
inline constexpr char kPrefDevToolsShowPanelToolbar[] =
    "astra.dev_tools.show_panel_toolbar";

// Whether compact panel mode is enabled (bool).
// Default: false — standard spacing by default.
//
// Compact mode uses smaller tab sizes and reduced padding for users
// who want more space for panel content.
inline constexpr char kPrefDevToolsCompactMode[] =
    "astra.dev_tools.compact_mode";

// Whether to remember the last active panel across sessions (bool).
// Default: true — users typically return to the same panel.
inline constexpr char kPrefDevToolsRememberLastPanel[] =
    "astra.dev_tools.remember_last_panel";

// The last active Astra DevTools panel ID (string).
// Default: empty string.
//
// Only used when kPrefDevToolsRememberLastPanel is true.
// This is set automatically when the active panel changes.
inline constexpr char kPrefDevToolsLastActivePanel[] =
    "astra.dev_tools.last_active_panel";

// List of Astra DevTools panels with their state (list of dicts).
// Default: empty list — model uses built-in defaults when empty.
//
// Each entry is a dict with: id, title, icon, visible, pinned, position.
// Panel ordering, visibility, and pin state persist through this pref.
inline constexpr char kPrefDevToolsPanelList[] =
    "astra.dev_tools.panel_list";

// Theme for Astra DevTools panels (string).
// Values: "light", "dark", "system".
// Default: "system" — follows OS theme.
inline constexpr char kPrefDevToolsTheme[] =
    "astra.dev_tools.theme";

// -- Omnibox pref keys -----------------------------------------------------
//
// IMPORTANT: The omnibox / autocomplete system is fully owned by Chromium.
// Astra only adds custom actions and suggestion providers — it never
// replaces or reimplements AutocompleteController or AutocompleteProvider.
//
// Chromium owner: AutocompleteController
//   (components/omnibox/browser/autocomplete_controller.h)
// Chromium owner: AutocompleteProvider
//   (components/omnibox/browser/autocomplete_provider.h)
// Chromium owner: OmniboxEditModel (chrome/browser/ui/omnibox/omnibox_edit_model.h)
//
// These Astra-specific prefs control only presentation and Astra-specific
// behavior — never the underlying Chromium omnibox engine.

// Whether Astra suggestions appear in the omnibox dropdown (bool).
// When false, the Chromium patch point skips Astra suggestions entirely.
// Default: true — Astra suggestions are shown by default.
inline constexpr char kPrefOmniboxShowAstraSuggestions[] =
    "astra.omnibox.show_astra_suggestions";

// Maximum number of Astra suggestions to show in the omnibox (int).
// Caps the number of Astra results among all omnibox suggestions.
// Default: 5.
inline constexpr char kPrefOmniboxMaxAstraSuggestions[] =
    "astra.omnibox.max_astra_suggestions";

// Position of Astra suggestions relative to native ones (string).
// Values: "top" or "bottom".
// Default: "bottom" — Astra suggestions appear after native ones.
inline constexpr char kPrefOmniboxSuggestionPosition[] =
    "astra.omnibox.suggestion_position";

// Whether the Astra omnibox provider is enabled (bool).
// When disabled, no Astra suggestions are generated at all.
// Default: true.
inline constexpr char kPrefOmniboxProviderEnabled[] =
    "astra.omnibox.provider_enabled";

// Dictionary of category enablement states (dict: string -> bool).
// Keys: "workspace", "tab", "navigation", "tool".
// Controls which action categories appear in suggestions.
// Default: all categories enabled.
inline constexpr char kPrefOmniboxCategoryEnabled[] =
    "astra.omnibox.category_enabled";

// List of recently executed Astra omnibox actions (list of dicts).
// Each dict has: action_type (int), payload (string).
// Most recent first.
// Default: empty list — no recent actions on a fresh profile.
inline constexpr char kPrefOmniboxRecentActions[] =
    "astra.omnibox.recent_actions";

// Maximum number of recent actions to remember (int).
// Default: 10.
inline constexpr char kPrefOmniboxMaxRecentActions[] =
    "astra.omnibox.max_recent_actions";

// -- Omnibox decoration pref keys ------------------------------------------
//
// Presentation settings for the Astra location bar / omnibox decoration.
// The decoration adds Astra-specific action buttons to the Chrome omnibox.
// These prefs control the decoration's appearance, position, and which
// action buttons are shown.
//
// Chromium owner: LocationBarView
//   (chrome/browser/ui/views/location_bar/location_bar_view.h)
// Chromium owner: OmniboxView (chrome/browser/ui/views/omnibox/omnibox_view.h)
//
// The actual omnibox engine is fully owned by Chromium.  These prefs
// control only the Astra decoration overlay — never the omnibox itself.

// Whether the Astra decoration is shown in the omnibox (bool).
// When false, no Astra UI appears in the location bar.
// Default: true — Astra decoration is shown by default.
inline constexpr char kPrefOmniboxDecorationShowDecoration[] =
    "astra.omnibox.decoration.show_decoration";

// Position of the Astra decoration in the omnibox (string).
// Values: "left" (leading, before the security icon) or
//         "right" (trailing, after the star / other icons).
// Default: "left" — leading edge of the omnibox.
inline constexpr char kPrefOmniboxDecorationPosition[] =
    "astra.omnibox.decoration.position";

// Maximum number of action buttons visible in the decoration (int).
// Actions beyond this limit are shown in an overflow menu.
// Clamped to [2, 8].
// Default: 4 — shows the 4 most important actions.
inline constexpr char kPrefOmniboxDecorationMaxVisibleActions[] =
    "astra.omnibox.decoration.max_visible_actions";

// Whether to show text labels on action buttons (bool).
// When true, buttons display both an icon and a text label.
// Default: false — icon-only buttons for a cleaner look.
inline constexpr char kPrefOmniboxDecorationShowLabels[] =
    "astra.omnibox.decoration.show_labels";

// Icon size for decoration action buttons (int).
// 0 = small (16dp), 1 = medium (20dp), 2 = large (24dp).
// Default: 1 = medium.
inline constexpr char kPrefOmniboxDecorationIconSize[] =
    "astra.omnibox.decoration.icon_size";

// Button style for decoration action buttons (int).
// 0 = icon only, 1 = icon with label, 2 = chip (rounded pill).
// Default: 0 = icon only.
inline constexpr char kPrefOmniboxDecorationButtonStyle[] =
    "astra.omnibox.decoration.button_style";

// Whether the decoration is only visible when the omnibox is focused (bool).
// When true, the decoration is hidden until the user focuses the omnibox.
// Default: false — always visible.
inline constexpr char kPrefOmniboxDecorationShowOnFocusOnly[] =
    "astra.omnibox.decoration.show_on_focus_only";

// Whether the workspace switcher button is shown (bool).
// Default: true — workspace switching is a core Astra feature.
inline constexpr char kPrefOmniboxDecorationShowWorkspace[] =
    "astra.omnibox.decoration.show_workspace";

// Whether the focus mode button is shown (bool).
// Default: true.
inline constexpr char kPrefOmniboxDecorationShowFocusMode[] =
    "astra.omnibox.decoration.show_focus_mode";

// Whether the screenshot button is shown (bool).
// Default: true.
inline constexpr char kPrefOmniboxDecorationShowScreenshot[] =
    "astra.omnibox.decoration.show_screenshot";

// Whether the quick note button is shown (bool).
// Default: true.
inline constexpr char kPrefOmniboxDecorationShowNote[] =
    "astra.omnibox.decoration.show_note";

// Whether the split view button is shown (bool).
// Default: true.
inline constexpr char kPrefOmniboxDecorationShowSplitView[] =
    "astra.omnibox.decoration.show_split_view";

// Whether the reading list button is shown (bool).
// Default: true.
inline constexpr char kPrefOmniboxDecorationShowReadingList[] =
    "astra.omnibox.decoration.show_reading_list";

// Whether the translate button is shown (bool).
// Default: true.
inline constexpr char kPrefOmniboxDecorationShowTranslate[] =
    "astra.omnibox.decoration.show_translate";

// Whether the share button is shown (bool).
// Default: true.
inline constexpr char kPrefOmniboxDecorationShowShare[] =
    "astra.omnibox.decoration.show_share";

// Whether an overflow menu is shown for hidden actions (bool).
// When true, a "..." button appears when there are more actions than
// max_visible_actions.  Clicking it shows all actions in a dropdown.
// Default: true.
inline constexpr char kPrefOmniboxDecorationOverflowMenu[] =
    "astra.omnibox.decoration.overflow_menu";

// Whether the decoration expands on hover (bool).
// When true, hovering over the decoration reveals more buttons or labels.
// Default: false — keep the decoration stable by default.
inline constexpr char kPrefOmniboxDecorationHoverExpansion[] =
    "astra.omnibox.decoration.hover_expansion";

// -- Recent tabs pref keys -------------------------------------------------
//
// IMPORTANT: Recent tabs / recently closed tabs are fully owned by Chromium's
// TabRestoreService and SessionService.  Astra does NOT store or manage
// recent tab data — it only projects TabRestoreService state through
// AstraRecentTabsHelper and adds presentation preferences.
//
// Chromium owner: TabRestoreService (chrome/browser/sessions/tab_restore_service.h)
// Chromium owner: SessionService (chrome/browser/sessions/session_service.h)
//
// The Astra-specific prefs below control only presentation and UX behavior —
// never the actual recent tab data itself.

// Maximum number of recently closed tabs to show in the Astra sidebar
// and command palette (int).
// Default: 10.
//
// This is a presentation limit — TabRestoreService may track more entries.
inline constexpr char kPrefRecentTabsMaxCount[] = "astra.recent_tabs.max_count";

// Whether the recently closed section is shown in the Astra sidebar (bool).
// Default: true — the section is shown by default.
//
// This is a presentation preference — it only controls sidebar visibility.
// Recently closed tabs are still tracked by Chromium regardless.
inline constexpr char kPrefRecentTabsShowInSidebar[] =
    "astra.recent_tabs.show_in_sidebar";

// Whether timestamps are shown next to recently closed tabs (bool).
// Default: true — "time ago" labels are shown by default.
//
// This is a presentation preference for the Astra UI.
inline constexpr char kPrefRecentTabsShowTimestamps[] =
    "astra.recent_tabs.show_timestamps";

// -- History helper pref keys -----------------------------------------------
//
// IMPORTANT: Browsing history is fully owned by Chromium's HistoryService.
// Astra does NOT store or manage history data — it only projects
// HistoryService state through AstraHistoryHelper and adds presentation
// preferences.
//
// Chromium owner: HistoryService (components/history/core/browser/history_service.h)
// Chromium owner: TopSites (components/history/core/browser/top_sites.h)
//
// The Astra-specific history prefs below control only presentation and UX
// behavior — never the actual history data itself.

// Whether the history section is shown in the Astra sidebar (bool).
// Default: true — history is a core sidebar section.
//
// This is a presentation preference — it only controls sidebar visibility.
// History is still tracked by Chromium regardless.
inline constexpr char kPrefHistoryShowInSidebar[] =
    "astra.history.show_in_sidebar";

// Default sort order for history listings (string).
// Values: "time_desc", "time_asc", "most_visited".
// Default: "time_desc" — most recent first.
inline constexpr char kPrefHistorySortOrder[] =
    "astra.history.sort_order";

// Maximum number of history results per query (int).
// Default: 50.
// Clamped: 10-500.
inline constexpr char kPrefHistoryMaxResults[] =
    "astra.history.max_results";

// Whether favicons are shown in history listings (bool).
// Default: true — favicons help with visual identification.
inline constexpr char kPrefHistoryShowFavicons[] =
    "astra.history.show_favicons";

// Whether visit counts are shown in history listings (bool).
// Default: false — visit count is secondary information.
inline constexpr char kPrefHistoryShowVisitCount[] =
    "astra.history.show_visit_count";

// Whether visit times are shown in history listings (bool).
// Default: true — timestamps are essential for history.
inline constexpr char kPrefHistoryShowVisitTime[] =
    "astra.history.show_visit_time";

// History display mode (string).
// Values: "list", "compact", "card".
// Default: "list" — standard list view.
inline constexpr char kPrefHistoryDisplayMode[] =
    "astra.history.display_mode";

// Whether history results are grouped by date (bool).
// Default: true — grouping by date is a common history pattern.
inline constexpr char kPrefHistoryGroupByDate[] =
    "astra.history.group_by_date";

// Maximum number of history items per day group (int).
// Default: 20.
// Clamped: 5-100.
inline constexpr char kPrefHistoryItemsPerDay[] =
    "astra.history.items_per_day";

// Whether only typed URLs are shown in history (bool).
// Default: false — show all history by default.
inline constexpr char kPrefHistoryShowTypedUrlsOnly[] =
    "astra.history.show_typed_urls_only";

// Whether history deletion is enabled (bool).
// Default: true — deletion is allowed by default.
// When false, delete/clear operations are no-ops (for policy-restricted
// environments or child accounts).
inline constexpr char kPrefHistoryDeletionEnabled[] =
    "astra.history.deletion_enabled";

// Whether old history is auto-deleted based on retention policy (bool).
// Default: false — off by default, user opts in.
inline constexpr char kPrefHistoryAutoDelete[] =
    "astra.history.auto_delete";

// History retention period in days (int).
// Default: 90 days.
// Clamped: 1-3650 (about 10 years).
inline constexpr char kPrefHistoryRetentionDays[] =
    "astra.history.retention_days";

// -- Tab search pref keys --------------------------------------------------
//
// Tab search presentation preferences.
//
// IMPORTANT: Actual tab data is fully owned by Chromium's TabStripModel,
// TabRestoreService, and BookmarkModel.  Astra only adds search UI and
// presentation preferences on top.
//
// Chromium owner: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)
// Chromium owner: TabSearchBubbleHost
//   (chrome/browser/ui/views/tab_search/tab_search_bubble_host.h)

// Maximum number of visible results in tab search (int).
// Default: 15 — matches the bubble's kMaxResults constant.
// This is a presentation preference.
inline constexpr char kPrefTabSearchMaxVisible[] =
    "astra.tab_search.max_visible";

// Whether URLs are shown in tab search results (bool).
// Default: true — URLs help users identify tabs.
inline constexpr char kPrefTabSearchShowUrls[] =
    "astra.tab_search.show_urls";

// Whether tab thumbnails/previews are shown in tab search (bool).
// Default: true — thumbnails help with visual recognition.
//
// TODO(astra): Wire up to actual tab thumbnails from ThumbnailTabHelper.
// Chromium component: chrome/browser/thumbnail/thumbnail_tab_helper.h
inline constexpr char kPrefTabSearchShowThumbnails[] =
    "astra.tab_search.show_thumbnails";

// Whether the "recent tabs" section appears on empty query (bool).
// Default: true — recent tabs provide quick access to frequently used tabs.
inline constexpr char kPrefTabSearchShowRecentSection[] =
    "astra.tab_search.show_recent_section";

// Default sort order for tab search results (int, maps to
// AstraTabSearchSortOrder enum).
// 0 = kRelevance (rank by search score),
// 1 = kRecency (most recent first),
// 2 = kAlphabetical (A-Z by title).
// Default: 0 = relevance — best matches first.
inline constexpr char kPrefTabSearchSortOrder[] =
    "astra.tab_search.sort_order";
inline constexpr int kDefaultTabSearchSortOrder = 0;

// -- Glance / peek pref keys ------------------------------------------------
//
// Glance presentation preferences and behavior settings.
//
// The glance / peek feature shows a small preview bubble for tabs and links.
// These prefs control how the glance looks and behaves.
//
// Chromium subsystem reused: views::BubbleDialogDelegateView
//   (ui/views/bubble/bubble_dialog_delegate_view.h)
// Actual tab/WebContents state is owned by Chromium's TabStripModel and
// content::WebContents.  These prefs control only presentation and behavior.

// Default display mode for glance (string).
// Values: "compact", "expanded".
// Default: "expanded" — full preview with action bar and status bar.
inline constexpr char kPrefGlanceDefaultDisplayMode[] =
    "astra.glance.default_display_mode";

// Show delay for hover-triggered glance in milliseconds (int).
// How long the user must hover over a tab/link before the glance appears.
// Default: 500ms — prevents accidental glances from quick mouse moves.
inline constexpr char kPrefGlanceShowDelayMs[] =
    "astra.glance.show_delay_ms";

// Auto-hide delay for glance in milliseconds (int).
// How long after the mouse leaves the glance area before it closes.
// Default: 300ms — hysteresis to prevent flicker when mouse briefly leaves.
inline constexpr char kPrefGlanceAutoHideDelayMs[] =
    "astra.glance.auto_hide_delay_ms";

// Compact mode width in pixels (int).
// Default: 420.
inline constexpr char kPrefGlanceCompactWidth[] =
    "astra.glance.compact_width";

// Compact mode height in pixels (int).
// Default: 280.
inline constexpr char kPrefGlanceCompactHeight[] =
    "astra.glance.compact_height";

// Expanded mode width in pixels (int).
// Default: 560.
inline constexpr char kPrefGlanceExpandedWidth[] =
    "astra.glance.expanded_width";

// Expanded mode height in pixels (int).
// Default: 420.
inline constexpr char kPrefGlanceExpandedHeight[] =
    "astra.glance.expanded_height";

// Whether the action bar is shown in expanded mode (bool).
// Default: true — action bar provides quick access to common actions.
inline constexpr char kPrefGlanceShowActionBar[] =
    "astra.glance.show_action_bar";

// Whether the status bar is shown in expanded mode (bool).
// Default: true — status bar shows URL and security info.
inline constexpr char kPrefGlanceShowStatusBar[] =
    "astra.glance.show_status_bar";

// Whether the resize handle is shown (bool).
// Default: true — allows users to resize the glance bubble.
inline constexpr char kPrefGlanceShowResizeHandle[] =
    "astra.glance.show_resize_handle";

// Whether hover peek (hover-triggered glance) is enabled (bool).
// When false, glance only appears on explicit user action (keyboard shortcut,
// context menu).
// Default: true — hover peek is a core peek feature.
inline constexpr char kPrefGlanceHoverPeekEnabled[] =
    "astra.glance.hover_peek_enabled";

// Default glance position / anchor side (string).
// Values: "left", "right", "top", "bottom".
// Controls which side of the anchor the glance bubble appears on.
// Default: "right" — appears to the right of sidebar items.
inline constexpr char kPrefGlanceDefaultPosition[] =
    "astra.glance.default_position";

// Size preset for quick resize (string).
// Values: "small", "medium", "large".
// Default: "medium" — the standard expanded size.
inline constexpr char kPrefGlanceSizePreset[] =
    "astra.glance.size_preset";

// Whether the glance is pinned open by default (bool).
// When pinned, the glance stays open until explicitly closed (no auto-hide).
// Default: false — glance auto-hides normally.
inline constexpr char kPrefGlancePinnedByDefault[] =
    "astra.glance.pinned_by_default";

// Whether the glance remembers its last used size (bool).
// When true, the next glance uses the same size as the previous one.
// Default: true — size persistence is convenient for users.
inline constexpr char kPrefGlanceRememberSize[] =
    "astra.glance.remember_size";

// Maximum number of recent glance entries to remember (int).
// Default: 10.
inline constexpr char kPrefGlanceRecentMaxCount[] =
    "astra.glance.recent_max_count";

// Whether to show the settings button in the glance header (bool).
// Default: true — quick access to glance settings.
inline constexpr char kPrefGlanceShowSettingsButton[] =
    "astra.glance.show_settings_button";

// Whether animations are enabled for glance (bool).
// When false, entrance/exit animations are skipped.
// Default: true — smooth animations improve perceived quality.
inline constexpr char kPrefGlanceAnimationsEnabled[] =
    "astra.glance.animations_enabled";

// Small size preset width in pixels (int).
// Default: 320.
inline constexpr char kPrefGlanceSmallWidth[] =
    "astra.glance.small_width";

// Small size preset height in pixels (int).
// Default: 240.
inline constexpr char kPrefGlanceSmallHeight[] =
    "astra.glance.small_height";

// Large size preset width in pixels (int).
// Default: 720.
inline constexpr char kPrefGlanceLargeWidth[] =
    "astra.glance.large_width";

// Large size preset height in pixels (int).
// Default: 540.
inline constexpr char kPrefGlanceLargeHeight[] =
    "astra.glance.large_height";

// List of recent glance URLs (list of strings).
// Most recent first.  Capped at kPrefGlanceRecentMaxCount.
//
// This tracks URLs that the user has previewed with glance, for quick
// re-preview or history.
inline constexpr char kPrefGlanceRecentUrls[] =
    "astra.glance.recent_urls";

// -- Tab hover preview pref keys -------------------------------------------
//
// Tab hover preview presentation preferences and behavior settings.
//
// These control how Astra's tab hover preview cards look and behave.
// Tab data (title, URL, favicon, media state) is owned by Chromium's
// TabStripModel and content::WebContents. These prefs control only the
// presentation and behavior of the hover preview UI.
//
// Chromium subsystem reused: PrefService (persistence).
// The model (AstraTabHoverModel) reads these prefs and projects them
// into the view layer.  Views never read prefs directly.

// Whether tab hover cards are shown on hover (bool).
// When false, no hover preview appears when hovering over tabs.
// Default: true — hover previews are a core tab identification feature.
inline constexpr char kPrefTabHoverShowCards[] =
    "astra.tab_hover.show_cards";

// Hover show delay in milliseconds (int).
// How long the user must hover over a tab before the preview card appears.
// Default: 500ms — prevents accidental previews from quick mouse passes.
inline constexpr char kPrefTabHoverShowDelayMs[] =
    "astra.tab_hover.show_delay_ms";

// Hover hide delay in milliseconds (int).
// How long after the mouse leaves the tab before the preview card closes.
// Default: 300ms — hysteresis to prevent flicker from brief mouse exits.
inline constexpr char kPrefTabHoverHideDelayMs[] =
    "astra.tab_hover.hide_delay_ms";

// Whether the preview image/thumbnail is shown (bool).
// Default: true — thumbnails help with visual tab identification.
//
// TODO(astra): Wire up to actual tab thumbnails from TabThumbnailTracker.
// Chromium component: chrome/browser/ui/tabs/tab_thumbnail_tracker.h
inline constexpr char kPrefTabHoverShowPreviewImage[] =
    "astra.tab_hover.show_preview_image";

// Whether the tab title is shown in the hover card (bool).
// Default: true — title is essential for identifying tabs.
inline constexpr char kPrefTabHoverShowTitle[] =
    "astra.tab_hover.show_title";

// Whether the tab URL/domain is shown in the hover card (bool).
// Default: true — URL helps verify the tab's identity.
inline constexpr char kPrefTabHoverShowUrl[] =
    "astra.tab_hover.show_url";

// Whether the favicon is shown in the hover card (bool).
// Default: true — favicons provide quick visual recognition.
inline constexpr char kPrefTabHoverShowFavicon[] =
    "astra.tab_hover.show_favicon";

// Whether the close button is shown in the hover card (bool).
// Default: true — quick tab closing from the hover card is convenient.
inline constexpr char kPrefTabHoverShowCloseButton[] =
    "astra.tab_hover.show_close_button";

// Preview image size (int, maps to AstraTabHoverPreviewSize enum).
// 0 = small, 1 = medium, 2 = large.
// Default: 1 = medium — balance of preview detail and compactness.
inline constexpr char kPrefTabHoverPreviewImageSize[] =
    "astra.tab_hover.preview_image_size";

// Card position relative to the tab (int, maps to AstraTabHoverCardPosition).
// 0 = above, 1 = below, 2 = auto.
// Default: 2 = auto — position adapts to available screen space.
inline constexpr char kPrefTabHoverCardPosition[] =
    "astra.tab_hover.card_position";

// Whether peek mode is enabled (bool).
// When true, holding hover longer shows an expanded peek preview.
// Default: true — peek mode provides quick deeper previews.
inline constexpr char kPrefTabHoverEnablePeekMode[] =
    "astra.tab_hover.enable_peek_mode";

// Peek activation delay in milliseconds (int).
// How long to hold hover before expanding to peek mode.
// Default: 1500ms — gives users time to read the basic preview first.
inline constexpr char kPrefTabHoverPeekDelayMs[] =
    "astra.tab_hover.peek_delay_ms";

// Whether the tab index number is shown (bool).
// When true, a badge shows the tab's 1-based index in the strip.
// Default: false — most users don't need index numbers.
inline constexpr char kPrefTabHoverShowTabIndex[] =
    "astra.tab_hover.show_tab_index";

// Whether the media playback indicator is shown (bool).
// When true, shows an icon for tabs playing audio or video.
// Default: true — media state is useful context.
inline constexpr char kPrefTabHoverShowMediaIndicator[] =
    "astra.tab_hover.show_media_indicator";

// Whether the mute toggle button is shown (bool).
// When true, users can mute/unmute tabs directly from the hover card.
// Default: true — quick mute control is convenient.
inline constexpr char kPrefTabHoverShowMuteButton[] =
    "astra.tab_hover.show_mute_button";

// Whether animations are enabled for tab hover cards (bool).
// When false, entrance/exit and peek expansion animations are skipped.
// Default: true — smooth animations improve perceived quality.
inline constexpr char kPrefTabHoverAnimationEnabled[] =
    "astra.tab_hover.animation_enabled";

// Defaults ------------------------------------------------------------------

inline constexpr int kDefaultSidebarWidth = 280;
inline constexpr bool kDefaultSidebarVisible = true;
inline constexpr bool kDefaultSidebarPinned = true;
inline constexpr char kDefaultSidebarPosition[] = "left";
inline constexpr bool kDefaultSidebarAutoHide = false;
inline constexpr bool kDefaultSidebarShowSectionIcons = true;
inline constexpr bool kDefaultSidebarShowSectionLabels = true;
inline constexpr bool kDefaultSidebarCompactMode = false;
inline constexpr bool kDefaultSidebarAutoHideOnTabClick = false;
inline constexpr bool kDefaultSidebarShowTabCountBadges = true;
inline constexpr bool kDefaultSidebarShowWorkspaceBadge = true;
inline constexpr bool kDefaultSidebarShowTabGroupsSection = true;
inline constexpr bool kDefaultSidebarShowHistorySection = true;
inline constexpr bool kDefaultSidebarShowRecentlyClosedSection = true;
inline constexpr bool kDefaultSidebarShowReadingListSection = true;
inline constexpr bool kDefaultSidebarShowNotesSection = true;
inline constexpr bool kDefaultSidebarShowDownloadsSection = true;
inline constexpr bool kDefaultSidebarShowPasswordsSection = true;
inline constexpr bool kDefaultSidebarShowExtensionsSection = true;
inline constexpr bool kDefaultSidebarAnimationEnabled = true;
inline constexpr bool kDefaultSidebarRememberLastSection = true;
inline constexpr char kDefaultSidebarLastActiveSection[] = "open_tabs";
inline constexpr char kDefaultSidebarDefaultActiveSection[] = "open_tabs";
inline constexpr int kSidebarMinWidth = 200;
inline constexpr int kSidebarMaxWidth = 500;
inline constexpr char kDefaultSplitViewOrientation[] = "horizontal";
inline constexpr double kDefaultSplitViewRatio = 0.5;
inline constexpr bool kDefaultSplitViewEnabled = true;
inline constexpr bool kDefaultSplitViewSnapToPresets = false;
inline constexpr bool kDefaultSplitViewDividerVisible = true;
inline constexpr bool kDefaultSplitViewRememberRatio = true;
inline constexpr bool kDefaultSplitViewMinimapEnabled = false;
inline constexpr bool kDefaultMemorySaverEnabled = true;
inline constexpr int kDefaultMemorySaverTimeoutMinutes = 5;
inline constexpr bool kDefaultMemorySaverSuspendActiveWorkspace = false;
inline constexpr char kDefaultPiPDefaultSize[] = "medium";
inline constexpr bool kDefaultPiPAlwaysOnTop = true;
inline constexpr char kDefaultPiPSnapPosition[] = "bottom_right";
inline constexpr bool kDefaultPiPSnapToCornerEnabled = true;
inline constexpr double kDefaultPiPOpacity = 1.0;
inline constexpr bool kDefaultPiPAutoPipOnTabSwitch = false;
inline constexpr int kDefaultPiPMaxWindows = 3;
inline constexpr bool kDefaultPiPControlsAutoHide = true;
inline constexpr int kDefaultPiPControlsAutoHideDelayMs = 3000;
inline constexpr bool kDefaultPiPControlsShowTopBar = true;
inline constexpr bool kDefaultPiPControlsShowBottomBar = true;
inline constexpr bool kDefaultPiPControlsShowResizeHandle = true;
inline constexpr double kDefaultPiPControlsOpacity = 1.0;
inline constexpr char kDefaultPiPControlsDefaultSizePreset[] = "medium";
inline constexpr bool kDefaultPiPControlsShowAlwaysOnTopButton = true;
inline constexpr bool kDefaultPiPControlsShowPlaybackControls = true;
inline constexpr bool kDefaultPiPControlsShowSkipButtons = true;
inline constexpr int kDefaultPiPControlsSkipDurationSeconds = 10;
inline constexpr bool kDefaultSearchShowDefaultEngine = true;
inline constexpr bool kDefaultSearchShowOtherEngines = false;
inline constexpr bool kDefaultSearchSuggestionsEnabled = true;
inline constexpr int kDefaultSearchMaxRecentQueries = 10;
inline constexpr bool kDefaultHighContrastMode = false;
inline constexpr bool kDefaultReducedMotion = false;
inline constexpr double kDefaultAccessibilityFontScale = 1.0;
inline constexpr bool kDefaultCaretBrowsingEnabled = false;
inline constexpr bool kDefaultStickyKeysEnabled = false;
inline constexpr bool kDefaultSlowKeysEnabled = false;
inline constexpr int kDefaultSlowKeysDelayMs = 500;
inline constexpr bool kDefaultMouseKeysEnabled = false;
inline constexpr bool kDefaultLargeCursorEnabled = false;
inline constexpr int kDefaultLargeCursorSize = 3;
inline constexpr bool kDefaultMagnifierEnabled = false;
inline constexpr double kDefaultMagnifierScale = 2.0;
inline constexpr int kDefaultMagnifierType = 0;  // 0 = docked
inline constexpr bool kDefaultSelectToSpeakEnabled = false;
inline constexpr bool kDefaultDictationEnabled = false;
inline constexpr bool kDefaultVirtualKeyboardEnabled = false;
inline constexpr int kDefaultMinimumFontSize = 0;
inline constexpr int kDefaultFontWeightAdjustment = 0;
inline constexpr double kDefaultLetterSpacing = 1.0;
inline constexpr double kDefaultLineHeight = 1.0;
inline constexpr int kDefaultContrastLevel = 0;  // 0 = normal
inline constexpr bool kDefaultNightLightEnabled = false;
inline constexpr int kDefaultColorTemperature = 50;
inline constexpr bool kDefaultColorInversionEnabled = false;
inline constexpr int kDefaultAnimationReductionLevel = 0;  // 0 = off
inline constexpr bool kDefaultAutoScrollEnabled = false;
inline constexpr double kDefaultScrollSpeed = 1.0;
inline constexpr char kDefaultThemeAccentColor[] = "";
inline constexpr char kDefaultThemeMode[] = "system";
inline constexpr char kDefaultThemeScheme[] = "";
inline constexpr bool kDefaultThemeUseWorkspaceAccent = true;
inline constexpr int kDefaultThemeCustomAccentColor = 0xFF5B8FF9;  // Google Blue 500
inline constexpr bool kDefaultThemeHighContrast = false;
inline constexpr bool kDefaultThemeShowAccentOnTabs = true;
inline constexpr bool kDefaultThemeShowAccentOnSidebar = true;
inline constexpr double kDefaultThemeAccentIntensity = 1.0;
inline constexpr bool kDefaultThemeUseAutoSchedule = false;
inline constexpr char kDefaultThemeAutoLightStart[] = "07:00";
inline constexpr char kDefaultThemeAutoDarkStart[] = "19:00";
inline constexpr int kDefaultCommandPaletteRecentCount = 10;
inline constexpr bool kDefaultCommandPaletteShowWorkspaces = true;
inline constexpr int kDefaultCommandPaletteMaxVisible = 20;
inline constexpr bool kDefaultCommandPaletteShowDescriptions = true;
inline constexpr bool kDefaultCommandPaletteShowShortcuts = true;
inline constexpr bool kDefaultCommandPaletteShowRecentSection = true;

// Command delegate defaults
inline constexpr int kDefaultCommandRecentMax = 20;

// Omnibox defaults
inline constexpr bool kDefaultOmniboxShowAstraSuggestions = true;
inline constexpr int kDefaultOmniboxMaxAstraSuggestions = 5;
inline constexpr char kDefaultOmniboxSuggestionPosition[] = "bottom";
inline constexpr bool kDefaultOmniboxProviderEnabled = true;
inline constexpr int kDefaultOmniboxMaxRecentActions = 10;

// Omnibox decoration defaults
inline constexpr bool kDefaultOmniboxDecorationShowDecoration = true;
inline constexpr char kDefaultOmniboxDecorationPosition[] = "left";
inline constexpr int kDefaultOmniboxDecorationMaxVisibleActions = 4;
inline constexpr bool kDefaultOmniboxDecorationShowLabels = false;
inline constexpr int kDefaultOmniboxDecorationIconSize = 1;  // medium
inline constexpr int kDefaultOmniboxDecorationButtonStyle = 0;  // icon only
inline constexpr bool kDefaultOmniboxDecorationShowOnFocusOnly = false;
inline constexpr bool kDefaultOmniboxDecorationShowWorkspace = true;
inline constexpr bool kDefaultOmniboxDecorationShowFocusMode = true;
inline constexpr bool kDefaultOmniboxDecorationShowScreenshot = true;
inline constexpr bool kDefaultOmniboxDecorationShowNote = true;
inline constexpr bool kDefaultOmniboxDecorationShowSplitView = true;
inline constexpr bool kDefaultOmniboxDecorationShowReadingList = true;
inline constexpr bool kDefaultOmniboxDecorationShowTranslate = true;
inline constexpr bool kDefaultOmniboxDecorationShowShare = true;
inline constexpr bool kDefaultOmniboxDecorationOverflowMenu = true;
inline constexpr bool kDefaultOmniboxDecorationHoverExpansion = false;

// Focus mode / pomodoro defaults
inline constexpr int kDefaultFocusShortBreakMinutes = 5;
inline constexpr int kDefaultFocusLongBreakMinutes = 15;
inline constexpr int kDefaultFocusLongBreakInterval = 4;
inline constexpr bool kDefaultFocusAutoStartNextPhase = false;
inline constexpr int kDefaultFocusTotalFocusSeconds = 0;
inline constexpr int kDefaultFocusSessionsCompleted = 0;
inline constexpr int kDefaultFocusCyclesCompleted = 0;
inline constexpr bool kDefaultFocusWarningsEnabled = true;
inline constexpr char kDefaultFocusAutoStartTime[] = "09:00";
inline constexpr char kDefaultFocusAutoEndTime[] = "17:00";
inline constexpr bool kDefaultFocusPaused = false;
inline constexpr int kDefaultFocusPausedRemainingSeconds = 0;
inline constexpr int kDefaultFocusDefaultDurationMinutes = 25;

// Focus mode presentation / UI defaults
inline constexpr bool kDefaultFocusShowIndicator = true;
inline constexpr int kDefaultFocusIndicatorPosition = 1;  // 1 = top_right
inline constexpr int kDefaultFocusIndicatorStyle = 1;     // 1 = full
inline constexpr bool kDefaultFocusShowTimerInIndicator = true;
inline constexpr bool kDefaultFocusShowSessionStats = true;
inline constexpr bool kDefaultFocusBlockDistractingSites = false;
inline constexpr bool kDefaultFocusNotificationSound = true;
inline constexpr bool kDefaultFocusBreakReminders = false;
inline constexpr int kDefaultFocusBreakIntervalMinutes = 25;
inline constexpr int kDefaultFocusBreakDurationMinutes = 5;
inline constexpr bool kDefaultFocusDimNonFocusTabs = false;
inline constexpr bool kDefaultFocusHideSidebar = true;

// Password helper defaults
inline constexpr bool kDefaultPasswordShowSuggestions = true;
inline constexpr bool kDefaultPasswordAutoFillEnabled = true;
inline constexpr bool kDefaultPasswordManagerShortcut = true;
inline constexpr bool kDefaultPasswordShowHealth = true;
inline constexpr bool kDefaultPasswordBreachAlertsEnabled = true;
inline constexpr int kDefaultPasswordMaxSidebarPasswords = 20;
inline constexpr bool kDefaultPasswordBreachDetected = false;

// Safety helper defaults
inline constexpr bool kDefaultSafeBrowsingEnabled = true;
inline constexpr int kDefaultSafeBrowsingLevel = 1;
inline constexpr bool kDefaultEnhancedProtection = false;
inline constexpr bool kDefaultWarnOnPasswordReuse = true;
inline constexpr int kDefaultPasswordProtectionLevel = 1;
inline constexpr bool kDefaultShowSecurityButton = true;
inline constexpr bool kDefaultShowThreatNotifications = true;
inline constexpr bool kDefaultBlockDangerousDownloads = true;
inline constexpr bool kDefaultWarnOnDangerousDownloads = true;
inline constexpr bool kDefaultAutoReportSafetyIssues = false;
inline constexpr bool kDefaultMixedContentWarning = true;
inline constexpr bool kDefaultShowSiteInfoButton = true;
inline constexpr bool kDefaultSafetyCheckReminders = true;
inline constexpr bool kDefaultCookieProtection = true;

// Extension helper defaults
inline constexpr bool kDefaultExtensionShowToolbar = true;
inline constexpr bool kDefaultExtensionShowInSidebar = true;
inline constexpr bool kDefaultExtensionManagerShortcut = true;
inline constexpr bool kDefaultExtensionShowRecommended = true;
inline constexpr int kDefaultExtensionMaxSidebarCount = 10;
inline constexpr char kDefaultExtensionSortOrder[] = "name";

// Downloads helper defaults
inline constexpr bool kDefaultDownloadsShowInSidebar = true;
inline constexpr bool kDefaultDownloadsShowNotifications = true;
inline constexpr bool kDefaultDownloadsAutoOpen = false;
inline constexpr char kDefaultDownloadsSortOrder[] = "newest_first";
inline constexpr int kDefaultDownloadsMaxRecent = 20;
inline constexpr int kMinDownloadsMaxRecent = 5;
inline constexpr int kMaxDownloadsMaxRecent = 100;
inline constexpr bool kDefaultDownloadsShowSpeed = true;
inline constexpr bool kDefaultDownloadsShowFileSize = true;
inline constexpr bool kDefaultDownloadsShowProgress = true;
inline constexpr char kDefaultDownloadsDisplayMode[] = "list";
inline constexpr bool kDefaultDownloadsPromptForLocation = true;
inline constexpr bool kDefaultDownloadsSafeBrowsingWarnings = true;

// Incognito defaults
inline constexpr bool kDefaultIncognitoShowSidebarBadge = true;
inline constexpr bool kDefaultIncognitoConfirmCloseAll = false;
inline constexpr bool kDefaultIncognitoWarnOnExternalOpen = false;
inline constexpr char kDefaultIncognitoDefaultWorkspace[] = "default";

// Session restore defaults
inline constexpr char kDefaultSessionRestoreMode[] = "all";
inline constexpr bool kDefaultSessionRestoreEnabled = true;
inline constexpr double kDefaultSessionRestoreLastSaveTime = 0.0;
inline constexpr int kDefaultSessionRestoreLastTabCount = 0;
inline constexpr int kDefaultSessionRestoreLastWindowCount = 0;
inline constexpr int kDefaultSessionRestoreLastWorkspaceCount = 0;
inline constexpr bool kDefaultSessionRestoreLazyLoading = true;
inline constexpr bool kDefaultSessionRestoreShowPrompt = false;
inline constexpr char kDefaultSessionRestoreLoadMode[] = "smart";
inline constexpr bool kDefaultSessionRestoreOnStartup = true;
inline constexpr int kDefaultSessionRestoreMaxTabsPerWorkspace = 0;
inline constexpr bool kDefaultSessionRestoreWorkspacesIndividually = false;
inline constexpr int kDefaultSessionRestoreAutoSaveInterval = 5;
inline constexpr int kDefaultSessionRestoreMaxSavedSessions = 10;

// Window feature defaults
inline constexpr bool kDefaultWindowDefaultMaximized = false;
inline constexpr bool kDefaultWindowNewWindowsInActiveWorkspace = true;
inline constexpr bool kDefaultWindowRememberSizeAndPosition = true;
inline constexpr int kDefaultWindowDefaultWidth = 1280;
inline constexpr int kDefaultWindowDefaultHeight = 800;

// Workspace overview defaults
inline constexpr int kDefaultWorkspaceOverviewViewMode = 0;      // 0 = grid
inline constexpr int kDefaultWorkspaceOverviewCardSize = 1;      // 1 = medium
inline constexpr bool kDefaultWorkspaceOverviewShowStatistics = true;
inline constexpr bool kDefaultWorkspaceOverviewShowHibernation = true;
inline constexpr bool kDefaultWorkspaceOverviewShowQuickAdd = true;
inline constexpr char kDefaultWorkspaceOverviewSortOrder[] = "manual";
inline constexpr int kDefaultWorkspaceOverviewBulkDeleteThreshold = 1;
inline constexpr bool kDefaultWorkspaceOverviewIncludeHibernated = true;

// Workspace window defaults
inline constexpr bool kDefaultWorkspaceWindowRememberPlacement = true;
inline constexpr bool kDefaultWorkspaceWindowNewInActiveWorkspace = true;
inline constexpr bool kDefaultWorkspaceWindowAutoTile = false;
inline constexpr int kDefaultWorkspaceWindowDefaultWindowCount = 1;

// Workspace import/export defaults
inline constexpr bool kDefaultWorkspaceExportIncludeTabs = true;
inline constexpr bool kDefaultWorkspaceExportIncludeSettings = true;
inline constexpr bool kDefaultWorkspaceExportIncludeFavorites = true;
inline constexpr bool kDefaultWorkspaceExportIncludeMetadata = false;
inline constexpr int kDefaultWorkspaceImportMode = 0;  // 0 = merge
inline constexpr int kDefaultWorkspaceImportConflictResolution = 0;  // 0 = rename
inline constexpr bool kDefaultWorkspaceImportOpenTabs = true;
inline constexpr bool kDefaultWorkspaceImportApplyFavorites = true;

// DevTools defaults
inline constexpr char kDefaultDevToolsDefaultDockState[] = "bottom";
inline constexpr char kDefaultDevToolsDefaultPanel[] = "";
inline constexpr bool kDefaultDevToolsAutoOpen = false;
inline constexpr bool kDefaultDevToolsAstraPanelVisible = true;
inline constexpr char kDefaultDevToolsPanelSide[] = "right";
inline constexpr bool kDefaultDevToolsShowPanelIcons = true;
inline constexpr bool kDefaultDevToolsShowPanelLabels = true;
inline constexpr int kDefaultDevToolsPanelWidth = 240;
inline constexpr bool kDefaultDevToolsExperimentsEnabled = false;
inline constexpr bool kDefaultDevToolsAutoExpandWorkspacePanel = true;
inline constexpr bool kDefaultDevToolsShowPanelToolbar = true;
inline constexpr bool kDefaultDevToolsCompactMode = false;
inline constexpr bool kDefaultDevToolsRememberLastPanel = true;
inline constexpr char kDefaultDevToolsLastActivePanel[] = "";
inline constexpr char kDefaultDevToolsTheme[] = "system";

// Recent tabs helper defaults
inline constexpr int kDefaultRecentTabsMaxCount = 10;
inline constexpr bool kDefaultRecentTabsShowInSidebar = true;
inline constexpr bool kDefaultRecentTabsShowTimestamps = true;

// History helper defaults
inline constexpr bool kDefaultHistoryShowInSidebar = true;
inline constexpr char kDefaultHistorySortOrder[] = "time_desc";
inline constexpr int kDefaultHistoryMaxResults = 50;
inline constexpr bool kDefaultHistoryShowFavicons = true;
inline constexpr bool kDefaultHistoryShowVisitCount = false;
inline constexpr bool kDefaultHistoryShowVisitTime = true;
inline constexpr char kDefaultHistoryDisplayMode[] = "list";
inline constexpr bool kDefaultHistoryGroupByDate = true;
inline constexpr int kDefaultHistoryItemsPerDay = 20;
inline constexpr bool kDefaultHistoryShowTypedUrlsOnly = false;
inline constexpr bool kDefaultHistoryDeletionEnabled = true;
inline constexpr bool kDefaultHistoryAutoDelete = false;
inline constexpr int kDefaultHistoryRetentionDays = 90;

// Tab search defaults
inline constexpr int kDefaultTabSearchMaxVisible = 15;
inline constexpr bool kDefaultTabSearchShowUrls = true;
inline constexpr bool kDefaultTabSearchShowThumbnails = true;
inline constexpr bool kDefaultTabSearchShowRecentSection = true;

// Glance defaults
inline constexpr char kDefaultGlanceDefaultDisplayMode[] = "expanded";
inline constexpr int kDefaultGlanceShowDelayMs = 500;
inline constexpr int kDefaultGlanceAutoHideDelayMs = 300;
inline constexpr int kDefaultGlanceCompactWidth = 420;
inline constexpr int kDefaultGlanceCompactHeight = 280;
inline constexpr int kDefaultGlanceExpandedWidth = 560;
inline constexpr int kDefaultGlanceExpandedHeight = 420;
inline constexpr bool kDefaultGlanceShowActionBar = true;
inline constexpr bool kDefaultGlanceShowStatusBar = true;
inline constexpr bool kDefaultGlanceShowResizeHandle = true;
inline constexpr bool kDefaultGlanceHoverPeekEnabled = true;
inline constexpr char kDefaultGlanceDefaultPosition[] = "right";
inline constexpr char kDefaultGlanceSizePreset[] = "medium";
inline constexpr bool kDefaultGlancePinnedByDefault = false;
inline constexpr bool kDefaultGlanceRememberSize = true;
inline constexpr int kDefaultGlanceRecentMaxCount = 10;
inline constexpr bool kDefaultGlanceShowSettingsButton = true;
inline constexpr bool kDefaultGlanceAnimationsEnabled = true;
inline constexpr int kDefaultGlanceSmallWidth = 320;
inline constexpr int kDefaultGlanceSmallHeight = 240;
inline constexpr int kDefaultGlanceLargeWidth = 720;
inline constexpr int kDefaultGlanceLargeHeight = 540;

// Tab hover defaults
inline constexpr bool kDefaultTabHoverShowCards = true;
inline constexpr int kDefaultTabHoverShowDelayMs = 500;
inline constexpr int kDefaultTabHoverHideDelayMs = 300;
inline constexpr bool kDefaultTabHoverShowPreviewImage = true;
inline constexpr bool kDefaultTabHoverShowTitle = true;
inline constexpr bool kDefaultTabHoverShowUrl = true;
inline constexpr bool kDefaultTabHoverShowFavicon = true;
inline constexpr bool kDefaultTabHoverShowCloseButton = true;
inline constexpr int kDefaultTabHoverPreviewImageSize = 1;  // 1 = medium
inline constexpr int kDefaultTabHoverCardPosition = 2;      // 2 = auto
inline constexpr bool kDefaultTabHoverEnablePeekMode = true;
inline constexpr int kDefaultTabHoverPeekDelayMs = 1500;
inline constexpr bool kDefaultTabHoverShowTabIndex = false;
inline constexpr bool kDefaultTabHoverShowMediaIndicator = true;
inline constexpr bool kDefaultTabHoverShowMuteButton = true;
inline constexpr bool kDefaultTabHoverAnimationEnabled = true;

// -- Profile menu pref keys -------------------------------------------------
//
// Presentation and behavior settings for the Astra profile menu.
// These control how the profile menu looks and behaves.
//
// Chromium owns: profile data, identity, sync status
//   (Profile, IdentityManager, SyncService).
// Astra owns: menu presentation settings, workspace list integration,
//   menu position, display modes.
//
// The profile menu model reads these prefs via PrefService and projects
// them into the menu views.  The model is the single source of truth
// for menu state; views never read prefs directly.

// Whether the workspace list is shown in the profile menu (bool).
// When true, the menu includes a section with all workspaces for
// quick switching.  When false, only profile info and actions are shown.
// Default: true — workspace switching is a core Astra feature.
inline constexpr char kPrefProfileMenuShowWorkspaces[] =
    "astra.profile_menu.show_workspaces";

// Maximum number of workspaces shown in the menu before scrolling (int).
// Clamped between 3 and 10.  Controls the height of the workspace
// list section in the profile menu.
// Default: 6 — shows a reasonable number without taking too much space.
inline constexpr char kPrefProfileMenuMaxWorkspaces[] =
    "astra.profile_menu.max_workspaces";

// Whether the profile avatar is shown in the menu header (bool).
// When false, the header shows only name and email text.
// Default: true — avatars help with profile identification.
inline constexpr char kPrefProfileMenuShowAvatar[] =
    "astra.profile_menu.show_avatar";

// Whether sync status is shown in the profile menu (bool).
// When true, the menu shows sync state (syncing, signed-in, error).
// Default: true — sync status is important profile context.
inline constexpr char kPrefProfileMenuShowSyncStatus[] =
    "astra.profile_menu.show_sync_status";

// Workspace display mode in the menu (int).
// 0 = icons_only — just color dots, no names
// 1 = names_only — just names, no color dots
// 2 = icons_and_names — both color dots and names (default)
// Default: 2 = icons_and_names — most informative.
inline constexpr char kPrefProfileMenuWorkspaceDisplayMode[] =
    "astra.profile_menu.workspace_display_mode";

// Menu position / alignment (string).
// "left" — menu appears to the left of the anchor
// "right" — menu appears to the right of the anchor
// Default: "right" — matches Chromium's default avatar menu position.
inline constexpr char kPrefProfileMenuPosition[] =
    "astra.profile_menu.position";

// Whether to show the "recently closed" section in the menu (bool).
// When true, the menu includes a section with recently closed tabs
// for quick restoration.
// Default: true — quick access to recently closed tabs is useful.
inline constexpr char kPrefProfileMenuShowRecentlyClosed[] =
    "astra.profile_menu.show_recently_closed";

// Whether to show the sign-in promo in the menu (bool).
// When true and the user is not signed in, the menu shows a
// "Sign in to Chrome" promo banner.
// Default: true — matches Chromium's default behavior.
inline constexpr char kPrefProfileMenuShowSignInPromo[] =
    "astra.profile_menu.show_sign_in_promo";

// Profile menu defaults
inline constexpr bool kDefaultProfileMenuShowWorkspaces = true;
inline constexpr int kDefaultProfileMenuMaxWorkspaces = 6;
inline constexpr bool kDefaultProfileMenuShowAvatar = true;
inline constexpr bool kDefaultProfileMenuShowSyncStatus = true;
inline constexpr int kDefaultProfileMenuWorkspaceDisplayMode = 2;  // icons_and_names
inline constexpr char kDefaultProfileMenuPosition[] = "right";
inline constexpr bool kDefaultProfileMenuShowRecentlyClosed = true;
inline constexpr bool kDefaultProfileMenuShowSignInPromo = true;

// -- New tab page (NTP) pref keys ------------------------------------------
//
// Presentation and behavior settings for the Astra new tab page.
// These control how the NTP looks and which sections are visible.
//
// Chromium owns: actual browsing data (top sites, history, downloads).
// Astra owns: NTP presentation layout, custom shortcuts, background style,
//   greeting settings, section visibility, card styling.
//
// The model (AstraNewTabModel) reads these prefs and projects them into
// view state.  Views never read prefs directly.

// Whether the greeting section is shown (bool).
// Default: true — greeting provides a friendly welcome.
inline constexpr char kPrefNtpShowGreeting[] = "astra.ntp.show_greeting";

// Whether the search bar is shown (bool).
// Default: true — search is the primary NTP action.
inline constexpr char kPrefNtpShowSearchBar[] = "astra.ntp.show_search_bar";

// Whether workspace cards are shown (bool).
// Default: true — workspace quick access is a core Astra feature.
inline constexpr char kPrefNtpShowWorkspaceCards[] =
    "astra.ntp.show_workspace_cards";

// Whether shortcuts are shown (bool).
// Default: true — shortcuts are the primary NTP content.
inline constexpr char kPrefNtpShowShortcuts[] = "astra.ntp.show_shortcuts";

// Whether the recently closed section is shown (bool).
// Default: true — quick access to recently closed tabs.
inline constexpr char kPrefNtpShowRecentlyClosed[] =
    "astra.ntp.show_recently_closed";

// Whether quick actions are shown (bool).
// Default: true — quick actions provide fast access to common features.
inline constexpr char kPrefNtpShowQuickActions[] =
    "astra.ntp.show_quick_actions";

// Number of shortcut columns (int).
// Clamped between 3 and 8.
// Default: 4.
inline constexpr char kPrefNtpShortcutColumns[] =
    "astra.ntp.shortcut_columns";

// Maximum number of workspace cards shown (int).
// Clamped between 3 and 10.
// Default: 5.
inline constexpr char kPrefNtpMaxWorkspacesShown[] =
    "astra.ntp.max_workspaces_shown";

// Maximum number of recently closed items shown (int).
// Clamped between 3 and 10.
// Default: 8.
inline constexpr char kPrefNtpMaxRecentlyClosedShown[] =
    "astra.ntp.max_recently_closed_shown";

// Shortcut layout mode (int).
// 0 = grid, 1 = list.
// Default: 0 = grid.
inline constexpr char kPrefNtpShortcutLayoutMode[] =
    "astra.ntp.shortcut_layout_mode";

// Workspace card style (int).
// 0 = compact, 1 = full.
// Default: 1 = full.
inline constexpr char kPrefNtpWorkspaceCardStyle[] =
    "astra.ntp.workspace_card_style";

// Background style (int).
// 0 = simple (solid/theme), 1 = gradient, 2 = image.
// Default: 0 = simple.
inline constexpr char kPrefNtpBackgroundStyle[] =
    "astra.ntp.background_style";

// Custom background image URL (string).
// Only meaningful when background_style is kImage.
// Default: empty string.
inline constexpr char kPrefNtpCustomBackgroundUrl[] =
    "astra.ntp.custom_background_url";

// Whether "most visited" sites are shown as shortcuts (bool).
// When false, only user-added custom shortcuts appear.
// Default: true — most visited sites provide useful defaults.
inline constexpr char kPrefNtpShowMostVisited[] =
    "astra.ntp.show_most_visited";

// Greeting style (int).
// 0 = formal ("Good morning"), 1 = casual ("Hey there"),
// 2 = minimal (just the time).
// Default: 0 = formal.
inline constexpr char kPrefNtpGreetingStyle[] =
    "astra.ntp.greeting_style";

// Custom shortcut list (list of dicts).
// Each dict contains: title (string), url (string), icon_url (string, optional).
// Default: empty list — user starts with no custom shortcuts.
inline constexpr char kPrefNtpCustomShortcuts[] =
    "astra.ntp.custom_shortcuts";

// Quick action list (list of strings).
// Ordered list of quick action IDs shown in the NTP quick actions row.
// Default: [new_workspace, screenshot, focus_mode, history, downloads, bookmarks]
inline constexpr char kPrefNtpQuickActions[] = "astra.ntp.quick_actions";

// Recently closed entries (list of dicts).
// Astra-projected copy of recently closed tabs for the NTP.
// Note: Real data comes from TabRestoreService; this is a cached projection
// for the NTP model layer.
// TODO(astra): Replace with direct TabRestoreService observation.
inline constexpr char kPrefNtpRecentlyClosed[] =
    "astra.ntp.recently_closed";

// NTP defaults
inline constexpr bool kDefaultNtpShowGreeting = true;
inline constexpr bool kDefaultNtpShowSearchBar = true;
inline constexpr bool kDefaultNtpShowWorkspaceCards = true;
inline constexpr bool kDefaultNtpShowShortcuts = true;
inline constexpr bool kDefaultNtpShowRecentlyClosed = true;
inline constexpr bool kDefaultNtpShowQuickActions = true;
inline constexpr int kDefaultNtpShortcutColumns = 4;
inline constexpr int kMinNtpShortcutColumns = 3;
inline constexpr int kMaxNtpShortcutColumns = 8;
inline constexpr int kDefaultNtpMaxWorkspacesShown = 5;
inline constexpr int kMinNtpMaxWorkspacesShown = 3;
inline constexpr int kMaxNtpMaxWorkspacesShown = 10;
inline constexpr int kDefaultNtpMaxRecentlyClosedShown = 8;
inline constexpr int kMinNtpMaxRecentlyClosedShown = 3;
inline constexpr int kMaxNtpMaxRecentlyClosedShown = 10;
inline constexpr int kDefaultNtpShortcutLayoutMode = 0;  // grid
inline constexpr int kDefaultNtpWorkspaceCardStyle = 1;  // full
inline constexpr int kDefaultNtpBackgroundStyle = 0;    // simple
inline constexpr char kDefaultNtpCustomBackgroundUrl[] = "";
inline constexpr bool kDefaultNtpShowMostVisited = true;
inline constexpr int kDefaultNtpGreetingStyle = 0;  // formal

// -- Screenshot pref keys --------------------------------------------------
//
// Screenshot presentation and behavior preferences.
//
// Actual screenshot capture is owned by Chromium's screenshot subsystem and
// AstraScreenshotService. These prefs control only the presentation and
// default behavior of the Astra screenshot UI (capture bubble, region
// overlay, etc.).
//
// Chromium owner: ScreenshotManager / ScreenshotService
//   (chrome/browser/screenshot/, content/browser/screenshot/)
// Patch point: ScreenshotService / ScreenshotManager UI delegate.

// Default screenshot capture type (int, maps to AstraScreenshotType).
// 0 = kVisibleArea, 1 = kFullPage, 2 = kRegion.
// Default: 0 = visible area.
inline constexpr char kPrefScreenshotDefaultCaptureType[] =
    "astra.screenshot.default_capture_type";
inline constexpr int kDefaultScreenshotDefaultCaptureType = 0;

// Whether the capture bubble is shown after taking a screenshot (bool).
// When false, screenshots are saved/copied silently without the bubble UI.
// Default: true — the bubble provides useful feedback and actions.
inline constexpr char kPrefScreenshotShowCaptureBubble[] =
    "astra.screenshot.show_capture_bubble";
inline constexpr bool kDefaultScreenshotShowCaptureBubble = true;

// Whether the capture bubble auto-dismisses after the configured delay (bool).
// Default: true — auto-dismiss prevents the bubble from lingering.
inline constexpr char kPrefScreenshotAutoDismissBubble[] =
    "astra.screenshot.auto_dismiss_bubble";
inline constexpr bool kDefaultScreenshotAutoDismissBubble = true;

// Auto-dismiss delay in seconds (int).
// Default: 5 seconds.
inline constexpr char kPrefScreenshotAutoDismissDelaySeconds[] =
    "astra.screenshot.auto_dismiss_delay_seconds";
inline constexpr int kDefaultScreenshotAutoDismissDelaySeconds = 5;

// Image format for screenshots (int, maps to AstraScreenshotImageFormat).
// 0 = kPng, 1 = kJpeg, 2 = kWebP.
// Default: 0 = PNG (lossless).
inline constexpr char kPrefScreenshotImageFormat[] =
    "astra.screenshot.image_format";
inline constexpr int kDefaultScreenshotImageFormat = 0;

// JPEG quality percentage (int, 0-100).
// Only applies when image format is JPEG.
// Default: 85 — good quality with reasonable file size.
inline constexpr char kPrefScreenshotJpegQuality[] =
    "astra.screenshot.jpeg_quality";
inline constexpr int kDefaultScreenshotJpegQuality = 85;
inline constexpr int kMinScreenshotJpegQuality = 1;
inline constexpr int kMaxScreenshotJpegQuality = 100;

// Whether to show the filename in the capture bubble (bool).
// Default: true.
inline constexpr char kPrefScreenshotShowFilenameInBubble[] =
    "astra.screenshot.show_filename_in_bubble";
inline constexpr bool kDefaultScreenshotShowFilenameInBubble = true;

// Whether to show image dimensions in the capture bubble (bool).
// Default: true — dimensions provide useful context.
inline constexpr char kPrefScreenshotShowDimensionsInBubble[] =
    "astra.screenshot.show_dimensions_in_bubble";
inline constexpr bool kDefaultScreenshotShowDimensionsInBubble = true;

// Whether to show the file size in the capture bubble (bool).
// Default: true — file size helps users gauge if the image is too large.
inline constexpr char kPrefScreenshotShowFileSizeInBubble[] =
    "astra.screenshot.show_file_size_in_bubble";
inline constexpr bool kDefaultScreenshotShowFileSizeInBubble = true;

// Default save location (string).
// Values: "downloads", "clipboard", "ask".
// Default: "downloads" — screenshots save to the Downloads folder.
inline constexpr char kPrefScreenshotDefaultSaveLocation[] =
    "astra.screenshot.default_save_location";
inline constexpr char kDefaultScreenshotDefaultSaveLocation[] = "downloads";

// Whether to automatically copy the screenshot to clipboard after capture (bool).
// Default: false — users explicitly copy via the bubble button.
inline constexpr char kPrefScreenshotCopyToClipboardAfterCapture[] =
    "astra.screenshot.copy_to_clipboard_after_capture";
inline constexpr bool kDefaultScreenshotCopyToClipboardAfterCapture = false;

// Whether to show a grid overlay during region selection (bool).
// Default: false — grid can be distracting for casual use.
inline constexpr char kPrefScreenshotShowGridInRegionSelection[] =
    "astra.screenshot.show_grid_in_region_selection";
inline constexpr bool kDefaultScreenshotShowGridInRegionSelection = false;

// Whether to show a magnifier loupe during region selection (bool).
// Default: true — magnifier helps with pixel-perfect selections.
inline constexpr char kPrefScreenshotShowMagnifierInRegionSelection[] =
    "astra.screenshot.show_magnifier_in_region_selection";
inline constexpr bool kDefaultScreenshotShowMagnifierInRegionSelection = true;

// Region selection aspect ratio lock mode (int).
// 0 = kFree (no lock), 1 = kRatio4x3, 2 = kRatio16x9, 3 = kRatio1x1.
// Default: 0 = free aspect ratio.
inline constexpr char kPrefScreenshotRegionAspectRatioLock[] =
    "astra.screenshot.region_aspect_ratio_lock";
inline constexpr int kDefaultScreenshotRegionAspectRatioLock = 0;

// Grid size in pixels for region selection snap-to-grid (int).
// Default: 20 pixels.
inline constexpr char kPrefScreenshotGridSizePixels[] =
    "astra.screenshot.grid_size_pixels";
inline constexpr int kDefaultScreenshotGridSizePixels = 20;
inline constexpr int kMinScreenshotGridSizePixels = 5;
inline constexpr int kMaxScreenshotGridSizePixels = 200;

// Whether snap-to-grid is enabled for region selection (bool).
// Default: false.
inline constexpr char kPrefScreenshotSnapToGrid[] =
    "astra.screenshot.snap_to_grid";
inline constexpr bool kDefaultScreenshotSnapToGrid = false;

// Maximum number of recent screenshots to remember (int).
// Default: 10.
inline constexpr char kPrefScreenshotMaxRecentCaptures[] =
    "astra.screenshot.max_recent_captures";
inline constexpr int kDefaultScreenshotMaxRecentCaptures = 10;
inline constexpr int kMinScreenshotMaxRecentCaptures = 1;
inline constexpr int kMaxScreenshotMaxRecentCaptures = 100;

// -- Notifications ---------------------------------------------------------
//
// Notification presentation and behavior preferences.
//
// Actual notification delivery is owned by Chromium's MessageCenter and
// NotificationPlatformBridge.  Astra adds presentation metadata and
// per-source filtering on top of Chromium's notification system.
//
// Chromium owner: MessageCenter / NotificationService
//   (chrome/browser/notifications/notification_service.h)
//   (ui/message_center/message_center.h)
// Astra projection: AstraNotificationService
//
// TODO(astra): Wire into Chromium's notification system as an observer.
//   Patch point: message_center::MessageCenter::AddObserver().

// Whether notifications are enabled globally (bool).
// Default: true — notifications are on by default.
inline constexpr char kPrefNotificationsEnabled[] =
    "astra.notifications.enabled";
inline constexpr bool kDefaultNotificationsEnabled = true;

// Whether do-not-disturb mode is enabled (bool).
// When DND is on, notifications are recorded but not shown as popups.
// Default: false — DND is off by default.
inline constexpr char kPrefNotificationDoNotDisturb[] =
    "astra.notifications.do_not_disturb";
inline constexpr bool kDefaultNotificationDoNotDisturb = false;

// Whether message previews are shown in notifications (bool).
// When false, only the title is shown in popup notifications.
// Default: true — show full message previews.
inline constexpr char kPrefNotificationShowPreviews[] =
    "astra.notifications.show_previews";
inline constexpr bool kDefaultNotificationShowPreviews = true;

// Whether notification sounds are played (bool).
// Default: true — sound enabled by default.
inline constexpr char kPrefNotificationSoundEnabled[] =
    "astra.notifications.sound_enabled";
inline constexpr bool kDefaultNotificationSoundEnabled = true;

// Auto-dismiss timeout in seconds (int).
// Notifications auto-close after this duration.
// Clamped: [1, 60].
// Default: 8 seconds.
inline constexpr char kPrefNotificationTimeoutSeconds[] =
    "astra.notifications.timeout_seconds";
inline constexpr int kDefaultNotificationTimeoutSeconds = 8;
inline constexpr int kMinNotificationTimeoutSeconds = 1;
inline constexpr int kMaxNotificationTimeoutSeconds = 60;

// Maximum number of visible notifications at once (int).
// Clamped: [1, 20].
// Default: 5.
inline constexpr char kPrefNotificationMaxVisible[] =
    "astra.notifications.max_visible";
inline constexpr int kDefaultNotificationMaxVisible = 5;
inline constexpr int kMinNotificationMaxVisible = 1;
inline constexpr int kMaxNotificationMaxVisible = 20;

// Notification position on screen (string).
// Values: "top_right", "top_left", "bottom_right", "bottom_left".
// Default: "top_right".
inline constexpr char kPrefNotificationPosition[] =
    "astra.notifications.position";
inline constexpr char kDefaultNotificationPosition[] = "top_right";

// Whether to show the notification icon (bool).
// Default: true.
inline constexpr char kPrefNotificationShowIcon[] =
    "astra.notifications.show_icon";
inline constexpr bool kDefaultNotificationShowIcon = true;

// Whether to show the notification timestamp (bool).
// Default: true.
inline constexpr char kPrefNotificationShowTimestamp[] =
    "astra.notifications.show_timestamp";
inline constexpr bool kDefaultNotificationShowTimestamp = true;

// Whether to show the close button on notifications (bool).
// Default: true.
inline constexpr char kPrefNotificationShowCloseButton[] =
    "astra.notifications.show_close_button";
inline constexpr bool kDefaultNotificationShowCloseButton = true;

// Notification visual style (string).
// Values: "default", "compact", "minimal".
// Default: "default".
inline constexpr char kPrefNotificationStyle[] =
    "astra.notifications.style";
inline constexpr char kDefaultNotificationStyle[] = "default";

// Whether similar notifications are stacked / grouped (bool).
// Default: true — stacking reduces visual clutter.
inline constexpr char kPrefNotificationStackNotifications[] =
    "astra.notifications.stack_notifications";
inline constexpr bool kDefaultNotificationStackNotifications = true;

// Whether quiet mode is enabled (bool).
// Quiet mode: no sound, no popups, just badge counter.
// Default: false.
inline constexpr char kPrefNotificationQuietMode[] =
    "astra.notifications.quiet_mode";
inline constexpr bool kDefaultNotificationQuietMode = false;

// Maximum number of history items to remember (int).
// Clamped: [10, 1000].
// Default: 100.
inline constexpr char kPrefNotificationHistorySize[] =
    "astra.notifications.history_size";
inline constexpr int kDefaultNotificationHistorySize = 100;
inline constexpr int kMinNotificationHistorySize = 10;
inline constexpr int kMaxNotificationHistorySize = 1000;

// -- Autofill helper -------------------------------------------------------
//
// Autofill state is fully owned by Chromium's autofill subsystem.
// Astra only projects the state and adds presentation preferences.
//
// Chromium component: PersonalDataManager
//   (components/autofill/core/browser/personal_data_manager.h)
// Chromium component: AutofillProfile
//   (components/autofill/core/browser/data_model/autofill_profile.h)
// Chromium component: CreditCard
//   (components/autofill/core/browser/data_model/credit_card.h)
//
// These Astra-specific prefs control only presentation — they never
// store autofill data. See AstraAutofillHelper for the projection layer.

// Whether autofill is globally enabled in Astra UI (bool).
// This controls whether Astra UI surfaces autofill functionality.
// The actual autofill behavior is controlled by Chromium's settings.
// Default: true — autofill is on by default.
inline constexpr char kPrefAutofillEnabled[] = "astra.autofill.enabled";
inline constexpr bool kDefaultAutofillEnabled = true;

// Whether address autofill is enabled in Astra UI (bool).
// Default: true — address autofill is on by default.
inline constexpr char kPrefAddressAutofillEnabled[] =
    "astra.autofill.address_enabled";
inline constexpr bool kDefaultAddressAutofillEnabled = true;

// Whether credit card autofill is enabled in Astra UI (bool).
// Default: true — credit card autofill is on by default.
inline constexpr char kPrefCreditCardAutofillEnabled[] =
    "astra.autofill.credit_card_enabled";
inline constexpr bool kDefaultCreditCardAutofillEnabled = true;

// Whether auto sign-in is enabled in Astra UI (bool).
// Auto sign-in automatically fills and submits login forms.
// Default: true — auto sign-in is on by default.
inline constexpr char kPrefAutosignInEnabled[] =
    "astra.autofill.autosign_in_enabled";
inline constexpr bool kDefaultAutosignInEnabled = true;

// Whether the autofill popup is shown (bool).
// Default: true — popup is shown by default.
inline constexpr char kPrefShowAutofillPopup[] =
    "astra.autofill.show_popup";
inline constexpr bool kDefaultShowAutofillPopup = true;

// Autofill popup position (string).
// Values: "below_field", "above_field", "auto".
// Default: "auto".
inline constexpr char kPrefAutofillPopupPosition[] =
    "astra.autofill.popup_position";
inline constexpr char kDefaultAutofillPopupPosition[] = "auto";

// Maximum number of suggestions shown in the autofill popup (int).
// Default: 6.
// Clamped: 1 to 20.
inline constexpr char kPrefAutofillMaxSuggestions[] =
    "astra.autofill.max_suggestions";
inline constexpr int kDefaultAutofillMaxSuggestions = 6;
inline constexpr int kMinAutofillMaxSuggestions = 1;
inline constexpr int kMaxAutofillMaxSuggestions = 20;

// Whether suggestion icons are shown (bool).
// Default: true — icons are shown by default.
inline constexpr char kPrefShowSuggestionIcons[] =
    "astra.autofill.show_suggestion_icons";
inline constexpr bool kDefaultShowSuggestionIcons = true;

// Whether suggestion labels are shown (bool).
// Default: true — labels are shown by default.
inline constexpr char kPrefShowSuggestionLabels[] =
    "astra.autofill.show_suggestion_labels";
inline constexpr bool kDefaultShowSuggestionLabels = true;

// Whether suggestion subtext is shown (bool).
// Default: true — subtext is shown by default.
inline constexpr char kPrefShowSuggestionSubtext[] =
    "astra.autofill.show_suggestion_subtext";
inline constexpr bool kDefaultShowSuggestionSubtext = true;

// Whether autofill triggers on tap/click (bool).
// Default: true — autofill on tap is on by default.
inline constexpr char kPrefAutofillOnTap[] = "astra.autofill.on_tap";
inline constexpr bool kDefaultAutofillOnTap = true;

// Suggestions sort order (string).
// Values: "most_recent", "most_used", "alphabetical".
// Default: "most_recent".
inline constexpr char kPrefAutofillSuggestionsSortOrder[] =
    "astra.autofill.suggestions_sort_order";
inline constexpr char kDefaultAutofillSuggestionsSortOrder[] = "most_recent";

// Whether credit card network icons are shown (bool).
// Default: true — card icons are shown by default.
inline constexpr char kPrefShowCreditCardIcons[] =
    "astra.autofill.show_credit_card_icons";
inline constexpr bool kDefaultShowCreditCardIcons = true;

// Whether quick checkout flow is enabled (bool).
// Quick checkout provides a streamlined payment flow.
// Default: false — off by default (experimental feature).
inline constexpr char kPrefAutofillQuickCheckout[] =
    "astra.autofill.quick_checkout";
inline constexpr bool kDefaultAutofillQuickCheckout = false;

// -- Sync helper ---------------------------------------------------------
//
// Sync state is fully owned by Chromium's SyncService.
// Astra only projects the state and adds presentation preferences.
//
// Chromium component: SyncService (components/sync/service/sync_service.h)
// Chromium factory: ProfileSyncServiceFactory
//   (chrome/browser/sync/profile_sync_service_factory.h)
//
// These Astra-specific prefs control only presentation — they never
// store sync data. See AstraSyncHelper for the projection layer.

// Whether sync is enabled in Astra UI (bool).
// Default: true — sync is on by default.
inline constexpr char kPrefSyncEnabled[] = "astra.sync.enabled";
inline constexpr bool kDefaultSyncEnabled = true;

// Whether sync status is shown in Astra UI (bool).
// Default: true — status is shown by default.
inline constexpr char kPrefShowSyncStatus[] = "astra.sync.show_status";
inline constexpr bool kDefaultShowSyncStatus = true;

// Whether sync errors are shown in Astra UI (bool).
// Default: true — errors are shown by default.
inline constexpr char kPrefShowSyncErrors[] = "astra.sync.show_errors";
inline constexpr bool kDefaultShowSyncErrors = true;

// Whether sync only happens on WiFi (bool).
// Default: false — sync on any network.
inline constexpr char kPrefSyncOnWifiOnly[] = "astra.sync.wifi_only";
inline constexpr bool kDefaultSyncOnWifiOnly = false;

// Sync frequency setting (string).
// Values: "auto", "hourly", "daily".
// Default: "auto" — Chromium manages sync timing.
inline constexpr char kPrefSyncFrequency[] = "astra.sync.frequency";
inline constexpr char kDefaultSyncFrequency[] = "auto";

// Whether automatic sync is enabled (bool).
// Default: true — auto sync is on by default.
inline constexpr char kPrefAutoSync[] = "astra.sync.auto_sync";
inline constexpr bool kDefaultAutoSync = true;

// Whether all sync data is encrypted (bool).
// Default: false — matches Chromium default behavior.
inline constexpr char kPrefEncryptAllData[] = "astra.sync.encrypt_all";
inline constexpr bool kDefaultEncryptAllData = false;

// Whether the account avatar is shown in UI (bool).
// Default: true — avatar is shown by default.
inline constexpr char kPrefShowAccountAvatar[] = "astra.sync.show_avatar";
inline constexpr bool kDefaultShowAccountAvatar = true;

// Whether last sync time is shown in UI (bool).
// Default: true — last sync time is shown by default.
inline constexpr char kPrefShowLastSyncTime[] = "astra.sync.show_last_sync_time";
inline constexpr bool kDefaultShowLastSyncTime = true;

// Whether bookmarks are synced (bool).
// Default: true — bookmarks sync on by default.
inline constexpr char kPrefSyncBookmarks[] = "astra.sync.bookmarks";
inline constexpr bool kDefaultSyncBookmarks = true;

// Whether passwords are synced (bool).
// Default: true — passwords sync on by default.
inline constexpr char kPrefSyncPasswords[] = "astra.sync.passwords";
inline constexpr bool kDefaultSyncPasswords = true;

// Whether history is synced (bool).
// Default: true — history sync on by default.
inline constexpr char kPrefSyncHistory[] = "astra.sync.history";
inline constexpr bool kDefaultSyncHistory = true;

// Whether open tabs are synced (bool).
// Default: true — tabs sync on by default.
inline constexpr char kPrefSyncTabs[] = "astra.sync.tabs";
inline constexpr bool kDefaultSyncTabs = true;

// Whether settings are synced (bool).
// Default: true — settings sync on by default.
inline constexpr char kPrefSyncSettings[] = "astra.sync.settings";
inline constexpr bool kDefaultSyncSettings = true;

// Whether extensions are synced (bool).
// Default: true — extensions sync on by default.
inline constexpr char kPrefSyncExtensions[] = "astra.sync.extensions";
inline constexpr bool kDefaultSyncExtensions = true;

// Whether autofill is synced (bool).
// Default: true — autofill sync on by default.
inline constexpr char kPrefSyncAutofill[] = "astra.sync.autofill";
inline constexpr bool kDefaultSyncAutofill = true;

// Whether reading list is synced (bool).
// Default: true — reading list sync on by default.
inline constexpr char kPrefSyncReadingList[] = "astra.sync.reading_list";
inline constexpr bool kDefaultSyncReadingList = true;

// Whether the sync icon is shown in the toolbar (bool).
// Default: true — icon is shown by default.
inline constexpr char kPrefShowSyncIconInToolbar[] =
    "astra.sync.show_icon_in_toolbar";
inline constexpr bool kDefaultShowSyncIconInToolbar = true;

}  // namespace prefs
}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_PREFS_H_
