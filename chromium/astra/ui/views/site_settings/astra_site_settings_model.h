// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_SITE_SETTINGS_ASTRA_SITE_SETTINGS_MODEL_H_
#define ASTRA_UI_VIEWS_SITE_SETTINGS_ASTRA_SITE_SETTINGS_MODEL_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"

namespace astra {

// =========================================================================
// Astra site settings — per-site permission management
// =========================================================================
//
// AstraSiteSettingsModel manages the list of sites and their permission
// settings.  This is the "All sites" and "Site permissions" page in
// Chrome settings.
//
// Chromium subsystems reused:
//   - HostContentSettingsMap (truth source for site permissions)
//   - PermissionManager (permission state management)
//   - BrowsingDataModel (site data and quota)
//
// Astra owns:
//   - Presentation state (search query, active category, sort order)
//   - Projection of Chromium permission state into a displayable list
//   - Site metadata (favicon, last visit, storage usage)
//
// TODO(astra): Wire model to HostContentSettingsMap for real permission data.
//   Chromium owner: components/content_settings/core/browser/host_content_settings_map.h
//   Patch point: chrome/browser/ui/webui/settings/site_settings_handler.h
// =========================================================================

// Permission types that can be configured per-site.
enum class AstraSitePermissionType {
  kCamera,
  kMicrophone,
  kGeolocation,
  kNotifications,
  kJavaScript,
  kImages,
  kSound,
  kPopups,
  kClipboardRead,
  kClipboardWrite,
  kFullscreen,
  kPointerLock,
  kMidi,
  kMidiSysex,
  kBluetooth,
  kUsb,
  kSerial,
  kHid,
  kNfc,
  kSensors,
  kIdleDetection,
  kPaymentHandler,
  kBackgroundSync,
  kStorageAccess,
  kScreenCapture,
  kWakeLock,
  kPictureInPicture,
  kAutoPlay,
  kDownloads,
  kMultipleDownloads,
  kFileSystemRead,
  kFileSystemWrite,
  kAds,
  kInsecureContent,
  kCookies,
  kWebXR,
};

// Permission setting values.
enum class AstraContentSetting {
  kAllow,       // Allow for all sites / this site
  kBlock,       // Block for all sites / this site
  kAsk,         // Ask every time (default for most permissions)
  kSessionOnly, // Allow only for this session
  kDetectImportantContent, // Special value for some permissions
};

// Category for grouping permissions.
enum class AstraSiteSettingsCategory {
  kAll,           // All sites
  kRecent,        // Recently visited sites
  kAllPermissions, // All permission types (permissions view)
  kPermissions,   // Permission categories view
  kCookies,       // Cookies and site data
  kLocation,      // Location
  kCamera,        // Camera
  kMicrophone,    // Microphone
  kNotifications, // Notifications
  kJavaScript,    // JavaScript
  kImages,        // Images
  kPopups,        // Popups and redirects
  kSound,         // Sound
  kAds,           // Ads
  kBackgroundSync, // Background sync
  kAutomaticDownloads, // Automatic downloads
  kUnsandboxedPlugins,  // Unsandboxed plugin access
  kHandlers,      // Handlers
  kMidi,          // MIDI devices
  kSerialPorts,   // Serial ports
  kUsbDevices,    // USB devices
  kBluetoothDevices, // Bluetooth devices
  kHidDevices,    // HID devices
  kNfcDevices,    // NFC devices
  kZoomLevels,    // Zoom levels
  kPdfDocuments,  // PDF documents
  kProtectedContent, // Protected content IDs
  kInsecureContent, // Insecure content
  kAdditionalPermissions, // Additional permissions
};

// Sort options for sites list.
enum class AstraSiteSettingsSort {
  kName,          // Alphabetical by name
  kStorage,       // By storage used (descending)
  kMostVisited,   // By visit count (descending)
  kLastVisited,   // By last visit time (descending)
  kPermissionCount, // By number of allowed permissions
};

// Filter for sites list.
enum class AstraSiteSettingsFilter {
  kAll,
  kAllowed,       // Sites with at least one allowed permission
  kBlocked,       // Sites with at least one blocked permission
  kWithData,      // Sites with stored data
};

// A single permission entry for a site.
struct AstraSitePermission {
  AstraSitePermissionType type = AstraSitePermissionType::kNotifications;
  AstraContentSetting setting = AstraContentSetting::kAsk;
  bool is_default = true;     // Whether this is the default setting
  bool is_important = false;  // Whether this is considered an "important" permission
  base::Time last_used;       // Last time this permission was used by the site
};

// A single site entry with its permissions.
struct AstraSiteSettingsEntry {
  std::string id;
  std::string origin;           // e.g. "https://example.com"
  std::u16string display_name;  // e.g. "example.com"
  std::u16string title;         // Page title of most recent visit
  std::string favicon_url;

  // Storage / data usage.
  int64_t storage_bytes = 0;
  int64_t cookies_count = 0;
  int64_t cache_bytes = 0;

  // Usage stats.
  int visit_count = 0;
  base::Time last_visit;
  base::Time first_visit;

  // Permissions set for this site.
  std::vector<AstraSitePermission> permissions;

  // Whether site is bookmarked.
  bool is_bookmarked = false;

  // Is this an incognito site entry.
  bool is_incognito = false;
};

// Site settings group — for grouping sites by category (e.g. "Recently visited").
struct AstraSiteSettingsGroup {
  std::string id;
  std::u16string name;
  std::vector<AstraSiteSettingsEntry> sites;
};

// Observer for site settings model.
class AstraSiteSettingsObserver : public base::CheckedObserver {
 public:
  // Called when the list of sites changes.
  virtual void OnSitesChanged(AstraSiteSettingsModel* model) {}

  // Called when the active category changes.
  virtual void OnCategoryChanged(AstraSiteSettingsModel* model,
                                 AstraSiteSettingsCategory category) {}

  // Called when search query changes.
  virtual void OnSearchQueryChanged(AstraSiteSettingsModel* model,
                                    const std::string& query) {}

  // Called when a site's permission changes.
  virtual void OnSitePermissionChanged(AstraSiteSettingsModel* model,
                                       const std::string& site_id,
                                       AstraSitePermissionType type,
                                       AstraContentSetting setting) {}

  // Called when the model is about to be destroyed.
  virtual void OnSiteSettingsModelShutdown(
      AstraSiteSettingsModel* model) {}

 protected:
  ~AstraSiteSettingsObserver() override = default;
};

// Model for site settings page.
class AstraSiteSettingsModel {
 public:
  AstraSiteSettingsModel();
  ~AstraSiteSettingsModel();

  AstraSiteSettingsModel(const AstraSiteSettingsModel&) = delete;
  AstraSiteSettingsModel& operator=(const AstraSiteSettingsModel&) = delete;

  // -- Observer management --------------------------------------------------

  void AddObserver(AstraSiteSettingsObserver* observer);
  void RemoveObserver(AstraSiteSettingsObserver* observer);

  // -- Site management ------------------------------------------------------

  // Get all sites (full list).
  const std::vector<AstraSiteSettingsEntry>& GetAllSites() const;

  // Get filtered and sorted list of sites.
  std::vector<AstraSiteSettingsEntry> GetFilteredSites() const;

  // Get a site by ID.
  const AstraSiteSettingsEntry* GetSite(const std::string& site_id) const;

  // Get site count.
  size_t GetSiteCount() const { return sites_.size(); }

  // Get grouped sites (for "All sites" view with sections like "Recently visited").
  std::vector<AstraSiteSettingsGroup> GetGroupedSites() const;

  // Remove a site (clears data and resets permissions).
  void RemoveSite(const std::string& site_id);

  // Clear all data for a site.
  void ClearSiteData(const std::string& site_id);

  // Reset all permissions for a site to default.
  void ResetSitePermissions(const std::string& site_id);

  // -- Permission management ------------------------------------------------

  // Set a specific permission for a site.
  void SetSitePermission(const std::string& site_id,
                         AstraSitePermissionType type,
                         AstraContentSetting setting);

  // Get default setting for a permission type.
  AstraContentSetting GetDefaultPermission(AstraSitePermissionType type) const;

  // Set default setting for a permission type.
  void SetDefaultPermission(AstraSitePermissionType type,
                            AstraContentSetting setting);

  // Get display name for a permission type.
  static std::u16string GetPermissionName(AstraSitePermissionType type);

  // Get description for a permission type.
  static std::u16string GetPermissionDescription(AstraSitePermissionType type);

  // Get icon name for a permission type.
  static std::string GetPermissionIconName(AstraSitePermissionType type);

  // -- Category / filter / sort ---------------------------------------------

  void SetCategory(AstraSiteSettingsCategory category);
  AstraSiteSettingsCategory GetCategory() const { return category_; }

  void SetFilter(AstraSiteSettingsFilter filter);
  AstraSiteSettingsFilter GetFilter() const { return filter_; }

  void SetSort(AstraSiteSettingsSort sort);
  AstraSiteSettingsSort GetSort() const { return sort_; }

  void SetSearchQuery(const std::string& query);
  const std::string& GetSearchQuery() const { return search_query_; }

  // Get categories list for sidebar.
  static std::vector<std::pair<AstraSiteSettingsCategory, std::u16string>>
      GetCategories();

  // -- Populate / data ------------------------------------------------------

  // Populate with sample sites for demo/testing.
  void PopulateSampleSites();

  // -- State ----------------------------------------------------------------

  bool IsLoading() const { return loading_; }
  void SetLoading(bool loading);

 private:
  // Notify helpers.
  void NotifySitesChanged();
  void NotifyCategoryChanged();
  void NotifySearchQueryChanged();
  void NotifySitePermissionChanged(const std::string& site_id,
                                   AstraSitePermissionType type,
                                   AstraContentSetting setting);

  // Find a site by ID (non-const).
  AstraSiteSettingsEntry* FindSite(const std::string& site_id);

  // Filter helpers.
  bool MatchesFilter(const AstraSiteSettingsEntry& site) const;
  bool MatchesCategory(const AstraSiteSettingsEntry& site) const;
  bool MatchesSearch(const AstraSiteSettingsEntry& site) const;

  // Sort helpers.
  static bool CompareSites(const AstraSiteSettingsEntry& a,
                           const AstraSiteSettingsEntry& b,
                           AstraSiteSettingsSort sort);

  // All sites.
  std::vector<AstraSiteSettingsEntry> sites_;

  // Default permission settings.
  std::vector<std::pair<AstraSitePermissionType, AstraContentSetting>>
      default_permissions_;

  // UI state.
  AstraSiteSettingsCategory category_ = AstraSiteSettingsCategory::kAll;
  AstraSiteSettingsFilter filter_ = AstraSiteSettingsFilter::kAll;
  AstraSiteSettingsSort sort_ = AstraSiteSettingsSort::kLastVisited;
  std::string search_query_;
  bool loading_ = false;

  // Next site ID counter.
  int next_site_id_ = 1;

  base::ObserverList<AstraSiteSettingsObserver> observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SITE_SETTINGS_ASTRA_SITE_SETTINGS_MODEL_H_
