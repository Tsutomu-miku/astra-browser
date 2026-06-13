#include "astra/ui/views/settings/astra_settings_section_view.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/observer_list.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/button/toggle_button.h"
#include "ui/views/controls/combobox/combobox.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/separator.h"
#include "ui/views/controls/slider.h"
#include "ui/views/layout/box_layout.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kSectionHeaderBottomPadding = 8;
constexpr int kSectionDescriptionBottomPadding = 12;
constexpr int kRowSpacing = 8;
constexpr int kRowHeight = 36;
constexpr int kControlLabelSpacing = 12;
constexpr int kDividerPadding = 12;
constexpr int kSectionHeaderFontSizeDelta = 1;
constexpr int kExpandButtonSize = 24;
constexpr int kIconSize = 20;
constexpr int kIconTextSpacing = 10;
constexpr int kSettingCountBadgeHorizontalPadding = 8;
constexpr int kSettingCountBadgeVerticalPadding = 2;
constexpr int kSettingCountBadgeCornerRadius = 10;

// TODO(astra): Move color tokens to astra/ui/color/astra_color_ids.h
// with a proper ColorProvider mixin.
constexpr ui::ColorId kSectionHeaderTextColor =
    ui::kColorLabelForegroundPrimary;
constexpr ui::ColorId kSectionDescriptionTextColor =
    ui::kColorLabelForegroundSecondary;
constexpr ui::ColorId kRowLabelTextColor = ui::kColorLabelForegroundPrimary;
constexpr ui::ColorId kDividerColor = ui::kColorSeparator;
constexpr ui::ColorId kSettingCountBadgeBackground = ui::kColorSysSurface4;
constexpr ui::ColorId kSettingCountBadgeTextColor =
    ui::kColorLabelForegroundSecondary;

// Expand/collapse arrow characters.
constexpr char16_t kExpandArrowChar = u'\u25B6';  // ▶ (right-pointing triangle)
constexpr char16_t kCollapseArrowChar = u'\u25BC';  // ▼ (down-pointing triangle)

// Default icon characters (used as text-based icons until vector icons are
// available).
constexpr char16_t kDefaultIconChar = u'\u2699';  // ⚙ (gear)

}  // namespace

// =========================================================================
// Construction / destruction
// =========================================================================

AstraSettingsSectionView::AstraSettingsSectionView(
    const std::u16string& title)
    : title_(title) {
  // Main vertical layout for the section.
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(),
      kSectionHeaderBottomPadding));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Section header with icon, title, expand button, and count badge.
  BuildHeader();

  // Rows container (vertical stack of control rows).
  auto rows = std::make_unique<views::View>();
  rows_container_ = rows.get();
  auto* rows_layout = rows->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(), kRowSpacing));
  rows_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);
  AddChildView(std::move(rows));
}

AstraSettingsSectionView::~AstraSettingsSectionView() = default;

// =========================================================================
// Observer management
// =========================================================================

void AstraSettingsSectionView::AddObserver(Observer* observer) {
  if (observer) {
    observers_.AddObserver(observer);
  }
}

void AstraSettingsSectionView::RemoveObserver(Observer* observer) {
  if (observer) {
    observers_.RemoveObserver(observer);
  }
}

// =========================================================================
// Section identity
// =========================================================================

void AstraSettingsSectionView::SetSection(const std::string& section_id,
                                          const std::u16string& title,
                                          const std::u16string& description) {
  section_id_ = section_id;
  title_ = title;

  if (header_label_) {
    header_label_->SetText(title_);
  }

  SetDescription(description);
}

// =========================================================================
// Section metadata
// =========================================================================

void AstraSettingsSectionView::SetDescription(
    const std::u16string& description) {
  if (description == description_) {
    return;
  }
  description_ = description;

  if (description_.empty()) {
    if (description_label_) {
      RemoveChildViewT(description_label_);
      description_label_ = nullptr;
    }
    return;
  }

  if (!description_label_) {
    auto desc_label = std::make_unique<views::Label>(description_);
    desc_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    desc_label->SetMultiLine(true);
    desc_label->SetAutoColorReadabilityEnabled(false);
    desc_label->SetEnabledColorId(kSectionDescriptionTextColor);
    desc_label->SetBorder(views::CreateEmptyBorder(
        gfx::Insets::TLBR(0, 0, kSectionDescriptionBottomPadding, 0)));
    description_label_ = desc_label.get();
    // Insert after header, before rows container.
    AddChildViewAt(std::move(desc_label), 1);
  } else {
    description_label_->SetText(description_);
  }

  // Description visibility follows expanded state.
  if (description_label_) {
    description_label_->SetVisible(expanded_);
  }
}

void AstraSettingsSectionView::AddSearchKeyword(const std::u16string& keyword) {
  keywords_.push_back(keyword);
}

void AstraSettingsSectionView::AddSearchKeywords(
    const std::vector<std::u16string>& keywords) {
  for (const auto& kw : keywords) {
    keywords_.push_back(kw);
  }
}

bool AstraSettingsSectionView::MatchesSearch(
    const std::u16string& query) const {
  if (query.empty()) {
    return true;
  }

  std::u16string lower_query = base::ToLowerASCII(query);

  // Match against title.
  if (base::ToLowerASCII(title_).find(lower_query) != std::u16string::npos) {
    return true;
  }

  // Match against description.
  if (!description_.empty() &&
      base::ToLowerASCII(description_).find(lower_query) !=
          std::u16string::npos) {
    return true;
  }

  // Match against keywords.
  for (const auto& kw : keywords_) {
    if (base::ToLowerASCII(kw).find(lower_query) != std::u16string::npos) {
      return true;
    }
  }

  // Match against row labels.
  for (const auto& label : row_labels_) {
    if (base::ToLowerASCII(label).find(lower_query) != std::u16string::npos) {
      return true;
    }
  }

  return false;
}

void AstraSettingsSectionView::SetIconName(const std::string& icon_name) {
  if (icon_name_ == icon_name) {
    return;
  }
  icon_name_ = icon_name;
  UpdateIcon();
}

// =========================================================================
// Expand / collapse
// =========================================================================

void AstraSettingsSectionView::SetExpanded(bool expanded) {
  if (expanded_ == expanded) {
    return;
  }
  expanded_ = expanded;

  // Show/hide the description and rows container.
  if (description_label_) {
    description_label_->SetVisible(expanded_);
  }
  rows_container_->SetVisible(expanded_);

  UpdateExpandButton();
  NotifyExpandedChanged();
  InvalidateLayout();
}

void AstraSettingsSectionView::ToggleExpanded() {
  SetExpanded(!expanded_);
}

void AstraSettingsSectionView::SetExpandable(bool expandable) {
  if (expandable_ == expandable) {
    return;
  }
  expandable_ = expandable;

  if (expand_button_) {
    expand_button_->SetVisible(expandable_);
  }

  // If no longer expandable, ensure expanded.
  if (!expandable_ && !expanded_) {
    SetExpanded(true);
  }
}

// =========================================================================
// Setting count
// =========================================================================

void AstraSettingsSectionView::SetSettingCount(int count) {
  if (setting_count_ == count) {
    return;
  }
  setting_count_ = count;
  UpdateSettingCountBadge();
}

void AstraSettingsSectionView::UpdateSettingCountBadge() {
  if (!setting_count_badge_) {
    return;
  }

  std::u16string text = base::NumberToString16(setting_count_);
  setting_count_badge_->SetText(text);
  setting_count_badge_->SetVisible(setting_count_ > 0);
}

// =========================================================================
// Setting views
// =========================================================================

void AstraSettingsSectionView::AddSettingView(views::View* setting_view) {
  if (rows_container_ && setting_view) {
    rows_container_->AddChildView(setting_view);
  }
}

void AstraSettingsSectionView::ClearSettingViews() {
  if (rows_container_) {
    rows_container_->RemoveAllChildViews();
    row_labels_.clear();
  }
}

// =========================================================================
// Row builders
// =========================================================================

views::ToggleButton* AstraSettingsSectionView::AddToggleRow(
    const std::u16string& label,
    bool initial_value,
    base::RepeatingClosure callback) {
  auto toggle = std::make_unique<views::ToggleButton>(std::move(callback));
  toggle->SetIsOn(initial_value);
  views::ToggleButton* toggle_ptr = toggle.get();

  RegisterRowLabel(label);
  auto* row = CreateLabeledRow(label, std::move(toggle));
  rows_container_->AddChildView(std::unique_ptr<views::View>(row));

  return toggle_ptr;
}

views::Combobox* AstraSettingsSectionView::AddComboboxRow(
    const std::u16string& label,
    std::unique_ptr<ui::ComboboxModel> model,
    base::RepeatingClosure callback) {
  auto combobox = std::make_unique<views::Combobox>(std::move(model));
  combobox->SetCallback(std::move(callback));
  views::Combobox* combobox_ptr = combobox.get();

  RegisterRowLabel(label);
  auto* row = CreateLabeledRow(label, std::move(combobox));
  rows_container_->AddChildView(std::unique_ptr<views::View>(row));

  return combobox_ptr;
}

views::Slider* AstraSettingsSectionView::AddSliderRow(
    const std::u16string& label,
    double initial_value,
    ValueFormatter value_formatter,
    base::RepeatingCallback<void(double)> callback) {
  // Row container: label + (slider + value label).
  auto row = std::make_unique<views::View>();
  auto* row_layout = row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets(),
      kControlLabelSpacing));
  row_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kSpaceBetween);
  row_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  row->SetPreferredSize(gfx::Size(0, kRowHeight));

  auto label_view = std::make_unique<views::Label>(label);
  label_view->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label_view->SetAutoColorReadabilityEnabled(false);
  label_view->SetEnabledColorId(kRowLabelTextColor);
  row->AddChildView(std::move(label_view));

  // Slider + value container.
  auto slider_container = std::make_unique<views::View>();
  auto* slider_layout = slider_container->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal, gfx::Insets(), 8));
  slider_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  slider_container->SetPreferredSize(gfx::Size(180, kRowHeight));

  // Value label (created before the slider so we can reference it in the
  // slider callback).
  auto value_label = std::make_unique<views::Label>(
      value_formatter.Run(initial_value));
  value_label->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  value_label->SetAutoColorReadabilityEnabled(false);
  value_label->SetEnabledColorId(ui::kColorLabelForegroundSecondary);
  views::Label* value_label_ptr = value_label.get();

  // Slider with callback that updates both the value label and calls the
  // user's callback.
  auto slider = std::make_unique<views::Slider>(
      base::BindRepeating(
          [](base::RepeatingCallback<void(double)> user_callback,
             ValueFormatter formatter,
             views::Label* value_label,
             double value) {
            user_callback.Run(value);
            if (value_label) {
              value_label->SetText(formatter.Run(value));
            }
          },
          callback, value_formatter, value_label_ptr),
      views::Slider::Style::kMaterial);
  slider->SetValue(initial_value);
  slider->SetPreferredSize(gfx::Size(120, kRowHeight));
  views::Slider* slider_ptr = slider.get();
  slider_container->AddChildView(std::move(slider));
  slider_container->AddChildView(std::move(value_label));

  row->AddChildView(std::move(slider_container));

  RegisterRowLabel(label);
  rows_container_->AddChildView(std::move(row));

  return slider_ptr;
}

views::Label* AstraSettingsSectionView::AddInfoRow(
    const std::u16string& label,
    const std::u16string& value) {
  auto value_label = std::make_unique<views::Label>(value);
  value_label->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  value_label->SetAutoColorReadabilityEnabled(false);
  value_label->SetEnabledColorId(ui::kColorLabelForegroundSecondary);
  views::Label* value_label_ptr = value_label.get();

  RegisterRowLabel(label);
  auto* row = CreateLabeledRow(label, std::move(value_label));
  rows_container_->AddChildView(std::unique_ptr<views::View>(row));

  return value_label_ptr;
}

views::MdTextButton* AstraSettingsSectionView::AddButtonRow(
    const std::u16string& label,
    const std::u16string& button_text,
    base::RepeatingClosure callback) {
  auto button = views::MdTextButton::Create(std::move(callback), button_text);
  views::MdTextButton* button_ptr = button.get();

  RegisterRowLabel(label);
  auto* row = CreateLabeledRow(label, std::move(button));
  rows_container_->AddChildView(std::unique_ptr<views::View>(row));

  return button_ptr;
}

void AstraSettingsSectionView::AddDivider() {
  auto divider = std::make_unique<views::Separator>();
  divider->SetColorId(kDividerColor);
  divider->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(kDividerPadding / 2, 0)));
  rows_container_->AddChildView(std::move(divider));
}

// =========================================================================
// Private helpers
// =========================================================================

void AstraSettingsSectionView::BuildHeader() {
  auto header_row = std::make_unique<views::View>();
  auto* header_layout = header_row->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal, gfx::Insets(),
          8));
  header_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  header_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kStart);

  // Section icon.
  auto icon_view = std::make_unique<views::ImageView>();
  icon_view->SetImageSize(gfx::Size(kIconSize, kIconSize));
  icon_view->SetPreferredSize(gfx::Size(kIconSize, kIconSize));
  icon_view_ = icon_view.get();
  header_row->AddChildView(std::move(icon_view));

  // Title label.
  auto header = std::make_unique<views::Label>(title_);
  header->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header->SetAutoColorReadabilityEnabled(false);
  header->SetFontList(
      header->font_list().DeriveWithSizeDelta(kSectionHeaderFontSizeDelta)
          .DeriveWithWeight(gfx::Font::Weight::MEDIUM));
  header->SetEnabledColorId(kSectionHeaderTextColor);
  header_label_ = header.get();
  header_layout->SetFlexForView(header.get(), 1);
  header_row->AddChildView(std::move(header));

  // Setting count badge.
  auto count_badge = std::make_unique<views::Label>(std::u16string());
  count_badge->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  count_badge->SetAutoColorReadabilityEnabled(false);
  count_badge->SetEnabledColorId(kSettingCountBadgeTextColor);
  count_badge->SetBackground(views::CreateThemedRoundedRectBackground(
      kSettingCountBadgeBackground, kSettingCountBadgeCornerRadius));
  count_badge->SetBorder(views::CreateEmptyBorder(gfx::Insets::VH(
      kSettingCountBadgeVerticalPadding, kSettingCountBadgeHorizontalPadding)));
  count_badge->SetVisible(false);
  count_badge->SetFontList(
      count_badge->font_list().DeriveWithSizeDelta(-1)
          .DeriveWithWeight(gfx::Font::Weight::MEDIUM));
  setting_count_badge_ = count_badge.get();
  header_row->AddChildView(std::move(count_badge));

  // Expand/collapse button.
  auto expand_button = views::MdTextButton::Create(
      base::BindRepeating(&AstraSettingsSectionView::OnExpandButtonPressed,
                          base::Unretained(this)),
      std::u16string(1, kCollapseArrowChar));
  expand_button->SetPreferredSize(
      gfx::Size(kExpandButtonSize, kExpandButtonSize));
  expand_button->SetVisible(expandable_);
  expand_button_ = expand_button.get();
  header_row->AddChildView(std::move(expand_button));

  header_row_ = header_row.get();
  AddChildView(std::move(header_row));

  // Initialize the icon display.
  UpdateIcon();
}

void AstraSettingsSectionView::UpdateExpandButton() {
  if (!expand_button_) {
    return;
  }
  char16_t arrow = expanded_ ? kCollapseArrowChar : kExpandArrowChar;
  expand_button_->SetText(std::u16string(1, arrow));
}

void AstraSettingsSectionView::UpdateIcon() {
  if (!icon_view_) {
    return;
  }

  // TODO(astra): Use proper vector icons from a resource bundle.
  // For now, we use a text-based icon approach with a label.
  // We'll show a default gear character.

  // Since ImageView requires a real image, for the text-based icon
  // approach we could use a label.  For simplicity and to keep the
  // ImageView API, we leave the icon view empty for now and rely on
  // the text content for identification.
  //
  // In a real Chromium build, this would be a vector icon from
  // chrome/app/theme/chromium/product_logo_* or similar.

  // For now, hide the icon view if no icon name is set, since we
  // don't have actual image resources in the test build.
  icon_view_->SetVisible(!icon_name_.empty());
}

void AstraSettingsSectionView::NotifyExpandedChanged() {
  for (auto& observer : observers_) {
    observer.OnSectionExpandedChanged(this, expanded_);
  }
}

void AstraSettingsSectionView::OnExpandButtonPressed() {
  ToggleExpanded();
}

views::View* AstraSettingsSectionView::CreateLabeledRow(
    const std::u16string& label_text,
    std::unique_ptr<views::View> control) {
  auto row = std::make_unique<views::View>();
  auto* layout = row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets(),
      kControlLabelSpacing));
  layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kSpaceBetween);
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  row->SetPreferredSize(gfx::Size(0, kRowHeight));

  auto label = std::make_unique<views::Label>(label_text);
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SetAutoColorReadabilityEnabled(false);
  label->SetEnabledColorId(kRowLabelTextColor);
  row->AddChildView(std::move(label));

  row->AddChildView(std::move(control));

  return row.release();
}

void AstraSettingsSectionView::RegisterRowLabel(const std::u16string& label) {
  row_labels_.push_back(label);
}

void AstraSettingsSectionView::UpdateLabelColors() {
  // Labels use color IDs, so they update automatically via the ColorProvider.
  // This method is a placeholder for any manual color updates needed.
}

// =========================================================================
// Theme handling
// =========================================================================

void AstraSettingsSectionView::OnThemeChanged() {
  views::View::OnThemeChanged();
  UpdateLabelColors();
}

}  // namespace astra
