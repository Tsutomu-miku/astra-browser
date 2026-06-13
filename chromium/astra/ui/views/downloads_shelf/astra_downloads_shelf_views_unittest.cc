// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/downloads_shelf/astra_downloads_shelf_view.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

class AstraDownloadsShelfViewTest : public testing::Test {
 protected:
  void SetUp() override {}

  base::test::TaskEnvironment task_environment_;
};

// Test basic shelf view construction with null helper.
TEST_F(AstraDownloadsShelfViewTest, CreateWithNullHelper) {
  // Shelf should handle null helper gracefully.
  auto shelf = std::make_unique<AstraDownloadsShelfView>(nullptr);

  EXPECT_FALSE(shelf->IsShelfVisible());
  EXPECT_EQ(nullptr, shelf->helper());
  EXPECT_EQ(nullptr, shelf->delegate());
  EXPECT_TRUE(shelf->GetAutoHide());
  EXPECT_EQ(5, shelf->GetMaxItems());
}

// Test auto-hide settings.
TEST_F(AstraDownloadsShelfViewTest, AutoHideSettings) {
  auto shelf = std::make_unique<AstraDownloadsShelfView>(nullptr);

  EXPECT_TRUE(shelf->GetAutoHide());
  shelf->SetAutoHide(false);
  EXPECT_FALSE(shelf->GetAutoHide());
  shelf->SetAutoHide(true);
  EXPECT_TRUE(shelf->GetAutoHide());

  base::TimeDelta delay = base::Seconds(8);
  EXPECT_EQ(delay, shelf->GetAutoHideDelay());
  shelf->SetAutoHideDelay(base::Seconds(30));
  EXPECT_EQ(base::Seconds(30), shelf->GetAutoHideDelay());
}

// Test max items setting.
TEST_F(AstraDownloadsShelfViewTest, MaxItemsSetting) {
  auto shelf = std::make_unique<AstraDownloadsShelfView>(nullptr);

  EXPECT_EQ(5, shelf->GetMaxItems());
  shelf->SetMaxItems(10);
  EXPECT_EQ(10, shelf->GetMaxItems());

  // Min clamp.
  shelf->SetMaxItems(0);
  EXPECT_EQ(1, shelf->GetMaxItems());
}

// Test shelf item view construction.
TEST_F(AstraDownloadsShelfViewTest, ShelfItemViewBasic) {
  AstraDownloadsShelfItemView item(1, u"test_file.zip");

  EXPECT_EQ(1, item.download_id());
  EXPECT_EQ(AstraDownloadState::kInProgress, item.state());

  // Test state changes.
  item.SetState(AstraDownloadState::kCompleted);
  EXPECT_EQ(AstraDownloadState::kCompleted, item.state());

  item.SetState(AstraDownloadState::kFailed);
  EXPECT_EQ(AstraDownloadState::kFailed, item.state());

  item.SetState(AstraDownloadState::kPaused);
  EXPECT_EQ(AstraDownloadState::kPaused, item.state());
}

// Test shelf item progress.
TEST_F(AstraDownloadsShelfViewTest, ShelfItemProgress) {
  AstraDownloadsShelfItemView item(1, u"test.pdf");

  // Default progress is 0.
  item.SetProgress(0.5);
  // Progress is clamped.
  item.SetProgress(-1.0);
  item.SetProgress(2.0);
  // Should not crash.
  SUCCEED();
}

// Test shelf item bytes and speed.
TEST_F(AstraDownloadsShelfViewTest, ShelfItemBytesAndSpeed) {
  AstraDownloadsShelfItemView item(1, u"test.zip");

  item.SetBytes(1024, 1048576);
  item.SetSpeed(500000);

  // Should not crash.
  SUCCEED();
}

// Test shelf item danger state.
TEST_F(AstraDownloadsShelfViewTest, ShelfItemDanger) {
  AstraDownloadsShelfItemView item(1, u"suspicious.exe");

  item.SetDanger(true);
  // Should not crash, just update visual.
  SUCCEED();

  item.SetDanger(false);
  SUCCEED();
}

// Test shelf item delegate.
TEST_F(AstraDownloadsShelfViewTest, ShelfItemDelegate) {
  AstraDownloadsShelfItemView item(1, u"test.txt");

  // Null delegate should not crash.
  item.SetProgress(0.5);

  // Test with delegate set to null (default).
  // No-op calls should not crash.
  SUCCEED();
}

// Test shelf show/hide.
TEST_F(AstraDownloadsShelfViewTest, ShowHide) {
  auto shelf = std::make_unique<AstraDownloadsShelfView>(nullptr);

  EXPECT_FALSE(shelf->IsShelfVisible());

  shelf->Show();
  EXPECT_TRUE(shelf->IsShelfVisible());

  shelf->Hide();
  EXPECT_FALSE(shelf->IsShelfVisible());

  // Double-show should be no-op.
  shelf->Show();
  shelf->Show();
  EXPECT_TRUE(shelf->IsShelfVisible());
}

// Test preferred size.
TEST_F(AstraDownloadsShelfViewTest, PreferredSize) {
  auto shelf = std::make_unique<AstraDownloadsShelfView>(nullptr);
  gfx::Size size = shelf->CalculatePreferredSize(views::SizeBounds());
  EXPECT_EQ(48, size.height());
}

// Test item preferred size.
TEST_F(AstraDownloadsShelfViewTest, ItemPreferredSize) {
  AstraDownloadsShelfItemView item(1, u"test.txt");
  gfx::Size size = item.CalculatePreferredSize(views::SizeBounds());
  EXPECT_EQ(180, size.width());
  EXPECT_EQ(36, size.height());
}

// Test tooltip text.
TEST_F(AstraDownloadsShelfViewTest, ItemTooltipText) {
  AstraDownloadsShelfItemView item(1, u"test_file_with_long_name.pdf");
  std::u16string tooltip = item.GetTooltipText(gfx::Point());
  EXPECT_FALSE(tooltip.empty());
  // Should contain filename.
  EXPECT_NE(std::u16string::npos, tooltip.find(u"test_file_with_long_name.pdf"));
}

// Test URL setter.
TEST_F(AstraDownloadsShelfViewTest, ItemURL) {
  AstraDownloadsShelfItemView item(1, u"test.txt");
  item.SetURL(GURL("https://example.com/download"));
  // Should not crash.
  SUCCEED();
}

// Test all download states update display.
TEST_F(AstraDownloadsShelfViewTest, AllStatesUpdateDisplay) {
  AstraDownloadsShelfItemView item(1, u"test.zip");

  item.SetState(AstraDownloadState::kInProgress);
  item.SetState(AstraDownloadState::kPaused);
  item.SetState(AstraDownloadState::kCompleted);
  item.SetState(AstraDownloadState::kFailed);
  item.SetState(AstraDownloadState::kCancelled);
  item.SetState(AstraDownloadState::kInterrupted);
  item.SetState(AstraDownloadState::kDangerous);
  item.SetState(AstraDownloadState::kUnknown);

  // None should crash.
  SUCCEED();
}

// Test helper setter.
TEST_F(AstraDownloadsShelfViewTest, SetHelper) {
  auto shelf = std::make_unique<AstraDownloadsShelfView>(nullptr);

  EXPECT_EQ(nullptr, shelf->helper());

  // Setting to null again should not crash.
  shelf->SetHelper(nullptr);
  EXPECT_EQ(nullptr, shelf->helper());
}

// Test delegate setter.
TEST_F(AstraDownloadsShelfViewTest, SetDelegate) {
  auto shelf = std::make_unique<AstraDownloadsShelfView>(nullptr);

  EXPECT_EQ(nullptr, shelf->delegate());

  shelf->set_delegate(nullptr);
  EXPECT_EQ(nullptr, shelf->delegate());
}

// Test cancel auto-hide.
TEST_F(AstraDownloadsShelfViewTest, CancelAutoHide) {
  auto shelf = std::make_unique<AstraDownloadsShelfView>(nullptr);

  // Cancel on no-op timer should not crash.
  shelf->CancelAutoHide();
  SUCCEED();
}

// Test hide after delay.
TEST_F(AstraDownloadsShelfViewTest, HideAfterDelay) {
  auto shelf = std::make_unique<AstraDownloadsShelfView>(nullptr);

  shelf->Show();
  shelf->HideAfterDelay(base::Milliseconds(10));

  // Timer is running but task not flushed yet.
  EXPECT_TRUE(shelf->IsShelfVisible());
}

// TODO(astra): Add tests with real AstraDownloadsHelper (mock).
// TODO(astra): Add tests for item view layout correctness.
// TODO(astra): Add tests for shelf view with ViewsTestBase.

}  // namespace astra
