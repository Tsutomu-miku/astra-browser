#include "astra/ui/views/bookmarks_bar/astra_bookmarks_bar_view.h"

#include <algorithm>
#include <utility>

#include "base/i18n/number_formatting.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/flex_layout_types.h"
#include "ui/views/menu/menu_runner.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/widget/widget.h"

#include "astra/ui/views/bookmarks_bar/astra_bookmarks_bar_item_view.h"
#include "astra/ui/views/bookmarks_bar/astra_bookmarks_bar_model.h"

namespace astra {

AstraBookmarksBarView::AstraBookmarksBarView(
    AstraBookmarksBarModel* model)
    : model_(model) {
  if (model_) {
    model_observation_.Observe(model_);
  }

  BuildLayout();

  if (model_) {
    RebuildItems();
  }
}

AstraBookmarksBarView::~AstraBookmarksBarView() = default;

void AstraBookmarksBarView::SetModel(AstraBookmarksBarModel* model) {
  if (model_observation_.IsObserving()) {
    model_observation_.Reset();
  }
  model_ = model;
  if (model_) {
    model_observation_.Observe(model_);
    RebuildItems();
  }
}

void AstraBookmarksBarView::SetVisible(bool visible) {
  if (bar_visible_ == visible) {
    return;
  }
  bar_visible_ = visible;
  views::View::SetVisible(visible);
  // TODO(astra): Add slide animation when showing/hiding.
  // Chromium pattern: views::SlideAnimation
}

AstraBookmarksBarItemView* AstraBookmarksBarView::GetItemAt(
    int index) const {
  if (index < 0 || index >= static_cast<int>(item_views_.size())) {
    return nullptr;
  }
  return item_views_[index];
}

int AstraBookmarksBarView::GetItemCount() const {
  return static_cast<int>(item_views_.size());
}

void AstraBookmarksBarView::SetShowFavicons(bool show) {
  show_favicons_ = show;
  for (auto* item : item_views_) {
    item->SetShowIcon(show);
  }
  Layout();
}

void AstraBookmarksBarView::SetShowText(bool show) {
  show_text_ = show;
  for (auto* item : item_views_) {
    item->SetShowText(show);
  }
  Layout();
}

void AstraBookmarksBarView::SetMaxItemWidth(int width) {
  max_item_width_ = width;
  for (auto* item : item_views_) {
    item->SetMaxWidth(width);
  }
  Layout();
}

void AstraBookmarksBarView::SetShowOtherBookmarksButton(bool show) {
  show_other_bookmarks_button_ = show;
  if (other_bookmarks_button_) {
    other_bookmarks_button_->SetVisible(show);
  }
  Layout();
}

// -- AstraBookmarksBarObserver -----------------------------------------

void AstraBookmarksBarView::OnBookmarksBarLoaded(
    AstraBookmarksBarModel* model) {
  DCHECK_EQ(model, model_);
  RebuildItems();
}

void AstraBookmarksBarView::OnBookmarkAdded(
    AstraBookmarksBarModel* model,
    int64_t bookmark_id) {
  DCHECK_EQ(model, model_);
  // Simple approach: rebuild everything.
  RebuildItems();
}

void AstraBookmarksBarView::OnBookmarkRemoved(
    AstraBookmarksBarModel* model,
    int64_t bookmark_id) {
  DCHECK_EQ(model, model_);
  RebuildItems();
}

void AstraBookmarksBarView::OnBookmarkChanged(
    AstraBookmarksBarModel* model,
    int64_t bookmark_id) {
  DCHECK_EQ(model, model_);
  auto* item_view = FindItemView(bookmark_id);
  if (item_view && model) {
    auto* item = model->GetItem(bookmark_id);
    if (item) {
      item_view->SetTitle(item->title);
      item_view->SetURL(item->url);
      item_view->SetIsFolder(item->is_folder);
    }
  }
}

void AstraBookmarksBarView::OnBookmarksReordered(
    AstraBookmarksBarModel* model) {
  DCHECK_EQ(model, model_);
  RebuildItems();
}

void AstraBookmarksBarView::OnBookmarksBarVisibilityChanged(
    AstraBookmarksBarModel* model,
    bool visible) {
  DCHECK_EQ(model, model_);
  SetVisible(visible);
}

void AstraBookmarksBarView::OnBookmarksBarModelShutdown(
    AstraBookmarksBarModel* model) {
  DCHECK_EQ(model, model_);
  model_observation_.Reset();
  model_ = nullptr;
}

// -- AstraBookmarksBarItemDelegate -------------------------------------

void AstraBookmarksBarView::OnBookmarkClicked(
    int64_t bookmark_id,
    bool is_middle_click,
    bool is_shift_click) {
  if (!delegate_ || !model_) {
    return;
  }

  auto* item = model_->GetItem(bookmark_id);
  if (!item) {
    return;
  }

  if (item->is_folder) {
    // Folder click is handled by OnFolderClicked.
    return;
  }

  if (is_middle_click || is_shift_click) {
    delegate_->OpenBookmarkInNewTab(bookmark_id);
  } else {
    delegate_->OpenBookmark(bookmark_id);
  }
}

void AstraBookmarksBarView::OnFolderClicked(
    int64_t folder_id,
    views::View* anchor_view) {
  if (!delegate_) {
    return;
  }
  // For folders, show a menu with the folder's contents.
  // In a full implementation, we'd show a bookmark folder menu.
  // TODO(astra): Show folder dropdown menu.
}

void AstraBookmarksBarView::OnBookmarkRightClicked(
    int64_t bookmark_id,
    const gfx::Point& point) {
  ShowContextMenu(bookmark_id, point);
}

void AstraBookmarksBarView::OnBookmarkDragStarted(
    int64_t bookmark_id,
    const ui::MouseEvent& event) {
  // TODO(astra): Start drag and drop operation.
}

// -- views::View -------------------------------------------------------

gfx::Size AstraBookmarksBarView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  int width = available_size.width().value_or(0);
  return gfx::Size(width, kBarHeight);
}

void AstraBookmarksBarView::Layout() {
  views::View::Layout();
}

void AstraBookmarksBarView::OnThemeChanged() {
  views::View::OnThemeChanged();
  UpdateColors();
}

void AstraBookmarksBarView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  views::View::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kToolbar;
  node_data->SetName(u"Bookmarks bar");
}

// -- Private methods ---------------------------------------------------

void AstraBookmarksBarView::BuildLayout() {
  auto* layout = SetLayoutManager(std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);
  layout->SetInteriorMargin(
      gfx::Insets::VH(0, kLeadingPadding));

  BuildLeadingButtons();
  BuildItemsContainer();
  BuildTrailingButtons();
}

void AstraBookmarksBarView::BuildLeadingButtons() {
  leading_container_ =
      AddChildView(std::make_unique<views::View>());
  auto* layout = leading_container_->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);

  // TODO(astra): Add apps launcher button.
}

void AstraBookmarksBarView::BuildItemsContainer() {
  scroll_view_ =
      AddChildView(std::make_unique<views::ScrollView>(
          views::ScrollView::ScrollWithLayers::kEnabled));
  scroll_view_->SetHorizontalScrollBarMode(
      views::ScrollView::ScrollBarMode::kHiddenButEnabled);
  scroll_view_->SetVerticalScrollBarMode(
      views::ScrollView::ScrollBarMode::kDisabled);
  scroll_view_->SetBackgroundColor(absl::nullopt);
  scroll_view_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded,
          /*weight=*/1.0f));

  items_container_ =
      scroll_view_->SetContents(std::make_unique<views::View>());
  auto* items_layout = items_container_->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  items_layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  items_layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);
  items_layout->SetDefault(
      views::kMarginsKey,
      gfx::Insets::TLBR(0, 0, 0, kItemSpacing));
}

void AstraBookmarksBarView::BuildTrailingButtons() {
  trailing_container_ =
      AddChildView(std::make_unique<views::View>());
  trailing_container_->SetProperty(
      views::kMarginsKey,
      gfx::Insets::TLBR(0, 0, 0, kTrailingPadding));
  auto* layout = trailing_container_->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);
  layout->SetDefault(
      views::kMarginsKey,
      gfx::Insets::TLBR(0, kTrailingButtonSpacing, 0, 0));

  // Overflow button
  overflow_button_ = trailing_container_->AddChildView(
      std::make_unique<views::ImageButton>(
          base::BindRepeating(
              &AstraBookmarksBarView::OnOverflowButtonClicked,
              base::Unretained(this))));
  overflow_button_->SetPreferredSize(
      gfx::Size(kButtonSize, kButtonSize));
  overflow_button_->SetTooltipText(u"More bookmarks");
  overflow_button_->SetVisible(false);

  // Other bookmarks button
  other_bookmarks_button_ = trailing_container_->AddChildView(
      std::make_unique<views::ImageButton>(
          base::BindRepeating(
              &AstraBookmarksBarView::OnOtherBookmarksClicked,
              base::Unretained(this))));
  other_bookmarks_button_->SetPreferredSize(
      gfx::Size(kButtonSize, kButtonSize));
  other_bookmarks_button_->SetTooltipText(u"Other bookmarks");
  other_bookmarks_button_->SetVisible(show_other_bookmarks_button_);

  // Add bookmark button
  add_bookmark_button_ = trailing_container_->AddChildView(
      std::make_unique<views::ImageButton>(
          base::BindRepeating(
              &AstraBookmarksBarView::OnAddBookmarkClicked,
              base::Unretained(this))));
  add_bookmark_button_->SetPreferredSize(
      gfx::Size(kButtonSize, kButtonSize));
  add_bookmark_button_->SetTooltipText(u"Add bookmark");
}

void AstraBookmarksBarView::RebuildItems() {
  if (!items_container_ || !model_) {
    return;
  }

  items_container_->RemoveAllChildViews();
  item_views_.clear();

  auto items = model_->GetItems();
  for (const auto& item : items) {
    auto item_view = CreateItemView(item);
    item_view->set_delegate(this);
    item_views_.push_back(
        items_container_->AddChildView(std::move(item_view)));
  }

  items_container_->InvalidateLayout();
  Layout();
}

void AstraBookmarksBarView::UpdateItems() {
  if (!model_) {
    return;
  }
  for (auto* item_view : item_views_) {
    auto* item = model_->GetItem(item_view->GetBookmarkId());
    if (item) {
      item_view->SetTitle(item->title);
      item_view->SetURL(item->url);
      item_view->SetIsFolder(item->is_folder);
    }
  }
}

std::unique_ptr<AstraBookmarksBarItemView>
AstraBookmarksBarView::CreateItemView(
    const AstraBookmarksBarItem& item) {
  auto view = std::make_unique<AstraBookmarksBarItemView>(
      item.id, item.title, item.is_folder);
  view->SetURL(item.url);
  view->SetShowIcon(show_favicons_);
  view->SetShowText(show_text_);
  view->SetMaxWidth(max_item_width_);
  if (!item.favicon.IsEmpty()) {
    view->SetFavicon(item.favicon);
  }
  return view;
}

AstraBookmarksBarItemView* AstraBookmarksBarView::FindItemView(
    int64_t bookmark_id) const {
  for (auto* view : item_views_) {
    if (view->GetBookmarkId() == bookmark_id) {
      return view;
    }
  }
  return nullptr;
}

void AstraBookmarksBarView::UpdateColors() {
  // Colors would be updated from the color provider in production.
  // For now, this is a placeholder.
  SchedulePaint();
}

void AstraBookmarksBarView::OnOtherBookmarksClicked() {
  if (delegate_) {
    delegate_->OnOtherBookmarksClicked(other_bookmarks_button_);
  }
}

void AstraBookmarksBarView::OnAddBookmarkClicked() {
  if (delegate_) {
    delegate_->OnAddBookmarkClicked();
  }
}

void AstraBookmarksBarView::OnOverflowButtonClicked() {
  // Show overflow menu with bookmarks that don't fit.
  // TODO(astra): Implement overflow menu.
}

void AstraBookmarksBarView::ShowContextMenu(
    int64_t bookmark_id,
    const gfx::Point& point) {
  // TODO(astra): Show context menu with options like:
  //   - Open in new tab
  //   - Open in incognito window
  //   - Edit...
  //   - Delete
  //   - Cut / Copy / Paste
  if (context_menu_runner_) {
    context_menu_runner_.reset();
  }
}

void AstraBookmarksBarView::HandleDragReorder(
    int64_t dragged_id,
    int target_index) {
  if (!model_) {
    return;
  }
  model_->MoveBookmark(dragged_id, target_index);
}

}  // namespace astra
