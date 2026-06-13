// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/reader_mode/astra_reader_mode_view.h"

#include <memory>
#include <string>

#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkColor.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/slider.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/style/typography.h"
#include "ui/views/view_class_properties.h"

namespace astra {

namespace {

constexpr int kBubbleWidth = 320;
constexpr int kSectionPadding = 16;
constexpr int kMinFontSize = 10;
constexpr int kMaxFontSize = 32;

// Theme background colors (for preview).
SkColor ThemeBgColor(AstraReaderModeView::Theme theme) {
  switch (theme) {
    case AstraReaderModeView::Theme::kLight:
      return SK_ColorWHITE;
    case AstraReaderModeView::Theme::kDark:
      return SkColorSetRGB(32, 33, 36);
    case AstraReaderModeView::Theme::kSepia:
      return SkColorSetRGB(244, 236, 216);
    case AstraReaderModeView::Theme::kSystem:
      return SK_ColorWHITE;
  }
  return SK_ColorWHITE;
}

// Theme text colors (for preview).
SkColor ThemeTextColor(AstraReaderModeView::Theme theme) {
  switch (theme) {
    case AstraReaderModeView::Theme::kLight:
      return SK_ColorBLACK;
    case AstraReaderModeView::Theme::kDark:
      return SK_ColorWHITE;
    case AstraReaderModeView::Theme::kSepia:
      return SkColorSetRGB(60, 40, 20);
    case AstraReaderModeView::Theme::kSystem:
      return SK_ColorBLACK;
  }
  return SK_ColorBLACK;
}

// Font family name.
std::string FontFamilyName(AstraReaderModeView::FontFamily font) {
  switch (font) {
    case AstraReaderModeView::FontFamily::kSerif:
      return "Serif";
    case AstraReaderModeView::FontFamily::kSansSerif:
      return "Sans-serif";
    case AstraReaderModeView::FontFamily::kMonospace:
      return "Monospace";
  }
  return "Serif";
}

}  // namespace

// ===========================================================================
// AstraReaderModeView
// ===========================================================================

AstraReaderModeView::AstraReaderModeView(views::View* anchor_view)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_RIGHT) {
  SetShowCloseButton(true);
  SetButtons(ui::DIALOG_BUTTON_NONE);
  set_fixed_width(kBubbleWidth);

  BuildUI();
}

AstraReaderModeView::~AstraReaderModeView() = default;

void AstraReaderModeView::SetTheme(Theme theme) {
  theme_ = theme;
  if (theme_light_button_) theme_light_button_->SetProminent(theme == Theme::kLight);
  if (theme_dark_button_) theme_dark_button_->SetProminent(theme == Theme::kDark);
  if (theme_sepia_button_) theme_sepia_button_->SetProminent(theme == Theme::kSepia);
}

void AstraReaderModeView::SetFontSize(int size_pt) {
  font_size_pt_ = std::clamp(size_pt, kMinFontSize, kMaxFontSize);
  if (font_size_label_) {
    font_size_label_->SetText(
        base::UTF8ToUTF16(std::to_string(font_size_pt_) + " pt"));
  }
}

void AstraReaderModeView::SetLineSpacing(double multiplier) {
  line_spacing_ = std::clamp(multiplier, 1.0, 2.5);
  // TODO(astra): Update slider.
}

void AstraReaderModeView::SetFontFamily(FontFamily font) {
  font_family_ = font;
  // TODO(astra): Update font family display.
}

void AstraReaderModeView::SetHideImages(bool hide) {
  hide_images_ = hide;
  if (hide_images_checkbox_) {
    hide_images_checkbox_->SetChecked(hide);
  }
}

void AstraReaderModeView::SetHighlightLine(bool highlight) {
  highlight_line_ = highlight;
  if (highlight_checkbox_) {
    highlight_checkbox_->SetChecked(highlight);
  }
}

void AstraReaderModeView::SetAutoScroll(bool auto_scroll) {
  auto_scroll_ = auto_scroll;
  if (auto_scroll_checkbox_) {
    auto_scroll_checkbox_->SetChecked(auto_scroll);
  }
}

void AstraReaderModeView::SetReaderModeAvailable(bool available) {
  reader_mode_available_ = available;
  if (enter_button_) {
    enter_button_->SetEnabled(available);
  }
}

void AstraReaderModeView::SetThemeChangedCallback(
    ThemeChangedCallback callback) {
  theme_callback_ = std::move(callback);
}

void AstraReaderModeView::SetFontSizeChangedCallback(
    FontSizeChangedCallback callback) {
  font_size_callback_ = std::move(callback);
}

void AstraReaderModeView::SetLineSpacingChangedCallback(
    LineSpacingChangedCallback callback) {
  line_spacing_callback_ = std::move(callback);
}

void AstraReaderModeView::SetFontChangedCallback(
    FontChangedCallback callback) {
  font_callback_ = std::move(callback);
}

void AstraReaderModeView::SetHideImagesCallback(ToggleCallback callback) {
  hide_images_callback_ = std::move(callback);
}

void AstraReaderModeView::SetHighlightLineCallback(ToggleCallback callback) {
  highlight_callback_ = std::move(callback);
}

void AstraReaderModeView::SetAutoScrollCallback(ToggleCallback callback) {
  auto_scroll_callback_ = std::move(callback);
}

void AstraReaderModeView::SetEnterReaderModeCallback(
    EnterReaderModeCallback callback) {
  enter_callback_ = std::move(callback);
}

void AstraReaderModeView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  set_margins(gfx::Insets::VH(0, 0));

  BuildEnterButton();
  BuildAppearanceSection();
  BuildFocusModeSection();
  BuildTextToSpeechSection();
}

void AstraReaderModeView::BuildEnterButton() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(16, kSectionPadding), 0));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  enter_button_ = section->AddChildView(
      views::MdTextButton::CreateProminent(
          base::BindRepeating(
              &AstraReaderModeView::OnEnterReaderMode,
              base::Unretained(this)),
          u"📖 Enter Reader Mode"));
  enter_button_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));
}

void AstraReaderModeView::BuildAppearanceSection() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 10));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  auto* header = section->AddChildView(
      std::make_unique<views::Label>(u"Appearance"));
  header->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header->SetAutoColorReadabilityEnabled(false);
  header->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  // Theme buttons row.
  auto* theme_row = section->AddChildView(std::make_unique<views::View>());
  theme_row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(), 6));

  theme_light_button_ = theme_row->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraReaderModeView::OnThemeLight,
              base::Unretained(this)),
          u"☀️ Light"));
  theme_light_button_->SetProminent(theme_ == Theme::kLight);
  theme_light_button_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  theme_dark_button_ = theme_row->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraReaderModeView::OnThemeDark,
              base::Unretained(this)),
          u"🌙 Dark"));
  theme_dark_button_->SetProminent(theme_ == Theme::kDark);
  theme_dark_button_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  theme_sepia_button_ = theme_row->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraReaderModeView::OnThemeSepia,
              base::Unretained(this)),
          u"📜 Sepia"));
  theme_sepia_button_->SetProminent(theme_ == Theme::kSepia);
  theme_sepia_button_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  // Font size row.
  auto* font_size_row = section->AddChildView(std::make_unique<views::View>());
  font_size_row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(), 8));
  font_size_row->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  auto* font_size_label_title = font_size_row->AddChildView(
      std::make_unique<views::Label>(u"Font size"));
  font_size_label_title->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  font_size_label_title->SetAutoColorReadabilityEnabled(false);
  font_size_label_title->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));

  auto* spacer = font_size_row->AddChildView(std::make_unique<views::View>());
  spacer->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  font_size_dec_button_ = font_size_row->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraReaderModeView::OnFontSizeDecrease,
              base::Unretained(this)),
          u"−"));

  font_size_label_ = font_size_row->AddChildView(
      std::make_unique<views::Label>(
          base::UTF8ToUTF16(std::to_string(font_size_pt_) + " pt")));
  font_size_label_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  font_size_label_->SetAutoColorReadabilityEnabled(false);
  font_size_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));
  font_size_label_->SetPreferredSize(gfx::Size(50, 20));

  font_size_inc_button_ = font_size_row->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraReaderModeView::OnFontSizeIncrease,
              base::Unretained(this)),
          u"+"));

  // Font family row.
  auto* font_row = section->AddChildView(std::make_unique<views::View>());
  font_row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(), 8));
  font_row->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  auto* font_label = font_row->AddChildView(
      std::make_unique<views::Label>(u"Font"));
  font_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  font_label->SetAutoColorReadabilityEnabled(false);
  font_label->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));

  auto* font_spacer = font_row->AddChildView(std::make_unique<views::View>());
  font_spacer->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  font_family_button_ = font_row->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraReaderModeView::OnFontFamilyChanged,
              base::Unretained(this)),
          base::UTF8ToUTF16(FontFamilyName(font_family_) + " ▼")));
}

void AstraReaderModeView::BuildFocusModeSection() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 8));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  auto* header = section->AddChildView(
      std::make_unique<views::Label>(u"Focus mode"));
  header->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header->SetAutoColorReadabilityEnabled(false);
  header->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  hide_images_checkbox_ = section->AddChildView(
      std::make_unique<views::Checkbox>(
          base::BindRepeating(
              &AstraReaderModeView::OnHideImagesToggled,
              base::Unretained(this)),
          u"Hide images"));
  hide_images_checkbox_->SetChecked(hide_images_);

  highlight_checkbox_ = section->AddChildView(
      std::make_unique<views::Checkbox>(
          base::BindRepeating(
              &AstraReaderModeView::OnHighlightLineToggled,
              base::Unretained(this)),
          u"Highlight current line"));
  highlight_checkbox_->SetChecked(highlight_line_);

  auto_scroll_checkbox_ = section->AddChildView(
      std::make_unique<views::Checkbox>(
          base::BindRepeating(
              &AstraReaderModeView::OnAutoScrollToggled,
              base::Unretained(this)),
          u"Auto-scroll"));
  auto_scroll_checkbox_->SetChecked(auto_scroll_);
}

void AstraReaderModeView::BuildTextToSpeechSection() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 8));

  auto* header = section->AddChildView(
      std::make_unique<views::Label>(u"Text to speech"));
  header->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header->SetAutoColorReadabilityEnabled(false);
  header->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  auto* tts_row = section->AddChildView(std::make_unique<views::View>());
  tts_row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(), 8));
  tts_row->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  tts_play_button_ = tts_row->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraReaderModeView::OnPlayTTS,
              base::Unretained(this)),
          u"▶️ Play"));

  tts_speed_label_ = tts_row->AddChildView(
      std::make_unique<views::Label>(u"Speed: 1.0x"));
  tts_speed_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  tts_speed_label_->SetAutoColorReadabilityEnabled(false);
  tts_speed_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
}

void AstraReaderModeView::OnEnterReaderMode() {
  if (enter_callback_) {
    enter_callback_.Run();
  }
}

void AstraReaderModeView::OnThemeLight() {
  SetTheme(Theme::kLight);
  if (theme_callback_) {
    theme_callback_.Run(Theme::kLight);
  }
}

void AstraReaderModeView::OnThemeDark() {
  SetTheme(Theme::kDark);
  if (theme_callback_) {
    theme_callback_.Run(Theme::kDark);
  }
}

void AstraReaderModeView::OnThemeSepia() {
  SetTheme(Theme::kSepia);
  if (theme_callback_) {
    theme_callback_.Run(Theme::kSepia);
  }
}

void AstraReaderModeView::OnFontSizeIncrease() {
  SetFontSize(font_size_pt_ + 1);
  if (font_size_callback_) {
    font_size_callback_.Run(font_size_pt_);
  }
}

void AstraReaderModeView::OnFontSizeDecrease() {
  SetFontSize(font_size_pt_ - 1);
  if (font_size_callback_) {
    font_size_callback_.Run(font_size_pt_);
  }
}

void AstraReaderModeView::OnLineSpacingChanged(double value) {
  line_spacing_ = value;
  if (line_spacing_callback_) {
    line_spacing_callback_.Run(value);
  }
}

void AstraReaderModeView::OnFontFamilyChanged() {
  // Cycle through font families.
  FontFamily next = FontFamily::kSerif;
  switch (font_family_) {
    case FontFamily::kSerif:
      next = FontFamily::kSansSerif;
      break;
    case FontFamily::kSansSerif:
      next = FontFamily::kMonospace;
      break;
    case FontFamily::kMonospace:
      next = FontFamily::kSerif;
      break;
  }
  font_family_ = next;

  if (font_family_button_) {
    font_family_button_ = nullptr;  // Will be recreated on rebuild.
    // TODO(astra): Update button text instead of rebuilding.
  }

  if (font_callback_) {
    font_callback_.Run(font_family_);
  }
}

void AstraReaderModeView::OnHideImagesToggled() {
  hide_images_ = hide_images_checkbox_->GetChecked();
  if (hide_images_callback_) {
    hide_images_callback_.Run(hide_images_);
  }
}

void AstraReaderModeView::OnHighlightLineToggled() {
  highlight_line_ = highlight_checkbox_->GetChecked();
  if (highlight_callback_) {
    highlight_callback_.Run(highlight_line_);
  }
}

void AstraReaderModeView::OnAutoScrollToggled() {
  auto_scroll_ = auto_scroll_checkbox_->GetChecked();
  if (auto_scroll_callback_) {
    auto_scroll_callback_.Run(auto_scroll_);
  }
}

void AstraReaderModeView::OnPlayTTS() {
  tts_playing_ = !tts_playing_;
  if (tts_play_button_) {
    tts_play_button_->SetText(tts_playing_ ? u"⏸️ Pause" : u"▶️ Play");
  }
  // TODO(astra): Integrate with Chromium text-to-speech.
}

std::u16string AstraReaderModeView::GetWindowTitle() const {
  return u"Reader Mode";
}

void AstraReaderModeView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;
}

}  // namespace astra
