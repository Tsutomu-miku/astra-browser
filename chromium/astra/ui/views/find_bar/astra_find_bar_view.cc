// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/find_bar/astra_find_bar_view.h"

#include <algorithm>

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
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace astra {

namespace {

// Draw a magnifying glass icon.
void DrawFindIcon(gfx::Canvas* canvas,
                  const gfx::Rect& bounds,
                  SkColor color) {
  if (bounds.IsEmpty())
    return;

  int cx = bounds.x() + bounds.width() / 2 - 1;
  int cy = bounds.y() + bounds.height() / 2 - 1;
  int size = std::min(bounds.width(), bounds.height()) * 2 / 3;

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
}

// Draw an upward arrow (previous match).
void DrawUpArrowIcon(gfx::Canvas* canvas,
                     const gfx::Rect& bounds,
                     SkColor color) {
  if (bounds.IsEmpty())
    return;

  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int half = std::min(bounds.width(), bounds.height()) / 3;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(2);
  flags.setAntiAlias(true);

  canvas->DrawLine(gfx::Point(cx, cy - half), gfx::Point(cx, cy + half),
                   flags);
  canvas->DrawLine(gfx::Point(cx - half / 2, cy - half / 2),
                   gfx::Point(cx, cy - half), flags);
  canvas->DrawLine(gfx::Point(cx + half / 2, cy - half / 2),
                   gfx::Point(cx, cy - half), flags);
}

// Draw a downward arrow (next match).
void DrawDownArrowIcon(gfx::Canvas* canvas,
                       const gfx::Rect& bounds,
                       SkColor color) {
  if (bounds.IsEmpty())
    return;

  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int half = std::min(bounds.width(), bounds.height()) / 3;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(2);
  flags.setAntiAlias(true);

  canvas->DrawLine(gfx::Point(cx, cy - half), gfx::Point(cx, cy + half),
                   flags);
  canvas->DrawLine(gfx::Point(cx - half / 2, cy + half / 2),
                   gfx::Point(cx, cy + half), flags);
  canvas->DrawLine(gfx::Point(cx + half / 2, cy + half / 2),
                   gfx::Point(cx, cy + half), flags);
}

// Draw an X (close) icon.
void DrawCloseIcon(gfx::Canvas* canvas,
                   const gfx::Rect& bounds,
                   SkColor color) {
  if (bounds.IsEmpty())
    return;

  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int half = std::min(bounds.width(), bounds.height()) / 3;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(2);
  flags.setAntiAlias(true);

  canvas->DrawLine(gfx::Point(cx - half, cy - half),
                   gfx::Point(cx + half, cy + half), flags);
  canvas->DrawLine(gfx::Point(cx + half, cy - half),
                   gfx::Point(cx - half, cy + half), flags);
}

// Draw a highlight icon (paintbrush/bucket representation).
void DrawHighlightIcon(gfx::Canvas* canvas,
                       const gfx::Rect& bounds,
                       SkColor color,
                       bool active) {
  if (bounds.IsEmpty())
    return;

  int x = bounds.x() + 2;
  int y = bounds.y() + 2;
  int w = bounds.width() - 4;
  int h = bounds.height() - 4;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(active ? cc::PaintFlags::kFill_Style
                        : cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.5);
  flags.setAntiAlias(true);

  // Simple "highlight" marker: underline shape.
  canvas->DrawLine(gfx::Point(x, y + h), gfx::Point(x + w, y + h), flags);
  canvas->DrawLine(gfx::Point(x + w / 4, y + h - 3),
                   gfx::Point(x + w * 3 / 4, y + h - 3), flags);
}

// Draw case sensitivity icon (Aa).
void DrawCaseIcon(gfx::Canvas* canvas,
                  const gfx::Rect& bounds,
                  SkColor color,
                  bool active) {
  if (bounds.IsEmpty())
    return;

  int x = bounds.x();
  int y = bounds.y();

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);

  gfx::FontList font;
  canvas->DrawStringRect(u"Aa", font, color, gfx::Rect(x, y, bounds.width(), bounds.height()),
                         gfx::Canvas::TEXT_ALIGN_CENTER,
                         gfx::Canvas::TEXT_VALIGN_MIDDLE);

  if (active) {
    canvas->DrawLine(gfx::Point(x, y + bounds.height() - 2),
                     gfx::Point(x + bounds.width(), y + bounds.height() - 2),
                     flags);
  }
}

}  // namespace

// =========================================================================
// AstraFindBarView
// =========================================================================

AstraFindBarView::AstraFindBarView() {
  SetLayoutManager(std::make_unique<views::FillLayout>());
  BuildLayout();
}

AstraFindBarView::~AstraFindBarView() = default;

// -- Show / hide ------------------------------------------------------------

void AstraFindBarView::Show() {
  SetVisible(true);
  if (textfield_)
    textfield_->RequestFocus();
}

void AstraFindBarView::Hide() {
  SetVisible(false);
}

// -- Find state -------------------------------------------------------------

void AstraFindBarView::SetFindText(const std::u16string& text) {
  find_text_ = text;
  if (textfield_)
    textfield_->SetText(text);
}

std::u16string AstraFindBarView::GetFindText() const {
  if (textfield_)
    return textfield_->GetText();
  return find_text_;
}

void AstraFindBarView::SetMatchInfo(int current_match, int total_matches) {
  current_match_ = current_match;
  total_matches_ = total_matches;
  UpdateMatchLabel();
  UpdateButtonStates();
}

void AstraFindBarView::SetHighlightAll(bool highlight) {
  highlight_all_ = highlight;
  if (highlight_button_) {
    // Update visual state.
    SchedulePaint();
  }
  UpdateButtonStates();
}

void AstraFindBarView::SetCaseSensitive(bool case_sensitive) {
  case_sensitive_ = case_sensitive;
  UpdateButtonStates();
}

// -- views::View ------------------------------------------------------------

gfx::Size AstraFindBarView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return gfx::Size(0, kFindBarHeight);
}

void AstraFindBarView::Layout() {
  if (!find_icon_ || !textfield_ || !match_label_ || !close_button_)
    return;

  int x = kHorizontalPadding;
  int y = (kFindBarHeight - kIconSize) / 2;

  // Find icon.
  find_icon_->SetBounds(x, y, kIconSize, kIconSize);
  x += kIconSize + kIconTextSpacing;

  // Close button (on the right).
  int close_x = width() - kHorizontalPadding - kButtonSize;
  close_button_->SetBounds(close_x, (kFindBarHeight - kButtonSize) / 2,
                           kButtonSize, kButtonSize);
  int right_edge = close_x - kButtonSpacing;

  // Case button.
  right_edge -= kButtonSize;
  case_button_->SetBounds(right_edge, (kFindBarHeight - kButtonSize) / 2,
                          kButtonSize, kButtonSize);
  right_edge -= kButtonSpacing;

  // Highlight button.
  right_edge -= kButtonSize;
  highlight_button_->SetBounds(right_edge, (kFindBarHeight - kButtonSize) / 2,
                               kButtonSize, kButtonSize);
  right_edge -= kButtonSpacing;

  // Next button.
  right_edge -= kButtonSize;
  next_button_->SetBounds(right_edge, (kFindBarHeight - kButtonSize) / 2,
                          kButtonSize, kButtonSize);
  right_edge -= kButtonSpacing;

  // Previous button.
  right_edge -= kButtonSize;
  previous_button_->SetBounds(right_edge, (kFindBarHeight - kButtonSize) / 2,
                              kButtonSize, kButtonSize);
  right_edge -= kButtonSpacing;

  // Match label.
  int match_width = 80;
  right_edge -= match_width;
  match_label_->SetBounds(right_edge, 0, match_width, kFindBarHeight);
  right_edge -= kIconTextSpacing;

  // Text field fills remaining space.
  int text_width = right_edge - x;
  textfield_->SetBounds(x, (kFindBarHeight - 24) / 2, text_width, 24);
}

void AstraFindBarView::OnThemeChanged() {
  views::View::OnThemeChanged();
  UpdateColors();
}

void AstraFindBarView::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  node_data->role = ax::mojom::Role::kDialog;
  node_data->SetName(u"Find in page");
}

// -- views::TextfieldController ---------------------------------------------

void AstraFindBarView::ContentsChanged(views::Textfield* sender,
                                       const std::u16string& new_contents) {
  find_text_ = new_contents;
  if (delegate_)
    delegate_->OnFindTextChanged(new_contents);
}

bool AstraFindBarView::HandleKeyEvent(views::Textfield* sender,
                                      const ui::KeyEvent& key_event) {
  if (key_event.type() != ui::ET_KEY_PRESSED)
    return false;

  switch (key_event.key_code()) {
    case ui::VKEY_RETURN:
      if (key_event.IsShiftDown()) {
        if (delegate_)
          delegate_->OnFindPrevious();
      } else {
        if (delegate_)
          delegate_->OnFindNext();
      }
      return true;
    case ui::VKEY_ESCAPE:
      if (delegate_)
        delegate_->OnFindBarClosed();
      return true;
    default:
      break;
  }
  return false;
}

// -- Private methods --------------------------------------------------------

void AstraFindBarView::BuildLayout() {
  // Find icon.
  auto find_icon = std::make_unique<views::ImageView>();
  find_icon->SetImageSize(gfx::Size(kIconSize, kIconSize));
  find_icon_ = AddChildView(std::move(find_icon));

  // Text field.
  auto textfield = std::make_unique<views::Textfield>();
  textfield->set_controller(this);
  textfield->SetPlaceholderText(u"Find in page");
  textfield_ = AddChildView(std::move(textfield));

  // Match count label.
  auto match_label = std::make_unique<views::Label>(u"0 of 0");
  match_label->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  match_label->SetAutoColorReadabilityEnabled(false);
  match_label_ = AddChildView(std::move(match_label));

  // Previous button.
  auto previous_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraFindBarView::OnPreviousPressed,
                          base::Unretained(this)));
  previous_button->SetImageSize(gfx::Size(kButtonSize, kButtonSize));
  previous_button_ = AddChildView(std::move(previous_button));

  // Next button.
  auto next_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraFindBarView::OnNextPressed,
                          base::Unretained(this)));
  next_button->SetImageSize(gfx::Size(kButtonSize, kButtonSize));
  next_button_ = AddChildView(std::move(next_button));

  // Highlight all button.
  auto highlight_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraFindBarView::OnHighlightAllToggled,
                          base::Unretained(this)));
  highlight_button->SetImageSize(gfx::Size(kButtonSize, kButtonSize));
  highlight_button_ = AddChildView(std::move(highlight_button));

  // Case sensitivity button.
  auto case_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraFindBarView::OnCaseSensitiveToggled,
                          base::Unretained(this)));
  case_button->SetImageSize(gfx::Size(kButtonSize, kButtonSize));
  case_button_ = AddChildView(std::move(case_button));

  // Close button.
  auto close_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraFindBarView::OnClosePressed,
                          base::Unretained(this)));
  close_button->SetImageSize(gfx::Size(kButtonSize, kButtonSize));
  close_button_ = AddChildView(std::move(close_button));

  UpdateColors();
  UpdateMatchLabel();
  UpdateButtonStates();
}

void AstraFindBarView::UpdateColors() {
  if (!GetWidget())
    return;

  SkColor text_color = GetNativeTheme()->GetSystemColor(
      ui::NativeTheme::kColorId_LabelEnabledColor);
  SkColor secondary_color =
      GetNativeTheme()->GetSystemColor(
          ui::NativeTheme::kColorId_LabelSecondaryColor);
  SkColor bg_color = GetNativeTheme()->GetSystemColor(
      ui::NativeTheme::kColorId_WindowBackground);

  SetBackground(views::CreateSolidBackground(bg_color));

  if (match_label_)
    match_label_->SetEnabledColor(secondary_color);

  // Redraw icons.
  if (find_icon_) {
    gfx::Canvas canvas(gfx::Size(kIconSize, kIconSize), /*image_scale=*/1.0f,
                       false);
    DrawFindIcon(&canvas, gfx::Rect(0, 0, kIconSize, kIconSize),
                 secondary_color);
    find_icon_->SetImage(
        gfx::ImageSkia(canvas.GetBitmap(), gfx::Size(kIconSize, kIconSize)));
  }

  if (previous_button_) {
    gfx::Canvas canvas(gfx::Size(kButtonSize, kButtonSize),
                       /*image_scale=*/1.0f, false);
    DrawUpArrowIcon(&canvas, gfx::Rect(0, 0, kButtonSize, kButtonSize),
                    text_color);
    previous_button_->SetImage(
        views::ImageButton::STATE_NORMAL,
        gfx::ImageSkia(canvas.GetBitmap(),
                       gfx::Size(kButtonSize, kButtonSize)));
  }

  if (next_button_) {
    gfx::Canvas canvas(gfx::Size(kButtonSize, kButtonSize),
                       /*image_scale=*/1.0f, false);
    DrawDownArrowIcon(&canvas, gfx::Rect(0, 0, kButtonSize, kButtonSize),
                      text_color);
    next_button_->SetImage(
        views::ImageButton::STATE_NORMAL,
        gfx::ImageSkia(canvas.GetBitmap(),
                       gfx::Size(kButtonSize, kButtonSize)));
  }

  if (close_button_) {
    gfx::Canvas canvas(gfx::Size(kButtonSize, kButtonSize),
                       /*image_scale=*/1.0f, false);
    DrawCloseIcon(&canvas, gfx::Rect(0, 0, kButtonSize, kButtonSize),
                  text_color);
    close_button_->SetImage(
        views::ImageButton::STATE_NORMAL,
        gfx::ImageSkia(canvas.GetBitmap(),
                       gfx::Size(kButtonSize, kButtonSize)));
  }

  if (highlight_button_) {
    gfx::Canvas canvas(gfx::Size(kButtonSize, kButtonSize),
                       /*image_scale=*/1.0f, false);
    DrawHighlightIcon(&canvas, gfx::Rect(0, 0, kButtonSize, kButtonSize),
                      text_color, highlight_all_);
    highlight_button_->SetImage(
        views::ImageButton::STATE_NORMAL,
        gfx::ImageSkia(canvas.GetBitmap(),
                       gfx::Size(kButtonSize, kButtonSize)));
  }

  if (case_button_) {
    gfx::Canvas canvas(gfx::Size(kButtonSize, kButtonSize),
                       /*image_scale=*/1.0f, false);
    DrawCaseIcon(&canvas, gfx::Rect(0, 0, kButtonSize, kButtonSize),
                 text_color, case_sensitive_);
    case_button_->SetImage(
        views::ImageButton::STATE_NORMAL,
        gfx::ImageSkia(canvas.GetBitmap(),
                       gfx::Size(kButtonSize, kButtonSize)));
  }
}

void AstraFindBarView::UpdateMatchLabel() {
  if (!match_label_)
    return;

  if (find_text_.empty()) {
    match_label_->SetText(std::u16string());
    return;
  }

  if (total_matches_ == 0) {
    match_label_->SetText(u"0 results");
    return;
  }

  match_label_->SetText(base::NumberToString16(current_match_) + u" of " +
                        base::NumberToString16(total_matches_));
}

void AstraFindBarView::UpdateButtonStates() {
  bool has_text = !find_text_.empty();
  bool has_matches = total_matches_ > 0;

  if (previous_button_)
    previous_button_->SetEnabled(has_matches);
  if (next_button_)
    next_button_->SetEnabled(has_matches);
  if (highlight_button_)
    highlight_button_->SetEnabled(has_text);
  if (case_button_)
    case_button_->SetEnabled(has_text);
}

// -- Button handlers --------------------------------------------------------

void AstraFindBarView::OnPreviousPressed() {
  if (delegate_)
    delegate_->OnFindPrevious();
}

void AstraFindBarView::OnNextPressed() {
  if (delegate_)
    delegate_->OnFindNext();
}

void AstraFindBarView::OnClosePressed() {
  if (delegate_)
    delegate_->OnFindBarClosed();
}

void AstraFindBarView::OnHighlightAllToggled() {
  SetHighlightAll(!highlight_all_);
  if (delegate_)
    delegate_->OnHighlightAllChanged(highlight_all_);
}

void AstraFindBarView::OnCaseSensitiveToggled() {
  SetCaseSensitive(!case_sensitive_);
  if (delegate_)
    delegate_->OnCaseSensitivityChanged(case_sensitive_);
}

}  // namespace astra
