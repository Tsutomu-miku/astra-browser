// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_containers/astra_tab_containers_view.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkCanvas.h"
#include "skia/core/SkColor.h"
#include "skia/core/SkPaint.h"
#include "skia/core/SkRRect.h"
#include "skia/core/SkRect.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/style/typography.h"
#include "ui/views/view_class_properties.h"

namespace astra {

namespace {

constexpr int kBubbleWidth = 380;
constexpr int kItemHeight = 48;
constexpr int kSectionPadding = 16;
constexpr int kItemSpacing = 4;
constexpr int kMaxVisibleItems = 6;
constexpr int kColorDotSize = 12;

}  // namespace

// ===========================================================================
// AstraContainerItemView
// ===========================================================================

AstraContainerItemView::AstraContainerItemView(
    const ContainerInfo& info,
    SelectCallback select_callback,
    EditCallback edit_callback,
    DeleteCallback delete_callback)
    : container_id_(info.container_id),
      name_(info.name),
      color_(info.color),
      tab_count_(info.tab_count),
      is_default_(info.is_default),
      is_active_(info.is_active),
      select_callback_(std::move(select_callback)),
      edit_callback_(std::move(edit_callback)),
      delete_callback_(std::move(delete_callback)) {
  BuildLayout();
}

AstraContainerItemView::~AstraContainerItemView() = default;

void AstraContainerItemView::SetActive(bool active) {
  is_active_ = active;
  SchedulePaint();
}

void AstraContainerItemView::BuildLayout() {
  SetPreferredSize(
      gfx::Size(kBubbleWidth - kSectionPadding * 2, kItemHeight));
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(0, 8),
      12));
  SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  SetBorder(views::CreateRoundedRectBorder(
      1, 8, SkColorSetA(SK_ColorGRAY, 0x40)));

  // Color dot (painted).
  auto* dot_spacer = AddChildView(std::make_unique<views::View>());
  dot_spacer->SetPreferredSize(gfx::Size(kColorDotSize, kColorDotSize));

  // Name.
  name_label_ = AddChildView(std::make_unique<views::Label>(name_));
  name_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  name_label_->SetAutoColorReadabilityEnabled(false);
  name_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));
  name_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);
  name_label_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  // Tab count.
  std::u16string count_text =
      base::UTF8ToUTF16(std::to_string(tab_count_) + " tabs");
  count_label_ = AddChildView(std::make_unique<views::Label>(count_text));
  count_label_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  count_label_->SetAutoColorReadabilityEnabled(false);
  count_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));

  // Open button.
  open_button_ = AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(&AstraContainerItemView::OnSelect,
                              base::Unretained(this)),
          u"Open"));
}

void AstraContainerItemView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);
  PaintColorDot(canvas);
}

void AstraContainerItemView::PaintColorDot(gfx::Canvas* canvas) {
  // Draw color dot in the left padding area.
  int dot_x = 8 + kColorDotSize / 2;
  int dot_y = kItemHeight / 2;

  SkPaint paint;
  paint.setColor(color_);
  paint.setStyle(SkPaint::kFill_Style);
  paint.setAntiAlias(true);

  canvas->DrawCircle(gfx::Point(dot_x, dot_y), kColorDotSize / 2, paint);

  // Active ring.
  if (is_active_) {
    SkPaint ring_paint;
    ring_paint.setColor(SkColorSetA(color_, 0x40));
    ring_paint.setStyle(SkPaint::kStroke_Style);
    ring_paint.setStrokeWidth(2);
    ring_paint.setAntiAlias(true);
    canvas->DrawCircle(
        gfx::Point(dot_x, dot_y), kColorDotSize / 2 + 3, ring_paint);
  }
}

void AstraContainerItemView::OnSelect() {
  if (select_callback_) {
    select_callback_.Run(container_id_);
  }
}

void AstraContainerItemView::OnEdit() {
  if (edit_callback_) {
    edit_callback_.Run(container_id_);
  }
}

void AstraContainerItemView::OnDelete() {
  if (delete_callback_) {
    delete_callback_.Run(container_id_);
  }
}

void AstraContainerItemView::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  name_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
  count_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));
}

// ===========================================================================
// AstraTabContainersView
// ===========================================================================

AstraTabContainersView::AstraTabContainersView(views::View* anchor_view)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_RIGHT) {
  SetShowCloseButton(true);
  SetButtons(ui::DIALOG_BUTTON_NONE);
  set_fixed_width(kBubbleWidth);

  BuildUI();
}

AstraTabContainersView::~AstraTabContainersView() = default;

void AstraTabContainersView::SetContainers(
    const std::vector<AstraContainerItemView::ContainerInfo>& containers) {
  containers_ = containers;
  RefreshContainers();
}

void AstraTabContainersView::SetActiveContainer(
    const std::string& container_id) {
  active_container_id_ = container_id;
  for (auto* view : container_views_) {
    view->SetActive(view->container_id() == container_id);
  }
}

void AstraTabContainersView::SetContainerSelectedCallback(
    ContainerSelectedCallback callback) {
  select_callback_ = std::move(callback);
}

void AstraTabContainersView::SetContainerCreateCallback(
    ContainerCreateCallback callback) {
  create_callback_ = std::move(callback);
}

void AstraTabContainersView::SetContainerEditCallback(
    ContainerEditCallback callback) {
  edit_callback_ = std::move(callback);
}

void AstraTabContainersView::SetContainerDeleteCallback(
    ContainerDeleteCallback callback) {
  delete_callback_ = std::move(callback);
}

void AstraTabContainersView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  set_margins(gfx::Insets::VH(0, 0));

  BuildHeader();
  BuildContainerList();
}

void AstraTabContainersView::BuildHeader() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 12));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  new_button_ = section->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraTabContainersView::OnNewContainer,
              base::Unretained(this)),
          u"+ New Container"));
}

void AstraTabContainersView::BuildContainerList() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), kItemSpacing));
  section->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  auto* scroll_view = section->AddChildView(
      std::make_unique<views::ScrollView>());
  scroll_view->SetClipHeight(kItemHeight * kMaxVisibleItems +
                              kItemSpacing * (kMaxVisibleItems - 1));
  scroll_view->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  containers_list_ = scroll_view->SetContents(
      std::make_unique<views::View>());
  containers_list_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(), kItemSpacing));
}

void AstraTabContainersView::RefreshContainers() {
  if (!containers_list_) return;

  containers_list_->RemoveAllChildViews();
  container_views_.clear();

  for (const auto& container : containers_) {
    auto info = container;
    info.is_active = (container.container_id == active_container_id_);

    auto* item = containers_list_->AddChildView(
        std::make_unique<AstraContainerItemView>(
            info,
            base::BindRepeating(
                &AstraTabContainersView::SetActiveContainer,
                base::Unretained(this)),
            base::BindRepeating(
                [](ContainerEditCallback cb, const std::string& id) {
                  if (cb) cb.Run(id);
                },
                edit_callback_),
            base::BindRepeating(
                [](ContainerDeleteCallback cb, const std::string& id) {
                  if (cb) cb.Run(id);
                },
                delete_callback_)));
    container_views_.push_back(item);
  }

  InvalidateLayout();
}

void AstraTabContainersView::OnNewContainer() {
  if (create_callback_) {
    create_callback_.Run();
  }
}

std::u16string AstraTabContainersView::GetWindowTitle() const {
  return u"Tab Containers";
}

void AstraTabContainersView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
  // Sub-views handle their own theme changes.
}

}  // namespace astra
