#ifndef ASTRA_APP_ASTRA_ACCELERATOR_REGISTRAR_H_
#define ASTRA_APP_ASTRA_ACCELERATOR_REGISTRAR_H_

#include <vector>

#include "base/containers/flat_set.h"
#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/events/keycodes/keyboard_codes.h"

namespace views {
class FocusManager;
}

namespace views {
class AcceleratorTarget;
}

namespace ui {
class KeyEvent;
}

namespace astra {

// =========================================================================
// AstraAcceleratorObserver
// =========================================================================
//
// Observer interface for accelerator registration and activation events.
// Observers can be registered with the accelerator registrar to be notified
// when accelerators are registered, unregistered, or activated.
//
// All methods have empty default implementations so subclasses can override
// only the methods they care about.
//
// Inherits from base::CheckedObserver so observers can safely be removed
// while iteration is in progress.
//
// TODO(astra): Consider moving this observer to the browser layer if it
//   needs to interact with browser-level services.  For now it lives in
//   the app layer since it's part of the accelerator registration system.
// =========================================================================
class AstraAcceleratorObserver : public base::CheckedObserver {
 public:
  // Called when an accelerator is registered with the FocusManager.
  // |accelerator| is the accelerator that was registered.
  // |command_id| is the Astra command ID associated with the accelerator.
  virtual void OnAcceleratorRegistered(const ui::Accelerator& accelerator,
                                       int command_id) {}

  // Called when an accelerator is unregistered from the FocusManager.
  // |accelerator| is the accelerator that was unregistered.
  // |command_id| is the Astra command ID associated with the accelerator.
  virtual void OnAcceleratorUnregistered(const ui::Accelerator& accelerator,
                                         int command_id) {}

  // Called when a registered accelerator is activated (pressed).
  // This is useful for analytics, recent commands tracking, etc.
  // |accelerator| is the accelerator that was activated.
  // |command_id| is the Astra command ID associated with the accelerator.
  virtual void OnAcceleratorActivated(const ui::Accelerator& accelerator,
                                      int command_id) {}

 protected:
  ~AstraAcceleratorObserver() override = default;
};

// =========================================================================
// Astra accelerator registrar
// =========================================================================
//
// Helper that registers Astra-specific accelerators with a widget's
// FocusManager.  This provides a programmatic way to add Astra shortcuts
// at runtime, as an alternative to patching Chrome's static accelerator
// table.
//
// Two approaches for getting Astra accelerators into the shortcut pipeline:
//
//   1. Static table merge (preferred): Patch
//      chrome/browser/ui/views/accelerator_table.cc to include Astra
//      entries.  This is how Chrome handles all accelerators — they are
//      compiled into the table and registered by FocusManager at widget
//      creation time.  See patch 0007-accelerator-table.md.
//
//   2. Runtime registration (fallback): Call
//      RegisterAstraAccelerators(focus_manager, target) after BrowserView
//      construction.  This works from the Astra layer without patching
//      the accelerator table, but is less integrated (e.g. the Chrome
//      shortcut system may not be aware of these accelerators for
//      tooltip/menu display).
//
// Both approaches result in the same dispatch path:
//   Key -> FocusManager -> BrowserCommandController -> AstraCommandDelegate
//
// Chromium subsystem reused:
//   - views::FocusManager — accelerator registration and matching
//   - views::AcceleratorTarget — command dispatch
//   - ui::Accelerator — accelerator data type
//
// Platform support:
//   - macOS: Uses Cmd (EF_COMMAND_DOWN) as primary modifier
//   - Windows/Linux: Uses Ctrl (EF_CONTROL_DOWN) as primary modifier
//   - Both platforms support Shift, Alt, and other modifiers
//
// Conflict handling:
//   - Astra accelerators are designed to not conflict with Chrome shortcuts
//   - If a conflict is detected at registration time, it is logged and
//     the Chrome accelerator takes precedence (since it's registered first)
//   - Conflicts can be queried via GetConflictingAccelerators() for debugging
//
// TODO(astra): Decide which approach to use as the primary mechanism.
// The static table merge (approach 1) is more idiomatic and consistent
// with how Chrome manages accelerators, but requires a Chromium patch.
// Runtime registration (approach 2) is patch-free but less integrated.
// Chromium component: chrome/browser/ui/views/accelerator_table.cc
// =========================================================================

// Result of accelerator registration.
struct AstraAcceleratorRegistrationResult {
  // Number of accelerators successfully registered.
  int registered_count = 0;

  // Number of accelerators that conflicted with existing accelerators
  // and were skipped.
  int conflict_count = 0;

  // List of key combinations that conflicted (for debugging).
  std::vector<ui::Accelerator> conflicts;
};

// Registers all Astra accelerators with the given FocusManager.
//
// Call this after a browser window widget is created to add Astra
// shortcuts to its focus manager.  The accelerators are dispatched
// through the FocusManager's normal accelerator handling pipeline.
//
// Each accelerator is registered with the given AcceleratorTarget,
// which is typically the BrowserView (which routes to
// BrowserCommandController).
//
// Parameters:
//   focus_manager - The FocusManager to register accelerators with.
//   target        - The AcceleratorTarget that will handle the commands.
//                   This is typically BrowserView or an Astra-specific
//                   target that forwards to AstraCommandDelegate.
//
// Returns a registration result with counts and conflicts.
//
// TODO(astra): Verify that the focus manager's accelerator target is
//   correctly set up to route through BrowserCommandController.  In
//   Chrome, BrowserView is the accelerator target and forwards to
//   BrowserCommandController.  Astra commands in the 60000+ range are
//   then forwarded to AstraCommandDelegate via the command-forwarding
//   patch (patch 0003).
// Chromium component: chrome/browser/ui/views/frame/browser_view.cc
AstraAcceleratorRegistrationResult RegisterAstraAccelerators(
    views::FocusManager* focus_manager,
    views::AcceleratorTarget* target);

// Unregisters all Astra accelerators from the given FocusManager.
//
// Call this during widget teardown or if Astra shortcuts should be
// temporarily disabled.
//
// Parameters:
//   focus_manager - The FocusManager to unregister from.
//   target        - The AcceleratorTarget that was used for registration.
void UnregisterAstraAccelerators(views::FocusManager* focus_manager,
                                 views::AcceleratorTarget* target);

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
//
// TODO(astra): Store registered accelerators per-FocusManager rather than
//   globally.  Currently the registrar tracks all registered accelerators
//   globally, which works for a single-window scenario but not for
//   multi-window.
bool RegisterAccelerator(views::FocusManager* focus_manager,
                         views::AcceleratorTarget* target,
                         const ui::Accelerator& accelerator,
                         int command_id);

// Unregisters a single Astra accelerator.
//
// Parameters:
//   focus_manager - The FocusManager to unregister from.
//   target        - The AcceleratorTarget used during registration.
//   accelerator   - The accelerator to unregister.
//
// Returns true if the accelerator was found and unregistered, false if
// it was not registered.
bool UnregisterAccelerator(views::FocusManager* focus_manager,
                           views::AcceleratorTarget* target,
                           const ui::Accelerator& accelerator);

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
//
// TODO(astra): Implement per-FocusManager lookup.  Currently this checks
//   the global list of registered Astra accelerators, which works for
//   single-window scenarios.
std::optional<ui::Accelerator> FindAcceleratorByKeyEvent(
    const ui::KeyEvent& event);

// Returns the number of registered Astra accelerators.
//
// This counts all accelerators that have been successfully registered
// via RegisterAstraAccelerators() or RegisterAccelerator().
//
// Returns the count of registered accelerators.
int GetRegisteredAcceleratorCount();

// Returns all registered Astra accelerators.
//
// Returns a vector of all registered ui::Accelerator objects.
std::vector<ui::Accelerator> GetAllRegisteredAccelerators();

// Returns the list of Astra accelerators that would conflict with
// existing accelerators in the given FocusManager.
//
// This is useful for debugging shortcut conflicts without actually
// modifying the FocusManager's state.
//
// Parameters:
//   focus_manager - The FocusManager to check against.
//
// Returns a vector of conflicting accelerators.
std::vector<ui::Accelerator> GetConflictingAccelerators(
    views::FocusManager* focus_manager);

// Returns whether a given accelerator is already registered with the
// FocusManager (by any target).
//
// This is used to detect conflicts before registering Astra accelerators.
//
// Parameters:
//   focus_manager - The FocusManager to check.
//   accelerator   - The accelerator to check for.
//
// Returns true if the accelerator is already registered.
bool IsAcceleratorRegistered(views::FocusManager* focus_manager,
                             const ui::Accelerator& accelerator);

// Returns whether any registered Astra accelerators have conflicts.
//
// A conflict is when an Astra accelerator shares the same key combination
// as a known Chrome shortcut (from the reserved accelerator list).
//
// Returns true if at least one registered accelerator conflicts with
// a known Chrome shortcut.
bool HasConflicts();

// Returns the number of registered Astra accelerators that conflict
// with known Chrome shortcuts.
//
// Returns the count of conflicting accelerators.
int GetConflictCount();

// -- Observer management ----------------------------------------------------

// Adds an observer for accelerator registration and activation events.
//
// Parameters:
//   observer - The observer to add.  Must not be null.
void AddAcceleratorObserver(AstraAcceleratorObserver* observer);

// Removes an observer.
//
// Parameters:
//   observer - The observer to remove.  Must not be null.
void RemoveAcceleratorObserver(AstraAcceleratorObserver* observer);

// Notifies all observers that an accelerator was activated.
//
// This is called from the accelerator target when an Astra accelerator
// is pressed.  External code should only call this for testing.
//
// Parameters:
//   accelerator - The accelerator that was activated.
//   command_id  - The associated command ID.
void NotifyAcceleratorActivated(const ui::Accelerator& accelerator,
                                int command_id);

// -- Constants -------------------------------------------------------------

// Priority for Astra accelerators in the FocusManager.
//
// Chrome uses default priority (0) for its accelerators. Astra uses
// the same priority so that Astra accelerators are treated equally.
// If an Astra accelerator conflicts with a Chrome one, the Chrome
// accelerator wins because it was registered first.
//
// Use a higher priority only if Astra should intentionally override
// a Chrome shortcut (which should be rare).
constexpr int kAstraAcceleratorPriority = 0;

// Whether to skip conflicting accelerators or override them.
//
// When true (default), conflicting accelerators are skipped and logged.
// When false, Astra accelerators override existing ones at the same priority.
//
// TODO(astra): Consider making this a pref or feature flag for users who
// want Astra shortcuts to override Chrome ones.
constexpr bool kSkipConflictingAccelerators = true;

// -- Testing helpers -------------------------------------------------------

// Resets the registrar's internal state for testing.
//
// This clears the registered accelerator set, observer list, and conflict
// tracking.  Only for use in unit tests.
void ResetAcceleratorRegistrarForTesting();

}  // namespace astra

#endif  // ASTRA_APP_ASTRA_ACCELERATOR_REGISTRAR_H_
