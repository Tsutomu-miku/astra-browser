// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/downloads_shelf/astra_downloads_shelf_view.h"

#include <algorithm>

#include "base/i18n/number_formatting.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "astra/browser/astra_downloads_helper.h"
#include "astra/ui/views/downloads_shelf/astra_downloads_shelf_item_view.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace astra {

namespace {

// Draw a simple download icon (down arrow).
void DrawDownloadIcon(gfx::Canvas* canvas,
                      const gfx::Rect& bounds,
                      SkColor color) {
  if (bounds.IsEmpty())
    return;

  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int size = std::min(bounds.width(), bounds.height()) * 3 / 4;
  int half = size / 2;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(2);
  flags.setAntiAlias(true);

  // Down arrow.
  canvas->DrawLine(gfx::Point(cx, cy - half), gfx::Point(cx, cy + half / 2),
                   flags);
  canvas->DrawLine(gfx::Point(cx - half / 2, cy), gfx::Point(cx, cy + half / 2),
                   flags);
  canvas->DrawLine(gfx::Point(cx + half / 2, cy), gfx::Point(cx, cy + half / 2),
                   flags);
  // Horizontal line at bottom.
  canvas->DrawLine(gfx::Point(cx - half / 2, cy + half / 2),
                   gfx::Point(cx + half / 2, cy + half / 2), flags);
}

}  // namespace

// =========================================================================
// AstraDownloadsShelfView
// =========================================================================

AstraDownloadsShelfView::AstraDownloadsShelfView(AstraDownloadsHelper* helper)
    : helper_(helper) {
  SetLayoutManager(std::make_unique<views::FillLayout>());
  BuildLayout();
  SetHelper(helper);
}

AstraDownloadsShelfView::~AstraDownloadsShelfView() = default;

// -- Show / hide ------------------------------------------------------------

void AstraDownloadsShelfView::Show() {
  if (shelf_visible_)
    return;

  shelf_visible_ = true;
  SetVisible(true);

  // TODO(astra): Add slide-in animation.
  // Chromium owner: views::Animation / gfx::SlideAnimation
  // For now just make visible.
  OnShowAnimationComplete();
}

void AstraDownloadsShelfView::Hide() {
  if (!shelf_visible_)
    return;

  shelf_visible_ = false;

  // TODO(astra): Add slide-out animation.
  // Chromium owner: views::Animation / gfx::SlideAnimation
  // For now just hide immediately.
  OnHideAnimationComplete();
}

void AstraDownloadsShelfView::HideAfterDelay(base::TimeDelta delay) {
  auto_hide_timer_.Start(FROM_HERE, delay, this,
                         &AstraDownloadsShelfView::OnAutoHideTimerFired);
}

void AstraDownloadsShelfView::CancelAutoHide() {
  auto_hide_timer_.Stop();
}

// -- Helper access ----------------------------------------------------------

void AstraDownloadsShelfView::SetHelper(AstraDownloadsHelper* helper) {
  if (helper_observation_.IsObserving())
    helper_observation_.Reset();

  helper_ = helper;

  if (helper_) {
    helper_observation_.Observe(helper_);
    RebuildItems();
  }
}

// -- Display options --------------------------------------------------------

void AstraDownloadsShelfView::SetAutoHide(bool auto_hide) {
  auto_hide_ = auto_hide;
  if (auto_hide_)
    MaybeStartAutoHide();
  else
    CancelAutoHide();
}

void AstraDownloadsShelfView::SetAutoHideDelay(base::TimeDelta delay) {
  auto_hide_delay_ = delay;
}

void AstraDownloadsShelfView::SetMaxItems(int max_items) {
  max_items_ = std::max(1, max_items);
  RebuildItems();
}

// -- AstraDownloadsObserver -------------------------------------------------

void AstraDownloadsShelfView::OnDownloadStarted(int download_id) {
  AddDownloadItem(download_id);
  Show();
  CancelAutoHide();
}

void AstraDownloadsShelfView::OnDownloadUpdated(int download_id) {
  UpdateDownloadItem(download_id);
}

void AstraDownloadsShelfView::OnDownloadCompleted(int download_id) {
  UpdateDownloadItem(download_id);
  MaybeStartAutoHide();
}

void AstraDownloadsShelfView::OnDownloadFailed(int download_id,
                                               const std::string& error) {
  UpdateDownloadItem(download_id);
  MaybeStartAutoHide();
}

void AstraDownloadsShelfView::OnDownloadRemoved(int download_id) {
  RemoveDownloadItem(download_id);
  if (item_views_.empty() && shelf_visible_) {
    Hide();
  }
}

void AstraDownloadsShelfView::OnAllDownloadsCleared() {
  RebuildItems();
  if (item_views_.empty() && shelf_visible_) {
    Hide();
  }
}

void AstraDownloadsShelfView::OnDownloadsSettingsChanged() {
  RebuildItems();
}

// -- views::View ------------------------------------------------------------

gfx::Size AstraDownloadsShelfView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return gfx::Size(0, kShelfHeight);
}

void AstraDownloadsShelfView::Layout() {
  if (!header_ || !scroll_view_ || !trailing_)
    return;

  int content_width = width();
  int content_height = kShelfHeight;

  // Header on the left.
  header_->SetBounds(kHorizontalPadding, 0, kHeaderWidth, content_height);

  // Trailing controls on the right.
  trailing_->SetBounds(content_width - kTrailingWidth - kHorizontalPadding, 0,
                       kTrailingWidth, content_height);

  // Scroll view fills the middle.
  int scroll_x = kHorizontalPadding + kHeaderWidth + kItemSpacing;
  int scroll_width = content_width - scroll_x - kTrailingWidth -
                     kHorizontalPadding - kItemSpacing;
  scroll_view_->SetBounds(scroll_x, 0, scroll_width, content_height);

  // Layout items container inside scroll view.
  if (items_container_) {
    int items_width = 0;
    int y = (content_height - kItemHeight) / 2;
    for (size_t i = 0; i < item_views_.size(); ++i) {
      if (i > 0)
        items_width += kItemSpacing;
      item_views_[i]->SetBounds(items_width, y,
                                AstraDownloadsShelfItemView::kItemWidth,
                                kItemHeight);
      items_width += AstraDownloadsShelfItemView::kItemWidth;
    }
    items_container_->SetBounds(0, 0, items_width, content_height);
  }
}

void AstraDownloadsShelfView::OnThemeChanged() {
  views::View::OnThemeChanged();
  UpdateColors();
}

void AstraDownloadsShelfView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  node_data->role = ax::mojom::Role::kGenericContainer;
  node_data->SetName(u"Downloads shelf");
}

// -- Private methods --------------------------------------------------------

void AstraDownloadsShelfView::BuildLayout() {
  BuildHeader();
  BuildItemsContainer();
  BuildTrailingControls();
  UpdateColors();
}

void AstraDownloadsShelfView::BuildHeader() {
  auto header = std::make_unique<views::View>();
  header->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets(0, 0),
      kItemSpacing));
  header_ = AddChildView(std::move(header));

  // Icon.
  auto icon_view = std::make_unique<views::ImageView>();
  icon_view->SetImageSize(gfx::Size(16, 16));
  icon_view_ = header_->AddChildView(std::move(icon_view));

  // Title label.
  auto title_label = std::make_unique<views::Label>(u"Downloads");
  title_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label->SetAutoColorReadabilityEnabled(false);
  title_label_ = header_->AddChildView(std::move(title_label));
}

void AstraDownloadsShelfView::BuildItemsContainer() {
  auto scroll_view = std::make_unique<views::ScrollView>();
  scroll_view->SetHorizontalScrollBarMode(
      views::ScrollView::ScrollBarMode::kDisabled);
  scroll_view->SetVerticalScrollBarMode(
      views::ScrollView::ScrollBarMode::kDisabled);
  scroll_view_ = AddChildView(std::move(scroll_view));

  auto items_container = std::make_unique<views::View>();
  items_container_ = items_container.get();
  scroll_view_->SetContents(std::move(items_container));
}

void AstraDownloadsShelfView::BuildTrailingControls() {
  auto trailing = std::make_unique<views::View>();
  trailing->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets(0, 0),
      kItemSpacing));
  trailing_ = AddChildView(std::move(trailing));

  // Show all button.
  auto show_all_button = std::make_unique<views::LabelButton>(
      base::BindRepeating(&AstraDownloadsShelfView::OnShowAllClicked,
                          base::Unretained(this)),
      u"Show all");
  show_all_button->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  show_all_button_ = trailing_->AddChildView(std::move(show_all_button));

  // Close button.
  auto close_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraDownloadsShelfView::OnCloseClicked,
                          base::Unretained(this)));
  close_button->SetImageSize(gfx::Size(kCloseButtonSize, kCloseButtonSize));
  close_button_ = trailing_->AddChildView(std::move(close_button));
}

void AstraDownloadsShelfView::RebuildItems() {
  if (!items_container_)
    return;

  // Remove all existing item views.
  items_container_->RemoveAllChildViews();
  item_views_.clear();

  if (!helper_)
    return;

  // Get downloads from helper and add items.
  // Add most recent downloads first, limited by max_items_.
  auto download_ids = helper_->GetAllDownloadIds();
  int count = std::min(static_cast<int>(download_ids.size()), max_items_);

  // Add in reverse order (most recent first).
  for (int i = 0; i < count; ++i) {
    int id = download_ids[download_ids.size() - 1 - i];
    auto item =
        std::make_unique<AstraDownloadsShelfItemView>(id, helper_->GetFilename(id));
    item->set_delegate(this);

    AstraDownloadState state = helper_->GetState(id);
    item->SetState(state);
    item->SetProgress(helper_->GetProgress(id));

    int64_t received, total;
    helper_->GetBytes(id, &received, &total);
    item->SetBytes(received, total);

    item->SetSpeed(helper_->GetSpeed(id));
    item->SetDanger(helper_->IsDangerous(id));

    AstraDownloadsShelfItemView* item_ptr =
        items_container_->AddChildView(std::move(item));
    item_views_.push_back(item_ptr);
  }

  Layout();
}

void AstraDownloadsShelfView::AddDownloadItem(int download_id) {
  if (!items_container_ || !helper_)
    return;

  // Check if already exists.
  for (auto* item : item_views_) {
    if (item->download_id() == download_id) {
      UpdateDownloadItem(download_id);
      return;
    }
  }

  auto item = std::make_unique<AstraDownloadsShelfItemView>(
      download_id, helper_->GetFilename(download_id));
  item->set_delegate(this);

  AstraDownloadState state = helper_->GetState(download_id);
  item->SetState(state);
  item->SetProgress(helper_->GetProgress(download_id));

  int64_t received, total;
  helper_->GetBytes(download_id, &received, &total);
  item->SetBytes(received, total);

  item->SetSpeed(helper_->GetSpeed(download_id));
  item->SetDanger(helper_->IsDangerous(download_id));

  AstraDownloadsShelfItemView* item_ptr =
      items_container_->AddChildViewAt(std::move(item), 0);
  item_views_.insert(item_views_.begin(), item_ptr);

  // Enforce max items.
  while (static_cast<int>(item_views_.size()) > max_items_) {
    auto* last = item_views_.back();
    items_container_->RemoveChildViewT(last);
    item_views_.pop_back();
  }

  Layout();
}

void AstraDownloadsShelfView::RemoveDownloadItem(int download_id) {
  if (!items_container_)
    return;

  for (auto it = item_views_.begin(); it != item_views_.end(); ++it) {
    if ((*it)->download_id() == download_id) {
      items_container_->RemoveChildViewT(*it);
      item_views_.erase(it);
      Layout();
      return;
    }
  }
}

void AstraDownloadsShelfView::UpdateDownloadItem(int download_id) {
  if (!helper_)
    return;

  for (auto* item : item_views_) {
    if (item->download_id() == download_id) {
      item->SetFilename(helper_->GetFilename(download_id));
      item->SetState(helper_->GetState(download_id));
      item->SetProgress(helper_->GetProgress(download_id));

      int64_t received, total;
      helper_->GetBytes(download_id, &received, &total);
      item->SetBytes(received, total);

      item->SetSpeed(helper_->GetSpeed(download_id));
      item->SetDanger(helper_->IsDangerous(download_id));
      return;
    }
  }
}

bool AstraDownloadsShelfView::AllDownloadsComplete() const {
  if (!helper_)
    return true;

  auto ids = helper_->GetAllDownloadIds();
  for (int id : ids) {
    AstraDownloadState state = helper_->GetState(id);
    if (state == AstraDownloadState::kInProgress ||
        state == AstraDownloadState::kPaused) {
      return false;
    }
  }
  return true;
}

void AstraDownloadsShelfView::UpdateColors() {
  if (!GetWidget())
    return;

  SkColor text_color = GetNativeTheme()->GetSystemColor(
      ui::NativeTheme::kColorId_LabelEnabledColor);
  SkColor bg_color = GetNativeTheme()->GetSystemColor(
      ui::NativeTheme::kColorId_WindowBackground);

  // Set background.
  SetBackground(views::CreateSolidBackground(bg_color));

  if (title_label_)
    title_label_->SetEnabledColor(text_color);

  // Draw download icon.
  if (icon_view_) {
    gfx::Canvas canvas(gfx::Size(16, 16), /*image_scale=*/1.0f, false);
    DrawDownloadIcon(&canvas, gfx::Rect(0, 0, 16, 16), text_color);
    icon_view_->SetImage(
        gfx::ImageSkia(canvas.GetBitmap(), gfx::Size(16, 16)));
  }
}

void AstraDownloadsShelfView::MaybeStartAutoHide() {
  if (!auto_hide_ || !shelf_visible_)
    return;

  if (AllDownloadsComplete() && !auto_hide_timer_.IsRunning()) {
    HideAfterDelay(auto_hide_delay_);
  }
}

// -- Button handlers --------------------------------------------------------

void AstraDownloadsShelfView::OnShowAllClicked() {
  if (delegate_)
    delegate_->OnShowAllDownloads();
}

void AstraDownloadsShelfView::OnCloseClicked() {
  Hide();
  if (delegate_)
    delegate_->OnShelfClosed();
}

// -- AstraDownloadsShelfItemDelegate ----------------------------------------

void AstraDownloadsShelfView::OnDownloadClicked(int download_id) {
  if (delegate_)
    delegate_->OnOpenDownload(download_id);
}

void AstraDownloadsShelfView::OnPauseDownload(int download_id) {
  if (helper_)
    helper_->PauseDownload(download_id);
}

void AstraDownloadsShelfView::OnResumeDownload(int download_id) {
  if (helper_)
    helper_->ResumeDownload(download_id);
}

void AstraDownloadsShelfView::OnCancelDownload(int download_id) {
  if (helper_)
    helper_->CancelDownload(download_id);
}

void AstraDownloadsShelfView::OnShowInFolder(int download_id) {
  if (delegate_)
    delegate_->OnShowDownloadInFolder(download_id);
}

void AstraDownloadsShelfView::OnDismissItem(int download_id) {
  if (helper_)
    helper_->RemoveDownload(download_id);
}

// -- Auto-hide --------------------------------------------------------------

void AstraDownloadsShelfView::OnAutoHideTimerFired() {
  if (AllDownloadsComplete() && shelf_visible_)
    Hide();
}

// -- Animation callbacks ----------------------------------------------------

void AstraDownloadsShelfView::OnShowAnimationComplete() {
  // TODO(astra): Hook into slide-in animation completion.
}

void AstraDownloadsShelfView::OnHideAnimationComplete() {
  SetVisible(false);
  // TODO(astra): Hook into slide-out animation completion.
}

// -- AstraDownloadsShelfItemDelegate ----------------------------------------
// (inherited implicitly — shelf acts as item delegate)

// Note: The shelf view implements AstraDownloadsShelfItemDelegate by
// forwarding item actions to its own Delegate. These are defined inline
// as part of the delegate contract.

}  // namespace astra
