// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/bookmarks_manager/astra_bookmarks_manager_view.h"

#include <memory>
#include <string>

#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkColor.h"
#include "skia/core/SkPath.h"
#include "ui/base/l10n/l10n_util.h"
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
#include "ui/views/widget/widget.h"

namespace astra {

namespace {

// Helper to draw a folder icon programmatically.
void DrawFolderIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int w = std::min(bounds.width(), bounds.height()) * 0.8f;
  int h = w * 0.7f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);

  SkPath path;
  // Tab part.
  path.moveTo(cx - w / 2, cy - h / 2 + h * 0.3f);
  path.lineTo(cx - w / 2 + w * 0.35f, cy - h / 2);
  path.lineTo(cx - w / 2 + w * 0.55f, cy - h / 2);
  path.lineTo(cx - w / 2 + w * 0.55f + 2, cy - h / 2 + h * 0.3f);
  // Body.
  path.lineTo(cx + w / 2, cy - h / 2 + h * 0.3f);
  path.lineTo(cx + w / 2, cy + h / 2);
  path.lineTo(cx - w / 2, cy + h / 2);
  path.close();

  canvas->DrawPath(path, flags);
}

// Helper to draw a bookmark star icon.
void DrawBookmarkStar(gfx::Canvas* canvas,
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
  // Five-pointed star.
  for (int i = 0; i < 5; i++) {
    float angle = -M_PI_2 + i * 2 * M_PI / 5;
    float x = cx + size * cos(angle);
    float y = cy + size * sin(angle);
    if (i == 0) {
      path.moveTo(x, y);
    } else {
      path.lineTo(x, y);
    }
    // Inner point.
    float inner_angle = angle + M_PI / 5;
    float inner_size = size * 0.4f;
    float ix = cx + inner_size * cos(inner_angle);
    float iy = cy + inner_size * sin(inner_angle);
    path.lineTo(ix, iy);
  }
  path.close();
  canvas->DrawPath(path, flags);
}

// Helper to draw a search icon (magnifying glass).
void DrawSearchIcon(gfx::Canvas* canvas,
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

  // Circle.
  canvas->DrawCircle(gfx::Point(cx - radius * 0.2f, cy - radius * 0.2f),
                     radius, flags);

  // Handle.
  SkPath path;
  path.moveTo(cx + radius * 0.5f, cy + radius * 0.5f);
  path.lineTo(cx + radius * 1.1f, cy + radius * 1.1f);
  canvas->DrawPath(path, flags);
}

// Helper to draw an add (plus) icon.
void DrawAddIcon(gfx::Canvas* canvas,
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

// Helper to draw a grid icon.
void DrawGridIcon(gfx::Canvas* canvas,
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

  // 2x2 grid of squares.
  canvas->DrawRect(gfx::Rect(cx - size / 2, cy - size / 2, cell, cell), flags);
  canvas->DrawRect(gfx::Rect(cx + 2, cy - size / 2, cell, cell), flags);
  canvas->DrawRect(gfx::Rect(cx - size / 2, cy + 2, cell, cell), flags);
  canvas->DrawRect(gfx::Rect(cx + 2, cy + 2, cell, cell), flags);
}

// Helper to draw a list icon (three horizontal lines).
void DrawListIcon(gfx::Canvas* canvas,
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

// Helper to draw a sort icon.
void DrawSortIcon(gfx::Canvas* canvas,
                  const gfx::Rect& bounds,
                  SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int w = std::min(bounds.width(), bounds.height()) * 0.5f;
  int h = w * 0.7f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);

  // Three horizontal lines of decreasing length.
  flags.setStrokeWidth(2);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  canvas->DrawLine(gfx::Point(cx - w / 2, cy - h / 2),
                   gfx::Point(cx + w / 2, cy - h / 2), flags);
  canvas->DrawLine(gfx::Point(cx - w / 2, cy),
                   gfx::Point(cx + w / 4, cy), flags);
  canvas->DrawLine(gfx::Point(cx - w / 2, cy + h / 2),
                   gfx::Point(cx, cy + h / 2), flags);

  // Arrow on the right side.
  flags.setStyle(cc::PaintFlags::kFill_Style);
  SkPath arrow;
  arrow.moveTo(cx + w * 0.6f, cy - h * 0.4f);
  arrow.lineTo(cx + w * 0.85f, cy - h * 0.15f);
  arrow.lineTo(cx + w * 0.35f, cy - h * 0.15f);
  arrow.close();
  canvas->DrawPath(arrow, flags);
}

// Helper to draw a "more" (three dots) icon.
void DrawMoreIcon(gfx::Canvas* canvas,
                  const gfx::Rect& bounds,
                  SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float dot_radius = 2.0f;
  float spacing = 5;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kFill_Style);

  canvas->DrawCircle(gfx::Point(cx - spacing, cy), dot_radius, flags);
  canvas->DrawCircle(gfx::Point(cx, cy), dot_radius, flags);
  canvas->DrawCircle(gfx::Point(cx + spacing, cy), dot_radius, flags);
}

// Helper to draw a chevron (expand/collapse arrow).
void DrawChevron(gfx::Canvas* canvas,
                 const gfx::Rect& bounds,
                 SkColor color,
                 bool expanded) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int size = std::min(bounds.width(), bounds.height()) * 0.3f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(2);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  if (expanded) {
    // Chevron pointing down.
    SkPath path;
    path.moveTo(cx - size, cy - size * 0.4f);
    path.lineTo(cx, cy + size * 0.4f);
    path.lineTo(cx + size, cy - size * 0.4f);
    canvas->DrawPath(path, flags);
  } else {
    // Chevron pointing right.
    SkPath path;
    path.moveTo(cx - size * 0.4f, cy - size);
    path.lineTo(cx + size * 0.4f, cy);
    path.lineTo(cx - size * 0.4f, cy + size);
    canvas->DrawPath(path, flags);
  }
}

}  // namespace

// ===========================================================================
// AstraBookmarkItemView
// ===========================================================================

BEGIN_METADATA(AstraBookmarkItemView)
END_METADATA

AstraBookmarkItemView::AstraBookmarkItemView(const AstraBookmarkEntry& entry)
    : entry_(entry) {
  Build();
}

AstraBookmarkItemView::~AstraBookmarkItemView() = default;

void AstraBookmarkItemView::Update(const AstraBookmarkEntry& entry) {
  entry_ = entry;
  if (title_label_) {
    title_label_->SetText(entry.title);
  }
  if (url_label_) {
    url_label_->SetText(base::UTF8ToUTF16(entry.url));
  }
  SchedulePaint();
}

void AstraBookmarkItemView::Build() {
  // Title label.
  auto title_label = std::make_unique<views::Label>(entry_.title);
  title_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label->SetAutoColorReadabilityEnabled(false);
  title_label->SetEnabledColor(SK_ColorBLACK);
  title_label_ = AddChildView(std::move(title_label));

  // URL label.
  auto url_label =
      std::make_unique<views::Label>(base::UTF8ToUTF16(entry_.url));
  url_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  url_label->SetAutoColorReadabilityEnabled(false);
  url_label->SetEnabledColor(SkColorSetA(SK_ColorBLACK, 0x80));
  url_label_ = AddChildView(std::move(url_label));

  // More button.
  auto more_button = std::make_unique<views::ImageButton>();
  more_button->SetTooltipText(u"More actions");
  more_button->SetAccessibleName(u"More bookmark actions");
  more_button->SetMinSize(gfx::Size(kButtonSize, kButtonSize));
  more_button_ = AddChildView(std::move(more_button));
}

void AstraBookmarkItemView::SetDisplayMode(AstraBookmarksDisplayMode mode) {
  if (display_mode_ == mode) {
    return;
  }
  display_mode_ = mode;
  InvalidateLayout();
  SchedulePaint();
}

void AstraBookmarkItemView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  gfx::Rect bounds = GetContentsBounds();

  // Background card.
  cc::PaintFlags bg_flags;
  bg_flags.setColor(SK_ColorWHITE);
  bg_flags.setAntiAlias(true);
  bg_flags.setStyle(cc::PaintFlags::kFill_Style);
  canvas->DrawRoundRect(gfx::RectF(bounds), 8, bg_flags);

  // Border.
  bg_flags.setColor(SkColorSetA(SK_ColorBLACK, 0x0F));
  bg_flags.setStyle(cc::PaintFlags::kStroke_Style);
  bg_flags.setStrokeWidth(1);
  canvas->DrawRoundRect(gfx::RectF(bounds), 8, bg_flags);

  SkColor icon_color = SK_ColorBLACK;
  // TODO(astra): Use theme colors.

  if (display_mode_ == AstraBookmarksDisplayMode::kGrid) {
    // Favicon area at top.
    gfx::Rect favicon_bounds(bounds.x() + 16, bounds.y() + 16, 24, 24);
    DrawBookmarkStar(canvas, favicon_bounds, icon_color);
  } else {
    // Favicon on the left.
    gfx::Rect favicon_bounds(bounds.x() + 12, bounds.y() + 10, 20, 20);
    DrawBookmarkStar(canvas, favicon_bounds, icon_color);
  }

  // More button icon.
  if (more_button_ && more_button_->GetVisible()) {
    DrawMoreIcon(canvas, more_button_->GetContentsBounds(),
                 SkColorSetA(SK_ColorBLACK, 0x60));
  }
}

gfx::Size AstraBookmarkItemView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  if (display_mode_ == AstraBookmarksDisplayMode::kGrid) {
    return gfx::Size(kBookmarkCardWidth, kBookmarkCardHeight);
  }
  // List mode: full width, fixed height.
  int width = available_size.width().value_or(kBookmarkCardWidth * 3);
  return gfx::Size(width, 44);
}

void AstraBookmarkItemView::Layout() {
  gfx::Rect bounds = GetContentsBounds();

  if (display_mode_ == AstraBookmarksDisplayMode::kGrid) {
    // Grid layout: icon on top, title + url below, more button top-right.
    int icon_size = 24;
    int icon_y = bounds.y() + 16;
    int icon_x = bounds.x() + 16;
    (void)icon_x;
    (void)icon_y;
    (void)icon_size;

    // More button in top right.
    if (more_button_) {
      more_button_->SetBounds(bounds.right() - 32, bounds.y() + 8, 24, 24);
    }

    // Title below the icon.
    int text_x = bounds.x() + 16;
    int text_width = bounds.width() - 32;
    int title_y = bounds.y() + 48;
    if (title_label_) {
      title_label_->SetBounds(text_x, title_y, text_width, 20);
    }

    // URL below title.
    if (url_label_) {
      url_label_->SetBounds(text_x, title_y + 22, text_width, 18);
    }
  } else {
    // List layout: icon on left, title + url in middle, more button on right.
    int icon_end = bounds.x() + 12 + 20 + 12;
    int right_start = bounds.right() - 32;

    if (more_button_) {
      more_button_->SetBounds(bounds.right() - 32, bounds.y() + 10, 24, 24);
    }

    int text_width = right_start - icon_end - 8;
    int text_y = bounds.y() + 6;

    if (title_label_) {
      title_label_->SetBounds(icon_end, text_y, text_width, 18);
    }
    if (url_label_) {
      url_label_->SetBounds(icon_end, text_y + 20, text_width, 16);
    }
  }
}

void AstraBookmarkItemView::OnThemeChanged() {
  views::View::OnThemeChanged();
  SchedulePaint();
}

void AstraBookmarkItemView::DrawFavicon(gfx::Canvas* canvas,
                                        const gfx::Rect& bounds) {
  DrawBookmarkStar(canvas, bounds, SK_ColorBLACK);
}

void AstraBookmarkItemView::DrawMoreIcon(gfx::Canvas* canvas,
                                         const gfx::Rect& bounds,
                                         SkColor color) {
  astra::DrawMoreIcon(canvas, bounds, color);
}

// ===========================================================================
// AstraBookmarkFolderItemView
// ===========================================================================

BEGIN_METADATA(AstraBookmarkFolderItemView)
END_METADATA

AstraBookmarkFolderItemView::AstraBookmarkFolderItemView(
    const AstraBookmarkFolder& folder,
    int depth)
    : folder_id_(folder.id),
      title_(folder.title),
      total_bookmarks_(folder.total_bookmarks),
      depth_(depth) {
  Build();
}

AstraBookmarkFolderItemView::~AstraBookmarkFolderItemView() = default;

void AstraBookmarkFolderItemView::Update(
    const AstraBookmarkFolder& folder) {
  title_ = folder.title;
  total_bookmarks_ = folder.total_bookmarks;
  if (title_label_) {
    title_label_->SetText(title_);
  }
  if (count_label_) {
    count_label_->SetText(
        base::UTF8ToUTF16(std::to_string(total_bookmarks_)));
  }
  SchedulePaint();
}

void AstraBookmarkFolderItemView::SetExpanded(bool expanded) {
  if (expanded_ == expanded) {
    return;
  }
  expanded_ = expanded;
  SchedulePaint();
}

void AstraBookmarkFolderItemView::Build() {
  // Expand/collapse button.
  auto expand_button = std::make_unique<views::ImageButton>();
  expand_button->SetTooltipText(u"Toggle folder");
  expand_button->SetAccessibleName(u"Expand or collapse folder");
  expand_button->SetMinSize(gfx::Size(20, 20));
  expand_button->SetCallback(base::BindRepeating(
      &AstraBookmarkFolderItemView::OnExpandClicked,
      base::Unretained(this)));
  expand_button_ = AddChildView(std::move(expand_button));

  // Title label.
  auto title_label = std::make_unique<views::Label>(title_);
  title_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label->SetAutoColorReadabilityEnabled(false);
  title_label->SetEnabledColor(SK_ColorBLACK);
  title_label_ = AddChildView(std::move(title_label));

  // Count label.
  auto count_label = std::make_unique<views::Label>(
      base::UTF8ToUTF16(std::to_string(total_bookmarks_)));
  count_label->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  count_label->SetAutoColorReadabilityEnabled(false);
  count_label->SetEnabledColor(SkColorSetA(SK_ColorBLACK, 0x50));
  count_label_ = AddChildView(std::move(count_label));
}

void AstraBookmarkFolderItemView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  gfx::Rect bounds = GetContentsBounds();

  // Selected background.
  if (selected_) {
    cc::PaintFlags bg_flags;
    bg_flags.setColor(SkColorSetRGB(0xE8, 0xF0, 0xFE));
    bg_flags.setAntiAlias(true);
    bg_flags.setStyle(cc::PaintFlags::kFill_Style);
    canvas->DrawRoundRect(gfx::RectF(bounds.x() + 4, bounds.y(),
                                     bounds.width() - 8, bounds.height()),
                          6, bg_flags);
  }

  SkColor icon_color = SK_ColorBLACK;
  // TODO(astra): Use theme colors.

  // Folder icon.
  int icon_x = bounds.x() + 8 + depth_ * 16 + 20;
  gfx::Rect icon_bounds(icon_x, bounds.y() + (bounds.height() - 20) / 2,
                        20, 20);
  DrawFolderIcon(canvas, icon_bounds, SkColorSetRGB(0x42, 0x85, 0xF4));

  // Expand/collapse chevron.
  if (expand_button_ && expand_button_->GetVisible()) {
    int chevron_x = bounds.x() + 8 + depth_ * 16;
    gfx::Rect chevron_bounds(chevron_x,
                             bounds.y() + (bounds.height() - 16) / 2,
                             16, 16);
    DrawChevron(canvas, chevron_bounds,
                SkColorSetA(SK_ColorBLACK, 0x60), expanded_);
  }
}

gfx::Size AstraBookmarkFolderItemView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  int width = available_size.width().value_or(240);
  return gfx::Size(width, 32);
}

void AstraBookmarkFolderItemView::Layout() {
  gfx::Rect bounds = GetContentsBounds();
  int h = bounds.height();

  // Expand button.
  int expand_x = bounds.x() + 8 + depth_ * 16;
  if (expand_button_) {
    expand_button_->SetBounds(expand_x, (h - 20) / 2, 16, 20);
  }

  // Folder icon is painted, not a child view.
  int icon_end = bounds.x() + 8 + depth_ * 16 + 16 + 8 + 20 + 8;

  // Count label on the right.
  if (count_label_) {
    int count_w = 40;
    count_label_->SetBounds(bounds.right() - count_w - 8, (h - 20) / 2,
                            count_w, 20);
  }

  // Title fills the remaining space.
  int title_x = icon_end;
  int title_w = bounds.right() - 48 - title_x;
  if (title_label_) {
    title_label_->SetBounds(title_x, (h - 20) / 2, title_w, 20);
  }
}

bool AstraBookmarkFolderItemView::OnMousePressed(const ui::MouseEvent& event) {
  if (select_callback_) {
    select_callback_.Run(folder_id_);
  }
  return true;
}

void AstraBookmarkFolderItemView::OnThemeChanged() {
  views::View::OnThemeChanged();
  SchedulePaint();
}

void AstraBookmarkFolderItemView::OnExpandClicked() {
  if (expand_callback_) {
    expand_callback_.Run(folder_id_);
  }
}

void AstraBookmarkFolderItemView::DrawFolderIcon(gfx::Canvas* canvas,
                                                 const gfx::Rect& bounds,
                                                 SkColor color) {
  astra::DrawFolderIcon(canvas, bounds, color);
}

void AstraBookmarkFolderItemView::DrawChevron(gfx::Canvas* canvas,
                                              const gfx::Rect& bounds,
                                              SkColor color,
                                              bool expanded) {
  astra::DrawChevron(canvas, bounds, color, expanded);
}

// ===========================================================================
// AstraBookmarksManagerView
// ===========================================================================

BEGIN_METADATA(AstraBookmarksManagerView)
END_METADATA

AstraBookmarksManagerView::AstraBookmarksManagerView() {
  Build();
}

AstraBookmarksManagerView::AstraBookmarksManagerView(
    AstraBookmarksManagerModel* model)
    : model_(model) {
  if (model_) {
    model_observation_.Observe(model_);
  }
  Build();
  RebuildFolderTree();
  RebuildBookmarkContent();
  UpdateStatusBar();
}

AstraBookmarksManagerView::~AstraBookmarksManagerView() = default;

void AstraBookmarksManagerView::SetModel(AstraBookmarksManagerModel* model) {
  if (model_ == model) {
    return;
  }
  if (model_observation_.IsObserving()) {
    model_observation_.Reset();
  }
  model_ = model;
  if (model_) {
    model_observation_.Observe(model_);
  }
  RebuildFolderTree();
  RebuildBookmarkContent();
  UpdateStatusBar();
}

void AstraBookmarksManagerView::SetDisplayMode(AstraBookmarksDisplayMode mode) {
  if (display_mode_ == mode) {
    return;
  }
  display_mode_ = mode;
  for (auto* item : bookmark_items_) {
    item->SetDisplayMode(mode);
  }
  if (content_container_) {
    content_container_->InvalidateLayout();
  }
  SchedulePaint();
}

void AstraBookmarksManagerView::SetSelectedFolder(AstraBookmarkId folder_id) {
  if (selected_folder_id_ == folder_id) {
    return;
  }
  selected_folder_id_ = folder_id;
  RebuildBookmarkContent();
  UpdateStatusBar();
}

// -- AstraBookmarksManagerObserver: ----------------------------------------

void AstraBookmarksManagerView::OnBookmarksModelChanged() {
  RebuildFolderTree();
  RebuildBookmarkContent();
  UpdateStatusBar();
}

void AstraBookmarksManagerView::OnBookmarkAdded(AstraBookmarkId id) {
  RebuildBookmarkContent();
  UpdateStatusBar();
}

void AstraBookmarksManagerView::OnBookmarkRemoved(AstraBookmarkId id) {
  RebuildFolderTree();
  RebuildBookmarkContent();
  UpdateStatusBar();
}

void AstraBookmarksManagerView::OnBookmarkChanged(AstraBookmarkId id) {
  RebuildBookmarkContent();
}

void AstraBookmarksManagerView::OnFolderExpanded(AstraBookmarkId folder_id) {
  RebuildFolderTree();
}

void AstraBookmarksManagerView::OnSearchChanged(const std::u16string& query) {
  RebuildBookmarkContent();
  UpdateStatusBar();
}

void AstraBookmarksManagerView::OnBookmarksManagerModelShutdown() {
  model_ = nullptr;
  model_observation_.Reset();
}

// -- views::View: ----------------------------------------------------------

void AstraBookmarksManagerView::Layout() {
  gfx::Rect bounds = GetContentsBounds();

  // Toolbar at top.
  if (toolbar_) {
    toolbar_->SetBounds(bounds.x(), bounds.y(), bounds.width(),
                        kToolbarHeight);
  }

  // Status bar at bottom.
  if (status_bar_) {
    status_bar_->SetBounds(bounds.x(), bounds.bottom() - kStatusBarHeight,
                           bounds.width(), kStatusBarHeight);
  }

  // Main content area between toolbar and status bar.
  int content_y = bounds.y() + kToolbarHeight;
  int content_h = bounds.height() - kToolbarHeight - kStatusBarHeight;

  // Sidebar on the left.
  if (sidebar_container_) {
    sidebar_container_->SetBounds(bounds.x(), content_y, kSidebarWidth,
                                  content_h);
  }

  // Main content on the right.
  if (content_scroll_) {
    content_scroll_->SetBounds(bounds.x() + kSidebarWidth, content_y,
                               bounds.width() - kSidebarWidth, content_h);
  }
}

gfx::Size AstraBookmarksManagerView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return gfx::Size(800, 600);
}

void AstraBookmarksManagerView::OnThemeChanged() {
  views::View::OnThemeChanged();
  SchedulePaint();
}

// -- TextfieldController: --------------------------------------------------

void AstraBookmarksManagerView::ContentsChanged(views::Textfield* sender,
                                                const std::u16string& new_contents) {
  if (model_) {
    model_->SetSearchQuery(new_contents);
  }
}

// -- Build helpers ----------------------------------------------------------

void AstraBookmarksManagerView::Build() {
  SetLayoutManager(std::make_unique<views::FillLayout>());

  BuildToolbar();
  BuildSidebar();
  BuildContent();
  BuildStatusBar();
}

void AstraBookmarksManagerView::BuildToolbar() {
  auto toolbar = std::make_unique<views::View>();
  toolbar->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SkColorSetA(SK_ColorBLACK, 0x0F)));
  toolbar_ = AddChildView(std::move(toolbar));

  // Search field.
  auto search_field = std::make_unique<views::Textfield>();
  search_field->SetPlaceholderText(u"Search bookmarks");
  search_field->SetAccessibleName(u"Search bookmarks");
  search_field->set_controller(this);
  search_field_ = toolbar->AddChildView(std::move(search_field));

  // Add bookmark button.
  auto add_button = std::make_unique<views::ImageButton>();
  add_button->SetTooltipText(u"Add bookmark");
  add_button->SetAccessibleName(u"Add new bookmark");
  add_button->SetMinSize(gfx::Size(kButtonSize, kButtonSize));
  add_button->SetCallback(base::BindRepeating(
      &AstraBookmarksManagerView::OnAddBookmarkClicked,
      base::Unretained(this)));
  add_button_ = toolbar->AddChildView(std::move(add_button));

  // Sort button.
  auto sort_button = std::make_unique<views::ImageButton>();
  sort_button->SetTooltipText(u"Sort");
  sort_button->SetAccessibleName(u"Sort bookmarks");
  sort_button->SetMinSize(gfx::Size(kButtonSize, kButtonSize));
  sort_button->SetCallback(base::BindRepeating(
      &AstraBookmarksManagerView::OnSortClicked, base::Unretained(this)));
  sort_button_ = toolbar->AddChildView(std::move(sort_button));

  // Grid view toggle button.
  auto grid_button = std::make_unique<views::ImageButton>();
  grid_button->SetTooltipText(u"Grid view");
  grid_button->SetAccessibleName(u"Switch to grid view");
  grid_button->SetMinSize(gfx::Size(kButtonSize, kButtonSize));
  grid_button->SetCallback(base::BindRepeating(
      &AstraBookmarksManagerView::OnGridViewClicked, base::Unretained(this)));
  grid_view_button_ = toolbar->AddChildView(std::move(grid_button));

  // List view toggle button.
  auto list_button = std::make_unique<views::ImageButton>();
  list_button->SetTooltipText(u"List view");
  list_button->SetAccessibleName(u"Switch to list view");
  list_button->SetMinSize(gfx::Size(kButtonSize, kButtonSize));
  list_button->SetCallback(base::BindRepeating(
      &AstraBookmarksManagerView::OnListViewClicked, base::Unretained(this)));
  list_view_button_ = toolbar->AddChildView(std::move(list_button));
}

void AstraBookmarksManagerView::BuildSidebar() {
  auto sidebar = std::make_unique<views::View>();
  sidebar->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 0, 1, SkColorSetA(SK_ColorBLACK, 0x0F)));
  sidebar_container_ = AddChildView(std::move(sidebar));

  // Sidebar scroll view.
  auto sidebar_scroll = std::make_unique<views::ScrollView>();
  sidebar_scroll->SetBackgroundColor(SK_ColorWHITE);
  sidebar_scroll_ = sidebar;

  // Folder tree container.
  auto folder_tree = std::make_unique<views::View>();
  folder_tree_ = folder_tree.get();

  sidebar_scroll->SetContents(std::move(folder_tree));
  sidebar->AddChildView(std::move(sidebar_scroll));
}

void AstraBookmarksManagerView::BuildContent() {
  auto content_scroll = std::make_unique<views::ScrollView>();
  content_scroll->SetBackgroundColor(SK_ColorWHITE);
  content_scroll_ = AddChildView(std::move(content_scroll));

  // Content container.
  auto content_container = std::make_unique<views::View>();
  content_container_ = content_container.get();

  content_scroll_->SetContents(std::move(content_container));

  // Empty state view (hidden by default).
  auto empty_state = std::make_unique<views::View>();
  empty_state->SetVisible(false);
  empty_state_view_ = content_container_->AddChildView(std::move(empty_state));
}

void AstraBookmarksManagerView::BuildStatusBar() {
  auto status_bar = std::make_unique<views::View>();
  status_bar->SetBorder(views::CreateSolidSidedBorder(
      1, 0, 0, 0, SkColorSetA(SK_ColorBLACK, 0x0F)));
  status_bar_ = AddChildView(std::move(status_bar));

  auto status_label = std::make_unique<views::Label>(u"0 bookmarks");
  status_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  status_label->SetAutoColorReadabilityEnabled(false);
  status_label->SetEnabledColor(SkColorSetA(SK_ColorBLACK, 0x60));
  status_label_ = status_bar->AddChildView(std::move(status_label));
}

// -- Tree and content rebuilding --------------------------------------------

void AstraBookmarksManagerView::RebuildFolderTree() {
  if (!folder_tree_ || !model_) {
    return;
  }

  // Clear existing items.
  folder_tree_->RemoveAllChildViews();
  folder_items_.clear();

  // Add the three top-level folders and their children recursively.
  const auto& root = model_->GetRootFolder();
  AddFolderItemsRecursive(root, 0, folder_tree_);

  folder_tree_->InvalidateLayout();
}

void AstraBookmarksManagerView::AddFolderItemsRecursive(
    const AstraBookmarkFolder& folder,
    int depth,
    views::View* container) {
  // Skip the root folder itself (depth 0), only add its children.
  if (depth > 0 || folder.id == 0) {
    // Actually add the folder item if it's not the root.
    if (folder.id != 0) {
      auto folder_item =
          std::make_unique<AstraBookmarkFolderItemView>(folder, depth - 1);
      folder_item->SetExpanded(model_->IsFolderExpanded(folder.id));
      folder_item->SetExpandCallback(base::BindRepeating(
          &AstraBookmarksManagerView::OnFolderExpand, base::Unretained(this)));
      folder_item->SetSelectCallback(base::BindRepeating(
          &AstraBookmarksManagerView::OnFolderSelect, base::Unretained(this)));
      folder_items_.push_back(
          container->AddChildView(std::move(folder_item)));
    }
  }

  // Only recurse if folder is expanded (or it's the root).
  if (folder.id == 0 || model_->IsFolderExpanded(folder.id)) {
    for (const auto& subfolder : folder.subfolders) {
      AddFolderItemsRecursive(subfolder, depth + 1, container);
    }
  }
}

void AstraBookmarksManagerView::RebuildBookmarkContent() {
  if (!content_container_ || !model_) {
    return;
  }

  // Clear existing items but keep the empty state view.
  content_container_->RemoveAllChildViews();
  bookmark_items_.clear();

  auto bookmarks = GetDisplayedBookmarks();

  // Show empty state if no bookmarks.
  if (bookmarks.empty()) {
    if (empty_state_view_) {
      empty_state_view_->SetVisible(true);
    }
    content_container_->InvalidateLayout();
    return;
  }

  if (empty_state_view_) {
    empty_state_view_->SetVisible(false);
  }

  // Create bookmark item views.
  for (const auto& entry : bookmarks) {
    auto item = std::make_unique<AstraBookmarkItemView>(entry);
    item->SetDisplayMode(display_mode_);
    bookmark_items_.push_back(
        content_container_->AddChildView(std::move(item)));
  }

  content_container_->InvalidateLayout();
  content_container_->SchedulePaint();
}

void AstraBookmarksManagerView::UpdateStatusBar() {
  if (!status_label_ || !model_) {
    return;
  }

  auto bookmarks = GetDisplayedBookmarks();
  std::u16string text;
  if (!model_->GetSearchQuery().empty()) {
    text = base::UTF8ToUTF16(
        std::to_string(bookmarks.size()) + " search results");
  } else {
    text = base::UTF8ToUTF16(
        std::to_string(bookmarks.size()) + " bookmarks");
  }
  status_label_->SetText(text);
}

const AstraBookmarkFolder* AstraBookmarksManagerView::GetDisplayedFolder()
    const {
  if (!model_) {
    return nullptr;
  }
  if (selected_folder_id_ == 0) {
    return &model_->GetRootFolder();
  }
  return model_->GetFolder(selected_folder_id_);
}

std::vector<AstraBookmarkEntry>
AstraBookmarksManagerView::GetDisplayedBookmarks() const {
  if (!model_) {
    return {};
  }

  // If there's a search query, return search results.
  if (!model_->GetSearchQuery().empty()) {
    return model_->SearchBookmarks(model_->GetSearchQuery());
  }

  // Otherwise, return bookmarks from the selected folder.
  const AstraBookmarkFolder* folder = GetDisplayedFolder();
  if (!folder) {
    return {};
  }

  // If showing root, combine children from all top-level folders
  // (bar + other + mobile) for a flat view.
  if (folder->id == 0) {
    // For root view: show combined from bar and other folders.
    std::vector<AstraBookmarkEntry> all;
    for (const auto& subfolder : folder->subfolders) {
      for (const auto& entry : subfolder.children) {
        // Apply category filter.
        if (model_->GetCategoryFilter().empty() ||
            entry.category == model_->GetCategoryFilter()) {
          all.push_back(entry);
        }
      }
    }
    return all;
  }

  // Apply category filter.
  std::vector<AstraBookmarkEntry> filtered;
  for (const auto& entry : folder->children) {
    if (model_->GetCategoryFilter().empty() ||
        entry.category == model_->GetCategoryFilter()) {
      filtered.push_back(entry);
    }
  }
  return filtered;
}

// -- Button handlers --------------------------------------------------------

void AstraBookmarksManagerView::OnAddBookmarkClicked() {
  // TODO(astra): Show add bookmark dialog. For scaffold, add a sample.
  if (!model_) {
    return;
  }

  const AstraBookmarkFolder* target = GetDisplayedFolder();
  if (!target) {
    target = model_->GetBookmarksBarFolder();
  }
  if (!target) {
    return;
  }

  model_->AddBookmark(target->id, u"New Bookmark",
                      "https://example.com/new-bookmark");
}

void AstraBookmarksManagerView::OnGridViewClicked() {
  SetDisplayMode(AstraBookmarksDisplayMode::kGrid);
}

void AstraBookmarksManagerView::OnListViewClicked() {
  SetDisplayMode(AstraBookmarksDisplayMode::kList);
}

void AstraBookmarksManagerView::OnSortClicked() {
  // TODO(astra): Show sort menu. For scaffold, cycle through sort types.
  if (!model_ || selected_folder_id_ == 0) {
    return;
  }
  model_->SortFolder(selected_folder_id_, AstraBookmarkSortType::kName);
}

void AstraBookmarksManagerView::OnFolderExpand(AstraBookmarkId folder_id) {
  if (!model_) {
    return;
  }
  bool expanded = model_->IsFolderExpanded(folder_id);
  model_->SetFolderExpanded(folder_id, !expanded);
}

void AstraBookmarksManagerView::OnFolderSelect(AstraBookmarkId folder_id) {
  SetSelectedFolder(folder_id);
}

// -- Icon drawing -----------------------------------------------------------

void AstraBookmarksManagerView::DrawSearchIcon(gfx::Canvas* canvas,
                                               const gfx::Rect& bounds,
                                               SkColor color) {
  astra::DrawSearchIcon(canvas, bounds, color);
}

void AstraBookmarksManagerView::DrawAddIcon(gfx::Canvas* canvas,
                                            const gfx::Rect& bounds,
                                            SkColor color) {
  astra::DrawAddIcon(canvas, bounds, color);
}

void AstraBookmarksManagerView::DrawGridIcon(gfx::Canvas* canvas,
                                             const gfx::Rect& bounds,
                                             SkColor color) {
  astra::DrawGridIcon(canvas, bounds, color);
}

void AstraBookmarksManagerView::DrawListIcon(gfx::Canvas* canvas,
                                             const gfx::Rect& bounds,
                                             SkColor color) {
  astra::DrawListIcon(canvas, bounds, color);
}

void AstraBookmarksManagerView::DrawSortIcon(gfx::Canvas* canvas,
                                             const gfx::Rect& bounds,
                                             SkColor color) {
  astra::DrawSortIcon(canvas, bounds, color);
}

}  // namespace astra
