// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_PAGE_INFO_ASTRA_PAGE_INFO_MODEL_H_
#define ASTRA_UI_VIEWS_PAGE_INFO_ASTRA_PAGE_INFO_MODEL_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "skia/core/SkColor.h"

namespace astra {

// =========================================================================
// AstraPageInfoModel — data model for the page info bubble
// =========================================================================
//
// This model projects Chromium's page/site information for display in the
// page info bubble (the bubble shown when clicking the lock/icon in the
// address bar).
//
// Chromium subsystems reused:
//   - PageInfoDelegate (chrome/browser/page_info/page_info_delegate.h)
//   - PermissionManager (components/permissions/permission_manager.h)
//   - SSLStatus / certificate info (content/public/browser/ssl_status.h)
//   - CookieSettings (components/content_settings/core/browser/cookie_settings.h)
//
// TODO(astra): Wire to PageInfoDelegate and PermissionManager.
//   Reference: chrome/browser/page_info/page_info.h
//   Patch point: chrome/browser/ui/views/page_info/page_info_bubble_view.h
// =========================================================================

// Security status of the current page.
enum class AstraSecurityStatus {
  kSecure,    // HTTPS with valid certificate
  kInsecure,  // HTTP or invalid certificate
  kWarning,   // Mixed content or other warnings
  kNeutral,   // Internal pages, chrome://, etc.
  kUnknown,   // Status not yet determined
};

// Permission setting values.
enum class AstraPermissionSetting {
  kAllow,
  kBlock,
  kAsk,
};

// Source of the permission setting.
enum class AstraPermissionSource {
  kDefault,     // Global default
  kUser,        // User-set for this site
  kPolicy,      // Enterprise policy
  kExtension,   // Extension override
  kInsecure,    // Insecure origin blocks
};

// Types of permissions shown in page info.
enum class AstraPagePermissionType {
  kCamera,
  kMicrophone,
  kGeolocation,
  kNotifications,
  kClipboard,
  kPopups,
  kImages,
  kJavaScript,
  kSound,
  kFullscreen,
};

// A single permission entry.
struct AstraPagePermissionEntry {
  AstraPagePermissionType type = AstraPagePermissionType::kCamera;
  std::u16string name;
  AstraPermissionSetting setting = AstraPermissionSetting::kAsk;
  bool is_default = true;
  AstraPermissionSource source = AstraPermissionSource::kDefault;
  bool is_managed = false;
  bool show_in_page_info = true;
};

// Site information.
struct AstraSiteInfo {
  std::u16string origin;           // Full origin (e.g. "https://example.com")
  std::u16string display_name;     // Short display name
  std::u16string url;              // Full URL
  AstraSecurityStatus security_status = AstraSecurityStatus::kUnknown;
  std::u16string security_summary; // Short summary like "Connection is secure"
  std::u16string security_details; // Longer description
};

// Cookie / site data information.
struct AstraCookieInfo {
  int cookies_in_use = 0;
  int site_data_count = 0;
  int blocked_cookies = 0;
  bool third_party_cookies_allowed = true;
};

// Connection / certificate information.
struct AstraConnectionInfo {
  std::u16string protocol_version;     // e.g. "TLS 1.3"
  std::u16string cipher_suite;         // e.g. "TLS_AES_128_GCM"
  std::u16string key_exchange;         // e.g. "X25519"
  std::u16string certificate_issuer;   // e.g. "Let's Encrypt"
  std::u16string certificate_subject;  // e.g. "example.com"
  std::u16string valid_from;
  std::u16string valid_until;
  bool certificate_is_valid = false;
};

// Observer for AstraPageInfoModel.
class AstraPageInfoModelObserver : public base::CheckedObserver {
 public:
  // Called when permission settings change.
  virtual void OnPermissionChanged(AstraPageInfoModel* model,
                                   AstraPagePermissionType type) {}

  // Called when cookie/site data info changes.
  virtual void OnCookiesChanged(AstraPageInfoModel* model) {}

  // Called when security status changes.
  virtual void OnSecurityStatusChanged(AstraPageInfoModel* model) {}

  // Called when the model is about to be destroyed.
  virtual void OnPageInfoModelShutdown(AstraPageInfoModel* model) {}

 protected:
  ~AstraPageInfoModelObserver() override = default;
};

class AstraPageInfoModel {
 public:
  AstraPageInfoModel();
  ~AstraPageInfoModel();

  AstraPageInfoModel(const AstraPageInfoModel&) = delete;
  AstraPageInfoModel& operator=(const AstraPageInfoModel&) = delete;

  // -- Observer management --------------------------------------------------

  void AddObserver(AstraPageInfoModelObserver* observer);
  void RemoveObserver(AstraPageInfoModelObserver* observer);

  // -- Site info ------------------------------------------------------------

  const AstraSiteInfo& GetSiteInfo() const;
  void SetSiteInfo(const AstraSiteInfo& info);

  // Get icon color for the current security status.
  static SkColor GetSecurityStatusColor(AstraSecurityStatus status);

  // Get a short label for the security status.
  static std::u16string GetSecurityStatusLabel(AstraSecurityStatus status);

  // Get the icon name for the security status.
  static std::string GetSecurityIconName(AstraSecurityStatus status);

  // -- Permissions ----------------------------------------------------------

  const std::vector<AstraPagePermissionEntry>& GetPermissions() const;

  // Get a specific permission entry by type.
  const AstraPagePermissionEntry* GetPermission(
      AstraPagePermissionType type) const;

  // Toggle a permission between its default state and user-set state.
  void TogglePermission(AstraPagePermissionType type);

  // Set a permission to a specific setting.
  void SetPermission(AstraPagePermissionType type,
                     AstraPermissionSetting setting);

  // Reset a permission to its default value.
  void ResetPermission(AstraPagePermissionType type);

  // Get display name for a permission type.
  static std::u16string GetPermissionName(AstraPagePermissionType type);

  // Get icon name for a permission type.
  static std::string GetPermissionIconName(AstraPagePermissionType type);

  // Get display string for a permission setting.
  static std::u16string GetPermissionSettingLabel(AstraPermissionSetting setting);

  // -- Cookies / site data --------------------------------------------------

  const AstraCookieInfo& GetCookies() const;
  void SetCookies(const AstraCookieInfo& info);

  // -- Connection / certificate --------------------------------------------

  const AstraConnectionInfo& GetConnectionInfo() const;
  void SetConnectionInfo(const AstraConnectionInfo& info);

  // -- Actions --------------------------------------------------------------

  // Open site settings page.
  void OpenSiteSettings();

  // Open cookies dialog.
  void OpenCookiesDialog();

  // Open certificate viewer.
  void OpenCertificateViewer();

 private:
  // Notify helpers.
  void NotifyPermissionChanged(AstraPagePermissionType type);
  void NotifyCookiesChanged();
  void NotifySecurityStatusChanged();

  // Find a non-const permission entry by type.
  AstraPagePermissionEntry* FindPermission(AstraPagePermissionType type);

  // Initialize sample data for testing.
  void InitSampleData();

  // Site information.
  AstraSiteInfo site_info_;

  // Permission entries.
  std::vector<AstraPagePermissionEntry> permissions_;

  // Cookie / site data info.
  AstraCookieInfo cookie_info_;

  // Connection / certificate info.
  AstraConnectionInfo connection_info_;

  // Observers.
  base::ObserverList<AstraPageInfoModelObserver> observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_PAGE_INFO_ASTRA_PAGE_INFO_MODEL_H_
