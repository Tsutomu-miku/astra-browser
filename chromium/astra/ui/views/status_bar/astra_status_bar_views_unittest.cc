// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/status_bar/astra_status_bar_view.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

class AstraStatusBarViewTest : public testing::Test {
 protected:
  void SetUp() override {}

  base::test::TaskEnvironment task_environment_;
};

// Test basic status bar construction.
TEST_F(AstraStatusBarViewTest, Create) {
  auto status_bar = std::make_unique<AstraStatusBarView>();

  EXPECT_EQ(AstraSecurityLevel::kNone, status_bar->security_level());
  EXPECT_DOUBLE_EQ(1.0, status_bar->zoom_level());
  EXPECT_EQ(0, status_bar->download_count());
}

// Test status text.
TEST_F(AstraStatusBarViewTest, StatusText) {
  auto status_bar = std::make_unique<AstraStatusBarView>();

  status_bar->SetStatusText(u"https://example.com/page");
  // Should not crash.
  SUCCEED();

  status_bar->ClearStatusText();
  SUCCEED();
}

// Test page URL.
TEST_F(AstraStatusBarViewTest, PageURL) {
  auto status_bar = std::make_unique<AstraStatusBarView>();

  status_bar->SetPageURL(GURL("https://example.com"));
  SUCCEED();

  status_bar->SetPageURL(GURL());
  SUCCEED();
}

// Test security levels.
TEST_F(AstraStatusBarViewTest, SecurityLevels) {
  auto status_bar = std::make_unique<AstraStatusBarView>();

  status_bar->SetSecurityLevel(AstraSecurityLevel::kSecure);
  EXPECT_EQ(AstraSecurityLevel::kSecure, status_bar->security_level());

  status_bar->SetSecurityLevel(AstraSecurityLevel::kSecureWithWarning);
  EXPECT_EQ(AstraSecurityLevel::kSecureWithWarning,
            status_bar->security_level());

  status_bar->SetSecurityLevel(AstraSecurityLevel::kDangerous);
  EXPECT_EQ(AstraSecurityLevel::kDangerous, status_bar->security_level());

  status_bar->SetSecurityLevel(AstraSecurityLevel::kInternalPage);
  EXPECT_EQ(AstraSecurityLevel::kInternalPage, status_bar->security_level());

  status_bar->SetSecurityLevel(AstraSecurityLevel::kNone);
  EXPECT_EQ(AstraSecurityLevel::kNone, status_bar->security_level());
}

// Test zoom level.
TEST_F(AstraStatusBarViewTest, ZoomLevel) {
  auto status_bar = std::make_unique<AstraStatusBarView>();

  EXPECT_DOUBLE_EQ(1.0, status_bar->zoom_level());

  status_bar->SetZoomLevel(1.5);
  EXPECT_DOUBLE_EQ(1.5, status_bar->zoom_level());

  status_bar->SetZoomLevel(0.5);
  EXPECT_DOUBLE_EQ(0.5, status_bar->zoom_level());

  status_bar->SetZoomLevel(2.0);
  EXPECT_DOUBLE_EQ(2.0, status_bar->zoom_level());
}

// Test download count.
TEST_F(AstraStatusBarViewTest, DownloadCount) {
  auto status_bar = std::make_unique<AstraStatusBarView>();

  EXPECT_EQ(0, status_bar->download_count());

  status_bar->SetDownloadCount(3);
  EXPECT_EQ(3, status_bar->download_count());

  status_bar->SetDownloadCount(0);
  EXPECT_EQ(0, status_bar->download_count());

  // Negative should be clamped to 0.
  status_bar->SetDownloadCount(-5);
  EXPECT_EQ(0, status_bar->download_count());
}

// Test preferred size.
TEST_F(AstraStatusBarViewTest, PreferredSize) {
  auto status_bar = std::make_unique<AstraStatusBarView>();
  gfx::Size size = status_bar->CalculatePreferredSize(views::SizeBounds());
  EXPECT_EQ(24, size.height());
}

// Test visibility.
TEST_F(AstraStatusBarViewTest, Visibility) {
  auto status_bar = std::make_unique<AstraStatusBarView>();

  status_bar->SetVisible(true);
  EXPECT_TRUE(status_bar->GetVisible());

  status_bar->SetVisible(false);
  EXPECT_FALSE(status_bar->GetVisible());
}

// Test delegate (null delegate should not crash).
TEST_F(AstraStatusBarViewTest, NullDelegateNoCrash) {
  auto status_bar = std::make_unique<AstraStatusBarView>();

  // All operations with null delegate should not crash.
  status_bar->SetZoomLevel(1.2);
  status_bar->SetSecurityLevel(AstraSecurityLevel::kSecure);
  status_bar->SetDownloadCount(5);
  SUCCEED();
}

// Test status text with long URL.
TEST_F(AstraStatusBarViewTest, LongStatusText) {
  auto status_bar = std::make_unique<AstraStatusBarView>();

  std::u16string long_url = u"https://very.long.domain.name.example.com/"
                            u"very/long/path/with/many/components/"
                            u"and-a-very-long-filename.html";
  status_bar->SetStatusText(long_url);
  // Should not crash even with very long text.
  SUCCEED();
}

// Test all security level transitions.
TEST_F(AstraStatusBarViewTest, AllSecurityTransitions) {
  auto status_bar = std::make_unique<AstraStatusBarView>();

  // Test every possible transition.
  std::vector<AstraSecurityLevel> levels = {
      AstraSecurityLevel::kNone,
      AstraSecurityLevel::kSecure,
      AstraSecurityLevel::kSecureWithWarning,
      AstraSecurityLevel::kDangerous,
      AstraSecurityLevel::kInternalPage,
  };

  for (auto from : levels) {
    for (auto to : levels) {
      status_bar->SetSecurityLevel(from);
      status_bar->SetSecurityLevel(to);
      // Should not crash.
    }
  }
  SUCCEED();
}

// TODO(astra): Add tests with ViewsTestBase for layout validation
// TODO(astra): Add tests for delegate callbacks with mock delegate

}  // namespace astra
