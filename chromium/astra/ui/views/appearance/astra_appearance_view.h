// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_APPEARANCE_ASTRA_APPEARANCE_VIEW_H_
#define ASTRA_UI_VIEWS_APPEARANCE_ASTRA_APPEARANCE_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"

namespace views {
class BoxLayout;
class Checkbox;
class Label;
class MdTextButton;
class Slider;
}  // namespace views

namespace astra {

// =========================================================================
// AstraThemeCardView — single theme option card
// =========================================================================
//
// A card showing one theme option with a preview of colors.
//
// Layout:
//   +------------+
//   |  ████████  |
//   |  ██    ██  |
//   |  ████████  |
//   |            |
//   |  Light     |
//   +------------+
// =========================================================================

class AstraThemeCardView : public views::View {
 public:
  using SelectCallback =
      base::RepeatingCallback<void(const std::string& theme_id)>;

  struct ThemeInfo {
    std::string theme_id;
    std::u16string name;
    SkColor background_color = SK_ColorWHITE;
    SkColor text_color = SK_ColorBLACK;
    SkColor accent_color = SkColorSetRGB(0x1A, 0x73, 0xE8);
    SkColor toolbar_color = SkColorSetRGB(0xF1, 0xF3, 0xF4);
    bool is_selected = false;
    bool is_dark = false;
  };

  AstraThemeCardView(const ThemeInfo& info,
                     SelectCallback select_callback);
  ~AstraThemeCardView() override;

  AstraThemeCardView(const AstraThemeCardView&) = delete;
  AstraThemeCardView& operator=(const AstraThemeCardView&) = delete;

  const std::string& theme_id() const { return theme_id_; }
  bool is_selected() const { return is_selected_; }

  void SetSelected(bool selected);

  // -- views::View ---------------------------------------------------------

  void OnPaint(gfx::Canvas* canvas) override;
  void OnThemeChanged() override;

 private:
  void BuildLayout();
  void OnClicked();
  void PaintPreview(gfx::Canvas* canvas);

  std::string theme_id_;
  std::u16string name_;
  SkColor background_color_ = SK_ColorWHITE;
  SkColor text_color_ = SK_ColorBLACK;
  SkColor accent_color_ = SkColorSetRGB(0x1A, 0x73, 0xE8);
  SkColor toolbar_color_ = SkColorSetRGB(0xF1, 0xF3, 0xF4);
  bool is_selected_ = false;
  bool is_dark_ = false;

  SelectCallback select_callback_;

  raw_ptr<views::Label> name_label_ = nullptr;
};

// =========================================================================
// AstraAppearanceView — appearance customization panel
// =========================================================================
//
// A bubble showing appearance settings: theme selection, font size,
// toolbar density, and other visual customizations.
//
// Layout:
//   +-------------------------------------------+
//   |  Appearance                   [Close]    |
//   +-------------------------------------------+
//   |  Themes                                    |
//   |  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐  |
//   |  │██████│ │██████│ │██████│ │██████│  |
//   |  │      │ │      │ │      │ │      │  |
//   |  │██████│ │██████│ │██████│ │██████│  |
//   |  │Light │ │ Dark │ │Sepia │ │System│  |
//   |  └──────┘ └──────┘ └──────┘ └──────┘  |
//   +-------------------------------------------+
//   |  Font size                                 |
//   |  Small ●──────●──────● Large              |
//   +-------------------------------------------+
//   |  Compact mode    [ ]                       |
//   |  Show home button [✓]                      |
//   |  Show bookmarks bar [✓]                    |
//   +-------------------------------------------+
//
// This is a presentation-only view. Appearance settings are persisted
// by Astra's preferences service and applied to the Chromium theme system.
//
// Chromium subsystems reused:
//   - views::BubbleDialogDelegateView
//   - ThemeService / NativeTheme (source of theme state)
// =========================================================================

class AstraAppearanceView : public views::BubbleDialogDelegateView {
 public:
  using ThemeSelectedCallback =
      base::RepeatingCallback<void(const std::string& theme_id)>;
  using FontSizeChangedCallback =
      base::RepeatingCallback<void(int size_index)>;
  using ToggleCallback = base::RepeatingCallback<void(bool enabled)>;

  enum class FontSizeLevel { kSmall, kMedium, kLarge, kExtraLarge };

  explicit AstraAppearanceView(views::View* anchor_view);
  ~AstraAppearanceView() override;

  AstraAppearanceView(const AstraAppearanceView&) = delete;
  AstraAppearanceView& operator=(const AstraAppearanceView&) = delete;

  // -- Data ----------------------------------------------------------------

  void SetThemes(
      const std::vector<AstraThemeCardView::ThemeInfo>& themes);
  void SetSelectedTheme(const std::string& theme_id);
  void SetFontSize(FontSizeLevel level);
  void SetCompactMode(bool enabled);
  void SetShowHomeButton(bool show);
  void SetShowBookmarksBar(bool show);

  // -- Callbacks -----------------------------------------------------------

  void SetThemeSelectedCallback(ThemeSelectedCallback callback);
  void SetFontSizeChangedCallback(FontSizeChangedCallback callback);
  void SetCompactModeCallback(ToggleCallback callback);
  void SetShowHomeButtonCallback(ToggleCallback callback);
  void SetShowBookmarksBarCallback(ToggleCallback callback);

  // -- views::BubbleDialogDelegateView -------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;

 private:
  void BuildUI();
  void BuildThemesSection();
  void BuildFontSizeSection();
  void BuildTogglesSection();

  void RefreshThemes();

  void OnThemeSelected(const std::string& theme_id);
  void OnFontSizeSmall();
  void OnFontSizeMedium();
  void OnFontSizeLarge();
  void OnFontSizeExtraLarge();
  void OnCompactModeToggled();
  void OnShowHomeButtonToggled();
  void OnShowBookmarksBarToggled();

  std::vector<AstraThemeCardView::ThemeInfo> themes_;
  std::string selected_theme_id_;
  FontSizeLevel font_size_ = FontSizeLevel::kMedium;
  bool compact_mode_ = false;
  bool show_home_button_ = true;
  bool show_bookmarks_bar_ = true;

  ThemeSelectedCallback theme_callback_;
  FontSizeChangedCallback font_size_callback_;
  ToggleCallback compact_callback_;
  ToggleCallback home_button_callback_;
  ToggleCallback bookmarks_bar_callback_;

  raw_ptr<views::View> themes_row_ = nullptr;
  raw_ptr<views::MdTextButton> font_small_button_ = nullptr;
  raw_ptr<views::MdTextButton> font_medium_button_ = nullptr;
  raw_ptr<views::MdTextButton> font_large_button_ = nullptr;
  raw_ptr<views::MdTextButton> font_xlarge_button_ = nullptr;
  raw_ptr<views::Checkbox> compact_checkbox_ = nullptr;
  raw_ptr<views::Checkbox> home_checkbox_ = nullptr;
  raw_ptr<views::Checkbox> bookmarks_checkbox_ = nullptr;

  std::vector<raw_ptr<AstraThemeCardView>> theme_cards_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_APPEARANCE_ASTRA_APPEARANCE_VIEW_H_
