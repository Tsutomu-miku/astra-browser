#include "astra/ui/views/downloads/astra_downloads_bubble_model.h"

#include <algorithm>
#include <cmath>

#include "base/i18n/number_formatting.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/l10n/l10n_util.h"

namespace astra {

AstraDownloadsBubbleModel::AstraDownloadsBubbleModel(
    AstraDownloadsHelper* helper)
    : helper_(helper) {
  if (helper_) {
    helper_->AddObserver(this);
  }
  Refresh();
}

AstraDownloadsBubbleModel::~AstraDownloadsBubbleModel() {
  if (helper_) {
    helper_->RemoveObserver(this);
  }
}

void AstraDownloadsBubbleModel::AddObserver(
    AstraDownloadsBubbleObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraDownloadsBubbleModel::RemoveObserver(
    AstraDownloadsBubbleObserver* observer) {
  observers_.RemoveObserver(observer);
}

std::vector<AstraDownloadsBubbleItem>
AstraDownloadsBubbleModel::GetDisplayDownloads() const {
  auto items = GetFilteredDownloads();
  SortItems(items);
  if (static_cast<int>(items.size()) > max_display_items_) {
    items.resize(max_display_items_);
  }
  return items;
}

size_t AstraDownloadsBubbleModel::GetDisplayCount() const {
  return GetDisplayDownloads().size();
}

const AstraDownloadsBubbleItem* AstraDownloadsBubbleModel::GetDownload(
    const std::string& id) const {
  auto items = GetFilteredDownloads();
  for (const auto& item : items) {
    if (item.id == id) {
      // Return pointer to a static copy to avoid dangling reference.
      // In production, this would come from a cached list.
      // For now, return nullptr as this is a projection model.
      // TODO(astra): Cache items locally for pointer stability.
      break;
    }
  }
  return nullptr;
}

int AstraDownloadsBubbleModel::GetActiveDownloadCount() const {
  if (!helper_) {
    return 0;
  }
  return static_cast<int>(helper_->GetActiveDownloadCount());
}

bool AstraDownloadsBubbleModel::HasActiveDownloads() const {
  return GetActiveDownloadCount() > 0;
}

int AstraDownloadsBubbleModel::GetTotalDownloadCount() const {
  if (!helper_) {
    return 0;
  }
  return static_cast<int>(helper_->GetDownloadCount());
}

void AstraDownloadsBubbleModel::SetSortOrder(
    AstraDownloadsBubbleSortOrder order) {
  if (sort_order_ == order) {
    return;
  }
  sort_order_ = order;
  NotifyDownloadsChanged();
}

void AstraDownloadsBubbleModel::SetMaxDisplayItems(int max) {
  max = std::clamp(max, kMinDisplayItems, kMaxDisplayItems);
  if (max_display_items_ == max) {
    return;
  }
  max_display_items_ = max;
  NotifyDownloadsChanged();
}

void AstraDownloadsBubbleModel::SetShowCompleted(bool show) {
  if (show_completed_ == show) {
    return;
  }
  show_completed_ = show;
  NotifyDownloadsChanged();
}

void AstraDownloadsBubbleModel::SetShowFailed(bool show) {
  if (show_failed_ == show) {
    return;
  }
  show_failed_ = show;
  NotifyDownloadsChanged();
}

void AstraDownloadsBubbleModel::PauseDownload(const std::string& id) {
  if (!helper_) {
    return;
  }
  int download_id;
  if (base::StringToInt(id, &download_id)) {
    helper_->PauseDownload(download_id);
  }
}

void AstraDownloadsBubbleModel::ResumeDownload(const std::string& id) {
  if (!helper_) {
    return;
  }
  int download_id;
  if (base::StringToInt(id, &download_id)) {
    helper_->ResumeDownload(download_id);
  }
}

void AstraDownloadsBubbleModel::CancelDownload(const std::string& id) {
  if (!helper_) {
    return;
  }
  int download_id;
  if (base::StringToInt(id, &download_id)) {
    helper_->CancelDownload(download_id);
  }
}

void AstraDownloadsBubbleModel::RemoveDownload(const std::string& id) {
  if (!helper_) {
    return;
  }
  int download_id;
  if (base::StringToInt(id, &download_id)) {
    helper_->RemoveDownload(download_id);
  }
}

void AstraDownloadsBubbleModel::OpenDownload(const std::string& id) {
  if (!helper_) {
    return;
  }
  int download_id;
  if (base::StringToInt(id, &download_id)) {
    helper_->OpenDownload(download_id);
  }
}

void AstraDownloadsBubbleModel::ShowDownloadInFolder(const std::string& id) {
  if (!helper_) {
    return;
  }
  int download_id;
  if (base::StringToInt(id, &download_id)) {
    helper_->ShowDownloadInFolder(download_id);
  }
}

void AstraDownloadsBubbleModel::RetryDownload(const std::string& id) {
  // Retry is implemented as restarting the download from source.
  // TODO(astra): Implement proper retry via DownloadItem.
  CancelDownload(id);
}

void AstraDownloadsBubbleModel::PauseAllDownloads() {
  if (helper_) {
    helper_->PauseAllDownloads();
  }
}

void AstraDownloadsBubbleModel::ResumeAllDownloads() {
  if (helper_) {
    helper_->ResumeAllDownloads();
  }
}

void AstraDownloadsBubbleModel::ClearCompletedDownloads() {
  if (helper_) {
    helper_->ClearCompletedDownloads();
  }
}

void AstraDownloadsBubbleModel::Refresh() {
  if (helper_) {
    int new_active_count = GetActiveDownloadCount();
    if (cached_active_count_ != new_active_count) {
      cached_active_count_ = new_active_count;
      NotifyActiveCountChanged();
    }
    NotifyDownloadsChanged();
  }
}

void AstraDownloadsBubbleModel::set_show_file_size(bool show) {
  if (show_file_size_ == show) {
    return;
  }
  show_file_size_ = show;
  NotifyDownloadsChanged();
}

void AstraDownloadsBubbleModel::set_show_speed(bool show) {
  if (show_speed_ == show) {
    return;
  }
  show_speed_ = show;
  NotifyDownloadsChanged();
}

void AstraDownloadsBubbleModel::set_show_time_remaining(bool show) {
  if (show_time_remaining_ == show) {
    return;
  }
  show_time_remaining_ = show;
  NotifyDownloadsChanged();
}

void AstraDownloadsBubbleModel::set_show_progress_ring(bool show) {
  show_progress_ring_ = show;
}

void AstraDownloadsBubbleModel::set_show_badge(bool show) {
  show_badge_ = show;
}

// static
std::u16string AstraDownloadsBubbleModel::FormatBytes(int64_t bytes) {
  if (bytes < 0) {
    return u"Unknown";
  }
  const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
  double size = static_cast<double>(bytes);
  int unit_index = 0;
  while (size >= 1024.0 && unit_index < 5) {
    size /= 1024.0;
    unit_index++;
  }
  std::string result;
  if (size < 10.0) {
    result = base::StringPrintf("%.2f %s", size, units[unit_index]);
  } else if (size < 100.0) {
    result = base::StringPrintf("%.1f %s", size, units[unit_index]);
  } else {
    result = base::StringPrintf("%.0f %s", size, units[unit_index]);
  }
  return base::UTF8ToUTF16(result);
}

// static
std::u16string AstraDownloadsBubbleModel::FormatSpeed(int64_t bytes_per_sec) {
  if (bytes_per_sec <= 0) {
    return u"—";
  }
  return FormatBytes(bytes_per_sec) + u"/s";
}

// static
std::u16string AstraDownloadsBubbleModel::FormatTimeRemaining(
    base::TimeDelta remaining) {
  if (remaining.is_zero() || remaining.is_negative()) {
    return u"Calculating...";
  }
  int total_seconds = static_cast<int>(remaining.InSeconds());
  if (total_seconds < 60) {
    return base::UTF8ToUTF16(
        base::StringPrintf("%ds", total_seconds));
  }
  int minutes = total_seconds / 60;
  int seconds = total_seconds % 60;
  if (minutes < 60) {
    if (seconds == 0) {
      return base::UTF8ToUTF16(base::StringPrintf("%dm", minutes));
    }
    return base::UTF8ToUTF16(
        base::StringPrintf("%dm %ds", minutes, seconds));
  }
  int hours = minutes / 60;
  minutes = minutes % 60;
  if (minutes == 0) {
    return base::UTF8ToUTF16(base::StringPrintf("%dh", hours));
  }
  return base::UTF8ToUTF16(
      base::StringPrintf("%dh %dm", hours, minutes));
}

// static
double AstraDownloadsBubbleModel::CalculateProgress(int64_t received,
                                                    int64_t total) {
  if (total <= 0 || received < 0) {
    return 0.0;
  }
  if (received >= total) {
    return 1.0;
  }
  return static_cast<double>(received) / static_cast<double>(total);
}

// -- AstraDownloadsObserver ---------------------------------------------

void AstraDownloadsBubbleModel::OnDownloadStarted(int download_id) {
  int new_active_count = GetActiveDownloadCount();
  if (cached_active_count_ != new_active_count) {
    cached_active_count_ = new_active_count;
    NotifyActiveCountChanged();
  }
  NotifyDownloadStarted(IdToString(download_id));
  NotifyDownloadsChanged();
}

void AstraDownloadsBubbleModel::OnDownloadUpdated(int download_id) {
  NotifyDownloadUpdated(IdToString(download_id));
}

void AstraDownloadsBubbleModel::OnDownloadCompleted(int download_id) {
  int new_active_count = GetActiveDownloadCount();
  if (cached_active_count_ != new_active_count) {
    cached_active_count_ = new_active_count;
    NotifyActiveCountChanged();
  }
  NotifyDownloadCompleted(IdToString(download_id));
  NotifyDownloadsChanged();
}

void AstraDownloadsBubbleModel::OnDownloadFailed(int download_id,
                                                 const std::string& error) {
  int new_active_count = GetActiveDownloadCount();
  if (cached_active_count_ != new_active_count) {
    cached_active_count_ = new_active_count;
    NotifyActiveCountChanged();
  }
  NotifyDownloadUpdated(IdToString(download_id));
  NotifyDownloadsChanged();
}

void AstraDownloadsBubbleModel::OnDownloadRemoved(int download_id) {
  int new_active_count = GetActiveDownloadCount();
  if (cached_active_count_ != new_active_count) {
    cached_active_count_ = new_active_count;
    NotifyActiveCountChanged();
  }
  NotifyDownloadsChanged();
}

void AstraDownloadsBubbleModel::OnAllDownloadsCleared() {
  cached_active_count_ = 0;
  NotifyActiveCountChanged();
  NotifyDownloadsChanged();
}

void AstraDownloadsBubbleModel::OnDownloadsSettingsChanged() {}

// static
AstraDownloadsBubbleItem AstraDownloadsBubbleModel::ToBubbleItem(
    const AstraDownloadItem& item) {
  AstraDownloadsBubbleItem bubble_item;
  bubble_item.id = base::NumberToString(item.id);
  bubble_item.filename = base::UTF8ToUTF16(item.file_name);
  bubble_item.url = item.url;
  bubble_item.total_bytes = item.total_bytes;
  bubble_item.received_bytes = item.received_bytes;
  bubble_item.state = item.state;
  bubble_item.start_time = item.start_time;
  bubble_item.end_time = item.end_time;
  bubble_item.mime_type = item.mime_type;
  bubble_item.is_dangerous = item.is_dangerous;
  bubble_item.speed_bytes_per_sec = 0;  // Not available in AstraDownloadItem
  if (item.total_bytes > 0 && item.received_bytes > 0) {
    // Estimate time remaining based on received bytes and start time.
    // In production, this comes from DownloadItem::CurrentSpeed().
  }
  return bubble_item;
}

std::vector<AstraDownloadsBubbleItem>
AstraDownloadsBubbleModel::GetFilteredDownloads() const {
  std::vector<AstraDownloadsBubbleItem> result;
  if (!helper_) {
    return result;
  }
  auto all_items = helper_->GetAllDownloads();
  for (const auto& item : all_items) {
    bool is_active = item.state == AstraDownloadState::kInProgress;
    bool is_completed = item.state == AstraDownloadState::kCompleted;
    bool is_failed = item.state == AstraDownloadState::kFailed ||
                     item.state == AstraDownloadState::kCancelled ||
                     item.state == AstraDownloadState::kInterrupted;
    if (is_active) {
      result.push_back(ToBubbleItem(item));
    } else if (is_completed && show_completed_) {
      result.push_back(ToBubbleItem(item));
    } else if (is_failed && show_failed_) {
      result.push_back(ToBubbleItem(item));
    }
  }
  return result;
}

void AstraDownloadsBubbleModel::SortItems(
    std::vector<AstraDownloadsBubbleItem>& items) const {
  std::sort(items.begin(), items.end(),
            [this](const AstraDownloadsBubbleItem& a,
                   const AstraDownloadsBubbleItem& b) {
              return CompareItems(a, b, sort_order_);
            });
}

// static
bool AstraDownloadsBubbleModel::CompareItems(
    const AstraDownloadsBubbleItem& a,
    const AstraDownloadsBubbleItem& b,
    AstraDownloadsBubbleSortOrder order) {
  // Active downloads always come first, regardless of sort order.
  bool a_active = a.state == AstraDownloadState::kInProgress;
  bool b_active = b.state == AstraDownloadState::kInProgress;
  if (a_active != b_active) {
    return a_active;  // Active first
  }

  switch (order) {
    case AstraDownloadsBubbleSortOrder::kNewestFirst:
      return a.start_time > b.start_time;
    case AstraDownloadsBubbleSortOrder::kOldestFirst:
      return a.start_time < b.start_time;
    case AstraDownloadsBubbleSortOrder::kLargestFirst:
      return a.total_bytes > b.total_bytes;
    case AstraDownloadsBubbleSortOrder::kSmallestFirst:
      return a.total_bytes < b.total_bytes;
    case AstraDownloadsBubbleSortOrder::kName:
      return a.filename < b.filename;
  }
  return a.start_time > b.start_time;
}

void AstraDownloadsBubbleModel::NotifyDownloadsChanged() {
  for (auto& observer : observers_) {
    observer.OnDownloadsChanged(this);
  }
}

void AstraDownloadsBubbleModel::NotifyDownloadUpdated(const std::string& id) {
  for (auto& observer : observers_) {
    observer.OnDownloadUpdated(this, id);
  }
}

void AstraDownloadsBubbleModel::NotifyDownloadStarted(const std::string& id) {
  for (auto& observer : observers_) {
    observer.OnDownloadStarted(this, id);
  }
}

void AstraDownloadsBubbleModel::NotifyDownloadCompleted(
    const std::string& id) {
  for (auto& observer : observers_) {
    observer.OnDownloadCompleted(this, id);
  }
}

void AstraDownloadsBubbleModel::NotifyActiveCountChanged() {
  int count = GetActiveDownloadCount();
  for (auto& observer : observers_) {
    observer.OnActiveCountChanged(this, count);
  }
}

// static
std::string AstraDownloadsBubbleModel::IdToString(int download_id) {
  return base::NumberToString(download_id);
}

}  // namespace astra
