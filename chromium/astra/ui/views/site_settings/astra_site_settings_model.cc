// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/site_settings/astra_site_settings_model.h"

#include <algorithm>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"

namespace astra {

namespace {

// Permission metadata.
struct PermissionInfo {
  AstraSitePermissionType type;
  const char* name;
  const char* description;
  const char* icon_name;
  bool important;
  AstraContentSetting default_setting;
};

const PermissionInfo kPermissionInfos[] = {
    {AstraSitePermissionType::kCamera, "Camera",
     "Use your camera", "camera", true, AstraContentSetting::kAsk},
    {AstraSitePermissionType::kMicrophone, "Microphone",
     "Use your microphone", "mic", true, AstraContentSetting::kAsk},
    {AstraSitePermissionType::kGeolocation, "Location",
     "Know your location", "location", true, AstraContentSetting::kAsk},
    {AstraSitePermissionType::kNotifications, "Notifications",
     "Send notifications", "bell", true, AstraContentSetting::kAsk},
    {AstraSitePermissionType::kJavaScript, "JavaScript",
     "Run JavaScript", "js", false, AstraContentSetting::kAllow},
    {AstraSitePermissionType::kImages, "Images",
     "Show images", "image", false, AstraContentSetting::kAllow},
    {AstraSitePermissionType::kSound, "Sound",
     "Play sound", "sound", false, AstraContentSetting::kAllow},
    {AstraSitePermissionType::kPopups, "Pop-ups and redirects",
     "Allow pop-ups and redirects", "popup", false, AstraContentSetting::kBlock},
    {AstraSitePermissionType::kClipboardRead, "Clipboard read",
     "See text and images copied to the clipboard", "clipboard", true,
     AstraContentSetting::kAsk},
    {AstraSitePermissionType::kClipboardWrite, "Clipboard write",
     "Modify text and images copied to the clipboard", "clipboard", false,
     AstraContentSetting::kAllow},
    {AstraSitePermissionType::kFullscreen, "Fullscreen",
     "Open in fullscreen mode", "fullscreen", false,
     AstraContentSetting::kAsk},
    {AstraSitePermissionType::kPointerLock, "Pointer lock",
     "Lock your mouse pointer", "pointer_lock", false,
     AstraContentSetting::kAsk},
    {AstraSitePermissionType::kMidi, "MIDI devices",
     "Use MIDI devices", "midi", false, AstraContentSetting::kAsk},
    {AstraSitePermissionType::kBluetooth, "Bluetooth devices",
     "Connect to Bluetooth devices", "bluetooth", true,
     AstraContentSetting::kAsk},
    {AstraSitePermissionType::kUsb, "USB devices",
     "Connect to USB devices", "usb", false, AstraContentSetting::kAsk},
    {AstraSitePermissionType::kSerial, "Serial ports",
     "Connect to serial ports", "serial", false, AstraContentSetting::kAsk},
    {AstraSitePermissionType::kHid, "HID devices",
     "Connect to HID devices", "hid", false, AstraContentSetting::kAsk},
    {AstraSitePermissionType::kNfc, "NFC devices",
     "Use NFC to communicate with nearby devices", "nfc", false,
     AstraContentSetting::kAsk},
    {AstraSitePermissionType::kSensors, "Sensors",
     "Use motion and light sensors", "sensors", false,
     AstraContentSetting::kAllow},
    {AstraSitePermissionType::kIdleDetection, "Idle detection",
     "Detect when you're actively using your device", "idle", false,
     AstraContentSetting::kAsk},
    {AstraSitePermissionType::kPaymentHandler, "Payment handler",
     "Handle payments on your behalf", "payment", true,
     AstraContentSetting::kAsk},
    {AstraSitePermissionType::kBackgroundSync, "Background sync",
     "Send and receive data in the background", "sync", false,
     AstraContentSetting::kAllow},
    {AstraSitePermissionType::kStorageAccess, "Storage access",
     "Access storage on this device", "storage", false,
     AstraContentSetting::kAllow},
    {AstraSitePermissionType::kScreenCapture, "Screen capture",
     "Share your screen", "screen", true, AstraContentSetting::kAsk},
    {AstraSitePermissionType::kWakeLock, "Wake lock",
     "Prevent your device from sleeping", "wake_lock", false,
     AstraContentSetting::kAsk},
    {AstraSitePermissionType::kPictureInPicture, "Picture in picture",
     "Show picture-in-picture video", "pip", false,
     AstraContentSetting::kAllow},
    {AstraSitePermissionType::kAutoPlay, "Autoplay",
     "Play audio and video automatically", "autoplay", false,
     AstraContentSetting::kBlock},
    {AstraSitePermissionType::kDownloads, "Downloads",
     "Download files automatically", "download", false,
     AstraContentSetting::kAsk},
    {AstraSitePermissionType::kMultipleDownloads, "Multiple automatic downloads",
     "Download multiple files automatically", "download", false,
     AstraContentSetting::kAsk},
    {AstraSitePermissionType::kAds, "Ads",
     "Allow ads on sites", "ad", false, AstraContentSetting::kAllow},
    {AstraSitePermissionType::kInsecureContent, "Insecure content",
     "Load insecure content on secure pages", "insecure", true,
     AstraContentSetting::kBlock},
    {AstraSitePermissionType::kCookies, "Cookies",
     "Use cookies and site data", "cookie", true, AstraContentSetting::kAllow},
    {AstraSitePermissionType::kWebXR, "Virtual reality",
     "Use virtual and augmented reality devices", "vr", false,
     AstraContentSetting::kAsk},
};

const PermissionInfo* FindPermissionInfo(AstraSitePermissionType type) {
  for (const auto& info : kPermissionInfos) {
    if (info.type == type) {
      return &info;
    }
  }
  return nullptr;
}

// Category info.
struct CategoryInfo {
  AstraSiteSettingsCategory category;
  const char* name;
  const char* icon_name;
};

const CategoryInfo kCategoryInfos[] = {
    {AstraSiteSettingsCategory::kAll, "All sites", "globe"},
    {AstraSiteSettingsCategory::kRecent, "Recently visited", "clock"},
    {AstraSiteSettingsCategory::kAllPermissions, "All permissions", "shield"},
    {AstraSiteSettingsCategory::kCookies, "Cookies and site data", "cookie"},
    {AstraSiteSettingsCategory::kLocation, "Location", "location"},
    {AstraSiteSettingsCategory::kCamera, "Camera", "camera"},
    {AstraSiteSettingsCategory::kMicrophone, "Microphone", "mic"},
    {AstraSiteSettingsCategory::kNotifications, "Notifications", "bell"},
    {AstraSiteSettingsCategory::kJavaScript, "JavaScript", "js"},
    {AstraSiteSettingsCategory::kImages, "Images", "image"},
    {AstraSiteSettingsCategory::kPopups, "Pop-ups and redirects", "popup"},
    {AstraSiteSettingsCategory::kSound, "Sound", "sound"},
    {AstraSiteSettingsCategory::kAds, "Ads", "ad"},
    {AstraSiteSettingsCategory::kBackgroundSync, "Background sync", "sync"},
    {AstraSiteSettingsCategory::kAutomaticDownloads, "Automatic downloads",
     "download"},
    {AstraSiteSettingsCategory::kMidi, "MIDI devices", "midi"},
    {AstraSiteSettingsCategory::kSerialPorts, "Serial ports", "serial"},
    {AstraSiteSettingsCategory::kUsbDevices, "USB devices", "usb"},
    {AstraSiteSettingsCategory::kBluetoothDevices, "Bluetooth devices",
     "bluetooth"},
    {AstraSiteSettingsCategory::kHidDevices, "HID devices", "hid"},
    {AstraSiteSettingsCategory::kZoomLevels, "Zoom levels", "zoom"},
    {AstraSiteSettingsCategory::kPdfDocuments, "PDF documents", "pdf"},
    {AstraSiteSettingsCategory::kProtectedContent, "Protected content IDs",
     "shield"},
    {AstraSiteSettingsCategory::kInsecureContent, "Insecure content",
     "insecure"},
    {AstraSiteSettingsCategory::kAdditionalPermissions, "Additional permissions",
     "more"},
};

// Helper to format storage size.
std::u16string FormatStorageSize(int64_t bytes) {
  if (bytes < 1024) {
    return base::UTF8ToUTF16(base::NumberToString(bytes) + " B");
  }
  if (bytes < 1024 * 1024) {
    return base::UTF8ToUTF16(
        base::NumberToString(bytes / 1024) + " KB");
  }
  if (bytes < 1024 * 1024 * 1024) {
    return base::UTF8ToUTF16(
        base::NumberToString(bytes / (1024 * 1024)) + " MB");
  }
  return base::UTF8ToUTF16(
      base::NumberToString(bytes / (1024 * 1024 * 1024)) + " GB");
}

}  // namespace

// ===========================================================================
// Static helpers
// ===========================================================================

std::u16string AstraSiteSettingsModel::GetPermissionName(
    AstraSitePermissionType type) {
  const auto* info = FindPermissionInfo(type);
  return info ? base::UTF8ToUTF16(info->name) : std::u16string();
}

std::u16string AstraSiteSettingsModel::GetPermissionDescription(
    AstraSitePermissionType type) {
  const auto* info = FindPermissionInfo(type);
  return info ? base::UTF8ToUTF16(info->description) : std::u16string();
}

std::string AstraSiteSettingsModel::GetPermissionIconName(
    AstraSitePermissionType type) {
  const auto* info = FindPermissionInfo(type);
  return info ? info->icon_name : std::string();
}

std::vector<std::pair<AstraSiteSettingsCategory, std::u16string>>
AstraSiteSettingsModel::GetCategories() {
  std::vector<std::pair<AstraSiteSettingsCategory, std::u16string>> result;
  for (const auto& cat : kCategoryInfos) {
    result.push_back({cat.category, base::UTF8ToUTF16(cat.name)});
  }
  return result;
}

// ===========================================================================
// AstraSiteSettingsModel
// ===========================================================================

AstraSiteSettingsModel::AstraSiteSettingsModel() {
  // Initialize default permissions.
  for (const auto& info : kPermissionInfos) {
    default_permissions_.push_back({info.type, info.default_setting});
  }
}

AstraSiteSettingsModel::~AstraSiteSettingsModel() {
  for (auto& observer : observers_) {
    observer.OnSiteSettingsModelShutdown(this);
  }
}

void AstraSiteSettingsModel::AddObserver(
    AstraSiteSettingsObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraSiteSettingsModel::RemoveObserver(
    AstraSiteSettingsObserver* observer) {
  observers_.RemoveObserver(observer);
}

const std::vector<AstraSiteSettingsEntry>&
AstraSiteSettingsModel::GetAllSites() const {
  return sites_;
}

std::vector<AstraSiteSettingsEntry>
AstraSiteSettingsModel::GetFilteredSites() const {
  std::vector<AstraSiteSettingsEntry> result;
  for (const auto& site : sites_) {
    if (MatchesFilter(site) && MatchesCategory(site) && MatchesSearch(site)) {
      result.push_back(site);
    }
  }

  std::sort(result.begin(), result.end(),
            [this](const AstraSiteSettingsEntry& a,
                   const AstraSiteSettingsEntry& b) {
              return CompareSites(a, b, sort_);
            });

  return result;
}

const AstraSiteSettingsEntry* AstraSiteSettingsModel::GetSite(
    const std::string& site_id) const {
  for (const auto& site : sites_) {
    if (site.id == site_id) {
      return &site;
    }
  }
  return nullptr;
}

std::vector<AstraSiteSettingsGroup>
AstraSiteSettingsModel::GetGroupedSites() const {
  std::vector<AstraSiteSettingsGroup> groups;

  base::Time now = base::Time::Now();
  base::Time one_day_ago = now - base::Days(1);
  base::Time one_week_ago = now - base::Days(7);
  base::Time four_weeks_ago = now - base::Days(28);

  auto filtered = GetFilteredSites();

  AstraSiteSettingsGroup today;
  today.id = "today";
  today.name = u"Today";

  AstraSiteSettingsGroup last_week;
  last_week.id = "last_week";
  last_week.name = u"Last week";

  AstraSiteSettingsGroup older;
  older.id = "older";
  older.name = u"Older";

  for (const auto& site : filtered) {
    if (site.last_visit >= one_day_ago) {
      today.sites.push_back(site);
    } else if (site.last_visit >= one_week_ago) {
      last_week.sites.push_back(site);
    } else {
      older.sites.push_back(site);
    }
  }

  if (!today.sites.empty()) {
    groups.push_back(std::move(today));
  }
  if (!last_week.sites.empty()) {
    groups.push_back(std::move(last_week));
  }
  if (!older.sites.empty()) {
    groups.push_back(std::move(older));
  }

  return groups;
}

void AstraSiteSettingsModel::RemoveSite(const std::string& site_id) {
  auto it = std::find_if(sites_.begin(), sites_.end(),
                         [&site_id](const AstraSiteSettingsEntry& s) {
                           return s.id == site_id;
                         });
  if (it == sites_.end()) {
    return;
  }
  sites_.erase(it);
  NotifySitesChanged();
}

void AstraSiteSettingsModel::ClearSiteData(const std::string& site_id) {
  auto* site = FindSite(site_id);
  if (!site) {
    return;
  }
  site->storage_bytes = 0;
  site->cookies_count = 0;
  site->cache_bytes = 0;
  // TODO(astra): Clear actual site data via BrowsingDataRemover.
  // Chromium owner: chrome/browser/browsing_data/browsing_data_remover.h
  NotifySitesChanged();
}

void AstraSiteSettingsModel::ResetSitePermissions(
    const std::string& site_id) {
  auto* site = FindSite(site_id);
  if (!site) {
    return;
  }
  for (auto& perm : site->permissions) {
    auto default_setting = GetDefaultPermission(perm.type);
    if (perm.setting != default_setting) {
      perm.setting = default_setting;
      perm.is_default = true;
    }
  }
  NotifySitesChanged();
}

void AstraSiteSettingsModel::SetSitePermission(const std::string& site_id,
                                               AstraSitePermissionType type,
                                               AstraContentSetting setting) {
  auto* site = FindSite(site_id);
  if (!site) {
    return;
  }

  // Find existing permission entry.
  auto it = std::find_if(site->permissions.begin(), site->permissions.end(),
                         [type](const AstraSitePermission& p) {
                           return p.type == type;
                         });

  if (it != site->permissions.end()) {
    if (it->setting == setting) {
      return;
    }
    it->setting = setting;
    it->is_default = (setting == GetDefaultPermission(type));
  } else {
    AstraSitePermission perm;
    perm.type = type;
    perm.setting = setting;
    perm.is_default = (setting == GetDefaultPermission(type));
    const auto* info = FindPermissionInfo(type);
    perm.is_important = info ? info->important : false;
    perm.last_used = base::Time::Now();
    site->permissions.push_back(perm);
  }

  NotifySitePermissionChanged(site_id, type, setting);
  NotifySitesChanged();
}

AstraContentSetting AstraSiteSettingsModel::GetDefaultPermission(
    AstraSitePermissionType type) const {
  for (const auto& p : default_permissions_) {
    if (p.first == type) {
      return p.second;
    }
  }
  return AstraContentSetting::kAsk;
}

void AstraSiteSettingsModel::SetDefaultPermission(AstraSitePermissionType type,
                                                  AstraContentSetting setting) {
  for (auto& p : default_permissions_) {
    if (p.first == type) {
      if (p.second == setting) {
        return;
      }
      p.second = setting;
      // Update all sites' is_default flags.
      for (auto& site : sites_) {
        for (auto& perm : site->permissions) {
          if (perm.type == type) {
            perm.is_default = (perm.setting == setting);
          }
        }
      }
      NotifySitesChanged();
      return;
    }
  }
}

void AstraSiteSettingsModel::SetCategory(AstraSiteSettingsCategory category) {
  if (category_ == category) {
    return;
  }
  category_ = category;
  NotifyCategoryChanged();
  NotifySitesChanged();
}

void AstraSiteSettingsModel::SetFilter(AstraSiteSettingsFilter filter) {
  if (filter_ == filter) {
    return;
  }
  filter_ = filter;
  NotifySitesChanged();
}

void AstraSiteSettingsModel::SetSort(AstraSiteSettingsSort sort) {
  if (sort_ == sort) {
    return;
  }
  sort_ = sort;
  NotifySitesChanged();
}

void AstraSiteSettingsModel::SetSearchQuery(const std::string& query) {
  if (search_query_ == query) {
    return;
  }
  search_query_ = query;
  NotifySearchQueryChanged();
  NotifySitesChanged();
}

void AstraSiteSettingsModel::SetLoading(bool loading) {
  loading_ = loading;
  NotifySitesChanged();
}

void AstraSiteSettingsModel::PopulateSampleSites() {
  sites_.clear();

  base::Time now = base::Time::Now();

  // Sample site 1: Google
  {
    AstraSiteSettingsEntry site;
    site.id = "site_" + base::NumberToString(next_site_id_++);
    site.origin = "https://www.google.com";
    site.display_name = u"Google";
    site.title = u"Google";
    site.storage_bytes = 15 * 1024 * 1024;  // 15 MB
    site.cookies_count = 42;
    site.cache_bytes = 5 * 1024 * 1024;
    site.visit_count = 1247;
    site.last_visit = now - base::Minutes(30);
    site.first_visit = now - base::Days(365);
    site.is_bookmarked = true;

    // Google has camera and mic allowed.
    AstraSitePermission camera_perm;
    camera_perm.type = AstraSitePermissionType::kCamera;
    camera_perm.setting = AstraContentSetting::kAllow;
    camera_perm.is_default = false;
    camera_perm.is_important = true;
    camera_perm.last_used = now - base::Hours(2);
    site.permissions.push_back(camera_perm);

    AstraSitePermission mic_perm;
    mic_perm.type = AstraSitePermissionType::kMicrophone;
    mic_perm.setting = AstraContentSetting::kAllow;
    mic_perm.is_default = false;
    mic_perm.is_important = true;
    mic_perm.last_used = now - base::Hours(2);
    site.permissions.push_back(mic_perm);

    AstraSitePermission notif_perm;
    notif_perm.type = AstraSitePermissionType::kNotifications;
    notif_perm.setting = AstraContentSetting::kAllow;
    notif_perm.is_default = false;
    notif_perm.is_important = true;
    site.permissions.push_back(notif_perm);

    sites_.push_back(std::move(site));
  }

  // Sample site 2: YouTube
  {
    AstraSiteSettingsEntry site;
    site.id = "site_" + base::NumberToString(next_site_id_++);
    site.origin = "https://www.youtube.com";
    site.display_name = u"YouTube";
    site.title = u"YouTube";
    site.storage_bytes = 8 * 1024 * 1024;
    site.cookies_count = 18;
    site.cache_bytes = 25 * 1024 * 1024;
    site.visit_count = 892;
    site.last_visit = now - base::Hours(2);
    site.first_visit = now - base::Days(200);
    site.is_bookmarked = false;

    AstraSitePermission autoplay_perm;
    autoplay_perm.type = AstraSitePermissionType::kAutoPlay;
    autoplay_perm.setting = AstraContentSetting::kAllow;
    autoplay_perm.is_default = false;
    site.permissions.push_back(autoplay_perm);

    AstraSitePermission pip_perm;
    pip_perm.type = AstraSitePermissionType::kPictureInPicture;
    pip_perm.setting = AstraContentSetting::kAllow;
    pip_perm.is_default = true;
    site.permissions.push_back(pip_perm);

    sites_.push_back(std::move(site));
  }

  // Sample site 3: GitHub
  {
    AstraSiteSettingsEntry site;
    site.id = "site_" + base::NumberToString(next_site_id_++);
    site.origin = "https://github.com";
    site.display_name = u"GitHub";
    site.title = u"GitHub";
    site.storage_bytes = 3 * 1024 * 1024;
    site.cookies_count = 12;
    site.cache_bytes = 2 * 1024 * 1024;
    site.visit_count = 523;
    site.last_visit = now - base::Hours(5);
    site.first_visit = now - base::Days(180);
    site.is_bookmarked = true;

    AstraSitePermission notif_perm;
    notif_perm.type = AstraSitePermissionType::kNotifications;
    notif_perm.setting = AstraContentSetting::kAllow;
    notif_perm.is_default = false;
    notif_perm.is_important = true;
    site.permissions.push_back(notif_perm);

    AstraSitePermission clipboard_perm;
    clipboard_perm.type = AstraSitePermissionType::kClipboardRead;
    clipboard_perm.setting = AstraContentSetting::kAllow;
    clipboard_perm.is_default = false;
    clipboard_perm.is_important = true;
    site.permissions.push_back(clipboard_perm);

    sites_.push_back(std::move(site));
  }

  // Sample site 4: A news site with notifications blocked
  {
    AstraSiteSettingsEntry site;
    site.id = "site_" + base::NumberToString(next_site_id_++);
    site.origin = "https://news.example.com";
    site.display_name = u"News Example";
    site.title = u"Breaking News - News Example";
    site.storage_bytes = 1 * 1024 * 1024;
    site.cookies_count = 25;
    site.cache_bytes = 1 * 1024 * 1024;
    site.visit_count = 67;
    site.last_visit = now - base::Days(1);
    site.first_visit = now - base::Days(90);

    AstraSitePermission notif_perm;
    notif_perm.type = AstraSitePermissionType::kNotifications;
    notif_perm.setting = AstraContentSetting::kBlock;
    notif_perm.is_default = false;
    notif_perm.is_important = true;
    site.permissions.push_back(notif_perm);

    AstraSitePermission popups_perm;
    popups_perm.type = AstraSitePermissionType::kPopups;
    popups_perm.setting = AstraContentSetting::kAllow;
    popups_perm.is_default = false;
    site.permissions.push_back(popups_perm);

    sites_.push_back(std::move(site));
  }

  // Sample site 5: Maps / location
  {
    AstraSiteSettingsEntry site;
    site.id = "site_" + base::NumberToString(next_site_id_++);
    site.origin = "https://maps.example.com";
    site.display_name = u"Maps Example";
    site.title = u"Maps - Directions & Traffic";
    site.storage_bytes = 5 * 1024 * 1024;
    site.cookies_count = 8;
    site.cache_bytes = 10 * 1024 * 1024;
    site.visit_count = 156;
    site.last_visit = now - base::Days(2);
    site.first_visit = now - base::Days(150);

    AstraSitePermission location_perm;
    location_perm.type = AstraSitePermissionType::kGeolocation;
    location_perm.setting = AstraContentSetting::kAllow;
    location_perm.is_default = false;
    location_perm.is_important = true;
    location_perm.last_used = now - base::Days(2);
    site.permissions.push_back(location_perm);

    sites_.push_back(std::move(site));
  }

  // Sample site 6: Video conferencing
  {
    AstraSiteSettingsEntry site;
    site.id = "site_" + base::NumberToString(next_site_id_++);
    site.origin = "https://meet.example.com";
    site.display_name = u"Meet Example";
    site.title = u"Meet Example - Video Calls";
    site.storage_bytes = 2 * 1024 * 1024;
    site.cookies_count = 6;
    site.cache_bytes = 3 * 1024 * 1024;
    site.visit_count = 89;
    site.last_visit = now - base::Days(3);
    site.first_visit = now - base::Days(60);
    site.is_bookmarked = true;

    AstraSitePermission camera_perm;
    camera_perm.type = AstraSitePermissionType::kCamera;
    camera_perm.setting = AstraContentSetting::kAllow;
    camera_perm.is_default = false;
    camera_perm.is_important = true;
    camera_perm.last_used = now - base::Days(3);
    site.permissions.push_back(camera_perm);

    AstraSitePermission mic_perm;
    mic_perm.type = AstraSitePermissionType::kMicrophone;
    mic_perm.setting = AstraContentSetting::kAllow;
    mic_perm.is_default = false;
    mic_perm.is_important = true;
    mic_perm.last_used = now - base::Days(3);
    site.permissions.push_back(mic_perm);

    AstraSitePermission screen_perm;
    screen_perm.type = AstraSitePermissionType::kScreenCapture;
    screen_perm.setting = AstraContentSetting::kAsk;
    screen_perm.is_default = true;
    screen_perm.is_important = true;
    site.permissions.push_back(screen_perm);

    sites_.push_back(std::move(site));
  }

  // Sample site 7: Shopping
  {
    AstraSiteSettingsEntry site;
    site.id = "site_" + base::NumberToString(next_site_id_++);
    site.origin = "https://shop.example.com";
    site.display_name = u"Shop Example";
    site.title = u"Shop Example - Deals & Products";
    site.storage_bytes = 4 * 1024 * 1024;
    site.cookies_count = 56;
    site.cache_bytes = 8 * 1024 * 1024;
    site.visit_count = 234;
    site.last_visit = now - base::Days(4);
    site.first_visit = now - base::Days(200);

    AstraSitePermission payment_perm;
    payment_perm.type = AstraSitePermissionType::kPaymentHandler;
    payment_perm.setting = AstraContentSetting::kAllow;
    payment_perm.is_default = false;
    payment_perm.is_important = true;
    site.permissions.push_back(payment_perm);

    sites_.push_back(std::move(site));
  }

  // Sample site 8: Social media
  {
    AstraSiteSettingsEntry site;
    site.id = "site_" + base::NumberToString(next_site_id_++);
    site.origin = "https://social.example.com";
    site.display_name = u"Social Example";
    site.title = u"Social Example - Connect with Friends";
    site.storage_bytes = 12 * 1024 * 1024;
    site.cookies_count = 38;
    site.cache_bytes = 15 * 1024 * 1024;
    site.visit_count = 756;
    site.last_visit = now - base::Days(5);
    site.first_visit = now - base::Days(400);

    AstraSitePermission notif_perm;
    notif_perm.type = AstraSitePermissionType::kNotifications;
    notif_perm.setting = AstraContentSetting::kAllow;
    notif_perm.is_default = false;
    notif_perm.is_important = true;
    site.permissions.push_back(notif_perm);

    AstraSitePermission camera_perm;
    camera_perm.type = AstraSitePermissionType::kCamera;
    camera_perm.setting = AstraContentSetting::kAsk;
    camera_perm.is_default = true;
    camera_perm.is_important = true;
    site.permissions.push_back(camera_perm);

    AstraSitePermission clipboard_perm;
    clipboard_perm.type = AstraSitePermissionType::kClipboardRead;
    clipboard_perm.setting = AstraContentSetting::kAsk;
    clipboard_perm.is_default = true;
    clipboard_perm.is_important = true;
    site.permissions.push_back(clipboard_perm);

    sites_.push_back(std::move(site));
  }

  // Sample site 9: Banking
  {
    AstraSiteSettingsEntry site;
    site.id = "site_" + base::NumberToString(next_site_id_++);
    site.origin = "https://bank.example.com";
    site.display_name = u"Bank Example";
    site.title = u"Bank Example - Online Banking";
    site.storage_bytes = 500 * 1024;  // 500 KB
    site.cookies_count = 4;
    site.cache_bytes = 500 * 1024;
    site.visit_count = 45;
    site.last_visit = now - base::Days(7);
    site.first_visit = now - base::Days(100);

    // Bank site has no special permissions - very restrictive
    sites_.push_back(std::move(site));
  }

  // Sample site 10: Music streaming
  {
    AstraSiteSettingsEntry site;
    site.id = "site_" + base::NumberToString(next_site_id_++);
    site.origin = "https://music.example.com";
    site.display_name = u"Music Example";
    site.title = u"Music Example - Stream Your Favorite Songs";
    site.storage_bytes = 20 * 1024 * 1024;
    site.cookies_count = 15;
    site.cache_bytes = 50 * 1024 * 1024;
    site.visit_count = 432;
    site.last_visit = now - base::Days(6);
    site.first_visit = now - base::Days(250);

    AstraSitePermission sound_perm;
    sound_perm.type = AstraSitePermissionType::kSound;
    sound_perm.setting = AstraContentSetting::kAllow;
    sound_perm.is_default = true;
    site.permissions.push_back(sound_perm);

    AstraSitePermission autoplay_perm;
    autoplay_perm.type = AstraSitePermissionType::kAutoPlay;
    autoplay_perm.setting = AstraContentSetting::kAllow;
    autoplay_perm.is_default = false;
    site.permissions.push_back(autoplay_perm);

    sites_.push_back(std::move(site));
  }

  // Sample site 11: Developer docs
  {
    AstraSiteSettingsEntry site;
    site.id = "site_" + base::NumberToString(next_site_id_++);
    site.origin = "https://docs.example.dev";
    site.display_name = u"Developer Docs";
    site.title = u"Example Developer Documentation";
    site.storage_bytes = 800 * 1024;
    site.cookies_count = 3;
    site.cache_bytes = 4 * 1024 * 1024;
    site.visit_count = 178;
    site.last_visit = now - base::Days(10);
    site.first_visit = now - base::Days(120);

    sites_.push_back(std::move(site));
  }

  // Sample site 12: File storage
  {
    AstraSiteSettingsEntry site;
    site.id = "site_" + base::NumberToString(next_site_id_++);
    site.origin = "https://drive.example.com";
    site.display_name = u"Drive Example";
    site.title = u"Drive Example - Cloud Storage";
    site.storage_bytes = 50 * 1024 * 1024;  // 50 MB
    site.cookies_count = 10;
    site.cache_bytes = 20 * 1024 * 1024;
    site.visit_count = 267;
    site.last_visit = now - base::Days(8);
    site.first_visit = now - base::Days(180);

    AstraSitePermission fs_perm;
    fs_perm.type = AstraSitePermissionType::kFileSystemRead;
    fs_perm.setting = AstraContentSetting::kAsk;
    fs_perm.is_default = true;
    site.permissions.push_back(fs_perm);

    sites_.push_back(std::move(site));
  }

  NotifySitesChanged();
}

// ===========================================================================
// Private helpers
// ===========================================================================

void AstraSiteSettingsModel::NotifySitesChanged() {
  for (auto& observer : observers_) {
    observer.OnSitesChanged(this);
  }
}

void AstraSiteSettingsModel::NotifyCategoryChanged() {
  for (auto& observer : observers_) {
    observer.OnCategoryChanged(this, category_);
  }
}

void AstraSiteSettingsModel::NotifySearchQueryChanged() {
  for (auto& observer : observers_) {
    observer.OnSearchQueryChanged(this, search_query_);
  }
}

void AstraSiteSettingsModel::NotifySitePermissionChanged(
    const std::string& site_id,
    AstraSitePermissionType type,
    AstraContentSetting setting) {
  for (auto& observer : observers_) {
    observer.OnSitePermissionChanged(this, site_id, type, setting);
  }
}

AstraSiteSettingsEntry* AstraSiteSettingsModel::FindSite(
    const std::string& site_id) {
  for (auto& site : sites_) {
    if (site.id == site_id) {
      return &site;
    }
  }
  return nullptr;
}

bool AstraSiteSettingsModel::MatchesFilter(
    const AstraSiteSettingsEntry& site) const {
  switch (filter_) {
    case AstraSiteSettingsFilter::kAll:
      return true;
    case AstraSiteSettingsFilter::kAllowed:
      for (const auto& p : site.permissions) {
        if (p.setting == AstraContentSetting::kAllow && !p.is_default) {
          return true;
        }
      }
      return false;
    case AstraSiteSettingsFilter::kBlocked:
      for (const auto& p : site.permissions) {
        if (p.setting == AstraContentSetting::kBlock && !p.is_default) {
          return true;
        }
      }
      return false;
    case AstraSiteSettingsFilter::kWithData:
      return site.storage_bytes > 0 || site.cookies_count > 0;
  }
  return true;
}

bool AstraSiteSettingsModel::MatchesCategory(
    const AstraSiteSettingsEntry& site) const {
  switch (category_) {
    case AstraSiteSettingsCategory::kAll:
    case AstraSiteSettingsCategory::kAllPermissions:
      return true;
    case AstraSiteSettingsCategory::kRecent:
      return !site.last_visit.is_null() &&
             site.last_visit >= base::Time::Now() - base::Days(28);
    case AstraSiteSettingsCategory::kCookies:
      return site.cookies_count > 0 ||
             std::any_of(site.permissions.begin(), site.permissions.end(),
                         [](const AstraSitePermission& p) {
                           return p.type == AstraSitePermissionType::kCookies;
                         });
    case AstraSiteSettingsCategory::kLocation:
      return std::any_of(site.permissions.begin(), site.permissions.end(),
                         [](const AstraSitePermission& p) {
                           return p.type ==
                               AstraSitePermissionType::kGeolocation;
                         });
    case AstraSiteSettingsCategory::kCamera:
      return std::any_of(site.permissions.begin(), site.permissions.end(),
                         [](const AstraSitePermission& p) {
                           return p.type == AstraSitePermissionType::kCamera;
                         });
    case AstraSiteSettingsCategory::kMicrophone:
      return std::any_of(site.permissions.begin(), site.permissions.end(),
                         [](const AstraSitePermission& p) {
                           return p.type ==
                               AstraSitePermissionType::kMicrophone;
                         });
    case AstraSiteSettingsCategory::kNotifications:
      return std::any_of(site.permissions.begin(), site.permissions.end(),
                         [](const AstraSitePermission& p) {
                           return p.type ==
                               AstraSitePermissionType::kNotifications;
                         });
    case AstraSiteSettingsCategory::kJavaScript:
      return std::any_of(site.permissions.begin(), site.permissions.end(),
                         [](const AstraSitePermission& p) {
                           return p.type ==
                               AstraSitePermissionType::kJavaScript;
                         });
    case AstraSiteSettingsCategory::kImages:
      return std::any_of(site.permissions.begin(), site.permissions.end(),
                         [](const AstraSitePermission& p) {
                           return p.type == AstraSitePermissionType::kImages;
                         });
    case AstraSiteSettingsCategory::kPopups:
      return std::any_of(site.permissions.begin(), site.permissions.end(),
                         [](const AstraSitePermission& p) {
                           return p.type == AstraSitePermissionType::kPopups;
                         });
    case AstraSiteSettingsCategory::kSound:
      return std::any_of(site.permissions.begin(), site.permissions.end(),
                         [](const AstraSitePermission& p) {
                           return p.type == AstraSitePermissionType::kSound;
                         });
    case AstraSiteSettingsCategory::kAds:
      return std::any_of(site.permissions.begin(), site.permissions.end(),
                         [](const AstraSitePermission& p) {
                           return p.type == AstraSitePermissionType::kAds;
                         });
    case AstraSiteSettingsCategory::kBackgroundSync:
      return std::any_of(site.permissions.begin(), site.permissions.end(),
                         [](const AstraSitePermission& p) {
                           return p.type ==
                               AstraSitePermissionType::kBackgroundSync;
                         });
    case AstraSiteSettingsCategory::kAutomaticDownloads:
      return std::any_of(site.permissions.begin(), site.permissions.end(),
                         [](const AstraSitePermission& p) {
                           return p.type ==
                               AstraSitePermissionType::kMultipleDownloads ||
                                  p.type == AstraSitePermissionType::kDownloads;
                         });
    default:
      return true;
  }
}

bool AstraSiteSettingsModel::MatchesSearch(
    const AstraSiteSettingsEntry& site) const {
  if (search_query_.empty()) {
    return true;
  }
  std::string query_lower = base::ToLowerASCII(search_query_);
  std::string origin_lower = base::ToLowerASCII(site.origin);
  std::string display_lower = base::UTF16ToUTF8(
      base::ToLowerASCII(site.display_name));
  std::string title_lower = base::UTF16ToUTF8(base::ToLowerASCII(site.title));
  return origin_lower.find(query_lower) != std::string::npos ||
         display_lower.find(query_lower) != std::string::npos ||
         title_lower.find(query_lower) != std::string::npos;
}

bool AstraSiteSettingsModel::CompareSites(const AstraSiteSettingsEntry& a,
                                          const AstraSiteSettingsEntry& b,
                                          AstraSiteSettingsSort sort) {
  switch (sort) {
    case AstraSiteSettingsSort::kName:
      return a.display_name < b.display_name;
    case AstraSiteSettingsSort::kStorage:
      return a.storage_bytes > b.storage_bytes;
    case AstraSiteSettingsSort::kMostVisited:
      return a.visit_count > b.visit_count;
    case AstraSiteSettingsSort::kLastVisited:
      return a.last_visit > b.last_visit;
    case AstraSiteSettingsSort::kPermissionCount: {
      size_t a_allowed = 0, b_allowed = 0;
      for (const auto& p : a.permissions) {
        if (p.setting == AstraContentSetting::kAllow) a_allowed++;
      }
      for (const auto& p : b.permissions) {
        if (p.setting == AstraContentSetting::kAllow) b_allowed++;
      }
      return a_allowed > b_allowed;
    }
  }
  return false;
}

}  // namespace astra
