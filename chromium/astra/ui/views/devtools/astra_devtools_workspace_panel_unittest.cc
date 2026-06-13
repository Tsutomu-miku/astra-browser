// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/devtools/astra_devtools_workspace_panel.h"

#include "base/memory/raw_ptr.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/controls/label.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

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

}  // namespace astra
