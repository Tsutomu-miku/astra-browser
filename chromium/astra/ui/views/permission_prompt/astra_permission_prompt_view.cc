// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/permission_prompt/astra_permission_prompt_view.h"

#include <memory>
#include <string>

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkColor.h"
#include "skia/core/SkPath.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/models/combobox_model.h"
#include "ui/base/models/simple_combobox_model.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/combobox/combobox.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/flex_layout_view.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/style/typography.h"
#include "ui/views/view.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/widget/widget.h"

namespace astra {

namespace {

// Constants.
constexpr int kIconSize = 20;
constexpr int kCloseButtonSize = 24;
constexpr int kButtonRowSpacing = 8;
constexpr int kContentPadding = 16;
constexpr int kPromptMinWidth = 360;
constexpr int kPromptMaxWidth = 440;

// Draw a camera icon.
void DrawCameraIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Camera body.
  SkPath path;
  path.addRect(SkRect::MakeXYWH(cx - s, cy - s * 0.7f, s * 2, s * 1.4f));
  canvas->DrawPath(path, flags);

  // Lens.
  canvas->DrawCircle(gfx::Point(cx, cy), s * 0.45f, flags);

  // Flash / viewfinder top.
  SkPath top;
  top.moveTo(cx - s * 0.6f, cy - s * 0.7f);
  top.lineTo(cx - s * 0.4f, cy - s * 1.1f);
  top.lineTo(cx + s * 0.4f, cy - s * 1.1f);
  top.lineTo(cx + s * 0.6f, cy - s * 0.7f);
  canvas->DrawPath(top, flags);
}

// Draw a microphone icon.
void DrawMicIcon(gfx::Canvas* canvas,
                 const gfx::Rect& bounds,
                 SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Mic capsule.
  SkRect rect = SkRect::MakeXYWH(cx - s * 0.5f, cy - s, s, s * 1.4f);
  SkPath path;
  path.addOval(rect);
  canvas->DrawPath(path, flags);

  // Stand / base.
  SkPath stand;
  stand.moveTo(cx - s * 0.8f, cy + s);
  stand.arcTo(SkRect::MakeXYWH(cx - s * 0.8f, cy + s * 0.3f, s * 1.6f, s * 1.4f),
              180, 180, false);
  canvas->DrawPath(stand, flags);

  // Bottom bar.
  SkPath bar;
  bar.moveTo(cx, cy + s);
  bar.lineTo(cx, cy + s * 1.5f);
  canvas->DrawPath(bar, flags);
}

// Draw a location / map pin icon.
void DrawLocationIcon(gfx::Canvas* canvas,
                      const gfx::Rect& bounds,
                      SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Pin shape (teardrop).
  SkPath path;
  path.moveTo(cx, cy - s);
  path.arcTo(SkRect::MakeXYWH(cx - s, cy - s, s * 2, s * 2), 270, -180, false);
  path.lineTo(cx, cy + s);
  path.close();
  canvas->DrawPath(path, flags);

  // Inner dot.
  canvas->DrawCircle(gfx::Point(cx, cy - s * 0.1f), s * 0.25f, flags);
}

// Draw a notifications / bell icon.
void DrawBellIcon(gfx::Canvas* canvas,
                  const gfx::Rect& bounds,
                  SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Bell body.
  SkPath path;
  path.moveTo(cx - s * 0.8f, cy + s * 0.3f);
  path.arcTo(SkRect::MakeXYWH(cx - s, cy - s * 0.3f, s * 2, s * 1.2f),
             180, 180, false);
  path.lineTo(cx + s * 0.8f, cy + s * 0.3f);
  path.quadTo(cx + s * 0.6f, cy + s * 0.6f, cx + s * 0.5f, cy + s * 0.7f);
  path.lineTo(cx - s * 0.5f, cy + s * 0.7f);
  path.quadTo(cx - s * 0.6f, cy + s * 0.6f, cx - s * 0.8f, cy + s * 0.3f);
  canvas->DrawPath(path, flags);

  // Bell bottom / clapper.
  canvas->DrawCircle(gfx::Point(cx, cy + s * 0.95f), s * 0.15f, flags);

  // Top mount.
  canvas->DrawRect(gfx::Rect(cx - s * 0.3f, cy - s * 0.5f, s * 0.6f, s * 0.2f),
                   flags);
}

// Draw a shield / security icon.
void DrawShieldIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  SkPath path;
  path.moveTo(cx, cy - s);
  path.lineTo(cx + s * 0.8f, cy - s * 0.5f);
  path.lineTo(cx + s * 0.8f, cy + s * 0.3f);
  path.lineTo(cx, cy + s);
  path.lineTo(cx - s * 0.8f, cy + s * 0.3f);
  path.lineTo(cx - s * 0.8f, cy - s * 0.5f);
  path.close();
  canvas->DrawPath(path, flags);

  // Checkmark inside.
  SkPath check;
  check.moveTo(cx - s * 0.35f, cy);
  check.lineTo(cx - s * 0.1f, cy + s * 0.25f);
  check.lineTo(cx + s * 0.4f, cy - s * 0.25f);
  canvas->DrawPath(check, flags);
}

// Draw a close (X) icon.
void DrawCloseIcon(gfx::Canvas* canvas,
                   const gfx::Rect& bounds,
                   SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.3f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  SkPath path;
  path.moveTo(cx - s, cy - s);
  path.lineTo(cx + s, cy + s);
  path.moveTo(cx + s, cy - s);
  path.lineTo(cx - s, cy + s);
  canvas->DrawPath(path, flags);
}

// Draw chevron/arrow icon.
void DrawChevronIcon(gfx::Canvas* canvas,
                     const gfx::Rect& bounds,
                     SkColor color,
                     bool pointing_right) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.25f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(2);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  SkPath path;
  if (pointing_right) {
    path.moveTo(cx - s * 0.5f, cy - s);
    path.lineTo(cx + s * 0.5f, cy);
    path.lineTo(cx - s * 0.5f, cy + s);
  } else {
    path.moveTo(cx + s * 0.5f, cy - s);
    path.lineTo(cx - s * 0.5f, cy);
    path.lineTo(cx + s * 0.5f, cy + s);
  }
  canvas->DrawPath(path, flags);
}

// Draw screen / monitor icon.
void DrawScreenIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Monitor rectangle.
  canvas->DrawRect(
      gfx::Rect(cx - s, cy - s * 0.7f, s * 2, s * 1.2f), flags);

  // Stand.
  SkPath stand;
  stand.moveTo(cx - s * 0.4f, cy + s * 0.5f);
  stand.lineTo(cx - s * 0.6f, cy + s);
  stand.lineTo(cx + s * 0.6f, cy + s);
  stand.lineTo(cx + s * 0.4f, cy + s * 0.5f);
  canvas->DrawPath(stand, flags);
}

// Draw folder / file icon.
void DrawFolderIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Folder shape.
  SkPath path;
  path.moveTo(cx - s, cy - s * 0.7f);
  path.lineTo(cx - s * 0.3f, cy - s * 0.7f);
  path.lineTo(cx - s * 0.1f, cy - s * 0.3f);
  path.lineTo(cx + s, cy - s * 0.3f);
  path.lineTo(cx + s, cy + s * 0.7f);
  path.lineTo(cx - s, cy + s * 0.7f);
  path.close();
  canvas->DrawPath(path, flags);
}

// Draw bluetooth icon.
void DrawBluetoothIcon(gfx::Canvas* canvas,
                       const gfx::Rect& bounds,
                       SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Bluetooth symbol.
  SkPath path;
  path.moveTo(cx - s * 0.3f, cy - s);
  path.lineTo(cx + s * 0.3f, cy);
  path.lineTo(cx - s * 0.3f, cy + s);
  canvas->DrawPath(path, flags);

  // The "B" shape — top and bottom triangles.
  SkPath top_tri;
  top_tri.moveTo(cx + s * 0.3f, cy - s * 0.6f);
  top_tri.lineTo(cx + s * 0.7f, cy - s * 0.3f);
  top_tri.lineTo(cx + s * 0.3f, cy);
  top_tri.close();
  canvas->DrawPath(top_tri, flags);

  SkPath bot_tri;
  bot_tri.moveTo(cx + s * 0.3f, cy + s * 0.6f);
  bot_tri.lineTo(cx + s * 0.7f, cy + s * 0.3f);
  bot_tri.lineTo(cx + s * 0.3f, cy);
  bot_tri.close();
  canvas->DrawPath(bot_tri, flags);
}

// Draw USB icon.
void DrawUsbIcon(gfx::Canvas* canvas,
                 const gfx::Rect& bounds,
                 SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Trident shape.
  SkPath path;
  path.moveTo(cx, cy + s * 0.7f);
  path.lineTo(cx, cy - s * 0.3f);
  canvas->DrawPath(path, flags);

  // Left prong.
  canvas->DrawCircle(gfx::Point(cx - s * 0.6f, cy - s * 0.6f), s * 0.25f, flags);
  SkPath left_arm;
  left_arm.moveTo(cx - s * 0.6f, cy - s * 0.35f);
  left_arm.lineTo(cx - s * 0.6f, cy - s * 0.1f);
  canvas->DrawPath(left_arm, flags);

  // Middle prong.
  SkPath top;
  top.moveTo(cx - s * 0.2f, cy - s * 0.6f);
  top.lineTo(cx + s * 0.2f, cy - s * 0.6f);
  top.lineTo(cx + s * 0.2f, cy - s * 0.3f);
  top.lineTo(cx - s * 0.2f, cy - s * 0.3f);
  top.close();
  canvas->DrawPath(top, flags);

  // Right prong.
  canvas->DrawCircle(gfx::Point(cx + s * 0.6f, cy - s * 0.6f), s * 0.25f, flags);
  SkPath right_arm;
  right_arm.moveTo(cx + s * 0.6f, cy - s * 0.35f);
  right_arm.lineTo(cx + s * 0.6f, cy - s * 0.1f);
  canvas->DrawPath(right_arm, flags);
}

}  // namespace

// ===========================================================================
// AstraPermissionPromptView
// ===========================================================================

AstraPermissionPromptView::AstraPermissionPromptView(
    views::View* anchor_view,
    AstraPermissionPromptModel* model)
    : BubbleDialogDelegateView(anchor_view, views::BubbleBorder::TOP_LEFT) {
  SetModel(model);
  BuildUI();
}

AstraPermissionPromptView::~AstraPermissionPromptView() {
  if (scoped_observation_.IsObserving()) {
    scoped_observation_.RemoveObserver();
  }
}

void AstraPermissionPromptView::SetModel(AstraPermissionPromptModel* model) {
  if (model_ == model) {
    return;
  }

  if (scoped_observation_.IsObserving()) {
    scoped_observation_.RemoveObserver();
  }

  model_ = model;

  if (model_) {
    scoped_observation_.Observe(model_);
    RefreshFromModel();
  }
}

void AstraPermissionPromptView::RefreshFromModel() {
  if (!model_) {
    return;
  }

  UpdateActiveRequest();

  // Show/hide multi-request navigation.
  bool multiple = model_->GetRequestCount() > 1;
  if (nav_row_) {
    nav_row_->SetVisible(multiple);
    if (multiple) {
      std::u16string counter =
          base::NumberToString16(model_->GetAllRequests().size() > 0
                                     ? (FindActiveIndex() + 1)
                                     : 0) +
          u" of " +
          base::NumberToString16(model_->GetRequestCount());
      counter_label_->SetText(counter);
    }
  }
}

int AstraPermissionPromptView::FindActiveIndex() const {
  if (!model_) return 0;
  auto& requests = model_->GetAllRequests();
  std::string active_id = model_->GetActiveRequestId();
  for (size_t i = 0; i < requests.size(); ++i) {
    if (requests[i].id == active_id) return static_cast<int>(i);
  }
  return 0;
}

std::u16string AstraPermissionPromptView::GetWindowTitle() const {
  if (!model_) {
    return std::u16string();
  }
  const auto* req = model_->GetActiveRequest();
  return req ? req->title : std::u16string();
}

void AstraPermissionPromptView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();

  auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  SkColor text_color = color_provider->GetColor(ui::kColorLabelForeground);
  SkColor secondary =
      color_provider->GetColor(ui::kColorLabelForegroundSecondary);
  SkColor bg_color = color_provider->GetColor(ui::kColorDialogBackground);
  SkColor icon_color = color_provider->GetColor(ui::kColorIcon);

  SetBackground(views::CreateSolidBackground(bg_color));

  if (title_label_) {
    title_label_->SetEnabledColor(text_color);
  }
  if (message_label_) {
    message_label_->SetEnabledColor(text_color);
  }
  if (origin_label_) {
    origin_label_->SetEnabledColor(secondary);
  }

  SchedulePaint();
}

void AstraPermissionPromptView::OnPermissionRequested(
    AstraPermissionPromptModel* model,
    const std::string& request_id) {
  DCHECK_EQ(model, model_);
  RefreshFromModel();
  if (GetWidget()) {
    SizeToContents();
  }
}

void AstraPermissionPromptView::OnPermissionDecided(
    AstraPermissionPromptModel* model,
    const std::string& request_id,
    AstraPermissionAction action) {
  DCHECK_EQ(model, model_);
  // Request will be removed, handled by OnPermissionDismissed.
}

void AstraPermissionPromptView::OnPermissionDismissed(
    AstraPermissionPromptModel* model,
    const std::string& request_id) {
  DCHECK_EQ(model, model_);
  if (!model_->IsPromptVisible()) {
    // No more requests — close the bubble.
    if (GetWidget()) {
      GetWidget()->CloseWithReason(
          views::Widget::ClosedReason::kUnspecified);
    }
    return;
  }
  RefreshFromModel();
}

void AstraPermissionPromptView::OnActiveRequestChanged(
    AstraPermissionPromptModel* model,
    const std::string& request_id) {
  DCHECK_EQ(model, model_);
  UpdateActiveRequest();
}

void AstraPermissionPromptView::OnPermissionPromptModelShutdown(
    AstraPermissionPromptModel* model) {
  DCHECK_EQ(model, model_);
  scoped_observation_.RemoveObserver();
  model_ = nullptr;
}

void AstraPermissionPromptView::BuildUI() {
  set_margins(gfx::Insets(0));
  SetArrow(views::BubbleBorder::Arrow::TOP_LEFT);
  set_close_on_deactivate(true);
  set_close_on_esc(true);

  auto* layout = SetLayoutManager(std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kVertical);

  BuildHeader();
  BuildContent();
  BuildFooter();

  set_min_size(gfx::Size(kPromptMinWidth, 0));
  set_max_size(gfx::Size(kPromptMaxWidth, 0));
}

void AstraPermissionPromptView::BuildHeader() {
  auto header = std::make_unique<views::View>();
  auto* header_layout =
      header->SetLayoutManager(std::make_unique<views::FlexLayout>());
  header_layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  header_layout->SetCrossAxisAlignment(views::LayoutAlignment::kStart);
  header_layout->SetInteriorMargin(gfx::Insets::VH(kContentPadding, kContentPadding));

  // Icon.
  auto icon = std::make_unique<views::ImageView>();
  icon->SetImageSize(gfx::Size(kIconSize, kIconSize));
  icon_view_ = header->AddChildView(std::move(icon));
  icon_view_->SetProperty(views::kMarginsKey,
                          gfx::Insets::VH(0, 0, 0, 12));

  // Title + origin column.
  auto* title_col = header->AddChildView(std::make_unique<views::View>());
  auto* title_layout =
      title_col->SetLayoutManager(std::make_unique<views::FlexLayout>());
  title_layout->SetOrientation(views::LayoutOrientation::kVertical);
  header_layout->SetFlexForView(title_col, 1);

  auto title = std::make_unique<views::Label>();
  title->SetFontList(views::style::GetFont(
      views::style::CONTEXT_DIALOG_TITLE, views::style::STYLE_PRIMARY));
  title->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title->SetMultiLine(true);
  title->SetAutoColorReadabilityEnabled(false);
  title_label_ = title_col->AddChildView(std::move(title));

  auto origin = std::make_unique<views::Label>();
  origin->SetFontList(views::style::GetFont(
      views::style::CONTEXT_LABEL, views::style::STYLE_SECONDARY));
  origin->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  origin->SetAutoColorReadabilityEnabled(false);
  origin_label_ = title_col->AddChildView(std::move(origin));

  // Close button.
  auto close_btn = std::make_unique<views::ImageButton>();
  close_btn->SetMinSize(gfx::Size(kCloseButtonSize, kCloseButtonSize));
  close_btn->SetTooltipText(u"Close");
  close_btn->SetAccessibleName(u"Close permission prompt");
  close_btn->SetCallback(base::BindRepeating(
      &AstraPermissionPromptView::OnCloseClicked, base::Unretained(this)));
  close_button_ = header->AddChildView(std::move(close_btn));
  close_button_->SetProperty(views::kMarginsKey,
                             gfx::Insets::VH(-4, -4, 0, 0));

  AddChildView(std::move(header));

  // Navigation row (for multiple requests).
  auto nav = std::make_unique<views::View>();
  auto* nav_layout =
      nav->SetLayoutManager(std::make_unique<views::FlexLayout>());
  nav_layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  nav_layout->SetMainAxisAlignment(views::LayoutAlignment::kCenter);
  nav_layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);
  nav_layout->SetInteriorMargin(gfx::Insets::VH(0, kContentPadding, 8,
                                                kContentPadding));

  auto prev = std::make_unique<views::ImageButton>();
  prev->SetMinSize(gfx::Size(20, 20));
  prev->SetTooltipText(u"Previous permission");
  prev->SetCallback(base::BindRepeating(
      &AstraPermissionPromptView::OnPrevClicked, base::Unretained(this)));
  prev_button_ = nav->AddChildView(std::move(prev));

  auto counter = std::make_unique<views::Label>();
  counter->SetFontList(views::style::GetFont(
      views::style::CONTEXT_LABEL, views::style::STYLE_SECONDARY));
  counter->SetAutoColorReadabilityEnabled(false);
  counter_label_ = nav->AddChildView(std::move(counter));
  counter_label_->SetProperty(views::kMarginsKey,
                              gfx::Insets::VH(0, 8));

  auto next = std::make_unique<views::ImageButton>();
  next->SetMinSize(gfx::Size(20, 20));
  next->SetTooltipText(u"Next permission");
  next->SetCallback(base::BindRepeating(
      &AstraPermissionPromptView::OnNextClicked, base::Unretained(this)));
  next_button_ = nav->AddChildView(std::move(next));

  nav_row_ = AddChildView(std::move(nav));
  nav_row_->SetVisible(false);
}

void AstraPermissionPromptView::BuildContent() {
  auto content = std::make_unique<views::View>();
  auto* content_layout =
      content->SetLayoutManager(std::make_unique<views::FlexLayout>());
  content_layout->SetOrientation(views::LayoutOrientation::kVertical);
  content_layout->SetInteriorMargin(gfx::Insets::VH(8, kContentPadding));

  // Message / description.
  auto message = std::make_unique<views::Label>();
  message->SetMultiLine(true);
  message->SetAllowCharacterBreak(true);
  message->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  message->SetAutoColorReadabilityEnabled(false);
  message->SetFontList(views::style::GetFont(
      views::style::CONTEXT_LABEL, views::style::STYLE_PRIMARY));
  message_label_ = content->AddChildView(std::move(message));
  message_label_->SetProperty(views::kMarginsKey,
                              gfx::Insets::VH(0, 0, 12, 0));

  // Device selector (for camera/mic/etc.).
  auto device_combobox = std::make_unique<views::Combobox>(
      std::make_unique<ui::SimpleComboboxModel>(
          std::vector<ui::SimpleComboboxModel::Item>{}));
  device_combobox->SetCallback(base::BindRepeating(
      &AstraPermissionPromptView::OnDeviceChanged, base::Unretained(this)));
  device_combobox_ = content->AddChildView(std::move(device_combobox));
  device_combobox_->SetVisible(false);

  // Remember decision checkbox.
  auto remember =
      std::make_unique<views::Checkbox>(u"Remember this decision");
  remember->SetChecked(true);
  remember->SetCallback(base::BindRepeating(
      &AstraPermissionPromptView::OnRememberToggled, base::Unretained(this)));
  remember_checkbox_ = content->AddChildView(std::move(remember));
  remember_checkbox_->SetProperty(views::kMarginsKey,
                                   gfx::Insets::VH(12, 0, 0, 0));

  content_view_ = AddChildView(std::move(content));
}

void AstraPermissionPromptView::BuildFooter() {
  auto footer = std::make_unique<views::View>();
  auto* footer_layout =
      footer->SetLayoutManager(std::make_unique<views::FlexLayout>());
  footer_layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  footer_layout->SetMainAxisAlignment(views::LayoutAlignment::kEnd);
  footer_layout->SetInteriorMargin(gfx::Insets::VH(12, kContentPadding,
                                                   kContentPadding,
                                                   kContentPadding));

  // Block button.
  auto block = views::MdTextButton::CreateSecondaryUiButton(
      base::BindRepeating(
          &AstraPermissionPromptView::OnBlockClicked, base::Unretained(this)),
      u"Block");
  block_button_ = footer->AddChildView(std::move(block));
  block_button_->SetProperty(views::kMarginsKey,
                             gfx::Insets::VH(0, kButtonRowSpacing, 0, 0));

  // Allow once button.
  auto allow_once = views::MdTextButton::CreateSecondaryUiButton(
      base::BindRepeating(&AstraPermissionPromptView::OnAllowOnceClicked,
                          base::Unretained(this)),
      u"Allow once");
  allow_once_button_ = footer->AddChildView(std::move(allow_once));
  allow_once_button_->SetProperty(views::kMarginsKey,
                                  gfx::Insets::VH(0, kButtonRowSpacing, 0, 0));

  // Allow button (primary).
  auto allow = views::MdTextButton::CreatePrimaryUiButton(
      base::BindRepeating(
          &AstraPermissionPromptView::OnAllowClicked, base::Unretained(this)),
      u"Allow");
  allow_button_ = footer->AddChildView(std::move(allow));

  button_row_ = AddChildView(std::move(footer));
}

void AstraPermissionPromptView::UpdateActiveRequest() {
  if (!model_) {
    return;
  }

  const auto* req = model_->GetActiveRequest();
  if (!req) {
    return;
  }

  // Update title.
  title_label_->SetText(req->title);

  // Update origin.
  origin_label_->SetText(req->origin);

  // Update message.
  message_label_->SetText(req->message);

  // Update icon.
  SkBitmap bitmap;
  bitmap.allocN32Pixels(kIconSize, kIconSize);
  bitmap.eraseColor(SK_ColorTRANSPARENT);
  gfx::Canvas canvas(gfx::Size(kIconSize, kIconSize), 1.0f, false);
  DrawPermissionIcon(&canvas, gfx::Rect(0, 0, kIconSize, kIconSize), req->type);
  icon_view_->SetImage(
      gfx::ImageSkia::CreateFrom1xBitmap(canvas.GetBitmap()));

  // Device selector visibility.
  bool has_devices = !req->device_options.empty();
  device_combobox_->SetVisible(has_devices);
  if (has_devices) {
    std::vector<ui::SimpleComboboxModel::Item> items;
    for (const auto& device : req->device_options) {
      items.push_back(ui::SimpleComboboxModel::Item(device));
    }
    device_combobox_->SetModel(
        std::make_unique<ui::SimpleComboboxModel>(std::move(items)));
    device_combobox_->SetSelectedIndex(
        std::clamp(req->selected_device_index, 0,
                   static_cast<int>(req->device_options.size()) - 1));
  }

  // Remember checkbox visibility.
  remember_checkbox_->SetVisible(req->has_remember_option);
  remember_checkbox_->SetChecked(model_->GetRememberDecision());

  // Allow once button visibility.
  bool supports_one_time =
      AstraPermissionPromptModel::SupportsOneTimePermission(req->type);
  allow_once_button_->SetVisible(supports_one_time);

  // Update counter.
  if (model_->GetRequestCount() > 1) {
    nav_row_->SetVisible(true);
    int idx = 0;
    auto& requests = model_->GetAllRequests();
    for (size_t i = 0; i < requests.size(); ++i) {
      if (requests[i].id == req->id) {
        idx = static_cast<int>(i);
        break;
      }
    }
    counter_label_->SetText(
        base::NumberToString16(idx + 1) + u" of " +
        base::NumberToString16(model_->GetRequestCount()));
  } else {
    nav_row_->SetVisible(false);
  }

  // Update close button icon.
  SkColor icon_color = GetColorProvider()
                           ? GetColorProvider()->GetColor(ui::kColorIcon)
                           : SK_ColorBLACK;
  SkBitmap close_bitmap;
  close_bitmap.allocN32Pixels(kCloseButtonSize, kCloseButtonSize);
  close_bitmap.eraseColor(SK_ColorTRANSPARENT);
  gfx::Canvas close_canvas(
      gfx::Size(kCloseButtonSize, kCloseButtonSize), 1.0f, false);
  DrawCloseIcon(&close_canvas,
                gfx::Rect(0, 0, kCloseButtonSize, kCloseButtonSize),
                icon_color);

  // Update prev/next icons.
  SkBitmap prev_bitmap, next_bitmap;
  prev_bitmap.allocN32Pixels(20, 20);
  prev_bitmap.eraseColor(SK_ColorTRANSPARENT);
  gfx::Canvas prev_canvas(gfx::Size(20, 20), 1.0f, false);
  DrawChevronIcon(&prev_canvas, gfx::Rect(0, 0, 20, 20), icon_color, false);

  next_bitmap.allocN32Pixels(20, 20);
  next_bitmap.eraseColor(SK_ColorTRANSPARENT);
  gfx::Canvas next_canvas(gfx::Size(20, 20), 1.0f, false);
  DrawChevronIcon(&next_canvas, gfx::Rect(0, 0, 20, 20), icon_color, true);

  SchedulePaint();

  if (GetWidget()) {
    SizeToContents();
  }
}

void AstraPermissionPromptView::OnBlockClicked() {
  if (model_) {
    model_->Block();
  }
}

void AstraPermissionPromptView::OnAllowClicked() {
  if (model_) {
    model_->Allow();
  }
}

void AstraPermissionPromptView::OnAllowOnceClicked() {
  if (model_) {
    model_->AllowOnce();
  }
}

void AstraPermissionPromptView::OnCloseClicked() {
  if (model_) {
    model_->Dismiss();
  }
  if (GetWidget()) {
    GetWidget()->CloseWithReason(
        views::Widget::ClosedReason::kCloseButtonClicked);
  }
}

void AstraPermissionPromptView::OnRememberToggled() {
  if (model_ && remember_checkbox_) {
    model_->SetRememberDecision(remember_checkbox_->GetChecked());
  }
}

void AstraPermissionPromptView::OnDeviceChanged() {
  // TODO(astra): Update selected device in model.
}

void AstraPermissionPromptView::OnPrevClicked() {
  if (model_) {
    model_->PreviousRequest();
  }
}

void AstraPermissionPromptView::OnNextClicked() {
  if (model_) {
    model_->NextRequest();
  }
}

void AstraPermissionPromptView::DrawPermissionIcon(
    gfx::Canvas* canvas,
    const gfx::Rect& bounds,
    AstraPermissionType type) {
  SkColor color = SK_ColorBLACK;  // Will be themed

  switch (type) {
    case AstraPermissionType::kCamera:
    case AstraPermissionType::kCameraAndMicrophone:
      DrawCameraIcon(canvas, bounds, color);
      break;
    case AstraPermissionType::kMicrophone:
      DrawMicIcon(canvas, bounds, color);
      break;
    case AstraPermissionType::kGeolocation:
      DrawLocationIcon(canvas, bounds, color);
      break;
    case AstraPermissionType::kNotifications:
      DrawBellIcon(canvas, bounds, color);
      break;
    case AstraPermissionType::kScreenCapture:
    case AstraPermissionType::kPictureInPicture:
    case AstraPermissionType::kFullscreen:
      DrawScreenIcon(canvas, bounds, color);
      break;
    case AstraPermissionType::kFileSystemRead:
    case AstraPermissionType::kFileSystemWrite:
    case AstraPermissionType::kDownloads:
    case AstraPermissionType::kMultipleDownloads:
      DrawFolderIcon(canvas, bounds, color);
      break;
    case AstraPermissionType::kBluetooth:
      DrawBluetoothIcon(canvas, bounds, color);
      break;
    case AstraPermissionType::kUsb:
      DrawUsbIcon(canvas, bounds, color);
      break;
    case AstraPermissionType::kStorageAccess:
    case AstraPermissionType::kCookies:
    case AstraPermissionType::kIdleDetection:
    case AstraPermissionType::kClipboardRead:
    case AstraPermissionType::kClipboardWrite:
    case AstraPermissionType::kSound:
    case AstraPermissionType::kJavaScript:
    case AstraPermissionType::kPopups:
    case AstraPermissionType::kAds:
    case AstraPermissionType::kPaymentHandler:
    case AstraPermissionType::kAutoPlay:
    default:
      DrawShieldIcon(canvas, bounds, color);
      break;
  }
}

}  // namespace astra
