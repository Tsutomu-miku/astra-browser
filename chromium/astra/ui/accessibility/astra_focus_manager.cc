// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/accessibility/astra_focus_manager.h"

#include <algorithm>

#include "base/observer_list.h"
#include "ui/events/event.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace astra {
namespace accessibility {

// =========================================================================
// AstraFocusManager
// =========================================================================

AstraFocusManager::AstraFocusManager(views::View* container)
    : container_(container) {
  // TODO(astra): Register as a focus change listener on the container's
  //   focus manager to track focus changes from all sources (mouse,
  //   keyboard, programmatic).
  // Chromium component: views/focus/focus_manager.h (FocusChangeListener)
}

AstraFocusManager::~AstraFocusManager() {
  // Notify observers that we're being destroyed.
  for (AstraFocusManagerObserver& observer : observers_) {
    observer.OnFocusManagerDestroyed();
  }

  // Clear focus ring on all items.
  // TODO(astra): Use focus ring helper from accessibility utils.
  for (views::View* item : items_) {
    if (item) {
      item->SchedulePaint();
    }
  }
}

// -- Item management ---------------------------------------------------------

void AstraFocusManager::SetItems(const std::vector<views::View*>& items) {
  // Clear focus ring on old items if needed.
  if (show_focus_ring_) {
    for (views::View* item : items_) {
      if (item) {
        // TODO(astra): Use HideFocusRing helper.
        item->SchedulePaint();
      }
    }
  }

  items_ = items;

  // TODO(astra): Verify all items are descendants of container_.
  //   Items that are not in the container's hierarchy can cause issues
  //   with focus management.
}

void AstraFocusManager::AddItem(views::View* item) {
  if (!item) {
    return;
  }
  items_.push_back(item);
}

void AstraFocusManager::RemoveItem(views::View* item) {
  // TODO(astra): If the removed item was focused, move focus to a
  //   neighboring item or clear focus.
  auto it = std::find(items_.begin(), items_.end(), item);
  if (it != items_.end()) {
    items_.erase(it);
  }

  if (saved_focus_ == item) {
    saved_focus_ = nullptr;
  }
}

// -- Focus movement ----------------------------------------------------------

views::View* AstraFocusManager::MoveFocus(AstraFocusDirection direction,
                                          bool wrap) {
  if (items_.empty()) {
    return nullptr;
  }

  int current_index = GetFocusedIndex();
  int new_index = current_index;

  switch (direction) {
    case AstraFocusDirection::kUp:
      if (current_index < 0) {
        // No item focused — focus the last item.
        new_index = static_cast<int>(items_.size()) - 1;
      } else if (current_index > 0) {
        new_index = current_index - 1;
      } else if (wrap) {
        // Wrap around to last item.
        new_index = static_cast<int>(items_.size()) - 1;
      } else {
        // At first item and not wrapping — stay put.
        return items_[current_index];
      }
      break;

    case AstraFocusDirection::kDown:
      if (current_index < 0) {
        // No item focused — focus the first item.
        new_index = 0;
      } else if (current_index < static_cast<int>(items_.size()) - 1) {
        new_index = current_index + 1;
      } else if (wrap) {
        // Wrap around to first item.
        new_index = 0;
      } else {
        // At last item and not wrapping — stay put.
        return items_[current_index];
      }
      break;

    case AstraFocusDirection::kFirst:
      new_index = 0;
      break;

    case AstraFocusDirection::kLast:
      new_index = static_cast<int>(items_.size()) - 1;
      break;
  }

  // Clamp to valid range.
  new_index = std::max(0, std::min(static_cast<int>(items_.size()) - 1, new_index));

  views::View* old_focus = (current_index >= 0) ? items_[current_index] : nullptr;
  views::View* new_focus = items_[new_index];

  if (old_focus == new_focus) {
    return new_focus;
  }

  if (new_focus) {
    new_focus->RequestFocus();
  }

  UpdateFocusRing(old_focus, new_focus);
  NotifyFocusChanged(old_focus, new_focus);

  return new_focus;
}

bool AstraFocusManager::FocusItem(views::View* item) {
  int index = FindItemIndex(item);
  if (index < 0) {
    return false;
  }

  views::View* old_focus = GetFocusedItem();

  if (item) {
    item->RequestFocus();
  }

  UpdateFocusRing(old_focus, item);
  NotifyFocusChanged(old_focus, item);

  return true;
}

views::View* AstraFocusManager::GetFocusedItem() const {
  if (!container_ || items_.empty()) {
    return nullptr;
  }

  views::FocusManager* focus_manager = container_->GetFocusManager();
  if (!focus_manager) {
    return nullptr;
  }

  views::View* focused = focus_manager->GetFocusedView();
  if (!focused) {
    return nullptr;
  }

  // Check if the focused view is one of our items (or a descendant of one).
  for (views::View* item : items_) {
    if (item == focused || item->Contains(focused)) {
      return item;
    }
  }

  return nullptr;
}

int AstraFocusManager::GetFocusedIndex() const {
  views::View* focused = GetFocusedItem();
  return FindItemIndex(focused);
}

// -- Key event handling ------------------------------------------------------

bool AstraFocusManager::HandleKeyEvent(const ui::KeyEvent& event) {
  if (event.type() != ui::ET_KEY_PRESSED) {
    return false;
  }

  switch (event.key_code()) {
    case ui::VKEY_UP:
      MoveFocus(AstraFocusDirection::kUp, wrap_around_);
      return true;

    case ui::VKEY_DOWN:
      MoveFocus(AstraFocusDirection::kDown, wrap_around_);
      return true;

    case ui::VKEY_HOME:
      MoveFocus(AstraFocusDirection::kFirst, /*wrap=*/false);
      return true;

    case ui::VKEY_END:
      MoveFocus(AstraFocusDirection::kLast, /*wrap=*/false);
      return true;

    case ui::VKEY_PRIOR:  // Page Up
      // TODO(astra): Implement page-up movement (jump by page size).
      //   For now, jump to first item.
      MoveFocus(AstraFocusDirection::kFirst, /*wrap=*/false);
      return true;

    case ui::VKEY_NEXT:  // Page Down
      // TODO(astra): Implement page-down movement.
      MoveFocus(AstraFocusDirection::kLast, /*wrap=*/false);
      return true;

    default:
      return false;
  }
}

// -- Focus ring --------------------------------------------------------------

void AstraFocusManager::SetShowFocusRing(bool show) {
  if (show_focus_ring_ == show) {
    return;
  }

  show_focus_ring_ = show;

  // Update the focus ring on the currently focused item.
  views::View* focused = GetFocusedItem();
  if (focused) {
    // TODO(astra): Use ShowFocusRing/HideFocusRing helpers.
    focused->SchedulePaint();
  }
}

// -- Focus restoration -------------------------------------------------------

void AstraFocusManager::SaveFocus() {
  saved_focus_ = GetFocusedItem();
}

bool AstraFocusManager::RestoreFocus() {
  if (!saved_focus_) {
    return false;
  }

  // Verify the saved focus is still in our items list.
  if (FindItemIndex(saved_focus_) < 0) {
    saved_focus_ = nullptr;
    return false;
  }

  views::View* old_focus = GetFocusedItem();
  saved_focus_->RequestFocus();

  UpdateFocusRing(old_focus, saved_focus_);
  NotifyFocusChanged(old_focus, saved_focus_);

  return true;
}

bool AstraFocusManager::HasSavedFocus() const {
  if (!saved_focus_) {
    return false;
  }
  // Verify the saved item is still in the list.
  return FindItemIndex(saved_focus_) >= 0;
}

void AstraFocusManager::ClearSavedFocus() {
  saved_focus_ = nullptr;
}

// -- Observer ----------------------------------------------------------------

void AstraFocusManager::AddObserver(AstraFocusManagerObserver* observer) {
  if (observer) {
    observers_.AddObserver(observer);
  }
}

void AstraFocusManager::RemoveObserver(AstraFocusManagerObserver* observer) {
  if (observer) {
    observers_.RemoveObserver(observer);
  }
}

// -- Private helpers ---------------------------------------------------------

int AstraFocusManager::FindItemIndex(views::View* item) const {
  if (!item) {
    return -1;
  }
  auto it = std::find(items_.begin(), items_.end(), item);
  if (it != items_.end()) {
    return static_cast<int>(it - items_.begin());
  }
  return -1;
}

views::View* AstraFocusManager::GetItemAt(int index) const {
  if (index < 0 || index >= static_cast<int>(items_.size())) {
    return nullptr;
  }
  return items_[index];
}

void AstraFocusManager::UpdateFocusRing(views::View* old_focus,
                                        views::View* new_focus) {
  if (!show_focus_ring_) {
    return;
  }

  // TODO(astra): Use ShowFocusRing/HideFocusRing helpers from
  //   astra_accessibility_util.h.  For now, we just schedule a repaint.
  //   The actual focus ring rendering should be handled by the view
  //   or by a focus ring view installed on the item.
  //
  // Chromium component: views/controls/focus_ring.h (views::FocusRing)
  //   views::FocusRing installs itself as a child view and paints
  //   a focus ring around its parent.
  //
  // TODO(astra): Determine the Astra design for focus rings.
  //   Options:
  //   1. Use Chromium's views::FocusRing with custom colors
  //   2. Use a custom focus ring view with Astra-specific styling
  //   3. Use the view's border or outline property
  //
  // For now, we schedule a paint so that views that implement their own
  // focus ring painting can update.
  if (old_focus && old_focus != new_focus) {
    old_focus->SchedulePaint();
  }
  if (new_focus) {
    new_focus->SchedulePaint();
  }
}

void AstraFocusManager::NotifyFocusChanged(views::View* old_focus,
                                           views::View* new_focus) {
  for (AstraFocusManagerObserver& observer : observers_) {
    observer.OnFocusChanged(old_focus, new_focus);
  }
}

}  // namespace accessibility
}  // namespace astra
