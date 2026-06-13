// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_READER_MODE_ASTRA_READER_MODE_VIEW_H_
#define ASTRA_UI_VIEWS_READER_MODE_ASTRA_READER_MODE_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"

namespace views {
class BoxLayout;
class Label;
class MdTextButton;
class Checkbox;
class Slider;
}  // namespace views

namespace astra {

// =========================================================================
// AstraReaderModeView — reader mode settings panel
// =========================================================================
//
// A bubble showing controls for distraction-free reading mode:
// font size, line spacing, theme (light/dark/sepia), font family,
// and focus mode options.
//
// Layout:
//   +-------------------------------------------+
//   |  Reader Mode                  [Close]    |
//   +-------------------------------------------+
//   |  [ Enter Reader Mode ]                     |
//   +-------------------------------------------+
//   |  Appearance                               |
//   |  Theme: [☀️ Light] [🌙 Dark] [📜 Sepia] |
//   |  Font size:  [-] 16 [+]                   |
//   |  Line spacing: [──●───]                   |
//   +-------------------------------------------+
//   |  Font: [ Serif  ▼]                        |
//   +-------------------------------------------+
//   |  Focus mode                               |
//   |  [x] Hide images                          |
//   |  [x] Highlight current line               |
//   |  [ ] Auto-scroll                          |
//   +-------------------------------------------+
//   |  Text to speech                           |
//   |  [ ▶️ Play ]  Speed: 1.0x                 |
//   +-------------------------------------------+
//
// This is a presentation-only view. Reader mode rendering is handled by
// Chromium's DOM Distiller / Reader Mode.
//
// Chromium subsystems reused:
//   - views::BubbleDialogDelegateView
//   - views::Slider / views::Checkbox
//   - DOM Distiller (reader mode engine)
// =========================================================================

class AstraReaderModeView : public views::BubbleDialogDelegateView {
 public:
  enum class Theme { kLight, kDark, kSepia, kSystem };
  enum class FontFamily { kSerif, kSansSerif, kMonospace };

  using ThemeChangedCallback =
      base::RepeatingCallback<void(Theme theme)>;
  using FontSizeChangedCallback =
      base::RepeatingCallback<void(int size_pt)>;
  using LineSpacingChangedCallback =
      base::RepeatingCallback<void(double multiplier)>;
  using FontChangedCallback =
      base::RepeatingCallback<void(FontFamily font)>;
  using ToggleCallback = base::RepeatingCallback<void(bool enabled)>;
  using EnterReaderModeCallback = base::RepeatingClosure;

  explicit AstraReaderModeView(views::View* anchor_view);
  ~AstraReaderModeView() override;

  AstraReaderModeView(const AstraReaderModeView&) = delete;
  AstraReaderModeView& operator=(const AstraReaderModeView&) = delete;

  // -- Settings ------------------------------------------------------------

  void SetTheme(Theme theme);
  void SetFontSize(int size_pt);
  void SetLineSpacing(double multiplier);
  void SetFontFamily(FontFamily font);

  void SetHideImages(bool hide);
  void SetHighlightLine(bool highlight);
  void SetAutoScroll(bool auto_scroll);

  void SetReaderModeAvailable(bool available);

  // -- Callbacks -----------------------------------------------------------

  void SetThemeChangedCallback(ThemeChangedCallback callback);
  void SetFontSizeChangedCallback(FontSizeChangedCallback callback);
  void SetLineSpacingChangedCallback(LineSpacingChangedCallback callback);
  void SetFontChangedCallback(FontChangedCallback callback);
  void SetHideImagesCallback(ToggleCallback callback);
  void SetHighlightLineCallback(ToggleCallback callback);
  void SetAutoScrollCallback(ToggleCallback callback);
  void SetEnterReaderModeCallback(EnterReaderModeCallback callback);

  // -- views::BubbleDialogDelegateView -------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;

 private:
  void BuildUI();
  void BuildEnterButton();
  void BuildAppearanceSection();
  void BuildFocusModeSection();
  void BuildTextToSpeechSection();

  void OnEnterReaderMode();
  void OnThemeLight();
  void OnThemeDark();
  void OnThemeSepia();
  void OnFontSizeIncrease();
  void OnFontSizeDecrease();
  void OnLineSpacingChanged(double value);
  void OnFontFamilyChanged();
  void OnHideImagesToggled();
  void OnHighlightLineToggled();
  void OnAutoScrollToggled();
  void OnPlayTTS();

  // State.
  Theme theme_ = Theme::kLight;
  int font_size_pt_ = 16;
  double line_spacing_ = 1.5;
  FontFamily font_family_ = FontFamily::kSerif;
  bool hide_images_ = false;
  bool highlight_line_ = false;
  bool auto_scroll_ = false;
  bool reader_mode_available_ = true;
  bool tts_playing_ = false;

  // Callbacks.
  ThemeChangedCallback theme_callback_;
  FontSizeChangedCallback font_size_callback_;
  LineSpacingChangedCallback line_spacing_callback_;
  FontChangedCallback font_callback_;
  ToggleCallback hide_images_callback_;
  ToggleCallback highlight_callback_;
  ToggleCallback auto_scroll_callback_;
  EnterReaderModeCallback enter_callback_;

  // Child views.
  raw_ptr<views::MdTextButton> enter_button_ = nullptr;
  raw_ptr<views::MdTextButton> theme_light_button_ = nullptr;
  raw_ptr<views::MdTextButton> theme_dark_button_ = nullptr;
  raw_ptr<views::MdTextButton> theme_sepia_button_ = nullptr;
  raw_ptr<views::MdTextButton> font_size_dec_button_ = nullptr;
  raw_ptr<views::Label> font_size_label_ = nullptr;
  raw_ptr<views::MdTextButton> font_size_inc_button_ = nullptr;
  raw_ptr<views::View> font_family_button_ = nullptr;
  raw_ptr<views::Checkbox> hide_images_checkbox_ = nullptr;
  raw_ptr<views::Checkbox> highlight_checkbox_ = nullptr;
  raw_ptr<views::Checkbox> auto_scroll_checkbox_ = nullptr;
  raw_ptr<views::MdTextButton> tts_play_button_ = nullptr;
  raw_ptr<views::Label> tts_speed_label_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_READER_MODE_ASTRA_READER_MODE_VIEW_H_
