// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_EXTENSIONS_PAGE_ASTRA_EXTENSIONS_PAGE_MODEL_H_
#define ASTRA_UI_VIEWS_EXTENSIONS_PAGE_ASTRA_EXTENSIONS_PAGE_MODEL_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "ui/gfx/image/image_skia.h"

namespace astra {

// Extension install state.
enum class AstraExtensionState {
  kEnabled,
  kDisabled,
  kCorrupted,
  kTerminated,
};

// Extension type categories.
enum class AstraExtensionType {
  kExtension,
  kTheme,
  kHostedApp,
  kPackagedApp,
  kLegacyPackagedApp,
};

// Extension permission warning level.
enum class AstraExtensionPermissionLevel {
  kLow,
  kMedium,
  kHigh,
};

// A single installed extension.
struct AstraExtensionEntry {
  std::string id;
  std::u16string name;
  std::string version;
  std::u16string description;
  gfx::ImageSkia icon;
  std::string icon_url;
  AstraExtensionState state = AstraExtensionState::kEnabled;
  AstraExtensionType type = AstraExtensionType::kExtension;
  AstraExtensionPermissionLevel permission_level =
      AstraExtensionPermissionLevel::kLow;

  bool is_enabled = true;
  bool allows_incognito = false;
  bool allows_in_incognito = false;
  bool has_errors = false;
  bool is_chrome_app = false;
  bool is_bookmark_app = false;
  bool from_webstore = true;

  base::Time install_time;
  std::string publisher;
  std::string publisher_url;
  std::string homepage_url;
  std::string support_url;

  // Astra-specific metadata.
  std::string workspace;
  std::string category;  // e.g. "Productivity", "Developer Tools", "Social"
  std::string folder;    // Extension folder/group
  bool is_pinned = false;
  bool is_in_sidebar = false;

  // Permission warnings (short human-readable strings).
  std::vector<std::u16string> permissions;

  // Number of times the extension has been used recently.
  int recent_usage_count = 0;
};

// A category/grouping of extensions.
struct AstraExtensionCategory {
  std::string id;
  std::u16string name;
  int count = 0;
};

// Observer for AstraExtensionsPageModel.
class AstraExtensionsPageObserver : public base::CheckedObserver {
 public:
  // Called when the list of extensions changes.
  virtual void OnExtensionsChanged(AstraExtensionsPageModel* model) {}

  // Called when a single extension is added.
  virtual void OnExtensionAdded(AstraExtensionsPageModel* model,
                                const std::string& id) {}

  // Called when a single extension is removed.
  virtual void OnExtensionRemoved(AstraExtensionsPageModel* model,
                                  const std::string& id) {}

  // Called when a single extension's state changes.
  virtual void OnExtensionUpdated(AstraExtensionsPageModel* model,
                                  const std::string& id) {}

  // Called when the search query changes.
  virtual void OnSearchChanged(AstraExtensionsPageModel* model,
                               const std::u16string& query) {}

  // Called when the filter changes.
  virtual void OnFilterChanged(AstraExtensionsPageModel* model) {}

  // Called when the model is about to be destroyed.
  virtual void OnExtensionsPageModelShutdown(AstraExtensionsPageModel* model) {}

 protected:
  ~AstraExtensionsPageObserver() override = default;
};

// Filter types for extensions.
enum class AstraExtensionFilter {
  kAll,
  kEnabled,
  kDisabled,
  kThemes,
  kApps,
  kWithErrors,
  kPinned,
  kSidebar,
};

// Sort types for extensions.
enum class AstraExtensionSortType {
  kName,
  kInstallDate,
  kRecentUsage,
  kPermissionLevel,
};

// Model for the extensions management page.
//
// Owns extension entries and filtering/search logic.  Extension data
// comes from Chromium's ExtensionService and ExtensionRegistry — this
// model projects and augments it with Astra-specific categorization,
// sidebar pinning, and workspace association.
//
// Chromium owner: ExtensionService / ExtensionRegistry
//   (extensions/browser/extension_service.h)
//   (extensions/browser/extension_registry.h)
//
// TODO(astra): Wire up to Chromium's ExtensionService via a
// KeyedService wrapper.  Patch point:
// chrome/browser/extensions/extension_service.cc
// or chrome/browser/ui/webui/extensions/extensions_ui.cc.
class AstraExtensionsPageModel {
 public:
  AstraExtensionsPageModel();
  ~AstraExtensionsPageModel();

  AstraExtensionsPageModel(const AstraExtensionsPageModel&) = delete;
  AstraExtensionsPageModel& operator=(const AstraExtensionsPageModel&) = delete;

  // -- Observer management --------------------------------------------------

  void AddObserver(AstraExtensionsPageObserver* observer);
  void RemoveObserver(AstraExtensionsPageObserver* observer);

  // -- Extension data -------------------------------------------------------

  // Get all extensions (unfiltered).
  const std::vector<AstraExtensionEntry>& GetAllExtensions() const;

  // Get filtered extensions (after search + filter + category).
  std::vector<AstraExtensionEntry> GetFilteredExtensions() const;

  // Get a specific extension by ID. Returns nullptr if not found.
  const AstraExtensionEntry* GetExtension(const std::string& id) const;

  // Get total count of extensions.
  size_t GetTotalCount() const;

  // Get count of enabled extensions.
  size_t GetEnabledCount() const;

  // -- Filtering ------------------------------------------------------------

  void SetFilter(AstraExtensionFilter filter);
  AstraExtensionFilter GetFilter() const { return filter_; }

  // Get available filter options with display names.
  std::vector<std::pair<AstraExtensionFilter, std::u16string>>
  GetFilterOptions() const;

  // -- Search ---------------------------------------------------------------

  void SetSearchQuery(const std::u16string& query);
  const std::u16string& GetSearchQuery() const { return search_query_; }

  // -- Categories -----------------------------------------------------------

  // Get available extension categories.
  std::vector<AstraExtensionCategory> GetCategories() const;

  void SetCategoryFilter(const std::string& category);
  const std::string& GetCategoryFilter() const { return category_filter_; }

  // -- Sorting --------------------------------------------------------------

  void SetSortType(AstraExtensionSortType sort_type);
  AstraExtensionSortType GetSortType() const { return sort_type_; }

  std::vector<std::pair<AstraExtensionSortType, std::u16string>>
  GetSortOptions() const;

  // -- Extension manipulation -----------------------------------------------

  // Enable/disable an extension.
  void SetExtensionEnabled(const std::string& id, bool enabled);

  // Toggle extension enabled state.
  void ToggleExtensionEnabled(const std::string& id);

  // Uninstall/remove an extension.
  void RemoveExtension(const std::string& id);

  // Toggle pin to toolbar.
  void ToggleExtensionPinned(const std::string& id);

  // Toggle show in sidebar.
  void ToggleExtensionInSidebar(const std::string& id);

  // Toggle allow in incognito.
  void ToggleExtensionIncognito(const std::string& id);

  // Move extension to a category/folder.
  void SetExtensionCategory(const std::string& id,
                            const std::string& category);

  // Set extension workspace association.
  void SetExtensionWorkspace(const std::string& id,
                             const std::string& workspace);

  // -- Sample data ----------------------------------------------------------

  // Populate with sample extensions for testing/development.
  void PopulateSampleExtensions();

  // -- State ----------------------------------------------------------------

  bool IsLoading() const { return loading_; }
  void SetLoading(bool loading);

  // Install from web store (stub).
  void InstallExtensionFromWebstore(const std::string& webstore_id);

  // Open Chrome Web Store (stub).
  void OpenChromeWebStore();

  // Manage shortcuts (stub).
  void ManageShortcuts();

 private:
  // Notify observers that extensions changed.
  void NotifyExtensionsChanged();

  // Notify observers that filter changed.
  void NotifyFilterChanged();

  // Notify observers that search changed.
  void NotifySearchChanged();

  // Check if an extension matches the current search query.
  bool MatchesSearch(const AstraExtensionEntry& entry) const;

  // Check if an extension matches the current filter.
  bool MatchesFilter(const AstraExtensionEntry& entry) const;

  // Check if an extension matches the category filter.
  bool MatchesCategory(const AstraExtensionEntry& entry) const;

  // Apply filters and sort to get the displayed extensions.
  std::vector<AstraExtensionEntry> ApplyFilters(
      const std::vector<AstraExtensionEntry>& entries) const;

  // Sort a list of extensions according to current sort type.
  void SortExtensions(std::vector<AstraExtensionEntry>& entries) const;

  // Find a non-const extension by ID.
  AstraExtensionEntry* FindExtension(const std::string& id);

  // All installed extensions (unfiltered).
  std::vector<AstraExtensionEntry> all_extensions_;

  // Current filter.
  AstraExtensionFilter filter_ = AstraExtensionFilter::kAll;

  // Current search query.
  std::u16string search_query_;

  // Current category filter (empty = all).
  std::string category_filter_;

  // Current sort type.
  AstraExtensionSortType sort_type_ = AstraExtensionSortType::kName;

  // Loading state.
  bool loading_ = false;

  base::ObserverList<AstraExtensionsPageObserver> observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_EXTENSIONS_PAGE_ASTRA_EXTENSIONS_PAGE_MODEL_H_
