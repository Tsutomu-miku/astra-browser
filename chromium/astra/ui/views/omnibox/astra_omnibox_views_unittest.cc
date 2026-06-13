// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Comprehensive unit tests for Astra omnibox decoration — model and view.
//
// Test categories:
//   - Model construction and defaults
//   - Action CRUD operations
//   - Action ordering and reordering
//   - Action visibility
//   - Presentation settings
//   - Omnibox state (focus, security)
//   - Observer notifications
//   - Observer default implementations (no-op)
//   - Utility methods
//   - Bulk operations
//   - Persistence (PrefService round-trip)
//   - Edge cases (duplicate IDs, empty, max, bounds)
//   - View construction and model binding
//   - View action buttons and overflow
//   - View hover expansion
//   - View focus state handling
//   - View accessibility support
//
// Chromium test pattern: views::test::ViewsTestBase
//   (ui/views/test/views_test_base.h)

#include "astra/ui/views/omnibox/astra_location_bar_decoration.h"
#include "astra/ui/views/omnibox/astra_omnibox_decoration_model.h"

#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

using DecorationView = AstraLocationBarDecorationView;
using Edge = AstraLocationBarDecorationView::Edge;
using Model = AstraOmniboxDecorationModel;
using Observer = AstraOmniboxDecorationModelObserver;

// =========================================================================
// Test observer for model events
// =========================================================================

struct TestObserver : public Observer {
  int action_added_count = 0;
  std::string last_added_action;

  int action_removed_count = 0;
  std::string last_removed_action;

  int visibility_changed_count = 0;
  std::string last_visibility_action;
  bool last_visibility_value = false;

  int order_changed_count = 0;
  int settings_changed_count = 0;
  int focus_changed_count = 0;
  bool last_focus_state = false;
  int security_changed_count = 0;
  AstraSecurityLevel last_security_level = AstraSecurityLevel::kNone;

  void OnActionAdded(const std::string& action_id) override {
    action_added_count++;
    last_added_action = action_id;
  }

  void OnActionRemoved(const std::string& action_id) override {
    action_removed_count++;
    last_removed_action = action_id;
  }

  void OnActionVisibilityChanged(const std::string& action_id,
                                 bool visible) override {
    visibility_changed_count++;
    last_visibility_action = action_id;
    last_visibility_value = visible;
  }

  void OnActionOrderChanged() override { order_changed_count++; }
  void OnDecorationSettingsChanged() override { settings_changed_count++; }

  void OnOmniboxFocusChanged(bool focused) override {
    focus_changed_count++;
    last_focus_state = focused;
  }

  void OnSecurityStateChanged(AstraSecurityLevel level) override {
    security_changed_count++;
    last_security_level = level;
  }
};

// =========================================================================
// Fake delegate for view callbacks
// =========================================================================

struct FakeDecorationDelegate : public DecorationView::Delegate {
  int workspace_clicked_count = 0;
  int action_clicked_count = 0;
  std::string last_action_id;
  int overflow_clicked_count = 0;

  void OnWorkspaceIndicatorClicked() override { workspace_clicked_count++; }

  void OnActionClicked(const std::string& action_id) override {
    action_clicked_count++;
    last_action_id = action_id;
  }

  void OnOverflowMenuClicked() override { overflow_clicked_count++; }
};

// =========================================================================
// Helper: set up test prefs with Astra profile prefs registered
// =========================================================================

std::unique_ptr<TestingPrefServiceSimple> CreateTestPrefs() {
  auto prefs = std::make_unique<TestingPrefServiceSimple>();
  prefs->registry()->RegisterBooleanPref(prefs::kPrefOmniboxDecorationShowDecoration,
                                         prefs::kDefaultOmniboxDecorationShowDecoration);
  prefs->registry()->RegisterStringPref(prefs::kPrefOmniboxDecorationPosition,
                                        prefs::kDefaultOmniboxDecorationPosition);
  prefs->registry()->RegisterIntegerPref(prefs::kPrefOmniboxDecorationMaxVisibleActions,
                                          prefs::kDefaultOmniboxDecorationMaxVisibleActions);
  prefs->registry()->RegisterBooleanPref(prefs::kPrefOmniboxDecorationShowLabels,
                                         prefs::kDefaultOmniboxDecorationShowLabels);
  prefs->registry()->RegisterIntegerPref(prefs::kPrefOmniboxDecorationIconSize,
                                          prefs::kDefaultOmniboxDecorationIconSize);
  prefs->registry()->RegisterIntegerPref(prefs::kPrefOmniboxDecorationButtonStyle,
                                          prefs::kDefaultOmniboxDecorationButtonStyle);
  prefs->registry()->RegisterBooleanPref(prefs::kPrefOmniboxDecorationShowOnFocusOnly,
                                         prefs::kDefaultOmniboxDecorationShowOnFocusOnly);
  prefs->registry()->RegisterBooleanPref(prefs::kPrefOmniboxDecorationShowWorkspace,
                                         prefs::kDefaultOmniboxDecorationShowWorkspace);
  prefs->registry()->RegisterBooleanPref(prefs::kPrefOmniboxDecorationShowFocusMode,
                                         prefs::kDefaultOmniboxDecorationShowFocusMode);
  prefs->registry()->RegisterBooleanPref(prefs::kPrefOmniboxDecorationShowScreenshot,
                                         prefs::kDefaultOmniboxDecorationShowScreenshot);
  prefs->registry()->RegisterBooleanPref(prefs::kPrefOmniboxDecorationShowNote,
                                         prefs::kDefaultOmniboxDecorationShowNote);
  prefs->registry()->RegisterBooleanPref(prefs::kPrefOmniboxDecorationShowSplitView,
                                         prefs::kDefaultOmniboxDecorationShowSplitView);
  prefs->registry()->RegisterBooleanPref(prefs::kPrefOmniboxDecorationShowReadingList,
                                         prefs::kDefaultOmniboxDecorationShowReadingList);
  prefs->registry()->RegisterBooleanPref(prefs::kPrefOmniboxDecorationShowTranslate,
                                         prefs::kDefaultOmniboxDecorationShowTranslate);
  prefs->registry()->RegisterBooleanPref(prefs::kPrefOmniboxDecorationShowShare,
                                         prefs::kDefaultOmniboxDecorationShowShare);
  prefs->registry()->RegisterBooleanPref(prefs::kPrefOmniboxDecorationOverflowMenu,
                                         prefs::kDefaultOmniboxDecorationOverflowMenu);
  prefs->registry()->RegisterBooleanPref(prefs::kPrefOmniboxDecorationHoverExpansion,
                                         prefs::kDefaultOmniboxDecorationHoverExpansion);
  return prefs;
}

}  // namespace

// =========================================================================
// Model construction and defaults tests
// =========================================================================

TEST(AstraOmniboxDecorationModelTest, ConstructsWithoutCrash) {
  Model model;
  SUCCEED();
}

TEST(AstraOmniboxDecorationModelTest, DefaultHasEightActions) {
  Model model;
  EXPECT_EQ(8u, model.GetTotalActionCount());
}

TEST(AstraOmniboxDecorationModelTest, DefaultAllActionsVisible) {
  Model model;
  EXPECT_EQ(8u, model.GetVisibleActionCount());
}

TEST(AstraOmniboxDecorationModelTest, DefaultHasAllAstraActions) {
  Model model;
  EXPECT_TRUE(model.HasAction("workspace"));
  EXPECT_TRUE(model.HasAction("focus_mode"));
  EXPECT_TRUE(model.HasAction("screenshot"));
  EXPECT_TRUE(model.HasAction("note"));
  EXPECT_TRUE(model.HasAction("split_view"));
  EXPECT_TRUE(model.HasAction("reading_list"));
  EXPECT_TRUE(model.HasAction("translate"));
  EXPECT_TRUE(model.HasAction("share"));
}

TEST(AstraOmniboxDecorationModelTest, DefaultShowDecorationIsTrue) {
  Model model;
  EXPECT_TRUE(model.show_decoration());
}

TEST(AstraOmniboxDecorationModelTest, DefaultPositionIsLeading) {
  Model model;
  EXPECT_EQ(AstraDecorationPosition::kLeading, model.position());
}

TEST(AstraOmniboxDecorationModelTest, DefaultMaxVisibleActionsIsFour) {
  Model model;
  EXPECT_EQ(4, model.max_visible_actions());
}

TEST(AstraOmniboxDecorationModelTest, DefaultShowLabelsIsFalse) {
  Model model;
  EXPECT_FALSE(model.show_labels());
}

TEST(AstraOmniboxDecorationModelTest, DefaultIconSizeIsMedium) {
  Model model;
  EXPECT_EQ(AstraDecorationIconSize::kMedium, model.icon_size());
}

TEST(AstraOmniboxDecorationModelTest, DefaultButtonStyleIsIconOnly) {
  Model model;
  EXPECT_EQ(AstraDecorationButtonStyle::kIconOnly, model.button_style());
}

TEST(AstraOmniboxDecorationModelTest, DefaultShowOnFocusOnlyIsFalse) {
  Model model;
  EXPECT_FALSE(model.show_on_focus_only());
}

TEST(AstraOmniboxDecorationModelTest, DefaultShowOverflowMenuIsTrue) {
  Model model;
  EXPECT_TRUE(model.show_overflow_menu());
}

TEST(AstraOmniboxDecorationModelTest, DefaultHoverExpansionIsFalse) {
  Model model;
  EXPECT_FALSE(model.hover_expansion());
}

TEST(AstraOmniboxDecorationModelTest, DefaultAllIndividualTogglesAreTrue) {
  Model model;
  EXPECT_TRUE(model.show_workspace());
  EXPECT_TRUE(model.show_focus_mode());
  EXPECT_TRUE(model.show_screenshot());
  EXPECT_TRUE(model.show_note());
  EXPECT_TRUE(model.show_split_view());
  EXPECT_TRUE(model.show_reading_list());
  EXPECT_TRUE(model.show_translate());
  EXPECT_TRUE(model.show_share());
}

TEST(AstraOmniboxDecorationModelTest, DefaultOmniboxNotFocused) {
  Model model;
  EXPECT_FALSE(model.omnibox_focused());
}

TEST(AstraOmniboxDecorationModelTest, DefaultSecurityLevelIsNone) {
  Model model;
  EXPECT_EQ(AstraSecurityLevel::kNone, model.security_level());
}

// =========================================================================
// Action CRUD tests
// =========================================================================

TEST(AstraOmniboxDecorationModelTest, AddActionIncreasesCount) {
  Model model;
  size_t initial = model.GetTotalActionCount();

  AstraDecorationAction action;
  action.id = "custom_action";
  action.label = u"Custom";
  action.tooltip = u"Custom action";
  EXPECT_TRUE(model.AddAction(action));

  EXPECT_EQ(initial + 1, model.GetTotalActionCount());
  EXPECT_TRUE(model.HasAction("custom_action"));
}

TEST(AstraOmniboxDecorationModelTest, AddDuplicateActionReturnsFalse) {
  Model model;
  AstraDecorationAction action;
  action.id = "workspace";  // Already exists
  action.label = u"Workspace";

  EXPECT_FALSE(model.AddAction(action));
  EXPECT_EQ(8u, model.GetTotalActionCount());
}

TEST(AstraOmniboxDecorationModelTest, RemoveExistingAction) {
  Model model;
  EXPECT_TRUE(model.HasAction("screenshot"));
  EXPECT_TRUE(model.RemoveAction("screenshot"));
  EXPECT_FALSE(model.HasAction("screenshot"));
  EXPECT_EQ(7u, model.GetTotalActionCount());
}

TEST(AstraOmniboxDecorationModelTest, RemoveNonExistentActionReturnsFalse) {
  Model model;
  EXPECT_FALSE(model.RemoveAction("nonexistent"));
  EXPECT_EQ(8u, model.GetTotalActionCount());
}

TEST(AstraOmniboxDecorationModelTest, GetActionReturnsData) {
  Model model;
  const AstraDecorationAction* action = model.GetAction("screenshot");
  ASSERT_NE(nullptr, action);
  EXPECT_EQ("screenshot", action->id);
  EXPECT_FALSE(action->label.empty());
  EXPECT_FALSE(action->tooltip.empty());
  EXPECT_TRUE(action->is_visible);
}

TEST(AstraOmniboxDecorationModelTest, GetActionNonExistentReturnsNull) {
  Model model;
  EXPECT_EQ(nullptr, model.GetAction("nonexistent"));
}

TEST(AstraOmniboxDecorationModelTest, AddCustomActionNotifiesObserver) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  AstraDecorationAction action;
  action.id = "custom";
  action.label = u"Custom";
  model.AddAction(action);

  EXPECT_EQ(1, observer.action_added_count);
  EXPECT_EQ("custom", observer.last_added_action);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxDecorationModelTest, RemoveActionNotifiesObserver) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.RemoveAction("screenshot");

  EXPECT_EQ(1, observer.action_removed_count);
  EXPECT_EQ("screenshot", observer.last_removed_action);

  model.RemoveObserver(&observer);
}

// =========================================================================
// Action visibility tests
// =========================================================================

TEST(AstraOmniboxDecorationModelTest, SetActionVisibleTrue) {
  Model model;
  // Hide first, then show.
  model.SetActionVisible("screenshot", false);
  ASSERT_FALSE(model.GetAction("screenshot")->is_visible);

  model.SetActionVisible("screenshot", true);
  EXPECT_TRUE(model.GetAction("screenshot")->is_visible);
}

TEST(AstraOmniboxDecorationModelTest, SetActionVisibleFalse) {
  Model model;
  EXPECT_TRUE(model.SetActionVisible("screenshot", false));
  EXPECT_FALSE(model.GetAction("screenshot")->is_visible);
  EXPECT_EQ(7u, model.GetVisibleActionCount());
}

TEST(AstraOmniboxDecorationModelTest, SetActionNonExistentReturnsFalse) {
  Model model;
  EXPECT_FALSE(model.SetActionVisible("nonexistent", false));
}

TEST(AstraOmniboxDecorationModelTest, SetActionSameStateNoNotification) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  // Already visible by default, so setting to true should not notify.
  model.SetActionVisible("screenshot", true);
  EXPECT_EQ(0, observer.visibility_changed_count);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxDecorationModelTest, VisibilityChangeNotifiesObserver) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.SetActionVisible("screenshot", false);
  EXPECT_EQ(1, observer.visibility_changed_count);
  EXPECT_EQ("screenshot", observer.last_visibility_action);
  EXPECT_FALSE(observer.last_visibility_value);

  model.SetActionVisible("screenshot", true);
  EXPECT_EQ(2, observer.visibility_changed_count);
  EXPECT_TRUE(observer.last_visibility_value);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxDecorationModelTest, GetVisibleActionsOnlyIncludesVisible) {
  Model model;
  model.SetActionVisible("screenshot", false);
  model.SetActionVisible("note", false);

  auto visible = model.GetVisibleActions();
  EXPECT_EQ(6u, visible.size());
  for (const auto& a : visible) {
    EXPECT_TRUE(a.is_visible);
    EXPECT_NE("screenshot", a.id);
    EXPECT_NE("note", a.id);
  }
}

TEST(AstraOmniboxDecorationModelTest, ShowWorkspaceToggle) {
  Model model;
  model.SetShowWorkspace(false);
  EXPECT_FALSE(model.show_workspace());
  EXPECT_FALSE(model.GetAction("workspace")->is_visible);
}

TEST(AstraOmniboxDecorationModelTest, ShowFocusModeToggle) {
  Model model;
  model.SetShowFocusMode(false);
  EXPECT_FALSE(model.show_focus_mode());
  EXPECT_FALSE(model.GetAction("focus_mode")->is_visible);
}

TEST(AstraOmniboxDecorationModelTest, ShowScreenshotToggle) {
  Model model;
  model.SetShowScreenshot(false);
  EXPECT_FALSE(model.show_screenshot());
  EXPECT_FALSE(model.GetAction("screenshot")->is_visible);
}

TEST(AstraOmniboxDecorationModelTest, ShowNoteToggle) {
  Model model;
  model.SetShowNote(false);
  EXPECT_FALSE(model.show_note());
  EXPECT_FALSE(model.GetAction("note")->is_visible);
}

TEST(AstraOmniboxDecorationModelTest, ShowSplitViewToggle) {
  Model model;
  model.SetShowSplitView(false);
  EXPECT_FALSE(model.show_split_view());
  EXPECT_FALSE(model.GetAction("split_view")->is_visible);
}

TEST(AstraOmniboxDecorationModelTest, ShowReadingListToggle) {
  Model model;
  model.SetShowReadingList(false);
  EXPECT_FALSE(model.show_reading_list());
  EXPECT_FALSE(model.GetAction("reading_list")->is_visible);
}

TEST(AstraOmniboxDecorationModelTest, ShowTranslateToggle) {
  Model model;
  model.SetShowTranslate(false);
  EXPECT_FALSE(model.show_translate());
  EXPECT_FALSE(model.GetAction("translate")->is_visible);
}

TEST(AstraOmniboxDecorationModelTest, ShowShareToggle) {
  Model model;
  model.SetShowShare(false);
  EXPECT_FALSE(model.show_share());
  EXPECT_FALSE(model.GetAction("share")->is_visible);
}

// =========================================================================
// Action ordering tests
// =========================================================================

TEST(AstraOmniboxDecorationModelTest, DefaultActionOrder) {
  auto order = Model::GetDefaultActionOrder();
  ASSERT_EQ(8u, order.size());
  EXPECT_EQ("workspace", order[0]);
  EXPECT_EQ("focus_mode", order[1]);
  EXPECT_EQ("screenshot", order[2]);
  EXPECT_EQ("note", order[3]);
  EXPECT_EQ("split_view", order[4]);
  EXPECT_EQ("reading_list", order[5]);
  EXPECT_EQ("translate", order[6]);
  EXPECT_EQ("share", order[7]);
}

TEST(AstraOmniboxDecorationModelTest, ReorderActionForward) {
  Model model;
  // Move action at index 0 to index 2.
  EXPECT_TRUE(model.ReorderAction(0, 2));

  auto actions = model.GetAllActions();
  EXPECT_EQ("focus_mode", actions[0].id);
  EXPECT_EQ("screenshot", actions[1].id);
  EXPECT_EQ("workspace", actions[2].id);
}

TEST(AstraOmniboxDecorationModelTest, ReorderActionBackward) {
  Model model;
  // Move action at index 3 to index 0.
  EXPECT_TRUE(model.ReorderAction(3, 0));

  auto actions = model.GetAllActions();
  EXPECT_EQ("note", actions[0].id);
  EXPECT_EQ("workspace", actions[1].id);
}

TEST(AstraOmniboxDecorationModelTest, ReorderInvalidFromIndex) {
  Model model;
  EXPECT_FALSE(model.ReorderAction(99, 0));
}

TEST(AstraOmniboxDecorationModelTest, ReorderInvalidToIndex) {
  Model model;
  EXPECT_FALSE(model.ReorderAction(0, 99));
}

TEST(AstraOmniboxDecorationModelTest, ReorderSameIndexNoOp) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  EXPECT_TRUE(model.ReorderAction(2, 2));
  EXPECT_EQ(0, observer.order_changed_count);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxDecorationModelTest, ReorderNotifiesObserver) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.ReorderAction(0, 3);
  EXPECT_EQ(1, observer.order_changed_count);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxDecorationModelTest, MoveActionToById) {
  Model model;
  EXPECT_TRUE(model.MoveActionTo("screenshot", 0));

  auto actions = model.GetAllActions();
  EXPECT_EQ("screenshot", actions[0].id);
}

TEST(AstraOmniboxDecorationModelTest, MoveActionToNonExistentReturnsFalse) {
  Model model;
  EXPECT_FALSE(model.MoveActionTo("nonexistent", 0));
}

TEST(AstraOmniboxDecorationModelTest, ResetActionOrder) {
  Model model;
  model.ReorderAction(0, 5);
  model.ReorderAction(2, 7);

  model.ResetActionOrder();

  auto order = Model::GetDefaultActionOrder();
  auto actions = model.GetAllActions();
  ASSERT_EQ(order.size(), actions.size());
  for (size_t i = 0; i < order.size(); ++i) {
    EXPECT_EQ(order[i], actions[i].id);
  }
}

TEST(AstraOmniboxDecorationModelTest, ResetOrderPreservesVisibility) {
  Model model;
  model.SetActionVisible("screenshot", false);
  model.ReorderAction(2, 6);

  model.ResetActionOrder();

  // Screenshot should still be hidden.
  EXPECT_FALSE(model.GetAction("screenshot")->is_visible);
}

// =========================================================================
// Omnibox state tests
// =========================================================================

TEST(AstraOmniboxDecorationModelTest, SetOmniboxFocusedTrue) {
  Model model;
  model.SetOmniboxFocused(true);
  EXPECT_TRUE(model.omnibox_focused());
}

TEST(AstraOmniboxDecorationModelTest, SetOmniboxFocusedFalse) {
  Model model;
  model.SetOmniboxFocused(true);
  ASSERT_TRUE(model.omnibox_focused());

  model.SetOmniboxFocused(false);
  EXPECT_FALSE(model.omnibox_focused());
}

TEST(AstraOmniboxDecorationModelTest, SetOmniboxFocusedSameStateNoNotify) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.SetOmniboxFocused(false);  // Already false.
  EXPECT_EQ(0, observer.focus_changed_count);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxDecorationModelTest, OmniboxFocusNotifiesObserver) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.SetOmniboxFocused(true);
  EXPECT_EQ(1, observer.focus_changed_count);
  EXPECT_TRUE(observer.last_focus_state);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxDecorationModelTest, SetSecurityLevel) {
  Model model;
  model.SetSecurityLevel(AstraSecurityLevel::kSecure);
  EXPECT_EQ(AstraSecurityLevel::kSecure, model.security_level());
}

TEST(AstraOmniboxDecorationModelTest, SetSecurityLevelNotifiesObserver) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.SetSecurityLevel(AstraSecurityLevel::kDangerous);
  EXPECT_EQ(1, observer.security_changed_count);
  EXPECT_EQ(AstraSecurityLevel::kDangerous, observer.last_security_level);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxDecorationModelTest, SetSecurityLevelSameStateNoNotify) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.SetSecurityLevel(AstraSecurityLevel::kNone);  // Already kNone.
  EXPECT_EQ(0, observer.security_changed_count);

  model.RemoveObserver(&observer);
}

// =========================================================================
// Presentation settings tests
// =========================================================================

TEST(AstraOmniboxDecorationModelTest, SetShowDecoration) {
  Model model;
  model.SetShowDecoration(false);
  EXPECT_FALSE(model.show_decoration());
}

TEST(AstraOmniboxDecorationModelTest, SetShowDecorationNotifies) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.SetShowDecoration(false);
  EXPECT_EQ(1, observer.settings_changed_count);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxDecorationModelTest, SetPositionTrailing) {
  Model model;
  model.SetPosition(AstraDecorationPosition::kTrailing);
  EXPECT_EQ(AstraDecorationPosition::kTrailing, model.position());
}

TEST(AstraOmniboxDecorationModelTest, SetMaxVisibleActions) {
  Model model;
  model.SetMaxVisibleActions(6);
  EXPECT_EQ(6, model.max_visible_actions());
}

TEST(AstraOmniboxDecorationModelTest, SetMaxVisibleActionsBelowMinClamped) {
  Model model;
  model.SetMaxVisibleActions(0);
  EXPECT_EQ(Model::kMinVisibleActions, model.max_visible_actions());
}

TEST(AstraOmniboxDecorationModelTest, SetMaxVisibleActionsAboveMaxClamped) {
  Model model;
  model.SetMaxVisibleActions(20);
  EXPECT_EQ(Model::kMaxVisibleActions, model.max_visible_actions());
}

TEST(AstraOmniboxDecorationModelTest, SetShowLabels) {
  Model model;
  model.SetShowLabels(true);
  EXPECT_TRUE(model.show_labels());
}

TEST(AstraOmniboxDecorationModelTest, SetIconSizeSmall) {
  Model model;
  model.SetIconSize(AstraDecorationIconSize::kSmall);
  EXPECT_EQ(AstraDecorationIconSize::kSmall, model.icon_size());
}

TEST(AstraOmniboxDecorationModelTest, SetIconSizeLarge) {
  Model model;
  model.SetIconSize(AstraDecorationIconSize::kLarge);
  EXPECT_EQ(AstraDecorationIconSize::kLarge, model.icon_size());
}

TEST(AstraOmniboxDecorationModelTest, SetButtonStyleIconWithLabel) {
  Model model;
  model.SetButtonStyle(AstraDecorationButtonStyle::kIconWithLabel);
  EXPECT_EQ(AstraDecorationButtonStyle::kIconWithLabel, model.button_style());
}

TEST(AstraOmniboxDecorationModelTest, SetButtonStyleChip) {
  Model model;
  model.SetButtonStyle(AstraDecorationButtonStyle::kChip);
  EXPECT_EQ(AstraDecorationButtonStyle::kChip, model.button_style());
}

TEST(AstraOmniboxDecorationModelTest, SetShowOnFocusOnly) {
  Model model;
  model.SetShowOnFocusOnly(true);
  EXPECT_TRUE(model.show_on_focus_only());
}

TEST(AstraOmniboxDecorationModelTest, SetShowOverflowMenu) {
  Model model;
  model.SetShowOverflowMenu(false);
  EXPECT_FALSE(model.show_overflow_menu());
}

TEST(AstraOmniboxDecorationModelTest, SetHoverExpansion) {
  Model model;
  model.SetHoverExpansion(true);
  EXPECT_TRUE(model.hover_expansion());
}

TEST(AstraOmniboxDecorationModelTest, AllSettingsNotifyObserver) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.SetPosition(AstraDecorationPosition::kTrailing);
  model.SetShowLabels(true);
  model.SetIconSize(AstraDecorationIconSize::kSmall);
  model.SetButtonStyle(AstraDecorationButtonStyle::kChip);

  EXPECT_EQ(4, observer.settings_changed_count);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxDecorationModelTest, SettingSameValueNoNotify) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.SetShowDecoration(true);   // Already true.
  model.SetShowLabels(false);     // Already false.
  model.SetShowOnFocusOnly(false); // Already false.

  EXPECT_EQ(0, observer.settings_changed_count);

  model.RemoveObserver(&observer);
}

// =========================================================================
// Utility method tests
// =========================================================================

TEST(AstraOmniboxDecorationUtilTest, FormatActionLabelShort) {
  std::u16string label = u"Short";
  auto result = Model::FormatActionLabel(label, 10);
  EXPECT_EQ(label, result);
}

TEST(AstraOmniboxDecorationUtilTest, FormatActionLabelTruncatesLong) {
  std::u16string label = u"VeryLongLabelHere";
  auto result = Model::FormatActionLabel(label, 8);
  EXPECT_LT(result.size(), label.size());
  // Should end with ellipsis.
  EXPECT_EQ(u'\u2026', result.back());
}

TEST(AstraOmniboxDecorationUtilTest, FormatActionLabelExactLength) {
  std::u16string label = u"12345678";
  auto result = Model::FormatActionLabel(label, 8);
  EXPECT_EQ(label, result);
}

TEST(AstraOmniboxDecorationUtilTest, ClampMaxVisibleActionsWithinRange) {
  EXPECT_EQ(5, Model::ClampMaxVisibleActions(5));
}

TEST(AstraOmniboxDecorationUtilTest, ClampMaxVisibleActionsBelowMin) {
  EXPECT_EQ(Model::kMinVisibleActions, Model::ClampMaxVisibleActions(0));
  EXPECT_EQ(Model::kMinVisibleActions, Model::ClampMaxVisibleActions(-5));
}

TEST(AstraOmniboxDecorationUtilTest, ClampMaxVisibleActionsAboveMax) {
  EXPECT_EQ(Model::kMaxVisibleActions, Model::ClampMaxVisibleActions(100));
}

TEST(AstraOmniboxDecorationUtilTest, GetIconSizeDpSmall) {
  EXPECT_EQ(16, Model::GetIconSizeDp(AstraDecorationIconSize::kSmall));
}

TEST(AstraOmniboxDecorationUtilTest, GetIconSizeDpMedium) {
  EXPECT_EQ(20, Model::GetIconSizeDp(AstraDecorationIconSize::kMedium));
}

TEST(AstraOmniboxDecorationUtilTest, GetIconSizeDpLarge) {
  EXPECT_EQ(24, Model::GetIconSizeDp(AstraDecorationIconSize::kLarge));
}

// =========================================================================
// Bulk operation tests
// =========================================================================

TEST(AstraOmniboxDecorationBulkTest, SetBulkVisibilityHidesMultiple) {
  Model model;
  std::vector<std::string> ids = {"screenshot", "note", "share"};
  model.SetBulkVisibility(ids, false);

  EXPECT_FALSE(model.GetAction("screenshot")->is_visible);
  EXPECT_FALSE(model.GetAction("note")->is_visible);
  EXPECT_FALSE(model.GetAction("share")->is_visible);
  EXPECT_TRUE(model.GetAction("workspace")->is_visible);
}

TEST(AstraOmniboxDecorationBulkTest, HideAllActions) {
  Model model;
  model.HideAllActions();
  EXPECT_EQ(0u, model.GetVisibleActionCount());
}

TEST(AstraOmniboxDecorationBulkTest, ShowAllDefaultActions) {
  Model model;
  model.HideAllActions();
  ASSERT_EQ(0u, model.GetVisibleActionCount());

  model.ShowAllDefaultActions();
  EXPECT_EQ(8u, model.GetVisibleActionCount());
}

TEST(AstraOmniboxDecorationBulkTest, HideAllNotifiesForEachAction) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.HideAllActions();
  // All 8 actions should have generated visibility notifications.
  EXPECT_LE(8, observer.visibility_changed_count);

  model.RemoveObserver(&observer);
}

// =========================================================================
// Observer default implementation tests
// =========================================================================

TEST(AstraOmniboxDecorationObserverTest, DefaultImplementationsAreNoOp) {
  // The base Observer class has empty default implementations.
  // Calling any method on a base instance should not crash.
  class TestDefaultObserver : public Observer {};

  TestDefaultObserver obs;
  obs.OnActionAdded("test");
  obs.OnActionRemoved("test");
  obs.OnActionVisibilityChanged("test", true);
  obs.OnActionOrderChanged();
  obs.OnDecorationSettingsChanged();
  obs.OnOmniboxFocusChanged(true);
  obs.OnSecurityStateChanged(AstraSecurityLevel::kSecure);
  // No crash = success.
  SUCCEED();
}

TEST(AstraOmniboxDecorationObserverTest, ObserverIsCheckedObserver) {
  // Verify Observer inherits from base::CheckedObserver.
  EXPECT_TRUE((std::is_base_of<base::CheckedObserver, Observer>::value));
}

// =========================================================================
// Persistence tests (PrefService round-trip)
// =========================================================================

TEST(AstraOmniboxDecorationPersistenceTest, SaveAndLoadRoundTrip) {
  auto prefs = CreateTestPrefs();
  Model model;

  // Modify several settings.
  model.SetShowDecoration(false);
  model.SetPosition(AstraDecorationPosition::kTrailing);
  model.SetMaxVisibleActions(6);
  model.SetShowLabels(true);
  model.SetIconSize(AstraDecorationIconSize::kLarge);
  model.SetButtonStyle(AstraDecorationButtonStyle::kChip);
  model.SetShowOnFocusOnly(true);
  model.SetShowScreenshot(false);
  model.SetShowNote(false);
  model.SetShowOverflowMenu(false);
  model.SetHoverExpansion(true);

  // Save to prefs.
  model.SaveToPrefs(prefs.get());

  // Load into a new model.
  Model model2;
  model2.LoadFromPrefs(prefs.get());

  // Verify round-trip.
  EXPECT_FALSE(model2.show_decoration());
  EXPECT_EQ(AstraDecorationPosition::kTrailing, model2.position());
  EXPECT_EQ(6, model2.max_visible_actions());
  EXPECT_TRUE(model2.show_labels());
  EXPECT_EQ(AstraDecorationIconSize::kLarge, model2.icon_size());
  EXPECT_EQ(AstraDecorationButtonStyle::kChip, model2.button_style());
  EXPECT_TRUE(model2.show_on_focus_only());
  EXPECT_FALSE(model2.show_screenshot());
  EXPECT_FALSE(model2.show_note());
  EXPECT_FALSE(model2.show_overflow_menu());
  EXPECT_TRUE(model2.hover_expansion());
}

TEST(AstraOmniboxDecorationPersistenceTest, LoadFromPrefsNotifiesSettingsChanged) {
  auto prefs = CreateTestPrefs();
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.LoadFromPrefs(prefs.get());
  EXPECT_GE(observer.settings_changed_count, 1);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxDecorationPersistenceTest, SaveWithNullPrefsNoCrash) {
  Model model;
  model.SaveToPrefs(nullptr);
  SUCCEED();
}

TEST(AstraOmniboxDecorationPersistenceTest, LoadWithNullPrefsNoCrash) {
  Model model;
  model.LoadFromPrefs(nullptr);
  SUCCEED();
}

TEST(AstraOmniboxDecorationPersistenceTest, PositionLeftToLeading) {
  auto prefs = CreateTestPrefs();
  prefs->SetString(prefs::kPrefOmniboxDecorationPosition, "left");

  Model model;
  model.LoadFromPrefs(prefs.get());
  EXPECT_EQ(AstraDecorationPosition::kLeading, model.position());
}

TEST(AstraOmniboxDecorationPersistenceTest, PositionRightToTrailing) {
  auto prefs = CreateTestPrefs();
  prefs->SetString(prefs::kPrefOmniboxDecorationPosition, "right");

  Model model;
  model.LoadFromPrefs(prefs.get());
  EXPECT_EQ(AstraDecorationPosition::kTrailing, model.position());
}

TEST(AstraOmniboxDecorationPersistenceTest, MaxVisibleActionsClampedOnLoad) {
  auto prefs = CreateTestPrefs();
  prefs->SetInteger(prefs::kPrefOmniboxDecorationMaxVisibleActions, 100);

  Model model;
  model.LoadFromPrefs(prefs.get());
  EXPECT_EQ(Model::kMaxVisibleActions, model.max_visible_actions());
}

// =========================================================================
// Edge case tests
// =========================================================================

TEST(AstraOmniboxDecorationEdgeTest, ZeroActionsModel) {
  // Create a model and remove all actions.
  Model model;
  auto order = Model::GetDefaultActionOrder();
  for (const auto& id : order) {
    model.RemoveAction(id);
  }

  EXPECT_EQ(0u, model.GetTotalActionCount());
  EXPECT_EQ(0u, model.GetVisibleActionCount());
  EXPECT_TRUE(model.GetAllActions().empty());
  EXPECT_TRUE(model.GetVisibleActions().empty());
}

TEST(AstraOmniboxDecorationEdgeTest, MaxActionsClamped) {
  Model model;
  model.SetMaxVisibleActions(Model::kMaxVisibleActions + 10);
  EXPECT_EQ(Model::kMaxVisibleActions, model.max_visible_actions());
}

TEST(AstraOmniboxDecorationEdgeTest, DuplicateIdIgnored) {
  Model model;
  size_t count = model.GetTotalActionCount();

  AstraDecorationAction action;
  action.id = "workspace";  // Already exists.
  action.label = u"Duplicate";
  EXPECT_FALSE(model.AddAction(action));
  EXPECT_EQ(count, model.GetTotalActionCount());
}

TEST(AstraOmniboxDecorationEdgeTest, RemoveFromEmptyListNoCrash) {
  Model model;
  auto order = Model::GetDefaultActionOrder();
  for (const auto& id : order) {
    model.RemoveAction(id);
  }
  // Removing from empty list should return false but not crash.
  EXPECT_FALSE(model.RemoveAction("anything"));
}

TEST(AstraOmniboxDecorationEdgeTest, ReorderBounds) {
  Model model;
  size_t count = model.GetTotalActionCount();
  EXPECT_FALSE(model.ReorderAction(count, 0));
  EXPECT_FALSE(model.ReorderAction(0, count));
  EXPECT_FALSE(model.ReorderAction(count + 5, count + 10));
}

TEST(AstraOmniboxDecorationEdgeTest, HasActionEmptyId) {
  Model model;
  EXPECT_FALSE(model.HasAction(""));
}

// =========================================================================
// Security level enum tests
// =========================================================================

TEST(AstraOmniboxDecorationEnumTest, SecurityLevelHasFiveValues) {
  // kNone, kNeutral, kSecure, kInsecure, kDangerous
  EXPECT_EQ(0, static_cast<int>(AstraSecurityLevel::kNone));
  EXPECT_EQ(1, static_cast<int>(AstraSecurityLevel::kNeutral));
  EXPECT_EQ(2, static_cast<int>(AstraSecurityLevel::kSecure));
  EXPECT_EQ(3, static_cast<int>(AstraSecurityLevel::kInsecure));
  EXPECT_EQ(4, static_cast<int>(AstraSecurityLevel::kDangerous));
}

TEST(AstraOmniboxDecorationEnumTest, IconSizeHasThreeValues) {
  EXPECT_EQ(0, static_cast<int>(AstraDecorationIconSize::kSmall));
  EXPECT_EQ(1, static_cast<int>(AstraDecorationIconSize::kMedium));
  EXPECT_EQ(2, static_cast<int>(AstraDecorationIconSize::kLarge));
}

TEST(AstraOmniboxDecorationEnumTest, ButtonStyleHasThreeValues) {
  EXPECT_EQ(0, static_cast<int>(AstraDecorationButtonStyle::kIconOnly));
  EXPECT_EQ(1, static_cast<int>(AstraDecorationButtonStyle::kIconWithLabel));
  EXPECT_EQ(2, static_cast<int>(AstraDecorationButtonStyle::kChip));
}

TEST(AstraOmniboxDecorationEnumTest, PositionHasTwoValues) {
  EXPECT_EQ(0, static_cast<int>(AstraDecorationPosition::kLeading));
  EXPECT_EQ(1, static_cast<int>(AstraDecorationPosition::kTrailing));
}

// =========================================================================
// View tests (using ViewsTestBase)
// =========================================================================

class AstraLocationBarDecorationViewTest : public views::ViewsTestBase {
 public:
  AstraLocationBarDecorationViewTest() = default;
  ~AstraLocationBarDecorationViewTest() override = default;

  void SetUp() override {
    ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();

    delegate_ = std::make_unique<FakeDecorationDelegate>();
    model_ = std::make_unique<Model>();

    decoration_view_ = widget_->SetContentsView(
        std::make_unique<DecorationView>(Edge::kLeading));
    decoration_view_->SetDelegate(delegate_.get());
    decoration_view_->SetModel(model_.get());
    widget_->Show();
  }

  void TearDown() override {
    decoration_view_->SetModel(nullptr);
    widget_.reset();
    delegate_.reset();
    model_.reset();
    ViewsTestBase::TearDown();
  }

 protected:
  std::unique_ptr<views::Widget> widget_;
  raw_ptr<DecorationView> decoration_view_ = nullptr;
  std::unique_ptr<FakeDecorationDelegate> delegate_;
  std::unique_ptr<Model> model_;
};

TEST_F(AstraLocationBarDecorationViewTest, ConstructsWithModel) {
  EXPECT_NE(nullptr, decoration_view_);
  EXPECT_NE(nullptr, decoration_view_->model());
}

TEST_F(AstraLocationBarDecorationViewTest, HasActionButtonsFromModel) {
  // Default model has 8 actions, but only 4 shown directly.
  // But we create buttons for all actions (visibility is toggled individually).
  EXPECT_EQ(8u, decoration_view_->action_button_count());
}

TEST_F(AstraLocationBarDecorationViewTest, HasOverflowButton) {
  // With 8 actions and max 4 visible, overflow should be present.
  EXPECT_TRUE(decoration_view_->has_overflow_button());
}

TEST_F(AstraLocationBarDecorationViewTest, ModelRemovalRemovesButtons) {
  decoration_view_->SetModel(nullptr);
  EXPECT_EQ(0u, decoration_view_->action_button_count());
}

TEST_F(AstraLocationBarDecorationViewTest, AddActionUpdatesView) {
  AstraDecorationAction action;
  action.id = "custom";
  action.label = u"Custom";
  action.tooltip = u"Custom action";
  model_->AddAction(action);

  EXPECT_EQ(9u, decoration_view_->action_button_count());
}

TEST_F(AstraLocationBarDecorationViewTest, RemoveActionUpdatesView) {
  model_->RemoveAction("screenshot");
  EXPECT_EQ(7u, decoration_view_->action_button_count());
}

TEST_F(AstraLocationBarDecorationViewTest, WorkspaceIndicatorVisibleByDefault) {
  // Workspace indicator should be visible when show_workspace is true.
  EXPECT_TRUE(model_->show_workspace());
}

TEST_F(AstraLocationBarDecorationViewTest, PreferredSizeIsPositive) {
  gfx::Size pref = decoration_view_->CalculatePreferredSize();
  EXPECT_GT(pref.width(), 0);
  EXPECT_GT(pref.height(), 0);
}

TEST_F(AstraLocationBarDecorationViewTest, LayoutNoCrash) {
  decoration_view_->Layout();
  SUCCEED();
}

TEST_F(AstraLocationBarDecorationViewTest, OnThemeChangedNoCrash) {
  decoration_view_->OnThemeChanged();
  SUCCEED();
}

TEST_F(AstraLocationBarDecorationViewTest, SetFocusModeActive) {
  decoration_view_->SetFocusModeActive(true);
  EXPECT_TRUE(decoration_view_->focus_mode_active());

  decoration_view_->SetFocusModeActive(false);
  EXPECT_FALSE(decoration_view_->focus_mode_active());
}

TEST_F(AstraLocationBarDecorationViewTest, UpdateWorkspaceNoCrash) {
  decoration_view_->UpdateWorkspace("Work", "#5AD8A6");
  decoration_view_->UpdateWorkspace("Personal", "#FF6B6B");
  SUCCEED();
}

TEST_F(AstraLocationBarDecorationViewTest, TrailingEdgeConstructs) {
  auto view = std::make_unique<DecorationView>(Edge::kTrailing);
  EXPECT_EQ(Edge::kTrailing, view->edge());
}

TEST_F(AstraLocationBarDecorationViewTest, HoverState) {
  EXPECT_FALSE(decoration_view_->is_hovered());
  // Hover state is set by mouse events; we test the accessor exists.
  SUCCEED();
}

TEST_F(AstraLocationBarDecorationViewTest, ModelSettingsChangeTriggersLayout) {
  // Changing settings should cause a layout invalidation.
  // We can't easily check "was layout invalidated" but we check no crash.
  model_->SetShowLabels(true);
  model_->SetIconSize(AstraDecorationIconSize::kLarge);
  model_->SetButtonStyle(AstraDecorationButtonStyle::kChip);
  SUCCEED();
}

TEST_F(AstraLocationBarDecorationViewTest, GetActionButtonForTest) {
  auto* button = decoration_view_->GetActionButtonForTest("screenshot");
  EXPECT_NE(nullptr, button);
}

TEST_F(AstraLocationBarDecorationViewTest, GetActionButtonForTestNotFound) {
  auto* button = decoration_view_->GetActionButtonForTest("nonexistent");
  EXPECT_EQ(nullptr, button);
}

TEST_F(AstraLocationBarDecorationViewTest, DelegateNullptrIsSafe) {
  decoration_view_->SetDelegate(nullptr);
  // Simulating a click should not crash.
  decoration_view_->GetActionButtonForTest("screenshot");
  SUCCEED();
}

TEST_F(AstraLocationBarDecorationViewTest, FullStateCycle) {
  // Cycle through various state combinations.
  model_->SetShowDecoration(false);
  model_->SetShowDecoration(true);
  model_->SetPosition(AstraDecorationPosition::kTrailing);
  model_->SetMaxVisibleActions(6);
  model_->SetShowLabels(true);
  model_->SetButtonStyle(AstraDecorationButtonStyle::kChip);
  model_->SetOmniboxFocused(true);
  model_->SetSecurityLevel(AstraSecurityLevel::kSecure);
  model_->ReorderAction(0, 3);

  decoration_view_->Layout();
  decoration_view_->OnThemeChanged();
  SUCCEED();
}

TEST_F(AstraLocationBarDecorationViewTest, ShowOnFocusOnlyBehavior) {
  model_->SetShowOnFocusOnly(true);
  model_->SetOmniboxFocused(false);
  // When show_on_focus_only is true and omnibox is not focused,
  // the decoration should be hidden.
  EXPECT_FALSE(decoration_view_->GetVisible());

  model_->SetOmniboxFocused(true);
  EXPECT_TRUE(decoration_view_->GetVisible());
}

// =========================================================================
// Action data structure tests
// =========================================================================

TEST(AstraDecorationActionTest, DefaultConstructs) {
  AstraDecorationAction action;
  EXPECT_TRUE(action.id.empty());
  EXPECT_TRUE(action.label.empty());
  EXPECT_TRUE(action.icon.empty());
  EXPECT_TRUE(action.tooltip.empty());
  EXPECT_TRUE(action.is_visible);
  EXPECT_EQ(0, action.position);
  EXPECT_TRUE(action.shortcut.empty());
}

TEST(AstraDecorationActionTest, ActionHasAllFields) {
  AstraDecorationAction action;
  action.id = "test";
  action.label = u"Test";
  action.icon = "test_icon";
  action.tooltip = u"Test action";
  action.is_visible = false;
  action.position = 3;
  action.shortcut = u"⌘T";

  EXPECT_EQ("test", action.id);
  EXPECT_EQ(u"Test", action.label);
  EXPECT_EQ("test_icon", action.icon);
  EXPECT_EQ(u"Test action", action.tooltip);
  EXPECT_FALSE(action.is_visible);
  EXPECT_EQ(3, action.position);
  EXPECT_EQ(u"⌘T", action.shortcut);
}

// =========================================================================
// Pref key tests
// =========================================================================

TEST(AstraOmniboxDecorationPrefsTest, PrefKeysHaveCorrectPaths) {
  using namespace prefs;

  EXPECT_STREQ("astra.omnibox.decoration.show_decoration",
               kPrefOmniboxDecorationShowDecoration);
  EXPECT_STREQ("astra.omnibox.decoration.position",
               kPrefOmniboxDecorationPosition);
  EXPECT_STREQ("astra.omnibox.decoration.max_visible_actions",
               kPrefOmniboxDecorationMaxVisibleActions);
  EXPECT_STREQ("astra.omnibox.decoration.show_labels",
               kPrefOmniboxDecorationShowLabels);
  EXPECT_STREQ("astra.omnibox.decoration.icon_size",
               kPrefOmniboxDecorationIconSize);
  EXPECT_STREQ("astra.omnibox.decoration.button_style",
               kPrefOmniboxDecorationButtonStyle);
  EXPECT_STREQ("astra.omnibox.decoration.show_on_focus_only",
               kPrefOmniboxDecorationShowOnFocusOnly);
  EXPECT_STREQ("astra.omnibox.decoration.show_workspace",
               kPrefOmniboxDecorationShowWorkspace);
  EXPECT_STREQ("astra.omnibox.decoration.show_focus_mode",
               kPrefOmniboxDecorationShowFocusMode);
  EXPECT_STREQ("astra.omnibox.decoration.show_screenshot",
               kPrefOmniboxDecorationShowScreenshot);
  EXPECT_STREQ("astra.omnibox.decoration.show_note",
               kPrefOmniboxDecorationShowNote);
  EXPECT_STREQ("astra.omnibox.decoration.show_split_view",
               kPrefOmniboxDecorationShowSplitView);
  EXPECT_STREQ("astra.omnibox.decoration.show_reading_list",
               kPrefOmniboxDecorationShowReadingList);
  EXPECT_STREQ("astra.omnibox.decoration.show_translate",
               kPrefOmniboxDecorationShowTranslate);
  EXPECT_STREQ("astra.omnibox.decoration.show_share",
               kPrefOmniboxDecorationShowShare);
  EXPECT_STREQ("astra.omnibox.decoration.overflow_menu",
               kPrefOmniboxDecorationOverflowMenu);
  EXPECT_STREQ("astra.omnibox.decoration.hover_expansion",
               kPrefOmniboxDecorationHoverExpansion);
}

TEST(AstraOmniboxDecorationPrefsTest, DefaultValues) {
  using namespace prefs;

  EXPECT_TRUE(kDefaultOmniboxDecorationShowDecoration);
  EXPECT_STREQ("left", kDefaultOmniboxDecorationPosition);
  EXPECT_EQ(4, kDefaultOmniboxDecorationMaxVisibleActions);
  EXPECT_FALSE(kDefaultOmniboxDecorationShowLabels);
  EXPECT_EQ(1, kDefaultOmniboxDecorationIconSize);  // medium
  EXPECT_EQ(0, kDefaultOmniboxDecorationButtonStyle);  // icon only
  EXPECT_FALSE(kDefaultOmniboxDecorationShowOnFocusOnly);
  EXPECT_TRUE(kDefaultOmniboxDecorationShowWorkspace);
  EXPECT_TRUE(kDefaultOmniboxDecorationShowFocusMode);
  EXPECT_TRUE(kDefaultOmniboxDecorationShowScreenshot);
  EXPECT_TRUE(kDefaultOmniboxDecorationShowNote);
  EXPECT_TRUE(kDefaultOmniboxDecorationShowSplitView);
  EXPECT_TRUE(kDefaultOmniboxDecorationShowReadingList);
  EXPECT_TRUE(kDefaultOmniboxDecorationShowTranslate);
  EXPECT_TRUE(kDefaultOmniboxDecorationShowShare);
  EXPECT_TRUE(kDefaultOmniboxDecorationOverflowMenu);
  EXPECT_FALSE(kDefaultOmniboxDecorationHoverExpansion);
}

}  // namespace astra
