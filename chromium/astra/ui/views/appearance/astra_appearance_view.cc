// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/appearance/astra_appearance_view.h"

#include <algorithm>
#include <memory>
#include <string>

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
#include "ui/views/controls/checkbox.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/style/typography.h"
#include "ui/views/view_class_properties.h"

namespace astra {

namespace {

constexpr int kBubbleWidth = 360;
constexpr int kSectionPadding = 16;
constexpr int kCardWidth = 72;
constexpr int kCardHeight = 80;
constexpr int kCardSpacing = 10;
constexpr int kPreviewHeight = 50;

}  // namespace

// ===========================================================================
// AstraThemeCardView
// ===========================================================================

AstraThemeCardView::AstraThemeCardView(
    const ThemeInfo& info,
    SelectCallback select_callback)
    : theme_id_(info.theme_id),
      name_(info.name),
      background_color_(info.background_color),
      text_color_(info.text_color),
      accent_color_(info.accent_color),
      toolbar_color_(info.toolbar_color),
      is_selected_(info.is_selected),
      is_dark_(info.is_dark),
      select_callback_(std::move(select_callback)) {
  BuildLayout();
}

AstraThemeCardView::~AstraThemeCardView() = default;

void AstraThemeCardView::SetSelected(bool selected) {
  is_selected_ = selected;
  SchedulePaint();
}

void AstraThemeCardView::BuildLayout() {
  SetPreferredSize(gfx::Size(kCardWidth, kCardHeight));
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(6, 6),
      6));
  SetBorder(views::CreateRoundedRectBorder(
      is_selected_ ? 3 : 1, 8,
      is_selected_ ? SkColorSetRGB(0x1A, 0x73, 0xE8) : SK_ColorGRAY));
  SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  name_label_ = AddChildView(
      std::make_unique<views::Label>(name_));
  name_label_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  name_label_->SetAutoColorReadabilityEnabled(false);
  name_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
  name_label_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kPreferred));
}

void AstraThemeCardView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);
  PaintPreview(canvas);
}

void AstraThemeCardView::PaintPreview(gfx::Canvas* canvas) {
  gfx::Rect bounds = GetLocalBounds();
  int preview_x = 6;
  int preview_y = 6;
  int preview_w = bounds.width() - 12;
  int preview_h = kPreviewHeight;

  // Outer rounded rect (window frame).
  SkPaint frame_paint;
  frame_paint.setColor(toolbar_color_);
  frame_paint.setStyle(SkPaint::kFill_Style);
  frame_paint.setAntiAlias(true);

  SkRRect rrect;
  SkRect rect = SkRect::MakeXYWH(preview_x, preview_y, preview_w, preview_h);
  rrect.setRectXY(rect, 4, 4);
  canvas->drawRRect(rrect, frame_paint);

  // Toolbar accent line.
  SkPaint accent_paint;
  accent_paint.setColor(accent_color_);
  accent_paint.setStyle(SkPaint::kFill_Style);
  accent_paint.setAntiAlias(true);

  SkRect accent_rect =
      SkRect::MakeXYWH(preview_x + 4, preview_y + 4,
                       preview_w - 8, 6);
  canvas->drawRoundRect(accent_rect, 2, 2, accent_paint);

  // Content area.
  SkPaint content_paint;
  content_paint.setColor(background_color_);
  content_paint.setStyle(SkPaint::kFill_Style);
  content_paint.setAntiAlias(true);

  SkRect content_rect =
      SkRect::MakeXYWH(preview_x + 2, preview_y + 14,
                       preview_w - 4, preview_h - 16);
  canvas->drawRect(content_rect, content_paint);

  // Text lines (simulated).
  SkPaint text_paint;
  text_paint.setColor(text_color_);
  text_paint.setStyle(SkPaint::kFill_Style);
  text_paint.setAlpha(0x40);
  text_paint.setAntiAlias(true);

  int line_y = preview_y + 20;
  for (int i = 0; i < 3; i++) {
    int line_w = preview_w - 8 - (i * 10);
    if (line_w < 10) line_w = 10;
    SkRect line = SkRect::MakeXYWH(
        preview_x + 4, line_y, line_w, 2);
    canvas->drawRoundRect(line, 1, 1, text_paint);
    line_y += 6;
  }
}

void AstraThemeCardView::OnClicked() {
  if (select_callback_) {
    select_callback_.Run(theme_id_);
  }
}

void AstraThemeCardView::OnThemeChanged() {
  views::View::OnThemeChanged();
  SchedulePaint();
}

// ===========================================================================
// AstraAppearanceView
// ===========================================================================

AstraAppearanceView::AstraAppearanceView(views::View* anchor_view)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_RIGHT) {
  SetShowCloseButton(true);
  SetButtons(ui::DIALOG_BUTTON_NONE);
  set_fixed_width(kBubbleWidth);

  BuildUI();
}

AstraAppearanceView::~AstraAppearanceView() = default;

void AstraAppearanceView::SetThemes(
    const std::vector<AstraThemeCardView::ThemeInfo>& themes) {
  themes_ = themes;
  RefreshThemes();
}

void AstraAppearanceView::SetSelectedTheme(
    const std::string& theme_id) {
  selected_theme_id_ = theme_id;
  for (auto& theme : themes_) {
    theme.is_selected = (theme.theme_id == theme_id);
  }
  RefreshThemes();
}

void AstraAppearanceView::SetFontSize(FontSizeLevel level) {
  font_size_ = level;

  auto set_prominent = [](views::MdTextButton* btn, bool prominent) {
    if (btn) {
      btn->SetProminent(prominent);
    }
  };

  set_prominent(font_small_button_, font_size_ == FontSizeLevel::kSmall);
  set_prominent(font_medium_button_, font_size_ == FontSizeLevel::kMedium);
  set_prominent(font_large_button_, font_size_ == FontSizeLevel::kLarge);
  set_prominent(font_xlarge_button_,
                font_size_ == FontSizeLevel::kExtraLarge);
}

void AstraAppearanceView::SetCompactMode(bool enabled) {
  compact_mode_ = enabled;
  if (compact_checkbox_) {
    compact_checkbox_->SetChecked(enabled);
  }
}

void AstraAppearanceView::SetShowHomeButton(bool show) {
  show_home_button_ = show;
  if (home_checkbox_) {
    home_checkbox_->SetChecked(show);
  }
}

void AstraAppearanceView::SetShowBookmarksBar(bool show) {
  show_bookmarks_bar_ = show;
  if (bookmarks_checkbox_) {
    bookmarks_checkbox_->SetChecked(show);
  }
}

void AstraAppearanceView::SetThemeSelectedCallback(
    ThemeSelectedCallback callback) {
  theme_callback_ = std::move(callback);
}

void AstraAppearanceView::SetFontSizeChangedCallback(
    FontSizeChangedCallback callback) {
  font_size_callback_ = std::move(callback);
}

void AstraAppearanceView::SetCompactModeCallback(
    ToggleCallback callback) {
  compact_callback_ = std::move(callback);
}

void AstraAppearanceView::SetShowHomeButtonCallback(
    ToggleCallback callback) {
  home_button_callback_ = std::move(callback);
}

void AstraAppearanceView::SetShowBookmarksBarCallback(
    ToggleCallback callback) {
  bookmarks_bar_callback_ = std::move(callback);
}

void AstraAppearanceView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  set_margins(gfx::Insets::VH(0, 0));

  BuildThemesSection();
  BuildFontSizeSection();
  BuildTogglesSection();
}

void AstraAppearanceView::BuildThemesSection() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 10));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  auto* header = section->AddChildView(
      std::make_unique<views::Label>(u"Themes"));
  header->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header->SetAutoColorReadabilityEnabled(false);
  header->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  themes_row_ = section->AddChildView(std::make_unique<views::View>());
  themes_row_->SetLayoutManager(
      std::make_unique<views::FlexLayout>());
  themes_row_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kPreferred));
}

void AstraAppearanceView::BuildFontSizeSection() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 10));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  auto* header = section->AddChildView(
      std::make_unique<views::Label>(u"Font size"));
  header->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header->SetAutoColorReadabilityEnabled(false);
  header->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  auto* buttons_row = section->AddChildView(
      std::make_unique<views::View>());
  buttons_row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(), 6));

  font_small_button_ = buttons_row->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraAppearanceView::OnFontSizeSmall,
              base::Unretained(this)),
          u"S"));
  font_small_button_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  font_medium_button_ = buttons_row->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraAppearanceView::OnFontSizeMedium,
              base::Unretained(this)),
          u"M"));
  font_medium_button_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));
  font_medium_button_->SetProminent(true);

  font_large_button_ = buttons_row->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraAppearanceView::OnFontSizeLarge,
              base::Unretained(this)),
          u"L"));
  font_large_button_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  font_xlarge_button_ = buttons_row->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraAppearanceView::OnFontSizeExtraLarge,
              base::Unretained(this)),
          u"XL"));
  font_xlarge_button_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));
}

void AstraAppearanceView::BuildTogglesSection() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 8));

  compact_checkbox_ = section->AddChildView(
      std::make_unique<views::Checkbox>(
          base::BindRepeating(
              &AstraAppearanceView::OnCompactModeToggled,
              base::Unretained(this)),
          u"Compact mode"));
  compact_checkbox_->SetChecked(compact_mode_);

  home_checkbox_ = section->AddChildView(
      std::make_unique<views::Checkbox>(
          base::BindRepeating(
              &AstraAppearanceView::OnShowHomeButtonToggled,
              base::Unretained(this)),
          u"Show home button"));
  home_checkbox_->SetChecked(show_home_button_);

  bookmarks_checkbox_ = section->AddChildView(
      std::make_unique<views::Checkbox>(
          base::BindRepeating(
              &AstraAppearanceView::OnShowBookmarksBarToggled,
              base::Unretained(this)),
          u"Show bookmarks bar"));
  bookmarks_checkbox_->SetChecked(show_bookmarks_bar_);
}

void AstraAppearanceView::RefreshThemes() {
  if (!themes_row_) return;

  themes_row_->RemoveAllChildViews();
  theme_cards_.clear();

  for (const auto& theme : themes_) {
    auto theme_info = theme;
    theme_info.is_selected = (theme.theme_id == selected_theme_id_);

    auto* card = themes_row_->AddChildView(
        std::make_unique<AstraThemeCardView>(
            theme_info,
            base::BindRepeating(
                &AstraAppearanceView::OnThemeSelected,
                base::Unretained(this))));
    theme_cards_.push_back(card);
  }

  InvalidateLayout();
}

void AstraAppearanceView::OnThemeSelected(
    const std::string& theme_id) {
  SetSelectedTheme(theme_id);
  if (theme_callback_) {
    theme_callback_.Run(theme_id);
  }
}

void AstraAppearanceView::OnFontSizeSmall() {
  SetFontSize(FontSizeLevel::kSmall);
  if (font_size_callback_) {
    font_size_callback_.Run(static_cast<int>(FontSizeLevel::kSmall));
  }
}

void AstraAppearanceView::OnFontSizeMedium() {
  SetFontSize(FontSizeLevel::kMedium);
  if (font_size_callback_) {
    font_size_callback_.Run(static_cast<int>(FontSizeLevel::kMedium));
  }
}

void AstraAppearanceView::OnFontSizeLarge() {
  SetFontSize(FontSizeLevel::kLarge);
  if (font_size_callback_) {
    font_size_callback_.Run(static_cast<int>(FontSizeLevel::kLarge));
  }
}

void AstraAppearanceView::OnFontSizeExtraLarge() {
  SetFontSize(FontSizeLevel::kExtraLarge);
  if (font_size_callback_) {
    font_size_callback_.Run(static_cast<int>(FontSizeLevel::kExtraLarge));
  }
}

void AstraAppearanceView::OnCompactModeToggled() {
  compact_mode_ = compact_checkbox_->GetChecked();
  if (compact_callback_) {
    compact_callback_.Run(compact_mode_);
  }
}

void AstraAppearanceView::OnShowHomeButtonToggled() {
  show_home_button_ = home_checkbox_->GetChecked();
  if (home_button_callback_) {
    home_button_callback_.Run(show_home_button_);
  }
}

void AstraAppearanceView::OnShowBookmarksBarToggled() {
  show_bookmarks_bar_ = bookmarks_checkbox_->GetChecked();
  if (bookmarks_bar_callback_) {
    bookmarks_bar_callback_.Run(show_bookmarks_bar_);
  }
}

std::u16string AstraAppearanceView::GetWindowTitle() const {
  return u"Appearance";
}

void AstraAppearanceView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
  // Theme cards repaint automatically.
}

}  // namespace astra
