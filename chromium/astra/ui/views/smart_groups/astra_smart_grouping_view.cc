// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/smart_groups/astra_smart_grouping_view.h"

#include <algorithm>
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
#include "ui/views/controls/scroll_view.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/style/typography.h"
#include "ui/views/view_class_properties.h"

namespace astra {

namespace {

constexpr int kBubbleWidth = 360;
constexpr int kSuggestionHeight = 64;
constexpr int kSectionPadding = 16;
constexpr int kRowSpacing = 8;
constexpr int kMaxVisibleSuggestions = 4;
constexpr int kColorDotSize = 10;

// Parse a hex color string like "#RRGGBB" to SkColor.
SkColor ParseHexColor(const std::string& hex) {
  if (hex.empty() || hex[0] != '#') return SK_ColorGRAY;
  std::string clean = hex.substr(1);
  if (clean.size() != 6) return SK_ColorGRAY;
  unsigned int val = 0;
  if (sscanf(clean.c_str(), "%x", &val) != 1) return SK_ColorGRAY;
  return SkColorSetRGB(SkColorGetR(val), SkColorGetG(val), SkColorGetB(val));
}

// Join domain names into a preview string.
std::u16string JoinDomains(const std::vector<std::string>& domains,
                           int max_count = 3) {
  std::string result;
  for (size_t i = 0; i < std::min(domains.size(), size_t(max_count)); ++i) {
    if (i > 0) result += ", ";
    result += domains[i];
  }
  if (domains.size() > size_t(max_count)) {
    result += "…";
  }
  return base::UTF8ToUTF16(result);
}

}  // namespace

// ===========================================================================
// AstraSmartGroupSuggestionView
// ===========================================================================

AstraSmartGroupSuggestionView::AstraSmartGroupSuggestionView(
    const Suggestion& suggestion,
    ApplyCallback apply_callback,
    DismissCallback dismiss_callback)
    : suggestion_id_(suggestion.suggestion_id),
      name_(suggestion.name),
      group_type_(suggestion.group_type),
      tab_count_(suggestion.tab_count),
      sample_domains_(suggestion.sample_domains),
      color_(suggestion.color),
      selected_(true),
      apply_callback_(std::move(apply_callback)),
      dismiss_callback_(std::move(dismiss_callback)) {
  BuildLayout();
}

AstraSmartGroupSuggestionView::~AstraSmartGroupSuggestionView() = default;

void AstraSmartGroupSuggestionView::BuildLayout() {
  SetPreferredSize(gfx::Size(kBubbleWidth - kSectionPadding * 2, kSuggestionHeight));
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(8, 12),
      10));
  SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  SetBorder(views::CreateRoundedRectBorder(
      1, 6, SK_ColorGRAY));

  // Checkbox.
  checkbox_ = AddChildView(std::make_unique<views::Checkbox>(
      base::BindRepeating(
          &AstraSmartGroupSuggestionView::OnToggled,
          base::Unretained(this)),
      std::u16string()));
  checkbox_->SetChecked(selected_);

  // Color dot.
  color_dot_ = AddChildView(std::make_unique<views::View>());
  color_dot_->SetPreferredSize(
      gfx::Size(kColorDotSize, kColorDotSize));
  color_dot_->SetBackground(
      views::CreateRoundedRectBackground(
          ParseHexColor(color_), kColorDotSize / 2));

  // Text column.
  auto* text_col = AddChildView(std::make_unique<views::View>());
  text_col->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(), 3));
  text_col->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  name_label_ = text_col->AddChildView(
      std::make_unique<views::Label>(
          name_ + u" (" +
          base::UTF8ToUTF16(std::to_string(tab_count_)) + u" tabs)"));
  name_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  name_label_->SetAutoColorReadabilityEnabled(false);
  name_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));
  name_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);

  sample_label_ = text_col->AddChildView(
      std::make_unique<views::Label>(JoinDomains(sample_domains_)));
  sample_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  sample_label_->SetAutoColorReadabilityEnabled(false);
  sample_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));
  sample_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);

  // Apply button.
  apply_button_ = AddChildView(
      views::MdTextButton::CreateProminent(
          base::BindRepeating(
              &AstraSmartGroupSuggestionView::OnApplyClicked,
              base::Unretained(this)),
          u"Group"));
}

void AstraSmartGroupSuggestionView::SetSelected(bool selected) {
  selected_ = selected;
  if (checkbox_) {
    checkbox_->SetChecked(selected);
  }
}

void AstraSmartGroupSuggestionView::OnToggled() {
  selected_ = checkbox_->GetChecked();
}

void AstraSmartGroupSuggestionView::OnApplyClicked() {
  if (apply_callback_) {
    apply_callback_.Run(suggestion_id_);
  }
}

void AstraSmartGroupSuggestionView::OnDismissClicked() {
  if (dismiss_callback_) {
    dismiss_callback_.Run(suggestion_id_);
  }
}

void AstraSmartGroupSuggestionView::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  name_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
  sample_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));

  SetBackground(views::CreateRoundedRectBackground(
      cp->GetColor(ui::kColorDialogBackground), 6));
}

// ===========================================================================
// AstraSmartGroupingView
// ===========================================================================

AstraSmartGroupingView::AstraSmartGroupingView(views::View* anchor_view)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_RIGHT) {
  SetShowCloseButton(true);
  SetButtons(ui::DIALOG_BUTTON_NONE);
  set_fixed_width(kBubbleWidth);

  BuildUI();
}

AstraSmartGroupingView::~AstraSmartGroupingView() = default;

void AstraSmartGroupingView::SetSuggestions(
    const std::vector<AstraSmartGroupSuggestionView::Suggestion>& suggestions) {
  suggestions_ = suggestions;
  RefreshSuggestionsList();
}

void AstraSmartGroupingView::SetGroupBy(GroupBy group_by) {
  group_by_ = group_by;

  if (domain_button_) domain_button_->SetProminent(group_by == GroupBy::kDomain);
  if (time_button_) time_button_->SetProminent(group_by == GroupBy::kTime);
  if (purpose_button_) purpose_button_->SetProminent(group_by == GroupBy::kPurpose);
  if (workspace_button_) workspace_button_->SetProminent(group_by == GroupBy::kWorkspace);
}

void AstraSmartGroupingView::SetApplySuggestionsCallback(
    ApplySuggestionsCallback callback) {
  apply_callback_ = std::move(callback);
}

void AstraSmartGroupingView::SetDismissSuggestionCallback(
    DismissSuggestionCallback callback) {
  dismiss_callback_ = std::move(callback);
}

void AstraSmartGroupingView::SetGroupByChangedCallback(
    GroupByChangedCallback callback) {
  group_by_changed_callback_ = std::move(callback);
}

void AstraSmartGroupingView::SetRefreshCallback(RefreshCallback callback) {
  refresh_callback_ = std::move(callback);
}

void AstraSmartGroupingView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  set_margins(gfx::Insets::VH(0, 0));

  BuildGroupByRow();
  BuildSuggestionsList();
  BuildActionButtons();
}

void AstraSmartGroupingView::BuildGroupByRow() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 8));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  auto* label = section->AddChildView(
      std::make_unique<views::Label>(u"Group by"));
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SetAutoColorReadabilityEnabled(false);
  label->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  // Buttons row.
  auto* buttons_row = section->AddChildView(std::make_unique<views::View>());
  buttons_row->SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kHorizontal)
      .SetDefault(views::kFlexBehaviorKey,
                  views::FlexSpecification(
                      views::LayoutFlexOrientation::kHorizontal,
                      views::MinimumFlexSizeRule::kScaleToZero,
                      views::MaximumFlexSizeRule::kPreferred))
      .SetWrapBehavior(views::FlexLayout::WrapBehavior::kWrap)
      .SetColumnGap(4)
      .SetRowGap(4);

  domain_button_ = buttons_row->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraSmartGroupingView::OnGroupByDomain,
              base::Unretained(this)),
          u"Domain"));
  domain_button_->SetProminent(true);

  time_button_ = buttons_row->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraSmartGroupingView::OnGroupByTime,
              base::Unretained(this)),
          u"Time"));

  purpose_button_ = buttons_row->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraSmartGroupingView::OnGroupByPurpose,
              base::Unretained(this)),
          u"Purpose"));

  workspace_button_ = buttons_row->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraSmartGroupingView::OnGroupByWorkspace,
              base::Unretained(this)),
          u"Workspace"));
}

void AstraSmartGroupingView::BuildSuggestionsList() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), kRowSpacing));
  section->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  count_label_ = section->AddChildView(
      std::make_unique<views::Label>(u"Suggestions (0)"));
  count_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  count_label_->SetAutoColorReadabilityEnabled(false);
  count_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  scroll_view_ = section->AddChildView(
      std::make_unique<views::ScrollView>());
  scroll_view_->SetClipHeight(kSuggestionHeight * kMaxVisibleSuggestions +
                                kRowSpacing * (kMaxVisibleSuggestions - 1));
  scroll_view_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  suggestions_list_ = scroll_view_->SetContents(
      std::make_unique<views::View>());
  suggestions_list_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(), kRowSpacing));
}

void AstraSmartGroupingView::BuildActionButtons() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets::VH(12, kSectionPadding), 8));
  section->SetBorder(views::CreateSolidSidedBorder(
      1, 0, 0, 0, SK_ColorGRAY));

  apply_button_ = section->AddChildView(
      views::MdTextButton::CreateProminent(
          base::BindRepeating(
              &AstraSmartGroupingView::OnApplySelected,
              base::Unretained(this)),
          u"Apply selected"));
  apply_button_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  refresh_button_ = section->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraSmartGroupingView::OnRefreshClicked,
              base::Unretained(this)),
          u"Refresh"));
}

void AstraSmartGroupingView::RefreshSuggestionsList() {
  if (!suggestions_list_) return;

  suggestions_list_->RemoveAllChildViews();
  suggestion_views_.clear();

  if (count_label_) {
    count_label_->SetText(base::UTF8ToUTF16(
        "Suggestions (" + std::to_string(suggestions_.size()) + ")"));
  }

  for (const auto& suggestion : suggestions_) {
    auto* view = suggestions_list_->AddChildView(
        std::make_unique<AstraSmartGroupSuggestionView>(
            suggestion,
            base::BindRepeating(
                &AstraSmartGroupingView::OnApplySuggestion,
                base::Unretained(this)),
            base::BindRepeating(
                &AstraSmartGroupingView::OnDismissSuggestion,
                base::Unretained(this))));
    suggestion_views_.push_back(view);
  }

  InvalidateLayout();
}

void AstraSmartGroupingView::OnGroupByDomain() {
  SetGroupBy(GroupBy::kDomain);
  if (group_by_changed_callback_) {
    group_by_changed_callback_.Run("domain");
  }
}

void AstraSmartGroupingView::OnGroupByTime() {
  SetGroupBy(GroupBy::kTime);
  if (group_by_changed_callback_) {
    group_by_changed_callback_.Run("time");
  }
}

void AstraSmartGroupingView::OnGroupByPurpose() {
  SetGroupBy(GroupBy::kPurpose);
  if (group_by_changed_callback_) {
    group_by_changed_callback_.Run("purpose");
  }
}

void AstraSmartGroupingView::OnGroupByWorkspace() {
  SetGroupBy(GroupBy::kWorkspace);
  if (group_by_changed_callback_) {
    group_by_changed_callback_.Run("workspace");
  }
}

void AstraSmartGroupingView::OnApplySelected() {
  if (!apply_callback_) return;

  std::vector<std::string> selected_ids;
  for (auto* view : suggestion_views_) {
    if (view->selected()) {
      selected_ids.push_back(view->suggestion_id());
    }
  }
  apply_callback_.Run(selected_ids);
}

void AstraSmartGroupingView::OnRefreshClicked() {
  if (refresh_callback_) {
    refresh_callback_.Run();
  }
}

void AstraSmartGroupingView::OnDismissSuggestion(const std::string& id) {
  if (dismiss_callback_) {
    dismiss_callback_.Run(id);
  }
}

void AstraSmartGroupingView::OnApplySuggestion(const std::string& id) {
  if (apply_callback_) {
    apply_callback_.Run({id});
  }
}

std::u16string AstraSmartGroupingView::GetWindowTitle() const {
  return u"Smart Groups";
}

void AstraSmartGroupingView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  count_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
}

}  // namespace astra
