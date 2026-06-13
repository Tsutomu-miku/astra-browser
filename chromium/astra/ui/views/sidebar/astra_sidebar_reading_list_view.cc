#include "astra/ui/views/sidebar/astra_sidebar_reading_list_view.h"

#include <memory>
#include <utility>

#include "astra/ui/color/astra_color_ids.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"
#include "url/gurl.h"

#include "astra/browser/astra_reading_list_service.h"
#include "astra/ui/views/sidebar/astra_reading_list_item_view.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kReadingListSectionTopPadding = 0;
constexpr int kReadingListSectionBottomPadding = 8;
constexpr int kReadingListHeaderHeight = 28;
constexpr int kReadingListHeaderHorizontalPadding = 12;
constexpr int kReadingListHeaderVerticalPadding = 8;
constexpr int kReadingListHeaderFontSizeDelta = 1;
constexpr int kReadingListGroupSpacing = 8;
constexpr int kReadingListItemSpacing = 2;

// Astra color IDs for the reading list panel.
// Uses the Astra sidebar color system from astra/ui/color/astra_color_ids.h.
// Chromium subsystem: ui::ColorProvider (ui/color/color_provider.h)
constexpr ui::ColorId kReadingListHeaderTextColorId =
    kColorAstraSidebarSectionHeaderText;
constexpr ui::ColorId kReadingListCountTextColorId =
    kColorAstraSidebarItemSecondaryText;

// Titles.
const char16_t kUnreadTitle[] = u"Unread";
const char16_t kReadTitle[] = u"Read";

}  // namespace

// =========================================================================
// Construction / destruction
// =========================================================================

AstraSidebarReadingListView::AstraSidebarReadingListView(
    AstraReadingListService* reading_list_service)
    : reading_list_service_(reading_list_service) {
  BuildLayout();

  if (reading_list_service_) {
    service_observation_.Observe(reading_list_service_);
    // If the model is already loaded, populate now.
    if (reading_list_service_->IsModelLoaded()) {
      UpdateFromService();
    }
  }
}

AstraSidebarReadingListView::~AstraSidebarReadingListView() = default;

// =========================================================================
// Layout
// =========================================================================

void AstraSidebarReadingListView::BuildLayout() {
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(kReadingListSectionTopPadding, 0),
      kReadingListGroupSpacing));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // -- Unread group --

  // Unread header row: title + count + expand/collapse.
  unread_header_ = AddChildView(std::make_unique<views::View>());
  auto* unread_header_layout = unread_header_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(kReadingListHeaderVerticalPadding,
                          kReadingListHeaderHorizontalPadding),
          8));
  unread_header_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  unread_header_->SetPaintToLayer();
  unread_header_->layer()->SetFillsBoundsOpaquely(false);

  unread_label_ = unread_header_->AddChildView(
      std::make_unique<views::Label>(kUnreadTitle));
  unread_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  unread_label_->SetAutoColorReadabilityEnabled(false);
  unread_label_->SetFontList(
      unread_label_->font_list().DeriveWithSizeDelta(
          kReadingListHeaderFontSizeDelta));
  unread_header_layout->SetFlexForView(unread_label_, 1);

  unread_count_label_ = unread_header_->AddChildView(
      std::make_unique<views::Label>());
  unread_count_label_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  unread_count_label_->SetAutoColorReadabilityEnabled(false);
  unread_count_label_->SetFontList(
      unread_count_label_->font_list().DeriveWithSizeDelta(
          kReadingListHeaderFontSizeDelta));

  // Unread items container.
  unread_items_container_ = AddChildView(std::make_unique<views::View>());
  auto* unread_items_layout = unread_items_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets::VH(0, 4), kReadingListItemSpacing));
  unread_items_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // -- Read group --

  // Read header row.
  read_header_ = AddChildView(std::make_unique<views::View>());
  auto* read_header_layout = read_header_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(kReadingListHeaderVerticalPadding,
                          kReadingListHeaderHorizontalPadding),
          8));
  read_header_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  read_header_->SetPaintToLayer();
  read_header_->layer()->SetFillsBoundsOpaquely(false);

  read_label_ = read_header_->AddChildView(
      std::make_unique<views::Label>(kReadTitle));
  read_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  read_label_->SetAutoColorReadabilityEnabled(false);
  read_label_->SetFontList(
      read_label_->font_list().DeriveWithSizeDelta(
          kReadingListHeaderFontSizeDelta));
  read_header_layout->SetFlexForView(read_label_, 1);

  read_count_label_ = read_header_->AddChildView(
      std::make_unique<views::Label>());
  read_count_label_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  read_count_label_->SetAutoColorReadabilityEnabled(false);
  read_count_label_->SetFontList(
      read_count_label_->font_list().DeriveWithSizeDelta(
          kReadingListHeaderFontSizeDelta));

  // Read items container.
  read_items_container_ = AddChildView(std::make_unique<views::View>());
  auto* read_items_layout = read_items_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets::VH(0, 4), kReadingListItemSpacing));
  read_items_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Initial state: apply expand/collapse.
  SetUnreadExpanded(unread_expanded_);
  SetReadExpanded(read_expanded_);
}

// =========================================================================
// Data population
// =========================================================================

void AstraSidebarReadingListView::UpdateFromService() {
  if (!reading_list_service_) {
    return;
  }

  // Clear existing items.
  unread_items_container_->RemoveAllChildViews();
  read_items_container_->RemoveAllChildViews();

  // Populate from service.
  PopulateUnreadItems();
  PopulateReadItems();

  // Update count labels.
  size_t unread_count = reading_list_service_->GetUnreadCount();
  size_t read_count = reading_list_service_->GetEntryCount() - unread_count;
  unread_count_label_->SetText(base::NumberToString16(unread_count));
  read_count_label_->SetText(base::NumberToString16(read_count));

  // Show/hide read group based on whether there are any read items.
  bool has_read = read_count > 0;
  read_header_->SetVisible(has_read);
  if (has_read) {
    read_items_container_->SetVisible(read_expanded_);
  } else {
    read_items_container_->SetVisible(false);
  }

  InvalidateLayout();
}

void AstraSidebarReadingListView::PopulateUnreadItems() {
  if (!reading_list_service_) {
    return;
  }

  auto entries = reading_list_service_->GetUnreadEntries();
  for (const auto& entry : entries) {
    unread_items_container_->AddChildView(CreateItemView(entry));
  }
}

void AstraSidebarReadingListView::PopulateReadItems() {
  if (!reading_list_service_) {
    return;
  }

  auto entries = reading_list_service_->GetReadEntries();
  for (const auto& entry : entries) {
    read_items_container_->AddChildView(CreateItemView(entry));
  }
}

std::unique_ptr<AstraReadingListItemView>
AstraSidebarReadingListView::CreateItemView(
    const AstraReadingListEntry& entry) {
  std::u16 title = base::UTF8ToUTF16(entry.title);
  std::u16 domain = GetDomainDisplayString(entry.url);

  auto item = std::make_unique<AstraReadingListItemView>(
      entry.url, title, domain, entry.is_read);
  item->set_delegate(this);

  return item;
}

// static
std::u16 AstraSidebarReadingListView::GetDomainDisplayString(const GURL& url) {
  if (!url.is_valid()) {
    return u"Unknown";
  }

  // TODO(astra): Use net::RegistryControlledDomainService::GetDomainAndRegistry
  // for a cleaner domain display (e.g. "en.wikipedia.org" -> "wikipedia.org").
  // Chromium component: net/base/registry_controlled_domains/
  //
  // For now, return the host directly.
  return base::UTF8ToUTF16(url.host());
}

AstraReadingListItemView*
AstraSidebarReadingListView::FindItemInContainer(views::View* container,
                                                  const GURL& url) const {
  if (!container) {
    return nullptr;
  }
  for (views::View* child : container->children()) {
    auto* item = static_cast<AstraReadingListItemView*>(child);
    if (item && item->url() == url) {
      return item;
    }
  }
  return nullptr;
}

// =========================================================================
// Expand / collapse
// =========================================================================

void AstraSidebarReadingListView::SetUnreadExpanded(bool expanded) {
  unread_expanded_ = expanded;
  if (unread_items_container_) {
    unread_items_container_->SetVisible(expanded);
  }
  // TODO(astra): Update expand/collapse arrow icon on the header.
}

void AstraSidebarReadingListView::SetReadExpanded(bool expanded) {
  read_expanded_ = expanded;
  if (read_items_container_ && read_header_->GetVisible()) {
    read_items_container_->SetVisible(expanded);
  }
  // TODO(astra): Update expand/collapse arrow icon on the header.
}

// =========================================================================
// AstraReadingListServiceObserver
// =========================================================================

void AstraSidebarReadingListView::OnReadingListEntryAdded(
    const AstraReadingListEntry& entry) {
  // TODO(astra): Incremental add instead of full rebuild.
  // For now, full rebuild is simpler and correct.
  // The section view already supports InsertItemAt for incremental updates.
  UpdateFromService();
}

void AstraSidebarReadingListView::OnReadingListEntryRemoved(const GURL& url) {
  // TODO(astra): Incremental remove instead of full rebuild.
  UpdateFromService();
}

void AstraSidebarReadingListView::OnReadingListEntryUpdated(
    const AstraReadingListEntry& entry) {
  // TODO(astra): Incremental update instead of full rebuild.
  // For an update, the item may move between unread and read groups.
  UpdateFromService();
}

void AstraSidebarReadingListView::OnReadingListModelLoaded() {
  // Model finished loading — do initial full populate.
  UpdateFromService();
}

// =========================================================================
// AstraReadingListItemDelegate
// =========================================================================

void AstraSidebarReadingListView::OnReadingListItemClicked(const GURL& url) {
  if (!reading_list_service_) {
    return;
  }

  // TODO(astra): Open the URL in the active tab.
  // This should be done through the browser's tab strip / navigation
  // controller, not directly by the sidebar.
  //
  // Options:
  //   1. Use a delegate that goes through the Browser's NavigateController
  //      or adds a new tab.
  //   2. Dispatch through AstraCommandDelegate with a command like
  //      kAstraCommandOpenReadingListEntry.
  //
  // Chromium subsystem: NavigateController / TabStripModel / Browser.
  //
  // For now, this is a stub — clicking logs an intent but doesn't navigate.
  //
  // TODO(astra): Mark the entry as read when opened? Chrome's reading list
  // UI marks items as read when opened. This is a UX decision.
}

void AstraSidebarReadingListView::OnReadingListToggleRead(const GURL& url) {
  if (!reading_list_service_) {
    return;
  }
  reading_list_service_->ToggleReadState(url);
  // The UI will update via the service observer (OnReadingListEntryUpdated).
}

void AstraSidebarReadingListView::OnReadingListRemove(const GURL& url) {
  if (!reading_list_service_) {
    return;
  }
  reading_list_service_->RemoveEntry(url);
  // The UI will update via the service observer (OnReadingListEntryRemoved).
}

// =========================================================================
// views::View overrides
// =========================================================================

gfx::Size AstraSidebarReadingListView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return views::View::CalculatePreferredSize(available_size);
}

void AstraSidebarReadingListView::OnThemeChanged() {
  views::View::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  SkColor header_color =
      color_provider->GetColor(kReadingListHeaderTextColorId);
  SkColor count_color =
      color_provider->GetColor(kReadingListCountTextColorId);

  if (unread_label_) {
    unread_label_->SetEnabledColor(header_color);
  }
  if (unread_count_label_) {
    unread_count_label_->SetEnabledColor(count_color);
  }
  if (read_label_) {
    read_label_->SetEnabledColor(header_color);
  }
  if (read_count_label_) {
    read_count_label_->SetEnabledColor(count_color);
  }
}

void AstraSidebarReadingListView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kList;
  node_data->SetName("Reading list");
}

}  // namespace astra
