#include "astra/ui/views/downloads/astra_downloads_bubble_view.h"

#include <algorithm>

#include "base/i18n/number_formatting.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/flex_layout_types.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/widget/widget.h"

#include "astra/ui/views/downloads/astra_downloads_bubble_item_view.h"
#include "astra/ui/views/downloads/astra_downloads_bubble_model.h"

namespace astra {

// static
views::Widget* AstraDownloadsBubbleView::ShowBubble(
    views::View* anchor_view,
    const gfx::Rect& anchor_rect,
    AstraDownloadsBubbleModel* model,
    AstraDownloadsBubbleDelegate* delegate) {
  auto* bubble = new AstraDownloadsBubbleView(anchor_view, anchor_rect, model,
                                              delegate);
  views::Widget* widget =
      views::BubbleDialogDelegateView::CreateBubble(bubble);
  widget->Show();
  return widget;
}

AstraDownloadsBubbleView::AstraDownloadsBubbleView(
    views::View* anchor_view,
    const gfx::Rect& anchor_rect,
    AstraDownloadsBubbleModel* model,
    AstraDownloadsBubbleDelegate* delegate)
    : views::BubbleDialogDelegateView(anchor_view,
                                      views::BubbleBorder::TOP_RIGHT,
                                      views::BubbleBorder::STANDARD_SHADOW),
      model_(model),
      delegate_(delegate) {
  set_close_on_deactivate(true);
  set_close_on_esc(true);
  SetAnchorRect(anchor_rect);
}

AstraDownloadsBubbleView::~AstraDownloadsBubbleView() = default;

void AstraDownloadsBubbleView::SetModel(AstraDownloadsBubbleModel* model) {
  model_ = model;
  RefreshFromModel();
}

void AstraDownloadsBubbleView::RefreshFromModel() {
  if (!model_) {
    return;
  }
  RebuildItems();
  UpdateHeader();
  UpdateActiveCountBadge();

  bool has_items = !item_views_.empty();
  if (has_items) {
    HideEmptyState();
  } else {
    ShowEmptyState();
  }
}

void AstraDownloadsBubbleView::UpdateDownloadItem(
    const std::string& download_id) {
  auto* item_view = FindItemView(download_id);
  if (item_view && model_) {
    // Find the item data from the model.
    auto items = model_->GetDisplayDownloads();
    for (const auto& item : items) {
      if (item.id == download_id) {
        item_view->UpdateFromItem(item);
        break;
      }
    }
  }
}

void AstraDownloadsBubbleView::AddDownloadItem(
    const AstraDownloadsBubbleItem& item) {
  // Prepend the new item (newest first).
  auto item_view = CreateItemView(item);
  item_view->set_delegate(this);
  auto* raw = item_view.get();
  if (items_container_) {
    items_container_->AddChildViewAt(std::move(item_view), 0);
  }
  item_views_.insert(item_views_.begin(), raw);
  UpdateHeader();
  UpdateActiveCountBadge();
  HideEmptyState();
}

void AstraDownloadsBubbleView::RemoveDownloadItem(
    const std::string& download_id) {
  auto* item_view = FindItemView(download_id);
  if (item_view && items_container_) {
    // Remove from vector.
    auto it = std::find(item_views_.begin(), item_views_.end(), item_view);
    if (it != item_views_.end()) {
      item_views_.erase(it);
    }
    // Remove from view hierarchy.
    items_container_->RemoveChildViewT(item_view);
  }
  UpdateHeader();
  UpdateActiveCountBadge();
  if (item_views_.empty()) {
    ShowEmptyState();
  }
}

void AstraDownloadsBubbleView::ShowEmptyState() {
  if (empty_state_view_) {
    empty_state_view_->SetVisible(true);
  }
  if (scroll_view_) {
    scroll_view_->SetVisible(false);
  }
  if (footer_) {
    footer_->SetVisible(false);
  }
}

void AstraDownloadsBubbleView::HideEmptyState() {
  if (empty_state_view_) {
    empty_state_view_->SetVisible(false);
  }
  if (scroll_view_) {
    scroll_view_->SetVisible(true);
  }
  if (footer_) {
    footer_->SetVisible(true);
  }
}

void AstraDownloadsBubbleView::UpdateActiveCountBadge() {
  if (!active_count_badge_ || !model_) {
    return;
  }
  int count = model_->GetActiveDownloadCount();
  if (count > 0) {
    active_count_badge_->SetText(base::NumberToString16(count));
    active_count_badge_->SetVisible(true);
  } else {
    active_count_badge_->SetVisible(false);
  }
}

void AstraDownloadsBubbleView::Init() {
  SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kVertical);

  BuildHeader();
  BuildContentArea();
  BuildEmptyState();
  BuildFooter();

  if (model_) {
    RefreshFromModel();
  } else {
    ShowEmptyState();
  }
}

void AstraDownloadsBubbleView::BuildHeader() {
  header_ = AddChildView(std::make_unique<views::View>());
  header_->SetPreferredSize(gfx::Size(kBubbleWidth, kHeaderHeight));
  header_->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));
  auto* layout = header_->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);
  layout->SetInteriorMargin(gfx::Insets::VH(0, 16));

  // Title
  title_label_ = header_->AddChildView(std::make_unique<views::Label>());
  title_label_->SetText(u"Downloads");
  title_label_->SetFontList(
      title_label_->font_list().DeriveWithSizeDelta(2));
  title_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::LayoutOrientation::kHorizontal,
                               views::MinimumFlexSizeRule::kScaleToZero,
                               views::MaximumFlexSizeRule::kUnbounded,
                               /*weight=*/1.0f));

  // Active count badge
  active_count_badge_ =
      header_->AddChildView(std::make_unique<views::Label>());
  active_count_badge_->SetText(u"0");
  active_count_badge_->SetVisible(false);
  active_count_badge_->SetProperty(views::kMarginsKey,
                                    gfx::Insets::TLBR(0, 0, 0, 8));

  // Settings button
  settings_button_ =
      header_->AddChildView(std::make_unique<views::ImageButton>(
          base::BindRepeating(
              &AstraDownloadsBubbleView::OnSettingsButtonPressed,
              base::Unretained(this))));
  settings_button_->SetPreferredSize(gfx::Size(24, 24));
  settings_button_->SetTooltipText(u"Download settings");
}

void AstraDownloadsBubbleView::BuildContentArea() {
  scroll_view_ =
      AddChildView(std::make_unique<views::ScrollView>(
          views::ScrollView::ScrollWithLayers::kEnabled));
  scroll_view_->SetBackgroundColor(absl::nullopt);
  scroll_view_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::LayoutOrientation::kVertical,
                               views::MinimumFlexSizeRule::kScaleToZero,
                               views::MaximumFlexSizeRule::kUnbounded,
                               /*weight=*/1.0f));

  items_container_ = scroll_view_->SetContents(
      std::make_unique<views::View>());
  auto* layout = items_container_->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kVertical);
}

void AstraDownloadsBubbleView::BuildEmptyState() {
  empty_state_view_ =
      AddChildView(std::make_unique<views::View>());
  empty_state_view_->SetPreferredSize(gfx::Size(kBubbleWidth, 120));
  empty_state_view_->SetVisible(false);
  auto* layout = empty_state_view_->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kVertical);
  layout->SetMainAxisAlignment(views::LayoutAlignment::kCenter);
  layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);

  auto* icon = empty_state_view_->AddChildView(
      std::make_unique<views::ImageView>());
  icon->SetPreferredSize(gfx::Size(48, 48));
  icon->SetProperty(views::kMarginsKey, gfx::Insets::TLBR(0, 0, 12, 0));

  auto* label = empty_state_view_->AddChildView(
      std::make_unique<views::Label>());
  label->SetText(u"No downloads");
  label->SetHorizontalAlignment(gfx::ALIGN_CENTER);
}

void AstraDownloadsBubbleView::BuildFooter() {
  footer_ = AddChildView(std::make_unique<views::View>());
  footer_->SetPreferredSize(gfx::Size(kBubbleWidth, kFooterHeight));
  footer_->SetBorder(views::CreateSolidSidedBorder(
      1, 0, 0, 0, SK_ColorGRAY));
  auto* layout = footer_->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);
  layout->SetMainAxisAlignment(views::LayoutAlignment::kSpaceBetween);
  layout->SetInteriorMargin(gfx::Insets::VH(0, 16));

  // "Show all" button
  show_all_button_ =
      footer_->AddChildView(std::make_unique<views::LabelButton>(
          base::BindRepeating(
              &AstraDownloadsBubbleView::OnShowAllButtonPressed,
              base::Unretained(this)),
          u"Show all"));
  show_all_button_->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  // "Clear all" button
  clear_all_button_ =
      footer_->AddChildView(std::make_unique<views::LabelButton>(
          base::BindRepeating(
              &AstraDownloadsBubbleView::OnClearAllButtonPressed,
              base::Unretained(this)),
          u"Clear all"));
  clear_all_button_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
}

void AstraDownloadsBubbleView::RebuildItems() {
  if (!items_container_ || !model_) {
    return;
  }

  // Clear existing items.
  items_container_->RemoveAllChildViews();
  item_views_.clear();

  auto items = model_->GetDisplayDownloads();
  for (const auto& item : items) {
    auto item_view = CreateItemView(item);
    item_view->set_delegate(this);
    item_views_.push_back(
        items_container_->AddChildView(std::move(item_view)));
  }

  items_container_->InvalidateLayout();
}

AstraDownloadsBubbleItemView*
AstraDownloadsBubbleView::FindItemView(
    const std::string& download_id) const {
  for (auto* view : item_views_) {
    if (view->download_id() == download_id) {
      return view;
    }
  }
  return nullptr;
}

std::unique_ptr<AstraDownloadsBubbleItemView>
AstraDownloadsBubbleView::CreateItemView(
    const AstraDownloadsBubbleItem& item) {
  auto view =
      std::make_unique<AstraDownloadsBubbleItemView>(item.id);
  view->UpdateFromItem(item);
  if (model_) {
    view->SetShowFileSize(model_->show_file_size());
    view->SetShowSpeed(model_->show_speed());
    view->SetShowTimeRemaining(model_->show_time_remaining());
  }
  return view;
}

void AstraDownloadsBubbleView::UpdateColors() {
  // Colors would be updated from the color provider in production.
  // For now, this is a placeholder.
}

void AstraDownloadsBubbleView::UpdateHeader() {
  UpdateActiveCountBadge();
}

void AstraDownloadsBubbleView::OnSettingsButtonPressed() {
  if (delegate_) {
    delegate_->OnDownloadsSettingsRequested();
  }
}

void AstraDownloadsBubbleView::OnClearAllButtonPressed() {
  if (delegate_) {
    delegate_->OnClearAllDownloadsRequested();
  }
}

void AstraDownloadsBubbleView::OnShowAllButtonPressed() {
  if (delegate_) {
    delegate_->OnShowAllDownloadsRequested();
  }
  GetWidget()->Close();
}

void AstraDownloadsBubbleView::OnWidgetDestroying(views::Widget* widget) {
  if (delegate_) {
    delegate_->OnDownloadsBubbleClosing();
  }
  views::BubbleDialogDelegateView::OnWidgetDestroying(widget);
}

bool AstraDownloadsBubbleView::AcceleratorPressed(
    const ui::Accelerator& accelerator) {
  return views::BubbleDialogDelegateView::AcceleratorPressed(accelerator);
}

gfx::Size AstraDownloadsBubbleView::CalculatePreferredSize() const {
  int height = kHeaderHeight + kFooterHeight;
  if (model_) {
    int item_count = static_cast<int>(model_->GetDisplayCount());
    if (item_count > 0) {
      height +=
          item_count * AstraDownloadsBubbleItemView::kItemHeight +
          (item_count - 1) * kItemSpacing;
    } else {
      // Empty state
      height += 120;
    }
  }
  height = std::min(height, kMaxBubbleHeight);
  return gfx::Size(kBubbleWidth, height);
}

void AstraDownloadsBubbleView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
  UpdateColors();
}

void AstraDownloadsBubbleView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  views::BubbleDialogDelegateView::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kDialog;
  node_data->SetName(u"Downloads");
}

// -- AstraDownloadsBubbleItemDelegate -----------------------------------

void AstraDownloadsBubbleView::OnDownloadItemClicked(
    const std::string& download_id) {
  if (model_) {
    model_->OpenDownload(download_id);
  }
}

void AstraDownloadsBubbleView::OnPauseDownload(
    const std::string& download_id) {
  if (model_) {
    model_->PauseDownload(download_id);
  }
}

void AstraDownloadsBubbleView::OnResumeDownload(
    const std::string& download_id) {
  if (model_) {
    model_->ResumeDownload(download_id);
  }
}

void AstraDownloadsBubbleView::OnCancelDownload(
    const std::string& download_id) {
  if (model_) {
    model_->CancelDownload(download_id);
  }
}

void AstraDownloadsBubbleView::OnOpenDownload(
    const std::string& download_id) {
  if (model_) {
    model_->OpenDownload(download_id);
  }
  GetWidget()->Close();
}

void AstraDownloadsBubbleView::OnShowDownloadInFolder(
    const std::string& download_id) {
  if (model_) {
    model_->ShowDownloadInFolder(download_id);
  }
}

void AstraDownloadsBubbleView::OnRetryDownload(
    const std::string& download_id) {
  if (model_) {
    model_->RetryDownload(download_id);
  }
}

void AstraDownloadsBubbleView::OnRemoveDownload(
    const std::string& download_id) {
  if (model_) {
    model_->RemoveDownload(download_id);
  }
}

}  // namespace astra
