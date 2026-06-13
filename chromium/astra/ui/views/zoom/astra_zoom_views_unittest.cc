// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/zoom/astra_zoom_button.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

class AstraZoomButtonTest : public testing::Test {
 protected:
  void SetUp() override {}

  base::test::TaskEnvironment task_environment_;
};

// Test basic zoom button construction.
TEST_F(AstraZoomButtonTest, Create) {
  auto zoom_button = std::make_unique<AstraZoomButton>();

  EXPECT_DOUBLE_EQ(1.0, zoom_button->zoom_level());
  EXPECT_DOUBLE_EQ(1.0, zoom_button->default_zoom());
}

// Test zoom level changes.
TEST_F(AstraZoomButtonTest, ZoomLevel) {
  auto zoom_button = std::make_unique<AstraZoomButton>();

  zoom_button->SetZoomLevel(1.25);
  EXPECT_DOUBLE_EQ(1.25, zoom_button->zoom_level());

  zoom_button->SetZoomLevel(0.75);
  EXPECT_DOUBLE_EQ(0.75, zoom_button->zoom_level());

  zoom_button->SetZoomLevel(2.0);
  EXPECT_DOUBLE_EQ(2.0, zoom_button->zoom_level());

  zoom_button->SetZoomLevel(0.25);
  EXPECT_DOUBLE_EQ(0.25, zoom_button->zoom_level());

  // Reset to default.
  zoom_button->SetZoomLevel(1.0);
  EXPECT_DOUBLE_EQ(1.0, zoom_button->zoom_level());
}

// Test default zoom changes.
TEST_F(AstraZoomButtonTest, DefaultZoom) {
  auto zoom_button = std::make_unique<AstraZoomButton>();

  zoom_button->SetDefaultZoom(1.5);
  EXPECT_DOUBLE_EQ(1.5, zoom_button->default_zoom());

  zoom_button->SetDefaultZoom(0.8);
  EXPECT_DOUBLE_EQ(0.8, zoom_button->default_zoom());

  zoom_button->SetDefaultZoom(1.0);
  EXPECT_DOUBLE_EQ(1.0, zoom_button->default_zoom());
}

// Test bubble visibility.
TEST_F(AstraZoomButtonTest, BubbleVisibility) {
  auto zoom_button = std::make_unique<AstraZoomButton>();

  // Initially no bubble.
  EXPECT_FALSE(zoom_button->IsBubbleShowing());
  // Should not crash.
  SUCCEED();
}

// Test null delegate doesn't crash.
TEST_F(AstraZoomButtonTest, NullDelegateNoCrash) {
  auto zoom_button = std::make_unique<AstraZoomButton>();

  // All operations with null delegate should not crash.
  zoom_button->SetZoomLevel(1.5);
  zoom_button->SetDefaultZoom(1.2);
  SUCCEED();
}

// Test common zoom levels.
TEST_F(AstraZoomButtonTest, CommonZoomLevels) {
  auto zoom_button = std::make_unique<AstraZoomButton>();

  // Standard Chrome zoom levels.
  std::vector<double> levels = {
      0.25, 0.33, 0.50, 0.67, 0.75, 0.80, 0.90,
      1.00, 1.10, 1.25, 1.50, 1.75, 2.00, 2.50,
      3.00, 4.00, 5.00
  };

  for (double level : levels) {
    zoom_button->SetZoomLevel(level);
    EXPECT_DOUBLE_EQ(level, zoom_button->zoom_level());
  }
}

// Test zoom bubble view construction.
TEST_F(AstraZoomButtonTest, BubbleView) {
  // Bubble requires an anchor view, which is tricky in unit tests.
  // We test the zoom button itself instead.
  auto zoom_button = std::make_unique<AstraZoomButton>();
  EXPECT_NE(nullptr, zoom_button.get());
}

// Test zoom level precision.
TEST_F(AstraZoomButtonTest, ZoomLevelPrecision) {
  auto zoom_button = std::make_unique<AstraZoomButton>();

  zoom_button->SetZoomLevel(1.333);
  EXPECT_NEAR(1.333, zoom_button->zoom_level(), 0.001);

  zoom_button->SetZoomLevel(0.667);
  EXPECT_NEAR(0.667, zoom_button->zoom_level(), 0.001);
}

// Test setting zoom to same value.
TEST_F(AstraZoomButtonTest, SameZoomValue) {
  auto zoom_button = std::make_unique<AstraZoomButton>();

  zoom_button->SetZoomLevel(1.5);
  double before = zoom_button->zoom_level();
  zoom_button->SetZoomLevel(1.5);
  double after = zoom_button->zoom_level();
  EXPECT_DOUBLE_EQ(before, after);
}

// Test default zoom independence from current zoom.
TEST_F(AstraZoomButtonTest, ZoomAndDefaultIndependent) {
  auto zoom_button = std::make_unique<AstraZoomButton>();

  zoom_button->SetZoomLevel(1.5);
  zoom_button->SetDefaultZoom(1.25);

  EXPECT_DOUBLE_EQ(1.5, zoom_button->zoom_level());
  EXPECT_DOUBLE_EQ(1.25, zoom_button->default_zoom());

  // Changing default should not affect current zoom.
  zoom_button->SetDefaultZoom(1.0);
  EXPECT_DOUBLE_EQ(1.5, zoom_button->zoom_level());
  EXPECT_DOUBLE_EQ(1.0, zoom_button->default_zoom());

  // Changing zoom should not affect default.
  zoom_button->SetZoomLevel(2.0);
  EXPECT_DOUBLE_EQ(2.0, zoom_button->zoom_level());
  EXPECT_DOUBLE_EQ(1.0, zoom_button->default_zoom());
}

// TODO(astra): Add tests with ViewsTestBase for bubble showing/hiding
// TODO(astra): Add tests for bubble button interactions
// TODO(astra): Add tests for delegate callbacks with mock delegate

}  // namespace astra
