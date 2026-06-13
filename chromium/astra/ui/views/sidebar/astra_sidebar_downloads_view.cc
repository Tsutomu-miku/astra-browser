#include "astra/ui/views/sidebar/astra_sidebar_downloads_view.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

#include "astra/ui/color/astra_color_ids.h"
#include "base/i18n/number_formatting.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/common/webui_url_constants.h"
#include "components/download/public/common/download_item.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/web_contents.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/base/page_transition_types.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/link.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"

#include "astra/ui/views/sidebar/astra_download_item_view.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kDownloadsSectionHeaderHeight = 28;
constexpr int kDownloadsSectionHorizontalPadding = 12;
constexpr int kDownloadsSectionVerticalPadding = 8;
constexpr int kDownloadsItemSpacing = 2;
constexpr int kDownloadsHeaderFontSizeDelta = 1;
constexpr int kShowAllLinkTopPadding = 4;

// Section title.
const char16_t kDownloadsTitle[] = u"Downloads";
const char16_t kShowAllText[] = u"Show all downloads";
const char16_t kEmptyDownloadsText[] = u"No downloads";
const char16_t kSearchPlaceholder[] = u"Search downloads...";

// Astra color IDs.
constexpr ui::ColorId kDownloadsSectionHeaderTextColorId =
    kColorAstraSidebarSectionHeaderText;
constexpr ui::ColorId kDownloadsShowAllTextColorId = kColorAstraSidebarItemText;
constexpr ui::ColorId kDownloadsBackgroundColorId =
    kColorAstraSidebarBackground;

}  // namespace

// =========================================================================
// Construction
// =========================================================================

AstraSidebarDownloadsView::AstraSidebarDownloadsView(
    content::DownloadManager* download_manager)
    : AstraSidebarSectionView(kDownloadsTitle,
                              AstraSidebarSectionType::kDownloads),
      download_manager_(download_manager) {
  BuildDownloadsLayout();

  // Configure base section appearance.
  SetShowChevron(true);
  SetShowItemCount(true);
  SetShowSearch(true);
  SetShowMoreButton(true);
  SetEmptyStateText(kEmptyDownloadsText);

  if (download_manager_) {
    download_manager_->AddObserver(this);
    is_observing_ = true;
    RefreshFromManager();
  }
}

AstraSidebarDownloadsView::~AstraSidebarDownloadsView() {
  if (download_manager_ && is_observing_) {
    download_manager_->RemoveObserver(this);
    is_observing_ = false;
  }
}

// =========================================================================
// Layout
// =========================================================================

void AstraSidebarDownloadsView::BuildDownloadsLayout() {
  // Base class handles header, content, and footer layout.
  // The items container is already set up.

  // Configure items container layout.
  if (items_container()) {
    auto* items_layout = items_container()->SetLayoutManager(
        std::make_unique<views::BoxLayout>(
            views::BoxLayout::Orientation::kVertical, gfx::Insets::VH(0, 4),
            kDownloadsItemSpacing));
    items_layout->set_cross_axis_alignment(
        views::BoxLayout::CrossAxisAlignment::kStretch);
  }

  // Footer: "Show all downloads" link.
  if (GetFooterView()) {
    GetFooterView()->SetVisible(true);
    // TODO(astra): Configure footer show more link.
  }

  // Accessibility for the whole section.
  SetAccessibleName(u"Downloads");
}

// =========================================================================
// Download data projection
// =========================================================================

void AstraSidebarDownloadsView::SetDownloads(
    const std::vector<AstraDownloadItemInfo>& downloads) {
  downloads_ = downloads;
  RebuildItems();
}

int AstraSidebarDownloadsView::GetDownloadCount() const {
  return static_cast<int>(downloads_.size());
}

AstraDownloadItemInfo AstraSidebarDownloadsView::GetDownloadAt(
    int index) const {
  if (index < 0 || index >= static_cast<int>(downloads_.size())) {
    return AstraDownloadItemInfo();
  }
  return downloads_[index];
}

void AstraSidebarDownloadsView::AddDownload(
    const AstraDownloadItemInfo& download) {
  downloads_.push_back(download);
  ApplySortOrder();
  RebuildItems();
}

void AstraSidebarDownloadsView::RemoveDownload(int index) {
  if (index < 0 || index >= static_cast<int>(downloads_.size())) {
    return;
  }
  downloads_.erase(downloads_.begin() + index);
  RebuildItems();

  // Adjust selection.
  if (selected_index_ >= static_cast<int>(downloads_.size())) {
    selected_index_ = static_cast<int>(downloads_.size()) - 1;
  }
}

void AstraSidebarDownloadsView::ClearAllDownloads() {
  downloads_.clear();
  RemoveAllItems();
  SetItemCount(0);
  SetEmpty(true);
  selected_index_ = -1;
}

void AstraSidebarDownloadsView::ClearCompletedDownloads() {
  std::vector<AstraDownloadItemInfo> remaining;
  for (const auto& dl : downloads_) {
    if (dl.state != AstraDownloadState::kComplete) {
      remaining.push_back(dl);
    }
  }
  downloads_ = std::move(remaining);
  RebuildItems();
}

void AstraSidebarDownloadsView::UpdateDownload(
    int index,
    const AstraDownloadItemInfo& download) {
  if (index < 0 || index >= static_cast<int>(downloads_.size())) {
    return;
  }
  downloads_[index] = download;
  // TODO(astra): Update just the corresponding item view instead of full rebuild.
  RebuildItems();
}

// =========================================================================
// Selection
// =========================================================================

void AstraSidebarDownloadsView::SetSelectedDownload(int index) {
  selected_index_ = index;
  // TODO(astra): Update visual selection state of item views.
}

void AstraSidebarDownloadsView::ClearSelection() {
  selected_index_ = -1;
  // TODO(astra): Clear visual selection on all items.
}

// =========================================================================
// Category visibility
// =========================================================================

void AstraSidebarDownloadsView::SetShowInProgress(bool show) {
  if (show_in_progress_ == show) {
    return;
  }
  show_in_progress_ = show;
  RebuildItems();
}

void AstraSidebarDownloadsView::SetShowCompleted(bool show) {
  if (show_completed_ == show) {
    return;
  }
  show_completed_ = show;
  RebuildItems();
}

void AstraSidebarDownloadsView::SetShowCancelled(bool show) {
  if (show_cancelled_ == show) {
    return;
  }
  show_cancelled_ = show;
  RebuildItems();
}

int AstraSidebarDownloadsView::GetInProgressCount() const {
  int count = 0;
  for (const auto& dl : downloads_) {
    if (dl.state == AstraDownloadState::kInProgress ||
        dl.state == AstraDownloadState::kPaused) {
      ++count;
    }
  }
  return count;
}

int AstraSidebarDownloadsView::GetCompletedCount() const {
  int count = 0;
  for (const auto& dl : downloads_) {
    if (dl.state == AstraDownloadState::kComplete) {
      ++count;
    }
  }
  return count;
}

int AstraSidebarDownloadsView::GetCancelledCount() const {
  int count = 0;
  for (const auto& dl : downloads_) {
    if (dl.state == AstraDownloadState::kCancelled ||
        dl.state == AstraDownloadState::kFailed ||
        dl.state == AstraDownloadState::kInterrupted) {
      ++count;
    }
  }
  return count;
}

int64_t AstraSidebarDownloadsView::GetTotalDownloadedSize() const {
  int64_t total = 0;
  for (const auto& dl : downloads_) {
    if (dl.state == AstraDownloadState::kComplete) {
      total += dl.total_bytes;
    } else {
      total += dl.received_bytes;
    }
  }
  return total;
}

// =========================================================================
// Download actions
// =========================================================================

void AstraSidebarDownloadsView::PauseDownload(int index) {
  if (index < 0 || index >= static_cast<int>(downloads_.size())) {
    return;
  }
  const auto& dl = downloads_[index];
  if (dl.state != AstraDownloadState::kInProgress) {
    return;
  }

  if (delegate_) {
    delegate_->OnPauseDownload(dl.id);
  }
  // State update will come via observer callback.
}

void AstraSidebarDownloadsView::ResumeDownload(int index) {
  if (index < 0 || index >= static_cast<int>(downloads_.size())) {
    return;
  }
  const auto& dl = downloads_[index];
  if (dl.state != AstraDownloadState::kPaused) {
    return;
  }

  if (delegate_) {
    delegate_->OnResumeDownload(dl.id);
  }
}

void AstraSidebarDownloadsView::CancelDownload(int index) {
  if (index < 0 || index >= static_cast<int>(downloads_.size())) {
    return;
  }
  const auto& dl = downloads_[index];

  if (delegate_) {
    delegate_->OnCancelDownload(dl.id);
  }
}

void AstraSidebarDownloadsView::OpenDownload(int index) {
  if (index < 0 || index >= static_cast<int>(downloads_.size())) {
    return;
  }
  const auto& dl = downloads_[index];
  if (dl.state != AstraDownloadState::kComplete) {
    return;
  }

  if (delegate_) {
    delegate_->OnOpenDownload(dl.id);
  }
}

void AstraSidebarDownloadsView::ShowDownloadInFolder(int index) {
  if (index < 0 || index >= static_cast<int>(downloads_.size())) {
    return;
  }
  const auto& dl = downloads_[index];

  if (delegate_) {
    delegate_->OnShowDownloadInFolder(dl.id);
  }
}

void AstraSidebarDownloadsView::RetryDownload(int index) {
  if (index < 0 || index >= static_cast<int>(downloads_.size())) {
    return;
  }
  const auto& dl = downloads_[index];

  if (delegate_) {
    delegate_->OnRetryDownload(dl.id);
  }
}

// =========================================================================
// Sorting
// =========================================================================

void AstraSidebarDownloadsView::SetSortBy(AstraDownloadSortBy sort_by) {
  if (sort_by_ == sort_by) {
    return;
  }
  sort_by_ = sort_by;
  ApplySortOrder();
  RebuildItems();
}

// =========================================================================
// Search
// =========================================================================

void AstraSidebarDownloadsView::SearchDownloads(
    const std::u16string& query) {
  SetSearchQuery(query);
  ApplyFilters();
  RebuildItems();
}

int AstraSidebarDownloadsView::GetSearchResultsCount() const {
  if (GetSearchQuery().empty()) {
    return static_cast<int>(downloads_.size());
  }
  // TODO(astra): Count actual search matches.
  return static_cast<int>(downloads_.size());
}

// =========================================================================
// Display options
// =========================================================================

void AstraSidebarDownloadsView::SetAlwaysShowProgress(bool show) {
  if (always_show_progress_ == show) {
    return;
  }
  always_show_progress_ = show;
  // TODO(astra): Update all item views.
}

void AstraSidebarDownloadsView::SetShowFileSize(bool show) {
  if (show_file_size_ == show) {
    return;
  }
  show_file_size_ = show;
  // TODO(astra): Update all item views.
}

void AstraSidebarDownloadsView::SetShowSpeed(bool show) {
  if (show_speed_ == show) {
    return;
  }
  show_speed_ = show;
  // TODO(astra): Update all item views.
}

void AstraSidebarDownloadsView::SetShowTimeRemaining(bool show) {
  if (show_time_remaining_ == show) {
    return;
  }
  show_time_remaining_ = show;
  // TODO(astra): Update all item views.
}

double AstraSidebarDownloadsView::GetOverallProgress() const {
  int64_t total_bytes = 0;
  int64_t received_bytes = 0;

  for (const auto& dl : downloads_) {
    if (dl.state == AstraDownloadState::kInProgress ||
        dl.state == AstraDownloadState::kPaused) {
      total_bytes += dl.total_bytes;
      received_bytes += dl.received_bytes;
    }
  }

  if (total_bytes == 0) {
    return 0.0;
  }
  return static_cast<double>(received_bytes) / static_cast<double>(total_bytes);
}

int AstraSidebarDownloadsView::GetActiveDownloadCount() const {
  return GetInProgressCount();
}

// =========================================================================
// Auto-open
// =========================================================================

void AstraSidebarDownloadsView::SetAutoOpenDownloads(bool auto_open) {
  auto_open_downloads_ = auto_open;
}

// =========================================================================
// Manager integration
// =========================================================================

void AstraSidebarDownloadsView::RefreshFromManager() {
  if (!download_manager_) {
    return;
  }

  // Rebuild downloads_ vector from DownloadManager state.
  std::vector<download::DownloadItem*> all_downloads;
  download_manager_->GetAllDownloads(&all_downloads);

  downloads_.clear();

  for (auto* item : all_downloads) {
    if (!item || item->IsTransient()) {
      continue;
    }

    AstraDownloadItemInfo info;
    info.id = GetDownloadId(item);
    info.filename =
        base::UTF8ToUTF16(item->GetFileNameToReportUser().value());
    info.url = item->GetURL();
    info.total_bytes = item->GetTotalBytes();
    info.received_bytes = item->GetReceivedBytes();

    // Map download state.
    switch (item->GetState()) {
      case download::DownloadItem::IN_PROGRESS:
        info.state = item->IsPaused() ? AstraDownloadState::kPaused
                                      : AstraDownloadState::kInProgress;
        break;
      case download::DownloadItem::COMPLETE:
        info.state = AstraDownloadState::kComplete;
        break;
      case download::DownloadItem::CANCELLED:
        info.state = AstraDownloadState::kCancelled;
        break;
      case download::DownloadItem::INTERRUPTED:
        info.state = AstraDownloadState::kInterrupted;
        break;
      case download::DownloadItem::MAX_DOWNLOAD_STATE:
        info.state = AstraDownloadState::kInterrupted;
        break;
    }

    info.start_time = item->GetStartTime();
    info.end_time = item->GetEndTime();
    info.file_path = item->GetTargetFilePath();
    info.mime_type = item->GetMimeType();
    info.is_dangerous = item->IsDangerous();

    downloads_.push_back(info);
  }

  ApplySortOrder();
  RebuildItems();
}

void AstraSidebarDownloadsView::SetSectionVisible(bool visible) {
  SetVisible(visible);
}

// =========================================================================
// content::DownloadManager::Observer
// =========================================================================

void AstraSidebarDownloadsView::OnDownloadCreated(
    content::DownloadManager* /*manager*/,
    download::DownloadItem* /*item*/) {
  // TODO(astra): Incrementally add the new download.
  RefreshFromManager();
}

void AstraSidebarDownloadsView::OnDownloadUpdated(
    content::DownloadManager* /*manager*/,
    download::DownloadItem* item) {
  std::string download_id = GetDownloadId(item);

  // Find and update the download info.
  int index = FindDownloadIndex(download_id);
  if (index >= 0) {
    // Update the info struct.
    downloads_[index].received_bytes = item->GetReceivedBytes();
    downloads_[index].total_bytes = item->GetTotalBytes();

    switch (item->GetState()) {
      case download::DownloadItem::IN_PROGRESS:
        downloads_[index].state =
            item->IsPaused() ? AstraDownloadState::kPaused
                             : AstraDownloadState::kInProgress;
        break;
      case download::DownloadItem::COMPLETE:
        downloads_[index].state = AstraDownloadState::kComplete;
        downloads_[index].end_time = item->GetEndTime();
        break;
      case download::DownloadItem::CANCELLED:
        downloads_[index].state = AstraDownloadState::kCancelled;
        break;
      case download::DownloadItem::INTERRUPTED:
        downloads_[index].state = AstraDownloadState::kInterrupted;
        break;
      case download::DownloadItem::MAX_DOWNLOAD_STATE:
        break;
    }

    // Update the item view if it exists.
    AstraDownloadItemView* item_view = FindItemView(download_id);
    if (item_view) {
      item_view->UpdateState(downloads_[index].state,
                             downloads_[index].received_bytes,
                             downloads_[index].total_bytes);
    }

    // Auto-open if configured.
    if (auto_open_downloads_ &&
        item->GetState() == download::DownloadItem::COMPLETE) {
      if (delegate_) {
        delegate_->OnOpenDownload(download_id);
      }
    }
  } else {
    // Not found — fall back to full rebuild.
    RefreshFromManager();
  }
}

void AstraSidebarDownloadsView::OnDownloadRemoved(
    content::DownloadManager* /*manager*/,
    download::DownloadItem* /*item*/) {
  // TODO(astra): Incrementally remove the item.
  RefreshFromManager();
}

void AstraSidebarDownloadsView::ManagerGoingDown(
    content::DownloadManager* manager) {
  if (download_manager_ == manager && is_observing_) {
    download_manager_->RemoveObserver(this);
    is_observing_ = false;
  }
  download_manager_ = nullptr;

  RemoveAllItems();
  downloads_.clear();
  SetItemCount(0);
  SetEmpty(true);
}

// =========================================================================
// AstraDownloadItemDelegate
// =========================================================================

void AstraSidebarDownloadsView::OnDownloadItemClicked(
    const std::string& download_id) {
  if (delegate_) {
    delegate_->OnDownloadClicked(download_id);
  }

  if (!download_manager_) {
    return;
  }

  // Find the DownloadItem and handle based on state.
  download::DownloadItem* item = nullptr;
  uint64_t numeric_id = 0;
  if (base::StringToUint64(download_id, &numeric_id)) {
    item = download_manager_->GetDownload(numeric_id);
  }
  if (!item) {
    return;
  }

  switch (item->GetState()) {
    case download::DownloadItem::IN_PROGRESS:
    case download::DownloadItem::PAUSED:
      OnShowAllClicked();
      break;
    case download::DownloadItem::COMPLETE:
      item->OpenDownload();
      break;
    case download::DownloadItem::CANCELLED:
    case download::DownloadItem::INTERRUPTED:
      OnShowAllClicked();
      break;
    case download::DownloadItem::MAX_DOWNLOAD_STATE:
      break;
  }
}

void AstraSidebarDownloadsView::OnDownloadCancelRequested(
    const std::string& download_id) {
  if (delegate_) {
    delegate_->OnCancelDownload(download_id);
  }

  if (!download_manager_) {
    return;
  }

  download::DownloadItem* item = nullptr;
  uint64_t numeric_id = 0;
  if (base::StringToUint64(download_id, &numeric_id)) {
    item = download_manager_->GetDownload(numeric_id);
  }

  if (item && item->GetState() == download::DownloadItem::IN_PROGRESS) {
    item->Cancel(true /* user_cancel */);
  }
}

void AstraSidebarDownloadsView::OnDownloadPauseRequested(
    const std::string& download_id) {
  if (delegate_) {
    delegate_->OnPauseDownload(download_id);
  }
}

void AstraSidebarDownloadsView::OnDownloadResumeRequested(
    const std::string& download_id) {
  if (delegate_) {
    delegate_->OnResumeDownload(download_id);
  }
}

void AstraSidebarDownloadsView::OnDownloadOpenRequested(
    const std::string& download_id) {
  if (delegate_) {
    delegate_->OnOpenDownload(download_id);
  }
}

void AstraSidebarDownloadsView::OnDownloadShowInFolderRequested(
    const std::string& download_id) {
  if (delegate_) {
    delegate_->OnShowDownloadInFolder(download_id);
  }
}

// =========================================================================
// AstraSidebarSectionView overrides
// =========================================================================

void AstraSidebarDownloadsView::OnSearchQueryChanged(
    const std::u16string& query) {
  AstraSidebarSectionView::OnSearchQueryChanged(query);
  ApplyFilters();
  RebuildItems();
}

void AstraSidebarDownloadsView::OnShowMoreClicked() {
  OnShowAllClicked();
}

void AstraSidebarDownloadsView::OnMoreButtonClicked() {
  // TODO(astra): Show section options menu (clear completed, etc.).
  AstraSidebarSectionView::OnMoreButtonClicked();
}

// =========================================================================
// Private helpers
// =========================================================================

AstraDownloadItemView* AstraSidebarDownloadsView::FindItemView(
    const std::string& download_id) const {
  if (!items_container()) {
    return nullptr;
  }

  for (views::View* child : items_container()->children()) {
    auto* item_view = static_cast<AstraDownloadItemView*>(child);
    if (item_view->download_id() == download_id) {
      return item_view;
    }
  }
  return nullptr;
}

int AstraSidebarDownloadsView::FindDownloadIndex(
    const std::string& download_id) const {
  for (size_t i = 0; i < downloads_.size(); ++i) {
    if (downloads_[i].id == download_id) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

std::unique_ptr<AstraDownloadItemView>
AstraSidebarDownloadsView::CreateItemView(
    const AstraDownloadItemInfo& info) {
  auto view = std::make_unique<AstraDownloadItemView>(
      info.id, info.filename, info.state, info.received_bytes,
      info.total_bytes);
  view->set_delegate(this);
  view->ShowProgressBar(always_show_progress_ ||
                        info.state == AstraDownloadState::kInProgress ||
                        info.state == AstraDownloadState::kPaused);
  return view;
}

// static
std::string AstraSidebarDownloadsView::GetDownloadId(
    download::DownloadItem* item) {
  // TODO(astra): Use the download's GUID when available.
  std::string guid = item->GetGuid();
  if (!guid.empty()) {
    return guid;
  }
  return base::NumberToString(item->GetId());
}

void AstraSidebarDownloadsView::OnShowAllClicked() {
  // Open chrome://downloads in a new tab.
  // TODO(astra): Navigate the current browser's tab strip properly.
  // For now, this is a placeholder.
}

void AstraSidebarDownloadsView::RebuildItems() {
  if (!items_container()) {
    return;
  }

  // Clear existing items.
  items_container()->RemoveAllChildViews();

  int visible_count = 0;

  // Add downloads that pass current filters.
  for (const auto& info : downloads_) {
    // Category filter.
    bool is_active =
        (info.state == AstraDownloadState::kInProgress ||
         info.state == AstraDownloadState::kPaused);
    bool is_completed = (info.state == AstraDownloadState::kComplete);
    bool is_cancelled =
        (info.state == AstraDownloadState::kCancelled ||
         info.state == AstraDownloadState::kFailed ||
         info.state == AstraDownloadState::kInterrupted);

    if (is_active && !show_in_progress_) continue;
    if (is_completed && !show_completed_) continue;
    if (is_cancelled && !show_cancelled_) continue;

    // Search filter.
    if (!GetSearchQuery().empty()) {
      // TODO(astra): Implement actual search matching.
    }

    items_container()->AddChildView(CreateItemView(info));
    ++visible_count;
  }

  SetItemCount(static_cast<int>(downloads_.size()));
  SetEmpty(visible_count == 0);
  UpdateShowAllVisibility();
  InvalidateLayout();
}

void AstraSidebarDownloadsView::ApplySortOrder() {
  std::sort(downloads_.begin(), downloads_.end(),
            [this](const AstraDownloadItemInfo& a,
                   const AstraDownloadItemInfo& b) {
              return CompareDownloads(a, b, sort_by_);
            });
}

void AstraSidebarDownloadsView::ApplyFilters() {
  // Filters are applied during RebuildItems().
  // This method exists for symmetry and future optimization.
}

void AstraSidebarDownloadsView::UpdateShowAllVisibility() {
  if (GetFooterView()) {
    bool has_items =
        items_container() && !items_container()->children().empty();
    GetFooterView()->SetVisible(has_items);
  }
}

// static
bool AstraSidebarDownloadsView::CompareDownloads(
    const AstraDownloadItemInfo& a,
    const AstraDownloadItemInfo& b,
    AstraDownloadSortBy sort_by) {
  switch (sort_by) {
    case AstraDownloadSortBy::kNewestFirst:
      return a.start_time > b.start_time;
    case AstraDownloadSortBy::kOldestFirst:
      return a.start_time < b.start_time;
    case AstraDownloadSortBy::kLargestFirst:
      return a.total_bytes > b.total_bytes;
    case AstraDownloadSortBy::kSmallestFirst:
      return a.total_bytes < b.total_bytes;
    case AstraDownloadSortBy::kName:
      return a.filename < b.filename;
  }
  return false;
}

// =========================================================================
// views::View overrides
// =========================================================================

gfx::Size AstraSidebarDownloadsView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return AstraSidebarSectionView::CalculatePreferredSize(available_size);
}

void AstraSidebarDownloadsView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  AstraSidebarSectionView::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kList;
  node_data->SetName("Downloads");
}

void AstraSidebarDownloadsView::OnThemeChanged() {
  AstraSidebarSectionView::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  SetBackground(views::CreateSolidBackground(
      color_provider->GetColor(kDownloadsBackgroundColorId)));
}

}  // namespace astra
