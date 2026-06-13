// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_HISTORY_PAGE_ASTRA_HISTORY_PAGE_MODEL_H_
#define ASTRA_UI_VIEWS_HISTORY_PAGE_ASTRA_HISTORY_PAGE_MODEL_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "ui/gfx/image/image_skia.h"

namespace astra {

// A single history entry.
struct AstraHistoryEntry {
  std::string id;
  std::u16string title;
  std::string url;
  std::string host;  // e.g. "example.com"
  base::Time visit_time;
  int visit_count = 1;
  gfx::ImageSkia favicon;
  std::string favicon_url;
  bool is_bookmarked = false;
  std::vector<std::string> related_searches;

  // Astra-specific metadata.
  std::string category;  // e.g. "Work", "Social", "News"
  std::string workspace; // Workspace this visit is associated with
};

// A day's worth of history entries.
struct AstraHistoryDay {
  base::Time date;
  std::u16string date_label;  // e.g. "Today", "Yesterday", "Monday, June 9"
  std::vector<AstraHistoryEntry> entries;
  int total_visits = 0;
};

// Filter options for history.
enum class AstraHistoryFilter {
  kAll,
  kToday,
  kYesterday,
  kLast7Days,
  kLast30Days,
  kThisMonth,
};

// Observer for AstraHistoryPageModel.
class AstraHistoryPageObserver : public base::CheckedObserver {
 public:
  // Called when the list of history entries changes.
  virtual void OnHistoryChanged(AstraHistoryPageModel* model) {}

  // Called when the filter changes.
  virtual void OnFilterChanged(AstraHistoryPageModel* model,
                               AstraHistoryFilter filter) {}

  // Called when the search query changes.
  virtual void OnSearchChanged(AstraHistoryPageModel* model,
                               const std::u16string& query) {}

  // Called when a history entry is removed.
  virtual void OnHistoryEntryRemoved(AstraHistoryPageModel* model,
                                     const std::string& id) {}

  // Called when the model is about to be destroyed.
  virtual void OnHistoryPageModelShutdown(AstraHistoryPageModel* model) {}

 protected:
  ~AstraHistoryPageObserver() override = default;
};

// Model for the history page.
//
// Owns the history entries and filtering/search logic.  History data
// comes from Chromium's HistoryService — this model projects and
// augments it with Astra-specific categorization and workspace info.
//
// Chromium owner: HistoryService / BrowsingDataHandler
//   (components/history/core/browser/history_service.h)
//   (chrome/browser/browsing_data/browsing_data_handler.cc)
//
// TODO(astra): Wire up to Chromium's HistoryService via a
// KeyedService wrapper.  Patch point:
// chrome/browser/ui/webui/history/history_ui.cc
// or chrome/browser/history/history_service_factory.cc.
class AstraHistoryPageModel {
 public:
  AstraHistoryPageModel();
  ~AstraHistoryPageModel();

  AstraHistoryPageModel(const AstraHistoryPageModel&) = delete;
  AstraHistoryPageModel& operator=(const AstraHistoryPageModel&) = delete;

  // -- Observer management --------------------------------------------------

  void AddObserver(AstraHistoryPageObserver* observer);
  void RemoveObserver(AstraHistoryPageObserver* observer);

  // -- History data ---------------------------------------------------------

  // Get history grouped by day.
  const std::vector<AstraHistoryDay>& GetDays() const;

  // Get the total number of history entries.
  size_t GetTotalEntryCount() const;

  // Get a specific entry by ID. Returns nullptr if not found.
  const AstraHistoryEntry* GetEntry(const std::string& id) const;

  // -- Filtering ------------------------------------------------------------

  void SetFilter(AstraHistoryFilter filter);
  AstraHistoryFilter GetFilter() const { return filter_; }

  // Get available filter options.
  std::vector<std::pair<AstraHistoryFilter, std::u16string>> GetFilterOptions()
      const;

  // -- Search ---------------------------------------------------------------

  void SetSearchQuery(const std::u16string& query);
  const std::u16string& GetSearchQuery() const { return search_query_; }

  // -- Categories -----------------------------------------------------------

  // Get available history categories (Astra-specific).
  std::vector<std::string> GetCategories() const;

  void SetCategoryFilter(const std::string& category);
  const std::string& GetCategoryFilter() const { return category_filter_; }

  // -- Entry manipulation ---------------------------------------------------

  // Remove a specific history entry.
  void RemoveEntry(const std::string& id);

  // Remove all entries in the current filter range.
  void RemoveAllInRange();

  // Clear all browsing data.
  void ClearAllHistory();

  // -- Sample data ----------------------------------------------------------

  // Populate with sample history for testing/development.
  void PopulateSampleHistory();

  // -- State ----------------------------------------------------------------

  // Whether history is currently loading.
  bool IsLoading() const { return loading_; }
  void SetLoading(bool loading);

 private:
  // Notify observers that history changed.
  void NotifyHistoryChanged();

  // Notify observers that filter changed.
  void NotifyFilterChanged();

  // Notify observers that search changed.
  void NotifySearchChanged();

  // Apply current filter and search to generate the displayed days.
  void ApplyFilters();

  // Generate day grouping from flat entry list.
  std::vector<AstraHistoryDay> GroupEntriesByDay(
      const std::vector<AstraHistoryEntry>& entries) const;

  // Check if an entry matches the current search query.
  bool MatchesSearch(const AstraHistoryEntry& entry) const;

  // Check if an entry matches the current filter.
  bool MatchesFilter(const AstraHistoryEntry& entry) const;

  // Check if an entry matches the category filter.
  bool MatchesCategory(const AstraHistoryEntry& entry) const;

  // All history entries (unfiltered).
  std::vector<AstraHistoryEntry> all_entries_;

  // Filtered and grouped history (what the UI displays).
  std::vector<AstraHistoryDay> filtered_days_;

  // Current filter.
  AstraHistoryFilter filter_ = AstraHistoryFilter::kAll;

  // Current search query.
  std::u16string search_query_;

  // Current category filter (empty = all).
  std::string category_filter_;

  // Loading state.
  bool loading_ = false;

  base::ObserverList<AstraHistoryPageObserver> observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_HISTORY_PAGE_ASTRA_HISTORY_PAGE_MODEL_H_
