#include "astra/ui/views/sidebar/astra_sidebar_recently_closed_view.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "astra/browser/astra_recent_tabs_helper.h"
#include "astra/ui/color/astra_color_ids.h"
#include "astra/ui/views/sidebar/astra_recently_closed_item_view.h"
#include "base/containers/flat_set.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/label_button.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kRecentlyClosedSectionHeaderHeight = 28;
constexpr int kRecentlyClosedSectionHorizontalPadding = 12;
constexpr int kRecentlyClosedSectionVerticalPadding = 8;
constexpr int kRecentlyClosedItemSpacing = 2;
constexpr int kRecentlyClosedFooterLinkHeight = 28;
constexpr int kRecentlyClosedFooterLinkFontSizeDelta = 0;
constexpr int kRecentlyClosedEmptyStateHeight = 40;
constexpr int kRecentlyClosedSearchHeight = 28;
constexpr int kRecentlyClosedSearchHorizontalPadding = 12;

// Section title.
const char16_t kRecentlyClosedSectionTitle[] = u"Recently closed";

// Footer link texts.
const char16_t kRestoreAllText[] = u"Restore all";
const char16_t kClearAllText[] = u"Clear all";

// Empty state texts.
const char16_t kEmptyStateText[] = u"No recently closed tabs";
const char16_t kEmptySearchText[] = u"No matching results";

// Collapsed/expanded indicator text.
const char16_t kChevronDown[] = u" ▾";
const char16_t kChevronRight[] = u" ▸";

// Astra color IDs.
constexpr ui::ColorId kRecentlyClosedHeaderTextColorId =
    kColorAstraSidebarSectionHeaderText;
constexpr ui::ColorId kRecentlyClosedFooterLinkColorId =
    kColorAstraSidebarItemText;
constexpr ui::ColorId kRecentlyClosedSecondaryTextColorId =
    kColorAstraSidebarItemSecondaryText;
constexpr ui::ColorId kRecentlyClosedSearchBgColorId =
    kColorAstraSidebarItemHoverBackground;

// Helper to check if a string contains a query (case-insensitive).
bool StringContainsQuery(const std::u16string& text,
                         const std::u16string& query) {
  if (query.empty()) {
    return true;
  }
  // Simple case-insensitive substring search.
  // TODO(astra): Use base::i18n::LowerCase for proper i18n case folding.
  std::u16 text_lower;
  std::u16 query_lower;
  for (char16_t c : text) {
    text_lower += std::towlower(c);
  }
  for (char16_t c : query) {
    query_lower += std::towlower(c);
  }
  return text_lower.find(query_lower) != std::u16string::npos;
}

}  // namespace

// =========================================================================
// Construction / destruction
// =========================================================================

AstraSidebarRecentlyClosedView::AstraSidebarRecentlyClosedView() {
  BuildLayout();
}

AstraSidebarRecentlyClosedView::AstraSidebarRecentlyClosedView(
    Profile* profile)
    : profile_(profile) {
  DCHECK(profile_);
  BuildLayout();
  Refresh();
}

AstraSidebarRecentlyClosedView::~AstraSidebarRecentlyClosedView() = default;

// =========================================================================
// Layout
// =========================================================================

void AstraSidebarRecentlyClosedView::BuildLayout() {
  // Vertical box layout for the entire section.
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);
  layout->set_between_child_spacing(0);

  // Section header button — click to collapse/expand.
  header_button_ = AddChildView(std::make_unique<views::LabelButton>(
      base::BindRepeating(
          &AstraSidebarRecentlyClosedView::OnHeaderClicked,
          base::Unretained(this)),
      kRecentlyClosedSectionTitle));
  header_button_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header_button_->SetBorder(views::CreateEmptyBorder(gfx::Insets::VH(
      kRecentlyClosedSectionVerticalPadding,
      kRecentlyClosedSectionHorizontalPadding)));
  header_button_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  // TODO(astra): Replace text-based chevron with a proper image icon.

  // Search container (hidden by default).
  search_container_ = AddChildView(std::make_unique<views::View>());
  auto* search_layout = search_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets::VH(0, kRecentlyClosedSearchHorizontalPadding), 0));
  search_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);
  search_textfield_ =
      search_container_->AddChildView(std::make_unique<views::Textfield>());
  search_textfield_->SetPlaceholderText(u"Search recently closed");
  search_textfield_->set_controller(this);
  search_textfield_->SetAccessibleName(u"Search recently closed");
  search_container_->SetVisible(false);

  // Items container.
  items_container_ = AddChildView(std::make_unique<views::View>());
  auto* items_layout = items_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets::VH(0, 4),
          kRecentlyClosedItemSpacing));
  items_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Empty state label — shown when there are no items.
  empty_state_label_ = AddChildView(std::make_unique<views::Label>(kEmptyStateText));
  empty_state_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  empty_state_label_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(kRecentlyClosedEmptyStateHeight / 2 - 8,
                      kRecentlyClosedSectionHorizontalPadding)));
  empty_state_label_->SetAutoColorReadabilityEnabled(false);
  empty_state_label_->SetVisible(false);
  empty_state_label_->SetAccessibleName(u"No recently closed tabs");

  // Footer container.
  footer_container_ = AddChildView(std::make_unique<views::View>());
  auto* footer_layout = footer_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(0, kRecentlyClosedSectionHorizontalPadding), 8));
  footer_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  // "Restore all" link.
  restore_all_link_ = footer_container_->AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(
              &AstraSidebarRecentlyClosedView::OnRestoreAllClicked,
              base::Unretained(this)),
          kRestoreAllText));
  restore_all_link_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  restore_all_link_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  restore_all_link_->SetAccessibleName(u"Restore all recently closed tabs");

  // "Clear all" link.
  clear_all_link_ = footer_container_->AddChildView(
      std::make_unique<views::LabelButton>(
          base::BindRepeating(
              &AstraSidebarRecentlyClosedView::OnClearAllClicked,
              base::Unretained(this)),
          kClearAllText));
  clear_all_link_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  clear_all_link_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  clear_all_link_->SetAccessibleName(u"Clear all recently closed tabs");
  clear_all_link_->SetVisible(false);

  // Accessibility for the whole section.
  SetAccessibleName(u"Recently closed tabs");
  SetFocusBehavior(FocusBehavior::ACCESSIBLE_ONLY);

  UpdateFooterVisibility();
  UpdateEmptyStateVisibility();
}

// =========================================================================
// Item management
// =========================================================================

void AstraSidebarRecentlyClosedView::SetRecentlyClosed(
    const std::vector<AstraRecentlyClosedItem>& items) {
  items_ = items;
  RebuildItems();
}

int AstraSidebarRecentlyClosedView::GetItemCount() const {
  return static_cast<int>(item_views_.size());
}

AstraRecentlyClosedItem AstraSidebarRecentlyClosedView::GetItemAt(
    int index) const {
  auto filtered = GetFilteredItems();
  DCHECK_GE(index, 0);
  DCHECK_LT(index, static_cast<int>(filtered.size()));
  return filtered[index];
}

void AstraSidebarRecentlyClosedView::AddItem(
    const AstraRecentlyClosedItem& item) {
  // Insert at the front (most recent first).
  items_.insert(items_.begin(), item);
  RebuildItems();
}

void AstraSidebarRecentlyClosedView::RemoveItem(int index) {
  auto filtered = GetFilteredItems();
  if (index < 0 || index >= static_cast<int>(filtered.size())) {
    return;
  }

  // Find the item in the full list by ID.
  const std::string& id = filtered[index].id;
  auto it = base::ranges::find(items_, id, &AstraRecentlyClosedItem::id);
  if (it != items_.end()) {
    items_.erase(it);
  }

  // Adjust selection if needed.
  if (selected_index_ == index) {
    selected_index_ = -1;
  } else if (selected_index_ > index) {
    selected_index_--;
  }

  RebuildItems();
}

void AstraSidebarRecentlyClosedView::ClearAll() {
  items_.clear();
  selected_index_ = -1;
  RebuildItems();
}

// =========================================================================
// Selection
// =========================================================================

void AstraSidebarRecentlyClosedView::SetSelectedItem(int index) {
  if (index < -1 || index >= static_cast<int>(item_views_.size())) {
    return;
  }
  if (selected_index_ == index) {
    return;
  }

  // Clear old selection.
  if (selected_index_ >= 0 &&
      selected_index_ < static_cast<int>(item_views_.size()) &&
      item_views_[selected_index_]) {
    item_views_[selected_index_]->SetSelected(false);
  }

  selected_index_ = index;

  // Set new selection.
  if (selected_index_ >= 0 && item_views_[selected_index_]) {
    item_views_[selected_index_]->SetSelected(true);
  }
}

int AstraSidebarRecentlyClosedView::GetSelectedIndex() const {
  return selected_index_;
}

void AstraSidebarRecentlyClosedView::ClearSelection() {
  SetSelectedItem(-1);
}

// =========================================================================
// Restore operations
// =========================================================================

void AstraSidebarRecentlyClosedView::RestoreTab(int index) {
  auto filtered = GetFilteredItems();
  if (index < 0 || index >= static_cast<int>(filtered.size())) {
    return;
  }

  const auto& item = filtered[index];
  if (item.type != AstraRecentlyClosedType::kTab) {
    // If it's a window, still restore via RestoreWindow.
    RestoreWindow(index);
    return;
  }

  if (delegate_) {
    delegate_->OnRestoreTab(item.id);
  }

  // Remove from our projection (it's been restored).
  RemoveItem(index);
}

void AstraSidebarRecentlyClosedView::RestoreWindow(int index) {
  auto filtered = GetFilteredItems();
  if (index < 0 || index >= static_cast<int>(filtered.size())) {
    return;
  }

  const auto& item = filtered[index];
  if (delegate_) {
    delegate_->OnRestoreWindow(item.id);
  }

  // Remove from our projection.
  RemoveItem(index);
}

void AstraSidebarRecentlyClosedView::RestoreAll() {
  if (delegate_) {
    delegate_->OnRestoreAllRequested();
  }
  // Clear the list after restoring all.
  ClearAll();
}

// =========================================================================
// Display options
// =========================================================================

void AstraSidebarRecentlyClosedView::SetMaxItems(int max) {
  if (max_items_ == max) {
    return;
  }
  max_items_ = max;
  RebuildItems();
}

int AstraSidebarRecentlyClosedView::GetMaxItems() const {
  return max_items_;
}

void AstraSidebarRecentlyClosedView::SetShowWindows(bool show) {
  if (show_windows_ == show) {
    return;
  }
  show_windows_ = show;
  RebuildItems();
}

bool AstraSidebarRecentlyClosedView::GetShowWindows() const {
  return show_windows_;
}

void AstraSidebarRecentlyClosedView::SetShowTabs(bool show) {
  if (show_tabs_ == show) {
    return;
  }
  show_tabs_ = show;
  RebuildItems();
}

bool AstraSidebarRecentlyClosedView::GetShowTabs() const {
  return show_tabs_;
}

void AstraSidebarRecentlyClosedView::SetShowFavicons(bool show) {
  if (show_favicons_ == show) {
    return;
  }
  show_favicons_ = show;
  ApplyDisplayOptions();
}

bool AstraSidebarRecentlyClosedView::GetShowFavicons() const {
  return show_favicons_;
}

void AstraSidebarRecentlyClosedView::SetShowTime(bool show) {
  if (show_time_ == show) {
    return;
  }
  show_time_ = show;
  ApplyDisplayOptions();
}

bool AstraSidebarRecentlyClosedView::GetShowTime() const {
  return show_time_;
}

void AstraSidebarRecentlyClosedView::SetShowTabCount(bool show) {
  if (show_tab_count_ == show) {
    return;
  }
  show_tab_count_ = show;
  ApplyDisplayOptions();
}

bool AstraSidebarRecentlyClosedView::GetShowTabCount() const {
  return show_tab_count_;
}

void AstraSidebarRecentlyClosedView::SetGroupBySession(bool group) {
  if (group_by_session_ == group) {
    return;
  }
  group_by_session_ = group;
  RebuildItems();
}

bool AstraSidebarRecentlyClosedView::GetGroupBySession() const {
  return group_by_session_;
}

// =========================================================================
// Counts
// =========================================================================

int AstraSidebarRecentlyClosedView::GetTabCount() const {
  int count = 0;
  for (const auto& item : items_) {
    if (item.type == AstraRecentlyClosedType::kWindow) {
      count += item.tab_count;
    } else {
      count += 1;
    }
  }
  return count;
}

int AstraSidebarRecentlyClosedView::GetWindowCount() const {
  int count = 0;
  for (const auto& item : items_) {
    if (item.type == AstraRecentlyClosedType::kWindow) {
      ++count;
    }
  }
  return count;
}

// =========================================================================
// Search
// =========================================================================

void AstraSidebarRecentlyClosedView::SearchRecentlyClosed(
    const std::u16string& query) {
  search_query_ = query;
  if (search_textfield_) {
    search_textfield_->SetText(query);
  }
  RebuildItems();

  if (delegate_) {
    delegate_->OnSearch(query);
  }
}

int AstraSidebarRecentlyClosedView::GetSearchResultsCount() const {
  if (search_query_.empty()) {
    return GetItemCount();
  }
  return static_cast<int>(item_views_.size());
}

void AstraSidebarRecentlyClosedView::SetShowSearch(bool show) {
  if (show_search_ == show) {
    return;
  }
  show_search_ = show;
  if (search_container_) {
    search_container_->SetVisible(show);
  }
  InvalidateLayout();
}

bool AstraSidebarRecentlyClosedView::GetShowSearch() const {
  return show_search_;
}

// =========================================================================
// Footer buttons
// =========================================================================

void AstraSidebarRecentlyClosedView::SetShowRestoreAllButton(bool show) {
  if (show_restore_all_button_ == show) {
    return;
  }
  show_restore_all_button_ = show;
  UpdateFooterVisibility();
}

bool AstraSidebarRecentlyClosedView::GetShowRestoreAllButton() const {
  return show_restore_all_button_;
}

void AstraSidebarRecentlyClosedView::SetShowClearAllButton(bool show) {
  if (show_clear_all_button_ == show) {
    return;
  }
  show_clear_all_button_ = show;
  UpdateFooterVisibility();
}

bool AstraSidebarRecentlyClosedView::GetShowClearAllButton() const {
  return show_clear_all_button_;
}

// =========================================================================
// View access
// =========================================================================

AstraRecentlyClosedItemView* AstraSidebarRecentlyClosedView::GetItemViewAt(
    int index) {
  DCHECK_GE(index, 0);
  DCHECK_LT(index, static_cast<int>(item_views_.size()));
  return item_views_[index];
}

// =========================================================================
// Section expansion
// =========================================================================

void AstraSidebarRecentlyClosedView::SetExpanded(bool expanded) {
  if (is_expanded_ == expanded) {
    return;
  }
  is_expanded_ = expanded;

  bool has_items = !item_views_.empty();

  // Toggle visibility of content.
  if (items_container_) {
    items_container_->SetVisible(is_expanded_ && has_items);
  }
  if (footer_container_) {
    footer_container_->SetVisible(is_expanded_ && has_items &&
      (show_restore_all_button_ || show_clear_all_button_));
  }
  if (search_container_) {
    search_container_->SetVisible(is_expanded_ && show_search_);
  }

  // Show/hide empty state.
  UpdateEmptyStateVisibility();

  // Update header chevron.
  std::u16string title = kRecentlyClosedSectionTitle;
  title += is_expanded_ ? std::u16string(kChevronDown)
                        : std::u16string(kChevronRight);
  header_button_->SetText(title);

  InvalidateLayout();
}

bool AstraSidebarRecentlyClosedView::HasItems() const {
  return !item_views_.empty();
}

// =========================================================================
// Refresh (TabRestoreService integration)
// =========================================================================

void AstraSidebarRecentlyClosedView::Refresh() {
  if (!profile_) {
    return;
  }

  // Get recently closed tabs from the helper.
  // TODO(astra): Use TabRestoreService directly for full data including
  // windows, sessions, and proper entry IDs.
  // Chromium owner: sessions::TabRestoreService
  //   (chrome/browser/sessions/tab_restore_service.h)
  std::vector<AstraRecentlyClosedTab> tabs =
      AstraRecentTabsHelper::GetRecentlyClosedTabs(profile_, max_items_);

  // Convert to our item format.
  std::vector<AstraRecentlyClosedItem> items;
  items.reserve(tabs.size());
  for (const auto& tab : tabs) {
    AstraRecentlyClosedItem item;
    item.id = base::NumberToString(tab.entry_id);
    item.title = tab.title;
    item.url = tab.url;
    item.type = AstraRecentlyClosedType::kTab;
    item.close_time = tab.close_time;
    item.tab_count = 1;
    item.has_favicon = false;
    item.session_id = 0;
    item.is_incognito = false;
    items.push_back(std::move(item));
  }

  SetRecentlyClosed(items);
}

// =========================================================================
// Internal helpers
// =========================================================================

void AstraSidebarRecentlyClosedView::ClearItems() {
  if (items_container_) {
    items_container_->RemoveAllChildViews();
  }
  item_views_.clear();
}

void AstraSidebarRecentlyClosedView::RebuildItems() {
  ClearItems();

  auto filtered = GetFilteredItems();

  for (size_t i = 0; i < filtered.size(); ++i) {
    const auto& item = filtered[i];
    auto item_view = std::make_unique<AstraRecentlyClosedItemView>(
        item.title, item.url, item.close_time,
        0 /* entry_id, legacy */);

    item_view->SetIsWindow(item.type == AstraRecentlyClosedType::kWindow);
    item_view->SetTabCount(item.tab_count);
    item_view->SetId(item.id);

    // Click handler.
    int index = static_cast<int>(i);
    item_view->set_callback(base::BindRepeating(
        &AstraSidebarRecentlyClosedView::OnItemClicked,
        base::Unretained(this), index));

    // Set selection state.
    item_view->SetSelected(index == selected_index_);

    // Apply favicon visibility.
    if (!show_favicons_) {
      // TODO(astra): Hide the icon view properly.
    }

    raw_ptr<AstraRecentlyClosedItemView> raw = item_view.get();
    item_views_.push_back(raw);
    items_container_->AddChildView(std::move(item_view));
  }

  ApplyDisplayOptions();
  UpdateFooterVisibility();
  UpdateEmptyStateVisibility();

  // Update container visibility based on expansion.
  if (items_container_) {
    items_container_->SetVisible(is_expanded_ && !item_views_.empty());
  }

  InvalidateLayout();
}

std::vector<AstraRecentlyClosedItem>
AstraSidebarRecentlyClosedView::GetFilteredItems() const {
  std::vector<AstraRecentlyClosedItem> result;
  result.reserve(items_.size());

  for (const auto& item : items_) {
    // Filter by type.
    if (item.type == AstraRecentlyClosedType::kTab && !show_tabs_) {
      continue;
    }
    if (item.type == AstraRecentlyClosedType::kWindow && !show_windows_) {
      continue;
    }

    // Filter by search query.
    if (!search_query_.empty()) {
      bool matches_title = StringContainsQuery(item.title, search_query_);
      bool matches_url = StringContainsQuery(
          base::UTF8ToUTF16(item.url.spec()), search_query_);
      if (!matches_title && !matches_url) {
        continue;
      }
    }

    result.push_back(item);

    // Apply max items limit.
    if (static_cast<int>(result.size()) >= max_items_) {
      break;
    }
  }

  // Note: group_by_session_ affects display grouping, not filtering.
  // Items are kept in chronological order (newest first).

  return result;
}

void AstraSidebarRecentlyClosedView::ApplyDisplayOptions() {
  for (auto* item_view : item_views_) {
    if (!item_view) {
      continue;
    }
    // These options affect the item's display.
    // TODO(astra): Implement proper control over individual elements.
    item_view->SetVisible(true);
  }
}

void AstraSidebarRecentlyClosedView::UpdateFooterVisibility() {
  bool has_items = !item_views_.empty();
  bool show_footer = is_expanded_ && has_items &&
      (show_restore_all_button_ || show_clear_all_button_);

  if (footer_container_) {
    footer_container_->SetVisible(show_footer);
  }
  if (restore_all_link_) {
    restore_all_link_->SetVisible(show_restore_all_button_);
  }
  if (clear_all_link_) {
    clear_all_link_->SetVisible(show_clear_all_button_);
  }
}

void AstraSidebarRecentlyClosedView::UpdateEmptyStateVisibility() {
  bool show_empty = is_expanded_ && item_views_.empty();

  if (empty_state_label_) {
    // Show search-specific empty message if searching.
    if (!search_query_.empty()) {
      empty_state_label_->SetText(kEmptySearchText);
    } else {
      empty_state_label_->SetText(kEmptyStateText);
    }
    empty_state_label_->SetVisible(show_empty);
  }
}

// =========================================================================
// Handlers for user actions
// =========================================================================

void AstraSidebarRecentlyClosedView::OnHeaderClicked() {
  SetExpanded(!is_expanded_);
}

void AstraSidebarRecentlyClosedView::OnRestoreAllClicked() {
  RestoreAll();
}

void AstraSidebarRecentlyClosedView::OnClearAllClicked() {
  if (delegate_) {
    delegate_->OnClearAllRequested();
  }
  ClearAll();
}

void AstraSidebarRecentlyClosedView::OnItemClicked(int index) {
  auto filtered = GetFilteredItems();
  if (index < 0 || index >= static_cast<int>(filtered.size())) {
    return;
  }

  SetSelectedItem(index);

  const auto& item = filtered[index];
  if (delegate_) {
    delegate_->OnItemClicked(item.id);
  }

  // Clicking an item restores it.
  if (item.type == AstraRecentlyClosedType::kWindow) {
    RestoreWindow(index);
  } else {
    RestoreTab(index);
  }
}

void AstraSidebarRecentlyClosedView::OnSearchTextChanged() {
  if (search_textfield_) {
    SearchRecentlyClosed(search_textfield_->GetText());
  }
}

// =========================================================================
// views::View overrides
// =========================================================================

gfx::Size AstraSidebarRecentlyClosedView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return views::View::CalculatePreferredSize(available_size);
}

void AstraSidebarRecentlyClosedView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kList;
  node_data->SetName("Recently closed tabs");
}

void AstraSidebarRecentlyClosedView::OnThemeChanged() {
  views::View::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  // Section header color.
  if (header_button_) {
    header_button_->SetEnabledTextColors(
        color_provider->GetColor(kRecentlyClosedHeaderTextColorId));
  }

  // Empty state label color.
  if (empty_state_label_) {
    empty_state_label_->SetEnabledColor(
        color_provider->GetColor(kRecentlyClosedSecondaryTextColorId));
  }

  // Footer link color.
  if (restore_all_link_) {
    restore_all_link_->SetEnabledTextColors(
        color_provider->GetColor(kRecentlyClosedFooterLinkColorId));
  }
  if (clear_all_link_) {
    clear_all_link_->SetEnabledTextColors(
        color_provider->GetColor(kRecentlyClosedFooterLinkColorId));
  }

  // Search box background.
  if (search_textfield_) {
    // TODO(astra): Style the search field properly.
  }
}

}  // namespace astra
