// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/page_info/astra_page_info_model.h"

#include <algorithm>

#include "base/strings/utf_string_conversions.h"

namespace astra {

namespace {

// Permission metadata struct.
struct PermissionInfo {
  AstraPagePermissionType type;
  const char* name;
  const char* icon_name;
};

const PermissionInfo kPermissionInfos[] = {
    {AstraPagePermissionType::kCamera, "Camera", "camera"},
    {AstraPagePermissionType::kMicrophone, "Microphone", "mic"},
    {AstraPagePermissionType::kGeolocation, "Location", "location"},
    {AstraPagePermissionType::kNotifications, "Notifications", "bell"},
    {AstraPagePermissionType::kClipboard, "Clipboard", "clipboard"},
    {AstraPagePermissionType::kPopups, "Pop-ups and redirects", "popup"},
    {AstraPagePermissionType::kImages, "Images", "image"},
    {AstraPagePermissionType::kJavaScript, "JavaScript", "js"},
    {AstraPagePermissionType::kSound, "Sound", "sound"},
    {AstraPagePermissionType::kFullscreen, "Fullscreen", "fullscreen"},
};

const PermissionInfo* FindPermissionInfo(AstraPagePermissionType type) {
  for (const auto& info : kPermissionInfos) {
    if (info.type == type) {
      return &info;
    }
  }
  return nullptr;
}

// Security status color mappings.
struct SecurityStatusInfo {
  AstraSecurityStatus status;
  SkColor color;
  const char* label;
  const char* icon_name;
};

const SecurityStatusInfo kSecurityInfos[] = {
    {AstraSecurityStatus::kSecure,
     SkColorSetRGB(0x1E, 0x8E, 0x3E),
     "Connection is secure",
     "lock"},
    {AstraSecurityStatus::kInsecure,
     SkColorSetRGB(0xD9, 0x30, 0x25),
     "Connection is not secure",
     "warning"},
    {AstraSecurityStatus::kWarning,
     SkColorSetRGB(0xEA, 0x86, 0x00),
     "Connection is not fully secure",
     "info"},
    {AstraSecurityStatus::kNeutral,
     SkColorSetRGB(0x5F, 0x63, 0x68),
     "Site information",
     "info"},
    {AstraSecurityStatus::kUnknown,
     SkColorSetRGB(0x5F, 0x63, 0x68),
     "Checking site information...",
     "info"},
};

const SecurityStatusInfo* FindSecurityInfo(AstraSecurityStatus status) {
  for (const auto& info : kSecurityInfos) {
    if (info.status == status) {
      return &info;
    }
  }
  return nullptr;
}

// Permission setting labels.
struct PermissionSettingInfo {
  AstraPermissionSetting setting;
  const char* label;
};

const PermissionSettingInfo kSettingInfos[] = {
    {AstraPermissionSetting::kAllow, "Allow"},
    {AstraPermissionSetting::kBlock, "Block"},
    {AstraPermissionSetting::kAsk, "Ask"},
};

const PermissionSettingInfo* FindSettingInfo(AstraPermissionSetting setting) {
  for (const auto& info : kSettingInfos) {
    if (info.setting == setting) {
      return &info;
    }
  }
  return nullptr;
}

}  // namespace

// ===========================================================================
// Static helpers
// ===========================================================================

SkColor AstraPageInfoModel::GetSecurityStatusColor(AstraSecurityStatus status) {
  const auto* info = FindSecurityInfo(status);
  return info ? info->color : SkColorSetRGB(0x5F, 0x63, 0x68);
}

std::u16string AstraPageInfoModel::GetSecurityStatusLabel(
    AstraSecurityStatus status) {
  const auto* info = FindSecurityInfo(status);
  return info ? base::UTF8ToUTF16(info->label) : std::u16string();
}

std::string AstraPageInfoModel::GetSecurityIconName(AstraSecurityStatus status) {
  const auto* info = FindSecurityInfo(status);
  return info ? info->icon_name : std::string();
}

std::u16string AstraPageInfoModel::GetPermissionName(
    AstraPagePermissionType type) {
  const auto* info = FindPermissionInfo(type);
  return info ? base::UTF8ToUTF16(info->name) : std::u16string();
}

std::string AstraPageInfoModel::GetPermissionIconName(
    AstraPagePermissionType type) {
  const auto* info = FindPermissionInfo(type);
  return info ? info->icon_name : std::string();
}

std::u16string AstraPageInfoModel::GetPermissionSettingLabel(
    AstraPermissionSetting setting) {
  const auto* info = FindSettingInfo(setting);
  return info ? base::UTF8ToUTF16(info->label) : std::u16string();
}

// ===========================================================================
// AstraPageInfoModel
// ===========================================================================

AstraPageInfoModel::AstraPageInfoModel() {
  InitSampleData();
}

AstraPageInfoModel::~AstraPageInfoModel() {
  for (auto& observer : observers_) {
    observer.OnPageInfoModelShutdown(this);
  }
}

void AstraPageInfoModel::AddObserver(AstraPageInfoModelObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraPageInfoModel::RemoveObserver(AstraPageInfoModelObserver* observer) {
  observers_.RemoveObserver(observer);
}

const AstraSiteInfo& AstraPageInfoModel::GetSiteInfo() const {
  return site_info_;
}

void AstraPageInfoModel::SetSiteInfo(const AstraSiteInfo& info) {
  AstraSecurityStatus old_status = site_info_.security_status;
  site_info_ = info;
  if (old_status != info.security_status) {
    NotifySecurityStatusChanged();
  }
}

const std::vector<AstraPagePermissionEntry>&
AstraPageInfoModel::GetPermissions() const {
  return permissions_;
}

const AstraPagePermissionEntry* AstraPageInfoModel::GetPermission(
    AstraPagePermissionType type) const {
  for (const auto& entry : permissions_) {
    if (entry.type == type) {
      return &entry;
    }
  }
  return nullptr;
}

void AstraPageInfoModel::TogglePermission(AstraPagePermissionType type) {
  auto* entry = FindPermission(type);
  if (!entry) {
    return;
  }

  // TODO(astra): Wire to PermissionManager::SetContentSetting.
  // Patch point: components/permissions/permission_manager.cc

  // Toggle: Ask -> Allow -> Block -> Ask
  switch (entry->setting) {
    case AstraPermissionSetting::kAsk:
      entry->setting = AstraPermissionSetting::kAllow;
      break;
    case AstraPermissionSetting::kAllow:
      entry->setting = AstraPermissionSetting::kBlock;
      break;
    case AstraPermissionSetting::kBlock:
      entry->setting = AstraPermissionSetting::kAsk;
      break;
  }
  entry->is_default = false;
  entry->source = AstraPermissionSource::kUser;

  NotifyPermissionChanged(type);
}

void AstraPageInfoModel::SetPermission(AstraPagePermissionType type,
                                       AstraPermissionSetting setting) {
  auto* entry = FindPermission(type);
  if (!entry || entry->setting == setting) {
    return;
  }

  // TODO(astra): Wire to PermissionManager::SetContentSetting.
  // Patch point: components/permissions/permission_manager.cc

  entry->setting = setting;
  entry->is_default = false;
  entry->source = AstraPermissionSource::kUser;

  NotifyPermissionChanged(type);
}

void AstraPageInfoModel::ResetPermission(AstraPagePermissionType type) {
  auto* entry = FindPermission(type);
  if (!entry || entry->is_default) {
    return;
  }

  // TODO(astra): Wire to PermissionManager::ResetPermission.
  // Patch point: components/permissions/permission_manager.cc

  entry->setting = AstraPermissionSetting::kAsk;
  entry->is_default = true;
  entry->source = AstraPermissionSource::kDefault;

  NotifyPermissionChanged(type);
}

const AstraCookieInfo& AstraPageInfoModel::GetCookies() const {
  return cookie_info_;
}

void AstraPageInfoModel::SetCookies(const AstraCookieInfo& info) {
  cookie_info_ = info;
  NotifyCookiesChanged();
}

const AstraConnectionInfo& AstraPageInfoModel::GetConnectionInfo() const {
  return connection_info_;
}

void AstraPageInfoModel::SetConnectionInfo(const AstraConnectionInfo& info) {
  connection_info_ = info;
}

void AstraPageInfoModel::OpenSiteSettings() {
  // TODO(astra): Wire to SiteSettings dialog.
  // Reference: chrome/browser/ui/site_settings/site_settings_helper.cc
  // Patch point: chrome/browser/ui/views/site_settings/site_settings_popup_view.h
}

void AstraPageInfoModel::OpenCookiesDialog() {
  // TODO(astra): Wire to Cookies dialog.
  // Reference: chrome/browser/ui/cookie_dialog_controller.cc
  // Patch point: chrome/browser/ui/views/page_info/page_info_cookies_view.h
}

void AstraPageInfoModel::OpenCertificateViewer() {
  // TODO(astra): Wire to certificate viewer.
  // Reference: chrome/browser/certificate_viewer.cc
  // Patch point: chrome/browser/ui/views/certificate_viewer_dialog_win.cc
}

// ===========================================================================
// Private helpers
// ===========================================================================

void AstraPageInfoModel::NotifyPermissionChanged(AstraPagePermissionType type) {
  for (auto& observer : observers_) {
    observer.OnPermissionChanged(this, type);
  }
}

void AstraPageInfoModel::NotifyCookiesChanged() {
  for (auto& observer : observers_) {
    observer.OnCookiesChanged(this);
  }
}

void AstraPageInfoModel::NotifySecurityStatusChanged() {
  for (auto& observer : observers_) {
    observer.OnSecurityStatusChanged(this);
  }
}

AstraPagePermissionEntry* AstraPageInfoModel::FindPermission(
    AstraPagePermissionType type) {
  for (auto& entry : permissions_) {
    if (entry.type == type) {
      return &entry;
    }
  }
  return nullptr;
}

void AstraPageInfoModel::InitSampleData() {
  // Sample site info.
  site_info_.origin = u"https://example.com";
  site_info_.display_name = u"example.com";
  site_info_.url = u"https://example.com/path/to/page";
  site_info_.security_status = AstraSecurityStatus::kSecure;
  site_info_.security_summary = u"Connection is secure";
  site_info_.security_details =
      u"Your information (for example, passwords or credit card numbers) is "
      u"private when it goes to and from this site.";

  // Sample permissions.
  std::vector<AstraPagePermissionType> default_permissions = {
      AstraPagePermissionType::kCamera,
      AstraPagePermissionType::kMicrophone,
      AstraPagePermissionType::kGeolocation,
      AstraPagePermissionType::kNotifications,
      AstraPagePermissionType::kClipboard,
      AstraPagePermissionType::kPopups,
      AstraPagePermissionType::kImages,
      AstraPagePermissionType::kJavaScript,
      AstraPagePermissionType::kSound,
      AstraPagePermissionType::kFullscreen,
  };

  for (auto type : default_permissions) {
    AstraPagePermissionEntry entry;
    entry.type = type;
    entry.name = GetPermissionName(type);
    entry.setting = AstraPermissionSetting::kAsk;
    entry.is_default = true;
    entry.source = AstraPermissionSource::kDefault;
    entry.is_managed = false;
    entry.show_in_page_info = true;
    permissions_.push_back(std::move(entry));
  }

  // Mark a few as non-default for sample purposes.
  auto* cam = FindPermission(AstraPagePermissionType::kCamera);
  if (cam) {
    cam->setting = AstraPermissionSetting::kAllow;
    cam->is_default = false;
    cam->source = AstraPermissionSource::kUser;
  }

  auto* popups = FindPermission(AstraPagePermissionType::kPopups);
  if (popups) {
    popups->setting = AstraPermissionSetting::kBlock;
    popups->is_default = false;
    popups->source = AstraPermissionSource::kUser;
  }

  // Sample cookie info.
  cookie_info_.cookies_in_use = 12;
  cookie_info_.site_data_count = 5;
  cookie_info_.blocked_cookies = 3;
  cookie_info_.third_party_cookies_allowed = false;

  // Sample connection info.
  connection_info_.protocol_version = u"TLS 1.3";
  connection_info_.cipher_suite = u"TLS_AES_128_GCM_SHA256";
  connection_info_.key_exchange = u"X25519";
  connection_info_.certificate_issuer = u"Let's Encrypt Authority X3";
  connection_info_.certificate_subject = u"example.com";
  connection_info_.valid_from = u"Jan 1, 2025";
  connection_info_.valid_until = u"Mar 31, 2025";
  connection_info_.certificate_is_valid = true;
}

}  // namespace astra
