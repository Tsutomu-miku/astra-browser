// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/zoom/astra_zoom_button.h"

#include <algorithm>
#include <cmath>

#include "base/i18n/number_formatting.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget.h"
#include "ui/views/bubble/bubble_frame_view.h"

namespace astra {

namespace {

// Draw a magnifying glass with plus/minus for zoom.
void DrawZoomIcon(gfx::Canvas* canvas,
                  const gfx::Rect& bounds,
                  SkColor color,
                  bool zoom_in) {
  if (bounds.IsEmpty())
    return;

  int cx = bounds.x() + bounds.width() / 2 - 2;
  int cy = bounds.y() + bounds.height() / 2 - 2;
  int size = std::min(bounds.width(), bounds.height()) * 3 / 5;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(2);
  flags.setAntiAlias(true);

  // Lens circle.
  canvas->DrawCircle(gfx::Point(cx - size / 6, cy - size / 6),
                     size / 2, flags);

  // Handle.
  canvas->DrawLine(gfx::Point(cx + size / 3, cy + size / 3),
                   gfx::Point(cx + size / 2 + 2, cy + size / 2 + 2), flags);

  // Plus or minus inside.
  int icon_cx = cx - size / 6;
  int icon_cy = cy - size / 6;
  int half = size / 5;

  canvas->DrawLine(gfx::Point(icon_cx - half, icon_cy),
                   gfx::Point(icon_cx + half, icon_cy), flags);

  if (zoom_in) {
    canvas->DrawLine(gfx::Point(icon_cx, icon_cy - half),
                     gfx::Point(icon_cx, icon_cy + half), flags);
  }
}

// Draw a zoom-out icon (magnifying glass with minus).
void DrawZoomOutIcon(gfx::Canvas* canvas,
                     const gfx::Rect& bounds,
                     SkColor color) {
  DrawZoomIcon(canvas, bounds, color, /*zoom_in=*/false);
}

// Draw a zoom-in icon (magnifying glass with plus).
void DrawZoomInIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color) {
  DrawZoomIcon(canvas, bounds, color, /*zoom_in=*/true);
}

}  // namespace

// =========================================================================
// AstraZoomButton
// =========================================================================

AstraZoomButton::AstraZoomButton()
    : views::ImageButton(base::BindRepeating(
          &AstraZoomButton::HandleButtonPress, base::Unretained(this))) {
  SetImageSize(gfx::Size(kButtonSize, kButtonSize));
  UpdateButtonDisplay();
}

AstraZoomButton::~AstraZoomButton() {
  if (bubble_) {
    bubble_->GetWidget()->Close();
    bubble_ = nullptr;
  }
}

// -- Zoom state -------------------------------------------------------------

void AstraZoomButton::SetZoomLevel(double zoom_level) {
  zoom_level_ = zoom_level;
  UpdateButtonDisplay();
  if (bubble_)
    bubble_->SetZoomLevel(zoom_level_);
}

void AstraZoomButton::SetDefaultZoom(double default_zoom) {
  default_zoom_ = default_zoom;
  if (bubble_)
    bubble_->SetDefaultZoom(default_zoom_);
}

void AstraZoomButton::ShowBubble() {
  if (bubble_ && bubble_->GetWidget()) {
    bubble_->GetWidget()->Show();
    return;
  }

  // Create bubble anchored to this button.
  auto bubble = std::make_unique<AstraZoomBubbleView>(this);
  bubble->set_delegate(this);
  bubble->SetZoomLevel(zoom_level_);
  bubble->SetDefaultZoom(default_zoom_);

  bubble_ = bubble.get();
  views::BubbleDialogDelegateView::CreateBubble(std::move(bubble))->Show();
}

void AstraZoomButton::HideBubble() {
  if (bubble_ && bubble_->GetWidget()) {
    bubble_->GetWidget()->Close();
  }
}

bool AstraZoomButton::IsBubbleShowing() const {
  return bubble_ && bubble_->GetWidget() &&
         bubble_->GetWidget()->IsVisible();
}

// -- views::ImageButton -----------------------------------------------------

void AstraZoomButton::OnThemeChanged() {
  views::ImageButton::OnThemeChanged();
  UpdateButtonDisplay();
}

bool AstraZoomButton::OnMousePressed(const ui::MouseEvent& event) {
  if (event.IsOnlyLeftMouseButton()) {
    HandleButtonPress();
    return true;
  }
  return views::ImageButton::OnMousePressed(event);
}

void AstraZoomButton::OnGestureEvent(ui::GestureEvent* event) {
  if (event->type() == ui::ET_GESTURE_TAP) {
    HandleButtonPress();
    event->SetHandled();
  }
}

// -- Private methods --------------------------------------------------------

void AstraZoomButton::UpdateButtonDisplay() {
  // Draw zoom icon with current zoom level text.
  // For simplicity, we just use a zoom-in icon.
  // TODO(astra): Consider using a text label instead of icon for zoom level.
  if (!GetWidget())
    return;

  SkColor text_color = GetNativeTheme()->GetSystemColor(
      ui::NativeTheme::kColorId_LabelEnabledColor);

  gfx::Canvas canvas(gfx::Size(kButtonSize, kButtonSize),
                     /*image_scale=*/1.0f, false);
  DrawZoomInIcon(&canvas, gfx::Rect(0, 0, kButtonSize, kButtonSize),
                 text_color);
  SetImage(views::ImageButton::STATE_NORMAL,
           gfx::ImageSkia(canvas.GetBitmap(),
                          gfx::Size(kButtonSize, kButtonSize)));

  // Also set tooltip with zoom percentage.
  int zoom_percent = static_cast<int>(std::round(zoom_level_ * 100.0));
  SetTooltipText(base::NumberToString16(zoom_percent) + u"%");
}

void AstraZoomButton::HandleButtonPress() {
  if (IsBubbleShowing()) {
    HideBubble();
  } else {
    ShowBubble();
  }
  if (delegate_)
    delegate_->OnZoomButtonClicked();
}

// =========================================================================
// AstraZoomBubbleView
// =========================================================================

AstraZoomBubbleView::AstraZoomBubbleView(views::View* anchor_view)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_RIGHT) {
  SetButtons(ui::DIALOG_BUTTON_NONE);
  SetShowCloseButton(false);
  BuildLayout();
}

AstraZoomBubbleView::~AstraZoomBubbleView() = default;

// -- Zoom state -------------------------------------------------------------

void AstraZoomBubbleView::SetZoomLevel(double zoom_level) {
  zoom_level_ = zoom_level;
  UpdateZoomDisplay();
  UpdateButtonStates();
}

void AstraZoomBubbleView::SetDefaultZoom(double default_zoom) {
  default_zoom_ = default_zoom;
  UpdateZoomDisplay();
}

// -- views::BubbleDialogDelegateView ----------------------------------------

gfx::Size AstraZoomBubbleView::CalculatePreferredSize() const {
  return gfx::Size(kBubbleWidth, kBubbleHeight);
}

void AstraZoomBubbleView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();

  if (!GetWidget())
    return;

  SkColor text_color = GetNativeTheme()->GetSystemColor(
      ui::NativeTheme::kColorId_LabelEnabledColor);

  // Redraw icons.
  if (zoom_out_button_) {
    gfx::Canvas canvas(gfx::Size(kButtonSize, kButtonSize),
                       /*image_scale=*/1.0f, false);
    DrawZoomOutIcon(&canvas, gfx::Rect(0, 0, kButtonSize, kButtonSize),
                    text_color);
    zoom_out_button_->SetImage(
        views::ImageButton::STATE_NORMAL,
        gfx::ImageSkia(canvas.GetBitmap(),
                       gfx::Size(kButtonSize, kButtonSize)));
  }

  if (zoom_in_button_) {
    gfx::Canvas canvas(gfx::Size(kButtonSize, kButtonSize),
                       /*image_scale=*/1.0f, false);
    DrawZoomInIcon(&canvas, gfx::Rect(0, 0, kButtonSize, kButtonSize),
                   text_color);
    zoom_in_button_->SetImage(
        views::ImageButton::STATE_NORMAL,
        gfx::ImageSkia(canvas.GetBitmap(),
                       gfx::Size(kButtonSize, kButtonSize)));
  }
}

void AstraZoomBubbleView::WindowClosing() {
  if (delegate_)
    delegate_->OnBubbleClosed();
}

// -- Private methods --------------------------------------------------------

void AstraZoomBubbleView::BuildLayout() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(12), 8));

  // Zoom level label (large).
  auto zoom_label = std::make_unique<views::Label>(u"100%");
  zoom_label->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  zoom_label->SetAutoColorReadabilityEnabled(false);
  zoom_label->SetFontList(zoom_label->font_list().Derive(8, gfx::Font::NORMAL,
                                                        gfx::Font::Weight::BOLD));
  zoom_label_ = AddChildView(std::move(zoom_label));

  // Control buttons row.
  auto controls_row = std::make_unique<views::View>();
  controls_row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets(), 8,
      views::BoxLayout::MainAxisAlignment::kCenter));

  // Zoom out button.
  auto zoom_out_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraZoomBubbleView::OnZoomOutPressed,
                          base::Unretained(this)));
  zoom_out_button->SetImageSize(gfx::Size(kButtonSize, kButtonSize));
  zoom_out_button_ = controls_row->AddChildView(std::move(zoom_out_button));

  // Reset button.
  auto reset_button = std::make_unique<views::LabelButton>(
      base::BindRepeating(&AstraZoomBubbleView::OnZoomResetPressed,
                          base::Unretained(this)),
      u"Reset");
  reset_button->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  reset_button_ = controls_row->AddChildView(std::move(reset_button));

  // Zoom in button.
  auto zoom_in_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraZoomBubbleView::OnZoomInPressed,
                          base::Unretained(this)));
  zoom_in_button->SetImageSize(gfx::Size(kButtonSize, kButtonSize));
  zoom_in_button_ = controls_row->AddChildView(std::move(zoom_in_button));

  AddChildView(std::move(controls_row));

  // Default zoom label.
  auto default_label = std::make_unique<views::Label>(u"Default: 100%");
  default_label->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  default_label->SetAutoColorReadabilityEnabled(false);
  default_label_ = AddChildView(std::move(default_label));

  UpdateZoomDisplay();
  UpdateButtonStates();
}

void AstraZoomBubbleView::UpdateZoomDisplay() {
  if (!zoom_label_ || !default_label_)
    return;

  int zoom_percent = static_cast<int>(std::round(zoom_level_ * 100.0));
  zoom_label_->SetText(base::NumberToString16(zoom_percent) + u"%");

  int default_percent = static_cast<int>(std::round(default_zoom_ * 100.0));
  default_label_->SetText(u"Default: " +
                          base::NumberToString16(default_percent) + u"%");

  // Highlight if not at default.
  if (zoom_label_) {
    bool at_default = std::abs(zoom_level_ - default_zoom_) < 0.01;
    // TODO(astra): Change color when not at default.
  }
}

void AstraZoomBubbleView::UpdateButtonStates() {
  // TODO(astra): Disable buttons at min/max zoom levels.
  if (zoom_out_button_)
    zoom_out_button_->SetEnabled(true);
  if (zoom_in_button_)
    zoom_in_button_->SetEnabled(true);
  if (reset_button_)
    reset_button_->SetEnabled(std::abs(zoom_level_ - default_zoom_) > 0.01);
}

// -- Button handlers --------------------------------------------------------

void AstraZoomBubbleView::OnZoomOutPressed() {
  if (delegate_)
    delegate_->OnZoomOut();
}

void AstraZoomBubbleView::OnZoomResetPressed() {
  if (delegate_)
    delegate_->OnZoomReset();
}

void AstraZoomBubbleView::OnZoomInPressed() {
  if (delegate_)
    delegate_->OnZoomIn();
}

}  // namespace astra
