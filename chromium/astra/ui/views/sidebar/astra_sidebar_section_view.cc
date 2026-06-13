#include "astra/ui/views/sidebar/astra_sidebar_section_view.h"

#include <memory>
#include <utility>

#include "base/check.h"
#include "base/i18n/number_formatting.h"
#include "base/strings/utf_string_conversions.h"
#include "astra/ui/color/astra_color_ids.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"

#include "astra/ui/views/sidebar/astra_sidebar_drag_types.h"
#include "astra/ui/views/sidebar/astra_sidebar_drop_indicator_view.h"
#include "astra/ui/views/sidebar/astra_sidebar_item_view.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kSectionHeaderHeight = 28;
constexpr int kSectionHorizontalPadding = 12;
constexpr int kSectionVerticalPadding = 8;
constexpr int kSectionItemSpacing = 2;
constexpr int kSectionHeaderFontSizeDelta = 1;
constexpr int kChevronSize = 12;
constexpr int kHeaderIconSize = 16;
constexpr int kCountBadgePaddingHorizontal = 6;
constexpr int kCountBadgePaddingVertical = 1;
constexpr int kCountBadgeRadius = 8;
constexpr int kSearchFieldHeight = 24;
constexpr int kAddButtonSize = 20;
constexpr int kMoreButtonSize = 20;
constexpr int kFooterHeight = 28;
constexpr int kFooterFontSizeDelta = 0;
constexpr int kLoadingStateHeight = 40;
constexpr int kEmptyStateHeight = 48;

// Default empty state text.
const char16_t kDefaultEmptyText[] = u"No items";
const char16_t kLoadingText[] = u"Loading...";
const char16_t kShowMoreText[] = u"Show more";

// Astra color IDs.
// Chromium subsystem: ui::ColorProvider (ui/color/color_provider.h)
// Astra color system: astra/ui/color/astra_color_ids.h
constexpr ui::ColorId kSectionHeaderTextColorId =
    kColorAstraSidebarSectionHeaderText;
constexpr ui::ColorId kSectionSecondaryTextColorId =
    kColorAstraSidebarItemSecondaryText;
constexpr ui::ColorId kSectionBackgroundColorId =
    kColorAstraSidebarBackground;
constexpr ui::ColorId kSectionCountBadgeBgColorId =
    kColorAstraSidebarBadgeBackground;
constexpr ui::ColorId kSectionCountBadgeTextColorId =
    kColorAstraSidebarBadgeText;
constexpr ui::ColorId kSectionFooterTextColorId =
    kColorAstraSidebarItemText;

}  // namespace

// =========================================================================
// Construction and layout
// =========================================================================

AstraSidebarSectionView::AstraSidebarSectionView(
    const std::u16string& title,
    AstraSidebarSectionType type)
    : title_(title), section_type_(type) {
  // Vertical box layout for the whole section: header + content + footer.
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  layout->set_between_child_spacing(0);
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  BuildHeader();
  BuildContent();
  BuildFooter();

  // Drop indicator — positioned manually, not managed by the layout.
  // Shown during drag-and-drop to indicate where a dragged item will land.
  drop_indicator_ =
      AddChildView(std::make_unique<AstraSidebarDropIndicatorView>());

  // Accessibility: treat the section as a group with the header as its name.
  SetAccessibleName(title_);
}

AstraSidebarSectionView::~AstraSidebarSectionView() = default;

void AstraSidebarSectionView::BuildHeader() {
  header_view_ = AddChildView(std::make_unique<views::View>());
  auto* header_layout = header_view_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(kSectionVerticalPadding,
                          kSectionHorizontalPadding),
          0));
  header_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  header_view_->SetPreferredSize(gfx::Size(0, kSectionHeaderHeight));

  // Section icon (initially hidden).
  header_icon_ = header_view_->AddChildView(std::make_unique<views::ImageView>());
  header_icon_->SetPreferredSize(gfx::Size(kHeaderIconSize, kHeaderIconSize));
  header_icon_->SetVisible(false);

  // Chevron (expand/collapse).
  chevron_view_ = header_view_->AddChildView(std::make_unique<views::ImageView>());
  chevron_view_->SetPreferredSize(gfx::Size(kChevronSize, kChevronSize));
  // TODO(astra): Set chevron vector icon from ui/resources/vector_icons/.
  // Chromium owner: ui/views/vector_icons/chevron_*.h

  // Title label.
  header_label_ = header_view_->AddChildView(std::make_unique<views::Label>(title_));
  header_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header_label_->SetAutoColorReadabilityEnabled(false);
  header_label_->SetFontList(
      header_label_->font_list().DeriveWithSizeDelta(
          kSectionHeaderFontSizeDelta));
  header_layout->SetFlexForView(header_label_, 1);

  // Item count badge (initially hidden).
  count_badge_ = header_view_->AddChildView(std::make_unique<views::Label>());
  count_badge_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  count_badge_->SetAutoColorReadabilityEnabled(false);
  count_badge_->SetVisible(false);
  // TODO(astra): Style the count badge with rounded background.
  // Chromium pattern: views::Label with rounded background via
  // views::Painter or SetBorder with custom insets.

  // Search field (initially hidden).
  search_field_ =
      header_view_->AddChildView(std::make_unique<views::Textfield>());
  search_field_->SetPlaceholderText(u"Search...");
  search_field_->SetPreferredSize(gfx::Size(120, kSearchFieldHeight));
  search_field_->SetVisible(false);
  search_field_->SetAccessibleName(u"Search section");
  // TODO(astra): Connect text changed callback.
  // search_field_->SetTextChangedCallback(base::BindRepeating(...));

  // Add button (initially hidden).
  add_button_ = header_view_->AddChildView(
      std::make_unique<views::ImageButton>(base::BindRepeating(
          &AstraSidebarSectionView::OnAddButtonClicked,
          base::Unretained(this))));
  add_button_->SetPreferredSize(gfx::Size(kAddButtonSize, kAddButtonSize));
  add_button_->SetVisible(false);
  add_button_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  add_button_->SetAccessibleName(u"Add item");
  // TODO(astra): Set plus icon from vector resources.

  // More button (initially hidden).
  more_button_ = header_view_->AddChildView(
      std::make_unique<views::ImageButton>(base::BindRepeating(
          &AstraSidebarSectionView::OnMoreButtonClicked,
          base::Unretained(this))));
  more_button_->SetPreferredSize(gfx::Size(kMoreButtonSize, kMoreButtonSize));
  more_button_->SetVisible(false);
  more_button_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  more_button_->SetAccessibleName(u"More options");
  // TODO(astra): Set more-vert icon from vector resources.

  UpdateHeaderVisibility();
}

void AstraSidebarSectionView::BuildContent() {
  // Scroll view wrapping the items area.
  scroll_view_ = AddChildView(std::make_unique<views::ScrollView>());
  scroll_view_->SetClipBounds(true);
  scroll_view_->SetBackgroundColor(SK_ColorTRANSPARENT);
  static_cast<views::BoxLayout*>(GetLayoutManager())
      ->SetFlexForView(scroll_view_, 1);

  // Content view holds items + empty state + loading state (stacked).
  content_view_ = scroll_view_->SetContents(std::make_unique<views::View>());
  auto* content_layout = content_view_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets::VH(0, 4), kSectionItemSpacing));
  content_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Items container.
  items_container_ = content_view_->AddChildView(std::make_unique<views::View>());
  auto* items_layout = items_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets::VH(0, 0),
          kSectionItemSpacing));
  items_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Empty state label (initially shown).
  empty_state_label_ =
      content_view_->AddChildView(std::make_unique<views::Label>(kDefaultEmptyText));
  empty_state_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  empty_state_label_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(kEmptyStateHeight / 2 - 8, kSectionHorizontalPadding)));
  empty_state_label_->SetAutoColorReadabilityEnabled(false);
  empty_state_label_->SetVisible(true);

  // Loading state view (initially hidden).
  loading_state_view_ =
      content_view_->AddChildView(std::make_unique<views::View>());
  auto* loading_layout = loading_state_view_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal));
  loading_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  loading_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kCenter);
  loading_state_view_->SetPreferredSize(gfx::Size(0, kLoadingStateHeight));
  auto* loading_label = loading_state_view_->AddChildView(
      std::make_unique<views::Label>(kLoadingText));
  loading_label->SetAutoColorReadabilityEnabled(false);
  loading_state_view_->SetVisible(false);

  UpdateContentState();
}

void AstraSidebarSectionView::BuildFooter() {
  footer_view_ = AddChildView(std::make_unique<views::View>());
  auto* footer_layout = footer_view_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(0, kSectionHorizontalPadding),
          0));
  footer_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  footer_view_->SetPreferredSize(gfx::Size(0, kFooterHeight));
  footer_view_->SetVisible(false);  // Hidden by default.

  // "Show more" link.
  show_more_label_ =
      footer_view_->AddChildView(std::make_unique<views::Label>(kShowMoreText));
  show_more_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  show_more_label_->SetAutoColorReadabilityEnabled(false);
  show_more_label_->SetFontList(
      show_more_label_->font_list().DeriveWithSizeDelta(
          kFooterFontSizeDelta));
  footer_layout->SetFlexForView(show_more_label_, 1);

  // Status text (right-aligned).
  status_label_ = footer_view_->AddChildView(std::make_unique<views::Label>());
  status_label_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  status_label_->SetAutoColorReadabilityEnabled(false);
  status_label_->SetVisible(false);
}

// =========================================================================
// Title
// =========================================================================

void AstraSidebarSectionView::SetTitle(const std::u16string& title) {
  if (title_ == title) {
    return;
  }
  title_ = title;
  if (header_label_) {
    header_label_->SetText(title_);
  }
  SetAccessibleName(title_);
}

// =========================================================================
// Expand / collapse
// =========================================================================

void AstraSidebarSectionView::SetExpanded(bool expanded) {
  if (is_expanded_ == expanded) {
    return;
  }
  is_expanded_ = expanded;

  // Show/hide content and footer.
  if (scroll_view_) {
    scroll_view_->SetVisible(is_expanded_);
  }
  if (footer_view_) {
    // Footer is only visible when expanded AND has content to show.
    bool footer_visible = is_expanded_ && show_more_label_->GetVisible();
    footer_view_->SetVisible(footer_visible);
  }

  UpdateChevron();
  InvalidateLayout();
}

void AstraSidebarSectionView::ToggleExpanded() {
  SetExpanded(!is_expanded_);
}

// =========================================================================
// Item count
// =========================================================================

void AstraSidebarSectionView::SetItemCount(int count) {
  if (item_count_ == count) {
    return;
  }
  item_count_ = count;
  UpdateItemCountBadge();
}

void AstraSidebarSectionView::SetShowItemCount(bool show) {
  if (show_item_count_ == show) {
    return;
  }
  show_item_count_ = show;
  UpdateHeaderVisibility();
}

// =========================================================================
// Chevron
// =========================================================================

void AstraSidebarSectionView::SetShowChevron(bool show) {
  if (show_chevron_ == show) {
    return;
  }
  show_chevron_ = show;
  UpdateHeaderVisibility();
}

// =========================================================================
// Search
// =========================================================================

void AstraSidebarSectionView::SetShowSearch(bool show) {
  if (show_search_ == show) {
    return;
  }
  show_search_ = show;
  UpdateHeaderVisibility();
}

void AstraSidebarSectionView::SetSearchQuery(const std::u16string& query) {
  if (search_query_ == query) {
    return;
  }
  search_query_ = query;
  if (search_field_) {
    search_field_->SetText(search_query_);
  }
  OnSearchQueryChanged(search_query_);
}

// =========================================================================
// Add button
// =========================================================================

void AstraSidebarSectionView::SetShowAddButton(bool show) {
  if (show_add_button_ == show) {
    return;
  }
  show_add_button_ = show;
  UpdateHeaderVisibility();
}

// =========================================================================
// More button
// =========================================================================

void AstraSidebarSectionView::SetShowMoreButton(bool show) {
  if (show_more_button_ == show) {
    return;
  }
  show_more_button_ = show;
  UpdateHeaderVisibility();
}

// =========================================================================
// Context menu
// =========================================================================

void AstraSidebarSectionView::SetShowContextMenu(bool show) {
  show_context_menu_ = show;
}

// =========================================================================
// Section color
// =========================================================================

void AstraSidebarSectionView::SetSectionColor(SkColor color) {
  if (section_color_ == color) {
    return;
  }
  section_color_ = color;
  // TODO(astra): Apply tint to icon and accent elements.
  if (header_icon_ && section_color_ != SK_ColorTRANSPARENT) {
    // Would apply color filter to the icon image.
  }
  OnThemeChanged();
}

// =========================================================================
// Drag and drop
// =========================================================================

void AstraSidebarSectionView::SetDragDropEnabled(bool enabled) {
  if (drag_drop_enabled_ == enabled) {
    return;
  }
  drag_drop_enabled_ = enabled;
  // TODO(astra): Update item views to enable/disable drag source behavior.
}

// =========================================================================
// Item management
// =========================================================================

void AstraSidebarSectionView::AddItemView(views::View* item) {
  if (!items_container_ || !item) {
    return;
  }
  items_container_->AddChildView(item);
  is_empty_ = false;
  UpdateContentState();
}

void AstraSidebarSectionView::RemoveAllItems() {
  if (!items_container_) {
    return;
  }
  items_container_->RemoveAllChildViews();
  is_empty_ = true;
  UpdateContentState();
}

views::View* AstraSidebarSectionView::GetItemViewAt(int index) {
  if (!items_container_ || index < 0 ||
      static_cast<size_t>(index) >= items_container_->children().size()) {
    return nullptr;
  }
  return items_container_->children()[index];
}

const views::View* AstraSidebarSectionView::GetItemViewAt(int index) const {
  if (!items_container_ || index < 0 ||
      static_cast<size_t>(index) >= items_container_->children().size()) {
    return nullptr;
  }
  return items_container_->children()[index];
}

int AstraSidebarSectionView::GetItemViewCount() const {
  if (!items_container_) {
    return 0;
  }
  return static_cast<int>(items_container_->children().size());
}

// =========================================================================
// Sort order
// =========================================================================

void AstraSidebarSectionView::SetSortOrder(AstraSidebarSortOrder order) {
  sort_order_ = order;
  // Subclasses override to actually sort items.
}

// =========================================================================
// Filter
// =========================================================================

void AstraSidebarSectionView::SetFilter(AstraSidebarFilter filter) {
  filter_ = filter;
  // Subclasses override to actually filter items.
}

// =========================================================================
// Loading / empty state
// =========================================================================

void AstraSidebarSectionView::SetLoading(bool loading) {
  if (is_loading_ == loading) {
    return;
  }
  is_loading_ = loading;
  UpdateContentState();
}

void AstraSidebarSectionView::SetEmpty(bool empty) {
  if (is_empty_ == empty) {
    return;
  }
  is_empty_ = empty;
  UpdateContentState();
}

void AstraSidebarSectionView::SetEmptyStateText(const std::u16string& text) {
  if (empty_state_label_) {
    empty_state_label_->SetText(text);
  }
}

// =========================================================================
// Item helpers (AstraSidebarItemView compatibility)
// =========================================================================

AstraSidebarItemView* AstraSidebarSectionView::AddItem(
    std::unique_ptr<AstraSidebarItemView> item) {
  if (!items_container_ || !item) {
    return nullptr;
  }
  AstraSidebarItemView* raw = items_container_->AddChildView(std::move(item));
  is_empty_ = false;
  UpdateContentState();
  return raw;
}

AstraSidebarItemView* AstraSidebarSectionView::InsertItemAt(
    size_t index,
    std::unique_ptr<AstraSidebarItemView> item) {
  if (!items_container_ || !item) {
    return nullptr;
  }
  AstraSidebarItemView* raw =
      items_container_->AddChildViewAt(std::move(item), index);
  is_empty_ = false;
  UpdateContentState();
  return raw;
}

void AstraSidebarSectionView::RemoveItemAt(size_t index) {
  DCHECK_LT(index, GetItemCountTyped());
  if (!items_container_) {
    return;
  }
  items_container_->RemoveChildViewT(items_container_->children()[index]);
  is_empty_ = items_container_->children().empty();
  UpdateContentState();
}

AstraSidebarItemView* AstraSidebarSectionView::GetItemAt(
    size_t index) const {
  if (index >= GetItemCountTyped()) {
    return nullptr;
  }
  return static_cast<AstraSidebarItemView*>(
      items_container_->children()[index]);
}

void AstraSidebarSectionView::ClearItems() {
  RemoveAllItems();
}

size_t AstraSidebarSectionView::GetItemCountTyped() const {
  return static_cast<size_t>(GetItemViewCount());
}

// =========================================================================
// Section visibility
// =========================================================================

void AstraSidebarSectionView::SetSectionVisible(bool visible) {
  SetVisible(visible);
}

// =========================================================================
// Drop target
// =========================================================================

void AstraSidebarSectionView::OnDragEnter(const AstraSidebarDragData& drag_data,
                                          int y_in_section) {
  has_active_drag_ = true;

  if (!drop_delegate_) {
    return;
  }

  AstraSidebarDropResult result = drop_delegate_->OnDragEnterSection(
      section_type_, drag_data, y_in_section);

  if (result.is_valid && drop_indicator_) {
    int indicator_y = GetItemsContainerY();
    if (result.insert_index >= 0) {
      views::View* item = GetItemViewAt(result.insert_index);
      if (item) {
        gfx::Point item_origin = item->origin();
        indicator_y = GetItemsContainerY() + item_origin.y() - 1;
      }
    } else {
      size_t count = GetItemCountTyped();
      if (count > 0) {
        views::View* last_item = GetItemViewAt(static_cast<int>(count - 1));
        if (last_item) {
          indicator_y = GetItemsContainerY() + last_item->bounds().bottom() +
                        kSectionItemSpacing;
        }
      } else {
        indicator_y = GetItemsContainerY() + kSectionVerticalPadding;
      }
    }
    drop_indicator_->ShowAtPosition(indicator_y);
  }
}

void AstraSidebarSectionView::OnDragOver(const AstraSidebarDragData& drag_data,
                                         int y_in_section) {
  if (!drop_delegate_ || !has_active_drag_) {
    return;
  }

  AstraSidebarDropResult result = drop_delegate_->OnDragOverSection(
      section_type_, drag_data, y_in_section);

  if (result.is_valid && drop_indicator_) {
    int indicator_y = GetItemsContainerY();
    if (result.insert_index >= 0 &&
        static_cast<size_t>(result.insert_index) < GetItemCountTyped()) {
      views::View* item = GetItemViewAt(result.insert_index);
      if (item) {
        indicator_y = GetItemsContainerY() + item->y() - 1;
      }
    } else {
      size_t count = GetItemCountTyped();
      if (count > 0) {
        views::View* last_item = GetItemViewAt(static_cast<int>(count - 1));
        if (last_item) {
          indicator_y = GetItemsContainerY() + last_item->bounds().bottom() +
                        kSectionItemSpacing;
        }
      } else {
        indicator_y = GetItemsContainerY() + kSectionVerticalPadding;
      }
    }
    drop_indicator_->ShowAtPosition(indicator_y);
  } else if (drop_indicator_) {
    drop_indicator_->HideIndicator();
  }
}

void AstraSidebarSectionView::OnDragLeave() {
  has_active_drag_ = false;

  if (drop_indicator_) {
    drop_indicator_->HideIndicator();
  }

  if (drop_delegate_) {
    drop_delegate_->OnDragLeaveSection(section_type_);
  }
}

bool AstraSidebarSectionView::OnDrop(const AstraSidebarDragData& drag_data) {
  bool handled = false;

  if (drop_delegate_) {
    AstraSidebarDropResult result;
    result.is_valid = true;
    result.insert_index = -1;

    handled =
        drop_delegate_->OnDropInSection(section_type_, drag_data, result);
  }

  has_active_drag_ = false;
  if (drop_indicator_) {
    drop_indicator_->HideIndicator();
  }

  return handled;
}

int AstraSidebarSectionView::GetInsertIndexFromY(int y_in_section) const {
  int items_y = GetItemsContainerY();
  int y_in_items = y_in_section - items_y;

  if (!items_container_) {
    return 0;
  }

  const auto& children = items_container_->children();
  if (children.empty()) {
    return 0;
  }

  for (size_t i = 0; i < children.size(); ++i) {
    const views::View* child = children[i];
    int child_midpoint = child->y() + child->height() / 2;
    if (y_in_items < child_midpoint) {
      return static_cast<int>(i);
    }
  }

  return static_cast<int>(children.size());
}

// =========================================================================
// Protected virtual handlers (default implementations)
// =========================================================================

void AstraSidebarSectionView::OnAddButtonClicked() {
  // Subclasses override to handle add button clicks.
}

void AstraSidebarSectionView::OnMoreButtonClicked() {
  // Subclasses override to show a context menu for more options.
}

void AstraSidebarSectionView::OnSearchQueryChanged(const std::u16string& /*query*/) {
  // Subclasses override to filter items based on search query.
}

void AstraSidebarSectionView::OnShowMoreClicked() {
  // Subclasses override to handle "show more" footer link.
}

void AstraSidebarSectionView::OnHeaderClicked() {
  ToggleExpanded();
}

// =========================================================================
// Private helpers
// =========================================================================

void AstraSidebarSectionView::UpdateHeaderVisibility() {
  if (!header_view_) {
    return;
  }

  // Icon visibility: show if section_color is set or icon is configured.
  if (header_icon_) {
    header_icon_->SetVisible(section_color_ != SK_ColorTRANSPARENT);
  }

  // Chevron visibility.
  if (chevron_view_) {
    chevron_view_->SetVisible(show_chevron_);
  }

  // Count badge visibility.
  if (count_badge_) {
    count_badge_->SetVisible(show_item_count_);
    if (show_item_count_) {
      UpdateItemCountBadge();
    }
  }

  // Search field visibility.
  if (search_field_) {
    search_field_->SetVisible(show_search_);
  }

  // Add button visibility.
  if (add_button_) {
    add_button_->SetVisible(show_add_button_);
  }

  // More button visibility.
  if (more_button_) {
    more_button_->SetVisible(show_more_button_);
  }
}

void AstraSidebarSectionView::UpdateContentState() {
  if (!content_view_) {
    return;
  }

  bool has_items = items_container_ && !items_container_->children().empty();

  // Items container visibility.
  if (items_container_) {
    items_container_->SetVisible(!is_loading_ && has_items);
  }

  // Empty state visibility — shown when not loading and no items.
  if (empty_state_label_) {
    empty_state_label_->SetVisible(!is_loading_ && !has_items && is_empty_);
  }

  // Loading state visibility.
  if (loading_state_view_) {
    loading_state_view_->SetVisible(is_loading_);
  }
}

void AstraSidebarSectionView::UpdateItemCountBadge() {
  if (!count_badge_ || !show_item_count_) {
    return;
  }
  count_badge_->SetText(base::FormatNumber(item_count_));
}

void AstraSidebarSectionView::UpdateChevron() {
  if (!chevron_view_) {
    return;
  }
  // TODO(astra): Rotate chevron based on expanded state.
  // In Chromium, this is done via ImageView::SetImageRotation or
  // by swapping the icon asset.
}

int AstraSidebarSectionView::GetItemsContainerY() const {
  if (!items_container_) {
    return 0;
  }
  // Convert from items_container coords to section coords.
  // items_container is inside content_view_, which is inside scroll_view_.
  return items_container_->GetBoundsInRoot().y();
}

// =========================================================================
// views::View overrides
// =========================================================================

gfx::Size AstraSidebarSectionView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return views::View::CalculatePreferredSize(available_size);
}

void AstraSidebarSectionView::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  if (header_label_) {
    header_label_->SetEnabledColor(
        color_provider->GetColor(kSectionHeaderTextColorId));
  }

  if (count_badge_) {
    count_badge_->SetEnabledColor(
        color_provider->GetColor(kSectionCountBadgeTextColorId));
  }

  if (empty_state_label_) {
    empty_state_label_->SetEnabledColor(
        color_provider->GetColor(kSectionSecondaryTextColorId));
  }

  if (show_more_label_) {
    show_more_label_->SetEnabledColor(
        color_provider->GetColor(kSectionFooterTextColorId));
  }

  if (status_label_) {
    status_label_->SetEnabledColor(
        color_provider->GetColor(kSectionSecondaryTextColorId));
  }

  if (scroll_view_ && layer()) {
    SetBackground(views::CreateSolidBackground(
        color_provider->GetColor(kSectionBackgroundColorId)));
  }
}

}  // namespace astra
