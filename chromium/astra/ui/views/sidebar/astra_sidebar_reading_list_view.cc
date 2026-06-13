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
constexpr int kSearchFieldHeight = 28;
constexpr int kSearchFieldHorizontalPadding = 12;
constexpr int kReadingListMainHeaderFontSizeDelta = 2;

// Astra color IDs for the reading list panel.
// Uses the Astra sidebar color system from astra/ui/color/astra_color_ids.h.
// Chromium subsystem: ui::ColorProvider (ui/color/color_provider.h)
constexpr ui::ColorId kReadingListHeaderTextColorId =
    kColorAstraSidebarSectionHeaderText;
constexpr ui::ColorId kReadingListCountTextColorId =
    kColorAstraSidebarItemSecondaryText;
constexpr ui::ColorId kReadingListEmptyTextColorId =
    kColorAstraSidebarItemSecondaryText;
constexpr ui::ColorId kReadingListMainHeaderTextColorId =
    kColorAstraSidebarSectionHeaderText;

// Titles.
const char16_t kReadingListTitle[] = u"Reading List";
const char16_t kUnreadTitle[] = u"Unread";
const char16_t kReadTitle[] = u"Read";
const char16_t kSearchPlaceholder[] = u"Search reading list...";
const char16_t kNoUnreadLabel[] = u"No unread items";
const char16_t kNoReadLabel[] = u"No read items";
const char16_t kAddButtonLabel[] = u"+";
const char16_t kLoadingText[] = u"Loading...";

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

  // -- Main header row: title + add button --

  header_row_ = AddChildView(std::make_unique<views::View>());
  auto* header_layout = header_row_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(kReadingListHeaderVerticalPadding,
                          kReadingListHeaderHorizontalPadding),
          8));
  header_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  header_label_ = header_row_->AddChildView(
      std::make_unique<views::Label>(kReadingListTitle));
  header_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header_label_->SetAutoColorReadabilityEnabled(false);
  header_label_->SetFontList(
      header_label_->font_list().DeriveWithSizeDelta(
          kReadingListMainHeaderFontSizeDelta).DeriveWithWeight(
              gfx::Font::Weight::MEDIUM));
  header_layout->SetFlexForView(header_label_, 1);

  add_button_ = header_row_->AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(
              &AstraSidebarReadingListView::OnAddButtonPressed,
              base::Unretained(this)),
          kAddButtonLabel));
  add_button_->SetTooltipText(u"Add to reading list");
  add_button_->SetAccessibleName(u"Add to reading list");

  // -- Search field --

  search_field_ = AddChildView(std::make_unique<views::Textfield>());
  search_field_->SetPlaceholderText(kSearchPlaceholder);
  search_field_->SetController(this);
  search_field_->SetAccessibleName(u"Search reading list");
  search_field_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(0, kSearchFieldHorizontalPadding)));
  search_field_->SetPreferredSize(gfx::Size(0, kSearchFieldHeight));

  // -- Loading state --

  loading_view_ = AddChildView(std::make_unique<views::View>());
  auto* loading_layout = loading_view_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets::VH(16, kReadingListHeaderHorizontalPadding), 0));
  loading_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  auto* loading_label = loading_view_->AddChildView(
      std::make_unique<views::Label>(kLoadingText));
  loading_label->SetAutoColorReadabilityEnabled(false);
  loading_view_->SetVisible(false);

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

  // Unread empty state.
  unread_empty_label_ = AddChildView(std::make_unique<views::Label>(kNoUnreadLabel));
  unread_empty_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  unread_empty_label_->SetAutoColorReadabilityEnabled(false);
  unread_empty_label_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(4, kReadingListHeaderHorizontalPadding + 4)));
  unread_empty_label_->SetVisible(false);

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

  // Read empty state.
  read_empty_label_ = AddChildView(std::make_unique<views::Label>(kNoReadLabel));
  read_empty_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  read_empty_label_->SetAutoColorReadabilityEnabled(false);
  read_empty_label_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(4, kReadingListHeaderHorizontalPadding + 4)));
  read_empty_label_->SetVisible(false);

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

  // Populate from service (with sort/filter/search applied).
  PopulateUnreadItems();
  PopulateReadItems();

  // Update count labels and empty states.
  UpdateCountLabels();
  UpdateEmptyStates();

  // Show/hide read group based on whether there are any read items.
  size_t read_count = read_items_container_->children().size();
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

  auto entries = GetFilteredUnreadEntries();
  for (const auto& entry : entries) {
    unread_items_container_->AddChildView(CreateItemView(entry));
  }
}

void AstraSidebarReadingListView::PopulateReadItems() {
  if (!reading_list_service_) {
    return;
  }

  auto entries = GetFilteredReadEntries();
  for (const auto& entry : entries) {
    read_items_container_->AddChildView(CreateItemView(entry));
  }
}

std::unique_ptr<AstraReadingListItemView>
AstraSidebarReadingListView::CreateItemView(
    const AstraReadingListEntry& entry) {
  std::u16 title = base::UTF8ToUTF16(entry.title);
  std::u16 domain = GetDomainDisplayString(entry.url);

  bool is_read = entry.status == AstraReadingListStatus::kRead;
  auto item = std::make_unique<AstraReadingListItemView>(
      entry.url, title, domain, is_read);
  item->set_delegate(this);
  item->SetEstimatedReadTime(entry.estimated_read_time);
  if (entry.word_count > 0) {
    item->SetWordCount(entry.word_count);
  }

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
  if (delegate_) {
    delegate_->OnReadingListItemOpen(url, /*open_in_new_tab=*/false);
  }

  // TODO(astra): Mark the entry as read when opened? Chrome's reading list
  // UI marks items as read when opened. This is a UX decision.
  // For now, we leave the read state unchanged.
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
  SkColor empty_color =
      color_provider->GetColor(kReadingListEmptyTextColorId);
  SkColor main_header_color =
      color_provider->GetColor(kReadingListMainHeaderTextColorId);

  if (header_label_) {
    header_label_->SetEnabledColor(main_header_color);
  }
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
  if (unread_empty_label_) {
    unread_empty_label_->SetEnabledColor(empty_color);
  }
  if (read_empty_label_) {
    read_empty_label_->SetEnabledColor(empty_color);
  }
}

void AstraSidebarReadingListView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kList;
  node_data->SetName("Reading list");
}

// =========================================================================
// Search
// =========================================================================

void AstraSidebarReadingListView::SearchEntries(const std::string& query) {
  if (search_field_) {
    search_field_->SetText(base::UTF8ToUTF16(query));
  }
  // UpdateFromService will be called by ContentsChanged if the text changed.
  // If called programmatically with the same text, force update.
  if (delegate_) {
    delegate_->OnReadingListSearch(query);
  }
  UpdateFromService();
}

std::string AstraSidebarReadingListView::GetSearchQuery() const {
  if (!search_field_) {
    return std::string();
  }
  return base::UTF16ToUTF8(search_field_->GetText());
}

// =========================================================================
// Sorting
// =========================================================================

void AstraSidebarReadingListView::SetSortOrder(AstraReadingListSortOrder order) {
  if (sort_order_ == order) {
    return;
  }
  sort_order_ = order;
  UpdateFromService();
}

// =========================================================================
// Filter
// =========================================================================

void AstraSidebarReadingListView::SetFilter(AstraReadingListView filter) {
  if (filter_ == filter) {
    return;
  }
  filter_ = filter;
  UpdateFromService();
}

// =========================================================================
// Selection
// =========================================================================

void AstraSidebarReadingListView::SetSelectedItem(const GURL& url) {
  if (selected_url_ == url) {
    return;
  }

  // Clear old selection.
  if (!selected_url_.is_empty()) {
    auto* old_item = FindItemInContainer(unread_items_container_, selected_url_);
    if (!old_item) {
      old_item = FindItemInContainer(read_items_container_, selected_url_);
    }
    if (old_item) {
      old_item->SetSelected(false);
    }
  }

  selected_url_ = url;

  // Set new selection.
  if (!selected_url_.is_empty()) {
    auto* new_item = FindItemInContainer(unread_items_container_, selected_url_);
    if (!new_item) {
      new_item = FindItemInContainer(read_items_container_, selected_url_);
    }
    if (new_item) {
      new_item->SetSelected(true);
    }
  }
}

void AstraSidebarReadingListView::ClearSelection() {
  SetSelectedItem(GURL());
}

// =========================================================================
// Bulk operations
// =========================================================================

void AstraSidebarReadingListView::MarkAllAsRead() {
  if (!reading_list_service_) {
    return;
  }
  auto entries = GetFilteredUnreadEntries();
  for (const auto& entry : entries) {
    if (entry.status != AstraReadingListStatus::kRead) {
      reading_list_service_->SetReadStatus(entry.url, true);
    }
  }
  // UI updates via observer notifications.
}

void AstraSidebarReadingListView::DeleteAllRead() {
  if (!reading_list_service_) {
    return;
  }
  auto entries = GetFilteredReadEntries();
  for (const auto& entry : entries) {
    reading_list_service_->RemoveEntry(entry.url);
  }
  // UI updates via observer notifications.
}

// =========================================================================
// Loading state
// =========================================================================

void AstraSidebarReadingListView::SetLoading(bool loading) {
  if (is_loading_ == loading) {
    return;
  }
  is_loading_ = loading;
  UpdateLoadingState();
}

void AstraSidebarReadingListView::UpdateLoadingState() {
  if (loading_view_) {
    loading_view_->SetVisible(is_loading_);
  }
  if (unread_header_) {
    unread_header_->SetVisible(!is_loading_);
  }
  if (unread_items_container_) {
    unread_items_container_->SetVisible(!is_loading_ && unread_expanded_);
  }
  if (read_header_) {
    read_header_->SetVisible(!is_loading_ &&
                             read_items_container_ &&
                             read_items_container_->children().size() > 0);
  }
}

// =========================================================================
// Count getters
// =========================================================================

int AstraSidebarReadingListView::GetTotalItemCount() const {
  return GetUnreadCount() + GetReadCount();
}

int AstraSidebarReadingListView::GetUnreadCount() const {
  if (!unread_items_container_) {
    return 0;
  }
  return static_cast<int>(unread_items_container_->children().size());
}

int AstraSidebarReadingListView::GetReadCount() const {
  if (!read_items_container_) {
    return 0;
  }
  return static_cast<int>(read_items_container_->children().size());
}

// =========================================================================
// AstraReadingListServiceObserver (additional)
// =========================================================================

void AstraSidebarReadingListView::OnReadingListEntryStatusChanged(
    const GURL& url, bool is_read) {
  // Convenience event — full update also fires via OnReadingListEntryUpdated.
  // For now, just do a full rebuild.
  // TODO(astra): Incremental update for status changes.
  UpdateFromService();
}

void AstraSidebarReadingListView::OnReadingListReordered() {
  UpdateFromService();
}

void AstraSidebarReadingListView::OnReadingListReloaded() {
  UpdateFromService();
}

// =========================================================================
// TextfieldController (search box)
// =========================================================================

void AstraSidebarReadingListView::ContentsChanged(
    views::Textfield* sender, const std::u16string& new_contents) {
  if (delegate_) {
    delegate_->OnReadingListSearch(base::UTF16ToUTF8(new_contents));
  }
  UpdateFromService();
}

// =========================================================================
// Filtered entries helpers
// =========================================================================

std::vector<AstraReadingListEntry>
AstraSidebarReadingListView::GetFilteredUnreadEntries() const {
  if (!reading_list_service_) {
    return std::vector<AstraReadingListEntry>();
  }

  std::vector<AstraReadingListEntry> entries;
  std::string query = GetSearchQuery();

  // Apply view filter.
  switch (filter_) {
    case AstraReadingListView::kAll:
    case AstraReadingListView::kUnread:
      entries = reading_list_service_->GetUnreadEntries();
      break;
    case AstraReadingListView::kFavorites: {
      auto all = reading_list_service_->GetFavoriteEntries();
      for (const auto& e : all) {
        if (e.status != AstraReadingListStatus::kRead) {
          entries.push_back(e);
        }
      }
      break;
    }
    case AstraReadingListView::kFolders:
      // TODO(astra): Folder-based view. For now, fall back to all unread.
      entries = reading_list_service_->GetUnreadEntries();
      break;
  }

  // Apply search filter.
  if (!query.empty()) {
    std::vector<AstraReadingListEntry> filtered;
    std::string query_lower = base::ToLowerASCII(query);
    for (const auto& entry : entries) {
      std::string title_lower = base::ToLowerASCII(entry.title);
      std::string url_lower = base::ToLowerASCII(entry.url.spec());
      if (title_lower.find(query_lower) != std::string::npos ||
          url_lower.find(query_lower) != std::string::npos) {
        filtered.push_back(entry);
      }
    }
    entries = std::move(filtered);
  }

  // Apply sort order.
  if (sort_order_ != AstraReadingListSortOrder::kByDateAdded) {
    // TODO(astra): Use service's SortEntries method when available.
    // For now, do a simple client-side sort.
    switch (sort_order_) {
      case AstraReadingListSortOrder::kByTitle: {
        std::sort(entries.begin(), entries.end(),
                  [](const AstraReadingListEntry& a,
                     const AstraReadingListEntry& b) {
                    return base::CompareCaseInsensitiveASCII(a.title, b.title) < 0;
                  });
        break;
      }
      case AstraReadingListSortOrder::kByEstimatedReadTime: {
        std::sort(entries.begin(), entries.end(),
                  [](const AstraReadingListEntry& a,
                     const AstraReadingListEntry& b) {
                    return a.estimated_read_time < b.estimated_read_time;
                  });
        break;
      }
      case AstraReadingListSortOrder::kByDateRead:
      case AstraReadingListSortOrder::kByDateAdded:
      default:
        // Already sorted by date added (newest first) from the service.
        break;
    }
  }

  return entries;
}

std::vector<AstraReadingListEntry>
AstraSidebarReadingListView::GetFilteredReadEntries() const {
  if (!reading_list_service_) {
    return std::vector<AstraReadingListEntry>();
  }

  std::vector<AstraReadingListEntry> entries;
  std::string query = GetSearchQuery();

  // Apply view filter.
  switch (filter_) {
    case AstraReadingListView::kAll:
    case AstraReadingListView::kUnread:
      entries = reading_list_service_->GetReadEntries();
      break;
    case AstraReadingListView::kFavorites: {
      auto all = reading_list_service_->GetFavoriteEntries();
      for (const auto& e : all) {
        if (e.status == AstraReadingListStatus::kRead) {
          entries.push_back(e);
        }
      }
      break;
    }
    case AstraReadingListView::kFolders:
      // TODO(astra): Folder-based view. For now, fall back to all read.
      entries = reading_list_service_->GetReadEntries();
      break;
  }

  // Apply search filter.
  if (!query.empty()) {
    std::vector<AstraReadingListEntry> filtered;
    std::string query_lower = base::ToLowerASCII(query);
    for (const auto& entry : entries) {
      std::string title_lower = base::ToLowerASCII(entry.title);
      std::string url_lower = base::ToLowerASCII(entry.url.spec());
      if (title_lower.find(query_lower) != std::string::npos ||
          url_lower.find(query_lower) != std::string::npos) {
        filtered.push_back(entry);
      }
    }
    entries = std::move(filtered);
  }

  // Apply sort order.
  if (sort_order_ != AstraReadingListSortOrder::kByDateAdded) {
    switch (sort_order_) {
      case AstraReadingListSortOrder::kByTitle: {
        std::sort(entries.begin(), entries.end(),
                  [](const AstraReadingListEntry& a,
                     const AstraReadingListEntry& b) {
                    return base::CompareCaseInsensitiveASCII(a.title, b.title) < 0;
                  });
        break;
      }
      case AstraReadingListSortOrder::kByEstimatedReadTime: {
        std::sort(entries.begin(), entries.end(),
                  [](const AstraReadingListEntry& a,
                     const AstraReadingListEntry& b) {
                    return a.estimated_read_time < b.estimated_read_time;
                  });
        break;
      }
      case AstraReadingListSortOrder::kByDateRead: {
        std::sort(entries.begin(), entries.end(),
                  [](const AstraReadingListEntry& a,
                     const AstraReadingListEntry& b) {
                    return a.last_read_time > b.last_read_time;
                  });
        break;
      }
      case AstraReadingListSortOrder::kByDateAdded:
      default:
        break;
    }
  }

  return entries;
}

// =========================================================================
// Empty states
// =========================================================================

void AstraSidebarReadingListView::UpdateEmptyStates() {
  if (!unread_empty_label_ || !read_empty_label_) {
    return;
  }

  size_t unread_count = unread_items_container_
                            ? unread_items_container_->children().size()
                            : 0;
  size_t read_count = read_items_container_
                          ? read_items_container_->children().size()
                          : 0;

  unread_empty_label_->SetVisible(unread_count == 0 && !is_loading_);
  read_empty_label_->SetVisible(read_count == 0 && !is_loading_ &&
                                read_header_ && read_header_->GetVisible());
}

// =========================================================================
// Count labels
// =========================================================================

void AstraSidebarReadingListView::UpdateCountLabels() {
  if (!unread_count_label_ || !read_count_label_) {
    return;
  }

  size_t unread_count = unread_items_container_
                            ? unread_items_container_->children().size()
                            : 0;
  size_t read_count = read_items_container_
                          ? read_items_container_->children().size()
                          : 0;

  unread_count_label_->SetText(base::NumberToString16(unread_count));
  read_count_label_->SetText(base::NumberToString16(read_count));
}

// =========================================================================
// Header click handlers
// =========================================================================

void AstraSidebarReadingListView::OnUnreadHeaderClicked() {
  SetUnreadExpanded(!unread_expanded_);
}

void AstraSidebarReadingListView::OnReadHeaderClicked() {
  SetReadExpanded(!read_expanded_);
}

// =========================================================================
// Add button handler
// =========================================================================

void AstraSidebarReadingListView::OnAddButtonPressed() {
  if (delegate_) {
    delegate_->OnAddCurrentPageRequested();
  }
}

}  // namespace astra
