// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_TAB_GROUPS_PAGE_ASTRA_TAB_GROUPS_PAGE_MODEL_H_
#define ASTRA_UI_VIEWS_TAB_GROUPS_PAGE_ASTRA_TAB_GROUPS_PAGE_MODEL_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/image/image_skia.h"

namespace astra {

// Tab group color options.
enum class AstraTabGroupColor {
  kGrey,
  kBlue,
  kRed,
  kYellow,
  kGreen,
  kPink,
  kPurple,
  kCyan,
  kOrange,
};

// Tab group display state.
enum class AstraTabGroupState {
  kExpanded,
  kCollapsed,
  kFrozen,  // Frozen/throttled background tab group
};

// A single tab within a group.
struct AstraTabGroupTab {
  std::string id;
  std::u16string title;
  std::string url;
  gfx::ImageSkia favicon;
  std::string favicon_url;
  bool is_pinned = false;
  bool is_active = false;
  bool is_loading = false;
  base::Time last_active_time;
  int tab_index = 0;
  std::string workspace;
};

// A tab group.
struct AstraTabGroup {
  std::string id;
  std::u16string title;
  AstraTabGroupColor color = AstraTabGroupColor::kGrey;
  AstraTabGroupState state = AstraTabGroupState::kExpanded;
  std::vector<AstraTabGroupTab> tabs;
  base::Time created_time;
  base::Time last_accessed_time;
  std::string workspace;
  std::string category;  // e.g. "Work", "Research", "Personal"
  bool is_pinned = false;
  int tab_count = 0;
  int active_tab_index = -1;

  // Stats.
  int total_tabs = 0;
  int pinned_tabs = 0;
  int unread_tabs = 0;
  size_t memory_estimate_bytes = 0;
};

// A tab group category/folder.
struct AstraTabGroupCategory {
  std::string id;
  std::u16string name;
  std::vector<std::string> group_ids;
  int count = 0;
};

// Sort modes for tab groups.
enum class AstraTabGroupSortType {
  kName,
  kTabCount,
  kLastAccessed,
  kCreatedDate,
  kMemoryUsage,
  kColor,
};

// Filter modes for tab groups.
enum class AstraTabGroupFilter {
  kAll,
  kExpandedOnly,
  kCollapsedOnly,
  kFrozenOnly,
  kPinned,
  kUnreadTabs,
};

// Observer for AstraTabGroupsPageModel.
class AstraTabGroupsPageObserver : public base::CheckedObserver {
 public:
  // Called when the list of groups changes.
  virtual void OnTabGroupsChanged(AstraTabGroupsPageModel* model) {}

  // Called when a single group is added.
  virtual void OnTabGroupAdded(AstraTabGroupsPageModel* model,
                               const std::string& group_id) {}

  // Called when a single group is removed.
  virtual void OnTabGroupRemoved(AstraTabGroupsPageModel* model,
                                 const std::string& group_id) {}

  // Called when a group's state changes (expand/collapse, rename, etc.).
  virtual void OnTabGroupUpdated(AstraTabGroupsPageModel* model,
                                 const std::string& group_id) {}

  // Called when a tab is added to a group.
  virtual void OnTabAddedToGroup(AstraTabGroupsPageModel* model,
                                 const std::string& group_id,
                                 const std::string& tab_id) {}

  // Called when a tab is removed from a group.
  virtual void OnTabRemovedFromGroup(AstraTabGroupsPageModel* model,
                                     const std::string& group_id,
                                     const std::string& tab_id) {}

  // Called when the search query changes.
  virtual void OnSearchChanged(AstraTabGroupsPageModel* model,
                               const std::u16string& query) {}

  // Called when the filter changes.
  virtual void OnFilterChanged(AstraTabGroupsPageModel* model) {}

  // Called when the model is about to be destroyed.
  virtual void OnTabGroupsPageModelShutdown(AstraTabGroupsPageModel* model) {}

 protected:
  ~AstraTabGroupsPageObserver() override = default;
};

// Model for the tab groups management page.
//
// Owns tab groups and filtering/search logic.  Tab group data
// comes from Chromium's TabStripModel and tab_groups::TabGroupId —
// this model projects and augments it with Astra-specific
// categorization, workspace association, and saved groups.
//
// Chromium owner: TabStripModel / TabGroup
//   (chrome/browser/ui/tabs/tab_strip_model.h)
//   (components/tab_groups/core/tab_group_id.h)
//
// TODO(astra): Wire up to Chromium's TabStripModel and tab_groups
// system via a KeyedService wrapper.
// Patch point: chrome/browser/ui/views/tabs/tab_group_header.cc
// or chrome/browser/ui/tabs/tab_group_model.cc.
class AstraTabGroupsPageModel {
 public:
  AstraTabGroupsPageModel();
  ~AstraTabGroupsPageModel();

  AstraTabGroupsPageModel(const AstraTabGroupsPageModel&) = delete;
  AstraTabGroupsPageModel& operator=(const AstraTabGroupsPageModel&) = delete;

  // -- Observer management --------------------------------------------------

  void AddObserver(AstraTabGroupsPageObserver* observer);
  void RemoveObserver(AstraTabGroupsPageObserver* observer);

  // -- Group data -----------------------------------------------------------

  // Get all groups (unfiltered).
  const std::vector<AstraTabGroup>& GetAllGroups() const;

  // Get filtered and sorted groups.
  std::vector<AstraTabGroup> GetFilteredGroups() const;

  // Get a specific group by ID. Returns nullptr if not found.
  const AstraTabGroup* GetGroup(const std::string& group_id) const;

  // Get total group count.
  size_t GetGroupCount() const;

  // Get total tab count across all groups.
  size_t GetTotalTabCount() const;

  // -- Filtering & Sorting --------------------------------------------------

  void SetFilter(AstraTabGroupFilter filter);
  AstraTabGroupFilter GetFilter() const { return filter_; }

  std::vector<std::pair<AstraTabGroupFilter, std::u16string>>
  GetFilterOptions() const;

  void SetSortType(AstraTabGroupSortType sort_type);
  AstraTabGroupSortType GetSortType() const { return sort_type_; }

  std::vector<std::pair<AstraTabGroupSortType, std::u16string>>
  GetSortOptions() const;

  // -- Search ---------------------------------------------------------------

  void SetSearchQuery(const std::u16string& query);
  const std::u16string& GetSearchQuery() const { return search_query_; }

  // -- Categories -----------------------------------------------------------

  std::vector<AstraTabGroupCategory> GetCategories() const;
  void SetCategoryFilter(const std::string& category);
  const std::string& GetCategoryFilter() const { return category_filter_; }

  // -- Group operations -----------------------------------------------------

  // Create a new tab group.
  std::string CreateGroup(const std::u16string& title,
                          AstraTabGroupColor color,
                          const std::string& workspace = std::string());

  // Remove a tab group and all its tabs.
  void RemoveGroup(const std::string& group_id);

  // Rename a tab group.
  void RenameGroup(const std::string& group_id,
                   const std::u16string& new_title);

  // Change group color.
  void SetGroupColor(const std::string& group_id,
                     AstraTabGroupColor color);

  // Toggle group expanded/collapsed state.
  void ToggleGroupCollapsed(const std::string& group_id);

  // Set group expanded state directly.
  void SetGroupExpanded(const std::string& group_id, bool expanded);

  // Toggle group pinned state.
  void ToggleGroupPinned(const std::string& group_id);

  // Freeze/unfreeze a group (throttle background tabs).
  void ToggleGroupFrozen(const std::string& group_id);

  // Move a tab into a group.
  void AddTabToGroup(const std::string& group_id,
                     const AstraTabGroupTab& tab);

  // Remove a tab from a group.
  void RemoveTabFromGroup(const std::string& group_id,
                          const std::string& tab_id);

  // Move a tab between groups.
  void MoveTab(const std::string& source_group_id,
               const std::string& target_group_id,
               const std::string& tab_id,
               int target_index = -1);

  // Move a group within the list.
  void MoveGroup(const std::string& group_id, int target_index);

  // Set group category.
  void SetGroupCategory(const std::string& group_id,
                        const std::string& category);

  // Set group workspace.
  void SetGroupWorkspace(const std::string& group_id,
                         const std::string& workspace);

  // Ungroup all tabs (dissolve the group).
  void Ungroup(const std::string& group_id);

  // Close all tabs in a group.
  void CloseGroupTabs(const std::string& group_id);

  // Mute/unmute all tabs in a group.
  void SetGroupMuted(const std::string& group_id, bool muted);

  // -- Color helpers --------------------------------------------------------

  static SkColor GetGroupColor(AstraTabGroupColor color);
  static std::u16string GetGroupName(AstraTabGroupColor color);
  static std::vector<AstraTabGroupColor> GetAllColors();

  // -- Sample data ----------------------------------------------------------

  // Populate with sample groups for testing/development.
  void PopulateSampleGroups();

  // -- State ----------------------------------------------------------------

  bool IsLoading() const { return loading_; }
  void SetLoading(bool loading);

 private:
  // Notify observers.
  void NotifyGroupsChanged();
  void NotifyGroupAdded(const std::string& group_id);
  void NotifyGroupRemoved(const std::string& group_id);
  void NotifyGroupUpdated(const std::string& group_id);
  void NotifyFilterChanged();
  void NotifySearchChanged();

  // Filter helpers.
  bool MatchesSearch(const AstraTabGroup& group) const;
  bool MatchesFilter(const AstraTabGroup& group) const;
  bool MatchesCategory(const AstraTabGroup& group) const;

  // Apply all filters and sort.
  std::vector<AstraTabGroup> ApplyFiltersAndSort(
      const std::vector<AstraTabGroup>& groups) const;

  // Sort groups.
  void SortGroups(std::vector<AstraTabGroup>& groups) const;

  // Find a non-const group by ID.
  AstraTabGroup* FindGroup(const std::string& group_id);

  // Recalculate group stats (tab counts, etc.).
  void UpdateGroupStats(AstraTabGroup* group);

  // All tab groups.
  std::vector<AstraTabGroup> all_groups_;

  // Current filter.
  AstraTabGroupFilter filter_ = AstraTabGroupFilter::kAll;

  // Current sort type.
  AstraTabGroupSortType sort_type_ = AstraTabGroupSortType::kLastAccessed;

  // Current search query.
  std::u16string search_query_;

  // Current category filter (empty = all).
  std::string category_filter_;

  // Loading state.
  bool loading_ = false;

  // Next group ID counter.
  int next_group_id_ = 1;

  base::ObserverList<AstraTabGroupsPageObserver> observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_TAB_GROUPS_PAGE_ASTRA_TAB_GROUPS_PAGE_MODEL_H_
