// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/split_view/astra_split_layout_picker_view.h"

#include <memory>
#include <string>

#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkColor.h"
#include "skia/core/SkPath.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/background.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/style/typography.h"
#include "ui/views/view_class_properties.h"

namespace astra {

namespace {

constexpr int kPreviewWidth = 64;
constexpr int kPreviewHeight = 48;
constexpr int kPreviewRadius = 4;
constexpr int kGridSpacing = 12;
constexpr int kSectionPadding = 12;
constexpr int kPickerWidth = 360;

// Draw a 2-pane horizontal layout preview.
void DrawTwoPaneHorizontalPreview(gfx::Canvas* canvas,
                                  const gfx::Rect& bounds,
                                  float left_ratio,
                                  SkColor color,
                                  SkColor bg_color) {
  int w = bounds.width();
  int h = bounds.height();
  int divider_x = bounds.x() + static_cast<int>(w * left_ratio);

  // Left pane.
  cc::PaintFlags left_flags;
  left_flags.setColor(color);
  left_flags.setStyle(cc::PaintFlags::kFill_Style);
  left_flags.setAntiAlias(true);
  canvas->DrawRoundRect(
      gfx::Rect(bounds.x(), bounds.y(),
                divider_x - bounds.x() - 1, h),
      kPreviewRadius, left_flags);

  // Right pane (lighter).
  cc::PaintFlags right_flags;
  right_flags.setColor(SkColorSetA(color, 0x40));
  right_flags.setStyle(cc::PaintFlags::kFill_Style);
  right_flags.setAntiAlias(true);
  canvas->DrawRoundRect(
      gfx::Rect(divider_x + 1, bounds.y(),
                bounds.right() - divider_x - 1, h),
      kPreviewRadius, right_flags);

  // Divider line.
  cc::PaintFlags divider_flags;
  divider_flags.setColor(bg_color);
  divider_flags.setStrokeWidth(2);
  divider_flags.setStyle(cc::PaintFlags::kStroke_Style);
  divider_flags.setAntiAlias(true);
  canvas->DrawLine(gfx::Point(divider_x, bounds.y()),
                   gfx::Point(divider_x, bounds.bottom()),
                   divider_flags);
}

// Draw a 2-pane vertical layout preview.
void DrawTwoPaneVerticalPreview(gfx::Canvas* canvas,
                                const gfx::Rect& bounds,
                                float top_ratio,
                                SkColor color,
                                SkColor bg_color) {
  int w = bounds.width();
  int h = bounds.height();
  int divider_y = bounds.y() + static_cast<int>(h * top_ratio);

  // Top pane.
  cc::PaintFlags top_flags;
  top_flags.setColor(color);
  top_flags.setStyle(cc::PaintFlags::kFill_Style);
  top_flags.setAntiAlias(true);
  canvas->DrawRoundRect(
      gfx::Rect(bounds.x(), bounds.y(), w, divider_y - bounds.y() - 1),
      kPreviewRadius, top_flags);

  // Bottom pane.
  cc::PaintFlags bottom_flags;
  bottom_flags.setColor(SkColorSetA(color, 0x40));
  bottom_flags.setStyle(cc::PaintFlags::kFill_Style);
  bottom_flags.setAntiAlias(true);
  canvas->DrawRoundRect(
      gfx::Rect(bounds.x(), divider_y + 1,
                w, bounds.bottom() - divider_y - 1),
      kPreviewRadius, bottom_flags);
}

// Draw a 3-pane horizontal layout preview.
void DrawThreePaneHorizontalPreview(gfx::Canvas* canvas,
                                    const gfx::Rect& bounds,
                                    SkColor color) {
  int w = bounds.width();
  int h = bounds.height();
  int pane_w = w / 3;

  for (int i = 0; i < 3; i++) {
    cc::PaintFlags flags;
    flags.setColor(SkColorSetA(color, 0xFF - i * 0x40));
    flags.setStyle(cc::PaintFlags::kFill_Style);
    flags.setAntiAlias(true);
    int x = bounds.x() + i * pane_w + (i > 0 ? 1 : 0);
    int pw = pane_w - (i < 2 ? 1 : 0);
    canvas->DrawRoundRect(gfx::Rect(x, bounds.y(), pw, h),
                          kPreviewRadius, flags);
  }
}

// Draw a 2x2 grid layout preview.
void DrawGridTwoByTwoPreview(gfx::Canvas* canvas,
                             const gfx::Rect& bounds,
                             SkColor color) {
  int w = bounds.width();
  int h = bounds.height();
  int half_w = w / 2;
  int half_h = h / 2;

  // Four quadrants with varying opacity.
  for (int row = 0; row < 2; row++) {
    for (int col = 0; col < 2; col++) {
      int alpha = 0xFF - (row * 2 + col) * 0x30;
      cc::PaintFlags flags;
      flags.setColor(SkColorSetA(color, alpha));
      flags.setStyle(cc::PaintFlags::kFill_Style);
      flags.setAntiAlias(true);
      int x = bounds.x() + col * half_w + (col > 0 ? 1 : 0);
      int y = bounds.y() + row * half_h + (row > 0 ? 1 : 0);
      int pw = half_w - (col < 1 ? 0 : 1);
      int ph = half_h - (row < 1 ? 0 : 1);
      canvas->DrawRoundRect(gfx::Rect(x, y, pw, ph),
                            kPreviewRadius / 2, flags);
    }
  }
}

// Draw a picture-in-picture layout preview.
void DrawPiPPreview(gfx::Canvas* canvas,
                     const gfx::Rect& bounds,
                     SkColor color) {
  // Main content.
  cc::PaintFlags main_flags;
  main_flags.setColor(SkColorSetA(color, 0x60));
  main_flags.setStyle(cc::PaintFlags::kFill_Style);
  main_flags.setAntiAlias(true);
  canvas->DrawRoundRect(bounds, kPreviewRadius, main_flags);

  // PiP window (bottom right corner).
  int pip_w = bounds.width() / 3;
  int pip_h = bounds.height() / 3;
  gfx::Rect pip_rect(bounds.right() - pip_w - 3,
                     bounds.bottom() - pip_h - 3,
                     pip_w, pip_h);

  cc::PaintFlags pip_flags;
  pip_flags.setColor(color);
  pip_flags.setStyle(cc::PaintFlags::kFill_Style);
  pip_flags.setAntiAlias(true);
  canvas->DrawRoundRect(pip_rect, 2, pip_flags);
}

// Draw a tab shift layout preview.
void DrawTabShiftPreview(gfx::Canvas* canvas,
                         const gfx::Rect& bounds,
                         SkColor color) {
  int tab_w = bounds.width() / 5;

  // Tab strip (left).
  cc::PaintFlags tab_flags;
  tab_flags.setColor(color);
  tab_flags.setStyle(cc::PaintFlags::kFill_Style);
  tab_flags.setAntiAlias(true);
  canvas->DrawRoundRect(
      gfx::Rect(bounds.x(), bounds.y(), tab_w, bounds.height()),
      kPreviewRadius, tab_flags);

  // Main content (right).
  cc::PaintFlags main_flags;
  main_flags.setColor(SkColorSetA(color, 0x30));
  main_flags.setStyle(cc::PaintFlags::kFill_Style);
  main_flags.setAntiAlias(true);
  canvas->DrawRoundRect(
      gfx::Rect(bounds.x() + tab_w + 1, bounds.y(),
                bounds.width() - tab_w - 1, bounds.height()),
      kPreviewRadius, main_flags);
}

}  // namespace

// ===========================================================================
// AstraSplitLayoutPreviewView
// ===========================================================================

AstraSplitLayoutPreviewView::AstraSplitLayoutPreviewView(
    const AstraSplitLayout& layout,
    base::RepeatingCallback<void()> callback)
    : views::Button(std::move(callback)),
      layout_id_(layout.id),
      layout_name_(layout.name) {
  BuildUI();
}

AstraSplitLayoutPreviewView::~AstraSplitLayoutPreviewView() = default;

void AstraSplitLayoutPreviewView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(), 4));
  SetPreferredSize(gfx::Size(kPreviewWidth, kPreviewHeight + 20));
  SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  // Preview area.
  preview_area_ = AddChildView(std::make_unique<views::View>());
  preview_area_->SetPreferredSize(gfx::Size(kPreviewWidth, kPreviewHeight));
  preview_area_->SetBackground(
      views::CreateRoundedRectBackground(
          SK_ColorTRANSPARENT, kPreviewRadius));

  // Name label.
  name_label_ = AddChildView(std::make_unique<views::Label>(layout_name_));
  name_label_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  name_label_->SetAutoColorReadabilityEnabled(false);
  name_label_->SetFontList(
      views::style::GetFont(views::style::CONTEXT_LABEL,
                            views::style::STYLE_SECONDARY));

  // Set tooltip.
  SetTooltipText(layout_name_);
}

void AstraSplitLayoutPreviewView::OnThemeChanged() {
  views::Button::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  name_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));

  // Redraw preview.
  preview_area_->SetBackground(
      views::CreateRoundedRectBackground(
          cp->GetColor(ui::kColorSubtleEmphasisBackground),
          kPreviewRadius));
}

// ===========================================================================
// AstraSplitLayoutPickerView
// ===========================================================================

AstraSplitLayoutPickerView::AstraSplitLayoutPickerView(
    views::View* anchor_view,
    AstraSplitLayoutsModel* model)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_RIGHT),
      model_(model) {
  SetShowCloseButton(false);
  SetButtons(ui::DIALOG_BUTTON_NONE);
  set_fixed_width(kPickerWidth);

  BuildUI();

  if (model_) {
    scoped_observation_.Observe(model_);
  }
  RefreshFromModel();
}

AstraSplitLayoutPickerView::~AstraSplitLayoutPickerView() = default;

void AstraSplitLayoutPickerView::SetModel(
    AstraSplitLayoutsModel* model) {
  if (model_ == model) return;
  scoped_observation_.Reset();
  model_ = model;
  if (model_) {
    scoped_observation_.Observe(model_);
  }
  RefreshFromModel();
}

void AstraSplitLayoutPickerView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  set_margins(gfx::Insets::VH(0, 0));

  // Header.
  BuildHeader();

  // Built-in layouts section.
  BuildBuiltInSection();

  // User layouts section.
  BuildUserSection();
}

void AstraSplitLayoutPickerView::BuildHeader() {
  auto* header = AddChildView(std::make_unique<views::View>());
  header->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(12, 16), 8));
  header->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  auto* title = header->AddChildView(
      std::make_unique<views::Label>(u"Split Layouts"));
  title->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title->SetAutoColorReadabilityEnabled(false);
  title->SetFontList(
      views::style::GetFont(views::style::CONTEXT_DIALOG_TITLE,
                            views::style::STYLE_PRIMARY));
  title->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  save_button_ = header->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraSplitLayoutPickerView::OnSaveClicked,
              base::Unretained(this)),
          u"Save layout"));
}

void AstraSplitLayoutPickerView::BuildBuiltInSection() {
  builtin_section_ = AddChildView(std::make_unique<views::View>());
  builtin_section_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(0, kSectionPadding), 8));

  auto* label = builtin_section_->AddChildView(
      std::make_unique<views::Label>(u"Presets"));
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SetAutoColorReadabilityEnabled(false);
  label->SetFontList(
      views::style::GetFont(views::style::CONTEXT_LABEL,
                            views::style::STYLE_SECONDARY));

  builtin_grid_ = builtin_section_->AddChildView(
      std::make_unique<views::View>());
  builtin_grid_->SetLayoutManager(
      std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kHorizontal)
      .SetDefault(views::kFlexBehaviorKey,
                  views::FlexSpecification(
                      views::LayoutFlexOrientation::kHorizontal,
                      views::MinimumFlexSizeRule::kPreferred,
                      views::MaximumFlexSizeRule::kPreferred))
      .SetWrapBehavior(views::FlexLayout::WrapBehavior::kWrap);
}

void AstraSplitLayoutPickerView::BuildUserSection() {
  user_section_ = AddChildView(std::make_unique<views::View>());
  user_section_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 8));
  user_section_->SetVisible(false);

  auto* label = user_section_->AddChildView(
      std::make_unique<views::Label>(u"Your layouts"));
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SetAutoColorReadabilityEnabled(false);
  label->SetFontList(
      views::style::GetFont(views::style::CONTEXT_LABEL,
                            views::style::STYLE_SECONDARY));

  user_grid_ = user_section_->AddChildView(
      std::make_unique<views::View>());
  user_grid_->SetLayoutManager(
      std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kHorizontal)
      .SetDefault(views::kFlexBehaviorKey,
                  views::FlexSpecification(
                      views::LayoutFlexOrientation::kHorizontal,
                      views::MinimumFlexSizeRule::kPreferred,
                      views::MaximumFlexSizeRule::kPreferred))
      .SetWrapBehavior(views::FlexLayout::WrapBehavior::kWrap);
}

void AstraSplitLayoutPickerView::RefreshFromModel() {
  if (!model_) return;
  RebuildLayouts();
}

void AstraSplitLayoutPickerView::RebuildLayouts() {
  if (!builtin_grid_ || !user_grid_) return;

  builtin_grid_->RemoveAllChildViews();
  user_grid_->RemoveAllChildViews();

  auto layouts = model_->GetAllLayouts();
  size_t user_count = 0;

  for (const auto& layout : layouts) {
    auto* preview = layout.is_builtin
        ? builtin_grid_->AddChildView(
            std::make_unique<AstraSplitLayoutPreviewView>(
                layout,
                base::BindRepeating(
                    &AstraSplitLayoutPickerView::OnLayoutClicked,
                    base::Unretained(this),
                    layout.id)))
        : user_grid_->AddChildView(
            std::make_unique<AstraSplitLayoutPreviewView>(
                layout,
                base::BindRepeating(
                    &AstraSplitLayoutPickerView::OnLayoutClicked,
                    base::Unretained(this),
                    layout.id)));
    if (!layout.is_builtin) user_count++;
  }

  // Show user section only if there are user layouts.
  user_section_->SetVisible(user_count > 0);

  InvalidateLayout();
  SizeToContents();
}

// -- Observer callbacks ----------------------------------------------------

void AstraSplitLayoutPickerView::OnLayoutSaved(
    const AstraSplitLayout& layout) {
  RebuildLayouts();
}

void AstraSplitLayoutPickerView::OnLayoutDeleted(
    const std::string& layout_id) {
  RebuildLayouts();
}

void AstraSplitLayoutPickerView::OnLayoutApplied(
    const std::string& layout_id) {
  // Visual feedback could be added here.
}

void AstraSplitLayoutPickerView::OnLayoutsReordered() {
  RebuildLayouts();
}

void AstraSplitLayoutPickerView::OnSplitLayoutsModelShutdown() {
  model_ = nullptr;
  scoped_observation_.Reset();
}

// -- Bubble dialog ---------------------------------------------------------

std::u16string AstraSplitLayoutPickerView::GetWindowTitle() const {
  return u"Split Layouts";
}

void AstraSplitLayoutPickerView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
  // Update child views.
}

// -- Event handlers --------------------------------------------------------

void AstraSplitLayoutPickerView::OnSaveClicked() {
  // TODO(astra): Save current split view configuration as a new layout.
  //   Would prompt for name and save to the model.
}

void AstraSplitLayoutPickerView::OnLayoutClicked(
    const std::string& layout_id) {
  if (model_) {
    model_->ApplyLayout(layout_id);
  }
}

// -- Static icon helpers ---------------------------------------------------

void AstraSplitLayoutPickerView::DrawLayoutIcon(
    gfx::Canvas* canvas,
    const gfx::Rect& bounds,
    AstraSplitLayoutMode mode,
    SkColor color) {
  SkColor bg = SK_ColorWHITE;
  switch (mode) {
    case AstraSplitLayoutMode::kTwoPaneHorizontal:
      DrawTwoPaneHorizontalPreview(canvas, bounds, 0.5f, color, bg);
      break;
    case AstraSplitLayoutMode::kTwoPaneVertical:
      DrawTwoPaneVerticalPreview(canvas, bounds, 0.5f, color, bg);
      break;
    case AstraSplitLayoutMode::kThreePaneHorizontal:
    case AstraSplitLayoutMode::kThreePaneVertical:
      DrawThreePaneHorizontalPreview(canvas, bounds, color);
      break;
    case AstraSplitLayoutMode::kGridTwoByTwo:
    case AstraSplitLayoutMode::kGridThreeByTwo:
      DrawGridTwoByTwoPreview(canvas, bounds, color);
      break;
    case AstraSplitLayoutMode::kPictureInPicture:
      DrawPiPPreview(canvas, bounds, color);
      break;
    case AstraSplitLayoutMode::kTabShift:
      DrawTabShiftPreview(canvas, bounds, color);
      break;
  }
}

}  // namespace astra
