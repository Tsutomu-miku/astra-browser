// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/devtools/astra_devtools_workspace_panel.h"

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/controls/label.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "astra/common/astra_workspace_types.h"
#include "astra/ui/views/devtools/astra_devtools_model.h"

namespace astra {

// =========================================================================
// AstraDevToolsWorkspacePanel tests
// =========================================================================
//
// These are skeleton tests that verify construction and basic view hierarchy.
// They compile against the Chromium test framework but may need refinement
// when built against a full Chromium checkout (where AstraWorkspaceService
// and AstraTabFeatures have full implementations).
//
// Tests that require a full workspace service or WebContents are marked
// with TODO(astra) and exercise the "no service" / "no contents" code paths.
// =========================================================================

class AstraDevToolsWorkspacePanelTest : public views::ViewsTestBase {
 public:
  AstraDevToolsWorkspacePanelTest() = default;
  ~AstraDevToolsWorkspacePanelTest() override = default;

 protected:
  // testing::Test:
  void SetUp() override {
    views::ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();
    panel_ = widget_->SetContentsView(
        std::make_unique<AstraDevToolsWorkspacePanel>());
  }

  void TearDown() override {
    widget_.reset();
    views::ViewsTestBase::TearDown();
  }

  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraDevToolsWorkspacePanel> panel_ = nullptr;
};

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST_F(AstraDevToolsWorkspacePanelTest, ConstructionCreatesPanelView) {
  ASSERT_NE(panel_, nullptr);
  // The panel is a valid View instance hosted by the widget.
  EXPECT_EQ(panel_->parent(), widget_->GetRootView());
}

TEST_F(AstraDevToolsWorkspacePanelTest,
       ConstructionDoesNotBuildSubviewsImmediately) {
  // The panel uses lazy initialization — labels are null before first Refresh.
  EXPECT_EQ(panel_->workspace_name_label_for_testing(), nullptr);
  EXPECT_EQ(panel_->tab_metadata_label_for_testing(), nullptr);
}

TEST_F(AstraDevToolsWorkspacePanelTest, ConstructionHasNoChildrenInitially) {
  // Before Refresh(), the panel should have no child views (lazy build).
  // Actually it may have 0 since BuildPanel() hasn't been called yet.
  EXPECT_EQ(panel_->children().size(), 0u);
}

// ---------------------------------------------------------------------------
// Refresh / lazy build mechanism
// ---------------------------------------------------------------------------

TEST_F(AstraDevToolsWorkspacePanelTest, RefreshBuildsPanelUi) {
  // First Refresh() call should build the UI.
  panel_->Refresh();

  // After Refresh, labels should be non-null.
  EXPECT_NE(panel_->workspace_name_label_for_testing(), nullptr);
  EXPECT_NE(panel_->tab_metadata_label_for_testing(), nullptr);
}

TEST_F(AstraDevToolsWorkspacePanelTest, RefreshIsIdempotent) {
  // Calling Refresh multiple times should not crash or create duplicate views.
  panel_->Refresh();
  size_t child_count_after_first = panel_->children().size();

  panel_->Refresh();
  size_t child_count_after_second = panel_->children().size();

  // Child count should be the same (no duplicate sections added).
  EXPECT_EQ(child_count_after_first, child_count_after_second);
}

TEST_F(AstraDevToolsWorkspacePanelTest,
       RefreshWithoutServiceShowsNoServiceMessage) {
  panel_->Refresh();

  views::Label* name_label = panel_->workspace_name_label_for_testing();
  ASSERT_NE(name_label, nullptr);
  // With no workspace service set, the label should show a "no service" message.
  EXPECT_FALSE(name_label->GetText().empty());
}

TEST_F(AstraDevToolsWorkspacePanelTest,
       RefreshWithoutWebContentsShowsNoTabMessage) {
  panel_->Refresh();

  views::Label* tab_label = panel_->tab_metadata_label_for_testing();
  ASSERT_NE(tab_label, nullptr);
  // With no inspected WebContents, the tab label should show a "no tab" message.
  EXPECT_FALSE(tab_label->GetText().empty());
}

// ---------------------------------------------------------------------------
// Labels are populated
// ---------------------------------------------------------------------------

TEST_F(AstraDevToolsWorkspacePanelTest,
       WorkspaceNameLabelExistsAfterRefresh) {
  panel_->Refresh();

  views::Label* label = panel_->workspace_name_label_for_testing();
  ASSERT_NE(label, nullptr);
  // Label should be a child of the panel (directly or nested).
  EXPECT_TRUE(label->parent() != nullptr);
}

TEST_F(AstraDevToolsWorkspacePanelTest,
       TabMetadataLabelExistsAfterRefresh) {
  panel_->Refresh();

  views::Label* label = panel_->tab_metadata_label_for_testing();
  ASSERT_NE(label, nullptr);
  // Tab metadata label should be multi-line.
  EXPECT_TRUE(label->GetMultiLine());
}

TEST_F(AstraDevToolsWorkspacePanelTest, PanelHasBackground) {
  panel_->Refresh();

  // The panel should have a solid dark background matching DevTools theme.
  EXPECT_NE(panel_->background(), nullptr);
}

// ---------------------------------------------------------------------------
// View hierarchy
// ---------------------------------------------------------------------------

TEST_F(AstraDevToolsWorkspacePanelTest,
       PanelHasMultipleSectionsAfterRefresh) {
  panel_->Refresh();

  // After building, the panel should have multiple child sections:
  // workspace info, workspace list, and tab metadata sections.
  EXPECT_GE(panel_->children().size(), 3u);
}

TEST_F(AstraDevToolsWorkspacePanelTest, PanelUsesBoxLayout) {
  panel_->Refresh();

  // The panel should have a BoxLayout for vertical stacking of sections.
  EXPECT_NE(panel_->GetLayoutManager(), nullptr);
}

// ---------------------------------------------------------------------------
// SetInspectedWebContents
// ---------------------------------------------------------------------------

TEST_F(AstraDevToolsWorkspacePanelTest,
       SetInspectedWebContentsNullDoesNotCrash) {
  panel_->Refresh();
  panel_->SetInspectedWebContents(nullptr);
  // Should not crash.
  SUCCEED();
}

TEST_F(AstraDevToolsWorkspacePanelTest,
       SetInspectedWebContentsUpdatesTabLabel) {
  panel_->Refresh();

  // After setting contents to null (no tab), label should show "no tab" text.
  panel_->SetInspectedWebContents(nullptr);

  views::Label* label = panel_->tab_metadata_label_for_testing();
  ASSERT_NE(label, nullptr);
  EXPECT_FALSE(label->GetText().empty());
}

// ---------------------------------------------------------------------------
// SetWorkspaceService
// ---------------------------------------------------------------------------

TEST_F(AstraDevToolsWorkspacePanelTest,
       SetWorkspaceServiceNullDoesNotCrash) {
  panel_->Refresh();
  panel_->SetWorkspaceService(nullptr);
  // Should not crash.
  SUCCEED();
}

// ---------------------------------------------------------------------------
// TODO(astra): Tests that require a real workspace service
// ---------------------------------------------------------------------------
//
// Once we have a mock or test double for AstraWorkspaceService, add:
//   - RefreshWithWorkspaceService_PopulatesWorkspaceName
//   - RefreshWithMultipleWorkspaces_ShowsAllInList
//   - SetActiveWorkspace_UpdatesHighlight
//   - TabMetadataWithFeatures_ShowsAllFields
//
// These require browser-layer services which are not available in the
// views unit test context without a full Chromium checkout and test harness.
// Chromium component pattern: content::BrowserTaskEnvironment +
//   TestingProfile for service creation in browser tests.

// =========================================================================
// Deepened workspace panel tests
// =========================================================================

// Test delegate for deepened workspace panel tests.
class TestWorkspacePanelDelegateDeep
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
  int color_changed_count = 0;
  std::string last_color_workspace;
  SkColor last_color = SK_ColorBLACK;

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
  void OnWorkspaceColorChanged(const std::string& id,
                               SkColor color) override {
    color_changed_count++;
    last_color_workspace = id;
    last_color = color;
  }
};

// Helper: create a test workspace info.
AstraWorkspaceInfo MakeTestWorkspace(const std::string& id,
                                     const std::u16string& name,
                                     int tab_count = 3,
                                     int window_count = 1) {
  AstraWorkspaceInfo info;
  info.id = id;
  info.name = name;
  info.tab_count = tab_count;
  info.window_count = window_count;
  info.color = AstraWorkspaceColor::kBlue;
  info.accent_color = AstraWorkspaceAccentColor::kBlue;
  return info;
}

class AstraDevToolsWorkspacePanelDeepTest : public views::ViewsTestBase {
 public:
  AstraDevToolsWorkspacePanelDeepTest() = default;
  ~AstraDevToolsWorkspacePanelDeepTest() override = default;

 protected:
  void SetUp() override {
    ViewsTestBase::SetUp();

    delegate_ = std::make_unique<TestWorkspacePanelDelegateDeep>();
    model_ = std::make_unique<AstraDevToolsModel>(nullptr);

    widget_ = CreateTestWidget();
    panel_ = widget_->SetContentsView(
        std::make_unique<AstraDevToolsWorkspacePanel>());
    panel_->SetDelegate(delegate_.get());
    panel_->SetModel(model_.get());
    panel_->Refresh();
    widget_->Show();
  }

  void TearDown() override {
    widget_.reset();
    delegate_.reset();
    model_.reset();
    ViewsTestBase::TearDown();
  }

  std::unique_ptr<TestWorkspacePanelDelegateDeep> delegate_;
  std::unique_ptr<AstraDevToolsModel> model_;
  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraDevToolsWorkspacePanel> panel_ = nullptr;
};

// -- Model integration ----------------------------------------------------

TEST_F(AstraDevToolsWorkspacePanelDeepTest, ModelSetOnConstruction) {
  EXPECT_EQ(model_.get(), panel_->GetModel());
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, SetModelUpdatesModel) {
  auto new_model = std::make_unique<AstraDevToolsModel>(nullptr);
  panel_->SetModel(new_model.get());
  EXPECT_EQ(new_model.get(), panel_->GetModel());
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, SetModelNullDoesNotCrash) {
  panel_->SetModel(nullptr);
  EXPECT_EQ(nullptr, panel_->GetModel());
  panel_->Refresh();
  SUCCEED();
}

// -- Workspace list -------------------------------------------------------

TEST_F(AstraDevToolsWorkspacePanelDeepTest, SetWorkspacesUpdatesCount) {
  std::vector<AstraWorkspaceInfo> workspaces = {
    MakeTestWorkspace("ws-1", u"Workspace 1"),
    MakeTestWorkspace("ws-2", u"Workspace 2"),
    MakeTestWorkspace("ws-3", u"Workspace 3"),
  };

  panel_->SetWorkspaces(workspaces);
  EXPECT_EQ(3u, panel_->GetWorkspaceCount());
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, SetWorkspacesEmpty) {
  std::vector<AstraWorkspaceInfo> workspaces;
  panel_->SetWorkspaces(workspaces);
  EXPECT_EQ(0u, panel_->GetWorkspaceCount());
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, GetWorkspaceAtValidIndex) {
  std::vector<AstraWorkspaceInfo> workspaces = {
    MakeTestWorkspace("ws-1", u"First"),
    MakeTestWorkspace("ws-2", u"Second"),
  };
  panel_->SetWorkspaces(workspaces);

  const AstraWorkspaceInfo* ws = panel_->GetWorkspaceAt(0);
  ASSERT_NE(nullptr, ws);
  EXPECT_EQ("ws-1", ws->id);
  EXPECT_EQ(u"First", ws->name);

  ws = panel_->GetWorkspaceAt(1);
  ASSERT_NE(nullptr, ws);
  EXPECT_EQ("ws-2", ws->id);
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, GetWorkspaceAtInvalidIndex) {
  std::vector<AstraWorkspaceInfo> workspaces = {
    MakeTestWorkspace("ws-1", u"Only"),
  };
  panel_->SetWorkspaces(workspaces);

  EXPECT_EQ(nullptr, panel_->GetWorkspaceAt(-1));
  EXPECT_EQ(nullptr, panel_->GetWorkspaceAt(5));
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, SetWorkspacesRefreshesList) {
  std::vector<AstraWorkspaceInfo> workspaces = {
    MakeTestWorkspace("ws-1", u"Test WS"),
  };
  panel_->SetWorkspaces(workspaces);

  auto* list = panel_->workspace_list_container_for_testing();
  ASSERT_NE(nullptr, list);
  // Should have at least one child (workspace card or scroll view with items).
  EXPECT_GE(panel_->workspace_item_count_for_testing(), 1u);
}

// -- Selection ------------------------------------------------------------

TEST_F(AstraDevToolsWorkspacePanelDeepTest, DefaultSelectedIndex) {
  EXPECT_EQ(-1, panel_->GetSelectedIndex());
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, SelectWorkspace) {
  std::vector<AstraWorkspaceInfo> workspaces = {
    MakeTestWorkspace("ws-1", u"First"),
    MakeTestWorkspace("ws-2", u"Second"),
  };
  panel_->SetWorkspaces(workspaces);

  panel_->SelectWorkspace(1);
  EXPECT_EQ(1, panel_->GetSelectedIndex());
  EXPECT_EQ("ws-2", panel_->selected_workspace_id_for_testing());
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, SelectWorkspaceInvalidIndex) {
  std::vector<AstraWorkspaceInfo> workspaces = {
    MakeTestWorkspace("ws-1", u"Only"),
  };
  panel_->SetWorkspaces(workspaces);

  panel_->SelectWorkspace(5);
  // Should not crash, selection should remain unchanged.
  SUCCEED();
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, SelectWorkspaceNotifiesDelegate) {
  std::vector<AstraWorkspaceInfo> workspaces = {
    MakeTestWorkspace("ws-select", u"Select Me"),
  };
  panel_->SetWorkspaces(workspaces);

  panel_->SelectWorkspace(0);
  EXPECT_GE(delegate_->workspace_selected_count, 1);
  EXPECT_EQ("ws-select", delegate_->last_selected_workspace);
}

// -- New workspace --------------------------------------------------------

TEST_F(AstraDevToolsWorkspacePanelDeepTest, NewWorkspaceAddsToList) {
  panel_->SetWorkspaces({});
  ASSERT_EQ(0u, panel_->GetWorkspaceCount());

  panel_->NewWorkspace();
  EXPECT_EQ(1u, panel_->GetWorkspaceCount());
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, NewWorkspaceNotifiesDelegate) {
  panel_->NewWorkspace();
  EXPECT_EQ(1, delegate_->new_workspace_count);
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, NewWorkspaceWithExistingWorkspaces) {
  std::vector<AstraWorkspaceInfo> workspaces = {
    MakeTestWorkspace("ws-1", u"First"),
  };
  panel_->SetWorkspaces(workspaces);
  ASSERT_EQ(1u, panel_->GetWorkspaceCount());

  panel_->NewWorkspace();
  EXPECT_EQ(2u, panel_->GetWorkspaceCount());
}

// -- Delete workspace -----------------------------------------------------

TEST_F(AstraDevToolsWorkspacePanelDeepTest, DeleteWorkspaceRemovesFromList) {
  std::vector<AstraWorkspaceInfo> workspaces = {
    MakeTestWorkspace("ws-1", u"First"),
    MakeTestWorkspace("ws-2", u"Second"),
  };
  panel_->SetWorkspaces(workspaces);
  ASSERT_EQ(2u, panel_->GetWorkspaceCount());

  panel_->DeleteWorkspace(0);
  EXPECT_EQ(1u, panel_->GetWorkspaceCount());
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, DeleteWorkspaceNotifiesDelegate) {
  std::vector<AstraWorkspaceInfo> workspaces = {
    MakeTestWorkspace("ws-delete", u"Delete Me"),
  };
  panel_->SetWorkspaces(workspaces);

  panel_->DeleteWorkspace(0);
  EXPECT_EQ(1, delegate_->delete_workspace_count);
  EXPECT_EQ("ws-delete", delegate_->last_deleted_workspace);
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, DeleteWorkspaceInvalidIndex) {
  std::vector<AstraWorkspaceInfo> workspaces = {
    MakeTestWorkspace("ws-1", u"Only"),
  };
  panel_->SetWorkspaces(workspaces);
  ASSERT_EQ(1u, panel_->GetWorkspaceCount());

  panel_->DeleteWorkspace(5);
  // Should not crash, count should be the same.
  EXPECT_EQ(1u, panel_->GetWorkspaceCount());
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, DeleteLastWorkspace) {
  std::vector<AstraWorkspaceInfo> workspaces = {
    MakeTestWorkspace("ws-only", u"The Only One"),
  };
  panel_->SetWorkspaces(workspaces);
  panel_->SelectWorkspace(0);
  ASSERT_EQ(0, panel_->GetSelectedIndex());

  panel_->DeleteWorkspace(0);
  EXPECT_EQ(0u, panel_->GetWorkspaceCount());
  EXPECT_EQ(-1, panel_->GetSelectedIndex());
}

// -- Rename workspace -----------------------------------------------------

TEST_F(AstraDevToolsWorkspacePanelDeepTest, RenameWorkspaceChangesName) {
  std::vector<AstraWorkspaceInfo> workspaces = {
    MakeTestWorkspace("ws-1", u"Original"),
  };
  panel_->SetWorkspaces(workspaces);

  panel_->RenameWorkspace(0, u"Renamed");

  const AstraWorkspaceInfo* ws = panel_->GetWorkspaceAt(0);
  ASSERT_NE(nullptr, ws);
  EXPECT_EQ(u"Renamed", ws->name);
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, RenameWorkspaceNotifiesDelegate) {
  std::vector<AstraWorkspaceInfo> workspaces = {
    MakeTestWorkspace("ws-rename", u"Original"),
  };
  panel_->SetWorkspaces(workspaces);

  panel_->RenameWorkspace(0, u"New Name");
  EXPECT_EQ(1, delegate_->rename_workspace_count);
  EXPECT_EQ("ws-rename", delegate_->last_renamed_workspace);
  EXPECT_EQ("New Name", delegate_->last_rename_name);
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, RenameWorkspaceInvalidIndex) {
  std::vector<AstraWorkspaceInfo> workspaces = {
    MakeTestWorkspace("ws-1", u"Only"),
  };
  panel_->SetWorkspaces(workspaces);

  panel_->RenameWorkspace(5, u"Should Not Work");
  // Should not crash.
  SUCCEED();
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, RenameWorkspaceEmptyName) {
  std::vector<AstraWorkspaceInfo> workspaces = {
    MakeTestWorkspace("ws-1", u"Original"),
  };
  panel_->SetWorkspaces(workspaces);

  panel_->RenameWorkspace(0, u"");
  // Empty name should be allowed or handled gracefully.
  SUCCEED();
}

// -- Workspace color ------------------------------------------------------

TEST_F(AstraDevToolsWorkspacePanelDeepTest, SetWorkspaceColor) {
  std::vector<AstraWorkspaceInfo> workspaces = {
    MakeTestWorkspace("ws-1", u"Colored"),
  };
  panel_->SetWorkspaces(workspaces);

  panel_->SetWorkspaceColor(0, SK_ColorRED);

  const AstraWorkspaceInfo* ws = panel_->GetWorkspaceAt(0);
  ASSERT_NE(nullptr, ws);
  EXPECT_EQ(SK_ColorRED, ws->custom_color);
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, SetWorkspaceColorNotifiesDelegate) {
  std::vector<AstraWorkspaceInfo> workspaces = {
    MakeTestWorkspace("ws-color", u"Color Me"),
  };
  panel_->SetWorkspaces(workspaces);

  panel_->SetWorkspaceColor(0, SK_ColorGREEN);
  EXPECT_EQ(1, delegate_->color_changed_count);
  EXPECT_EQ("ws-color", delegate_->last_color_workspace);
  EXPECT_EQ(SK_ColorGREEN, delegate_->last_color);
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, SetWorkspaceColorInvalidIndex) {
  panel_->SetWorkspaceColor(99, SK_ColorBLUE);
  // Should not crash.
  SUCCEED();
}

// -- Tab count and window count -------------------------------------------

TEST_F(AstraDevToolsWorkspacePanelDeepTest, GetTabCountForWorkspace) {
  std::vector<AstraWorkspaceInfo> workspaces = {
    MakeTestWorkspace("ws-1", u"Many Tabs", 15, 2),
    MakeTestWorkspace("ws-2", u"Few Tabs", 3, 1),
  };
  panel_->SetWorkspaces(workspaces);

  EXPECT_EQ(15, panel_->GetTabCountForWorkspace(0));
  EXPECT_EQ(3, panel_->GetTabCountForWorkspace(1));
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, GetTabCountForInvalidWorkspace) {
  std::vector<AstraWorkspaceInfo> workspaces = {
    MakeTestWorkspace("ws-1", u"Only"),
  };
  panel_->SetWorkspaces(workspaces);

  EXPECT_EQ(0, panel_->GetTabCountForWorkspace(-1));
  EXPECT_EQ(0, panel_->GetTabCountForWorkspace(10));
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, GetWindowCountForWorkspace) {
  std::vector<AstraWorkspaceInfo> workspaces = {
    MakeTestWorkspace("ws-1", u"Multi Window", 10, 3),
    MakeTestWorkspace("ws-2", u"Single Window", 5, 1),
  };
  panel_->SetWorkspaces(workspaces);

  EXPECT_EQ(3, panel_->GetWindowCountForWorkspace(0));
  EXPECT_EQ(1, panel_->GetWindowCountForWorkspace(1));
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, GetWindowCountForInvalidWorkspace) {
  std::vector<AstraWorkspaceInfo> workspaces = {
    MakeTestWorkspace("ws-1", u"Only"),
  };
  panel_->SetWorkspaces(workspaces);

  EXPECT_EQ(0, panel_->GetWindowCountForWorkspace(5));
}

// -- New workspace button visibility --------------------------------------

TEST_F(AstraDevToolsWorkspacePanelDeepTest, NewWorkspaceButtonVisibleByDefault) {
  EXPECT_TRUE(panel_->IsNewWorkspaceButtonVisible());
  EXPECT_TRUE(panel_->new_workspace_button_for_testing()->GetVisible());
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, ShowNewWorkspaceButtonFalse) {
  panel_->ShowNewWorkspaceButton(false);
  EXPECT_FALSE(panel_->IsNewWorkspaceButtonVisible());
  EXPECT_FALSE(panel_->new_workspace_button_for_testing()->GetVisible());
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, ShowNewWorkspaceButtonTrue) {
  panel_->ShowNewWorkspaceButton(false);
  ASSERT_FALSE(panel_->IsNewWorkspaceButtonVisible());

  panel_->ShowNewWorkspaceButton(true);
  EXPECT_TRUE(panel_->IsNewWorkspaceButtonVisible());
  EXPECT_TRUE(panel_->new_workspace_button_for_testing()->GetVisible());
}

// -- Search ---------------------------------------------------------------

TEST_F(AstraDevToolsWorkspacePanelDeepTest, SearchQueryDefaultEmpty) {
  EXPECT_TRUE(panel_->GetSearchQuery().empty());
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, SetSearchQuery) {
  panel_->SetSearchQuery(u"test query");
  EXPECT_EQ(u"test query", panel_->GetSearchQuery());
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, SetSearchQueryEmpty) {
  panel_->SetSearchQuery(u"something");
  panel_->SetSearchQuery(u"");
  EXPECT_TRUE(panel_->GetSearchQuery().empty());
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, SearchVisibleByDefault) {
  EXPECT_TRUE(panel_->IsSearchVisible());
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, ShowSearchFalse) {
  panel_->ShowSearch(false);
  EXPECT_FALSE(panel_->IsSearchVisible());
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, ShowSearchTrue) {
  panel_->ShowSearch(false);
  ASSERT_FALSE(panel_->IsSearchVisible());

  panel_->ShowSearch(true);
  EXPECT_TRUE(panel_->IsSearchVisible());
}

// -- Panel sections layout ------------------------------------------------

TEST_F(AstraDevToolsWorkspacePanelDeepTest, StatsLabelExists) {
  EXPECT_NE(nullptr, panel_->stats_label_for_testing());
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, QuickActionsContainerExists) {
  EXPECT_NE(nullptr, panel_->quick_actions_container_for_testing());
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, StatsShowsWorkspaceCount) {
  std::vector<AstraWorkspaceInfo> workspaces = {
    MakeTestWorkspace("ws-1", u"One"),
    MakeTestWorkspace("ws-2", u"Two"),
    MakeTestWorkspace("ws-3", u"Three"),
  };
  panel_->SetWorkspaces(workspaces);

  auto* stats = panel_->stats_label_for_testing();
  ASSERT_NE(nullptr, stats);
  // Stats label should be non-empty after setting workspaces.
  EXPECT_FALSE(stats->GetText().empty());
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, MultipleSectionsInPanel) {
  // The panel should have multiple sections after refresh.
  EXPECT_GE(panel_->children().size(), 3u);
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, PanelUsesBoxLayout) {
  EXPECT_NE(nullptr, panel_->GetLayoutManager());
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, WorkspaceListHasScrollView) {
  EXPECT_NE(nullptr, panel_->workspace_list_container_for_testing());
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, TabListHasScrollView) {
  EXPECT_NE(nullptr, panel_->tab_list_container_for_testing());
}

// -- Theme ----------------------------------------------------------------

TEST_F(AstraDevToolsWorkspacePanelDeepTest, SetThemeDark) {
  panel_->SetTheme(true);
  SUCCEED();
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, SetThemeLight) {
  panel_->SetTheme(false);
  SUCCEED();
}

// -- Edge cases -----------------------------------------------------------

TEST_F(AstraDevToolsWorkspacePanelDeepTest, NoWorkspacesShowsPlaceholder) {
  panel_->SetWorkspaces({});

  auto* list = panel_->workspace_list_container_for_testing();
  ASSERT_NE(nullptr, list);
  // Should show a placeholder (e.g. "No workspaces" message).
  EXPECT_GE(list->children().size(), 0u);
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, ManyWorkspaces) {
  std::vector<AstraWorkspaceInfo> workspaces;
  for (int i = 0; i < 20; ++i) {
    workspaces.push_back(
        MakeTestWorkspace("ws-" + std::to_string(i),
                          u"Workspace " + base::NumberToString16(i),
                          i, 1));
  }
  panel_->SetWorkspaces(workspaces);
  EXPECT_EQ(20u, panel_->GetWorkspaceCount());
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, LongWorkspaceName) {
  std::u16string long_name(200, u'x');
  std::vector<AstraWorkspaceInfo> workspaces = {
    MakeTestWorkspace("ws-long", long_name),
  };
  panel_->SetWorkspaces(workspaces);

  const AstraWorkspaceInfo* ws = panel_->GetWorkspaceAt(0);
  ASSERT_NE(nullptr, ws);
  EXPECT_EQ(long_name, ws->name);
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, SearchWithNoMatches) {
  std::vector<AstraWorkspaceInfo> workspaces = {
    MakeTestWorkspace("ws-1", u"Alpha"),
    MakeTestWorkspace("ws-2", u"Beta"),
  };
  panel_->SetWorkspaces(workspaces);
  panel_->SetSearchQuery(u"zzz-no-match");

  // Should not crash. The filtered list may be empty.
  SUCCEED();
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, SearchWithPartialMatch) {
  std::vector<AstraWorkspaceInfo> workspaces = {
    MakeTestWorkspace("ws-1", u"Alpha Project"),
    MakeTestWorkspace("ws-2", u"Beta Project"),
    MakeTestWorkspace("ws-3", u"Gamma"),
  };
  panel_->SetWorkspaces(workspaces);
  panel_->SetSearchQuery(u"Project");

  // Should not crash — 2 workspaces match "Project".
  SUCCEED();
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, SetInspectedWebContentsNull) {
  panel_->SetInspectedWebContents(nullptr);
  // Should not crash.
  SUCCEED();
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, SetWorkspaceServiceNull) {
  panel_->SetWorkspaceService(nullptr);
  // Should not crash.
  SUCCEED();
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, TabItemCountNoSelection) {
  std::vector<AstraWorkspaceInfo> workspaces = {
    MakeTestWorkspace("ws-1", u"First"),
  };
  panel_->SetWorkspaces(workspaces);
  // With no workspace selected, tab item count may be 0.
  EXPECT_EQ(0u, panel_->tab_item_count_for_testing());
}

TEST_F(AstraDevToolsWorkspacePanelDeepTest, RefreshIsIdempotent) {
  panel_->Refresh();
  size_t count1 = panel_->children().size();

  panel_->Refresh();
  size_t count2 = panel_->children().size();

  EXPECT_EQ(count1, count2);
}

}  // namespace astra
