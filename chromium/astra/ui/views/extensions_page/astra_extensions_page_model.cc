// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/extensions_page/astra_extensions_page_model.h"

#include <algorithm>

#include "base/i18n/case_conversion.h"
#include "base/strings/string16_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"

namespace astra {

namespace {

// Returns a deterministic color based on a string hash (for placeholder icons).
SkColor HashColor(const std::string& str) {
  size_t hash = std::hash<std::string>{}(str);
  static const SkColor kColors[] = {
      SkColorSetRGB(0x42, 0x85, 0xF4),  // Blue
      SkColorSetRGB(0xEA, 0x43, 0x35),  // Red
      SkColorSetRGB(0x34, 0xA8, 0x53),  // Green
      SkColorSetRGB(0xFB, 0xBC, 0x04),  // Yellow
      SkColorSetRGB(0x9C, 0x27, 0xB0),  // Purple
      SkColorSetRGB(0xFF, 0x6D, 0x00),  // Orange
      SkColorSetRGB(0x00, 0x96, 0x88),  // Teal
      SkColorSetRGB(0x3F, 0x51, 0xB5),  // Indigo
  };
  return kColors[hash % std::size(kColors)];
}

}  // namespace

AstraExtensionsPageModel::AstraExtensionsPageModel() = default;

AstraExtensionsPageModel::~AstraExtensionsPageModel() {
  for (auto& observer : observers_) {
    observer.OnExtensionsPageModelShutdown(this);
  }
}

void AstraExtensionsPageModel::AddObserver(
    AstraExtensionsPageObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraExtensionsPageModel::RemoveObserver(
    AstraExtensionsPageObserver* observer) {
  observers_.RemoveObserver(observer);
}

const std::vector<AstraExtensionEntry>&
AstraExtensionsPageModel::GetAllExtensions() const {
  return all_extensions_;
}

std::vector<AstraExtensionEntry>
AstraExtensionsPageModel::GetFilteredExtensions() const {
  return ApplyFilters(all_extensions_);
}

const AstraExtensionEntry* AstraExtensionsPageModel::GetExtension(
    const std::string& id) const {
  for (const auto& ext : all_extensions_) {
    if (ext.id == id) {
      return &ext;
    }
  }
  return nullptr;
}

size_t AstraExtensionsPageModel::GetTotalCount() const {
  return all_extensions_.size();
}

size_t AstraExtensionsPageModel::GetEnabledCount() const {
  size_t count = 0;
  for (const auto& ext : all_extensions_) {
    if (ext.is_enabled && ext.state == AstraExtensionState::kEnabled) {
      ++count;
    }
  }
  return count;
}

void AstraExtensionsPageModel::SetFilter(AstraExtensionFilter filter) {
  if (filter_ == filter) {
    return;
  }
  filter_ = filter;
  NotifyFilterChanged();
  NotifyExtensionsChanged();
}

std::vector<std::pair<AstraExtensionFilter, std::u16string>>
AstraExtensionsPageModel::GetFilterOptions() const {
  return {
      {AstraExtensionFilter::kAll, u"All extensions"},
      {AstraExtensionFilter::kEnabled, u"Enabled"},
      {AstraExtensionFilter::kDisabled, u"Disabled"},
      {AstraExtensionFilter::kThemes, u"Themes"},
      {AstraExtensionFilter::kApps, u"Apps"},
      {AstraExtensionFilter::kWithErrors, u"With errors"},
      {AstraExtensionFilter::kPinned, u"Pinned to toolbar"},
      {AstraExtensionFilter::kSidebar, u"In sidebar"},
  };
}

void AstraExtensionsPageModel::SetSearchQuery(const std::u16string& query) {
  if (search_query_ == query) {
    return;
  }
  search_query_ = query;
  NotifySearchChanged();
  NotifyExtensionsChanged();
}

std::vector<AstraExtensionCategory>
AstraExtensionsPageModel::GetCategories() const {
  std::map<std::string, AstraExtensionCategory> cat_map;
  for (const auto& ext : all_extensions_) {
    if (ext.category.empty()) {
      continue;
    }
    auto it = cat_map.find(ext.category);
    if (it == cat_map.end()) {
      AstraExtensionCategory cat;
      cat.id = ext.category;
      cat.name = base::UTF8ToUTF16(ext.category);
      cat.count = 1;
      cat_map[ext.category] = cat;
    } else {
      it->second.count++;
    }
  }
  std::vector<AstraExtensionCategory> result;
  for (const auto& [id, cat] : cat_map) {
    result.push_back(cat);
  }
  std::sort(result.begin(), result.end(),
            [](const AstraExtensionCategory& a,
               const AstraExtensionCategory& b) { return a.name < b.name; });
  return result;
}

void AstraExtensionsPageModel::SetCategoryFilter(const std::string& category) {
  if (category_filter_ == category) {
    return;
  }
  category_filter_ = category;
  NotifyFilterChanged();
  NotifyExtensionsChanged();
}

void AstraExtensionsPageModel::SetSortType(AstraExtensionSortType sort_type) {
  if (sort_type_ == sort_type) {
    return;
  }
  sort_type_ = sort_type;
  NotifyExtensionsChanged();
}

std::vector<std::pair<AstraExtensionSortType, std::u16string>>
AstraExtensionsPageModel::GetSortOptions() const {
  return {
      {AstraExtensionSortType::kName, u"Name"},
      {AstraExtensionSortType::kInstallDate, u"Install date"},
      {AstraExtensionSortType::kRecentUsage, u"Recently used"},
      {AstraExtensionSortType::kPermissionLevel, u"Permission level"},
  };
}

void AstraExtensionsPageModel::SetExtensionEnabled(const std::string& id,
                                                   bool enabled) {
  auto* ext = FindExtension(id);
  if (!ext) {
    return;
  }
  if (ext->is_enabled == enabled) {
    return;
  }
  ext->is_enabled = enabled;
  ext->state = enabled ? AstraExtensionState::kEnabled
                       : AstraExtensionState::kDisabled;
  for (auto& observer : observers_) {
    observer.OnExtensionUpdated(this, id);
  }
  NotifyExtensionsChanged();
}

void AstraExtensionsPageModel::ToggleExtensionEnabled(const std::string& id) {
  const auto* ext = GetExtension(id);
  if (!ext) {
    return;
  }
  SetExtensionEnabled(id, !ext->is_enabled);
}

void AstraExtensionsPageModel::RemoveExtension(const std::string& id) {
  auto it = std::find_if(all_extensions_.begin(), all_extensions_.end(),
                         [&id](const AstraExtensionEntry& e) {
                           return e.id == id;
                         });
  if (it == all_extensions_.end()) {
    return;
  }
  all_extensions_.erase(it);
  for (auto& observer : observers_) {
    observer.OnExtensionRemoved(this, id);
  }
  NotifyExtensionsChanged();
}

void AstraExtensionsPageModel::ToggleExtensionPinned(const std::string& id) {
  auto* ext = FindExtension(id);
  if (!ext) {
    return;
  }
  ext->is_pinned = !ext->is_pinned;
  for (auto& observer : observers_) {
    observer.OnExtensionUpdated(this, id);
  }
  NotifyExtensionsChanged();
}

void AstraExtensionsPageModel::ToggleExtensionInSidebar(const std::string& id) {
  auto* ext = FindExtension(id);
  if (!ext) {
    return;
  }
  ext->is_in_sidebar = !ext->is_in_sidebar;
  for (auto& observer : observers_) {
    observer.OnExtensionUpdated(this, id);
  }
  NotifyExtensionsChanged();
}

void AstraExtensionsPageModel::ToggleExtensionIncognito(const std::string& id) {
  auto* ext = FindExtension(id);
  if (!ext) {
    return;
  }
  ext->allows_in_incognito = !ext->allows_in_incognito;
  for (auto& observer : observers_) {
    observer.OnExtensionUpdated(this, id);
  }
  NotifyExtensionsChanged();
}

void AstraExtensionsPageModel::SetExtensionCategory(
    const std::string& id,
    const std::string& category) {
  auto* ext = FindExtension(id);
  if (!ext || ext->category == category) {
    return;
  }
  ext->category = category;
  for (auto& observer : observers_) {
    observer.OnExtensionUpdated(this, id);
  }
  NotifyExtensionsChanged();
}

void AstraExtensionsPageModel::SetExtensionWorkspace(
    const std::string& id,
    const std::string& workspace) {
  auto* ext = FindExtension(id);
  if (!ext || ext->workspace == workspace) {
    return;
  }
  ext->workspace = workspace;
  for (auto& observer : observers_) {
    observer.OnExtensionUpdated(this, id);
  }
  NotifyExtensionsChanged();
}

void AstraExtensionsPageModel::PopulateSampleExtensions() {
  all_extensions_.clear();

  base::Time now = base::Time::Now();
  base::TimeDelta one_day = base::Days(1);

  struct SampleExt {
    const char* name;
    const char* version;
    const char* description;
    AstraExtensionType type;
    AstraExtensionPermissionLevel perm_level;
    const char* category;
    bool pinned;
    bool in_sidebar;
    int days_ago;
    int usage;
    const char* publisher;
    const char* workspace;
    const char* permissions[5];
  };

  const SampleExt samples[] = {
      // Ad blockers & privacy
      {"uBlock Origin", "1.57.0",
       "Efficient wide-spectrum content blocker with CPU and memory efficiency "
       "as primary features.",
       AstraExtensionType::kExtension, AstraExtensionPermissionLevel::kHigh,
       "Privacy & Security", true, false, 120, 45, "Raymond Hill (gorhill)",
       "", {"Read and change all your data on websites you visit",
            "Display notifications", nullptr, nullptr, nullptr}},

      {"Dark Reader", "4.9.73",
       "Dark mode for every website. Take care of your eyes, use dark theme "
       "for night and daily browsing.",
       AstraExtensionType::kExtension, AstraExtensionPermissionLevel::kMedium,
       "Accessibility", true, true, 90, 30, "Alexander Shutau", "Personal",
       {"Read and change all your data on websites you visit",
        "Manage your downloads", nullptr, nullptr, nullptr}},

      // Productivity
      {"Grammarly", "14.1124.0",
       "Improve your writing with Grammarly's AI-powered writing assistant.",
       AstraExtensionType::kExtension, AstraExtensionPermissionLevel::kMedium,
       "Productivity", true, false, 60, 25, "Grammarly", "Work",
       {"Read and change all your data on websites you visit",
        "Display notifications", nullptr, nullptr, nullptr}},

      {"Notion Web Clipper", "7.1.0",
       "Save web pages, articles, and PDFs to Notion with one click.",
       AstraExtensionType::kExtension, AstraExtensionPermissionLevel::kLow,
       "Productivity", false, true, 45, 12, "Notion Labs", "Work",
       {"Read your browsing history", "Display notifications",
        "Access your data for sites in the notion.so domain",
        nullptr, nullptr}},

      {"Todoist for Chrome", "11.2.0",
       "The world's #1 task manager. Capture and organize tasks the moment "
       "they pop into your head.",
       AstraExtensionType::kExtension, AstraExtensionPermissionLevel::kLow,
       "Productivity", false, false, 30, 8, "Doist", "Personal",
       {"Read your browsing history", "Display notifications",
        nullptr, nullptr, nullptr}},

      // Developer tools
      {"React Developer Tools", "5.0.2",
       "Adds React debugging tools to the Chrome Developer Tools.",
       AstraExtensionType::kExtension, AstraExtensionPermissionLevel::kLow,
       "Developer Tools", false, true, 75, 20, "Meta", "",
       {"Access your data for all websites",
        nullptr, nullptr, nullptr, nullptr}},

      {"Redux DevTools", "3.0.19",
       "Redux DevTools extension for debugging application's state changes.",
       AstraExtensionType::kExtension, AstraExtensionPermissionLevel::kLow,
       "Developer Tools", false, false, 65, 15, "Mihail Diordiev", "",
       {"Access your data for all websites",
        nullptr, nullptr, nullptr, nullptr}},

      {"JSON Viewer", "0.18.1",
       "The most beautiful and customizable JSON/JSONP highlighter.",
       AstraExtensionType::kExtension, AstraExtensionPermissionLevel::kLow,
       "Developer Tools", true, false, 100, 22, "Tulios", "",
       {"Read and change all your data on websites you visit",
        nullptr, nullptr, nullptr, nullptr}},

      {"Wappalyzer", "6.10.65",
       "Identify technology on websites, including CMS, frameworks, "
       "and analytics tools.",
       AstraExtensionType::kExtension, AstraExtensionPermissionLevel::kMedium,
       "Developer Tools", false, false, 40, 5, "Wappalyzer", "",
       {"Read your browsing history",
        "Read and change all your data on websites you visit",
        nullptr, nullptr, nullptr}},

      // Social & communication
      {"Honey", "16.0.2",
       "Automatically find and apply coupon codes when you shop online.",
       AstraExtensionType::kExtension, AstraExtensionPermissionLevel::kMedium,
       "Shopping", false, false, 150, 18, "PayPal", "Personal",
       {"Read and change all your data on websites you visit",
        "Display notifications", "Read your browsing history",
        nullptr, nullptr}},

      {"Rakuten", "7.3.0",
       "Earn Cash Back at over 3,500 stores with just one click.",
       AstraExtensionType::kExtension, AstraExtensionPermissionLevel::kMedium,
       "Shopping", false, false, 180, 10, "Rakuten", "Personal",
       {"Read and change all your data on websites you visit",
        "Display notifications", nullptr, nullptr, nullptr}},

      // Entertainment
      {"Video Speed Controller", "0.7.3",
       "Speed up, slow down, advance and rewind any HTML5 video with quick shortcuts.",
       AstraExtensionType::kExtension, AstraExtensionPermissionLevel::kLow,
       "Entertainment", true, true, 200, 35, "codebicycle", "",
       {"Read and change all your data on websites you visit",
        nullptr, nullptr, nullptr, nullptr}},

      {"Volume Master", "2.4.2",
       "Control your video and audio volume with one click. Up to 600%.",
       AstraExtensionType::kExtension, AstraExtensionPermissionLevel::kLow,
       "Entertainment", false, false, 110, 28, "zcythecoder", "",
       {"Read and change all your data on websites you visit",
        nullptr, nullptr, nullptr, nullptr}},

      // Tab & window management
      {"OneTab", "1.63",
       "Convert your tabs into a list. Save up to 95% memory and reduce tab clutter.",
       AstraExtensionType::kExtension, AstraExtensionPermissionLevel::kMedium,
       "Productivity", false, false, 365, 40, "OneTab", "Work",
       {"Read your browsing history", "Manage your bookmarks",
        "Display notifications", nullptr, nullptr}},

      {"Tab Wrangler", "6.10.0",
       "Automatically closes inactive tabs and makes it easy to get them back.",
       AstraExtensionType::kExtension, AstraExtensionPermissionLevel::kLow,
       "Productivity", false, false, 200, 15, "Tab Wrangler", "Work",
       {"Read your browsing history", "Modify websites you visit",
        nullptr, nullptr, nullptr}},

      // Design
      {"ColorZilla", "3.4.1",
       "Advanced color picker, eyedropper, color analyzer, gradient generator.",
       AstraExtensionType::kExtension, AstraExtensionPermissionLevel::kLow,
       "Design", false, false, 250, 12, "Alex Sirota", "",
       {"Read and change all your data on websites you visit",
        "Display notifications", nullptr, nullptr, nullptr}},

      {"WhatFont", "5.1.4",
       "The easiest way to identify fonts on web pages.",
       AstraExtensionType::kExtension, AstraExtensionPermissionLevel::kLow,
       "Design", false, false, 180, 8, "Chengyin Liu", "",
       {"Read and change all your data on websites you visit",
        nullptr, nullptr, nullptr, nullptr}},

      // Themes
      {"Just Black", "1.0",
       "A simple dark theme for Chrome.",
       AstraExtensionType::kTheme, AstraExtensionPermissionLevel::kLow,
       "Themes", false, false, 300, 0, "The Chrome Team", "",
       {nullptr, nullptr, nullptr, nullptr, nullptr}},

      {"Material Dark", "1.4",
       "A material design inspired dark theme.",
       AstraExtensionType::kTheme, AstraExtensionPermissionLevel::kLow,
       "Themes", false, false, 200, 0, "Lapwat", "",
       {nullptr, nullptr, nullptr, nullptr, nullptr}},

      // Apps
      {"Google Docs Offline", "1.65.1",
       "Edit, create, and view your documents, spreadsheets, and presentations "
       "without an internet connection.",
       AstraExtensionType::kHostedApp, AstraExtensionPermissionLevel::kLow,
       "Productivity", false, false, 365, 20, "Google", "",
       {"Read and change your data on docs.google.com",
        nullptr, nullptr, nullptr, nullptr}},

      {"Gmail", "1.0",
       "Google's email service optimized for your browser.",
       AstraExtensionType::kHostedApp, AstraExtensionPermissionLevel::kLow,
       "Communication", false, false, 400, 10, "Google", "",
       {nullptr, nullptr, nullptr, nullptr, nullptr}},

      // Passwords
      {"Bitwarden", "2024.6.2",
       "A secure and free password manager for all of your devices.",
       AstraExtensionType::kExtension, AstraExtensionPermissionLevel::kHigh,
       "Privacy & Security", true, true, 150, 50, "Bitwarden", "Personal",
       {"Read and change all your data on websites you visit",
        "Display notifications", "Access your data for all websites",
        nullptr, nullptr}},
  };

  for (size_t i = 0; i < std::size(samples); ++i) {
    const auto& s = samples[i];
    AstraExtensionEntry ext;
    ext.id = "ext_" + base::NumberToString(i + 1);
    ext.name = base::UTF8ToUTF16(s.name);
    ext.version = s.version;
    ext.description = base::UTF8ToUTF16(s.description);
    ext.type = s.type;
    ext.permission_level = s.perm_level;
    ext.category = s.category;
    ext.is_pinned = s.pinned;
    ext.is_in_sidebar = s.in_sidebar;
    ext.is_enabled = true;
    ext.state = AstraExtensionState::kEnabled;
    ext.install_time = now - one_day * s.days_ago;
    ext.recent_usage_count = s.usage;
    ext.publisher = s.publisher;
    ext.workspace = s.workspace;
    ext.from_webstore = true;
    ext.allows_incognito = false;
    ext.allows_in_incognito = false;

    // Add permission strings.
    for (int j = 0; j < 5 && s.permissions[j]; ++j) {
      ext.permissions.push_back(base::UTF8ToUTF16(s.permissions[j]));
    }

    // Generate icon placeholder.
    SkColor color = HashColor(s.name);
    SkBitmap bitmap;
    bitmap.allocN32Pixels(32, 32);
    bitmap.eraseColor(color);
    gfx::ImageSkia icon = gfx::ImageSkia::CreateFrom1xBitmap(bitmap);
    ext.icon = icon;

    all_extensions_.push_back(std::move(ext));
  }

  // Mark a few as disabled for sample data.
  if (all_extensions_.size() >= 5) {
    all_extensions_[3].is_enabled = false;
    all_extensions_[3].state = AstraExtensionState::kDisabled;
    all_extensions_[7].is_enabled = false;
    all_extensions_[7].state = AstraExtensionState::kDisabled;
  }

  // Mark one with errors.
  if (all_extensions_.size() >= 10) {
    all_extensions_[9].has_errors = true;
    all_extensions_[9].state = AstraExtensionState::kCorrupted;
  }

  NotifyExtensionsChanged();
}

void AstraExtensionsPageModel::SetLoading(bool loading) {
  loading_ = loading;
}

void AstraExtensionsPageModel::InstallExtensionFromWebstore(
    const std::string& webstore_id) {
  // TODO(astra): Wire to chrome/browser/extensions/extension_install_prompt.h
  // and ExtensionInstallPrompt.
}

void AstraExtensionsPageModel::OpenChromeWebStore() {
  // TODO(astra): Open Chrome Web Store in a new tab.
  // Patch point: chrome/browser/ui/webui/extensions/extensions_ui.cc
}

void AstraExtensionsPageModel::ManageShortcuts() {
  // TODO(astra): Open extension shortcuts management page.
  // Patch point: chrome/browser/extensions/extension_shortcut_commands.cc
}

void AstraExtensionsPageModel::NotifyExtensionsChanged() {
  for (auto& observer : observers_) {
    observer.OnExtensionsChanged(this);
  }
}

void AstraExtensionsPageModel::NotifyFilterChanged() {
  for (auto& observer : observers_) {
    observer.OnFilterChanged(this);
  }
}

void AstraExtensionsPageModel::NotifySearchChanged() {
  for (auto& observer : observers_) {
    observer.OnSearchChanged(this, search_query_);
  }
}

bool AstraExtensionsPageModel::MatchesSearch(
    const AstraExtensionEntry& entry) const {
  if (search_query_.empty()) {
    return true;
  }
  std::u16string lower_query = base::i18n::ToLower(search_query_);
  std::u16string lower_name = base::i18n::ToLower(entry.name);
  std::u16string lower_desc = base::i18n::ToLower(entry.description);
  std::u16string lower_publisher =
      base::i18n::ToLower(base::UTF8ToUTF16(entry.publisher));

  return lower_name.find(lower_query) != std::u16string::npos ||
         lower_desc.find(lower_query) != std::u16string::npos ||
         lower_publisher.find(lower_query) != std::u16string::npos;
}

bool AstraExtensionsPageModel::MatchesFilter(
    const AstraExtensionEntry& entry) const {
  switch (filter_) {
    case AstraExtensionFilter::kAll:
      return true;
    case AstraExtensionFilter::kEnabled:
      return entry.is_enabled && entry.state == AstraExtensionState::kEnabled;
    case AstraExtensionFilter::kDisabled:
      return !entry.is_enabled || entry.state == AstraExtensionState::kDisabled;
    case AstraExtensionFilter::kThemes:
      return entry.type == AstraExtensionType::kTheme;
    case AstraExtensionFilter::kApps:
      return entry.type == AstraExtensionType::kHostedApp ||
             entry.type == AstraExtensionType::kPackagedApp ||
             entry.type == AstraExtensionType::kLegacyPackagedApp;
    case AstraExtensionFilter::kWithErrors:
      return entry.has_errors || entry.state == AstraExtensionState::kCorrupted;
    case AstraExtensionFilter::kPinned:
      return entry.is_pinned;
    case AstraExtensionFilter::kSidebar:
      return entry.is_in_sidebar;
  }
  return true;
}

bool AstraExtensionsPageModel::MatchesCategory(
    const AstraExtensionEntry& entry) const {
  if (category_filter_.empty()) {
    return true;
  }
  return entry.category == category_filter_;
}

std::vector<AstraExtensionEntry> AstraExtensionsPageModel::ApplyFilters(
    const std::vector<AstraExtensionEntry>& entries) const {
  std::vector<AstraExtensionEntry> result;
  for (const auto& ext : entries) {
    if (MatchesSearch(ext) && MatchesFilter(ext) && MatchesCategory(ext)) {
      result.push_back(ext);
    }
  }
  SortExtensions(result);
  return result;
}

void AstraExtensionsPageModel::SortExtensions(
    std::vector<AstraExtensionEntry>& entries) const {
  switch (sort_type_) {
    case AstraExtensionSortType::kName:
      std::sort(entries.begin(), entries.end(),
                [](const AstraExtensionEntry& a,
                   const AstraExtensionEntry& b) { return a.name < b.name; });
      break;
    case AstraExtensionSortType::kInstallDate:
      std::sort(entries.begin(), entries.end(),
                [](const AstraExtensionEntry& a,
                   const AstraExtensionEntry& b) {
                  return a.install_time > b.install_time;
                });
      break;
    case AstraExtensionSortType::kRecentUsage:
      std::sort(entries.begin(), entries.end(),
                [](const AstraExtensionEntry& a,
                   const AstraExtensionEntry& b) {
                  return a.recent_usage_count > b.recent_usage_count;
                });
      break;
    case AstraExtensionSortType::kPermissionLevel:
      std::sort(entries.begin(), entries.end(),
                [](const AstraExtensionEntry& a,
                   const AstraExtensionEntry& b) {
                  return a.permission_level > b.permission_level;
                });
      break;
  }
}

AstraExtensionEntry* AstraExtensionsPageModel::FindExtension(
    const std::string& id) {
  for (auto& ext : all_extensions_) {
    if (ext.id == id) {
      return &ext;
    }
  }
  return nullptr;
}

}  // namespace astra
