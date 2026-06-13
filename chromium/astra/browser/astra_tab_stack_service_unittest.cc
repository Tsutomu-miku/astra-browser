// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Unit tests for AstraTabStackService.
//
// Tests verify:
//   - Stack CRUD: CreateStack, DeleteStack, RenameStack, SetStackColor
//   - Stack existence: DoesStackExist, GetStack, GetStackCount
//   - Tab membership: AddTabToStack, RemoveTabFromStack, GetTabsInStack
//   - Tab ordering: MoveTabInStack, MoveTabsToStack
//   - Collapse/expand: CollapseStack, ExpandStack, ToggleStackCollapsed
//   - Bulk operations: CloseStack, UndoCloseStack, MergeStacks, DuplicateStack
//   - Metadata: note, pinned, timestamps, order_index
//   - Search: SearchStacks, GetStacksByColor, GetRecentlyAccessedStacks
//   - Settings: all 8 settings (get/set/defaults)
//   - Observer notifications: all 8 observer events
//   - Edge cases: empty names, invalid IDs, duplicate adds, etc.
//   - AstraTabStackInfo struct: IsEmpty, HasTab, default values
//
// This service manages tab stack metadata only.  Chromium owns the actual
// tab state (TabStripModel, WebContents).  Tab indices in this service
// are metadata references — in production they map to TabStripModel
// positions.
//
// Chromium test pattern: TestingProfile + base::test::TaskEnvironment
//   (chrome/test/base/testing_profile.h)

#include "astra/browser/astra_tab_stack_service.h"

#include <memory>
#include <vector>

#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

namespace {

// Test observer that records all notifications for verification.
class TestTabStackObserver : public AstraTabStackObserver {
 public:
  void OnStackCreated(AstraTabStackService* service,
                      const std::string& stack_id) override {
    created_count_++;
    last_created_service_ = service;
    last_created_stack_id_ = stack_id;
  }

  void OnStackDeleted(AstraTabStackService* service,
                      const std::string& stack_id) override {
    deleted_count_++;
    last_deleted_service_ = service;
    last_deleted_stack_id_ = stack_id;
  }

  void OnStackChanged(AstraTabStackService* service,
                      const std::string& stack_id) override {
    changed_count_++;
    last_changed_service_ = service;
    last_changed_stack_id_ = stack_id;
  }

  void OnStackCollapsedChanged(AstraTabStackService* service,
                               const std::string& stack_id,
                               bool collapsed) override {
    collapsed_changed_count_++;
    last_collapsed_service_ = service;
    last_collapsed_stack_id_ = stack_id;
    last_collapsed_state_ = collapsed;
  }

  void OnTabAddedToStack(AstraTabStackService* service,
                         int tab_index,
                         const std::string& stack_id) override {
    tab_added_count_++;
    last_added_service_ = service;
    last_added_tab_index_ = tab_index;
    last_added_stack_id_ = stack_id;
  }

  void OnTabRemovedFromStack(AstraTabStackService* service,
                             int tab_index,
                             const std::string& stack_id) override {
    tab_removed_count_++;
    last_removed_service_ = service;
    last_removed_tab_index_ = tab_index;
    last_removed_stack_id_ = stack_id;
  }

  void OnStacksReordered(AstraTabStackService* service) override {
    reordered_count_++;
    last_reordered_service_ = service;
  }

  void OnTabStackServiceShutdown(AstraTabStackService* service) override {
    shutdown_count_++;
    last_shutdown_service_ = service;
  }

  // Counters
  int created_count_ = 0;
  int deleted_count_ = 0;
  int changed_count_ = 0;
  int collapsed_changed_count_ = 0;
  int tab_added_count_ = 0;
  int tab_removed_count_ = 0;
  int reordered_count_ = 0;
  int shutdown_count_ = 0;

  // Last recorded values
  raw_ptr<AstraTabStackService> last_created_service_ = nullptr;
  std::string last_created_stack_id_;

  raw_ptr<AstraTabStackService> last_deleted_service_ = nullptr;
  std::string last_deleted_stack_id_;

  raw_ptr<AstraTabStackService> last_changed_service_ = nullptr;
  std::string last_changed_stack_id_;

  raw_ptr<AstraTabStackService> last_collapsed_service_ = nullptr;
  std::string last_collapsed_stack_id_;
  bool last_collapsed_state_ = false;

  raw_ptr<AstraTabStackService> last_added_service_ = nullptr;
  int last_added_tab_index_ = -1;
  std::string last_added_stack_id_;

  raw_ptr<AstraTabStackService> last_removed_service_ = nullptr;
  int last_removed_tab_index_ = -1;
  std::string last_removed_stack_id_;

  raw_ptr<AstraTabStackService> last_reordered_service_ = nullptr;

  raw_ptr<AstraTabStackService> last_shutdown_service_ = nullptr;
};

}  // namespace

// =========================================================================
// TabStackService test fixture
// =========================================================================

class TabStackServiceTest : public testing::Test {
 protected:
  TabStackServiceTest() {
    profile_ = std::make_unique<TestingProfile>();
    service_ = std::make_unique<AstraTabStackService>(profile_.get());
    DCHECK(service_);
  }

  ~TabStackServiceTest() override = default;

  void SetUp() override {
    // Service should start with no stacks.
    ASSERT_EQ(0u, service_->GetStackCount());
  }

  void TearDown() override {
    // Clean up observers to avoid dangling references.
    for (auto& observer : test_observers_) {
      service_->RemoveObserver(observer.get());
    }
    test_observers_.clear();
  }

  // Helper to create a test observer and register it with the service.
  TestTabStackObserver* AddTestObserver() {
    auto observer = std::make_unique<TestTabStackObserver>();
    TestTabStackObserver* raw = observer.get();
    service_->AddObserver(raw);
    test_observers_.push_back(std::move(observer));
    return raw;
  }

  // Helper to create a stack and return its ID.
  std::string CreateTestStack(const std::string& name,
                              const std::string& color = std::string()) {
    return service_->CreateStack(name, color);
  }

  PrefService* prefs() { return profile_->GetPrefs(); }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<AstraTabStackService> service_;
  std::vector<std::unique_ptr<TestTabStackObserver>> test_observers_;
};

// =========================================================================
// Stack creation tests
// =========================================================================

TEST_F(TabStackServiceTest, CreateStackReturnsValidId) {
  std::string id = CreateTestStack("Work", "#5B8FF9");
  EXPECT_FALSE(id.empty());
}

TEST_F(TabStackServiceTest, CreateStackIncreasesCount) {
  EXPECT_EQ(0u, service_->GetStackCount());
  CreateTestStack("Work", "#5B8FF9");
  EXPECT_EQ(1u, service_->GetStackCount());
  CreateTestStack("Personal", "#5AD8A6");
  EXPECT_EQ(2u, service_->GetStackCount());
}

TEST_F(TabStackServiceTest, CreatedStackHasCorrectProperties) {
  std::string id = CreateTestStack("My Stack", "#FF6B6B");

  const AstraTabStackInfo* stack = service_->GetStack(id);
  ASSERT_NE(nullptr, stack);
  EXPECT_EQ(id, stack->stack_id);
  EXPECT_EQ("My Stack", stack->name);
  EXPECT_EQ("#FF6B6B", stack->color);
  EXPECT_EQ(0u, stack->tab_count);
  EXPECT_FALSE(stack->is_collapsed);
  EXPECT_FALSE(stack->is_pinned);
  EXPECT_TRUE(stack->note.empty());
  EXPECT_TRUE(stack->tab_indices.empty());
  EXPECT_FALSE(stack->created_time.is_null());
  EXPECT_FALSE(stack->last_accessed.is_null());
}

TEST_F(TabStackServiceTest, CreateStackNotifiesObserver) {
  auto* observer = AddTestObserver();

  std::string id = CreateTestStack("Test", "#5B8FF9");

  EXPECT_EQ(1, observer->created_count_);
  EXPECT_EQ(id, observer->last_created_stack_id_);
  EXPECT_EQ(service_.get(), observer->last_created_service_);
}

TEST_F(TabStackServiceTest, CreateStackMultipleUniqueIds) {
  std::string id1 = CreateTestStack("Stack 1", "#5B8FF9");
  std::string id2 = CreateTestStack("Stack 2", "#5AD8A6");
  std::string id3 = CreateTestStack("Stack 3", "#FF6B6B");

  EXPECT_NE(id1, id2);
  EXPECT_NE(id2, id3);
  EXPECT_NE(id1, id3);
}

TEST_F(TabStackServiceTest, CreateStackWithEmptyName) {
  // Creating with empty name should still work (uses default name).
  std::string id = CreateTestStack("", "#5B8FF9");
  EXPECT_FALSE(id.empty());

  const AstraTabStackInfo* stack = service_->GetStack(id);
  ASSERT_NE(nullptr, stack);
  EXPECT_FALSE(stack->name.empty());  // should have default name
}

TEST_F(TabStackServiceTest, CreateStackWithEmptyColorUsesDefault) {
  std::string id = CreateTestStack("Test", "");
  EXPECT_FALSE(id.empty());

  const AstraTabStackInfo* stack = service_->GetStack(id);
  ASSERT_NE(nullptr, stack);
  EXPECT_FALSE(stack->color.empty());
}

TEST_F(TabStackServiceTest, CreateStackHasTimestamps) {
  std::string id = CreateTestStack("Test", "#5B8FF9");

  const AstraTabStackInfo* stack = service_->GetStack(id);
  ASSERT_NE(nullptr, stack);
  EXPECT_FALSE(stack->created_time.is_null());
  EXPECT_FALSE(stack->last_accessed.is_null());
  // created_time and last_accessed should be roughly equal at creation.
  EXPECT_LE((stack->last_accessed - stack->created_time).InSeconds(), 1);
}

TEST_F(TabStackServiceTest, CreateManyStacks) {
  const size_t kNumStacks = 50;
  for (size_t i = 0; i < kNumStacks; ++i) {
    CreateTestStack("Stack " + std::to_string(i), "#5B8FF9");
  }
  EXPECT_EQ(kNumStacks, service_->GetStackCount());
  EXPECT_EQ(kNumStacks, service_->GetAllStacks().size());
}

// =========================================================================
// Stack deletion tests
// =========================================================================

TEST_F(TabStackServiceTest, DeleteStackExisting) {
  std::string id = CreateTestStack("Work", "#5B8FF9");
  ASSERT_EQ(1u, service_->GetStackCount());

  bool result = service_->DeleteStack(id);
  EXPECT_TRUE(result);
  EXPECT_EQ(0u, service_->GetStackCount());
  EXPECT_EQ(nullptr, service_->GetStack(id));
}

TEST_F(TabStackServiceTest, DeleteStackNotifiesObserver) {
  auto* observer = AddTestObserver();
  std::string id = CreateTestStack("Work", "#5B8FF9");

  service_->DeleteStack(id);

  EXPECT_EQ(1, observer->deleted_count_);
  EXPECT_EQ(id, observer->last_deleted_stack_id_);
  EXPECT_EQ(service_.get(), observer->last_deleted_service_);
}

TEST_F(TabStackServiceTest, DeleteStackNonExistentReturnsFalse) {
  bool result = service_->DeleteStack("nonexistent-id");
  EXPECT_FALSE(result);
}

TEST_F(TabStackServiceTest, DeleteStackEmptyId) {
  bool result = service_->DeleteStack("");
  EXPECT_FALSE(result);
}

TEST_F(TabStackServiceTest, DeleteStackDoesNotAffectOthers) {
  std::string id1 = CreateTestStack("Stack 1", "#5B8FF9");
  std::string id2 = CreateTestStack("Stack 2", "#5AD8A6");
  ASSERT_EQ(2u, service_->GetStackCount());

  service_->DeleteStack(id1);

  EXPECT_EQ(1u, service_->GetStackCount());
  EXPECT_NE(nullptr, service_->GetStack(id2));
  EXPECT_EQ(nullptr, service_->GetStack(id1));
}

TEST_F(TabStackServiceTest, DeleteAllStacks) {
  std::string id1 = CreateTestStack("1", "#5B8FF9");
  std::string id2 = CreateTestStack("2", "#5AD8A6");
  std::string id3 = CreateTestStack("3", "#FF6B6B");
  ASSERT_EQ(3u, service_->GetStackCount());

  service_->DeleteStack(id1);
  service_->DeleteStack(id2);
  service_->DeleteStack(id3);

  EXPECT_EQ(0u, service_->GetStackCount());
}

// =========================================================================
// Stack rename tests
// =========================================================================

TEST_F(TabStackServiceTest, RenameStackExisting) {
  std::string id = CreateTestStack("Old Name", "#5B8FF9");

  bool result = service_->RenameStack(id, "New Name");
  EXPECT_TRUE(result);

  const AstraTabStackInfo* stack = service_->GetStack(id);
  ASSERT_NE(nullptr, stack);
  EXPECT_EQ("New Name", stack->name);
}

TEST_F(TabStackServiceTest, RenameStackNotifiesObserver) {
  auto* observer = AddTestObserver();
  std::string id = CreateTestStack("Old", "#5B8FF9");

  service_->RenameStack(id, "New");

  EXPECT_EQ(1, observer->changed_count_);
  EXPECT_EQ(id, observer->last_changed_stack_id_);
}

TEST_F(TabStackServiceTest, RenameStackNonExistentReturnsFalse) {
  bool result = service_->RenameStack("nonexistent", "New Name");
  EXPECT_FALSE(result);
}

TEST_F(TabStackServiceTest, RenameStackSameNameNoChange) {
  std::string id = CreateTestStack("Name", "#5B8FF9");
  auto* observer = AddTestObserver();

  // Renaming to the same name should succeed but not notify.
  bool result = service_->RenameStack(id, "Name");
  EXPECT_TRUE(result);
  EXPECT_EQ(0, observer->changed_count_);
}

TEST_F(TabStackServiceTest, RenameStackToEmpty) {
  std::string id = CreateTestStack("Original", "#5B8FF9");

  bool result = service_->RenameStack(id, "");
  EXPECT_TRUE(result);

  const AstraTabStackInfo* stack = service_->GetStack(id);
  ASSERT_NE(nullptr, stack);
  EXPECT_EQ("", stack->name);
}

// =========================================================================
// Stack color tests
// =========================================================================

TEST_F(TabStackServiceTest, SetStackColorExisting) {
  std::string id = CreateTestStack("Test", "#5B8FF9");

  bool result = service_->SetStackColor(id, "#FF6B6B");
  EXPECT_TRUE(result);

  const AstraTabStackInfo* stack = service_->GetStack(id);
  ASSERT_NE(nullptr, stack);
  EXPECT_EQ("#FF6B6B", stack->color);
}

TEST_F(TabStackServiceTest, SetStackColorNonExistentReturnsFalse) {
  bool result = service_->SetStackColor("nonexistent", "#FF6B6B");
  EXPECT_FALSE(result);
}

TEST_F(TabStackServiceTest, SetStackColorNotifiesObserver) {
  auto* observer = AddTestObserver();
  std::string id = CreateTestStack("Test", "#5B8FF9");

  service_->SetStackColor(id, "#FF6B6B");

  EXPECT_EQ(1, observer->changed_count_);
  EXPECT_EQ(id, observer->last_changed_stack_id_);
}

TEST_F(TabStackServiceTest, SetStackColorSameColorNoNotification) {
  auto* observer = AddTestObserver();
  std::string id = CreateTestStack("Test", "#5B8FF9");

  service_->SetStackColor(id, "#5B8FF9");

  // No state change, so no observer notification.
  EXPECT_EQ(0, observer->changed_count_);
}

// =========================================================================
// Stack query tests
// =========================================================================

TEST_F(TabStackServiceTest, GetStackNonExistentReturnsNull) {
  EXPECT_EQ(nullptr, service_->GetStack("nonexistent"));
}

TEST_F(TabStackServiceTest, GetAllStacksEmpty) {
  std::vector<AstraTabStackInfo> stacks = service_->GetAllStacks();
  EXPECT_TRUE(stacks.empty());
}

TEST_F(TabStackServiceTest, GetAllStacksReturnsAll) {
  CreateTestStack("Stack 1", "#5B8FF9");
  CreateTestStack("Stack 2", "#5AD8A6");
  CreateTestStack("Stack 3", "#FF6B6B");

  std::vector<AstraTabStackInfo> stacks = service_->GetAllStacks();
  EXPECT_EQ(3u, stacks.size());
}

TEST_F(TabStackServiceTest, GetAllStacksSortedByOrderIndex) {
  // Stacks should be returned in order of creation (by order_index).
  std::string id1 = CreateTestStack("First", "#5B8FF9");
  std::string id2 = CreateTestStack("Second", "#5AD8A6");
  std::string id3 = CreateTestStack("Third", "#FF6B6B");

  std::vector<AstraTabStackInfo> stacks = service_->GetAllStacks();
  ASSERT_EQ(3u, stacks.size());
  EXPECT_EQ(id1, stacks[0].stack_id);
  EXPECT_EQ(id2, stacks[1].stack_id);
  EXPECT_EQ(id3, stacks[2].stack_id);
}

TEST_F(TabStackServiceTest, GetStackCountEmpty) {
  EXPECT_EQ(0u, service_->GetStackCount());
}

TEST_F(TabStackServiceTest, GetStackCountMultiple) {
  CreateTestStack("1", "#5B8FF9");
  EXPECT_EQ(1u, service_->GetStackCount());
  CreateTestStack("2", "#5AD8A6");
  EXPECT_EQ(2u, service_->GetStackCount());
}

TEST_F(TabStackServiceTest, DoesStackExistValidId) {
  std::string id = CreateTestStack("Test", "#5B8FF9");
  EXPECT_TRUE(service_->DoesStackExist(id));
}

TEST_F(TabStackServiceTest, DoesStackExistInvalidId) {
  EXPECT_FALSE(service_->DoesStackExist("nonexistent"));
  EXPECT_FALSE(service_->DoesStackExist(""));
}

TEST_F(TabStackServiceTest, DoesStackExistAfterDelete) {
  std::string id = CreateTestStack("Test", "#5B8FF9");
  ASSERT_TRUE(service_->DoesStackExist(id));

  service_->DeleteStack(id);
  EXPECT_FALSE(service_->DoesStackExist(id));
}

TEST_F(TabStackServiceTest, FindStackByName) {
  CreateTestStack("Work", "#5B8FF9");
  CreateTestStack("Personal", "#5AD8A6");

  const AstraTabStackInfo* stack = service_->FindStackByName("Work");
  ASSERT_NE(nullptr, stack);
  EXPECT_EQ("Work", stack->name);
}

TEST_F(TabStackServiceTest, FindStackByNameCaseInsensitive) {
  CreateTestStack("Work Stack", "#5B8FF9");

  const AstraTabStackInfo* stack1 = service_->FindStackByName("work stack");
  const AstraTabStackInfo* stack2 = service_->FindStackByName("WORK STACK");
  const AstraTabStackInfo* stack3 = service_->FindStackByName("Work Stack");

  ASSERT_NE(nullptr, stack1);
  ASSERT_NE(nullptr, stack2);
  ASSERT_NE(nullptr, stack3);
  EXPECT_EQ(stack1, stack2);
  EXPECT_EQ(stack2, stack3);
}

TEST_F(TabStackServiceTest, FindStackByNameNotFoundReturnsNull) {
  CreateTestStack("Work", "#5B8FF9");
  EXPECT_EQ(nullptr, service_->FindStackByName("Nonexistent"));
}

TEST_F(TabStackServiceTest, FindStackByNameEmptyReturnsNull) {
  CreateTestStack("Work", "#5B8FF9");
  EXPECT_EQ(nullptr, service_->FindStackByName(""));
}

// =========================================================================
// Tab membership tests
// =========================================================================

TEST_F(TabStackServiceTest, AddTabToStack) {
  std::string id = CreateTestStack("Work", "#5B8FF9");

  bool result = service_->AddTabToStack(0, id);
  EXPECT_TRUE(result);
  EXPECT_EQ(1u, service_->GetTabCountInStack(id));
  EXPECT_TRUE(service_->IsTabInStack(0, id));
}

TEST_F(TabStackServiceTest, AddTabToStackNotifiesObserver) {
  auto* observer = AddTestObserver();
  std::string id = CreateTestStack("Work", "#5B8FF9");

  service_->AddTabToStack(5, id);

  EXPECT_EQ(1, observer->tab_added_count_);
  EXPECT_EQ(5, observer->last_added_tab_index_);
  EXPECT_EQ(id, observer->last_added_stack_id_);
}

TEST_F(TabStackServiceTest, AddTabToNonExistentStackReturnsFalse) {
  bool result = service_->AddTabToStack(0, "nonexistent");
  EXPECT_FALSE(result);
}

TEST_F(TabStackServiceTest, AddSameTabToSameStackIsNoOp) {
  std::string id = CreateTestStack("Work", "#5B8FF9");
  auto* observer = AddTestObserver();

  ASSERT_TRUE(service_->AddTabToStack(0, id));
  int count_before = observer->tab_added_count_;

  // Adding same tab to same stack should be a no-op.
  bool result = service_->AddTabToStack(0, id);
  EXPECT_FALSE(result);
  EXPECT_EQ(count_before, observer->tab_added_count_);
  EXPECT_EQ(1u, service_->GetTabCountInStack(id));
}

TEST_F(TabStackServiceTest, AddTabToAnotherStackMovesIt) {
  std::string id1 = CreateTestStack("Stack 1", "#5B8FF9");
  std::string id2 = CreateTestStack("Stack 2", "#5AD8A6");
  auto* observer = AddTestObserver();

  ASSERT_TRUE(service_->AddTabToStack(0, id1));
  ASSERT_TRUE(service_->IsTabInStack(0, id1));

  // Move tab to second stack.
  bool result = service_->AddTabToStack(0, id2);
  EXPECT_TRUE(result);

  EXPECT_FALSE(service_->IsTabInStack(0, id1));
  EXPECT_TRUE(service_->IsTabInStack(0, id2));
  EXPECT_EQ(0u, service_->GetTabCountInStack(id1));
  EXPECT_EQ(1u, service_->GetTabCountInStack(id2));

  // Should have fired remove and add.
  EXPECT_EQ(1, observer->tab_removed_count_);
  EXPECT_EQ(2, observer->tab_added_count_);
}

TEST_F(TabStackServiceTest, RemoveTabFromStack) {
  std::string id = CreateTestStack("Work", "#5B8FF9");
  ASSERT_TRUE(service_->AddTabToStack(0, id));
  ASSERT_EQ(1u, service_->GetTabCountInStack(id));

  bool result = service_->RemoveTabFromStack(0, id);
  EXPECT_TRUE(result);
  EXPECT_EQ(0u, service_->GetTabCountInStack(id));
  EXPECT_FALSE(service_->IsTabInStack(0, id));
}

TEST_F(TabStackServiceTest, RemoveTabFromStackNotifiesObserver) {
  auto* observer = AddTestObserver();
  std::string id = CreateTestStack("Work", "#5B8FF9");
  service_->AddTabToStack(3, id);

  service_->RemoveTabFromStack(3, id);

  EXPECT_EQ(1, observer->tab_removed_count_);
  EXPECT_EQ(3, observer->last_removed_tab_index_);
  EXPECT_EQ(id, observer->last_removed_stack_id_);
}

TEST_F(TabStackServiceTest, RemoveTabFromWrongStackReturnsFalse) {
  std::string id = CreateTestStack("Work", "#5B8FF9");
  service_->AddTabToStack(0, id);

  bool result = service_->RemoveTabFromStack(0, "other-stack");
  EXPECT_FALSE(result);
}

TEST_F(TabStackServiceTest, RemoveTabFromAllStacks) {
  std::string id = CreateTestStack("Work", "#5B8FF9");
  service_->AddTabToStack(0, id);
  ASSERT_TRUE(service_->IsTabInStack(0, id));

  bool result = service_->RemoveTabFromAllStacks(0);
  EXPECT_TRUE(result);
  EXPECT_FALSE(service_->IsTabInStack(0, id));
  EXPECT_TRUE(service_->GetStackForTab(0).empty());
}

TEST_F(TabStackServiceTest, RemoveTabFromAllStacksNotInStack) {
  bool result = service_->RemoveTabFromAllStacks(99);
  EXPECT_FALSE(result);
}

TEST_F(TabStackServiceTest, GetTabsInStack) {
  std::string id = CreateTestStack("Work", "#5B8FF9");
  service_->AddTabToStack(0, id);
  service_->AddTabToStack(1, id);
  service_->AddTabToStack(2, id);

  std::vector<int> tabs = service_->GetTabsInStack(id);
  ASSERT_EQ(3u, tabs.size());
  EXPECT_EQ(0, tabs[0]);
  EXPECT_EQ(1, tabs[1]);
  EXPECT_EQ(2, tabs[2]);
}

TEST_F(TabStackServiceTest, GetTabsInStackEmpty) {
  std::string id = CreateTestStack("Empty", "#5B8FF9");
  std::vector<int> tabs = service_->GetTabsInStack(id);
  EXPECT_TRUE(tabs.empty());
}

TEST_F(TabStackServiceTest, GetTabsInStackNonExistent) {
  std::vector<int> tabs = service_->GetTabsInStack("nonexistent");
  EXPECT_TRUE(tabs.empty());
}

TEST_F(TabStackServiceTest, GetTabCountInStack) {
  std::string id = CreateTestStack("Work", "#5B8FF9");
  EXPECT_EQ(0u, service_->GetTabCountInStack(id));

  service_->AddTabToStack(0, id);
  EXPECT_EQ(1u, service_->GetTabCountInStack(id));

  service_->AddTabToStack(1, id);
  EXPECT_EQ(2u, service_->GetTabCountInStack(id));
}

TEST_F(TabStackServiceTest, GetTabCountInStackNonExistent) {
  EXPECT_EQ(0u, service_->GetTabCountInStack("nonexistent"));
}

TEST_F(TabStackServiceTest, GetStackForTab) {
  std::string id = CreateTestStack("Work", "#5B8FF9");
  service_->AddTabToStack(5, id);

  EXPECT_EQ(id, service_->GetStackForTab(5));
}

TEST_F(TabStackServiceTest, GetStackForTabNotInStack) {
  EXPECT_TRUE(service_->GetStackForTab(99).empty());
}

TEST_F(TabStackServiceTest, IsTabInStack) {
  std::string id = CreateTestStack("Work", "#5B8FF9");
  service_->AddTabToStack(3, id);

  EXPECT_TRUE(service_->IsTabInStack(3, id));
  EXPECT_FALSE(service_->IsTabInStack(99, id));
  EXPECT_FALSE(service_->IsTabInStack(3, "nonexistent"));
}

TEST_F(TabStackServiceTest, MoveTabsToStack) {
  std::string id = CreateTestStack("Work", "#5B8FF9");

  std::vector<int> tabs = {0, 1, 2, 3, 4};
  size_t added = service_->MoveTabsToStack(tabs, id);
  EXPECT_EQ(5u, added);
  EXPECT_EQ(5u, service_->GetTabCountInStack(id));
}

TEST_F(TabStackServiceTest, MoveTabsToStackNonExistent) {
  std::vector<int> tabs = {0, 1, 2};
  size_t added = service_->MoveTabsToStack(tabs, "nonexistent");
  EXPECT_EQ(0u, added);
}

TEST_F(TabStackServiceTest, MoveTabsToStackPartialDuplicates) {
  std::string id = CreateTestStack("Work", "#5B8FF9");
  service_->AddTabToStack(0, id);

  std::vector<int> tabs = {0, 1, 2};  // tab 0 is already in the stack
  size_t added = service_->MoveTabsToStack(tabs, id);
  EXPECT_EQ(2u, added);  // only 2 new tabs
  EXPECT_EQ(3u, service_->GetTabCountInStack(id));
}

// =========================================================================
// Tab ordering tests
// =========================================================================

TEST_F(TabStackServiceTest, MoveTabInStackForward) {
  std::string id = CreateTestStack("Work", "#5B8FF9");
  service_->AddTabToStack(0, id);
  service_->AddTabToStack(1, id);
  service_->AddTabToStack(2, id);
  service_->AddTabToStack(3, id);

  // Move tab at index 0 to position 2.
  bool result = service_->MoveTabInStack(0, 2);
  EXPECT_TRUE(result);

  std::vector<int> tabs = service_->GetTabsInStack(id);
  ASSERT_EQ(4u, tabs.size());
  EXPECT_EQ(1, tabs[0]);
  EXPECT_EQ(2, tabs[1]);
  EXPECT_EQ(0, tabs[2]);
  EXPECT_EQ(3, tabs[3]);
}

TEST_F(TabStackServiceTest, MoveTabInStackBackward) {
  std::string id = CreateTestStack("Work", "#5B8FF9");
  service_->AddTabToStack(0, id);
  service_->AddTabToStack(1, id);
  service_->AddTabToStack(2, id);
  service_->AddTabToStack(3, id);

  // Move tab at index 3 to position 0.
  bool result = service_->MoveTabInStack(3, 0);
  EXPECT_TRUE(result);

  std::vector<int> tabs = service_->GetTabsInStack(id);
  ASSERT_EQ(4u, tabs.size());
  EXPECT_EQ(3, tabs[0]);
  EXPECT_EQ(0, tabs[1]);
  EXPECT_EQ(1, tabs[2]);
  EXPECT_EQ(2, tabs[3]);
}

TEST_F(TabStackServiceTest, MoveTabInStackSamePosition) {
  std::string id = CreateTestStack("Work", "#5B8FF9");
  service_->AddTabToStack(0, id);
  service_->AddTabToStack(1, id);

  bool result = service_->MoveTabInStack(0, 0);
  EXPECT_FALSE(result);  // no change
}

TEST_F(TabStackServiceTest, MoveTabInStackTabNotInStack) {
  std::string id = CreateTestStack("Work", "#5B8FF9");
  service_->AddTabToStack(0, id);

  bool result = service_->MoveTabInStack(99, 0);
  EXPECT_FALSE(result);
}

TEST_F(TabStackServiceTest, MoveTabInStackClampsToValidRange) {
  std::string id = CreateTestStack("Work", "#5B8FF9");
  service_->AddTabToStack(0, id);
  service_->AddTabToStack(1, id);
  service_->AddTabToStack(2, id);

  // Move to position way past the end — should clamp.
  bool result = service_->MoveTabInStack(0, 100);
  EXPECT_TRUE(result);

  std::vector<int> tabs = service_->GetTabsInStack(id);
  ASSERT_EQ(3u, tabs.size());
  // Tab 0 was at position 0, moving to position 100, should clamp to position 2 (last).
  EXPECT_EQ(1, tabs[0]);
  EXPECT_EQ(2, tabs[1]);
  EXPECT_EQ(0, tabs[2]);
}

TEST_F(TabStackServiceTest, MoveTabInStackNegativeIndex) {
  std::string id = CreateTestStack("Work", "#5B8FF9");
  service_->AddTabToStack(0, id);
  service_->AddTabToStack(1, id);
  service_->AddTabToStack(2, id);

  // Tab with negative index doesn't exist.
  bool result = service_->MoveTabInStack(-1, 0);
  EXPECT_FALSE(result);
}

// =========================================================================
// Collapse/expand tests
// =========================================================================

TEST_F(TabStackServiceTest, DefaultStackIsExpanded) {
  std::string id = CreateTestStack("Test", "#5B8FF9");
  EXPECT_FALSE(service_->IsStackCollapsed(id));
}

TEST_F(TabStackServiceTest, CollapseStack) {
  std::string id = CreateTestStack("Test", "#5B8FF9");
  ASSERT_FALSE(service_->IsStackCollapsed(id));

  service_->CollapseStack(id);
  EXPECT_TRUE(service_->IsStackCollapsed(id));
}

TEST_F(TabStackServiceTest, CollapseStackNotifiesObserver) {
  auto* observer = AddTestObserver();
  std::string id = CreateTestStack("Test", "#5B8FF9");

  service_->CollapseStack(id);

  EXPECT_EQ(1, observer->collapsed_changed_count_);
  EXPECT_EQ(id, observer->last_collapsed_stack_id_);
  EXPECT_TRUE(observer->last_collapsed_state_);
}

TEST_F(TabStackServiceTest, ExpandStack) {
  std::string id = CreateTestStack("Test", "#5B8FF9");
  service_->CollapseStack(id);
  ASSERT_TRUE(service_->IsStackCollapsed(id));

  service_->ExpandStack(id);
  EXPECT_FALSE(service_->IsStackCollapsed(id));
}

TEST_F(TabStackServiceTest, ExpandStackNotifiesObserver) {
  auto* observer = AddTestObserver();
  std::string id = CreateTestStack("Test", "#5B8FF9");
  service_->CollapseStack(id);
  ASSERT_EQ(1, observer->collapsed_changed_count_);

  service_->ExpandStack(id);

  EXPECT_EQ(2, observer->collapsed_changed_count_);
  EXPECT_EQ(id, observer->last_collapsed_stack_id_);
  EXPECT_FALSE(observer->last_collapsed_state_);
}

TEST_F(TabStackServiceTest, ToggleStackCollapsed) {
  std::string id = CreateTestStack("Test", "#5B8FF9");
  ASSERT_FALSE(service_->IsStackCollapsed(id));

  bool collapsed = service_->ToggleStackCollapsed(id);
  EXPECT_TRUE(collapsed);
  EXPECT_TRUE(service_->IsStackCollapsed(id));

  collapsed = service_->ToggleStackCollapsed(id);
  EXPECT_FALSE(collapsed);
  EXPECT_FALSE(service_->IsStackCollapsed(id));
}

TEST_F(TabStackServiceTest, CollapseAlreadyCollapsedIsNoOp) {
  auto* observer = AddTestObserver();
  std::string id = CreateTestStack("Test", "#5B8FF9");

  service_->CollapseStack(id);
  int count_before = observer->collapsed_changed_count_;

  service_->CollapseStack(id);
  // Should not notify again (no state change).
  EXPECT_EQ(count_before, observer->collapsed_changed_count_);
}

TEST_F(TabStackServiceTest, ExpandAlreadyExpandedIsNoOp) {
  auto* observer = AddTestObserver();
  std::string id = CreateTestStack("Test", "#5B8FF9");

  service_->ExpandStack(id);
  // Should not notify (no state change since already expanded).
  EXPECT_EQ(0, observer->collapsed_changed_count_);
}

TEST_F(TabStackServiceTest, IsStackCollapsedNonExistentReturnsFalse) {
  EXPECT_FALSE(service_->IsStackCollapsed("nonexistent"));
}

TEST_F(TabStackServiceTest, CollapseAllStacks) {
  CreateTestStack("Stack 1", "#5B8FF9");
  CreateTestStack("Stack 2", "#5AD8A6");
  CreateTestStack("Stack 3", "#FF6B6B");

  size_t changed = service_->CollapseAllStacks();
  EXPECT_EQ(3u, changed);

  auto stacks = service_->GetAllStacks();
  for (const auto& stack : stacks) {
    EXPECT_TRUE(stack.is_collapsed);
  }
}

TEST_F(TabStackServiceTest, CollapseAllStacksNotifiesObservers) {
  auto* observer = AddTestObserver();
  CreateTestStack("Stack 1", "#5B8FF9");
  CreateTestStack("Stack 2", "#5AD8A6");

  size_t changed = service_->CollapseAllStacks();

  EXPECT_EQ(2u, changed);
  EXPECT_EQ(2, observer->collapsed_changed_count_);
}

TEST_F(TabStackServiceTest, CollapseAllStacksAlreadyCollapsedIsNoOp) {
  auto* observer = AddTestObserver();
  std::string id = CreateTestStack("Test", "#5B8FF9");
  service_->CollapseStack(id);
  int collapsed_before = observer->collapsed_changed_count_;

  size_t changed = service_->CollapseAllStacks();

  EXPECT_EQ(0u, changed);
  EXPECT_EQ(collapsed_before, observer->collapsed_changed_count_);
}

TEST_F(TabStackServiceTest, CollapseAllStacksEmptyReturnsZero) {
  size_t changed = service_->CollapseAllStacks();
  EXPECT_EQ(0u, changed);
}

TEST_F(TabStackServiceTest, ExpandAllStacks) {
  std::string id1 = CreateTestStack("Stack 1", "#5B8FF9");
  std::string id2 = CreateTestStack("Stack 2", "#5AD8A6");
  service_->CollapseStack(id1);
  service_->CollapseStack(id2);
  ASSERT_TRUE(service_->IsStackCollapsed(id1));
  ASSERT_TRUE(service_->IsStackCollapsed(id2));

  size_t changed = service_->ExpandAllStacks();
  EXPECT_EQ(2u, changed);

  auto stacks = service_->GetAllStacks();
  for (const auto& stack : stacks) {
    EXPECT_FALSE(stack.is_collapsed);
  }
}

TEST_F(TabStackServiceTest, ExpandAllStacksNotifiesObservers) {
  auto* observer = AddTestObserver();
  std::string id1 = CreateTestStack("Stack 1", "#5B8FF9");
  std::string id2 = CreateTestStack("Stack 2", "#5AD8A6");
  service_->CollapseStack(id1);
  service_->CollapseStack(id2);

  size_t changed = service_->ExpandAllStacks();

  EXPECT_EQ(2u, changed);
  EXPECT_EQ(4, observer->collapsed_changed_count_);  // 2 collapse + 2 expand
}

TEST_F(TabStackServiceTest, ExpandAllStacksEmptyReturnsZero) {
  size_t changed = service_->ExpandAllStacks();
  EXPECT_EQ(0u, changed);
}

// =========================================================================
// Close / undo close tests
// =========================================================================

TEST_F(TabStackServiceTest, CloseStackRemovesTabs) {
  std::string id = CreateTestStack("Work", "#5B8FF9");
  service_->AddTabToStack(0, id);
  service_->AddTabToStack(1, id);
  service_->AddTabToStack(2, id);
  ASSERT_EQ(3u, service_->GetTabCountInStack(id));

  size_t closed = service_->CloseStack(id);
  EXPECT_EQ(3u, closed);
  EXPECT_EQ(0u, service_->GetTabCountInStack(id));
  EXPECT_TRUE(service_->DoesStackExist(id));  // stack still exists
}

TEST_F(TabStackServiceTest, CloseEmptyStack) {
  std::string id = CreateTestStack("Empty", "#5B8FF9");

  size_t closed = service_->CloseStack(id);
  EXPECT_EQ(0u, closed);
  EXPECT_TRUE(service_->DoesStackExist(id));
}

TEST_F(TabStackServiceTest, CloseStackNonExistent) {
  size_t closed = service_->CloseStack("nonexistent");
  EXPECT_EQ(0u, closed);
}

TEST_F(TabStackServiceTest, UndoCloseStack) {
  std::string id = CreateTestStack("Work", "#5B8FF9");
  service_->AddTabToStack(0, id);
  service_->AddTabToStack(1, id);

  service_->CloseStack(id);
  ASSERT_EQ(0u, service_->GetTabCountInStack(id));

  bool result = service_->UndoCloseStack(id);
  EXPECT_TRUE(result);
  EXPECT_EQ(2u, service_->GetTabCountInStack(id));
  EXPECT_TRUE(service_->IsTabInStack(0, id));
  EXPECT_TRUE(service_->IsTabInStack(1, id));
}

TEST_F(TabStackServiceTest, UndoCloseStackNoCloseHistory) {
  std::string id = CreateTestStack("Work", "#5B8FF9");

  // No close has been done, so undo should fail.
  bool result = service_->UndoCloseStack(id);
  EXPECT_FALSE(result);
}

TEST_F(TabStackServiceTest, UndoCloseStackNonExistent) {
  bool result = service_->UndoCloseStack("nonexistent");
  EXPECT_FALSE(result);
}

// =========================================================================
// Stack note tests
// =========================================================================

TEST_F(TabStackServiceTest, SetStackNote) {
  std::string id = CreateTestStack("Work", "#5B8FF9");

  bool result = service_->SetStackNote(id, "Research notes for project");
  EXPECT_TRUE(result);
  EXPECT_EQ("Research notes for project", service_->GetStackNote(id));
}

TEST_F(TabStackServiceTest, GetStackNoteDefaultEmpty) {
  std::string id = CreateTestStack("Work", "#5B8FF9");
  EXPECT_TRUE(service_->GetStackNote(id).empty());
}

TEST_F(TabStackServiceTest, GetStackNoteNonExistent) {
  EXPECT_TRUE(service_->GetStackNote("nonexistent").empty());
}

TEST_F(TabStackServiceTest, SetStackNoteNonExistentReturnsFalse) {
  bool result = service_->SetStackNote("nonexistent", "test");
  EXPECT_FALSE(result);
}

TEST_F(TabStackServiceTest, SetStackNoteNotifiesObserver) {
  auto* observer = AddTestObserver();
  std::string id = CreateTestStack("Work", "#5B8FF9");

  service_->SetStackNote(id, "New note");

  EXPECT_EQ(1, observer->changed_count_);
  EXPECT_EQ(id, observer->last_changed_stack_id_);
}

TEST_F(TabStackServiceTest, SetStackNoteSameNoteNoChange) {
  auto* observer = AddTestObserver();
  std::string id = CreateTestStack("Work", "#5B8FF9");
  service_->SetStackNote(id, "Same note");
  ASSERT_EQ(1, observer->changed_count_);

  service_->SetStackNote(id, "Same note");
  EXPECT_EQ(1, observer->changed_count_);  // no second notification
}

// =========================================================================
// Stack pinned tests
// =========================================================================

TEST_F(TabStackServiceTest, DefaultStackIsNotPinned) {
  std::string id = CreateTestStack("Work", "#5B8FF9");
  EXPECT_FALSE(service_->IsStackPinned(id));
}

TEST_F(TabStackServiceTest, SetStackPinned) {
  std::string id = CreateTestStack("Work", "#5B8FF9");

  bool result = service_->SetStackPinned(id, true);
  EXPECT_TRUE(result);
  EXPECT_TRUE(service_->IsStackPinned(id));

  result = service_->SetStackPinned(id, false);
  EXPECT_TRUE(result);
  EXPECT_FALSE(service_->IsStackPinned(id));
}

TEST_F(TabStackServiceTest, SetStackPinnedNonExistentReturnsFalse) {
  bool result = service_->SetStackPinned("nonexistent", true);
  EXPECT_FALSE(result);
}

TEST_F(TabStackServiceTest, IsStackPinnedNonExistentReturnsFalse) {
  EXPECT_FALSE(service_->IsStackPinned("nonexistent"));
}

TEST_F(TabStackServiceTest, PinnedStacksAppearFirst) {
  std::string id1 = CreateTestStack("Regular 1", "#5B8FF9");
  std::string id2 = CreateTestStack("Regular 2", "#5AD8A6");
  std::string id3 = CreateTestStack("Pinned", "#FF6B6B");

  service_->SetStackPinned(id3, true);

  auto stacks = service_->GetAllStacks();
  ASSERT_EQ(3u, stacks.size());
  EXPECT_EQ(id3, stacks[0].stack_id);  // pinned first
  EXPECT_EQ(id1, stacks[1].stack_id);
  EXPECT_EQ(id2, stacks[2].stack_id);
}

TEST_F(TabStackServiceTest, SetStackPinnedNotifiesObserver) {
  auto* observer = AddTestObserver();
  std::string id = CreateTestStack("Work", "#5B8FF9");

  service_->SetStackPinned(id, true);

  // Should fire OnStackChanged and OnStacksReordered.
  EXPECT_GE(observer->changed_count_, 1);
  EXPECT_GE(observer->reordered_count_, 1);
}

TEST_F(TabStackServiceTest, SetStackPinnedSameStateNoChange) {
  auto* observer = AddTestObserver();
  std::string id = CreateTestStack("Work", "#5B8FF9");

  // Already unpinned, setting to false should be no-op.
  bool result = service_->SetStackPinned(id, false);
  EXPECT_TRUE(result);
  EXPECT_EQ(0, observer->changed_count_);
}

// =========================================================================
// Stack timestamp tests
// =========================================================================

TEST_F(TabStackServiceTest, GetStackLastAccessed) {
  std::string id = CreateTestStack("Work", "#5B8FF9");

  base::Time accessed = service_->GetStackLastAccessed(id);
  EXPECT_FALSE(accessed.is_null());
}

TEST_F(TabStackServiceTest, GetStackLastAccessedNonExistent) {
  base::Time accessed = service_->GetStackLastAccessed("nonexistent");
  EXPECT_TRUE(accessed.is_null());
}

TEST_F(TabStackServiceTest, AccessUpdatesLastAccessed) {
  std::string id = CreateTestStack("Work", "#5B8FF9");
  base::Time before = service_->GetStackLastAccessed(id);

  // Advance time and modify the stack — should update last_accessed.
  task_environment_.FastForwardBy(base::Seconds(1));
  service_->RenameStack(id, "New Name");

  base::Time after = service_->GetStackLastAccessed(id);
  EXPECT_GT(after, before);
}

// =========================================================================
// Stack order index tests
// =========================================================================

TEST_F(TabStackServiceTest, GetStackOrderIndex) {
  std::string id1 = CreateTestStack("First", "#5B8FF9");
  std::string id2 = CreateTestStack("Second", "#5AD8A6");
  std::string id3 = CreateTestStack("Third", "#FF6B6B");

  EXPECT_EQ(0, service_->GetStackOrderIndex(id1));
  EXPECT_EQ(1, service_->GetStackOrderIndex(id2));
  EXPECT_EQ(2, service_->GetStackOrderIndex(id3));
}

TEST_F(TabStackServiceTest, GetStackOrderIndexNonExistent) {
  EXPECT_EQ(-1, service_->GetStackOrderIndex("nonexistent"));
}

TEST_F(TabStackServiceTest, SetStackOrderIndex) {
  std::string id1 = CreateTestStack("First", "#5B8FF9");
  std::string id2 = CreateTestStack("Second", "#5AD8A6");
  std::string id3 = CreateTestStack("Third", "#FF6B6B");

  // Move first stack to last position.
  bool result = service_->SetStackOrderIndex(id1, 2);
  EXPECT_TRUE(result);

  auto stacks = service_->GetAllStacks();
  ASSERT_EQ(3u, stacks.size());
  EXPECT_EQ(id2, stacks[0].stack_id);
  EXPECT_EQ(id3, stacks[1].stack_id);
  EXPECT_EQ(id1, stacks[2].stack_id);
}

TEST_F(TabStackServiceTest, SetStackOrderIndexNonExistent) {
  bool result = service_->SetStackOrderIndex("nonexistent", 0);
  EXPECT_FALSE(result);
}

TEST_F(TabStackServiceTest, SetStackOrderIndexNotifiesReorder) {
  auto* observer = AddTestObserver();
  std::string id = CreateTestStack("First", "#5B8FF9");
  CreateTestStack("Second", "#5AD8A6");

  service_->SetStackOrderIndex(id, 1);
  EXPECT_GE(observer->reordered_count_, 1);
}

// =========================================================================
// Bulk operations tests
// =========================================================================

TEST_F(TabStackServiceTest, CloseAllStacks) {
  CreateTestStack("Stack 1", "#5B8FF9");
  CreateTestStack("Stack 2", "#5AD8A6");
  CreateTestStack("Stack 3", "#FF6B6B");
  ASSERT_EQ(3u, service_->GetStackCount());

  size_t closed = service_->CloseAllStacks();
  EXPECT_EQ(3u, closed);
  EXPECT_EQ(0u, service_->GetStackCount());
}

TEST_F(TabStackServiceTest, CloseAllStacksEmpty) {
  size_t closed = service_->CloseAllStacks();
  EXPECT_EQ(0u, closed);
}

TEST_F(TabStackServiceTest, CloseAllStacksNotifiesObservers) {
  auto* observer = AddTestObserver();
  CreateTestStack("Stack 1", "#5B8FF9");
  CreateTestStack("Stack 2", "#5AD8A6");

  service_->CloseAllStacks();

  EXPECT_EQ(2, observer->deleted_count_);
}

TEST_F(TabStackServiceTest, GetStacksByColor) {
  CreateTestStack("Blue 1", "#5B8FF9");
  CreateTestStack("Green 1", "#5AD8A6");
  CreateTestStack("Blue 2", "#5B8FF9");
  CreateTestStack("Red 1", "#E86452");

  auto blue_stacks = service_->GetStacksByColor("#5B8FF9");
  EXPECT_EQ(2u, blue_stacks.size());

  auto green_stacks = service_->GetStacksByColor("#5AD8A6");
  EXPECT_EQ(1u, green_stacks.size());

  auto purple_stacks = service_->GetStacksByColor("#9270CA");
  EXPECT_EQ(0u, purple_stacks.size());
}

TEST_F(TabStackServiceTest, GetStacksByColorCaseInsensitive) {
  CreateTestStack("Blue", "#5B8FF9");

  auto stacks1 = service_->GetStacksByColor("#5b8ff9");
  auto stacks2 = service_->GetStacksByColor("#5B8FF9");

  EXPECT_EQ(1u, stacks1.size());
  EXPECT_EQ(1u, stacks2.size());
}

TEST_F(TabStackServiceTest, GetRecentlyAccessedStacks) {
  std::string id1 = CreateTestStack("Oldest", "#5B8FF9");
  task_environment_.FastForwardBy(base::Seconds(1));
  std::string id2 = CreateTestStack("Middle", "#5AD8A6");
  task_environment_.FastForwardBy(base::Seconds(1));
  std::string id3 = CreateTestStack("Newest", "#FF6B6B");

  // Touch id1 to make it most recent.
  task_environment_.FastForwardBy(base::Seconds(1));
  service_->RenameStack(id1, "Oldest Updated");

  auto recent = service_->GetRecentlyAccessedStacks(3);
  ASSERT_EQ(3u, recent.size());
  EXPECT_EQ(id1, recent[0].stack_id);  // most recently accessed (renamed)
  EXPECT_EQ(id3, recent[1].stack_id);  // created last
  EXPECT_EQ(id2, recent[2].stack_id);  // created middle
}

TEST_F(TabStackServiceTest, GetRecentlyAccessedStacksMaxCount) {
  for (int i = 0; i < 10; ++i) {
    CreateTestStack("Stack " + std::to_string(i), "#5B8FF9");
  }

  auto recent = service_->GetRecentlyAccessedStacks(3);
  EXPECT_EQ(3u, recent.size());
}

TEST_F(TabStackServiceTest, GetRecentlyAccessedStacksNegativeMax) {
  CreateTestStack("Test", "#5B8FF9");

  auto recent = service_->GetRecentlyAccessedStacks(-1);
  // Negative max should return all.
  EXPECT_EQ(1u, recent.size());
}

TEST_F(TabStackServiceTest, GetRecentlyAccessedStacksEmpty) {
  auto recent = service_->GetRecentlyAccessedStacks(5);
  EXPECT_TRUE(recent.empty());
}

TEST_F(TabStackServiceTest, SearchStacksByName) {
  CreateTestStack("Work Projects", "#5B8FF9");
  CreateTestStack("Personal", "#5AD8A6");
  CreateTestStack("Work Notes", "#FF6B6B");

  auto results = service_->SearchStacks("Work");
  EXPECT_EQ(2u, results.size());
}

TEST_F(TabStackServiceTest, SearchStacksByNote) {
  std::string id = CreateTestStack("Research", "#5B8FF9");
  service_->SetStackNote(id, "Contains important documentation");

  auto results = service_->SearchStacks("documentation");
  ASSERT_EQ(1u, results.size());
  EXPECT_EQ(id, results[0].stack_id);
}

TEST_F(TabStackServiceTest, SearchStacksCaseInsensitive) {
  CreateTestStack("Work Stack", "#5B8FF9");

  auto results1 = service_->SearchStacks("work");
  auto results2 = service_->SearchStacks("WORK");
  auto results3 = service_->SearchStacks("Work");

  EXPECT_EQ(1u, results1.size());
  EXPECT_EQ(1u, results2.size());
  EXPECT_EQ(1u, results3.size());
}

TEST_F(TabStackServiceTest, SearchStacksEmptyQueryReturnsAll) {
  CreateTestStack("Stack 1", "#5B8FF9");
  CreateTestStack("Stack 2", "#5AD8A6");

  auto results = service_->SearchStacks("");
  EXPECT_EQ(2u, results.size());
}

TEST_F(TabStackServiceTest, SearchStacksNoMatches) {
  CreateTestStack("Work", "#5B8FF9");

  auto results = service_->SearchStacks("zzzzzzzzz");
  EXPECT_TRUE(results.empty());
}

TEST_F(TabStackServiceTest, MergeStacks) {
  std::string id1 = CreateTestStack("Source", "#5B8FF9");
  std::string id2 = CreateTestStack("Target", "#5AD8A6");

  service_->AddTabToStack(0, id1);
  service_->AddTabToStack(1, id1);
  service_->AddTabToStack(2, id2);

  bool result = service_->MergeStacks(id1, id2);
  EXPECT_TRUE(result);

  EXPECT_FALSE(service_->DoesStackExist(id1));
  EXPECT_TRUE(service_->DoesStackExist(id2));
  EXPECT_EQ(3u, service_->GetTabCountInStack(id2));
}

TEST_F(TabStackServiceTest, MergeStacksSameStack) {
  std::string id = CreateTestStack("Test", "#5B8FF9");
  bool result = service_->MergeStacks(id, id);
  EXPECT_FALSE(result);
}

TEST_F(TabStackServiceTest, MergeStacksSourceNonExistent) {
  std::string id = CreateTestStack("Target", "#5B8FF9");
  bool result = service_->MergeStacks("nonexistent", id);
  EXPECT_FALSE(result);
}

TEST_F(TabStackServiceTest, MergeStacksTargetNonExistent) {
  std::string id = CreateTestStack("Source", "#5B8FF9");
  bool result = service_->MergeStacks(id, "nonexistent");
  EXPECT_FALSE(result);
}

TEST_F(TabStackServiceTest, MergeStacksNotifiesObservers) {
  auto* observer = AddTestObserver();
  std::string id1 = CreateTestStack("Source", "#5B8FF9");
  std::string id2 = CreateTestStack("Target", "#5AD8A6");
  service_->AddTabToStack(0, id1);

  service_->MergeStacks(id1, id2);

  EXPECT_EQ(1, observer->deleted_count_);  // source deleted
  EXPECT_GE(observer->tab_removed_count_, 1);  // tab removed from source
  EXPECT_GE(observer->tab_added_count_, 1);  // tab added to target
}

TEST_F(TabStackServiceTest, DuplicateStack) {
  std::string id = CreateTestStack("Original", "#5B8FF9");
  service_->SetStackNote(id, "Test note");
  service_->AddTabToStack(0, id);
  service_->AddTabToStack(1, id);

  std::string dup_id = service_->DuplicateStack(id);
  EXPECT_FALSE(dup_id.empty());
  EXPECT_NE(id, dup_id);

  const AstraTabStackInfo* dup = service_->GetStack(dup_id);
  ASSERT_NE(nullptr, dup);
  EXPECT_EQ("Original copy", dup->name);
  EXPECT_EQ("#5B8FF9", dup->color);
  EXPECT_EQ("Test note", dup->note);
  EXPECT_EQ(2u, dup->tab_count);
}

TEST_F(TabStackServiceTest, DuplicateStackNonExistent) {
  std::string dup_id = service_->DuplicateStack("nonexistent");
  EXPECT_TRUE(dup_id.empty());
}

TEST_F(TabStackServiceTest, DuplicateStackNotifiesObserver) {
  auto* observer = AddTestObserver();
  std::string id = CreateTestStack("Original", "#5B8FF9");

  std::string dup_id = service_->DuplicateStack(id);
  EXPECT_FALSE(dup_id.empty());
  EXPECT_EQ(2, observer->created_count_);  // original + duplicate
}

// =========================================================================
// Settings tests
// =========================================================================

TEST_F(TabStackServiceTest, DefaultStackColorDefault) {
  EXPECT_EQ(AstraTabStackService::kDefaultStackColor,
            service_->GetDefaultStackColor());
}

TEST_F(TabStackServiceTest, DefaultStackColorSetGet) {
  service_->SetDefaultStackColor("#FF6B6B");
  EXPECT_EQ("#FF6B6B", service_->GetDefaultStackColor());
}

TEST_F(TabStackServiceTest, AutoCollapseInactiveDefault) {
  EXPECT_EQ(AstraTabStackService::kDefaultAutoCollapseInactive,
            service_->GetAutoCollapseInactive());
}

TEST_F(TabStackServiceTest, AutoCollapseInactiveSetGet) {
  service_->SetAutoCollapseInactive(true);
  EXPECT_TRUE(service_->GetAutoCollapseInactive());

  service_->SetAutoCollapseInactive(false);
  EXPECT_FALSE(service_->GetAutoCollapseInactive());
}

TEST_F(TabStackServiceTest, AutoCollapseAfterSecondsDefault) {
  EXPECT_EQ(AstraTabStackService::kDefaultAutoCollapseAfterSeconds,
            service_->GetAutoCollapseAfterSeconds());
}

TEST_F(TabStackServiceTest, AutoCollapseAfterSecondsSetGet) {
  service_->SetAutoCollapseAfterSeconds(600);
  EXPECT_EQ(600, service_->GetAutoCollapseAfterSeconds());
}

TEST_F(TabStackServiceTest, ShowStackCountBadgeDefault) {
  EXPECT_EQ(AstraTabStackService::kDefaultShowCountBadge,
            service_->GetShowStackCountBadge());
}

TEST_F(TabStackServiceTest, ShowStackCountBadgeSetGet) {
  service_->SetShowStackCountBadge(false);
  EXPECT_FALSE(service_->GetShowStackCountBadge());

  service_->SetShowStackCountBadge(true);
  EXPECT_TRUE(service_->GetShowStackCountBadge());
}

TEST_F(TabStackServiceTest, ShowStackTabCountDefault) {
  EXPECT_EQ(AstraTabStackService::kDefaultShowTabCount,
            service_->GetShowStackTabCount());
}

TEST_F(TabStackServiceTest, ShowStackTabCountSetGet) {
  service_->SetShowStackTabCount(false);
  EXPECT_FALSE(service_->GetShowStackTabCount());

  service_->SetShowStackTabCount(true);
  EXPECT_TRUE(service_->GetShowStackTabCount());
}

TEST_F(TabStackServiceTest, StackCreationBehaviorDefault) {
  EXPECT_EQ(AstraTabStackService::kDefaultStackCreationBehavior,
            service_->GetStackCreationBehavior());
}

TEST_F(TabStackServiceTest, StackCreationBehaviorSetGet) {
  service_->SetStackCreationBehavior(
      AstraTabStackService::kStackCreationBehaviorMoveToFront);
  EXPECT_EQ(AstraTabStackService::kStackCreationBehaviorMoveToFront,
            service_->GetStackCreationBehavior());

  service_->SetStackCreationBehavior(
      AstraTabStackService::kStackCreationBehaviorPreserveOrder);
  EXPECT_EQ(AstraTabStackService::kStackCreationBehaviorPreserveOrder,
            service_->GetStackCreationBehavior());
}

TEST_F(TabStackServiceTest, AutoStackRelatedDefault) {
  EXPECT_EQ(AstraTabStackService::kDefaultAutoStackRelated,
            service_->GetAutoStackRelated());
}

TEST_F(TabStackServiceTest, AutoStackRelatedSetGet) {
  service_->SetAutoStackRelated(true);
  EXPECT_TRUE(service_->GetAutoStackRelated());

  service_->SetAutoStackRelated(false);
  EXPECT_FALSE(service_->GetAutoStackRelated());
}

TEST_F(TabStackServiceTest, MinimumTabsPerStackDefault) {
  EXPECT_EQ(AstraTabStackService::kDefaultMinimumTabsPerStack,
            service_->GetMinimumTabsPerStack());
}

TEST_F(TabStackServiceTest, MinimumTabsPerStackSetGet) {
  service_->SetMinimumTabsPerStack(3);
  EXPECT_EQ(3, service_->GetMinimumTabsPerStack());

  service_->SetMinimumTabsPerStack(0);
  EXPECT_EQ(0, service_->GetMinimumTabsPerStack());
}

TEST_F(TabStackServiceTest, SettingsPersistViaPrefs) {
  // Set values via service API.
  service_->SetDefaultStackColor("#FF6B6B");
  service_->SetAutoCollapseInactive(true);
  service_->SetAutoCollapseAfterSeconds(120);
  service_->SetShowStackCountBadge(false);
  service_->SetShowStackTabCount(false);
  service_->SetStackCreationBehavior(
      AstraTabStackService::kStackCreationBehaviorMoveToFront);
  service_->SetAutoStackRelated(true);
  service_->SetMinimumTabsPerStack(5);

  // Verify via PrefService directly.
  PrefService* pref_service = prefs();
  EXPECT_EQ("#FF6B6B",
            pref_service->GetString(AstraTabStackService::kPrefStackDefaultColor));
  EXPECT_TRUE(pref_service->GetBoolean(
      AstraTabStackService::kPrefStackAutoCollapseInactive));
  EXPECT_EQ(120, pref_service->GetInteger(
      AstraTabStackService::kPrefStackAutoCollapseAfterSeconds));
  EXPECT_FALSE(pref_service->GetBoolean(
      AstraTabStackService::kPrefStackShowCountBadge));
  EXPECT_FALSE(pref_service->GetBoolean(
      AstraTabStackService::kPrefStackShowTabCount));
  EXPECT_EQ(AstraTabStackService::kStackCreationBehaviorMoveToFront,
            pref_service->GetInteger(
                AstraTabStackService::kPrefStackCreationBehavior));
  EXPECT_TRUE(pref_service->GetBoolean(
      AstraTabStackService::kPrefStackAutoStackRelated));
  EXPECT_EQ(5, pref_service->GetInteger(
      AstraTabStackService::kPrefStackMinimumTabsPerStack));
}

// =========================================================================
// Observer pattern tests
// =========================================================================

TEST_F(TabStackServiceTest, MultipleObserversAllNotified) {
  auto* observer1 = AddTestObserver();
  auto* observer2 = AddTestObserver();
  auto* observer3 = AddTestObserver();

  CreateTestStack("Test", "#5B8FF9");

  EXPECT_EQ(1, observer1->created_count_);
  EXPECT_EQ(1, observer2->created_count_);
  EXPECT_EQ(1, observer3->created_count_);
}

TEST_F(TabStackServiceTest, RemoveObserverStopsNotifications) {
  auto* observer = AddTestObserver();
  std::string id = CreateTestStack("Stack 1", "#5B8FF9");
  ASSERT_EQ(1, observer->created_count_);

  service_->RemoveObserver(observer);

  CreateTestStack("Stack 2", "#5AD8A6");
  // Should not have been notified after removal.
  EXPECT_EQ(1, observer->created_count_);
}

TEST_F(TabStackServiceTest, ShutdownNotifiesObservers) {
  auto* observer = AddTestObserver();

  service_->Shutdown();

  EXPECT_EQ(1, observer->shutdown_count_);
  EXPECT_EQ(service_.get(), observer->last_shutdown_service_);
}

TEST_F(TabStackServiceTest, ShutdownClearsObservers) {
  auto* observer = AddTestObserver();
  service_->Shutdown();

  // After shutdown, operations should not notify.
  // (But service may be in a shutdown state.)
  EXPECT_EQ(1, observer->shutdown_count_);
}

namespace {

// Partial observer that only overrides one method.
// This tests that observer methods have default empty implementations.
class PartialObserver : public AstraTabStackObserver {
 public:
  void OnStackCreated(AstraTabStackService* service,
                      const std::string& stack_id) override {
    created_count_++;
    last_created_stack_id_ = stack_id;
  }

  int created_count_ = 0;
  std::string last_created_stack_id_;
};

}  // namespace

TEST_F(TabStackServiceTest, PartialObserverOnlyOverridesNeededMethods) {
  // Observers should be able to override only the methods they care about.
  // All other methods have default empty implementations.
  auto partial = std::make_unique<PartialObserver>();
  service_->AddObserver(partial.get());

  CreateTestStack("Test Stack", "#5B8FF9");

  EXPECT_EQ(1, partial->created_count_);

  // Other operations should not crash even though we don't override them.
  std::string id = CreateTestStack("Another", "#FF6B6B");
  service_->RenameStack(id, "Renamed");
  service_->SetStackColor(id, "#5AD8A6");
  service_->SetStackNote(id, "A note");
  service_->SetStackPinned(id, true);
  service_->CollapseStack(id);
  service_->ExpandStack(id);
  service_->DeleteStack(id);

  service_->RemoveObserver(partial.get());
  // No crash = success for the default implementations.
}

// =========================================================================
// AstraTabStackInfo struct tests
// =========================================================================

TEST(AstraTabStackInfoStructTest, DefaultValues) {
  AstraTabStackInfo stack;
  EXPECT_TRUE(stack.stack_id.empty());
  EXPECT_TRUE(stack.name.empty());
  EXPECT_TRUE(stack.color.empty());
  EXPECT_FALSE(stack.is_collapsed);
  EXPECT_FALSE(stack.is_pinned);
  EXPECT_TRUE(stack.note.empty());
  EXPECT_TRUE(stack.tab_indices.empty());
  EXPECT_EQ(0, stack.order_index);
  EXPECT_TRUE(stack.created_time.is_null());
  EXPECT_TRUE(stack.last_accessed.is_null());
  EXPECT_EQ(0u, stack.tab_count);
}

TEST(AstraTabStackInfoStructTest, IsEmpty) {
  AstraTabStackInfo stack;
  EXPECT_TRUE(stack.IsEmpty());

  stack.tab_indices.push_back(0);
  EXPECT_FALSE(stack.IsEmpty());
}

TEST(AstraTabStackInfoStructTest, HasTab) {
  AstraTabStackInfo stack;
  EXPECT_FALSE(stack.HasTab(0));
  EXPECT_FALSE(stack.HasTab(5));

  stack.tab_indices.push_back(3);
  stack.tab_indices.push_back(7);

  EXPECT_TRUE(stack.HasTab(3));
  EXPECT_TRUE(stack.HasTab(7));
  EXPECT_FALSE(stack.HasTab(0));
  EXPECT_FALSE(stack.HasTab(5));
}

// =========================================================================
// Edge case tests
// =========================================================================

TEST_F(TabStackServiceTest, EmptyStackNameCreatesDefault) {
  std::string id = CreateTestStack("", "#5B8FF9");
  const AstraTabStackInfo* stack = service_->GetStack(id);
  ASSERT_NE(nullptr, stack);
  EXPECT_FALSE(stack->name.empty());
}

TEST_F(TabStackServiceTest, InvalidStackWithNoTabs) {
  // Stacks with no tabs should still exist and be queryable.
  std::string id = CreateTestStack("Empty", "#5B8FF9");
  EXPECT_TRUE(service_->DoesStackExist(id));
  EXPECT_EQ(0u, service_->GetTabCountInStack(id));
  EXPECT_TRUE(service_->GetTabsInStack(id).empty());

  const AstraTabStackInfo* stack = service_->GetStack(id);
  ASSERT_NE(nullptr, stack);
  EXPECT_TRUE(stack->IsEmpty());
}

TEST_F(TabStackServiceTest, DeletingNonExistentStack) {
  bool result = service_->DeleteStack("does-not-exist");
  EXPECT_FALSE(result);
}

TEST_F(TabStackServiceTest, AddingSameTabToMultipleStacksMovesIt) {
  std::string id1 = CreateTestStack("Stack 1", "#5B8FF9");
  std::string id2 = CreateTestStack("Stack 2", "#5AD8A6");

  service_->AddTabToStack(0, id1);
  ASSERT_TRUE(service_->IsTabInStack(0, id1));
  ASSERT_FALSE(service_->IsTabInStack(0, id2));

  // Add to second stack — should move, not duplicate.
  service_->AddTabToStack(0, id2);

  EXPECT_FALSE(service_->IsTabInStack(0, id1));
  EXPECT_TRUE(service_->IsTabInStack(0, id2));
  EXPECT_EQ(1u, service_->GetTabCountInStack(id2));
}

TEST_F(TabStackServiceTest, RemovingTabFromNonExistentStack) {
  bool result = service_->RemoveTabFromStack(0, "nonexistent");
  EXPECT_FALSE(result);
}

TEST_F(TabStackServiceTest, RemovingTabNotInStack) {
  std::string id = CreateTestStack("Work", "#5B8FF9");
  service_->AddTabToStack(0, id);

  bool result = service_->RemoveTabFromStack(99, id);
  EXPECT_FALSE(result);
}

TEST_F(TabStackServiceTest, NegativeTabIndex) {
  std::string id = CreateTestStack("Work", "#5B8FF9");

  bool result = service_->AddTabToStack(-1, id);
  EXPECT_FALSE(result);

  EXPECT_FALSE(service_->IsTabInStack(-1, id));
  EXPECT_TRUE(service_->GetStackForTab(-1).empty());
}

TEST_F(TabStackServiceTest, StackCountZeroAfterAllDeleted) {
  std::vector<std::string> ids;
  for (int i = 0; i < 10; ++i) {
    ids.push_back(CreateTestStack("Stack " + std::to_string(i), "#5B8FF9"));
  }

  // Delete every other stack.
  for (size_t i = 0; i < ids.size(); i += 2) {
    service_->DeleteStack(ids[i]);
  }

  EXPECT_EQ(5u, service_->GetStackCount());

  // Delete the rest.
  for (size_t i = 1; i < ids.size(); i += 2) {
    service_->DeleteStack(ids[i]);
  }

  EXPECT_EQ(0u, service_->GetStackCount());
}

TEST_F(TabStackServiceTest, BulkOperationsOnManyStacks) {
  const size_t kNumStacks = 20;
  for (size_t i = 0; i < kNumStacks; ++i) {
    CreateTestStack("Stack " + std::to_string(i), "#5B8FF9");
  }
  ASSERT_EQ(kNumStacks, service_->GetStackCount());

  // Collapse all.
  size_t collapsed = service_->CollapseAllStacks();
  EXPECT_EQ(kNumStacks, collapsed);

  // Collapsing again should be a no-op.
  collapsed = service_->CollapseAllStacks();
  EXPECT_EQ(0u, collapsed);

  // Expand all.
  size_t expanded = service_->ExpandAllStacks();
  EXPECT_EQ(kNumStacks, expanded);

  // Expanding again should be a no-op.
  expanded = service_->ExpandAllStacks();
  EXPECT_EQ(0u, expanded);
}

TEST_F(TabStackServiceTest, FindStackByNameFirstMatchWins) {
  CreateTestStack("Alpha", "#5B8FF9");
  CreateTestStack("alpha", "#FF6B6B");

  // Case-insensitive lookup should return the first matching stack.
  const AstraTabStackInfo* stack = service_->FindStackByName("ALPHA");
  ASSERT_NE(nullptr, stack);
  // The first one created should match (first in order_index order).
  EXPECT_EQ("Alpha", stack->name);
}

TEST_F(TabStackServiceTest, CollapseAllThenExpandAll) {
  auto* observer = AddTestObserver();

  CreateTestStack("A", "#5B8FF9");
  CreateTestStack("B", "#5AD8A6");
  CreateTestStack("C", "#FF6B6B");

  size_t collapsed = service_->CollapseAllStacks();
  EXPECT_EQ(3u, collapsed);
  EXPECT_EQ(3, observer->collapsed_changed_count_);

  size_t expanded = service_->ExpandAllStacks();
  EXPECT_EQ(3u, expanded);
  EXPECT_EQ(6, observer->collapsed_changed_count_);
}

TEST_F(TabStackServiceTest, GetStackColorPaletteIsConsistent) {
  auto palette1 = AstraTabStackService::GetStackColorPalette();
  auto palette2 = AstraTabStackService::GetStackColorPalette();

  ASSERT_EQ(palette1.size(), palette2.size());
  for (size_t i = 0; i < palette1.size(); ++i) {
    EXPECT_EQ(palette1[i], palette2[i]);
  }
}

TEST_F(TabStackServiceTest, GetStackColorPaletteAllValidHex) {
  auto palette = AstraTabStackService::GetStackColorPalette();

  EXPECT_FALSE(palette.empty());
  for (const auto& color : palette) {
    EXPECT_EQ('#', color[0]);
    EXPECT_EQ(7u, color.size());
  }
}

TEST_F(TabStackServiceTest, ServiceShutdown) {
  // The service implements KeyedService::Shutdown() for cleanup.
  // Should clean up observers and any active operations.
  service_->Shutdown();
  // No crash = success.
}

TEST_F(TabStackServiceTest, GetStackPaletteIncludesDefaultColor) {
  auto palette = AstraTabStackService::GetStackColorPalette();

  bool found_default = false;
  for (const auto& color : palette) {
    if (color == AstraTabStackService::kDefaultStackColor) {
      found_default = true;
      break;
    }
  }
  EXPECT_TRUE(found_default);
}

TEST_F(TabStackServiceTest, DuplicateEmptyStack) {
  std::string id = CreateTestStack("Empty", "#5B8FF9");

  std::string dup_id = service_->DuplicateStack(id);
  EXPECT_FALSE(dup_id.empty());
  EXPECT_NE(id, dup_id);

  const AstraTabStackInfo* dup = service_->GetStack(dup_id);
  ASSERT_NE(nullptr, dup);
  EXPECT_TRUE(dup->IsEmpty());
  EXPECT_EQ(0u, dup->tab_count);
}

TEST_F(TabStackServiceTest, MergeEmptyStacks) {
  std::string id1 = CreateTestStack("Empty 1", "#5B8FF9");
  std::string id2 = CreateTestStack("Empty 2", "#5AD8A6");

  bool result = service_->MergeStacks(id1, id2);
  EXPECT_TRUE(result);
  EXPECT_FALSE(service_->DoesStackExist(id1));
  EXPECT_TRUE(service_->DoesStackExist(id2));
  EXPECT_EQ(0u, service_->GetTabCountInStack(id2));
}

TEST_F(TabStackServiceTest, SearchEmptyStacks) {
  auto results = service_->SearchStacks("anything");
  EXPECT_TRUE(results.empty());
}

TEST_F(TabStackServiceTest, RecentlyAccessedEmptyService) {
  auto recent = service_->GetRecentlyAccessedStacks(10);
  EXPECT_TRUE(recent.empty());
}

TEST_F(TabStackServiceTest, StacksByColorEmpty) {
  auto stacks = service_->GetStacksByColor("#5B8FF9");
  EXPECT_TRUE(stacks.empty());
}

TEST_F(TabStackServiceTest, CloseAllOnEmptyService) {
  EXPECT_EQ(0u, service_->CloseAllStacks());
}

TEST_F(TabStackServiceTest, AddTabUpdatesLastAccessed) {
  std::string id = CreateTestStack("Work", "#5B8FF9");
  base::Time before = service_->GetStackLastAccessed(id);

  // Need to advance time.
  task_environment_.FastForwardBy(base::Seconds(1));

  service_->AddTabToStack(0, id);

  base::Time after = service_->GetStackLastAccessed(id);
  EXPECT_GT(after, before);
}

TEST_F(TabStackServiceTest, RemoveTabUpdatesLastAccessed) {
  std::string id = CreateTestStack("Work", "#5B8FF9");
  service_->AddTabToStack(0, id);
  base::Time before = service_->GetStackLastAccessed(id);

  task_environment_.FastForwardBy(base::Seconds(1));

  service_->RemoveTabFromStack(0, id);

  base::Time after = service_->GetStackLastAccessed(id);
  EXPECT_GT(after, before);
}

// =========================================================================
// Integration/documentation tests
// =========================================================================

TEST(AstraTabStackIntegrationTest, ChromiumAnalog) {
  // Astra tab stacks are analogous to Chromium tab groups but richer:
  //   - Named stacks (not just color/label)
  //   - Ordered stack list (sidebar projection)
  //   - Collapsible in sidebar
  //   - Notes and pinning
  //   - Persisted in prefs
  //
  // Chromium owner: TabGroupModel + TabGroup
  //   (chrome/browser/ui/tabs/tab_group.h)
  SUCCEED();
}

TEST(AstraTabStackIntegrationTest, ServiceTruthModel) {
  // Truth model for tab stacks:
  //   - Stack definitions: AstraTabStackService (PrefService-backed)
  //   - Tab membership: stored as tab_indices in stack metadata
  //   - Tab order: TabStripModel (Chromium-owned), mirrored in service
  //   - UI projection: Sidebar views (pure projection, no state)
  //
  // The service is the single source of truth for stack metadata.
  // UI always re-reads from services, never stores state.
  SUCCEED();
}

}  // namespace astra
