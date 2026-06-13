#ifndef ASTRA_APP_ASTRA_ACCELERATOR_TABLE_H_
#define ASTRA_APP_ASTRA_ACCELERATOR_TABLE_H_

#include <optional>
#include <string>
#include <vector>

#include "base/containers/span.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/events/keycodes/keyboard_codes.h"

namespace astra {

// =========================================================================
// Astra accelerator table
// =========================================================================
//
// Defines the keyboard shortcuts (accelerators) for Astra-specific commands.
// These are merged into Chrome's accelerator table at the Chromium patch
// point (chrome/browser/ui/views/accelerator_table.cc) so the native focus
// manager / browser command controller handles them identically to Chrome's
// own accelerators.
//
// Architecture:
//
//   Key press  ->  FocusManager  ->  accelerator table lookup
//                                     (Chrome + Astra merged)
//                                  ->  BrowserCommandController
//                                  ->  [Astra range?]  ->  AstraCommandDelegate
//
// Chromium subsystem reused:
//   - views::FocusManager — accelerator matching and dispatch
//   - BrowserCommandController — command execution entry point
//   - ui::Accelerator — accelerator data type and shortcut text formatting
//
// Astra owns:
//   - Which Astra command IDs have shortcuts
//   - The default keybindings
//   - Shortcut display text for the command palette
//
// TODO(astra): Shortcut customization.  Chrome does not currently expose a
// general-purpose shortcut customization UI for browser commands (only for
// extension commands via the commands API).  When we add user-customizable
// shortcuts, they should be stored in Astra prefs and applied on top of this
// table at FocusManager registration time.
// Chromium subsystem to reuse: PrefService + a custom preferences page
// (chrome://settings/shortcuts if it exists, or a WebUI Astra settings page).
// =========================================================================

// A single accelerator mapping: a command ID paired with a key combination,
// plus metadata for display and conflict detection.
//
// Mirrors Chromium's AcceleratorMapping struct in
// chrome/browser/ui/views/accelerator_table.cc but adds Astra-specific
// fields for the command palette and shortcut settings UI.
//
// Fields:
//   - command_id: The Astra command ID (from astra/browser/astra_command_delegate.h)
//   - accelerator_id: String identifier for this accelerator action (e.g.
//     "astra.toggle_sidebar").  Used by the command palette, settings UI,
//     and for pref-based shortcut customization.
//   - keycode: The keyboard key (ui::KeyboardCode).
//   - modifiers: The modifier keys (bitmask of ui::EventFlags).
//   - is_default: Whether this is a default (factory) accelerator.  User-
//     customized accelerators have this set to false.
//   - description: Human-readable description of what the shortcut does.
struct AstraAcceleratorEntry {
  int command_id;
  std::string accelerator_id;
  ui::KeyboardCode keycode;
  int modifiers;
  bool is_default;
  std::string description;
};

// Returns the full list of Astra accelerator entries.
//
// The patch point in chrome/browser/ui/views/accelerator_table.cc appends
// these entries to the Chrome accelerator table so they are processed by
// FocusManager and dispatched through BrowserCommandController.
//
// The returned span is valid for the lifetime of the process (static data).
base::span<const AstraAcceleratorEntry> GetAstraAcceleratorTable();

// Returns all accelerators for a given command ID.
//
// A command may have multiple keyboard shortcuts (primary + alternatives).
// Returns an empty vector if the command has no accelerator.
std::vector<AstraAcceleratorEntry> GetAcceleratorsForCommand(int command_id);

// Returns the primary accelerator for a command, or nullptr if none.
//
// The primary accelerator is the first one listed in the table.  This is
// what the command palette shows next to the command name.
const AstraAcceleratorEntry* GetPrimaryAcceleratorForCommand(int command_id);

// Returns a human-readable shortcut string for a command's primary
// accelerator, e.g. "Command+Shift+P" on Mac or "Ctrl+Shift+P" on Win/Linux.
//
// Returns an empty string if the command has no accelerator.
//
// Used by the command palette UI to display shortcuts alongside command
// names, and by any UI surface that wants to show a keybinding hint.
std::string GetShortcutTextForCommand(int command_id);

// Returns a human-readable shortcut string for a single accelerator entry.
//
// Same formatting rules as GetShortcutTextForCommand, but for any
// accelerator entry (not just a command lookup).
std::string GetShortcutText(const AstraAcceleratorEntry& entry);

// ---------------------------------------------------------------------------
// Action-based accelerator lookup (string ID)
// ---------------------------------------------------------------------------
//
// These functions look up accelerators by their string accelerator ID
// (e.g. "astra.command_palette").  This is useful for the command palette
// and for pref-based shortcut customization.

// Returns the default accelerator for a given action ID, or nullopt if
// the action has no default accelerator.
//
// The default accelerator is the primary (first) accelerator entry for
// the action that has is_default == true.
//
// Parameters:
//   action_id - The string accelerator ID (e.g. "astra.toggle_sidebar").
//
// Returns the default ui::Accelerator, or std::nullopt if not found.
std::optional<ui::Accelerator> GetDefaultAcceleratorForAction(
    const std::string& action_id);

// Returns all registered accelerator entries.
//
// This returns the full accelerator table as a vector, including all
// alternative shortcuts for each command.
//
// Returns a vector of all AstraAcceleratorEntry entries.
std::vector<AstraAcceleratorEntry> GetAllAccelerators();

// Returns the human-readable description of an accelerator action.
//
// Parameters:
//   action_id - The string accelerator ID.
//
// Returns the description string, or empty if the action is not found.
std::string GetAcceleratorDescription(const std::string& action_id);

// Formats a ui::Accelerator as a human-readable shortcut string.
//
// Examples:
//   - Ctrl+Shift+P (Windows/Linux)
//   - Command+Shift+P (Mac)
//   - Alt+Shift+G
//
// Parameters:
//   accel - The accelerator to format.
//
// Returns a formatted shortcut string like "Ctrl+Shift+P".
//
// TODO(astra): Replace with ui::Accelerator::GetShortcutText() for proper
//   localized shortcut formatting with platform-appropriate symbols.
//   Chromium component: ui/base/accelerators/accelerator.h
std::string FormatAcceleratorText(const ui::Accelerator& accel);

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
bool IsAcceleratorConflicting(const ui::Accelerator& accel);

}  // namespace astra

#endif  // ASTRA_APP_ASTRA_ACCELERATOR_TABLE_H_
