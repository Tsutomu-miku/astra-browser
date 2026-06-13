// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_DOWNLOADS_PAGE_ASTRA_DOWNLOADS_PAGE_MODEL_H_
#define ASTRA_UI_VIEWS_DOWNLOADS_PAGE_ASTRA_DOWNLOADS_PAGE_MODEL_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "ui/gfx/image/image_skia.h"

namespace astra {

// Download state enumeration for the downloads page.
//
// Mirrors Chromium's download::DownloadItem::DownloadState plus a paused
// state (which is a sub-state of in-progress in Chromium but presented as
// a separate state in the downloads page UI).
//
// Chromium owner: download::DownloadItem::DownloadState
//   (components/download/public/common/download_item.h)
enum class AstraDownloadPageState {
  kInProgress,
  kComplete,
  kCancelled,
  kInterrupted,
  kPaused,
};

// Danger type for a download.
//
// Chromium owner: download::DownloadDangerType
//   (components/download/public/common/download_danger_type.h)
enum class AstraDownloadDangerType {
  kSafe,
  kDangerous,
  kUncommon,
  kDangerousHost,
};

// Filter options for the downloads page.
enum class AstraDownloadsPageFilter {
  kAll,
  kInProgress,
  kCompleted,
  kCancelled,
  kInterrupted,
};

// A single download entry displayed on the downloads page.
//
// This is a presentation projection of download data, combining Chromium
// download state with Astra-specific metadata (workspace, category).
//
// Chromium owner: download::DownloadItem
//   (components/download/public/common/download_item.h)
//
// TODO(astra): Project from real download::DownloadItem once wired to
// DownloadManager.  Patch point: None — uses public DownloadItem API.
struct AstraDownloadEntry {
  std::string id;
  std::u16string file_name;
  std::string url;
  std::string file_path;
  int64_t total_bytes = -1;
  int64_t received_bytes = 0;
  AstraDownloadPageState state = AstraDownloadPageState::kInProgress;
  AstraDownloadDangerType danger_type = AstraDownloadDangerType::kSafe;
  base::Time start_time;
  base::Time end_time;
  std::string mime_type;
  gfx::ImageSkia icon;
  bool is_openable = false;
  int opener_tab_id = -1;
  std::string workspace;
  std::string category;
};

// Observer for AstraDownloadsPageModel.
//
// All methods have empty default implementations so observers only need to
// override the events they care about.
//
// TODO(astra): Add per-download fine-grained observers for performance
// when the page has hundreds of entries.
class AstraDownloadsPageObserver : public base::CheckedObserver {
 public:
  // Called when the full list of downloads changes (add, remove,
  // re-filter, etc.).
  virtual void OnDownloadsChanged() {}

  // Called when a new download is added.
  virtual void OnDownloadAdded(const std::string& id) {}

  // Called when a download is removed.
  virtual void OnDownloadRemoved(const std::string& id) {}

  // Called when a download is updated (progress, state, etc.).
  virtual void OnDownloadUpdated(const std::string& id) {}

  // Called when the search query changes.
  virtual void OnSearchChanged(const std::u16string& query) {}

  // Called when the filter (state or category) changes.
  virtual void OnFilterChanged() {}

  // Called when the model is about to be destroyed.
  virtual void OnDownloadsPageModelShutdown() {}

 protected:
  ~AstraDownloadsPageObserver() override = default;
};

// Model for the full downloads page.
//
// Owns the list of download entries and provides filtering, searching,
// and categorization for the downloads page view.
//
// This is a projection layer — real download data lives in Chromium's
// DownloadManager and is projected into AstraDownloadEntry structs.
// The model also adds Astra-specific metadata (workspace, category).
//
// Chromium owner: DownloadManager / DownloadItem
//   (content/public/browser/download_manager.h)
//   (components/download/public/common/download_item.h)
//
// Truth source: Chromium DownloadManager.
// Astra adds: workspace, category, page-level filtering/search.
//
// TODO(astra): Wire this up to Chromium's DownloadManager via
// DownloadManager::Observer.  Patch point: None — public interface.
// TODO(astra): Make this a ProfileKeyedService for profile-scoped lifetime.
class AstraDownloadsPageModel {
 public:
  AstraDownloadsPageModel();
  ~AstraDownloadsPageModel();

  AstraDownloadsPageModel(const AstraDownloadsPageModel&) = delete;
  AstraDownloadsPageModel& operator=(const AstraDownloadsPageModel&) = delete;

  // -- Observer management --------------------------------------------------

  void AddObserver(AstraDownloadsPageObserver* observer);
  void RemoveObserver(AstraDownloadsPageObserver* observer);

  // -- Download data access -------------------------------------------------

  // Get all downloads sorted by start_time descending (newest first).
  // This returns the filtered view after search + filter + category are
  // applied.
  const std::vector<AstraDownloadEntry>& GetDownloads() const;

  // Get a specific download by ID.  Returns nullptr if not found.
  const AstraDownloadEntry* GetDownload(const std::string& id) const;

  // Get the total count of downloads (all downloads, unfiltered).
  size_t GetCount() const;

  // -- Search ---------------------------------------------------------------

  void SetSearchQuery(const std::u16string& query);
  const std::u16string& GetSearchQuery() const { return search_query_; }

  // -- State filter ---------------------------------------------------------

  void SetFilter(AstraDownloadsPageFilter filter);
  AstraDownloadsPageFilter GetFilter() const { return filter_; }

  // Get available filter options with labels.
  std::vector<std::pair<AstraDownloadsPageFilter, std::u16string>>
  GetFilterOptions() const;

  // -- Category filter ------------------------------------------------------

  void SetCategoryFilter(const std::string& category);
  const std::string& GetCategoryFilter() const { return category_filter_; }

  // Get all unique categories from the current download set.
  std::vector<std::string> GetCategories() const;

  // -- Download actions -----------------------------------------------------
  //
  // These are model-level action stubs.  In production they delegate to
  // Chromium's DownloadManager via AstraDownloadsHelper.

  void RemoveDownload(const std::string& id);
  void ClearAllDownloads();

  void OpenDownload(const std::string& id);
  void ShowInFolder(const std::string& id);

  void PauseDownload(const std::string& id);
  void ResumeDownload(const std::string& id);
  void CancelDownload(const std::string& id);
  void RetryDownload(const std::string& id);

  // -- Sample data ----------------------------------------------------------

  // Populate the model with sample downloads for testing/development.
  // Creates 15+ sample entries in various states and categories.
  void PopulateSampleDownloads();

  // -- Loading state --------------------------------------------------------

  bool IsLoading() const { return loading_; }
  void SetLoading(bool loading);

  // -- Statistics -----------------------------------------------------------

  // Total bytes of all completed downloads.
  int64_t GetTotalDownloadedBytes() const;

 private:
  // -- Filtering helpers ----------------------------------------------------

  bool MatchesSearch(const AstraDownloadEntry& entry) const;
  bool MatchesFilter(const AstraDownloadEntry& entry) const;
  bool MatchesCategory(const AstraDownloadEntry& entry) const;

  // Apply all filters (search + state + category) and rebuild
  // filtered_downloads_.
  void ApplyFilters();

  // Internal helper that returns filtered downloads without modifying state.
  std::vector<AstraDownloadEntry> GetFilteredDownloads() const;

  // -- Notification helpers -------------------------------------------------

  void NotifyDownloadsChanged();
  void NotifyDownloadAdded(const std::string& id);
  void NotifyDownloadRemoved(const std::string& id);
  void NotifyDownloadUpdated(const std::string& id);
  void NotifySearchChanged();
  void NotifyFilterChanged();
  void NotifyShutdown();

  // -- Data -----------------------------------------------------------------

  // All download entries (unfiltered).
  std::vector<AstraDownloadEntry> all_downloads_;

  // Filtered download entries (what the UI displays).
  // Cached result of ApplyFilters().
  std::vector<AstraDownloadEntry> filtered_downloads_;

  // Current search query.
  std::u16string search_query_;

  // Current state filter.
  AstraDownloadsPageFilter filter_ = AstraDownloadsPageFilter::kAll;

  // Current category filter (empty = all categories).
  std::string category_filter_;

  // Loading state.
  bool loading_ = false;

  // Observers.
  base::ObserverList<AstraDownloadsPageObserver> observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_DOWNLOADS_PAGE_ASTRA_DOWNLOADS_PAGE_MODEL_H_
