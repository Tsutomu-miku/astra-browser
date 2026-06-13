// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/extensions_menu/astra_extensions_menu_model.h"

#include <algorithm>

#include "base/check.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"

namespace astra {

AstraExtensionsMenuModel::AstraExtensionsMenuModel() = default;

AstraExtensionsMenuModel::~AstraExtensionsMenuModel() {
  for (auto& observer : observers_) {
    observer.OnExtensionsMenuModelShutdown(this);
  }
}

void AstraExtensionsMenuModel::AddObserver(
    AstraExtensionsMenuObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraExtensionsMenuModel::RemoveObserver(
    AstraExtensionsMenuObserver* observer) {
  observers_.RemoveObserver(observer);
}

const std::vector<AstraExtensionMenuEntry>&
AstraExtensionsMenuModel::GetAllExtensions() const {
  return extensions_;
}

std::vector<AstraExtensionMenuEntry>
AstraExtensionsMenuModel::GetExtensionsByCategory(
    AstraExtensionCategory category) const {
  std::vector<AstraExtensionMenuEntry> result;
  for (const auto& entry : extensions_) {
    if (entry.category == category) {
      result.push_back(entry);
    }
  }
  return result;
}

size_t AstraExtensionsMenuModel::GetExtensionCount() const {
  return extensions_.size();
}

const AstraExtensionMenuEntry* AstraExtensionsMenuModel::GetExtension(
    const std::string& extension_id) const {
  int index = FindExtensionIndex(extension_id);
  if (index < 0) {
    return nullptr;
  }
  return &extensions_[index];
}

void AstraExtensionsMenuModel::SetExtension(
    const AstraExtensionMenuEntry& entry) {
  int index = FindExtensionIndex(entry.extension_id);
  if (index >= 0) {
    extensions_[index] = entry;
  } else {
    extensions_.push_back(entry);
  }
  NotifyExtensionsChanged();
}

void AstraExtensionsMenuModel::RemoveExtension(
    const std::string& extension_id) {
  int index = FindExtensionIndex(extension_id);
  if (index < 0) {
    return;
  }
  extensions_.erase(extensions_.begin() + index);
  NotifyExtensionsChanged();
}

void AstraExtensionsMenuModel::SetExtensionState(
    const std::string& extension_id,
    AstraExtensionState state) {
  int index = FindExtensionIndex(extension_id);
  if (index < 0) {
    return;
  }
  if (extensions_[index].state == state) {
    return;
  }
  extensions_[index].state = state;
  NotifyExtensionChanged(extension_id);
}

void AstraExtensionsMenuModel::SetExtensionPinned(
    const std::string& extension_id,
    bool pinned) {
  int index = FindExtensionIndex(extension_id);
  if (index < 0) {
    return;
  }
  if (extensions_[index].pinned_to_toolbar == pinned) {
    return;
  }
  extensions_[index].pinned_to_toolbar = pinned;
  NotifyExtensionChanged(extension_id);
}

void AstraExtensionsMenuModel::SetExtensionBadge(
    const std::string& extension_id,
    const std::u16string& badge_text,
    SkColor badge_color) {
  int index = FindExtensionIndex(extension_id);
  if (index < 0) {
    return;
  }
  extensions_[index].badge_text = badge_text;
  extensions_[index].badge_color = badge_color;
  extensions_[index].has_badge = !badge_text.empty();
  NotifyExtensionChanged(extension_id);
}

void AstraExtensionsMenuModel::SetExtensionIcon(
    const std::string& extension_id,
    const gfx::ImageSkia& icon) {
  int index = FindExtensionIndex(extension_id);
  if (index < 0) {
    return;
  }
  extensions_[index].icon = icon;
  NotifyExtensionChanged(extension_id);
}

size_t AstraExtensionsMenuModel::GetPinnedCount() const {
  size_t count = 0;
  for (const auto& entry : extensions_) {
    if (entry.pinned_to_toolbar) {
      count++;
    }
  }
  return count;
}

size_t AstraExtensionsMenuModel::GetActiveCount() const {
  size_t count = 0;
  for (const auto& entry : extensions_) {
    if (entry.category == AstraExtensionCategory::kActive) {
      count++;
    }
  }
  return count;
}

size_t AstraExtensionsMenuModel::GetInactiveCount() const {
  size_t count = 0;
  for (const auto& entry : extensions_) {
    if (entry.category == AstraExtensionCategory::kInactive) {
      count++;
    }
  }
  return count;
}

size_t AstraExtensionsMenuModel::GetBlockedCount() const {
  size_t count = 0;
  for (const auto& entry : extensions_) {
    if (entry.category == AstraExtensionCategory::kBlocked) {
      count++;
    }
  }
  return count;
}

void AstraExtensionsMenuModel::PopulateSampleExtensions() {
  extensions_.clear();

  // Pinned extensions.
  AstraExtensionMenuEntry adblock;
  adblock.extension_id = "adblock_ext_id";
  adblock.name = u"AdBlock";
  adblock.description = u"Block ads and trackers";
  adblock.state = AstraExtensionState::kEnabled;
  adblock.category = AstraExtensionCategory::kPinned;
  adblock.pinned_to_toolbar = true;
  adblock.can_show_in_toolbar = true;
  adblock.has_permission_for_current_page = true;
  adblock.badge_text = u"42";
  adblock.badge_color = SK_ColorRED;
  adblock.has_badge = true;
  extensions_.push_back(adblock);

  AstraExtensionMenuEntry password_ext;
  password_ext.extension_id = "password_ext_id";
  password_ext.name = u"Bitwarden";
  password_ext.description = u"Free password manager";
  password_ext.state = AstraExtensionState::kEnabled;
  password_ext.category = AstraExtensionCategory::kPinned;
  password_ext.pinned_to_toolbar = true;
  password_ext.can_show_in_toolbar = true;
  password_ext.has_permission_for_current_page = true;
  extensions_.push_back(password_ext);

  // Active extensions.
  AstraExtensionMenuEntry translate_ext;
  translate_ext.extension_id = "translate_ext_id";
  translate_ext.name = u"Google Translate";
  translate_ext.description = u"Translate pages between languages";
  translate_ext.state = AstraExtensionState::kEnabled;
  translate_ext.category = AstraExtensionCategory::kActive;
  translate_ext.pinned_to_toolbar = false;
  translate_ext.can_show_in_toolbar = true;
  translate_ext.has_permission_for_current_page = true;
  extensions_.push_back(translate_ext);

  AstraExtensionMenuEntry dark_mode;
  dark_mode.extension_id = "dark_mode_ext_id";
  dark_mode.name = u"Dark Reader";
  dark_mode.description = u"Dark mode for every website";
  dark_mode.state = AstraExtensionState::kEnabled;
  dark_mode.category = AstraExtensionCategory::kActive;
  dark_mode.pinned_to_toolbar = false;
  dark_mode.can_show_in_toolbar = true;
  dark_mode.has_permission_for_current_page = true;
  extensions_.push_back(dark_mode);

  // Inactive extensions.
  AstraExtensionMenuEntry vim_ext;
  vim_ext.extension_id = "vim_ext_id";
  vim_ext.name = u"Vimium";
  vim_ext.description = u"Vim-like keyboard shortcuts";
  vim_ext.state = AstraExtensionState::kEnabled;
  vim_ext.category = AstraExtensionCategory::kInactive;
  vim_ext.pinned_to_toolbar = false;
  vim_ext.can_show_in_toolbar = true;
  vim_ext.has_permission_for_current_page = false;
  extensions_.push_back(vim_ext);

  AstraExtensionMenuEntry screenshot_ext;
  screenshot_ext.extension_id = "screenshot_ext_id";
  screenshot_ext.name = u"FireShot";
  screenshot_ext.description = u"Capture full page screenshots";
  screenshot_ext.state = AstraExtensionState::kDisabled;
  screenshot_ext.category = AstraExtensionCategory::kInactive;
  screenshot_ext.pinned_to_toolbar = false;
  screenshot_ext.can_show_in_toolbar = true;
  screenshot_ext.has_permission_for_current_page = true;
  extensions_.push_back(screenshot_ext);

  // Blocked extension.
  AstraExtensionMenuEntry blocked_ext;
  blocked_ext.extension_id = "blocked_ext_id";
  blocked_ext.name = u"Pop-up Blocker";
  blocked_ext.description = u"Block pop-up windows";
  blocked_ext.state = AstraExtensionState::kBlocked;
  blocked_ext.category = AstraExtensionCategory::kBlocked;
  blocked_ext.pinned_to_toolbar = false;
  blocked_ext.can_show_in_toolbar = true;
  blocked_ext.has_permission_for_current_page = false;
  extensions_.push_back(blocked_ext);

  NotifyExtensionsChanged();
}

void AstraExtensionsMenuModel::ClearAll() {
  if (extensions_.empty()) {
    return;
  }
  extensions_.clear();
  NotifyExtensionsChanged();
}

void AstraExtensionsMenuModel::SetSearchQuery(const std::u16string& query) {
  if (search_query_ == query) {
    return;
  }
  search_query_ = query;
  NotifyExtensionsChanged();
}

const std::u16string& AstraExtensionsMenuModel::GetSearchQuery() const {
  return search_query_;
}

std::vector<AstraExtensionMenuEntry>
AstraExtensionsMenuModel::GetFilteredExtensions() const {
  if (search_query_.empty()) {
    return extensions_;
  }

  std::vector<AstraExtensionMenuEntry> result;
  for (const auto& entry : extensions_) {
    if (MatchesSearch(entry)) {
      result.push_back(entry);
    }
  }
  return result;
}

void AstraExtensionsMenuModel::SetCompactMode(bool compact) {
  if (compact_mode_ == compact) {
    return;
  }
  compact_mode_ = compact;
  NotifyExtensionsChanged();
}

bool AstraExtensionsMenuModel::GetCompactMode() const {
  return compact_mode_;
}

void AstraExtensionsMenuModel::NotifyExtensionsChanged() {
  for (auto& observer : observers_) {
    observer.OnExtensionsChanged(this);
  }
}

void AstraExtensionsMenuModel::NotifyExtensionChanged(
    const std::string& extension_id) {
  for (auto& observer : observers_) {
    observer.OnExtensionChanged(this, extension_id);
  }
}

int AstraExtensionsMenuModel::FindExtensionIndex(
    const std::string& extension_id) const {
  for (size_t i = 0; i < extensions_.size(); i++) {
    if (extensions_[i].extension_id == extension_id) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

bool AstraExtensionsMenuModel::MatchesSearch(
    const AstraExtensionMenuEntry& entry) const {
  if (search_query_.empty()) {
    return true;
  }

  std::u16string query_lower = base::ToLowerASCII(search_query_);
  std::u16string name_lower = base::ToLowerASCII(entry.name);
  std::u16string desc_lower = base::ToLowerASCII(entry.description);

  return name_lower.find(query_lower) != std::u16string::npos ||
         desc_lower.find(query_lower) != std::u16string::npos;
}

}  // namespace astra
