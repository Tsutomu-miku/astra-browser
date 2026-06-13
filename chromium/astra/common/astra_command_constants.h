#ifndef ASTRA_COMMON_ASTRA_COMMAND_CONSTANTS_H_
#define ASTRA_COMMON_ASTRA_COMMAND_CONSTANTS_H_

namespace astra {

// =========================================================================
// Astra command ID range
// =========================================================================
//
// Astra-specific command IDs occupy the range [kAstraCommandFirst,
// kAstraCommandLast).  This range is reserved to avoid collisions with
// Chromium's built-in command IDs, which live from 0 through ~59999
// (see chrome/app/chrome_command_ids.h and IDC_LAST).
//
// The actual command ID enum is defined in browser/astra_command_delegate.h
// (browser layer).  These boundary constants live in the common layer so
// that app-layer and ui-layer code can check whether a command falls in
// the Astra range without depending on the full command enum.
//
// Chromium patch point: chrome/app/chrome_command_ids.h
//   — reserves the 60000+ range for Astra via a patch.
// Chromium patch point: chrome/browser/ui/browser_command_controller.cc
//   — forwards Astra-range commands to AstraCommandDelegate.
//
// === BOUNDARY RULE (non-negotiable) ===
//   Standard browser commands (new tab, back, forward, reload, close, zoom,
//   find, downloads, history, settings, etc.) MUST NOT be added to the
//   Astra command set.  They go through Chrome's BrowserCommandController
//   unchanged.  Only Astra-specific product commands belong in this range.
// =========================================================================

// First command ID in the Astra range.
inline constexpr int kAstraCommandFirst = 60000;

// One past the last command ID in the Astra range.
// The actual value is determined by the full enum in
// browser/astra_command_delegate.h.  This constant provides a stable
// upper bound for range checks in layers that don't include the full enum.
//
// TODO(astra): Keep this in sync with the kAstraCommandLast enum value
//   in browser/astra_command_delegate.h.  Consider generating both from a
//   single source to avoid drift.  The 500-entry gap below is generous
//   for future expansion.
inline constexpr int kAstraCommandLast = kAstraCommandFirst + 500;

// Returns true if |command_id| falls within the Astra command ID range.
// This is a pure range check — it does not verify that the command ID
// corresponds to a defined Astra command.
constexpr bool IsAstraCommandId(int command_id) {
  return command_id >= kAstraCommandFirst && command_id < kAstraCommandLast;
}

// =========================================================================
// Keyboard shortcut identifiers
// =========================================================================
//
// These are string identifiers used for accelerator registration and
// command palette display.  The actual key bindings are defined in the
// app-layer accelerator table (astra/app/astra_accelerator_table.cc) and
// registered through the Chromium accelerator table patch point.
//
// TODO(astra): Define the full set of Astra accelerator IDs once the
//   command set stabilizes.  Chromium component: ui/base/accelerators.
//   Patch point: chrome/browser/ui/views/accelerator_table.cc.

// Command palette accelerator ID.
inline constexpr char kAstraAcceleratorCommandPalette[] =
    "astra.command_palette";

// Toggle sidebar accelerator ID.
inline constexpr char kAstraAcceleratorToggleSidebar[] =
    "astra.toggle_sidebar";

// Next workspace accelerator ID.
inline constexpr char kAstraAcceleratorNextWorkspace[] =
    "astra.next_workspace";

// Previous workspace accelerator ID.
inline constexpr char kAstraAcceleratorPreviousWorkspace[] =
    "astra.previous_workspace";

// Toggle split view accelerator ID.
inline constexpr char kAstraAcceleratorToggleSplitView[] =
    "astra.toggle_split_view";

// Toggle tab favorite accelerator ID.
inline constexpr char kAstraAcceleratorToggleFavorite[] =
    "astra.toggle_favorite";

// New workspace accelerator ID.
inline constexpr char kAstraAcceleratorNewWorkspace[] =
    "astra.new_workspace";

// Close current workspace accelerator ID.
inline constexpr char kAstraAcceleratorCloseWorkspace[] =
    "astra.close_workspace";

// Rename current workspace accelerator ID.
inline constexpr char kAstraAcceleratorRenameWorkspace[] =
    "astra.rename_workspace";

// Show all workspaces / overview accelerator ID.
inline constexpr char kAstraAcceleratorShowAllWorkspaces[] =
    "astra.show_all_workspaces";

// Toggle focus mode accelerator ID.
inline constexpr char kAstraAcceleratorToggleFocusMode[] =
    "astra.toggle_focus_mode";

// Open tab search accelerator ID.
inline constexpr char kAstraAcceleratorOpenTabSearch[] =
    "astra.open_tab_search";

// Glance peek accelerator ID (keyboard alternative).
inline constexpr char kAstraAcceleratorOpenGlance[] =
    "astra.open_glance";

// Screenshot visible area accelerator ID.
inline constexpr char kAstraAcceleratorScreenshotVisible[] =
    "astra.screenshot_visible";

// Screenshot full page accelerator ID.
inline constexpr char kAstraAcceleratorScreenshotFullPage[] =
    "astra.screenshot_full_page";

// Screenshot region accelerator ID.
inline constexpr char kAstraAcceleratorScreenshotRegion[] =
    "astra.screenshot_region";

// Toggle Picture-in-Picture accelerator ID.
inline constexpr char kAstraAcceleratorTogglePip[] =
    "astra.toggle_pip";

// Stack selected tabs accelerator ID.
inline constexpr char kAstraAcceleratorStackTabs[] =
    "astra.stack_tabs";

// Unstack tabs accelerator ID.
inline constexpr char kAstraAcceleratorUnstackTabs[] =
    "astra.unstack_tabs";

// Open notes accelerator ID.
inline constexpr char kAstraAcceleratorOpenNotes[] =
    "astra.open_notes";

// Open reading list accelerator ID.
inline constexpr char kAstraAcceleratorOpenReadingList[] =
    "astra.open_reading_list";

// Open downloads sidebar accelerator ID.
inline constexpr char kAstraAcceleratorOpenDownloads[] =
    "astra.open_downloads";

// Open bookmarks sidebar accelerator ID.
inline constexpr char kAstraAcceleratorOpenBookmarks[] =
    "astra.open_bookmarks";

// Open history sidebar accelerator ID.
inline constexpr char kAstraAcceleratorOpenHistory[] =
    "astra.open_history";

// Move tab to next workspace accelerator ID.
inline constexpr char kAstraAcceleratorMoveTabToNextWorkspace[] =
    "astra.move_tab_to_next_workspace";

// Move tab to previous workspace accelerator ID.
inline constexpr char kAstraAcceleratorMoveTabToPreviousWorkspace[] =
    "astra.move_tab_to_previous_workspace";

// Workspace quick switch 1 accelerator ID.
inline constexpr char kAstraAcceleratorSwitchToWorkspace1[] =
    "astra.switch_to_workspace_1";

// Workspace quick switch 2 accelerator ID.
inline constexpr char kAstraAcceleratorSwitchToWorkspace2[] =
    "astra.switch_to_workspace_2";

// Workspace quick switch 3 accelerator ID.
inline constexpr char kAstraAcceleratorSwitchToWorkspace3[] =
    "astra.switch_to_workspace_3";

// Workspace quick switch 4 accelerator ID.
inline constexpr char kAstraAcceleratorSwitchToWorkspace4[] =
    "astra.switch_to_workspace_4";

// Workspace quick switch 5 accelerator ID.
inline constexpr char kAstraAcceleratorSwitchToWorkspace5[] =
    "astra.switch_to_workspace_5";

// Workspace quick switch 6 accelerator ID.
inline constexpr char kAstraAcceleratorSwitchToWorkspace6[] =
    "astra.switch_to_workspace_6";

// Workspace quick switch 7 accelerator ID.
inline constexpr char kAstraAcceleratorSwitchToWorkspace7[] =
    "astra.switch_to_workspace_7";

// Workspace quick switch 8 accelerator ID.
inline constexpr char kAstraAcceleratorSwitchToWorkspace8[] =
    "astra.switch_to_workspace_8";

// Workspace quick switch 9 accelerator ID.
inline constexpr char kAstraAcceleratorSwitchToWorkspace9[] =
    "astra.switch_to_workspace_9";

// Toggle DevTools accelerator ID (Astra-specific).
inline constexpr char kAstraAcceleratorToggleDevTools[] =
    "astra.toggle_devtools";

// --- Split view operations ---

// Toggle split view orientation (horizontal / vertical) accelerator ID.
inline constexpr char kAstraAcceleratorSplitViewOrientationToggle[] =
    "astra.split_view_orientation_toggle";

// Resize split view — grow primary pane accelerator ID.
inline constexpr char kAstraAcceleratorSplitViewGrowPrimary[] =
    "astra.split_view_grow_primary";

// Resize split view — shrink primary pane accelerator ID.
inline constexpr char kAstraAcceleratorSplitViewShrinkPrimary[] =
    "astra.split_view_shrink_primary";

// --- Tab stack navigation ---

// Navigate to next tab in stack accelerator ID.
inline constexpr char kAstraAcceleratorNextTabInStack[] =
    "astra.next_tab_in_stack";

// Navigate to previous tab in stack accelerator ID.
inline constexpr char kAstraAcceleratorPreviousTabInStack[] =
    "astra.previous_tab_in_stack";

// --- DevTools Astra panels ---

// Open Astra workspace inspector DevTools panel accelerator ID.
inline constexpr char kAstraAcceleratorDevToolsWorkspacePanel[] =
    "astra.devtools_workspace_panel";

// Open Astra tab inspector DevTools panel accelerator ID.
inline constexpr char kAstraAcceleratorDevToolsTabPanel[] =
    "astra.devtools_tab_panel";

}  // namespace astra

#endif  // ASTRA_COMMON_ASTRA_COMMAND_CONSTANTS_H_
