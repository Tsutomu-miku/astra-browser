#include "astra/browser/astra_downloads_helper.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "base/i18n/number_formatting.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"

// TODO(astra): The following includes reference Chromium headers that are
// only available in a full Chromium checkout.  In this overlay repo, the
// types are forward-declared in the header and the real definitions come
// from Chromium at build time.
//
// Chromium owner: DownloadManager (content/public/browser/download_manager.h)
// Chromium owner: DownloadItem (components/download/public/common/download_item.h)
// #include "content/public/browser/download_manager.h"
// #include "components/download/public/common/download_item.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Helper to format a byte value with the appropriate unit suffix.
std::string FormatBytesWithUnit(int64_t bytes, const char* per_second_suffix) {
  if (bytes < 0) {
    return per_second_suffix ? std::string("0 B") + per_second_suffix : "0 B";
  }

  const int64_t kKiloByte = 1024;
  const int64_t kMegaByte = 1024 * 1024;
  const int64_t kGigaByte = 1024 * 1024 * 1024;
  const int64_t kTeraByte = 1024LL * 1024 * 1024 * 1024;

  std::string result;
  if (bytes < kKiloByte) {
    result = base::StringPrintf("%" PRId64 " B", bytes);
  } else if (bytes < kMegaByte) {
    result = base::StringPrintf("%.1f KB", static_cast<double>(bytes) / kKiloByte);
  } else if (bytes < kGigaByte) {
    result = base::StringPrintf("%.1f MB", static_cast<double>(bytes) / kMegaByte);
  } else if (bytes < kTeraByte) {
    result = base::StringPrintf("%.1f GB", static_cast<double>(bytes) / kGigaByte);
  } else {
    result = base::StringPrintf("%.1f TB", static_cast<double>(bytes) / kTeraByte);
  }

  if (per_second_suffix) {
    result += per_second_suffix;
  }
  return result;
}

}  // namespace

// =========================================================================
// Construction / destruction
// =========================================================================

AstraDownloadsHelper::AstraDownloadsHelper(Profile* profile)
    : profile_(profile) {
  // TODO(astra): Start observing DownloadManager for live updates.
  //   download::DownloadManager* manager = GetDownloadManager();
  //   if (manager) {
  //     manager->AddObserver(this);
  //     is_observing_manager_ = true;
  //   }
  //
  // Chromium observer: DownloadManager::Observer
  //   (content/public/browser/download_manager.h)
}

AstraDownloadsHelper::~AstraDownloadsHelper() {
  // Observers should already be cleaned up by Shutdown().
  DCHECK(!is_observing_manager_);
}

void AstraDownloadsHelper::Shutdown() {
  // TODO(astra): Remove observer from DownloadManager.
  //   download::DownloadManager* manager = GetDownloadManager();
  //   if (manager && is_observing_manager_) {
  //     manager->RemoveObserver(this);
  //     is_observing_manager_ = false;
  //   }
  is_observing_manager_ = false;
  observers_.Clear();
  profile_ = nullptr;
}

// =========================================================================
// Download list queries
// =========================================================================

size_t AstraDownloadsHelper::GetDownloadCount() const {
  download::DownloadManager* manager = GetDownloadManager();
  if (!manager) {
    return 0;
  }

  // TODO(astra): Get the total count from DownloadManager.
  //   size_t count = 0;
  //   std::vector<download::DownloadItem*> items;
  //   manager->GetAllDownloads(&items);
  //   return items.size();
  //
  // Chromium method: DownloadManager::GetAllDownloads()
  return 0;
}

size_t AstraDownloadsHelper::GetActiveDownloadCount() const {
  download::DownloadManager* manager = GetDownloadManager();
  if (!manager) {
    return 0;
  }

  // TODO(astra): Use DownloadManager::InProgressCount() for active downloads.
  //   return manager->InProgressCount();
  //
  // Chromium method: DownloadManager::InProgressCount()
  return 0;
}

size_t AstraDownloadsHelper::GetCompletedDownloadCount() const {
  download::DownloadManager* manager = GetDownloadManager();
  if (!manager) {
    return 0;
  }

  // TODO(astra): Count completed downloads from GetAllDownloads().
  //   std::vector<download::DownloadItem*> items;
  //   manager->GetAllDownloads(&items);
  //   size_t count = 0;
  //   for (auto* item : items) {
  //     if (item->GetState() == download::DownloadItem::COMPLETE) {
  //       ++count;
  //     }
  //   }
  //   return count;
  return 0;
}

AstraDownloadItem AstraDownloadsHelper::GetDownload(int id) const {
  AstraDownloadItem item;

  download::DownloadManager* manager = GetDownloadManager();
  if (!manager) {
    return item;
  }

  // TODO(astra): Use DownloadManager::GetDownload(id) to get the item.
  //   download::DownloadItem* download = manager->GetDownload(id);
  //   if (!download) {
  //     return item;
  //   }
  //   // Project DownloadItem into AstraDownloadItem.
  //   item.id = download->GetId();
  //   item.url = download->GetURL();
  //   item.file_name = download->GetFileNameToReportUser().AsUTF8Unsafe();
  //   item.file_path = download->GetTargetFilePath();
  //   item.total_bytes = download->GetTotalBytes();
  //   item.received_bytes = download->GetReceivedBytes();
  //   switch (download->GetState()) {
  //     case download::DownloadItem::IN_PROGRESS:
  //       item.state = AstraDownloadState::kInProgress;
  //       break;
  //     case download::DownloadItem::COMPLETE:
  //       item.state = AstraDownloadState::kCompleted;
  //       break;
  //     case download::DownloadItem::CANCELLED:
  //       item.state = AstraDownloadState::kCancelled;
  //       break;
  //     case download::DownloadItem::INTERRUPTED:
  //       item.state = AstraDownloadState::kInterrupted;
  //       break;
  //     default:
  //       item.state = AstraDownloadState::kInProgress;
  //   }
  //   item.start_time = download->GetStartTime();
  //   item.end_time = download->GetEndTime();
  //   item.is_dangerous = download->IsDangerous();
  //   item.mime_type = download->GetMimeType();
  //   item.is_paused = download->IsPaused();
  //
  // Chromium method: DownloadManager::GetDownload()
  // Chromium type: download::DownloadItem

  return item;
}

std::vector<AstraDownloadItem> AstraDownloadsHelper::GetAllDownloads() const {
  std::vector<AstraDownloadItem> result;

  download::DownloadManager* manager = GetDownloadManager();
  if (!manager) {
    return result;
  }

  // TODO(astra): Get all downloads from DownloadManager and project them.
  //   std::vector<download::DownloadItem*> items;
  //   manager->GetAllDownloads(&items);
  //   // Sort by start time descending (newest first).
  //   std::sort(items.begin(), items.end(),
  //       [](download::DownloadItem* a, download::DownloadItem* b) {
  //         return a->GetStartTime() > b->GetStartTime();
  //       });
  //   for (auto* download : items) {
  //     AstraDownloadItem item;
  //     // ... project fields ...
  //     result.push_back(std::move(item));
  //   }
  //
  // Chromium method: DownloadManager::GetAllDownloads()

  return result;
}

std::vector<AstraDownloadItem> AstraDownloadsHelper::GetActiveDownloads() const {
  std::vector<AstraDownloadItem> result;

  auto all = GetAllDownloads();
  for (const auto& item : all) {
    if (item.state == AstraDownloadState::kInProgress) {
      result.push_back(item);
    }
  }
  return result;
}

std::vector<AstraDownloadItem> AstraDownloadsHelper::GetRecentDownloads(
    int max_count) const {
  std::vector<AstraDownloadItem> result;

  int effective_max = max_count > 0 ? max_count : GetMaxRecentDownloads();
  if (effective_max <= 0) {
    return result;
  }

  auto all = GetAllDownloads();
  for (const auto& item : all) {
    if (item.state == AstraDownloadState::kCompleted ||
        item.state == AstraDownloadState::kCancelled ||
        item.state == AstraDownloadState::kFailed ||
        item.state == AstraDownloadState::kInterrupted) {
      result.push_back(item);
      if (static_cast<int>(result.size()) >= effective_max) {
        break;
      }
    }
  }
  return result;
}

// =========================================================================
// Progress and speed
// =========================================================================

double AstraDownloadsHelper::GetDownloadProgress(int id) const {
  auto item = GetDownload(id);
  return CalculateProgress(item.received_bytes, item.total_bytes);
}

int64_t AstraDownloadsHelper::GetDownloadSpeed(int id) const {
  download::DownloadManager* manager = GetDownloadManager();
  if (!manager) {
    return 0;
  }

  // TODO(astra): Use DownloadItem::CurrentSpeed() for real speed.
  //   download::DownloadItem* download = manager->GetDownload(id);
  //   if (!download || download->GetState() != download::DownloadItem::IN_PROGRESS) {
  //     return 0;
  //   }
  //   return download->CurrentSpeed();
  //
  // Chromium method: download::DownloadItem::CurrentSpeed()
  return 0;
}

bool AstraDownloadsHelper::IsDownloading() const {
  return GetActiveDownloadCount() > 0;
}

// =========================================================================
// Download operations
// =========================================================================

void AstraDownloadsHelper::PauseDownload(int id) {
  download::DownloadManager* manager = GetDownloadManager();
  if (!manager) {
    return;
  }

  // TODO(astra): Call Pause() on the download item.
  //   download::DownloadItem* download = manager->GetDownload(id);
  //   if (!download) {
  //     return;
  //   }
  //   if (download->IsPaused()) {
  //     return;  // Already paused.
  //   }
  //   download->Pause();
  //   NotifyDownloadUpdated(id);
  //
  // Chromium method: download::DownloadItem::Pause()
}

void AstraDownloadsHelper::ResumeDownload(int id) {
  download::DownloadManager* manager = GetDownloadManager();
  if (!manager) {
    return;
  }

  // TODO(astra): Call Resume() on the download item.
  //   download::DownloadItem* download = manager->GetDownload(id);
  //   if (!download) {
  //     return;
  //   }
  //   if (!download->IsPaused()) {
  //     return;  // Not paused.
  //   }
  //   download->Resume();
  //   NotifyDownloadUpdated(id);
  //
  // Chromium method: download::DownloadItem::Resume()
}

void AstraDownloadsHelper::CancelDownload(int id) {
  download::DownloadManager* manager = GetDownloadManager();
  if (!manager) {
    return;
  }

  // TODO(astra): Call Cancel() on the download item.
  //   download::DownloadItem* download = manager->GetDownload(id);
  //   if (!download) {
  //     return;
  //   }
  //   if (download->GetState() != download::DownloadItem::IN_PROGRESS) {
  //     return;  // Already done.
  //   }
  //   download->Cancel(true);  // true = user-initiated
  //   // Observer will fire OnDownloadRemoved / OnDownloadUpdated.
  //
  // Chromium method: download::DownloadItem::Cancel()
}

void AstraDownloadsHelper::RemoveDownload(int id) {
  download::DownloadManager* manager = GetDownloadManager();
  if (!manager) {
    return;
  }

  // TODO(astra): Call Remove() on the download item.
  //   download::DownloadItem* download = manager->GetDownload(id);
  //   if (!download) {
  //     return;
  //   }
  //   download->Remove();
  //   NotifyDownloadRemoved(id);
  //
  // Chromium method: download::DownloadItem::Remove()
}

void AstraDownloadsHelper::ClearCompletedDownloads() {
  auto all = GetAllDownloads();
  for (const auto& item : all) {
    if (item.state == AstraDownloadState::kCompleted) {
      RemoveDownload(item.id);
    }
  }
}

void AstraDownloadsHelper::OpenDownload(int id) {
  download::DownloadManager* manager = GetDownloadManager();
  if (!manager) {
    return;
  }

  // TODO(astra): Call OpenDownload() on the download item.
  //   download::DownloadItem* download = manager->GetDownload(id);
  //   if (!download) {
  //     return;
  //   }
  //   if (download->GetState() != download::DownloadItem::COMPLETE) {
  //     return;
  //   }
  //   download->OpenDownload();
  //
  // Chromium method: download::DownloadItem::OpenDownload()
}

void AstraDownloadsHelper::ShowDownloadInFolder(int id) {
  download::DownloadManager* manager = GetDownloadManager();
  if (!manager) {
    return;
  }

  // TODO(astra): Call ShowDownloadInShell() on the download item.
  //   download::DownloadItem* download = manager->GetDownload(id);
  //   if (!download) {
  //     return;
  //   }
  //   download->ShowDownloadInShell();
  //
  // Chromium method: download::DownloadItem::ShowDownloadInShell()
}

// =========================================================================
// Bulk operations
// =========================================================================

void AstraDownloadsHelper::PauseAllDownloads() {
  auto active = GetActiveDownloads();
  for (const auto& item : active) {
    if (!item.is_paused) {
      PauseDownload(item.id);
    }
  }
}

void AstraDownloadsHelper::ResumeAllDownloads() {
  auto all = GetAllDownloads();
  for (const auto& item : all) {
    if (item.state == AstraDownloadState::kInProgress && item.is_paused) {
      ResumeDownload(item.id);
    }
  }
}

void AstraDownloadsHelper::CancelAllDownloads() {
  auto active = GetActiveDownloads();
  for (const auto& item : active) {
    CancelDownload(item.id);
  }
}

void AstraDownloadsHelper::ClearAllDownloads() {
  auto all = GetAllDownloads();
  for (const auto& item : all) {
    RemoveDownload(item.id);
  }
  NotifyAllDownloadsCleared();
}

// =========================================================================
// Presentation settings
// =========================================================================

bool AstraDownloadsHelper::GetShowDownloadsInSidebar() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return kDefaultShowDownloadsInSidebar;
  }
  return prefs->GetBoolean(prefs::kPrefDownloadsShowInSidebar);
}

void AstraDownloadsHelper::SetShowDownloadsInSidebar(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefDownloadsShowInSidebar) == show) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefDownloadsShowInSidebar, show);
  NotifyDownloadsSettingsChanged();
}

bool AstraDownloadsHelper::ToggleShowDownloadsInSidebar() {
  bool new_state = !GetShowDownloadsInSidebar();
  SetShowDownloadsInSidebar(new_state);
  return GetShowDownloadsInSidebar();
}

bool AstraDownloadsHelper::GetShowDownloadNotifications() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return kDefaultShowDownloadNotifications;
  }
  return prefs->GetBoolean(prefs::kPrefDownloadsShowNotifications);
}

void AstraDownloadsHelper::SetShowDownloadNotifications(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefDownloadsShowNotifications) == show) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefDownloadsShowNotifications, show);
  NotifyDownloadsSettingsChanged();
}

bool AstraDownloadsHelper::ToggleShowDownloadNotifications() {
  bool new_state = !GetShowDownloadNotifications();
  SetShowDownloadNotifications(new_state);
  return GetShowDownloadNotifications();
}

bool AstraDownloadsHelper::GetAutoOpenDownloads() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return kDefaultAutoOpenDownloads;
  }
  return prefs->GetBoolean(prefs::kPrefDownloadsAutoOpen);
}

void AstraDownloadsHelper::SetAutoOpenDownloads(bool auto_open) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefDownloadsAutoOpen) == auto_open) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefDownloadsAutoOpen, auto_open);
  NotifyDownloadsSettingsChanged();
}

bool AstraDownloadsHelper::ToggleAutoOpenDownloads() {
  bool new_state = !GetAutoOpenDownloads();
  SetAutoOpenDownloads(new_state);
  return GetAutoOpenDownloads();
}

std::string AstraDownloadsHelper::GetDownloadsSortOrder() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return kDefaultDownloadsSortOrder;
  }
  return prefs->GetString(prefs::kPrefDownloadsSortOrder);
}

void AstraDownloadsHelper::SetDownloadsSortOrder(const std::string& sort_order) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetString(prefs::kPrefDownloadsSortOrder) == sort_order) {
    return;
  }

  prefs->SetString(prefs::kPrefDownloadsSortOrder, sort_order);
  NotifyDownloadsSettingsChanged();
}

int AstraDownloadsHelper::GetMaxRecentDownloads() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return kDefaultMaxRecentDownloads;
  }
  return prefs->GetInteger(prefs::kPrefDownloadsMaxRecent);
}

void AstraDownloadsHelper::SetMaxRecentDownloads(int max_count) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  int clamped = ClampMaxRecentDownloads(max_count);
  int current = prefs->GetInteger(prefs::kPrefDownloadsMaxRecent);

  if (current == clamped) {
    return;
  }

  prefs->SetInteger(prefs::kPrefDownloadsMaxRecent, clamped);
  NotifyDownloadsSettingsChanged();
}

int AstraDownloadsHelper::ClampMaxRecentDownloads(int value) {
  if (value < kMinRecentDownloads) {
    return kMinRecentDownloads;
  }
  if (value > kMaxRecentDownloadsLimit) {
    return kMaxRecentDownloadsLimit;
  }
  return value;
}

bool AstraDownloadsHelper::GetShowDownloadSpeed() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return kDefaultShowDownloadSpeed;
  }
  return prefs->GetBoolean(prefs::kPrefDownloadsShowSpeed);
}

void AstraDownloadsHelper::SetShowDownloadSpeed(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefDownloadsShowSpeed) == show) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefDownloadsShowSpeed, show);
  NotifyDownloadsSettingsChanged();
}

bool AstraDownloadsHelper::ToggleShowDownloadSpeed() {
  bool new_state = !GetShowDownloadSpeed();
  SetShowDownloadSpeed(new_state);
  return GetShowDownloadSpeed();
}

bool AstraDownloadsHelper::GetShowFileSize() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return kDefaultShowFileSize;
  }
  return prefs->GetBoolean(prefs::kPrefDownloadsShowFileSize);
}

void AstraDownloadsHelper::SetShowFileSize(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefDownloadsShowFileSize) == show) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefDownloadsShowFileSize, show);
  NotifyDownloadsSettingsChanged();
}

bool AstraDownloadsHelper::ToggleShowFileSize() {
  bool new_state = !GetShowFileSize();
  SetShowFileSize(new_state);
  return GetShowFileSize();
}

bool AstraDownloadsHelper::GetShowDownloadProgress() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return kDefaultShowDownloadProgress;
  }
  return prefs->GetBoolean(prefs::kPrefDownloadsShowProgress);
}

void AstraDownloadsHelper::SetShowDownloadProgress(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefDownloadsShowProgress) == show) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefDownloadsShowProgress, show);
  NotifyDownloadsSettingsChanged();
}

bool AstraDownloadsHelper::ToggleShowDownloadProgress() {
  bool new_state = !GetShowDownloadProgress();
  SetShowDownloadProgress(new_state);
  return GetShowDownloadProgress();
}

std::string AstraDownloadsHelper::GetDownloadsDisplayMode() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return kDefaultDownloadsDisplayMode;
  }
  return prefs->GetString(prefs::kPrefDownloadsDisplayMode);
}

void AstraDownloadsHelper::SetDownloadsDisplayMode(const std::string& mode) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetString(prefs::kPrefDownloadsDisplayMode) == mode) {
    return;
  }

  prefs->SetString(prefs::kPrefDownloadsDisplayMode, mode);
  NotifyDownloadsSettingsChanged();
}

bool AstraDownloadsHelper::GetPromptForDownloadLocation() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return kDefaultPromptForDownloadLocation;
  }
  return prefs->GetBoolean(prefs::kPrefDownloadsPromptForLocation);
}

void AstraDownloadsHelper::SetPromptForDownloadLocation(bool prompt) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefDownloadsPromptForLocation) == prompt) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefDownloadsPromptForLocation, prompt);
  NotifyDownloadsSettingsChanged();
}

bool AstraDownloadsHelper::TogglePromptForDownloadLocation() {
  bool new_state = !GetPromptForDownloadLocation();
  SetPromptForDownloadLocation(new_state);
  return GetPromptForDownloadLocation();
}

bool AstraDownloadsHelper::GetSafeBrowsingWarnings() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return kDefaultSafeBrowsingWarnings;
  }
  return prefs->GetBoolean(prefs::kPrefDownloadsSafeBrowsingWarnings);
}

void AstraDownloadsHelper::SetSafeBrowsingWarnings(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefDownloadsSafeBrowsingWarnings) == show) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefDownloadsSafeBrowsingWarnings, show);
  NotifyDownloadsSettingsChanged();
}

bool AstraDownloadsHelper::ToggleSafeBrowsingWarnings() {
  bool new_state = !GetSafeBrowsingWarnings();
  SetSafeBrowsingWarnings(new_state);
  return GetSafeBrowsingWarnings();
}

// =========================================================================
// Utility methods
// =========================================================================

std::string AstraDownloadsHelper::FormatFileSize(int64_t bytes) {
  return FormatBytesWithUnit(bytes, nullptr);
}

std::string AstraDownloadsHelper::FormatDownloadSpeed(int64_t bytes_per_second) {
  return FormatBytesWithUnit(bytes_per_second, "/s");
}

std::string AstraDownloadsHelper::FormatTimeRemaining(base::TimeDelta remaining) {
  if (remaining <= base::TimeDelta()) {
    return "Calculating...";
  }

  int total_seconds = static_cast<int>(remaining.InSeconds());
  if (total_seconds <= 0) {
    return "Calculating...";
  }

  int hours = total_seconds / 3600;
  int minutes = (total_seconds % 3600) / 60;
  int seconds = total_seconds % 60;

  if (hours > 0) {
    if (minutes > 0) {
      return base::StringPrintf("%dh %dm", hours, minutes);
    }
    return base::StringPrintf("%dh", hours);
  } else if (minutes > 0) {
    if (seconds > 0) {
      return base::StringPrintf("%dm %ds", minutes, seconds);
    }
    return base::StringPrintf("%dm", minutes);
  } else {
    return base::StringPrintf("%ds", seconds);
  }
}

double AstraDownloadsHelper::CalculateProgress(int64_t received, int64_t total) {
  if (total <= 0 || received < 0) {
    return 0.0;
  }
  if (received >= total) {
    return 1.0;
  }
  return static_cast<double>(received) / static_cast<double>(total);
}

// =========================================================================
// Observers
// =========================================================================

void AstraDownloadsHelper::AddObserver(AstraDownloadsObserver* observer) {
  if (observer) {
    observers_.AddObserver(observer);
  }
}

void AstraDownloadsHelper::RemoveObserver(AstraDownloadsObserver* observer) {
  if (observer) {
    observers_.RemoveObserver(observer);
  }
}

// =========================================================================
// Notification helpers
// =========================================================================

void AstraDownloadsHelper::NotifyDownloadStarted(int download_id) {
  for (auto& observer : observers_) {
    observer.OnDownloadStarted(download_id);
  }
}

void AstraDownloadsHelper::NotifyDownloadUpdated(int download_id) {
  for (auto& observer : observers_) {
    observer.OnDownloadUpdated(download_id);
  }
}

void AstraDownloadsHelper::NotifyDownloadCompleted(int download_id) {
  for (auto& observer : observers_) {
    observer.OnDownloadCompleted(download_id);
  }
}

void AstraDownloadsHelper::NotifyDownloadFailed(int download_id,
                                                const std::string& error) {
  for (auto& observer : observers_) {
    observer.OnDownloadFailed(download_id, error);
  }
}

void AstraDownloadsHelper::NotifyDownloadRemoved(int download_id) {
  for (auto& observer : observers_) {
    observer.OnDownloadRemoved(download_id);
  }
}

void AstraDownloadsHelper::NotifyAllDownloadsCleared() {
  for (auto& observer : observers_) {
    observer.OnAllDownloadsCleared();
  }
}

void AstraDownloadsHelper::NotifyDownloadsSettingsChanged() {
  for (auto& observer : observers_) {
    observer.OnDownloadsSettingsChanged();
  }
}

// =========================================================================
// Internal helpers
// =========================================================================

download::DownloadManager* AstraDownloadsHelper::GetDownloadManager() const {
  if (!profile_) {
    return nullptr;
  }

  // TODO(astra): Use content::BrowserContext::GetDownloadManager() when
  // building against the full Chromium source tree.  In the overlay, we
  // return nullptr as a placeholder since the real download manager isn't
  // linked.
  //
  // Chromium owner: content::DownloadManager
  //   (content/public/browser/download_manager.h)
  // The download manager is a BrowserContextKeyedService, one per profile.
  //
  // Access via: profile->GetDownloadManager()
  //   or content::BrowserContext::GetDownloadManager(profile)
  //
  // Patch point: None needed — standard DownloadManager access.
  return nullptr;
}

PrefService* AstraDownloadsHelper::GetPrefs() const {
  if (!profile_) {
    return nullptr;
  }
  return profile_->GetPrefs();
}

}  // namespace astra
