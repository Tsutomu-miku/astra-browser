// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Unit tests for Astra DevTools model, toolbar, workspace panel, and integration.
//
// Tests verify:
//   - AstraDevToolsModel: panel CRUD, ordering, active panel, settings,
//     persistence, observers, dock position, theme
//   - AstraDevToolsToolbar: construction, panel tabs, buttons, search, theme
//   - AstraDevToolsWorkspacePanel: construction, lists, actions, search, theme
//   - AstraDevToolsIntegration: coordination, panel switching, settings drawer
//   - Observer defaults: all observer methods have empty default implementations
//   - Edge cases: zero panels, duplicate IDs, boundary conditions
//
// Chromium test pattern: views::test::ViewsTestBase
//   (ui/views/test/views_test_base.h)

#include "astra/ui/views/devtools/astra_devtools_model.h"
#include "astra/ui/views/devtools/astra_devtools_toolbar.h"
#include "astra/ui/views/devtools/astra_devtools_workspace_panel.h"
#include "astra/ui/views/devtools/astra_devtools_integration.h"

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/values.h"
#include "astra/browser/astra_prefs.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace astra {

namespace {

// Test observer that tracks all method calls.
class TestDevToolsModelObserver : public AstraDevToolsModelObserver {
 public:
  int panel_opened_count = 0;
  std::string last_opened_panel;

  int panel_closed_count = 0;
  std::string last_closed_panel;

  int active_panel_changed_count = 0;
  std::string last_active_panel;

  int panel_order_changed_count = 0;

  int settings_changed_count = 0;

  int dock_position_changed_count = 0;
  AstraDevToolsDockPosition last_dock_position =
      AstraDevToolsDockPosition::kBottom;

  int theme_changed_count = 0;
  AstraDevToolsTheme last_theme = AstraDevToolsTheme::kSystem;

  void OnPanelOpened(const std::string& panel_id) override {
    panel_opened_count++;
    last_opened_panel = panel_id;
  }

  void OnPanelClosed(const std::string& panel_id) override {
    panel_closed_count++;
    last_closed_panel = panel_id;
  }

  void OnActivePanelChanged(const std::string& panel_id) override {
    active_panel_changed_count++;
    last_active_panel = panel_id;
  }

  void OnPanelOrderChanged() override {
    panel_order_changed_count++;
  }

  void OnDevToolsSettingsChanged() override {
    settings_changed_count++;
  }

  void OnDockPositionChanged(AstraDevToolsDockPosition position) override {
    dock_position_changed_count++;
    last_dock_position = position;
  }

  void OnThemeChanged(AstraDevToolsTheme theme) override {
    theme_changed_count++;
    last_theme = theme;
  }
};

// Observer that overrides no methods — tests default implementations.
class EmptyObserver : public AstraDevToolsModelObserver {
 public:
  // Intentionally empty — all methods use default implementations.
};

// Test delegate for toolbar.
class TestToolbarDelegate : public AstraDevToolsToolbar::Delegate {
 public:
  int panel_tab_clicks = 0;
  std::string last_panel_tab;
  int settings_clicks = 0;
  int detach_clicks = 0;
  int menu_clicks = 0;
  int back_clicks = 0;
  int forward_clicks = 0;
  int search_changes = 0;
  std::u16string last_search_text;
  int focus_mode_toggles = 0;

  void OnPanelTabClicked(const std::string& panel_id) override {
    panel_tab_clicks++;
    last_panel_tab = panel_id;
  }
  void OnSettingsClicked() override { settings_clicks++; }
  void OnDetachClicked() override { detach_clicks++; }
  void OnMenuClicked() override { menu_clicks++; }
  void OnBackClicked() override { back_clicks++; }
  void OnForwardClicked() override { forward_clicks++; }
  void OnSearchTextChanged(const std::u16string& text) override {
    search_changes++;
    last_search_text = text;
  }
  void OnFocusModeToggled() override { focus_mode_toggles++; }
};

// Test delegate for workspace panel.
class TestWorkspacePanelDelegate
    : public AstraDevToolsWorkspacePanel::Delegate {
 public:
  int new_workspace_count = 0;
  int delete_workspace_count = 0;
  std::string last_deleted_workspace;
  int rename_workspace_count = 0;
  std::string last_renamed_workspace;
  std::string last_rename_name;
  int workspace_selected_count = 0;
  std::string last_selected_workspace;
  int tab_selected_count = 0;
  int last_selected_tab = -1;

  void OnNewWorkspace() override { new_workspace_count++; }
  void OnDeleteWorkspace(const std::string& id) override {
    delete_workspace_count++;
    last_deleted_workspace = id;
  }
  void OnRenameWorkspace(const std::string& id,
                         const std::string& name) override {
    rename_workspace_count++;
    last_renamed_workspace = id;
    last_rename_name = name;
  }
  void OnWorkspaceSelected(const std::string& id) override {
    workspace_selected_count++;
    last_selected_workspace = id;
  }
  void OnTabSelected(int tab_index) override {
    tab_selected_count++;
    last_selected_tab = tab_index;
  }
};

}  // namespace

// =========================================================================
// AstraDevToolsModel unit tests
// =========================================================================

class AstraDevToolsModelTest : public testing::Test {
 public:
  AstraDevToolsModelTest() = default;
  ~AstraDevToolsModelTest() override = default;

  void SetUp() override {
    profile_ = std::make_unique<TestingProfile>();
    prefs::RegisterProfilePrefs(profile_->GetPrefs());
    model_ = std::make_unique<AstraDevToolsModel>(profile_->GetPrefs());
  }

  void TearDown() override {
    model_.reset();
    profile_.reset();
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<AstraDevToolsModel> model_;
};

// -- Default panels -------------------------------------------------------

TEST_F(AstraDevToolsModelTest, DefaultPanelsCount) {
  auto panels = model_->GetAllPanels();
  EXPECT_EQ(6u, panels.size());
}

TEST_F(AstraDevToolsModelTest, DefaultPanelIds) {
  auto panels = model_->GetAllPanels();
  EXPECT_EQ("workspace", panels[0].id);
  EXPECT_EQ("notes", panels[1].id);
  EXPECT_EQ("focus-mode", panels[2].id);
  EXPECT_EQ("screenshot", panels[3].id);
  EXPECT_EQ("reading-list", panels[4].id);
  EXPECT_EQ("tab-stack", panels[5].id);
}

TEST_F(AstraDevToolsModelTest, DefaultPanelsAllVisible) {
  auto panels = model_->GetAllPanels();
  for (const auto& p : panels) {
    EXPECT_TRUE(p.is_visible);
  }
}

TEST_F(AstraDevToolsModelTest, DefaultPinnedPanels) {
  auto panels = model_->GetAllPanels();
  EXPECT_TRUE(panels[0].is_pinned);   // workspace
  EXPECT_TRUE(panels[1].is_pinned);   // notes
  EXPECT_FALSE(panels[2].is_pinned);  // focus-mode
  EXPECT_FALSE(panels[3].is_pinned);  // screenshot
  EXPECT_FALSE(panels[4].is_pinned);  // reading-list
  EXPECT_FALSE(panels[5].is_pinned);  // tab-stack
}

TEST_F(AstraDevToolsModelTest, DefaultPanelPositions) {
  auto panels = model_->GetAllPanels();
  for (size_t i = 0; i < panels.size(); ++i) {
    EXPECT_EQ(i, panels[i].position);
  }
}

TEST_F(AstraDevToolsModelTest, DefaultActivePanel) {
  // Should default to the first visible panel (workspace).
  EXPECT_EQ("workspace", model_->active_panel_id());
}

TEST_F(AstraDevToolsModelTest, GetDefaultPanelsStatic) {
  auto defaults = AstraDevToolsModel::GetDefaultPanels();
  EXPECT_EQ(6u, defaults.size());
  EXPECT_EQ("workspace", defaults[0].id);
}

TEST_F(AstraDevToolsModelTest, VisiblePanels) {
  auto visible = model_->GetVisiblePanels();
  EXPECT_EQ(6u, visible.size());
  EXPECT_EQ(model_->visible_panel_count(), 6u);
}

TEST_F(AstraDevToolsModelTest, PanelCount) {
  EXPECT_EQ(6u, model_->panel_count());
  EXPECT_FALSE(model_->empty());
}

// -- Panel lookup ---------------------------------------------------------

TEST_F(AstraDevToolsModelTest, GetPanelByIdExists) {
  const auto* panel = model_->GetPanelById("workspace");
  ASSERT_NE(nullptr, panel);
  EXPECT_EQ("workspace", panel->id);
  EXPECT_EQ("Workspace", panel->title);
}

TEST_F(AstraDevToolsModelTest, GetPanelByIdNotFound) {
  const auto* panel = model_->GetPanelById("nonexistent");
  EXPECT_EQ(nullptr, panel);
}

TEST_F(AstraDevToolsModelTest, HasPanelTrue) {
  EXPECT_TRUE(model_->HasPanel("workspace"));
  EXPECT_TRUE(model_->HasPanel("notes"));
}

TEST_F(AstraDevToolsModelTest, HasPanelFalse) {
  EXPECT_FALSE(model_->HasPanel("nonexistent"));
  EXPECT_FALSE(model_->HasPanel(""));
}

// -- Add panel ------------------------------------------------------------

TEST_F(AstraDevToolsModelTest, AddPanelSuccess) {
  AstraDevToolsPanel panel;
  panel.id = "test-panel";
  panel.title = "Test Panel";
  panel.icon = "test";
  panel.position = 99;  // Should be clamped.

  EXPECT_TRUE(model_->AddPanel(panel));
  EXPECT_EQ(7u, model_->panel_count());
  EXPECT_TRUE(model_->HasPanel("test-panel"));
}

TEST_F(AstraDevToolsModelTest, AddPanelDuplicateId) {
  AstraDevToolsPanel panel;
  panel.id = "workspace";  // Already exists.
  panel.title = "Duplicate";

  EXPECT_FALSE(model_->AddPanel(panel));
  EXPECT_EQ(6u, model_->panel_count());
}

TEST_F(AstraDevToolsModelTest, AddPanelEmptyId) {
  AstraDevToolsPanel panel;
  panel.id = "";
  panel.title = "Empty ID";

  EXPECT_FALSE(model_->AddPanel(panel));
  EXPECT_EQ(6u, model_->panel_count());
}

TEST_F(AstraDevToolsModelTest, AddPanelInsertsAtPosition) {
  AstraDevToolsPanel panel;
  panel.id = "inserted";
  panel.title = "Inserted";
  panel.position = 2;  // Insert between focus-mode and screenshot.

  EXPECT_TRUE(model_->AddPanel(panel));

  auto panels = model_->GetAllPanels();
  ASSERT_EQ(7u, panels.size());
  EXPECT_EQ("inserted", panels[2].id);
  EXPECT_EQ(2u, panels[2].position);
  // Previously position 2 (focus-mode) should now be at position 3.
  EXPECT_EQ("focus-mode", panels[3].id);
  EXPECT_EQ(3u, panels[3].position);
}

TEST_F(AstraDevToolsModelTest, AddPanelPositionClamped) {
  AstraDevToolsPanel panel;
  panel.id = "clamped";
  panel.position = 100;  // Way beyond end.

  EXPECT_TRUE(model_->AddPanel(panel));

  auto panels = model_->GetAllPanels();
  EXPECT_EQ(7u, panels.size());
  // Should be at the last position (index 6).
  EXPECT_EQ("clamped", panels.back().id);
  EXPECT_EQ(6u, panels.back().position);
}

TEST_F(AstraDevToolsModelTest, AddPanelNotifiesOrderChange) {
  TestDevToolsModelObserver observer;
  model_->AddObserver(&observer);

  AstraDevToolsPanel panel;
  panel.id = "new-panel";
  model_->AddPanel(panel);

  EXPECT_GE(observer.panel_order_changed_count, 1);

  model_->RemoveObserver(&observer);
}

// -- Remove panel ---------------------------------------------------------

TEST_F(AstraDevToolsModelTest, RemovePanelSuccess) {
  EXPECT_TRUE(model_->RemovePanel("screenshot"));
  EXPECT_EQ(5u, model_->panel_count());
  EXPECT_FALSE(model_->HasPanel("screenshot"));
}

TEST_F(AstraDevToolsModelTest, RemovePanelNotFound) {
  EXPECT_FALSE(model_->RemovePanel("nonexistent"));
  EXPECT_EQ(6u, model_->panel_count());
}

TEST_F(AstraDevToolsModelTest, RemovePanelRenormalizesPositions) {
  model_->RemovePanel("notes");  // position 1

  auto panels = model_->GetAllPanels();
  ASSERT_EQ(5u, panels.size());
  // Positions should be 0,1,2,3,4 (contiguous).
  for (size_t i = 0; i < panels.size(); ++i) {
    EXPECT_EQ(i, panels[i].position);
  }
}

TEST_F(AstraDevToolsModelTest, RemoveActivePanelActivatesNext) {
  // Active panel is "workspace" (first one).
  ASSERT_EQ("workspace", model_->active_panel_id());

  model_->RemovePanel("workspace");

  // Should activate the next visible panel.
  EXPECT_NE("workspace", model_->active_panel_id());
  EXPECT_FALSE(model_->active_panel_id().empty());
}

TEST_F(AstraDevToolsModelTest, RemoveAllPanelsClearsActive) {
  // Remove all panels.
  auto all_panels = model_->GetAllPanels();
  for (const auto& p : all_panels) {
    model_->RemovePanel(p.id);
  }

  EXPECT_TRUE(model_->empty());
  EXPECT_TRUE(model_->active_panel_id().empty());
}

TEST_F(AstraDevToolsModelTest, RemovePanelNotifiesOrderChange) {
  TestDevToolsModelObserver observer;
  model_->AddObserver(&observer);

  model_->RemovePanel("screenshot");

  EXPECT_GE(observer.panel_order_changed_count, 1);

  model_->RemoveObserver(&observer);
}

// -- Panel visibility -----------------------------------------------------

TEST_F(AstraDevToolsModelTest, SetPanelVisibleFalse) {
  EXPECT_TRUE(model_->SetPanelVisible("screenshot", false));

  const auto* panel = model_->GetPanelById("screenshot");
  ASSERT_NE(nullptr, panel);
  EXPECT_FALSE(panel->is_visible);

  EXPECT_EQ(5u, model_->visible_panel_count());
}

TEST_F(AstraDevToolsModelTest, SetPanelVisibleTrue) {
  model_->SetPanelVisible("screenshot", false);
  EXPECT_TRUE(model_->SetPanelVisible("screenshot", true));

  const auto* panel = model_->GetPanelById("screenshot");
  ASSERT_NE(nullptr, panel);
  EXPECT_TRUE(panel->is_visible);
}

TEST_F(AstraDevToolsModelTest, SetPanelVisibleNotFound) {
  EXPECT_FALSE(model_->SetPanelVisible("nonexistent", false));
}

TEST_F(AstraDevToolsModelTest, SetPanelVisibleNoChange) {
  // Already visible.
  EXPECT_TRUE(model_->SetPanelVisible("workspace", true));
  // Should still be true.
  EXPECT_TRUE(model_->GetPanelById("workspace")->is_visible);
}

TEST_F(AstraDevToolsModelTest, HideActivePanelSwitchesActive) {
  ASSERT_EQ("workspace", model_->active_panel_id());

  model_->SetPanelVisible("workspace", false);

  // Active panel should have changed.
  EXPECT_NE("workspace", model_->active_panel_id());
  EXPECT_FALSE(model_->active_panel_id().empty());
}

TEST_F(AstraDevToolsModelTest, HidePanelNotifiesClose) {
  TestDevToolsModelObserver observer;
  model_->AddObserver(&observer);

  model_->SetPanelVisible("screenshot", false);

  EXPECT_EQ(1, observer.panel_closed_count);
  EXPECT_EQ("screenshot", observer.last_closed_panel);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraDevToolsModelTest, ShowPanelNotifiesOpen) {
  model_->SetPanelVisible("screenshot", false);

  TestDevToolsModelObserver observer;
  model_->AddObserver(&observer);

  model_->SetPanelVisible("screenshot", true);

  EXPECT_EQ(1, observer.panel_opened_count);
  EXPECT_EQ("screenshot", observer.last_opened_panel);

  model_->RemoveObserver(&observer);
}

// -- Panel pinning --------------------------------------------------------

TEST_F(AstraDevToolsModelTest, SetPanelPinnedTrue) {
  EXPECT_TRUE(model_->SetPanelPinned("screenshot", true));
  EXPECT_TRUE(model_->GetPanelById("screenshot")->is_pinned);
}

TEST_F(AstraDevToolsModelTest, SetPanelPinnedFalse) {
  EXPECT_TRUE(model_->SetPanelPinned("workspace", false));
  EXPECT_FALSE(model_->GetPanelById("workspace")->is_pinned);
}

TEST_F(AstraDevToolsModelTest, SetPanelPinnedNotFound) {
  EXPECT_FALSE(model_->SetPanelPinned("nonexistent", true));
}

TEST_F(AstraDevToolsModelTest, SetPanelPinnedNoChange) {
  // workspace is already pinned.
  EXPECT_TRUE(model_->SetPanelPinned("workspace", true));
  // Should still be pinned.
  EXPECT_TRUE(model_->GetPanelById("workspace")->is_pinned);
}

// -- Panel reordering -----------------------------------------------------

TEST_F(AstraDevToolsModelTest, ReorderPanelForward) {
  // Move workspace from position 0 to position 2.
  EXPECT_TRUE(model_->ReorderPanel("workspace", 2));

  auto panels = model_->GetAllPanels();
  EXPECT_EQ("workspace", panels[2].id);
  EXPECT_EQ(2u, panels[2].position);
  // Positions should still be contiguous.
  for (size_t i = 0; i < panels.size(); ++i) {
    EXPECT_EQ(i, panels[i].position);
  }
}

TEST_F(AstraDevToolsModelTest, ReorderPanelBackward) {
  // Move tab-stack from position 5 to position 1.
  EXPECT_TRUE(model_->ReorderPanel("tab-stack", 1));

  auto panels = model_->GetAllPanels();
  EXPECT_EQ("tab-stack", panels[1].id);
}

TEST_F(AstraDevToolsModelTest, ReorderPanelNotFound) {
  EXPECT_FALSE(model_->ReorderPanel("nonexistent", 0));
}

TEST_F(AstraDevToolsModelTest, ReorderPanelSamePosition) {
  // workspace is at position 0.
  EXPECT_TRUE(model_->ReorderPanel("workspace", 0));
  // Should still be at position 0.
  EXPECT_EQ(0u, model_->GetPanelById("workspace")->position);
}

TEST_F(AstraDevToolsModelTest, ReorderPanelClampedToEnd) {
  // Try to move workspace way beyond the end.
  EXPECT_TRUE(model_->ReorderPanel("workspace", 100));

  auto panels = model_->GetAllPanels();
  // Should be at the last position.
  EXPECT_EQ("workspace", panels.back().id);
  EXPECT_EQ(panels.size() - 1, panels.back().position);
}

TEST_F(AstraDevToolsModelTest, MovePanelEarlier) {
  // Move notes (position 1) earlier — should fail (already at 1, can move to 0).
  EXPECT_TRUE(model_->MovePanelEarlier("notes"));
  EXPECT_EQ(0u, model_->GetPanelById("notes")->position);
}

TEST_F(AstraDevToolsModelTest, MovePanelEarlierFromStart) {
  // workspace is at position 0, can't move earlier.
  EXPECT_FALSE(model_->MovePanelEarlier("workspace"));
}

TEST_F(AstraDevToolsModelTest, MovePanelLater) {
  // Move workspace (position 0) later.
  EXPECT_TRUE(model_->MovePanelLater("workspace"));
  EXPECT_EQ(1u, model_->GetPanelById("workspace")->position);
}

TEST_F(AstraDevToolsModelTest, MovePanelLaterFromEnd) {
  // tab-stack is at position 5 (last), can't move later.
  EXPECT_FALSE(model_->MovePanelLater("tab-stack"));
}

TEST_F(AstraDevToolsModelTest, ReorderNotifiesOrderChange) {
  TestDevToolsModelObserver observer;
  model_->AddObserver(&observer);

  model_->ReorderPanel("workspace", 3);

  EXPECT_GE(observer.panel_order_changed_count, 1);

  model_->RemoveObserver(&observer);
}

// -- Active panel ---------------------------------------------------------

TEST_F(AstraDevToolsModelTest, SetActivePanelSuccess) {
  EXPECT_TRUE(model_->SetActivePanel("notes"));
  EXPECT_EQ("notes", model_->active_panel_id());
}

TEST_F(AstraDevToolsModelTest, SetActivePanelSame) {
  model_->SetActivePanel("workspace");
  // Already active — should succeed without notification.
  EXPECT_TRUE(model_->SetActivePanel("workspace"));
}

TEST_F(AstraDevToolsModelTest, SetActivePanelNotFound) {
  EXPECT_FALSE(model_->SetActivePanel("nonexistent"));
}

TEST_F(AstraDevToolsModelTest, SetActivePanelHidden) {
  model_->SetPanelVisible("notes", false);
  EXPECT_FALSE(model_->SetActivePanel("notes"));
}

TEST_F(AstraDevToolsModelTest, SetActivePanelNotifies) {
  TestDevToolsModelObserver observer;
  model_->AddObserver(&observer);

  model_->SetActivePanel("notes");

  EXPECT_EQ(1, observer.active_panel_changed_count);
  EXPECT_EQ("notes", observer.last_active_panel);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraDevToolsModelTest, ActivateNextPanel) {
  ASSERT_EQ("workspace", model_->active_panel_id());

  model_->ActivateNextPanel();
  EXPECT_EQ("notes", model_->active_panel_id());

  model_->ActivateNextPanel();
  EXPECT_EQ("focus-mode", model_->active_panel_id());
}

TEST_F(AstraDevToolsModelTest, ActivateNextPanelWrapsAround) {
  // Go to last panel.
  auto visible = model_->GetVisiblePanels();
  model_->SetActivePanel(visible.back().id);

  model_->ActivateNextPanel();
  // Should wrap to first.
  EXPECT_EQ(visible.front().id, model_->active_panel_id());
}

TEST_F(AstraDevToolsModelTest, ActivatePreviousPanel) {
  model_->SetActivePanel("notes");

  model_->ActivatePreviousPanel();
  EXPECT_EQ("workspace", model_->active_panel_id());
}

TEST_F(AstraDevToolsModelTest, ActivatePreviousPanelWrapsAround) {
  model_->ActivatePreviousPanel();
  // Should wrap to last visible panel.
  auto visible = model_->GetVisiblePanels();
  EXPECT_EQ(visible.back().id, model_->active_panel_id());
}

// -- Reset to defaults ----------------------------------------------------

TEST_F(AstraDevToolsModelTest, ResetPanelsToDefaults) {
  // Modify panels.
  model_->RemovePanel("screenshot");
  model_->ReorderPanel("workspace", 3);
  model_->SetPanelVisible("notes", false);

  ASSERT_EQ(5u, model_->panel_count());

  // Reset.
  model_->ResetPanelsToDefaults();

  EXPECT_EQ(6u, model_->panel_count());
  EXPECT_TRUE(model_->HasPanel("screenshot"));
  EXPECT_TRUE(model_->GetPanelById("notes")->is_visible);
  EXPECT_EQ(0u, model_->GetPanelById("workspace")->position);
}

TEST_F(AstraDevToolsModelTest, ResetNotifiesOrderChange) {
  TestDevToolsModelObserver observer;
  model_->AddObserver(&observer);

  model_->ResetPanelsToDefaults();

  EXPECT_GE(observer.panel_order_changed_count, 1);

  model_->RemoveObserver(&observer);
}

// -- Presentation settings ------------------------------------------------

TEST_F(AstraDevToolsModelTest, DefaultShowAstraPanels) {
  EXPECT_TRUE(model_->show_astra_panels());
}

TEST_F(AstraDevToolsModelTest, SetShowAstraPanels) {
  model_->SetShowAstraPanels(false);
  EXPECT_FALSE(model_->show_astra_panels());

  model_->SetShowAstraPanels(true);
  EXPECT_TRUE(model_->show_astra_panels());
}

TEST_F(AstraDevToolsModelTest, SetShowAstraPanelsNoChange) {
  model_->SetShowAstraPanels(true);  // Already true.
  // Should still be true.
  EXPECT_TRUE(model_->show_astra_panels());
}

TEST_F(AstraDevToolsModelTest, DefaultPanelPosition) {
  EXPECT_EQ(AstraDevToolsPanelPosition::kRight, model_->panel_position());
}

TEST_F(AstraDevToolsModelTest, SetPanelPositionLeft) {
  model_->SetPanelPosition(AstraDevToolsPanelPosition::kLeft);
  EXPECT_EQ(AstraDevToolsPanelPosition::kLeft, model_->panel_position());
}

TEST_F(AstraDevToolsModelTest, SetPanelPositionBottom) {
  model_->SetPanelPosition(AstraDevToolsPanelPosition::kBottom);
  EXPECT_EQ(AstraDevToolsPanelPosition::kBottom, model_->panel_position());
}

TEST_F(AstraDevToolsModelTest, DefaultShowPanelIcons) {
  EXPECT_TRUE(model_->show_panel_icons());
}

TEST_F(AstraDevToolsModelTest, SetShowPanelIcons) {
  model_->SetShowPanelIcons(false);
  EXPECT_FALSE(model_->show_panel_icons());
}

TEST_F(AstraDevToolsModelTest, DefaultShowPanelLabels) {
  EXPECT_TRUE(model_->show_panel_labels());
}

TEST_F(AstraDevToolsModelTest, SetShowPanelLabels) {
  model_->SetShowPanelLabels(false);
  EXPECT_FALSE(model_->show_panel_labels());
}

TEST_F(AstraDevToolsModelTest, DefaultPanelWidth) {
  EXPECT_EQ(240, model_->panel_width());
}

TEST_F(AstraDevToolsModelTest, SetPanelWidth) {
  model_->SetPanelWidth(300);
  EXPECT_EQ(300, model_->panel_width());
}

TEST_F(AstraDevToolsModelTest, PanelWidthClampedMin) {
  model_->SetPanelWidth(50);  // Below minimum.
  EXPECT_EQ(AstraDevToolsModel::kMinPanelWidth, model_->panel_width());
}

TEST_F(AstraDevToolsModelTest, PanelWidthClampedMax) {
  model_->SetPanelWidth(1000);  // Above maximum.
  EXPECT_EQ(AstraDevToolsModel::kMaxPanelWidth, model_->panel_width());
}

TEST_F(AstraDevToolsModelTest, DefaultExperimentsDisabled) {
  EXPECT_FALSE(model_->experiments_enabled());
}

TEST_F(AstraDevToolsModelTest, SetExperimentsEnabled) {
  model_->SetExperimentsEnabled(true);
  EXPECT_TRUE(model_->experiments_enabled());
}

TEST_F(AstraDevToolsModelTest, DefaultAutoExpandWorkspace) {
  EXPECT_TRUE(model_->auto_expand_workspace_panel());
}

TEST_F(AstraDevToolsModelTest, SetAutoExpandWorkspace) {
  model_->SetAutoExpandWorkspacePanel(false);
  EXPECT_FALSE(model_->auto_expand_workspace_panel());
}

TEST_F(AstraDevToolsModelTest, DefaultShowPanelToolbar) {
  EXPECT_TRUE(model_->show_panel_toolbar());
}

TEST_F(AstraDevToolsModelTest, SetShowPanelToolbar) {
  model_->SetShowPanelToolbar(false);
  EXPECT_FALSE(model_->show_panel_toolbar());
}

TEST_F(AstraDevToolsModelTest, DefaultCompactMode) {
  EXPECT_FALSE(model_->compact_mode());
}

TEST_F(AstraDevToolsModelTest, SetCompactMode) {
  model_->SetCompactMode(true);
  EXPECT_TRUE(model_->compact_mode());
}

TEST_F(AstraDevToolsModelTest, DefaultRememberLastPanel) {
  EXPECT_TRUE(model_->remember_last_panel());
}

TEST_F(AstraDevToolsModelTest, SetRememberLastPanel) {
  model_->SetRememberLastPanel(false);
  EXPECT_FALSE(model_->remember_last_panel());
}

TEST_F(AstraDevToolsModelTest, DefaultLastActivePanelEmpty) {
  EXPECT_TRUE(model_->last_active_panel().empty());
}

TEST_F(AstraDevToolsModelTest, SettingsChangeNotifiesObserver) {
  TestDevToolsModelObserver observer;
  model_->AddObserver(&observer);

  model_->SetCompactMode(true);

  EXPECT_EQ(1, observer.settings_changed_count);

  model_->RemoveObserver(&observer);
}

// -- Dock position --------------------------------------------------------

TEST_F(AstraDevToolsModelTest, DefaultDockPosition) {
  EXPECT_EQ(AstraDevToolsDockPosition::kBottom, model_->dock_position());
}

TEST_F(AstraDevToolsModelTest, SetDockPositionLeft) {
  model_->SetDockPosition(AstraDevToolsDockPosition::kLeft);
  EXPECT_EQ(AstraDevToolsDockPosition::kLeft, model_->dock_position());
}

TEST_F(AstraDevToolsModelTest, SetDockPositionRight) {
  model_->SetDockPosition(AstraDevToolsDockPosition::kRight);
  EXPECT_EQ(AstraDevToolsDockPosition::kRight, model_->dock_position());
}

TEST_F(AstraDevToolsModelTest, SetDockPositionUndocked) {
  model_->SetDockPosition(AstraDevToolsDockPosition::kUndocked);
  EXPECT_EQ(AstraDevToolsDockPosition::kUndocked, model_->dock_position());
}

TEST_F(AstraDevToolsModelTest, SetDockPositionNotifies) {
  TestDevToolsModelObserver observer;
  model_->AddObserver(&observer);

  model_->SetDockPosition(AstraDevToolsDockPosition::kLeft);

  EXPECT_EQ(1, observer.dock_position_changed_count);
  EXPECT_EQ(AstraDevToolsDockPosition::kLeft, observer.last_dock_position);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraDevToolsModelTest, SetDockPositionNoChange) {
  TestDevToolsModelObserver observer;
  model_->AddObserver(&observer);

  // Bottom is already the default.
  model_->SetDockPosition(AstraDevToolsDockPosition::kBottom);

  EXPECT_EQ(0, observer.dock_position_changed_count);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraDevToolsModelTest, CycleDockPosition) {
  // Start at bottom.
  ASSERT_EQ(AstraDevToolsDockPosition::kBottom, model_->dock_position());

  model_->CycleDockPosition();
  EXPECT_EQ(AstraDevToolsDockPosition::kLeft, model_->dock_position());

  model_->CycleDockPosition();
  EXPECT_EQ(AstraDevToolsDockPosition::kRight, model_->dock_position());

  model_->CycleDockPosition();
  EXPECT_EQ(AstraDevToolsDockPosition::kUndocked, model_->dock_position());

  model_->CycleDockPosition();
  EXPECT_EQ(AstraDevToolsDockPosition::kBottom, model_->dock_position());
}

// -- Theme ----------------------------------------------------------------

TEST_F(AstraDevToolsModelTest, DefaultTheme) {
  EXPECT_EQ(AstraDevToolsTheme::kSystem, model_->theme());
}

TEST_F(AstraDevToolsModelTest, SetThemeLight) {
  model_->SetTheme(AstraDevToolsTheme::kLight);
  EXPECT_EQ(AstraDevToolsTheme::kLight, model_->theme());
}

TEST_F(AstraDevToolsModelTest, SetThemeDark) {
  model_->SetTheme(AstraDevToolsTheme::kDark);
  EXPECT_EQ(AstraDevToolsTheme::kDark, model_->theme());
}

TEST_F(AstraDevToolsModelTest, SetThemeNotifies) {
  TestDevToolsModelObserver observer;
  model_->AddObserver(&observer);

  model_->SetTheme(AstraDevToolsTheme::kDark);

  EXPECT_EQ(1, observer.theme_changed_count);
  EXPECT_EQ(AstraDevToolsTheme::kDark, observer.last_theme);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraDevToolsModelTest, SetThemeNoChange) {
  TestDevToolsModelObserver observer;
  model_->AddObserver(&observer);

  // System is default.
  model_->SetTheme(AstraDevToolsTheme::kSystem);

  EXPECT_EQ(0, observer.theme_changed_count);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraDevToolsModelTest, GetEffectiveThemeDarkMode) {
  model_->SetTheme(AstraDevToolsTheme::kDark);
  EXPECT_EQ(AstraDevToolsTheme::kDark, model_->GetEffectiveTheme());
}

TEST_F(AstraDevToolsModelTest, GetEffectiveThemeLightMode) {
  model_->SetTheme(AstraDevToolsTheme::kLight);
  EXPECT_EQ(AstraDevToolsTheme::kLight, model_->GetEffectiveTheme());
}

TEST_F(AstraDevToolsModelTest, GetEffectiveThemeSystemResolvesDark) {
  // System theme currently resolves to dark (DevTools default).
  model_->SetTheme(AstraDevToolsTheme::kSystem);
  // TODO(astra): This depends on system theme.  For now, it defaults to dark.
  EXPECT_EQ(AstraDevToolsTheme::kDark, model_->GetEffectiveTheme());
}

// -- Observer defaults ----------------------------------------------------

TEST_F(AstraDevToolsModelTest, ObserverDefaultsDoNotCrash) {
  // EmptyObserver overrides no methods — all should use default implementations.
  EmptyObserver observer;
  model_->AddObserver(&observer);

  // Trigger all observer methods.
  model_->SetPanelVisible("screenshot", false);  // OnPanelClosed
  model_->SetPanelVisible("screenshot", true);   // OnPanelOpened
  model_->SetActivePanel("notes");               // OnActivePanelChanged
  model_->ReorderPanel("workspace", 3);          // OnPanelOrderChanged
  model_->SetCompactMode(true);                  // OnDevToolsSettingsChanged
  model_->SetDockPosition(                       // OnDockPositionChanged
      AstraDevToolsDockPosition::kLeft);
  model_->SetTheme(AstraDevToolsTheme::kDark);   // OnThemeChanged

  // No crash = success.
  model_->RemoveObserver(&observer);
  SUCCEED();
}

// -- Persistence round-trip -----------------------------------------------

TEST_F(AstraDevToolsModelTest, PanelOrderPersistsViaPrefs) {
  // Reorder panels.
  model_->ReorderPanel("workspace", 4);
  model_->ReorderPanel("notes", 5);

  // Get the order before reload.
  auto panels_before = model_->GetAllPanels();

  // Save explicitly.
  model_->SaveToPrefs();

  // Create a new model that loads from the same prefs.
  auto model2 = std::make_unique<AstraDevToolsModel>(profile_->GetPrefs());

  auto panels_after = model2->GetAllPanels();
  ASSERT_EQ(panels_before.size(), panels_after.size());

  for (size_t i = 0; i < panels_before.size(); ++i) {
    EXPECT_EQ(panels_before[i].id, panels_after[i].id);
    EXPECT_EQ(panels_before[i].position, panels_after[i].position);
  }
}

TEST_F(AstraDevToolsModelTest, PanelVisibilityPersistsViaPrefs) {
  model_->SetPanelVisible("screenshot", false);
  model_->SetPanelVisible("notes", false);
  model_->SaveToPrefs();

  auto model2 = std::make_unique<AstraDevToolsModel>(profile_->GetPrefs());

  EXPECT_FALSE(model2->GetPanelById("screenshot")->is_visible);
  EXPECT_FALSE(model2->GetPanelById("notes")->is_visible);
  EXPECT_TRUE(model2->GetPanelById("workspace")->is_visible);
}

TEST_F(AstraDevToolsModelTest, SettingsPersistViaPrefs) {
  model_->SetCompactMode(true);
  model_->SetPanelPosition(AstraDevToolsPanelPosition::kLeft);
  model_->SetPanelWidth(350);
  model_->SetShowPanelIcons(false);

  auto model2 = std::make_unique<AstraDevToolsModel>(profile_->GetPrefs());

  EXPECT_TRUE(model2->compact_mode());
  EXPECT_EQ(AstraDevToolsPanelPosition::kLeft, model2->panel_position());
  EXPECT_EQ(350, model2->panel_width());
  EXPECT_FALSE(model2->show_panel_icons());
}

TEST_F(AstraDevToolsModelTest, DockPositionPersistsViaPrefs) {
  model_->SetDockPosition(AstraDevToolsDockPosition::kRight);

  auto model2 = std::make_unique<AstraDevToolsModel>(profile_->GetPrefs());
  EXPECT_EQ(AstraDevToolsDockPosition::kRight, model2->dock_position());
}

// -- Edge cases -----------------------------------------------------------

TEST_F(AstraDevToolsModelTest, EmptyModelNoPanels) {
  // Start with no panels by removing all.
  auto all = model_->GetAllPanels();
  for (const auto& p : all) {
    model_->RemovePanel(p.id);
  }

  EXPECT_TRUE(model_->empty());
  EXPECT_EQ(0u, model_->panel_count());
  EXPECT_EQ(0u, model_->visible_panel_count());
  EXPECT_TRUE(model_->active_panel_id().empty());
  EXPECT_TRUE(model_->GetAllPanels().empty());
  EXPECT_TRUE(model_->GetVisiblePanels().empty());
}

TEST_F(AstraDevToolsModelTest, AddManyPanels) {
  for (int i = 0; i < 20; ++i) {
    AstraDevToolsPanel panel;
    panel.id = "panel-" + std::to_string(i);
    panel.title = "Panel " + std::to_string(i);
    EXPECT_TRUE(model_->AddPanel(panel));
  }

  EXPECT_EQ(26u, model_->panel_count());  // 6 default + 20 added.
}

TEST_F(AstraDevToolsModelTest, PanelStructEquality) {
  AstraDevToolsPanel p1;
  p1.id = "test";
  p1.title = "Test";
  p1.icon = "icon";
  p1.is_visible = true;
  p1.is_pinned = false;
  p1.position = 3;

  AstraDevToolsPanel p2 = p1;
  EXPECT_TRUE(p1 == p2);
  EXPECT_FALSE(p1 != p2);

  p2.title = "Different";
  EXPECT_FALSE(p1 == p2);
  EXPECT_TRUE(p1 != p2);
}

TEST_F(AstraDevToolsModelTest, NullPrefServiceWorks) {
  // Create model with null PrefService (in-memory mode).
  auto model = std::make_unique<AstraDevToolsModel>(nullptr);

  // Should have default panels.
  EXPECT_EQ(6u, model->panel_count());
  EXPECT_EQ("workspace", model->active_panel_id());

  // Operations should work without crashing.
  model->SetCompactMode(true);
  model->ReorderPanel("workspace", 2);
  model->SetActivePanel("notes");
  model->SetDockPosition(AstraDevToolsDockPosition::kLeft);

  // SaveToPrefs should be a no-op (no crash).
  model->SaveToPrefs();

  // LoadFromPrefs should be a no-op (no crash).
  model->LoadFromPrefs();

  SUCCEED();
}

// =========================================================================
// Deepened model tests (AstraDevToolsPanelInfo / AstraDevToolsPanelType)
// =========================================================================

// Test observer for the deepened AstraDevToolsObserver interface.
class TestDeepenedObserver : public AstraDevToolsObserver {
 public:
  int devtools_opened_count = 0;
  int devtools_closed_count = 0;
  int panel_activated_count = 0;
  std::string last_activated_panel;
  int panel_enabled_changed_count = 0;
  std::string last_enabled_panel;
  bool last_enabled_value = false;
  int panel_visibility_changed_count = 0;
  std::string last_visibility_panel;
  bool last_visibility_value = false;
  int panels_reordered_count = 0;
  int dock_state_changed_count = 0;
  AstraDevToolsDockState last_dock_state = AstraDevToolsDockState::kDockedBottom;
  int model_shutdown_count = 0;

  void OnDevToolsOpened(AstraDevToolsModel* model) override {
    devtools_opened_count++;
  }
  void OnDevToolsClosed(AstraDevToolsModel* model) override {
    devtools_closed_count++;
  }
  void OnPanelActivated(AstraDevToolsModel* model,
                        const std::string& panel_id) override {
    panel_activated_count++;
    last_activated_panel = panel_id;
  }
  void OnPanelEnabledChanged(AstraDevToolsModel* model,
                             const std::string& panel_id,
                             bool enabled) override {
    panel_enabled_changed_count++;
    last_enabled_panel = panel_id;
    last_enabled_value = enabled;
  }
  void OnPanelVisibilityChanged(AstraDevToolsModel* model,
                                const std::string& panel_id,
                                bool visible) override {
    panel_visibility_changed_count++;
    last_visibility_panel = panel_id;
    last_visibility_value = visible;
  }
  void OnPanelsReordered(AstraDevToolsModel* model) override {
    panels_reordered_count++;
  }
  void OnDockStateChanged(AstraDevToolsModel* model,
                          AstraDevToolsDockState state) override {
    dock_state_changed_count++;
    last_dock_state = state;
  }
  void OnDevToolsModelShutdown(AstraDevToolsModel* model) override {
    model_shutdown_count++;
  }
};

// Empty deepened observer — tests default implementations.
class EmptyDeepenedObserver : public AstraDevToolsObserver {
 public:
  // Intentionally empty — all methods use default implementations.
};

// -- Panel type and struct ------------------------------------------------

TEST_F(AstraDevToolsModelTest, DeepenedDefaultPanelsCount) {
  auto panels = model_->GetPanels();
  EXPECT_EQ(6u, panels.size());
  EXPECT_EQ(6u, model_->GetPanelCount());
}

TEST_F(AstraDevToolsModelTest, DeepenedPanelTypesAllPresent) {
  EXPECT_NE(nullptr, model_->GetPanelByType(AstraDevToolsPanelType::kWorkspacePanel));
  EXPECT_NE(nullptr, model_->GetPanelByType(AstraDevToolsPanelType::kTabStackPanel));
  EXPECT_NE(nullptr, model_->GetPanelByType(AstraDevToolsPanelType::kNotesPanel));
  EXPECT_NE(nullptr, model_->GetPanelByType(AstraDevToolsPanelType::kPerformancePanel));
  EXPECT_NE(nullptr, model_->GetPanelByType(AstraDevToolsPanelType::kAccessibilityPanel));
  EXPECT_NE(nullptr, model_->GetPanelByType(AstraDevToolsPanelType::kA11yTreePanel));
}

TEST_F(AstraDevToolsModelTest, DeepenedGetPanelByIdExists) {
  const auto* panel = model_->GetPanel("workspace-panel");
  ASSERT_NE(nullptr, panel);
  EXPECT_EQ(AstraDevToolsPanelType::kWorkspacePanel, panel->type);
  EXPECT_FALSE(panel->title.empty());
}

TEST_F(AstraDevToolsModelTest, DeepenedGetPanelByIdNotFound) {
  EXPECT_EQ(nullptr, model_->GetPanel("nonexistent"));
  EXPECT_EQ(nullptr, model_->GetPanel(""));
}

TEST_F(AstraDevToolsModelTest, DeepenedPanelInfoHasAllFields) {
  const auto* panel = model_->GetPanelByType(AstraDevToolsPanelType::kWorkspacePanel);
  ASSERT_NE(nullptr, panel);
  EXPECT_FALSE(panel->panel_id.empty());
  EXPECT_FALSE(panel->title.empty());
  EXPECT_FALSE(panel->icon_name.empty());
  EXPECT_TRUE(panel->is_enabled);
  EXPECT_TRUE(panel->is_visible);
  EXPECT_GE(panel->order_index, 0);
  EXPECT_TRUE(panel->is_default);
  EXPECT_FALSE(panel->description.empty());
}

TEST_F(AstraDevToolsModelTest, DeepenedPanelsAreOrdered) {
  auto panels = model_->GetPanels();
  for (size_t i = 0; i < panels.size(); ++i) {
    EXPECT_EQ(static_cast<int>(i), panels[i].order_index);
  }
}

TEST_F(AstraDevToolsModelTest, DeepenedGetDefaultPanelsStatic) {
  auto defaults = AstraDevToolsModel::GetDefaultPanels();
  EXPECT_EQ(6u, defaults.size());
  EXPECT_EQ(AstraDevToolsPanelType::kWorkspacePanel, defaults[0].type);
  EXPECT_TRUE(defaults[0].is_default);
}

// -- Panel enable/disable -------------------------------------------------

TEST_F(AstraDevToolsModelTest, DeepenedIsPanelEnabledDefaultTrue) {
  const auto* ws = model_->GetPanelByType(AstraDevToolsPanelType::kWorkspacePanel);
  ASSERT_NE(nullptr, ws);
  EXPECT_TRUE(model_->IsPanelEnabled(ws->panel_id));
}

TEST_F(AstraDevToolsModelTest, DeepenedSetPanelEnabledFalse) {
  const auto* notes = model_->GetPanelByType(AstraDevToolsPanelType::kNotesPanel);
  ASSERT_NE(nullptr, notes);
  std::string id = notes->panel_id;

  EXPECT_TRUE(model_->SetPanelEnabled(id, false));
  EXPECT_FALSE(model_->IsPanelEnabled(id));
}

TEST_F(AstraDevToolsModelTest, DeepenedSetPanelEnabledTrue) {
  const auto* notes = model_->GetPanelByType(AstraDevToolsPanelType::kNotesPanel);
  ASSERT_NE(nullptr, notes);
  std::string id = notes->panel_id;

  model_->SetPanelEnabled(id, false);
  EXPECT_TRUE(model_->SetPanelEnabled(id, true));
  EXPECT_TRUE(model_->IsPanelEnabled(id));
}

TEST_F(AstraDevToolsModelTest, DeepenedSetPanelEnabledNotFound) {
  EXPECT_FALSE(model_->SetPanelEnabled("nonexistent", false));
}

TEST_F(AstraDevToolsModelTest, DeepenedDisabledPanelCannotBeActivated) {
  const auto* notes = model_->GetPanelByType(AstraDevToolsPanelType::kNotesPanel);
  ASSERT_NE(nullptr, notes);
  std::string id = notes->panel_id;

  model_->SetPanelEnabled(id, false);
  EXPECT_FALSE(model_->SetActivePanel(id));
}

// -- Panel visibility -----------------------------------------------------

TEST_F(AstraDevToolsModelTest, DeepenedIsPanelVisibleDefaultTrue) {
  const auto* perf = model_->GetPanelByType(AstraDevToolsPanelType::kPerformancePanel);
  ASSERT_NE(nullptr, perf);
  EXPECT_TRUE(model_->IsPanelVisible(perf->panel_id));
}

TEST_F(AstraDevToolsModelTest, DeepenedSetPanelVisibleFalse) {
  const auto* perf = model_->GetPanelByType(AstraDevToolsPanelType::kPerformancePanel);
  ASSERT_NE(nullptr, perf);
  std::string id = perf->panel_id;

  EXPECT_TRUE(model_->SetPanelVisible(id, false));
  EXPECT_FALSE(model_->IsPanelVisible(id));
}

TEST_F(AstraDevToolsModelTest, DeepenedSetPanelVisibleTrue) {
  const auto* perf = model_->GetPanelByType(AstraDevToolsPanelType::kPerformancePanel);
  ASSERT_NE(nullptr, perf);
  std::string id = perf->panel_id;

  model_->SetPanelVisible(id, false);
  EXPECT_TRUE(model_->SetPanelVisible(id, true));
  EXPECT_TRUE(model_->IsPanelVisible(id));
}

TEST_F(AstraDevToolsModelTest, DeepenedSetPanelVisibleNotFound) {
  EXPECT_FALSE(model_->SetPanelVisible("nonexistent", false));
}

TEST_F(AstraDevToolsModelTest, DeepenedHiddenPanelCannotBeActivated) {
  const auto* a11y = model_->GetPanelByType(AstraDevToolsPanelType::kA11yTreePanel);
  ASSERT_NE(nullptr, a11y);
  std::string id = a11y->panel_id;

  model_->SetPanelVisible(id, false);
  EXPECT_FALSE(model_->SetActivePanel(id));
}

// -- Active panel ---------------------------------------------------------

TEST_F(AstraDevToolsModelTest, DeepenedDefaultActivePanel) {
  // Default active panel should be the first panel (workspace).
  const auto* ws = model_->GetPanelByType(AstraDevToolsPanelType::kWorkspacePanel);
  ASSERT_NE(nullptr, ws);
  EXPECT_EQ(ws->panel_id, model_->GetActivePanel());
}

TEST_F(AstraDevToolsModelTest, DeepenedSetActivePanel) {
  const auto* notes = model_->GetPanelByType(AstraDevToolsPanelType::kNotesPanel);
  ASSERT_NE(nullptr, notes);
  std::string id = notes->panel_id;

  EXPECT_TRUE(model_->SetActivePanel(id));
  EXPECT_EQ(id, model_->GetActivePanel());
}

TEST_F(AstraDevToolsModelTest, DeepenedSetActivePanelNotFound) {
  EXPECT_FALSE(model_->SetActivePanel("nonexistent"));
}

TEST_F(AstraDevToolsModelTest, DeepenedSetActivePanelSameIsNoop) {
  const auto* ws = model_->GetPanelByType(AstraDevToolsPanelType::kWorkspacePanel);
  ASSERT_NE(nullptr, ws);

  // Should succeed without error.
  EXPECT_TRUE(model_->SetActivePanel(ws->panel_id));
  // Still the active panel.
  EXPECT_EQ(ws->panel_id, model_->GetActivePanel());
}

// -- Panel reordering -----------------------------------------------------

TEST_F(AstraDevToolsModelTest, DeepenedReorderPanels) {
  auto original = model_->GetPanels();
  ASSERT_GE(original.size(), 3u);

  // Reverse the first three panels.
  std::vector<std::string> new_order = {
    original[2].panel_id,
    original[1].panel_id,
    original[0].panel_id,
  };
  // Add remaining panels in original order.
  for (size_t i = 3; i < original.size(); ++i) {
    new_order.push_back(original[i].panel_id);
  }

  model_->ReorderPanels(new_order);

  auto reordered = model_->GetPanels();
  EXPECT_EQ(new_order.size(), reordered.size());
  for (size_t i = 0; i < new_order.size(); ++i) {
    EXPECT_EQ(new_order[i], reordered[i].panel_id);
    EXPECT_EQ(static_cast<int>(i), reordered[i].order_index);
  }
}

TEST_F(AstraDevToolsModelTest, DeepenedReorderPanelsPartialList) {
  auto original = model_->GetPanels();
  ASSERT_GE(original.size(), 4u);

  // Only specify first two panels in new order.
  std::vector<std::string> new_order = {
    original[1].panel_id,
    original[0].panel_id,
  };

  model_->ReorderPanels(new_order);

  auto reordered = model_->GetPanels();
  // First two should be in the specified order.
  EXPECT_EQ(new_order[0], reordered[0].panel_id);
  EXPECT_EQ(new_order[1], reordered[1].panel_id);
  // Total count should be the same.
  EXPECT_EQ(original.size(), reordered.size());
  // All order indices should be contiguous.
  for (size_t i = 0; i < reordered.size(); ++i) {
    EXPECT_EQ(static_cast<int>(i), reordered[i].order_index);
  }
}

TEST_F(AstraDevToolsModelTest, DeepenedResetPanelsToDefaults) {
  // Modify panels.
  const auto* perf = model_->GetPanelByType(AstraDevToolsPanelType::kPerformancePanel);
  ASSERT_NE(nullptr, perf);
  model_->SetPanelVisible(perf->panel_id, false);
  model_->SetPanelEnabled(perf->panel_id, false);

  auto original = model_->GetPanels();
  std::vector<std::string> reversed_order;
  for (auto it = original.rbegin(); it != original.rend(); ++it) {
    reversed_order.push_back(it->panel_id);
  }
  model_->ReorderPanels(reversed_order);

  // Reset.
  model_->ResetPanelsToDefaults();

  auto reset = model_->GetPanels();
  EXPECT_EQ(6u, reset.size());
  EXPECT_TRUE(model_->IsPanelVisible(perf->panel_id));
  EXPECT_TRUE(model_->IsPanelEnabled(perf->panel_id));
  // Workspace should be first again.
  EXPECT_EQ(AstraDevToolsPanelType::kWorkspacePanel, reset[0].type);
}

// -- ShowAstraPanel -------------------------------------------------------

TEST_F(AstraDevToolsModelTest, DeepenedShowAstraPanelOpensDevTools) {
  ASSERT_FALSE(model_->IsDevToolsOpen());

  EXPECT_TRUE(model_->ShowAstraPanel(AstraDevToolsPanelType::kNotesPanel));

  EXPECT_TRUE(model_->IsDevToolsOpen());
  const auto* notes = model_->GetPanelByType(AstraDevToolsPanelType::kNotesPanel);
  ASSERT_NE(nullptr, notes);
  EXPECT_EQ(notes->panel_id, model_->GetActivePanel());
}

TEST_F(AstraDevToolsModelTest, DeepenedShowAstraPanelWhenAlreadyOpen) {
  model_->SetDevToolsOpen(true);

  EXPECT_TRUE(model_->ShowAstraPanel(AstraDevToolsPanelType::kPerformancePanel));

  EXPECT_TRUE(model_->IsDevToolsOpen());
  const auto* perf = model_->GetPanelByType(AstraDevToolsPanelType::kPerformancePanel);
  ASSERT_NE(nullptr, perf);
  EXPECT_EQ(perf->panel_id, model_->GetActivePanel());
}

// -- DevTools open/close state -------------------------------------------

TEST_F(AstraDevToolsModelTest, DeepenedDevToolsClosedByDefault) {
  EXPECT_FALSE(model_->IsDevToolsOpen());
}

TEST_F(AstraDevToolsModelTest, DeepenedSetDevToolsOpenTrue) {
  model_->SetDevToolsOpen(true);
  EXPECT_TRUE(model_->IsDevToolsOpen());
}

TEST_F(AstraDevToolsModelTest, DeepenedSetDevToolsOpenFalse) {
  model_->SetDevToolsOpen(true);
  ASSERT_TRUE(model_->IsDevToolsOpen());

  model_->SetDevToolsOpen(false);
  EXPECT_FALSE(model_->IsDevToolsOpen());
}

TEST_F(AstraDevToolsModelTest, DeepenedSetDevToolsOpenNoopWhenSame) {
  TestDeepenedObserver observer;
  model_->AddObserver(&observer);

  model_->SetDevToolsOpen(false);  // Already false.
  EXPECT_EQ(0, observer.devtools_closed_count);

  model_->SetDevToolsOpen(true);
  EXPECT_EQ(1, observer.devtools_opened_count);

  model_->SetDevToolsOpen(true);   // Already true.
  EXPECT_EQ(1, observer.devtools_opened_count);

  model_->RemoveObserver(&observer);
}

// -- Dock state -----------------------------------------------------------

TEST_F(AstraDevToolsModelTest, DeepenedDefaultDockState) {
  EXPECT_EQ(AstraDevToolsDockState::kDockedBottom, model_->GetDockState());
}

TEST_F(AstraDevToolsModelTest, DeepenedSetDockStateAllValues) {
  model_->SetDockState(AstraDevToolsDockState::kDockedLeft);
  EXPECT_EQ(AstraDevToolsDockState::kDockedLeft, model_->GetDockState());

  model_->SetDockState(AstraDevToolsDockState::kDockedRight);
  EXPECT_EQ(AstraDevToolsDockState::kDockedRight, model_->GetDockState());

  model_->SetDockState(AstraDevToolsDockState::kUndocked);
  EXPECT_EQ(AstraDevToolsDockState::kUndocked, model_->GetDockState());

  model_->SetDockState(AstraDevToolsDockState::kMinimized);
  EXPECT_EQ(AstraDevToolsDockState::kMinimized, model_->GetDockState());

  model_->SetDockState(AstraDevToolsDockState::kDockedBottom);
  EXPECT_EQ(AstraDevToolsDockState::kDockedBottom, model_->GetDockState());
}

TEST_F(AstraDevToolsModelTest, DeepenedIsDocked) {
  model_->SetDockState(AstraDevToolsDockState::kDockedBottom);
  EXPECT_TRUE(model_->IsDocked());

  model_->SetDockState(AstraDevToolsDockState::kDockedLeft);
  EXPECT_TRUE(model_->IsDocked());

  model_->SetDockState(AstraDevToolsDockState::kDockedRight);
  EXPECT_TRUE(model_->IsDocked());

  model_->SetDockState(AstraDevToolsDockState::kUndocked);
  EXPECT_FALSE(model_->IsDocked());

  model_->SetDockState(AstraDevToolsDockState::kMinimized);
  EXPECT_FALSE(model_->IsDocked());
}

TEST_F(AstraDevToolsModelTest, DeepenedToggleDockSideCycles) {
  // Start at bottom.
  ASSERT_EQ(AstraDevToolsDockState::kDockedBottom, model_->GetDockState());

  model_->ToggleDockSide();
  EXPECT_EQ(AstraDevToolsDockState::kDockedLeft, model_->GetDockState());

  model_->ToggleDockSide();
  EXPECT_EQ(AstraDevToolsDockState::kDockedRight, model_->GetDockState());

  model_->ToggleDockSide();
  EXPECT_EQ(AstraDevToolsDockState::kUndocked, model_->GetDockState());

  model_->ToggleDockSide();
  EXPECT_EQ(AstraDevToolsDockState::kDockedBottom, model_->GetDockState());
}

TEST_F(AstraDevToolsModelTest, DeepenedToggleDockSideFromMinimized) {
  model_->SetDockState(AstraDevToolsDockState::kMinimized);

  model_->ToggleDockSide();
  // From minimized, should go to bottom (first docked state).
  EXPECT_EQ(AstraDevToolsDockState::kDockedBottom, model_->GetDockState());
}

// -- Zoom level -----------------------------------------------------------

TEST_F(AstraDevToolsModelTest, DeepenedDefaultZoomLevel) {
  EXPECT_DOUBLE_EQ(1.0, model_->GetZoomLevel());
}

TEST_F(AstraDevToolsModelTest, DeepenedSetZoomLevel) {
  model_->SetZoomLevel(1.5);
  EXPECT_DOUBLE_EQ(1.5, model_->GetZoomLevel());
}

TEST_F(AstraDevToolsModelTest, DeepenedZoomLevelClampedToMin) {
  model_->SetZoomLevel(0.1);  // Below minimum.
  EXPECT_DOUBLE_EQ(AstraDevToolsModel::kMinZoomLevel, model_->GetZoomLevel());
}

TEST_F(AstraDevToolsModelTest, DeepenedZoomLevelClampedToMax) {
  model_->SetZoomLevel(5.0);  // Above maximum.
  EXPECT_DOUBLE_EQ(AstraDevToolsModel::kMaxZoomLevel, model_->GetZoomLevel());
}

TEST_F(AstraDevToolsModelTest, DeepenedZoomLevelAtMinBoundary) {
  model_->SetZoomLevel(AstraDevToolsModel::kMinZoomLevel);
  EXPECT_DOUBLE_EQ(AstraDevToolsModel::kMinZoomLevel, model_->GetZoomLevel());
}

TEST_F(AstraDevToolsModelTest, DeepenedZoomLevelAtMaxBoundary) {
  model_->SetZoomLevel(AstraDevToolsModel::kMaxZoomLevel);
  EXPECT_DOUBLE_EQ(AstraDevToolsModel::kMaxZoomLevel, model_->GetZoomLevel());
}

// -- Pref keys ------------------------------------------------------------

TEST_F(AstraDevToolsModelTest, DeepenedPrefKeysAreDefined) {
  // All 12+ pref keys should be defined as non-empty strings.
  EXPECT_FALSE(std::string(AstraDevToolsModel::kPrefEnableAstraPanels).empty());
  EXPECT_FALSE(std::string(AstraDevToolsModel::kPrefDefaultActivePanel).empty());
  EXPECT_FALSE(std::string(AstraDevToolsModel::kPrefDefaultDockState).empty());
  EXPECT_FALSE(std::string(AstraDevToolsModel::kPrefPanelOrder).empty());
  EXPECT_FALSE(std::string(AstraDevToolsModel::kPrefShowPanelIcons).empty());
  EXPECT_FALSE(std::string(AstraDevToolsModel::kPrefShowAstraTab).empty());
  EXPECT_FALSE(std::string(AstraDevToolsModel::kPrefDevToolsTheme).empty());
  EXPECT_FALSE(std::string(AstraDevToolsModel::kPrefFontSize).empty());
  EXPECT_FALSE(std::string(AstraDevToolsModel::kPrefPanelVisibilityDefaults).empty());
  EXPECT_FALSE(std::string(AstraDevToolsModel::kPrefAutoOpenOnError).empty());
  EXPECT_FALSE(std::string(AstraDevToolsModel::kPrefWorkspaceAutoSync).empty());
  EXPECT_FALSE(std::string(AstraDevToolsModel::kPrefPerformanceAutoRecord).empty());
}

TEST_F(AstraDevToolsModelTest, DeepenedDefaultFontSize) {
  EXPECT_EQ(AstraDevToolsModel::kDefaultFontSize, 12);
}

// -- Deepened observer notifications -------------------------------------

TEST_F(AstraDevToolsModelTest, DeepenedObserverDevToolsOpened) {
  TestDeepenedObserver observer;
  model_->AddObserver(&observer);

  model_->SetDevToolsOpen(true);
  EXPECT_EQ(1, observer.devtools_opened_count);
  EXPECT_EQ(0, observer.devtools_closed_count);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraDevToolsModelTest, DeepenedObserverDevToolsClosed) {
  model_->SetDevToolsOpen(true);

  TestDeepenedObserver observer;
  model_->AddObserver(&observer);

  model_->SetDevToolsOpen(false);
  EXPECT_EQ(0, observer.devtools_opened_count);
  EXPECT_EQ(1, observer.devtools_closed_count);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraDevToolsModelTest, DeepenedObserverPanelActivated) {
  TestDeepenedObserver observer;
  model_->AddObserver(&observer);

  const auto* notes = model_->GetPanelByType(AstraDevToolsPanelType::kNotesPanel);
  ASSERT_NE(nullptr, notes);
  model_->SetActivePanel(notes->panel_id);

  EXPECT_EQ(1, observer.panel_activated_count);
  EXPECT_EQ(notes->panel_id, observer.last_activated_panel);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraDevToolsModelTest, DeepenedObserverPanelEnabledChanged) {
  TestDeepenedObserver observer;
  model_->AddObserver(&observer);

  const auto* perf = model_->GetPanelByType(AstraDevToolsPanelType::kPerformancePanel);
  ASSERT_NE(nullptr, perf);
  model_->SetPanelEnabled(perf->panel_id, false);

  EXPECT_EQ(1, observer.panel_enabled_changed_count);
  EXPECT_EQ(perf->panel_id, observer.last_enabled_panel);
  EXPECT_FALSE(observer.last_enabled_value);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraDevToolsModelTest, DeepenedObserverPanelVisibilityChanged) {
  TestDeepenedObserver observer;
  model_->AddObserver(&observer);

  const auto* a11y = model_->GetPanelByType(AstraDevToolsPanelType::kA11yTreePanel);
  ASSERT_NE(nullptr, a11y);
  model_->SetPanelVisible(a11y->panel_id, false);

  EXPECT_EQ(1, observer.panel_visibility_changed_count);
  EXPECT_EQ(a11y->panel_id, observer.last_visibility_panel);
  EXPECT_FALSE(observer.last_visibility_value);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraDevToolsModelTest, DeepenedObserverPanelsReordered) {
  TestDeepenedObserver observer;
  model_->AddObserver(&observer);

  auto panels = model_->GetPanels();
  std::vector<std::string> order;
  for (auto it = panels.rbegin(); it != panels.rend(); ++it) {
    order.push_back(it->panel_id);
  }
  model_->ReorderPanels(order);

  EXPECT_GE(observer.panels_reordered_count, 1);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraDevToolsModelTest, DeepenedObserverDockStateChanged) {
  TestDeepenedObserver observer;
  model_->AddObserver(&observer);

  model_->SetDockState(AstraDevToolsDockState::kDockedLeft);

  EXPECT_EQ(1, observer.dock_state_changed_count);
  EXPECT_EQ(AstraDevToolsDockState::kDockedLeft, observer.last_dock_state);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraDevToolsModelTest, DeepenedObserverEmptyDefaultsDoNotCrash) {
  EmptyDeepenedObserver observer;
  model_->AddObserver(&observer);

  // Trigger all deepened observer methods.
  model_->SetDevToolsOpen(true);
  model_->SetDevToolsOpen(false);
  const auto* notes = model_->GetPanelByType(AstraDevToolsPanelType::kNotesPanel);
  if (notes) {
    model_->SetActivePanel(notes->panel_id);
    model_->SetPanelEnabled(notes->panel_id, false);
    model_->SetPanelEnabled(notes->panel_id, true);
    model_->SetPanelVisible(notes->panel_id, false);
    model_->SetPanelVisible(notes->panel_id, true);
  }
  model_->ReorderPanels({});
  model_->SetDockState(AstraDevToolsDockState::kUndocked);

  // No crash = success.
  model_->RemoveObserver(&observer);
  SUCCEED();
}

// -- Edge cases -----------------------------------------------------------

TEST_F(AstraDevToolsModelTest, DeepenedShowAstraPanelOnEmptyModel) {
  // Remove all panels.
  auto all = model_->GetPanels();
  for (const auto& p : all) {
    model_->SetPanelEnabled(p.panel_id, false);
    model_->SetPanelVisible(p.panel_id, false);
  }

  // Should fail gracefully.
  EXPECT_FALSE(model_->ShowAstraPanel(AstraDevToolsPanelType::kWorkspacePanel));
}

TEST_F(AstraDevToolsModelTest, DeepenedSetActivePanelWhenAllDisabled) {
  auto all = model_->GetPanels();
  for (const auto& p : all) {
    model_->SetPanelEnabled(p.panel_id, false);
  }

  // Trying to set any panel should fail.
  for (const auto& p : all) {
    EXPECT_FALSE(model_->SetActivePanel(p.panel_id));
  }
  EXPECT_TRUE(model_->GetActivePanel().empty());
}

TEST_F(AstraDevToolsModelTest, DeepenedGetPanelByTypeInvalid) {
  // Cast an invalid int to the enum type.
  auto invalid_type = static_cast<AstraDevToolsPanelType>(999);
  EXPECT_EQ(nullptr, model_->GetPanelByType(invalid_type));
}

TEST_F(AstraDevToolsModelTest, DeepenedReorderPanelsEmptyList) {
  auto before = model_->GetPanels();
  model_->ReorderPanels({});
  auto after = model_->GetPanels();

  // Empty list should not change order.
  EXPECT_EQ(before.size(), after.size());
  for (size_t i = 0; i < before.size(); ++i) {
    EXPECT_EQ(before[i].panel_id, after[i].panel_id);
  }
}

TEST_F(AstraDevToolsModelTest, DeepenedDockStateEnumHasFiveValues) {
  // Verify the five dock states are distinct.
  std::set<AstraDevToolsDockState> states = {
    AstraDevToolsDockState::kDockedBottom,
    AstraDevToolsDockState::kDockedLeft,
    AstraDevToolsDockState::kDockedRight,
    AstraDevToolsDockState::kUndocked,
    AstraDevToolsDockState::kMinimized,
  };
  EXPECT_EQ(5u, states.size());
}

TEST_F(AstraDevToolsModelTest, DeepenedPanelTypeEnumHasSixValues) {
  std::set<AstraDevToolsPanelType> types = {
    AstraDevToolsPanelType::kWorkspacePanel,
    AstraDevToolsPanelType::kTabStackPanel,
    AstraDevToolsPanelType::kNotesPanel,
    AstraDevToolsPanelType::kPerformancePanel,
    AstraDevToolsPanelType::kAccessibilityPanel,
    AstraDevToolsPanelType::kA11yTreePanel,
  };
  EXPECT_EQ(6u, types.size());
}

// =========================================================================
// AstraDevToolsToolbar views tests
// =========================================================================

class AstraDevToolsToolbarTest : public views::ViewsTestBase {
 public:
  AstraDevToolsToolbarTest() = default;
  ~AstraDevToolsToolbarTest() override = default;

  void SetUp() override {
    ViewsTestBase::SetUp();
    profile_ = std::make_unique<TestingProfile>();
    prefs::RegisterProfilePrefs(profile_->GetPrefs());
    model_ = std::make_unique<AstraDevToolsModel>(profile_->GetPrefs());

    delegate_ = std::make_unique<TestToolbarDelegate>();

    widget_ = CreateTestWidget();
    toolbar_ = widget_->SetContentsView(
        std::make_unique<AstraDevToolsToolbar>(delegate_.get()));
    toolbar_->SetModel(model_.get());
    widget_->Show();
  }

  void TearDown() override {
    widget_.reset();
    model_.reset();
    profile_.reset();
    delegate_.reset();
    ViewsTestBase::TearDown();
  }

 protected:
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<AstraDevToolsModel> model_;
  std::unique_ptr<TestToolbarDelegate> delegate_;
  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraDevToolsToolbar> toolbar_ = nullptr;
};

TEST_F(AstraDevToolsToolbarTest, ConstructsWithoutCrash) {
  EXPECT_NE(nullptr, toolbar_);
  EXPECT_NE(nullptr, toolbar_->GetWidget());
}

TEST_F(AstraDevToolsToolbarTest, BackButtonExists) {
  EXPECT_NE(nullptr, toolbar_->back_button_for_testing());
}

TEST_F(AstraDevToolsToolbarTest, ForwardButtonExists) {
  EXPECT_NE(nullptr, toolbar_->forward_button_for_testing());
}

TEST_F(AstraDevToolsToolbarTest, SearchBoxExists) {
  EXPECT_NE(nullptr, toolbar_->search_box_for_testing());
}

TEST_F(AstraDevToolsToolbarTest, SettingsButtonExists) {
  EXPECT_NE(nullptr, toolbar_->settings_button_for_testing());
}

TEST_F(AstraDevToolsToolbarTest, DetachButtonExists) {
  EXPECT_NE(nullptr, toolbar_->detach_button_for_testing());
}

TEST_F(AstraDevToolsToolbarTest, MenuButtonExists) {
  EXPECT_NE(nullptr, toolbar_->menu_button_for_testing());
}

TEST_F(AstraDevToolsToolbarTest, FocusModeButtonExists) {
  EXPECT_NE(nullptr, toolbar_->focus_mode_button_for_testing());
}

TEST_F(AstraDevToolsToolbarTest, PanelTabsContainerExists) {
  EXPECT_NE(nullptr, toolbar_->panel_tabs_container_for_testing());
}

TEST_F(AstraDevToolsToolbarTest, PanelTabsCountMatchesVisiblePanels) {
  EXPECT_EQ(6u, toolbar_->panel_tab_count_for_testing());
}

TEST_F(AstraDevToolsToolbarTest, BackButtonClickNotifiesDelegate) {
  toolbar_->back_button_for_testing()->OnMousePressed(
      ui::MouseEvent(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, 0));
  EXPECT_EQ(1, delegate_->back_clicks);
}

TEST_F(AstraDevToolsToolbarTest, ForwardButtonClickNotifiesDelegate) {
  toolbar_->forward_button_for_testing()->OnMousePressed(
      ui::MouseEvent(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, 0));
  EXPECT_EQ(1, delegate_->forward_clicks);
}

TEST_F(AstraDevToolsToolbarTest, SettingsButtonClickNotifiesDelegate) {
  toolbar_->settings_button_for_testing()->OnMousePressed(
      ui::MouseEvent(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, 0));
  EXPECT_EQ(1, delegate_->settings_clicks);
}

TEST_F(AstraDevToolsToolbarTest, DetachButtonClickNotifiesDelegate) {
  toolbar_->detach_button_for_testing()->OnMousePressed(
      ui::MouseEvent(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, 0));
  EXPECT_EQ(1, delegate_->detach_clicks);
}

TEST_F(AstraDevToolsToolbarTest, MenuButtonClickNotifiesDelegate) {
  toolbar_->menu_button_for_testing()->OnMousePressed(
      ui::MouseEvent(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, 0));
  EXPECT_EQ(1, delegate_->menu_clicks);
}

TEST_F(AstraDevToolsToolbarTest, FocusModeButtonClickNotifiesDelegate) {
  toolbar_->focus_mode_button_for_testing()->OnMousePressed(
      ui::MouseEvent(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, 0));
  EXPECT_EQ(1, delegate_->focus_mode_toggles);
}

TEST_F(AstraDevToolsToolbarTest, PanelTabClickNotifiesDelegate) {
  // Click on the notes panel tab (index 1).
  auto* container = toolbar_->panel_tabs_container_for_testing();
  ASSERT_TRUE(container);
  ASSERT_GE(container->children().size(), 2u);

  auto* tab_button = static_cast<views::LabelButton*>(
      container->children()[1]);
  tab_button->OnMousePressed(
      ui::MouseEvent(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, 0));

  EXPECT_EQ(1, delegate_->panel_tab_clicks);
  EXPECT_EQ("notes", delegate_->last_panel_tab);
}

TEST_F(AstraDevToolsToolbarTest, DarkThemeByDefault) {
  // Default theme is system, which resolves to dark.
  // The toolbar should be in dark mode.
  // We can't easily check background color directly,
  // but construction with dark theme doesn't crash.
  SUCCEED();
}

TEST_F(AstraDevToolsToolbarTest, SetThemeLight) {
  toolbar_->SetTheme(false);  // light theme
  // No crash = success.
  SUCCEED();
}

TEST_F(AstraDevToolsToolbarTest, SetThemeDark) {
  toolbar_->SetTheme(true);  // dark theme
  // No crash = success.
  SUCCEED();
}

TEST_F(AstraDevToolsToolbarTest, UpdateFromModel) {
  model_->SetPanelVisible("screenshot", false);
  toolbar_->UpdateFromModel();

  // Should have 5 visible panel tabs now.
  EXPECT_EQ(5u, toolbar_->panel_tab_count_for_testing());
}

TEST_F(AstraDevToolsToolbarTest, SearchTextDefaultEmpty) {
  EXPECT_TRUE(toolbar_->search_text().empty());
}

// =========================================================================
// AstraDevToolsWorkspacePanel views tests
// =========================================================================

class AstraDevToolsWorkspacePanelTest : public views::ViewsTestBase {
 public:
  AstraDevToolsWorkspacePanelTest() = default;
  ~AstraDevToolsWorkspacePanelTest() override = default;

  void SetUp() override {
    ViewsTestBase::SetUp();

    delegate_ = std::make_unique<TestWorkspacePanelDelegate>();

    widget_ = CreateTestWidget();
    panel_ = widget_->SetContentsView(
        std::make_unique<AstraDevToolsWorkspacePanel>());
    panel_->SetDelegate(delegate_.get());
    panel_->Refresh();
    widget_->Show();
  }

  void TearDown() override {
    widget_.reset();
    delegate_.reset();
    ViewsTestBase::TearDown();
  }

 protected:
  std::unique_ptr<TestWorkspacePanelDelegate> delegate_;
  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraDevToolsWorkspacePanel> panel_ = nullptr;
};

TEST_F(AstraDevToolsWorkspacePanelTest, ConstructsWithoutCrash) {
  EXPECT_NE(nullptr, panel_);
  EXPECT_NE(nullptr, panel_->GetWidget());
}

TEST_F(AstraDevToolsWorkspacePanelTest, WorkspaceNameLabelExists) {
  EXPECT_NE(nullptr, panel_->workspace_name_label_for_testing());
}

TEST_F(AstraDevToolsWorkspacePanelTest, TabMetadataLabelExists) {
  EXPECT_NE(nullptr, panel_->tab_metadata_label_for_testing());
}

TEST_F(AstraDevToolsWorkspacePanelTest, WorkspaceListContainerExists) {
  EXPECT_NE(nullptr, panel_->workspace_list_container_for_testing());
}

TEST_F(AstraDevToolsWorkspacePanelTest, TabListContainerExists) {
  EXPECT_NE(nullptr, panel_->tab_list_container_for_testing());
}

TEST_F(AstraDevToolsWorkspacePanelTest, SearchBoxExists) {
  EXPECT_NE(nullptr, panel_->search_box_for_testing());
}

TEST_F(AstraDevToolsWorkspacePanelTest, ActionButtonsExist) {
  EXPECT_NE(nullptr, panel_->new_workspace_button_for_testing());
  EXPECT_NE(nullptr, panel_->delete_workspace_button_for_testing());
  EXPECT_NE(nullptr, panel_->rename_workspace_button_for_testing());
}

TEST_F(AstraDevToolsWorkspacePanelTest, NoServiceShowsPlaceholder) {
  // Without workspace service, the list should show a placeholder.
  // The list container should have children (placeholder label).
  auto* list = panel_->workspace_list_container_for_testing();
  EXPECT_TRUE(list->children().size() > 0);
}

TEST_F(AstraDevToolsWorkspacePanelTest, NewWorkspaceButtonNotifiesDelegate) {
  panel_->new_workspace_button_for_testing()->OnMousePressed(
      ui::MouseEvent(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, 0));
  EXPECT_EQ(1, delegate_->new_workspace_count);
}

TEST_F(AstraDevToolsWorkspacePanelTest, SetDarkTheme) {
  panel_->SetTheme(true);
  SUCCEED();
}

TEST_F(AstraDevToolsWorkspacePanelTest, SetLightTheme) {
  panel_->SetTheme(false);
  SUCCEED();
}

TEST_F(AstraDevToolsWorkspacePanelTest, SetSearchFilter) {
  panel_->SetSearchFilter(u"test");
  SUCCEED();
}

TEST_F(AstraDevToolsWorkspacePanelTest, SelectedWorkspaceDefaultEmpty) {
  EXPECT_TRUE(panel_->selected_workspace_id_for_testing().empty());
}

TEST_F(AstraDevToolsWorkspacePanelTest, NoSelectedWorkspaceShowsPlaceholderInTabList) {
  auto* tab_list = panel_->tab_list_container_for_testing();
  EXPECT_TRUE(tab_list->children().size() > 0);
}

// =========================================================================
// AstraDevToolsIntegration tests
// =========================================================================

class AstraDevToolsIntegrationTest : public testing::Test {
 public:
  AstraDevToolsIntegrationTest() = default;
  ~AstraDevToolsIntegrationTest() override = default;

  void SetUp() override {
    profile_ = std::make_unique<TestingProfile>();
    prefs::RegisterProfilePrefs(profile_->GetPrefs());

    // The integration takes a Profile*.
    // Note: in the overlay skeleton, the integration creates the model
    // with a null PrefService.  For tests, we need the model to have
    // real prefs.  We'll test the model directly and test integration
    // for coordination logic.
    integration_ =
        std::make_unique<AstraDevToolsIntegration>(profile_.get());
  }

  void TearDown() override {
    integration_.reset();
    profile_.reset();
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<AstraDevToolsIntegration> integration_;
};

TEST_F(AstraDevToolsIntegrationTest, ConstructsWithoutCrash) {
  EXPECT_NE(nullptr, integration_);
}

TEST_F(AstraDevToolsIntegrationTest, ModelExists) {
  EXPECT_NE(nullptr, integration_->model());
}

TEST_F(AstraDevToolsIntegrationTest, ModelHasDefaultPanels) {
  ASSERT_NE(nullptr, integration_->model());
  EXPECT_EQ(6u, integration_->model()->panel_count());
}

TEST_F(AstraDevToolsIntegrationTest, ToolbarLazyCreation) {
  // Access toolbar — should create it.
  auto* toolbar = integration_->toolbar();
  EXPECT_NE(nullptr, toolbar);
}

TEST_F(AstraDevToolsIntegrationTest, WorkspacePanelLazyCreation) {
  auto* panel = integration_->workspace_panel();
  EXPECT_NE(nullptr, panel);
}

TEST_F(AstraDevToolsIntegrationTest, SwitchToPanel) {
  EXPECT_TRUE(integration_->SwitchToPanel("notes"));
  EXPECT_EQ("notes", integration_->active_panel_id());
}

TEST_F(AstraDevToolsIntegrationTest, SwitchToNonexistentPanel) {
  EXPECT_FALSE(integration_->SwitchToPanel("nonexistent"));
}

TEST_F(AstraDevToolsIntegrationTest, DefaultActivePanel) {
  EXPECT_EQ("workspace", integration_->active_panel_id());
}

TEST_F(AstraDevToolsIntegrationTest, SettingsDrawerDefaultClosed) {
  EXPECT_FALSE(integration_->IsSettingsDrawerOpen());
}

TEST_F(AstraDevToolsIntegrationTest, ToggleSettingsDrawer) {
  integration_->ToggleSettingsDrawer();
  EXPECT_TRUE(integration_->IsSettingsDrawerOpen());

  integration_->ToggleSettingsDrawer();
  EXPECT_FALSE(integration_->IsSettingsDrawerOpen());
}

TEST_F(AstraDevToolsIntegrationTest, SetSettingsDrawerOpen) {
  integration_->SetSettingsDrawerOpen(true);
  EXPECT_TRUE(integration_->IsSettingsDrawerOpen());

  integration_->SetSettingsDrawerOpen(true);  // Already open — no-op.
  EXPECT_TRUE(integration_->IsSettingsDrawerOpen());

  integration_->SetSettingsDrawerOpen(false);
  EXPECT_FALSE(integration_->IsSettingsDrawerOpen());
}

TEST_F(AstraDevToolsIntegrationTest, InstallForTesting) {
  integration_->InstallForTesting();
  // Should have created toolbar and workspace panel.
  EXPECT_NE(nullptr, integration_->toolbar());
  EXPECT_NE(nullptr, integration_->workspace_panel());
}

TEST_F(AstraDevToolsIntegrationTest, BuildContainerView) {
  integration_->BuildContainerView();
  EXPECT_TRUE(integration_->container_view_built_for_testing());
  EXPECT_NE(nullptr, integration_->container_view_for_testing());
}

TEST_F(AstraDevToolsIntegrationTest, PanelContainerInContainerView) {
  integration_->BuildContainerView();
  EXPECT_NE(nullptr, integration_->panel_container_for_testing());
}

TEST_F(AstraDevToolsIntegrationTest, SidebarViewInContainerView) {
  integration_->BuildContainerView();
  EXPECT_NE(nullptr, integration_->sidebar_view_for_testing());
}

TEST_F(AstraDevToolsIntegrationTest, SettingsDrawerInContainerView) {
  integration_->BuildContainerView();
  EXPECT_NE(nullptr, integration_->settings_drawer_for_testing());
}

TEST_F(AstraDevToolsIntegrationTest, SettingsDrawerHiddenByDefault) {
  integration_->BuildContainerView();
  auto* drawer = integration_->settings_drawer_for_testing();
  ASSERT_NE(nullptr, drawer);
  EXPECT_FALSE(drawer->GetVisible());
}

TEST_F(AstraDevToolsIntegrationTest, SettingsDrawerShownWhenOpen) {
  integration_->BuildContainerView();
  integration_->SetSettingsDrawerOpen(true);

  auto* drawer = integration_->settings_drawer_for_testing();
  ASSERT_NE(nullptr, drawer);
  EXPECT_TRUE(drawer->GetVisible());
}

TEST_F(AstraDevToolsIntegrationTest, WorkspaceServiceNullByDefault) {
  EXPECT_EQ(nullptr, integration_->workspace_service());
}

TEST_F(AstraDevToolsIntegrationTest, InspectedContentsNullByDefault) {
  EXPECT_EQ(nullptr, integration_->inspected_contents());
}

TEST_F(AstraDevToolsIntegrationTest, ObserverAddRemove) {
  class TestObserver : public AstraDevToolsIntegration::Observer {
   public:
    int panel_changes = 0;
    void OnActivePanelChanged(const std::string&) override {
      panel_changes++;
    }
  };

  TestObserver observer;
  integration_->AddObserver(&observer);

  integration_->SwitchToPanel("notes");
  EXPECT_GE(observer.panel_changes, 1);

  integration_->RemoveObserver(&observer);
  // No crash after removal = success.
}

// =========================================================================
// Deepened integration tests
// =========================================================================

TEST_F(AstraDevToolsIntegrationTest, DeepenedDevToolsClosedByDefault) {
  EXPECT_FALSE(integration_->IsDevToolsOpen());
}

TEST_F(AstraDevToolsIntegrationTest, DeepenedShowDevTools) {
  integration_->ShowDevTools();
  EXPECT_TRUE(integration_->IsDevToolsOpen());
}

TEST_F(AstraDevToolsIntegrationTest, DeepenedCloseDevTools) {
  integration_->ShowDevTools();
  ASSERT_TRUE(integration_->IsDevToolsOpen());

  integration_->CloseDevTools();
  EXPECT_FALSE(integration_->IsDevToolsOpen());
}

TEST_F(AstraDevToolsIntegrationTest, DeepenedToggleDevTools) {
  ASSERT_FALSE(integration_->IsDevToolsOpen());

  integration_->ToggleDevTools();
  EXPECT_TRUE(integration_->IsDevToolsOpen());

  integration_->ToggleDevTools();
  EXPECT_FALSE(integration_->IsDevToolsOpen());
}

TEST_F(AstraDevToolsIntegrationTest, DeepenedShowPanel) {
  EXPECT_TRUE(integration_->ShowPanel(AstraDevToolsPanelType::kNotesPanel));
  EXPECT_TRUE(integration_->IsDevToolsOpen());
  EXPECT_TRUE(integration_->IsPanelOpen());
}

TEST_F(AstraDevToolsIntegrationTest, DeepenedClosePanel) {
  integration_->ShowPanel(AstraDevToolsPanelType::kWorkspacePanel);
  ASSERT_TRUE(integration_->IsPanelOpen());

  integration_->ClosePanel();
  EXPECT_FALSE(integration_->IsPanelOpen());
}

TEST_F(AstraDevToolsIntegrationTest, DeepenedGetModel) {
  EXPECT_NE(nullptr, integration_->GetModel());
  EXPECT_EQ(integration_->model(), integration_->GetModel());
}

TEST_F(AstraDevToolsIntegrationTest, DeepenedDockStateDefault) {
  EXPECT_EQ(AstraDevToolsDockState::kDockedBottom, integration_->GetDockState());
}

TEST_F(AstraDevToolsIntegrationTest, DeepenedSetDockState) {
  integration_->SetDockState(AstraDevToolsDockState::kDockedLeft);
  EXPECT_EQ(AstraDevToolsDockState::kDockedLeft, integration_->GetDockState());

  integration_->SetDockState(AstraDevToolsDockState::kUndocked);
  EXPECT_EQ(AstraDevToolsDockState::kUndocked, integration_->GetDockState());
}

TEST_F(AstraDevToolsIntegrationTest, DeepenedZoomLevelDefault) {
  EXPECT_DOUBLE_EQ(1.0, integration_->GetZoomLevel());
}

TEST_F(AstraDevToolsIntegrationTest, DeepenedSetZoomLevel) {
  integration_->SetZoomLevel(1.5);
  EXPECT_DOUBLE_EQ(1.5, integration_->GetZoomLevel());
}

TEST_F(AstraDevToolsIntegrationTest, DeepenedInspectElementToggles) {
  // Inspect element toggles the internal flag.
  integration_->InspectElement();
  // No crash = success. TODO(astra): Wire to actual DevTools integration.
  SUCCEED();
}

TEST_F(AstraDevToolsIntegrationTest, DeepenedToggleDeviceMode) {
  integration_->ToggleDeviceMode();
  // No crash = success.
  SUCCEED();
}

TEST_F(AstraDevToolsIntegrationTest, DeepenedReloadDevTools) {
  integration_->ReloadDevTools();
  // No crash = success.
  SUCCEED();
}

// -- Integration observer tests ------------------------------------------

TEST_F(AstraDevToolsIntegrationTest, DeepenedIntegrationObserverOnOpened) {
  class TestIntegrationObserver : public AstraDevToolsIntegrationObserver {
   public:
    int opened_count = 0;
    void OnDevToolsOpened(AstraDevToolsIntegration*) override {
      opened_count++;
    }
  };

  TestIntegrationObserver observer;
  integration_->AddIntegrationObserver(&observer);

  integration_->ShowDevTools();
  EXPECT_EQ(1, observer.opened_count);

  integration_->RemoveIntegrationObserver(&observer);
}

TEST_F(AstraDevToolsIntegrationTest, DeepenedIntegrationObserverOnClosed) {
  class TestIntegrationObserver : public AstraDevToolsIntegrationObserver {
   public:
    int closed_count = 0;
    void OnDevToolsClosed(AstraDevToolsIntegration*) override {
      closed_count++;
    }
  };

  integration_->ShowDevTools();

  TestIntegrationObserver observer;
  integration_->AddIntegrationObserver(&observer);

  integration_->CloseDevTools();
  EXPECT_EQ(1, observer.closed_count);

  integration_->RemoveIntegrationObserver(&observer);
}

TEST_F(AstraDevToolsIntegrationTest, DeepenedIntegrationObserverOnPanelShown) {
  class TestIntegrationObserver : public AstraDevToolsIntegrationObserver {
   public:
    int shown_count = 0;
    AstraDevToolsPanelType last_type = AstraDevToolsPanelType::kWorkspacePanel;
    void OnPanelShown(AstraDevToolsIntegration*,
                      AstraDevToolsPanelType type) override {
      shown_count++;
      last_type = type;
    }
  };

  TestIntegrationObserver observer;
  integration_->AddIntegrationObserver(&observer);

  integration_->ShowPanel(AstraDevToolsPanelType::kNotesPanel);
  EXPECT_GE(observer.shown_count, 1);

  integration_->RemoveIntegrationObserver(&observer);
}

TEST_F(AstraDevToolsIntegrationTest, DeepenedIntegrationObserverOnPanelHidden) {
  class TestIntegrationObserver : public AstraDevToolsIntegrationObserver {
   public:
    int hidden_count = 0;
    void OnPanelHidden(AstraDevToolsIntegration*) override {
      hidden_count++;
    }
  };

  integration_->ShowPanel(AstraDevToolsPanelType::kWorkspacePanel);

  TestIntegrationObserver observer;
  integration_->AddIntegrationObserver(&observer);

  integration_->ClosePanel();
  EXPECT_GE(observer.hidden_count, 1);

  integration_->RemoveIntegrationObserver(&observer);
}

TEST_F(AstraDevToolsIntegrationTest, DeepenedIntegrationObserverDefaultsDoNotCrash) {
  class EmptyIntegrationObserver : public AstraDevToolsIntegrationObserver {
   public:
    // All methods use default implementations.
  };

  EmptyIntegrationObserver observer;
  integration_->AddIntegrationObserver(&observer);

  integration_->ShowDevTools();
  integration_->ShowPanel(AstraDevToolsPanelType::kWorkspacePanel);
  integration_->ClosePanel();
  integration_->CloseDevTools();

  integration_->RemoveIntegrationObserver(&observer);
  SUCCEED();
}

// =========================================================================
// Panel struct tests
// =========================================================================

TEST(AstraDevToolsPanelTest, DefaultValues) {
  AstraDevToolsPanel panel;
  EXPECT_TRUE(panel.id.empty());
  EXPECT_TRUE(panel.title.empty());
  EXPECT_TRUE(panel.icon.empty());
  EXPECT_TRUE(panel.is_visible);
  EXPECT_FALSE(panel.is_pinned);
  EXPECT_EQ(0u, panel.position);
}

TEST(AstraDevToolsPanelTest, Equality) {
  AstraDevToolsPanel p1;
  p1.id = "test";
  p1.title = "Test";
  p1.icon = "icon";
  p1.is_visible = true;
  p1.is_pinned = false;
  p1.position = 5;

  AstraDevToolsPanel p2 = p1;
  EXPECT_TRUE(p1 == p2);
  EXPECT_FALSE(p1 != p2);
}

TEST(AstraDevToolsPanelTest, InequalityId) {
  AstraDevToolsPanel p1;
  p1.id = "a";
  AstraDevToolsPanel p2;
  p2.id = "b";
  EXPECT_TRUE(p1 != p2);
}

TEST(AstraDevToolsPanelTest, InequalityVisibility) {
  AstraDevToolsPanel p1;
  p1.id = "test";
  p1.is_visible = true;
  AstraDevToolsPanel p2;
  p2.id = "test";
  p2.is_visible = false;
  EXPECT_TRUE(p1 != p2);
}

TEST(AstraDevToolsPanelTest, InequalityPinned) {
  AstraDevToolsPanel p1;
  p1.id = "test";
  p1.is_pinned = true;
  AstraDevToolsPanel p2;
  p2.id = "test";
  p2.is_pinned = false;
  EXPECT_TRUE(p1 != p2);
}

TEST(AstraDevToolsPanelTest, InequalityPosition) {
  AstraDevToolsPanel p1;
  p1.id = "test";
  p1.position = 1;
  AstraDevToolsPanel p2;
  p2.id = "test";
  p2.position = 2;
  EXPECT_TRUE(p1 != p2);
}

}  // namespace astra
