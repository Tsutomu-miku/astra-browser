// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/clear_browsing_data/astra_clear_browsing_data_dialog.h"

#include <memory>
#include <string>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkColor.h"
#include "skia/core/SkPath.h"
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
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/combobox/combobox.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
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
constexpr int kHeaderIconSize = 24;
constexpr int kDataTypeIconSize = 18;
constexpr int kDialogMinWidth = 400;
constexpr int kDialogMaxWidth = 480;
constexpr int kContentPadding = 16;
constexpr int kCheckboxRowSpacing = 8;
constexpr int kSectionSpacing = 16;
constexpr int kButtonRowSpacing = 8;

}  // namespace

// ===========================================================================
// Icon drawing helpers
// ===========================================================================

void AstraClearBrowsingDataDialog::DrawTrashIcon(gfx::Canvas* canvas,
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

  // Trash can body.
  SkPath body;
  body.moveTo(cx - s * 0.7f, cy - s * 0.3f);
  body.lineTo(cx - s * 0.5f, cy + s);
  body.lineTo(cx + s * 0.5f, cy + s);
  body.lineTo(cx + s * 0.7f, cy - s * 0.3f);
  body.close();
  canvas->DrawPath(body, flags);

  // Lid.
  SkPath lid;
  lid.moveTo(cx - s * 0.85f, cy - s * 0.3f);
  lid.lineTo(cx + s * 0.85f, cy - s * 0.3f);
  canvas->DrawPath(lid, flags);

  // Lid top / handle.
  SkPath handle;
  handle.moveTo(cx - s * 0.4f, cy - s * 0.7f);
  handle.lineTo(cx + s * 0.4f, cy - s * 0.7f);
  handle.lineTo(cx + s * 0.3f, cy - s * 0.3f);
  handle.lineTo(cx - s * 0.3f, cy - s * 0.3f);
  handle.close();
  canvas->DrawPath(handle, flags);
}

void AstraClearBrowsingDataDialog::DrawClockIcon(gfx::Canvas* canvas,
                                                 const gfx::Rect& bounds,
                                                 SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.4f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Clock face.
  canvas->DrawCircle(gfx::Point(cx, cy), s, flags);

  // Hour hand.
  SkPath hour_hand;
  hour_hand.moveTo(cx, cy);
  hour_hand.lineTo(cx, cy - s * 0.5f);
  canvas->DrawPath(hour_hand, flags);

  // Minute hand.
  SkPath min_hand;
  min_hand.moveTo(cx, cy);
  min_hand.lineTo(cx + s * 0.4f, cy - s * 0.15f);
  canvas->DrawPath(min_hand, flags);
}

void AstraClearBrowsingDataDialog::DrawHistoryIcon(gfx::Canvas* canvas,
                                                   const gfx::Rect& bounds,
                                                   SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.38f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Partial circle (open at top-left).
  SkPath arc;
  arc.arcTo(SkRect::MakeXYWH(cx - s, cy - s, s * 2, s * 2), 135, 200, false);
  canvas->DrawPath(arc, flags);

  // Arrow tip at top-left.
  SkPath arrow;
  arrow.moveTo(cx - s * 0.9f, cy - s * 0.5f);
  arrow.lineTo(cx - s * 0.5f, cy - s * 0.7f);
  arrow.lineTo(cx - s * 0.7f, cy - s * 0.3f);
  canvas->DrawPath(arrow, flags);

  // Center dot.
  canvas->DrawCircle(gfx::Point(cx, cy), s * 0.12f, flags);

  // Hand pointing to upper right.
  SkPath hand;
  hand.moveTo(cx, cy);
  hand.lineTo(cx + s * 0.4f, cy - s * 0.3f);
  canvas->DrawPath(hand, flags);
}

void AstraClearBrowsingDataDialog::DrawDownloadIcon(gfx::Canvas* canvas,
                                                    const gfx::Rect& bounds,
                                                    SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.38f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Downward arrow.
  SkPath arrow;
  arrow.moveTo(cx, cy - s * 0.8f);
  arrow.lineTo(cx, cy + s * 0.4f);
  canvas->DrawPath(arrow, flags);

  // Arrowhead.
  SkPath arrowhead;
  arrowhead.moveTo(cx - s * 0.45f, cy + s * 0.1f);
  arrowhead.lineTo(cx, cy + s * 0.75f);
  arrowhead.lineTo(cx + s * 0.45f, cy + s * 0.1f);
  canvas->DrawPath(arrowhead, flags);

  // Bottom tray.
  SkPath tray;
  tray.moveTo(cx - s * 0.85f, cy + s * 0.85f);
  tray.lineTo(cx + s * 0.85f, cy + s * 0.85f);
  canvas->DrawPath(tray, flags);
}

void AstraClearBrowsingDataDialog::DrawCookieIcon(gfx::Canvas* canvas,
                                                  const gfx::Rect& bounds,
                                                  SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.4f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Cookie body (circle with a bite taken out).
  SkPath cookie;
  cookie.addCircle(cx, cy, s);
  // Bite mark at top-right.
  SkPath bite;
  bite.addCircle(cx + s * 0.5f, cy - s * 0.6f, s * 0.45f);
  cookie.op(cookie, bite, SkPathOp::kDifference_SkPathOp);
  canvas->DrawPath(cookie, flags);

  // Chocolate chips.
  canvas->DrawCircle(gfx::Point(cx - s * 0.35f, cy - s * 0.15f), s * 0.1f,
                     flags);
  canvas->DrawCircle(gfx::Point(cx + s * 0.25f, cy + s * 0.2f), s * 0.1f,
                     flags);
  canvas->DrawCircle(gfx::Point(cx - s * 0.1f, cy + s * 0.45f), s * 0.1f,
                     flags);
}

void AstraClearBrowsingDataDialog::DrawCacheIcon(gfx::Canvas* canvas,
                                                 const gfx::Rect& bounds,
                                                 SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.38f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Stack of 3 rectangles / sheets.
  SkPath top;
  top.moveTo(cx - s, cy - s * 0.7f);
  top.lineTo(cx + s, cy - s * 0.7f);
  top.lineTo(cx + s * 0.85f, cy - s * 0.3f);
  top.lineTo(cx - s * 0.85f, cy - s * 0.3f);
  top.close();
  canvas->DrawPath(top, flags);

  SkPath middle;
  middle.moveTo(cx - s * 0.85f, cy - s * 0.15f);
  middle.lineTo(cx + s * 0.85f, cy - s * 0.15f);
  middle.lineTo(cx + s * 0.7f, cy + s * 0.25f);
  middle.lineTo(cx - s * 0.7f, cy + s * 0.25f);
  middle.close();
  canvas->DrawPath(middle, flags);

  SkPath bottom;
  bottom.moveTo(cx - s * 0.7f, cy + s * 0.4f);
  bottom.lineTo(cx + s * 0.7f, cy + s * 0.4f);
  bottom.lineTo(cx + s * 0.55f, cy + s * 0.8f);
  bottom.lineTo(cx - s * 0.55f, cy + s * 0.8f);
  bottom.close();
  canvas->DrawPath(bottom, flags);
}

void AstraClearBrowsingDataDialog::DrawFormIcon(gfx::Canvas* canvas,
                                                const gfx::Rect& bounds,
                                                SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.4f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Form sheet.
  SkPath sheet;
  sheet.moveTo(cx - s * 0.7f, cy - s);
  sheet.lineTo(cx + s * 0.7f, cy - s);
  sheet.lineTo(cx + s * 0.7f, cy + s);
  sheet.lineTo(cx - s * 0.7f, cy + s);
  sheet.close();
  canvas->DrawPath(sheet, flags);

  // Form lines.
  canvas->DrawLine(gfx::Point(cx - s * 0.5f, cy - s * 0.5f),
                   gfx::Point(cx + s * 0.5f, cy - s * 0.5f), flags);
  canvas->DrawLine(gfx::Point(cx - s * 0.5f, cy - s * 0.1f),
                   gfx::Point(cx + s * 0.5f, cy - s * 0.1f), flags);
  canvas->DrawLine(gfx::Point(cx - s * 0.5f, cy + s * 0.3f),
                   gfx::Point(cx + s * 0.1f, cy + s * 0.3f), flags);

  // Checkbox.
  SkPath checkbox;
  checkbox.moveTo(cx - s * 0.5f, cy + s * 0.55f);
  checkbox.lineTo(cx - s * 0.3f, cy + s * 0.55f);
  checkbox.lineTo(cx - s * 0.3f, cy + s * 0.75f);
  checkbox.lineTo(cx - s * 0.5f, cy + s * 0.75f);
  checkbox.close();
  canvas->DrawPath(checkbox, flags);

  // Checkmark in checkbox.
  SkPath check;
  check.moveTo(cx - s * 0.48f, cy + s * 0.67f);
  check.lineTo(cx - s * 0.4f, cy + s * 0.75f);
  check.lineTo(cx - s * 0.32f, cy + s * 0.58f);
  canvas->DrawPath(check, flags);
}

void AstraClearBrowsingDataDialog::DrawPasswordIcon(gfx::Canvas* canvas,
                                                    const gfx::Rect& bounds,
                                                    SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.4f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Lock body.
  SkPath body;
  body.moveTo(cx - s * 0.6f, cy);
  body.lineTo(cx - s * 0.6f, cy + s * 0.7f);
  body.lineTo(cx + s * 0.6f, cy + s * 0.7f);
  body.lineTo(cx + s * 0.6f, cy);
  body.close();
  canvas->DrawPath(body, flags);

  // Shackle / U-shape on top.
  SkPath shackle;
  shackle.moveTo(cx - s * 0.35f, cy);
  shackle.lineTo(cx - s * 0.35f, cy - s * 0.5f);
  shackle.arcTo(
      SkRect::MakeXYWH(cx - s * 0.35f, cy - s * 0.85f, s * 0.7f, s * 0.7f),
      180, 180, false);
  shackle.lineTo(cx + s * 0.35f, cy);
  canvas->DrawPath(shackle, flags);

  // Keyhole dot.
  canvas->DrawCircle(gfx::Point(cx, cy + s * 0.25f), s * 0.12f, flags);

  // Keyhole slot.
  SkPath slot;
  slot.moveTo(cx, cy + s * 0.35f);
  slot.lineTo(cx, cy + s * 0.55f);
  canvas->DrawPath(slot, flags);
}

void AstraClearBrowsingDataDialog::DrawSettingsIcon(gfx::Canvas* canvas,
                                                    const gfx::Rect& bounds,
                                                    SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.4f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Gear / cog shape.
  SkPath gear;
  int num_teeth = 8;
  for (int i = 0; i < num_teeth; ++i) {
    float angle = (i * 360.0f / num_teeth) - 90.0f;
    float rad = angle * 3.14159f / 180.0f;
    float next_angle = ((i + 1) * 360.0f / num_teeth) - 90.0f;
    float next_rad = next_angle * 3.14159f / 180.0f;

    float outer_r = s;
    float inner_r = s * 0.7f;

    float x1 = cx + cosf(rad) * outer_r;
    float y1 = cy + sinf(rad) * outer_r;
    float x2 = cx + cosf(next_rad - 0.15f) * inner_r;
    float y2 = cy + sinf(next_rad - 0.15f) * inner_r;

    if (i == 0) {
      gear.moveTo(x1, y1);
    } else {
      gear.lineTo(x1, y1);
    }
    gear.lineTo(x2, y2);
  }
  gear.close();
  canvas->DrawPath(gear, flags);

  // Center hole.
  canvas->DrawCircle(gfx::Point(cx, cy), s * 0.3f, flags);
}

void AstraClearBrowsingDataDialog::DrawAppIcon(gfx::Canvas* canvas,
                                               const gfx::Rect& bounds,
                                               SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.4f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  float cell_size = s * 0.6f;
  float gap = s * 0.25f;

  // 2x2 grid of app icons (squares with rounded corners).
  for (int row = 0; row < 2; ++row) {
    for (int col = 0; col < 2; ++col) {
      float x = cx - cell_size - gap / 2 + col * (cell_size + gap);
      float y = cy - cell_size - gap / 2 + row * (cell_size + gap);
      SkPath cell;
      cell.addRoundRect(
          SkRect::MakeXYWH(x, y, cell_size, cell_size), s * 0.12f,
          s * 0.12f);
      canvas->DrawPath(cell, flags);
    }
  }
}

void AstraClearBrowsingDataDialog::DrawDataTypeIcon(
    gfx::Canvas* canvas,
    const gfx::Rect& bounds,
    AstraClearBrowsingDataType type,
    SkColor color) {
  switch (type) {
    case AstraClearBrowsingDataType::kBrowsingHistory:
      DrawHistoryIcon(canvas, bounds, color);
      break;
    case AstraClearBrowsingDataType::kDownloadHistory:
      DrawDownloadIcon(canvas, bounds, color);
      break;
    case AstraClearBrowsingDataType::kCookiesAndSiteData:
      DrawCookieIcon(canvas, bounds, color);
      break;
    case AstraClearBrowsingDataType::kCachedImagesAndFiles:
      DrawCacheIcon(canvas, bounds, color);
      break;
    case AstraClearBrowsingDataType::kAutofillFormData:
      DrawFormIcon(canvas, bounds, color);
      break;
    case AstraClearBrowsingDataType::kPasswordsAndSigninData:
      DrawPasswordIcon(canvas, bounds, color);
      break;
    case AstraClearBrowsingDataType::kSiteSettings:
      DrawSettingsIcon(canvas, bounds, color);
      break;
    case AstraClearBrowsingDataType::kHostedAppData:
      DrawAppIcon(canvas, bounds, color);
      break;
  }
}

// ===========================================================================
// AstraClearBrowsingDataDialog
// ===========================================================================

AstraClearBrowsingDataDialog::AstraClearBrowsingDataDialog(
    views::View* anchor_view,
    AstraClearBrowsingDataModel* model)
    : BubbleDialogDelegateView(anchor_view, views::BubbleBorder::TOP_RIGHT) {
  SetModel(model);
  BuildUI();
}

AstraClearBrowsingDataDialog::~AstraClearBrowsingDataDialog() {
  if (scoped_observation_.IsObserving()) {
    scoped_observation_.RemoveObserver();
  }
}

void AstraClearBrowsingDataDialog::SetModel(
    AstraClearBrowsingDataModel* model) {
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

void AstraClearBrowsingDataDialog::RefreshFromModel() {
  if (!model_) {
    return;
  }
  UpdateCheckboxStates();
  UpdateButtonStates();

  if (result_label_) {
    result_label_->SetText(model_->GetResultMessage());
    result_label_->SetVisible(!model_->GetResultMessage().empty());
  }
}

std::u16string AstraClearBrowsingDataDialog::GetWindowTitle() const {
  return u"Clear browsing data";
}

void AstraClearBrowsingDataDialog::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();

  auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  SkColor bg_color = color_provider->GetColor(ui::kColorDialogBackground);
  SetBackground(views::CreateSolidBackground(bg_color));

  SkColor icon_color = color_provider->GetColor(ui::kColorIcon);

  // Redraw header icon.
  if (header_icon_) {
    SkBitmap bitmap;
    bitmap.allocN32Pixels(kHeaderIconSize, kHeaderIconSize);
    bitmap.eraseColor(SK_ColorTRANSPARENT);
    gfx::Canvas canvas(gfx::Size(kHeaderIconSize, kHeaderIconSize), 1.0f,
                       false);
    DrawTrashIcon(&canvas, gfx::Rect(0, 0, kHeaderIconSize, kHeaderIconSize),
                  icon_color);
    header_icon_->SetImage(
        gfx::ImageSkia::CreateFrom1xBitmap(canvas.GetBitmap()));
  }

  // Redraw data type icons.
  for (const auto& row : checkbox_rows_) {
    if (row.checkbox && row.checkbox->GetImage(views::Checkbox::IMAGE_ICON)) {
      SkBitmap bitmap;
      bitmap.allocN32Pixels(kDataTypeIconSize, kDataTypeIconSize);
      bitmap.eraseColor(SK_ColorTRANSPARENT);
      gfx::Canvas canvas(gfx::Size(kDataTypeIconSize, kDataTypeIconSize),
                         1.0f, false);
      DrawDataTypeIcon(&canvas,
                       gfx::Rect(0, 0, kDataTypeIconSize, kDataTypeIconSize),
                       row.type, icon_color);
      row.checkbox->SetImage(
          views::Checkbox::ImagePosition::IMAGE_ICON,
          gfx::ImageSkia::CreateFrom1xBitmap(canvas.GetBitmap()));
    }
  }

  SchedulePaint();
}

void AstraClearBrowsingDataDialog::OnTimeRangeChanged(
    AstraClearBrowsingDataModel* model,
    AstraClearBrowsingDataTimeRange range) {
  DCHECK_EQ(model, model_);
  UpdateButtonStates();
}

void AstraClearBrowsingDataDialog::OnDataTypeToggled(
    AstraClearBrowsingDataModel* model,
    AstraClearBrowsingDataType type,
    bool selected) {
  DCHECK_EQ(model, model_);
  UpdateCheckboxStates();
  UpdateButtonStates();
}

void AstraClearBrowsingDataDialog::OnClearStarted(
    AstraClearBrowsingDataModel* model) {
  DCHECK_EQ(model, model_);
  UpdateButtonStates();
  if (result_label_) {
    result_label_->SetVisible(false);
  }
}

void AstraClearBrowsingDataDialog::OnClearCompleted(
    AstraClearBrowsingDataModel* model,
    bool success) {
  DCHECK_EQ(model, model_);
  UpdateButtonStates();
}

void AstraClearBrowsingDataDialog::OnResultMessageChanged(
    AstraClearBrowsingDataModel* model,
    const std::u16string& message) {
  DCHECK_EQ(model, model_);
  if (result_label_) {
    result_label_->SetText(message);
    result_label_->SetVisible(!message.empty());
  }
  if (GetWidget()) {
    SizeToContents();
  }
}

void AstraClearBrowsingDataDialog::OnClearBrowsingDataModelShutdown(
    AstraClearBrowsingDataModel* model) {
  DCHECK_EQ(model, model_);
  scoped_observation_.RemoveObserver();
  model_ = nullptr;
}

views::Checkbox* AstraClearBrowsingDataDialog::GetDataTypeCheckbox(
    AstraClearBrowsingDataType type) {
  for (const auto& row : checkbox_rows_) {
    if (row.type == type) {
      return row.checkbox;
    }
  }
  return nullptr;
}

// ===========================================================================
// UI Building
// ===========================================================================

void AstraClearBrowsingDataDialog::BuildUI() {
  set_margins(gfx::Insets(0));
  SetArrow(views::BubbleBorder::Arrow::TOP_RIGHT);
  set_close_on_deactivate(true);
  set_close_on_esc(true);

  auto* layout = SetLayoutManager(std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kVertical);

  BuildHeader();
  BuildTimeRangeRow();
  BuildDataTypeCheckboxes();
  BuildResultArea();
  BuildFooter();
  BuildButtons();

  set_min_size(gfx::Size(kDialogMinWidth, 0));
  set_max_size(gfx::Size(kDialogMaxWidth, 0));
}

void AstraClearBrowsingDataDialog::BuildHeader() {
  auto header = std::make_unique<views::View>();
  auto* header_layout =
      header->SetLayoutManager(std::make_unique<views::FlexLayout>());
  header_layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  header_layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);
  header_layout->SetInteriorMargin(
      gfx::Insets::VH(kContentPadding, kContentPadding));

  // Icon.
  auto icon = std::make_unique<views::ImageView>();
  icon->SetImageSize(gfx::Size(kHeaderIconSize, kHeaderIconSize));
  header_icon_ = header->AddChildView(std::move(icon));
  header_icon_->SetProperty(views::kMarginsKey,
                            gfx::Insets::VH(0, 0, 0, 12));

  // Title.
  auto title = std::make_unique<views::Label>(u"Clear browsing data");
  title->SetFontList(views::style::GetFont(
      views::style::CONTEXT_DIALOG_TITLE, views::style::STYLE_PRIMARY));
  title->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title->SetAutoColorReadabilityEnabled(false);
  title_label_ = header->AddChildView(std::move(title));

  AddChildView(std::move(header));
}

void AstraClearBrowsingDataDialog::BuildTimeRangeRow() {
  auto row = std::make_unique<views::View>();
  auto* row_layout =
      row->SetLayoutManager(std::make_unique<views::FlexLayout>());
  row_layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  row_layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);
  row_layout->SetInteriorMargin(
      gfx::Insets::VH(0, kContentPadding, kSectionSpacing, kContentPadding));

  auto label = std::make_unique<views::Label>(u"Time range");
  label->SetFontList(views::style::GetFont(
      views::style::CONTEXT_LABEL, views::style::STYLE_PRIMARY));
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SetAutoColorReadabilityEnabled(false);
  row->AddChildView(std::move(label));

  // Build combobox model from time ranges.
  std::vector<ui::SimpleComboboxModel::Item> items;
  auto ranges = AstraClearBrowsingDataModel::GetAllTimeRanges();
  for (const auto& range : ranges) {
    items.push_back(ui::SimpleComboboxModel::Item(
        AstraClearBrowsingDataModel::GetTimeRangeName(range)));
  }

  auto combobox = std::make_unique<views::Combobox>(
      std::make_unique<ui::SimpleComboboxModel>(std::move(items)));
  combobox->SetCallback(base::BindRepeating(
      &AstraClearBrowsingDataDialog::OnTimeRangeChanged,
      base::Unretained(this)));
  combobox->SetSelectedIndex(static_cast<size_t>(
      AstraClearBrowsingDataModel::GetAllTimeRanges().size() > 0 ? 1 : 0));
  time_range_combobox_ = row->AddChildView(std::move(combobox));
  time_range_combobox_->SetProperty(views::kMarginsKey,
                                    gfx::Insets::VH(0, 0, 0, 0));
  row_layout->SetFlexForView(time_range_combobox_, 1);

  AddChildView(std::move(row));
}

void AstraClearBrowsingDataDialog::BuildDataTypeCheckboxes() {
  auto container = std::make_unique<views::View>();
  auto* container_layout =
      container->SetLayoutManager(std::make_unique<views::FlexLayout>());
  container_layout->SetOrientation(views::LayoutOrientation::kVertical);
  container_layout->SetInteriorMargin(
      gfx::Insets::VH(0, kContentPadding, kSectionSpacing, kContentPadding));

  auto data_types = AstraClearBrowsingDataModel::GetAllDataTypes();
  for (size_t i = 0; i < data_types.size(); ++i) {
    auto type = data_types[i];

    // Row: checkbox icon + label + description.
    auto checkbox = std::make_unique<views::Checkbox>(
        AstraClearBrowsingDataModel::GetDataTypeName(type));
    checkbox->SetAccessibleName(
        AstraClearBrowsingDataModel::GetDataTypeName(type));
    checkbox->SetSubtitleLabel(
        AstraClearBrowsingDataModel::GetDataTypeDescription(type));
    checkbox->SetCallback(base::BindRepeating(
        &AstraClearBrowsingDataDialog::OnDataTypeToggled,
        base::Unretained(this), type));
    checkbox->SetImageLabelSpacing(8);

    // Set icon.
    SkBitmap bitmap;
    bitmap.allocN32Pixels(kDataTypeIconSize, kDataTypeIconSize);
    bitmap.eraseColor(SK_ColorTRANSPARENT);
    gfx::Canvas canvas(gfx::Size(kDataTypeIconSize, kDataTypeIconSize), 1.0f,
                       false);
    DrawDataTypeIcon(&canvas,
                     gfx::Rect(0, 0, kDataTypeIconSize, kDataTypeIconSize),
                     type, SK_ColorBLACK);
    checkbox->SetImage(
        views::Checkbox::ImagePosition::IMAGE_ICON,
        gfx::ImageSkia::CreateFrom1xBitmap(canvas.GetBitmap()));

    auto* checkbox_ptr = container->AddChildView(std::move(checkbox));

    if (i > 0) {
      checkbox_ptr->SetProperty(views::kMarginsKey,
                                gfx::Insets::VH(kCheckboxRowSpacing, 0, 0, 0));
    }

    checkbox_rows_.push_back({checkbox_ptr, type});
  }

  AddChildView(std::move(container));
}

void AstraClearBrowsingDataDialog::BuildResultArea() {
  auto result = std::make_unique<views::View>();
  auto* result_layout =
      result->SetLayoutManager(std::make_unique<views::FlexLayout>());
  result_layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  result_layout->SetInteriorMargin(
      gfx::Insets::VH(0, kContentPadding, kSectionSpacing, kContentPadding));

  auto label = std::make_unique<views::Label>();
  label->SetFontList(views::style::GetFont(
      views::style::CONTEXT_LABEL, views::style::STYLE_SECONDARY));
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SetAutoColorReadabilityEnabled(false);
  label->SetMultiLine(true);
  label->SetVisible(false);
  result_label_ = result->AddChildView(std::move(label));

  AddChildView(std::move(result));
}

void AstraClearBrowsingDataDialog::BuildFooter() {
  auto footer = std::make_unique<views::View>();
  auto* footer_layout =
      footer->SetLayoutManager(std::make_unique<views::FlexLayout>());
  footer_layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  footer_layout->SetInteriorMargin(
      gfx::Insets::VH(0, kContentPadding, kSectionSpacing, kContentPadding));

  auto label = std::make_unique<views::Label>(
      u"Note: Signed-in data like passwords and history may still appear in "
      u"your Google Account if sync is enabled.");
  label->SetFontList(views::style::GetFont(
      views::style::CONTEXT_LABEL, views::style::STYLE_SECONDARY));
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SetAutoColorReadabilityEnabled(false);
  label->SetMultiLine(true);
  footer_label_ = footer->AddChildView(std::move(label));

  AddChildView(std::move(footer));
}

void AstraClearBrowsingDataDialog::BuildButtons() {
  auto button_row = std::make_unique<views::View>();
  auto* button_layout =
      button_row->SetLayoutManager(std::make_unique<views::FlexLayout>());
  button_layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  button_layout->SetMainAxisAlignment(views::LayoutAlignment::kEnd);
  button_layout->SetInteriorMargin(
      gfx::Insets::VH(0, kContentPadding, kContentPadding, kContentPadding));

  // Cancel button.
  auto cancel = views::MdTextButton::CreateSecondaryUiButton(
      base::BindRepeating(&AstraClearBrowsingDataDialog::OnCancelClicked,
                          base::Unretained(this)),
      u"Cancel");
  cancel_button_ = button_row->AddChildView(std::move(cancel));
  cancel_button_->SetProperty(views::kMarginsKey,
                              gfx::Insets::VH(0, kButtonRowSpacing, 0, 0));

  // Clear data button (primary).
  auto clear = views::MdTextButton::CreatePrimaryUiButton(
      base::BindRepeating(&AstraClearBrowsingDataDialog::OnClearClicked,
                          base::Unretained(this)),
      u"Clear data");
  clear_button_ = button_row->AddChildView(std::move(clear));

  AddChildView(std::move(button_row));
}

// ===========================================================================
// Update helpers
// ===========================================================================

void AstraClearBrowsingDataDialog::UpdateCheckboxStates() {
  if (!model_) {
    return;
  }
  for (const auto& row : checkbox_rows_) {
    if (row.checkbox) {
      row.checkbox->SetChecked(model_->IsDataTypeSelected(row.type));
    }
  }
}

void AstraClearBrowsingDataDialog::UpdateButtonStates() {
  if (!model_ || !clear_button_) {
    return;
  }
  bool can_clear = model_->GetSelectedDataTypesCount() > 0 && !model_->IsLoading();
  clear_button_->SetEnabled(can_clear);

  if (cancel_button_) {
    cancel_button_->SetEnabled(!model_->IsLoading());
  }
}

// ===========================================================================
// Button / control handlers
// ===========================================================================

void AstraClearBrowsingDataDialog::OnClearClicked() {
  if (model_) {
    model_->ClearData();
  }
}

void AstraClearBrowsingDataDialog::OnCancelClicked() {
  if (GetWidget()) {
    GetWidget()->CloseWithReason(
        views::Widget::ClosedReason::kCancelButtonClicked);
  }
}

void AstraClearBrowsingDataDialog::OnTimeRangeChanged() {
  if (!model_ || !time_range_combobox_) {
    return;
  }
  auto ranges = AstraClearBrowsingDataModel::GetAllTimeRanges();
  size_t selected = time_range_combobox_->GetSelectedIndex();
  if (selected < ranges.size()) {
    model_->SetTimeRange(ranges[selected]);
  }
}

void AstraClearBrowsingDataDialog::OnDataTypeToggled(
    AstraClearBrowsingDataType type) {
  if (model_) {
    model_->ToggleDataType(type);
  }
}

}  // namespace astra
