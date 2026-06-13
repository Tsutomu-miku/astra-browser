// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/extensions_page/astra_extensions_page_view.h"

#include <algorithm>
#include <memory>
#include <string>

#include "base/i18n/case_conversion.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkColor.h"
#include "skia/core/SkPath.h"
#include "ui/base/l10n/l10n_util.h"
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
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/button/toggle_button.h"
#include "ui/views/controls/combobox/combobox.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/separator.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/flex_layout_view.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/layout/table_layout.h"
#include "ui/views/view.h"
#include "ui/views/view_class_properties.h"

namespace astra {

namespace {

// Constants.
constexpr int kToolbarHeight = 56;
constexpr int kSidebarWidth = 200;
constexpr int kCategoryItemHeight = 36;
constexpr int kCardSize = 220;
constexpr int kCardSpacing = 16;
constexpr int kCardPadding = 16;
constexpr int kIconSize = 48;
constexpr int kCompactIconSize = 24;
constexpr int kSearchFieldWidth = 280;
constexpr int kToolbarSpacing = 12;

// Helper: returns a deterministic color based on string hash.
SkColor HashColor(const std::string& str) {
  size_t hash = std::hash<std::string>{}(str);
  static const SkColor kColors[] = {
      SkColorSetRGB(0x42, 0x85, 0xF4), SkColorSetRGB(0xEA, 0x43, 0x35),
      SkColorSetRGB(0x34, 0xA8, 0x53), SkColorSetRGB(0xFB, 0xBC, 0x04),
      SkColorSetRGB(0x9C, 0x27, 0xB0), SkColorSetRGB(0xFF, 0x6D, 0x00),
      SkColorSetRGB(0x00, 0x96, 0x88), SkColorSetRGB(0x3F, 0x51, 0xB5),
  };
  return kColors[hash % std::size(kColors)];
}

// Draw a puzzle piece icon (for extensions).
void DrawPuzzleIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(2);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Main square.
  gfx::RectF square(cx - s, cy - s * 0.6f, s * 2, s * 1.8f);
  SkPath path;
  path.addRect(
      SkRect::MakeXYWH(square.x(), square.y(), square.width(), square.height()));
  canvas->DrawPath(path, flags);

  // Top puzzle knob (circle with cut-out).
  float knob_r = s * 0.35f;
  float knob_cy = cy - s * 0.6f;
  canvas->DrawCircle(gfx::PointF(cx, knob_cy), knob_r, flags);

  // Bottom puzzle notch (arc cut into the bottom).
  SkPath notch;
  notch.moveTo(cx - s * 0.5f, cy + s * 1.2f);
  notch.arcTo(SkRect::MakeXYWH(cx - s * 0.5f, cy + s * 0.9f, s, s * 0.6f),
              180, 180, false);
  canvas->DrawPath(notch, flags);
}

// Draw a pin icon (pushpin).
void DrawPinIcon(gfx::Canvas* canvas,
                 const gfx::Rect& bounds,
                 SkColor color,
                 bool filled) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(
      filled ? cc::PaintFlags::kFill_Style : cc::PaintFlags::kStroke_Style);

  SkPath path;
  // Pushpin head (diamond shape).
  path.moveTo(cx, cy - s);
  path.lineTo(cx + s * 0.6f, cy - s * 0.3f);
  path.lineTo(cx + s * 0.4f, cy + s * 0.5f);
  path.lineTo(cx, cy + s);
  path.lineTo(cx - s * 0.4f, cy + s * 0.5f);
  path.lineTo(cx - s * 0.6f, cy - s * 0.3f);
  path.close();
  canvas->DrawPath(path, flags);
}

// Draw a magnifying glass / search icon.
void DrawSearchIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color) {
  int cx = bounds.x() + bounds.width() / 2 - 2;
  int cy = bounds.y() + bounds.height() / 2 - 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.3f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(2);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  // Circle.
  canvas->DrawCircle(gfx::Point(cx, cy), s, flags);

  // Handle.
  SkPath handle;
  handle.moveTo(cx + s * 0.7f, cy + s * 0.7f);
  handle.lineTo(cx + s * 1.4f, cy + s * 1.4f);
  canvas->DrawPath(handle, flags);
}

// Draw a grid icon.
void DrawGridIcon(gfx::Canvas* canvas,
                  const gfx::Rect& bounds,
                  SkColor color) {
  int x = bounds.x() + (bounds.width() - 14) / 2;
  int y = bounds.y() + (bounds.height() - 14) / 2;
  int cell = 4;
  int gap = 2;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);

  // 2x2 grid of squares.
  for (int row = 0; row < 2; ++row) {
    for (int col = 0; col < 2; ++col) {
      canvas->DrawRect(
          gfx::Rect(x + col * (cell + gap), y + row * (cell + gap), cell, cell),
          flags);
    }
  }
}

// Draw a list icon.
void DrawListIcon(gfx::Canvas* canvas,
                  const gfx::Rect& bounds,
                  SkColor color) {
  int x = bounds.x() + (bounds.width() - 16) / 2;
  int y = bounds.y() + (bounds.height() - 14) / 2;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);

  // Three horizontal lines.
  for (int i = 0; i < 3; ++i) {
    canvas->DrawRect(gfx::Rect(x, y + i * 5, 16, 2), flags);
  }
}

// Draw more (three dots) icon.
void DrawMoreIcon(gfx::Canvas* canvas,
                  const gfx::Rect& bounds,
                  SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float dot_r = 1.5f;
  float spacing = 4;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);

  canvas->DrawCircle(gfx::Point(cx - spacing, cy), dot_r, flags);
  canvas->DrawCircle(gfx::Point(cx, cy), dot_r, flags);
  canvas->DrawCircle(gfx::Point(cx + spacing, cy), dot_r, flags);
}

// Draw an add / plus icon.
void DrawAddIcon(gfx::Canvas* canvas,
                 const gfx::Rect& bounds,
                 SkColor color) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(2);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);

  SkPath path;
  path.moveTo(cx - s, cy);
  path.lineTo(cx + s, cy);
  path.moveTo(cx, cy - s);
  path.lineTo(cx, cy + s);
  canvas->DrawPath(path, flags);
}

// Draw a shop / bag icon for Web Store.
void DrawShopIcon(gfx::Canvas* canvas,
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

  // Bag body.
  SkPath path;
  path.moveTo(cx - s, cy - s * 0.3f);
  path.lineTo(cx - s * 0.7f, cy + s);
  path.lineTo(cx + s * 0.7f, cy + s);
  path.lineTo(cx + s, cy - s * 0.3f);
  path.close();
  canvas->DrawPath(path, flags);

  // Bag handles.
  SkPath handles;
  handles.moveTo(cx - s * 0.6f, cy - s * 0.3f);
  handles.arcTo(SkRect::MakeXYWH(cx - s * 0.6f, cy - s * 0.9f, s * 0.6f, s * 0.6f),
                180, -90, false);
  handles.moveTo(cx + s * 0.6f, cy - s * 0.3f);
  handles.arcTo(SkRect::MakeXYWH(cx, cy - s * 0.9f, s * 0.6f, s * 0.6f),
                0, 90, false);
  canvas->DrawPath(handles, flags);
}

// Draw a shield icon for permission level.
void DrawShieldIcon(gfx::Canvas* canvas,
                    const gfx::Rect& bounds,
                    SkColor color,
                    bool filled) {
  int cx = bounds.x() + bounds.width() / 2;
  int cy = bounds.y() + bounds.height() / 2;
  float s = std::min(bounds.width(), bounds.height()) * 0.35f;

  cc::PaintFlags flags;
  flags.setColor(color);
  flags.setStrokeWidth(1.5f);
  flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  flags.setAntiAlias(true);
  flags.setStyle(
      filled ? cc::PaintFlags::kFill_Style : cc::PaintFlags::kStroke_Style);

  SkPath path;
  path.moveTo(cx, cy - s);
  path.lineTo(cx + s * 0.8f, cy - s * 0.5f);
  path.lineTo(cx + s * 0.8f, cy + s * 0.3f);
  path.lineTo(cx, cy + s);
  path.lineTo(cx - s * 0.8f, cy + s * 0.3f);
  path.lineTo(cx - s * 0.8f, cy - s * 0.5f);
  path.close();
  canvas->DrawPath(path, flags);
}

}  // namespace

// ===========================================================================
// AstraExtensionCardView
// ===========================================================================

AstraExtensionCardView::AstraExtensionCardView(
    const std::string& extension_id)
    : extension_id_(extension_id) {
  BuildLayout();
  SetFocusBehavior(FocusBehavior::ALWAYS);
}

AstraExtensionCardView::~AstraExtensionCardView() = default;

void AstraExtensionCardView::UpdateFromModel(
    const AstraExtensionsPageModel* model) {
  const auto* ext = model->GetExtension(extension_id_);
  if (!ext) {
    return;
  }

  name_label_->SetText(ext->name);
  version_label_->SetText(base::UTF8ToUTF16("v" + ext->version));
  desc_label_->SetText(ext->description);

  // Status text.
  std::u16string status;
  if (ext->has_errors) {
    status = u"Has errors";
  } else if (!ext->is_enabled) {
    status = u"Disabled";
  } else if (ext->state == AstraExtensionState::kEnabled) {
    status = u"Enabled";
  } else {
    status = u"Unknown";
  }
  status_label_->SetText(status);

  // Toggle.
  enabled_toggle_->SetIsOn(ext->is_enabled &&
                           ext->state == AstraExtensionState::kEnabled);

  // Pin button state.
  pin_button_->SetToggled(ext->is_pinned);

  // Error indicator visibility.
  error_indicator_->SetVisible(ext->has_errors ||
                               ext->state == AstraExtensionState::kCorrupted);

  // Update icon.
  SkColor color = HashColor(ext->name);
  SkBitmap bitmap;
  bitmap.allocN32Pixels(kIconSize, kIconSize);
  gfx::Canvas canvas(gfx::Size(kIconSize, kIconSize), 1.0f, false);
  canvas.DrawColor(SK_ColorWHITE);
  gfx::Rect icon_bounds(0, 0, kIconSize, kIconSize);
  DrawPuzzleIcon(&canvas, icon_bounds, color);
  icon_view_->SetImage(
      gfx::ImageSkia::CreateFrom1xBitmap(canvas.GetBitmap()));

  // Accessibility.
  SetAccessibleName(ext->name);
  SetTooltipText(ext->description);
}

void AstraExtensionCardView::SetCompact(bool compact) {
  if (compact_ == compact) {
    return;
  }
  compact_ = compact;
  BuildLayout();
  InvalidateLayout();
}

void AstraExtensionCardView::OnThemeChanged() {
  views::View::OnThemeChanged();
  UpdateColors();
}

void AstraExtensionCardView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  gfx::Rect bounds = GetContentsBounds();

  cc::PaintFlags flags;
  flags.setColor(is_hovered_ ? SkColorSetA(SK_ColorBLACK, 0x0A)
                             : SkColorSetA(SK_ColorBLACK, 0x05));
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);

  // Card background with rounded corners.
  SkPath path;
  float radius = 8.0f;
  path.addRoundRect(
      SkRect::MakeXYWH(bounds.x(), bounds.y(), bounds.width(), bounds.height()),
      radius, radius);
  canvas->DrawPath(path, flags);

  // Border.
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setColor(SkColorSetA(SK_ColorBLACK, 0x0D));
  flags.setStrokeWidth(1);
  canvas->DrawPath(path, flags);
}

bool AstraExtensionCardView::OnMousePressed(const ui::MouseEvent& event) {
  // Handle click on card.
  RequestFocus();
  return true;
}

void AstraExtensionCardView::OnMouseEntered(const ui::MouseEvent& event) {
  is_hovered_ = true;
  SchedulePaint();
}

void AstraExtensionCardView::OnMouseExited(const ui::MouseEvent& event) {
  is_hovered_ = false;
  SchedulePaint();
}

void AstraExtensionCardView::BuildLayout() {
  RemoveAllChildViews();
  extension_cards_.clear();
  icon_view_ = nullptr;
  name_label_ = nullptr;
  version_label_ = nullptr;
  desc_label_ = nullptr;
  status_label_ = nullptr;
  enabled_toggle_ = nullptr;
  pin_button_ = nullptr;
  details_button_ = nullptr;

  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(kCardPadding, kCardPadding), 8));

  // Top row: icon + name + version + actions.
  auto* top_row = AddChildView(std::make_unique<views::View>());
  auto* top_layout =
      top_row->SetLayoutManager(std::make_unique<views::FlexLayout>());
  top_layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  top_layout->SetCrossAxisAlignment(views::LayoutAlignment::kStart);

  // Icon.
  auto icon = std::make_unique<views::ImageView>();
  icon->SetImageSize(gfx::Size(kIconSize, kIconSize));
  icon_view_ = top_row->AddChildView(std::move(icon));
  icon_view_->SetProperty(views::kMarginsKey, gfx::Insets(0, 0, 0, 12));

  // Name + version column.
  auto* name_col = top_row->AddChildView(std::make_unique<views::View>());
  auto* name_layout =
      name_col->SetLayoutManager(std::make_unique<views::FlexLayout>());
  name_layout->SetOrientation(views::LayoutOrientation::kVertical);
  name_layout->SetFlexForView(name_col, 1);

  auto name = std::make_unique<views::Label>();
  name->SetFontList(views::style::GetFont(
      views::style::CONTEXT_LABEL, views::style::STYLE_EMPHASIZED));
  name->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  name->SetAutoColorReadabilityEnabled(false);
  name_label_ = name_col->AddChildView(std::move(name));

  auto version = std::make_unique<views::Label>();
  version->SetFontList(views::style::GetFont(
      views::style::CONTEXT_LABEL, views::style::STYLE_SECONDARY));
  version->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  version->SetAutoColorReadabilityEnabled(false);
  version_label_ = name_col->AddChildView(std::move(version));

  // Action buttons on the right.
  auto* actions_row = top_row->AddChildView(std::make_unique<views::View>());
  auto* actions_layout =
      actions_row->SetLayoutManager(std::make_unique<views::FlexLayout>());
  actions_layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  actions_layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);

  auto details = std::make_unique<views::ImageButton>();
  details->SetMinSize(gfx::Size(20, 20));
  details->SetImageHorizontalAlignment(views::ImageButton::ALIGN_CENTER);
  details->SetImageVerticalAlignment(views::ImageButton::ALIGN_MIDDLE);
  details->SetCallback(base::BindRepeating(
      &AstraExtensionCardView::OnDetailsClicked, base::Unretained(this)));
  details->SetTooltipText(u"More details");
  details_button_ = actions_row->AddChildView(std::move(details));

  // Description.
  auto desc = std::make_unique<views::Label>();
  desc->SetMultiLine(true);
  desc->SetAllowCharacterBreak(true);
  desc->SetMaxLines(compact_ ? 1 : 3);
  desc->SetElideBehavior(gfx::ELIDE_TAIL);
  desc->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  desc->SetFontList(views::style::GetFont(
      views::style::CONTEXT_LABEL, views::style::STYLE_SECONDARY));
  desc->SetAutoColorReadabilityEnabled(false);
  desc_label_ = AddChildView(std::move(desc));

  // Bottom row: status + toggle + pin.
  auto* bottom_row = AddChildView(std::make_unique<views::View>());
  auto* bottom_layout =
      bottom_row->SetLayoutManager(std::make_unique<views::FlexLayout>());
  bottom_layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  bottom_layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);

  auto status = std::make_unique<views::Label>();
  status->SetFontList(views::style::GetFont(
      views::style::CONTEXT_LABEL, views::style::STYLE_SECONDARY));
  status->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  status->SetAutoColorReadabilityEnabled(false);
  status_label_ = bottom_row->AddChildView(std::move(status));
  bottom_layout->SetFlexForView(status_label_, 1);

  // Error indicator.
  auto error = std::make_unique<views::View>();
  error->SetPreferredSize(gfx::Size(8, 8));
  error->SetVisible(false);
  error_indicator_ = bottom_row->AddChildView(std::move(error));
  error_indicator_->SetProperty(views::kMarginsKey, gfx::Insets(0, 8));

  auto pin = std::make_unique<views::ImageButton>();
  pin->SetMinSize(gfx::Size(24, 24));
  pin->SetCallback(base::BindRepeating(
      &AstraExtensionCardView::OnPinClicked, base::Unretained(this)));
  pin->SetTooltipText(u"Pin to toolbar");
  pin_button_ = bottom_row->AddChildView(std::move(pin));

  auto toggle = std::make_unique<views::ToggleButton>();
  toggle->SetCallback(base::BindRepeating(
      &AstraExtensionCardView::OnEnabledToggled, base::Unretained(this)));
  enabled_toggle_ = bottom_row->AddChildView(std::move(toggle));
  enabled_toggle_->SetProperty(views::kMarginsKey, gfx::Insets(0, 8, 0, 0));

  // Set minimum size.
  SetPreferredSize(gfx::Size(kCardSize, 0));
  SetMinSize(gfx::Size(kCardSize, 140));

  UpdateColors();
}

void AstraExtensionCardView::UpdateColors() {
  if (!GetWidget()) {
    return;
  }

  auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  SkColor text_color = color_provider->GetColor(ui::kColorLabelForeground);
  SkColor secondary_color =
      color_provider->GetColor(ui::kColorLabelForegroundSecondary);

  if (name_label_) {
    name_label_->SetEnabledColor(text_color);
  }
  if (version_label_) {
    version_label_->SetEnabledColor(secondary_color);
  }
  if (desc_label_) {
    desc_label_->SetEnabledColor(secondary_color);
  }
  if (status_label_) {
    status_label_->SetEnabledColor(secondary_color);
  }

  // Error indicator color.
  if (error_indicator_) {
    error_indicator_->SetBackground(views::CreateSolidBackground(
        SkColorSetRGB(0xEA, 0x43, 0x35)));
    error_indicator_->SetBorder(views::CreateRoundedRectBorder(
        0, 4, SkColorSetRGB(0xEA, 0x43, 0x35)));
  }

  SchedulePaint();
}

void AstraExtensionCardView::OnEnabledToggled() {
  // TODO(astra): Notify model/delegate.
  // For now, we just update visually — in production this would call
  // ExtensionService to enable/disable the extension.
}

void AstraExtensionCardView::OnPinClicked() {
  // TODO(astra): Notify model/delegate.
}

void AstraExtensionCardView::OnDetailsClicked() {
  // TODO(astra): Show extension detail dialog or navigate to detail view.
}

// ===========================================================================
// AstraExtensionsPageView
// ===========================================================================

AstraExtensionsPageView::AstraExtensionsPageView() {
  BuildUI();
}

AstraExtensionsPageView::~AstraExtensionsPageView() {
  if (scoped_observation_.IsObserving()) {
    scoped_observation_.RemoveObserver();
  }
}

void AstraExtensionsPageView::SetModel(AstraExtensionsPageModel* model) {
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

void AstraExtensionsPageView::SetDisplayMode(bool compact) {
  if (compact_mode_ == compact) {
    return;
  }
  compact_mode_ = compact;

  for (auto* card : extension_cards_) {
    card->SetCompact(compact);
  }

  RefreshExtensionCards();
  InvalidateLayout();
  SchedulePaint();
}

void AstraExtensionsPageView::RefreshFromModel() {
  if (!model_) {
    return;
  }

  RefreshCategories();
  RefreshExtensionCards();
  UpdateEmptyState();
}

void AstraExtensionsPageView::OnThemeChanged() {
  views::View::OnThemeChanged();

  auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  SkColor bg_color = color_provider->GetColor(ui::kColorDialogBackground);
  SetBackground(views::CreateSolidBackground(bg_color));

  SchedulePaint();
}

void AstraExtensionsPageView::Layout() {
  views::View::Layout();

  gfx::Rect bounds = GetContentsBounds();

  // Toolbar at top.
  if (toolbar_) {
    toolbar_->SetBounds(bounds.x(), bounds.y(), bounds.width(), kToolbarHeight);
  }

  int content_y = bounds.y() + kToolbarHeight;
  int content_height = bounds.height() - kToolbarHeight;

  // Sidebar on left.
  if (categories_sidebar_) {
    categories_sidebar_->SetBounds(bounds.x(), content_y,
                                   kSidebarWidth, content_height);
  }

  // Content area to the right of sidebar.
  int content_x = bounds.x() + kSidebarWidth;
  int content_width = bounds.width() - kSidebarWidth;

  if (content_scroll_view_) {
    content_scroll_view_->SetBounds(content_x, content_y,
                                     content_width, content_height);
  }
}

gfx::Size AstraExtensionsPageView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return gfx::Size(800, 600);
}

void AstraExtensionsPageView::ContentsChanged(
    views::Textfield* sender,
    const std::u16string& new_contents) {
  if (sender == search_field_ && model_) {
    model_->SetSearchQuery(new_contents);
  }
}

bool AstraExtensionsPageView::HandleKeyEvent(
    views::Textfield* sender,
    const ui::KeyEvent& key_event) {
  // Handle Enter/Escape in search field.
  if (key_event.key_code() == ui::VKEY_ESCAPE) {
    search_field_->SetText(std::u16string());
    return true;
  }
  return false;
}

void AstraExtensionsPageView::OnExtensionsChanged(
    AstraExtensionsPageModel* model) {
  DCHECK_EQ(model, model_);
  RefreshExtensionCards();
  RefreshCategories();
  UpdateEmptyState();
}

void AstraExtensionsPageView::OnFilterChanged(
    AstraExtensionsPageModel* model) {
  DCHECK_EQ(model, model_);
  RefreshExtensionCards();
  UpdateEmptyState();
}

void AstraExtensionsPageView::OnSearchChanged(
    AstraExtensionsPageModel* model,
    const std::u16string& query) {
  DCHECK_EQ(model, model_);
  // Search field might already have this text, but sync anyway.
  if (search_field_->GetText() != query) {
    search_field_->SetText(query);
  }
  RefreshExtensionCards();
  UpdateEmptyState();
}

void AstraExtensionsPageView::OnExtensionsPageModelShutdown(
    AstraExtensionsPageModel* model) {
  DCHECK_EQ(model, model_);
  scoped_observation_.RemoveObserver();
  model_ = nullptr;
}

int AstraExtensionsPageView::GetExtensionCardCount() const {
  return static_cast<int>(extension_cards_.size());
}

AstraExtensionCardView* AstraExtensionsPageView::GetExtensionCardAt(
    int index) const {
  if (index < 0 || index >= static_cast<int>(extension_cards_.size())) {
    return nullptr;
  }
  return extension_cards_[index];
}

void AstraExtensionsPageView::BuildUI() {
  SetLayoutManager(std::make_unique<views::FillLayout>());

  // Main container.
  auto* main = AddChildView(std::make_unique<views::View>());
  main->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));

  BuildToolbar();
  BuildCategoriesSidebar();
  BuildContentArea();
}

void AstraExtensionsPageView::BuildToolbar() {
  auto toolbar = std::make_unique<views::View>();
  toolbar->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SkColorSetA(SK_ColorBLACK, 0x0D)));

  auto* layout = toolbar->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kHorizontal);
  layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);
  layout->SetInteriorMargin(gfx::Insets::VH(0, 16));

  // Search field.
  auto search = std::make_unique<views::Textfield>();
  search->SetPlaceholderText(u"Search extensions");
  search->SetAccessibleName(u"Search extensions");
  search->set_controller(this);
  search->SetBorder(views::CreateRoundedRectBorder(
      1, 20, SkColorSetA(SK_ColorBLACK, 0x1A)));
  search->SetVerticalAlignment(views::Textfield::Alignment::ALIGN_CENTER);
  search_field_ = toolbar->AddChildView(std::move(search));
  search_field_->SetProperty(views::kMarginsKey,
                             gfx::Insets::VH(0, kToolbarSpacing));
  search_field_->SetPreferredSize(gfx::Size(kSearchFieldWidth, 32));

  // Sort combobox.
  auto sort_combobox = std::make_unique<views::Combobox>(
      std::make_unique<ui::SimpleComboboxModel>(std::vector<ui::SimpleComboboxModel::Item>{
          ui::SimpleComboboxModel::Item(u"Sort by name"),
          ui::SimpleComboboxModel::Item(u"Sort by install date"),
          ui::SimpleComboboxModel::Item(u"Sort by recent usage"),
          ui::SimpleComboboxModel::Item(u"Sort by permission level"),
      }));
  sort_combobox->SetCallback(base::BindRepeating(
      &AstraExtensionsPageView::OnSortChanged, base::Unretained(this)));
  sort_combobox_ = toolbar->AddChildView(std::move(sort_combobox));
  sort_combobox_->SetProperty(views::kMarginsKey,
                              gfx::Insets::VH(0, kToolbarSpacing));

  // View mode toggle.
  auto view_mode = std::make_unique<views::ImageButton>();
  view_mode->SetMinSize(gfx::Size(32, 32));
  view_mode->SetTooltipText(u"Toggle view mode");
  view_mode->SetAccessibleName(u"Toggle grid/list view");
  view_mode->SetCallback(base::BindRepeating(
      &AstraExtensionsPageView::OnDisplayModeToggled,
      base::Unretained(this)));
  view_mode_button_ = toolbar->AddChildView(std::move(view_mode));
  view_mode_button_->SetProperty(views::kMarginsKey,
                                 gfx::Insets::VH(0, kToolbarSpacing));

  // Spacer to push buttons to right.
  auto spacer = std::make_unique<views::View>();
  toolbar->AddChildView(std::move(spacer));
  layout->SetFlexForView(spacer.get(), 1);

  // Web Store button.
  auto webstore_btn = views::MdTextButton::CreateSecondaryUiButton(
      base::BindRepeating(
          &AstraExtensionsPageView::OnOpenWebStore, base::Unretained(this)),
      u"Chrome Web Store");
  webstore_button_ = toolbar->AddChildView(std::move(webstore_btn));
  webstore_button_->SetProperty(views::kMarginsKey,
                                gfx::Insets::VH(0, kToolbarSpacing));

  // Add extension button.
  auto add_btn = views::MdTextButton::CreatePrimaryUiButton(
      base::BindRepeating(
          &AstraExtensionsPageView::OnAddExtension, base::Unretained(this)),
      u"Add extension");
  add_button_ = toolbar->AddChildView(std::move(add_btn));

  toolbar_ = AddChildView(std::move(toolbar));
}

void AstraExtensionsPageView::BuildCategoriesSidebar() {
  auto sidebar = std::make_unique<views::View>();
  sidebar->SetBackground(
      views::CreateSolidBackground(SkColorSetRGB(0xF8, 0xF9, 0xFA)));
  sidebar->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 0, 1, SkColorSetA(SK_ColorBLACK, 0x0D)));

  auto* layout = sidebar->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kVertical);

  // Header.
  auto header = std::make_unique<views::Label>();
  header->SetText(u"Categories");
  header->SetFontList(views::style::GetFont(
      views::style::CONTEXT_LABEL, views::style::STYLE_EMPHASIZED));
  header->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header->SetAutoColorReadabilityEnabled(false);
  categories_header_ = sidebar->AddChildView(std::move(header));
  categories_header_->SetProperty(views::kMarginsKey,
                                  gfx::Insets::VH(16, 16));

  // Categories container.
  auto container = std::make_unique<views::View>();
  container->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  categories_container_ = sidebar->AddChildView(std::move(container));
  layout->SetFlexForView(categories_container_, 1);

  categories_sidebar_ = AddChildView(std::move(sidebar));
}

void AstraExtensionsPageView::BuildContentArea() {
  auto scroll_view = std::make_unique<views::ScrollView>();
  scroll_view->SetBackgroundColor(SK_ColorTRANSPARENT);
  scroll_view->SetDrawOverflowIndicator(false);

  auto content = std::make_unique<views::View>();
  content->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(16), 16));

  extensions_container_ = content.get();
  scroll_view->SetContents(std::move(content));

  content_scroll_view_ = AddChildView(std::move(scroll_view));

  BuildEmptyState();
}

void AstraExtensionsPageView::BuildEmptyState() {
  auto empty = std::make_unique<views::View>();
  auto* layout = empty->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kVertical);
  layout->SetMainAxisAlignment(views::LayoutAlignment::kCenter);
  layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);

  auto title = std::make_unique<views::Label>();
  title->SetText(u"No extensions found");
  title->SetFontList(views::style::GetFont(
      views::style::CONTEXT_DIALOG_TITLE, views::style::STYLE_PRIMARY));
  title->SetAutoColorReadabilityEnabled(false);
  empty_state_title_ = empty->AddChildView(std::move(title));
  empty_state_title_->SetProperty(views::kMarginsKey,
                                  gfx::Insets::VH(0, 0, 8, 0));

  auto desc = std::make_unique<views::Label>();
  desc->SetText(u"Try a different search or category");
  desc->SetFontList(views::style::GetFont(
      views::style::CONTEXT_LABEL, views::style::STYLE_SECONDARY));
  desc->SetAutoColorReadabilityEnabled(false);
  empty_state_desc_ = empty->AddChildView(std::move(desc));

  empty_state_ = extensions_container_->AddChildView(std::move(empty));
  empty_state_->SetVisible(false);
}

void AstraExtensionsPageView::RefreshExtensionCards() {
  if (!model_ || !extensions_container_) {
    return;
  }

  // Remove old cards.
  extension_cards_.clear();

  // Remove all child views except empty state from container.
  // Actually let's just clear everything and rebuild.
  // We need to keep empty_state_ pointer valid, so save it.
  bool empty_visible = empty_state_ ? empty_state_->GetVisible() : false;
  extensions_container_->RemoveAllChildViews();
  empty_state_ = nullptr;
  empty_state_title_ = nullptr;
  empty_state_desc_ = nullptr;

  // Recreate empty state.
  BuildEmptyState();

  auto filtered = model_->GetFilteredExtensions();

  if (filtered.empty()) {
    if (!model_->GetSearchQuery().empty()) {
      empty_state_title_->SetText(u"No extensions match your search");
      empty_state_desc_->SetText(u"Try a different search term");
    } else if (!model_->GetCategoryFilter().empty()) {
      empty_state_title_->SetText(
          u"No extensions in this category");
      empty_state_desc_->SetText(u"Try a different category");
    } else {
      empty_state_title_->SetText(u"No extensions installed");
      empty_state_desc_->SetText(
          u"Visit the Chrome Web Store to add extensions");
    }
    empty_state_->SetVisible(true);
    return;
  }

  empty_state_->SetVisible(false);

  // Add cards in a grid flow layout.
  // Using BoxLayout horizontal inside a Flex container won't give us wrapping.
  // For simplicity, we'll use a vertical layout with rows.
  // But for scaffold purposes, let's use a vertical list of cards.
  // TODO(astra): Use FlowLayout or GridLayout for proper grid.

  for (const auto& ext : filtered) {
    auto card = std::make_unique<AstraExtensionCardView>(ext.id);
    card->SetCompact(compact_mode_);
    card->UpdateFromModel(model_);
    extension_cards_.push_back(
        extensions_container_->AddChildView(std::move(card)));
  }

  // Update layout.
  extensions_container_->InvalidateLayout();
}

void AstraExtensionsPageView::RefreshCategories() {
  if (!model_ || !categories_container_) {
    return;
  }

  categories_container_->RemoveAllChildViews();

  // "All extensions" item.
  auto all_item = std::make_unique<views::Label>();
  all_item->SetText(u"All extensions");
  all_item->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  all_item->SetAutoColorReadabilityEnabled(false);
  all_item->SetFontList(views::style::GetFont(
      views::style::CONTEXT_LABEL, views::style::STYLE_PRIMARY));
  all_item->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(8, 16)));
  auto* all_label =
      categories_container_->AddChildView(std::move(all_item));
  all_label->SetBackground(
      views::CreateSolidBackground(SkColorSetRGB(0xE8, 0xF0, 0xFE)));

  auto categories = model_->GetCategories();
  for (const auto& cat : categories) {
    auto item = std::make_unique<views::Label>();
    item->SetText(cat.name + u" (" +
                  base::NumberToString16(cat.count) + u")");
    item->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    item->SetAutoColorReadabilityEnabled(false);
    item->SetFontList(views::style::GetFont(
        views::style::CONTEXT_LABEL, views::style::STYLE_SECONDARY));
    item->SetBorder(views::CreateEmptyBorder(
        gfx::Insets::VH(8, 24)));
    categories_container_->AddChildView(std::move(item));
  }

  categories_container_->InvalidateLayout();
}

void AstraExtensionsPageView::UpdateEmptyState() {
  if (!model_ || !extensions_container_) {
    return;
  }

  auto filtered = model_->GetFilteredExtensions();
  if (empty_state_) {
    empty_state_->SetVisible(filtered.empty());
  }
}

void AstraExtensionsPageView::OnSortChanged() {
  if (!model_ || !sort_combobox_) {
    return;
  }
  int index = sort_combobox_->GetSelectedIndex().value_or(0);
  AstraExtensionSortType sort_type = AstraExtensionSortType::kName;
  switch (index) {
    case 0: sort_type = AstraExtensionSortType::kName; break;
    case 1: sort_type = AstraExtensionSortType::kInstallDate; break;
    case 2: sort_type = AstraExtensionSortType::kRecentUsage; break;
    case 3: sort_type = AstraExtensionSortType::kPermissionLevel; break;
  }
  model_->SetSortType(sort_type);
}

void AstraExtensionsPageView::OnDisplayModeToggled() {
  SetDisplayMode(!compact_mode_);
}

void AstraExtensionsPageView::OnCategorySelected(
    const std::string& category_id) {
  if (!model_) {
    return;
  }
  selected_category_ = category_id;
  model_->SetCategoryFilter(category_id);
}

void AstraExtensionsPageView::OnAddExtension() {
  if (delegate_) {
    delegate_->OnInstallExtension();
  }
}

void AstraExtensionsPageView::OnOpenWebStore() {
  if (delegate_) {
    delegate_->OnOpenChromeWebStore();
  }
  if (model_) {
    model_->OpenChromeWebStore();
  }
}

void AstraExtensionsPageView::OnManageShortcuts() {
  if (delegate_) {
    delegate_->OnManageShortcuts();
  }
  if (model_) {
    model_->ManageShortcuts();
  }
}

void AstraExtensionsPageView::DrawExtensionIcon(gfx::Canvas* canvas,
                                                  const gfx::Rect& bounds,
                                                  SkColor color,
                                                  const std::u16string& name) {
  DrawPuzzleIcon(canvas, bounds, color);
}

}  // namespace astra
