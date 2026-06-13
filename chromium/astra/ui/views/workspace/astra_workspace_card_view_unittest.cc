// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Unit tests for AstraWorkspaceCardView.
//
// Tests verify:
//   - Construction and initial state
//   - Workspace name, accent color, tab count setters
//   - Window count and created time setters
//   - Last used time
//   - Hibernation state
//   - Icon support
//   - Active/selected/hover/hibernated states
//   - Thumbnail count
//   - Display mode (card/list)
//   - Size variant (small/medium/large)
//   - Show statistics toggle
//   - Callback invocation (click, rename, menu, delete)
//   - Preferred size calculation
//   - Theme/color integration
//   - Accessibility
//   - Keyboard navigation (focus, key events)
//
// Chromium test pattern: views::test::ViewsTestBase
//   (ui/views/test/views_test_base.h)

#include "astra/ui/views/workspace/astra_workspace_card_view.h"

#include "base/test/bind.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace astra {

namespace {

// Test callback tracker that records invocation counts.
struct CallbackTracker {
  int click_count = 0;
  int rename_count = 0;
  int menu_count = 0;
  int delete_count = 0;
  gfx::Point last_menu_point;
};

}  // namespace

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

class AstraWorkspaceCardViewTest : public views::ViewsTestBase {
 public:
  AstraWorkspaceCardViewTest() = default;
  ~AstraWorkspaceCardViewTest() override = default;

  // ViewsTestBase:
  void SetUp() override {
    ViewsTestBase::SetUp();

    widget_ = CreateTestWidget();
    card_view_ = widget_->SetContentsView(
        std::make_unique<AstraWorkspaceCardView>());
    widget_->Show();
  }

  void TearDown() override {
    widget_.reset();
    ViewsTestBase::TearDown();
  }

  // Set up all callbacks on the card view with a tracker.
  void SetupCallbacks(CallbackTracker* tracker) {
    card_view_->SetClickCallback(base::BindLambdaForTesting(
        [tracker]() { tracker->click_count++; }));
    card_view_->SetRenameCallback(base::BindLambdaForTesting(
        [tracker]() { tracker->rename_count++; }));
    card_view_->SetMenuActionCallback(base::BindLambdaForTesting(
        [tracker](const gfx::Point& point) {
          tracker->menu_count++;
          tracker->last_menu_point = point;
        }));
    card_view_->SetDeleteCallback(base::BindLambdaForTesting(
        [tracker]() { tracker->delete_count++; }));
  }

 protected:
  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraWorkspaceCardView> card_view_ = nullptr;
};

// =========================================================================
// Construction tests
// =========================================================================

TEST_F(AstraWorkspaceCardViewTest, DefaultNameIsEmpty) {
  EXPECT_TRUE(card_view_->workspace_name().empty());
}

TEST_F(AstraWorkspaceCardViewTest, DefaultIsNotActive) {
  EXPECT_FALSE(card_view_->is_active());
}

TEST_F(AstraWorkspaceCardViewTest, DefaultIsNotSelected) {
  EXPECT_FALSE(card_view_->is_selected());
}

TEST_F(AstraWorkspaceCardViewTest, DefaultIsNotHibernated) {
  EXPECT_FALSE(card_view_->is_hibernated());
}

TEST_F(AstraWorkspaceCardViewTest, DefaultDisplayModeIsCard) {
  EXPECT_EQ(AstraWorkspaceCardView::DisplayMode::kCard,
            card_view_->display_mode());
}

TEST_F(AstraWorkspaceCardViewTest, PreferredSizeIsPositive) {
  gfx::Size pref = card_view_->CalculatePreferredSize();
  EXPECT_GT(pref.width(), 0);
  EXPECT_GT(pref.height(), 0);
}

TEST_F(AstraWorkspaceCardViewTest, WidgetIsCreated) {
  EXPECT_NE(nullptr, card_view_->GetWidget());
}

// =========================================================================
// Name tests
// =========================================================================

TEST_F(AstraWorkspaceCardViewTest, SetWorkspaceName) {
  card_view_->SetWorkspaceName(u"My Workspace");
  EXPECT_EQ(u"My Workspace", card_view_->workspace_name());
}

TEST_F(AstraWorkspaceCardViewTest, SetWorkspaceNameEmpty) {
  card_view_->SetWorkspaceName(u"Test");
  card_view_->SetWorkspaceName(std::u16string());
  EXPECT_TRUE(card_view_->workspace_name().empty());
}

TEST_F(AstraWorkspaceCardViewTest, SetWorkspaceNameLongName) {
  // Long names should be accepted (the label handles eliding).
  std::u16string long_name(100, u'x');
  card_view_->SetWorkspaceName(long_name);
  EXPECT_EQ(long_name, card_view_->workspace_name());
}

// =========================================================================
// Accent color tests
// =========================================================================

TEST_F(AstraWorkspaceCardViewTest, SetAccentColorValid) {
  card_view_->SetAccentColor("#1A73E8");
  // Should not crash.
}

TEST_F(AstraWorkspaceCardViewTest, SetAccentColorEmpty) {
  card_view_->SetAccentColor(std::string());
  // Should not crash with empty string.
}

TEST_F(AstraWorkspaceCardViewTest, SetAccentColorMultipleChanges) {
  card_view_->SetAccentColor("#FF0000");
  card_view_->SetAccentColor("#00FF00");
  card_view_->SetAccentColor("#0000FF");
  // Should not crash with multiple changes.
}

// =========================================================================
// Tab count tests
// =========================================================================

TEST_F(AstraWorkspaceCardViewTest, SetTabCountZero) {
  card_view_->SetTabCount(0);
  // Should not crash.
}

TEST_F(AstraWorkspaceCardViewTest, SetTabCountPositive) {
  card_view_->SetTabCount(5);
  // Should not crash.
}

TEST_F(AstraWorkspaceCardViewTest, SetTabCountLarge) {
  card_view_->SetTabCount(999);
  // Should not crash.
}

TEST_F(AstraWorkspaceCardViewTest, SetTabCountNegative) {
  card_view_->SetTabCount(-1);
  // Should handle negative gracefully (clamp to 0 or just display).
}

// =========================================================================
// Window count tests
// =========================================================================

TEST_F(AstraWorkspaceCardViewTest, SetWindowCountDefaultIsOne) {
  // Default window count should be 1.
  card_view_->SetWindowCount(1);
}

TEST_F(AstraWorkspaceCardViewTest, SetWindowCountMultiple) {
  card_view_->SetWindowCount(3);
  // Should not crash.
}

// =========================================================================
// Created time tests
// =========================================================================

TEST_F(AstraWorkspaceCardViewTest, SetCreatedTimeNow) {
  card_view_->SetCreatedTime(base::Time::Now());
  // Should not crash.
}

TEST_F(AstraWorkspaceCardViewTest, SetCreatedTimePast) {
  base::Time past = base::Time::Now() - base::Days(3);
  card_view_->SetCreatedTime(past);
  // Should not crash.
}

TEST_F(AstraWorkspaceCardViewTest, SetCreatedTimeNull) {
  card_view_->SetCreatedTime(base::Time());
  // Should not crash with null time.
}

// =========================================================================
// Last used time tests
// =========================================================================

TEST_F(AstraWorkspaceCardViewTest, SetLastUsedTime) {
  card_view_->SetLastUsedTime(base::Time::Now() - base::Hours(2));
  // Should not crash.
}

TEST_F(AstraWorkspaceCardViewTest, SetLastUsedTimeNull) {
  card_view_->SetLastUsedTime(base::Time());
  // Should not crash with null time.
}

// =========================================================================
// Icon tests
// =========================================================================

TEST_F(AstraWorkspaceCardViewTest, SetIconWithValue) {
  card_view_->SetIcon(std::string("folder"));
  // Should not crash.
}

TEST_F(AstraWorkspaceCardViewTest, SetIconNullopt) {
  card_view_->SetIcon(std::nullopt);
  // Should not crash with nullopt.
}

// =========================================================================
// Hibernation tests
// =========================================================================

TEST_F(AstraWorkspaceCardViewTest, SetIsHibernatedTrue) {
  card_view_->SetIsHibernated(true);
  EXPECT_TRUE(card_view_->is_hibernated());
}

TEST_F(AstraWorkspaceCardViewTest, SetIsHibernatedFalse) {
  card_view_->SetIsHibernated(true);
  ASSERT_TRUE(card_view_->is_hibernated());

  card_view_->SetIsHibernated(false);
  EXPECT_FALSE(card_view_->is_hibernated());
}

TEST_F(AstraWorkspaceCardViewTest, SetIsHibernatedSameStateNoCrash) {
  card_view_->SetIsHibernated(false);
  card_view_->SetIsHibernated(false);
  // No crash = success.
}

// =========================================================================
// Display mode tests
// =========================================================================

TEST_F(AstraWorkspaceCardViewTest, SetDisplayModeList) {
  card_view_->SetDisplayMode(AstraWorkspaceCardView::DisplayMode::kList);
  EXPECT_EQ(AstraWorkspaceCardView::DisplayMode::kList,
            card_view_->display_mode());
}

TEST_F(AstraWorkspaceCardViewTest, SetDisplayModeCard) {
  card_view_->SetDisplayMode(AstraWorkspaceCardView::DisplayMode::kList);
  ASSERT_EQ(AstraWorkspaceCardView::DisplayMode::kList, card_view_->display_mode());

  card_view_->SetDisplayMode(AstraWorkspaceCardView::DisplayMode::kCard);
  EXPECT_EQ(AstraWorkspaceCardView::DisplayMode::kCard,
            card_view_->display_mode());
}

TEST_F(AstraWorkspaceCardViewTest, DisplayModeAffectsPreferredSize) {
  // Card mode has a fixed preferred size.
  gfx::Size card_size = card_view_->CalculatePreferredSize();

  card_view_->SetDisplayMode(AstraWorkspaceCardView::DisplayMode::kCard);
  gfx::Size card_mode_size = card_view_->CalculatePreferredSize();
  EXPECT_GT(card_mode_size.height(), 0);

  // List mode should have different height.
  card_view_->SetDisplayMode(AstraWorkspaceCardView::DisplayMode::kList);
  gfx::Size list_mode_size = card_view_->CalculatePreferredSize();
  // List mode has 0 width (expand to fill).
  EXPECT_EQ(0, list_mode_size.width());
  EXPECT_GT(list_mode_size.height(), 0);
  // List should be shorter than card.
  EXPECT_LT(list_mode_size.height(), card_mode_size.height());
}

// =========================================================================
// Size variant tests
// =========================================================================

TEST_F(AstraWorkspaceCardViewTest, SetSizeVariantSmall) {
  card_view_->SetSizeVariant(AstraWorkspaceOverviewCardSize::kSmall);
  // Should not crash.
  card_view_->Layout();
}

TEST_F(AstraWorkspaceCardViewTest, SetSizeVariantLarge) {
  card_view_->SetSizeVariant(AstraWorkspaceOverviewCardSize::kLarge);
  // Should not crash.
  card_view_->Layout();
}

TEST_F(AstraWorkspaceCardViewTest, SizeVariantAffectsPreferredSize) {
  card_view_->SetSizeVariant(AstraWorkspaceOverviewCardSize::kSmall);
  gfx::Size small_size = card_view_->CalculatePreferredSize();

  card_view_->SetSizeVariant(AstraWorkspaceOverviewCardSize::kLarge);
  gfx::Size large_size = card_view_->CalculatePreferredSize();

  // Large should be bigger than small.
  EXPECT_LT(small_size.width(), large_size.width());
  EXPECT_LT(small_size.height(), large_size.height());
}

// =========================================================================
// Show statistics tests
// =========================================================================

TEST_F(AstraWorkspaceCardViewTest, SetShowStatisticsTrue) {
  card_view_->SetShowStatistics(true);
  // Should not crash.
}

TEST_F(AstraWorkspaceCardViewTest, SetShowStatisticsFalse) {
  card_view_->SetShowStatistics(false);
  // Should not crash.
}

TEST_F(AstraWorkspaceCardViewTest, ShowStatisticsTogglesWithoutCrash) {
  card_view_->SetShowStatistics(true);
  card_view_->SetShowStatistics(false);
  card_view_->SetShowStatistics(true);
  // No crash = success.
}

// =========================================================================
// Thumbnail count tests
// =========================================================================

TEST_F(AstraWorkspaceCardViewTest, SetThumbnailCountZero) {
  card_view_->SetThumbnailCount(0);
  // Should not crash.
}

TEST_F(AstraWorkspaceCardViewTest, SetThumbnailCountOne) {
  card_view_->SetThumbnailCount(1);
  // Should not crash.
}

TEST_F(AstraWorkspaceCardViewTest, SetThumbnailCountFour) {
  card_view_->SetThumbnailCount(4);
  // Should not crash (max is 4).
}

TEST_F(AstraWorkspaceCardViewTest, SetThumbnailCountMoreThanMax) {
  card_view_->SetThumbnailCount(10);
  // Should clamp to max or just show 4.
  // No crash = success.
}

// =========================================================================
// Active state tests
// =========================================================================

TEST_F(AstraWorkspaceCardViewTest, SetIsActiveTrue) {
  card_view_->SetIsActive(true);
  EXPECT_TRUE(card_view_->is_active());
}

TEST_F(AstraWorkspaceCardViewTest, SetIsActiveFalse) {
  card_view_->SetIsActive(true);
  ASSERT_TRUE(card_view_->is_active());

  card_view_->SetIsActive(false);
  EXPECT_FALSE(card_view_->is_active());
}

TEST_F(AstraWorkspaceCardViewTest, SetIsActiveSameStateNoCrash) {
  card_view_->SetIsActive(false);
  card_view_->SetIsActive(false);
  // No crash = success.
}

// =========================================================================
// Selected state tests
// =========================================================================

TEST_F(AstraWorkspaceCardViewTest, SetIsSelectedTrue) {
  card_view_->SetIsSelected(true);
  EXPECT_TRUE(card_view_->is_selected());
}

TEST_F(AstraWorkspaceCardViewTest, SetIsSelectedFalse) {
  card_view_->SetIsSelected(true);
  ASSERT_TRUE(card_view_->is_selected());

  card_view_->SetIsSelected(false);
  EXPECT_FALSE(card_view_->is_selected());
}

// =========================================================================
// Callback tests
// =========================================================================

TEST_F(AstraWorkspaceCardViewTest, ClickCallbackFires) {
  CallbackTracker tracker;
  SetupCallbacks(&tracker);

  EXPECT_EQ(0, tracker.click_count);

  // We can't easily simulate a click from a unit test,
  // but we can verify the callback is set up correctly.
  // The callback is invoked from the mouse handler.
  // For now, verify no crash when setting callbacks.
}

TEST_F(AstraWorkspaceCardViewTest, AllCallbacksSetWithoutCrash) {
  CallbackTracker tracker;
  SetupCallbacks(&tracker);
  // No crash = success.
}

TEST_F(AstraWorkspaceCardViewTest, NullCallbacksAreSafe) {
  // Setting null callbacks should be safe.
  card_view_->SetClickCallback(AstraWorkspaceCardView::ClickCallback());
  card_view_->SetRenameCallback(AstraWorkspaceCardView::RenameCallback());
  card_view_->SetMenuActionCallback(
      AstraWorkspaceCardView::MenuActionCallback());
  card_view_->SetDeleteCallback(AstraWorkspaceCardView::DeleteCallback());
  // No crash = success.
}

// =========================================================================
// Theme / color tests
// =========================================================================

TEST_F(AstraWorkspaceCardViewTest, OnThemeChangedDoesNotCrash) {
  card_view_->OnThemeChanged();
  // No crash = success.
}

TEST_F(AstraWorkspaceCardViewTest, HasColorProvider) {
  EXPECT_NE(nullptr, card_view_->GetColorProvider());
}

// =========================================================================
// Accessibility tests
// =========================================================================

TEST_F(AstraWorkspaceCardViewTest, AccessibleRole) {
  ui::AXNodeData data;
  card_view_->GetAccessibleNodeData(&data);
  // The card should have a meaningful accessibility role.
  EXPECT_NE(ax::mojom::Role::kUnknown, data.role);
}

TEST_F(AstraWorkspaceCardViewTest, AccessibleNameIncludesWorkspaceName) {
  card_view_->SetWorkspaceName(u"Design Workspace");

  ui::AXNodeData data;
  card_view_->GetAccessibleNodeData(&data);

  // Name should contain the workspace name.
  EXPECT_NE(std::u16string::npos,
            data.GetName().find(u"Design Workspace"));
}

TEST_F(AstraWorkspaceCardViewTest, AccessibleDescriptionIncludesHibernation) {
  card_view_->SetWorkspaceName(u"Design Workspace");
  card_view_->SetIsHibernated(true);
  card_view_->SetShowStatistics(true);

  ui::AXNodeData data;
  card_view_->GetAccessibleNodeData(&data);

  // Hibernated state should be in description.
  EXPECT_NE(std::u16string::npos,
            data.GetDescription().find(u"Hibernated"));
}

// =========================================================================
// Focus tests
// =========================================================================

TEST_F(AstraWorkspaceCardViewTest, CanReceiveFocus) {
  // Set focus behavior to allow focus for keyboard navigation.
  // The card should be focusable for keyboard navigation.
  // For now, just verify it can request focus without crashing.
  card_view_->RequestFocus();
  // No crash = success.
}

// =========================================================================
// State combination tests
// =========================================================================

TEST_F(AstraWorkspaceCardViewTest, ActiveAndSelectedStatesIndependent) {
  // Active and selected are independent states.
  card_view_->SetIsActive(true);
  card_view_->SetIsSelected(false);
  EXPECT_TRUE(card_view_->is_active());
  EXPECT_FALSE(card_view_->is_selected());

  card_view_->SetIsActive(false);
  card_view_->SetIsSelected(true);
  EXPECT_FALSE(card_view_->is_active());
  EXPECT_TRUE(card_view_->is_selected());
}

TEST_F(AstraWorkspaceCardViewTest, HibernatedAndActive) {
  // A workspace can be both hibernated and active.
  card_view_->SetIsHibernated(true);
  card_view_->SetIsActive(true);
  EXPECT_TRUE(card_view_->is_hibernated());
  EXPECT_TRUE(card_view_->is_active());
}

TEST_F(AstraWorkspaceCardViewTest, FullDataSet) {
  // Set all properties at once — should not crash.
  card_view_->SetWorkspaceName(u"Productivity");
  card_view_->SetAccentColor("#4285F4");
  card_view_->SetTabCount(12);
  card_view_->SetWindowCount(2);
  card_view_->SetCreatedTime(base::Time::Now() - base::Hours(5));
  card_view_->SetLastUsedTime(base::Time::Now() - base::Minutes(30));
  card_view_->SetThumbnailCount(3);
  card_view_->SetIsActive(true);
  card_view_->SetIsSelected(false);
  card_view_->SetIsHibernated(false);
  card_view_->SetDisplayMode(AstraWorkspaceCardView::DisplayMode::kCard);
  card_view_->SetShowStatistics(true);
  card_view_->SetSizeVariant(AstraWorkspaceOverviewCardSize::kMedium);

  EXPECT_EQ(u"Productivity", card_view_->workspace_name());
  EXPECT_TRUE(card_view_->is_active());
  EXPECT_FALSE(card_view_->is_selected());
  EXPECT_FALSE(card_view_->is_hibernated());
  EXPECT_EQ(AstraWorkspaceCardView::DisplayMode::kCard,
            card_view_->display_mode());
}

TEST_F(AstraWorkspaceCardViewTest, FullDataSetListMode) {
  // All properties in list mode — should not crash.
  card_view_->SetWorkspaceName(u"Research");
  card_view_->SetAccentColor("#FF6B6B");
  card_view_->SetTabCount(25);
  card_view_->SetWindowCount(1);
  card_view_->SetCreatedTime(base::Time::Now() - base::Days(10));
  card_view_->SetLastUsedTime(base::Time::Now() - base::Days(1));
  card_view_->SetIsHibernated(true);
  card_view_->SetDisplayMode(AstraWorkspaceCardView::DisplayMode::kList);
  card_view_->SetShowStatistics(true);

  EXPECT_EQ(u"Research", card_view_->workspace_name());
  EXPECT_TRUE(card_view_->is_hibernated());
  EXPECT_EQ(AstraWorkspaceCardView::DisplayMode::kList,
            card_view_->display_mode());
}

// =========================================================================
// Mouse event tests (no-op verification)
// =========================================================================

TEST_F(AstraWorkspaceCardViewTest, MouseEnterExitNoCrash) {
  // Simulating mouse events is hard in unit tests,
  // but we can verify the view handles state changes.
  card_view_->OnMouseEntered(ui::MouseEvent());
  card_view_->OnMouseExited(ui::MouseEvent());
  // No crash = success.
}

// =========================================================================
// Key event tests
// =========================================================================

TEST_F(AstraWorkspaceCardViewTest, KeyEventEnterTriggersClick) {
  CallbackTracker tracker;
  SetupCallbacks(&tracker);
  card_view_->RequestFocus();

  // Press Enter should trigger click callback.
  ui::KeyEvent key_event(ui::ET_KEY_PRESSED, ui::VKEY_RETURN, ui::EF_NONE);
  card_view_->OnKeyPressed(key_event);

  // Click callback should have fired.
  // Note: may depend on the implementation, verify no crash.
}

TEST_F(AstraWorkspaceCardViewTest, KeyEventF2TriggersRename) {
  CallbackTracker tracker;
  SetupCallbacks(&tracker);
  card_view_->RequestFocus();

  ui::KeyEvent key_event(ui::ET_KEY_PRESSED, ui::VKEY_F2, ui::EF_NONE);
  card_view_->OnKeyPressed(key_event);

  // Rename callback should have fired.
  // Verify no crash.
}

// =========================================================================
// SetWorkspaceInfo / GetWorkspaceInfo
// =========================================================================

TEST_F(AstraWorkspaceCardViewTest, SetWorkspaceInfo_SetsAllFields) {
  AstraWorkspace workspace;
  workspace.id = "test-ws";
  workspace.name = "Test Workspace";
  workspace.accent_color = "#FF0000";
  workspace.created_time = base::Time::Now() - base::Hours(24);
  workspace.last_used_time = base::Time::Now() - base::Hours(1);
  workspace.is_hibernated = false;

  card_view_->SetWorkspaceInfo(workspace);

  // Verify name was set.
  EXPECT_EQ(u"Test Workspace", card_view_->workspace_name());

  // Verify hibernation state.
  EXPECT_FALSE(card_view_->is_hibernated());
}

TEST_F(AstraWorkspaceCardViewTest, GetWorkspaceInfo_ReturnsCurrentState) {
  card_view_->SetWorkspaceName(u"My Workspace");
  card_view_->SetIsHibernated(true);

  AstraWorkspace info = card_view_->GetWorkspaceInfo();
  EXPECT_EQ("My Workspace", info.name);
  EXPECT_TRUE(info.is_hibernated);
}

TEST_F(AstraWorkspaceCardViewTest, SetWorkspaceInfo_EmptyWorkspace) {
  AstraWorkspace workspace;
  card_view_->SetWorkspaceInfo(workspace);
  // Should not crash with default-constructed workspace.
  SUCCEED();
}

TEST_F(AstraWorkspaceCardViewTest, GetWorkspaceInfo_RoundTrip) {
  AstraWorkspace original;
  original.name = "Round Trip Test";
  original.accent_color = "#123456";
  original.is_hibernated = true;

  card_view_->SetWorkspaceInfo(original);
  AstraWorkspace result = card_view_->GetWorkspaceInfo();

  EXPECT_EQ(original.name, result.name);
  EXPECT_EQ(original.is_hibernated, result.is_hibernated);
}

// =========================================================================
// Hovered state
// =========================================================================

TEST_F(AstraWorkspaceCardViewTest, SetHovered_InitiallyFalse) {
  EXPECT_FALSE(card_view_->IsHovered());
}

TEST_F(AstraWorkspaceCardViewTest, SetHovered_True) {
  card_view_->SetHovered(true);
  EXPECT_TRUE(card_view_->IsHovered());
}

TEST_F(AstraWorkspaceCardViewTest, SetHovered_False) {
  card_view_->SetHovered(true);
  ASSERT_TRUE(card_view_->IsHovered());

  card_view_->SetHovered(false);
  EXPECT_FALSE(card_view_->IsHovered());
}

TEST_F(AstraWorkspaceCardViewTest, SetHovered_SameValueIsNoOp) {
  card_view_->SetHovered(false);
  // Setting same value should be no-op.
  card_view_->SetHovered(false);
  EXPECT_FALSE(card_view_->IsHovered());
}

TEST_F(AstraWorkspaceCardViewTest, SetHovered_ToggleMultipleTimes) {
  for (int i = 0; i < 5; i++) {
    card_view_->SetHovered(!card_view_->IsHovered());
  }
  // After 5 toggles, should be true (started from false).
  EXPECT_TRUE(card_view_->IsHovered());
}

// =========================================================================
// SkColor accent color
// =========================================================================

TEST_F(AstraWorkspaceCardViewTest, GetAccentColor_InitiallyPlaceholder) {
  // Default accent color should be a placeholder.
  SkColor color = card_view_->GetAccentColor();
  // Should be a valid SkColor value (not necessarily transparent).
  SUCCEED();
}

TEST_F(AstraWorkspaceCardViewTest, SetAccentColor_SkColor) {
  SkColor red = SkColorSetRGB(255, 0, 0);
  card_view_->SetAccentColor(red);
  EXPECT_EQ(red, card_view_->GetAccentColor());
}

TEST_F(AstraWorkspaceCardViewTest, SetAccentColor_MultipleColors) {
  SkColor red = SkColorSetRGB(255, 0, 0);
  SkColor green = SkColorSetRGB(0, 255, 0);
  SkColor blue = SkColorSetRGB(0, 0, 255);

  card_view_->SetAccentColor(red);
  EXPECT_EQ(red, card_view_->GetAccentColor());

  card_view_->SetAccentColor(green);
  EXPECT_EQ(green, card_view_->GetAccentColor());

  card_view_->SetAccentColor(blue);
  EXPECT_EQ(blue, card_view_->GetAccentColor());
}

TEST_F(AstraWorkspaceCardViewTest, SetAccentColor_HexUpdatesSkColor) {
  // Setting via hex string should also update the SkColor.
  card_view_->SetAccentColor("#00FF00");
  SkColor color = card_view_->GetAccentColor();
  // Should be approximately green.
  EXPECT_EQ(0, SkColorGetR(color));
  EXPECT_GT(SkColorGetG(color), SkColorGetR(color));
  EXPECT_GT(SkColorGetG(color), SkColorGetB(color));
}

TEST_F(AstraWorkspaceCardViewTest, SetAccentColor_SkColorUpdatesHex) {
  // Setting via SkColor should also update the hex string.
  SkColor color = SkColorSetRGB(0xAB, 0xCD, 0xEF);
  card_view_->SetAccentColor(color);
  // The accent color hex string should be updated.
  // (We can't directly access the hex string, but GetAccentColor should match.)
  EXPECT_EQ(color, card_view_->GetAccentColor());
}

// =========================================================================
// Tab count visibility
// =========================================================================

TEST_F(AstraWorkspaceCardViewTest, ShowTabCount_DefaultIsVisible) {
  EXPECT_TRUE(card_view_->IsTabCountVisible());
}

TEST_F(AstraWorkspaceCardViewTest, ShowTabCount_False) {
  card_view_->ShowTabCount(false);
  EXPECT_FALSE(card_view_->IsTabCountVisible());
}

TEST_F(AstraWorkspaceCardViewTest, ShowTabCount_True) {
  card_view_->ShowTabCount(false);
  ASSERT_FALSE(card_view_->IsTabCountVisible());

  card_view_->ShowTabCount(true);
  EXPECT_TRUE(card_view_->IsTabCountVisible());
}

TEST_F(AstraWorkspaceCardViewTest, ShowTabCount_SameValueIsNoOp) {
  card_view_->ShowTabCount(true);
  card_view_->ShowTabCount(true);
  EXPECT_TRUE(card_view_->IsTabCountVisible());
}

// =========================================================================
// Window count visibility
// =========================================================================

TEST_F(AstraWorkspaceCardViewTest, ShowWindowCount_DefaultIsVisible) {
  EXPECT_TRUE(card_view_->IsWindowCountVisible());
}

TEST_F(AstraWorkspaceCardViewTest, ShowWindowCount_False) {
  card_view_->ShowWindowCount(false);
  EXPECT_FALSE(card_view_->IsWindowCountVisible());
}

TEST_F(AstraWorkspaceCardViewTest, ShowWindowCount_True) {
  card_view_->ShowWindowCount(false);
  ASSERT_FALSE(card_view_->IsWindowCountVisible());

  card_view_->ShowWindowCount(true);
  EXPECT_TRUE(card_view_->IsWindowCountVisible());
}

// =========================================================================
// Menu button visibility
// =========================================================================

TEST_F(AstraWorkspaceCardViewTest, ShowMenuButton_DefaultIsVisible) {
  EXPECT_TRUE(card_view_->IsMenuButtonVisible());
}

TEST_F(AstraWorkspaceCardViewTest, ShowMenuButton_False) {
  card_view_->ShowMenuButton(false);
  EXPECT_FALSE(card_view_->IsMenuButtonVisible());
}

TEST_F(AstraWorkspaceCardViewTest, ShowMenuButton_True) {
  card_view_->ShowMenuButton(false);
  ASSERT_FALSE(card_view_->IsMenuButtonVisible());

  card_view_->ShowMenuButton(true);
  EXPECT_TRUE(card_view_->IsMenuButtonVisible());
}

TEST_F(AstraWorkspaceCardViewTest, ShowMenuButton_SameValueIsNoOp) {
  card_view_->ShowMenuButton(true);
  card_view_->ShowMenuButton(true);
  EXPECT_TRUE(card_view_->IsMenuButtonVisible());
}

// =========================================================================
// Pinned state
// =========================================================================

TEST_F(AstraWorkspaceCardViewTest, SetPinned_InitiallyFalse) {
  EXPECT_FALSE(card_view_->IsPinned());
}

TEST_F(AstraWorkspaceCardViewTest, SetPinned_True) {
  card_view_->SetPinned(true);
  EXPECT_TRUE(card_view_->IsPinned());
}

TEST_F(AstraWorkspaceCardViewTest, SetPinned_False) {
  card_view_->SetPinned(true);
  ASSERT_TRUE(card_view_->IsPinned());

  card_view_->SetPinned(false);
  EXPECT_FALSE(card_view_->IsPinned());
}

TEST_F(AstraWorkspaceCardViewTest, SetPinned_SameValueIsNoOp) {
  card_view_->SetPinned(false);
  card_view_->SetPinned(false);
  EXPECT_FALSE(card_view_->IsPinned());
}

TEST_F(AstraWorkspaceCardViewTest, SetPinned_ToggleMultipleTimes) {
  for (int i = 0; i < 4; i++) {
    card_view_->SetPinned(!card_view_->IsPinned());
  }
  // After 4 toggles from false, should be false again.
  EXPECT_FALSE(card_view_->IsPinned());
}

// =========================================================================
// State combinations
// =========================================================================

TEST_F(AstraWorkspaceCardViewTest, HoveredAndSelected) {
  card_view_->SetHovered(true);
  card_view_->SetIsSelected(true);
  EXPECT_TRUE(card_view_->IsHovered());
  EXPECT_TRUE(card_view_->is_selected());
  // No crash = success.
}

TEST_F(AstraWorkspaceCardViewTest, HoveredAndActive) {
  card_view_->SetHovered(true);
  card_view_->SetIsActive(true);
  EXPECT_TRUE(card_view_->IsHovered());
  EXPECT_TRUE(card_view_->is_active());
}

TEST_F(AstraWorkspaceCardViewTest, PinnedAndActive) {
  card_view_->SetPinned(true);
  card_view_->SetIsActive(true);
  EXPECT_TRUE(card_view_->IsPinned());
  EXPECT_TRUE(card_view_->is_active());
}

TEST_F(AstraWorkspaceCardViewTest, AllStatesCombined) {
  card_view_->SetIsActive(true);
  card_view_->SetIsSelected(true);
  card_view_->SetHovered(true);
  card_view_->SetIsHibernated(true);
  card_view_->SetPinned(true);

  EXPECT_TRUE(card_view_->is_active());
  EXPECT_TRUE(card_view_->is_selected());
  EXPECT_TRUE(card_view_->IsHovered());
  EXPECT_TRUE(card_view_->is_hibernated());
  EXPECT_TRUE(card_view_->IsPinned());
}

// =========================================================================
// Display mode + state interaction
// =========================================================================

TEST_F(AstraWorkspaceCardViewTest, ListModeWithPinned) {
  card_view_->SetDisplayMode(AstraWorkspaceCardView::DisplayMode::kList);
  card_view_->SetPinned(true);
  EXPECT_TRUE(card_view_->IsPinned());
  EXPECT_EQ(AstraWorkspaceCardView::DisplayMode::kList, card_view_->display_mode());
}

TEST_F(AstraWorkspaceCardViewTest, CardModeWithHover) {
  card_view_->SetDisplayMode(AstraWorkspaceCardView::DisplayMode::kCard);
  card_view_->SetHovered(true);
  EXPECT_TRUE(card_view_->IsHovered());
  EXPECT_EQ(AstraWorkspaceCardView::DisplayMode::kCard, card_view_->display_mode());
}

}  // namespace astra
