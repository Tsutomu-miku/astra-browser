// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/page_actions/astra_page_actions_model.h"

#include <algorithm>

#include "base/check.h"
#include "base/notreached.h"
#include "base/strings/utf_string_conversions.h"

namespace astra {

AstraPageActionsModel::AstraPageActionsModel() = default;

AstraPageActionsModel::~AstraPageActionsModel() {
  for (auto& observer : observers_) {
    observer.OnPageActionsModelShutdown(this);
  }
}

void AstraPageActionsModel::AddObserver(AstraPageActionsObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraPageActionsModel::RemoveObserver(AstraPageActionsObserver* observer) {
  observers_.RemoveObserver(observer);
}

const std::vector<AstraPageActionItem>& AstraPageActionsModel::GetAllActions()
    const {
  return actions_;
}

std::vector<AstraPageActionItem> AstraPageActionsModel::GetPinnedActions()
    const {
  std::vector<AstraPageActionItem> result;
  for (const auto& item : actions_) {
    if (item.visible && item.pinned) {
      result.push_back(item);
    }
  }
  // Apply max_visible_actions budget if set.
  if (max_visible_actions_ > 0 &&
      static_cast<int>(result.size()) > max_visible_actions_) {
    result.resize(max_visible_actions_);
  }
  return result;
}

std::vector<AstraPageActionItem> AstraPageActionsModel::GetOverflowActions()
    const {
  std::vector<AstraPageActionItem> result;
  int pinned_shown = 0;
  for (const auto& item : actions_) {
    if (!item.visible) {
      continue;
    }
    if (item.pinned) {
      if (max_visible_actions_ == 0 || pinned_shown < max_visible_actions_) {
        pinned_shown++;
        continue;  // Shown in main row.
      }
    }
    // Either not pinned, or pinned but overflowing the budget.
    result.push_back(item);
  }
  return result;
}

size_t AstraPageActionsModel::GetVisibleActionCount() const {
  size_t count = 0;
  for (const auto& item : actions_) {
    if (item.visible) {
      count++;
    }
  }
  return count;
}

size_t AstraPageActionsModel::GetPinnedActionCount() const {
  size_t count = 0;
  for (const auto& item : actions_) {
    if (item.visible && item.pinned) {
      count++;
    }
  }
  return count;
}

const AstraPageActionItem* AstraPageActionsModel::GetAction(
    AstraPageActionType type) const {
  int index = FindActionIndex(type);
  if (index < 0) {
    return nullptr;
  }
  return &actions_[index];
}

const AstraPageActionItem* AstraPageActionsModel::GetExtensionAction(
    const std::string& extension_id) const {
  int index = FindExtensionActionIndex(extension_id);
  if (index < 0) {
    return nullptr;
  }
  return &actions_[index];
}

void AstraPageActionsModel::SetAction(const AstraPageActionItem& item) {
  int index = -1;
  if (item.type == AstraPageActionType::kExtensionAction) {
    index = FindExtensionActionIndex(item.extension_id);
  } else {
    index = FindActionIndex(item.type);
  }

  if (index >= 0) {
    actions_[index] = item;
  } else {
    actions_.push_back(item);
  }
  SortActions();
  NotifyActionsChanged();
}

void AstraPageActionsModel::RemoveAction(AstraPageActionType type) {
  auto it = std::remove_if(
      actions_.begin(), actions_.end(),
      [type](const AstraPageActionItem& item) { return item.type == type; });
  if (it != actions_.end()) {
    actions_.erase(it, actions_.end());
    NotifyActionsChanged();
  }
}

void AstraPageActionsModel::RemoveExtensionAction(
    const std::string& extension_id) {
  auto it =
      std::remove_if(actions_.begin(), actions_.end(),
                     [&extension_id](const AstraPageActionItem& item) {
                       return item.type == AstraPageActionType::kExtensionAction &&
                              item.extension_id == extension_id;
                     });
  if (it != actions_.end()) {
    actions_.erase(it, actions_.end());
    NotifyActionsChanged();
  }
}

void AstraPageActionsModel::SetActionState(AstraPageActionType type,
                                           AstraPageActionState state) {
  int index = FindActionIndex(type);
  if (index < 0) {
    return;
  }
  if (actions_[index].state == state) {
    return;
  }
  actions_[index].state = state;
  NotifyActionChanged(type);
}

void AstraPageActionsModel::SetActionBadge(AstraPageActionType type,
                                           const std::u16string& badge_text,
                                           SkColor badge_color) {
  int index = FindActionIndex(type);
  if (index < 0) {
    return;
  }
  actions_[index].badge_text = badge_text;
  actions_[index].badge_color = badge_color;
  NotifyActionChanged(type);
}

void AstraPageActionsModel::SetActionVisible(AstraPageActionType type,
                                             bool visible) {
  int index = FindActionIndex(type);
  if (index < 0) {
    return;
  }
  if (actions_[index].visible == visible) {
    return;
  }
  actions_[index].visible = visible;
  NotifyActionsChanged();
}

void AstraPageActionsModel::SetActionPinned(AstraPageActionType type,
                                            bool pinned) {
  int index = FindActionIndex(type);
  if (index < 0) {
    return;
  }
  if (actions_[index].pinned == pinned) {
    return;
  }
  actions_[index].pinned = pinned;
  NotifyActionsChanged();
}

void AstraPageActionsModel::SetActionOrder(AstraPageActionType type, int order) {
  int index = FindActionIndex(type);
  if (index < 0) {
    return;
  }
  if (actions_[index].order == order) {
    return;
  }
  actions_[index].order = order;
  SortActions();
  NotifyActionsChanged();
}

void AstraPageActionsModel::PopulateDefaultActions() {
  actions_.clear();

  // Standard Chromium page actions (projected).
  actions_.push_back({
      .type = AstraPageActionType::kBookmarkStar,
      .label = u"Bookmark this tab",
      .icon_name = "bookmark",
      .state = AstraPageActionState::kDefault,
      .visible = true,
      .pinned = true,
      .order = 10,
  });

  actions_.push_back({
      .type = AstraPageActionType::kZoom,
      .label = u"Zoom",
      .icon_name = "zoom",
      .state = AstraPageActionState::kDefault,
      .visible = true,
      .pinned = false,
      .order = 20,
  });

  actions_.push_back({
      .type = AstraPageActionType::kTranslate,
      .label = u"Translate",
      .icon_name = "translate",
      .state = AstraPageActionState::kDefault,
      .visible = false,
      .pinned = false,
      .order = 30,
  });

  actions_.push_back({
      .type = AstraPageActionType::kFind,
      .label = u"Find in page",
      .icon_name = "find",
      .state = AstraPageActionState::kDefault,
      .visible = false,
      .pinned = false,
      .order = 40,
  });

  actions_.push_back({
      .type = AstraPageActionType::kPrint,
      .label = u"Print",
      .icon_name = "print",
      .state = AstraPageActionState::kDefault,
      .visible = false,
      .pinned = false,
      .order = 50,
  });

  actions_.push_back({
      .type = AstraPageActionType::kSavePassword,
      .label = u"Save password",
      .icon_name = "password",
      .state = AstraPageActionState::kDefault,
      .visible = false,
      .pinned = false,
      .order = 60,
  });

  actions_.push_back({
      .type = AstraPageActionType::kSendTabToSelf,
      .label = u"Send to your devices",
      .icon_name = "send_tab",
      .state = AstraPageActionState::kDefault,
      .visible = false,
      .pinned = false,
      .order = 70,
  });

  actions_.push_back({
      .type = AstraPageActionType::kSharingHub,
      .label = u"Share",
      .icon_name = "share",
      .state = AstraPageActionState::kDefault,
      .visible = false,
      .pinned = false,
      .order = 80,
  });

  // Astra-specific page actions.
  actions_.push_back({
      .type = AstraPageActionType::kReadingList,
      .label = u"Add to reading list",
      .icon_name = "reading_list",
      .state = AstraPageActionState::kDefault,
      .visible = true,
      .pinned = false,
      .order = 100,
  });

  actions_.push_back({
      .type = AstraPageActionType::kNote,
      .label = u"Add note",
      .icon_name = "note",
      .state = AstraPageActionState::kDefault,
      .visible = true,
      .pinned = false,
      .order = 110,
  });

  actions_.push_back({
      .type = AstraPageActionType::kScreenshot,
      .label = u"Take screenshot",
      .icon_name = "screenshot",
      .state = AstraPageActionState::kDefault,
      .visible = false,
      .pinned = false,
      .order = 120,
  });

  actions_.push_back({
      .type = AstraPageActionType::kFavorite,
      .label = u"Add to favorites",
      .icon_name = "favorite",
      .state = AstraPageActionState::kDefault,
      .visible = false,
      .pinned = false,
      .order = 130,
  });

  actions_.push_back({
      .type = AstraPageActionType::kFocusMode,
      .label = u"Focus mode",
      .icon_name = "focus_mode",
      .state = AstraPageActionState::kDefault,
      .visible = false,
      .pinned = false,
      .order = 140,
  });

  actions_.push_back({
      .type = AstraPageActionType::kSplitView,
      .label = u"Split view",
      .icon_name = "split_view",
      .state = AstraPageActionState::kDefault,
      .visible = false,
      .pinned = false,
      .order = 150,
  });

  actions_.push_back({
      .type = AstraPageActionType::kCommandPalette,
      .label = u"Command palette",
      .icon_name = "command_palette",
      .state = AstraPageActionState::kDefault,
      .visible = false,
      .pinned = false,
      .order = 160,
  });

  actions_.push_back({
      .type = AstraPageActionType::kSidebar,
      .label = u"Sidebar",
      .icon_name = "sidebar",
      .state = AstraPageActionState::kDefault,
      .visible = true,
      .pinned = true,
      .order = 170,
  });

  SortActions();
  NotifyActionsChanged();
}

void AstraPageActionsModel::ClearAllActions() {
  if (actions_.empty()) {
    return;
  }
  actions_.clear();
  NotifyActionsChanged();
}

void AstraPageActionsModel::SetExtensionAction(const std::string& extension_id,
                                               const std::u16string& name,
                                               const std::string& icon_name,
                                               bool pinned) {
  AstraPageActionItem item;
  item.type = AstraPageActionType::kExtensionAction;
  item.extension_id = extension_id;
  item.extension_name = name;
  item.label = name;
  item.icon_name = icon_name;
  item.visible = true;
  item.pinned = pinned;
  item.order = 200;  // Extension actions come after built-in actions.

  int index = FindExtensionActionIndex(extension_id);
  if (index >= 0) {
    actions_[index] = item;
  } else {
    actions_.push_back(item);
  }
  SortActions();
  NotifyActionsChanged();
}

std::vector<AstraPageActionItem> AstraPageActionsModel::GetExtensionActions()
    const {
  std::vector<AstraPageActionItem> result;
  for (const auto& item : actions_) {
    if (item.type == AstraPageActionType::kExtensionAction) {
      result.push_back(item);
    }
  }
  return result;
}

void AstraPageActionsModel::SetMaxVisibleActions(int max) {
  if (max_visible_actions_ == max) {
    return;
  }
  max_visible_actions_ = max;
  NotifyActionsChanged();
}

int AstraPageActionsModel::GetMaxVisibleActions() const {
  return max_visible_actions_;
}

bool AstraPageActionsModel::WouldOverflow(size_t index) const {
  if (max_visible_actions_ <= 0) {
    return false;
  }
  return index >= static_cast<size_t>(max_visible_actions_);
}

void AstraPageActionsModel::SetCompactMode(bool compact) {
  if (compact_mode_ == compact) {
    return;
  }
  compact_mode_ = compact;
  NotifyActionsChanged();
}

bool AstraPageActionsModel::GetCompactMode() const {
  return compact_mode_;
}

void AstraPageActionsModel::NotifyActionsChanged() {
  for (auto& observer : observers_) {
    observer.OnActionsChanged(this);
  }
}

void AstraPageActionsModel::NotifyActionChanged(AstraPageActionType type) {
  for (auto& observer : observers_) {
    observer.OnActionChanged(this, type);
  }
}

void AstraPageActionsModel::SortActions() {
  std::sort(actions_.begin(), actions_.end(),
            [](const AstraPageActionItem& a, const AstraPageActionItem& b) {
              if (a.pinned != b.pinned) {
                return a.pinned;  // Pinned first.
              }
              if (a.order != b.order) {
                return a.order < b.order;
              }
              return a.label < b.label;
            });
}

int AstraPageActionsModel::FindActionIndex(AstraPageActionType type) const {
  for (size_t i = 0; i < actions_.size(); i++) {
    if (actions_[i].type == type &&
        actions_[i].type != AstraPageActionType::kExtensionAction) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

int AstraPageActionsModel::FindExtensionActionIndex(
    const std::string& extension_id) const {
  for (size_t i = 0; i < actions_.size(); i++) {
    if (actions_[i].type == AstraPageActionType::kExtensionAction &&
        actions_[i].extension_id == extension_id) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

}  // namespace astra
