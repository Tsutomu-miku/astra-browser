// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/tab_rules/astra_tab_rules_view.h"

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
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/checkbox.h"
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
constexpr int kItemHeight = 100;
constexpr int kSectionPadding = 16;
constexpr int kItemSpacing = 8;
constexpr int kMaxVisibleItems = 4;

}  // namespace

// ===========================================================================
// AstraTabRuleItemView
// ===========================================================================

AstraTabRuleItemView::AstraTabRuleItemView(
    const RuleInfo& info,
    ToggleCallback toggle_callback,
    EditCallback edit_callback,
    DeleteCallback delete_callback)
    : rule_id_(info.rule_id),
      name_(info.name),
      enabled_(info.enabled),
      trigger_type_(info.trigger_type),
      trigger_value_(info.trigger_value),
      action_type_(info.action_type),
      action_value_(info.action_value),
      match_count_(info.match_count),
      toggle_callback_(std::move(toggle_callback)),
      edit_callback_(std::move(edit_callback)),
      delete_callback_(std::move(delete_callback)) {
  BuildLayout();
}

AstraTabRuleItemView::~AstraTabRuleItemView() = default;

void AstraTabRuleItemView::SetEnabled(bool enabled) {
  enabled_ = enabled;
  if (enabled_checkbox_) {
    enabled_checkbox_->SetChecked(enabled);
  }
}

void AstraTabRuleItemView::SetName(const std::u16string& name) {
  name_ = name;
  if (name_label_) {
    name_label_->SetText(name);
  }
}

void AstraTabRuleItemView::BuildLayout() {
  SetPreferredSize(
      gfx::Size(kBubbleWidth - kSectionPadding * 2, kItemHeight));
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(10, 12),
      4));
  SetBorder(views::CreateRoundedRectBorder(
      1, 6, SK_ColorGRAY));

  // Top row: checkbox + name + match count.
  auto* top_row = AddChildView(std::make_unique<views::View>());
  top_row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(), 8));
  top_row->SetCrossAxisAlignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  enabled_checkbox_ = top_row->AddChildView(
      std::make_unique<views::Checkbox>(
          base::BindRepeating(
              &AstraTabRuleItemView::OnToggled,
              base::Unretained(this)),
          std::u16string()));
  enabled_checkbox_->SetChecked(enabled_);

  name_label_ = top_row->AddChildView(
      std::make_unique<views::Label>(name_));
  name_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  name_label_->SetAutoColorReadabilityEnabled(false);
  name_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));
  name_label_->SetElideBehavior(gfx::ELIDE_MIDDLE);
  name_label_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  match_label_ = top_row->AddChildView(
      std::make_unique<views::Label>(
          base::UTF8ToUTF16(
              std::to_string(match_count_) + " tabs")));
  match_label_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  match_label_->SetAutoColorReadabilityEnabled(false);
  match_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));

  // Trigger row.
  trigger_label_ = AddChildView(
      std::make_unique<views::Label>(
          u"Trigger: " + TriggerTypeLabel(trigger_type_) +
          u" — " + trigger_value_));
  trigger_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  trigger_label_->SetAutoColorReadabilityEnabled(false);
  trigger_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));

  // Action row.
  action_label_ = AddChildView(
      std::make_unique<views::Label>(
          u"Action: " + ActionTypeLabel(action_type_) +
          (action_value_.empty() ? std::u16string() : u" — " + action_value_)));
  action_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  action_label_->SetAutoColorReadabilityEnabled(false);
  action_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_SECONDARY));

  // Action buttons row.
  auto* actions_row = AddChildView(std::make_unique<views::View>());
  actions_row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(), 6));

  edit_button_ = actions_row->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraTabRuleItemView::OnEditClicked,
              base::Unretained(this)),
          u"Edit"));

  delete_button_ = actions_row->AddChildView(
      views::MdTextButton::CreateSecondaryUiButton(
          base::BindRepeating(
              &AstraTabRuleItemView::OnDeleteClicked,
              base::Unretained(this)),
          u"Delete"));
}

void AstraTabRuleItemView::OnToggled() {
  enabled_ = enabled_checkbox_->GetChecked();
  if (toggle_callback_) {
    toggle_callback_.Run(rule_id_, enabled_);
  }
}

void AstraTabRuleItemView::OnEditClicked() {
  if (edit_callback_) {
    edit_callback_.Run(rule_id_);
  }
}

void AstraTabRuleItemView::OnDeleteClicked() {
  if (delete_callback_) {
    delete_callback_.Run(rule_id_);
  }
}

std::u16string AstraTabRuleItemView::TriggerTypeLabel(TriggerType type) {
  switch (type) {
    case TriggerType::kUrlPattern:
      return u"URL pattern";
    case TriggerType::kDomain:
      return u"Domain";
    case TriggerType::kTitleContains:
      return u"Title contains";
    case TriggerType::kIdleTime:
      return u"Idle time";
  }
  return u"Unknown";
}

std::u16string AstraTabRuleItemView::ActionTypeLabel(ActionType type) {
  switch (type) {
    case ActionType::kMoveToWorkspace:
      return u"Move to workspace";
    case ActionType::kSleep:
      return u"Sleep tab";
    case ActionType::kPin:
      return u"Pin tab";
    case ActionType::kMute:
      return u"Mute tab";
    case ActionType::kGroup:
      return u"Group tab";
    case ActionType::kCloseAfter:
      return u"Close after";
  }
  return u"Unknown";
}

void AstraTabRuleItemView::OnThemeChanged() {
  views::View::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  name_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
  trigger_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));
  action_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));
  match_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForegroundSecondary));

  SetBackground(views::CreateRoundedRectBackground(
      cp->GetColor(ui::kColorDialogBackground), 6));
}

// ===========================================================================
// AstraTabRulesView
// ===========================================================================

AstraTabRulesView::AstraTabRulesView(views::View* anchor_view)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_RIGHT) {
  SetShowCloseButton(true);
  SetButtons(ui::DIALOG_BUTTON_NONE);
  set_fixed_width(kBubbleWidth);

  BuildUI();
}

AstraTabRulesView::~AstraTabRulesView() = default;

void AstraTabRulesView::SetRules(
    const std::vector<AstraTabRuleItemView::RuleInfo>& rules) {
  rules_ = rules;
  RefreshRules();
}

void AstraTabRulesView::SetAddRuleCallback(AddRuleCallback callback) {
  add_callback_ = std::move(callback);
}

void AstraTabRulesView::SetToggleRuleCallback(ToggleRuleCallback callback) {
  toggle_callback_ = std::move(callback);
}

void AstraTabRulesView::SetEditRuleCallback(EditRuleCallback callback) {
  edit_callback_ = std::move(callback);
}

void AstraTabRulesView::SetDeleteRuleCallback(DeleteRuleCallback callback) {
  delete_callback_ = std::move(callback);
}

void AstraTabRulesView::BuildUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  set_margins(gfx::Insets::VH(0, 0));

  BuildAddButton();
  BuildRulesList();
}

void AstraTabRulesView::BuildAddButton() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), 0));
  section->SetBorder(views::CreateSolidSidedBorder(
      0, 0, 1, 0, SK_ColorGRAY));

  add_button_ = section->AddChildView(
      views::MdTextButton::CreateProminent(
          base::BindRepeating(
              &AstraTabRulesView::OnAddRule,
              base::Unretained(this)),
          u"⚙️ Add Rule"));
  add_button_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kHorizontal,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));
}

void AstraTabRulesView::BuildRulesList() {
  auto* section = AddChildView(std::make_unique<views::View>());
  section->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(12, kSectionPadding), kItemSpacing));
  section->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  count_label_ = section->AddChildView(
      std::make_unique<views::Label>(u"Rules (0)"));
  count_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  count_label_->SetAutoColorReadabilityEnabled(false);
  count_label_->SetFontList(
      views::style::GetFont(
          views::style::CONTEXT_LABEL,
          views::style::STYLE_PRIMARY));

  auto* scroll_view = section->AddChildView(
      std::make_unique<views::ScrollView>());
  scroll_view->SetClipHeight(kItemHeight * kMaxVisibleItems +
                              kItemSpacing * (kMaxVisibleItems - 1));
  scroll_view->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(
          views::LayoutFlexOrientation::kVertical,
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded));

  rules_list_ = scroll_view->SetContents(
      std::make_unique<views::View>());
  rules_list_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(), kItemSpacing));
}

void AstraTabRulesView::RefreshRules() {
  if (!rules_list_) return;

  rules_list_->RemoveAllChildViews();
  rule_views_.clear();

  if (count_label_) {
    int enabled_count = 0;
    for (const auto& rule : rules_) {
      if (rule.enabled) enabled_count++;
    }
    count_label_->SetText(base::UTF8ToUTF16(
        "Rules (" + std::to_string(rules_.size()) +
        ", " + std::to_string(enabled_count) + " active)"));
  }

  // Sort: enabled first, then by name.
  auto sorted = rules_;
  std::sort(sorted.begin(), sorted.end(),
            [](const auto& a, const auto& b) {
              if (a.enabled != b.enabled) return a.enabled > b.enabled;
              return a.name < b.name;
            });

  for (const auto& rule : sorted) {
    auto* item = rules_list_->AddChildView(
        std::make_unique<AstraTabRuleItemView>(
            rule,
            base::BindRepeating(
                &AstraTabRulesView::OnToggleRule,
                base::Unretained(this)),
            base::BindRepeating(
                &AstraTabRulesView::OnEditRule,
                base::Unretained(this)),
            base::BindRepeating(
                &AstraTabRulesView::OnDeleteRule,
                base::Unretained(this))));
    rule_views_.push_back(item);
  }

  InvalidateLayout();
}

void AstraTabRulesView::OnAddRule() {
  if (add_callback_) {
    add_callback_.Run();
  }
}

void AstraTabRulesView::OnToggleRule(
    const std::string& rule_id, bool enabled) {
  if (toggle_callback_) {
    toggle_callback_.Run(rule_id, enabled);
  }
}

void AstraTabRulesView::OnEditRule(const std::string& rule_id) {
  if (edit_callback_) {
    edit_callback_.Run(rule_id);
  }
}

void AstraTabRulesView::OnDeleteRule(const std::string& rule_id) {
  if (delete_callback_) {
    delete_callback_.Run(rule_id);
  }
}

std::u16string AstraTabRulesView::GetWindowTitle() const {
  return u"Tab Rules";
}

void AstraTabRulesView::OnThemeChanged() {
  views::BubbleDialogDelegateView::OnThemeChanged();
  const auto* cp = GetColorProvider();
  if (!cp) return;

  count_label_->SetEnabledColor(
      cp->GetColor(ui::kColorLabelForeground));
}

}  // namespace astra
