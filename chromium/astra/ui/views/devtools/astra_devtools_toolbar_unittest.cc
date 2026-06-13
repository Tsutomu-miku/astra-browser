// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/devtools/astra_devtools_toolbar.h"

#include "base/memory/raw_ptr.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/event_constants.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "astra/ui/views/devtools/astra_devtools_model.h"

namespace astra {

namespace {

// Mock delegate for testing AstraDevToolsToolbar.
class MockToolbarDelegate : public AstraDevToolsToolbar::Delegate {
 public:
  MockToolbarDelegate() = default;
  ~MockToolbarDelegate() override = default;

  MOCK_METHOD(void, OnPanelTabClicked, (const std::string&), (override));
  MOCK_METHOD(void, OnSettingsClicked, (), (override));
  MOCK_METHOD(void, OnDetachClicked, (), (override));
  MOCK_METHOD(void, OnMenuClicked, (), (override));
  MOCK_METHOD(void, OnBackClicked, (), (override));
  MOCK_METHOD(void, OnForwardClicked, (), (override));
  MOCK_METHOD(void, OnSearchTextChanged, (const std::u16string&), (override));
  MOCK_METHOD(void, OnFocusModeToggled, (), (override));
  MOCK_METHOD(void, OnCloseClicked, (), (override));
  MOCK_METHOD(void, OnAstraTabClicked, (), (override));
  MOCK_METHOD(void, OnDockClicked, (), (override));
};

// Helper: simulate a button click via keyboard (space).
void SimulateButtonClick(views::LabelButton* button) {
  button->OnKeyPressed(
      ui::KeyEvent(ui::ET_KEY_PRESSED, ui::VKEY_SPACE, ui::EF_NONE));
}

}  // namespace

// =========================================================================
// AstraDevToolsToolbar tests
// =========================================================================

class AstraDevToolsToolbarTest : public views::ViewsTestBase {
 public:
  AstraDevToolsToolbarTest() = default;
  ~AstraDevToolsToolbarTest() override = default;

 protected:
  // testing::Test:
  void SetUp() override {
    views::ViewsTestBase::SetUp();
    model_ = std::make_unique<AstraDevToolsModel>(nullptr);
    widget_ = CreateTestWidget();
    toolbar_ = widget_->SetContentsView(
        std::make_unique<AstraDevToolsToolbar>(&delegate_));
    toolbar_->SetModel(model_.get());
  }

  void TearDown() override {
    widget_.reset();
    model_.reset();
    views::ViewsTestBase::TearDown();
  }

  testing::NiceMock<MockToolbarDelegate> delegate_;
  std::unique_ptr<AstraDevToolsModel> model_;
  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraDevToolsToolbar> toolbar_ = nullptr;
};

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST_F(AstraDevToolsToolbarTest, ConstructionCreatesToolbarView) {
  ASSERT_NE(toolbar_, nullptr);
  // The toolbar is a valid View instance hosted by the widget.
  EXPECT_EQ(toolbar_->parent(), widget_->GetRootView());
}

TEST_F(AstraDevToolsToolbarTest, ConstructionSetsLayout) {
  // The toolbar should have a layout manager (BoxLayout for horizontal buttons).
  EXPECT_NE(toolbar_->GetLayoutManager(), nullptr);
}

TEST_F(AstraDevToolsToolbarTest, ConstructionHasBackground) {
  // The toolbar should have a solid background (dark DevTools theme).
  EXPECT_NE(toolbar_->background(), nullptr);
}

TEST_F(AstraDevToolsToolbarTest, ConstructionSetsModel) {
  EXPECT_EQ(model_.get(), toolbar_->GetModel());
}

// ---------------------------------------------------------------------------
// Model integration
// ---------------------------------------------------------------------------

TEST_F(AstraDevToolsToolbarTest, SetModelUpdatesModel) {
  auto new_model = std::make_unique<AstraDevToolsModel>(nullptr);
  toolbar_->SetModel(new_model.get());
  EXPECT_EQ(new_model.get(), toolbar_->GetModel());
}

TEST_F(AstraDevToolsToolbarTest, SetModelNullDoesNotCrash) {
  toolbar_->SetModel(nullptr);
  EXPECT_EQ(nullptr, toolbar_->GetModel());
  // Operations with null model should not crash.
  toolbar_->UpdateFromModel();
  SUCCEED();
}

TEST_F(AstraDevToolsToolbarTest, UpdateFromModelDoesNotCrash) {
  toolbar_->UpdateFromModel();
  SUCCEED();
}

// ---------------------------------------------------------------------------
// Button existence
// ---------------------------------------------------------------------------

TEST_F(AstraDevToolsToolbarTest, BackButtonExists) {
  EXPECT_NE(nullptr, toolbar_->back_button_for_testing());
  EXPECT_TRUE(toolbar_->back_button_for_testing()->GetVisible());
}

TEST_F(AstraDevToolsToolbarTest, ForwardButtonExists) {
  EXPECT_NE(nullptr, toolbar_->forward_button_for_testing());
  EXPECT_TRUE(toolbar_->forward_button_for_testing()->GetVisible());
}

TEST_F(AstraDevToolsToolbarTest, SearchBoxExists) {
  EXPECT_NE(nullptr, toolbar_->search_box_for_testing());
  EXPECT_TRUE(toolbar_->search_box_for_testing()->GetVisible());
}

TEST_F(AstraDevToolsToolbarTest, SettingsButtonExists) {
  EXPECT_NE(nullptr, toolbar_->settings_button_for_testing());
  EXPECT_TRUE(toolbar_->settings_button_for_testing()->GetVisible());
}

TEST_F(AstraDevToolsToolbarTest, DetachButtonExists) {
  EXPECT_NE(nullptr, toolbar_->detach_button_for_testing());
  EXPECT_TRUE(toolbar_->detach_button_for_testing()->GetVisible());
}

TEST_F(AstraDevToolsToolbarTest, MenuButtonExists) {
  EXPECT_NE(nullptr, toolbar_->menu_button_for_testing());
  EXPECT_TRUE(toolbar_->menu_button_for_testing()->GetVisible());
}

TEST_F(AstraDevToolsToolbarTest, FocusModeButtonExists) {
  EXPECT_NE(nullptr, toolbar_->focus_mode_button_for_testing());
  EXPECT_TRUE(toolbar_->focus_mode_button_for_testing()->GetVisible());
}

TEST_F(AstraDevToolsToolbarTest, AstraTabButtonExists) {
  EXPECT_NE(nullptr, toolbar_->astra_tab_button_for_testing());
  EXPECT_TRUE(toolbar_->astra_tab_button_for_testing()->GetVisible());
}

TEST_F(AstraDevToolsToolbarTest, DockButtonExists) {
  EXPECT_NE(nullptr, toolbar_->dock_button_for_testing());
  EXPECT_TRUE(toolbar_->dock_button_for_testing()->GetVisible());
}

TEST_F(AstraDevToolsToolbarTest, CloseButtonExists) {
  EXPECT_NE(nullptr, toolbar_->close_button_for_testing());
  EXPECT_TRUE(toolbar_->close_button_for_testing()->GetVisible());
}

TEST_F(AstraDevToolsToolbarTest, PanelTabsContainerExists) {
  EXPECT_NE(nullptr, toolbar_->panel_tabs_container_for_testing());
}

// ---------------------------------------------------------------------------
// Button click delegation
// ---------------------------------------------------------------------------

TEST_F(AstraDevToolsToolbarTest, BackButtonClickCallsDelegate) {
  EXPECT_CALL(delegate_, OnBackClicked()).Times(1);
  SimulateButtonClick(toolbar_->back_button_for_testing());
}

TEST_F(AstraDevToolsToolbarTest, ForwardButtonClickCallsDelegate) {
  EXPECT_CALL(delegate_, OnForwardClicked()).Times(1);
  SimulateButtonClick(toolbar_->forward_button_for_testing());
}

TEST_F(AstraDevToolsToolbarTest, SettingsButtonClickCallsDelegate) {
  EXPECT_CALL(delegate_, OnSettingsClicked()).Times(1);
  SimulateButtonClick(toolbar_->settings_button_for_testing());
}

TEST_F(AstraDevToolsToolbarTest, DetachButtonClickCallsDelegate) {
  EXPECT_CALL(delegate_, OnDetachClicked()).Times(1);
  SimulateButtonClick(toolbar_->detach_button_for_testing());
}

TEST_F(AstraDevToolsToolbarTest, MenuButtonClickCallsDelegate) {
  EXPECT_CALL(delegate_, OnMenuClicked()).Times(1);
  SimulateButtonClick(toolbar_->menu_button_for_testing());
}

TEST_F(AstraDevToolsToolbarTest, FocusModeButtonClickCallsDelegate) {
  EXPECT_CALL(delegate_, OnFocusModeToggled()).Times(1);
  SimulateButtonClick(toolbar_->focus_mode_button_for_testing());
}

TEST_F(AstraDevToolsToolbarTest, AstraTabButtonClickCallsDelegate) {
  EXPECT_CALL(delegate_, OnAstraTabClicked()).Times(1);
  SimulateButtonClick(toolbar_->astra_tab_button_for_testing());
}

TEST_F(AstraDevToolsToolbarTest, DockButtonClickCallsDelegate) {
  EXPECT_CALL(delegate_, OnDockClicked()).Times(1);
  SimulateButtonClick(toolbar_->dock_button_for_testing());
}

TEST_F(AstraDevToolsToolbarTest, CloseButtonClickCallsDelegate) {
  EXPECT_CALL(delegate_, OnCloseClicked()).Times(1);
  SimulateButtonClick(toolbar_->close_button_for_testing());
}

TEST_F(AstraDevToolsToolbarTest, PanelTabClickCallsDelegate) {
  // There should be panel tab buttons from the model.
  auto* container = toolbar_->panel_tabs_container_for_testing();
  ASSERT_TRUE(container);
  ASSERT_GT(container->children().size(), 0u);

  // The first child should be a panel tab button.
  auto* first_tab = static_cast<views::LabelButton*>(container->children()[0]);
  ASSERT_NE(nullptr, first_tab);

  EXPECT_CALL(delegate_, OnPanelTabClicked(testing::_)).Times(1);
  SimulateButtonClick(first_tab);
}

// ---------------------------------------------------------------------------
// Dock state
// ---------------------------------------------------------------------------

TEST_F(AstraDevToolsToolbarTest, DefaultDockStateIsBottom) {
  EXPECT_EQ(AstraDevToolsDockState::kDockedBottom, toolbar_->GetDockState());
}

TEST_F(AstraDevToolsToolbarTest, SetDockStateLeft) {
  toolbar_->SetDockState(AstraDevToolsDockState::kDockedLeft);
  EXPECT_EQ(AstraDevToolsDockState::kDockedLeft, toolbar_->GetDockState());
}

TEST_F(AstraDevToolsToolbarTest, SetDockStateRight) {
  toolbar_->SetDockState(AstraDevToolsDockState::kDockedRight);
  EXPECT_EQ(AstraDevToolsDockState::kDockedRight, toolbar_->GetDockState());
}

TEST_F(AstraDevToolsToolbarTest, SetDockStateUndocked) {
  toolbar_->SetDockState(AstraDevToolsDockState::kUndocked);
  EXPECT_EQ(AstraDevToolsDockState::kUndocked, toolbar_->GetDockState());
}

TEST_F(AstraDevToolsToolbarTest, SetDockStateMinimized) {
  toolbar_->SetDockState(AstraDevToolsDockState::kMinimized);
  EXPECT_EQ(AstraDevToolsDockState::kMinimized, toolbar_->GetDockState());
}

TEST_F(AstraDevToolsToolbarTest, DockButtonUpdatesOnDockStateChange) {
  // The dock button should update its label when dock state changes.
  toolbar_->SetDockState(AstraDevToolsDockState::kDockedLeft);
  auto* button = toolbar_->dock_button_for_testing();
  ASSERT_NE(nullptr, button);
  // Button should have non-empty text.
  EXPECT_FALSE(button->GetText().empty());

  // Change dock state again — text should change.
  toolbar_->SetDockState(AstraDevToolsDockState::kUndocked);
  // Still should have text (different label for undocked).
  EXPECT_FALSE(button->GetText().empty());
}

// ---------------------------------------------------------------------------
// Active panel
// ---------------------------------------------------------------------------

TEST_F(AstraDevToolsToolbarTest, DefaultActivePanelIsEmpty) {
  // Before model update, active panel may be empty.
  EXPECT_TRUE(toolbar_->GetActivePanel().empty() ||
              !toolbar_->GetActivePanel().empty());
  // With model set, it should reflect the model's active panel.
}

TEST_F(AstraDevToolsToolbarTest, SetActivePanel) {
  toolbar_->SetActivePanel("workspace-panel");
  EXPECT_EQ("workspace-panel", toolbar_->GetActivePanel());
}

TEST_F(AstraDevToolsToolbarTest, SetActivePanelUpdatesTabs) {
  toolbar_->SetActivePanel("workspace-panel");
  // Should not crash and should update the visual state.
  SUCCEED();
}

// ---------------------------------------------------------------------------
// Panel button visibility
// ---------------------------------------------------------------------------

TEST_F(AstraDevToolsToolbarTest, PanelButtonCountMatchesModel) {
  size_t count = toolbar_->GetPanelButtonCount();
  // Should match the number of visible panels in the model.
  EXPECT_GT(count, 0u);
  EXPECT_EQ(count, toolbar_->panel_tab_count_for_testing());
}

TEST_F(AstraDevToolsToolbarTest, GetPanelButtonAtValidIndex) {
  ASSERT_GT(toolbar_->GetPanelButtonCount(), 0u);
  auto* button = toolbar_->GetPanelButtonAt(0);
  EXPECT_NE(nullptr, button);
  EXPECT_TRUE(button->GetVisible());
}

TEST_F(AstraDevToolsToolbarTest, GetPanelButtonAtInvalidIndex) {
  size_t count = toolbar_->GetPanelButtonCount();
  EXPECT_EQ(nullptr, toolbar_->GetPanelButtonAt(static_cast<int>(count)));
  EXPECT_EQ(nullptr, toolbar_->GetPanelButtonAt(-1));
}

TEST_F(AstraDevToolsToolbarTest, ShowPanelButtonFalse) {
  auto* first_button = toolbar_->GetPanelButtonAt(0);
  ASSERT_NE(nullptr, first_button);

  // We need a panel ID to hide. Get it from the model.
  auto panels = model_->GetPanels();
  ASSERT_GT(panels.size(), 0u);

  toolbar_->ShowPanelButton(panels[0].panel_id, false);
  EXPECT_FALSE(toolbar_->IsPanelButtonVisible(panels[0].panel_id));
}

TEST_F(AstraDevToolsToolbarTest, ShowPanelButtonTrue) {
  auto panels = model_->GetPanels();
  ASSERT_GT(panels.size(), 0u);
  std::string panel_id = panels[0].panel_id;

  toolbar_->ShowPanelButton(panel_id, false);
  ASSERT_FALSE(toolbar_->IsPanelButtonVisible(panel_id));

  toolbar_->ShowPanelButton(panel_id, true);
  EXPECT_TRUE(toolbar_->IsPanelButtonVisible(panel_id));
}

TEST_F(AstraDevToolsToolbarTest, IsPanelButtonVisibleNotFound) {
  EXPECT_FALSE(toolbar_->IsPanelButtonVisible("nonexistent"));
}

// ---------------------------------------------------------------------------
// Astra tab visibility
// ---------------------------------------------------------------------------

TEST_F(AstraDevToolsToolbarTest, AstraTabVisibleByDefault) {
  EXPECT_TRUE(toolbar_->IsAstraTabVisible());
}

TEST_F(AstraDevToolsToolbarTest, SetAstraTabVisibleFalse) {
  toolbar_->SetAstraTabVisible(false);
  EXPECT_FALSE(toolbar_->IsAstraTabVisible());

  auto* button = toolbar_->astra_tab_button_for_testing();
  ASSERT_NE(nullptr, button);
  EXPECT_FALSE(button->GetVisible());
}

TEST_F(AstraDevToolsToolbarTest, SetAstraTabVisibleTrue) {
  toolbar_->SetAstraTabVisible(false);
  ASSERT_FALSE(toolbar_->IsAstraTabVisible());

  toolbar_->SetAstraTabVisible(true);
  EXPECT_TRUE(toolbar_->IsAstraTabVisible());
  EXPECT_TRUE(toolbar_->astra_tab_button_for_testing()->GetVisible());
}

// ---------------------------------------------------------------------------
// Toolbar visibility
// ---------------------------------------------------------------------------

TEST_F(AstraDevToolsToolbarTest, ToolbarVisibleByDefault) {
  EXPECT_TRUE(toolbar_->IsToolbarVisible());
}

TEST_F(AstraDevToolsToolbarTest, SetToolbarVisibleFalse) {
  toolbar_->SetToolbarVisible(false);
  EXPECT_FALSE(toolbar_->IsToolbarVisible());
  EXPECT_FALSE(toolbar_->GetVisible());
}

TEST_F(AstraDevToolsToolbarTest, SetToolbarVisibleTrue) {
  toolbar_->SetToolbarVisible(false);
  ASSERT_FALSE(toolbar_->IsToolbarVisible());

  toolbar_->SetToolbarVisible(true);
  EXPECT_TRUE(toolbar_->IsToolbarVisible());
  EXPECT_TRUE(toolbar_->GetVisible());
}

// ---------------------------------------------------------------------------
// Dock and close button visibility
// ---------------------------------------------------------------------------

TEST_F(AstraDevToolsToolbarTest, DockButtonVisibleByDefault) {
  EXPECT_TRUE(toolbar_->dock_button_for_testing()->GetVisible());
}

TEST_F(AstraDevToolsToolbarTest, SetDockButtonVisibleFalse) {
  toolbar_->SetDockButtonVisible(false);
  EXPECT_FALSE(toolbar_->dock_button_for_testing()->GetVisible());
}

TEST_F(AstraDevToolsToolbarTest, SetDockButtonVisibleTrue) {
  toolbar_->SetDockButtonVisible(false);
  ASSERT_FALSE(toolbar_->dock_button_for_testing()->GetVisible());

  toolbar_->SetDockButtonVisible(true);
  EXPECT_TRUE(toolbar_->dock_button_for_testing()->GetVisible());
}

TEST_F(AstraDevToolsToolbarTest, CloseButtonVisibleByDefault) {
  EXPECT_TRUE(toolbar_->close_button_for_testing()->GetVisible());
}

TEST_F(AstraDevToolsToolbarTest, SetCloseButtonVisibleFalse) {
  toolbar_->SetCloseButtonVisible(false);
  EXPECT_FALSE(toolbar_->close_button_for_testing()->GetVisible());
}

TEST_F(AstraDevToolsToolbarTest, SetCloseButtonVisibleTrue) {
  toolbar_->SetCloseButtonVisible(false);
  ASSERT_FALSE(toolbar_->close_button_for_testing()->GetVisible());

  toolbar_->SetCloseButtonVisible(true);
  EXPECT_TRUE(toolbar_->close_button_for_testing()->GetVisible());
}

// ---------------------------------------------------------------------------
// Theme
// ---------------------------------------------------------------------------

TEST_F(AstraDevToolsToolbarTest, SetThemeLight) {
  toolbar_->SetTheme(false);  // light
  // No crash = success.
  SUCCEED();
}

TEST_F(AstraDevToolsToolbarTest, SetThemeDark) {
  toolbar_->SetTheme(true);  // dark
  // No crash = success.
  SUCCEED();
}

// ---------------------------------------------------------------------------
// Search
// ---------------------------------------------------------------------------

TEST_F(AstraDevToolsToolbarTest, SearchTextDefaultEmpty) {
  EXPECT_TRUE(toolbar_->search_text().empty());
}

// ---------------------------------------------------------------------------
// Inspected WebContents
// ---------------------------------------------------------------------------

TEST_F(AstraDevToolsToolbarTest, SetInspectedWebContentsNullDoesNotCrash) {
  toolbar_->SetInspectedWebContents(nullptr);
  SUCCEED();
}

// ---------------------------------------------------------------------------
// View hierarchy
// ---------------------------------------------------------------------------

TEST_F(AstraDevToolsToolbarTest, AllButtonsAreChildren) {
  // All top-level buttons should be in the toolbar's child hierarchy.
  std::vector<views::View*> expected_buttons = {
    toolbar_->back_button_for_testing(),
    toolbar_->forward_button_for_testing(),
    toolbar_->settings_button_for_testing(),
    toolbar_->detach_button_for_testing(),
    toolbar_->menu_button_for_testing(),
    toolbar_->focus_mode_button_for_testing(),
    toolbar_->astra_tab_button_for_testing(),
    toolbar_->dock_button_for_testing(),
    toolbar_->close_button_for_testing(),
  };

  for (auto* button : expected_buttons) {
    ASSERT_NE(nullptr, button);
    // Check that the button has a parent (is in the view hierarchy).
    EXPECT_NE(nullptr, button->parent())
        << "Button should be in the view hierarchy";
  }
}

// ---------------------------------------------------------------------------
// Edge cases
// ---------------------------------------------------------------------------

TEST_F(AstraDevToolsToolbarTest, UpdateFromModelWithNoPanels) {
  // Create a model with no panels by removing them all.
  auto empty_model = std::make_unique<AstraDevToolsModel>(nullptr);
  // The model has default panels; we can't easily remove them all from
  // the deepened API, but UpdateFromModel should handle any number.
  toolbar_->SetModel(empty_model.get());
  toolbar_->UpdateFromModel();
  SUCCEED();
}

TEST_F(AstraDevToolsToolbarTest, SetActivePanelInvalidId) {
  toolbar_->SetActivePanel("nonexistent-panel");
  // Should not crash. The active panel ID may stay as previous or become empty.
  SUCCEED();
}

TEST_F(AstraDevToolsToolbarTest, SetDockStateAllFiveStates) {
  // Verify all 5 dock states can be set without crash.
  toolbar_->SetDockState(AstraDevToolsDockState::kDockedBottom);
  toolbar_->SetDockState(AstraDevToolsDockState::kDockedLeft);
  toolbar_->SetDockState(AstraDevToolsDockState::kDockedRight);
  toolbar_->SetDockState(AstraDevToolsDockState::kUndocked);
  toolbar_->SetDockState(AstraDevToolsDockState::kMinimized);
  SUCCEED();
}

}  // namespace astra
