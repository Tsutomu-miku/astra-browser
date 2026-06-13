// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/app/astra_accelerator_registrar.h"

#include <map>
#include <optional>
#include <vector>

#include "base/check.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/observer_list.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/events/event.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/views/focus/focus_manager.h"

#include "astra/app/astra_accelerator_table.h"
#include "astra/browser/astra_command_delegate.h"

namespace astra {

namespace {

// =========================================================================
// Internal state
// =========================================================================
//
// The registrar maintains internal state for:
//   - Registered accelerators (for count/lookup/conflict detection)
//   - Observer list (for registration and activation notifications)
//
// TODO(astra): Make this per-FocusManager (or per-BrowserView) instead of
//   global.  Currently the registrar tracks all registered accelerators
//   globally, which works for a single-window scenario but not for
//   multi-window.  For multi-window support, we'd need a registrar per
//   FocusManager instance.
//   Chromium pattern: BrowserView owns its FocusManager and accelerator
//   table registration.

// Registered accelerator entry: accelerator + command ID.
struct RegisteredAcceleratorEntry {
  ui::Accelerator accelerator;
  int command_id;

  bool operator<(const RegisteredAcceleratorEntry& other) const {
    if (accelerator.key_code() != other.accelerator.key_code()) {
      return accelerator.key_code() < other.accelerator.key_code();
    }
    return accelerator.modifiers() < other.accelerator.modifiers();
  }
};

// Set of registered Astra accelerators.
using RegisteredAcceleratorSet = std::vector<RegisteredAcceleratorEntry>;

RegisteredAcceleratorSet& GetRegisteredAccelerators() {
  static base::NoDestructor<RegisteredAcceleratorSet> registered;
  return *registered;
}

// Observer list for accelerator events.
using AcceleratorObserverList =
    base::ObserverList<AstraAcceleratorObserver>;

AcceleratorObserverList& GetObservers() {
  static base::NoDestructor<AcceleratorObserverList> observers;
  return *observers;
}

// =========================================================================
// Helper functions
// =========================================================================

// Converts an AstraAcceleratorEntry to a ui::Accelerator.
ui::Accelerator EntryToAccelerator(const AstraAcceleratorEntry& entry) {
  return ui::Accelerator(entry.keycode, entry.modifiers);
}

// Checks if an accelerator is already in the registered set.
bool IsInRegisteredSet(const ui::Accelerator& accelerator) {
  const auto& registered = GetRegisteredAccelerators();
  for (const auto& entry : registered) {
    if (entry.accelerator.key_code() == accelerator.key_code() &&
        entry.accelerator.modifiers() == accelerator.modifiers()) {
      return true;
    }
  }
  return false;
}

// Finds a registered accelerator entry.
const RegisteredAcceleratorEntry* FindInRegisteredSet(
    const ui::Accelerator& accelerator) {
  const auto& registered = GetRegisteredAccelerators();
  for (const auto& entry : registered) {
    if (entry.accelerator.key_code() == accelerator.key_code() &&
        entry.accelerator.modifiers() == accelerator.modifiers()) {
      return &entry;
    }
  }
  return nullptr;
}

// Adds an accelerator to the registered set.
void AddToRegisteredSet(const ui::Accelerator& accelerator, int command_id) {
  if (IsInRegisteredSet(accelerator)) {
    return;  // Already registered — don't add duplicates.
  }

  RegisteredAcceleratorEntry entry;
  entry.accelerator = accelerator;
  entry.command_id = command_id;
  GetRegisteredAccelerators().push_back(entry);
}

// Removes an accelerator from the registered set.
// Returns true if found and removed.
bool RemoveFromRegisteredSet(const ui::Accelerator& accelerator) {
  auto& registered = GetRegisteredAccelerators();
  for (auto it = registered.begin(); it != registered.end(); ++it) {
    if (it->accelerator.key_code() == accelerator.key_code() &&
        it->accelerator.modifiers() == accelerator.modifiers()) {
      registered.erase(it);
      return true;
    }
  }
  return false;
}

// Notifies all observers that an accelerator was registered.
void NotifyAcceleratorRegistered(const ui::Accelerator& accelerator,
                                 int command_id) {
  for (auto& observer : GetObservers()) {
    observer.OnAcceleratorRegistered(accelerator, command_id);
  }
}

// Notifies all observers that an accelerator was unregistered.
void NotifyAcceleratorUnregistered(const ui::Accelerator& accelerator,
                                   int command_id) {
  for (auto& observer : GetObservers()) {
    observer.OnAcceleratorUnregistered(accelerator, command_id);
  }
}

}  // namespace

// =========================================================================
// Public API — bulk registration
// =========================================================================

bool IsAcceleratorRegistered(views::FocusManager* focus_manager,
                             const ui::Accelerator& accelerator) {
  // Returns true if |accelerator| is already registered with |focus_manager|
  // by any target at any priority.
  //
  // This is used for conflict detection before registering Astra
  // accelerators, so we can log conflicts and skip them if configured.
  //
  // Note: FocusManager doesn't expose a direct "is registered" query.
  // We check using the internal registered set first, then fall back to
  // a best-effort check.
  //
  // TODO(astra): Verify the exact FocusManager API for checking
  //   existing accelerators. Chromium's FocusManager may have
  //   GetAcceleratorTarget() or similar.
  //   For now, we check our own registered set and assume no other
  //   conflicts for the conflict detection API.

  // Check our own registered set first.
  if (IsInRegisteredSet(accelerator)) {
    return true;
  }

  if (!focus_manager) {
    return false;
  }

  // Check if this accelerator is known to conflict with reserved Chrome
  // shortcuts.  This is a best-effort check.
  if (IsAcceleratorConflicting(accelerator)) {
    // Assume reserved Chrome shortcuts are always registered.
    return true;
  }

  DVLOG(3) << "Checking accelerator registration: "
           << accelerator.GetShortcutText();

  // TODO(astra): Replace with actual FocusManager API call.
  //   Common APIs to try:
  //     - bool IsAcceleratorRegistered(const ui::Accelerator&) const
  //     - AcceleratorTarget* GetAcceleratorTarget(const ui::Accelerator&)
  //     - void GetAccelerators(std::vector<ui::Accelerator>*)
  //
  // For now, we return based on our own tracking.  Conflict detection
  // against Chrome's own accelerators is best-effort via IsAcceleratorConflicting.
  return false;
}

std::vector<ui::Accelerator> GetConflictingAccelerators(
    views::FocusManager* focus_manager) {
  // Returns all Astra accelerators that would conflict with existing
  // accelerators in |focus_manager|.
  //
  // This is a debugging utility that doesn't modify any state.

  std::vector<ui::Accelerator> conflicts;

  if (!focus_manager) {
    return conflicts;
  }

  for (const auto& entry : GetAstraAcceleratorTable()) {
    ui::Accelerator accelerator = EntryToAccelerator(entry);
    if (IsAcceleratorRegistered(focus_manager, accelerator)) {
      conflicts.push_back(accelerator);
      DVLOG(1) << "Conflict detected: " << accelerator.GetShortcutText()
               << " (command ID: " << entry.command_id << ")";
    }
  }

  return conflicts;
}

AstraAcceleratorRegistrationResult RegisterAstraAccelerators(
    views::FocusManager* focus_manager,
    views::AcceleratorTarget* target) {
  // Registers all Astra accelerators with |focus_manager| and |target|.
  //
  // Each accelerator in the Astra accelerator table is registered with
  // the FocusManager at kAstraAcceleratorPriority. The AcceleratorTarget
  // (typically BrowserView) receives the accelerator events and forwards
  // them to BrowserCommandController, which then routes Astra command IDs
  // to AstraCommandDelegate.
  //
  // Conflict handling:
  //   - If kSkipConflictingAccelerators is true and a conflict is detected,
  //     the Astra accelerator is skipped and logged.
  //   - If kSkipConflictingAccelerators is false, the Astra accelerator
  //     is registered at the same priority, and since Chrome registers first,
  //     Chrome's accelerator takes precedence (FocusManager processes them
  //     in registration order at the same priority).
  //
  // Parameters:
  //   focus_manager - The FocusManager to register with.
  //   target        - The AcceleratorTarget to receive accelerator events.
  //                   Must outlive the registration.
  //
  // Returns a result struct with registration counts and conflicts.

  AstraAcceleratorRegistrationResult result;

  if (!focus_manager || !target) {
    DLOG(WARNING) << "RegisterAstraAccelerators called with null "
                  << "FocusManager or AcceleratorTarget — aborting.";
    return result;
  }

  DVLOG(1) << "Registering Astra accelerators with FocusManager...";

  int total = 0;

  for (const auto& entry : GetAstraAcceleratorTable()) {
    ui::Accelerator accelerator = EntryToAccelerator(entry);
    ++total;

    // Check for conflicts before registering.
    if (kSkipConflictingAccelerators &&
        IsAcceleratorRegistered(focus_manager, accelerator)) {
      result.conflict_count++;
      result.conflicts.push_back(accelerator);

      DLOG(WARNING) << "Astra accelerator conflict — skipping: "
                    << accelerator.GetShortcutText()
                    << " (command ID: " << entry.command_id << "). "
                    << "Chrome shortcut takes precedence.";
      continue;
    }

    // Register the accelerator with the FocusManager.
    focus_manager->RegisterAccelerator(accelerator, kAstraAcceleratorPriority,
                                       target);

    // Track in our internal set.
    AddToRegisteredSet(accelerator, entry.command_id);

    // Notify observers.
    NotifyAcceleratorRegistered(accelerator, entry.command_id);

    result.registered_count++;

    DVLOG(2) << "Registered Astra accelerator: "
             << accelerator.GetShortcutText()
             << " -> command " << entry.command_id;
  }

  DVLOG(1) << "Astra accelerator registration complete: "
           << result.registered_count << " registered, "
           << result.conflict_count << " conflicts (of " << total << " total)";

  return result;
}

void UnregisterAstraAccelerators(views::FocusManager* focus_manager,
                                 views::AcceleratorTarget* target) {
  // Unregisters all Astra accelerators from |focus_manager| and |target|.
  //
  // Call this during widget teardown or to temporarily disable Astra
  // shortcuts. Each accelerator in the Astra table is unregistered from
  // the FocusManager.
  //
  // Parameters:
  //   focus_manager - The FocusManager to unregister from.
  //   target        - The AcceleratorTarget used during registration.
  //                   Must match the target used for registration.

  if (!focus_manager || !target) {
    return;
  }

  DVLOG(1) << "Unregistering Astra accelerators from FocusManager...";

  int unregistered = 0;

  for (const auto& entry : GetAstraAcceleratorTable()) {
    ui::Accelerator accelerator = EntryToAccelerator(entry);

    // Unregister the accelerator from FocusManager.
    bool was_registered =
        focus_manager->UnregisterAccelerator(accelerator, target);

    // Update our internal tracking.
    bool was_tracked = RemoveFromRegisteredSet(accelerator);
    if (was_tracked) {
      NotifyAcceleratorUnregistered(accelerator, entry.command_id);
    }

    if (was_registered || was_tracked) {
      ++unregistered;
    }

    DVLOG(3) << "Unregistered Astra accelerator: "
             << accelerator.GetShortcutText()
             << " (was_registered=" << was_registered
             << ", was_tracked=" << was_tracked << ")";
  }

  DVLOG(1) << "Astra accelerator unregistration complete: "
           << unregistered << " unregistered";
}

// =========================================================================
// Public API — single accelerator registration
// =========================================================================

bool RegisterAccelerator(views::FocusManager* focus_manager,
                         views::AcceleratorTarget* target,
                         const ui::Accelerator& accelerator,
                         int command_id) {
  // Registers a single Astra accelerator with conflict detection.
  //
  // This is a fine-grained version of RegisterAstraAccelerators() that
  // registers one accelerator at a time.  Use this for dynamic accelerator
  // registration (e.g. user-customized shortcuts).
  //
  // Parameters:
  //   focus_manager - The FocusManager to register with.
  //   target        - The AcceleratorTarget that will handle the command.
  //   accelerator   - The accelerator to register.
  //   command_id    - The Astra command ID for this accelerator.
  //
  // Returns true if the accelerator was successfully registered, false if
  // it was skipped due to conflict or error.

  if (!focus_manager || !target) {
    DLOG(WARNING) << "RegisterAccelerator called with null FocusManager "
                  << "or AcceleratorTarget.";
    return false;
  }

  // Check for conflicts.
  if (kSkipConflictingAccelerators &&
      IsAcceleratorRegistered(focus_manager, accelerator)) {
    DLOG(WARNING) << "Astra accelerator conflict — skipping registration: "
                  << accelerator.GetShortcutText()
                  << " (command ID: " << command_id << ")";
    return false;
  }

  // Register with FocusManager.
  focus_manager->RegisterAccelerator(accelerator, kAstraAcceleratorPriority,
                                     target);

  // Track in our internal set.
  AddToRegisteredSet(accelerator, command_id);

  // Notify observers.
  NotifyAcceleratorRegistered(accelerator, command_id);

  DVLOG(2) << "Registered single Astra accelerator: "
           << accelerator.GetShortcutText()
           << " -> command " << command_id;

  return true;
}

bool UnregisterAccelerator(views::FocusManager* focus_manager,
                           views::AcceleratorTarget* target,
                           const ui::Accelerator& accelerator) {
  // Unregisters a single Astra accelerator.
  //
  // Parameters:
  //   focus_manager - The FocusManager to unregister from.
  //   target        - The AcceleratorTarget used during registration.
  //   accelerator   - The accelerator to unregister.
  //
  // Returns true if the accelerator was found and unregistered, false if
  // it was not registered.

  if (!focus_manager || !target) {
    return false;
  }

  // Unregister from FocusManager.
  bool was_registered =
      focus_manager->UnregisterAccelerator(accelerator, target);

  // Update our internal tracking.
  const RegisteredAcceleratorEntry* entry = FindInRegisteredSet(accelerator);
  int command_id = entry ? entry->command_id : -1;
  bool was_tracked = RemoveFromRegisteredSet(accelerator);

  if (was_tracked && command_id != -1) {
    NotifyAcceleratorUnregistered(accelerator, command_id);
  }

  DVLOG(3) << "Unregistered single Astra accelerator: "
           << accelerator.GetShortcutText()
           << " (was_registered=" << was_registered
           << ", was_tracked=" << was_tracked << ")";

  return was_registered || was_tracked;
}

// =========================================================================
// Public API — lookup and utilities
// =========================================================================

std::optional<ui::Accelerator> FindAcceleratorByKeyEvent(
    const ui::KeyEvent& event) {
  // Finds an accelerator that matches a given key event.
  //
  // This looks through all registered Astra accelerators to find one that
  // matches the key event's key code and modifiers.
  //
  // Parameters:
  //   event - The key event to match against.
  //
  // Returns the matching ui::Accelerator if found, or std::nullopt if
  // no registered accelerator matches the event.

  // Extract key code and modifiers from the event.
  ui::KeyboardCode key_code = event.key_code();
  int modifiers = event.modifiers();

  // Mask to only the relevant modifier flags.
  // Chromium typically uses EF_SHIFT_DOWN, EF_CONTROL_DOWN, EF_ALT_DOWN,
  // and EF_COMMAND_DOWN for accelerator matching.
  const int kModifierMask =
      ui::EF_SHIFT_DOWN | ui::EF_CONTROL_DOWN |
      ui::EF_ALT_DOWN | ui::EF_COMMAND_DOWN;
  modifiers = modifiers & kModifierMask;

  // Look up in registered set.
  ui::Accelerator candidate(key_code, modifiers);
  const RegisteredAcceleratorEntry* entry = FindInRegisteredSet(candidate);
  if (entry) {
    return entry->accelerator;
  }

  return std::nullopt;
}

int GetRegisteredAcceleratorCount() {
  // Returns the number of registered Astra accelerators.
  //
  // This counts all accelerators that have been successfully registered
  // via RegisterAstraAccelerators() or RegisterAccelerator().
  //
  // Returns the count of registered accelerators.

  return static_cast<int>(GetRegisteredAccelerators().size());
}

std::vector<ui::Accelerator> GetAllRegisteredAccelerators() {
  // Returns all registered Astra accelerators.
  //
  // Returns a vector of all registered ui::Accelerator objects.

  std::vector<ui::Accelerator> result;
  const auto& registered = GetRegisteredAccelerators();
  result.reserve(registered.size());
  for (const auto& entry : registered) {
    result.push_back(entry.accelerator);
  }
  return result;
}

bool HasConflicts() {
  // Returns whether any registered Astra accelerators have conflicts.
  //
  // A conflict is when a registered Astra accelerator shares the same
  // key combination as a known Chrome shortcut (from the reserved
  // accelerator list in the accelerator table).
  //
  // Returns true if at least one registered accelerator conflicts with
  // a known Chrome shortcut.

  const auto& registered = GetRegisteredAccelerators();
  for (const auto& entry : registered) {
    if (IsAcceleratorConflicting(entry.accelerator)) {
      return true;
    }
  }
  return false;
}

int GetConflictCount() {
  // Returns the number of registered Astra accelerators that conflict
  // with known Chrome shortcuts.
  //
  // Returns the count of conflicting accelerators.

  int count = 0;
  const auto& registered = GetRegisteredAccelerators();
  for (const auto& entry : registered) {
    if (IsAcceleratorConflicting(entry.accelerator)) {
      ++count;
    }
  }
  return count;
}

// =========================================================================
// Public API — observer management
// =========================================================================

void AddAcceleratorObserver(AstraAcceleratorObserver* observer) {
  // Adds an observer for accelerator registration and activation events.
  //
  // Parameters:
  //   observer - The observer to add.  Must not be null.

  DCHECK(observer);
  GetObservers().AddObserver(observer);
}

void RemoveAcceleratorObserver(AstraAcceleratorObserver* observer) {
  // Removes an observer.
  //
  // Parameters:
  //   observer - The observer to remove.  Must not be null.

  DCHECK(observer);
  GetObservers().RemoveObserver(observer);
}

void NotifyAcceleratorActivated(const ui::Accelerator& accelerator,
                                int command_id) {
  // Notifies all observers that an accelerator was activated.
  //
  // This is called from the accelerator target when an Astra accelerator
  // is pressed.  External code should only call this for testing.
  //
  // Parameters:
  //   accelerator - The accelerator that was activated.
  //   command_id  - The associated command ID.

  DVLOG(2) << "Astra accelerator activated: "
           << accelerator.GetShortcutText()
           << " (command ID: " << command_id << ")";

  for (auto& observer : GetObservers()) {
    observer.OnAcceleratorActivated(accelerator, command_id);
  }
}

// =========================================================================
// Testing helpers
// =========================================================================

void ResetAcceleratorRegistrarForTesting() {
  // Resets the registrar's internal state for testing.
  //
  // This clears the registered accelerator set, observer list, and
  // conflict tracking.  Only for use in unit tests.

  DVLOG(1) << "ResetAcceleratorRegistrarForTesting() — clearing all state.";

  GetRegisteredAccelerators().clear();

  // Note: we don't clear observers here because tests might want to
  // keep observers registered across resets.  Tests should manage
  // observer registration themselves.
}

}  // namespace astra
