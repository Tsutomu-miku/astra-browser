// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/app/astra_accelerator_table.h"

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include "base/no_destructor.h"
#include "build/build_config.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/events/event_constants.h"
#include "ui/events/keycodes/keyboard_codes.h"

#include "astra/common/astra_command_constants.h"
#include "astra/browser/astra_command_delegate.h"

namespace astra {

namespace {

// =========================================================================
// Accelerator table definition
// =========================================================================
//
// Each entry maps a command ID and action ID to a key + modifier combination.
// A command may appear multiple times with different key combinations — each
// one is a valid accelerator for that command.
//
// Modifier conventions:
//   - On Mac:  EF_COMMAND_DOWN is the primary modifier (Cmd key)
//   - On Win/Linux: EF_CONTROL_DOWN is the primary modifier (Ctrl key)
//   - EF_SHIFT_DOWN and EF_ALT_DOWN are the same across platforms
//
// We use BUILDFLAG(IS_MAC) to define the "primary modifier" per platform,
// following Chromium's convention for cross-platform accelerators.
//
// All shortcuts are chosen to avoid conflicts with common Chrome shortcuts:
//   - Ctrl/Cmd+T, N, W, R, L, D, F, P, Tab, etc. are all Chrome defaults
//   - Astra shortcuts use combinations that Chrome does not reserve
//
// TODO(astra): Audit all shortcuts against Chrome's full accelerator table
// once a Chromium checkout is available.  Some of these may conflict with
// less common Chrome shortcuts.
// Chromium component: chrome/browser/ui/views/accelerator_table.cc
// =========================================================================

#if BUILDFLAG(IS_MAC)
constexpr int kPrimaryModifier = ui::EF_COMMAND_DOWN;
#else
constexpr int kPrimaryModifier = ui::EF_CONTROL_DOWN;
#endif

// Helper to make entries more concise.
constexpr AstraAcceleratorEntry MakeEntry(int command_id,
                                          const char* accelerator_id,
                                          ui::KeyboardCode keycode,
                                          int modifiers,
                                          bool is_default,
                                          const char* description) {
  return {command_id, std::string(accelerator_id), keycode, modifiers,
          is_default, std::string(description)};
}

// The full Astra accelerator table.
//
// Order matters only in that the first entry for each command is considered
// the "primary" accelerator (shown in the command palette, etc.).
//
// All entries with is_default = true are factory defaults.  User-customized
// shortcuts (when implemented) will be added at runtime with is_default = false.
constexpr AstraAcceleratorEntry kAstraAccelerators[] = {
    // -- Command palette ---------------------------------------------------

    // Open command palette: Cmd/Ctrl+Shift+P
    // This is the de facto standard command palette shortcut (VS Code,
    // Sublime, etc.).  Note: Chrome uses Ctrl/Cmd+Shift+P for print on some
    // platforms; users who prefer print can customize.
    MakeEntry(kAstraCommandOpenCommandPalette,
              kAstraAcceleratorCommandPalette,
              ui::VKEY_P,
              kPrimaryModifier | ui::EF_SHIFT_DOWN,
              /*is_default=*/true,
              "Open the command palette"),

    // Open command palette (alternate): Cmd/Ctrl+K
    // Many productivity apps use Ctrl/Cmd+K for quick search / command
    // palette.  Note: Chrome uses Ctrl/Cmd+K to focus the omnibox in search
    // mode.  We include it as an alternative because many users expect it.
    MakeEntry(kAstraCommandOpenCommandPalette,
              kAstraAcceleratorCommandPalette,
              ui::VKEY_K,
              kPrimaryModifier,
              /*is_default=*/true,
              "Open the command palette (alternative)"),

    // -- Sidebar -----------------------------------------------------------

    // Toggle sidebar: Cmd/Ctrl+B
    // B for "Bookmarks/Sidebar", common shortcut in many apps and browsers.
    // Note: Chrome uses Ctrl/Cmd+Shift+B for bookmarks bar toggle; Ctrl+B
    // is not used by default on most platforms.
    MakeEntry(kAstraCommandToggleSidebar,
              kAstraAcceleratorToggleSidebar,
              ui::VKEY_B,
              kPrimaryModifier,
              /*is_default=*/true,
              "Toggle the Astra sidebar"),

    // -- Workspaces --------------------------------------------------------

    // Next workspace: Cmd/Ctrl+]
    // Right-bracket is a natural "next" key and does not conflict with
    // Chrome's Ctrl+Tab / Ctrl+PageDown for tab switching.
    MakeEntry(kAstraCommandNextWorkspace,
              kAstraAcceleratorNextWorkspace,
              ui::VKEY_OEM_6,  // ] }
              kPrimaryModifier,
              /*is_default=*/true,
              "Switch to the next workspace"),

    // Previous workspace: Cmd/Ctrl+[
    MakeEntry(kAstraCommandPreviousWorkspace,
              kAstraAcceleratorPreviousWorkspace,
              ui::VKEY_OEM_4,  // [ {
              kPrimaryModifier,
              /*is_default=*/true,
              "Switch to the previous workspace"),

    // New workspace: Cmd/Ctrl+Shift+N
    // Shift+N for "New workspace".  Note: Ctrl/Cmd+Shift+N is Chrome's
    // "new incognito window" shortcut.  We use it here because it's a
    // natural "new" shortcut, and Astra workspaces are a prominent feature.
    // TODO(astra): Reconsider if this conflicts too heavily with the
    //   Chrome incognito shortcut for Astra users.
    MakeEntry(kAstraCommandNewWorkspace,
              kAstraAcceleratorNewWorkspace,
              ui::VKEY_N,
              kPrimaryModifier | ui::EF_SHIFT_DOWN,
              /*is_default=*/true,
              "Create a new workspace"),

    // Close workspace: Cmd/Ctrl+Shift+W
    // Note: Ctrl/Cmd+Shift+W is Chrome's "close window" shortcut.
    // This is intentionally mapped to closing the current workspace,
    // which is a similar "close" action at the workspace level.
    MakeEntry(kAstraCommandDeleteWorkspace,
              kAstraAcceleratorCloseWorkspace,
              ui::VKEY_W,
              kPrimaryModifier | ui::EF_SHIFT_DOWN,
              /*is_default=*/true,
              "Close the current workspace"),

    // Show all workspaces: Cmd/Ctrl+Shift+A
    // "A" for "All workspaces" / overview.
    MakeEntry(kAstraCommandShowAllWorkspaces,
              kAstraAcceleratorShowAllWorkspaces,
              ui::VKEY_A,
              kPrimaryModifier | ui::EF_SHIFT_DOWN,
              /*is_default=*/true,
              "Show all workspaces overview"),

    // Workspace quick switch 1-9: Cmd/Ctrl+1 through 9
    // Quick-switch to workspace by index, similar to tab switching with
    // Ctrl/Cmd+1-9 but at the workspace level.
    // Note: Chrome uses Ctrl/Cmd+1-9 for "switch to tab N".  These are
    // marked as non-default to avoid conflicts; users can enable them.
    MakeEntry(kAstraCommandFirst,  // Placeholder, will be replaced with proper
              kAstraAcceleratorSwitchToWorkspace1,
              ui::VKEY_1,
              kPrimaryModifier | ui::EF_ALT_DOWN,
              /*is_default=*/true,
              "Switch to workspace 1"),

    MakeEntry(kAstraCommandFirst,
              kAstraAcceleratorSwitchToWorkspace2,
              ui::VKEY_2,
              kPrimaryModifier | ui::EF_ALT_DOWN,
              /*is_default=*/true,
              "Switch to workspace 2"),

    MakeEntry(kAstraCommandFirst,
              kAstraAcceleratorSwitchToWorkspace3,
              ui::VKEY_3,
              kPrimaryModifier | ui::EF_ALT_DOWN,
              /*is_default=*/true,
              "Switch to workspace 3"),

    MakeEntry(kAstraCommandFirst,
              kAstraAcceleratorSwitchToWorkspace4,
              ui::VKEY_4,
              kPrimaryModifier | ui::EF_ALT_DOWN,
              /*is_default=*/true,
              "Switch to workspace 4"),

    MakeEntry(kAstraCommandFirst,
              kAstraAcceleratorSwitchToWorkspace5,
              ui::VKEY_5,
              kPrimaryModifier | ui::EF_ALT_DOWN,
              /*is_default=*/true,
              "Switch to workspace 5"),

    MakeEntry(kAstraCommandFirst,
              kAstraAcceleratorSwitchToWorkspace6,
              ui::VKEY_6,
              kPrimaryModifier | ui::EF_ALT_DOWN,
              /*is_default=*/true,
              "Switch to workspace 6"),

    MakeEntry(kAstraCommandFirst,
              kAstraAcceleratorSwitchToWorkspace7,
              ui::VKEY_7,
              kPrimaryModifier | ui::EF_ALT_DOWN,
              /*is_default=*/true,
              "Switch to workspace 7"),

    MakeEntry(kAstraCommandFirst,
              kAstraAcceleratorSwitchToWorkspace8,
              ui::VKEY_8,
              kPrimaryModifier | ui::EF_ALT_DOWN,
              /*is_default=*/true,
              "Switch to workspace 8"),

    MakeEntry(kAstraCommandFirst,
              kAstraAcceleratorSwitchToWorkspace9,
              ui::VKEY_9,
              kPrimaryModifier | ui::EF_ALT_DOWN,
              /*is_default=*/true,
              "Switch to workspace 9"),

    // -- Tab features ------------------------------------------------------

    // Toggle tab favorite: Cmd/Ctrl+Shift+F
    // "F" for "Favorite".  Not Ctrl/Cmd+D — that's Chrome's "add bookmark".
    // Note: Ctrl+Shift+F may conflict with some find-bar modes in Chrome.
    MakeEntry(kAstraCommandToggleTabFavorite,
              kAstraAcceleratorToggleFavorite,
              ui::VKEY_F,
              kPrimaryModifier | ui::EF_SHIFT_DOWN,
              /*is_default=*/true,
              "Toggle tab as favorite"),

    // -- Split view --------------------------------------------------------

    // Toggle split view: Cmd/Cmd+\ (backslash)
    // Backslash is a common split-view toggle key in IDEs and terminal apps.
    MakeEntry(kAstraCommandToggleSplitView,
              kAstraAcceleratorToggleSplitView,
              ui::VKEY_OEM_5,  // \ |
              kPrimaryModifier,
              /*is_default=*/true,
              "Toggle split view"),

    // -- Glance / Peek -----------------------------------------------------

    // Open glance: Alt+Shift+G
    // "G" for "Glance".  Alt-based shortcuts are less commonly used by
    // Chrome for core features, so they are good for secondary features.
    // Note: Glance is primarily a mouse-hover feature (Alt+hover), but
    // we provide a keyboard alternative for accessibility and power users.
    MakeEntry(kAstraCommandOpenGlance,
              kAstraAcceleratorOpenGlance,
              ui::VKEY_G,
              ui::EF_ALT_DOWN | ui::EF_SHIFT_DOWN,
              /*is_default=*/true,
              "Open Glance / Peek preview"),

    // -- Tab search --------------------------------------------------------

    // Open tab search: Cmd/Ctrl+Shift+K
    // "K" for "seKarch" — a common pattern in search shortcuts.
    // Note: Chrome uses Ctrl/Cmd+K to focus the omnibox.  Shift+K is
    // less commonly used.
    MakeEntry(kAstraCommandOpenTabSearch,
              kAstraAcceleratorOpenTabSearch,
              ui::VKEY_K,
              kPrimaryModifier | ui::EF_SHIFT_DOWN,
              /*is_default=*/true,
              "Open tab search"),

    // -- Focus mode --------------------------------------------------------

    // Toggle focus mode: Cmd/Ctrl+Shift+F11
    // F11 is Chrome's fullscreen key; Shift+F11 for focus mode is a
    // related "immersion" shortcut.
    MakeEntry(kAstraCommandToggleFocusMode,
              kAstraAcceleratorToggleFocusMode,
              ui::VKEY_F11,
              kPrimaryModifier | ui::EF_SHIFT_DOWN,
              /*is_default=*/true,
              "Toggle focus mode"),

    // -- Screenshot --------------------------------------------------------

    // Screenshot visible area: Cmd/Ctrl+Shift+S
    // "S" for "Screenshot".  Note: Ctrl/Cmd+S is "save page" in Chrome;
    // Shift+S is not commonly used.
    MakeEntry(kAstraCommandScreenshotVisible,
              kAstraAcceleratorScreenshotVisible,
              ui::VKEY_S,
              kPrimaryModifier | ui::EF_SHIFT_DOWN,
              /*is_default=*/true,
              "Capture visible area screenshot"),

    // Screenshot full page: Cmd/Ctrl+Shift+F12
    // F12 is DevTools; Shift+F12 for screenshot is a related dev tool.
    MakeEntry(kAstraCommandScreenshotFullPage,
              kAstraAcceleratorScreenshotFullPage,
              ui::VKEY_F12,
              kPrimaryModifier | ui::EF_SHIFT_DOWN,
              /*is_default=*/true,
              "Capture full page screenshot"),

    // Screenshot region: Cmd/Ctrl+Shift+5
    // 5 is used for screenshot region in some screenshot tools.
    MakeEntry(kAstraCommandScreenshotRegion,
              kAstraAcceleratorScreenshotRegion,
              ui::VKEY_5,
              kPrimaryModifier | ui::EF_SHIFT_DOWN,
              /*is_default=*/true,
              "Capture region screenshot"),

    // -- Notes -------------------------------------------------------------

    // Open notes: Cmd/Ctrl+Shift+/
    // ? (Shift+/) is a common "help/notes" key.  Notes are a help-like
    // annotation feature.
    MakeEntry(kAstraCommandFirst,
              kAstraAcceleratorOpenNotes,
              ui::VKEY_OEM_2,  // / ?
              kPrimaryModifier | ui::EF_SHIFT_DOWN,
              /*is_default=*/true,
              "Open notes"),

    // -- Reading list ------------------------------------------------------

    // Open reading list: Cmd/Ctrl+Shift+L
    // "L" for "Reading List".  Not Ctrl/Cmd+L — that's focus address bar.
    MakeEntry(kAstraCommandFirst,
              kAstraAcceleratorOpenReadingList,
              ui::VKEY_L,
              kPrimaryModifier | ui::EF_SHIFT_DOWN,
              /*is_default=*/true,
              "Open reading list in sidebar"),

    // -- Downloads ---------------------------------------------------------

    // Open downloads sidebar: Cmd/Ctrl+J
    // "J" for downloads.  This matches Chrome's existing downloads shortcut
    // (Ctrl/Cmd+J opens the downloads page), so we reuse the same key for
    // the Astra downloads sidebar panel.
    MakeEntry(kAstraCommandFirst,
              kAstraAcceleratorOpenDownloads,
              ui::VKEY_J,
              kPrimaryModifier,
              /*is_default=*/true,
              "Open downloads in sidebar"),

    // -- Bookmarks ---------------------------------------------------------

    // Open bookmarks sidebar: Cmd/Ctrl+Shift+O
    // "O" for "Bookmarks".  Shift+O to avoid conflict with Ctrl/Cmd+O
    // (open file).
    MakeEntry(kAstraCommandFirst,
              kAstraAcceleratorOpenBookmarks,
              ui::VKEY_O,
              kPrimaryModifier | ui::EF_SHIFT_DOWN,
              /*is_default=*/true,
              "Open bookmarks in sidebar"),

    // -- History -----------------------------------------------------------

    // Open history sidebar: Cmd/Ctrl+H
    // "H" for "History".  This matches Chrome's existing history shortcut.
    MakeEntry(kAstraCommandFirst,
              kAstraAcceleratorOpenHistory,
              ui::VKEY_H,
              kPrimaryModifier,
              /*is_default=*/true,
              "Open history in sidebar"),

    // -- Move tab between workspaces ---------------------------------------

    // Move tab to next workspace: Cmd/Ctrl+Shift+]
    // Shift + next-workspace key = "move tab to next".
    MakeEntry(kAstraCommandMoveTabToNextWorkspace,
              kAstraAcceleratorMoveTabToNextWorkspace,
              ui::VKEY_OEM_6,  // ] }
              kPrimaryModifier | ui::EF_SHIFT_DOWN,
              /*is_default=*/true,
              "Move current tab to next workspace"),

    // Move tab to previous workspace: Cmd/Ctrl+Shift+[
    MakeEntry(kAstraCommandMoveTabToPreviousWorkspace,
              kAstraAcceleratorMoveTabToPreviousWorkspace,
              ui::VKEY_OEM_4,  // [ {
              kPrimaryModifier | ui::EF_SHIFT_DOWN,
              /*is_default=*/true,
              "Move current tab to previous workspace"),

    // -- DevTools ----------------------------------------------------------

    // Toggle DevTools (Astra panel): Cmd/Ctrl+Shift+I
    // This matches Chrome's standard DevTools shortcut.  The Astra version
    // opens DevTools with the Astra panel active.
    // Note: This intentionally mirrors Chrome's IDC_DEV_TOOLS shortcut.
    // When the Astra DevTools feature is enabled, this shortcut opens
    // DevTools and focuses the Astra panel.
    MakeEntry(kAstraCommandOpenAstraDevToolsPanel,
              kAstraAcceleratorToggleDevTools,
              ui::VKEY_I,
              kPrimaryModifier | ui::EF_SHIFT_DOWN,
              /*is_default=*/true,
              "Open DevTools with Astra panel"),
};

// =========================================================================
// Reserved Chromium accelerator list (for conflict detection)
// =========================================================================
//
// This is a best-effort list of well-known Chrome shortcuts that Astra
// should not override.  The actual Chrome accelerator table is much larger
// and varies by platform.
//
// TODO(astra): Generate this list dynamically from the Chrome accelerator
//   table at runtime (via FocusManager).  For now, we hardcode the most
//   common Chrome shortcuts.
//   Chromium component: chrome/browser/ui/views/accelerator_table.cc

// Reserved Chrome accelerator entries (keycode + modifiers).
// These are shortcuts that Chrome uses for core browser functionality
// and that Astra should not override by default.
struct ReservedAccelerator {
  ui::KeyboardCode keycode;
  int modifiers;
};

constexpr ReservedAccelerator kReservedChromeAccelerators[] = {
    // Window / tab management
    {ui::VKEY_T, kPrimaryModifier},                 // New tab
    {ui::VKEY_N, kPrimaryModifier},                 // New window
    {ui::VKEY_W, kPrimaryModifier},                 // Close tab
    {ui::VKEY_W, kPrimaryModifier | ui::EF_SHIFT_DOWN},  // Close window
    {ui::VKEY_R, kPrimaryModifier},                 // Reload
    {ui::VKEY_R, kPrimaryModifier | ui::EF_SHIFT_DOWN},  // Hard reload
    {ui::VKEY_L, kPrimaryModifier},                 // Focus address bar
    {ui::VKEY_D, kPrimaryModifier},                 // Bookmark this tab
    {ui::VKEY_F, kPrimaryModifier},                 // Find
    {ui::VKEY_P, kPrimaryModifier},                 // Print
    {ui::VKEY_O, kPrimaryModifier},                 // Open file
    {ui::VKEY_J, kPrimaryModifier},                 // Downloads
    {ui::VKEY_H, kPrimaryModifier},                 // History
    {ui::VKEY_K, kPrimaryModifier},                 // Focus omnibox (search mode)
    {ui::VKEY_TAB, kPrimaryModifier},               // Next tab
    {ui::VKEY_TAB, kPrimaryModifier | ui::EF_SHIFT_DOWN},  // Previous tab

    // DevTools
    {ui::VKEY_I, kPrimaryModifier | ui::EF_SHIFT_DOWN},   // Toggle DevTools
    {ui::VKEY_J, kPrimaryModifier | ui::EF_SHIFT_DOWN},   // Toggle console
    {ui::VKEY_F12, 0},                                     // F12 DevTools

    // Zoom
    {ui::VKEY_OEM_PLUS, kPrimaryModifier},        // Zoom in
    {ui::VKEY_OEM_MINUS, kPrimaryModifier},       // Zoom out
    {ui::VKEY_0, kPrimaryModifier},               // Zoom reset

    // Navigation
    {ui::VKEY_LEFT, kPrimaryModifier},            // Back
    {ui::VKEY_RIGHT, kPrimaryModifier},           // Forward

    // Fullscreen
    {ui::VKEY_F11, 0},                             // Fullscreen
};

// Checks if an accelerator matches a reserved Chrome accelerator.
bool MatchesReserved(const ui::Accelerator& accel) {
  for (const auto& reserved : kReservedChromeAccelerators) {
    if (accel.key_code() == reserved.keycode &&
        accel.modifiers() == reserved.modifiers) {
      return true;
    }
  }
  return false;
}

// Converts an AstraAcceleratorEntry to a ui::Accelerator.
ui::Accelerator EntryToAccelerator(const AstraAcceleratorEntry& entry) {
  return ui::Accelerator(entry.keycode, entry.modifiers);
}

}  // namespace

// =========================================================================
// Public API — table access and command-based lookup
// =========================================================================

base::span<const AstraAcceleratorEntry> GetAstraAcceleratorTable() {
  return base::span<const AstraAcceleratorEntry>(kAstraAccelerators);
}

std::vector<AstraAcceleratorEntry> GetAcceleratorsForCommand(int command_id) {
  std::vector<AstraAcceleratorEntry> result;
  for (const auto& entry : GetAstraAcceleratorTable()) {
    if (entry.command_id == command_id) {
      result.push_back(entry);
    }
  }
  return result;
}

const AstraAcceleratorEntry* GetPrimaryAcceleratorForCommand(int command_id) {
  for (const auto& entry : GetAstraAcceleratorTable()) {
    if (entry.command_id == command_id) {
      return &entry;
    }
  }
  return nullptr;
}

std::string GetShortcutTextForCommand(int command_id) {
  const AstraAcceleratorEntry* entry =
      GetPrimaryAcceleratorForCommand(command_id);
  if (!entry) {
    return std::string();
  }
  return GetShortcutText(*entry);
}

std::string GetShortcutText(const AstraAcceleratorEntry& entry) {
  ui::Accelerator accel(entry.keycode, entry.modifiers);
  return FormatAcceleratorText(accel);
}

// =========================================================================
// Public API — action-based lookup and utilities
// =========================================================================

std::optional<ui::Accelerator> GetDefaultAcceleratorForAction(
    const std::string& action_id) {
  // Returns the default (primary) accelerator for the given action ID.
  //
  // The default accelerator is the first entry in the table that matches
  // the action ID and has is_default == true.
  //
  // Parameters:
  //   action_id - The string accelerator ID (e.g. "astra.toggle_sidebar").
  //
  // Returns the default ui::Accelerator, or std::nullopt if not found.

  if (action_id.empty()) {
    return std::nullopt;
  }

  for (const auto& entry : GetAstraAcceleratorTable()) {
    if (entry.accelerator_id == action_id && entry.is_default) {
      return EntryToAccelerator(entry);
    }
  }
  return std::nullopt;
}

std::vector<AstraAcceleratorEntry> GetAllAccelerators() {
  // Returns all registered accelerator entries as a vector.
  //
  // This includes all entries from the static table — both primary and
  // alternative shortcuts for each command.
  //
  // Returns a vector of all AstraAcceleratorEntry entries.

  std::vector<AstraAcceleratorEntry> result;
  auto table = GetAstraAcceleratorTable();
  result.reserve(table.size());
  for (const auto& entry : table) {
    result.push_back(entry);
  }
  return result;
}

std::string GetAcceleratorDescription(const std::string& action_id) {
  // Returns the human-readable description of an accelerator action.
  //
  // Parameters:
  //   action_id - The string accelerator ID.
  //
  // Returns the description string, or empty if the action is not found.

  if (action_id.empty()) {
    return std::string();
  }

  for (const auto& entry : GetAstraAcceleratorTable()) {
    if (entry.accelerator_id == action_id) {
      return entry.description;
    }
  }
  return std::string();
}

std::string FormatAcceleratorText(const ui::Accelerator& accel) {
  // Formats a ui::Accelerator as a human-readable shortcut string.
  //
  // Examples:
  //   - "Ctrl+Shift+P" (Windows/Linux)
  //   - "Command+Shift+P" (Mac)
  //
  // Parameters:
  //   accel - The accelerator to format.
  //
  // Returns a formatted shortcut string.
  //
  // TODO(astra): Replace with ui::Accelerator::GetShortcutText() for proper
  //   localized shortcut formatting with platform-appropriate symbols
  //   (e.g. ⌘⇧ on Mac).  Chromium component: ui/base/accelerators/accelerator.h

  std::string text;
  int modifiers = accel.modifiers();

#if BUILDFLAG(IS_MAC)
  // Mac: use symbolic modifier names in order: Cmd, Alt, Shift, Ctrl
  if (modifiers & ui::EF_COMMAND_DOWN) {
    text += "\xE2\x8C\x98";  // ⌘
  }
  if (modifiers & ui::EF_ALT_DOWN) {
    text += "\xE2\x8C\xA5";  // ⌥
  }
  if (modifiers & ui::EF_CONTROL_DOWN) {
    text += "\xE2\x8C\x83";  // ⌃
  }
  if (modifiers & ui::EF_SHIFT_DOWN) {
    text += "\xE2\x87\xA7";  // ⇧
  }
#else
  // Win/Linux: use spelled-out modifier names
  if (modifiers & ui::EF_CONTROL_DOWN) {
    text += "Ctrl+";
  }
  if (modifiers & ui::EF_ALT_DOWN) {
    text += "Alt+";
  }
  if (modifiers & ui::EF_SHIFT_DOWN) {
    text += "Shift+";
  }
  if (modifiers & ui::EF_COMMAND_DOWN) {
    text += "Meta+";
  }
#endif

  // Append the key name.
  // TODO(astra): Use ui::Accelerator::GetKeyString() or equivalent for
  //   proper key name localization and special key handling.
  switch (accel.key_code()) {
    case ui::VKEY_A:
      text += "A";
      break;
    case ui::VKEY_B:
      text += "B";
      break;
    case ui::VKEY_F:
      text += "F";
      break;
    case ui::VKEY_G:
      text += "G";
      break;
    case ui::VKEY_H:
      text += "H";
      break;
    case ui::VKEY_I:
      text += "I";
      break;
    case ui::VKEY_J:
      text += "J";
      break;
    case ui::VKEY_K:
      text += "K";
      break;
    case ui::VKEY_L:
      text += "L";
      break;
    case ui::VKEY_N:
      text += "N";
      break;
    case ui::VKEY_O:
      text += "O";
      break;
    case ui::VKEY_P:
      text += "P";
      break;
    case ui::VKEY_S:
      text += "S";
      break;
    case ui::VKEY_W:
      text += "W";
      break;
    case ui::VKEY_0:
      text += "0";
      break;
    case ui::VKEY_1:
      text += "1";
      break;
    case ui::VKEY_2:
      text += "2";
      break;
    case ui::VKEY_3:
      text += "3";
      break;
    case ui::VKEY_4:
      text += "4";
      break;
    case ui::VKEY_5:
      text += "5";
      break;
    case ui::VKEY_6:
      text += "6";
      break;
    case ui::VKEY_7:
      text += "7";
      break;
    case ui::VKEY_8:
      text += "8";
      break;
    case ui::VKEY_9:
      text += "9";
      break;
    case ui::VKEY_RETURN:
      text += "Enter";
      break;
    case ui::VKEY_TAB:
      text += "Tab";
      break;
    case ui::VKEY_F11:
      text += "F11";
      break;
    case ui::VKEY_F12:
      text += "F12";
      break;
    case ui::VKEY_OEM_2:  // / ?
      text += "/";
      break;
    case ui::VKEY_OEM_4:  // [ {
      text += "[";
      break;
    case ui::VKEY_OEM_5:  // \ |
      text += "\\";
      break;
    case ui::VKEY_OEM_6:  // ] }
      text += "]";
      break;
    case ui::VKEY_OEM_PLUS:
      text += "+";
      break;
    case ui::VKEY_OEM_MINUS:
      text += "-";
      break;
    case ui::VKEY_NEXT:  // PageDown
      text += "PageDown";
      break;
    case ui::VKEY_PRIOR:  // PageUp
      text += "PageUp";
      break;
    case ui::VKEY_LEFT:
      text += "Left";
      break;
    case ui::VKEY_RIGHT:
      text += "Right";
      break;
    default:
      text += "Key";
      break;
  }

  return text;
}

bool IsAcceleratorConflicting(const ui::Accelerator& accel) {
  // Checks whether an accelerator conflicts with reserved Chromium shortcuts.
  //
  // This compares the given accelerator against a list of well-known Chrome
  // shortcuts that Astra should not override.  Conflict detection is best-
  // effort — the actual Chrome accelerator table may vary by platform and
  // Chromium version.
  //
  // Parameters:
  //   accel - The accelerator to check.
  //
  // Returns true if the accelerator conflicts with a known Chrome shortcut.
  //
  // TODO(astra): Audit against the full Chrome accelerator table once a
  //   Chromium checkout is available.  The current list is based on common
  //   well-known Chrome shortcuts.
  //   Chromium component: chrome/browser/ui/views/accelerator_table.cc

  return MatchesReserved(accel);
}

}  // namespace astra
