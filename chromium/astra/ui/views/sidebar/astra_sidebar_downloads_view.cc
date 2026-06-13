#include "astra/ui/views/sidebar/astra_sidebar_downloads_view.h"

#include <algorithm>
#include <string>
#include <vector>

#include "astra/ui/color/astra_color_ids.h"
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

// Astra color IDs for the downloads panel.
// Uses the Astra sidebar color system from astra/ui/color/astra_color_ids.h.
// Chromium subsystem: ui::ColorProvider (ui/color/color_provider.h)
constexpr ui::ColorId kDownloadsSectionHeaderTextColorId =
    kColorAstraSidebarSectionHeaderText;
constexpr ui::ColorId kDownloadsShowAllTextColorId = kColorAstraSidebarItemText;

}  // namespace

AstraSidebarDownloadsView::AstraSidebarDownloadsView(
    content::DownloadManager* download_manager)
    : download_manager_(download_manager) {
  BuildLayout();

  if (download_manager_) {
    download_manager_->AddObserver(this);
    is_observing_ = true;
    // Initial sync.
    RefreshFromManager();
  }
}

AstraSidebarDownloadsView::~AstraSidebarDownloadsView() {
  if (download_manager_ && is_observing_) {
    download_manager_->RemoveObserver(this);
    is_observing_ = false;
  }
}

void AstraSidebarDownloadsView::BuildLayout() {
  // Vertical box layout for the whole section: header + items + show all.
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  layout->set_between_child_spacing(0);
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Header label.
  header_label_ = AddChildView(std::make_unique<views::Label>(kDownloadsTitle));
  header_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header_label_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(kDownloadsSectionVerticalPadding,
                      kDownloadsSectionHorizontalPadding)));
  header_label_->SetFontList(
      header_label_->font_list().DeriveWithSizeDelta(kDownloadsHeaderFontSizeDelta));
  header_label_->SetAutoColorReadabilityEnabled(false);
  header_label_->SetAccessibleName(u"Downloads section");

  // Items container with internal item spacing.
  items_container_ = AddChildView(std::make_unique<views::View>());
  auto* items_layout = items_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets::VH(0, 4),
          kDownloadsItemSpacing));
  items_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // "Show all" link at the bottom.
  show_all_label_ = AddChildView(std::make_unique<views::Label>(kShowAllText));
  show_all_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  show_all_label_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(kShowAllLinkTopPadding, kDownloadsSectionHorizontalPadding)));
  show_all_label_->SetAutoColorReadabilityEnabled(false);
  // Make the label look like a link and respond to clicks.
  // TODO(astra): Use views::Link instead of Label once it's available
  // in the views layer being linked. views::Link has proper hover/click
  // handling and accessibility.
  // Chromium component: views::Link (ui/views/controls/link.h)
  show_all_label_->SetEnabled(true);
  show_all_label_->SetAccessibleName(u"Show all downloads");

  // Accessibility for the whole section.
  SetAccessibleName(u"Downloads");
  SetFocusBehavior(FocusBehavior::ACCESSIBLE_ONLY);
}

void AstraSidebarDownloadsView::RefreshFromManager() {
  if (!download_manager_) {
    return;
  }
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
  // A new download was created — rebuild the items list.
  // TODO(astra): For better performance, incrementally insert the new
  // download item at the correct position instead of rebuilding all items.
  // Active downloads go at the top, completed downloads go after active ones
  // sorted by end time descending.
  RebuildItems();
}

void AstraSidebarDownloadsView::OnDownloadUpdated(
    content::DownloadManager* /*manager*/,
    download::DownloadItem* item) {
  // An existing download was updated (progress, state change, etc.).
  std::string download_id = GetDownloadId(item);

  AstraDownloadItemView* item_view = FindItemView(download_id);
  if (item_view) {
    // Update the existing item view with new state.
    item_view->UpdateState(
        MapDownloadState(item->GetState()),
        item->GetReceivedBytes(),
        item->GetTotalBytes());

    // If the download state changed from active to completed (or vice versa),
    // the item may need to move between the active and recent sections.
    // For simplicity, we rebuild on state transitions.
    // TODO(astra): Detect state transitions and only rebuild when the
    // download moves between active and completed sections, not on every
    // progress update.
    // For now, always rebuild on state change to keep ordering correct.
    // This is conservative but ensures correct display order.
    //
    // Actually, for progress updates we can just update the item in place.
    // Only rebuild when the state changes to a terminal state.
    if (item->GetState() == download::DownloadItem::COMPLETE ||
        item->GetState() == download::DownloadItem::CANCELLED ||
        item->GetState() == download::DownloadItem::INTERRUPTED) {
      RebuildItems();
    }
  } else {
    // Item view not found — this shouldn't normally happen, but if it does,
    // fall back to a full rebuild.
    RebuildItems();
  }
}

void AstraSidebarDownloadsView::OnDownloadRemoved(
    content::DownloadManager* /*manager*/,
    download::DownloadItem* item) {
  // A download was removed — rebuild the list.
  // TODO(astra): Incrementally remove the item view instead of rebuilding.
  RebuildItems();
}

void AstraSidebarDownloadsView::ManagerGoingDown(
    content::DownloadManager* manager) {
  // The DownloadManager is being destroyed — clear our reference and
  // stop observing. This can happen during profile shutdown.
  if (download_manager_ == manager && is_observing_) {
    download_manager_->RemoveObserver(this);
    is_observing_ = false;
  }
  download_manager_ = nullptr;

  // Clear all items since there's no data source anymore.
  items_container_->RemoveAllChildViews();
  UpdateShowAllVisibility();
}

// =========================================================================
// AstraDownloadItemDelegate
// =========================================================================

void AstraSidebarDownloadsView::OnDownloadItemClicked(
    const std::string& download_id) {
  if (!download_manager_) {
    return;
  }

  // Find the DownloadItem by ID.
  // TODO(astra): Use download_manager_->GetDownloadByGuid() when available
  // and when we use GUIDs as IDs. For now, search by iterating.
  // Chromium owner: DownloadManager::GetDownloadByGuid or GetDownload
  download::DownloadItem* item = nullptr;

  // Try to parse as a numeric ID first (fallback for non-GUID IDs).
  uint64_t numeric_id = 0;
  if (base::StringToUint64(download_id, &numeric_id)) {
    item = download_manager_->GetDownload(numeric_id);
  }

  if (!item) {
    return;
  }

  // Handle based on download state.
  switch (item->GetState()) {
    case download::DownloadItem::IN_PROGRESS:
    case download::DownloadItem::PAUSED:
      // Active download — show it in the download shelf or page.
      // TODO(astra): Delegate to Chrome's download UI to show the download
      // in the shelf. In Chrome, this is handled by DownloadShelf::ShowDownload
      // or by navigating to chrome://downloads and highlighting the item.
      // For now, we open the downloads page.
      // Chromium owner: DownloadShelf (chrome/browser/ui/views/download/download_shelf_view.h)
      // Chromium owner: DownloadPage (chrome/browser/ui/webui/downloads/downloads_ui.h)
      OnShowAllClicked();
      break;

    case download::DownloadItem::COMPLETE:
      // Completed download — open the file.
      // TODO(astra): Check if the file still exists before opening.
      // DownloadItem::OpenDownload handles this safely.
      item->OpenDownload();
      break;

    case download::DownloadItem::CANCELLED:
    case download::DownloadItem::INTERRUPTED:
      // Cancelled or interrupted — could offer retry, but for now just
      // open the downloads page.
      OnShowAllClicked();
      break;

    case download::DownloadItem::MAX_DOWNLOAD_STATE:
      // Sentinel value — not a real state.
      break;
  }
}

void AstraSidebarDownloadsView::OnDownloadCancelRequested(
    const std::string& download_id) {
  if (!download_manager_) {
    return;
  }

  // Find the DownloadItem by ID.
  download::DownloadItem* item = nullptr;
  uint64_t numeric_id = 0;
  if (base::StringToUint64(download_id, &numeric_id)) {
    item = download_manager_->GetDownload(numeric_id);
  }

  if (!item || item->GetState() != download::DownloadItem::IN_PROGRESS) {
    return;
  }

  // Cancel the download. This is delegated to Chromium's DownloadItem.
  // The sidebar will be updated via the OnDownloadUpdated observer callback.
  item->Cancel(true /* user_cancel */);
}

// =========================================================================
// views::View
// =========================================================================

gfx::Size AstraSidebarDownloadsView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return views::View::CalculatePreferredSize(available_size);
}

void AstraSidebarDownloadsView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kList;
  node_data->SetName("Downloads");
}

void AstraSidebarDownloadsView::OnThemeChanged() {
  views::View::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  if (header_label_) {
    header_label_->SetEnabledColor(
        color_provider->GetColor(kDownloadsSectionHeaderTextColorId));
  }
  if (show_all_label_) {
    show_all_label_->SetEnabledColor(
        color_provider->GetColor(kDownloadsShowAllTextColorId));
  }
}

// =========================================================================
// Private helpers
// =========================================================================

AstraDownloadItemView* AstraSidebarDownloadsView::FindItemView(
    const std::string& download_id) const {
  if (!items_container_) {
    return nullptr;
  }

  for (views::View* child : items_container_->children()) {
    auto* item_view = static_cast<AstraDownloadItemView*>(child);
    if (item_view->download_id() == download_id) {
      return item_view;
    }
  }
  return nullptr;
}

std::unique_ptr<AstraDownloadItemView> AstraSidebarDownloadsView::CreateItemView(
    download::DownloadItem* item) {
  std::string download_id = GetDownloadId(item);

  // Get the filename. Use GetFileNameToReportUser() which returns the
  // display name (handles dangerous file type renames, etc.).
  // TODO(astra): Use base::UTF16ToWide or appropriate conversion based on
  // the platform's filename encoding.
  std::u16string filename = base::UTF8ToUTF16(item->GetFileNameToReportUser().value());

  auto view = std::make_unique<AstraDownloadItemView>(
      download_id,
      filename,
      MapDownloadState(item->GetState()),
      item->GetReceivedBytes(),
      item->GetTotalBytes());

  view->set_delegate(this);

  return view;
}

// static
AstraDownloadState AstraSidebarDownloadsView::MapDownloadState(
    download::DownloadItem::DownloadState state) {
  switch (state) {
    case download::DownloadItem::IN_PROGRESS:
    case download::DownloadItem::PAUSED:
      return AstraDownloadState::kInProgress;
    case download::DownloadItem::COMPLETE:
      return AstraDownloadState::kComplete;
    case download::DownloadItem::CANCELLED:
      return AstraDownloadState::kCancelled;
    case download::DownloadItem::INTERRUPTED:
      return AstraDownloadState::kInterrupted;
    case download::DownloadItem::MAX_DOWNLOAD_STATE:
      return AstraDownloadState::kInterrupted;  // Fallback.
  }
  return AstraDownloadState::kInterrupted;
}

// static
std::string AstraSidebarDownloadsView::GetDownloadId(download::DownloadItem* item) {
  // TODO(astra): Use the download's GUID when available for stability
  // across sessions. GetGuid() returns the persistent identifier.
  // For now, use the numeric ID as a string.
  // Chromium: download::DownloadItem::GetGuid()
  std::string guid = item->GetGuid();
  if (!guid.empty()) {
    return guid;
  }
  return base::NumberToString(item->GetId());
}

void AstraSidebarDownloadsView::OnShowAllClicked() {
  // Open chrome://downloads in a new tab.
  // TODO(astra): Navigate the current browser's tab strip to chrome://downloads
  // rather than creating a new browser. We need access to the Browser object
  // for this.
  //
  // TODO(astra): Use NavigateParams properly with an existing Browser.
  // For now, this is a placeholder that demonstrates the intent.
  // Chromium owner: Navigate() in chrome/browser/ui/browser_navigator.h
  //
  // TODO(astra): Wire this to the Browser that owns the sidebar. Currently
  // we don't have a Browser* in this view — the sidebar view has one, and
  // we should receive it via a setter or constructor.
  // Chromium patch point: BrowserView::GetSidebar() or similar accessor.
}

void AstraSidebarDownloadsView::RebuildItems() {
  if (!items_container_ || !download_manager_) {
    return;
  }

  // Clear existing items.
  items_container_->RemoveAllChildViews();

  // Get all downloads from the manager.
  std::vector<download::DownloadItem*> all_downloads;
  download_manager_->GetAllDownloads(&all_downloads);

  // Separate into active and completed downloads.
  std::vector<download::DownloadItem*> active_downloads;
  std::vector<download::DownloadItem*> completed_downloads;

  for (auto* item : all_downloads) {
    if (!item) {
      continue;
    }
    // Skip downloads that are hidden or in a temporary state.
    if (item->IsTransient()) {
      continue;
    }
    // Skip off-the-record downloads that shouldn't be displayed.
    // TODO(astra): Handle incognito downloads properly.
    // AllDownloadsNotifier handles this filtering automatically.
    // Chromium owner: AllDownloadsNotifier
    if (item->GetState() == download::DownloadItem::IN_PROGRESS ||
        item->GetState() == download::DownloadItem::PAUSED) {
      active_downloads.push_back(item);
    } else if (item->GetState() == download::DownloadItem::COMPLETE) {
      completed_downloads.push_back(item);
    }
    // Cancelled and interrupted downloads are not shown in the recent list.
    // They would clutter the display. The full downloads page shows all.
  }

  // Sort active downloads by start time (most recent first).
  std::sort(active_downloads.begin(), active_downloads.end(),
            [](download::DownloadItem* a, download::DownloadItem* b) {
              return a->GetStartTime() > b->GetStartTime();
            });

  // Sort completed downloads by end time (most recent first).
  std::sort(completed_downloads.begin(), completed_downloads.end(),
            [](download::DownloadItem* a, download::DownloadItem* b) {
              return a->GetEndTime() > b->GetEndTime();
            });

  // Limit recent/completed downloads to kMaxRecentItems.
  if (completed_downloads.size() > kMaxRecentItems) {
    completed_downloads.resize(kMaxRecentItems);
  }

  // Add active downloads first.
  for (auto* item : active_downloads) {
    items_container_->AddChildView(CreateItemView(item));
  }

  // Add recent completed downloads.
  for (auto* item : completed_downloads) {
    items_container_->AddChildView(CreateItemView(item));
  }

  UpdateShowAllVisibility();

  InvalidateLayout();
}

void AstraSidebarDownloadsView::UpdateShowAllVisibility() {
  if (!show_all_label_ || !items_container_) {
    return;
  }

  // Always show "Show all" link when there are any downloads.
  // Also show it when there are no downloads but we want to encourage discovery?
  // For now, show it whenever there are items in the list.
  bool has_items = !items_container_->children().empty();
  show_all_label_->SetVisible(has_items);
}

}  // namespace astra
