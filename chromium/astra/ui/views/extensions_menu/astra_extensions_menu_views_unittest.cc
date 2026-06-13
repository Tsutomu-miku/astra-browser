// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/extensions_menu/astra_extensions_menu_view.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

class AstraExtensionsMenuModelTest : public testing::Test {
 protected:
  void SetUp() override {
    model_ = std::make_unique<AstraExtensionsMenuModel>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<AstraExtensionsMenuModel> model_;
};

// Test that the model starts empty.
TEST_F(AstraExtensionsMenuModelTest, StartsEmpty) {
  EXPECT_EQ(0u, model_->GetExtensionCount());
  EXPECT_EQ(0u, model_->GetAllExtensions().size());
}

// Test PopulateSampleExtensions.
TEST_F(AstraExtensionsMenuModelTest, PopulateSampleExtensions) {
  model_->PopulateSampleExtensions();
  EXPECT_GT(model_->GetExtensionCount(), 0u);
  EXPECT_GT(model_->GetPinnedCount(), 0u);
  EXPECT_GT(model_->GetActiveCount(), 0u);
  EXPECT_GT(model_->GetInactiveCount(), 0u);
  EXPECT_GT(model_->GetBlockedCount(), 0u);
}

// Test SetExtension adds a new extension.
TEST_F(AstraExtensionsMenuModelTest, SetExtensionAdds) {
  AstraExtensionMenuEntry entry;
  entry.extension_id = "test_ext";
  entry.name = u"Test Extension";
  entry.category = AstraExtensionCategory::kActive;
  entry.state = AstraExtensionState::kEnabled;

  model_->SetExtension(entry);
  EXPECT_EQ(1u, model_->GetExtensionCount());

  const AstraExtensionMenuEntry* found = model_->GetExtension("test_ext");
  ASSERT_NE(nullptr, found);
  EXPECT_EQ(u"Test Extension", found->name);
}

// Test SetExtension updates an existing one.
TEST_F(AstraExtensionsMenuModelTest, SetExtensionUpdates) {
  AstraExtensionMenuEntry entry;
  entry.extension_id = "test_ext";
  entry.name = u"Old Name";
  model_->SetExtension(entry);

  entry.name = u"New Name";
  model_->SetExtension(entry);

  EXPECT_EQ(1u, model_->GetExtensionCount());
  const AstraExtensionMenuEntry* found = model_->GetExtension("test_ext");
  ASSERT_NE(nullptr, found);
  EXPECT_EQ(u"New Name", found->name);
}

// Test RemoveExtension.
TEST_F(AstraExtensionsMenuModelTest, RemoveExtension) {
  AstraExtensionMenuEntry entry;
  entry.extension_id = "test_ext";
  entry.name = u"Test";
  model_->SetExtension(entry);
  EXPECT_EQ(1u, model_->GetExtensionCount());

  model_->RemoveExtension("test_ext");
  EXPECT_EQ(0u, model_->GetExtensionCount());
  EXPECT_EQ(nullptr, model_->GetExtension("test_ext"));
}

// Test SetExtensionState.
TEST_F(AstraExtensionsMenuModelTest, SetExtensionState) {
  AstraExtensionMenuEntry entry;
  entry.extension_id = "test_ext";
  entry.state = AstraExtensionState::kEnabled;
  model_->SetExtension(entry);

  model_->SetExtensionState("test_ext", AstraExtensionState::kDisabled);

  const AstraExtensionMenuEntry* found = model_->GetExtension("test_ext");
  ASSERT_NE(nullptr, found);
  EXPECT_EQ(AstraExtensionState::kDisabled, found->state);
}

// Test SetExtensionPinned.
TEST_F(AstraExtensionsMenuModelTest, SetExtensionPinned) {
  AstraExtensionMenuEntry entry;
  entry.extension_id = "test_ext";
  entry.pinned_to_toolbar = false;
  entry.category = AstraExtensionCategory::kActive;
  model_->SetExtension(entry);

  model_->SetExtensionPinned("test_ext", true);

  const AstraExtensionMenuEntry* found = model_->GetExtension("test_ext");
  ASSERT_NE(nullptr, found);
  EXPECT_TRUE(found->pinned_to_toolbar);
}

// Test SetExtensionBadge.
TEST_F(AstraExtensionsMenuModelTest, SetExtensionBadge) {
  AstraExtensionMenuEntry entry;
  entry.extension_id = "test_ext";
  entry.has_badge = false;
  model_->SetExtension(entry);

  model_->SetExtensionBadge("test_ext", u"42", SK_ColorGREEN);

  const AstraExtensionMenuEntry* found = model_->GetExtension("test_ext");
  ASSERT_NE(nullptr, found);
  EXPECT_TRUE(found->has_badge);
  EXPECT_EQ(u"42", found->badge_text);
  EXPECT_EQ(SK_ColorGREEN, found->badge_color);
}

// Test GetExtensionsByCategory.
TEST_F(AstraExtensionsMenuModelTest, GetExtensionsByCategory) {
  model_->PopulateSampleExtensions();

  auto pinned = model_->GetExtensionsByCategory(AstraExtensionCategory::kPinned);
  EXPECT_EQ(model_->GetPinnedCount(), pinned.size());

  auto active = model_->GetExtensionsByCategory(AstraExtensionCategory::kActive);
  EXPECT_EQ(model_->GetActiveCount(), active.size());

  auto inactive = model_->GetExtensionsByCategory(AstraExtensionCategory::kInactive);
  EXPECT_EQ(model_->GetInactiveCount(), inactive.size());

  auto blocked = model_->GetExtensionsByCategory(AstraExtensionCategory::kBlocked);
  EXPECT_EQ(model_->GetBlockedCount(), blocked.size());
}

// Test search filtering.
TEST_F(AstraExtensionsMenuModelTest, SearchFilter) {
  model_->PopulateSampleExtensions();
  size_t total = model_->GetExtensionCount();
  EXPECT_GT(total, 0u);

  // Empty query = all results.
  auto filtered = model_->GetFilteredExtensions();
  EXPECT_EQ(total, filtered.size());

  // Set a specific query.
  model_->SetSearchQuery(u"ad");
  filtered = model_->GetFilteredExtensions();
  EXPECT_LT(filtered.size(), total);

  // Get the query back.
  EXPECT_EQ(u"ad", model_->GetSearchQuery());

  // Clear query.
  model_->SetSearchQuery(u"");
  filtered = model_->GetFilteredExtensions();
  EXPECT_EQ(total, filtered.size());
}

// Test ClearAll.
TEST_F(AstraExtensionsMenuModelTest, ClearAll) {
  model_->PopulateSampleExtensions();
  EXPECT_GT(model_->GetExtensionCount(), 0u);

  model_->ClearAll();
  EXPECT_EQ(0u, model_->GetExtensionCount());
}

// Test compact mode.
TEST_F(AstraExtensionsMenuModelTest, CompactMode) {
  EXPECT_FALSE(model_->GetCompactMode());

  model_->SetCompactMode(true);
  EXPECT_TRUE(model_->GetCompactMode());

  model_->SetCompactMode(false);
  EXPECT_FALSE(model_->GetCompactMode());
}

// Test observer fires on changes.
TEST_F(AstraExtensionsMenuModelTest, ObserverFires) {
  class TestObserver : public AstraExtensionsMenuObserver {
   public:
    int extensions_changed_count = 0;
    int extension_changed_count = 0;
    std::string last_extension_id;

    void OnExtensionsChanged(AstraExtensionsMenuModel* model) override {
      extensions_changed_count++;
    }
    void OnExtensionChanged(AstraExtensionsMenuModel* model,
                            const std::string& extension_id) override {
      extension_changed_count++;
      last_extension_id = extension_id;
    }
  };

  TestObserver observer;
  model_->AddObserver(&observer);

  AstraExtensionMenuEntry entry;
  entry.extension_id = "test_ext";
  entry.name = u"Test";
  model_->SetExtension(entry);
  EXPECT_EQ(1, observer.extensions_changed_count);

  model_->SetExtensionState("test_ext", AstraExtensionState::kDisabled);
  EXPECT_EQ(1, observer.extension_changed_count);
  EXPECT_EQ("test_ext", observer.last_extension_id);

  model_->RemoveObserver(&observer);
}

}  // namespace astra
