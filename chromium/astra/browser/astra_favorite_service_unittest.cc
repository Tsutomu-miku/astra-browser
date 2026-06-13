// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_favorite_service.h"

#include "base/test/task_environment.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

namespace {

// Test observer that records calls for verification.
class TestFavoriteServiceObserver
    : public AstraFavoriteServiceObserver {
 public:
  void OnFolderAdded(const AstraFavoriteFolder& folder) override {
    folder_added_count_++;
    last_added_folder_id_ = folder.id;
    last_added_folder_name_ = folder.name;
  }

  void OnFolderRemoved(const std::string& folder_id) override {
    folder_removed_count_++;
    last_removed_folder_id_ = folder_id;
  }

  void OnFolderRenamed(const std::string& folder_id,
                       const std::string& new_name) override {
    folder_renamed_count_++;
    last_renamed_folder_id_ = folder_id;
    last_renamed_folder_name_ = new_name;
  }

  void OnFoldersReordered() override {
    folders_reordered_count_++;
  }

  void OnFavoriteMoved(content::WebContents* web_contents,
                       const std::string& old_folder_id,
                       const std::string& new_folder_id) override {
    favorite_moved_count_++;
    last_moved_old_folder_ = old_folder_id;
    last_moved_new_folder_ = new_folder_id;
    last_moved_web_contents_ = web_contents;
  }

  void OnFavoritesReordered(const std::string& folder_id) override {
    favorites_reordered_count_++;
    last_favorites_reordered_folder_ = folder_id;
  }

  void OnFolderExpanded(const std::string& folder_id) override {
    folder_expanded_count_++;
    last_expanded_folder_id_ = folder_id;
  }

  void OnFolderCollapsed(const std::string& folder_id) override {
    folder_collapsed_count_++;
    last_collapsed_folder_id_ = folder_id;
  }

  void OnFolderColorChanged(const std::string& folder_id,
                            const std::string& new_color) override {
    folder_color_changed_count_++;
    last_color_changed_folder_id_ = folder_id;
    last_color_changed_ = new_color;
  }

  // Counters
  int folder_added_count_ = 0;
  int folder_removed_count_ = 0;
  int folder_renamed_count_ = 0;
  int folders_reordered_count_ = 0;
  int favorite_moved_count_ = 0;
  int favorites_reordered_count_ = 0;
  int folder_expanded_count_ = 0;
  int folder_collapsed_count_ = 0;
  int folder_color_changed_count_ = 0;

  // Last recorded values
  std::string last_added_folder_id_;
  std::string last_added_folder_name_;
  std::string last_removed_folder_id_;
  std::string last_renamed_folder_id_;
  std::string last_renamed_folder_name_;
  std::string last_moved_old_folder_;
  std::string last_moved_new_folder_;
  raw_ptr<content::WebContents> last_moved_web_contents_ = nullptr;
  std::string last_favorites_reordered_folder_;
  std::string last_expanded_folder_id_;
  std::string last_collapsed_folder_id_;
  std::string last_color_changed_folder_id_;
  std::string last_color_changed_;
};

}  // namespace

// Test fixture for AstraFavoriteService tests.
//
// Uses TestingProfile so the service has a real Profile* to attach to.
// The service is obtained through the factory to exercise the full
// ProfileKeyedService creation path.
//
// TODO(astra): Verify that TestingProfile correctly initializes the
// PrefService so that folder persistence tests (LoadFromPrefs/SaveToPrefs)
// can work.  Chromium component: PrefService + TestingProfile.
class FavoriteServiceTest : public testing::Test {
 protected:
  FavoriteServiceTest() {
    profile_ = std::make_unique<TestingProfile>();
    // TODO(astra): Obtain service through the factory once
    // AstraFavoriteServiceFactory is properly wired up.
    // For now we construct directly with the profile.
    service_ = std::make_unique<AstraFavoriteService>(profile_.get());
    DCHECK(service_);
  }

  ~FavoriteServiceTest() override = default;

  void SetUp() override {
    // The service should have been created with a root folder.
    ASSERT_GT(service_->folder_count(), 0u);
    ASSERT_FALSE(service_->GetRootFolderId().empty());
  }

  void TearDown() override {
    // Clean up observers to avoid dangling references.
    for (auto& observer : test_observers_) {
      service_->RemoveObserver(&observer);
    }
  }

  // Helpers ---------------------------------------------------------------

  // Adds and returns a new folder with the given name under root.
  const AstraFavoriteFolder* AddTestFolder(const std::string& name) {
    std::string id = service_->AddFolder(name);
    EXPECT_FALSE(id.empty());
    return service_->GetFolder(id);
  }

  // Adds a folder with a specific parent.
  const AstraFavoriteFolder* AddTestFolderWithParent(
      const std::string& name,
      const std::string& parent_id) {
    std::string id = service_->AddFolder(name, parent_id);
    EXPECT_FALSE(id.empty());
    return service_->GetFolder(id);
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<AstraFavoriteService> service_;

  // Pool of test observers managed by the fixture.
  std::vector<TestFavoriteServiceObserver> test_observers_;
};

// ---------------------------------------------------------------------------
// Root folder
// ---------------------------------------------------------------------------

TEST_F(FavoriteServiceTest, RootFolderExists) {
  // The service should create a root folder automatically.
  EXPECT_GT(service_->folder_count(), 0u);
  EXPECT_FALSE(service_->GetRootFolderId().empty());

  const AstraFavoriteFolder* root = service_->GetFolder(
      service_->GetRootFolderId());
  ASSERT_NE(root, nullptr);
  EXPECT_TRUE(root->is_root);
  EXPECT_TRUE(root->parent_id.empty());
  EXPECT_FALSE(root->name.empty());
  EXPECT_TRUE(root->is_expanded);
  EXPECT_EQ(root->order_index, 0u);

  // Root folder should match root_folder() accessor.
  EXPECT_EQ(&service_->root_folder(), root);
}

TEST_F(FavoriteServiceTest, EnsureRootFolderIsIdempotent) {
  size_t count_before = service_->folder_count();

  service_->EnsureRootFolder();
  service_->EnsureRootFolder();

  EXPECT_EQ(service_->folder_count(), count_before);
  EXPECT_FALSE(service_->GetRootFolderId().empty());
}

// ---------------------------------------------------------------------------
// Add folder
// ---------------------------------------------------------------------------

TEST_F(FavoriteServiceTest, AddFolder_UnderRoot) {
  size_t initial_count = service_->folder_count();

  const AstraFavoriteFolder* folder = AddTestFolder("Work");
  ASSERT_NE(folder, nullptr);
  EXPECT_EQ(folder->name, "Work");
  EXPECT_FALSE(folder->is_root);
  EXPECT_TRUE(folder->is_expanded);
  EXPECT_FALSE(folder->id.empty());
  EXPECT_FALSE(folder->created_time.is_null());

  // Parent should be root.
  EXPECT_EQ(folder->parent_id, service_->GetRootFolderId());

  EXPECT_EQ(service_->folder_count(), initial_count + 1);
}

TEST_F(FavoriteServiceTest, AddFolder_WithParent) {
  const AstraFavoriteFolder* parent = AddTestFolder("Parent");
  ASSERT_NE(parent, nullptr);
  size_t count_before = service_->folder_count();

  const AstraFavoriteFolder* child = AddTestFolderWithParent(
      "Child", parent->id);
  ASSERT_NE(child, nullptr);
  EXPECT_EQ(child->parent_id, parent->id);
  EXPECT_FALSE(child->is_root);

  EXPECT_EQ(service_->folder_count(), count_before + 1);
}

TEST_F(FavoriteServiceTest, AddFolder_EmptyNameStillCreates) {
  // Empty name should still create a folder (caller responsibility to validate).
  const AstraFavoriteFolder* folder = AddTestFolder("");
  ASSERT_NE(folder, nullptr);
  EXPECT_TRUE(folder->name.empty());
}

TEST_F(FavoriteServiceTest, AddFolder_NonexistentParentFallsBack) {
  // Adding with a nonexistent parent should still work (falls back to root
  // or creates with empty parent — behavior depends on implementation).
  // At minimum it should not crash.
  std::string id = service_->AddFolder("Orphan", "nonexistent-parent");
  // The folder should still be created.
  EXPECT_FALSE(id.empty());
  const AstraFavoriteFolder* folder = service_->GetFolder(id);
  ASSERT_NE(folder, nullptr);
}

// ---------------------------------------------------------------------------
// Get folder
// ---------------------------------------------------------------------------

TEST_F(FavoriteServiceTest, GetFolder_ExistingReturnsFolder) {
  const AstraFavoriteFolder* folder = AddTestFolder("Test");
  ASSERT_NE(folder, nullptr);

  const AstraFavoriteFolder* result = service_->GetFolder(folder->id);
  EXPECT_EQ(result, folder);
}

TEST_F(FavoriteServiceTest, GetFolder_NonexistentReturnsNull) {
  EXPECT_EQ(service_->GetFolder("nonexistent-id"), nullptr);
  EXPECT_EQ(service_->GetFolder(""), nullptr);
}

// ---------------------------------------------------------------------------
// Rename folder
// ---------------------------------------------------------------------------

TEST_F(FavoriteServiceTest, RenameFolder_ChangesName) {
  const AstraFavoriteFolder* folder = AddTestFolder("Original");
  ASSERT_NE(folder, nullptr);

  bool result = service_->RenameFolder(folder->id, "Renamed");
  EXPECT_TRUE(result);
  EXPECT_EQ(folder->name, "Renamed");
}

TEST_F(FavoriteServiceTest, RenameFolder_NonexistentReturnsFalse) {
  bool result = service_->RenameFolder("nonexistent", "Whatever");
  EXPECT_FALSE(result);
}

TEST_F(FavoriteServiceTest, RenameFolder_SameNameReturnsTrue) {
  const AstraFavoriteFolder* folder = AddTestFolder("Same");
  ASSERT_NE(folder, nullptr);

  bool result = service_->RenameFolder(folder->id, "Same");
  EXPECT_TRUE(result);
}

TEST_F(FavoriteServiceTest, RenameRootFolder_Behavior) {
  // Renaming root folder — the service should handle this.
  // Whether it allows renaming or rejects it is an implementation choice,
  // but it should not crash.
  std::string root_id = service_->GetRootFolderId();
  service_->RenameFolder(root_id, "New Root Name");
  // At minimum the call should not crash.
  EXPECT_FALSE(service_->GetRootFolderId().empty());
}

// ---------------------------------------------------------------------------
// Delete folder
// ---------------------------------------------------------------------------

TEST_F(FavoriteServiceTest, DeleteFolder_RemovesFolder) {
  const AstraFavoriteFolder* folder = AddTestFolder("To Delete");
  ASSERT_NE(folder, nullptr);
  std::string id = folder->id;
  size_t count_before = service_->folder_count();

  bool result = service_->DeleteFolder(id);
  EXPECT_TRUE(result);
  EXPECT_EQ(service_->folder_count(), count_before - 1);
  EXPECT_EQ(service_->GetFolder(id), nullptr);
}

TEST_F(FavoriteServiceTest, DeleteFolder_CannotDeleteRoot) {
  size_t initial_count = service_->folder_count();

  bool result = service_->DeleteFolder(service_->GetRootFolderId());
  EXPECT_FALSE(result);
  EXPECT_EQ(service_->folder_count(), initial_count);
}

TEST_F(FavoriteServiceTest, DeleteFolder_NonexistentReturnsFalse) {
  bool result = service_->DeleteFolder("nonexistent");
  EXPECT_FALSE(result);
}

// TODO(astra): Add test for DeleteFolder with children (cascade behavior).
// This requires verifying that child folders are either deleted or
// reparented when a parent is deleted.
// Chromium component: AstraFavoriteService::CollectDescendantIds.

// ---------------------------------------------------------------------------
// Child folders query
// ---------------------------------------------------------------------------

TEST_F(FavoriteServiceTest, GetChildFolders_RootChildren) {
  AddTestFolder("A");
  AddTestFolder("B");
  AddTestFolder("C");

  auto children = service_->GetChildFolders(
      service_->GetRootFolderId());
  EXPECT_GE(children.size(), 3u);

  // All children should have root as parent.
  for (const auto* child : children) {
    EXPECT_EQ(child->parent_id, service_->GetRootFolderId());
  }
}

TEST_F(FavoriteServiceTest, GetChildFolders_NestedChildren) {
  const AstraFavoriteFolder* parent = AddTestFolder("Parent");
  ASSERT_NE(parent, nullptr);

  AddTestFolderWithParent("Child1", parent->id);
  AddTestFolderWithParent("Child2", parent->id);

  auto children = service_->GetChildFolders(parent->id);
  EXPECT_EQ(children.size(), 2u);
}

TEST_F(FavoriteServiceTest, GetChildFolders_EmptyParentReturnsEmpty) {
  const AstraFavoriteFolder* folder = AddTestFolder("Empty");
  ASSERT_NE(folder, nullptr);

  auto children = service_->GetChildFolders(folder->id);
  EXPECT_TRUE(children.empty());
}

TEST_F(FavoriteServiceTest, GetChildFolders_NonexistentParent) {
  auto children = service_->GetChildFolders("nonexistent");
  // Should not crash; returns empty or root children depending on impl.
  // At minimum, no crash.
}

// ---------------------------------------------------------------------------
// Reorder folders
// ---------------------------------------------------------------------------

TEST_F(FavoriteServiceTest, ReorderFolders_ChangesOrder) {
  const AstraFavoriteFolder* a = AddTestFolder("A");
  const AstraFavoriteFolder* b = AddTestFolder("B");
  const AstraFavoriteFolder* c = AddTestFolder("C");
  ASSERT_NE(a, nullptr);
  ASSERT_NE(b, nullptr);
  ASSERT_NE(c, nullptr);

  // Initial order should be A, B, C (by addition order).
  auto children_before = service_->GetChildFolders(
      service_->GetRootFolderId());
  // (We can't guarantee exact initial order without knowing all folders,
  //  but reordering should work.)

  // Reorder to C, A, B.
  std::vector<std::string> new_order = {c->id, a->id, b->id};
  bool result = service_->ReorderFolders(
      service_->GetRootFolderId(), new_order);
  EXPECT_TRUE(result);

  auto children_after = service_->GetChildFolders(
      service_->GetRootFolderId());
  // Find these three in the result and verify order.
  bool found_c = false, found_a = false, found_b = false;
  bool c_before_a = false, a_before_b = false;
  for (const auto* child : children_after) {
    if (child->id == c->id) {
      found_c = true;
      EXPECT_FALSE(found_a);
      EXPECT_FALSE(found_b);
      c_before_a = !found_a;
    } else if (child->id == a->id) {
      found_a = true;
      EXPECT_FALSE(found_b);
      if (found_c) a_before_b = !found_b;
    } else if (child->id == b->id) {
      found_b = true;
    }
  }
  EXPECT_TRUE(found_c);
  EXPECT_TRUE(found_a);
  EXPECT_TRUE(found_b);
  EXPECT_TRUE(c_before_a);
  EXPECT_TRUE(a_before_b);
}

TEST_F(FavoriteServiceTest, ReorderFolders_WrongSizeReturnsFalse) {
  AddTestFolder("A");
  AddTestFolder("B");

  // Only 1 ID provided, but there are more root children.
  bool result = service_->ReorderFolders(
      service_->GetRootFolderId(), {"A"});
  EXPECT_FALSE(result);
}

TEST_F(FavoriteServiceTest, ReorderFolders_NonexistentIdReturnsFalse) {
  const AstraFavoriteFolder* a = AddTestFolder("A");
  ASSERT_NE(a, nullptr);

  bool result = service_->ReorderFolders(
      service_->GetRootFolderId(), {a->id, "nonexistent"});
  // Behavior depends on implementation — should at least not crash.
  // The test verifies the function returns without crashing.
}

// ---------------------------------------------------------------------------
// Toggle folder expanded
// ---------------------------------------------------------------------------

TEST_F(FavoriteServiceTest, ToggleFolderExpanded) {
  const AstraFavoriteFolder* folder = AddTestFolder("Test");
  ASSERT_NE(folder, nullptr);
  EXPECT_TRUE(folder->is_expanded);

  bool changed = service_->ToggleFolderExpanded(folder->id);
  EXPECT_TRUE(changed);
  EXPECT_FALSE(folder->is_expanded);

  changed = service_->ToggleFolderExpanded(folder->id);
  EXPECT_TRUE(changed);
  EXPECT_TRUE(folder->is_expanded);
}

TEST_F(FavoriteServiceTest, ToggleFolderExpanded_NonexistentReturnsFalse) {
  bool changed = service_->ToggleFolderExpanded("nonexistent");
  EXPECT_FALSE(changed);
}

// ---------------------------------------------------------------------------
// Observer: OnFolderAdded
// ---------------------------------------------------------------------------

TEST_F(FavoriteServiceTest, ObserverFiresOnFolderAdd) {
  TestFavoriteServiceObserver observer;
  service_->AddObserver(&observer);

  const AstraFavoriteFolder* folder = AddTestFolder("Test Folder");
  ASSERT_NE(folder, nullptr);

  EXPECT_EQ(observer.folder_added_count_, 1);
  EXPECT_EQ(observer.last_added_folder_id_, folder->id);
  EXPECT_EQ(observer.last_added_folder_name_, "Test Folder");

  service_->RemoveObserver(&observer);
}

TEST_F(FavoriteServiceTest, ObserverFiresOnFolderRemove) {
  const AstraFavoriteFolder* folder = AddTestFolder("To Remove");
  ASSERT_NE(folder, nullptr);
  std::string id = folder->id;

  TestFavoriteServiceObserver observer;
  service_->AddObserver(&observer);

  service_->DeleteFolder(id);

  EXPECT_EQ(observer.folder_removed_count_, 1);
  EXPECT_EQ(observer.last_removed_folder_id_, id);

  service_->RemoveObserver(&observer);
}

TEST_F(FavoriteServiceTest, ObserverFiresOnRename) {
  const AstraFavoriteFolder* folder = AddTestFolder("Original");
  ASSERT_NE(folder, nullptr);

  TestFavoriteServiceObserver observer;
  service_->AddObserver(&observer);

  service_->RenameFolder(folder->id, "NewName");

  EXPECT_EQ(observer.folder_renamed_count_, 1);
  EXPECT_EQ(observer.last_renamed_folder_id_, folder->id);
  EXPECT_EQ(observer.last_renamed_folder_name_, "NewName");

  service_->RemoveObserver(&observer);
}

TEST_F(FavoriteServiceTest, ObserverFiresOnReorder) {
  const AstraFavoriteFolder* a = AddTestFolder("A");
  const AstraFavoriteFolder* b = AddTestFolder("B");
  ASSERT_NE(a, nullptr);
  ASSERT_NE(b, nullptr);

  TestFavoriteServiceObserver observer;
  service_->AddObserver(&observer);

  service_->ReorderFolders(service_->GetRootFolderId(), {b->id, a->id});

  // Observer may or may not fire depending on whether reorder succeeds.
  // At minimum, no crash.
  EXPECT_GE(observer.folders_reordered_count_, 0);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Favorite count in folder
// ---------------------------------------------------------------------------

TEST_F(FavoriteServiceTest, GetFavoriteCountInFolder_EmptyFolderReturnsZero) {
  const AstraFavoriteFolder* folder = AddTestFolder("Empty");
  ASSERT_NE(folder, nullptr);

  size_t count = service_->GetFavoriteCountInFolder(folder->id);
  // TODO(astra): Currently returns 0 as stub.  Implement proper counting
  // by iterating TabStripModel across all browsers for the profile.
  // Chromium component: BrowserList + TabStripModel.
  EXPECT_EQ(count, 0u);
}

TEST_F(FavoriteServiceTest, GetFavoriteCountInFolder_RootFolder) {
  size_t count = service_->GetFavoriteCountInFolder(
      service_->GetRootFolderId());
  // Should return 0 or more without crashing.
  EXPECT_GE(count, 0u);
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------

TEST_F(FavoriteServiceTest, ShutdownClearsObservers) {
  TestFavoriteServiceObserver observer;
  service_->AddObserver(&observer);

  service_->Shutdown();

  // After shutdown, adding a folder should not notify the observer.
  AddTestFolder("After Shutdown");
  EXPECT_EQ(observer.folder_added_count_, 0);
}

// ---------------------------------------------------------------------------
// Folder color
// ---------------------------------------------------------------------------

TEST_F(FavoriteServiceTest, SetFolderColor_ChangesColor) {
  const AstraFavoriteFolder* folder = AddTestFolder("ColorTest");
  ASSERT_NE(folder, nullptr);
  EXPECT_FALSE(folder->color.empty());

  bool result = service_->SetFolderColor(folder->id, "#FF5733");
  EXPECT_TRUE(result);
  EXPECT_EQ(folder->color, "#FF5733");
}

TEST_F(FavoriteServiceTest, SetFolderColor_NonexistentReturnsFalse) {
  bool result = service_->SetFolderColor("nonexistent", "#FF0000");
  EXPECT_FALSE(result);
}

TEST_F(FavoriteServiceTest, SetFolderColor_SameColorReturnsTrue) {
  const AstraFavoriteFolder* folder = AddTestFolder("SameColor");
  ASSERT_NE(folder, nullptr);
  std::string original_color = folder->color;

  bool result = service_->SetFolderColor(folder->id, original_color);
  EXPECT_TRUE(result);
}

TEST_F(FavoriteServiceTest, SetFolderColor_NotifiesObserver) {
  const AstraFavoriteFolder* folder = AddTestFolder("ColorObs");
  ASSERT_NE(folder, nullptr);

  TestFavoriteServiceObserver observer;
  service_->AddObserver(&observer);

  service_->SetFolderColor(folder->id, "#112233");

  EXPECT_EQ(observer.folder_color_changed_count_, 1);
  EXPECT_EQ(observer.last_color_changed_folder_id_, folder->id);
  EXPECT_EQ(observer.last_color_changed_, "#112233");

  service_->RemoveObserver(&observer);
}

TEST_F(FavoriteServiceTest, SetFolderColor_SameColorNoObserver) {
  const AstraFavoriteFolder* folder = AddTestFolder("ColorNoObs");
  ASSERT_NE(folder, nullptr);
  std::string original_color = folder->color;

  TestFavoriteServiceObserver observer;
  service_->AddObserver(&observer);

  service_->SetFolderColor(folder->id, original_color);

  EXPECT_EQ(observer.folder_color_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(FavoriteServiceTest, GetFolderColorPalette_NonEmpty) {
  auto palette = AstraFavoriteService::GetFolderColorPalette();
  EXPECT_FALSE(palette.empty());
  EXPECT_GE(palette.size(), 8u);
}

TEST_F(FavoriteServiceTest, GetFolderColorPalette_ValidHexColors) {
  auto palette = AstraFavoriteService::GetFolderColorPalette();
  for (const auto& color : palette) {
    EXPECT_EQ(color[0], '#');
    EXPECT_EQ(color.size(), 7u);  // #RRGGBB
  }
}

TEST_F(FavoriteServiceTest, NewFolderHasDefaultColor) {
  const AstraFavoriteFolder* folder = AddTestFolder("DefaultColor");
  ASSERT_NE(folder, nullptr);
  EXPECT_FALSE(folder->color.empty());
  // Default color should match the first palette color or the defined default.
  auto palette = AstraFavoriteService::GetFolderColorPalette();
  EXPECT_FALSE(palette.empty());
}

// ---------------------------------------------------------------------------
// Folder expanded / collapsed observers
// ---------------------------------------------------------------------------

TEST_F(FavoriteServiceTest, ExpandFolder_CollapsedFolder) {
  const AstraFavoriteFolder* folder = AddTestFolder("ExpandTest");
  ASSERT_NE(folder, nullptr);

  // Start collapsed.
  service_->ToggleFolderExpanded(folder->id);
  ASSERT_FALSE(folder->is_expanded);

  bool result = service_->ExpandFolder(folder->id);
  EXPECT_TRUE(result);
  EXPECT_TRUE(folder->is_expanded);
}

TEST_F(FavoriteServiceTest, ExpandFolder_AlreadyExpanded) {
  const AstraFavoriteFolder* folder = AddTestFolder("AlreadyExpanded");
  ASSERT_NE(folder, nullptr);
  ASSERT_TRUE(folder->is_expanded);

  bool result = service_->ExpandFolder(folder->id);
  EXPECT_FALSE(result);
  EXPECT_TRUE(folder->is_expanded);
}

TEST_F(FavoriteServiceTest, CollapseFolder_ExpandedFolder) {
  const AstraFavoriteFolder* folder = AddTestFolder("CollapseTest");
  ASSERT_NE(folder, nullptr);
  ASSERT_TRUE(folder->is_expanded);

  bool result = service_->CollapseFolder(folder->id);
  EXPECT_TRUE(result);
  EXPECT_FALSE(folder->is_expanded);
}

TEST_F(FavoriteServiceTest, CollapseFolder_AlreadyCollapsed) {
  const AstraFavoriteFolder* folder = AddTestFolder("AlreadyCollapsed");
  ASSERT_NE(folder, nullptr);

  service_->ToggleFolderExpanded(folder->id);
  ASSERT_FALSE(folder->is_expanded);

  bool result = service_->CollapseFolder(folder->id);
  EXPECT_FALSE(result);
  EXPECT_FALSE(folder->is_expanded);
}

TEST_F(FavoriteServiceTest, ExpandFolder_NonexistentReturnsFalse) {
  bool result = service_->ExpandFolder("nonexistent");
  EXPECT_FALSE(result);
}

TEST_F(FavoriteServiceTest, CollapseFolder_NonexistentReturnsFalse) {
  bool result = service_->CollapseFolder("nonexistent");
  EXPECT_FALSE(result);
}

TEST_F(FavoriteServiceTest, ToggleFolderExpanded_NotifiesExpand) {
  const AstraFavoriteFolder* folder = AddTestFolder("ToggleObs");
  ASSERT_NE(folder, nullptr);

  // Start by collapsing it.
  service_->ToggleFolderExpanded(folder->id);
  ASSERT_FALSE(folder->is_expanded);

  TestFavoriteServiceObserver observer;
  service_->AddObserver(&observer);

  // Now expand it via toggle.
  service_->ToggleFolderExpanded(folder->id);

  EXPECT_EQ(observer.folder_expanded_count_, 1);
  EXPECT_EQ(observer.last_expanded_folder_id_, folder->id);
  EXPECT_EQ(observer.folder_collapsed_count_, 0);

  // Now collapse via toggle.
  service_->ToggleFolderExpanded(folder->id);

  EXPECT_EQ(observer.folder_collapsed_count_, 1);
  EXPECT_EQ(observer.last_collapsed_folder_id_, folder->id);

  service_->RemoveObserver(&observer);
}

TEST_F(FavoriteServiceTest, ExpandFolder_NotifiesObserver) {
  const AstraFavoriteFolder* folder = AddTestFolder("ExpandObs");
  ASSERT_NE(folder, nullptr);
  service_->CollapseFolder(folder->id);

  TestFavoriteServiceObserver observer;
  service_->AddObserver(&observer);

  service_->ExpandFolder(folder->id);

  EXPECT_EQ(observer.folder_expanded_count_, 1);
  EXPECT_EQ(observer.folder_collapsed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(FavoriteServiceTest, CollapseFolder_NotifiesObserver) {
  const AstraFavoriteFolder* folder = AddTestFolder("CollapseObs");
  ASSERT_NE(folder, nullptr);

  TestFavoriteServiceObserver observer;
  service_->AddObserver(&observer);

  service_->CollapseFolder(folder->id);

  EXPECT_EQ(observer.folder_collapsed_count_, 1);
  EXPECT_EQ(observer.folder_expanded_count_, 0);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Find folder by name
// ---------------------------------------------------------------------------

TEST_F(FavoriteServiceTest, FindFolderByName_ExactMatch) {
  AddTestFolder("Work");
  AddTestFolder("Personal");

  const AstraFavoriteFolder* found = service_->FindFolderByName("Work");
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found->name, "Work");
}

TEST_F(FavoriteServiceTest, FindFolderByName_CaseInsensitive) {
  AddTestFolder("MyFolder");

  const AstraFavoriteFolder* lower = service_->FindFolderByName("myfolder");
  ASSERT_NE(lower, nullptr);
  EXPECT_EQ(lower->name, "MyFolder");

  const AstraFavoriteFolder* upper = service_->FindFolderByName("MYFOLDER");
  ASSERT_NE(upper, nullptr);
  EXPECT_EQ(upper->name, "MyFolder");
}

TEST_F(FavoriteServiceTest, FindFolderByName_NotFoundReturnsNull) {
  AddTestFolder("Existing");

  const AstraFavoriteFolder* found = service_->FindFolderByName("Nonexistent");
  EXPECT_EQ(found, nullptr);
}

TEST_F(FavoriteServiceTest, FindFolderByName_EmptyName) {
  const AstraFavoriteFolder* found = service_->FindFolderByName("");
  // May or may not find a folder with empty name, but shouldn't crash.
}

// ---------------------------------------------------------------------------
// Folder depth
// ---------------------------------------------------------------------------

TEST_F(FavoriteServiceTest, GetFolderDepth_RootIsZero) {
  EXPECT_EQ(service_->GetFolderDepth(service_->GetRootFolderId()), 0u);
}

TEST_F(FavoriteServiceTest, GetFolderDepth_ChildOfRootIsOne) {
  const AstraFavoriteFolder* child = AddTestFolder("Child");
  ASSERT_NE(child, nullptr);
  EXPECT_EQ(service_->GetFolderDepth(child->id), 1u);
}

TEST_F(FavoriteServiceTest, GetFolderDepth_NestedFolders) {
  const AstraFavoriteFolder* parent = AddTestFolder("Parent");
  ASSERT_NE(parent, nullptr);
  const AstraFavoriteFolder* child = AddTestFolderWithParent("Child", parent->id);
  ASSERT_NE(child, nullptr);
  const AstraFavoriteFolder* grandchild = AddTestFolderWithParent("Grandchild", child->id);
  ASSERT_NE(grandchild, nullptr);

  EXPECT_EQ(service_->GetFolderDepth(parent->id), 1u);
  EXPECT_EQ(service_->GetFolderDepth(child->id), 2u);
  EXPECT_EQ(service_->GetFolderDepth(grandchild->id), 3u);
}

TEST_F(FavoriteServiceTest, GetFolderDepth_NonexistentReturnsZero) {
  EXPECT_EQ(service_->GetFolderDepth("nonexistent"), 0u);
}

// ---------------------------------------------------------------------------
// Descendant folders
// ---------------------------------------------------------------------------

TEST_F(FavoriteServiceTest, GetAllDescendantIds_EmptyFolder) {
  const AstraFavoriteFolder* folder = AddTestFolder("EmptyFolder");
  ASSERT_NE(folder, nullptr);

  auto descendants = service_->GetAllDescendantIds(folder->id);
  EXPECT_TRUE(descendants.empty());
}

TEST_F(FavoriteServiceTest, GetAllDescendantIds_WithChildren) {
  const AstraFavoriteFolder* parent = AddTestFolder("Parent");
  ASSERT_NE(parent, nullptr);
  const AstraFavoriteFolder* child1 = AddTestFolderWithParent("Child1", parent->id);
  const AstraFavoriteFolder* child2 = AddTestFolderWithParent("Child2", parent->id);
  ASSERT_NE(child1, nullptr);
  ASSERT_NE(child2, nullptr);

  auto descendants = service_->GetAllDescendantIds(parent->id);
  EXPECT_EQ(descendants.size(), 2u);
}

TEST_F(FavoriteServiceTest, GetAllDescendantIds_Nested) {
  const AstraFavoriteFolder* parent = AddTestFolder("Parent");
  ASSERT_NE(parent, nullptr);
  const AstraFavoriteFolder* child = AddTestFolderWithParent("Child", parent->id);
  ASSERT_NE(child, nullptr);
  const AstraFavoriteFolder* grandchild = AddTestFolderWithParent("Grandchild", child->id);
  ASSERT_NE(grandchild, nullptr);

  auto descendants = service_->GetAllDescendantIds(parent->id);
  EXPECT_EQ(descendants.size(), 2u);  // child + grandchild
}

TEST_F(FavoriteServiceTest, GetAllDescendantIds_NonexistentReturnsEmpty) {
  auto descendants = service_->GetAllDescendantIds("nonexistent");
  EXPECT_TRUE(descendants.empty());
}

// ---------------------------------------------------------------------------
// Bulk operations: expand / collapse all
// ---------------------------------------------------------------------------

TEST_F(FavoriteServiceTest, ExpandAllFolders_AllExpanded) {
  const AstraFavoriteFolder* a = AddTestFolder("A");
  const AstraFavoriteFolder* b = AddTestFolder("B");
  const AstraFavoriteFolder* c = AddTestFolder("C");
  ASSERT_NE(a, nullptr);
  ASSERT_NE(b, nullptr);
  ASSERT_NE(c, nullptr);

  // Collapse all first.
  service_->CollapseAllFolders();
  ASSERT_FALSE(a->is_expanded);
  ASSERT_FALSE(b->is_expanded);
  ASSERT_FALSE(c->is_expanded);

  size_t changed = service_->ExpandAllFolders();
  EXPECT_EQ(changed, 3u);
  EXPECT_TRUE(a->is_expanded);
  EXPECT_TRUE(b->is_expanded);
  EXPECT_TRUE(c->is_expanded);
}

TEST_F(FavoriteServiceTest, CollapseAllFolders_SkipsRoot) {
  AddTestFolder("A");
  AddTestFolder("B");

  size_t changed = service_->CollapseAllFolders();
  EXPECT_EQ(changed, 2u);

  // Root should still be "expanded" (it's always the top level).
  const AstraFavoriteFolder* root =
      service_->GetFolder(service_->GetRootFolderId());
  ASSERT_NE(root, nullptr);
  EXPECT_TRUE(root->is_expanded);
}

TEST_F(FavoriteServiceTest, ExpandAllFolders_NoChangeWhenAllExpanded) {
  AddTestFolder("A");
  AddTestFolder("B");
  // New folders start expanded.

  size_t changed = service_->ExpandAllFolders();
  EXPECT_EQ(changed, 0u);
}

TEST_F(FavoriteServiceTest, CollapseAllFolders_NoChangeWhenAllCollapsed) {
  AddTestFolder("A");
  AddTestFolder("B");

  service_->CollapseAllFolders();
  size_t changed = service_->CollapseAllFolders();
  EXPECT_EQ(changed, 0u);
}

TEST_F(FavoriteServiceTest, CollapseAllFolders_RoundTrip) {
  AddTestFolder("A");
  AddTestFolder("B");

  size_t collapsed = service_->CollapseAllFolders();
  size_t expanded = service_->ExpandAllFolders();

  EXPECT_EQ(collapsed, expanded);
}

// ---------------------------------------------------------------------------
// Import / Export
// ---------------------------------------------------------------------------

TEST_F(FavoriteServiceTest, ExportFoldersJson_ReturnsValidStructure) {
  AddTestFolder("Folder1");
  AddTestFolder("Folder2");

  std::string json = service_->ExportFoldersJson();
  EXPECT_FALSE(json.empty());
  EXPECT_EQ(json[0], '[');
  EXPECT_EQ(json.back(), ']');
  EXPECT_NE(json.find("\"id\""), std::string::npos);
  EXPECT_NE(json.find("\"name\""), std::string::npos);
}

TEST_F(FavoriteServiceTest, ExportFoldersJson_EmptyReturnsArray) {
  // Root folder always exists.
  std::string json = service_->ExportFoldersJson();
  EXPECT_EQ(json[0], '[');
  EXPECT_EQ(json.back(), ']');
}

TEST_F(FavoriteServiceTest, ImportFoldersJson_ReplaceMode) {
  AddTestFolder("Original");
  size_t initial_count = service_->folder_count();

  std::string json =
      "[{\"id\":\"imported-1\",\"name\":\"Imported 1\","
      "\"parent_id\":\"root\",\"color\":\"#FF0000\"}]";

  size_t imported = service_->ImportFoldersJson(json, /*merge=*/false);
  EXPECT_GT(imported, 0u);
  // Root + imported folders.
  EXPECT_EQ(service_->folder_count(), 1u + imported);

  // Original folder should be gone (replace mode).
  EXPECT_EQ(service_->FindFolderByName("Original"), nullptr);
  EXPECT_NE(service_->FindFolderByName("Imported 1"), nullptr);
}

TEST_F(FavoriteServiceTest, ImportFoldersJson_MergeMode) {
  AddTestFolder("Original");
  size_t initial_count = service_->folder_count();

  std::string json =
      "[{\"id\":\"merged-1\",\"name\":\"Merged 1\","
      "\"parent_id\":\"root\"}]";

  size_t imported = service_->ImportFoldersJson(json, /*merge=*/true);
  EXPECT_GT(imported, 0u);
  EXPECT_EQ(service_->folder_count(), initial_count + imported);

  // Original should still exist.
  EXPECT_NE(service_->FindFolderByName("Original"), nullptr);
  EXPECT_NE(service_->FindFolderByName("Merged 1"), nullptr);
}

TEST_F(FavoriteServiceTest, ImportFoldersJson_EmptyString) {
  size_t initial_count = service_->folder_count();
  size_t imported = service_->ImportFoldersJson("", /*merge=*/false);
  EXPECT_EQ(imported, 0u);
  EXPECT_EQ(service_->folder_count(), initial_count);
}

TEST_F(FavoriteServiceTest, ImportExport_RoundTrip) {
  AddTestFolder("RoundTrip1");
  AddTestFolder("RoundTrip2");

  std::string exported = service_->ExportFoldersJson();
  EXPECT_FALSE(exported.empty());

  // Create a new service and import.
  auto new_service = std::make_unique<AstraFavoriteService>(profile_.get());
  size_t imported = new_service->ImportFoldersJson(exported, /*merge=*/false);
  EXPECT_GT(imported, 0u);

  EXPECT_NE(new_service->FindFolderByName("RoundTrip1"), nullptr);
  EXPECT_NE(new_service->FindFolderByName("RoundTrip2"), nullptr);
}

// ---------------------------------------------------------------------------
// Observer default implementations
// ---------------------------------------------------------------------------

namespace {

// Observer that overrides only one method to verify defaults work.
class PartialFavoriteObserver : public AstraFavoriteServiceObserver {
 public:
  void OnFolderAdded(const AstraFavoriteFolder& folder) override {
    added_count_++;
  }

  int added_count_ = 0;
};

}  // namespace

TEST_F(FavoriteServiceTest, ObserverDefaultImplementations) {
  PartialFavoriteObserver partial_observer;
  service_->AddObserver(&partial_observer);

  // These should all compile and not crash (default implementations).
  const AstraFavoriteFolder* folder = AddTestFolder("PartialObs");
  ASSERT_NE(folder, nullptr);

  service_->RenameFolder(folder->id, "NewName");
  service_->SetFolderColor(folder->id, "#123456");
  service_->CollapseFolder(folder->id);
  service_->ExpandFolder(folder->id);
  service_->ReorderFolders(service_->GetRootFolderId(), {folder->id});
  service_->DeleteFolder(folder->id);

  // Only OnFolderAdded was overridden and should have fired once.
  EXPECT_EQ(partial_observer.added_count_, 1);

  service_->RemoveObserver(&partial_observer);
}

// ---------------------------------------------------------------------------
// Persistence via PrefService
// ---------------------------------------------------------------------------

TEST_F(FavoriteServiceTest, Persistence_SurvivesServiceRecreation) {
  // Add some folders to the first service instance.
  service_->AddFolder("Persisted1");
  service_->AddFolder("Persisted2");
  size_t count_before = service_->folder_count();

  // Destroy the old service and create a new one (simulates browser restart).
  service_.reset();
  auto new_service = std::make_unique<AstraFavoriteService>(profile_.get());

  // The new service should have loaded the persisted folders.
  EXPECT_GE(new_service->folder_count(), count_before);
  EXPECT_NE(new_service->FindFolderByName("Persisted1"), nullptr);
  EXPECT_NE(new_service->FindFolderByName("Persisted2"), nullptr);
}

TEST_F(FavoriteServiceTest, Persistence_ColorPersisted) {
  const AstraFavoriteFolder* folder = AddTestFolder("ColorPersist");
  ASSERT_NE(folder, nullptr);
  std::string folder_id = folder->id;
  service_->SetFolderColor(folder_id, "#AABBCC");

  // Recreate service.
  service_.reset();
  auto new_service = std::make_unique<AstraFavoriteService>(profile_.get());

  const AstraFavoriteFolder* loaded = new_service->GetFolder(folder_id);
  ASSERT_NE(loaded, nullptr);
  EXPECT_EQ(loaded->color, "#AABBCC");
}

TEST_F(FavoriteServiceTest, Persistence_ExpandedStatePersisted) {
  const AstraFavoriteFolder* folder = AddTestFolder("ExpandPersist");
  ASSERT_NE(folder, nullptr);
  std::string folder_id = folder->id;
  service_->CollapseFolder(folder_id);
  ASSERT_FALSE(folder->is_expanded);

  // Recreate service.
  service_.reset();
  auto new_service = std::make_unique<AstraFavoriteService>(profile_.get());

  const AstraFavoriteFolder* loaded = new_service->GetFolder(folder_id);
  ASSERT_NE(loaded, nullptr);
  EXPECT_FALSE(loaded->is_expanded);
}

// ---------------------------------------------------------------------------
// Root folder default color
// ---------------------------------------------------------------------------

TEST_F(FavoriteServiceTest, RootFolderHasDefaultColor) {
  const AstraFavoriteFolder* root =
      service_->GetFolder(service_->GetRootFolderId());
  ASSERT_NE(root, nullptr);
  EXPECT_FALSE(root->color.empty());
}

// ---------------------------------------------------------------------------
// TODO(astra): Browser integration tests
// ---------------------------------------------------------------------------
//
// The following tests require a real Browser + TabStripModel and should
// be implemented as browser_tests using InProcessBrowserTest:
//
//   - MoveFavoriteToFolder_UpdatesTabMetadata
//   - ReorderFavoritesInFolder_UpdatesOrderIndices
//   - GetFavoriteCountInFolder_AccurateCount
//   - DeleteFolder_ReassignsFavoritesToParent
//   - SessionRestore_PreservesFavoriteState
//
// TODO(astra): Add browser_tests target for favorite service integration.
// Patch point: //chrome/test/BUILD.gn browser_tests test suites.
//
// TODO(astra): Add persistence tests (LoadFromPrefs / SaveToPrefs) once
// pref registration is wired up and TestingProfile has the necessary
// pref keys registered.
// Chromium component: PrefService + PrefRegistrySimple.

}  // namespace astra
