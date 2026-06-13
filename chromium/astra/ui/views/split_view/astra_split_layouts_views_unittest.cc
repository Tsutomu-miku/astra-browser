// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/split_view/astra_split_layout_picker_view.h"
#include "astra/ui/views/split_view/astra_split_layouts_model.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

// ===========================================================================
// AstraSplitLayoutsModelTest
// ===========================================================================

class AstraSplitLayoutsModelTest : public testing::Test {
 protected:
  void SetUp() override {
    model_ = std::make_unique<AstraSplitLayoutsModel>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<AstraSplitLayoutsModel> model_;
};

// Test model creation with built-in layouts.
TEST_F(AstraSplitLayoutsModelTest, Creation) {
  EXPECT_GT(model_->GetLayoutCount(), 0u);
  EXPECT_GT(model_->GetBuiltInLayoutCount(), 0u);
  EXPECT_EQ(0u, model_->GetUserLayoutCount());
}

// Test built-in layouts are populated.
TEST_F(AstraSplitLayoutsModelTest, BuiltInLayouts) {
  auto builtins = model_->GetBuiltInLayouts();
  EXPECT_GT(builtins.size(), 5u);

  for (const auto& layout : builtins) {
    EXPECT_TRUE(layout.is_builtin);
    EXPECT_FALSE(layout.id.empty());
    EXPECT_FALSE(layout.name.empty());
    EXPECT_GT(layout.pane_ratios.size(), 0u);
  }
}

// Test saving a user layout.
TEST_F(AstraSplitLayoutsModelTest, SaveLayout) {
  size_t initial_count = model_->GetUserLayoutCount();

  std::string id = model_->SaveLayout(
      u"My Layout", u"Custom split layout",
      AstraSplitLayoutMode::kTwoPaneHorizontal,
      {0.6f, 0.4f}, "split_h");

  EXPECT_FALSE(id.empty());
  EXPECT_EQ(initial_count + 1, model_->GetUserLayoutCount());

  const auto* layout = model_->GetLayout(id);
  ASSERT_NE(nullptr, layout);
  EXPECT_EQ(u"My Layout", layout->name);
  EXPECT_FALSE(layout->is_builtin);
  EXPECT_EQ(AstraSplitLayoutMode::kTwoPaneHorizontal, layout->layout_mode);
  ASSERT_EQ(2u, layout->pane_ratios.size());
  EXPECT_FLOAT_EQ(0.6f, layout->pane_ratios[0]);
  EXPECT_FLOAT_EQ(0.4f, layout->pane_ratios[1]);
}

// Test deleting a user layout.
TEST_F(AstraSplitLayoutsModelTest, DeleteLayout) {
  std::string id = model_->SaveLayout(
      u"To Delete", u"Will be removed",
      AstraSplitLayoutMode::kTwoPaneVertical, {0.5f, 0.5f});

  EXPECT_EQ(1u, model_->GetUserLayoutCount());

  model_->DeleteLayout(id);
  EXPECT_EQ(0u, model_->GetUserLayoutCount());
  EXPECT_EQ(nullptr, model_->GetLayout(id));
}

// Test cannot delete built-in layout.
TEST_F(AstraSplitLayoutsModelTest, CannotDeleteBuiltIn) {
  auto builtins = model_->GetBuiltInLayouts();
  ASSERT_GT(builtins.size(), 0u);

  size_t before = model_->GetBuiltInLayoutCount();
  model_->DeleteLayout(builtins[0].id);
  EXPECT_EQ(before, model_->GetBuiltInLayoutCount());
}

// Test apply layout updates metadata.
TEST_F(AstraSplitLayoutsModelTest, ApplyLayout) {
  std::string id = model_->SaveLayout(
      u"Test Apply", u"Testing apply",
      AstraSplitLayoutMode::kThreePaneHorizontal,
      {0.33f, 0.34f, 0.33f});

  const auto* layout = model_->GetLayout(id);
  ASSERT_NE(nullptr, layout);
  EXPECT_EQ(0, layout->use_count);

  model_->ApplyLayout(id);
  layout = model_->GetLayout(id);
  ASSERT_NE(nullptr, layout);
  EXPECT_EQ(1, layout->use_count);
}

// Test apply non-existent layout is no-op.
TEST_F(AstraSplitLayoutsModelTest, ApplyNonExistent) {
  // Should not crash.
  model_->ApplyLayout("nonexistent_layout_id");
}

// Test recent layouts ordering.
TEST_F(AstraSplitLayoutsModelTest, RecentLayouts) {
  std::string id1 = model_->SaveLayout(
      u"Layout A", u"First layout",
      AstraSplitLayoutMode::kTwoPaneHorizontal, {0.5f, 0.5f});
  std::string id2 = model_->SaveLayout(
      u"Layout B", u"Second layout",
      AstraSplitLayoutMode::kGridTwoByTwo, {0.5f, 0.5f, 0.5f, 0.5f});

  // Apply id2 after id1, so id2 should be more recent.
  model_->ApplyLayout(id1);
  model_->ApplyLayout(id2);

  auto recent = model_->GetRecentLayouts(5);
  EXPECT_GE(recent.size(), 2u);
  // Most recent first.
  EXPECT_EQ(id2, recent[0].id);
  EXPECT_EQ(id1, recent[1].id);
}

// Test favorite layouts by use count.
TEST_F(AstraSplitLayoutsModelTest, FavoriteLayouts) {
  std::string id1 = model_->SaveLayout(
      u"Frequent", u"Used often",
      AstraSplitLayoutMode::kTwoPaneHorizontal, {0.7f, 0.3f});
  std::string id2 = model_->SaveLayout(
      u"Rare", u"Used rarely",
      AstraSplitLayoutMode::kPictureInPicture, {0.85f, 0.15f});

  // Use id1 more times.
  for (int i = 0; i < 5; i++) model_->ApplyLayout(id1);
  model_->ApplyLayout(id2);

  auto favs = model_->GetFavoriteLayouts(5);
  EXPECT_GE(favs.size(), 2u);
  EXPECT_EQ(id1, favs[0].id);
  EXPECT_EQ(id2, favs[1].id);
}

// Test rename layout.
TEST_F(AstraSplitLayoutsModelTest, RenameLayout) {
  std::string id = model_->SaveLayout(
      u"Old Name", u"Original",
      AstraSplitLayoutMode::kTwoPaneHorizontal, {0.5f, 0.5f});

  model_->RenameLayout(id, u"New Name");

  const auto* layout = model_->GetLayout(id);
  ASSERT_NE(nullptr, layout);
  EXPECT_EQ(u"New Name", layout->name);
}

// Test cannot rename built-in layout.
TEST_F(AstraSplitLayoutsModelTest, CannotRenameBuiltIn) {
  auto builtins = model_->GetBuiltInLayouts();
  ASSERT_GT(builtins.size(), 0u);

  auto original_name = builtins[0].name;
  model_->RenameLayout(builtins[0].id, u"New Name");

  const auto* layout = model_->GetLayout(builtins[0].id);
  ASSERT_NE(nullptr, layout);
  EXPECT_EQ(original_name, layout->name);
}

// Test observer pattern.
TEST_F(AstraSplitLayoutsModelTest, Observer) {
  class TestObserver : public AstraSplitLayoutsObserver {
   public:
    void OnLayoutSaved(const AstraSplitLayout& layout) override {
      saved_count_++;
      last_saved_id_ = layout.id;
    }
    void OnLayoutDeleted(const std::string& layout_id) override {
      deleted_count_++;
      last_deleted_id_ = layout_id;
    }
    void OnLayoutApplied(const std::string& layout_id) override {
      applied_count_++;
      last_applied_id_ = layout_id;
    }
    void OnLayoutsReordered() override {
      reordered_count_++;
    }

    int saved_count_ = 0;
    int deleted_count_ = 0;
    int applied_count_ = 0;
    int reordered_count_ = 0;
    std::string last_saved_id_;
    std::string last_deleted_id_;
    std::string last_applied_id_;
  };

  TestObserver observer;
  model_->AddObserver(&observer);

  // Save.
  std::string id = model_->SaveLayout(
      u"Observer Test", u"Testing",
      AstraSplitLayoutMode::kTabShift, {0.2f, 0.8f});
  EXPECT_EQ(1, observer.saved_count_);
  EXPECT_EQ(id, observer.last_saved_id_);

  // Apply.
  model_->ApplyLayout(id);
  EXPECT_EQ(1, observer.applied_count_);
  EXPECT_EQ(id, observer.last_applied_id_);

  // Delete.
  model_->DeleteLayout(id);
  EXPECT_EQ(1, observer.deleted_count_);
  EXPECT_EQ(id, observer.last_deleted_id_);

  model_->RemoveObserver(&observer);
}

// Test all layout modes exist in built-ins.
TEST_F(AstraSplitLayoutsModelTest, LayoutModes) {
  auto builtins = model_->GetBuiltInLayouts();

  bool has_two_horizontal = false;
  bool has_two_vertical = false;
  bool has_three_horizontal = false;
  bool has_grid = false;
  bool has_pip = false;
  bool has_tab_shift = false;

  for (const auto& layout : builtins) {
    switch (layout.layout_mode) {
      case AstraSplitLayoutMode::kTwoPaneHorizontal:
        has_two_horizontal = true;
        break;
      case AstraSplitLayoutMode::kTwoPaneVertical:
        has_two_vertical = true;
        break;
      case AstraSplitLayoutMode::kThreePaneHorizontal:
        has_three_horizontal = true;
        break;
      case AstraSplitLayoutMode::kGridTwoByTwo:
        has_grid = true;
        break;
      case AstraSplitLayoutMode::kPictureInPicture:
        has_pip = true;
        break;
      case AstraSplitLayoutMode::kTabShift:
        has_tab_shift = true;
        break;
      default:
        break;
    }
  }

  EXPECT_TRUE(has_two_horizontal);
  EXPECT_TRUE(has_two_vertical);
  EXPECT_TRUE(has_three_horizontal);
  EXPECT_TRUE(has_grid);
  EXPECT_TRUE(has_pip);
  EXPECT_TRUE(has_tab_shift);
}

// Test accent color on built-in layouts.
TEST_F(AstraSplitLayoutsModelTest, AccentColors) {
  auto builtins = model_->GetBuiltInLayouts();
  for (const auto& layout : builtins) {
    EXPECT_FALSE(layout.accent_color.empty());
    EXPECT_EQ('#', layout.accent_color[0]);
  }
}

// Test loading state.
TEST_F(AstraSplitLayoutsModelTest, LoadingState) {
  EXPECT_FALSE(model_->IsLoading());
  model_->SetLoading(true);
  EXPECT_TRUE(model_->IsLoading());
  model_->SetLoading(false);
  EXPECT_FALSE(model_->IsLoading());
}

}  // namespace astra
