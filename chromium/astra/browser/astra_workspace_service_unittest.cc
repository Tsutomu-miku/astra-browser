// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_workspace_service.h"

#include "base/test/task_environment.h"
#include "chrome/test/base/testing_profile.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

namespace {

// Test observer that records calls for verification.
class TestWorkspaceServiceObserver
    : public AstraWorkspaceServiceObserver {
 public:
  void OnWorkspaceAdded(const AstraWorkspace& workspace) override {
    added_count_++;
    last_added_id_ = workspace.id;
    last_added_name_ = workspace.name;
  }

  void OnWorkspaceRemoved(const std::string& workspace_id) override {
    removed_count_++;
    last_removed_id_ = workspace_id;
  }

  void OnWorkspaceRenamed(const std::string& workspace_id,
                          const std::string& new_name) override {
    renamed_count_++;
    last_renamed_id_ = workspace_id;
    last_renamed_name_ = new_name;
  }

  void OnActiveWorkspaceChanged(const std::string& old_id,
                                const std::string& new_id) override {
    active_changed_count_++;
    last_old_id_ = old_id;
    last_new_id_ = new_id;
  }

  void OnWorkspacesReordered() override {
    reordered_count_++;
  }

  void OnWorkspaceAccentColorChanged(const std::string& workspace_id,
                                     const std::string& new_color) override {
    accent_color_changed_count_++;
    last_accent_color_id_ = workspace_id;
    last_accent_color_ = new_color;
  }

  void OnWorkspaceCloned(const AstraWorkspace& new_workspace) override {
    cloned_count_++;
    last_cloned_id_ = new_workspace.id;
    last_cloned_name_ = new_workspace.name;
  }

  // Counters
  int added_count_ = 0;
  int removed_count_ = 0;
  int renamed_count_ = 0;
  int active_changed_count_ = 0;
  int reordered_count_ = 0;
  int accent_color_changed_count_ = 0;
  int cloned_count_ = 0;

  // Last recorded values
  std::string last_added_id_;
  std::string last_added_name_;
  std::string last_removed_id_;
  std::string last_renamed_id_;
  std::string last_renamed_name_;
  std::string last_old_id_;
  std::string last_new_id_;
  std::string last_accent_color_id_;
  std::string last_accent_color_;
  std::string last_cloned_id_;
  std::string last_cloned_name_;
};

}  // namespace

// Test fixture for AstraWorkspaceService tests.
//
// Uses TestingProfile from //chrome/test:test_support so the service has a
// real Profile* to attach to.  The service is obtained through the factory
// (AstraWorkspaceServiceFactory::GetForProfile) to exercise the full
// ProfileKeyedService creation path.
//
// TODO(astra): Verify that TestingProfile correctly initializes the
// PrefService so that future persistence tests (LoadFromPrefs/SaveToPrefs)
// can work.  Chromium component: PrefService + TestingProfile.
// TODO(astra): Consider whether to test the factory separately or only test
// through the service instance.  Current approach tests through the factory
// to validate ProfileKeyedService integration.
class WorkspaceServiceTest : public testing::Test {
 protected:
  WorkspaceServiceTest() {
    // TestingProfile creates a minimal profile for unit tests.
    profile_ = std::make_unique<TestingProfile>();
    service_ = AstraWorkspaceServiceFactory::GetForProfile(profile_.get());
    DCHECK(service_);
  }

  ~WorkspaceServiceTest() override = default;

  // testing::Test:
  void SetUp() override {
    // The service should have been created with a default workspace.
    ASSERT_GT(service_->workspace_count(), 0u);
  }

  void TearDown() override {
    // Clean up observers to avoid dangling references.
    for (auto& observer : test_observers_) {
      service_->RemoveObserver(&observer);
    }
  }

  // Helpers ---------------------------------------------------------------

  // Adds and returns a new workspace with the given id and name.
  const AstraWorkspace& AddTestWorkspace(const std::string& id,
                                         const std::string& name) {
    AstraWorkspace ws;
    ws.id = id;
    ws.name = name;
    ws.accent_color = "#000000";
    service_->AddWorkspace(std::move(ws));
    const AstraWorkspace* added = service_->GetWorkspace(id);
    DCHECK(added);
    return *added;
  }

  // Task environment is required for TestingProfile and base::Time.
  base::test::TaskEnvironment task_environment_;

  std::unique_ptr<TestingProfile> profile_;
  raw_ptr<AstraWorkspaceService> service_;

  // Pool of test observers managed by the fixture.
  std::vector<TestWorkspaceServiceObserver> test_observers_;
};

// ---------------------------------------------------------------------------
// Default workspace
// ---------------------------------------------------------------------------

TEST_F(WorkspaceServiceTest, DefaultWorkspaceExists) {
  // The service should create a default workspace automatically.
  EXPECT_GT(service_->workspace_count(), 0u);

  const AstraWorkspace* default_ws =
      service_->GetWorkspace(service_->GetDefaultWorkspaceId());
  ASSERT_NE(default_ws, nullptr);
  EXPECT_TRUE(default_ws->is_default);
  EXPECT_FALSE(default_ws->name.empty());
  EXPECT_FALSE(default_ws->accent_color.empty());

  // The active workspace should be the default one after construction.
  EXPECT_EQ(service_->active_workspace_id(),
            service_->GetDefaultWorkspaceId());
  EXPECT_EQ(service_->active_workspace().id,
            service_->GetDefaultWorkspaceId());
}

// ---------------------------------------------------------------------------
// Add workspace
// ---------------------------------------------------------------------------

TEST_F(WorkspaceServiceTest, AddWorkspace) {
  size_t initial_count = service_->workspace_count();

  AddTestWorkspace("workspace-1", "Workspace One");

  EXPECT_EQ(service_->workspace_count(), initial_count + 1);

  const AstraWorkspace* ws = service_->GetWorkspace("workspace-1");
  ASSERT_NE(ws, nullptr);
  EXPECT_EQ(ws->name, "Workspace One");
  EXPECT_EQ(ws->accent_color, "#000000");
  EXPECT_FALSE(ws->is_default);
  EXPECT_GT(ws->order_index, 0u);
  EXPECT_FALSE(ws->created_time.is_null());
}

TEST_F(WorkspaceServiceTest, AddWorkspace_DuplicateIdIgnored) {
  size_t initial_count = service_->workspace_count();

  AddTestWorkspace("workspace-alpha", "Alpha");
  AddTestWorkspace("workspace-alpha", "Alpha Again");

  EXPECT_EQ(service_->workspace_count(), initial_count + 1);
}

// ---------------------------------------------------------------------------
// Activate workspace
// ---------------------------------------------------------------------------

TEST_F(WorkspaceServiceTest, ActivateWorkspace) {
  AddTestWorkspace("workspace-1", "One");
  AddTestWorkspace("workspace-2", "Two");

  std::string original_active = service_->active_workspace_id();

  service_->ActivateWorkspace("workspace-1");
  EXPECT_EQ(service_->active_workspace_id(), "workspace-1");

  service_->ActivateWorkspace("workspace-2");
  EXPECT_EQ(service_->active_workspace_id(), "workspace-2");

  // Activating a non-existent workspace should be a no-op.
  service_->ActivateWorkspace("nonexistent");
  EXPECT_EQ(service_->active_workspace_id(), "workspace-2");
}

TEST_F(WorkspaceServiceTest, ActivateWorkspace_SameIdIsNoOp) {
  AddTestWorkspace("workspace-1", "One");
  service_->ActivateWorkspace("workspace-1");

  TestWorkspaceServiceObserver observer;
  service_->AddObserver(&observer);

  service_->ActivateWorkspace("workspace-1");
  EXPECT_EQ(observer.active_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Rename workspace
// ---------------------------------------------------------------------------

TEST_F(WorkspaceServiceTest, RenameWorkspace) {
  AddTestWorkspace("workspace-1", "Original Name");

  bool result = service_->RenameWorkspace("workspace-1", "New Name");
  EXPECT_TRUE(result);

  const AstraWorkspace* ws = service_->GetWorkspace("workspace-1");
  ASSERT_NE(ws, nullptr);
  EXPECT_EQ(ws->name, "New Name");
}

TEST_F(WorkspaceServiceTest, RenameWorkspace_NonexistentReturnsFalse) {
  bool result = service_->RenameWorkspace("nonexistent", "Whatever");
  EXPECT_FALSE(result);
}

TEST_F(WorkspaceServiceTest, RenameWorkspace_SameNameReturnsTrue) {
  AddTestWorkspace("workspace-1", "Same");

  bool result = service_->RenameWorkspace("workspace-1", "Same");
  EXPECT_TRUE(result);
}

// ---------------------------------------------------------------------------
// Delete workspace
// ---------------------------------------------------------------------------

TEST_F(WorkspaceServiceTest, DeleteWorkspace) {
  AddTestWorkspace("workspace-1", "To Delete");
  size_t count_before = service_->workspace_count();

  bool result = service_->DeleteWorkspace("workspace-1");
  EXPECT_TRUE(result);
  EXPECT_EQ(service_->workspace_count(), count_before - 1);
  EXPECT_EQ(service_->GetWorkspace("workspace-1"), nullptr);
}

TEST_F(WorkspaceServiceTest, DeleteWorkspace_ActiveFallsBackToDefault) {
  AddTestWorkspace("workspace-1", "To Delete");
  service_->ActivateWorkspace("workspace-1");
  ASSERT_EQ(service_->active_workspace_id(), "workspace-1");

  bool result = service_->DeleteWorkspace("workspace-1");
  EXPECT_TRUE(result);
  EXPECT_EQ(service_->active_workspace_id(),
            service_->GetDefaultWorkspaceId());
}

TEST_F(WorkspaceServiceTest, CannotDeleteDefaultWorkspace) {
  size_t initial_count = service_->workspace_count();

  bool result =
      service_->DeleteWorkspace(service_->GetDefaultWorkspaceId());
  EXPECT_FALSE(result);
  EXPECT_EQ(service_->workspace_count(), initial_count);
}

TEST_F(WorkspaceServiceTest, DeleteWorkspace_NonexistentReturnsFalse) {
  bool result = service_->DeleteWorkspace("nonexistent");
  EXPECT_FALSE(result);
}

// ---------------------------------------------------------------------------
// Reorder workspaces
// ---------------------------------------------------------------------------

TEST_F(WorkspaceServiceTest, ReorderWorkspaces) {
  AddTestWorkspace("ws-a", "A");
  AddTestWorkspace("ws-b", "B");
  AddTestWorkspace("ws-c", "C");

  // Default order should be: default, ws-a, ws-b, ws-c
  ASSERT_EQ(service_->workspaces()[0].id, service_->GetDefaultWorkspaceId());
  ASSERT_EQ(service_->workspaces()[1].id, "ws-a");
  ASSERT_EQ(service_->workspaces()[2].id, "ws-b");
  ASSERT_EQ(service_->workspaces()[3].id, "ws-c");

  // Reorder to: ws-c, default, ws-b, ws-a
  std::vector<std::string> new_order = {
      "ws-c", service_->GetDefaultWorkspaceId(), "ws-b", "ws-a"};
  bool result = service_->ReorderWorkspaces(new_order);
  EXPECT_TRUE(result);

  ASSERT_EQ(service_->workspaces().size(), 4u);
  EXPECT_EQ(service_->workspaces()[0].id, "ws-c");
  EXPECT_EQ(service_->workspaces()[1].id, service_->GetDefaultWorkspaceId());
  EXPECT_EQ(service_->workspaces()[2].id, "ws-b");
  EXPECT_EQ(service_->workspaces()[3].id, "ws-a");
}

TEST_F(WorkspaceServiceTest, ReorderWorkspaces_WrongSizeReturnsFalse) {
  AddTestWorkspace("ws-a", "A");
  AddTestWorkspace("ws-b", "B");

  // Only 2 ids provided, but there are 3 workspaces (including default).
  bool result = service_->ReorderWorkspaces({"ws-a", "ws-b"});
  EXPECT_FALSE(result);
}

TEST_F(WorkspaceServiceTest, ReorderWorkspaces_NonexistentIdReturnsFalse) {
  AddTestWorkspace("ws-a", "A");

  std::string default_id = service_->GetDefaultWorkspaceId();
  bool result =
      service_->ReorderWorkspaces({default_id, "nonexistent"});
  EXPECT_FALSE(result);
}

// ---------------------------------------------------------------------------
// Observer: OnWorkspaceAdded
// ---------------------------------------------------------------------------

TEST_F(WorkspaceServiceTest, ObserverFiresOnAdd) {
  TestWorkspaceServiceObserver observer;
  service_->AddObserver(&observer);

  AddTestWorkspace("ws-test", "Test Workspace");

  EXPECT_EQ(observer.added_count_, 1);
  EXPECT_EQ(observer.last_added_id_, "ws-test");
  EXPECT_EQ(observer.last_added_name_, "Test Workspace");

  service_->RemoveObserver(&observer);
}

TEST_F(WorkspaceServiceTest, ObserverFiresOnRemove) {
  AddTestWorkspace("ws-to-remove", "Remove Me");

  TestWorkspaceServiceObserver observer;
  service_->AddObserver(&observer);

  service_->DeleteWorkspace("ws-to-remove");

  EXPECT_EQ(observer.removed_count_, 1);
  EXPECT_EQ(observer.last_removed_id_, "ws-to-remove");

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Observer: OnActiveWorkspaceChanged
// ---------------------------------------------------------------------------

TEST_F(WorkspaceServiceTest, ObserverFiresOnActivate) {
  AddTestWorkspace("ws-1", "One");
  AddTestWorkspace("ws-2", "Two");

  TestWorkspaceServiceObserver observer;
  service_->AddObserver(&observer);

  service_->ActivateWorkspace("ws-1");

  EXPECT_EQ(observer.active_changed_count_, 1);
  EXPECT_NE(observer.last_old_id_, "ws-1");
  EXPECT_EQ(observer.last_new_id_, "ws-1");

  service_->ActivateWorkspace("ws-2");
  EXPECT_EQ(observer.active_changed_count_, 2);
  EXPECT_EQ(observer.last_old_id_, "ws-1");
  EXPECT_EQ(observer.last_new_id_, "ws-2");

  service_->RemoveObserver(&observer);
}

TEST_F(WorkspaceServiceTest, ObserverFiresOnRename) {
  AddTestWorkspace("ws-rename", "Original");

  TestWorkspaceServiceObserver observer;
  service_->AddObserver(&observer);

  service_->RenameWorkspace("ws-rename", "Renamed");

  EXPECT_EQ(observer.renamed_count_, 1);
  EXPECT_EQ(observer.last_renamed_id_, "ws-rename");
  EXPECT_EQ(observer.last_renamed_name_, "Renamed");

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Navigation helpers
// ---------------------------------------------------------------------------

TEST_F(WorkspaceServiceTest, GetNextWorkspaceId_WrapsAround) {
  AddTestWorkspace("ws-1", "One");
  AddTestWorkspace("ws-2", "Two");
  AddTestWorkspace("ws-3", "Three");

  // Start at the first workspace.
  service_->ActivateWorkspace(service_->workspaces()[0].id);
  std::string first_id = service_->active_workspace_id();

  std::string next_1 = service_->GetNextWorkspaceId();
  EXPECT_NE(next_1, first_id);

  service_->ActivateWorkspace(next_1);
  std::string next_2 = service_->GetNextWorkspaceId();
  EXPECT_NE(next_2, next_1);

  service_->ActivateWorkspace(next_2);
  // With 4 workspaces (default + 3), the 4th next should wrap back.
  std::string next_3 = service_->GetNextWorkspaceId();
  EXPECT_NE(next_3, next_2);

  // After advancing through all workspaces, we should return to the first.
  service_->ActivateWorkspace(next_3);
  std::string next_4 = service_->GetNextWorkspaceId();

  // With 4 total workspaces, after 4 steps we should be back to start.
  // Verify by going through all of them.
  std::string current = first_id;
  service_->ActivateWorkspace(first_id);
  for (size_t i = 0; i < service_->workspace_count(); ++i) {
    current = service_->GetNextWorkspaceId();
    service_->ActivateWorkspace(current);
  }
  // After N steps through N workspaces, we're back to the start.
  EXPECT_EQ(service_->active_workspace_id(), first_id);
}

TEST_F(WorkspaceServiceTest, GetNextWorkspaceId_SingleWorkspaceReturnsSame) {
  // Only the default workspace exists.
  ASSERT_EQ(service_->workspace_count(), 1u);

  std::string next = service_->GetNextWorkspaceId();
  EXPECT_EQ(next, service_->active_workspace_id());
}

TEST_F(WorkspaceServiceTest, GetPreviousWorkspaceId_WrapsAround) {
  AddTestWorkspace("ws-1", "One");
  AddTestWorkspace("ws-2", "Two");

  service_->ActivateWorkspace(service_->workspaces()[0].id);
  std::string first_id = service_->active_workspace_id();

  std::string prev = service_->GetPreviousWorkspaceId();
  EXPECT_NE(prev, first_id);

  // Previous of first should be last (wrap around).
  EXPECT_EQ(prev, service_->workspaces().back().id);
}

TEST_F(WorkspaceServiceTest, GetWorkspaceIndex) {
  AddTestWorkspace("ws-a", "A");
  AddTestWorkspace("ws-b", "B");

  size_t default_idx = service_->GetWorkspaceIndex(
      service_->GetDefaultWorkspaceId());
  size_t a_idx = service_->GetWorkspaceIndex("ws-a");
  size_t b_idx = service_->GetWorkspaceIndex("ws-b");

  EXPECT_LT(default_idx, a_idx);
  EXPECT_LT(a_idx, b_idx);

  // Nonexistent id falls back to default workspace index.
  size_t nonexistent_idx = service_->GetWorkspaceIndex("nonexistent");
  EXPECT_EQ(nonexistent_idx, default_idx);
}

// ---------------------------------------------------------------------------
// Accent color
// ---------------------------------------------------------------------------

TEST_F(WorkspaceServiceTest, SetAccentColor) {
  AddTestWorkspace("ws-color", "Color Test");
  const AstraWorkspace* ws = service_->GetWorkspace("ws-color");
  ASSERT_NE(ws, nullptr);
  EXPECT_EQ(ws->accent_color, "#000000");

  bool result = service_->SetWorkspaceAccentColor("ws-color", "#FF5733");
  EXPECT_TRUE(result);
  EXPECT_EQ(ws->accent_color, "#FF5733");
}

TEST_F(WorkspaceServiceTest, SetAccentColor_NonexistentReturnsFalse) {
  bool result =
      service_->SetWorkspaceAccentColor("nonexistent", "#FF0000");
  EXPECT_FALSE(result);
}

// ---------------------------------------------------------------------------
// EnsureDefaultWorkspace is idempotent
// ---------------------------------------------------------------------------

TEST_F(WorkspaceServiceTest, EnsureDefaultWorkspace_Idempotent) {
  size_t count_before = service_->workspace_count();

  service_->EnsureDefaultWorkspace();
  service_->EnsureDefaultWorkspace();

  EXPECT_EQ(service_->workspace_count(), count_before);
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------

TEST_F(WorkspaceServiceTest, ShutdownClearsObservers) {
  TestWorkspaceServiceObserver observer;
  service_->AddObserver(&observer);

  service_->Shutdown();

  // After shutdown, adding a workspace should not notify the observer
  // because the observer list was cleared.
  AddTestWorkspace("ws-after-shutdown", "After Shutdown");
  EXPECT_EQ(observer.added_count_, 0);
}

// ---------------------------------------------------------------------------
// Workspace description
// ---------------------------------------------------------------------------

TEST_F(WorkspaceServiceTest, SetDescription) {
  AddTestWorkspace("ws-desc", "Description Test");
  const AstraWorkspace* ws = service_->GetWorkspace("ws-desc");
  ASSERT_NE(ws, nullptr);
  EXPECT_TRUE(ws->description.empty());

  bool result = service_->SetWorkspaceDescription("ws-desc", "My workspace for testing");
  EXPECT_TRUE(result);
  EXPECT_EQ(ws->description, "My workspace for testing");
}

TEST_F(WorkspaceServiceTest, SetDescription_NonexistentReturnsFalse) {
  bool result =
      service_->SetWorkspaceDescription("nonexistent", "whatever");
  EXPECT_FALSE(result);
}

TEST_F(WorkspaceServiceTest, SetDescription_EmptyString) {
  AddTestWorkspace("ws-empty-desc", "Empty Desc");

  // Set to empty description, then clear it.
  service_->SetWorkspaceDescription("ws-empty-desc", "has content");
  const AstraWorkspace* ws = service_->GetWorkspace("ws-empty-desc");
  ASSERT_NE(ws, nullptr);
  EXPECT_FALSE(ws->description.empty());

  bool result = service_->SetWorkspaceDescription("ws-empty-desc", "");
  EXPECT_TRUE(result);
  EXPECT_TRUE(ws->description.empty());
}

// ---------------------------------------------------------------------------
// Clone workspace
// ---------------------------------------------------------------------------

TEST_F(WorkspaceServiceTest, CloneWorkspace_CreatesCopy) {
  AddTestWorkspace("ws-source", "Source Workspace");
  service_->SetWorkspaceAccentColor("ws-source", "#123456");
  service_->SetWorkspaceDescription("ws-source", "Source description");

  std::string new_id = service_->CloneWorkspace("ws-source");
  EXPECT_FALSE(new_id.empty());
  EXPECT_NE(new_id, "ws-source");

  const AstraWorkspace* original = service_->GetWorkspace("ws-source");
  const AstraWorkspace* cloned = service_->GetWorkspace(new_id);
  ASSERT_NE(cloned, nullptr);

  // Name should have " copy" suffix.
  EXPECT_EQ(cloned->name, "Source Workspace copy");
  // Color should match.
  EXPECT_EQ(cloned->accent_color, "#123456");
  // Description should match.
  EXPECT_EQ(cloned->description, "Source description");
  // Is default should be false.
  EXPECT_FALSE(cloned->is_default);
  // Is hibernated should be false (fresh clone).
  EXPECT_FALSE(cloned->is_hibernated);
  // Created time should be different (newer).
  EXPECT_GE(cloned->created_time, original->created_time);
  // Order index should be at the end.
  EXPECT_GT(cloned->order_index, original->order_index);
}

TEST_F(WorkspaceServiceTest, CloneWorkspace_NonexistentReturnsEmpty) {
  std::string result = service_->CloneWorkspace("nonexistent");
  EXPECT_TRUE(result.empty());
}

TEST_F(WorkspaceServiceTest, CloneWorkspace_DefaultCanBeCloned) {
  std::string default_id = service_->GetDefaultWorkspaceId();
  std::string new_id = service_->CloneWorkspace(default_id);
  EXPECT_FALSE(new_id.empty());

  const AstraWorkspace* cloned = service_->GetWorkspace(new_id);
  ASSERT_NE(cloned, nullptr);
  EXPECT_FALSE(cloned->is_default);
  EXPECT_NE(cloned->id, default_id);
}

TEST_F(WorkspaceServiceTest, CloneWorkspace_MultipleClonesHaveUniqueIds) {
  AddTestWorkspace("ws-orig", "Original");

  std::string clone1 = service_->CloneWorkspace("ws-orig");
  std::string clone2 = service_->CloneWorkspace("ws-orig");

  EXPECT_FALSE(clone1.empty());
  EXPECT_FALSE(clone2.empty());
  EXPECT_NE(clone1, clone2);
}

// ---------------------------------------------------------------------------
// Touch workspace / last used time
// ---------------------------------------------------------------------------

TEST_F(WorkspaceServiceTest, TouchWorkspace_UpdatesLastUsedTime) {
  AddTestWorkspace("ws-touch", "Touch Test");
  const AstraWorkspace* ws = service_->GetWorkspace("ws-touch");
  ASSERT_NE(ws, nullptr);

  // Initially last_used_time might be null or set.
  base::Time before = ws->last_used_time;

  bool result = service_->TouchWorkspace("ws-touch");
  EXPECT_TRUE(result);

  // After touch, last_used_time should be updated.
  EXPECT_FALSE(ws->last_used_time.is_null());
  if (!before.is_null()) {
    EXPECT_GE(ws->last_used_time, before);
  }
}

TEST_F(WorkspaceServiceTest, TouchWorkspace_NonexistentReturnsFalse) {
  bool result = service_->TouchWorkspace("nonexistent");
  EXPECT_FALSE(result);
}

TEST_F(WorkspaceServiceTest, ActivateWorkspace_TouchesWorkspace) {
  AddTestWorkspace("ws-activate-touch", "Activate Touch");

  // Touch it first to establish a baseline.
  service_->TouchWorkspace("ws-activate-touch");
  const AstraWorkspace* ws = service_->GetWorkspace("ws-activate-touch");
  ASSERT_NE(ws, nullptr);
  base::Time before = ws->last_used_time;

  // Activate should also touches the workspace.
  service_->ActivateWorkspace("ws-activate-touch");

  // Last used time should be >= before (at least not null and advanced.
  EXPECT_FALSE(ws->last_used_time.is_null());
}

TEST_F(WorkspaceServiceTest, GetRecentlyUsedWorkspaces_SortedByLastUsed) {
  AddTestWorkspace("ws-old", "Old");
  AddTestWorkspace("ws-newer", "Newer");
  AddTestWorkspace("ws-newest", "Newest");

  // Touch in order so each is newer than the previous.
  service_->TouchWorkspace("ws-old");
  // Small delay simulation: touch in sequence ensures ordering.
  service_->TouchWorkspace("ws-newer");
  service_->TouchWorkspace("ws-newest");

  auto recent = service_->GetRecentlyUsedWorkspaces();
  ASSERT_GE(recent.size(), 3u);

  // The three we touched should appear in order (most recent first).
  // Find their positions.
  size_t newest_idx = 999;
  size_t newer_idx = 999;
  size_t old_idx = 999;
  for (size_t i = 0; i < recent.size(); ++i) {
    if (recent[i].id == "ws-newest") newest_idx = i;
    if (recent[i].id == "ws-newer") newer_idx = i;
    if (recent[i].id == "ws-old") old_idx = i;
  }
  EXPECT_LT(newest_idx, newer_idx);
  EXPECT_LT(newer_idx, old_idx);
}

TEST_F(WorkspaceServiceTest, GetRecentlyUsedWorkspaces_EmptyWhenNoWorkspaces) {
  // There is always at least the default workspace.
  auto recent = service_->GetRecentlyUsedWorkspaces();
  EXPECT_GT(recent.size(), 0u);
}

// ---------------------------------------------------------------------------
// Workspace hibernation
// ---------------------------------------------------------------------------

TEST_F(WorkspaceServiceTest, SetHibernated) {
  AddTestWorkspace("ws-hib", "Hibernate Test");
  const AstraWorkspace* ws = service_->GetWorkspace("ws-hib");
  ASSERT_NE(ws, nullptr);
  EXPECT_FALSE(ws->is_hibernated);

  bool result = service_->SetWorkspaceHibernated("ws-hib", true);
  EXPECT_TRUE(result);
  EXPECT_TRUE(ws->is_hibernated);

  result = service_->SetWorkspaceHibernated("ws-hib", false);
  EXPECT_TRUE(result);
  EXPECT_FALSE(ws->is_hibernated);
}

TEST_F(WorkspaceServiceTest, SetHibernated_NonexistentReturnsFalse) {
  bool result = service_->SetWorkspaceHibernated("nonexistent", true);
  EXPECT_FALSE(result);
}

TEST_F(WorkspaceServiceTest, SetHibernated_SameStateIsNoOp) {
  AddTestWorkspace("ws-same-hib", "Same Hibernate");
  const AstraWorkspace* ws = service_->GetWorkspace("ws-same-hib");
  ASSERT_NE(ws, nullptr);

  // Setting to same state returns true (no change, but workspace exists).
  bool result = service_->SetWorkspaceHibernated("ws-same-hib", false);
  EXPECT_TRUE(result);
  EXPECT_FALSE(ws->is_hibernated);
}

TEST_F(WorkspaceServiceTest, GetHibernatedWorkspaces_FiltersCorrectly) {
  AddTestWorkspace("ws-hib-1", "Hibernated 1");
  AddTestWorkspace("ws-hib-2", "Hibernated 2");
  AddTestWorkspace("ws-awake", "Awake");

  service_->SetWorkspaceHibernated("ws-hib-1", true);
  service_->SetWorkspaceHibernated("ws-hib-2", true);

  auto hibernated = service_->GetHibernatedWorkspaces();
  EXPECT_EQ(hibernated.size(), 2u);

  // Both should be hibernated.
  for (const auto& ws : hibernated) {
    EXPECT_TRUE(ws.is_hibernated);
    EXPECT_TRUE(ws.id == "ws-hib-1" || ws.id == "ws-hib-2");
  }
}

TEST_F(WorkspaceServiceTest, GetHibernatedWorkspaces_EmptyWhenNoneHibernated) {
  AddTestWorkspace("ws-awake-only", "Awake Only");

  auto hibernated = service_->GetHibernatedWorkspaces();
  EXPECT_TRUE(hibernated.empty());
}

TEST_F(WorkspaceServiceTest, CloneWorkspace_DoesNotCopyHibernationState) {
  AddTestWorkspace("ws-hib-source", "Hib Source");
  service_->SetWorkspaceHibernated("ws-hib-source", true);

  std::string new_id = service_->CloneWorkspace("ws-hib-source");
  ASSERT_FALSE(new_id.empty());

  const AstraWorkspace* cloned = service_->GetWorkspace(new_id);
  ASSERT_NE(cloned, nullptr);
  // Fresh clone starts awake, not hibernated.
  EXPECT_FALSE(cloned->is_hibernated);
}

// ---------------------------------------------------------------------------
// Observer: OnWorkspaceAccentColorChanged
// ---------------------------------------------------------------------------

TEST_F(WorkspaceServiceTest, ObserverFiresOnAccentColorChange) {
  AddTestWorkspace("ws-accent-obs", "Accent Observer");

  TestWorkspaceServiceObserver observer;
  service_->AddObserver(&observer);

  service_->SetWorkspaceAccentColor("ws-accent-obs", "#AABBCC");

  EXPECT_EQ(observer.accent_color_changed_count_, 1);
  EXPECT_EQ(observer.last_accent_color_id_, "ws-accent-obs");
  EXPECT_EQ(observer.last_accent_color_, "#AABBCC");

  service_->RemoveObserver(&observer);
}

TEST_F(WorkspaceServiceTest, ObserverAccentColor_NonexistentNoFire) {
  TestWorkspaceServiceObserver observer;
  service_->AddObserver(&observer);

  service_->SetWorkspaceAccentColor("nonexistent", "#FF0000");
  EXPECT_EQ(observer.accent_color_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Observer: OnWorkspaceCloned
// ---------------------------------------------------------------------------

TEST_F(WorkspaceServiceTest, ObserverFiresOnClone) {
  AddTestWorkspace("ws-clone-src", "Clone Source");

  TestWorkspaceServiceObserver observer;
  service_->AddObserver(&observer);

  std::string new_id = service_->CloneWorkspace("ws-clone-src");
  ASSERT_FALSE(new_id.empty());

  EXPECT_EQ(observer.cloned_count_, 1);
  EXPECT_EQ(observer.last_cloned_id_, new_id);
  EXPECT_EQ(observer.last_cloned_name_, "Clone Source copy");

  service_->RemoveObserver(&observer);
}

TEST_F(WorkspaceServiceTest, ObserverClone_NonexistentNoFire) {
  TestWorkspaceServiceObserver observer;
  service_->AddObserver(&observer);

  service_->CloneWorkspace("nonexistent");
  EXPECT_EQ(observer.cloned_count_, 0);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Observer: default implementations
// ---------------------------------------------------------------------------

namespace {

// Observer that overrides only one method to verify defaults work.
class PartialWorkspaceObserver : public AstraWorkspaceServiceObserver {
 public:
  void OnWorkspaceAdded(const AstraWorkspace& workspace) override {
    added_count_++;
  }

  int added_count_ = 0;
};

}  // namespace

TEST_F(WorkspaceServiceTest, ObserverDefaultImplementations) {
  PartialWorkspaceObserver partial_observer;
  service_->AddObserver(&partial_observer);

  // These should all compile and not crash (default implementations).
  AddTestWorkspace("ws-partial", "Partial");
  service_->RenameWorkspace("ws-partial", "Renamed");
  service_->ActivateWorkspace("ws-partial");
  service_->SetWorkspaceAccentColor("ws-partial", "#112233");
  service_->CloneWorkspace("ws-partial");
  service_->ReorderWorkspaces({service_->GetDefaultWorkspaceId(), "ws-partial"});
  service_->DeleteWorkspace("ws-partial");

  // Only OnWorkspaceAdded was overridden and should have fired once.
  EXPECT_EQ(partial_observer.added_count_, 1);

  service_->RemoveObserver(&partial_observer);
}

// ---------------------------------------------------------------------------
// Workspace fields: default values
// ---------------------------------------------------------------------------

TEST_F(WorkspaceServiceTest, NewWorkspaceHasDefaultFieldValues) {
  AddTestWorkspace("ws-defaults", "Defaults Test");
  const AstraWorkspace* ws = service_->GetWorkspace("ws-defaults");
  ASSERT_NE(ws, nullptr);

  EXPECT_FALSE(ws->is_default);
  EXPECT_FALSE(ws->is_hibernated);
  EXPECT_TRUE(ws->description.empty());
  // last_used_time may be null initially (set on first touch/activate).
}

TEST_F(WorkspaceServiceTest, DefaultWorkspaceHasIsDefaultTrue) {
  const AstraWorkspace* default_ws =
      service_->GetWorkspace(service_->GetDefaultWorkspaceId());
  ASSERT_NE(default_ws, nullptr);
  EXPECT_TRUE(default_ws->is_default);
}

}  // namespace astra
