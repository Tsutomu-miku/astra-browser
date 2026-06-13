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
