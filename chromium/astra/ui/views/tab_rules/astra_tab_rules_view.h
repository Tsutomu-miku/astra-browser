// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_TAB_RULES_ASTRA_TAB_RULES_VIEW_H_
#define ASTRA_UI_VIEWS_TAB_RULES_ASTRA_TAB_RULES_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"

namespace views {
class BoxLayout;
class Checkbox;
class Label;
class MdTextButton;
class Textfield;
class Combobox;
}  // namespace views

namespace astra {

// =========================================================================
// AstraTabRuleItemView — single tab automation rule card
// =========================================================================
//
// A card showing one tab automation rule: name, trigger condition, action,
// and enable toggle.
//
// Layout:
//   +-------------------------------------------+
//   |  [x]  Auto-sleep work tabs                 |
//   |       Trigger: domain contains "work.com"  |
//   |       Action: sleep after 30m idle         |
//   |       [Edit] [Delete]                      |
//   +-------------------------------------------+
// =========================================================================

class AstraTabRuleItemView : public views::View {
 public:
  using ToggleCallback =
      base::RepeatingCallback<void(const std::string& rule_id, bool enabled)>;
  using EditCallback =
      base::RepeatingCallback<void(const std::string& rule_id)>;
  using DeleteCallback =
      base::RepeatingCallback<void(const std::string& rule_id)>;

  enum class TriggerType { kUrlPattern, kDomain, kTitleContains, kIdleTime };
  enum class ActionType {
    kMoveToWorkspace,
    kSleep,
    kPin,
    kMute,
    kGroup,
    kCloseAfter
  };

  struct RuleInfo {
    std::string rule_id;
    std::u16string name;
    bool enabled = true;
    TriggerType trigger_type = TriggerType::kDomain;
    std::u16string trigger_value;  // e.g. "work.com", "30m"
    ActionType action_type = ActionType::kSleep;
    std::u16string action_value;  // e.g. "Workspace Alpha", "30"
    int match_count = 0;  // Number of tabs currently matching
  };

  AstraTabRuleItemView(const RuleInfo& info,
                       ToggleCallback toggle_callback,
                       EditCallback edit_callback,
                       DeleteCallback delete_callback);
  ~AstraTabRuleItemView() override;

  AstraTabRuleItemView(const AstraTabRuleItemView&) = delete;
  AstraTabRuleItemView& operator=(const AstraTabRuleItemView&) = delete;

  const std::string& rule_id() const { return rule_id_; }
  bool enabled() const { return enabled_; }

  void SetEnabled(bool enabled);
  void SetName(const std::u16string& name);

  // -- views::View ---------------------------------------------------------

  void OnThemeChanged() override;

 private:
  void BuildLayout();
  void OnToggled();
  void OnEditClicked();
  void OnDeleteClicked();

  static std::u16 TriggerTypeLabel(TriggerType type);
  static std::u16 ActionTypeLabel(ActionType type);

  std::string rule_id_;
  std::u16string name_;
  bool enabled_ = true;
  TriggerType trigger_type_ = TriggerType::kDomain;
  std::u16string trigger_value_;
  ActionType action_type_ = ActionType::kSleep;
  std::u16string action_value_;
  int match_count_ = 0;

  ToggleCallback toggle_callback_;
  EditCallback edit_callback_;
  DeleteCallback delete_callback_;

  raw_ptr<views::Checkbox> enabled_checkbox_ = nullptr;
  raw_ptr<views::Label> name_label_ = nullptr;
  raw_ptr<views::Label> trigger_label_ = nullptr;
  raw_ptr<views::Label> action_label_ = nullptr;
  raw_ptr<views::Label> match_label_ = nullptr;
  raw_ptr<views::MdTextButton> edit_button_ = nullptr;
  raw_ptr<views::MdTextButton> delete_button_ = nullptr;
};

// =========================================================================
// AstraTabRulesView — tab automation rules panel
// =========================================================================
//
// A bubble showing tab automation rules: rules that automatically organize,
// sleep, or manage tabs based on triggers (URL pattern, domain, idle time).
//
// Layout:
//   +-------------------------------------------+
//   |  Tab Rules                    [Close]    |
//   +-------------------------------------------+
//   |  [ + Add rule ]                            |
//   +-------------------------------------------+
//   |  Active Rules (3)                          |
//   |  ┌─────────────────────────────────────┐  |
//   |  │ [x] Auto-sleep work tabs             │  |
//   |  │     Trigger: domain = work.com       │  |
//   |  │     Action: sleep after 30m idle     │  |
//   |  │     [Edit] [Delete]                  │  |
//   |  └─────────────────────────────────────┘  |
//   |  ┌─────────────────────────────────────┐  |
//   |  │ [x] Move news to Reading workspace   │  |
//   |  │     Trigger: domain contains news    │  |
//   |  │     Action: move to "Reading"        │  |
//   |  │     [Edit] [Delete]                  │  |
//   |  └─────────────────────────────────────┘  |
//   |  ...                                      |
//   +-------------------------------------------+
//
// This is a presentation-only view. Rule evaluation and execution is handled
// by Astra's tab rules service.
//
// Chromium subsystems reused:
//   - views::BubbleDialogDelegateView
//   - TabStripModel (for observing tab state)
// =========================================================================

class AstraTabRulesView : public views::BubbleDialogDelegateView {
 public:
  using AddRuleCallback = base::RepeatingClosure;
  using ToggleRuleCallback =
      base::RepeatingCallback<void(const std::string& rule_id, bool enabled)>;
  using EditRuleCallback =
      base::RepeatingCallback<void(const std::string& rule_id)>;
  using DeleteRuleCallback =
      base::RepeatingCallback<void(const std::string& rule_id)>;

  explicit AstraTabRulesView(views::View* anchor_view);
  ~AstraTabRulesView() override;

  AstraTabRulesView(const AstraTabRulesView&) = delete;
  AstraTabRulesView& operator=(const AstraTabRulesView&) = delete;

  // -- Data ----------------------------------------------------------------

  void SetRules(const std::vector<AstraTabRuleItemView::RuleInfo>& rules);

  // -- Callbacks -----------------------------------------------------------

  void SetAddRuleCallback(AddRuleCallback callback);
  void SetToggleRuleCallback(ToggleRuleCallback callback);
  void SetEditRuleCallback(EditRuleCallback callback);
  void SetDeleteRuleCallback(DeleteRuleCallback callback);

  // -- views::BubbleDialogDelegateView -------------------------------------

  std::u16string GetWindowTitle() const override;
  void OnThemeChanged() override;

 private:
  void BuildUI();
  void BuildAddButton();
  void BuildRulesList();

  void RefreshRules();

  void OnAddRule();
  void OnToggleRule(const std::string& rule_id, bool enabled);
  void OnEditRule(const std::string& rule_id);
  void OnDeleteRule(const std::string& rule_id);

  std::vector<AstraTabRuleItemView::RuleInfo> rules_;

  AddRuleCallback add_callback_;
  ToggleRuleCallback toggle_callback_;
  EditRuleCallback edit_callback_;
  DeleteRuleCallback delete_callback_;

  raw_ptr<views::MdTextButton> add_button_ = nullptr;
  raw_ptr<views::Label> count_label_ = nullptr;
  raw_ptr<views::View> rules_list_ = nullptr;

  std::vector<raw_ptr<AstraTabRuleItemView>> rule_views_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_TAB_RULES_ASTRA_TAB_RULES_VIEW_H_
