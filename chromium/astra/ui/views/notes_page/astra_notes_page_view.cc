// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/notes_page/astra_notes_page_view.h"

#include <memory>
#include <string>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
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

// Color map for note colors.
constexpr struct {
  const char* name;
  SkColor color;
} kNoteColors[] = {
    {"default", SK_ColorGRAY},
    {"yellow", 0xFFFFE082},
    {"green", 0xFFA5D6A7},
    {"blue", 0xFF90CAF9},
    {"pink", 0xFFF48FB1},
    {"purple", 0xFFCE93D8},
};

SkColor GetNoteColor(const std::string& color_name) {
  for (const auto& c : kNoteColors) {
    if (color_name == c.name) {
      return c.color;
    }
  }
  return SK_ColorGRAY;
}

// Helper to create a custom-painted icon as an ImageSkia.
gfx::ImageSkia CreateIconImage(int size,
                                base::RepeatingCallback<void(gfx::Canvas*,
                                                             const gfx::Rect&,
                                                             SkColor)> draw_fn,
                                SkColor color) {
  gfx::Canvas canvas(gfx::Size(size, size), /*image_scale=*/1.0f,
                     /*is_opaque=*/false);
  draw_fn.Run(&canvas, gfx::Rect(size, size), color);
  return gfx::ImageSkia::CreateFromBitmap(canvas.GetBitmap(), 1.0f);
}

// Draw generic helper for a rounded rectangle background.
void DrawRoundedRect(gfx::Canvas* canvas,
                     const gfx::Rect& bounds,
                     int radius,
                     SkColor color,
                     bool fill = true) {
  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(fill ? cc::PaintFlags::kFill_Style
                      : cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1);
  flags.setAntiAlias(true);
  canvas->DrawRoundRect(bounds, radius, flags);
}

}  // namespace

// ===========================================================================
// AstraNoteCardView
// ===========================================================================

BEGIN_METADATA(AstraNoteCardView)
END_METADATA

AstraNoteCardView::AstraNoteCardView(const AstraNoteEntry& entry)
    : entry_(entry) {
  Build();
}

AstraNoteCardView::~AstraNoteCardView() = default;

void AstraNoteCardView::Update(const AstraNoteEntry& entry) {
  entry_ = entry;
  if (title_label_) {
    title_label_->SetText(entry_.title);
  }
  if (preview_label_) {
    preview_label_->SetText(entry_.content.substr(0, 100));
  }
  if (date_label_) {
    // TODO(astra): Use proper date formatting.
    date_label_->SetText(base::UTF8ToUTF16(
        base::NumberToString(
            (base::Time::Now() - entry_.date_modified).InDays()) +
        "d ago"));
  }
  std::u16string tags_str;
  for (size_t i = 0; i < entry_.tags.size(); ++i) {
    if (i > 0) tags_str += u", ";
    tags_str += base::UTF8ToUTF16(entry_.tags[i]);
  }
  if (tags_label_) {
    tags_label_->SetText(tags_str);
  }
  SchedulePaint();
}

void AstraNoteCardView::SetDisplayMode(AstraNotesDisplayMode mode) {
  if (display_mode_ == mode) {
    return;
  }
  display_mode_ = mode;
  InvalidateLayout();
}

void AstraNoteCardView::SetSelected(bool selected) {
  if (selected_ == selected) {
    return;
  }
  selected_ = selected;
  SchedulePaint();
}

void AstraNoteCardView::Build() {
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  layout->set_main_axis_alignment(views::BoxLayout::MainAxisAlignment::kStart);
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);
  layout->set_between_child_spacing(4);
  SetBorder(views::CreateEmptyBorder(12, 12, 12, 12));

  title_label_ = AddChildView(std::make_unique<views::Label>(entry_.title));
  title_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label_->SetFontList(views::style::GetFont(
      views::style::CONTEXT_LABEL, views::style::STYLE_EMPHASIZED));
  title_label_->SetMultiLine(true);
  title_label_->SetMaximumWidth(kNoteCardWidth - 24);
  title_label_->SetElideBehavior(gfx::ELIDE_TAIL);

  preview_label_ = AddChildView(std::make_unique<views::Label>(
      entry_.content.substr(0, 100)));
  preview_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  preview_label_->SetFontList(views::style::GetFont(
      views::style::CONTEXT_LABEL, views::style::STYLE_SECONDARY));
  preview_label_->SetMultiLine(true);
  preview_label_->SetMaximumWidth(kNoteCardWidth - 24);
  preview_label_->SetElideBehavior(gfx::ELIDE_TAIL);
  preview_label_->SetAutoColorReadabilityEnabled(false);

  std::u16string tags_str;
  for (size_t i = 0; i < entry_.tags.size() && i < 3; ++i) {
    if (i > 0) tags_str += u", ";
    tags_str += base::UTF8ToUTF16(entry_.tags[i]);
  }
  tags_label_ = AddChildView(std::make_unique<views::Label>(tags_str));
  tags_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  tags_label_->SetFontList(views::style::GetFont(
      views::style::CONTEXT_LABEL, views::style::STYLE_SECONDARY));
  tags_label_->SetElideBehavior(gfx::ELIDE_TAIL);

  date_label_ = AddChildView(std::make_unique<views::Label>(
      base::UTF8ToUTF16("Modified " +
                        base::NumberToString(
                            (base::Time::Now() - entry_.date_modified).InDays()) +
                        "d ago")));
  date_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  date_label_->SetFontList(views::style::GetFont(
      views::style::CONTEXT_LABEL, views::style::STYLE_DISABLED));

  SetBackground(nullptr);
}

void AstraNoteCardView::DrawColorStripe(gfx::Canvas* canvas,
                                        const gfx::Rect& bounds,
                                        const std::string& color) {
  cc::PaintFlags flags;
  flags.setColor(GetNoteColor(color));
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);

  gfx::Rect stripe(bounds.x(), bounds.y(), 4, bounds.height());
  canvas->DrawRoundRect(stripe, 2, flags);
}

void AstraNoteCardView::DrawPinIcon(gfx::Canvas* canvas,
                                    const gfx::Rect& bounds,
                                    SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float size = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);

  SkPath path;
  // Pin head.
  path.moveTo(cx - size * 0.3f, cy - size);
  path.lineTo(cx + size * 0.3f, cy - size);
  path.lineTo(cx + size * 0.7f, cy - size * 0.3f);
  path.lineTo(cx + size * 0.3f, cy + size * 0.3f);
  path.lineTo(cx, cy + size);
  path.lineTo(cx - size * 0.3f, cy + size * 0.3f);
  path.lineTo(cx - size * 0.7f, cy - size * 0.3f);
  path.close();
  canvas->DrawPath(path, flags);
}

SkColor AstraNoteCardView::GetColorForName(
    const std::string& color_name) const {
  return GetNoteColor(color_name);
}

void AstraNoteCardView::OnPaint(gfx::Canvas* canvas) {
  gfx::Rect bounds = GetLocalBounds();

  // Background.
  SkColor bg_color = selected_
      ? GetColorProvider()->GetColor(ui::kColorBubbleBackground)
      : GetColorProvider()->GetColor(ui::kColorDialogBackground);
  cc::PaintFlags bg_flags;
  bg_flags.setColor(bg_color);
  bg_flags.setStyle(cc::PaintFlags::kFill_Style);
  bg_flags.setAntiAlias(true);
  canvas->DrawRoundRect(bounds, 8, bg_flags);

  // Color stripe on left.
  gfx::Rect stripe_bounds = bounds;
  stripe_bounds.set_width(4);
  stripe_bounds.Inset(gfx::Insets::VH(8, 0));
  DrawColorStripe(canvas, stripe_bounds, entry_.color);

  // Border.
  cc::PaintFlags border_flags;
  border_flags.setColor(
      GetColorProvider()->GetColor(ui::kColorSeparator));
  border_flags.setStyle(cc::PaintFlags::kStroke_Style);
  border_flags.setStrokeWidth(1);
  border_flags.setAntiAlias(true);
  canvas->DrawRoundRect(bounds, 8, border_flags);

  // Pin indicator if pinned.
  if (entry_.is_pinned) {
    gfx::Rect pin_bounds(bounds.right() - 24, bounds.y() + 8, 16, 16);
    DrawPinIcon(canvas, pin_bounds, SK_ColorDKGRAY);
  }

  views::View::OnPaint(canvas);
}

gfx::Size AstraNoteCardView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  if (display_mode_ == AstraNotesDisplayMode::kGrid) {
    return gfx::Size(kNoteCardWidth, kNoteCardHeight);
  }
  // List mode: full width, fixed height.
  int width = available_size.width().value_or(kNoteCardWidth * 2);
  return gfx::Size(width, 72);
}

void AstraNoteCardView::Layout() {
  views::View::Layout();
}

bool AstraNoteCardView::OnMousePressed(const ui::MouseEvent& event) {
  if (click_callback_) {
    click_callback_.Run(entry_.id);
  }
  return true;
}

void AstraNoteCardView::OnThemeChanged() {
  views::View::OnThemeChanged();
  // TODO(astra): Update text colors for dark/light theme.
}

// ===========================================================================
// AstraNoteFolderItemView
// ===========================================================================

BEGIN_METADATA(AstraNoteFolderItemView)
END_METADATA

AstraNoteFolderItemView::AstraNoteFolderItemView(SpecialFolder type,
                                                 int count)
    : special_type_(type),
      count_(count),
      depth_(0),
      is_special_(true) {
  switch (type) {
    case SpecialFolder::kAllNotes:
      name_ = u"All Notes";
      color_ = "default";
      break;
    case SpecialFolder::kPinned:
      name_ = u"Pinned";
      color_ = "yellow";
      break;
    case SpecialFolder::kArchive:
      name_ = u"Archive";
      color_ = "default";
      break;
  }
  Build();
}

AstraNoteFolderItemView::AstraNoteFolderItemView(
    const AstraNoteFolder& folder, int depth)
    : special_type_(SpecialFolder::kAllNotes),
      folder_id_(folder.id),
      name_(folder.name),
      color_(folder.color),
      count_(folder.note_count),
      depth_(depth),
      is_special_(false) {
  Build();
}

AstraNoteFolderItemView::~AstraNoteFolderItemView() = default;

void AstraNoteFolderItemView::Update(const AstraNoteFolder& folder) {
  name_ = folder.name;
  color_ = folder.color;
  count_ = folder.note_count;
  if (name_label_) {
    name_label_->SetText(name_);
  }
  if (count_label_) {
    count_label_->SetText(base::UTF8ToUTF16(base::NumberToString(count_)));
  }
  SchedulePaint();
}

void AstraNoteFolderItemView::SetCount(int count) {
  count_ = count;
  if (count_label_) {
    count_label_->SetText(base::UTF8ToUTF16(base::NumberToString(count_)));
  }
}

void AstraNoteFolderItemView::SetSelected(bool selected) {
  if (selected_ == selected) {
    return;
  }
  selected_ = selected;
  SchedulePaint();
}

void AstraNoteFolderItemView::Build() {
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal));
  layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kStart);
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  layout->set_between_child_spacing(8);
  SetBorder(views::CreateEmptyBorder(
      8, 12 + depth_ * 16, 8, 12));

  name_label_ = AddChildView(std::make_unique<views::Label>(name_));
  name_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  name_label_->SetFontList(views::style::GetFont(
      views::style::CONTEXT_LABEL, views::style::STYLE_PRIMARY));
  name_label_->SetProperty(views::kFlexBehaviorKey,
                           views::FlexSpecification::ForSizeRule(
                               views::MinimumFlexSizeRule::kScaleToZero,
                               views::MaximumFlexSizeRule::kUnbounded));

  count_label_ = AddChildView(
      std::make_unique<views::Label>(
          base::UTF8ToUTF16(base::NumberToString(count_))));
  count_label_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  count_label_->SetFontList(views::style::GetFont(
      views::style::CONTEXT_LABEL, views::style::STYLE_SECONDARY));
}

void AstraNoteFolderItemView::DrawFolderIcon(gfx::Canvas* canvas,
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

void AstraNoteFolderItemView::DrawSpecialIcon(gfx::Canvas* canvas,
                                              const gfx::Rect& bounds,
                                              SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float size = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  if (special_type_ == SpecialFolder::kAllNotes) {
    // Notepad icon: rectangle with lines.
    gfx::Rect note_rect(cx - size, cy - size, size * 2, size * 2);
    canvas->DrawRoundRect(note_rect, 2, flags);
    // Horizontal lines inside.
    for (int i = 0; i < 3; i++) {
      int y = cy - size * 0.4f + i * size * 0.4f;
      canvas->DrawLine(gfx::Point(cx - size * 0.6f, y),
                       gfx::Point(cx + size * 0.6f, y), flags);
    }
  } else if (special_type_ == SpecialFolder::kPinned) {
    // Pin icon.
    flags.setStyle(cc::PaintFlags::kFill_Style);
    SkPath path;
    path.moveTo(cx - size * 0.3f, cy - size);
    path.lineTo(cx + size * 0.3f, cy - size);
    path.lineTo(cx + size * 0.7f, cy - size * 0.3f);
    path.lineTo(cx + size * 0.3f, cy + size * 0.3f);
    path.lineTo(cx, cy + size);
    path.lineTo(cx - size * 0.3f, cy + size * 0.3f);
    path.lineTo(cx - size * 0.7f, cy - size * 0.3f);
    path.close();
    canvas->DrawPath(path, flags);
  } else if (special_type_ == SpecialFolder::kArchive) {
    // Archive box icon.
    SkPath path;
    // Box body.
    path.moveTo(cx - size, cy - size * 0.3f);
    path.lineTo(cx - size, cy + size);
    path.lineTo(cx + size, cy + size);
    path.lineTo(cx + size, cy - size * 0.3f);
    path.close();
    canvas->DrawPath(path, flags);
    // Lid.
    canvas->DrawLine(gfx::Point(cx - size, cy - size * 0.3f),
                     gfx::Point(cx + size, cy - size * 0.3f), flags);
    // Top flap.
    canvas->DrawLine(gfx::Point(cx - size * 0.8f, cy - size * 0.7f),
                     gfx::Point(cx + size * 0.8f, cy - size * 0.7f), flags);
  }
}

void AstraNoteFolderItemView::OnPaint(gfx::Canvas* canvas) {
  gfx::Rect bounds = GetLocalBounds();

  // Background when selected.
  if (selected_) {
    cc::PaintFlags flags;
    flags.setColor(
        GetColorProvider()->GetColor(ui::kColorMenuSelectionBackground));
    flags.setStyle(cc::PaintFlags::kFill_Style);
    flags.setAntiAlias(true);
    canvas->DrawRoundRect(bounds, 6, flags);
  }

  // Draw icon at the start.
  gfx::Rect icon_bounds(12 + depth_ * 16, bounds.CenterPoint().y() - 8,
                        16, 16);
  SkColor icon_color = is_special_
      ? GetColorProvider()->GetColor(ui::kColorIcon)
      : GetNoteColor(color_);

  if (is_special_) {
    DrawSpecialIcon(canvas, icon_bounds, icon_color);
  } else {
    DrawFolderIcon(canvas, icon_bounds, icon_color);
  }

  // Offset the label area to account for the icon we draw.
  // The labels are laid out starting at the left edge, so we shift the
  // whole content by translating the canvas for children.
  // Simpler approach: the labels are already positioned; we just need to
  // make sure there's space. The layout has 12+d*16 left padding, plus
  // we need to leave room for the 16px icon + 8px gap = 24px.
  // The build already uses 12+d*16 left padding. Let's adjust in Build().
  // Actually, let's just draw labels on top; the icon is in the padding.

  views::View::OnPaint(canvas);
}

gfx::Size AstraNoteFolderItemView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  int width = available_size.width().value_or(kSidebarWidth);
  return gfx::Size(width, 36);
}

void AstraNoteFolderItemView::Layout() {
  views::View::Layout();
}

bool AstraNoteFolderItemView::OnMousePressed(const ui::MouseEvent& event) {
  if (select_callback_) {
    select_callback_.Run(this);
  }
  return true;
}

void AstraNoteFolderItemView::OnThemeChanged() {
  views::View::OnThemeChanged();
}

// ===========================================================================
// AstraNoteTagChipView
// ===========================================================================

BEGIN_METADATA(AstraNoteTagChipView)
END_METADATA

AstraNoteTagChipView::AstraNoteTagChipView(const std::string& tag)
    : tag_(tag) {
  SetBorder(views::CreateEmptyBorder(4, 10, 4, 10));
}

AstraNoteTagChipView::~AstraNoteTagChipView() = default;

void AstraNoteTagChipView::SetSelected(bool selected) {
  if (selected_ == selected) {
    return;
  }
  selected_ = selected;
  SchedulePaint();
}

void AstraNoteTagChipView::OnPaint(gfx::Canvas* canvas) {
  gfx::Rect bounds = GetLocalBounds();

  SkColor bg_color = selected_
      ? GetColorProvider()->GetColor(ui::kColorButtonBackgroundProminent)
      : GetColorProvider()->GetColor(ui::kColorDialogBackground);
  SkColor text_color = selected_
      ? SK_ColorWHITE
      : GetColorProvider()->GetColor(ui::kColorLabelForeground);

  cc::PaintFlags bg_flags;
  bg_flags.setColor(bg_color);
  bg_flags.setStyle(cc::PaintFlags::kFill_Style);
  bg_flags.setAntiAlias(true);
  canvas->DrawRoundRect(bounds, 12, bg_flags);

  cc::PaintFlags border_flags;
  border_flags.setColor(
      GetColorProvider()->GetColor(ui::kColorSeparator));
  border_flags.setStyle(cc::PaintFlags::kStroke_Style);
  border_flags.setStrokeWidth(1);
  border_flags.setAntiAlias(true);
  canvas->DrawRoundRect(bounds, 12, border_flags);

  // Text.
  canvas->DrawStringRect(
      base::UTF8ToUTF16("#" + tag_),
      views::style::GetFont(views::style::CONTEXT_LABEL,
                            views::style::STYLE_SECONDARY),
      text_color, bounds, gfx::ALIGN_CENTER);
}

gfx::Size AstraNoteTagChipView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  // Approximate: padding + text width.
  int text_width = static_cast<int>(tag_.size()) * 7 + 10;  // rough estimate
  return gfx::Size(text_width + 20, 24);
}

bool AstraNoteTagChipView::OnMousePressed(const ui::MouseEvent& event) {
  if (click_callback_) {
    click_callback_.Run(tag_);
  }
  return true;
}

void AstraNoteTagChipView::OnThemeChanged() {
  views::View::OnThemeChanged();
}

// ===========================================================================
// AstraNotesPageView
// ===========================================================================

BEGIN_METADATA(AstraNotesPageView)
END_METADATA

AstraNotesPageView::AstraNotesPageView() {
  Build();
}

AstraNotesPageView::AstraNotesPageView(AstraNotesPageModel* model)
    : model_(model) {
  Build();
  if (model_) {
    model_observation_.Observe(model_);
    RebuildFolders();
    RebuildTags();
    RebuildNoteCards();
  }
}

AstraNotesPageView::~AstraNotesPageView() = default;

void AstraNotesPageView::SetModel(AstraNotesPageModel* model) {
  if (model_ == model) {
    return;
  }
  model_observation_.Reset();
  model_ = model;
  if (model_) {
    model_observation_.Observe(model_);
    RebuildFolders();
    RebuildTags();
    RebuildNoteCards();
  } else {
    note_cards_.clear();
    folder_items_.clear();
    tag_chips_.clear();
    if (notes_grid_) {
      notes_grid_->RemoveAllChildViews();
    }
    if (folder_list_) {
      folder_list_->RemoveAllChildViews();
    }
    if (tags_container_) {
      tags_container_->RemoveAllChildViews();
    }
  }
  UpdateEmptyState();
}

void AstraNotesPageView::SetDisplayMode(AstraNotesDisplayMode mode) {
  if (display_mode_ == mode) {
    return;
  }
  display_mode_ = mode;
  for (auto* card : note_cards_) {
    card->SetDisplayMode(mode);
  }
  InvalidateLayout();
}

// -- Build ------------------------------------------------------------------

void AstraNotesPageView::Build() {
  SetLayoutManager(std::make_unique<views::FillLayout>());

  // Main vertical container.
  auto* main_container = AddChildView(std::make_unique<views::View>());
  auto* main_layout = main_container->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  main_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Top toolbar.
  BuildToolbar();

  // Content area: sidebar + notes + editor.
  auto* content = main_container->AddChildView(
      std::make_unique<views::View>());
  content->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification::ForSizeRule(
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded, true));

  auto* content_layout = content->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal));
  content_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Left sidebar.
  BuildSidebar();
  content->AddChildView(sidebar_container_.get());

  // Middle note list.
  BuildNotesList();
  content->AddChildView(notes_container_.get());

  // Right editor.
  BuildEditor();
  content->AddChildView(editor_container_.get());

  UpdateEmptyState();
}

void AstraNotesPageView::BuildToolbar() {
  toolbar_ = new views::View();
  toolbar_->SetPreferredSize(gfx::Size(0, kToolbarHeight));
  toolbar_->SetBorder(
      views::CreateSolidSidedBorder(0, 0, 1, 0,
                                    GetColorProvider()
                                        ? GetColorProvider()->GetColor(
                                              ui::kColorSeparator)
                                        : SK_ColorGRAY));

  auto* layout = toolbar_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal));
  layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kStart);
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  layout->set_inside_border_insets(gfx::Insets::VH(0, kSidePadding));
  layout->set_between_child_spacing(kToolbarSpacing);

  // New note button.
  new_note_button_ = toolbar_->AddChildView(
      std::make_unique<views::ImageButton>(base::BindRepeating(
          &AstraNotesPageView::OnNewNoteClicked,
          base::Unretained(this))));
  new_note_button_->SetPreferredSize(
      gfx::Size(kButtonSize, kButtonSize));
  new_note_button_->SetTooltipText(u"New note");

  // Search field.
  search_field_ = toolbar_->AddChildView(
      std::make_unique<views::Textfield>());
  search_field_->set_controller(this);
  search_field_->SetPlaceholderText(u"Search notes...");
  search_field_->SetPreferredSize(
      gfx::Size(kSearchFieldWidth, kButtonSize));

  // Spacer.
  auto* spacer = toolbar_->AddChildView(std::make_unique<views::View>());
  spacer->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification::ForSizeRule(
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded, true));

  // Sort button.
  sort_button_ = toolbar_->AddChildView(
      std::make_unique<views::ImageButton>(base::BindRepeating(
          &AstraNotesPageView::OnSortClicked,
          base::Unretained(this))));
  sort_button_->SetPreferredSize(gfx::Size(kButtonSize, kButtonSize));
  sort_button_->SetTooltipText(u"Sort notes");

  // Grid view toggle.
  grid_view_button_ = toolbar_->AddChildView(
      std::make_unique<views::ImageButton>(base::BindRepeating(
          &AstraNotesPageView::OnGridViewClicked,
          base::Unretained(this))));
  grid_view_button_->SetPreferredSize(gfx::Size(kButtonSize, kButtonSize));
  grid_view_button_->SetTooltipText(u"Grid view");

  // List view toggle.
  list_view_button_ = toolbar_->AddChildView(
      std::make_unique<views::ImageButton>(base::BindRepeating(
          &AstraNotesPageView::OnListViewClicked,
          base::Unretained(this))));
  list_view_button_->SetPreferredSize(gfx::Size(kButtonSize, kButtonSize));
  list_view_button_->SetTooltipText(u"List view");
}

void AstraNotesPageView::BuildSidebar() {
  sidebar_container_ = new views::View();
  sidebar_container_->SetPreferredSize(gfx::Size(kSidebarWidth, 0));
  sidebar_container_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification::ForSizeRule(
          views::MinimumFlexSizeRule::kScaleToMinimum,
          views::MaximumFlexSizeRule::kPreferred, false));

  sidebar_container_->SetBorder(
      views::CreateSolidSidedBorder(0, 0, 0, 1,
                                    SK_ColorGRAY));

  auto* layout = sidebar_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Scrollable sidebar content.
  sidebar_scroll_ = sidebar_container_->AddChildView(
      std::make_unique<views::ScrollView>());
  sidebar_scroll_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification::ForSizeRule(
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded, true));
  sidebar_scroll_->ClipHeightTo(0, std::numeric_limits<int>::max());

  sidebar_content_ = new views::View();
  auto* content_layout = sidebar_content_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  content_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);
  content_layout->set_inside_border_insets(
      gfx::Insets::VH(kSidePadding, 0));

  sidebar_scroll_->SetContents(sidebar_content_);

  // Folders section header.
  auto* folders_label = sidebar_content_->AddChildView(
      std::make_unique<views::Label>(u"Folders"));
  folders_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  folders_label->SetFontList(views::style::GetFont(
      views::style::CONTEXT_LABEL, views::style::STYLE_BODY_3_BOLD));
  folders_label->SetBorder(
      views::CreateEmptyBorder(0, kSidePadding, 8, kSidePadding));

  // Folder list.
  folder_list_ = sidebar_content_->AddChildView(
      std::make_unique<views::View>());
  folder_list_->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  folder_list_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification::ForSizeRule(
          views::MinimumFlexSizeRule::kScaleToMinimum,
          views::MaximumFlexSizeRule::kPreferred, false));

  // Tags section.
  tags_section_ = sidebar_content_->AddChildView(
      std::make_unique<views::View>());
  tags_section_->SetBorder(
      views::CreateEmptyBorder(kSectionSpacing, 0, 0, 0));
  auto* tags_layout = tags_section_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  tags_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  auto* tags_label = tags_section_->AddChildView(
      std::make_unique<views::Label>(u"Tags"));
  tags_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  tags_label->SetFontList(views::style::GetFont(
      views::style::CONTEXT_LABEL, views::style::STYLE_BODY_3_BOLD));
  tags_label->SetBorder(
      views::CreateEmptyBorder(0, kSidePadding, 8, kSidePadding));

  tags_container_ = tags_section_->AddChildView(
      std::make_unique<views::View>());
  tags_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  tags_container_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification::ForSizeRule(
          views::MinimumFlexSizeRule::kScaleToMinimum,
          views::MaximumFlexSizeRule::kPreferred, false));
}

void AstraNotesPageView::BuildNotesList() {
  notes_container_ = new views::View();
  notes_container_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification::ForSizeRule(
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded, true));

  auto* layout = notes_container_->SetLayoutManager(
      std::make_unique<views::FillLayout>());

  notes_scroll_ = notes_container_->AddChildView(
      std::make_unique<views::ScrollView>());
  notes_scroll_->ClipHeightTo(0, std::numeric_limits<int>::max());

  notes_grid_ = new views::View();
  notes_grid_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  notes_grid_->SetBorder(
      views::CreateEmptyBorder(kSidePadding, kSidePadding,
                               kSidePadding, kSidePadding));

  // TODO(astra): Use a proper flow layout for grid mode.
  // For scaffold, vertical list is fine.

  notes_scroll_->SetContents(notes_grid_);

  // Empty state view (overlaid).
  empty_state_view_ = notes_container_->AddChildView(
      std::make_unique<views::View>());
  auto* empty_layout = empty_state_view_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  empty_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kCenter);
  empty_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  auto* empty_label = empty_state_view_->AddChildView(
      std::make_unique<views::Label>(u"No notes found"));
  empty_label->SetFontList(views::style::GetFont(
      views::style::CONTEXT_LABEL, views::style::STYLE_BODY_1));
  empty_state_view_->SetVisible(false);
}

void AstraNotesPageView::BuildEditor() {
  editor_container_ = new views::View();
  editor_container_->SetPreferredSize(gfx::Size(kEditorWidth, 0));
  editor_container_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification::ForSizeRule(
          views::MinimumFlexSizeRule::kScaleToMinimum,
          views::MaximumFlexSizeRule::kPreferred, false));
  editor_container_->SetBorder(
      views::CreateSolidSidedBorder(0, 1, 0, 0, SK_ColorGRAY));

  auto* layout = editor_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);
  layout->set_inside_border_insets(
      gfx::Insets::VH(kSidePadding, kSidePadding));
  layout->set_between_child_spacing(kToolbarSpacing);

  // Editor toolbar.
  editor_toolbar_ = editor_container_->AddChildView(
      std::make_unique<views::View>());
  auto* toolbar_layout = editor_toolbar_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal));
  toolbar_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kEnd);
  toolbar_layout->set_between_child_spacing(4);

  pin_button_ = editor_toolbar_->AddChildView(
      std::make_unique<views::ImageButton>(base::BindRepeating(
          &AstraNotesPageView::OnPinNoteClicked,
          base::Unretained(this))));
  pin_button_->SetPreferredSize(
      gfx::Size(kButtonSize, kButtonSize));
  pin_button_->SetTooltipText(u"Pin note");

  archive_button_ = editor_toolbar_->AddChildView(
      std::make_unique<views::ImageButton>(base::BindRepeating(
          &AstraNotesPageView::OnArchiveNoteClicked,
          base::Unretained(this))));
  archive_button_->SetPreferredSize(
      gfx::Size(kButtonSize, kButtonSize));
  archive_button_->SetTooltipText(u"Archive note");

  delete_button_ = editor_toolbar_->AddChildView(
      std::make_unique<views::ImageButton>(base::BindRepeating(
          &AstraNotesPageView::OnDeleteNoteClicked,
          base::Unretained(this))));
  delete_button_->SetPreferredSize(
      gfx::Size(kButtonSize, kButtonSize));
  delete_button_->SetTooltipText(u"Delete note");

  // Title field.
  editor_title_ = editor_container_->AddChildView(
      std::make_unique<views::Textfield>());
  editor_title_->set_controller(this);
  editor_title_->SetPlaceholderText(u"Note title");
  editor_title_->SetFontList(views::style::GetFont(
      views::style::CONTEXT_LABEL, views::style::STYLE_BODY_1));
  editor_title_->SetBorder(nullptr);

  // Color picker row.
  color_picker_ = editor_container_->AddChildView(
      std::make_unique<views::View>());
  auto* color_layout = color_picker_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal));
  color_layout->set_between_child_spacing(8);

  for (const auto& c : kNoteColors) {
    auto* color_btn = color_picker_->AddChildView(
        std::make_unique<views::ImageButton>(base::BindRepeating(
            &AstraNotesPageView::OnColorClicked,
            base::Unretained(this), std::string(c.name))));
    color_btn->SetPreferredSize(gfx::Size(20, 20));
    color_btn->SetTooltipText(base::UTF8ToUTF16(c.name));
    color_btn->SetBackground(views::CreateRoundedRectBackground(
        c.color, 10));
  }

  // Tags input.
  tags_input_ = editor_container_->AddChildView(
      std::make_unique<views::Textfield>());
  tags_input_->set_controller(this);
  tags_input_->SetPlaceholderText(u"Add tags (comma separated)");
  tags_input_->SetFontList(views::style::GetFont(
      views::style::CONTEXT_LABEL, views::style::STYLE_SECONDARY));

  // Content text area.
  editor_content_ = editor_container_->AddChildView(
      std::make_unique<views::Textfield>());
  editor_content_->set_controller(this);
  editor_content_->SetPlaceholderText(u"Start writing...");
  editor_content_->SetTextInputType(ui::TEXT_INPUT_TYPE_TEXT_AREA);
  editor_content_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification::ForSizeRule(
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded, true));
  editor_content_->SetBorder(nullptr);

  // Empty editor message.
  empty_editor_view_ = editor_container_->AddChildView(
      std::make_unique<views::View>());
  auto* empty_editor_layout = empty_editor_view_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  empty_editor_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kCenter);
  empty_editor_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  empty_editor_layout->SetDefaultFlex(1);
  empty_editor_view_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification::ForSizeRule(
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded, true));

  auto* empty_editor_label = empty_editor_view_->AddChildView(
      std::make_unique<views::Label>(u"Select a note to edit"));
  empty_editor_label->SetFontList(views::style::GetFont(
      views::style::CONTEXT_LABEL, views::style::STYLE_BODY_1));
  empty_editor_label->SetAutoColorReadabilityEnabled(false);
  empty_editor_label->SetEnabledColor(SK_ColorGRAY);

  // Initially show empty state, hide editor fields.
  editor_title_->SetVisible(false);
  editor_content_->SetVisible(false);
  editor_toolbar_->SetVisible(false);
  color_picker_->SetVisible(false);
  tags_input_->SetVisible(false);
  empty_editor_view_->SetVisible(true);
}

// -- Rebuild helpers --------------------------------------------------------

void AstraNotesPageView::RebuildFolders() {
  if (!folder_list_ || !model_) {
    return;
  }

  folder_items_.clear();
  folder_list_->RemoveAllChildViews();

  // Special folders.
  int total_count = static_cast<int>(model_->GetCount()) -
                    static_cast<int>(model_->GetArchivedCount());
  auto* all_notes = folder_list_->AddChildView(
      std::make_unique<AstraNoteFolderItemView>(
          AstraNoteFolderItemView::SpecialFolder::kAllNotes, total_count));
  all_notes->SetSelectCallback(base::BindRepeating(
      &AstraNotesPageView::OnFolderClicked, base::Unretained(this)));
  all_notes->SetSelected(true);
  folder_items_.push_back(all_notes);

  auto* pinned = folder_list_->AddChildView(
      std::make_unique<AstraNoteFolderItemView>(
          AstraNoteFolderItemView::SpecialFolder::kPinned,
          static_cast<int>(model_->GetPinnedCount())));
  pinned->SetSelectCallback(base::BindRepeating(
      &AstraNotesPageView::OnFolderClicked, base::Unretained(this)));
  folder_items_.push_back(pinned);

  auto* archive = folder_list_->AddChildView(
      std::make_unique<AstraNoteFolderItemView>(
          AstraNoteFolderItemView::SpecialFolder::kArchive,
          static_cast<int>(model_->GetArchivedCount())));
  archive->SetSelectCallback(base::BindRepeating(
      &AstraNotesPageView::OnFolderClicked, base::Unretained(this)));
  folder_items_.push_back(archive);

  // Custom folders.
  for (const auto& folder : model_->GetFolders()) {
    auto* item = folder_list_->AddChildView(
        std::make_unique<AstraNoteFolderItemView>(folder, 0));
    item->SetSelectCallback(base::BindRepeating(
        &AstraNotesPageView::OnFolderClicked, base::Unretained(this)));
    folder_items_.push_back(item);
  }

  folder_list_->InvalidateLayout();
}

void AstraNotesPageView::RebuildTags() {
  if (!tags_container_ || !model_) {
    return;
  }

  tag_chips_.clear();
  tags_container_->RemoveAllChildViews();

  auto tags = model_->GetAllTags();
  for (const auto& tag : tags) {
    auto* chip = tags_container_->AddChildView(
        std::make_unique<AstraNoteTagChipView>(tag));
    chip->SetClickCallback(base::BindRepeating(
        &AstraNotesPageView::OnTagClicked, base::Unretained(this)));
    tag_chips_.push_back(chip);
  }

  tags_container_->InvalidateLayout();
}

void AstraNotesPageView::RebuildNoteCards() {
  if (!notes_grid_ || !model_) {
    return;
  }

  note_cards_.clear();
  notes_grid_->RemoveAllChildViews();

  auto filtered = model_->GetFilteredNotes();
  for (const auto& note : filtered) {
    auto* card = notes_grid_->AddChildView(
        std::make_unique<AstraNoteCardView>(note));
    card->SetDisplayMode(display_mode_);
    card->SetClickCallback(base::BindRepeating(
        &AstraNotesPageView::OnNoteCardClicked, base::Unretained(this)));
    if (note.id == model_->GetActiveNoteId()) {
      card->SetSelected(true);
    }
    note_cards_.push_back(card);
  }

  // Spacer at bottom.
  auto* spacer = notes_grid_->AddChildView(
      std::make_unique<views::View>());
  spacer->SetPreferredSize(gfx::Size(0, kSidePadding));

  notes_grid_->InvalidateLayout();
  UpdateEmptyState();
}

void AstraNotesPageView::UpdateEditorFromActiveNote() {
  if (!model_) {
    return;
  }

  const std::string& active_id = model_->GetActiveNoteId();
  if (active_id.empty()) {
    editor_title_->SetVisible(false);
    editor_content_->SetVisible(false);
    editor_toolbar_->SetVisible(false);
    color_picker_->SetVisible(false);
    tags_input_->SetVisible(false);
    empty_editor_view_->SetVisible(true);
    return;
  }

  const AstraNoteEntry* note = model_->GetNote(active_id);
  if (!note) {
    return;
  }

  editor_title_->SetVisible(true);
  editor_content_->SetVisible(true);
  editor_toolbar_->SetVisible(true);
  color_picker_->SetVisible(true);
  tags_input_->SetVisible(true);
  empty_editor_view_->SetVisible(false);

  editor_title_->SetText(note->title);
  editor_content_->SetText(note->content);

  // Build tags string for input.
  std::u16string tags_str;
  for (size_t i = 0; i < note->tags.size(); ++i) {
    if (i > 0) tags_str += u", ";
    tags_str += base::UTF8ToUTF16(note->tags[i]);
  }
  tags_input_->SetText(tags_str);
}

void AstraNotesPageView::UpdateEmptyState() {
  if (!empty_state_view_ || !notes_grid_) {
    return;
  }
  bool has_notes = model_ && !model_->GetFilteredNotes().empty();
  empty_state_view_->SetVisible(!has_notes);
  notes_scroll_->SetVisible(has_notes);
}

// -- Icon drawing -----------------------------------------------------------

void AstraNotesPageView::DrawNoteIcon(gfx::Canvas* canvas,
                                      const gfx::Rect& bounds,
                                      SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float size = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setAntiAlias(true);

  gfx::Rect note_rect(cx - size, cy - size, size * 2, size * 2);
  canvas->DrawRoundRect(note_rect, 2, flags);
  // Folded corner.
  SkPath corner;
  corner.moveTo(cx + size, cy - size);
  corner.lineTo(cx + size - size * 0.3f, cy - size);
  corner.lineTo(cx + size, cy - size + size * 0.3f);
  corner.close();
  canvas->DrawPath(corner, flags);
  // Text lines.
  for (int i = 0; i < 3; i++) {
    int y = cy - size * 0.3f + i * size * 0.4f;
    canvas->DrawLine(gfx::Point(cx - size * 0.6f, y),
                     gfx::Point(cx + size * 0.4f, y), flags);
  }
}

void AstraNotesPageView::DrawSearchIcon(gfx::Canvas* canvas,
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

void AstraNotesPageView::DrawAddIcon(gfx::Canvas* canvas,
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

  canvas->DrawLine(gfx::Point(cx - size, cy), gfx::Point(cx + size, cy),
                   flags);
  canvas->DrawLine(gfx::Point(cx, cy - size), gfx::Point(cx, cy + size),
                   flags);
}

void AstraNotesPageView::DrawGridIcon(gfx::Canvas* canvas,
                                      const gfx::Rect& bounds,
                                      SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  int cell_size = std::min(bounds.width(), bounds.height()) * 0.18f;
  int gap = 3;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.5f);
  flags.setAntiAlias(true);

  // 2x2 grid.
  for (int row = 0; row < 2; row++) {
    for (int col = 0; col < 2; col++) {
      int x = cx - cell_size - gap / 2 + col * (cell_size + gap);
      int y = cy - cell_size - gap / 2 + row * (cell_size + gap);
      canvas->DrawRoundRect(gfx::Rect(x, y, cell_size, cell_size), 2,
                            flags);
    }
  }
}

void AstraNotesPageView::DrawListIcon(gfx::Canvas* canvas,
                                      const gfx::Rect& bounds,
                                      SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float w = std::min(bounds.width(), bounds.height()) * 0.6f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Three lines representing list items.
  for (int i = 0; i < 3; i++) {
    int y = cy - w * 0.4f + i * w * 0.4f;
    // Bullet.
    canvas->DrawCircle(gfx::Point(cx - w * 0.5f, y), 2, flags);
    // Line.
    canvas->DrawLine(gfx::Point(cx - w * 0.35f, y),
                     gfx::Point(cx + w * 0.5f, y), flags);
  }
}

void AstraNotesPageView::DrawSortIcon(gfx::Canvas* canvas,
                                      const gfx::Rect& bounds,
                                      SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float size = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);

  // Up arrow.
  SkPath up;
  up.moveTo(cx, cy - size);
  up.lineTo(cx - size * 0.5f, cy - size * 0.3f);
  up.lineTo(cx + size * 0.5f, cy - size * 0.3f);
  up.close();
  canvas->DrawPath(up, flags);

  // Down arrow.
  SkPath down;
  down.moveTo(cx, cy + size);
  down.lineTo(cx - size * 0.5f, cy + size * 0.3f);
  down.lineTo(cx + size * 0.5f, cy + size * 0.3f);
  down.close();
  canvas->DrawPath(down, flags);
}

void AstraNotesPageView::DrawFolderIcon(gfx::Canvas* canvas,
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

void AstraNotesPageView::DrawTagIcon(gfx::Canvas* canvas,
                                     const gfx::Rect& bounds,
                                     SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float size = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.5f);
  flags.setAntiAlias(true);

  SkPath path;
  path.moveTo(cx - size, cy - size * 0.5f);
  path.lineTo(cx + size * 0.5f, cy - size);
  path.lineTo(cx + size, cy - size * 0.5f);
  path.lineTo(cx + size * 0.5f, cy + size);
  path.lineTo(cx - size * 0.5f, cy + size * 0.5f);
  path.close();
  canvas->DrawPath(path, flags);

  // Dot.
  flags.setStyle(cc::PaintFlags::kFill_Style);
  canvas->DrawCircle(gfx::Point(cx + size * 0.25f, cy - size * 0.4f),
                     size * 0.15f, flags);
}

void AstraNotesPageView::DrawPinIcon(gfx::Canvas* canvas,
                                     const gfx::Rect& bounds,
                                     SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float size = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);

  SkPath path;
  path.moveTo(cx - size * 0.3f, cy - size);
  path.lineTo(cx + size * 0.3f, cy - size);
  path.lineTo(cx + size * 0.7f, cy - size * 0.3f);
  path.lineTo(cx + size * 0.3f, cy + size * 0.3f);
  path.lineTo(cx, cy + size);
  path.lineTo(cx - size * 0.3f, cy + size * 0.3f);
  path.lineTo(cx - size * 0.7f, cy - size * 0.3f);
  path.close();
  canvas->DrawPath(path, flags);
}

void AstraNotesPageView::DrawArchiveIcon(gfx::Canvas* canvas,
                                         const gfx::Rect& bounds,
                                         SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float size = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setAntiAlias(true);

  // Box body.
  canvas->DrawLine(gfx::Point(cx - size, cy - size * 0.2f),
                   gfx::Point(cx - size, cy + size), flags);
  canvas->DrawLine(gfx::Point(cx + size, cy - size * 0.2f),
                   gfx::Point(cx + size, cy + size), flags);
  canvas->DrawLine(gfx::Point(cx - size, cy + size),
                   gfx::Point(cx + size, cy + size), flags);
  // Top line.
  canvas->DrawLine(gfx::Point(cx - size, cy - size * 0.2f),
                   gfx::Point(cx + size, cy - size * 0.2f), flags);
  // Lid.
  canvas->DrawLine(gfx::Point(cx - size * 0.8f, cy - size * 0.7f),
                   gfx::Point(cx + size * 0.8f, cy - size * 0.7f), flags);
  canvas->DrawLine(gfx::Point(cx - size * 0.8f, cy - size * 0.7f),
                   gfx::Point(cx - size, cy - size * 0.2f), flags);
  canvas->DrawLine(gfx::Point(cx + size * 0.8f, cy - size * 0.7f),
                   gfx::Point(cx + size, cy - size * 0.2f), flags);
  // Down arrow inside.
  SkPath arrow;
  arrow.moveTo(cx, cy + size * 0.5f);
  arrow.lineTo(cx - size * 0.3f, cy + size * 0.1f);
  arrow.lineTo(cx + size * 0.3f, cy + size * 0.1f);
  arrow.close();
  canvas->DrawPath(arrow, flags);
}

void AstraNotesPageView::DrawTrashIcon(gfx::Canvas* canvas,
                                       const gfx::Rect& bounds,
                                       SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float size = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setAntiAlias(true);

  // Lid.
  canvas->DrawLine(gfx::Point(cx - size * 0.7f, cy - size * 0.6f),
                   gfx::Point(cx + size * 0.7f, cy - size * 0.6f), flags);
  // Handle.
  canvas->DrawLine(gfx::Point(cx - size * 0.3f, cy - size),
                   gfx::Point(cx + size * 0.3f, cy - size), flags);
  canvas->DrawLine(gfx::Point(cx - size * 0.3f, cy - size),
                   gfx::Point(cx - size * 0.3f, cy - size * 0.6f), flags);
  canvas->DrawLine(gfx::Point(cx + size * 0.3f, cy - size),
                   gfx::Point(cx + size * 0.3f, cy - size * 0.6f), flags);
  // Body.
  canvas->DrawLine(gfx::Point(cx - size * 0.6f, cy - size * 0.5f),
                   gfx::Point(cx - size * 0.5f, cy + size), flags);
  canvas->DrawLine(gfx::Point(cx + size * 0.6f, cy - size * 0.5f),
                   gfx::Point(cx + size * 0.5f, cy + size), flags);
  canvas->DrawLine(gfx::Point(cx - size * 0.5f, cy + size),
                   gfx::Point(cx + size * 0.5f, cy + size), flags);
  // Lines inside.
  canvas->DrawLine(gfx::Point(cx - size * 0.2f, cy - size * 0.3f),
                   gfx::Point(cx - size * 0.25f, cy + size * 0.8f), flags);
  canvas->DrawLine(gfx::Point(cx + size * 0.2f, cy - size * 0.3f),
                   gfx::Point(cx + size * 0.25f, cy + size * 0.8f), flags);
}

void AstraNotesPageView::DrawColorPaletteIcon(gfx::Canvas* canvas,
                                              const gfx::Rect& bounds,
                                              SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float radius = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1.5f);
  flags.setAntiAlias(true);

  // Palette outline.
  SkPath path;
  path.moveTo(cx, cy - radius);
  path.arcTo(cx + radius, cy - radius, cx + radius, cy, radius);
  path.arcTo(cx + radius, cy + radius, cx, cy + radius, radius);
  path.arcTo(cx - radius, cy + radius, cx - radius, cy, radius);
  path.lineTo(cx - radius, cy - radius * 0.3f);
  path.close();
  canvas->DrawPath(path, flags);

  // Paint blobs.
  SkColor blob_colors[] = {0xFFE57373, 0xFFFFB74D, 0xFF81C784, 0xFF64B5F6};
  float blob_r = radius * 0.18f;
  float positions[][2] = {
      {-radius * 0.4f, -radius * 0.3f},
      {radius * 0.1f, -radius * 0.5f},
      {radius * 0.5f, -radius * 0.1f},
      {radius * 0.2f, radius * 0.4f},
  };
  for (int i = 0; i < 4; i++) {
    flags.setColor(blob_colors[i]);
    flags.setStyle(cc::PaintFlags::kFill_Style);
    canvas->DrawCircle(
        gfx::Point(cx + positions[i][0], cy + positions[i][1]),
        blob_r, flags);
  }
}

void AstraNotesPageView::DrawEditIcon(gfx::Canvas* canvas,
                                      const gfx::Rect& bounds,
                                      SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float size = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setAntiAlias(true);

  // Pencil body.
  SkPath path;
  path.moveTo(cx - size * 0.7f, cy + size * 0.7f);
  path.lineTo(cx + size * 0.3f, cy - size * 0.3f);
  path.lineTo(cx + size * 0.7f, cy + size * 0.1f);
  path.lineTo(cx - size * 0.3f, cy + size);
  path.close();
  canvas->DrawPath(path, flags);

  // Tip.
  SkPath tip;
  tip.moveTo(cx + size * 0.3f, cy - size * 0.3f);
  tip.lineTo(cx + size * 0.5f, cy - size * 0.5f);
  tip.lineTo(cx + size * 0.7f, cy + size * 0.1f);
  tip.close();
  canvas->DrawPath(tip, flags);
}

void AstraNotesPageView::DrawCheckIcon(gfx::Canvas* canvas,
                                       const gfx::Rect& bounds,
                                       SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float size = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(2);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setAntiAlias(true);

  SkPath path;
  path.moveTo(cx - size * 0.6f, cy);
  path.lineTo(cx - size * 0.15f, cy + size * 0.5f);
  path.lineTo(cx + size * 0.6f, cy - size * 0.4f);
  canvas->DrawPath(path, flags);
}

void AstraNotesPageView::DrawMoreIcon(gfx::Canvas* canvas,
                                      const gfx::Rect& bounds,
                                      SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float dot_r = std::min(bounds.width(), bounds.height()) * 0.07f;
  float gap = dot_r * 3;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);

  canvas->DrawCircle(gfx::Point(cx - gap, cy), dot_r, flags);
  canvas->DrawCircle(gfx::Point(cx, cy), dot_r, flags);
  canvas->DrawCircle(gfx::Point(cx + gap, cy), dot_r, flags);
}

// -- Button handlers --------------------------------------------------------

void AstraNotesPageView::OnNewNoteClicked() {
  if (!model_) {
    return;
  }
  std::string folder = model_->GetFolderFilter();
  std::string id = model_->AddNote(u"New note", u"", folder, "default");
  model_->SetActiveNote(id);
}

void AstraNotesPageView::OnGridViewClicked() {
  SetDisplayMode(AstraNotesDisplayMode::kGrid);
}

void AstraNotesPageView::OnListViewClicked() {
  SetDisplayMode(AstraNotesDisplayMode::kList);
}

void AstraNotesPageView::OnSortClicked() {
  // TODO(astra): Show sort menu. For now, cycle through sort types.
  if (!model_) {
    return;
  }
  auto current = model_->GetSortType();
  auto next = static_cast<AstraNotesSortType>(
      (static_cast<int>(current) + 1) %
      static_cast<int>(AstraNotesSortType::kColor) + 1);
  model_->SetSortType(next);
}

void AstraNotesPageView::OnNoteCardClicked(const std::string& note_id) {
  if (!model_) {
    return;
  }
  model_->SetActiveNote(note_id);
}

void AstraNotesPageView::OnFolderClicked(AstraNoteFolderItemView* item) {
  if (!model_) {
    return;
  }

  // Deselect all.
  for (auto* folder_item : folder_items_) {
    folder_item->SetSelected(false);
  }
  item->SetSelected(true);

  if (item->is_special()) {
    switch (item->special_type()) {
      case AstraNoteFolderItemView::SpecialFolder::kAllNotes:
        model_->SetFilter(AstraNotesFilter::kAll);
        model_->SetFolderFilter("");
        break;
      case AstraNoteFolderItemView::SpecialFolder::kPinned:
        model_->SetFilter(AstraNotesFilter::kPinned);
        model_->SetFolderFilter("");
        break;
      case AstraNoteFolderItemView::SpecialFolder::kArchive:
        model_->SetFilter(AstraNotesFilter::kArchived);
        model_->SetFolderFilter("");
        break;
    }
  } else {
    model_->SetFilter(AstraNotesFilter::kAll);
    model_->SetFolderFilter(item->folder_id());
    // TODO(astra): Folder filter by name vs id. For now use name.
    model_->SetFolderFilter(base::UTF16ToUTF8(item->name()));
  }
}

void AstraNotesPageView::OnTagClicked(const std::string& tag) {
  if (!model_) {
    return;
  }

  std::string current = model_->GetTagFilter();
  if (current == tag) {
    model_->SetTagFilter("");
    for (auto* chip : tag_chips_) {
      chip->SetSelected(false);
    }
  } else {
    model_->SetTagFilter(tag);
    for (auto* chip : tag_chips_) {
      chip->SetSelected(chip->tag() == tag);
    }
  }
}

void AstraNotesPageView::OnDeleteNoteClicked() {
  if (!model_ || model_->GetActiveNoteId().empty()) {
    return;
  }
  std::string id = model_->GetActiveNoteId();
  model_->RemoveNote(id);
  // Active note will be cleared by model.
  UpdateEditorFromActiveNote();
}

void AstraNotesPageView::OnColorClicked(const std::string& color) {
  if (!model_ || model_->GetActiveNoteId().empty()) {
    return;
  }
  model_->UpdateNoteColor(model_->GetActiveNoteId(), color);
}

void AstraNotesPageView::OnPinNoteClicked() {
  if (!model_ || model_->GetActiveNoteId().empty()) {
    return;
  }
  model_->TogglePin(model_->GetActiveNoteId());
}

void AstraNotesPageView::OnArchiveNoteClicked() {
  if (!model_ || model_->GetActiveNoteId().empty()) {
    return;
  }
  const auto* note = model_->GetNote(model_->GetActiveNoteId());
  if (note && note->is_archived) {
    model_->UnarchiveNote(model_->GetActiveNoteId());
  } else {
    model_->ArchiveNote(model_->GetActiveNoteId());
  }
}

AstraNoteFolderItemView* AstraNotesPageView::GetSelectedFolderItem() const {
  for (auto* item : folder_items_) {
    if (item->selected()) {
      return item;
    }
  }
  return nullptr;
}

// -- AstraNotesPageObserver -------------------------------------------------

void AstraNotesPageView::OnNotesChanged() {
  RebuildNoteCards();
  RebuildFolders();
  RebuildTags();
}

void AstraNotesPageView::OnNoteAdded(const std::string& id) {
  // TODO(astra): Incremental add instead of full rebuild.
  RebuildNoteCards();
}

void AstraNotesPageView::OnNoteRemoved(const std::string& id) {
  RebuildNoteCards();
  UpdateEditorFromActiveNote();
}

void AstraNotesPageView::OnNoteUpdated(const std::string& id) {
  // Update matching card.
  if (!model_) {
    return;
  }
  const AstraNoteEntry* note = model_->GetNote(id);
  if (!note) {
    return;
  }
  for (auto* card : note_cards_) {
    if (card->note_id() == id) {
      card->Update(*note);
      break;
    }
  }
  if (id == model_->GetActiveNoteId()) {
    UpdateEditorFromActiveNote();
  }
}

void AstraNotesPageView::OnFolderAdded(const std::string& id) {
  RebuildFolders();
}

void AstraNotesPageView::OnFolderRemoved(const std::string& id) {
  RebuildFolders();
}

void AstraNotesPageView::OnActiveNoteChanged(const std::string& id) {
  for (auto* card : note_cards_) {
    card->SetSelected(card->note_id() == id);
  }
  UpdateEditorFromActiveNote();
}

void AstraNotesPageView::OnSearchChanged(const std::u16string& query) {
  // Model notifies; we just ensure the search field reflects it.
  if (search_field_ && search_field_->GetText() != query) {
    search_field_->SetText(query);
  }
  RebuildNoteCards();
}

void AstraNotesPageView::OnFilterChanged() {
  RebuildNoteCards();
}

void AstraNotesPageView::OnNotesPageModelShutdown() {
  model_observation_.Reset();
  model_ = nullptr;
}

// -- views::View ------------------------------------------------------------

void AstraNotesPageView::Layout() {
  views::View::Layout();
}

gfx::Size AstraNotesPageView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return gfx::Size(kSidebarWidth + kEditorWidth + 400, 600);
}

void AstraNotesPageView::OnThemeChanged() {
  views::View::OnThemeChanged();

  // TODO(astra): Update custom-drawn icon images for theme colors.
  // For now, the icons are drawn at paint time and will pick up new
  // colors on the next paint.
}

// -- TextfieldController ----------------------------------------------------

void AstraNotesPageView::ContentsChanged(views::Textfield* sender,
                                         const std::u16string& new_contents) {
  if (!model_) {
    return;
  }

  if (sender == search_field_) {
    model_->SetSearchQuery(new_contents);
  } else if (sender == editor_title_) {
    if (!model_->GetActiveNoteId().empty()) {
      model_->UpdateNoteTitle(model_->GetActiveNoteId(), new_contents);
    }
  } else if (sender == editor_content_) {
    if (!model_->GetActiveNoteId().empty()) {
      model_->UpdateNoteContent(model_->GetActiveNoteId(), new_contents);
    }
  }
  // TODO(astra): Handle tags_input_ changes.
}

bool AstraNotesPageView::HandleKeyEvent(views::Textfield* sender,
                                        const ui::KeyEvent& key_event) {
  // TODO(astra): Handle Enter to create new tag, Escape to clear, etc.
  return false;
}

}  // namespace astra
