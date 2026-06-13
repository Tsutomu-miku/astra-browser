// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_ACCESSIBILITY_ASTRA_FOCUS_MANAGER_H_
#define ASTRA_UI_ACCESSIBILITY_ASTRA_FOCUS_MANAGER_H_

#include <memory>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "ui/accessibility/ax_enums.mojom-forward.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget_observer.h"

namespace astra {
namespace accessibility {

// =========================================================================
// AstraFocusManager
// =========================================================================
//
// Astra-specific focus management for custom widgets and containers.
//
// This class provides focus behavior that extends Chromium's views::FocusManager
// with Astra-specific patterns:
//
//   - Vertical focus traversal for sidebar lists
//   - Focus ring highlighting for Astra widgets
//   - Focus restoration for bubble dialogs
//   - Focus history tracking for undoing focus changes
//
// Astra views that need custom focus behavior should use this class
// instead of reimplementing focus logic.
//
// Chromium subsystems reused:
//   - views::FocusManager — base focus management
//   - views::FocusChangeListener — focus change observation
//   - views::WidgetObserver — widget lifetime observation
//   - views::FocusTraversable — custom focus traversal order
//
// TODO(astra): Determine whether AstraFocusManager should be a
//   ProfileKeyedService or a Widget-level helper.  For now, it's a
//   per-container helper that can be attached to any view container.
// Chromium pattern: views/focus/focus_manager.h (views::FocusManager)
// Patch point: No Chromium patch needed — this is a Views-layer helper.
// =========================================================================

class AstraFocusManagerObserver;

// Direction for vertical focus traversal.
enum class AstraFocusDirection {
  kUp,    // Move focus to the previous item
  kDown,  // Move focus to the next item
  kFirst,  // Jump to the first item
  kLast,   // Jump to the last item
};

// =========================================================================
// AstraFocusManager
// =========================================================================
//
// Manages focus within an Astra container view (e.g., sidebar, space switcher,
// command palette).  Provides vertical arrow-key navigation, focus ring
// highlighting, and focus restoration.
//
// Usage:
//   auto focus_manager = std::make_unique<AstraFocusManager>(container_view);
//   focus_manager->set_items(my_focusable_items);
//   // In OnKeyPressed: focus_manager->HandleKeyEvent(event);
//
// TODO(astra): Integrate with views::FocusManager for full focus management.
//   The current implementation is a simplified version for list-style containers.
//   For more complex layouts, consider using views::FocusTraversable or
//   custom FocusSearch functions.
// Chromium component: views/focus/focus_manager.h
// Chromium component: views/focus/focus_traversable.h
class AstraFocusManager {
 public:
  // Creates a focus manager for the given container view.
  // |container| must outlive this focus manager.
  explicit AstraFocusManager(views::View* container);

  AstraFocusManager(const AstraFocusManager&) = delete;
  AstraFocusManager& operator=(const AstraFocusManager&) = delete;

  ~AstraFocusManager();

  // -- Item management -------------------------------------------------------

  // Sets the list of focusable items managed by this focus manager.
  // Items should be in traversal order (top to bottom for vertical lists).
  //
  // TODO(astra): Support dynamic item addition/removal instead of
  //   setting the full list.  For now, call SetItems when the list changes.
  void SetItems(const std::vector<views::View*>& items);

  // Adds an item to the end of the focus order.
  void AddItem(views::View* item);

  // Removes an item from the focus order.
  void RemoveItem(views::View* item);

  // Returns the current list of focusable items.
  const std::vector<views::View*>& items() const { return items_; }

  // -- Focus movement -----------------------------------------------------

  // Moves focus in the given direction.
  // Returns the view that received focus, or nullptr if no item was focused.
  // If |wrap| is true, focus wraps around from last to first (or vice versa).
  views::View* MoveFocus(AstraFocusDirection direction, bool wrap = true);

  // Moves focus to a specific item.
  // Returns true if the item was found and focused.
  bool FocusItem(views::View* item);

  // Returns the currently focused item, or nullptr if no item is focused.
  views::View* GetFocusedItem() const;

  // Returns the index of the currently focused item, or -1 if none.
  int GetFocusedIndex() const;

  // -- Key event handling -------------------------------------------------

  // Handles a key event for vertical focus navigation.
  // Supports: Up/Down arrows, Home/End, PageUp/PageDown.
  // Returns true if the key event was handled.
  bool HandleKeyEvent(const ui::KeyEvent& event);

  // -- Focus ring ---------------------------------------------------------

  // Sets whether to show a focus ring around the focused item.
  // Default is true.
  void SetShowFocusRing(bool show);
  bool show_focus_ring() const { return show_focus_ring_; }

  // Sets whether focus wraps around at the ends.
  // Default is true for list-style navigation.
  void SetWrapAround(bool wrap) { wrap_around_ = wrap; }
  bool wrap_around() const { return wrap_around_; }

  // -- Focus restoration -------------------------------------------------

  // Saves the currently focused item for later restoration.
  // Use this before showing a bubble dialog or modal overlay.
  //
  // TODO(astra): Implement a focus stack for nested focus restoration.
  //   When multiple dialogs can be opened sequentially, we need to
  //   push/pop focus state.
  void SaveFocus();

  // Restores focus to the previously saved item.
  // Returns true if focus was successfully restored.
  bool RestoreFocus();

  // Returns true if there is a saved focus position to restore to.
  bool HasSavedFocus() const;

  // Clears the saved focus position.
  void ClearSavedFocus();

  // -- Observer ----------------------------------------------------------

  // Adds/removes an observer.
  void AddObserver(AstraFocusManagerObserver* observer);
  void RemoveObserver(AstraFocusManagerObserver* observer);

 private:
  // Finds the index of the given item in the items_ list.
  // Returns -1 if not found.
  int FindItemIndex(views::View* item) const;

  // Gets the focusable item at the given index, clamping to valid range.
  // Returns nullptr if there are no items.
  views::View* GetItemAt(int index) const;

  // Updates the focus ring visibility on the newly focused item.
  void UpdateFocusRing(views::View* old_focus, views::View* new_focus);

  // Notifies observers that focus changed.
  void NotifyFocusChanged(views::View* old_focus, views::View* new_focus);

  // The container view that owns this focus manager.
  const raw_ptr<views::View> container_;

  // List of focusable items in traversal order.
  std::vector<views::View*> items_;

  // Whether to show a focus ring on the focused item.
  bool show_focus_ring_ = true;

  // Whether focus wraps at the ends.
  bool wrap_around_ = true;

  // The previously focused view (for focus restoration).
  raw_ptr<views::View> saved_focus_ = nullptr;

  // Observers.
  base::ObserverList<AstraFocusManagerObserver>::Unchecked observers_;
};

// =========================================================================
// AstraFocusManagerObserver
// =========================================================================
//
// Observer interface for AstraFocusManager focus changes.
//
// TODO(astra): Add more notification methods as needed (e.g., focus ring state,
//   wrap-around events).
class AstraFocusManagerObserver {
 public:
  virtual ~AstraFocusManagerObserver() = default;

  // Called when focus moves from |old_focus| to |new_focus|.
  // Either may be nullptr.
  virtual void OnFocusChanged(views::View* old_focus, views::View* new_focus) {}

  // Called when the focus manager is about to be destroyed.
  virtual void OnFocusManagerDestroyed() {}
};

// =========================================================================
// Sidebar focus manager specialization
// =========================================================================
//
// AstraSidebarFocusManager is a specialization of AstraFocusManager for
// sidebar containers.  It provides:
//
//   - Vertical arrow key navigation between sidebar sections
//   - Section collapse/expand with Left/Right arrows
//   - Jump to first/last section with Home/End
//   - Focus ring styling optimized for sidebar items
//
// TODO(astra): Implement as a subclass or as a configuration of AstraFocusManager.
//   The sidebar may need section-level focus management where Arrow Up/Down moves
//   between items within a section, and some key combination moves between
//   sections.  For now, AstraFocusManager handles flat list navigation.
//
// TODO(astra): Consider whether sidebar focus management needs section-aware
//   navigation.  Chromium's sidebar implementations (e.g., bookmarks
//   sidebar) use tree view models with hierarchical focus.
// Chromium pattern: chrome/browser/ui/views/bookmarks/bookmark_bar_view.h
// Chromium pattern: ui/views/controls/tree/tree_view.h
//
// For simplicity, the current AstraFocusManager can be configured for
// sidebar use by setting the appropriate items and wrap behavior.

}  // namespace accessibility
}  // namespace astra

#endif  // ASTRA_UI_ACCESSIBILITY_ASTRA_FOCUS_MANAGER_H_
