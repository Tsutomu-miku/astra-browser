#include "astra/ui/views/sidebar/astra_sidebar_section_view.h"

#include "base/check.h"
#include "astra/ui/color/astra_color_ids.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
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

// Astra color ID for section header text.
// Uses the Astra sidebar section header color from the Astra color system.
// Chromium subsystem: ui::ColorProvider (ui/color/color_provider.h)
// Astra color system: astra/ui/color/astra_color_ids.h
constexpr ui::ColorId kSectionHeaderTextColorId =
    kColorAstraSidebarSectionHeaderText;

}  // namespace

AstraSidebarSectionView::AstraSidebarSectionView(
    const std::u16string& title,
    AstraSidebarSectionType type)
    : section_type_(type) {
  // Vertical box layout for the whole section: header + items.
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  layout->set_between_child_spacing(0);

  // Header label.
  header_label_ = AddChildView(std::make_unique<views::Label>(title));
  header_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header_label_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(kSectionVerticalPadding, kSectionHorizontalPadding)));
  header_label_->SetFontList(
      header_label_->font_list().DeriveWithSizeDelta(kSectionHeaderFontSizeDelta));
  header_label_->SetAutoColorReadabilityEnabled(false);
  header_label_->SetAccessibleName(title);

  // Items container with internal item spacing.
  items_container_ = AddChildView(std::make_unique<views::View>());
  auto* items_layout = items_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets::VH(0, 4),
          kSectionItemSpacing));
  items_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Drop indicator — positioned manually, not managed by the layout.
  // Shown during drag-and-drop to indicate where a dragged item will land.
  // Owned by the view hierarchy.
  drop_indicator_ =
      AddChildView(std::make_unique<AstraSidebarDropIndicatorView>());

  // Accessibility: treat the section as a group with the header as its name.
  SetAccessibleName(title);
}

AstraSidebarSectionView::~AstraSidebarSectionView() = default;

AstraSidebarItemView* AstraSidebarSectionView::AddItem(
    std::unique_ptr<AstraSidebarItemView> item) {
  return items_container_->AddChildView(std::move(item));
}

AstraSidebarItemView* AstraSidebarSectionView::InsertItemAt(
    size_t index,
    std::unique_ptr<AstraSidebarItemView> item) {
  return items_container_->AddChildViewAt(std::move(item), index);
}

void AstraSidebarSectionView::RemoveItemAt(size_t index) {
  DCHECK_LT(index, GetItemCount());
  items_container_->RemoveChildViewT(items_container_->children()[index]);
}

AstraSidebarItemView* AstraSidebarSectionView::GetItemAt(size_t index) const {
  if (index >= GetItemCount()) {
    return nullptr;
  }
  return static_cast<AstraSidebarItemView*>(
      items_container_->children()[index]);
}

void AstraSidebarSectionView::ClearItems() {
  items_container_->RemoveAllChildViews();
}

size_t AstraSidebarSectionView::GetItemCount() const {
  return items_container_->children().size();
}

void AstraSidebarSectionView::SetSectionVisible(bool visible) {
  SetVisible(visible);
  // No explicit invalidation needed — visibility change propagates through
  // the View hierarchy.
}

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
  header_label_->SetEnabledColor(
      color_provider->GetColor(kSectionHeaderTextColorId));
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
    // Position the indicator at the insertion point.
    int indicator_y = GetItemsContainerY();
    if (result.insert_index >= 0) {
      // Position above the item at insert_index.
      AstraSidebarItemView* item = GetItemAt(result.insert_index);
      if (item) {
        gfx::Point item_origin = item->origin();
        // Convert from items_container coords to section coords.
        indicator_y =
            GetItemsContainerY() + item_origin.y() - /*half thickness*/ 1;
      }
    } else {
      // Insert at the end — position below the last item.
      size_t count = GetItemCount();
      if (count > 0) {
        AstraSidebarItemView* last_item = GetItemAt(count - 1);
        if (last_item) {
          indicator_y = GetItemsContainerY() + last_item->bounds().bottom() +
                        kSectionItemSpacing;
        }
      } else {
        // Empty section — show at the top of the items area.
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
        static_cast<size_t>(result.insert_index) < GetItemCount()) {
      AstraSidebarItemView* item = GetItemAt(result.insert_index);
      if (item) {
        indicator_y =
            GetItemsContainerY() + item->y() - /*half thickness*/ 1;
      }
    } else {
      // Append at end.
      size_t count = GetItemCount();
      if (count > 0) {
        AstraSidebarItemView* last_item = GetItemAt(count - 1);
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
    // Compute the final drop result at the current position.
    // TODO(astra): We should pass the actual drop position, not just
    // rely on the last OnDragOver result. For now, the delegate caches
    // the last result.
    AstraSidebarDropResult result;
    result.is_valid = true;
    result.insert_index = -1;

    handled = drop_delegate_->OnDropInSection(section_type_, drag_data, result);
  }

  // Clean up drag state.
  has_active_drag_ = false;
  if (drop_indicator_) {
    drop_indicator_->HideIndicator();
  }

  return handled;
}

int AstraSidebarSectionView::GetInsertIndexFromY(int y_in_section) const {
  int items_y = GetItemsContainerY();
  int y_in_items = y_in_section - items_y;

  const auto& children = items_container_->children();
  if (children.empty()) {
    return 0;  // Empty section — insert at index 0.
  }

  // Find which item (if any) the y position is over.
  for (size_t i = 0; i < children.size(); ++i) {
    const views::View* child = children[i];
    int child_midpoint = child->y() + child->height() / 2;
    if (y_in_items < child_midpoint) {
      return static_cast<int>(i);
    }
  }

  // Below all items — insert at the end.
  return static_cast<int>(children.size());
}

int AstraSidebarSectionView::GetItemsContainerY() const {
  return items_container_->y();
}

}  // namespace astra
