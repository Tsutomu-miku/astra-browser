// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/downloads/astra_downloads_bubble_model.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

class AstraDownloadsBubbleModelTest : public testing::Test {
 protected:
  void SetUp() override {
    // In a real test, we'd create a mock AstraDownloadsHelper.
    // For now, test with a null helper (basic functionality only).
  }

  base::test::TaskEnvironment task_environment_;
};

TEST_F(AstraDownloadsBubbleModelTest, FormatBytes) {
  EXPECT_EQ(u"0 B", AstraDownloadsBubbleModel::FormatBytes(0));
  EXPECT_EQ(u"512 B", AstraDownloadsBubbleModel::FormatBytes(512));
  EXPECT_EQ(u"1.00 KB", AstraDownloadsBubbleModel::FormatBytes(1024));
  EXPECT_EQ(u"1.5 KB", AstraDownloadsBubbleModel::FormatBytes(1536));
  EXPECT_EQ(u"1.00 MB",
            AstraDownloadsBubbleModel::FormatBytes(1024 * 1024));
  EXPECT_EQ(u"1.00 GB",
            AstraDownloadsBubbleModel::FormatBytes(1024 * 1024 * 1024));
}

TEST_F(AstraDownloadsBubbleModelTest, FormatSpeed) {
  EXPECT_EQ(u"—", AstraDownloadsBubbleModel::FormatSpeed(0));
  EXPECT_EQ(u"—", AstraDownloadsBubbleModel::FormatSpeed(-1));
  EXPECT_TRUE(
      AstraDownloadsBubbleModel::FormatSpeed(1024).find(u"/s") !=
      std::u16string::npos);
}

TEST_F(AstraDownloadsBubbleModelTest, FormatTimeRemaining) {
  EXPECT_EQ(u"Calculating...",
            AstraDownloadsBubbleModel::FormatTimeRemaining(
                base::TimeDelta()));
  EXPECT_EQ(u"Calculating...",
            AstraDownloadsBubbleModel::FormatTimeRemaining(
                base::Seconds(-10)));
  EXPECT_EQ(u"30s", AstraDownloadsBubbleModel::FormatTimeRemaining(
                       base::Seconds(30)));
  EXPECT_EQ(u"1m", AstraDownloadsBubbleModel::FormatTimeRemaining(
                       base::Minutes(1)));
  EXPECT_EQ(u"2m 30s", AstraDownloadsBubbleModel::FormatTimeRemaining(
                           base::Minutes(2) + base::Seconds(30)));
  EXPECT_EQ(u"1h", AstraDownloadsBubbleModel::FormatTimeRemaining(
                       base::Hours(1)));
  EXPECT_EQ(u"1h 30m", AstraDownloadsBubbleModel::FormatTimeRemaining(
                           base::Hours(1) + base::Minutes(30)));
}

TEST_F(AstraDownloadsBubbleModelTest, CalculateProgress) {
  EXPECT_DOUBLE_EQ(0.0, AstraDownloadsBubbleModel::CalculateProgress(0, 0));
  EXPECT_DOUBLE_EQ(0.0, AstraDownloadsBubbleModel::CalculateProgress(-1, 100));
  EXPECT_DOUBLE_EQ(1.0,
                   AstraDownloadsBubbleModel::CalculateProgress(100, 100));
  EXPECT_DOUBLE_EQ(1.0,
                   AstraDownloadsBubbleModel::CalculateProgress(200, 100));
  EXPECT_DOUBLE_EQ(0.5,
                   AstraDownloadsBubbleModel::CalculateProgress(50, 100));
  EXPECT_DOUBLE_EQ(0.0, AstraDownloadsBubbleModel::CalculateProgress(0, 100));
}

TEST_F(AstraDownloadsBubbleModelTest, SortOrder) {
  // Create a model with null helper (testing sort logic indirectly).
  // In a real test, we'd populate items and verify sorting.
  // For now, verify that the sort order can be set and retrieved.
  // TODO(astra): Add a mock AstraDownloadsHelper for full model testing.
  AstraDownloadsBubbleModel model(nullptr);

  model.SetSortOrder(AstraDownloadsBubbleSortOrder::kOldestFirst);
  EXPECT_EQ(AstraDownloadsBubbleSortOrder::kOldestFirst,
            model.GetSortOrder());

  model.SetSortOrder(AstraDownloadsBubbleSortOrder::kLargestFirst);
  EXPECT_EQ(AstraDownloadsBubbleSortOrder::kLargestFirst,
            model.GetSortOrder());
}

TEST_F(AstraDownloadsBubbleModelTest, MaxDisplayItems) {
  AstraDownloadsBubbleModel model(nullptr);

  // Default value.
  EXPECT_EQ(AstraDownloadsBubbleModel::kDefaultMaxDisplayItems,
            model.GetMaxDisplayItems());

  // Set within range.
  model.SetMaxDisplayItems(10);
  EXPECT_EQ(10, model.GetMaxDisplayItems());

  // Clamping: below minimum.
  model.SetMaxDisplayItems(0);
  EXPECT_EQ(AstraDownloadsBubbleModel::kMinDisplayItems,
            model.GetMaxDisplayItems());

  // Clamping: above maximum.
  model.SetMaxDisplayItems(100);
  EXPECT_EQ(AstraDownloadsBubbleModel::kMaxDisplayItems,
            model.GetMaxDisplayItems());
}

TEST_F(AstraDownloadsBubbleModelTest, DisplayOptions) {
  AstraDownloadsBubbleModel model(nullptr);

  // Defaults.
  EXPECT_TRUE(model.show_file_size());
  EXPECT_TRUE(model.show_speed());
  EXPECT_TRUE(model.show_time_remaining());
  EXPECT_TRUE(model.show_progress_ring());
  EXPECT_TRUE(model.show_badge());

  // Toggle each.
  model.set_show_file_size(false);
  EXPECT_FALSE(model.show_file_size());

  model.set_show_speed(false);
  EXPECT_FALSE(model.show_speed());

  model.set_show_time_remaining(false);
  EXPECT_FALSE(model.show_time_remaining());

  model.set_show_progress_ring(false);
  EXPECT_FALSE(model.show_progress_ring());

  model.set_show_badge(false);
  EXPECT_FALSE(model.show_badge());
}

TEST_F(AstraDownloadsBubbleModelTest, ShowFilters) {
  AstraDownloadsBubbleModel model(nullptr);

  // Defaults.
  EXPECT_TRUE(model.GetShowCompleted());
  EXPECT_FALSE(model.GetShowFailed());

  // Toggle.
  model.SetShowCompleted(false);
  EXPECT_FALSE(model.GetShowCompleted());

  model.SetShowFailed(true);
  EXPECT_TRUE(model.GetShowFailed());
}

TEST_F(AstraDownloadsBubbleModelTest, NullHelper) {
  // Test that the model handles a null helper gracefully.
  AstraDownloadsBubbleModel model(nullptr);

  EXPECT_EQ(0, model.GetActiveDownloadCount());
  EXPECT_FALSE(model.HasActiveDownloads());
  EXPECT_EQ(0, model.GetTotalDownloadCount());
  EXPECT_TRUE(model.GetDisplayDownloads().empty());
  EXPECT_EQ(0u, model.GetDisplayCount());
  EXPECT_EQ(nullptr, model.GetDownload("nonexistent"));

  // Actions should be no-ops with null helper.
  model.PauseDownload("1");
  model.ResumeDownload("1");
  model.CancelDownload("1");
  model.RemoveDownload("1");
  model.OpenDownload("1");
  model.ShowDownloadInFolder("1");
  model.RetryDownload("1");
  model.PauseAllDownloads();
  model.ResumeAllDownloads();
  model.ClearCompletedDownloads();
  model.Refresh();
}

// TODO(astra): Add tests for AstraDownloadsBubbleItemView
// TODO(astra): Add tests for AstraDownloadsBubbleView
// TODO(astra): Add tests for AstraDownloadsToolbarButton
// These require views test support (ViewsTestBase).

}  // namespace astra
