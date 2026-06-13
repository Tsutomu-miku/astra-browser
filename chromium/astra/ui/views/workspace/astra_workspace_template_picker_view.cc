// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/workspace/astra_workspace_template_picker_view.h"

#include <memory>
#include <string>

#include "base/strings/utf_string_conversions.h"
#include "skia/core/SkColor.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/style/typography.h"
#include "ui/views/view_class_properties.h"

namespace astra {

namespace {

constexpr int kCardWidth = 160;
constexpr int kCardHeight = 96;
constexpr int kAccentBarHeight = 4;
constexpr int kPickerWidth = 480;
constexpr int kGridSpacing = 12;
constexpr int kSectionPadding = 16;
constexpr int kCategoryChipHeight = 28;
constexpr SkColor kSelectedBorderColor = SK_ColorBLUE;

// Get the first letter of the template name as a simple icon fallback.
std::u16string GetIconText(const std::string& name) {
  if (name.empty()) return u"📄";
  return base::UTF8ToUTF16(name.substr(0, 1));
}

}  // namespace

// ===========================================================================
// AstraWorkspaceTemplateCardView
// ===========================================================================

AstraWorkspaceTemplateCardView::AstraWorkspaceTemplateCardView(
    const AstraWorkspaceTemplate& tmpl,
    SelectCallback select_callback)
    : views::Button(base::BindRepeating(
          [](SelectCallback cb, const ui::Event&) {
            // Callback is set after construction via SetCallback pattern;
            // actual click is handled below.
          },
          select_callback)),
      template_id_(tmpl.id),
      template_name_(tmpl.name),
      description_(tmpl.description),
      accent_color_(tmpl.color),
      tab_count_(static_cast<int>(tmpl.default_tabs.size())),
      category_(tmpl.category),
      select_callback_(std::move(select_callback)) {
  SetCallback(base::BindRepeating(
      &AstraWorkspaceTemplateCardView::OnButtonClicked,
      base::Unretained(this)));
  BuildLayout();
}

AstraWorkspaceTemplateCardView::~AstraWorkspaceTemplateCardView() = default;

void AstraWorkspaceTemplateCardView::BuildLayout() {
  SetPreferredSize(gfx::Size(kCardWidth, kCardHeight));
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));

  // Accent color bar.
  accent_bar_ = AddChildView(std::make_unique<views::View>());
  accent_bar_->SetPreferredSize(gfx::Size(kCardWidth, kAccentBarHeight));
  accent_bar_->SetBackground(
      views::CreateSolidBackground(SK_ColorGRAY));

  // Content area.
  auto* content = AddChildView(std::make_unique<views::View>());
  content->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(10, 12), 10));
  content->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  // Icon label (text-based placeholder).
  icon_label_ = content->AddChildView(std::make_unique<views::Label>(
      GetIconText(template_name_)));
  icon_label_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  icon_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_DIALOG_TITLE,
          views::style::STYLE_PRIMARY));
  icon_label_->SetAutoColorReadabilityEnabled(false);

  // Text column.
  auto* text_col = content->AddChildView(std::make_unique<views::View>());
  text_col->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(), 2));
  text_col->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  name_label_ = text_col->AddChildView(
      std::make_unique<views::Label>(base::UTF8ToUTF16(template_name_)));
  name_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  name_label_->SetAutoColorReadabilityEnabled(false);
  name_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));
  name_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);

  description_label_ = text_col->AddChildView(
      std::make_unique<views::Label>(base::UTF8ToUTF16(description_)));
  description_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  description_label_->SetAutoColorReadabilityEnabled(false);
  description_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
  description_label_->SetElideBehavior(gfx::ELIDE_END);
  description_label_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  tab_count_label_ = text_col->AddChildView(
      std::make_unique<views::Label>(base::UTF8ToUTF16(
          std::to_string(tab_count_) + " default tabs")));
  tab_count_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  tab_count_label_->SetAutoColorReadabilityEnabled(false);
  tab_count_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));

  UpdateVisualState();
}

void AstraWorkspaceTemplateCardView::SetSelected(bool selected) {
  if (is_selected_ == selected) return;
  is_selected_ = selected;
  UpdateVisualState();
}

void AstraWorkspaceTemplateCardView::UpdateVisualState() {
  // Update accent bar color.
  SkColor accent = SK_ColorGRAY;
  if (!accent_color_.empty() && accent_color_[0] == '#') {
    // Parse hex color.
    std::string hex = accent_color_.substr(1);
    if (hex.size() == 6) {
      unsigned int val = 0;
      if (sscanf(hex.c_str(), "%x", &val) == 1) {
        accent = SkColorSetRGB(SkColorGetR(val), SkColorGetG(val), SkColorGetB(val));
      }
    }
  }
  accent_bar_->SetBackground(views::CreateSolidBackground(accent));

  // Update border based on selected state.
  if (is_selected_) {
    SetBorder(views::CreateRoundedRectBorder(
        2, 6, kSelectedBorderColor));
  } else {
    SetBorder(views::CreateRoundedRectBorder(
        1, 6, SK_ColorGRAY));
  }

  SchedulePaint();
}

void AstraWorkspaceTemplateCardView::OnButtonClicked() {
  if (select_callback_) {
    select_callback_.Run(template_id_);
  }
}

void AstraWorkspaceTemplateCardView::OnThemeChanged() {
  views::Button::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  name_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
  description_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));
  tab_count_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));

  SetBackground(views::CreateRoundedRectBackground(
      cp->GetColor(ui::kColorDialogBackground), 6));

  UpdateVisualState();
}

void AstraWorkspaceTemplateCardView::OnPaintBackground(gfx::Canvas* canvas) {
  views::Button::OnPaintBackground(canvas);
}

void AstraWorkspaceTemplateCardView::OnPaintBorder(gfx::Canvas* canvas) {
  views::Button::OnPaintBorder(canvas);
}

// ===========================================================================
// AstraWorkspaceTemplatePickerView
// ===========================================================================

AstraWorkspaceTemplatePickerView::AstraWorkspaceTemplatePickerView(
    views::View* anchor_view,
    TemplateSelectedCallback callback)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_RIGHT),
      selected_callback_(std::move(callback)) {
  SetShowCloseButton(true);
  SetButtons(ui::DIALOG_BUTTON_NONE);
  set_fixed_width(kPickerWidth);

  BuildUI();
}

AstraWorkspaceTemplatePickerView::~AstraWorkspaceTemplatePickerView() = default;

void AstraWorkspaceTemplatePickerView::SetSelectedTemplate(
    const std::string& template_id) {
  selected_template_id_ = template_id;
  for (auto* card : template_cards_) {
    card->SetSelected(card->template_id() == template_id);
  }
  if (create_button_) {
    create_button_->SetEnabled(!template_id.empty());
  }
}

void AstraWorkspaceTemplatePickerView::SetCategoryFilter(
    std::optional<AstraWorkspaceTemplateCategory> category) {
  active_category_ = category;
  RefreshTemplateGrid();
}

void AstraWorkspaceTemplatePickerView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  set_margins(gfx::Insets::VH(0, 0));

  // Header (title).
  BuildHeader();

  // Category chips.
  BuildCategoryChips();

  // Template grid (scrollable).
  BuildTemplateGrid();

  // Footer with action buttons.
  BuildFooter();
}

void AstraWorkspaceTemplatePickerView::BuildHeader() {
  auto* header = AddChildView(std::make_unique<views::View>());
  header->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(14, 16), 8));
  header->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  auto* title = header->AddChildView(
      std::make_unique<views::Label>(u"Choose a template"));
  title->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title->SetAutoColorReadabilityEnabled(false);
  title->SetFontList(
      views::style::GetFont(views::style::CONTEXT_DIALOG_TITLE,
                            views::style::STYLE_PRIMARY));
  title->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));
}

void AstraWorkspaceTemplatePickerView::BuildCategoryChips() {
  category_bar_ = AddChildView(std::make_unique<views::View>());
  category_bar_->SetLayoutManager(
      std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kHorizontal)
      .SetDefault(views::kFlexBehaviorKey,
                  views::FlexSpecification(
                      views::LayoutFlexOrientation::kHorizontal,
                      views::MinimumFlexSizeRule::kPreferred,
                      views::MaximumFlexSizeRule::kPreferred))
      .SetWrapBehavior(views::FlexLayout::WrapBehavior::kWrap);
  category_bar_->SetBorder(
      views::CreateEmptyBorder(gfx::Insets::VH(0, kSectionPadding)));

  // "All" chip.
  auto* all_chip = category_bar_->AddChildView(
      std::make_unique<views::MdTextButton>(
          base::BindRepeating(
              [](AstraWorkspaceTemplatePickerView* picker) {
                picker->SetCategoryFilter(std::nullopt);
              },
              base::Unretained(this)),
          u"All"));
  all_chip->SetProminent(true);

  // Category chips.
  auto categories = GetAllTemplateCategories();
  for (auto cat : categories) {
    std::string name = GetTemplateCategoryDisplayName(cat);
    auto* chip = category_bar_->AddChildView(
        std::make_unique<views::MdTextButton>(
            base::BindRepeating(
                &AstraWorkspaceTemplatePickerView::OnCategoryClicked,
                base::Unretained(this), cat),
            base::UTF8ToUTF16(name)));
  }
}

void AstraWorkspaceTemplatePickerView::BuildTemplateGrid() {
  scroll_view_ = AddChildView(std::make_unique<views::ScrollView>());
  scroll_view_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));
  scroll_view_->SetBorder(
      views::CreateEmptyBorder(gfx::Insets::VH(12, kSectionPadding)));

  template_grid_ = scroll_view_->SetContents(
      std::make_unique<views::View>());
  template_grid_->SetLayoutManager(
      std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kHorizontal)
      .SetDefault(views::kFlexBehaviorKey,
                  views::FlexSpecification(
                      views::LayoutFlexOrientation::kHorizontal,
                      views::MinimumFlexSizeRule::kPreferred,
                      views::MaximumFlexSizeRule::kPreferred))
      .SetWrapBehavior(views::FlexLayout::WrapBehavior::kWrap)
      .SetInteriorMargin(gfx::Insets::VH(0, 0));

  RefreshTemplateGrid();
}

void AstraWorkspaceTemplatePickerView::BuildFooter() {
  auto* footer = AddChildView(std::make_unique<views::View>());
  footer->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(12, 16), 8));
  footer->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  footer->SetBorder(views::CreateSolidSidedBorder(
      1, 0, 0, 0, SK_ColorGRAY));

  cancel_button_ = footer->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraWorkspaceTemplatePickerView::OnCancelClicked,
              base::Unretained(this)),
          u"Cancel"));

  auto* spacer = footer->AddChildView(std::make_unique<views::View>());
  spacer->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  create_button_ = footer->AddChildView(
      views::MdTextButton::CreateProminent(
          base::BindRepeating(
              &AstraWorkspaceTemplatePickerView::OnCreateClicked,
              base::Unretained(this)),
          u"Create workspace"));
  create_button_->SetEnabled(false);
}

void AstraWorkspaceTemplatePickerView::RefreshTemplateGrid() {
  if (!template_grid_) return;

  template_grid_->RemoveAllChildViews();
  template_cards_.clear();

  std::vector<AstraWorkspaceTemplate> templates;
  if (active_category_.has_value()) {
    templates = GetTemplatesByCategory(*active_category_);
  } else {
    templates = GetBuiltInTemplates();
  }

  for (const auto& tmpl : templates) {
    auto* card = template_grid_->AddChildView(
        std::make_unique<AstraWorkspaceTemplateCardView>(
            tmpl,
            base::BindRepeating(
                &AstraWorkspaceTemplatePickerView::OnTemplateSelected,
                base::Unretained(this))));
    template_cards_.push_back(card);
  }

  InvalidateLayout();
}

void AstraWorkspaceTemplatePickerView::OnTemplateSelected(
    const std::string& template_id) {
  SetSelectedTemplate(template_id);
}

void AstraWorkspaceTemplatePickerView::OnCategoryClicked(
    AstraWorkspaceTemplateCategory category) {
  SetCategoryFilter(category);
}

void AstraWorkspaceTemplatePickerView::OnCreateClicked() {
  if (!selected_template_id_.empty() && selected_callback_) {
    selected_callback_.Run(selected_template_id_);
    GetWidget()->Close();
  }
}

void AstraWorkspaceTemplatePickerView::OnCancelClicked() {
  GetWidget()->Close();
}

std::u16string AstraWorkspaceTemplatePickerView::GetWindowTitle() const {
  return u"Choose a template";
}

void AstraWorkspaceTemplatePickerView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
}

}  // namespace astra
