#include "astra/browser/astra_prefs.h"

#include "components/prefs/pref_registry_simple.h"

namespace astra {
namespace prefs {

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
  // -- Workspaces ----------------------------------------------------------

  // Workspace list: list of dicts, each describing one workspace.
  // Default: empty list — AstraWorkspaceService ensures a default workspace
  // exists at runtime, but the persisted list starts empty so we can detect
  // a fresh profile vs. a migrated one.
  registry->RegisterListPref(kPrefWorkspaces);

  // Active workspace ID: identifier of the currently selected workspace.
  // Default: "default" — matches the default workspace created by
  // AstraWorkspaceService::EnsureDefaultWorkspace.
  registry->RegisterStringPref(kPrefActiveWorkspaceId, "default");

  // -- Workspace windows ----------------------------------------------------
  //
  // Window behavior preferences for workspaces.  These control how Astra
  // manages browser windows within workspaces.
  //
  // Chromium owns actual window management (BrowserList, BrowserWindow).
  // These prefs control only Astra-specific window behavior in the context
  // of workspaces.

  // Whether to remember and restore window placement per workspace.
  // Default: true — workspace window recall is a core Arc-style feature.
  registry->RegisterBooleanPref(kPrefWorkspaceWindowRememberPlacement,
                                kDefaultWorkspaceWindowRememberPlacement);

  // Whether new browser windows open in the active workspace.
  // Default: true — new windows belong to the current context.
  registry->RegisterBooleanPref(kPrefWorkspaceWindowNewInActiveWorkspace,
                                kDefaultWorkspaceWindowNewInActiveWorkspace);

  // Whether to auto-tile windows in a workspace.
  // Default: false — windows retain their individual positions by default.
  registry->RegisterBooleanPref(kPrefWorkspaceWindowAutoTile,
                                kDefaultWorkspaceWindowAutoTile);

  // Default number of windows to create for new workspaces.
  // Default: 1 — new workspaces start with one browser window.
  registry->RegisterIntegerPref(kPrefWorkspaceWindowDefaultWindowCount,
                                kDefaultWorkspaceWindowDefaultWindowCount);

  // -- Workspace overview ---------------------------------------------------
  //
  // Presentation settings for the workspace overview UI.
  // These control how workspaces are displayed in the overview.
  //
  // Workspace data is owned by AstraWorkspaceService.
  // These prefs control only the presentation layer.

  // Default view mode (0 = grid, 1 = list).
  // Default: 0 = grid view.
  registry->RegisterIntegerPref(kPrefWorkspaceOverviewViewMode,
                                kDefaultWorkspaceOverviewViewMode);

  // Default card size (0 = small, 1 = medium, 2 = large).
  // Default: 1 = medium.
  registry->RegisterIntegerPref(kPrefWorkspaceOverviewCardSize,
                                kDefaultWorkspaceOverviewCardSize);

  // Whether to show statistics on workspace cards.
  // Default: true — statistics provide useful context.
  registry->RegisterBooleanPref(kPrefWorkspaceOverviewShowStatistics,
                                kDefaultWorkspaceOverviewShowStatistics);

  // Whether to show hibernation indicator on cards.
  // Default: true — hibernation state is important context.
  registry->RegisterBooleanPref(kPrefWorkspaceOverviewShowHibernation,
                                kDefaultWorkspaceOverviewShowHibernation);

  // Whether to show the "New Workspace" quick-add card.
  // Default: true — quick workspace creation is a core feature.
  registry->RegisterBooleanPref(kPrefWorkspaceOverviewShowQuickAdd,
                                kDefaultWorkspaceOverviewShowQuickAdd);

  // Default sort order for workspaces in the overview.
  // Default: "manual" — user-defined order.
  registry->RegisterStringPref(kPrefWorkspaceOverviewSortOrder,
                               kDefaultWorkspaceOverviewSortOrder);

  // Bulk delete confirmation threshold.
  // Default: 1 — always confirm for safety.
  registry->RegisterIntegerPref(kPrefWorkspaceOverviewBulkDeleteThreshold,
                                kDefaultWorkspaceOverviewBulkDeleteThreshold);

  // Whether to include hibernated workspaces in the overview.
  // Default: true — show all workspaces.
  registry->RegisterBooleanPref(kPrefWorkspaceOverviewIncludeHibernated,
                                kDefaultWorkspaceOverviewIncludeHibernated);

  // -- Workspace import/export ---------------------------------------------
  //
  // Default settings for workspace import/export operations.
  // These control default behavior when no explicit options are provided.
  //
  // Import/export operates on workspace metadata and tab data.
  // Actual workspace state is owned by AstraWorkspaceService + Chromium.

  // Whether to include tabs in workspace exports by default.
  registry->RegisterBooleanPref(kPrefWorkspaceExportIncludeTabs,
                                kDefaultWorkspaceExportIncludeTabs);

  // Whether to include workspace settings in exports.
  registry->RegisterBooleanPref(kPrefWorkspaceExportIncludeSettings,
                                kDefaultWorkspaceExportIncludeSettings);

  // Whether to include favorite state in exported tabs.
  registry->RegisterBooleanPref(kPrefWorkspaceExportIncludeFavorites,
                                kDefaultWorkspaceExportIncludeFavorites);

  // Whether to include advanced metadata in exports.
  registry->RegisterBooleanPref(kPrefWorkspaceExportIncludeMetadata,
                                kDefaultWorkspaceExportIncludeMetadata);

  // Default import mode (0 = merge, 1 = replace).
  registry->RegisterIntegerPref(kPrefWorkspaceImportMode,
                                kDefaultWorkspaceImportMode);

  // Default conflict resolution (0 = rename, 1 = skip).
  registry->RegisterIntegerPref(kPrefWorkspaceImportConflictResolution,
                                kDefaultWorkspaceImportConflictResolution);

  // Whether to open tabs for imported workspaces by default.
  registry->RegisterBooleanPref(kPrefWorkspaceImportOpenTabs,
                                kDefaultWorkspaceImportOpenTabs);

  // Whether to apply favorite state from imported tabs.
  registry->RegisterBooleanPref(kPrefWorkspaceImportApplyFavorites,
                                kDefaultWorkspaceImportApplyFavorites);

  // -- Sidebar -------------------------------------------------------------

  registry->RegisterBooleanPref(kPrefSidebarVisible, kDefaultSidebarVisible);
  registry->RegisterIntegerPref(kPrefSidebarWidth, kDefaultSidebarWidth);
  registry->RegisterBooleanPref(kPrefSidebarPinned, kDefaultSidebarPinned);
  registry->RegisterStringPref(kPrefSidebarPosition, kDefaultSidebarPosition);
  registry->RegisterBooleanPref(kPrefSidebarAutoHide, kDefaultSidebarAutoHide);
  registry->RegisterListPref(kPrefSidebarPinnedSections);
  registry->RegisterBooleanPref(kPrefSidebarShowSectionIcons,
                                kDefaultSidebarShowSectionIcons);
  registry->RegisterBooleanPref(kPrefSidebarShowSectionLabels,
                                kDefaultSidebarShowSectionLabels);
  registry->RegisterBooleanPref(kPrefSidebarCompactMode,
                                kDefaultSidebarCompactMode);
  registry->RegisterBooleanPref(kPrefSidebarAutoHideOnTabClick,
                                kDefaultSidebarAutoHideOnTabClick);
  registry->RegisterBooleanPref(kPrefSidebarShowTabCountBadges,
                                kDefaultSidebarShowTabCountBadges);
  registry->RegisterBooleanPref(kPrefSidebarShowWorkspaceBadge,
                                kDefaultSidebarShowWorkspaceBadge);
  registry->RegisterBooleanPref(kPrefSidebarShowTabGroupsSection,
                                kDefaultSidebarShowTabGroupsSection);
  registry->RegisterBooleanPref(kPrefSidebarShowHistorySection,
                                kDefaultSidebarShowHistorySection);
  registry->RegisterBooleanPref(kPrefSidebarShowRecentlyClosedSection,
                                kDefaultSidebarShowRecentlyClosedSection);
  registry->RegisterBooleanPref(kPrefSidebarShowReadingListSection,
                                kDefaultSidebarShowReadingListSection);
  registry->RegisterBooleanPref(kPrefSidebarShowNotesSection,
                                kDefaultSidebarShowNotesSection);
  registry->RegisterBooleanPref(kPrefSidebarShowDownloadsSection,
                                kDefaultSidebarShowDownloadsSection);
  registry->RegisterBooleanPref(kPrefSidebarShowPasswordsSection,
                                kDefaultSidebarShowPasswordsSection);
  registry->RegisterBooleanPref(kPrefSidebarShowExtensionsSection,
                                kDefaultSidebarShowExtensionsSection);
  registry->RegisterBooleanPref(kPrefSidebarAnimationEnabled,
                                kDefaultSidebarAnimationEnabled);
  registry->RegisterBooleanPref(kPrefSidebarRememberLastSection,
                                kDefaultSidebarRememberLastSection);
  registry->RegisterStringPref(kPrefSidebarLastActiveSection,
                               kDefaultSidebarLastActiveSection);
  registry->RegisterStringPref(kPrefSidebarDefaultActiveSection,
                               kDefaultSidebarDefaultActiveSection);
  registry->RegisterListPref(kPrefSidebarSectionOrder);
  registry->RegisterListPref(kPrefSidebarCollapsedSections);

  // -- Split view ----------------------------------------------------------

  registry->RegisterStringPref(kPrefSplitViewDefaultOrientation,
                               kDefaultSplitViewOrientation);
  registry->RegisterDoublePref(kPrefSplitViewDefaultRatio,
                               kDefaultSplitViewRatio);
  registry->RegisterBooleanPref(kPrefSplitViewEnabled, kDefaultSplitViewEnabled);
  registry->RegisterBooleanPref(kPrefSplitViewSnapToPresets,
                                kDefaultSplitViewSnapToPresets);
  registry->RegisterBooleanPref(kPrefSplitViewDividerVisible,
                                kDefaultSplitViewDividerVisible);
  registry->RegisterBooleanPref(kPrefSplitViewRememberRatio,
                                kDefaultSplitViewRememberRatio);
  registry->RegisterBooleanPref(kPrefSplitViewMinimapEnabled,
                                kDefaultSplitViewMinimapEnabled);

  // -- Notes ---------------------------------------------------------------

  // Notes list: list of dicts, each describing one note.
  // Default: empty list — users start with no notes.
  // Notes are persisted per-profile via PrefService.
  //
  // TODO(astra): Consider whether notes should be syncable across devices.
  // If syncable, use RegisterSyncablePref instead of RegisterListPref.
  // Chromium subsystem: sync driver / PrefService sync integration.
  registry->RegisterListPref(kPrefNotes);

  // Default sort order for note lists.
  // Default: kDateDescending (most recent first).
  //
  // This is a presentation preference — it controls the default display
  // order of notes in the sidebar and note panel.
  registry->RegisterIntegerPref(kPrefNoteSortOrder, kDefaultNoteSortOrder);

  // -- Focus mode ----------------------------------------------------------

  // Default focus mode session duration in minutes.
  // Default: 25 minutes (Pomodoro-style focus session).
  registry->RegisterIntegerPref(kPrefFocusModeDefaultDuration, 25);

  // Distraction site blocklist: list of URL pattern strings.
  // Default: empty list — user populates their own distractions.
  registry->RegisterListPref(kPrefFocusModeBlocklist);

  // Whether focus mode should auto-start based on configured conditions.
  // Default: false — user opts in.
  registry->RegisterBooleanPref(kPrefFocusModeAutoStart, false);

  // Short break duration for pomodoro mode (minutes).
  // Default: 5 minutes — standard Pomodoro short break.
  registry->RegisterIntegerPref(kPrefFocusModeShortBreakDuration,
                                kDefaultFocusShortBreakMinutes);

  // Long break duration for pomodoro mode (minutes).
  // Default: 15 minutes — standard Pomodoro long break.
  registry->RegisterIntegerPref(kPrefFocusModeLongBreakDuration,
                                kDefaultFocusLongBreakMinutes);

  // Number of work sessions before a long break.
  // Default: 4 — every 4th work session is followed by a long break.
  registry->RegisterIntegerPref(kPrefFocusModeLongBreakInterval,
                                kDefaultFocusLongBreakInterval);

  // Whether to auto-start the next phase in pomodoro mode.
  // Default: false — user manually starts each phase.
  registry->RegisterBooleanPref(kPrefFocusModeAutoStartNextPhase,
                                kDefaultFocusAutoStartNextPhase);

  // Whitelist of sites always allowed during focus mode.
  // Default: empty list — no sites are whitelisted by default.
  registry->RegisterListPref(kPrefFocusModeWhitelist);

  // Total accumulated focus time in seconds (cumulative stat).
  // Default: 0 — no focus time on a fresh profile.
  registry->RegisterIntegerPref(kPrefFocusModeTotalFocusSeconds,
                                kDefaultFocusTotalFocusSeconds);

  // Total number of completed focus sessions (cumulative stat).
  // Default: 0 — no sessions completed on a fresh profile.
  registry->RegisterIntegerPref(kPrefFocusModeSessionsCompleted,
                                kDefaultFocusSessionsCompleted);

  // Total number of completed pomodoro cycles (cumulative stat).
  // Default: 0 — no cycles completed on a fresh profile.
  registry->RegisterIntegerPref(kPrefFocusModeCyclesCompleted,
                                kDefaultFocusCyclesCompleted);

  // Whether distraction warnings are enabled.
  // Default: true — warnings are on by default.
  registry->RegisterBooleanPref(kPrefFocusModeWarningsEnabled,
                                kDefaultFocusWarningsEnabled);

  // Auto-start start time (HH:MM 24h format).
  // Default: "09:00" — start of typical work day.
  registry->RegisterStringPref(kPrefFocusModeAutoStartTime,
                               kDefaultFocusAutoStartTime);

  // Auto-start end time (HH:MM 24h format).
  // Default: "17:00" — end of typical work day.
  registry->RegisterStringPref(kPrefFocusModeAutoEndTime,
                               kDefaultFocusAutoEndTime);

  // Auto-start days of week (list of ints, 0=Sun ... 6=Sat).
  // Default: [1,2,3,4,5] — Monday through Friday (weekdays).
  {
    base::Value::List default_days;
    default_days.Append(1);  // Monday
    default_days.Append(2);  // Tuesday
    default_days.Append(3);  // Wednesday
    default_days.Append(4);  // Thursday
    default_days.Append(5);  // Friday
    registry->RegisterListPref(kPrefFocusModeAutoStartDays,
                               std::move(default_days));
  }

  // Session presets (list of dicts).
  // Default: empty list — no custom presets on a fresh profile.
  // Users can create named presets for different focus tasks.
  registry->RegisterListPref(kPrefFocusModePresets);

  // Whether the focus session is currently paused.
  // Default: false — sessions start active, not paused.
  registry->RegisterBooleanPref(kPrefFocusModePaused, kDefaultFocusPaused);

  // Remaining seconds in a paused session.
  // Default: 0 — no paused time on a fresh profile.
  registry->RegisterIntegerPref(kPrefFocusModePausedRemainingSeconds,
                                kDefaultFocusPausedRemainingSeconds);

  // -- Focus mode presentation / UI -----------------------------------------
  //
  // These prefs control how the focus mode UI is presented.
  // They are read by AstraFocusModeModel (//astra/ui/views/focus_mode/)
  // and applied by the indicator / menu bubble views.
  //
  // Truth source: AstraFocusModeModel (views layer).

  // Whether the floating focus indicator is shown during focus mode.
  // Default: true — indicator is the primary way users see focus state.
  registry->RegisterBooleanPref(kPrefFocusModeShowIndicator,
                                kDefaultFocusShowIndicator);

  // Position of the focus mode indicator on screen.
  // 0 = top_left, 1 = top_right, 2 = bottom_left, 3 = bottom_right.
  // Default: 1 = top_right.
  registry->RegisterIntegerPref(kPrefFocusModeIndicatorPosition,
                                kDefaultFocusIndicatorPosition);

  // Visual style of the focus mode indicator.
  // 0 = minimal, 1 = full, 2 = badge.
  // Default: 1 = full.
  registry->RegisterIntegerPref(kPrefFocusModeIndicatorStyle,
                                kDefaultFocusIndicatorStyle);

  // Whether to show the timer countdown in the indicator.
  // Default: true — timer is core to the focus mode UX.
  registry->RegisterBooleanPref(kPrefFocusModeShowTimerInIndicator,
                                kDefaultFocusShowTimerInIndicator);

  // Whether to show session statistics in the focus mode menu.
  // Default: true — stats provide motivation and context.
  registry->RegisterBooleanPref(kPrefFocusModeShowSessionStats,
                                kDefaultFocusShowSessionStats);

  // Whether distraction site blocking is active during focus mode.
  // Default: false — user opts in to distraction blocking.
  registry->RegisterBooleanPref(kPrefFocusModeBlockDistractingSites,
                                kDefaultFocusBlockDistractingSites);

  // Whether notification sounds play for focus mode events.
  // Default: true — audio feedback helps users stay aware.
  registry->RegisterBooleanPref(kPrefFocusModeNotificationSound,
                                kDefaultFocusNotificationSound);

  // Whether break reminders are enabled.
  // Default: false — break reminders are opt-in.
  registry->RegisterBooleanPref(kPrefFocusModeBreakReminders,
                                kDefaultFocusBreakReminders);

  // Break reminder interval in minutes.
  // Default: 25 — matches Pomodoro work session length.
  registry->RegisterIntegerPref(kPrefFocusModeBreakIntervalMinutes,
                                kDefaultFocusBreakIntervalMinutes);

  // Suggested break duration in minutes.
  // Default: 5 — matches Pomodoro short break length.
  registry->RegisterIntegerPref(kPrefFocusModeBreakDurationMinutes,
                                kDefaultFocusBreakDurationMinutes);

  // Whether non-focus tabs are dimmed during focus mode.
  // Default: false — dimmed tabs can be confusing.
  registry->RegisterBooleanPref(kPrefFocusModeDimNonFocusTabs,
                                kDefaultFocusDimNonFocusTabs);

  // Whether the sidebar is automatically hidden during focus mode.
  // Default: true — hiding sidebar is a core focus mode feature.
  registry->RegisterBooleanPref(kPrefFocusModeHideSidebar,
                                kDefaultFocusHideSidebar);

  // -- Memory saver ---------------------------------------------------------

  // Whether the memory saver (auto-suspend inactive tabs) is enabled.
  // Default: true — auto-suspend is on by default to save memory.
  registry->RegisterBooleanPref(kPrefMemorySaverEnabled,
                                kDefaultMemorySaverEnabled);

  // Auto-suspend timeout in minutes.
  // Default: 5 minutes of inactivity before a tab is eligible for suspension.
  registry->RegisterIntegerPref(kPrefMemorySaverTimeoutMinutes,
                                kDefaultMemorySaverTimeoutMinutes);

  // Whether tabs in the active workspace can be auto-suspended.
  // Default: false — preserve the user's current workspace tabs.
  registry->RegisterBooleanPref(kPrefMemorySaverSuspendActiveWorkspace,
                                kDefaultMemorySaverSuspendActiveWorkspace);

  // -- Picture-in-Picture (PiP) ---------------------------------------------

  // Default PiP size preset: "small", "medium", or "large".
  // Default: "medium" — standard video PiP size.
  //
  // The actual PiP window is owned by Chromium's PictureInPictureWindowController.
  // This pref controls the default size that Astra applies when a new PiP
  // window is created or when the user resets to default.
  //
  // Chromium owner: PictureInPictureWindowController
  //   (chrome/browser/picture_in_picture/picture_in_picture_window_controller.h)
  registry->RegisterStringPref(kPrefPiPDefaultSize, kDefaultPiPDefaultSize);

  // Whether PiP windows are always-on-top by default.
  // Default: true — PiP windows should stay above other windows.
  registry->RegisterBooleanPref(kPrefPiPAlwaysOnTop, kDefaultPiPAlwaysOnTop);

  // Default snap position for PiP windows.
  registry->RegisterStringPref(kPrefPiPSnapPosition, kDefaultPiPSnapPosition);

  // Whether snap-to-corner is enabled for PiP windows.
  registry->RegisterBooleanPref(kPrefPiPSnapToCornerEnabled,
                                kDefaultPiPSnapToCornerEnabled);

  // PiP window opacity.
  registry->RegisterDoublePref(kPrefPiPOpacity, kDefaultPiPOpacity);

  // Whether auto-PiP on tab switch is enabled.
  registry->RegisterBooleanPref(kPrefPiPAutoPipOnTabSwitch,
                                kDefaultPiPAutoPipOnTabSwitch);

  // Maximum number of concurrent PiP windows.
  registry->RegisterIntegerPref(kPrefPiPMaxWindows, kDefaultPiPMaxWindows);

  // -- PiP controls presentation -------------------------------------------
  //
  // Presentation settings for the Astra PiP controls overlay.
  // These control how the controls look and behave.
  //
  // State is owned by AstraPipControlsModel (//astra/ui/views/pip/).
  // The controls view reads these settings and applies them.
  //
  // Chromium owner: PictureInPictureWindowViews
  //   (chrome/browser/ui/views/picture_in_picture/)
  // Patch point: Controls overlay is added as a child view.

  // Whether controls auto-hide after inactivity.
  registry->RegisterBooleanPref(kPrefPiPControlsAutoHide,
                                kDefaultPiPControlsAutoHide);

  // Auto-hide delay in milliseconds.
  registry->RegisterIntegerPref(kPrefPiPControlsAutoHideDelayMs,
                                kDefaultPiPControlsAutoHideDelayMs);

  // Whether the top bar is shown.
  registry->RegisterBooleanPref(kPrefPiPControlsShowTopBar,
                                kDefaultPiPControlsShowTopBar);

  // Whether the bottom bar is shown.
  registry->RegisterBooleanPref(kPrefPiPControlsShowBottomBar,
                                kDefaultPiPControlsShowBottomBar);

  // Whether the resize handle is shown.
  registry->RegisterBooleanPref(kPrefPiPControlsShowResizeHandle,
                                kDefaultPiPControlsShowResizeHandle);

  // Controls overlay opacity.
  registry->RegisterDoublePref(kPrefPiPControlsOpacity,
                               kDefaultPiPControlsOpacity);

  // Default size preset for PiP controls.
  registry->RegisterStringPref(kPrefPiPControlsDefaultSizePreset,
                               kDefaultPiPControlsDefaultSizePreset);

  // Whether the always-on-top button is shown.
  registry->RegisterBooleanPref(kPrefPiPControlsShowAlwaysOnTopButton,
                                kDefaultPiPControlsShowAlwaysOnTopButton);

  // Whether playback controls are shown.
  registry->RegisterBooleanPref(kPrefPiPControlsShowPlaybackControls,
                                kDefaultPiPControlsShowPlaybackControls);

  // Whether skip buttons are shown.
  registry->RegisterBooleanPref(kPrefPiPControlsShowSkipButtons,
                                kDefaultPiPControlsShowSkipButtons);

  // Skip duration in seconds.
  registry->RegisterIntegerPref(kPrefPiPControlsSkipDurationSeconds,
                                kDefaultPiPControlsSkipDurationSeconds);

  // Playback rate presets.
  {
    base::Value::List default_presets;
    default_presets.Append(0.5);
    default_presets.Append(1.0);
    default_presets.Append(1.25);
    default_presets.Append(1.5);
    default_presets.Append(2.0);
    registry->RegisterListPref(kPrefPiPControlsPlaybackRatePresets,
                               std::move(default_presets));
  }

  // -- Reading list ---------------------------------------------------------

  // Reading list entries: list of dicts, each describing one reading list item.
  // Default: empty list — users start with no reading list items.
  //
  // TODO(astra): In production, reading list data is owned by Chromium's
  // ReadingListModel.  This Astra-level pref is for the overlay skeleton
  // and will be removed once ReadingListModel is fully wired.
  // Chromium owner: ReadingListModel (components/reading_list/core/)
  registry->RegisterListPref(kPrefReadingListEntries);

  // Default sort order for reading list entries.
  // Default: "newest" — most recently added first.
  registry->RegisterStringPref(kPrefReadingListSortOrder,
                               kDefaultReadingListSortOrder);

  // -- Favorite folders ------------------------------------------------------

  // Favorite folder list: list of dicts, each describing one favorite folder.
  // Default: empty list — the "root" folder is implicit.
  // Folder metadata (name, parent, order) persists via PrefService.
  // Per-tab favorite folder membership is stored on AstraTabFeatures
  // (WebContentsUserData) and travels with tabs through session restore.
  //
  // Chromium analog: Bookmarks model (components/bookmarks/browser/).
  registry->RegisterListPref(kPrefFavoriteFolders);

  // -- Tab stacks -----------------------------------------------------------

  // Tab stack list: list of dicts, each describing one named tab stack.
  // Default: empty list — users start with no named stacks.
  // Stack metadata (name, color, order) persists via PrefService.
  // Per-tab stack membership is stored on AstraTabFeatures (WebContentsUserData)
  // and travels with tabs through session restore.
  //
  // Chromium analog: TabGroupModel persisted state.
  // Chromium owner: TabGroupModel (chrome/browser/ui/tabs/tab_group_model.h)
  registry->RegisterListPref(kPrefTabStacks);

  // -- Search engines ------------------------------------------------------

  // Search engine state is fully owned by Chromium's TemplateURLService.
  // Astra only projects the state and adds presentation preferences.
  //
  // Chromium component: TemplateURLService
  //   (components/search_engines/template_url_service.h)
  // Chromium factory: TemplateURLServiceFactory
  //   (chrome/browser/search_engines/template_url_service_factory.h)
  //
  // These Astra-specific prefs control only presentation — they never
  // store search engine data.  See AstraSearchEngineHelper for the
  // projection layer that reads from TemplateURLService.

  registry->RegisterBooleanPref(kPrefSearchShowDefaultEngine,
                                kDefaultSearchShowDefaultEngine);
  registry->RegisterBooleanPref(kPrefSearchShowOtherEngines,
                                kDefaultSearchShowOtherEngines);
  registry->RegisterBooleanPref(kPrefSearchSuggestionsEnabled,
                                kDefaultSearchSuggestionsEnabled);

  // Recent search queries list (most recent first).
  // Default: empty list — no recent queries on a fresh profile.
  registry->RegisterListPref(kPrefSearchRecentQueries);

  // Maximum number of recent search queries to remember.
  // Default: 10.
  registry->RegisterIntegerPref(kPrefSearchMaxRecentQueries,
                                kDefaultSearchMaxRecentQueries);

  // -- Accessibility -------------------------------------------------------

  // High contrast mode for Astra UI.
  // Default: false — follows system setting if enabled.
  //
  // TODO(astra): Consider syncing with Chromium's high contrast setting
  //   instead of having a separate Astra pref.  The Chromium pref is
  //   accessibility.high_contrast.enabled.
  // Chromium owner: AccessibilityManager
  //   (chrome/browser/accessibility/accessibility_manager.h)
  registry->RegisterBooleanPref(kPrefHighContrastMode,
                                kDefaultHighContrastMode);

  // Reduced motion mode for Astra UI.
  // Default: false — follows system setting if enabled.
  //
  // Chromium owner: NativeTheme (ui/native_theme/native_theme.h)
  //   prefers_reduced_transitions()
  registry->RegisterBooleanPref(kPrefReducedMotion, kDefaultReducedMotion);

  // Accessibility font scale factor.
  // Default: 1.0 — no scaling.
  //
  // TODO(astra): Consider integrating with Chromium's default font size
  //   setting or the accessibility large cursor / font settings.
  // Chromium owner: WebContents::SetZoomLevel for page zoom, or
  //   ui/views style defaults for UI text scaling.
  registry->RegisterDoublePref(kPrefAccessibilityFontScale,
                               kDefaultAccessibilityFontScale);

  // Caret browsing.
  registry->RegisterBooleanPref(kPrefCaretBrowsingEnabled,
                                kDefaultCaretBrowsingEnabled);

  // Sticky keys.
  registry->RegisterBooleanPref(kPrefStickyKeysEnabled,
                                kDefaultStickyKeysEnabled);

  // Slow keys.
  registry->RegisterBooleanPref(kPrefSlowKeysEnabled,
                                kDefaultSlowKeysEnabled);
  registry->RegisterIntegerPref(kPrefSlowKeysDelayMs,
                                kDefaultSlowKeysDelayMs);

  // Mouse keys.
  registry->RegisterBooleanPref(kPrefMouseKeysEnabled,
                                kDefaultMouseKeysEnabled);

  // Large cursor.
  registry->RegisterBooleanPref(kPrefLargeCursorEnabled,
                                kDefaultLargeCursorEnabled);
  registry->RegisterIntegerPref(kPrefLargeCursorSize,
                                kDefaultLargeCursorSize);

  // Magnifier.
  registry->RegisterBooleanPref(kPrefMagnifierEnabled,
                                kDefaultMagnifierEnabled);
  registry->RegisterDoublePref(kPrefMagnifierScale,
                               kDefaultMagnifierScale);
  registry->RegisterIntegerPref(kPrefMagnifierType,
                                kDefaultMagnifierType);

  // Select-to-speak.
  registry->RegisterBooleanPref(kPrefSelectToSpeakEnabled,
                                kDefaultSelectToSpeakEnabled);

  // Dictation.
  registry->RegisterBooleanPref(kPrefDictationEnabled,
                                kDefaultDictationEnabled);

  // Virtual keyboard.
  registry->RegisterBooleanPref(kPrefVirtualKeyboardEnabled,
                                kDefaultVirtualKeyboardEnabled);

  // Text helpers.
  registry->RegisterIntegerPref(kPrefMinimumFontSize,
                                kDefaultMinimumFontSize);
  registry->RegisterIntegerPref(kPrefFontWeightAdjustment,
                                kDefaultFontWeightAdjustment);
  registry->RegisterDoublePref(kPrefLetterSpacing,
                               kDefaultLetterSpacing);
  registry->RegisterDoublePref(kPrefLineHeight,
                               kDefaultLineHeight);

  // Contrast and color.
  registry->RegisterIntegerPref(kPrefContrastLevel,
                                kDefaultContrastLevel);
  registry->RegisterBooleanPref(kPrefNightLightEnabled,
                                kDefaultNightLightEnabled);
  registry->RegisterIntegerPref(kPrefColorTemperature,
                                kDefaultColorTemperature);
  registry->RegisterBooleanPref(kPrefColorInversionEnabled,
                                kDefaultColorInversionEnabled);

  // Animation and motion.
  registry->RegisterIntegerPref(kPrefAnimationReductionLevel,
                                kDefaultAnimationReductionLevel);
  registry->RegisterBooleanPref(kPrefAutoScrollEnabled,
                                kDefaultAutoScrollEnabled);
  registry->RegisterDoublePref(kPrefScrollSpeed,
                               kDefaultScrollSpeed);

  // -- Theme ---------------------------------------------------------------

  // Accent color for Astra UI.
  // Default: empty string — follows system accent color or Chromium theme.
  //
  // Chromium owner: ThemeService (chrome/browser/themes/theme_service.h)
  registry->RegisterStringPref(kPrefThemeAccentColor, kDefaultThemeAccentColor);

  // Theme mode: light, dark, or system.
  // Default: "system" — follows the OS theme setting.
  //
  // Chromium owner: ThemeService / NativeTheme
  registry->RegisterStringPref(kPrefThemeMode, kDefaultThemeMode);

  // Active named color scheme.
  // Default: empty — no named scheme active.
  registry->RegisterStringPref(kPrefThemeScheme, kDefaultThemeScheme);

  // Whether to use workspace accent for theming.
  registry->RegisterBooleanPref(kPrefThemeUseWorkspaceAccent,
                                kDefaultThemeUseWorkspaceAccent);

  // Custom accent color (ARGB integer).
  registry->RegisterIntegerPref(kPrefThemeCustomAccentColor,
                                kDefaultThemeCustomAccentColor);

  // High contrast mode for Astra UI.
  registry->RegisterBooleanPref(kPrefThemeHighContrast,
                                kDefaultThemeHighContrast);

  // Show accent on tab strips.
  registry->RegisterBooleanPref(kPrefThemeShowAccentOnTabs,
                                kDefaultThemeShowAccentOnTabs);

  // Show accent on sidebar.
  registry->RegisterBooleanPref(kPrefThemeShowAccentOnSidebar,
                                kDefaultThemeShowAccentOnSidebar);

  // Accent color intensity.
  registry->RegisterDoublePref(kPrefThemeAccentIntensity,
                               kDefaultThemeAccentIntensity);

  // Auto theme scheduling.
  registry->RegisterBooleanPref(kPrefThemeUseAutoSchedule,
                                kDefaultThemeUseAutoSchedule);

  // Auto theme light start time.
  registry->RegisterStringPref(kPrefThemeAutoLightStart,
                               kDefaultThemeAutoLightStart);

  // Auto theme dark start time.
  registry->RegisterStringPref(kPrefThemeAutoDarkStart,
                               kDefaultThemeAutoDarkStart);

  // -- Command palette -----------------------------------------------------

  // Number of recent commands to remember.
  // Default: 10 — shows the last 10 executed commands at the top.
  registry->RegisterIntegerPref(kPrefCommandPaletteRecentCount,
                                kDefaultCommandPaletteRecentCount);

  // Whether to show workspace commands in the command palette.
  // Default: true — workspace switch/create/delete commands are shown.
  registry->RegisterBooleanPref(kPrefCommandPaletteShowWorkspaces,
                                kDefaultCommandPaletteShowWorkspaces);

  // Maximum number of visible command results.
  // Default: 20 — matches kMaxResults constant.
  registry->RegisterIntegerPref(kPrefCommandPaletteMaxVisible,
                                kDefaultCommandPaletteMaxVisible);

  // Whether to show command descriptions in results.
  // Default: true — descriptions help users understand what commands do.
  registry->RegisterBooleanPref(kPrefCommandPaletteShowDescriptions,
                                kDefaultCommandPaletteShowDescriptions);

  // Whether to show keyboard shortcut hints in results.
  // Default: true — shortcuts help users learn keybindings.
  registry->RegisterBooleanPref(kPrefCommandPaletteShowShortcuts,
                                kDefaultCommandPaletteShowShortcuts);

  // Whether to show the "recent commands" section on empty query.
  // Default: true — recent commands are a quick way to re-run actions.
  registry->RegisterBooleanPref(kPrefCommandPaletteShowRecentSection,
                                kDefaultCommandPaletteShowRecentSection);

  // -- Command delegate ----------------------------------------------------

  // Recent command IDs list (most recent first).
  // Default: empty list — no recent commands on a fresh profile.
  registry->RegisterListPref(kPrefCommandRecentList);

  // Maximum number of recent commands to remember.
  // Default: 20 — keeps the last 20 executed commands.
  registry->RegisterIntegerPref(kPrefCommandRecentMax,
                                kDefaultCommandRecentMax);

  // Command aliases dictionary (alias -> command_id).
  // Default: empty dictionary — no custom aliases on a fresh profile.
  registry->RegisterDictionaryPref(kPrefCommandAliases);

  // -- Password helper -------------------------------------------------------
  //
  // Password state is fully owned by Chromium's Password Manager.
  // Astra only projects the state and adds presentation preferences.
  //
  // Chromium component: Password Manager (components/password_manager/)
  // Chromium factory: PasswordStoreFactory
  //   (chrome/browser/password_manager/password_store_factory.h)
  //
  // These Astra-specific prefs control only presentation — they never
  // store password data.  See AstraPasswordHelper for the projection layer
  // that reads from PasswordStore.

  // Whether password suggestions are shown in Astra UI surfaces.
  // Default: true — suggestions are shown by default.
  registry->RegisterBooleanPref(kPrefPasswordShowSuggestions,
                                kDefaultPasswordShowSuggestions);

  // Whether auto-fill is shown as enabled in Astra UI.
  // Default: true — auto-fill is on by default (matching Chromium behavior).
  registry->RegisterBooleanPref(kPrefPasswordAutoFillEnabled,
                                kDefaultPasswordAutoFillEnabled);

  // Whether the password manager shortcut appears in Astra UI.
  // Default: true — the shortcut is shown by default.
  registry->RegisterBooleanPref(kPrefPasswordManagerShortcut,
                                kDefaultPasswordManagerShortcut);

  // Whether the password health summary is shown in the sidebar.
  // Default: true — health summary is shown by default.
  registry->RegisterBooleanPref(kPrefPasswordShowHealth,
                                kDefaultPasswordShowHealth);

  // Whether breach alerts are shown in Astra UI.
  // Default: true — alerts are shown by default.
  registry->RegisterBooleanPref(kPrefPasswordBreachAlertsEnabled,
                                kDefaultPasswordBreachAlertsEnabled);

  // Maximum number of passwords to show in the sidebar section.
  // Default: 20.
  registry->RegisterIntegerPref(kPrefPasswordMaxSidebarPasswords,
                                kDefaultPasswordMaxSidebarPasswords);

  // Whether a breach has been detected and not yet acknowledged.
  // Default: false — no unacknowledged breach on a fresh profile.
  registry->RegisterBooleanPref(kPrefPasswordBreachDetected,
                                kDefaultPasswordBreachDetected);

  // -- Safety helper -------------------------------------------------------
  //
  // Safe browsing state and security decisions are fully owned by Chromium's
  // SafeBrowsingService.  Astra only projects the state and adds
  // presentation preferences.
  //
  // Chromium component: SafeBrowsingService
  //   (components/safe_browsing/core/browser/safe_browsing_service.h)
  // Chromium owner: SecurityStateModel
  //   (components/security_state/core/security_state.h)
  //
  // These Astra-specific prefs control only presentation — they never
  // store safety data or change Chromium's safe browsing behavior.
  // See AstraSafetyHelper for the projection layer.

  // Whether safe browsing is enabled in Astra UI.
  // Default: true — safe browsing is on by default.
  registry->RegisterBooleanPref(kPrefSafeBrowsingEnabled,
                                kDefaultSafeBrowsingEnabled);

  // Safe browsing protection level.
  // 0 = off, 1 = standard, 2 = enhanced.
  // Default: 1 = standard protection.
  registry->RegisterIntegerPref(kPrefSafeBrowsingLevel,
                                kDefaultSafeBrowsingLevel);

  // Whether enhanced protection is enabled.
  // Default: false — standard protection by default.
  registry->RegisterBooleanPref(kPrefEnhancedProtection,
                                kDefaultEnhancedProtection);

  // Whether password reuse warnings are shown.
  // Default: true — on by default for security.
  registry->RegisterBooleanPref(kPrefWarnOnPasswordReuse,
                                kDefaultWarnOnPasswordReuse);

  // Password protection level.
  // 0 = off, 1 = standard, 2 = enhanced.
  // Default: 1 = standard.
  registry->RegisterIntegerPref(kPrefPasswordProtectionLevel,
                                kDefaultPasswordProtectionLevel);

  // Whether the security button is shown in the toolbar.
  // Default: true.
  registry->RegisterBooleanPref(kPrefShowSecurityButton,
                                kDefaultShowSecurityButton);

  // Whether threat notifications are shown.
  // Default: true.
  registry->RegisterBooleanPref(kPrefShowThreatNotifications,
                                kDefaultShowThreatNotifications);

  // Whether dangerous downloads are blocked.
  // Default: true.
  registry->RegisterBooleanPref(kPrefBlockDangerousDownloads,
                                kDefaultBlockDangerousDownloads);

  // Whether to warn on dangerous downloads.
  // Default: true.
  registry->RegisterBooleanPref(kPrefWarnOnDangerousDownloads,
                                kDefaultWarnOnDangerousDownloads);

  // Whether to auto-report safety issues.
  // Default: false — opt-in for privacy.
  registry->RegisterBooleanPref(kPrefAutoReportSafetyIssues,
                                kDefaultAutoReportSafetyIssues);

  // Whether to show mixed content warnings.
  // Default: true.
  registry->RegisterBooleanPref(kPrefMixedContentWarning,
                                kDefaultMixedContentWarning);

  // Whether the site info button is shown.
  // Default: true.
  registry->RegisterBooleanPref(kPrefShowSiteInfoButton,
                                kDefaultShowSiteInfoButton);

  // Whether safety check reminders are shown.
  // Default: true.
  registry->RegisterBooleanPref(kPrefSafetyCheckReminders,
                                kDefaultSafetyCheckReminders);

  // Whether third-party cookie protection is enabled.
  // Default: true.
  registry->RegisterBooleanPref(kPrefCookieProtection,
                                kDefaultCookieProtection);

  // -- Extension helper ----------------------------------------------------
  //
  // Extension state is fully owned by Chromium's extensions system.
  // Astra only projects the state and adds presentation preferences.
  //
  // Chromium component: ExtensionRegistry (extensions/browser/extension_registry.h)
  // Chromium factory: ExtensionService (chrome/browser/extensions/extension_service.h)
  //
  // These Astra-specific prefs control only presentation — they never
  // store extension data or modify extension state.
  // See AstraExtensionHelper for the projection layer.

  // Whether the extension toolbar is shown in Astra UI.
  // Default: true — the toolbar is shown by default.
  registry->RegisterBooleanPref(kPrefExtensionShowToolbar,
                                kDefaultExtensionShowToolbar);

  // Whether extensions are shown in the Astra sidebar section.
  // Default: true — extensions are shown in the sidebar by default.
  registry->RegisterBooleanPref(kPrefExtensionShowInSidebar,
                                kDefaultExtensionShowInSidebar);

  // Whether the extension manager shortcut appears in Astra UI.
  // Default: true — the shortcut is shown by default.
  registry->RegisterBooleanPref(kPrefExtensionManagerShortcut,
                                kDefaultExtensionManagerShortcut);

  // Whether recommended extensions are shown in the sidebar.
  // Default: true — recommendations are shown by default.
  registry->RegisterBooleanPref(kPrefExtensionShowRecommended,
                                kDefaultExtensionShowRecommended);

  // Maximum number of extensions to show in the sidebar section.
  // Default: 10.
  registry->RegisterIntegerPref(kPrefExtensionMaxSidebarCount,
                                kDefaultExtensionMaxSidebarCount);

  // Default sort order for extension lists.
  // Default: "name" — sort alphabetically by extension name.
  registry->RegisterStringPref(kPrefExtensionSortOrder,
                               kDefaultExtensionSortOrder);

  // -- Downloads helper ---------------------------------------------------
  //
  // Download state is fully owned by Chromium's DownloadManager.
  // Astra only projects the state and adds presentation preferences.
  //
  // Chromium component: DownloadManager (content/public/browser/)
  // Chromium type: download::DownloadItem
  //   (components/download/public/common/download_item.h)
  //
  // These Astra-specific prefs control only presentation — they never
  // store download data.  See AstraDownloadsHelper for the projection layer.

  // Whether downloads are shown in the sidebar.
  // Default: true — downloads section is shown by default.
  registry->RegisterBooleanPref(kPrefDownloadsShowInSidebar,
                                kDefaultDownloadsShowInSidebar);

  // Whether download notifications are shown.
  // Default: true — notifications are shown by default.
  registry->RegisterBooleanPref(kPrefDownloadsShowNotifications,
                                kDefaultDownloadsShowNotifications);

  // Whether downloads auto-open when complete.
  // Default: false — auto-open is off by default for safety.
  registry->RegisterBooleanPref(kPrefDownloadsAutoOpen,
                                kDefaultDownloadsAutoOpen);

  // Download sort order.
  // Values: "newest_first" or "oldest_first".
  // Default: "newest_first" — most recent downloads first.
  registry->RegisterStringPref(kPrefDownloadsSortOrder,
                               kDefaultDownloadsSortOrder);

  // Maximum number of recent downloads to show in the sidebar.
  // Default: 20.
  registry->RegisterIntegerPref(kPrefDownloadsMaxRecent,
                                kDefaultDownloadsMaxRecent);

  // Whether download speed is shown in the UI.
  // Default: true — speed is shown by default.
  registry->RegisterBooleanPref(kPrefDownloadsShowSpeed,
                                kDefaultDownloadsShowSpeed);

  // Whether file size is shown in the UI.
  // Default: true — file size is shown by default.
  registry->RegisterBooleanPref(kPrefDownloadsShowFileSize,
                                kDefaultDownloadsShowFileSize);

  // Whether download progress bar is shown.
  // Default: true — progress bar is shown by default.
  registry->RegisterBooleanPref(kPrefDownloadsShowProgress,
                                kDefaultDownloadsShowProgress);

  // Downloads display mode.
  // Values: "list" or "compact".
  // Default: "list" — full list view.
  registry->RegisterStringPref(kPrefDownloadsDisplayMode,
                               kDefaultDownloadsDisplayMode);

  // Whether to prompt the user for download location.
  // Default: true — ask where to save by default.
  registry->RegisterBooleanPref(kPrefDownloadsPromptForLocation,
                                kDefaultDownloadsPromptForLocation);

  // Whether to show safe browsing warnings for dangerous downloads.
  // Default: true — warnings are shown by default for safety.
  registry->RegisterBooleanPref(kPrefDownloadsSafeBrowsingWarnings,
                                kDefaultDownloadsSafeBrowsingWarnings);

  // -- Incognito -----------------------------------------------------------

  // Whether the sidebar shows an incognito badge/indicator.
  // Default: true — the incognito indicator is shown by default as a
  // visual reminder that the user is in incognito mode.
  //
  // Chromium owns the actual incognito visual treatment (title bar color,
  // incognito icon).  This pref controls only Astra-specific UI elements.
  registry->RegisterBooleanPref(kPrefIncognitoShowSidebarBadge,
                                kDefaultIncognitoShowSidebarBadge);

  // Whether to confirm before closing all incognito windows.
  // Default: false — no confirmation by default, matching Chromium's
  // behavior of closing incognito windows immediately.
  registry->RegisterBooleanPref(kPrefIncognitoConfirmCloseAll,
                                kDefaultIncognitoConfirmCloseAll);

  // Whether to warn when external links open in incognito.
  // Default: false — no warning by default.
  registry->RegisterBooleanPref(kPrefIncognitoWarnOnExternalOpen,
                                kDefaultIncognitoWarnOnExternalOpen);

  // Default workspace ID for new incognito windows.
  // Default: "default" — incognito windows start on the default workspace.
  registry->RegisterStringPref(kPrefIncognitoDefaultWorkspace,
                               kDefaultIncognitoDefaultWorkspace);

  // -- Session restore -----------------------------------------------------

  // Session restore mode: "all", "last", or "none".
  // Default: "all" — restore all windows and tabs from the previous session.
  //
  // The actual session restore is performed by Chromium's SessionService.
  // This pref controls Astra's UI and metadata participation.
  //
  // Chromium owner: SessionService / SessionRestore
  //   (chrome/browser/sessions/session_service.h)
  // TODO(astra): Wire this to Chromium's session startup preferences.
  //   Patch point: chrome/browser/prefs/session_startup_pref.cc.
  registry->RegisterStringPref(kPrefSessionRestoreMode,
                               kDefaultSessionRestoreMode);

  // Whether session restore is enabled for Astra metadata.
  // Default: true — Astra metadata rides along with session restore.
  registry->RegisterBooleanPref(kPrefSessionRestoreEnabled,
                                kDefaultSessionRestoreEnabled);

  // Timestamp of the last session save.
  // Default: 0 — no session has been saved yet on this profile.
  registry->RegisterDoublePref(kPrefSessionRestoreLastSaveTime,
                               kDefaultSessionRestoreLastSaveTime);

  // Number of tabs in the last saved session.
  // Default: 0 — no tabs saved yet.
  registry->RegisterIntegerPref(kPrefSessionRestoreLastTabCount,
                                kDefaultSessionRestoreLastTabCount);

  // Number of windows in the last saved session.
  // Default: 0 — no windows saved yet.
  registry->RegisterIntegerPref(kPrefSessionRestoreLastWindowCount,
                                kDefaultSessionRestoreLastWindowCount);

  // Number of workspaces in the last saved session.
  // Default: 0 — no workspaces tracked yet.
  registry->RegisterIntegerPref(kPrefSessionRestoreLastWorkspaceCount,
                                kDefaultSessionRestoreLastWorkspaceCount);

  // Whether lazy loading is enabled for session restore.
  // Default: true — tabs are loaded on demand (lazy restore).
  registry->RegisterBooleanPref(kPrefSessionRestoreLazyLoading,
                                kDefaultSessionRestoreLazyLoading);

  // Whether to show the restore session prompt on startup.
  // Default: false — restore happens automatically without prompting.
  registry->RegisterBooleanPref(kPrefSessionRestoreShowPrompt,
                                kDefaultSessionRestoreShowPrompt);

  // Restore load mode.
  registry->RegisterStringPref(kPrefSessionRestoreLoadMode,
                               kDefaultSessionRestoreLoadMode);

  // Whether to restore on startup.
  registry->RegisterBooleanPref(kPrefSessionRestoreOnStartup,
                                kDefaultSessionRestoreOnStartup);

  // Max tabs per workspace restore.
  registry->RegisterIntegerPref(kPrefSessionRestoreMaxTabsPerWorkspace,
                                kDefaultSessionRestoreMaxTabsPerWorkspace);

  // Whether to restore workspaces individually.
  registry->RegisterBooleanPref(kPrefSessionRestoreWorkspacesIndividually,
                                kDefaultSessionRestoreWorkspacesIndividually);

  // Auto-save interval.
  registry->RegisterIntegerPref(kPrefSessionRestoreAutoSaveInterval,
                                kDefaultSessionRestoreAutoSaveInterval);

  // Max saved sessions.
  registry->RegisterIntegerPref(kPrefSessionRestoreMaxSavedSessions,
                                kDefaultSessionRestoreMaxSavedSessions);

  // Saved sessions dictionary.
  registry->RegisterDictPref(kPrefSessionRestoreSavedSessions,
                             base::Value::Dict());

  // -- Window features -----------------------------------------------------
  //
  // Default window behavior preferences.  Per-window runtime state does NOT
  // persist via PrefService — it travels with windows through session restore.
  // These prefs control default behaviors and global window settings.
  //
  // Chromium owner: Browser / BrowserList for actual window management.
  //   Astra only adds presentation preferences and convenience defaults.

  // Whether new browser windows open maximized.
  // Default: false — new windows open at default size.
  registry->RegisterBooleanPref(kPrefWindowDefaultMaximized,
                                kDefaultWindowDefaultMaximized);

  // Whether new windows open in the active workspace.
  // Default: true — new windows inherit the current workspace.
  registry->RegisterBooleanPref(kPrefWindowNewWindowsInActiveWorkspace,
                                kDefaultWindowNewWindowsInActiveWorkspace);

  // Whether window size and position are remembered across restarts.
  // Default: true — windows restore to previous positions.
  registry->RegisterBooleanPref(kPrefWindowRememberSizeAndPosition,
                                kDefaultWindowRememberSizeAndPosition);

  // Default window width in pixels.
  // Default: 1280.
  registry->RegisterIntegerPref(kPrefWindowDefaultWidth,
                                kDefaultWindowDefaultWidth);

  // Default window height in pixels.
  // Default: 800.
  registry->RegisterIntegerPref(kPrefWindowDefaultHeight,
                                kDefaultWindowDefaultHeight);

  // -- Omnibox ------------------------------------------------------------
  //
  // The omnibox / autocomplete system is fully owned by Chromium.
  // Astra only adds custom actions and suggestion providers.
  //
  // Chromium subsystems reused:
  //   - AutocompleteController (components/omnibox/browser/)
  //   - AutocompleteProvider (components/omnibox/browser/)
  //   - OmniboxEditModel (chrome/browser/ui/omnibox/)
  //
  // These Astra-specific prefs control only presentation and Astra-specific
  // behavior — they never affect the underlying Chromium omnibox engine.

  // Whether Astra suggestions are shown in the omnibox dropdown.
  registry->RegisterBooleanPref(kPrefOmniboxShowAstraSuggestions,
                                kDefaultOmniboxShowAstraSuggestions);

  // Maximum number of Astra suggestions to show.
  registry->RegisterIntegerPref(kPrefOmniboxMaxAstraSuggestions,
                                kDefaultOmniboxMaxAstraSuggestions);

  // Position of Astra suggestions relative to native ones.
  registry->RegisterStringPref(kPrefOmniboxSuggestionPosition,
                               kDefaultOmniboxSuggestionPosition);

  // Whether the Astra omnibox provider is enabled.
  registry->RegisterBooleanPref(kPrefOmniboxProviderEnabled,
                                kDefaultOmniboxProviderEnabled);

  // Dictionary of category enablement states.
  {
    base::Value::Dict default_categories;
    default_categories.Set("workspace", true);
    default_categories.Set("tab", true);
    default_categories.Set("navigation", true);
    default_categories.Set("tool", true);
    registry->RegisterDictPref(kPrefOmniboxCategoryEnabled,
                               std::move(default_categories));
  }

  // List of recent omnibox actions.
  registry->RegisterListPref(kPrefOmniboxRecentActions);

  // Maximum number of recent actions to remember.
  registry->RegisterIntegerPref(kPrefOmniboxMaxRecentActions,
                                kDefaultOmniboxMaxRecentActions);

  // -- Omnibox decoration -------------------------------------------------
  //
  // Presentation settings for the Astra location bar / omnibox decoration.
  // The decoration adds Astra-specific action buttons to the Chrome omnibox.
  //
  // Chromium owner: LocationBarView
  //   (chrome/browser/ui/views/location_bar/location_bar_view.h)
  //
  // These prefs control only the Astra decoration overlay — never the
  // underlying omnibox engine.

  // Whether the Astra decoration is shown in the omnibox.
  registry->RegisterBooleanPref(kPrefOmniboxDecorationShowDecoration,
                              kDefaultOmniboxDecorationShowDecoration);

  // Position of the decoration (left or right).
  registry->RegisterStringPref(kPrefOmniboxDecorationPosition,
                                kDefaultOmniboxDecorationPosition);

  // Maximum number of visible action buttons.
  registry->RegisterIntegerPref(kPrefOmniboxDecorationMaxVisibleActions,
                                 kDefaultOmniboxDecorationMaxVisibleActions);

  // Whether to show text labels on buttons.
  registry->RegisterBooleanPref(kPrefOmniboxDecorationShowLabels,
                                kDefaultOmniboxDecorationShowLabels);

  // Icon size for action buttons.
  registry->RegisterIntegerPref(kPrefOmniboxDecorationIconSize,
                                 kDefaultOmniboxDecorationIconSize);

  // Button style for action buttons.
  registry->RegisterIntegerPref(kPrefOmniboxDecorationButtonStyle,
                                 kDefaultOmniboxDecorationButtonStyle);

  // Whether decoration only shows when the omnibox is focused.
  registry->RegisterBooleanPref(kPrefOmniboxDecorationShowOnFocusOnly,
                                 kDefaultOmniboxDecorationShowOnFocusOnly);

  // Individual action button visibility toggles.
  registry->RegisterBooleanPref(kPrefOmniboxDecorationShowWorkspace,
                                 kDefaultOmniboxDecorationShowWorkspace);
  registry->RegisterBooleanPref(kPrefOmniboxDecorationShowFocusMode,
                                 kDefaultOmniboxDecorationShowFocusMode);
  registry->RegisterBooleanPref(kPrefOmniboxDecorationShowScreenshot,
                                 kDefaultOmniboxDecorationShowScreenshot);
  registry->RegisterBooleanPref(kPrefOmniboxDecorationShowNote,
                                 kDefaultOmniboxDecorationShowNote);
  registry->RegisterBooleanPref(kPrefOmniboxDecorationShowSplitView,
                                 kDefaultOmniboxDecorationShowSplitView);
  registry->RegisterBooleanPref(kPrefOmniboxDecorationShowReadingList,
                                 kDefaultOmniboxDecorationShowReadingList);
  registry->RegisterBooleanPref(kPrefOmniboxDecorationShowTranslate,
                                 kDefaultOmniboxDecorationShowTranslate);
  registry->RegisterBooleanPref(kPrefOmniboxDecorationShowShare,
                                 kDefaultOmniboxDecorationShowShare);

  // Whether to show an overflow menu for hidden actions.
  registry->RegisterBooleanPref(kPrefOmniboxDecorationOverflowMenu,
                                 kDefaultOmniboxDecorationOverflowMenu);

  // Whether the decoration expands on hover.
  registry->RegisterBooleanPref(kPrefOmniboxDecorationHoverExpansion,
                                 kDefaultOmniboxDecorationHoverExpansion);

  // -- DevTools ------------------------------------------------------------
  //
  // DevTools state and engine is fully owned by Chromium's DevTools subsystem.
  // Astra only adds presentation preferences and custom panel registration.
  //
  // Chromium subsystems reused:
  //   - DevToolsWindow (chrome/browser/devtools/devtools_window.h)
  //   - DevToolsManager (content/browser/devtools/devtools_manager.h)
  //   - chrome.devtools.panels extension API
  //
  // These Astra-specific prefs control only presentation — they never
  // affect the underlying DevTools engine or debugging state.
  // See AstraDevToolsHelper for the projection layer.

  // Default dock state for new DevTools windows.
  // Default: "bottom" — matches Chromium's default dock side.
  registry->RegisterStringPref(kPrefDevToolsDefaultDockState,
                               kDefaultDevToolsDefaultDockState);

  // Default panel to show when DevTools opens.
  // Default: empty string — uses Chromium's default (Elements panel).
  registry->RegisterStringPref(kPrefDevToolsDefaultPanel,
                               kDefaultDevToolsDefaultPanel);

  // Whether DevTools auto-opens for new tabs.
  // Default: false — DevTools must be manually opened.
  registry->RegisterBooleanPref(kPrefDevToolsAutoOpen,
                                kDefaultDevToolsAutoOpen);

  // Whether the Astra DevTools panel is visible.
  // Default: true — the Astra panel tab is shown.
  registry->RegisterBooleanPref(kPrefDevToolsAstraPanelVisible,
                                kDefaultDevToolsAstraPanelVisible);

  // Which side of DevTools the Astra panel sidebar appears on.
  // Default: "right".
  registry->RegisterStringPref(kPrefDevToolsPanelSide,
                               kDefaultDevToolsPanelSide);

  // Whether to show icons on panel tabs.
  // Default: true.
  registry->RegisterBooleanPref(kPrefDevToolsShowPanelIcons,
                                kDefaultDevToolsShowPanelIcons);

  // Whether to show text labels on panel tabs.
  // Default: true.
  registry->RegisterBooleanPref(kPrefDevToolsShowPanelLabels,
                                kDefaultDevToolsShowPanelLabels);

  // Width of the panel sidebar in pixels.
  // Default: 240.
  registry->RegisterIntegerPref(kPrefDevToolsPanelWidth,
                                kDefaultDevToolsPanelWidth);

  // Whether experimental Astra DevTools features are enabled.
  // Default: false — experiments are opt-in.
  registry->RegisterBooleanPref(kPrefDevToolsExperimentsEnabled,
                                kDefaultDevToolsExperimentsEnabled);

  // Whether the workspace panel auto-expands on DevTools open.
  // Default: true.
  registry->RegisterBooleanPref(kPrefDevToolsAutoExpandWorkspacePanel,
                                kDefaultDevToolsAutoExpandWorkspacePanel);

  // Whether the panel toolbar is shown.
  // Default: true.
  registry->RegisterBooleanPref(kPrefDevToolsShowPanelToolbar,
                                kDefaultDevToolsShowPanelToolbar);

  // Whether compact panel mode is enabled.
  // Default: false.
  registry->RegisterBooleanPref(kPrefDevToolsCompactMode,
                                kDefaultDevToolsCompactMode);

  // Whether to remember the last active panel.
  // Default: true.
  registry->RegisterBooleanPref(kPrefDevToolsRememberLastPanel,
                                kDefaultDevToolsRememberLastPanel);

  // Last active panel ID.
  // Default: empty string.
  registry->RegisterStringPref(kPrefDevToolsLastActivePanel,
                               kDefaultDevToolsLastActivePanel);

  // Panel list (persisted panel state).
  // Default: empty list — uses built-in defaults.
  registry->RegisterListPref(kPrefDevToolsPanelList);

  // DevTools theme.
  // Default: "system".
  registry->RegisterStringPref(kPrefDevToolsTheme,
                               kDefaultDevToolsTheme);

  // -- Recent tabs ---------------------------------------------------------
  //
  // Recent tab data is fully owned by Chromium's TabRestoreService and
  // SessionService.  Astra only projects the state and adds presentation
  // preferences.
  //
  // Chromium component: TabRestoreService
  //   (chrome/browser/sessions/tab_restore_service.h)
  // Chromium component: SessionService
  //   (chrome/browser/sessions/session_service.h)
  //
  // These Astra-specific prefs control only presentation — they never
  // store recent tab data.  See AstraRecentTabsHelper for the
  // projection layer.

  // Maximum number of recently closed tabs to show.
  // Default: 10.
  registry->RegisterIntegerPref(kPrefRecentTabsMaxCount,
                                kDefaultRecentTabsMaxCount);

  // Whether the recently closed section appears in the sidebar.
  // Default: true.
  registry->RegisterBooleanPref(kPrefRecentTabsShowInSidebar,
                                kDefaultRecentTabsShowInSidebar);

  // Whether timestamps are shown next to recently closed tabs.
  // Default: true.
  registry->RegisterBooleanPref(kPrefRecentTabsShowTimestamps,
                                kDefaultRecentTabsShowTimestamps);

  // -- History -------------------------------------------------------------
  //
  // History data is fully owned by Chromium's HistoryService.
  // Astra only projects the state and adds presentation preferences.
  //
  // Chromium component: HistoryService
  //   (components/history/core/browser/history_service.h)
  // Chromium component: TopSites
  //   (components/history/core/browser/top_sites.h)
  //
  // These Astra-specific prefs control only presentation — they never
  // store history data.  See AstraHistoryHelper for the projection layer.

  // Whether the history section appears in the sidebar.
  // Default: true.
  registry->RegisterBooleanPref(kPrefHistoryShowInSidebar,
                                kDefaultHistoryShowInSidebar);

  // Default sort order for history listings.
  // Default: "time_desc" — most recent first.
  registry->RegisterStringPref(kPrefHistorySortOrder,
                               kDefaultHistorySortOrder);

  // Maximum number of history results per query.
  // Default: 50.
  registry->RegisterIntegerPref(kPrefHistoryMaxResults,
                                kDefaultHistoryMaxResults);

  // Whether favicons are shown in history listings.
  // Default: true.
  registry->RegisterBooleanPref(kPrefHistoryShowFavicons,
                                kDefaultHistoryShowFavicons);

  // Whether visit counts are shown in history listings.
  // Default: false.
  registry->RegisterBooleanPref(kPrefHistoryShowVisitCount,
                                kDefaultHistoryShowVisitCount);

  // Whether visit times are shown in history listings.
  // Default: true.
  registry->RegisterBooleanPref(kPrefHistoryShowVisitTime,
                                kDefaultHistoryShowVisitTime);

  // History display mode.
  // Default: "list".
  registry->RegisterStringPref(kPrefHistoryDisplayMode,
                               kDefaultHistoryDisplayMode);

  // Whether history results are grouped by date.
  // Default: true.
  registry->RegisterBooleanPref(kPrefHistoryGroupByDate,
                                kDefaultHistoryGroupByDate);

  // Maximum number of history items per day group.
  // Default: 20.
  registry->RegisterIntegerPref(kPrefHistoryItemsPerDay,
                                kDefaultHistoryItemsPerDay);

  // Whether only typed URLs are shown.
  // Default: false.
  registry->RegisterBooleanPref(kPrefHistoryShowTypedUrlsOnly,
                                kDefaultHistoryShowTypedUrlsOnly);

  // Whether history deletion is enabled.
  // Default: true.
  registry->RegisterBooleanPref(kPrefHistoryDeletionEnabled,
                                kDefaultHistoryDeletionEnabled);

  // Whether old history is auto-deleted.
  // Default: false.
  registry->RegisterBooleanPref(kPrefHistoryAutoDelete,
                                kDefaultHistoryAutoDelete);

  // History retention period in days.
  // Default: 90.
  registry->RegisterIntegerPref(kPrefHistoryRetentionDays,
                                kDefaultHistoryRetentionDays);

  // -- Tab search ----------------------------------------------------------
  //
  // Tab search presentation preferences.
  //
  // Actual tab data is owned by Chromium's TabStripModel, TabRestoreService,
  // and BookmarkModel.  These prefs control only the presentation of the
  // Astra tab search UI.

  // Maximum number of visible results in tab search.
  // Default: 15 — reasonable balance of visibility and scannability.
  registry->RegisterIntegerPref(kPrefTabSearchMaxVisible,
                                kDefaultTabSearchMaxVisible);

  // Whether URLs are shown in tab search results.
  // Default: true — URLs help users identify tabs.
  registry->RegisterBooleanPref(kPrefTabSearchShowUrls,
                                kDefaultTabSearchShowUrls);

  // Whether tab thumbnails are shown in tab search.
  // Default: true — thumbnails aid visual recognition.
  registry->RegisterBooleanPref(kPrefTabSearchShowThumbnails,
                                kDefaultTabSearchShowThumbnails);

  // Whether the "recent tabs" section appears on empty query.
  // Default: true — quick access to frequently used tabs.
  registry->RegisterBooleanPref(kPrefTabSearchShowRecentSection,
                                kDefaultTabSearchShowRecentSection);

  // Default sort order for tab search results.
  // Default: 0 = relevance — best matches first.
  registry->RegisterIntegerPref(kPrefTabSearchSortOrder,
                                kDefaultTabSearchSortOrder);

  // -- Glance / peek -------------------------------------------------------
  //
  // Glance presentation preferences and behavior settings.
  //
  // The glance / peek feature shows a small preview bubble for tabs and links.
  // These prefs control how the glance looks and behaves.
  //
  // Chromium subsystem reused: views::BubbleDialogDelegateView
  //   (ui/views/bubble/bubble_dialog_delegate_view.h).
  // Actual tab/WebContents state is owned by Chromium's TabStripModel and
  // content::WebContents.  These prefs control only presentation and behavior.

  // Default display mode for glance.
  // Default: "expanded" — full preview with action bar and status bar.
  registry->RegisterStringPref(kPrefGlanceDefaultDisplayMode,
                               kDefaultGlanceDefaultDisplayMode);

  // Show delay for hover-triggered glance in milliseconds.
  // Default: 500ms — prevents accidental glances from quick mouse moves.
  registry->RegisterIntegerPref(kPrefGlanceShowDelayMs,
                                kDefaultGlanceShowDelayMs);

  // Auto-hide delay for glance in milliseconds.
  // Default: 300ms — hysteresis to prevent flicker.
  registry->RegisterIntegerPref(kPrefGlanceAutoHideDelayMs,
                                kDefaultGlanceAutoHideDelayMs);

  // Compact mode dimensions.
  registry->RegisterIntegerPref(kPrefGlanceCompactWidth,
                                kDefaultGlanceCompactWidth);
  registry->RegisterIntegerPref(kPrefGlanceCompactHeight,
                                kDefaultGlanceCompactHeight);

  // Expanded mode dimensions.
  registry->RegisterIntegerPref(kPrefGlanceExpandedWidth,
                                kDefaultGlanceExpandedWidth);
  registry->RegisterIntegerPref(kPrefGlanceExpandedHeight,
                                kDefaultGlanceExpandedHeight);

  // Whether the action bar is shown in expanded mode.
  // Default: true — action bar provides quick access to common actions.
  registry->RegisterBooleanPref(kPrefGlanceShowActionBar,
                                kDefaultGlanceShowActionBar);

  // Whether the status bar is shown in expanded mode.
  // Default: true — status bar shows URL and security info.
  registry->RegisterBooleanPref(kPrefGlanceShowStatusBar,
                                kDefaultGlanceShowStatusBar);

  // Whether the resize handle is shown.
  // Default: true — allows users to resize the glance bubble.
  registry->RegisterBooleanPref(kPrefGlanceShowResizeHandle,
                                kDefaultGlanceShowResizeHandle);

  // Whether hover peek is enabled.
  // Default: true — hover peek is a core peek feature.
  registry->RegisterBooleanPref(kPrefGlanceHoverPeekEnabled,
                                kDefaultGlanceHoverPeekEnabled);

  // Default glance position / anchor side.
  // Default: "right" — appears to the right of sidebar items.
  registry->RegisterStringPref(kPrefGlanceDefaultPosition,
                               kDefaultGlanceDefaultPosition);

  // Size preset for quick resize.
  // Default: "medium" — the standard expanded size.
  registry->RegisterStringPref(kPrefGlanceSizePreset,
                               kDefaultGlanceSizePreset);

  // Whether the glance is pinned open by default.
  // Default: false — glance auto-hides normally.
  registry->RegisterBooleanPref(kPrefGlancePinnedByDefault,
                                kDefaultGlancePinnedByDefault);

  // Whether the glance remembers its last used size.
  // Default: true — size persistence is convenient for users.
  registry->RegisterBooleanPref(kPrefGlanceRememberSize,
                                kDefaultGlanceRememberSize);

  // Maximum number of recent glance entries to remember.
  // Default: 10.
  registry->RegisterIntegerPref(kPrefGlanceRecentMaxCount,
                                kDefaultGlanceRecentMaxCount);

  // Whether to show the settings button in the glance header.
  // Default: true — quick access to glance settings.
  registry->RegisterBooleanPref(kPrefGlanceShowSettingsButton,
                                kDefaultGlanceShowSettingsButton);

  // Whether animations are enabled for glance.
  // Default: true — smooth animations improve perceived quality.
  registry->RegisterBooleanPref(kPrefGlanceAnimationsEnabled,
                                kDefaultGlanceAnimationsEnabled);

  // Small size preset dimensions.
  registry->RegisterIntegerPref(kPrefGlanceSmallWidth,
                                kDefaultGlanceSmallWidth);
  registry->RegisterIntegerPref(kPrefGlanceSmallHeight,
                                kDefaultGlanceSmallHeight);

  // Large size preset dimensions.
  registry->RegisterIntegerPref(kPrefGlanceLargeWidth,
                                kDefaultGlanceLargeWidth);
  registry->RegisterIntegerPref(kPrefGlanceLargeHeight,
                                kDefaultGlanceLargeHeight);

  // List of recent glance URLs.
  // Default: empty list — no recent glances on a fresh profile.
  registry->RegisterListPref(kPrefGlanceRecentUrls);

  // -- Tab hover preview ---------------------------------------------------
  //
  // Tab hover preview presentation preferences and behavior settings.
  //
  // Tab data (title, URL, favicon, media state) is owned by Chromium's
  // TabStripModel and content::WebContents. These prefs control only the
  // presentation and behavior of the hover preview UI.
  //
  // Chromium subsystem reused: PrefService (persistence).
  // The model (AstraTabHoverModel) reads these prefs and projects them
  // into the view layer.  Views never read prefs directly.

  // Whether tab hover cards are shown on hover.
  // Default: true — hover previews are a core tab identification feature.
  registry->RegisterBooleanPref(kPrefTabHoverShowCards,
                                kDefaultTabHoverShowCards);

  // Hover show delay in milliseconds.
  // Default: 500ms — prevents accidental previews from quick mouse passes.
  registry->RegisterIntegerPref(kPrefTabHoverShowDelayMs,
                                kDefaultTabHoverShowDelayMs);

  // Hover hide delay in milliseconds.
  // Default: 300ms — hysteresis to prevent flicker from brief mouse exits.
  registry->RegisterIntegerPref(kPrefTabHoverHideDelayMs,
                                kDefaultTabHoverHideDelayMs);

  // Whether the preview image/thumbnail is shown.
  // Default: true — thumbnails help with visual tab identification.
  registry->RegisterBooleanPref(kPrefTabHoverShowPreviewImage,
                                kDefaultTabHoverShowPreviewImage);

  // Whether the tab title is shown in the hover card.
  // Default: true — title is essential for identifying tabs.
  registry->RegisterBooleanPref(kPrefTabHoverShowTitle,
                                kDefaultTabHoverShowTitle);

  // Whether the tab URL/domain is shown in the hover card.
  // Default: true — URL helps verify the tab's identity.
  registry->RegisterBooleanPref(kPrefTabHoverShowUrl,
                                kDefaultTabHoverShowUrl);

  // Whether the favicon is shown in the hover card.
  // Default: true — favicons provide quick visual recognition.
  registry->RegisterBooleanPref(kPrefTabHoverShowFavicon,
                                kDefaultTabHoverShowFavicon);

  // Whether the close button is shown in the hover card.
  // Default: true — quick tab closing from the hover card is convenient.
  registry->RegisterBooleanPref(kPrefTabHoverShowCloseButton,
                                kDefaultTabHoverShowCloseButton);

  // Preview image size (0 = small, 1 = medium, 2 = large).
  // Default: 1 = medium — balance of preview detail and compactness.
  registry->RegisterIntegerPref(kPrefTabHoverPreviewImageSize,
                                kDefaultTabHoverPreviewImageSize);

  // Card position relative to the tab (0 = above, 1 = below, 2 = auto).
  // Default: 2 = auto — position adapts to available screen space.
  registry->RegisterIntegerPref(kPrefTabHoverCardPosition,
                                kDefaultTabHoverCardPosition);

  // Whether peek mode is enabled.
  // Default: true — peek mode provides quick deeper previews.
  registry->RegisterBooleanPref(kPrefTabHoverEnablePeekMode,
                                kDefaultTabHoverEnablePeekMode);

  // Peek activation delay in milliseconds.
  // Default: 1500ms — gives users time to read the basic preview first.
  registry->RegisterIntegerPref(kPrefTabHoverPeekDelayMs,
                                kDefaultTabHoverPeekDelayMs);

  // Whether the tab index number is shown.
  // Default: false — most users don't need index numbers.
  registry->RegisterBooleanPref(kPrefTabHoverShowTabIndex,
                                kDefaultTabHoverShowTabIndex);

  // Whether the media playback indicator is shown.
  // Default: true — media state is useful context.
  registry->RegisterBooleanPref(kPrefTabHoverShowMediaIndicator,
                                kDefaultTabHoverShowMediaIndicator);

  // Whether the mute toggle button is shown.
  // Default: true — quick mute control is convenient.
  registry->RegisterBooleanPref(kPrefTabHoverShowMuteButton,
                                kDefaultTabHoverShowMuteButton);

  // Whether animations are enabled for tab hover cards.
  // Default: true — smooth animations improve perceived quality.
  registry->RegisterBooleanPref(kPrefTabHoverAnimationEnabled,
                                kDefaultTabHoverAnimationEnabled);

  // -- Profile menu --------------------------------------------------------
  //
  // Presentation and behavior settings for the Astra profile menu.
  // These control how the profile menu looks and behaves.
  //
  // Chromium owns: profile data, identity, sync status.
  // Astra owns: menu presentation, workspace integration, display modes.
  //
  // The profile menu model (AstraProfileMenuModel) reads these prefs
  // and projects them into the menu views.

  // Whether to show the workspace list in the profile menu.
  // Default: true — workspace switching is a core Astra feature.
  registry->RegisterBooleanPref(kPrefProfileMenuShowWorkspaces,
                                kDefaultProfileMenuShowWorkspaces);

  // Maximum number of workspaces shown in the menu (clamped 3-10).
  // Default: 6 — reasonable balance of visibility and compactness.
  registry->RegisterIntegerPref(kPrefProfileMenuMaxWorkspaces,
                                kDefaultProfileMenuMaxWorkspaces);

  // Whether to show the profile avatar in the menu header.
  // Default: true — avatars help with profile identification.
  registry->RegisterBooleanPref(kPrefProfileMenuShowAvatar,
                                kDefaultProfileMenuShowAvatar);

  // Whether to show sync status in the profile menu.
  // Default: true — sync status is important context.
  registry->RegisterBooleanPref(kPrefProfileMenuShowSyncStatus,
                                kDefaultProfileMenuShowSyncStatus);

  // Workspace display mode (0=icons_only, 1=names_only, 2=icons_and_names).
  // Default: 2 = icons_and_names — most informative.
  registry->RegisterIntegerPref(kPrefProfileMenuWorkspaceDisplayMode,
                                kDefaultProfileMenuWorkspaceDisplayMode);

  // Menu position ("left" or "right").
  // Default: "right" — matches Chromium's default avatar menu position.
  registry->RegisterStringPref(kPrefProfileMenuPosition,
                               kDefaultProfileMenuPosition);

  // Whether to show the recently closed section.
  // Default: true — quick access to recently closed tabs.
  registry->RegisterBooleanPref(kPrefProfileMenuShowRecentlyClosed,
                                kDefaultProfileMenuShowRecentlyClosed);

  // Whether to show the sign-in promo.
  // Default: true — matches Chromium's default behavior.
  registry->RegisterBooleanPref(kPrefProfileMenuShowSignInPromo,
                                kDefaultProfileMenuShowSignInPromo);

  // -- New tab page (NTP) --------------------------------------------------
  //
  // Presentation and behavior settings for the Astra new tab page.
  // These control how the NTP looks and which sections are visible.
  //
  // Chromium owns: actual browsing data (top sites, history, downloads).
  // Astra owns: NTP presentation layout, custom shortcuts, background
  //   style, greeting settings, section visibility, card styling.
  //
  // Model: AstraNewTabModel (//astra/ui/views/newtab/).
  // The model reads these prefs and projects them into view state.
  // Views never read prefs directly.

  // Show greeting.
  registry->RegisterBooleanPref(kPrefNtpShowGreeting,
                                kDefaultNtpShowGreeting);

  // Show search bar.
  registry->RegisterBooleanPref(kPrefNtpShowSearchBar,
                                kDefaultNtpShowSearchBar);

  // Show workspace cards.
  registry->RegisterBooleanPref(kPrefNtpShowWorkspaceCards,
                                kDefaultNtpShowWorkspaceCards);

  // Show shortcuts.
  registry->RegisterBooleanPref(kPrefNtpShowShortcuts,
                                kDefaultNtpShowShortcuts);

  // Show recently closed.
  registry->RegisterBooleanPref(kPrefNtpShowRecentlyClosed,
                                kDefaultNtpShowRecentlyClosed);

  // Show quick actions.
  registry->RegisterBooleanPref(kPrefNtpShowQuickActions,
                                kDefaultNtpShowQuickActions);

  // Shortcut columns (clamped 3-8).
  registry->RegisterIntegerPref(kPrefNtpShortcutColumns,
                                kDefaultNtpShortcutColumns);

  // Max workspaces shown (clamped 3-10).
  registry->RegisterIntegerPref(kPrefNtpMaxWorkspacesShown,
                                kDefaultNtpMaxWorkspacesShown);

  // Max recently closed shown (clamped 3-10).
  registry->RegisterIntegerPref(kPrefNtpMaxRecentlyClosedShown,
                                kDefaultNtpMaxRecentlyClosedShown);

  // Shortcut layout mode (0=grid, 1=list).
  registry->RegisterIntegerPref(kPrefNtpShortcutLayoutMode,
                                kDefaultNtpShortcutLayoutMode);

  // Workspace card style (0=compact, 1=full).
  registry->RegisterIntegerPref(kPrefNtpWorkspaceCardStyle,
                                kDefaultNtpWorkspaceCardStyle);

  // Background style (0=simple, 1=gradient, 2=image).
  registry->RegisterIntegerPref(kPrefNtpBackgroundStyle,
                                kDefaultNtpBackgroundStyle);

  // Custom background URL.
  registry->RegisterStringPref(kPrefNtpCustomBackgroundUrl,
                               kDefaultNtpCustomBackgroundUrl);

  // Show most visited sites.
  registry->RegisterBooleanPref(kPrefNtpShowMostVisited,
                                kDefaultNtpShowMostVisited);

  // Greeting style (0=formal, 1=casual, 2=minimal).
  registry->RegisterIntegerPref(kPrefNtpGreetingStyle,
                                kDefaultNtpGreetingStyle);

  // Custom shortcuts list.
  registry->RegisterListPref(kPrefNtpCustomShortcuts);

  // Quick actions list.
  {
    base::Value::List default_actions;
    default_actions.Append("new_workspace");
    default_actions.Append("screenshot");
    default_actions.Append("focus_mode");
    default_actions.Append("history");
    default_actions.Append("downloads");
    default_actions.Append("bookmarks");
    registry->RegisterListPref(kPrefNtpQuickActions,
                               std::move(default_actions));
  }

  // Recently closed (cached projection of TabRestoreService).
  registry->RegisterListPref(kPrefNtpRecentlyClosed);

  // -- Screenshot ----------------------------------------------------------
  //
  // Screenshot presentation and behavior preferences.
  //
  // Actual screenshot capture and encoding is owned by Chromium's
  // screenshot subsystem and AstraScreenshotService. These prefs control
  // only the presentation and default behavior of the Astra screenshot UI.
  //
  // Chromium owner: ScreenshotManager / ScreenshotService
  //   (chrome/browser/screenshot/, content/browser/screenshot/)
  // Patch point: ScreenshotService / ScreenshotManager UI delegate.

  // Default capture type.
  registry->RegisterIntegerPref(kPrefScreenshotDefaultCaptureType,
                                kDefaultScreenshotDefaultCaptureType);

  // Whether to show the capture bubble after taking a screenshot.
  registry->RegisterBooleanPref(kPrefScreenshotShowCaptureBubble,
                                kDefaultScreenshotShowCaptureBubble);

  // Whether the capture bubble auto-dismisses.
  registry->RegisterBooleanPref(kPrefScreenshotAutoDismissBubble,
                                kDefaultScreenshotAutoDismissBubble);

  // Auto-dismiss delay in seconds.
  registry->RegisterIntegerPref(kPrefScreenshotAutoDismissDelaySeconds,
                                kDefaultScreenshotAutoDismissDelaySeconds);

  // Image format (PNG, JPEG, WebP).
  registry->RegisterIntegerPref(kPrefScreenshotImageFormat,
                                kDefaultScreenshotImageFormat);

  // JPEG quality percentage.
  registry->RegisterIntegerPref(kPrefScreenshotJpegQuality,
                                kDefaultScreenshotJpegQuality);

  // Show filename in capture bubble.
  registry->RegisterBooleanPref(kPrefScreenshotShowFilenameInBubble,
                                kDefaultScreenshotShowFilenameInBubble);

  // Show dimensions in capture bubble.
  registry->RegisterBooleanPref(kPrefScreenshotShowDimensionsInBubble,
                                kDefaultScreenshotShowDimensionsInBubble);

  // Show file size in capture bubble.
  registry->RegisterBooleanPref(kPrefScreenshotShowFileSizeInBubble,
                                kDefaultScreenshotShowFileSizeInBubble);

  // Default save location.
  registry->RegisterStringPref(kPrefScreenshotDefaultSaveLocation,
                               kDefaultScreenshotDefaultSaveLocation);

  // Copy to clipboard after capture.
  registry->RegisterBooleanPref(kPrefScreenshotCopyToClipboardAfterCapture,
                                kDefaultScreenshotCopyToClipboardAfterCapture);

  // Show grid in region selection.
  registry->RegisterBooleanPref(kPrefScreenshotShowGridInRegionSelection,
                                kDefaultScreenshotShowGridInRegionSelection);

  // Show magnifier in region selection.
  registry->RegisterBooleanPref(kPrefScreenshotShowMagnifierInRegionSelection,
                                kDefaultScreenshotShowMagnifierInRegionSelection);

  // Region aspect ratio lock mode.
  registry->RegisterIntegerPref(kPrefScreenshotRegionAspectRatioLock,
                                kDefaultScreenshotRegionAspectRatioLock);

  // Grid size in pixels for snap-to-grid.
  registry->RegisterIntegerPref(kPrefScreenshotGridSizePixels,
                                kDefaultScreenshotGridSizePixels);

  // Snap-to-grid for region selection.
  registry->RegisterBooleanPref(kPrefScreenshotSnapToGrid,
                                kDefaultScreenshotSnapToGrid);

  // Maximum number of recent captures to remember.
  registry->RegisterIntegerPref(kPrefScreenshotMaxRecentCaptures,
                                kDefaultScreenshotMaxRecentCaptures);

  // -- Notifications -------------------------------------------------------
  //
  // Notification presentation and behavior preferences.
  //
  // Actual notification delivery is owned by Chromium's MessageCenter and
  // NotificationPlatformBridge.  Astra adds presentation metadata and
  // per-source filtering on top of Chromium's notification system.
  //
  // Chromium owner: MessageCenter / NotificationService
  //   (chrome/browser/notifications/notification_service.h)
  // Astra projection: AstraNotificationService

  // Whether notifications are enabled globally.
  // Default: true — notifications are on by default.
  registry->RegisterBooleanPref(kPrefNotificationsEnabled,
                                kDefaultNotificationsEnabled);

  // Whether do-not-disturb mode is enabled.
  // Default: false — DND is off by default.
  registry->RegisterBooleanPref(kPrefNotificationDoNotDisturb,
                                kDefaultNotificationDoNotDisturb);

  // Whether message previews are shown in notifications.
  // Default: true — show full message content.
  registry->RegisterBooleanPref(kPrefNotificationShowPreviews,
                                kDefaultNotificationShowPreviews);

  // Whether notification sounds are played.
  // Default: true — sound is on by default.
  registry->RegisterBooleanPref(kPrefNotificationSoundEnabled,
                                kDefaultNotificationSoundEnabled);

  // Auto-dismiss timeout in seconds.
  // Default: 8 seconds — standard notification timeout.
  registry->RegisterIntegerPref(kPrefNotificationTimeoutSeconds,
                                kDefaultNotificationTimeoutSeconds);

  // Maximum number of visible notifications at once.
  // Default: 5 — reasonable balance of visibility and clutter.
  registry->RegisterIntegerPref(kPrefNotificationMaxVisible,
                                kDefaultNotificationMaxVisible);

  // Notification position on screen.
  // Default: "top_right" — standard desktop notification position.
  registry->RegisterStringPref(kPrefNotificationPosition,
                               kDefaultNotificationPosition);

  // Whether to show the notification icon.
  // Default: true — icons help identify notification source.
  registry->RegisterBooleanPref(kPrefNotificationShowIcon,
                                kDefaultNotificationShowIcon);

  // Whether to show the notification timestamp.
  // Default: true — timestamps provide useful context.
  registry->RegisterBooleanPref(kPrefNotificationShowTimestamp,
                                kDefaultNotificationShowTimestamp);

  // Whether to show the close button on notifications.
  // Default: true — users expect to dismiss notifications.
  registry->RegisterBooleanPref(kPrefNotificationShowCloseButton,
                                kDefaultNotificationShowCloseButton);

  // Notification visual style.
  // Default: "default" — standard notification style.
  registry->RegisterStringPref(kPrefNotificationStyle,
                               kDefaultNotificationStyle);

  // Whether similar notifications are stacked / grouped.
  // Default: true — stacking reduces visual clutter.
  registry->RegisterBooleanPref(kPrefNotificationStackNotifications,
                                kDefaultNotificationStackNotifications);

  // Whether quiet mode is enabled (no sound, no popups, just badge).
  // Default: false — quiet mode is off by default.
  registry->RegisterBooleanPref(kPrefNotificationQuietMode,
                                kDefaultNotificationQuietMode);

  // Maximum number of history items to remember.
  // Default: 100 — reasonable history depth without excessive memory.
  registry->RegisterIntegerPref(kPrefNotificationHistorySize,
                                kDefaultNotificationHistorySize);

  // -- Autofill helper -----------------------------------------------------
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

  // Whether autofill is globally enabled in Astra UI.
  // Default: true — autofill is on by default.
  registry->RegisterBooleanPref(kPrefAutofillEnabled,
                                kDefaultAutofillEnabled);

  // Whether address autofill is enabled in Astra UI.
  // Default: true — address autofill is on by default.
  registry->RegisterBooleanPref(kPrefAddressAutofillEnabled,
                                kDefaultAddressAutofillEnabled);

  // Whether credit card autofill is enabled in Astra UI.
  // Default: true — credit card autofill is on by default.
  registry->RegisterBooleanPref(kPrefCreditCardAutofillEnabled,
                                kDefaultCreditCardAutofillEnabled);

  // Whether auto sign-in is enabled in Astra UI.
  // Default: true — auto sign-in is on by default.
  registry->RegisterBooleanPref(kPrefAutosignInEnabled,
                                kDefaultAutosignInEnabled);

  // Whether the autofill popup is shown.
  // Default: true — popup is shown by default.
  registry->RegisterBooleanPref(kPrefShowAutofillPopup,
                                kDefaultShowAutofillPopup);

  // Autofill popup position.
  // Default: "auto".
  registry->RegisterStringPref(kPrefAutofillPopupPosition,
                               kDefaultAutofillPopupPosition);

  // Maximum number of suggestions shown.
  // Default: 6.
  registry->RegisterIntegerPref(kPrefAutofillMaxSuggestions,
                                kDefaultAutofillMaxSuggestions);

  // Whether suggestion icons are shown.
  // Default: true — icons are shown by default.
  registry->RegisterBooleanPref(kPrefShowSuggestionIcons,
                                kDefaultShowSuggestionIcons);

  // Whether suggestion labels are shown.
  // Default: true — labels are shown by default.
  registry->RegisterBooleanPref(kPrefShowSuggestionLabels,
                                kDefaultShowSuggestionLabels);

  // Whether suggestion subtext is shown.
  // Default: true — subtext is shown by default.
  registry->RegisterBooleanPref(kPrefShowSuggestionSubtext,
                                kDefaultShowSuggestionSubtext);

  // Whether autofill triggers on tap/click.
  // Default: true — autofill on tap is on by default.
  registry->RegisterBooleanPref(kPrefAutofillOnTap,
                                kDefaultAutofillOnTap);

  // Suggestions sort order.
  // Default: "most_recent".
  registry->RegisterStringPref(kPrefAutofillSuggestionsSortOrder,
                               kDefaultAutofillSuggestionsSortOrder);

  // Whether credit card network icons are shown.
  // Default: true — card icons are shown by default.
  registry->RegisterBooleanPref(kPrefShowCreditCardIcons,
                                kDefaultShowCreditCardIcons);

  // Whether quick checkout flow is enabled.
  // Default: false — off by default (experimental feature).
  registry->RegisterBooleanPref(kPrefAutofillQuickCheckout,
                                kDefaultAutofillQuickCheckout);

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

  // Whether sync is enabled in Astra UI.
  // Default: true — sync is on by default.
  registry->RegisterBooleanPref(kPrefSyncEnabled, kDefaultSyncEnabled);

  // Whether sync status is shown in Astra UI.
  // Default: true — status is shown by default.
  registry->RegisterBooleanPref(kPrefShowSyncStatus, kDefaultShowSyncStatus);

  // Whether sync errors are shown in Astra UI.
  // Default: true — errors are shown by default.
  registry->RegisterBooleanPref(kPrefShowSyncErrors, kDefaultShowSyncErrors);

  // Whether sync only happens on WiFi.
  // Default: false — sync on any network.
  registry->RegisterBooleanPref(kPrefSyncOnWifiOnly, kDefaultSyncOnWifiOnly);

  // Sync frequency setting.
  // Values: "auto", "hourly", "daily".
  // Default: "auto" — Chromium manages sync timing.
  registry->RegisterStringPref(kPrefSyncFrequency, kDefaultSyncFrequency);

  // Whether automatic sync is enabled.
  // Default: true — auto sync is on by default.
  registry->RegisterBooleanPref(kPrefAutoSync, kDefaultAutoSync);

  // Whether all sync data is encrypted.
  // Default: false — matches Chromium default behavior.
  registry->RegisterBooleanPref(kPrefEncryptAllData, kDefaultEncryptAllData);

  // Whether the account avatar is shown in UI.
  // Default: true — avatar is shown by default.
  registry->RegisterBooleanPref(kPrefShowAccountAvatar,
                                kDefaultShowAccountAvatar);

  // Whether last sync time is shown in UI.
  // Default: true — last sync time is shown by default.
  registry->RegisterBooleanPref(kPrefShowLastSyncTime,
                                kDefaultShowLastSyncTime);

  // Whether the sync icon is shown in the toolbar.
  // Default: true — icon is shown by default.
  registry->RegisterBooleanPref(kPrefShowSyncIconInToolbar,
                                kDefaultShowSyncIconInToolbar);

  // Data type enablement prefs.
  // All data types are enabled by default, matching Chromium behavior.
  registry->RegisterBooleanPref(kPrefSyncBookmarks, kDefaultSyncBookmarks);
  registry->RegisterBooleanPref(kPrefSyncPasswords, kDefaultSyncPasswords);
  registry->RegisterBooleanPref(kPrefSyncHistory, kDefaultSyncHistory);
  registry->RegisterBooleanPref(kPrefSyncTabs, kDefaultSyncTabs);
  registry->RegisterBooleanPref(kPrefSyncSettings, kDefaultSyncSettings);
  registry->RegisterBooleanPref(kPrefSyncExtensions, kDefaultSyncExtensions);
  registry->RegisterBooleanPref(kPrefSyncAutofill, kDefaultSyncAutofill);
  registry->RegisterBooleanPref(kPrefSyncReadingList, kDefaultSyncReadingList);

  // TODO(astra): Register additional Astra prefs as they are introduced.
  // Consider which prefs should be syncable (RegisterSyncablePref) vs.
  // local-only.  For example, workspace metadata is a good candidate for
  // sync (so workspaces follow the user across devices), while sidebar
  // width is typically per-device.
  //
  // Chromium component: sync driver / PrefService sync integration.
  // Patch point: components/sync_preferences or chrome/browser/prefs/.
}

}  // namespace prefs
}  // namespace astra
