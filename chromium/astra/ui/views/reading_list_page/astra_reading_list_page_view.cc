// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/reading_list_page/astra_reading_list_page_view.h"

#include <memory>
#include <string>

#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkColor.h"
#include "skia/core/SkPath.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/events/event.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/metadata/view_factory.h"
#include "ui/views/style/typography.h"
#include "ui/views/view.h"
#include "ui/views/view_class_properties.h"

namespace astra {

namespace {

// Helper to format a read time into a string.
std::u16string FormatReadTime(int minutes) {
  if (minutes < 1) {
    return u"< 1 min read";
  }
  return base::UTF8ToUTF16(base::NumberToString(minutes) + " min read");
}

// Helper to format a date relative to now.
std::u16string FormatDate(base::Time date) {
  if (date.is_null()) {
    return u"";
  }
  base::Time now = base::Time::Now();
  base::TimeDelta delta = now - date;
  int days = delta.InDays();
  if (days == 0) {
    return u"Today";
  }
  if (days == 1) {
    return u"Yesterday";
  }
  if (days < 7) {
    return base::UTF8ToUTF16(base::NumberToString(days) + " days ago");
  }
  if (days < 30) {
    int weeks = days / 7;
    return base::UTF8ToUTF16(base::NumberToString(weeks) +
                             (weeks == 1 ? " week ago" : " weeks ago"));
  }
  base::Time::Exploded exploded;
  date.LocalExplode(&exploded);
  const char* months[] = {
      "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  std::string label =
      std::string(months[exploded.month - 1]) + " " +
      base::NumberToString(exploded.day) + ", " +
      base::NumberToString(exploded.year);
  return base::UTF8ToUTF16(label);
}

}  // namespace

// ===========================================================================
// AstraReadingListItemView
// ===========================================================================

BEGIN_METADATA(AstraReadingListItemView)
END_METADATA

AstraReadingListItemView::AstraReadingListItemView(
    const AstraReadingListEntry& entry)
    : entry_(entry) {
  Build();
}

AstraReadingListItemView::~AstraReadingListItemView() = default;

void AstraReadingListItemView::Update(const AstraReadingListEntry& entry) {
  entry_ = entry;
  if (title_label_) {
    title_label_->SetText(entry_.title);
  }
  if (site_label_) {
    site_label_->SetText(base::UTF8ToUTF16(entry_.site_name));
  }
  if (preview_label_) {
    preview_label_->SetText(entry_.preview_text);
  }
  if (read_time_label_) {
    read_time_label_->SetText(FormatReadTime(entry_.estimated_read_time_minutes));
  }
  if (date_label_) {
    date_label_->SetText(FormatDate(entry_.date_added));
  }
  SchedulePaint();
}

void AstraReadingListItemView::SetDisplayMode(AstraReadingListDisplayMode mode) {
  if (display_mode_ == mode) {
    return;
  }
  display_mode_ = mode;
  InvalidateLayout();
  SchedulePaint();
}

void AstraReadingListItemView::SetSelected(bool selected) {
  if (selected_ == selected) {
    return;
  }
  selected_ = selected;
  SchedulePaint();
}

void AstraReadingListItemView::Build() {
  // Title label.
  title_label_ = AddChildView(std::make_unique<views::Label>(entry_.title));
  title_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label_->SetTextContext(views::style::CONTEXT_LABEL);
  title_label_->SetTextStyle(views::style::STYLE_PRIMARY);
  title_label_->SetMultiLine(true);

  // Site name label.
  site_label_ = AddChildView(std::make_unique<views::Label>(
      base::UTF8ToUTF16(entry_.site_name)));
  site_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  site_label_->SetTextContext(views::style::CONTEXT_LABEL);
  site_label_->SetTextStyle(views::style::STYLE_SECONDARY);

  // Preview text label.
  preview_label_ = AddChildView(std::make_unique<views::Label>(
      entry_.preview_text));
  preview_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  preview_label_->SetTextContext(views::style::CONTEXT_LABEL);
  preview_label_->SetTextStyle(views::style::STYLE_SECONDARY);
  preview_label_->SetMultiLine(true);

  // Read time label.
  read_time_label_ = AddChildView(std::make_unique<views::Label>(
      FormatReadTime(entry_.estimated_read_time_minutes)));
  read_time_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  read_time_label_->SetTextContext(views::style::CONTEXT_LABEL);
  read_time_label_->SetTextStyle(views::style::STYLE_SECONDARY);

  // Date label.
  date_label_ = AddChildView(std::make_unique<views::Label>(
      FormatDate(entry_.date_added)));
  date_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  date_label_->SetTextContext(views::style::CONTEXT_LABEL);
  date_label_->SetTextStyle(views::style::STYLE_SECONDARY);

  // Favorite button.
  auto favorite_button = std::make_unique<views::ImageButton>(
      base::BindRepeating([](AstraReadingListItemView* view) {
        if (view->favorite_callback_) {
          view->favorite_callback_.Run(view->entry_.id);
        }
      },
      base::Unretained(this)));
  favorite_button->SetPreferredSize(gfx::Size(20, 20));
  favorite_button_ = AddChildView(std::move(favorite_button));

  // More button.
  auto more_button = std::make_unique<views::ImageButton>(
      base::BindRepeating([](AstraReadingListItemView* view) {
        if (view->more_callback_) {
          view->more_callback_.Run(view->entry_.id);
        }
      },
      base::Unretained(this)));
  more_button->SetPreferredSize(gfx::Size(20, 20));
  more_button_ = AddChildView(std::move(more_button));
}

void AstraReadingListItemView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  const auto* color_provider = GetColorProvider();
  SkColor bg_color = color_provider ? color_provider->GetColor(ui::kColorWindow)
                                    : SK_ColorWHITE;
  SkColor border_color =
      color_provider ? color_provider->GetColor(ui::kColorSeparator)
                     : SK_ColorGRAY;
  SkColor text_color =
      color_provider ? color_provider->GetColor(ui::kColorLabelForeground)
                     : SK_ColorBLACK;
  SkColor secondary_color =
      color_provider ? color_provider->GetColor(ui::kColorLabelForegroundSecondary)
                     : SK_ColorGRAY;
  SkColor accent_color =
      color_provider ? color_provider->GetColor(ui::kColorAccent)
                     : SK_ColorBLUE;

  gfx::Rect bounds = GetContentsBounds();

  // Draw background (selected highlight).
  if (selected_) {
    cc::PaintFlags bg_flags;
    bg_flags.setColor(SkColorSetA(accent_color, 0x1A));
    bg_flags.setStyle(cc::PaintFlags::kFill_Style);
    bg_flags.setAntiAlias(true);
    canvas->DrawRoundRect(bounds, 8, bg_flags);
  }

  // Draw unread indicator (left bar).
  if (!entry_.is_read) {
    cc::PaintFlags unread_flags;
    unread_flags.setColor(accent_color);
    unread_flags.setStyle(cc::PaintFlags::kFill_Style);
    unread_flags.setAntiAlias(true);
    gfx::Rect unread_bar(bounds.x() + 4, bounds.y() + 16, 3,
                         display_mode_ == AstraReadingListDisplayMode::kList
                             ? bounds.height() - 32
                             : 24);
    canvas->DrawRoundRect(unread_bar, 2, unread_flags);
  }

  // Draw favicon area (placeholder circle).
  gfx::Rect favicon_bounds;
  if (display_mode_ == AstraReadingListDisplayMode::kList) {
    favicon_bounds = gfx::Rect(bounds.x() + 16, bounds.y() + 16, 32, 32);
  } else {
    favicon_bounds = gfx::Rect(bounds.x() + 16, bounds.y() + 16, 24, 24);
  }
  DrawFavicon(canvas, favicon_bounds);

  // Draw star (favorite) icon on the button.
  gfx::Rect star_bounds = favorite_button_->bounds();
  SkColor star_color = entry_.is_favorited ? SK_ColorYELLOW : secondary_color;
  DrawStar(canvas, star_bounds, star_color);

  // Draw more icon.
  gfx::Rect more_bounds = more_button_->bounds();
  DrawMore(canvas, more_bounds, secondary_color);

  // Draw clock icon next to read time.
  if (display_mode_ == AstraReadingListDisplayMode::kList) {
    gfx::Rect clock_bounds(read_time_label_->x() - 16, read_time_label_->y(),
                           14, 14);
    DrawClock(canvas, clock_bounds, secondary_color);
  }
}

void AstraReadingListItemView::DrawFavicon(gfx::Canvas* canvas,
                                           const gfx::Rect& bounds) {
  cc::PaintFlags flags;
  flags.setColor(SK_ColorLTGRAY);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);
  canvas->DrawCircle(
      gfx::Point(bounds.x() + bounds.width() / 2, bounds.y() + bounds.height() / 2),
      bounds.width() / 2, flags);

  // Draw a simple bookmark icon as placeholder.
  DrawBookmarkIcon(canvas, bounds, SK_ColorWHITE);
}

void AstraReadingListItemView::DrawStar(gfx::Canvas* canvas,
                                        const gfx::Rect& bounds,
                                        SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float size = std::min(bounds.width(), bounds.height()) * 0.4f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);

  SkPath path;
  for (int i = 0; i < 5; i++) {
    float angle = -M_PI_2 + i * 2 * M_PI / 5;
    float x = cx + size * cos(angle);
    float y = cy + size * sin(angle);
    if (i == 0) {
      path.moveTo(x, y);
    } else {
      path.lineTo(x, y);
    }
    float inner_angle = angle + M_PI / 5;
    float inner_size = size * 0.4f;
    float ix = cx + inner_size * cos(inner_angle);
    float iy = cy + inner_size * sin(inner_angle);
    path.lineTo(ix, iy);
  }
  path.close();
  canvas->DrawPath(path, flags);
}

void AstraReadingListItemView::DrawClock(gfx::Canvas* canvas,
                                         const gfx::Rect& bounds,
                                         SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float radius = std::min(bounds.width(), bounds.height()) * 0.4f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Circle.
  canvas->DrawCircle(gfx::Point(cx, cy), radius, flags);

  // Hour hand.
  SkPath path;
  path.moveTo(cx, cy);
  path.lineTo(cx, cy - radius * 0.5f);
  canvas->DrawPath(path, flags);

  // Minute hand.
  SkPath path2;
  path2.moveTo(cx, cy);
  path2.lineTo(cx + radius * 0.4f, cy - radius * 0.2f);
  canvas->DrawPath(path2, flags);
}

void AstraReadingListItemView::DrawMore(gfx::Canvas* canvas,
                                        const gfx::Rect& bounds,
                                        SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float dot_radius = 2.0f;
  float spacing = 4.0f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);

  canvas->DrawCircle(gfx::Point(cx, cy - spacing), dot_radius, flags);
  canvas->DrawCircle(gfx::Point(cx, cy), dot_radius, flags);
  canvas->DrawCircle(gfx::Point(cx, cy + spacing), dot_radius, flags);
}

void AstraReadingListItemView::DrawBookmarkIcon(gfx::Canvas* canvas,
                                                const gfx::Rect& bounds,
                                                SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int w = std::min(bounds.width(), bounds.height()) * 0.5f;
  int h = w * 1.2f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);

  SkPath path;
  path.moveTo(cx - w / 2, cy - h / 2);
  path.lineTo(cx + w / 2, cy - h / 2);
  path.lineTo(cx + w / 2, cy + h / 2);
  path.lineTo(cx, cy + h / 2 - w * 0.3f);
  path.lineTo(cx - w / 2, cy + h / 2);
  path.close();
  canvas->DrawPath(path, flags);
}

gfx::Size AstraReadingListItemView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  if (display_mode_ == AstraReadingListDisplayMode::kList) {
    int width = available_size.width().value_or(600);
    return gfx::Size(width, kListItemHeight);
  }
  return gfx::Size(kGridItemWidth, kGridItemHeight);
}

void AstraReadingListItemView::Layout() {
  gfx::Rect bounds = GetContentsBounds();
  int padding = 16;
  int favicon_size = 32;
  int icon_size = 20;

  if (display_mode_ == AstraReadingListDisplayMode::kList) {
    int left = bounds.x() + padding + favicon_size + padding;
    int right = bounds.right() - padding;
    int top = bounds.y() + padding;

    // Title (top line).
    int title_height = 24;
    title_label_->SetBoundsRect(
        gfx::Rect(left, top, right - left - icon_size * 2 - 16, title_height));
    title_label_->SetElideBehavior(gfx::ELIDE_TAIL);

    // Favorite button (top right).
    favorite_button_->SetBoundsRect(
        gfx::Rect(right - icon_size * 2 - 8, top, icon_size, icon_size));
    more_button_->SetBoundsRect(
        gfx::Rect(right - icon_size, top, icon_size, icon_size));

    // Site name (second line).
    int site_height = 18;
    site_label_->SetBoundsRect(
        gfx::Rect(left, top + title_height + 4, 200, site_height));
    site_label_->SetElideBehavior(gfx::ELIDE_TAIL);

    // Preview text (third line, multi-line).
    int preview_top = top + title_height + site_height + 8;
    int preview_height = 36;
    preview_label_->SetBoundsRect(
        gfx::Rect(left, preview_top, right - left - padding, preview_height));

    // Read time and date (bottom).
    int bottom_y = bounds.bottom() - padding - 18;
    read_time_label_->SetBoundsRect(
        gfx::Rect(left + 20, bottom_y, 120, 18));
    date_label_->SetBoundsRect(
        gfx::Rect(right - 120, bottom_y, 120, 18));
    date_label_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  } else {
    int left = bounds.x() + padding;
    int right = bounds.right() - padding;
    int top = bounds.y() + padding;

    // Favicon + site (top).
    site_label_->SetBoundsRect(
        gfx::Rect(left + favicon_size + 8, top + 6, 150, 20));
    site_label_->SetElideBehavior(gfx::ELIDE_TAIL);

    // Title.
    int title_top = top + favicon_size + 8;
    title_label_->SetBoundsRect(
        gfx::Rect(left, title_top, bounds.width() - padding * 2, 48));
    title_label_->SetElideBehavior(gfx::ELIDE_TAIL);

    // Preview.
    int preview_top = title_top + 52;
    preview_label_->SetBoundsRect(
        gfx::Rect(left, preview_top, bounds.width() - padding * 2, 36));

    // Bottom row: read time, date, favorite, more.
    int bottom_y = bounds.bottom() - padding - 20;
    read_time_label_->SetBoundsRect(
        gfx::Rect(left + 20, bottom_y, 100, 18));
    favorite_button_->SetBoundsRect(
        gfx::Rect(right - icon_size * 2 - 8, bottom_y - 2, icon_size, icon_size));
    more_button_->SetBoundsRect(
        gfx::Rect(right - icon_size, bottom_y - 2, icon_size, icon_size));

    // Hide date in grid mode to save space.
    date_label_->SetVisible(false);
  }
}

bool AstraReadingListItemView::OnMousePressed(const ui::MouseEvent& event) {
  if (event.IsOnlyLeftMouseButton()) {
    if (click_callback_) {
      click_callback_.Run(entry_.id);
    }
    return true;
  }
  return views::View::OnMousePressed(event);
}

void AstraReadingListItemView::OnThemeChanged() {
  views::View::OnThemeChanged();
  SchedulePaint();
}

// ===========================================================================
// AstraReadingListFilterItemView
// ===========================================================================

BEGIN_METADATA(AstraReadingListFilterItemView)
END_METADATA

AstraReadingListFilterItemView::AstraReadingListFilterItemView(
    const std::u16string& label,
    int count,
    bool is_selected)
    : label_(label), count_(count), selected_(is_selected) {
  Build();
}

AstraReadingListFilterItemView::~AstraReadingListFilterItemView() = default;

void AstraReadingListFilterItemView::SetSelected(bool selected) {
  if (selected_ == selected) {
    return;
  }
  selected_ = selected;
  SchedulePaint();
}

void AstraReadingListFilterItemView::SetCount(int count) {
  count_ = count;
  if (count_view_) {
    count_view_->SetText(base::UTF8ToUTF16(base::NumberToString(count)));
  }
}

void AstraReadingListFilterItemView::DrawBookmarkIcon(gfx::Canvas* canvas,
                                                      const gfx::Rect& bounds,
                                                      SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int w = bounds.width() * 0.6f;
  int h = w * 1.1f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);

  SkPath path;
  path.moveTo(cx - w / 2, cy - h / 2);
  path.lineTo(cx + w / 2, cy - h / 2);
  path.lineTo(cx + w / 2, cy + h / 2);
  path.lineTo(cx, cy + h / 2 - w * 0.3f);
  path.lineTo(cx - w / 2, cy + h / 2);
  path.close();
  canvas->DrawPath(path, flags);
}

void AstraReadingListFilterItemView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  const auto* color_provider = GetColorProvider();
  SkColor text_color =
      color_provider ? color_provider->GetColor(ui::kColorLabelForeground)
                     : SK_ColorBLACK;
  SkColor secondary_color =
      color_provider ? color_provider->GetColor(ui::kColorLabelForegroundSecondary)
                     : SK_ColorGRAY;
  SkColor accent_color =
      color_provider ? color_provider->GetColor(ui::kColorAccent)
                     : SK_ColorBLUE;
  SkColor bg_color =
      color_provider ? color_provider->GetColor(ui::kColorWindow)
                     : SK_ColorWHITE;

  gfx::Rect bounds = GetContentsBounds();

  // Selected background.
  if (selected_) {
    cc::PaintFlags bg_flags;
    bg_flags.setColor(SkColorSetA(accent_color, 0x14));
    bg_flags.setStyle(cc::PaintFlags::kFill_Style);
    bg_flags.setAntiAlias(true);
    canvas->DrawRoundRect(bounds, 6, bg_flags);
  }

  // Icon.
  gfx::Rect icon_bounds(bounds.x() + kIconPadding,
                        bounds.y() + (kItemHeight - kIconSize) / 2,
                        kIconSize, kIconSize);
  SkColor icon_color = selected_ ? accent_color : secondary_color;
  DrawBookmarkIcon(canvas, icon_bounds, icon_color);
}

gfx::Size AstraReadingListFilterItemView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  int width = available_size.width().value_or(220);
  return gfx::Size(width, kItemHeight);
}

void AstraReadingListFilterItemView::Layout() {
  gfx::Rect bounds = GetContentsBounds();
  int text_left = bounds.x() + kIconPadding + kIconSize + 12;
  int count_width = 40;

  label_view_->SetBoundsRect(
      gfx::Rect(text_left, bounds.y(),
                bounds.width() - text_left - count_width - 8, kItemHeight));
  label_view_->SetVerticalAlignment(gfx::ALIGN_MIDDLE);
  label_view_->SetElideBehavior(gfx::ELIDE_TAIL);

  count_view_->SetBoundsRect(
      gfx::Rect(bounds.right() - count_width - 8, bounds.y(),
                count_width, kItemHeight));
  count_view_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  count_view_->SetVerticalAlignment(gfx::ALIGN_MIDDLE);
}

bool AstraReadingListFilterItemView::OnMousePressed(
    const ui::MouseEvent& event) {
  if (event.IsOnlyLeftMouseButton()) {
    if (select_callback_) {
      select_callback_.Run();
    }
    return true;
  }
  return views::View::OnMousePressed(event);
}

void AstraReadingListFilterItemView::OnThemeChanged() {
  views::View::OnThemeChanged();
  SchedulePaint();
}

// ===========================================================================
// AstraReadingListFolderItemView
// ===========================================================================

BEGIN_METADATA(AstraReadingListFolderItemView)
END_METADATA

AstraReadingListFolderItemView::AstraReadingListFolderItemView(
    const AstraReadingListFolder& folder,
    bool is_selected)
    : folder_id_(folder.id),
      name_(folder.name),
      entry_count_(folder.entry_count),
      selected_(is_selected) {
  Build();
}

AstraReadingListFolderItemView::~AstraReadingListFolderItemView() = default;

void AstraReadingListFolderItemView::Update(
    const AstraReadingListFolder& folder) {
  name_ = folder.name;
  entry_count_ = folder.entry_count;
  if (name_label_) {
    name_label_->SetText(name_);
  }
  if (count_label_) {
    count_label_->SetText(base::UTF8ToUTF16(base::NumberToString(entry_count_)));
  }
  SchedulePaint();
}

void AstraReadingListFolderItemView::SetSelected(bool selected) {
  if (selected_ == selected) {
    return;
  }
  selected_ = selected;
  SchedulePaint();
}

void AstraReadingListFolderItemView::DrawFolderIcon(gfx::Canvas* canvas,
                                                    const gfx::Rect& bounds,
                                                    SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int w = bounds.width() * 0.85f;
  int h = w * 0.7f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);

  SkPath path;
  path.moveTo(cx - w / 2, cy - h / 2 + h * 0.3f);
  path.lineTo(cx - w / 2 + w * 0.35f, cy - h / 2);
  path.lineTo(cx - w / 2 + w * 0.55f, cy - h / 2);
  path.lineTo(cx - w / 2 + w * 0.55f + 2, cy - h / 2 + h * 0.3f);
  path.lineTo(cx + w / 2, cy - h / 2 + h * 0.3f);
  path.lineTo(cx + w / 2, cy + h / 2);
  path.lineTo(cx - w / 2, cy + h / 2);
  path.close();
  canvas->DrawPath(path, flags);
}

void AstraReadingListFolderItemView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  const auto* color_provider = GetColorProvider();
  SkColor secondary_color =
      color_provider ? color_provider->GetColor(ui::kColorLabelForegroundSecondary)
                     : SK_ColorGRAY;
  SkColor accent_color =
      color_provider ? color_provider->GetColor(ui::kColorAccent)
                     : SK_ColorBLUE;

  gfx::Rect bounds = GetContentsBounds();

  // Selected background.
  if (selected_) {
    cc::PaintFlags bg_flags;
    bg_flags.setColor(SkColorSetA(accent_color, 0x14));
    bg_flags.setStyle(cc::PaintFlags::kFill_Style);
    bg_flags.setAntiAlias(true);
    canvas->DrawRoundRect(bounds, 6, bg_flags);
  }

  // Folder icon.
  gfx::Rect icon_bounds(bounds.x() + kIconPadding,
                        bounds.y() + (kItemHeight - kIconSize) / 2,
                        kIconSize, kIconSize);
  SkColor icon_color = selected_ ? accent_color : secondary_color;
  DrawFolderIcon(canvas, icon_bounds, icon_color);
}

gfx::Size AstraReadingListFolderItemView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  int width = available_size.width().value_or(220);
  return gfx::Size(width, kItemHeight);
}

void AstraReadingListFolderItemView::Layout() {
  gfx::Rect bounds = GetContentsBounds();
  int text_left = bounds.x() + kIconPadding + kIconSize + 12;
  int count_width = 36;

  name_label_->SetBoundsRect(
      gfx::Rect(text_left, bounds.y(),
                bounds.width() - text_left - count_width - 8, kItemHeight));
  name_label_->SetVerticalAlignment(gfx::ALIGN_MIDDLE);
  name_label_->SetElideBehavior(gfx::ELIDE_TAIL);

  count_label_->SetBoundsRect(
      gfx::Rect(bounds.right() - count_width - 8, bounds.y(),
                count_width, kItemHeight));
  count_label_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  count_label_->SetVerticalAlignment(gfx::ALIGN_MIDDLE);
}

bool AstraReadingListFolderItemView::OnMousePressed(
    const ui::MouseEvent& event) {
  if (event.IsOnlyLeftMouseButton()) {
    if (select_callback_) {
      select_callback_.Run(folder_id_);
    }
    return true;
  }
  return views::View::OnMousePressed(event);
}

void AstraReadingListFolderItemView::OnThemeChanged() {
  views::View::OnThemeChanged();
  SchedulePaint();
}

// ===========================================================================
// AstraReadingListDetailView
// ===========================================================================

BEGIN_METADATA(AstraReadingListDetailView)
END_METADATA

AstraReadingListDetailView::AstraReadingListDetailView() {
  Build();
}

AstraReadingListDetailView::~AstraReadingListDetailView() = default;

void AstraReadingListDetailView::SetEntry(const AstraReadingListEntry* entry) {
  entry_ = entry;
  UpdateContent();
}

void AstraReadingListDetailView::Clear() {
  entry_ = nullptr;
  UpdateContent();
}

void AstraReadingListDetailView::Build() {
  empty_label_ = AddChildView(std::make_unique<views::Label>(
      u"Select an article to view details"));
  empty_label_->SetTextContext(views::style::CONTEXT_LABEL);
  empty_label_->SetTextStyle(views::style::STYLE_SECONDARY);
  empty_label_->SetMultiLine(true);
  empty_label_->SetHorizontalAlignment(gfx::ALIGN_CENTER);

  content_container_ = AddChildView(std::make_unique<views::View>());

  title_label_ = content_container_->AddChildView(
      std::make_unique<views::Label>());
  title_label_->SetTextContext(views::style::CONTEXT_HEADLINE);
  title_label_->SetTextStyle(views::style::STYLE_PRIMARY);
  title_label_->SetMultiLine(true);
  title_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  site_label_ = content_container_->AddChildView(
      std::make_unique<views::Label>());
  site_label_->SetTextContext(views::style::CONTEXT_LABEL);
  site_label_->SetTextStyle(views::style::STYLE_SECONDARY);

  preview_label_ = content_container_->AddChildView(
      std::make_unique<views::Label>());
  preview_label_->SetTextContext(views::style::CONTEXT_BODY_TEXT_LARGE);
  preview_label_->SetTextStyle(views::style::STYLE_SECONDARY);
  preview_label_->SetMultiLine(true);
  preview_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  read_time_label_ = content_container_->AddChildView(
      std::make_unique<views::Label>());
  read_time_label_->SetTextContext(views::style::CONTEXT_LABEL);
  read_time_label_->SetTextStyle(views::style::STYLE_SECONDARY);

  date_label_ = content_container_->AddChildView(
      std::make_unique<views::Label>());
  date_label_->SetTextContext(views::style::CONTEXT_LABEL);
  date_label_->SetTextStyle(views::style::STYLE_SECONDARY);

  category_label_ = content_container_->AddChildView(
      std::make_unique<views::Label>());
  category_label_->SetTextContext(views::style::CONTEXT_LABEL);
  category_label_->SetTextStyle(views::style::STYLE_SECONDARY);

  // Buttons.
  mark_read_button_ = content_container_->AddChildView(
      std::make_unique<views::ImageButton>(
          base::BindRepeating([](AstraReadingListDetailView* view) {
            if (view->mark_read_callback_) {
              view->mark_read_callback_.Run();
            }
          }, base::Unretained(this))));
  mark_read_button_->SetPreferredSize(gfx::Size(kButtonSize, kButtonSize));

  remove_button_ = content_container_->AddChildView(
      std::make_unique<views::ImageButton>(
          base::BindRepeating([](AstraReadingListDetailView* view) {
            if (view->remove_callback_) {
              view->remove_callback_.Run();
            }
          }, base::Unretained(this))));
  remove_button_->SetPreferredSize(gfx::Size(kButtonSize, kButtonSize));

  share_button_ = content_container_->AddChildView(
      std::make_unique<views::ImageButton>(
          base::BindRepeating([](AstraReadingListDetailView* view) {
            if (view->share_callback_) {
              view->share_callback_.Run();
            }
          }, base::Unretained(this))));
  share_button_->SetPreferredSize(gfx::Size(kButtonSize, kButtonSize));

  UpdateContent();
}

void AstraReadingListDetailView::UpdateContent() {
  if (!entry_) {
    empty_label_->SetVisible(true);
    content_container_->SetVisible(false);
    return;
  }

  empty_label_->SetVisible(false);
  content_container_->SetVisible(true);

  title_label_->SetText(entry_->title);
  site_label_->SetText(base::UTF8ToUTF16(entry_->site_name));
  preview_label_->SetText(entry_->preview_text);
  read_time_label_->SetText(FormatReadTime(entry_->estimated_read_time_minutes));
  date_label_->SetText(FormatDate(entry_->date_added));
  category_label_->SetText(base::UTF8ToUTF16(entry_->category));

  SchedulePaint();
}

void AstraReadingListDetailView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  const auto* color_provider = GetColorProvider();
  SkColor separator_color =
      color_provider ? color_provider->GetColor(ui::kColorSeparator)
                     : SK_ColorLTGRAY;
  SkColor secondary_color =
      color_provider ? color_provider->GetColor(ui::kColorLabelForegroundSecondary)
                     : SK_ColorGRAY;

  // Draw left border.
  cc::PaintFlags border_flags;
  border_flags.setColor(separator_color);
  border_flags.setStyle(cc::PaintFlags::kFill_Style);
  canvas->DrawRect(gfx::Rect(0, 0, 1, height()), border_flags);

  if (entry_) {
    // Draw clock icon.
    gfx::Rect clock_bounds(read_time_label_->x() - 18,
                           read_time_label_->y() + 2, 14, 14);
    DrawClockIcon(canvas, clock_bounds, secondary_color);

    // Draw button icons.
    gfx::Rect mark_bounds = mark_read_button_->bounds();
    DrawBookmarkFilled(canvas, mark_bounds, secondary_color);

    gfx::Rect trash_bounds = remove_button_->bounds();
    DrawTrashIcon(canvas, trash_bounds, secondary_color);

    gfx::Rect share_bounds = share_button_->bounds();
    DrawShareIcon(canvas, share_bounds, secondary_color);
  }
}

void AstraReadingListDetailView::DrawBookmarkFilled(gfx::Canvas* canvas,
                                                    const gfx::Rect& bounds,
                                                    SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int w = bounds.width() * 0.45f;
  int h = w * 1.2f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);

  SkPath path;
  path.moveTo(cx - w / 2, cy - h / 2);
  path.lineTo(cx + w / 2, cy - h / 2);
  path.lineTo(cx + w / 2, cy + h / 2);
  path.lineTo(cx, cy + h / 2 - w * 0.3f);
  path.lineTo(cx - w / 2, cy + h / 2);
  path.close();
  canvas->DrawPath(path, flags);
}

void AstraReadingListDetailView::DrawTrashIcon(gfx::Canvas* canvas,
                                               const gfx::Rect& bounds,
                                               SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int w = bounds.width() * 0.5f;
  int h = bounds.height() * 0.55f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(2);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Lid.
  SkPath lid;
  lid.moveTo(cx - w / 2, cy - h / 2 + 4);
  lid.lineTo(cx + w / 2, cy - h / 2 + 4);
  canvas->DrawPath(lid, flags);

  // Lid handle.
  SkPath handle;
  handle.moveTo(cx - w / 4, cy - h / 2);
  handle.lineTo(cx + w / 4, cy - h / 2);
  canvas->DrawPath(handle, flags);

  // Body.
  SkPath body;
  body.moveTo(cx - w / 2 + 2, cy - h / 2 + 4);
  body.lineTo(cx - w / 2 + 4, cy + h / 2);
  body.lineTo(cx + w / 2 - 4, cy + h / 2);
  body.lineTo(cx + w / 2 - 2, cy - h / 2 + 4);
  canvas->DrawPath(body, flags);

  // Trash lines.
  SkPath line1;
  line1.moveTo(cx - 2, cy - h / 2 + 10);
  line1.lineTo(cx - 2, cy + h / 2 - 6);
  canvas->DrawPath(line1, flags);

  SkPath line2;
  line2.moveTo(cx + 3, cy - h / 2 + 10);
  line2.lineTo(cx + 3, cy + h / 2 - 6);
  canvas->DrawPath(line2, flags);
}

void AstraReadingListDetailView::DrawShareIcon(gfx::Canvas* canvas,
                                               const gfx::Rect& bounds,
                                               SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float size = bounds.width() * 0.4f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(2);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Top dot.
  SkPath dot1;
  dot1.moveTo(cx, cy - size);
  dot1.lineTo(cx, cy - size - 1);
  canvas->DrawPath(dot1, flags);

  // Left dot.
  SkPath dot2;
  dot2.moveTo(cx - size, cy + size * 0.5f);
  dot2.lineTo(cx - size - 1, cy + size * 0.5f);
  canvas->DrawPath(dot2, flags);

  // Right dot.
  SkPath dot3;
  dot3.moveTo(cx + size, cy + size * 0.5f);
  dot3.lineTo(cx + size + 1, cy + size * 0.5f);
  canvas->DrawPath(dot3, flags);

  // Connecting lines.
  SkPath line1;
  line1.moveTo(cx, cy - size + 3);
  line1.lineTo(cx - size * 0.7f, cy + size * 0.3f);
  canvas->DrawPath(line1, flags);

  SkPath line2;
  line2.moveTo(cx, cy - size + 3);
  line2.lineTo(cx + size * 0.7f, cy + size * 0.3f);
  canvas->DrawPath(line2, flags);

  SkPath line3;
  line3.moveTo(cx - size + 3, cy + size * 0.5f);
  line3.lineTo(cx + size - 3, cy + size * 0.5f);
  canvas->DrawPath(line3, flags);
}

void AstraReadingListDetailView::DrawClockIcon(gfx::Canvas* canvas,
                                               const gfx::Rect& bounds,
                                               SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float radius = std::min(bounds.width(), bounds.height()) * 0.4f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  canvas->DrawCircle(gfx::Point(cx, cy), radius, flags);

  SkPath hour;
  hour.moveTo(cx, cy);
  hour.lineTo(cx, cy - radius * 0.5f);
  canvas->DrawPath(hour, flags);

  SkPath minute;
  minute.moveTo(cx, cy);
  minute.lineTo(cx + radius * 0.4f, cy - radius * 0.2f);
  canvas->DrawPath(minute, flags);
}

gfx::Size AstraReadingListDetailView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return gfx::Size(kPanelWidth, 0);
}

void AstraReadingListDetailView::Layout() {
  gfx::Rect bounds = GetContentsBounds();
  int padding = 20;

  if (!entry_) {
    empty_label_->SetBoundsRect(
        gfx::Rect(padding, bounds.height() / 2 - 40,
                  bounds.width() - padding * 2, 80));
    content_container_->SetVisible(false);
    return;
  }

  content_container_->SetVisible(true);
  int content_width = bounds.width() - padding * 2;
  int y = padding;

  // Site name.
  site_label_->SetBoundsRect(gfx::Rect(padding, y, content_width, 20));
  y += 24;

  // Title.
  int title_height = 64;
  title_label_->SetBoundsRect(gfx::Rect(padding, y, content_width, title_height));
  y += title_height + 12;

  // Category and date on same line.
  category_label_->SetBoundsRect(gfx::Rect(padding, y, 150, 20));
  date_label_->SetBoundsRect(
      gfx::Rect(padding + content_width - 120, y, 120, 20));
  date_label_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  y += 24;

  // Preview / description.
  int preview_height = 120;
  preview_label_->SetBoundsRect(
      gfx::Rect(padding, y, content_width, preview_height));
  y += preview_height + 16;

  // Read time.
  read_time_label_->SetBoundsRect(
      gfx::Rect(padding + 18, y, 150, 20));
  y += 28;

  // Action buttons at bottom.
  int button_y = bounds.bottom() - padding - kButtonSize;
  mark_read_button_->SetBoundsRect(
      gfx::Rect(padding, button_y, kButtonSize, kButtonSize));
  remove_button_->SetBoundsRect(
      gfx::Rect(padding + kButtonSize + 12, button_y, kButtonSize, kButtonSize));
  share_button_->SetBoundsRect(
      gfx::Rect(padding + kButtonSize * 2 + 24, button_y, kButtonSize,
                kButtonSize));
}

void AstraReadingListDetailView::OnThemeChanged() {
  views::View::OnThemeChanged();
  SchedulePaint();
}

// ===========================================================================
// AstraReadingListEmptyView
// ===========================================================================

BEGIN_METADATA(AstraReadingListEmptyView)
END_METADATA

AstraReadingListEmptyView::AstraReadingListEmptyView() {
  title_label_ = AddChildView(std::make_unique<views::Label>(
      u"No articles in your reading list"));
  title_label_->SetTextContext(views::style::CONTEXT_HEADLINE);
  title_label_->SetTextStyle(views::style::STYLE_PRIMARY);
  title_label_->SetHorizontalAlignment(gfx::ALIGN_CENTER);

  subtitle_label_ = AddChildView(std::make_unique<views::Label>(
      u"Save articles you want to read later"));
  subtitle_label_->SetTextContext(views::style::CONTEXT_BODY_TEXT_LARGE);
  subtitle_label_->SetTextStyle(views::style::STYLE_SECONDARY);
  subtitle_label_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
}

AstraReadingListEmptyView::~AstraReadingListEmptyView() = default;

void AstraReadingListEmptyView::SetIsSearchEmpty(bool is_search_empty) {
  is_search_empty_ = is_search_empty;
  if (is_search_empty_) {
    title_label_->SetText(u"No results found");
    subtitle_label_->SetText(u"Try different search keywords");
  } else {
    title_label_->SetText(u"No articles in your reading list");
    subtitle_label_->SetText(u"Save articles you want to read later");
  }
}

void AstraReadingListEmptyView::DrawEmptyIcon(gfx::Canvas* canvas,
                                              const gfx::Rect& bounds,
                                              SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int w = bounds.width() * 0.4f;
  int h = w * 1.3f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);
  flags.setAlpha(128);

  SkPath path;
  path.moveTo(cx - w / 2, cy - h / 2);
  path.lineTo(cx + w / 2, cy - h / 2);
  path.lineTo(cx + w / 2, cy + h / 2);
  path.lineTo(cx, cy + h / 2 - w * 0.3f);
  path.lineTo(cx - w / 2, cy + h / 2);
  path.close();
  canvas->DrawPath(path, flags);
}

void AstraReadingListEmptyView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  const auto* color_provider = GetColorProvider();
  SkColor secondary_color =
      color_provider ? color_provider->GetColor(ui::kColorLabelForegroundSecondary)
                     : SK_ColorGRAY;

  // Draw empty icon above the text.
  int icon_size = 64;
  gfx::Rect icon_bounds(width() / 2 - icon_size / 2,
                        height() / 2 - 80, icon_size, icon_size);
  DrawEmptyIcon(canvas, icon_bounds, secondary_color);
}

gfx::Size AstraReadingListEmptyView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  int width = available_size.width().value_or(400);
  return gfx::Size(width, 300);
}

void AstraReadingListEmptyView::Layout() {
  int center_y = height() / 2 + 20;
  int content_width = std::min(width(), 400);
  int x = (width() - content_width) / 2;

  title_label_->SetBoundsRect(gfx::Rect(x, center_y, content_width, 32));
  subtitle_label_->SetBoundsRect(
      gfx::Rect(x, center_y + 36, content_width, 24));
}

void AstraReadingListEmptyView::OnThemeChanged() {
  views::View::OnThemeChanged();
  SchedulePaint();
}

// ===========================================================================
// AstraReadingListPageView
// ===========================================================================

BEGIN_METADATA(AstraReadingListPageView)
END_METADATA

AstraReadingListPageView::AstraReadingListPageView() {
  Build();
}

AstraReadingListPageView::AstraReadingListPageView(AstraReadingListModel* model)
    : model_(model) {
  Build();
  if (model_) {
    model_observation_.Observe(model_);
    RebuildSidebar();
    RebuildListContent();
  }
}

AstraReadingListPageView::~AstraReadingListPageView() = default;

void AstraReadingListPageView::SetModel(AstraReadingListModel* model) {
  if (model_observation_.IsObserving()) {
    model_observation_.Reset();
  }
  model_ = model;
  if (model_) {
    model_observation_.Observe(model_);
  }
  RebuildSidebar();
  RebuildListContent();
  UpdateDetailPanel();
}

void AstraReadingListPageView::SetDisplayMode(AstraReadingListDisplayMode mode) {
  if (display_mode_ == mode) {
    return;
  }
  display_mode_ = mode;
  for (auto* item : list_items_) {
    item->SetDisplayMode(mode);
  }
  InvalidateLayout();
  SchedulePaint();
}

void AstraReadingListPageView::SetSelectedEntry(const std::string& id) {
  selected_entry_id_ = id;
  for (auto* item : list_items_) {
    item->SetSelected(item->entry_id() == id);
  }
  UpdateDetailPanel();
}

void AstraReadingListPageView::Build() {
  // Main layout via FlexLayout.
  auto* layout = SetLayoutManager(std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kVertical);
  layout->SetCrossAxisAlignment(views::LayoutAlignment::kStretch);

  // Toolbar.
  BuildToolbar();

  // Main content area (sidebar + items + detail panel).
  auto* content_row = AddChildView(std::make_unique<views::View>());
  content_row->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToMinimum,
                               views::MaximumFlexSizeRule::kUnbounded));
  auto* row_layout =
      content_row->SetLayoutManager(std::make_unique<views::FlexLayout>());
  row_layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  row_layout->SetCrossAxisAlignment(views::LayoutAlignment::kStretch);

  // Sidebar.
  BuildSidebar();
  content_row->AddChildView(sidebar_container_.get());

  // Content scroll area.
  BuildContent();
  content_row->AddChildView(content_scroll_.get());

  // Detail panel.
  BuildDetailPanel();
  content_row->AddChildView(detail_panel_.get());
}

void AstraReadingListPageView::BuildToolbar() {
  toolbar_ = AddChildView(std::make_unique<views::View>());
  toolbar_->SetPreferredSize(gfx::Size(0, kToolbarHeight));
  toolbar_->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorTRANSPARENT));

  auto* toolbar_layout =
      toolbar_->SetLayoutManager(std::make_unique<views::FlexLayout>());
  toolbar_layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  toolbar_layout->SetMainAxisAlignment(views::LayoutAlignment::kStart);
  toolbar_layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);
  toolbar_layout->SetInteriorMargin(gfx::Insets::VH(0, kContentPadding));

  // Search field.
  auto search_field = std::make_unique<views::Textfield>();
  search_field->SetPlaceholderText(u"Search reading list...");
  search_field->set_controller(this);
  search_field_ = toolbar_->AddChildView(std::move(search_field));
  search_field_->SetPreferredSize(gfx::Size(kSearchFieldWidth, 36));

  // Spacer.
  auto* spacer = toolbar_->AddChildView(std::make_unique<views::View>());
  spacer->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToMinimum,
                               views::MaximumFlexSizeRule::kUnbounded));

  // Sort button.
  auto sort_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraReadingListPageView::OnSortClicked,
                          base::Unretained(this)));
  sort_button->SetPreferredSize(gfx::Size(kButtonSize, kButtonSize));
  sort_button_ = toolbar_->AddChildView(std::move(sort_button));

  // List view button.
  auto list_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraReadingListPageView::OnListViewClicked,
                          base::Unretained(this)));
  list_button->SetPreferredSize(gfx::Size(kButtonSize, kButtonSize));
  list_view_button_ = toolbar_->AddChildView(std::move(list_button));

  // Grid view button.
  auto grid_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraReadingListPageView::OnGridViewClicked,
                          base::Unretained(this)));
  grid_button->SetPreferredSize(gfx::Size(kButtonSize, kButtonSize));
  grid_view_button_ = toolbar_->AddChildView(std::move(grid_button));

  // Add button.
  auto add_button = std::make_unique<views::ImageButton>(
      base::BindRepeating(&AstraReadingListPageView::OnAddEntryClicked,
                          base::Unretained(this)));
  add_button->SetPreferredSize(gfx::Size(kButtonSize, kButtonSize));
  add_button_ = toolbar_->AddChildView(std::move(add_button));
}

void AstraReadingListPageView::BuildSidebar() {
  sidebar_container_ = std::make_unique<views::View>();
  sidebar_container_->SetPreferredSize(gfx::Size(kSidebarWidth, 0));
  sidebar_container_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
                               views::MaximumFlexSizeRule::kPreferred));

  auto* layout =
      sidebar_container_->SetLayoutManager(std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kVertical);
  layout->SetCrossAxisAlignment(views::LayoutAlignment::kStretch);
  layout->SetInteriorMargin(gfx::Insets::VH(12, 8));

  // Filter list.
  filter_list_ = sidebar_container_->AddChildView(
      std::make_unique<views::View>());
  auto* filter_layout =
      filter_list_->SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  filter_layout->set_between_child_spacing(2);

  // Folders header.
  folders_header_label_ = sidebar_container_->AddChildView(
      std::make_unique<views::Label>(u"Folders"));
  folders_header_label_->SetTextContext(views::style::CONTEXT_LABEL);
  folders_header_label_->SetTextStyle(views::style::STYLE_SECONDARY);
  folders_header_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  folders_header_label_->SetBorder(
      views::CreateEmptyBorder(gfx::Insets::VH(12, 8)));

  // Folder list (scrollable).
  sidebar_scroll_ = sidebar_container_->AddChildView(
      std::make_unique<views::ScrollView>());
  sidebar_scroll_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToMinimum,
                               views::MaximumFlexSizeRule::kUnbounded));
  sidebar_scroll_->SetDrawOverflowIndicator(false);

  folder_list_ = std::make_unique<views::View>();
  auto* folder_layout =
      folder_list_->SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  folder_layout->set_between_child_spacing(2);
  sidebar_scroll_->SetContents(std::move(folder_list_));
}

void AstraReadingListPageView::BuildContent() {
  content_scroll_ = std::make_unique<views::ScrollView>();
  content_scroll_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToMinimum,
                               views::MaximumFlexSizeRule::kUnbounded));
  content_scroll_->SetDrawOverflowIndicator(false);

  content_container_ = std::make_unique<views::View>();
  content_container_->SetBorder(
      views::CreateEmptyBorder(gfx::Insets::VH(kContentPadding, kContentPadding)));
  content_scroll_->SetContents(std::move(content_container_));

  // Empty state view (initially hidden).
  empty_view_ = content_scroll_->contents()->AddChildView(
      std::make_unique<AstraReadingListEmptyView>());
  empty_view_->SetVisible(false);
}

void AstraReadingListPageView::BuildDetailPanel() {
  detail_panel_ = std::make_unique<AstraReadingListDetailView>();
  detail_panel_->SetPreferredSize(gfx::Size(kDetailPanelWidth, 0));
  detail_panel_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
                               views::MaximumFlexSizeRule::kPreferred));

  detail_panel_->SetMarkReadCallback(base::BindRepeating(
      &AstraReadingListPageView::OnDetailMarkRead, base::Unretained(this)));
  detail_panel_->SetRemoveCallback(base::BindRepeating(
      &AstraReadingListPageView::OnDetailRemove, base::Unretained(this)));
  detail_panel_->SetShareCallback(base::BindRepeating(
      &AstraReadingListPageView::OnDetailShare, base::Unretained(this)));
}

void AstraReadingListPageView::RebuildSidebar() {
  // Clear existing filter items.
  filter_list_->RemoveAllChildViews();
  filter_items_.clear();

  // Create standard filter items.
  auto add_filter = [&](const std::u16string& label, int count,
                        AstraReadingListFilter filter) {
    auto item = std::make_unique<AstraReadingListFilterItemView>(
        label, count,
        current_sidebar_filter_ == filter && selected_folder_id_.empty());
    item->SetSelectCallback(base::BindRepeating(
        &AstraReadingListPageView::OnFilterSelected, base::Unretained(this),
        filter));
    filter_items_.push_back(
        filter_list_->AddChildView(std::move(item)));
  };

  if (model_) {
    add_filter(u"All", static_cast<int>(model_->GetCount()),
               AstraReadingListFilter::kAll);
    add_filter(u"Unread", static_cast<int>(model_->GetUnreadCount()),
               AstraReadingListFilter::kUnread);
    add_filter(u"Favorites", static_cast<int>(model_->GetFavoritesCount()),
               AstraReadingListFilter::kFavorites);
  } else {
    add_filter(u"All", 0, AstraReadingListFilter::kAll);
    add_filter(u"Unread", 0, AstraReadingListFilter::kUnread);
    add_filter(u"Favorites", 0, AstraReadingListFilter::kFavorites);
  }

  // Clear existing folder items.
  folder_list_->RemoveAllChildViews();
  folder_items_.clear();

  // Create folder items.
  if (model_) {
    for (const auto& folder : model_->GetFolders()) {
      auto item = std::make_unique<AstraReadingListFolderItemView>(
          folder, selected_folder_id_ == folder.id);
      item->SetSelectCallback(base::BindRepeating(
          &AstraReadingListPageView::OnFolderSelected, base::Unretained(this)));
      folder_items_.push_back(
          folder_list_->AddChildView(std::move(item)));
    }
  }

  filter_list_->InvalidateLayout();
  folder_list_->InvalidateLayout();
}

void AstraReadingListPageView::RebuildListContent() {
  content_container_->RemoveAllChildViews();
  list_items_.clear();

  auto entries = GetDisplayedEntries();

  // Show empty state if no entries.
  if (entries.empty()) {
    empty_view_->SetVisible(true);
    empty_view_->SetIsSearchEmpty(!model_->GetSearchQuery().empty());
    content_container_->AddChildView(empty_view_.get());
    return;
  }

  empty_view_->SetVisible(false);

  // Create list items.
  for (const auto& entry : entries) {
    auto item = std::make_unique<AstraReadingListItemView>(entry);
    item->SetDisplayMode(display_mode_);
    item->SetSelected(entry.id == selected_entry_id_);
    item->SetClickCallback(base::BindRepeating(
        &AstraReadingListPageView::OnEntryClicked, base::Unretained(this)));
    item->SetFavoriteCallback(base::BindRepeating(
        &AstraReadingListPageView::OnEntryFavorite, base::Unretained(this)));
    item->SetMoreCallback(base::BindRepeating(
        &AstraReadingListPageView::OnEntryMore, base::Unretained(this)));
    list_items_.push_back(
        content_container_->AddChildView(std::move(item)));
  }

  content_container_->InvalidateLayout();
}

void AstraReadingListPageView::UpdateDetailPanel() {
  if (!model_ || selected_entry_id_.empty()) {
    detail_panel_->Clear();
    return;
  }
  const auto* entry = model_->GetEntry(selected_entry_id_);
  detail_panel_->SetEntry(entry);
}

std::vector<AstraReadingListEntry>
AstraReadingListPageView::GetDisplayedEntries() const {
  if (!model_) {
    return {};
  }
  return model_->GetFilteredEntries();
}

// -- AstraReadingListObserver: ----------------------------------------------

void AstraReadingListPageView::OnReadingListChanged() {
  RebuildSidebar();
  RebuildListContent();
  UpdateDetailPanel();
}

void AstraReadingListPageView::OnEntryAdded(const std::string& id) {
  OnReadingListChanged();
}

void AstraReadingListPageView::OnEntryRemoved(const std::string& id) {
  if (selected_entry_id_ == id) {
    selected_entry_id_.clear();
  }
  OnReadingListChanged();
}

void AstraReadingListPageView::OnEntryUpdated(const std::string& id) {
  OnReadingListChanged();
}

void AstraReadingListPageView::OnFolderAdded(const std::string& id) {
  RebuildSidebar();
}

void AstraReadingListPageView::OnFolderRemoved(const std::string& id) {
  if (selected_folder_id_ == id) {
    selected_folder_id_.clear();
  }
  RebuildSidebar();
}

void AstraReadingListPageView::OnSearchChanged(const std::u16string& query) {
  RebuildListContent();
}

void AstraReadingListPageView::OnFilterChanged() {
  RebuildListContent();
}

void AstraReadingListPageView::OnReadingListModelShutdown() {
  model_observation_.Reset();
  model_ = nullptr;
}

// -- views::View: -----------------------------------------------------------

void AstraReadingListPageView::Layout() {
  views::View::Layout();

  // Layout the content container items.
  int content_width = content_container_->width();
  int y = 0;

  if (display_mode_ == AstraReadingListDisplayMode::kList) {
    for (auto* item : list_items_) {
      gfx::Size size = item->CalculatePreferredSize(
          views::SizeBounds(content_width, std::nullopt));
      item->SetBoundsRect(gfx::Rect(0, y, content_width, size.height()));
      y += size.height() + kListItemSpacing;
    }
  } else {
    // Grid layout: calculate columns.
    int item_width = AstraReadingListItemView::kGridItemWidth;
    int spacing = kGridItemSpacing;
    int total = item_width + spacing;
    int columns = std::max(1, (content_width + spacing) / total);
    int actual_spacing = (content_width - columns * item_width) / (columns + 1);

    int col = 0;
    int row_height = 0;
    for (size_t i = 0; i < list_items_.size(); ++i) {
      auto* item = list_items_[i];
      int x = actual_spacing + col * (item_width + actual_spacing);
      item->SetBoundsRect(gfx::Rect(x, y, item_width,
                                    AstraReadingListItemView::kGridItemHeight));
      row_height = std::max(row_height,
                            AstraReadingListItemView::kGridItemHeight);
      ++col;
      if (col >= columns) {
        col = 0;
        y += row_height + kGridItemSpacing;
        row_height = 0;
      }
    }
  }
}

gfx::Size AstraReadingListPageView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return gfx::Size(800, 600);
}

void AstraReadingListPageView::OnThemeChanged() {
  views::View::OnThemeChanged();

  const auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  // Update toolbar border color.
  SkColor separator = color_provider->GetColor(ui::kColorSeparator);
  toolbar_->SetBorder(
      views::CreateSolidSidedBorder(0, 0, 1, 0, separator));

  SchedulePaint();
}

void AstraReadingListPageView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  // Draw toolbar button icons.
  const auto* color_provider = GetColorProvider();
  SkColor icon_color =
      color_provider ? color_provider->GetColor(ui::kColorIcon) : SK_ColorGRAY;

  gfx::Rect search_bounds = search_field_->bounds();
  // Search icon (drawn at start of textfield area - approximate).
  gfx::Rect search_icon(search_bounds.x() + 8,
                        search_bounds.y() + (search_bounds.height() - 18) / 2,
                        18, 18);
  DrawSearchIcon(canvas, search_icon, icon_color);

  gfx::Rect sort_bounds = sort_button_->bounds();
  DrawSortIcon(canvas, sort_bounds, icon_color);

  gfx::Rect list_bounds = list_view_button_->bounds();
  DrawListIcon(canvas, list_bounds, icon_color);

  gfx::Rect grid_bounds = grid_view_button_->bounds();
  DrawGridIcon(canvas, grid_bounds, icon_color);

  gfx::Rect add_bounds = add_button_->bounds();
  DrawAddIcon(canvas, add_bounds, icon_color);
}

// -- TextfieldController: ---------------------------------------------------

void AstraReadingListPageView::ContentsChanged(
    views::Textfield* sender,
    const std::u16string& new_contents) {
  if (model_) {
    model_->SetSearchQuery(new_contents);
  }
}

// -- Icon drawing helpers ---------------------------------------------------

void AstraReadingListPageView::DrawSearchIcon(gfx::Canvas* canvas,
                                              const gfx::Rect& bounds,
                                              SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float radius = std::min(bounds.width(), bounds.height()) * 0.28f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(2);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  canvas->DrawCircle(gfx::Point(cx - radius * 0.2f, cy - radius * 0.2f),
                     radius, flags);

  SkPath path;
  path.moveTo(cx + radius * 0.5f, cy + radius * 0.5f);
  path.lineTo(cx + radius * 1.1f, cy + radius * 1.1f);
  canvas->DrawPath(path, flags);
}

void AstraReadingListPageView::DrawAddIcon(gfx::Canvas* canvas,
                                           const gfx::Rect& bounds,
                                           SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int size = std::min(bounds.width(), bounds.height()) * 0.4f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(2);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  canvas->DrawLine(gfx::Point(cx - size, cy), gfx::Point(cx + size, cy), flags);
  canvas->DrawLine(gfx::Point(cx, cy - size), gfx::Point(cx, cy + size), flags);
}

void AstraReadingListPageView::DrawGridIcon(gfx::Canvas* canvas,
                                            const gfx::Rect& bounds,
                                            SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int size = std::min(bounds.width(), bounds.height()) * 0.5f;
  int cell = size / 2 - 2;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);

  canvas->DrawRect(gfx::Rect(cx - size / 2, cy - size / 2, cell, cell), flags);
  canvas->DrawRect(gfx::Rect(cx + 2, cy - size / 2, cell, cell), flags);
  canvas->DrawRect(gfx::Rect(cx - size / 2, cy + 2, cell, cell), flags);
  canvas->DrawRect(gfx::Rect(cx + 2, cy + 2, cell, cell), flags);
}

void AstraReadingListPageView::DrawListIcon(gfx::Canvas* canvas,
                                            const gfx::Rect& bounds,
                                            SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int w = std::min(bounds.width(), bounds.height()) * 0.6f;
  int spacing = w * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(2);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  canvas->DrawLine(gfx::Point(cx - w / 2, cy - spacing),
                   gfx::Point(cx + w / 2, cy - spacing), flags);
  canvas->DrawLine(gfx::Point(cx - w / 2, cy),
                   gfx::Point(cx + w / 2, cy), flags);
  canvas->DrawLine(gfx::Point(cx - w / 2, cy + spacing),
                   gfx::Point(cx + w / 2, cy + spacing), flags);
}

void AstraReadingListPageView::DrawSortIcon(gfx::Canvas* canvas,
                                            const gfx::Rect& bounds,
                                            SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int w = std::min(bounds.width(), bounds.height()) * 0.5f;
  int h = w * 0.7f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(2);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Up arrow.
  SkPath up;
  up.moveTo(cx, cy - h / 2);
  up.lineTo(cx - w / 2, cy - h / 2 + w * 0.3f);
  up.moveTo(cx, cy - h / 2);
  up.lineTo(cx + w / 2, cy - h / 2 + w * 0.3f);
  canvas->DrawPath(up, flags);

  // Horizontal line for sort bars.
  canvas->DrawLine(gfx::Point(cx - w / 2, cy),
                   gfx::Point(cx + w / 2, cy), flags);

  // Down arrow.
  SkPath down;
  down.moveTo(cx, cy + h / 2);
  down.lineTo(cx - w / 2, cy + h / 2 - w * 0.3f);
  down.moveTo(cx, cy + h / 2);
  down.lineTo(cx + w / 2, cy + h / 2 - w * 0.3f);
  canvas->DrawPath(down, flags);
}

// -- Button handlers --------------------------------------------------------

void AstraReadingListPageView::OnAddEntryClicked() {
  // TODO(astra): Show add dialog or add current tab.
  // For now, add a sample entry.
  if (model_) {
    model_->AddEntry(u"New Article", "https://example.com/article",
                     u"Preview text for the new article...",
                     8, "Technology", "Later");
  }
}

void AstraReadingListPageView::OnSortClicked() {
  // TODO(astra): Show sort dropdown menu.
  // For now, cycle through sort types.
  if (!model_) {
    return;
  }
  auto current = model_->GetSortType();
  using SortType = AstraReadingListSortType;
  switch (current) {
    case SortType::kNewestFirst:
      model_->SetSortType(SortType::kOldestFirst);
      break;
    case SortType::kOldestFirst:
      model_->SetSortType(SortType::kAlphabetical);
      break;
    case SortType::kAlphabetical:
      model_->SetSortType(SortType::kReadTime);
      break;
    case SortType::kReadTime:
      model_->SetSortType(SortType::kNewestFirst);
      break;
  }
}

void AstraReadingListPageView::OnListViewClicked() {
  SetDisplayMode(AstraReadingListDisplayMode::kList);
}

void AstraReadingListPageView::OnGridViewClicked() {
  SetDisplayMode(AstraReadingListDisplayMode::kGrid);
}

void AstraReadingListPageView::OnEntryClicked(const std::string& id) {
  SetSelectedEntry(id);
}

void AstraReadingListPageView::OnEntryFavorite(const std::string& id) {
  if (model_) {
    model_->ToggleFavorite(id);
  }
}

void AstraReadingListPageView::OnEntryMore(const std::string& id) {
  // TODO(astra): Show context menu with more actions.
  if (model_) {
    model_->MarkAsRead(id);
  }
}

void AstraReadingListPageView::OnFilterSelected(AstraReadingListFilter filter) {
  current_sidebar_filter_ = filter;
  selected_folder_id_.clear();
  if (model_) {
    model_->SetFilter(filter);
    model_->SetFolderFilter("");
  }
  RebuildSidebar();
}

void AstraReadingListPageView::OnFolderSelected(const std::string& folder_id) {
  selected_folder_id_ = folder_id;
  if (model_) {
    // Find folder by id and set folder filter by name.
    for (const auto& folder : model_->GetFolders()) {
      if (folder.id == folder_id) {
        model_->SetFolderFilter(folder.name);
        break;
      }
    }
    current_sidebar_filter_ = AstraReadingListFilter::kAll;
    model_->SetFilter(AstraReadingListFilter::kAll);
  }
  RebuildSidebar();
}

void AstraReadingListPageView::OnDetailMarkRead() {
  if (model_ && !selected_entry_id_.empty()) {
    const auto* entry = model_->GetEntry(selected_entry_id_);
    if (entry && entry->is_read) {
      model_->MarkAsUnread(selected_entry_id_);
    } else {
      model_->MarkAsRead(selected_entry_id_);
    }
  }
}

void AstraReadingListPageView::OnDetailRemove() {
  if (model_ && !selected_entry_id_.empty()) {
    std::string id = selected_entry_id_;
    selected_entry_id_.clear();
    model_->RemoveEntry(id);
  }
}

void AstraReadingListPageView::OnDetailShare() {
  // TODO(astra): Show share dialog.
}

}  // namespace astra
