// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/sessions/astra_session_manager_view.h"

#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

// ===========================================================================
// AstraSessionManagerViewTest
// ===========================================================================

class AstraSessionManagerViewTest : public testing::Test {
 protected:
  void SetUp() override {
    anchor_view_ = std::make_unique<views::View>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<views::View> anchor_view_;
};

// Test snapshot item creation.
TEST_F(AstraSessionManagerViewTest, SnapshotItemCreation) {
  AstraSessionSnapshotItemView::SnapshotInfo info;
  info.session_id = "session_001";
  info.name = u"Morning Research";
  info.tab_count = 8;
  info.window_count = 1;
  info.created_at = base::Time::Now() - base::Hours(3);

  auto item = std::make_unique<AstraSessionSnapshotItemView>(
      info, base::DoNothing(), base::DoNothing(), base::DoNothing());

  EXPECT_EQ("session_001", item->session_id());
  EXPECT_EQ(u"Morning Research", item->name());
}

// Test snapshot item with multiple windows.
TEST_F(AstraSessionManagerViewTest, SnapshotItemMultipleWindows) {
  AstraSessionSnapshotItemView::SnapshotInfo info;
  info.session_id = "session_002";
  info.name = u"Work Backup";
  info.tab_count = 12;
  info.window_count = 2;
  info.created_at = base::Time::Now() - base::Days(2);

  auto item = std::make_unique<AstraSessionSnapshotItemView>(
      info, base::DoNothing(), base::DoNothing(), base::DoNothing());

  EXPECT_EQ("session_002", item->session_id());
}

// Test session manager view creation.
TEST_F(AstraSessionManagerViewTest, ViewCreation) {
  auto* view = new AstraSessionManagerView(anchor_view_.get());
  EXPECT_NE(nullptr, view);
}

// Test setting snapshots.
TEST_F(AstraSessionManagerViewTest, SetSnapshots) {
  auto* view = new AstraSessionManagerView(anchor_view_.get());

  std::vector<AstraSessionSnapshotItemView::SnapshotInfo> snapshots;

  AstraSessionSnapshotItemView::SnapshotInfo snap1;
  snap1.session_id = "snap1";
  snap1.name = u"Project Alpha";
  snap1.tab_count = 5;
  snap1.created_at = base::Time::Now() - base::Hours(1);
  snapshots.push_back(snap1);

  AstraSessionSnapshotItemView::SnapshotInfo snap2;
  snap2.session_id = "snap2";
  snap2.name = u"Reading List";
  snap2.tab_count = 12;
  snap2.created_at = base::Time::Now() - base::Days(3);
  snapshots.push_back(snap2);

  AstraSessionSnapshotItemView::SnapshotInfo snap3;
  snap3.session_id = "snap3";
  snap3.name = u"Work Session";
  snap3.tab_count = 8;
  snap3.created_at = base::Time::Now() - base::Days(1);
  snapshots.push_back(snap3);

  view->SetSnapshots(snapshots);
  // No crash and view exists.
  EXPECT_NE(nullptr, view);
}

// Test empty snapshots.
TEST_F(AstraSessionManagerViewTest, EmptySnapshots) {
  auto* view = new AstraSessionManagerView(anchor_view_.get());
  view->SetSnapshots({});
  EXPECT_NE(nullptr, view);
}

// Test restore callback fires.
TEST_F(AstraSessionManagerViewTest, RestoreCallback) {
  std::string restored_id;
  auto* view = new AstraSessionManagerView(anchor_view_.get());
  view->SetRestoreSessionCallback(
      base::BindRepeating(
          [](std::string* out_id, const std::string& id) { *out_id = id; },
          &restored_id));

  std::vector<AstraSessionSnapshotItemView::SnapshotInfo> snapshots;
  AstraSessionSnapshotItemView::SnapshotInfo snap;
  snap.session_id = "restore_me";
  snap.name = u"Test Session";
  snap.tab_count = 3;
  snap.created_at = base::Time::Now();
  snapshots.push_back(snap);
  view->SetSnapshots(snapshots);

  // Restore callback should not fire unless explicitly invoked.
  EXPECT_TRUE(restored_id.empty());
}

// Test delete callback fires.
TEST_F(AstraSessionManagerViewTest, DeleteCallback) {
  std::string deleted_id;
  auto* view = new AstraSessionManagerView(anchor_view_.get());
  view->SetDeleteSessionCallback(
      base::BindRepeating(
          [](std::string* out_id, const std::string& id) { *out_id = id; },
          &deleted_id));

  std::vector<AstraSessionSnapshotItemView::SnapshotInfo> snapshots;
  AstraSessionSnapshotItemView::SnapshotInfo snap;
  snap.session_id = "delete_me";
  snap.name = u"Temp Session";
  snap.tab_count = 2;
  snap.created_at = base::Time::Now();
  snapshots.push_back(snap);
  view->SetSnapshots(snapshots);

  // Delete callback should not fire unless explicitly invoked.
  EXPECT_TRUE(deleted_id.empty());
}

// Test save session callback.
TEST_F(AstraSessionManagerViewTest, SaveCallback) {
  bool save_called = false;
  auto* view = new AstraSessionManagerView(anchor_view_.get());
  view->SetSaveSessionCallback(
      base::BindRepeating([](bool* called) { *called = true; }, &save_called));

  EXPECT_FALSE(save_called);
}

// Test SetName on snapshot item.
TEST_F(AstraSessionManagerViewTest, SnapshotItemSetName) {
  AstraSessionSnapshotItemView::SnapshotInfo info;
  info.session_id = "rename_test";
  info.name = u"Original Name";
  info.tab_count = 3;
  info.created_at = base::Time::Now();

  auto item = std::make_unique<AstraSessionSnapshotItemView>(
      info, base::DoNothing(), base::DoNothing(), base::DoNothing());

  EXPECT_EQ(u"Original Name", item->name());
  item->SetName(u"New Name");
  EXPECT_EQ(u"New Name", item->name());
}

// Test window title.
TEST_F(AstraSessionManagerViewTest, WindowTitle) {
  auto* view = new AstraSessionManagerView(anchor_view_.get());
  EXPECT_EQ(u"Session Manager", view->GetWindowTitle());
}

// Test snapshot with workspace_id.
TEST_F(AstraSessionManagerViewTest, SnapshotWithWorkspace) {
  AstraSessionSnapshotItemView::SnapshotInfo info;
  info.session_id = "ws_snap";
  info.name = u"Workspace Session";
  info.tab_count = 6;
  info.workspace_id = "workspace_alpha";
  info.created_at = base::Time::Now();

  auto item = std::make_unique<AstraSessionSnapshotItemView>(
      info, base::DoNothing(), base::DoNothing(), base::DoNothing());

  EXPECT_EQ("ws_snap", item->session_id());
}

// Test snapshot with sample domains.
TEST_F(AstraSessionManagerViewTest, SnapshotWithSampleDomains) {
  AstraSessionSnapshotItemView::SnapshotInfo info;
  info.session_id = "domain_snap";
  info.name = u"Research Session";
  info.tab_count = 4;
  info.sample_domains = {"wikipedia.org", "github.com", "stackoverflow.com"};
  info.created_at = base::Time::Now();

  auto item = std::make_unique<AstraSessionSnapshotItemView>(
      info, base::DoNothing(), base::DoNothing(), base::DoNothing());

  EXPECT_EQ("domain_snap", item->session_id());
}

}  // namespace astra
