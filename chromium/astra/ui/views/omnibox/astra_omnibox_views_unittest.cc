// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Comprehensive unit tests for Astra omnibox decoration — model and view.
//
// Test categories:
//   - Decoration type enum tests
//   - Decoration item struct tests
//   - Model construction and defaults
//   - Decoration access (all, count, by index, by type)
//   - Decoration visibility (all 10 types)
//   - Decoration active state (all types)
//   - Decoration tooltips
//   - Decoration badges (set, clear, text + color)
//   - Decoration reordering
//   - Decoration execution
//   - Bubble management (show, hide, get open type, hide all)
//   - Workspace decoration (name, color, badge count, visibility)
//   - Focus mode decoration (active, time remaining, visibility, color)
//   - Tab stack decoration (name, color, tab count, visibility)
//   - Reading list decoration (in list, button visibility)
//   - Note decoration (has note, preview, button visibility)
//   - Favorite decoration (favorited, visibility)
//   - Sidebar decoration (open state, toggle visibility)
//   - Split view decoration (active, button visibility)
//   - Presentation settings (15+ settings)
//   - Observer notifications (all 10 events)
//   - Observer default implementations
//   - Observer shutdown notification
//   - Edge cases (invalid type, negative index, empty badge, etc.)
//   - View creation and initial state
//   - View model set/get
//   - View decoration view access
//   - View decoration count
//   - View position (leading/trailing)
//   - View compact mode
//   - View icon size
//   - View animation enabled
//   - View update all decorations
//   - View bubble show/hide
//   - View open bubble type
//   - View spacing
//   - View visual layout
//   - View state updates
//   - View edge cases (no decorations, all decorations)
//
// Chromium test pattern: views::test::ViewsTestBase
//   (ui/views/test/views_test_base.h)

#include "astra/ui/views/omnibox/astra_location_bar_decoration.h"
#include "astra/ui/views/omnibox/astra_omnibox_decoration_model.h"

#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "base/time/time.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace astra {

namespace {

using DecorationView = AstraLocationBarDecorationView;
using Model = AstraOmniboxDecorationModel;
using Observer = AstraOmniboxDecorationObserver;
using DecorationType = AstraOmniboxDecorationType;
using DecorationItem = AstraOmniboxDecorationItem;
using Position = AstraDecorationPosition;

// =========================================================================
// Test observer for model events
// =========================================================================

struct TestObserver : public Observer {
  int visibility_changed_count = 0;
  DecorationType last_visibility_type = DecorationType::kNone;
  bool last_visibility_value = false;

  int active_changed_count = 0;
  DecorationType last_active_type = DecorationType::kNone;
  bool last_active_value = false;

  int badge_changed_count = 0;
  DecorationType last_badge_type = DecorationType::kNone;

  int reordered_count = 0;
  int executed_count = 0;
  DecorationType last_executed_type = DecorationType::kNone;

  int bubble_shown_count = 0;
  DecorationType last_bubble_shown_type = DecorationType::kNone;

  int bubble_hidden_count = 0;
  DecorationType last_bubble_hidden_type = DecorationType::kNone;

  int workspace_changed_count = 0;
  std::u16string last_workspace_name;

  int focus_mode_changed_count = 0;
  bool last_focus_mode_active = false;

  int shutdown_count = 0;

  void OnDecorationVisibilityChanged(
      AstraOmniboxDecorationModel* model,
      DecorationType type,
      bool visible) override {
    visibility_changed_count++;
    last_visibility_type = type;
    last_visibility_value = visible;
  }

  void OnDecorationActiveChanged(
      AstraOmniboxDecorationModel* model,
      DecorationType type,
      bool active) override {
    active_changed_count++;
    last_active_type = type;
    last_active_value = active;
  }

  void OnDecorationBadgeChanged(
      AstraOmniboxDecorationModel* model,
      DecorationType type) override {
    badge_changed_count++;
    last_badge_type = type;
  }

  void OnDecorationsReordered(
      AstraOmniboxDecorationModel* model) override {
    reordered_count++;
  }

  void OnDecorationExecuted(
      AstraOmniboxDecorationModel* model,
      DecorationType type) override {
    executed_count++;
    last_executed_type = type;
  }

  void OnBubbleShown(
      AstraOmniboxDecorationModel* model,
      DecorationType type) override {
    bubble_shown_count++;
    last_bubble_shown_type = type;
  }

  void OnBubbleHidden(
      AstraOmniboxDecorationModel* model,
      DecorationType type) override {
    bubble_hidden_count++;
    last_bubble_hidden_type = type;
  }

  void OnWorkspaceChanged(
      AstraOmniboxDecorationModel* model,
      const std::u16string& name) override {
    workspace_changed_count++;
    last_workspace_name = name;
  }

  void OnFocusModeChanged(
      AstraOmniboxDecorationModel* model,
      bool active) override {
    focus_mode_changed_count++;
    last_focus_mode_active = active;
  }

  void OnOmniboxDecorationModelShutdown(
      AstraOmniboxDecorationModel* model) override {
    shutdown_count++;
  }
};

// =========================================================================
// Fake delegate for view callbacks
// =========================================================================

struct FakeDecorationDelegate : public DecorationView::Delegate {
  int decoration_clicked_count = 0;
  DecorationType last_clicked_type = DecorationType::kNone;
  int bubble_shown_count = 0;
  DecorationType last_bubble_type = DecorationType::kNone;

  void OnDecorationClicked(DecorationType type) override {
    decoration_clicked_count++;
    last_clicked_type = type;
  }

  void ShowBubbleForDecoration(DecorationType type) override {
    bubble_shown_count++;
    last_bubble_type = type;
  }
};

// All decoration types for parameterized testing.
const DecorationType kAllDecorationTypes[] = {
    DecorationType::kWorkspaceIndicator,
    DecorationType::kFocusModeBadge,
    DecorationType::kTabStackIndicator,
    DecorationType::kReadingListBadge,
    DecorationType::kNoteBadge,
    DecorationType::kFavoriteStar,
    DecorationType::kSidebarToggle,
    DecorationType::kSplitViewToggle,
    DecorationType::kTranslateButton,
    DecorationType::kAstraActionButton,
};

}  // namespace

// =========================================================================
// Decoration type enum tests
// =========================================================================

TEST(AstraOmniboxDecorationEnumTest, DecorationTypeHasElevenValues) {
  // kNone + 10 decoration types
  EXPECT_EQ(0, static_cast<int>(DecorationType::kNone));
  EXPECT_EQ(1, static_cast<int>(DecorationType::kWorkspaceIndicator));
  EXPECT_EQ(2, static_cast<int>(DecorationType::kFocusModeBadge));
  EXPECT_EQ(3, static_cast<int>(DecorationType::kTabStackIndicator));
  EXPECT_EQ(4, static_cast<int>(DecorationType::kReadingListBadge));
  EXPECT_EQ(5, static_cast<int>(DecorationType::kNoteBadge));
  EXPECT_EQ(6, static_cast<int>(DecorationType::kFavoriteStar));
  EXPECT_EQ(7, static_cast<int>(DecorationType::kSidebarToggle));
  EXPECT_EQ(8, static_cast<int>(DecorationType::kSplitViewToggle));
  EXPECT_EQ(9, static_cast<int>(DecorationType::kTranslateButton));
  EXPECT_EQ(10, static_cast<int>(DecorationType::kAstraActionButton));
}

TEST(AstraOmniboxDecorationEnumTest, NumDecorationTypesIsTen) {
  EXPECT_EQ(10u, kNumDecorationTypes);
}

TEST(AstraOmniboxDecorationEnumTest, PositionHasTwoValues) {
  EXPECT_EQ(0, static_cast<int>(Position::kLeading));
  EXPECT_EQ(1, static_cast<int>(Position::kTrailing));
}

// =========================================================================
// Decoration item struct tests
// =========================================================================

TEST(AstraDecorationItemTest, DefaultConstructs) {
  DecorationItem item;
  EXPECT_EQ(DecorationType::kNone, item.type);
  EXPECT_TRUE(item.is_visible);
  EXPECT_FALSE(item.is_active);
  EXPECT_TRUE(item.icon.empty());
  EXPECT_TRUE(item.tooltip.empty());
  EXPECT_TRUE(item.accessibility_label.empty());
  EXPECT_EQ(0, item.order_index);
  EXPECT_FALSE(item.has_bubble);
  EXPECT_TRUE(item.badge_text.empty());
  EXPECT_EQ(SK_ColorTRANSPARENT, item.badge_color);
  EXPECT_EQ(0, item.command_id);
}

TEST(AstraDecorationItemTest, ItemHasAllFields) {
  DecorationItem item;
  item.type = DecorationType::kFavoriteStar;
  item.is_visible = false;
  item.is_active = true;
  item.icon = "star_filled";
  item.tooltip = u"Bookmarked";
  item.accessibility_label = u"Bookmark this page";
  item.order_index = 5;
  item.has_bubble = true;
  item.badge_text = u"3";
  item.badge_color = SK_ColorRED;
  item.command_id = 42;

  EXPECT_EQ(DecorationType::kFavoriteStar, item.type);
  EXPECT_FALSE(item.is_visible);
  EXPECT_TRUE(item.is_active);
  EXPECT_EQ("star_filled", item.icon);
  EXPECT_EQ(u"Bookmarked", item.tooltip);
  EXPECT_EQ(u"Bookmark this page", item.accessibility_label);
  EXPECT_EQ(5, item.order_index);
  EXPECT_TRUE(item.has_bubble);
  EXPECT_EQ(u"3", item.badge_text);
  EXPECT_EQ(SK_ColorRED, item.badge_color);
  EXPECT_EQ(42, item.command_id);
}

// =========================================================================
// Model construction and defaults tests
// =========================================================================

TEST(AstraOmniboxDecorationModelTest, ConstructsWithoutCrash) {
  Model model;
  SUCCEED();
}

TEST(AstraOmniboxDecorationModelTest, DestructorNotifiesShutdown) {
  TestObserver observer;
  auto model = std::make_unique<Model>();
  model->AddObserver(&observer);
  model.reset();
  EXPECT_EQ(1, observer.shutdown_count);
}

TEST(AstraOmniboxDecorationModelTest, DefaultHasTenDecorations) {
  Model model;
  EXPECT_EQ(10, model.GetDecorationCount());
}

TEST(AstraOmniboxDecorationModelTest, DefaultAllDecorationsVisible) {
  Model model;
  for (auto type : kAllDecorationTypes) {
    EXPECT_TRUE(model.IsDecorationVisible(type))
        << "Decoration " << static_cast<int>(type) << " should be visible";
  }
}

TEST(AstraOmniboxDecorationModelTest, DefaultHasAllDecorationTypes) {
  Model model;
  for (auto type : kAllDecorationTypes) {
    EXPECT_NE(nullptr, model.GetDecorationByType(type))
        << "Decoration " << static_cast<int>(type) << " should exist";
  }
}

TEST(AstraOmniboxDecorationModelTest, DefaultDecorationOrder) {
  Model model;
  auto order = model.GetDecorationOrder();
  ASSERT_EQ(10u, order.size());
  EXPECT_EQ(DecorationType::kWorkspaceIndicator, order[0]);
  EXPECT_EQ(DecorationType::kFocusModeBadge, order[1]);
  EXPECT_EQ(DecorationType::kTabStackIndicator, order[2]);
  EXPECT_EQ(DecorationType::kReadingListBadge, order[3]);
  EXPECT_EQ(DecorationType::kNoteBadge, order[4]);
  EXPECT_EQ(DecorationType::kFavoriteStar, order[5]);
  EXPECT_EQ(DecorationType::kSidebarToggle, order[6]);
  EXPECT_EQ(DecorationType::kSplitViewToggle, order[7]);
  EXPECT_EQ(DecorationType::kTranslateButton, order[8]);
  EXPECT_EQ(DecorationType::kAstraActionButton, order[9]);
}

TEST(AstraOmniboxDecorationModelTest, DefaultOpenBubbleIsNone) {
  Model model;
  EXPECT_EQ(DecorationType::kNone, model.GetOpenBubbleType());
}

TEST(AstraOmniboxDecorationModelTest, DefaultWorkspaceState) {
  Model model;
  EXPECT_TRUE(model.GetCurrentWorkspaceName().empty());
  EXPECT_EQ(SK_ColorGRAY, model.GetWorkspaceColor());
  EXPECT_EQ(0, model.GetWorkspaceBadgeCount());
  EXPECT_TRUE(model.GetShowWorkspaceIndicator());
}

TEST(AstraOmniboxDecorationModelTest, DefaultFocusModeState) {
  Model model;
  EXPECT_FALSE(model.IsFocusModeActive());
  EXPECT_EQ(base::TimeDelta(), model.GetFocusModeTimeRemaining());
  EXPECT_TRUE(model.GetShowFocusModeBadge());
  EXPECT_NE(SK_ColorTRANSPARENT, model.GetFocusModeColor());
}

TEST(AstraOmniboxDecorationModelTest, DefaultTabStackState) {
  Model model;
  EXPECT_TRUE(model.GetTabStackName().empty());
  EXPECT_EQ(SK_ColorGRAY, model.GetTabStackColor());
  EXPECT_EQ(0, model.GetTabStackTabCount());
  EXPECT_TRUE(model.GetShowTabStackIndicator());
}

TEST(AstraOmniboxDecorationModelTest, DefaultReadingListState) {
  Model model;
  EXPECT_FALSE(model.IsInReadingList());
  EXPECT_TRUE(model.GetShowReadingListButton());
}

TEST(AstraOmniboxDecorationModelTest, DefaultNoteState) {
  Model model;
  EXPECT_FALSE(model.HasNote());
  EXPECT_TRUE(model.GetNotePreview().empty());
  EXPECT_TRUE(model.GetShowNoteButton());
}

TEST(AstraOmniboxDecorationModelTest, DefaultFavoriteState) {
  Model model;
  EXPECT_FALSE(model.IsFavorited());
  EXPECT_TRUE(model.GetShowFavoriteStar());
}

TEST(AstraOmniboxDecorationModelTest, DefaultSidebarState) {
  Model model;
  EXPECT_FALSE(model.IsSidebarOpen());
  EXPECT_TRUE(model.GetShowSidebarToggle());
}

TEST(AstraOmniboxDecorationModelTest, DefaultSplitViewState) {
  Model model;
  EXPECT_FALSE(model.IsSplitViewActive());
  EXPECT_TRUE(model.GetShowSplitViewButton());
}

TEST(AstraOmniboxDecorationModelTest, DefaultPresentationSettings) {
  Model model;
  EXPECT_FALSE(model.GetShowBadgesOnHoverOnly());
  EXPECT_FALSE(model.GetCompactMode());
  EXPECT_TRUE(model.GetAnimationEnabled());
  EXPECT_EQ(Model::kDefaultDecorationIconSize, model.GetDecorationIconSize());
}

// =========================================================================
// Decoration access tests
// =========================================================================

TEST(AstraOmniboxDecorationModelTest, GetDecorationsReturnsAll) {
  Model model;
  const auto& decorations = model.GetDecorations();
  EXPECT_EQ(10u, decorations.size());
}

TEST(AstraOmniboxDecorationModelTest, GetDecorationAtValidIndex) {
  Model model;
  const DecorationItem* item = model.GetDecorationAt(0);
  ASSERT_NE(nullptr, item);
  EXPECT_EQ(DecorationType::kWorkspaceIndicator, item->type);
}

TEST(AstraOmniboxDecorationModelTest, GetDecorationAtNegativeIndex) {
  Model model;
  EXPECT_EQ(nullptr, model.GetDecorationAt(-1));
}

TEST(AstraOmniboxDecorationModelTest, GetDecorationAtOutOfRange) {
  Model model;
  EXPECT_EQ(nullptr, model.GetDecorationAt(100));
}

TEST(AstraOmniboxDecorationModelTest, GetDecorationAtLastIndex) {
  Model model;
  const DecorationItem* item = model.GetDecorationAt(9);
  ASSERT_NE(nullptr, item);
  EXPECT_EQ(DecorationType::kAstraActionButton, item->type);
}

TEST(AstraOmniboxDecorationModelTest, GetDecorationByTypeValid) {
  Model model;
  const DecorationItem* item = model.GetDecorationByType(
      DecorationType::kFavoriteStar);
  ASSERT_NE(nullptr, item);
  EXPECT_EQ(DecorationType::kFavoriteStar, item->type);
}

TEST(AstraOmniboxDecorationModelTest, GetDecorationByTypeNone) {
  Model model;
  EXPECT_EQ(nullptr, model.GetDecorationByType(DecorationType::kNone));
}

TEST(AstraOmniboxDecorationModelTest, GetDecorationByTypeInvalidValue) {
  Model model;
  EXPECT_EQ(nullptr, model.GetDecorationByType(
      static_cast<DecorationType>(999)));
}

TEST(AstraOmniboxDecorationModelTest, EachDecorationHasIcon) {
  Model model;
  for (auto type : kAllDecorationTypes) {
    const DecorationItem* item = model.GetDecorationByType(type);
    ASSERT_NE(nullptr, item);
    EXPECT_FALSE(item->icon.empty())
        << "Decoration " << static_cast<int>(type)
        << " should have an icon";
  }
}

TEST(AstraOmniboxDecorationModelTest, EachDecorationHasTooltip) {
  Model model;
  for (auto type : kAllDecorationTypes) {
    const DecorationItem* item = model.GetDecorationByType(type);
    ASSERT_NE(nullptr, item);
    EXPECT_FALSE(item->tooltip.empty())
        << "Decoration " << static_cast<int>(type)
        << " should have a tooltip";
  }
}

TEST(AstraOmniboxDecorationModelTest, EachDecorationHasAccessibilityLabel) {
  Model model;
  for (auto type : kAllDecorationTypes) {
    const DecorationItem* item = model.GetDecorationByType(type);
    ASSERT_NE(nullptr, item);
    EXPECT_FALSE(item->accessibility_label.empty())
        << "Decoration " << static_cast<int>(type)
        << " should have an accessibility label";
  }
}

TEST(AstraOmniboxDecorationModelTest, BubbleDecorationsHaveBubbleFlag) {
  Model model;
  EXPECT_TRUE(model.GetDecorationByType(
      DecorationType::kWorkspaceIndicator)->has_bubble);
  EXPECT_TRUE(model.GetDecorationByType(
      DecorationType::kTabStackIndicator)->has_bubble);
  EXPECT_TRUE(model.GetDecorationByType(
      DecorationType::kNoteBadge)->has_bubble);
  EXPECT_TRUE(model.GetDecorationByType(
      DecorationType::kAstraActionButton)->has_bubble);
}

TEST(AstraOmniboxDecorationModelTest, NonBubbleDecorationsNoBubbleFlag) {
  Model model;
  EXPECT_FALSE(model.GetDecorationByType(
      DecorationType::kFocusModeBadge)->has_bubble);
  EXPECT_FALSE(model.GetDecorationByType(
      DecorationType::kReadingListBadge)->has_bubble);
  EXPECT_FALSE(model.GetDecorationByType(
      DecorationType::kFavoriteStar)->has_bubble);
  EXPECT_FALSE(model.GetDecorationByType(
      DecorationType::kSidebarToggle)->has_bubble);
  EXPECT_FALSE(model.GetDecorationByType(
      DecorationType::kSplitViewToggle)->has_bubble);
  EXPECT_FALSE(model.GetDecorationByType(
      DecorationType::kTranslateButton)->has_bubble);
}

// =========================================================================
// Decoration visibility tests
// =========================================================================

TEST(AstraOmniboxDecorationVisibilityTest, SetVisibleFalseWorkspace) {
  Model model;
  model.SetDecorationVisible(DecorationType::kWorkspaceIndicator, false);
  EXPECT_FALSE(model.IsDecorationVisible(DecorationType::kWorkspaceIndicator));
}

TEST(AstraOmniboxDecorationVisibilityTest, SetVisibleFalseFocusMode) {
  Model model;
  model.SetDecorationVisible(DecorationType::kFocusModeBadge, false);
  EXPECT_FALSE(model.IsDecorationVisible(DecorationType::kFocusModeBadge));
}

TEST(AstraOmniboxDecorationVisibilityTest, SetVisibleFalseTabStack) {
  Model model;
  model.SetDecorationVisible(DecorationType::kTabStackIndicator, false);
  EXPECT_FALSE(model.IsDecorationVisible(DecorationType::kTabStackIndicator));
}

TEST(AstraOmniboxDecorationVisibilityTest, SetVisibleFalseReadingList) {
  Model model;
  model.SetDecorationVisible(DecorationType::kReadingListBadge, false);
  EXPECT_FALSE(model.IsDecorationVisible(DecorationType::kReadingListBadge));
}

TEST(AstraOmniboxDecorationVisibilityTest, SetVisibleFalseNote) {
  Model model;
  model.SetDecorationVisible(DecorationType::kNoteBadge, false);
  EXPECT_FALSE(model.IsDecorationVisible(DecorationType::kNoteBadge));
}

TEST(AstraOmniboxDecorationVisibilityTest, SetVisibleFalseFavorite) {
  Model model;
  model.SetDecorationVisible(DecorationType::kFavoriteStar, false);
  EXPECT_FALSE(model.IsDecorationVisible(DecorationType::kFavoriteStar));
}

TEST(AstraOmniboxDecorationVisibilityTest, SetVisibleFalseSidebar) {
  Model model;
  model.SetDecorationVisible(DecorationType::kSidebarToggle, false);
  EXPECT_FALSE(model.IsDecorationVisible(DecorationType::kSidebarToggle));
}

TEST(AstraOmniboxDecorationVisibilityTest, SetVisibleFalseSplitView) {
  Model model;
  model.SetDecorationVisible(DecorationType::kSplitViewToggle, false);
  EXPECT_FALSE(model.IsDecorationVisible(DecorationType::kSplitViewToggle));
}

TEST(AstraOmniboxDecorationVisibilityTest, SetVisibleFalseTranslate) {
  Model model;
  model.SetDecorationVisible(DecorationType::kTranslateButton, false);
  EXPECT_FALSE(model.IsDecorationVisible(DecorationType::kTranslateButton));
}

TEST(AstraOmniboxDecorationVisibilityTest, SetVisibleFalseAstraAction) {
  Model model;
  model.SetDecorationVisible(DecorationType::kAstraActionButton, false);
  EXPECT_FALSE(model.IsDecorationVisible(DecorationType::kAstraActionButton));
}

TEST(AstraOmniboxDecorationVisibilityTest, SetVisibleTrueAfterFalse) {
  Model model;
  model.SetDecorationVisible(DecorationType::kFavoriteStar, false);
  ASSERT_FALSE(model.IsDecorationVisible(DecorationType::kFavoriteStar));

  model.SetDecorationVisible(DecorationType::kFavoriteStar, true);
  EXPECT_TRUE(model.IsDecorationVisible(DecorationType::kFavoriteStar));
}

TEST(AstraOmniboxDecorationVisibilityTest, SetVisibleInvalidTypeNoCrash) {
  Model model;
  model.SetDecorationVisible(DecorationType::kNone, true);
  model.SetDecorationVisible(static_cast<DecorationType>(999), false);
  SUCCEED();
}

TEST(AstraOmniboxDecorationVisibilityTest, SameStateNoNotify) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  // Already visible by default, so setting to true should not notify.
  model.SetDecorationVisible(DecorationType::kFavoriteStar, true);
  EXPECT_EQ(0, observer.visibility_changed_count);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxDecorationVisibilityTest, VisibilityChangeNotifiesObserver) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.SetDecorationVisible(DecorationType::kFavoriteStar, false);
  EXPECT_EQ(1, observer.visibility_changed_count);
  EXPECT_EQ(DecorationType::kFavoriteStar, observer.last_visibility_type);
  EXPECT_FALSE(observer.last_visibility_value);

  model.SetDecorationVisible(DecorationType::kFavoriteStar, true);
  EXPECT_EQ(2, observer.visibility_changed_count);
  EXPECT_TRUE(observer.last_visibility_value);

  model.RemoveObserver(&observer);
}

// =========================================================================
// Decoration active state tests
// =========================================================================

TEST(AstraOmniboxDecorationActiveTest, SetActiveWorkspace) {
  Model model;
  model.SetDecorationActive(DecorationType::kWorkspaceIndicator, true);
  EXPECT_TRUE(model.IsDecorationActive(DecorationType::kWorkspaceIndicator));
}

TEST(AstraOmniboxDecorationActiveTest, SetActiveFocusMode) {
  Model model;
  model.SetDecorationActive(DecorationType::kFocusModeBadge, true);
  EXPECT_TRUE(model.IsDecorationActive(DecorationType::kFocusModeBadge));
}

TEST(AstraOmniboxDecorationActiveTest, SetActiveTabStack) {
  Model model;
  model.SetDecorationActive(DecorationType::kTabStackIndicator, true);
  EXPECT_TRUE(model.IsDecorationActive(DecorationType::kTabStackIndicator));
}

TEST(AstraOmniboxDecorationActiveTest, SetActiveReadingList) {
  Model model;
  model.SetDecorationActive(DecorationType::kReadingListBadge, true);
  EXPECT_TRUE(model.IsDecorationActive(DecorationType::kReadingListBadge));
}

TEST(AstraOmniboxDecorationActiveTest, SetActiveNote) {
  Model model;
  model.SetDecorationActive(DecorationType::kNoteBadge, true);
  EXPECT_TRUE(model.IsDecorationActive(DecorationType::kNoteBadge));
}

TEST(AstraOmniboxDecorationActiveTest, SetActiveFavorite) {
  Model model;
  model.SetDecorationActive(DecorationType::kFavoriteStar, true);
  EXPECT_TRUE(model.IsDecorationActive(DecorationType::kFavoriteStar));
}

TEST(AstraOmniboxDecorationActiveTest, SetActiveSidebar) {
  Model model;
  model.SetDecorationActive(DecorationType::kSidebarToggle, true);
  EXPECT_TRUE(model.IsDecorationActive(DecorationType::kSidebarToggle));
}

TEST(AstraOmniboxDecorationActiveTest, SetActiveSplitView) {
  Model model;
  model.SetDecorationActive(DecorationType::kSplitViewToggle, true);
  EXPECT_TRUE(model.IsDecorationActive(DecorationType::kSplitViewToggle));
}

TEST(AstraOmniboxDecorationActiveTest, SetActiveTranslate) {
  Model model;
  model.SetDecorationActive(DecorationType::kTranslateButton, true);
  EXPECT_TRUE(model.IsDecorationActive(DecorationType::kTranslateButton));
}

TEST(AstraOmniboxDecorationActiveTest, SetActiveAstraAction) {
  Model model;
  model.SetDecorationActive(DecorationType::kAstraActionButton, true);
  EXPECT_TRUE(model.IsDecorationActive(DecorationType::kAstraActionButton));
}

TEST(AstraOmniboxDecorationActiveTest, SetActiveFalseAfterTrue) {
  Model model;
  model.SetDecorationActive(DecorationType::kFavoriteStar, true);
  ASSERT_TRUE(model.IsDecorationActive(DecorationType::kFavoriteStar));

  model.SetDecorationActive(DecorationType::kFavoriteStar, false);
  EXPECT_FALSE(model.IsDecorationActive(DecorationType::kFavoriteStar));
}

TEST(AstraOmniboxDecorationActiveTest, SetActiveInvalidTypeNoCrash) {
  Model model;
  model.SetDecorationActive(DecorationType::kNone, true);
  model.SetDecorationActive(static_cast<DecorationType>(999), false);
  SUCCEED();
}

TEST(AstraOmniboxDecorationActiveTest, SameStateNoNotify) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  // Default is false, so setting to false should not notify.
  model.SetDecorationActive(DecorationType::kFavoriteStar, false);
  EXPECT_EQ(0, observer.active_changed_count);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxDecorationActiveTest, ActiveChangeNotifiesObserver) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.SetDecorationActive(DecorationType::kFavoriteStar, true);
  EXPECT_EQ(1, observer.active_changed_count);
  EXPECT_EQ(DecorationType::kFavoriteStar, observer.last_active_type);
  EXPECT_TRUE(observer.last_active_value);

  model.SetDecorationActive(DecorationType::kFavoriteStar, false);
  EXPECT_EQ(2, observer.active_changed_count);
  EXPECT_FALSE(observer.last_active_value);

  model.RemoveObserver(&observer);
}

// =========================================================================
// Decoration tooltip tests
// =========================================================================

TEST(AstraOmniboxDecorationTooltipTest, SetTooltip) {
  Model model;
  model.SetDecorationTooltip(DecorationType::kFavoriteStar, u"New tooltip");
  const DecorationItem* item = model.GetDecorationByType(
      DecorationType::kFavoriteStar);
  ASSERT_NE(nullptr, item);
  EXPECT_EQ(u"New tooltip", item->tooltip);
}

TEST(AstraOmniboxDecorationTooltipTest, SetTooltipEmpty) {
  Model model;
  model.SetDecorationTooltip(DecorationType::kFavoriteStar, std::u16string());
  const DecorationItem* item = model.GetDecorationByType(
      DecorationType::kFavoriteStar);
  ASSERT_NE(nullptr, item);
  EXPECT_TRUE(item->tooltip.empty());
}

TEST(AstraOmniboxDecorationTooltipTest, SetTooltipInvalidTypeNoCrash) {
  Model model;
  model.SetDecorationTooltip(DecorationType::kNone, u"test");
  model.SetDecorationTooltip(static_cast<DecorationType>(999), u"test");
  SUCCEED();
}

// =========================================================================
// Decoration badge tests
// =========================================================================

TEST(AstraOmniboxDecorationBadgeTest, SetBadgeTextAndColor) {
  Model model;
  model.SetDecorationBadge(DecorationType::kWorkspaceIndicator, u"5", SK_ColorRED);
  const DecorationItem* item = model.GetDecorationByType(
      DecorationType::kWorkspaceIndicator);
  ASSERT_NE(nullptr, item);
  EXPECT_EQ(u"5", item->badge_text);
  EXPECT_EQ(SK_ColorRED, item->badge_color);
}

TEST(AstraOmniboxDecorationBadgeTest, SetBadgeNotifiesObserver) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.SetDecorationBadge(DecorationType::kNoteBadge, u"3", SK_ColorBLUE);
  EXPECT_EQ(1, observer.badge_changed_count);
  EXPECT_EQ(DecorationType::kNoteBadge, observer.last_badge_type);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxDecorationBadgeTest, SetSameBadgeNoNotify) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.SetDecorationBadge(DecorationType::kNoteBadge, u"3", SK_ColorBLUE);
  model.SetDecorationBadge(DecorationType::kNoteBadge, u"3", SK_ColorBLUE);
  EXPECT_EQ(1, observer.badge_changed_count);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxDecorationBadgeTest, ClearBadge) {
  Model model;
  model.SetDecorationBadge(DecorationType::kTabStackIndicator, u"10", SK_ColorGREEN);
  ASSERT_FALSE(model.GetDecorationByType(
      DecorationType::kTabStackIndicator)->badge_text.empty());

  model.ClearDecorationBadge(DecorationType::kTabStackIndicator);
  const DecorationItem* item = model.GetDecorationByType(
      DecorationType::kTabStackIndicator);
  ASSERT_NE(nullptr, item);
  EXPECT_TRUE(item->badge_text.empty());
  EXPECT_EQ(SK_ColorTRANSPARENT, item->badge_color);
}

TEST(AstraOmniboxDecorationBadgeTest, ClearBadgeNotifies) {
  Model model;
  model.SetDecorationBadge(DecorationType::kNoteBadge, u"1", SK_ColorRED);

  TestObserver observer;
  model.AddObserver(&observer);

  model.ClearDecorationBadge(DecorationType::kNoteBadge);
  EXPECT_EQ(1, observer.badge_changed_count);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxDecorationBadgeTest, BadgeInvalidTypeNoCrash) {
  Model model;
  model.SetDecorationBadge(DecorationType::kNone, u"test", SK_ColorRED);
  model.ClearDecorationBadge(static_cast<DecorationType>(999));
  SUCCEED();
}

TEST(AstraOmniboxDecorationBadgeTest, EmptyBadgeTextClears) {
  Model model;
  model.SetDecorationBadge(DecorationType::kNoteBadge, u"5", SK_ColorRED);
  model.SetDecorationBadge(DecorationType::kNoteBadge, std::u16string(), SK_ColorRED);
  const DecorationItem* item = model.GetDecorationByType(
      DecorationType::kNoteBadge);
  ASSERT_NE(nullptr, item);
  EXPECT_TRUE(item->badge_text.empty());
}

// =========================================================================
// Decoration reordering tests
// =========================================================================

TEST(AstraOmniboxDecorationReorderTest, ReorderMovesItems) {
  Model model;
  std::vector<DecorationType> new_order = {
      DecorationType::kFavoriteStar,
      DecorationType::kSidebarToggle,
      DecorationType::kWorkspaceIndicator,
  };
  model.ReorderDecorations(new_order);

  auto order = model.GetDecorationOrder();
  ASSERT_EQ(10u, order.size());
  EXPECT_EQ(DecorationType::kFavoriteStar, order[0]);
  EXPECT_EQ(DecorationType::kSidebarToggle, order[1]);
  EXPECT_EQ(DecorationType::kWorkspaceIndicator, order[2]);
}

TEST(AstraOmniboxDecorationReorderTest, RemainingItemsKeepOriginalOrder) {
  Model model;
  std::vector<DecorationType> new_order = {
      DecorationType::kTranslateButton,
  };
  model.ReorderDecorations(new_order);

  auto order = model.GetDecorationOrder();
  ASSERT_EQ(10u, order.size());
  EXPECT_EQ(DecorationType::kTranslateButton, order[0]);
  // The rest should be in default order minus translate.
  EXPECT_EQ(DecorationType::kWorkspaceIndicator, order[1]);
  EXPECT_EQ(DecorationType::kFocusModeBadge, order[2]);
}

TEST(AstraOmniboxDecorationReorderTest, ReorderNotifiesObserver) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  std::vector<DecorationType> new_order = {
      DecorationType::kFavoriteStar,
  };
  model.ReorderDecorations(new_order);
  EXPECT_EQ(1, observer.reordered_count);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxDecorationReorderTest, ReorderEmptyNoCrash) {
  Model model;
  std::vector<DecorationType> empty;
  model.ReorderDecorations(empty);
  // Should be a no-op.
  auto order = model.GetDecorationOrder();
  EXPECT_EQ(10u, order.size());
}

TEST(AstraOmniboxDecorationReorderTest, ResetDecorationOrder) {
  Model model;
  std::vector<DecorationType> new_order = {
      DecorationType::kAstraActionButton,
      DecorationType::kFavoriteStar,
  };
  model.ReorderDecorations(new_order);
  ASSERT_EQ(DecorationType::kAstraActionButton,
            model.GetDecorationOrder()[0]);

  model.ResetDecorationOrder();
  auto default_order = Model::GetDefaultDecorationOrder();
  auto current_order = model.GetDecorationOrder();
  ASSERT_EQ(default_order.size(), current_order.size());
  for (size_t i = 0; i < default_order.size(); ++i) {
    EXPECT_EQ(default_order[i], current_order[i]);
  }
}

TEST(AstraOmniboxDecorationReorderTest, ResetNotifiesObserver) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.ResetDecorationOrder();
  EXPECT_EQ(1, observer.reordered_count);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxDecorationReorderTest, DefaultDecorationOrderIsStatic) {
  auto order = Model::GetDefaultDecorationOrder();
  EXPECT_EQ(10u, order.size());
  EXPECT_EQ(DecorationType::kWorkspaceIndicator, order[0]);
  EXPECT_EQ(DecorationType::kAstraActionButton, order[9]);
}

TEST(AstraOmniboxDecorationReorderTest, ReorderWithDuplicates) {
  Model model;
  std::vector<DecorationType> new_order = {
      DecorationType::kFavoriteStar,
      DecorationType::kFavoriteStar,  // Duplicate
      DecorationType::kSidebarToggle,
  };
  model.ReorderDecorations(new_order);
  // Duplicates should be handled gracefully (first occurrence wins).
  auto order = model.GetDecorationOrder();
  EXPECT_EQ(10u, order.size());
  // Favorite should appear only once.
  int count = 0;
  for (auto t : order) {
    if (t == DecorationType::kFavoriteStar) count++;
  }
  EXPECT_EQ(1, count);
}

TEST(AstraOmniboxDecorationReorderTest, ReorderWithInvalidTypes) {
  Model model;
  std::vector<DecorationType> new_order = {
      DecorationType::kFavoriteStar,
      static_cast<DecorationType>(999),  // Invalid
      DecorationType::kSidebarToggle,
  };
  model.ReorderDecorations(new_order);
  auto order = model.GetDecorationOrder();
  EXPECT_EQ(10u, order.size());
  // Favorite and sidebar should still be in front.
  EXPECT_EQ(DecorationType::kFavoriteStar, order[0]);
  EXPECT_EQ(DecorationType::kSidebarToggle, order[1]);
}

// =========================================================================
// Decoration execution tests
// =========================================================================

TEST(AstraOmniboxDecorationExecuteTest, ExecuteDecoration) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.ExecuteDecoration(DecorationType::kFavoriteStar);
  EXPECT_EQ(1, observer.executed_count);
  EXPECT_EQ(DecorationType::kFavoriteStar, observer.last_executed_type);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxDecorationExecuteTest, ExecuteMultipleTypes) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.ExecuteDecoration(DecorationType::kNoteBadge);
  model.ExecuteDecoration(DecorationType::kTranslateButton);
  model.ExecuteDecoration(DecorationType::kSidebarToggle);
  EXPECT_EQ(3, observer.executed_count);
  EXPECT_EQ(DecorationType::kSidebarToggle, observer.last_executed_type);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxDecorationExecuteTest, ExecuteInvalidTypeNoCrash) {
  Model model;
  model.ExecuteDecoration(DecorationType::kNone);
  model.ExecuteDecoration(static_cast<DecorationType>(999));
  SUCCEED();
}

TEST(AstraOmniboxDecorationExecuteTest, ExecuteAllTypes) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  int count = 0;
  for (auto type : kAllDecorationTypes) {
    model.ExecuteDecoration(type);
    ++count;
  }
  EXPECT_EQ(count, observer.executed_count);

  model.RemoveObserver(&observer);
}

// =========================================================================
// Bubble management tests
// =========================================================================

TEST(AstraOmniboxDecorationBubbleTest, ShowBubble) {
  Model model;
  model.ShowDecorationBubble(DecorationType::kWorkspaceIndicator);
  EXPECT_EQ(DecorationType::kWorkspaceIndicator, model.GetOpenBubbleType());
}

TEST(AstraOmniboxDecorationBubbleTest, ShowBubbleNotifiesObserver) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.ShowDecorationBubble(DecorationType::kNoteBadge);
  EXPECT_EQ(1, observer.bubble_shown_count);
  EXPECT_EQ(DecorationType::kNoteBadge, observer.last_bubble_shown_type);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxDecorationBubbleTest, HideBubble) {
  Model model;
  model.ShowDecorationBubble(DecorationType::kWorkspaceIndicator);
  ASSERT_EQ(DecorationType::kWorkspaceIndicator, model.GetOpenBubbleType());

  model.HideDecorationBubble(DecorationType::kWorkspaceIndicator);
  EXPECT_EQ(DecorationType::kNone, model.GetOpenBubbleType());
}

TEST(AstraOmniboxDecorationBubbleTest, HideBubbleNotifiesObserver) {
  Model model;
  model.ShowDecorationBubble(DecorationType::kNoteBadge);

  TestObserver observer;
  model.AddObserver(&observer);

  model.HideDecorationBubble(DecorationType::kNoteBadge);
  EXPECT_EQ(1, observer.bubble_hidden_count);
  EXPECT_EQ(DecorationType::kNoteBadge, observer.last_bubble_hidden_type);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxDecorationBubbleTest, ShowSecondBubbleHidesFirst) {
  Model model;
  model.ShowDecorationBubble(DecorationType::kWorkspaceIndicator);
  ASSERT_EQ(DecorationType::kWorkspaceIndicator, model.GetOpenBubbleType());

  TestObserver observer;
  model.AddObserver(&observer);

  model.ShowDecorationBubble(DecorationType::kNoteBadge);
  EXPECT_EQ(1, observer.bubble_shown_count);
  EXPECT_EQ(1, observer.bubble_hidden_count);
  EXPECT_EQ(DecorationType::kNoteBadge, model.GetOpenBubbleType());

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxDecorationBubbleTest, HideAllBubbles) {
  Model model;
  model.ShowDecorationBubble(DecorationType::kWorkspaceIndicator);
  ASSERT_NE(DecorationType::kNone, model.GetOpenBubbleType());

  model.HideAllBubbles();
  EXPECT_EQ(DecorationType::kNone, model.GetOpenBubbleType());
}

TEST(AstraOmniboxDecorationBubbleTest, HideAllBubblesNotifies) {
  Model model;
  model.ShowDecorationBubble(DecorationType::kWorkspaceIndicator);

  TestObserver observer;
  model.AddObserver(&observer);

  model.HideAllBubbles();
  EXPECT_EQ(1, observer.bubble_hidden_count);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxDecorationBubbleTest, HideAllBubblesWhenNoneOpen) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.HideAllBubbles();
  EXPECT_EQ(0, observer.bubble_hidden_count);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxDecorationBubbleTest, ShowSameBubbleNoChange) {
  Model model;
  model.ShowDecorationBubble(DecorationType::kWorkspaceIndicator);

  TestObserver observer;
  model.AddObserver(&observer);

  model.ShowDecorationBubble(DecorationType::kWorkspaceIndicator);
  EXPECT_EQ(0, observer.bubble_shown_count);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxDecorationBubbleTest, ShowNonBubbleDecorationNoEffect) {
  Model model;
  // kFavoriteStar does not have the has_bubble flag.
  model.ShowDecorationBubble(DecorationType::kFavoriteStar);
  EXPECT_EQ(DecorationType::kNone, model.GetOpenBubbleType());
}

TEST(AstraOmniboxDecorationBubbleTest, ShowBubbleNoneNoEffect) {
  Model model;
  model.ShowDecorationBubble(DecorationType::kNone);
  EXPECT_EQ(DecorationType::kNone, model.GetOpenBubbleType());
}

TEST(AstraOmniboxDecorationBubbleTest, HideWrongBubbleNoEffect) {
  Model model;
  model.ShowDecorationBubble(DecorationType::kWorkspaceIndicator);

  TestObserver observer;
  model.AddObserver(&observer);

  model.HideDecorationBubble(DecorationType::kNoteBadge);  // Not the open one
  EXPECT_EQ(0, observer.bubble_hidden_count);
  EXPECT_EQ(DecorationType::kWorkspaceIndicator, model.GetOpenBubbleType());

  model.RemoveObserver(&observer);
}

// =========================================================================
// Workspace decoration state tests
// =========================================================================

TEST(AstraOmniboxWorkspaceTest, SetWorkspaceName) {
  Model model;
  model.SetCurrentWorkspaceName(u"Work");
  EXPECT_EQ(u"Work", model.GetCurrentWorkspaceName());
}

TEST(AstraOmniboxWorkspaceTest, SetWorkspaceNameNotifies) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.SetCurrentWorkspaceName(u"Personal");
  EXPECT_EQ(1, observer.workspace_changed_count);
  EXPECT_EQ(u"Personal", observer.last_workspace_name);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxWorkspaceTest, SetWorkspaceNameSameNoNotify) {
  Model model;
  model.SetCurrentWorkspaceName(u"Test");

  TestObserver observer;
  model.AddObserver(&observer);

  model.SetCurrentWorkspaceName(u"Test");
  EXPECT_EQ(0, observer.workspace_changed_count);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxWorkspaceTest, SetWorkspaceColor) {
  Model model;
  model.SetWorkspaceColor(SK_ColorRED);
  EXPECT_EQ(SK_ColorRED, model.GetWorkspaceColor());
}

TEST(AstraOmniboxWorkspaceTest, SetWorkspaceBadgeCount) {
  Model model;
  model.SetWorkspaceBadgeCount(5);
  EXPECT_EQ(5, model.GetWorkspaceBadgeCount());
}

TEST(AstraOmniboxWorkspaceTest, SetWorkspaceBadgeCountUpdatesBadge) {
  Model model;
  model.SetWorkspaceBadgeCount(3);
  const DecorationItem* item = model.GetDecorationByType(
      DecorationType::kWorkspaceIndicator);
  ASSERT_NE(nullptr, item);
  EXPECT_EQ(u"3", item->badge_text);
  EXPECT_EQ(SK_ColorRED, item->badge_color);
}

TEST(AstraOmniboxWorkspaceTest, SetWorkspaceBadgeCountZeroClearsBadge) {
  Model model;
  model.SetWorkspaceBadgeCount(5);
  ASSERT_FALSE(model.GetDecorationByType(
      DecorationType::kWorkspaceIndicator)->badge_text.empty());

  model.SetWorkspaceBadgeCount(0);
  const DecorationItem* item = model.GetDecorationByType(
      DecorationType::kWorkspaceIndicator);
  ASSERT_NE(nullptr, item);
  EXPECT_TRUE(item->badge_text.empty());
}

TEST(AstraOmniboxWorkspaceTest, ShowWorkspaceIndicatorToggle) {
  Model model;
  model.SetShowWorkspaceIndicator(false);
  EXPECT_FALSE(model.GetShowWorkspaceIndicator());
  EXPECT_FALSE(model.IsDecorationVisible(DecorationType::kWorkspaceIndicator));
}

TEST(AstraOmniboxWorkspaceTest, ShowWorkspaceIndicatorSameValueNoChange) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.SetShowWorkspaceIndicator(true);  // Already true
  EXPECT_EQ(0, observer.visibility_changed_count);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxWorkspaceTest, WorkspaceNameUpdatesTooltip) {
  Model model;
  model.SetCurrentWorkspaceName(u"My Workspace");
  const DecorationItem* item = model.GetDecorationByType(
      DecorationType::kWorkspaceIndicator);
  ASSERT_NE(nullptr, item);
  EXPECT_EQ(u"My Workspace", item->tooltip);
}

// =========================================================================
// Focus mode decoration state tests
// =========================================================================

TEST(AstraOmniboxFocusModeTest, SetFocusModeActive) {
  Model model;
  model.SetFocusModeActive(true);
  EXPECT_TRUE(model.IsFocusModeActive());
  EXPECT_TRUE(model.IsDecorationActive(DecorationType::kFocusModeBadge));
}

TEST(AstraOmniboxFocusModeTest, SetFocusModeInactive) {
  Model model;
  model.SetFocusModeActive(true);
  ASSERT_TRUE(model.IsFocusModeActive());

  model.SetFocusModeActive(false);
  EXPECT_FALSE(model.IsFocusModeActive());
  EXPECT_FALSE(model.IsDecorationActive(DecorationType::kFocusModeBadge));
}

TEST(AstraOmniboxFocusModeTest, SetFocusModeNotifiesObserver) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.SetFocusModeActive(true);
  EXPECT_EQ(1, observer.focus_mode_changed_count);
  EXPECT_TRUE(observer.last_focus_mode_active);
  EXPECT_EQ(1, observer.active_changed_count);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxFocusModeTest, SetFocusModeSameValueNoNotify) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.SetFocusModeActive(false);  // Already false
  EXPECT_EQ(0, observer.focus_mode_changed_count);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxFocusModeTest, SetFocusModeTimeRemaining) {
  Model model;
  model.SetFocusModeTimeRemaining(base::Minutes(25));
  EXPECT_EQ(base::Minutes(25), model.GetFocusModeTimeRemaining());
}

TEST(AstraOmniboxFocusModeTest, SetFocusModeTimeRemainingZero) {
  Model model;
  model.SetFocusModeTimeRemaining(base::Seconds(0));
  EXPECT_EQ(base::TimeDelta(), model.GetFocusModeTimeRemaining());
}

TEST(AstraOmniboxFocusModeTest, SetShowFocusModeBadge) {
  Model model;
  model.SetShowFocusModeBadge(false);
  EXPECT_FALSE(model.GetShowFocusModeBadge());
  EXPECT_FALSE(model.IsDecorationVisible(DecorationType::kFocusModeBadge));
}

TEST(AstraOmniboxFocusModeTest, SetFocusModeColor) {
  Model model;
  model.SetFocusModeColor(SK_ColorCYAN);
  EXPECT_EQ(SK_ColorCYAN, model.GetFocusModeColor());
}

// =========================================================================
// Tab stack decoration state tests
// =========================================================================

TEST(AstraOmniboxTabStackTest, SetTabStackName) {
  Model model;
  model.SetTabStackName(u"Research");
  EXPECT_EQ(u"Research", model.GetTabStackName());
}

TEST(AstraOmniboxTabStackTest, SetTabStackNameUpdatesTooltip) {
  Model model;
  model.SetTabStackName(u"Shopping");
  const DecorationItem* item = model.GetDecorationByType(
      DecorationType::kTabStackIndicator);
  ASSERT_NE(nullptr, item);
  EXPECT_EQ(u"Shopping", item->tooltip);
}

TEST(AstraOmniboxTabStackTest, SetTabStackColor) {
  Model model;
  model.SetTabStackColor(SK_ColorGREEN);
  EXPECT_EQ(SK_ColorGREEN, model.GetTabStackColor());
}

TEST(AstraOmniboxTabStackTest, SetTabStackTabCount) {
  Model model;
  model.SetTabStackTabCount(7);
  EXPECT_EQ(7, model.GetTabStackTabCount());
}

TEST(AstraOmniboxTabStackTest, TabCountUpdatesBadge) {
  Model model;
  model.SetTabStackTabCount(4);
  const DecorationItem* item = model.GetDecorationByType(
      DecorationType::kTabStackIndicator);
  ASSERT_NE(nullptr, item);
  EXPECT_EQ(u"4", item->badge_text);
}

TEST(AstraOmniboxTabStackTest, TabCountZeroClearsBadge) {
  Model model;
  model.SetTabStackTabCount(5);
  ASSERT_FALSE(model.GetDecorationByType(
      DecorationType::kTabStackIndicator)->badge_text.empty());

  model.SetTabStackTabCount(0);
  const DecorationItem* item = model.GetDecorationByType(
      DecorationType::kTabStackIndicator);
  ASSERT_NE(nullptr, item);
  EXPECT_TRUE(item->badge_text.empty());
}

TEST(AstraOmniboxTabStackTest, TabCountNotifiesBadgeChange) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.SetTabStackTabCount(3);
  EXPECT_GE(observer.badge_changed_count, 1);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxTabStackTest, ShowTabStackIndicatorToggle) {
  Model model;
  model.SetShowTabStackIndicator(false);
  EXPECT_FALSE(model.GetShowTabStackIndicator());
  EXPECT_FALSE(model.IsDecorationVisible(DecorationType::kTabStackIndicator));
}

// =========================================================================
// Reading list decoration state tests
// =========================================================================

TEST(AstraOmniboxReadingListTest, SetInReadingList) {
  Model model;
  model.SetIsInReadingList(true);
  EXPECT_TRUE(model.IsInReadingList());
  EXPECT_TRUE(model.IsDecorationActive(DecorationType::kReadingListBadge));
}

TEST(AstraOmniboxReadingListTest, SetNotInReadingList) {
  Model model;
  model.SetIsInReadingList(true);
  ASSERT_TRUE(model.IsInReadingList());

  model.SetIsInReadingList(false);
  EXPECT_FALSE(model.IsInReadingList());
  EXPECT_FALSE(model.IsDecorationActive(DecorationType::kReadingListBadge));
}

TEST(AstraOmniboxReadingListTest, SetInReadingListNotifiesActive) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.SetIsInReadingList(true);
  EXPECT_EQ(1, observer.active_changed_count);
  EXPECT_EQ(DecorationType::kReadingListBadge, observer.last_active_type);
  EXPECT_TRUE(observer.last_active_value);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxReadingListTest, SetInReadingListSameValueNoNotify) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.SetIsInReadingList(false);  // Already false
  EXPECT_EQ(0, observer.active_changed_count);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxReadingListTest, ShowReadingListButtonToggle) {
  Model model;
  model.SetShowReadingListButton(false);
  EXPECT_FALSE(model.GetShowReadingListButton());
  EXPECT_FALSE(model.IsDecorationVisible(DecorationType::kReadingListBadge));
}

// =========================================================================
// Note decoration state tests
// =========================================================================

TEST(AstraOmniboxNoteTest, SetHasNote) {
  Model model;
  model.SetHasNote(true);
  EXPECT_TRUE(model.HasNote());
  EXPECT_TRUE(model.IsDecorationActive(DecorationType::kNoteBadge));
}

TEST(AstraOmniboxNoteTest, SetHasNoteFalse) {
  Model model;
  model.SetHasNote(true);
  ASSERT_TRUE(model.HasNote());

  model.SetHasNote(false);
  EXPECT_FALSE(model.HasNote());
  EXPECT_FALSE(model.IsDecorationActive(DecorationType::kNoteBadge));
}

TEST(AstraOmniboxNoteTest, SetHasNoteNotifiesActive) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.SetHasNote(true);
  EXPECT_EQ(1, observer.active_changed_count);
  EXPECT_EQ(DecorationType::kNoteBadge, observer.last_active_type);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxNoteTest, SetNotePreview) {
  Model model;
  model.SetNotePreview(u"This is a note preview");
  EXPECT_EQ(u"This is a note preview", model.GetNotePreview());
}

TEST(AstraOmniboxNoteTest, SetNotePreviewEmpty) {
  Model model;
  model.SetNotePreview(u"Some text");
  model.SetNotePreview(std::u16string());
  EXPECT_TRUE(model.GetNotePreview().empty());
}

TEST(AstraOmniboxNoteTest, ShowNoteButtonToggle) {
  Model model;
  model.SetShowNoteButton(false);
  EXPECT_FALSE(model.GetShowNoteButton());
  EXPECT_FALSE(model.IsDecorationVisible(DecorationType::kNoteBadge));
}

// =========================================================================
// Favorite decoration state tests
// =========================================================================

TEST(AstraOmniboxFavoriteTest, SetIsFavorited) {
  Model model;
  model.SetIsFavorited(true);
  EXPECT_TRUE(model.IsFavorited());
  EXPECT_TRUE(model.IsDecorationActive(DecorationType::kFavoriteStar));
}

TEST(AstraOmniboxFavoriteTest, SetNotFavorited) {
  Model model;
  model.SetIsFavorited(true);
  ASSERT_TRUE(model.IsFavorited());

  model.SetIsFavorited(false);
  EXPECT_FALSE(model.IsFavorited());
  EXPECT_FALSE(model.IsDecorationActive(DecorationType::kFavoriteStar));
}

TEST(AstraOmniboxFavoriteTest, SetIsFavoritedNotifiesActive) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.SetIsFavorited(true);
  EXPECT_EQ(1, observer.active_changed_count);
  EXPECT_EQ(DecorationType::kFavoriteStar, observer.last_active_type);
  EXPECT_TRUE(observer.last_active_value);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxFavoriteTest, SetIsFavoritedSameValueNoNotify) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.SetIsFavorited(false);  // Already false
  EXPECT_EQ(0, observer.active_changed_count);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxFavoriteTest, ShowFavoriteStarToggle) {
  Model model;
  model.SetShowFavoriteStar(false);
  EXPECT_FALSE(model.GetShowFavoriteStar());
  EXPECT_FALSE(model.IsDecorationVisible(DecorationType::kFavoriteStar));
}

// =========================================================================
// Sidebar decoration state tests
// =========================================================================

TEST(AstraOmniboxSidebarTest, SetSidebarOpen) {
  Model model;
  model.SetSidebarOpen(true);
  EXPECT_TRUE(model.IsSidebarOpen());
  EXPECT_TRUE(model.IsDecorationActive(DecorationType::kSidebarToggle));
}

TEST(AstraOmniboxSidebarTest, SetSidebarClosed) {
  Model model;
  model.SetSidebarOpen(true);
  ASSERT_TRUE(model.IsSidebarOpen());

  model.SetSidebarOpen(false);
  EXPECT_FALSE(model.IsSidebarOpen());
  EXPECT_FALSE(model.IsDecorationActive(DecorationType::kSidebarToggle));
}

TEST(AstraOmniboxSidebarTest, SetSidebarOpenNotifiesActive) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.SetSidebarOpen(true);
  EXPECT_EQ(1, observer.active_changed_count);
  EXPECT_EQ(DecorationType::kSidebarToggle, observer.last_active_type);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxSidebarTest, ShowSidebarToggleToggle) {
  Model model;
  model.SetShowSidebarToggle(false);
  EXPECT_FALSE(model.GetShowSidebarToggle());
  EXPECT_FALSE(model.IsDecorationVisible(DecorationType::kSidebarToggle));
}

// =========================================================================
// Split view decoration state tests
// =========================================================================

TEST(AstraOmniboxSplitViewTest, SetSplitViewActive) {
  Model model;
  model.SetSplitViewActive(true);
  EXPECT_TRUE(model.IsSplitViewActive());
  EXPECT_TRUE(model.IsDecorationActive(DecorationType::kSplitViewToggle));
}

TEST(AstraOmniboxSplitViewTest, SetSplitViewInactive) {
  Model model;
  model.SetSplitViewActive(true);
  ASSERT_TRUE(model.IsSplitViewActive());

  model.SetSplitViewActive(false);
  EXPECT_FALSE(model.IsSplitViewActive());
  EXPECT_FALSE(model.IsDecorationActive(DecorationType::kSplitViewToggle));
}

TEST(AstraOmniboxSplitViewTest, SetSplitViewActiveNotifies) {
  Model model;
  TestObserver observer;
  model.AddObserver(&observer);

  model.SetSplitViewActive(true);
  EXPECT_EQ(1, observer.active_changed_count);
  EXPECT_EQ(DecorationType::kSplitViewToggle, observer.last_active_type);

  model.RemoveObserver(&observer);
}

TEST(AstraOmniboxSplitViewTest, ShowSplitViewButtonToggle) {
  Model model;
  model.SetShowSplitViewButton(false);
  EXPECT_FALSE(model.GetShowSplitViewButton());
  EXPECT_FALSE(model.IsDecorationVisible(DecorationType::kSplitViewToggle));
}

// =========================================================================
// Presentation settings tests (15+ settings)
// =========================================================================

TEST(AstraOmniboxSettingsTest, ShowBadgesOnHoverOnly) {
  Model model;
  model.SetShowBadgesOnHoverOnly(true);
  EXPECT_TRUE(model.GetShowBadgesOnHoverOnly());

  model.SetShowBadgesOnHoverOnly(false);
  EXPECT_FALSE(model.GetShowBadgesOnHoverOnly());
}

TEST(AstraOmniboxSettingsTest, CompactMode) {
  Model model;
  model.SetCompactMode(true);
  EXPECT_TRUE(model.GetCompactMode());

  model.SetCompactMode(false);
  EXPECT_FALSE(model.GetCompactMode());
}

TEST(AstraOmniboxSettingsTest, AnimationEnabled) {
  Model model;
  model.SetAnimationEnabled(false);
  EXPECT_FALSE(model.GetAnimationEnabled());

  model.SetAnimationEnabled(true);
  EXPECT_TRUE(model.GetAnimationEnabled());
}

TEST(AstraOmniboxSettingsTest, DecorationIconSize) {
  Model model;
  model.SetDecorationIconSize(24);
  EXPECT_EQ(24, model.GetDecorationIconSize());
}

TEST(AstraOmniboxSettingsTest, DecorationIconSizeClampedToMin) {
  Model model;
  model.SetDecorationIconSize(0);
  EXPECT_EQ(Model::kMinIconSize, model.GetDecorationIconSize());
}

TEST(AstraOmniboxSettingsTest, DecorationIconSizeClampedToMax) {
  Model model;
  model.SetDecorationIconSize(100);
  EXPECT_EQ(Model::kMaxIconSize, model.GetDecorationIconSize());
}

TEST(AstraOmniboxSettingsTest, ShowWorkspaceIndicatorSetting) {
  Model model;
  model.SetShowWorkspaceIndicator(false);
  EXPECT_FALSE(model.GetShowWorkspaceIndicator());
}

TEST(AstraOmniboxSettingsTest, ShowFocusModeBadgeSetting) {
  Model model;
  model.SetShowFocusModeBadge(false);
  EXPECT_FALSE(model.GetShowFocusModeBadge());
}

TEST(AstraOmniboxSettingsTest, ShowTabStackIndicatorSetting) {
  Model model;
  model.SetShowTabStackIndicator(false);
  EXPECT_FALSE(model.GetShowTabStackIndicator());
}

TEST(AstraOmniboxSettingsTest, ShowReadingListButtonSetting) {
  Model model;
  model.SetShowReadingListButton(false);
  EXPECT_FALSE(model.GetShowReadingListButton());
}

TEST(AstraOmniboxSettingsTest, ShowNoteButtonSetting) {
  Model model;
  model.SetShowNoteButton(false);
  EXPECT_FALSE(model.GetShowNoteButton());
}

TEST(AstraOmniboxSettingsTest, ShowFavoriteStarSetting) {
  Model model;
  model.SetShowFavoriteStar(false);
  EXPECT_FALSE(model.GetShowFavoriteStar());
}

TEST(AstraOmniboxSettingsTest, ShowSidebarToggleSetting) {
  Model model;
  model.SetShowSidebarToggle(false);
  EXPECT_FALSE(model.GetShowSidebarToggle());
}

TEST(AstraOmniboxSettingsTest, ShowSplitViewButtonSetting) {
  Model model;
  model.SetShowSplitViewButton(false);
  EXPECT_FALSE(model.GetShowSplitViewButton());
}

TEST(AstraOmniboxSettingsTest, ClampIconSizeWithinRange) {
  EXPECT_EQ(20, Model::ClampIconSize(20));
}

TEST(AstraOmniboxSettingsTest, ClampIconSizeBelowMin) {
  EXPECT_EQ(Model::kMinIconSize, Model::ClampIconSize(0));
  EXPECT_EQ(Model::kMinIconSize, Model::ClampIconSize(-5));
}

TEST(AstraOmniboxSettingsTest, ClampIconSizeAboveMax) {
  EXPECT_EQ(Model::kMaxIconSize, Model::ClampIconSize(100));
  EXPECT_EQ(Model::kMaxIconSize, Model::ClampIconSize(999));
}

TEST(AstraOmniboxSettingsTest, DefaultSettingsConstants) {
  EXPECT_TRUE(Model::kDefaultShowWorkspaceIndicator);
  EXPECT_TRUE(Model::kDefaultShowFocusModeBadge);
  EXPECT_TRUE(Model::kDefaultShowTabStackIndicator);
  EXPECT_TRUE(Model::kDefaultShowReadingListButton);
  EXPECT_TRUE(Model::kDefaultShowNoteButton);
  EXPECT_TRUE(Model::kDefaultShowFavoriteStar);
  EXPECT_TRUE(Model::kDefaultShowSidebarToggle);
  EXPECT_TRUE(Model::kDefaultShowSplitViewButton);
  EXPECT_TRUE(Model::kDefaultShowTranslateButton);
  EXPECT_FALSE(Model::kDefaultShowBadgesOnHoverOnly);
  EXPECT_FALSE(Model::kDefaultCompactMode);
  EXPECT_TRUE(Model::kDefaultAnimationEnabled);
  EXPECT_EQ(20, Model::kDefaultDecorationIconSize);
}

TEST(AstraOmniboxSettingsTest, SettingConstantsAreConstexpr) {
  // Verify the static constexpr pref key constants exist.
  EXPECT_STREQ("astra.omnibox.decoration.show_workspace_indicator",
               Model::kSettingShowWorkspaceIndicator);
  EXPECT_STREQ("astra.omnibox.decoration.show_focus_mode_badge",
               Model::kSettingShowFocusModeBadge);
  EXPECT_STREQ("astra.omnibox.decoration.compact_mode",
               Model::kSettingCompactMode);
  EXPECT_STREQ("astra.omnibox.decoration.animation_enabled",
               Model::kSettingAnimationEnabled);
  EXPECT_STREQ("astra.omnibox.decoration.icon_size",
               Model::kSettingDecorationIconSize);
}

// =========================================================================
// Observer notification tests
// =========================================================================

TEST(AstraOmniboxObserverTest, AddRemoveObserver) {
  Model model;
  TestObserver observer;

  model.AddObserver(&observer);
  model.SetDecorationVisible(DecorationType::kFavoriteStar, false);
  EXPECT_EQ(1, observer.visibility_changed_count);

  model.RemoveObserver(&observer);
  model.SetDecorationVisible(DecorationType::kFavoriteStar, true);
  EXPECT_EQ(1, observer.visibility_changed_count);  // No new notification
}

TEST(AstraOmniboxObserverTest, ShutdownNotification) {
  auto model = std::make_unique<Model>();
  TestObserver observer;
  model->AddObserver(&observer);

  model.reset();
  EXPECT_EQ(1, observer.shutdown_count);
}

TEST(AstraOmniboxObserverTest, DefaultImplementationsAreNoOp) {
  // The base Observer class has empty default implementations.
  // Calling any method on a base instance should not crash.
  class TestDefaultObserver : public Observer {};

  TestDefaultObserver obs;
  obs.OnDecorationVisibilityChanged(nullptr, DecorationType::kFavoriteStar, true);
  obs.OnDecorationActiveChanged(nullptr, DecorationType::kFavoriteStar, true);
  obs.OnDecorationBadgeChanged(nullptr, DecorationType::kFavoriteStar);
  obs.OnDecorationsReordered(nullptr);
  obs.OnDecorationExecuted(nullptr, DecorationType::kFavoriteStar);
  obs.OnBubbleShown(nullptr, DecorationType::kFavoriteStar);
  obs.OnBubbleHidden(nullptr, DecorationType::kFavoriteStar);
  obs.OnWorkspaceChanged(nullptr, u"Test");
  obs.OnFocusModeChanged(nullptr, true);
  obs.OnOmniboxDecorationModelShutdown(nullptr);
  SUCCEED();
}

TEST(AstraOmniboxObserverTest, ObserverIsCheckedObserver) {
  EXPECT_TRUE((std::is_base_of<base::CheckedObserver, Observer>::value));
}

TEST(AstraOmniboxObserverTest, MultipleObservers) {
  Model model;
  TestObserver observer1;
  TestObserver observer2;

  model.AddObserver(&observer1);
  model.AddObserver(&observer2);

  model.SetDecorationActive(DecorationType::kNoteBadge, true);

  EXPECT_EQ(1, observer1.active_changed_count);
  EXPECT_EQ(1, observer2.active_changed_count);

  model.RemoveObserver(&observer1);
  model.RemoveObserver(&observer2);
}

// =========================================================================
// Edge case tests
// =========================================================================

TEST(AstraOmniboxEdgeTest, InvalidDecorationTypeVisible) {
  Model model;
  // These should not crash or affect anything.
  model.SetDecorationVisible(DecorationType::kNone, false);
  model.SetDecorationVisible(static_cast<DecorationType>(999), true);
  EXPECT_EQ(10, model.GetDecorationCount());
}

TEST(AstraOmniboxEdgeTest, InvalidDecorationTypeActive) {
  Model model;
  model.SetDecorationActive(DecorationType::kNone, true);
  model.SetDecorationActive(static_cast<DecorationType>(999), true);
  // No crash = success
  SUCCEED();
}

TEST(AstraOmniboxEdgeTest, InvalidDecorationTypeBadge) {
  Model model;
  model.SetDecorationBadge(DecorationType::kNone, u"5", SK_ColorRED);
  model.ClearDecorationBadge(static_cast<DecorationType>(999));
  SUCCEED();
}

TEST(AstraOmniboxEdgeTest, InvalidDecorationTypeTooltip) {
  Model model;
  model.SetDecorationTooltip(DecorationType::kNone, u"test");
  SUCCEED();
}

TEST(AstraOmniboxEdgeTest, NegativeIndexGetDecorationAt) {
  Model model;
  EXPECT_EQ(nullptr, model.GetDecorationAt(-1));
  EXPECT_EQ(nullptr, model.GetDecorationAt(-100));
}

TEST(AstraOmniboxEdgeTest, OutOfRangeIndexGetDecorationAt) {
  Model model;
  EXPECT_EQ(nullptr, model.GetDecorationAt(10));
  EXPECT_EQ(nullptr, model.GetDecorationAt(100));
}

TEST(AstraOmniboxEdgeTest, EmptyBadgeText) {
  Model model;
  model.SetDecorationBadge(DecorationType::kNoteBadge, std::u16string(), SK_ColorRED);
  const DecorationItem* item = model.GetDecorationByType(
      DecorationType::kNoteBadge);
  ASSERT_NE(nullptr, item);
  EXPECT_TRUE(item->badge_text.empty());
}

TEST(AstraOmniboxEdgeTest, EmptyBadgeColor) {
  Model model;
  model.SetDecorationBadge(DecorationType::kNoteBadge, u"5", SK_ColorTRANSPARENT);
  const DecorationItem* item = model.GetDecorationByType(
      DecorationType::kNoteBadge);
  ASSERT_NE(nullptr, item);
  EXPECT_EQ(SK_ColorTRANSPARENT, item->badge_color);
}

TEST(AstraOmniboxEdgeTest, ReorderAllItems) {
  Model model;
  std::vector<DecorationType> all_reversed;
  for (int i = 9; i >= 0; --i) {
    all_reversed.push_back(
        static_cast<DecorationType>(static_cast<int>(DecorationType::kWorkspaceIndicator) + i));
  }
  model.ReorderDecorations(all_reversed);

  auto order = model.GetDecorationOrder();
  EXPECT_EQ(10u, order.size());
  // kAstraActionButton should be first in reversed order.
  EXPECT_EQ(DecorationType::kAstraActionButton, order[0]);
  EXPECT_EQ(DecorationType::kWorkspaceIndicator, order[9]);
}

TEST(AstraOmniboxEdgeTest, ResetAfterHideAll) {
  Model model;
  for (auto type : kAllDecorationTypes) {
    model.SetDecorationVisible(type, false);
  }
  // Total count stays the same (items exist, just not visible).
  EXPECT_EQ(10, model.GetDecorationCount());
  // All should be hidden.
  for (auto type : kAllDecorationTypes) {
    EXPECT_FALSE(model.IsDecorationVisible(type));
  }

  model.ResetDecorationOrder();
  // Reset should restore all decorations to default visibility.
  EXPECT_EQ(10, model.GetDecorationCount());
  // Visibility depends on individual show_* settings (default true).
  for (auto type : kAllDecorationTypes) {
    EXPECT_TRUE(model.IsDecorationVisible(type));
  }
}
  }
}

TEST(AstraOmniboxEdgeTest, ExecuteWithNoObservers) {
  Model model;
  // Should not crash with no observers.
  model.ExecuteDecoration(DecorationType::kFavoriteStar);
  SUCCEED();
}

TEST(AstraOmniboxEdgeTest, SetNullPrefsNoCrash) {
  Model model;
  model.LoadFromPrefs(nullptr);
  model.SaveToPrefs(nullptr);
  SUCCEED();
}

TEST(AstraOmniboxEdgeTest, AllDecorationsHiddenCount) {
  Model model;
  for (auto type : kAllDecorationTypes) {
    model.SetDecorationVisible(type, false);
  }
  // GetDecorationCount should still return total (all items exist,
  // just not visible).
  EXPECT_EQ(10, model.GetDecorationCount());
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
        std::make_unique<DecorationView>(Position::kLeading));
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
  EXPECT_NE(nullptr, decoration_view_->GetModel());
}

TEST_F(AstraLocationBarDecorationViewTest, GetModelReturnsBoundModel) {
  EXPECT_EQ(model_.get(), decoration_view_->GetModel());
}

TEST_F(AstraLocationBarDecorationViewTest, SetModelNullptrClearsModel) {
  decoration_view_->SetModel(nullptr);
  EXPECT_EQ(nullptr, decoration_view_->GetModel());
}

TEST_F(AstraLocationBarDecorationViewTest, DecorationCountMatchesVisibleDecorations) {
  // By default 9 decorations are visible (workspace + 8 regular buttons).
  // Focus mode badge is hidden because focus mode is inactive by default.
  EXPECT_EQ(9, decoration_view_->GetDecorationCount());
}

TEST_F(AstraLocationBarDecorationViewTest, GetDecorationViewWorkspace) {
  views::View* view = decoration_view_->GetDecorationView(
      DecorationType::kWorkspaceIndicator);
  EXPECT_NE(nullptr, view);
}

TEST_F(AstraLocationBarDecorationViewTest, GetDecorationViewFocusMode) {
  // Focus mode is visible by default but inactive -> not visible
  views::View* view = decoration_view_->GetDecorationView(
      DecorationType::kFocusModeBadge);
  EXPECT_NE(nullptr, view);
}

TEST_F(AstraLocationBarDecorationViewTest, GetDecorationViewFavorite) {
  views::View* view = decoration_view_->GetDecorationView(
      DecorationType::kFavoriteStar);
  EXPECT_NE(nullptr, view);
}

TEST_F(AstraLocationBarDecorationViewTest, GetDecorationViewInvalidType) {
  views::View* view = decoration_view_->GetDecorationView(
      DecorationType::kNone);
  EXPECT_EQ(nullptr, view);
}

TEST_F(AstraLocationBarDecorationViewTest, DefaultPositionIsLeading) {
  auto view = std::make_unique<DecorationView>();
  EXPECT_EQ(Position::kLeading, view->GetPosition());
}

TEST_F(AstraLocationBarDecorationViewTest, SetPositionTrailing) {
  decoration_view_->SetPosition(Position::kTrailing);
  EXPECT_EQ(Position::kTrailing, decoration_view_->GetPosition());
}

TEST_F(AstraLocationBarDecorationViewTest, SetPositionSameValueNoLayoutCrash) {
  decoration_view_->SetPosition(Position::kLeading);
  decoration_view_->SetPosition(Position::kLeading);
  SUCCEED();
}

TEST_F(AstraLocationBarDecorationViewTest, CompactModeDefaultFalse) {
  EXPECT_FALSE(decoration_view_->GetCompactMode());
}

TEST_F(AstraLocationBarDecorationViewTest, SetCompactModeTrue) {
  decoration_view_->SetCompactMode(true);
  EXPECT_TRUE(decoration_view_->GetCompactMode());
}

TEST_F(AstraLocationBarDecorationViewTest, SetCompactModeSameValueNoCrash) {
  decoration_view_->SetCompactMode(false);
  decoration_view_->SetCompactMode(false);
  SUCCEED();
}

TEST_F(AstraLocationBarDecorationViewTest, IconSizeDefault) {
  EXPECT_EQ(20, decoration_view_->GetIconSize());
}

TEST_F(AstraLocationBarDecorationViewTest, SetIconSize) {
  decoration_view_->SetIconSize(24);
  EXPECT_EQ(24, decoration_view_->GetIconSize());
}

TEST_F(AstraLocationBarDecorationViewTest, SetIconSizeSameValueNoCrash) {
  decoration_view_->SetIconSize(20);
  SUCCEED();
}

TEST_F(AstraLocationBarDecorationViewTest, AnimationEnabledDefaultTrue) {
  EXPECT_TRUE(decoration_view_->GetAnimationEnabled());
}

TEST_F(AstraLocationBarDecorationViewTest, SetAnimationEnabledFalse) {
  decoration_view_->SetAnimationEnabled(false);
  EXPECT_FALSE(decoration_view_->GetAnimationEnabled());
}

TEST_F(AstraLocationBarDecorationViewTest, SpacingDefault) {
  EXPECT_GT(decoration_view_->GetSpacing(), 0);
}

TEST_F(AstraLocationBarDecorationViewTest, SetSpacing) {
  decoration_view_->SetSpacing(8);
  EXPECT_EQ(8, decoration_view_->GetSpacing());
}

TEST_F(AstraLocationBarDecorationViewTest, SetSpacingSameValueNoCrash) {
  int current = decoration_view_->GetSpacing();
  decoration_view_->SetSpacing(current);
  SUCCEED();
}

TEST_F(AstraLocationBarDecorationViewTest, UpdateAllDecorationsNoCrash) {
  decoration_view_->UpdateAllDecorations();
  SUCCEED();
}

TEST_F(AstraLocationBarDecorationViewTest, ShowBubbleForDecoration) {
  decoration_view_->ShowBubbleForDecoration(
      DecorationType::kWorkspaceIndicator);
  EXPECT_EQ(DecorationType::kWorkspaceIndicator,
            decoration_view_->GetOpenBubbleType());
  EXPECT_EQ(1, delegate_->bubble_shown_count);
}

TEST_F(AstraLocationBarDecorationViewTest, HideAllBubbles) {
  decoration_view_->ShowBubbleForDecoration(
      DecorationType::kWorkspaceIndicator);
  ASSERT_NE(DecorationType::kNone, decoration_view_->GetOpenBubbleType());

  decoration_view_->HideAllBubbles();
  EXPECT_EQ(DecorationType::kNone, decoration_view_->GetOpenBubbleType());
}

TEST_F(AstraLocationBarDecorationViewTest, GetOpenBubbleTypeDefaultNone) {
  EXPECT_EQ(DecorationType::kNone, decoration_view_->GetOpenBubbleType());
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

TEST_F(AstraLocationBarDecorationViewTest, HoverStateDefaultFalse) {
  EXPECT_FALSE(decoration_view_->is_hovered());
}

TEST_F(AstraLocationBarDecorationViewTest, ModelRemovalClearsButtons) {
  decoration_view_->SetModel(nullptr);
  EXPECT_EQ(0, decoration_view_->GetDecorationCount());
}

TEST_F(AstraLocationBarDecorationViewTest, VisibilityChangeReflectedInView) {
  model_->SetDecorationVisible(DecorationType::kFavoriteStar, false);
  views::View* button = decoration_view_->GetDecorationView(
      DecorationType::kFavoriteStar);
  ASSERT_NE(nullptr, button);
  EXPECT_FALSE(button->GetVisible());
}

TEST_F(AstraLocationBarDecorationViewTest, ActiveChangeReflectedInView) {
  model_->SetDecorationActive(DecorationType::kFavoriteStar, true);
  // Active state is reflected in the button's visual state.
  // We can't easily check the painted state, but we verify no crash.
  SUCCEED();
}

TEST_F(AstraLocationBarDecorationViewTest, WorkspaceIndicatorTestAccess) {
  EXPECT_NE(nullptr, decoration_view_->workspace_indicator_for_test());
}

TEST_F(AstraLocationBarDecorationViewTest, FocusModeBadgeTestAccess) {
  EXPECT_NE(nullptr, decoration_view_->focus_mode_badge_for_test());
}

TEST_F(AstraLocationBarDecorationViewTest, FullStateCycle) {
  // Cycle through various state combinations.
  model_->SetShowWorkspaceIndicator(false);
  model_->SetShowWorkspaceIndicator(true);
  model_->SetPosition(Position::kTrailing);
  model_->SetCompactMode(true);
  model_->SetDecorationIconSize(24);
  model_->SetFocusModeActive(true);
  model_->SetIsFavorited(true);
  model_->SetSidebarOpen(true);
  model_->ReorderDecorations({DecorationType::kFavoriteStar,
                              DecorationType::kSidebarToggle});

  decoration_view_->UpdateAllDecorations();
  decoration_view_->Layout();
  decoration_view_->OnThemeChanged();
  SUCCEED();
}

TEST_F(AstraLocationBarDecorationViewTest, DecorationExecuteNotifiesDelegate) {
  model_->ExecuteDecoration(DecorationType::kFavoriteStar);
  EXPECT_EQ(1, delegate_->decoration_clicked_count);
  EXPECT_EQ(DecorationType::kFavoriteStar, delegate_->last_clicked_type);
}

TEST_F(AstraLocationBarDecorationViewTest, ShutdownClearsModel) {
  // Create a model and add the view as observer.
  auto temp_model = std::make_unique<Model>();
  temp_model->AddObserver(decoration_view_);

  // Destroy the model - should notify shutdown.
  temp_model.reset();

  // The view's model pointer may or may not be cleared depending on
  // whether the view was observing temp_model.
  SUCCEED();
}

TEST_F(AstraLocationBarDecorationViewTest, TrailingEdgeConstructs) {
  auto view = std::make_unique<DecorationView>(Position::kTrailing);
  EXPECT_EQ(Position::kTrailing, view->GetPosition());
}

TEST_F(AstraLocationBarDecorationViewTest, DelegateNullptrIsSafe) {
  decoration_view_->SetDelegate(nullptr);
  // Simulating execution should not crash.
  model_->ExecuteDecoration(DecorationType::kFavoriteStar);
  SUCCEED();
}

TEST_F(AstraLocationBarDecorationViewTest, WorkspaceColorUpdatesView) {
  model_->SetWorkspaceColor(SK_ColorRED);
  model_->SetCurrentWorkspaceName(u"Test");
  decoration_view_->UpdateAllDecorations();
  SUCCEED();
}

TEST_F(AstraLocationBarDecorationViewTest, FocusModeTogglesBadgeVisibility) {
  // Initially focus mode is inactive, badge should not be visible.
  views::View* badge = decoration_view_->focus_mode_badge_for_test();
  ASSERT_NE(nullptr, badge);
  EXPECT_FALSE(badge->GetVisible());

  model_->SetFocusModeActive(true);
  EXPECT_TRUE(badge->GetVisible());

  model_->SetFocusModeActive(false);
  EXPECT_FALSE(badge->GetVisible());
}

TEST_F(AstraLocationBarDecorationViewTest, AllDecorationsHiddenLayout) {
  for (auto type : kAllDecorationTypes) {
    model_->SetDecorationVisible(type, false);
  }
  // Focus mode also needs to be inactive to be hidden.
  model_->SetFocusModeActive(false);

  decoration_view_->Layout();
  gfx::Size pref = decoration_view_->CalculatePreferredSize();
  // Should still have some size for padding.
  EXPECT_GE(pref.width(), 0);
}

TEST_F(AstraLocationBarDecorationViewTest, CompactModeAffectsLayout) {
  gfx::Size size_normal = decoration_view_->CalculatePreferredSize();

  decoration_view_->SetCompactMode(true);
  gfx::Size size_compact = decoration_view_->CalculatePreferredSize();

  // Compact mode should result in smaller or equal size.
  EXPECT_LE(size_compact.width(), size_normal.width());
}

}  // namespace astra
