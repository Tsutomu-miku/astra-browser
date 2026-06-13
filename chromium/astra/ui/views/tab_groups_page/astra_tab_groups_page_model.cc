// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_groups_page/astra_tab_groups_page_model.h"

#include <algorithm>

#include "base/i18n/case_conversion.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkColor.h"
#include "third_party/skia/include/core/SkColor.h"

namespace astra {

namespace {

// Returns a deterministic color based on a string hash (for favicon placeholders).
SkColor HashColor(const std::string& str) {
  size_t hash = std::hash<std::string>{}(str);
  static const SkColor kColors[] = {
      SkColorSetRGB(0x42, 0x85, 0xF4), SkColorSetRGB(0xEA, 0x43, 0x35),
      SkColorSetRGB(0x34, 0xA8, 0x53), SkColorSetRGB(0xFB, 0xBC, 0x04),
      SkColorSetRGB(0x9C, 0x27, 0xB0), SkColorSetRGB(0xFF, 0x6D, 0x00),
      SkColorSetRGB(0x00, 0x96, 0x88), SkColorSetRGB(0x3F, 0x51, 0xB5),
  };
  return kColors[hash % std::size(kColors)];
}

}  // namespace

// ===========================================================================
// Color helpers
// ===========================================================================

SkColor AstraTabGroupsPageModel::GetGroupColor(AstraTabGroupColor color) {
  switch (color) {
    case AstraTabGroupColor::kGrey:
      return SkColorSetRGB(0x9A, 0x9A, 0x9A);
    case AstraTabGroupColor::kBlue:
      return SkColorSetRGB(0x42, 0x85, 0xF4);
    case AstraTabGroupColor::kRed:
      return SkColorSetRGB(0xEA, 0x43, 0x35);
    case AstraTabGroupColor::kYellow:
      return SkColorSetRGB(0xFB, 0xBC, 0x04);
    case AstraTabGroupColor::kGreen:
      return SkColorSetRGB(0x34, 0xA8, 0x53);
    case AstraTabGroupColor::kPink:
      return SkColorSetRGB(0xEC, 0x40, 0x7A);
    case AstraTabGroupColor::kPurple:
      return SkColorSetRGB(0x9C, 0x27, 0xB0);
    case AstraTabGroupColor::kCyan:
      return SkColorSetRGB(0x03, 0x9B, 0xE5);
    case AstraTabGroupColor::kOrange:
      return SkColorSetRGB(0xFF, 0x6D, 0x00);
  }
  return SK_ColorGRAY;
}

std::u16string AstraTabGroupsPageModel::GetGroupName(AstraTabGroupColor color) {
  switch (color) {
    case AstraTabGroupColor::kGrey: return u"Grey";
    case AstraTabGroupColor::kBlue: return u"Blue";
    case AstraTabGroupColor::kRed: return u"Red";
    case AstraTabGroupColor::kYellow: return u"Yellow";
    case AstraTabGroupColor::kGreen: return u"Green";
    case AstraTabGroupColor::kPink: return u"Pink";
    case AstraTabGroupColor::kPurple: return u"Purple";
    case AstraTabGroupColor::kCyan: return u"Cyan";
    case AstraTabGroupColor::kOrange: return u"Orange";
  }
  return std::u16string();
}

std::vector<AstraTabGroupColor> AstraTabGroupsPageModel::GetAllColors() {
  return {
      AstraTabGroupColor::kGrey,   AstraTabGroupColor::kBlue,
      AstraTabGroupColor::kRed,    AstraTabGroupColor::kYellow,
      AstraTabGroupColor::kGreen,  AstraTabGroupColor::kPink,
      AstraTabGroupColor::kPurple, AstraTabGroupColor::kCyan,
      AstraTabGroupColor::kOrange,
  };
}

// ===========================================================================
// AstraTabGroupsPageModel
// ===========================================================================

AstraTabGroupsPageModel::AstraTabGroupsPageModel() = default;

AstraTabGroupsPageModel::~AstraTabGroupsPageModel() {
  for (auto& observer : observers_) {
    observer.OnTabGroupsPageModelShutdown(this);
  }
}

void AstraTabGroupsPageModel::AddObserver(
    AstraTabGroupsPageObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraTabGroupsPageModel::RemoveObserver(
    AstraTabGroupsPageObserver* observer) {
  observers_.RemoveObserver(observer);
}

const std::vector<AstraTabGroup>& AstraTabGroupsPageModel::GetAllGroups()
    const {
  return all_groups_;
}

std::vector<AstraTabGroup> AstraTabGroupsPageModel::GetFilteredGroups() const {
  return ApplyFiltersAndSort(all_groups_);
}

const AstraTabGroup* AstraTabGroupsPageModel::GetGroup(
    const std::string& group_id) const {
  for (const auto& group : all_groups_) {
    if (group.id == group_id) {
      return &group;
    }
  }
  return nullptr;
}

size_t AstraTabGroupsPageModel::GetGroupCount() const {
  return all_groups_.size();
}

size_t AstraTabGroupsPageModel::GetTotalTabCount() const {
  size_t count = 0;
  for (const auto& group : all_groups_) {
    count += group.total_tabs;
  }
  return count;
}

void AstraTabGroupsPageModel::SetFilter(AstraTabGroupFilter filter) {
  if (filter_ == filter) {
    return;
  }
  filter_ = filter;
  NotifyFilterChanged();
  NotifyGroupsChanged();
}

std::vector<std::pair<AstraTabGroupFilter, std::u16string>>
AstraTabGroupsPageModel::GetFilterOptions() const {
  return {
      {AstraTabGroupFilter::kAll, u"All groups"},
      {AstraTabGroupFilter::kExpandedOnly, u"Expanded only"},
      {AstraTabGroupFilter::kCollapsedOnly, u"Collapsed only"},
      {AstraTabGroupFilter::kFrozenOnly, u"Frozen only"},
      {AstraTabGroupFilter::kPinned, u"Pinned"},
      {AstraTabGroupFilter::kUnreadTabs, u"With unread tabs"},
  };
}

void AstraTabGroupsPageModel::SetSortType(AstraTabGroupSortType sort_type) {
  if (sort_type_ == sort_type) {
    return;
  }
  sort_type_ = sort_type;
  NotifyGroupsChanged();
}

std::vector<std::pair<AstraTabGroupSortType, std::u16string>>
AstraTabGroupsPageModel::GetSortOptions() const {
  return {
      {AstraTabGroupSortType::kName, u"Name"},
      {AstraTabGroupSortType::kTabCount, u"Tab count"},
      {AstraTabGroupSortType::kLastAccessed, u"Recently used"},
      {AstraTabGroupSortType::kCreatedDate, u"Created date"},
      {AstraTabGroupSortType::kMemoryUsage, u"Memory usage"},
      {AstraTabGroupSortType::kColor, u"Color"},
  };
}

void AstraTabGroupsPageModel::SetSearchQuery(const std::u16string& query) {
  if (search_query_ == query) {
    return;
  }
  search_query_ = query;
  NotifySearchChanged();
  NotifyGroupsChanged();
}

std::vector<AstraTabGroupCategory>
AstraTabGroupsPageModel::GetCategories() const {
  std::map<std::string, AstraTabGroupCategory> cat_map;
  for (const auto& group : all_groups_) {
    if (group.category.empty()) {
      continue;
    }
    auto it = cat_map.find(group.category);
    if (it == cat_map.end()) {
      AstraTabGroupCategory cat;
      cat.id = group.category;
      cat.name = base::UTF8ToUTF16(group.category);
      cat.group_ids.push_back(group.id);
      cat.count = 1;
      cat_map[group.category] = cat;
    } else {
      it->second.group_ids.push_back(group.id);
      it->second.count++;
    }
  }
  std::vector<AstraTabGroupCategory> result;
  for (const auto& [id, cat] : cat_map) {
    result.push_back(cat);
  }
  std::sort(result.begin(), result.end(),
            [](const AstraTabGroupCategory& a,
               const AstraTabGroupCategory& b) { return a.name < b.name; });
  return result;
}

void AstraTabGroupsPageModel::SetCategoryFilter(const std::string& category) {
  if (category_filter_ == category) {
    return;
  }
  category_filter_ = category;
  NotifyFilterChanged();
  NotifyGroupsChanged();
}

std::string AstraTabGroupsPageModel::CreateGroup(
    const std::u16string& title,
    AstraTabGroupColor color,
    const std::string& workspace) {
  AstraTabGroup group;
  group.id = "grp_" + base::NumberToString(next_group_id_++);
  group.title = title;
  group.color = color;
  group.state = AstraTabGroupState::kExpanded;
  group.workspace = workspace;
  group.created_time = base::Time::Now();
  group.last_accessed_time = base::Time::Now();
  group.tab_count = 0;
  group.total_tabs = 0;

  all_groups_.push_back(std::move(group));

  const std::string& new_id = all_groups_.back().id;
  for (auto& observer : observers_) {
    observer.OnTabGroupAdded(this, new_id);
  }
  NotifyGroupsChanged();
  return new_id;
}

void AstraTabGroupsPageModel::RemoveGroup(const std::string& group_id) {
  auto it = std::find_if(all_groups_.begin(), all_groups_.end(),
                         [&group_id](const AstraTabGroup& g) {
                           return g.id == group_id;
                         });
  if (it == all_groups_.end()) {
    return;
  }

  all_groups_.erase(it);

  for (auto& observer : observers_) {
    observer.OnTabGroupRemoved(this, group_id);
  }
  NotifyGroupsChanged();
}

void AstraTabGroupsPageModel::RenameGroup(const std::string& group_id,
                                          const std::u16string& new_title) {
  auto* group = FindGroup(group_id);
  if (!group || group->title == new_title) {
    return;
  }
  group->title = new_title;
  for (auto& observer : observers_) {
    observer.OnTabGroupUpdated(this, group_id);
  }
  NotifyGroupsChanged();
}

void AstraTabGroupsPageModel::SetGroupColor(const std::string& group_id,
                                            AstraTabGroupColor color) {
  auto* group = FindGroup(group_id);
  if (!group || group->color == color) {
    return;
  }
  group->color = color;
  for (auto& observer : observers_) {
    observer.OnTabGroupUpdated(this, group_id);
  }
  NotifyGroupsChanged();
}

void AstraTabGroupsPageModel::ToggleGroupCollapsed(
    const std::string& group_id) {
  auto* group = FindGroup(group_id);
  if (!group) {
    return;
  }
  if (group->state == AstraTabGroupState::kExpanded) {
    group->state = AstraTabGroupState::kCollapsed;
  } else if (group->state == AstraTabGroupState::kCollapsed) {
    group->state = AstraTabGroupState::kExpanded;
  }
  // Don't toggle frozen state — that's separate.

  for (auto& observer : observers_) {
    observer.OnTabGroupUpdated(this, group_id);
  }
  NotifyGroupsChanged();
}

void AstraTabGroupsPageModel::SetGroupExpanded(const std::string& group_id,
                                               bool expanded) {
  auto* group = FindGroup(group_id);
  if (!group) {
    return;
  }
  if (expanded && group->state == AstraTabGroupState::kExpanded) return;
  if (!expanded && group->state == AstraTabGroupState::kCollapsed) return;

  group->state = expanded ? AstraTabGroupState::kExpanded
                          : AstraTabGroupState::kCollapsed;

  for (auto& observer : observers_) {
    observer.OnTabGroupUpdated(this, group_id);
  }
  NotifyGroupsChanged();
}

void AstraTabGroupsPageModel::ToggleGroupPinned(const std::string& group_id) {
  auto* group = FindGroup(group_id);
  if (!group) {
    return;
  }
  group->is_pinned = !group->is_pinned;
  for (auto& observer : observers_) {
    observer.OnTabGroupUpdated(this, group_id);
  }
  NotifyGroupsChanged();
}

void AstraTabGroupsPageModel::ToggleGroupFrozen(const std::string& group_id) {
  auto* group = FindGroup(group_id);
  if (!group) {
    return;
  }
  if (group->state == AstraTabGroupState::kFrozen) {
    group->state = AstraTabGroupState::kExpanded;
  } else {
    group->state = AstraTabGroupState::kFrozen;
  }
  for (auto& observer : observers_) {
    observer.OnTabGroupUpdated(this, group_id);
  }
  NotifyGroupsChanged();
}

void AstraTabGroupsPageModel::AddTabToGroup(const std::string& group_id,
                                            const AstraTabGroupTab& tab) {
  auto* group = FindGroup(group_id);
  if (!group) {
    return;
  }
  group->tabs.push_back(tab);
  UpdateGroupStats(group);

  for (auto& observer : observers_) {
    observer.OnTabAddedToGroup(this, group_id, tab.id);
  }
  for (auto& observer : observers_) {
    observer.OnTabGroupUpdated(this, group_id);
  }
  NotifyGroupsChanged();
}

void AstraTabGroupsPageModel::RemoveTabFromGroup(
    const std::string& group_id,
    const std::string& tab_id) {
  auto* group = FindGroup(group_id);
  if (!group) {
    return;
  }
  auto tab_it = std::find_if(group->tabs.begin(), group->tabs.end(),
                             [&tab_id](const AstraTabGroupTab& t) {
                               return t.id == tab_id;
                             });
  if (tab_it == group->tabs.end()) {
    return;
  }
  group->tabs.erase(tab_it);
  UpdateGroupStats(group);

  for (auto& observer : observers_) {
    observer.OnTabRemovedFromGroup(this, group_id, tab_id);
  }
  for (auto& observer : observers_) {
    observer.OnTabGroupUpdated(this, group_id);
  }
  NotifyGroupsChanged();
}

void AstraTabGroupsPageModel::MoveTab(const std::string& source_group_id,
                                      const std::string& target_group_id,
                                      const std::string& tab_id,
                                      int target_index) {
  if (source_group_id == target_group_id) {
    // Reorder within same group.
    auto* group = FindGroup(source_group_id);
    if (!group) return;
    auto it = std::find_if(group->tabs.begin(), group->tabs.end(),
                           [&tab_id](const AstraTabGroupTab& t) {
                             return t.id == tab_id;
                           });
    if (it == group->tabs.end()) return;
    AstraTabGroupTab tab = *it;
    group->tabs.erase(it);
    if (target_index < 0 || target_index > static_cast<int>(group->tabs.size())) {
      target_index = static_cast<int>(group->tabs.size());
    }
    group->tabs.insert(group->tabs.begin() + target_index, std::move(tab));
    NotifyGroupsChanged();
    return;
  }

  auto* source = FindGroup(source_group_id);
  auto* target = FindGroup(target_group_id);
  if (!source || !target) return;

  auto it = std::find_if(source->tabs.begin(), source->tabs.end(),
                         [&tab_id](const AstraTabGroupTab& t) {
                           return t.id == tab_id;
                         });
  if (it == source->tabs.end()) return;

  AstraTabGroupTab tab = *it;
  source->tabs.erase(it);
  UpdateGroupStats(source);

  if (target_index < 0 || target_index > static_cast<int>(target->tabs.size())) {
    target->tabs.push_back(std::move(tab));
  } else {
    target->tabs.insert(target->tabs.begin() + target_index, std::move(tab));
  }
  UpdateGroupStats(target);

  for (auto& observer : observers_) {
    observer.OnTabRemovedFromGroup(this, source_group_id, tab_id);
  }
  for (auto& observer : observers_) {
    observer.OnTabAddedToGroup(this, target_group_id, tab_id);
  }
  NotifyGroupsChanged();
}

void AstraTabGroupsPageModel::MoveGroup(const std::string& group_id,
                                        int target_index) {
  auto it = std::find_if(all_groups_.begin(), all_groups_.end(),
                         [&group_id](const AstraTabGroup& g) {
                           return g.id == group_id;
                         });
  if (it == all_groups_.end()) return;

  if (target_index < 0 || target_index >= static_cast<int>(all_groups_.size())) {
    target_index = static_cast<int>(all_groups_.size()) - 1;
  }

  std::rotate(all_groups_.begin() + target_index,
              it, it + 1);

  NotifyGroupsChanged();
}

void AstraTabGroupsPageModel::SetGroupCategory(const std::string& group_id,
                                               const std::string& category) {
  auto* group = FindGroup(group_id);
  if (!group || group->category == category) {
    return;
  }
  group->category = category;
  for (auto& observer : observers_) {
    observer.OnTabGroupUpdated(this, group_id);
  }
  NotifyGroupsChanged();
}

void AstraTabGroupsPageModel::SetGroupWorkspace(const std::string& group_id,
                                                const std::string& workspace) {
  auto* group = FindGroup(group_id);
  if (!group || group->workspace == workspace) {
    return;
  }
  group->workspace = workspace;
  for (auto& observer : observers_) {
    observer.OnTabGroupUpdated(this, group_id);
  }
  NotifyGroupsChanged();
}

void AstraTabGroupsPageModel::Ungroup(const std::string& group_id) {
  auto* group = FindGroup(group_id);
  if (!group) {
    return;
  }
  // Ungroup = remove all tabs from the group (they become regular tabs).
  // In our model, we just clear the tabs.
  group->tabs.clear();
  UpdateGroupStats(group);

  for (auto& observer : observers_) {
    observer.OnTabGroupUpdated(this, group_id);
  }
  NotifyGroupsChanged();
}

void AstraTabGroupsPageModel::CloseGroupTabs(const std::string& group_id) {
  auto* group = FindGroup(group_id);
  if (!group) {
    return;
  }
  group->tabs.clear();
  UpdateGroupStats(group);
  for (auto& observer : observers_) {
    observer.OnTabGroupUpdated(this, group_id);
  }
  NotifyGroupsChanged();
}

void AstraTabGroupsPageModel::SetGroupMuted(const std::string& group_id,
                                            bool muted) {
  // TODO(astra): Mute all tabs in a group via TabStripModel.
  // Patch point: chrome/browser/ui/tabs/tab_strip_model.h
}

void AstraTabGroupsPageModel::PopulateSampleGroups() {
  all_groups_.clear();
  next_group_id_ = 1;

  base::Time now = base::Time::Now();
  base::TimeDelta one_hour = base::Hours(1);
  base::TimeDelta one_day = base::Days(1);

  struct SampleGroup {
    const char* title;
    AstraTabGroupColor color;
    const char* category;
    const char* workspace;
    bool is_pinned;
    AstraTabGroupState state;
    int days_ago_created;
    int hours_ago_accessed;
    struct TabDef {
      const char* title;
      const char* url;
      bool is_pinned;
      bool is_active;
      int hours_ago_active;
    };
    std::vector<TabDef> tabs;
  };

  std::vector<SampleGroup> samples = {
      // Work group
      {"Work", AstraTabGroupColor::kBlue, "Work", "Work", true,
       AstraTabGroupState::kExpanded, 14, 1,
       {
           {"Gmail - Inbox", "https://mail.google.com", false, false, 2},
           {"Google Calendar", "https://calendar.google.com", false, false, 3},
           {"Jira - Project Board", "https://jira.example.com/board", false, true, 0},
           {"Confluence - Docs", "https://wiki.example.com", false, false, 5},
           {"GitHub - Repo", "https://github.com/org/repo", true, false, 12},
           {"Slack - Team", "https://company.slack.com", false, false, 1},
           {"Figma - Design Files", "https://figma.com/files", false, false, 8},
       }},

      // Research group
      {"Research", AstraTabGroupColor::kGreen, "Research", "Personal", false,
       AstraTabGroupState::kExpanded, 7, 6,
       {
           {"Wikipedia - Quantum Computing",
            "https://en.wikipedia.org/wiki/Quantum_computing", false, false, 4},
           {"arXiv - Recent Papers", "https://arxiv.org/list/cs/recent", false, false,
            10},
           {"Google Scholar - Search results",
            "https://scholar.google.com/scholar?q=ai", false, false, 12},
           {"Stack Overflow - Answer",
            "https://stackoverflow.com/questions/12345", false, false, 6},
       }},

      // Shopping group
      {"Shopping", AstraTabGroupColor::kPink, "Personal", "Personal", false,
       AstraTabGroupState::kCollapsed, 5, 24,
       {
           {"Amazon - Wishlist", "https://amazon.com/wishlist", false, false, 20},
           {"Best Buy - Deals", "https://bestbuy.com/deals", false, false, 36},
           {"Etsy - Handmade", "https://etsy.com", false, false, 48},
       }},

      // Social media group
      {"Social", AstraTabGroupColor::kRed, "Personal", "Personal", false,
       AstraTabGroupState::kCollapsed, 10, 5,
       {
           {"Twitter / X", "https://twitter.com/home", false, false, 3},
           {"Reddit - Frontpage", "https://reddit.com", false, false, 2},
           {"LinkedIn - Feed", "https://linkedin.com/feed", false, false, 6},
           {"YouTube - Watch later", "https://youtube.com/playlist?list=WL", false,
            false, 8},
           {"Instagram", "https://instagram.com", false, false, 4},
       }},

      // Design resources group
      {"Design Resources", AstraTabGroupColor::kPurple, "Work", "Work", false,
       AstraTabGroupState::kExpanded, 3, 20,
       {
           {"Dribbble", "https://dribbble.com", false, false, 15},
           {"Behance", "https://behance.net", false, false, 20},
           {"Unsplash - Photos", "https://unsplash.com", false, false, 18},
           {"Color Hunt", "https://colorhunt.co", false, false, 25},
           {"Typewolf", "https://typewolf.com", false, false, 30},
       }},

      // Learning / courses group
      {"Learning", AstraTabGroupColor::kYellow, "Personal", "Personal", false,
       AstraTabGroupState::kFrozen, 21, 72,
       {
           {"Coursera - Course", "https://coursera.org/learn/course-name", false,
            false, 48},
           {"Khan Academy", "https://khanacademy.org", false, false, 96},
           {"FreeCodeCamp", "https://freecodecamp.org/learn", false, false, 60},
       }},

      // News group
      {"News", AstraTabGroupColor::kOrange, "Personal", "", false,
       AstraTabGroupState::kExpanded, 2, 2,
       {
           {"The New York Times", "https://nytimes.com", false, false, 1},
           {"The Guardian", "https://theguardian.com", false, false, 3},
           {"Reuters", "https://reuters.com", false, false, 4},
           {"BBC News", "https://bbc.com/news", false, false, 5},
           {"TechCrunch", "https://techcrunch.com", false, true, 0},
       }},

      // Entertainment group
      {"Entertainment", AstraTabGroupColor::kCyan, "Personal", "Personal",
       false, AstraTabGroupState::kCollapsed, 8, 48,
       {
           {"Netflix - Browse", "https://netflix.com/browse", false, false, 36},
           {"Spotify - Playlist", "https://open.spotify.com/playlist/123", false,
            false, 24},
           {"Twitch - Following", "https://twitch.tv/directory/following", false,
            false, 60},
       }},

      // Developer tools group
      {"Dev Tools", AstraTabGroupColor::kGrey, "Work", "Work", true,
       AstraTabGroupState::kExpanded, 30, 4,
       {
           {"Chrome DevTools - Docs", "https://developer.chrome.com/docs/devtools/",
            true, false, 10},
           {"MDN Web Docs", "https://developer.mozilla.org", true, false, 8},
           {"Stack Overflow", "https://stackoverflow.com", false, false, 2},
           {"GitHub - Dashboard", "https://github.com", false, false, 1},
           {"npm", "https://npmjs.com", false, false, 12},
           {"caniuse.com", "https://caniuse.com", false, false, 15},
           {"Bundlephobia", "https://bundlephobia.com", false, false, 20},
           {"Regex101", "https://regex101.com", false, false, 30},
       }},

      // Travel planning group
      {"Trip Planning", AstraTabGroupColor::kGreen, "Personal", "", false,
       AstraTabGroupState::kCollapsed, 1, 36,
       {
           {"Google Maps", "https://google.com/maps", false, false, 24},
           {"Booking.com - Hotels", "https://booking.com", false, false, 30},
           {"Skyscanner - Flights", "https://skyscanner.com", false, false, 28},
           {"TripAdvisor - Reviews", "https://tripadvisor.com", false, false, 32},
           {"Airbnb", "https://airbnb.com", false, false, 26},
       }},
  };

  for (const auto& sample : samples) {
    std::string id = CreateGroup(base::UTF8ToUTF16(sample.title), sample.color,
                                 sample.workspace);
    auto* group = FindGroup(id);
    if (!group) continue;

    group->category = sample.category;
    group->is_pinned = sample.is_pinned;
    group->state = sample.state;
    group->created_time = now - one_day * sample.days_ago_created;
    group->last_accessed_time = now - one_hour * sample.hours_ago_accessed;

    for (size_t i = 0; i < sample.tabs.size(); ++i) {
      const auto& t = sample.tabs[i];
      AstraTabGroupTab tab;
      tab.id = "tab_" + id + "_" + base::NumberToString(i);
      tab.title = base::UTF8ToUTF16(t.title);
      tab.url = t.url;
      tab.is_pinned = t.is_pinned;
      tab.is_active = t.is_active;
      tab.last_active_time = now - one_hour * t.hours_ago_active;
      tab.tab_index = static_cast<int>(i);
      tab.workspace = sample.workspace;

      // Generate favicon placeholder.
      SkColor color = HashColor(t.url);
      SkBitmap bitmap;
      bitmap.allocN32Pixels(16, 16);
      bitmap.eraseColor(color);
      tab.favicon = gfx::ImageSkia::CreateFrom1xBitmap(bitmap);

      group->tabs.push_back(std::move(tab));
    }

    UpdateGroupStats(group);
    group->memory_estimate_bytes = group->total_tabs * 50 * 1024 * 1024;  // ~50MB per tab
  }

  NotifyGroupsChanged();
}

void AstraTabGroupsPageModel::SetLoading(bool loading) {
  loading_ = loading;
}

// ===========================================================================
// Private helpers
// ===========================================================================

void AstraTabGroupsPageModel::NotifyGroupsChanged() {
  for (auto& observer : observers_) {
    observer.OnTabGroupsChanged(this);
  }
}

void AstraTabGroupsPageModel::NotifyGroupAdded(const std::string& group_id) {
  for (auto& observer : observers_) {
    observer.OnTabGroupAdded(this, group_id);
  }
}

void AstraTabGroupsPageModel::NotifyGroupRemoved(const std::string& group_id) {
  for (auto& observer : observers_) {
    observer.OnTabGroupRemoved(this, group_id);
  }
}

void AstraTabGroupsPageModel::NotifyGroupUpdated(const std::string& group_id) {
  for (auto& observer : observers_) {
    observer.OnTabGroupUpdated(this, group_id);
  }
}

void AstraTabGroupsPageModel::NotifyFilterChanged() {
  for (auto& observer : observers_) {
    observer.OnFilterChanged(this);
  }
}

void AstraTabGroupsPageModel::NotifySearchChanged() {
  for (auto& observer : observers_) {
    observer.OnSearchChanged(this, search_query_);
  }
}

bool AstraTabGroupsPageModel::MatchesSearch(
    const AstraTabGroup& group) const {
  if (search_query_.empty()) {
    return true;
  }
  std::u16string lower_query = base::i18n::ToLower(search_query_);
  std::u16string lower_title = base::i18n::ToLower(group.title);

  if (lower_title.find(lower_query) != std::u16string::npos) {
    return true;
  }

  // Also search within tab titles.
  for (const auto& tab : group.tabs) {
    std::u16string lower_tab_title = base::i18n::ToLower(tab.title);
    std::string lower_url = base::ToLowerASCII(tab.url);
    if (lower_tab_title.find(lower_query) != std::u16string::npos) {
      return true;
    }
    if (lower_url.find(base::UTF16ToUTF8(lower_query)) != std::string::npos) {
      return true;
    }
  }

  return false;
}

bool AstraTabGroupsPageModel::MatchesFilter(
    const AstraTabGroup& group) const {
  switch (filter_) {
    case AstraTabGroupFilter::kAll:
      return true;
    case AstraTabGroupFilter::kExpandedOnly:
      return group.state == AstraTabGroupState::kExpanded;
    case AstraTabGroupFilter::kCollapsedOnly:
      return group.state == AstraTabGroupState::kCollapsed;
    case AstraTabGroupFilter::kFrozenOnly:
      return group.state == AstraTabGroupState::kFrozen;
    case AstraTabGroupFilter::kPinned:
      return group.is_pinned;
    case AstraTabGroupFilter::kUnreadTabs:
      return group.unread_tabs > 0;
  }
  return true;
}

bool AstraTabGroupsPageModel::MatchesCategory(
    const AstraTabGroup& group) const {
  if (category_filter_.empty()) {
    return true;
  }
  return group.category == category_filter_;
}

std::vector<AstraTabGroup>
AstraTabGroupsPageModel::ApplyFiltersAndSort(
    const std::vector<AstraTabGroup>& groups) const {
  std::vector<AstraTabGroup> result;
  for (const auto& group : groups) {
    if (MatchesSearch(group) && MatchesFilter(group) &&
        MatchesCategory(group)) {
      result.push_back(group);
    }
  }
  SortGroups(result);
  return result;
}

void AstraTabGroupsPageModel::SortGroups(
    std::vector<AstraTabGroup>& groups) const {
  switch (sort_type_) {
    case AstraTabGroupSortType::kName:
      std::sort(groups.begin(), groups.end(),
                [](const AstraTabGroup& a, const AstraTabGroup& b) {
                  return a.title < b.title;
                });
      break;
    case AstraTabGroupSortType::kTabCount:
      std::sort(groups.begin(), groups.end(),
                [](const AstraTabGroup& a, const AstraTabGroup& b) {
                  return a.total_tabs > b.total_tabs;
                });
      break;
    case AstraTabGroupSortType::kLastAccessed:
      std::sort(groups.begin(), groups.end(),
                [](const AstraTabGroup& a, const AstraTabGroup& b) {
                  return a.last_accessed_time > b.last_accessed_time;
                });
      break;
    case AstraTabGroupSortType::kCreatedDate:
      std::sort(groups.begin(), groups.end(),
                [](const AstraTabGroup& a, const AstraTabGroup& b) {
                  return a.created_time > b.created_time;
                });
      break;
    case AstraTabGroupSortType::kMemoryUsage:
      std::sort(groups.begin(), groups.end(),
                [](const AstraTabGroup& a, const AstraTabGroup& b) {
                  return a.memory_estimate_bytes > b.memory_estimate_bytes;
                });
      break;
    case AstraTabGroupSortType::kColor:
      std::sort(groups.begin(), groups.end(),
                [](const AstraTabGroup& a, const AstraTabGroup& b) {
                  return static_cast<int>(a.color) < static_cast<int>(b.color);
                });
      break;
  }
}

AstraTabGroup* AstraTabGroupsPageModel::FindGroup(
    const std::string& group_id) {
  for (auto& group : all_groups_) {
    if (group.id == group_id) {
      return &group;
    }
  }
  return nullptr;
}

void AstraTabGroupsPageModel::UpdateGroupStats(AstraTabGroup* group) {
  if (!group) return;

  group->total_tabs = static_cast<int>(group->tabs.size());
  group->tab_count = group->total_tabs;

  int pinned = 0;
  int unread = 0;
  for (const auto& tab : group->tabs) {
    if (tab.is_pinned) pinned++;
    // For scaffold, consider tabs with last_active > 24h as "unread".
    // Actually let's just use a counter based on position for sample purposes.
  }
  group->pinned_tabs = pinned;

  // Update unread count: tabs not accessed in over 24 hours.
  base::Time now = base::Time::Now();
  for (const auto& tab : group->tabs) {
    if (now - tab.last_active_time > base::Hours(24)) {
      unread++;
    }
  }
  group->unread_tabs = unread;

  // Active tab index.
  group->active_tab_index = -1;
  for (size_t i = 0; i < group->tabs.size(); ++i) {
    if (group->tabs[i].is_active) {
      group->active_tab_index = static_cast<int>(i);
      break;
    }
  }
}

}  // namespace astra
