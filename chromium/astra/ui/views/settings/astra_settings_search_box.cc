#include "astra/ui/views/settings/astra_settings_search_box.h"

#include <memory>
#include <string>
#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "ui/color/color_id.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/render_text.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kSearchBoxHeight = 36;
constexpr int kSearchBoxHorizontalPadding = 12;
constexpr int kIconTextSpacing = 8;
constexpr int kSearchIconSize = 16;
constexpr int kClearButtonSize = 24;
constexpr int kClearButtonIconSize = 16;

// TODO(astra): Use proper color IDs from astra color system.
constexpr ui::ColorId kSearchBoxBackground = ui::kColorSysSurface4;
constexpr ui::ColorId kSearchBoxBorderColor = ui::kColorSysTonalOutline;
constexpr ui::ColorId kSearchIconColor = ui::kColorIconSecondary;
constexpr ui::ColorId kSearchTextColor = ui::kColorLabelForegroundPrimary;
constexpr ui::ColorId kSearchPlaceholderColor =
    ui::kColorLabelForegroundSecondary;

// Rounded corner radius for the search field.
constexpr int kSearchBoxCornerRadius = 8;

// The search icon character (magnifying glass) — used as a text icon.
// TODO(astra): Use a proper vector icon from the chrome resource set.
constexpr char16_t kSearchIconChar = 0x1F50D;  // 🔍

}  // namespace

// =========================================================================
// Construction / destruction
// =========================================================================

AstraSettingsSearchBox::AstraSettingsSearchBox(
    TextChangedCallback text_changed_callback)
    : text_changed_callback_(std::move(text_changed_callback)) {
  DCHECK(text_changed_callback_);

  // Main horizontal layout: icon + textfield + clear button.
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(0, kSearchBoxHorizontalPadding),
      kIconTextSpacing));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kStart);

  // Set background with rounded corners.
  SetBackground(views::CreateThemedRoundedRectBackground(
      kSearchBoxBackground, kSearchBoxCornerRadius));

  // Search icon (using a label with magnifying glass character as a
  // lightweight icon — in a full Chromium build we'd use a vector icon).
  auto icon_label = std::make_unique<views::Label>(
      std::u16string(1, kSearchIconChar));
  icon_label->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  icon_label->SetAutoColorReadabilityEnabled(false);
  icon_label->SetEnabledColorId(kSearchIconColor);
  icon_label->SetPreferredSize(gfx::Size(kSearchIconSize, kSearchIconSize));
  // Scale font to fit the icon size.
  icon_label->SetFontList(
      icon_label->font_list().DeriveWithSizeDelta(-2));
  search_icon_ = icon_label.get();
  AddChildView(std::move(icon_label));

  // Textfield — takes remaining space.
  auto textfield = std::make_unique<views::Textfield>();
  textfield->SetController(this);
  textfield->SetPlaceholderText(u"Search settings");
  textfield->SetBackgroundColor(SK_ColorTRANSPARENT);
  textfield->SetBorder(views::NullBorder());
  textfield->SetTextColorId(kSearchTextColor);
  textfield->SetPlaceholderTextColorId(kSearchPlaceholderColor);
  textfield->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  textfield->SetFontList(textfield->font_list().DeriveWithSizeDelta(0));
  // The textfield should expand to fill available space.
  layout->SetFlexForView(textfield.get(), 1);
  textfield_ = textfield.get();
  AddChildView(std::move(textfield));

  // Clear button (initially hidden).
  auto clear_button = views::MdTextButton::Create(
      base::BindRepeating(&AstraSettingsSearchBox::OnClearButtonPressed,
                          base::Unretained(this)),
      u"\u2715");  // ✕
  clear_button->SetPreferredSize(
      gfx::Size(kClearButtonSize, kClearButtonSize));
  clear_button->SetVisible(false);
  // TODO(astra): Style the clear button as an icon-only button.
  // Chromium pattern: views::ImageButton with vector icon.
  clear_button_ = clear_button.get();
  AddChildView(std::move(clear_button));
}

AstraSettingsSearchBox::~AstraSettingsSearchBox() = default;

// =========================================================================
// Query manipulation
// =========================================================================

void AstraSettingsSearchBox::SetQuery(const std::u16string& query) {
  if (textfield_) {
    textfield_->SetText(query);
    // ContentsChanged will be called automatically by SetText if the
    // controller is set.
    UpdateClearButtonVisibility();
  }
}

std::u16string AstraSettingsSearchBox::GetQuery() const {
  return textfield_ ? textfield_->GetText() : std::u16string();
}

void AstraSettingsSearchBox::Clear() {
  if (textfield_) {
    textfield_->SetText(std::u16string());
    UpdateClearButtonVisibility();
    if (!text_changed_callback_.is_null()) {
      text_changed_callback_.Run(std::u16string());
    }
  }
}

// =========================================================================
// Placeholder
// =========================================================================

void AstraSettingsSearchBox::SetPlaceholder(
    const std::u16string& placeholder) {
  if (textfield_) {
    textfield_->SetPlaceholderText(placeholder);
  }
}

std::u16string AstraSettingsSearchBox::GetPlaceholder() const {
  if (!textfield_) {
    return std::u16string();
  }
  return textfield_->GetPlaceholderText();
}

// =========================================================================
// Icon and button visibility
// =========================================================================

void AstraSettingsSearchBox::SetSearchIconVisible(bool visible) {
  if (search_icon_) {
    search_icon_->SetVisible(visible);
  }
}

void AstraSettingsSearchBox::SetClearButtonVisible(bool visible) {
  clear_button_explicitly_hidden_ = !visible;
  UpdateClearButtonVisibility();
}

// =========================================================================
// TextfieldController overrides
// =========================================================================

void AstraSettingsSearchBox::ContentsChanged(
    views::Textfield* sender,
    const std::u16string& new_contents) {
  UpdateClearButtonVisibility();
  if (!text_changed_callback_.is_null()) {
    text_changed_callback_.Run(new_contents);
  }
}

bool AstraSettingsSearchBox::HandleKeyEvent(views::Textfield* sender,
                                            const ui::KeyEvent& key_event) {
  // Escape clears the search query.
  if (key_event.type() == ui::ET_KEY_PRESSED &&
      key_event.key_code() == ui::VKEY_ESCAPE) {
    if (!GetQuery().empty()) {
      Clear();
      return true;
    }
  }
  return false;
}

// =========================================================================
// View overrides
// =========================================================================

void AstraSettingsSearchBox::OnThemeChanged() {
  views::View::OnThemeChanged();
  // Colors are set via color IDs, so they update automatically.
  // We just need to ensure the textfield colors are reapplied.
  if (textfield_) {
    textfield_->SetTextColorId(kSearchTextColor);
    textfield_->SetPlaceholderTextColorId(kSearchPlaceholderColor);
  }
}

gfx::Size AstraSettingsSearchBox::CalculatePreferredSize() const {
  return gfx::Size(views::View::CalculatePreferredSize().width(),
                   kSearchBoxHeight);
}

// =========================================================================
// Private helpers
// =========================================================================

void AstraSettingsSearchBox::OnClearButtonPressed() {
  Clear();
  // Focus the textfield after clearing.
  if (textfield_) {
    textfield_->RequestFocus();
  }
}

void AstraSettingsSearchBox::UpdateClearButtonVisibility() {
  if (!clear_button_) {
    return;
  }

  if (clear_button_explicitly_hidden_) {
    clear_button_->SetVisible(false);
    return;
  }

  bool has_text = !GetQuery().empty();
  if (clear_button_->GetVisible() != has_text) {
    clear_button_->SetVisible(has_text);
    InvalidateLayout();
  }
}

}  // namespace astra
