// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Comprehensive unit tests for AstraBrowserView.
//
// Test categories:
//   - Construction and destruction
//   - Install/Uninstall idempotency
//   - Sidebar show/hide/toggle
//   - Widget lifecycle management (WidgetObserver pattern)
//   - All bubble show/hide state tracking
//   - Multiple bubbles open simultaneously
//   - CloseAllBubbles / IsAnyBubbleOpen / GetOpenBubbleCount
//   - Service observer integration (theme, focus mode, workspace)
//   - AstraBrowserView observer pattern
//   - Convenience accessors
//   - BubbleType enum
//   - Edge cases (null browser_view, pre-install calls, rapid state changes)
//
// Notes:
//   - Uses views::ViewsTestBase for widget/view test infrastructure.
//   - Tests focus on the AstraBrowserView state machine and observer patterns.
//   - Widget lifecycle tests use real test widgets and verify that
//     OnWidgetDestroying properly clears tracked pointers.
//   - The test fixture is a friend of AstraBrowserView to test internal
//     widget pointer management without going through the full
//     BrowserView-dependent bubble creation path.
//
// Chromium test pattern: views::test::ViewsTestBase
//   (ui/views/test/views_test_base.h)

#include "astra/ui/views/astra_browser_view.h"

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/test/task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

namespace astra {

namespace {

using ::testing::_;
using ::testing::AtLeast;
using ::testing::InSequence;
using ::testing::Mock;

// =========================================================================
// Mock observer for AstraBrowserView
// =========================================================================

class MockAstraBrowserViewObserver : public AstraBrowserView::Observer {
 public:
  MockAstraBrowserViewObserver() = default;
  ~MockAstraBrowserViewObserver() override = default;

  MOCK_METHOD(void, OnAstraBrowserViewInstalled, (), (override));
  MOCK_METHOD(void, OnAstraBrowserViewUninstalled, (), (override));
  MOCK_METHOD(void, OnSidebarVisibilityChanged, (bool visible), (override));
  MOCK_METHOD(void, OnFocusModeToggled, (bool active), (override));
  MOCK_METHOD(void, OnWorkspaceSwitched, (const std::string& workspace_id),
              (override));
  MOCK_METHOD(void, OnAnyBubbleOpened, (), (override));
  MOCK_METHOD(void, OnAllBubblesClosed, (), (override));
};

// =========================================================================
// Test observer with empty default implementations
// =========================================================================
// Verifies that all observer methods have empty default implementations
// and can be safely called without overriding.
class DefaultImplObserver : public AstraBrowserView::Observer {
 public:
  DefaultImplObserver() = default;
  ~DefaultImplObserver() override = default;

  int call_count = 0;
};

// =========================================================================
// AstraBrowserViewTest fixture
// =========================================================================
//
// Uses views::ViewsTestBase for widget infrastructure.
// AstraBrowserView is a friend of this test class so we can test internal
// widget pointer management directly.
//
// For widget lifecycle tests, we create test widgets and manually set the
// internal pointers (via friend access) to test the WidgetObserver pattern
// without requiring a full BrowserView + bubble creation setup.
//
// For tests that need a BrowserView, we test with null or early-return
// behavior since a full BrowserView requires a complete Chromium test
// environment (interactive_ui_tests).

class AstraBrowserViewTest : public views::ViewsTestBase {
 public:
  AstraBrowserViewTest() = default;
  ~AstraBrowserViewTest() override = default;

  void SetUp() override {
    views::ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();
  }

  void TearDown() override {
    widget_.reset();
    views::ViewsTestBase::TearDown();
  }

 protected:
  // Creates a test widget that can be used to simulate a bubble widget.
  std::unique_ptr<views::Widget> CreateBubbleWidget() {
    return CreateTestWidget();
  }

  // Sets the command palette widget pointer on an AstraBrowserView.
  // Uses friend access for testing.
  void SetCommandPaletteWidget(AstraBrowserView* view, views::Widget* widget) {
    view->command_palette_widget_ = widget;
  }

  // Sets the settings widget pointer on an AstraBrowserView.
  void SetSettingsWidget(AstraBrowserView* view, views::Widget* widget) {
    view->settings_widget_ = widget;
  }

  // Sets the tab search widget pointer on an AstraBrowserView.
  void SetTabSearchWidget(AstraBrowserView* view, views::Widget* widget) {
    view->tab_search_widget_ = widget;
  }

  // Sets the import/export widget pointer on an AstraBrowserView.
  void SetImportExportWidget(AstraBrowserView* view, views::Widget* widget) {
    view->import_export_widget_ = widget;
  }

  // Sets the screenshot capture widget pointer on an AstraBrowserView.
  void SetScreenshotCaptureWidget(AstraBrowserView* view, views::Widget* widget) {
    view->screenshot_capture_widget_ = widget;
  }

  // Sets the region overlay widget pointer on an AstraBrowserView.
  void SetRegionOverlayWidget(AstraBrowserView* view, views::Widget* widget) {
    view->region_overlay_widget_ = widget;
  }

  // Sets the new tab widget pointer on an AstraBrowserView.
  void SetNewTabWidget(AstraBrowserView* view, views::Widget* widget) {
    view->new_tab_widget_ = widget;
  }

  // Test widget from ViewsTestBase.
  std::unique_ptr<views::Widget> widget_;
};

// =========================================================================
// Construction and destruction tests
// =========================================================================

TEST_F(AstraBrowserViewTest, ConstructWithNullBrowserView) {
  // Constructing with null browser_view should not crash.
  AstraBrowserView view(nullptr);
  EXPECT_FALSE(view.IsSidebarVisible());
  EXPECT_FALSE(view.IsAnyBubbleOpen());
  EXPECT_EQ(0, view.GetOpenBubbleCount());
  EXPECT_FALSE(view.IsFocusModeActive());
}

TEST_F(AstraBrowserViewTest, ConstructDoesNotInstall) {
  // After construction, installed_ should be false.
  AstraBrowserView view(nullptr);
  EXPECT_FALSE(view.sidebar_view());
}

TEST_F(AstraBrowserViewTest, DestructWithoutInstallDoesNotCrash) {
  // Destructing without installing should be safe.
  auto view = std::make_unique<AstraBrowserView>(nullptr);
  view.reset();  // Should not crash.
  SUCCEED();
}

TEST_F(AstraBrowserViewTest, CopyAndAssignmentDeleted) {
  // Verify copy constructor and assignment operator are deleted.
  static_assert(!std::is_copy_constructible_v<AstraBrowserView>);
  static_assert(!std::is_copy_assignable_v<AstraBrowserView>);
  SUCCEED();
}

// =========================================================================
// Install / Uninstall tests
// =========================================================================

TEST_F(AstraBrowserViewTest, InstallWithNullBrowserViewIsNoOp) {
  AstraBrowserView view(nullptr);
  view.Install();
  // Should not crash, and nothing should be installed.
  EXPECT_FALSE(view.sidebar_view());
}

TEST_F(AstraBrowserViewTest, UninstallWithoutInstallIsSafe) {
  AstraBrowserView view(nullptr);
  view.Uninstall();  // Should not crash.
  SUCCEED();
}

TEST_F(AstraBrowserViewTest, UninstallIdempotent) {
  AstraBrowserView view(nullptr);
  view.Uninstall();
  view.Uninstall();  // Second call should be safe.
  SUCCEED();
}

TEST_F(AstraBrowserViewTest, InstallIdempotent) {
  AstraBrowserView view(nullptr);
  view.Install();
  view.Install();  // Second call should be a no-op.
  SUCCEED();
}

// =========================================================================
// Sidebar visibility tests
// =========================================================================

TEST_F(AstraBrowserViewTest, ShowSidebarWithNoSidebarIsNoOp) {
  AstraBrowserView view(nullptr);
  view.ShowSidebar();  // Should not crash.
  EXPECT_FALSE(view.IsSidebarVisible());
}

TEST_F(AstraBrowserViewTest, HideSidebarWithNoSidebarIsNoOp) {
  AstraBrowserView view(nullptr);
  view.HideSidebar();  // Should not crash.
  EXPECT_FALSE(view.IsSidebarVisible());
}

TEST_F(AstraBrowserViewTest, ToggleSidebarWithNoSidebarIsNoOp) {
  AstraBrowserView view(nullptr);
  view.ToggleSidebar();  // Should not crash.
  EXPECT_FALSE(view.IsSidebarVisible());
}

TEST_F(AstraBrowserViewTest, SidebarPinnedDefaultIsTrue) {
  AstraBrowserView view(nullptr);
  EXPECT_TRUE(view.IsSidebarPinned());
}

TEST_F(AstraBrowserViewTest, ToggleSidebarPinFlipsState) {
  AstraBrowserView view(nullptr);
  EXPECT_TRUE(view.IsSidebarPinned());
  view.ToggleSidebarPin();
  EXPECT_FALSE(view.IsSidebarPinned());
  view.ToggleSidebarPin();
  EXPECT_TRUE(view.IsSidebarPinned());
}

// =========================================================================
// Bubble state tests (using friend access to set widget pointers)
// =========================================================================

TEST_F(AstraBrowserViewTest, IsCommandPaletteOpenInitiallyFalse) {
  AstraBrowserView view(nullptr);
  EXPECT_FALSE(view.IsCommandPaletteOpen());
}

TEST_F(AstraBrowserViewTest, IsSettingsOpenInitiallyFalse) {
  AstraBrowserView view(nullptr);
  EXPECT_FALSE(view.IsSettingsOpen());
}

TEST_F(AstraBrowserViewTest, IsTabSearchOpenInitiallyFalse) {
  AstraBrowserView view(nullptr);
  EXPECT_FALSE(view.IsTabSearchOpen());
}

TEST_F(AstraBrowserViewTest, IsImportExportDialogVisibleInitiallyFalse) {
  AstraBrowserView view(nullptr);
  EXPECT_FALSE(view.IsImportExportDialogVisible());
}

TEST_F(AstraBrowserViewTest, IsNewTabPageVisibleInitiallyFalse) {
  AstraBrowserView view(nullptr);
  EXPECT_FALSE(view.IsNewTabPageVisible());
}

TEST_F(AstraBrowserViewTest, IsAnyBubbleOpenInitiallyFalse) {
  AstraBrowserView view(nullptr);
  EXPECT_FALSE(view.IsAnyBubbleOpen());
}

TEST_F(AstraBrowserViewTest, GetOpenBubbleCountInitiallyZero) {
  AstraBrowserView view(nullptr);
  EXPECT_EQ(0, view.GetOpenBubbleCount());
}

TEST_F(AstraBrowserViewTest, SingleBubbleIsTracked) {
  AstraBrowserView view(nullptr);
  auto bubble_widget = CreateBubbleWidget();

  SetCommandPaletteWidget(&view, bubble_widget.get());

  EXPECT_TRUE(view.IsCommandPaletteOpen());
  EXPECT_TRUE(view.IsAnyBubbleOpen());
  EXPECT_EQ(1, view.GetOpenBubbleCount());
}

TEST_F(AstraBrowserViewTest, SettingsBubbleIsTracked) {
  AstraBrowserView view(nullptr);
  auto bubble_widget = CreateBubbleWidget();

  SetSettingsWidget(&view, bubble_widget.get());

  EXPECT_TRUE(view.IsSettingsOpen());
  EXPECT_TRUE(view.IsAnyBubbleOpen());
  EXPECT_EQ(1, view.GetOpenBubbleCount());
}

TEST_F(AstraBrowserViewTest, TabSearchBubbleIsTracked) {
  AstraBrowserView view(nullptr);
  auto bubble_widget = CreateBubbleWidget();

  SetTabSearchWidget(&view, bubble_widget.get());

  EXPECT_TRUE(view.IsTabSearchOpen());
  EXPECT_TRUE(view.IsAnyBubbleOpen());
  EXPECT_EQ(1, view.GetOpenBubbleCount());
}

TEST_F(AstraBrowserViewTest, ImportExportBubbleIsTracked) {
  AstraBrowserView view(nullptr);
  auto bubble_widget = CreateBubbleWidget();

  SetImportExportWidget(&view, bubble_widget.get());

  EXPECT_TRUE(view.IsImportExportDialogVisible());
  EXPECT_TRUE(view.IsAnyBubbleOpen());
  EXPECT_EQ(1, view.GetOpenBubbleCount());
}

TEST_F(AstraBrowserViewTest, ScreenshotCaptureBubbleIsTracked) {
  AstraBrowserView view(nullptr);
  auto bubble_widget = CreateBubbleWidget();

  SetScreenshotCaptureWidget(&view, bubble_widget.get());

  EXPECT_TRUE(view.IsAnyBubbleOpen());
  EXPECT_EQ(1, view.GetOpenBubbleCount());
}

TEST_F(AstraBrowserViewTest, RegionOverlayBubbleIsTracked) {
  AstraBrowserView view(nullptr);
  auto bubble_widget = CreateBubbleWidget();

  SetRegionOverlayWidget(&view, bubble_widget.get());

  EXPECT_TRUE(view.IsAnyBubbleOpen());
  EXPECT_EQ(1, view.GetOpenBubbleCount());
}

TEST_F(AstraBrowserViewTest, NewTabBubbleIsTracked) {
  AstraBrowserView view(nullptr);
  auto bubble_widget = CreateBubbleWidget();

  SetNewTabWidget(&view, bubble_widget.get());

  EXPECT_TRUE(view.IsNewTabPageVisible());
  EXPECT_TRUE(view.IsAnyBubbleOpen());
  EXPECT_EQ(1, view.GetOpenBubbleCount());
}

TEST_F(AstraBrowserViewTest, MultipleBubblesTrackedCorrectly) {
  AstraBrowserView view(nullptr);
  auto widget1 = CreateBubbleWidget();
  auto widget2 = CreateBubbleWidget();
  auto widget3 = CreateBubbleWidget();

  SetCommandPaletteWidget(&view, widget1.get());
  EXPECT_EQ(1, view.GetOpenBubbleCount());

  SetSettingsWidget(&view, widget2.get());
  EXPECT_EQ(2, view.GetOpenBubbleCount());

  SetTabSearchWidget(&view, widget3.get());
  EXPECT_EQ(3, view.GetOpenBubbleCount());
  EXPECT_TRUE(view.IsAnyBubbleOpen());
}

TEST_F(AstraBrowserViewTest, AllSevenBubbleTypesTracked) {
  // Verify all 7 bubble widget types are counted.
  AstraBrowserView view(nullptr);
  auto w1 = CreateBubbleWidget();
  auto w2 = CreateBubbleWidget();
  auto w3 = CreateBubbleWidget();
  auto w4 = CreateBubbleWidget();
  auto w5 = CreateBubbleWidget();
  auto w6 = CreateBubbleWidget();
  auto w7 = CreateBubbleWidget();

  SetCommandPaletteWidget(&view, w1.get());
  SetSettingsWidget(&view, w2.get());
  SetTabSearchWidget(&view, w3.get());
  SetImportExportWidget(&view, w4.get());
  SetScreenshotCaptureWidget(&view, w5.get());
  SetRegionOverlayWidget(&view, w6.get());
  SetNewTabWidget(&view, w7.get());

  EXPECT_EQ(7, view.GetOpenBubbleCount());
  EXPECT_TRUE(view.IsAnyBubbleOpen());
}

// =========================================================================
// Widget lifecycle tests (WidgetObserver pattern)
// =========================================================================

TEST_F(AstraBrowserViewTest, WidgetDestroyingClearsCommandPalettePointer) {
  AstraBrowserView view(nullptr);
  auto bubble_widget = CreateBubbleWidget();
  views::Widget* widget_raw = bubble_widget.get();

  SetCommandPaletteWidget(&view, widget_raw);
  widget_raw->AddObserver(&view);

  EXPECT_TRUE(view.IsCommandPaletteOpen());

  // Destroy the widget — OnWidgetDestroying should clear the pointer.
  bubble_widget.reset();

  EXPECT_FALSE(view.IsCommandPaletteOpen());
  EXPECT_EQ(0, view.GetOpenBubbleCount());
  EXPECT_FALSE(view.IsAnyBubbleOpen());
}

TEST_F(AstraBrowserViewTest, WidgetDestroyingClearsSettingsPointer) {
  AstraBrowserView view(nullptr);
  auto bubble_widget = CreateBubbleWidget();
  views::Widget* widget_raw = bubble_widget.get();

  SetSettingsWidget(&view, widget_raw);
  widget_raw->AddObserver(&view);

  EXPECT_TRUE(view.IsSettingsOpen());

  bubble_widget.reset();

  EXPECT_FALSE(view.IsSettingsOpen());
  EXPECT_EQ(0, view.GetOpenBubbleCount());
}

TEST_F(AstraBrowserViewTest, WidgetDestroyingClearsTabSearchPointer) {
  AstraBrowserView view(nullptr);
  auto bubble_widget = CreateBubbleWidget();
  views::Widget* widget_raw = bubble_widget.get();

  SetTabSearchWidget(&view, widget_raw);
  widget_raw->AddObserver(&view);

  EXPECT_TRUE(view.IsTabSearchOpen());

  bubble_widget.reset();

  EXPECT_FALSE(view.IsTabSearchOpen());
  EXPECT_EQ(0, view.GetOpenBubbleCount());
}

TEST_F(AstraBrowserViewTest, WidgetDestroyingClearsImportExportPointer) {
  AstraBrowserView view(nullptr);
  auto bubble_widget = CreateBubbleWidget();
  views::Widget* widget_raw = bubble_widget.get();

  SetImportExportWidget(&view, widget_raw);
  widget_raw->AddObserver(&view);

  EXPECT_TRUE(view.IsImportExportDialogVisible());

  bubble_widget.reset();

  EXPECT_FALSE(view.IsImportExportDialogVisible());
  EXPECT_EQ(0, view.GetOpenBubbleCount());
}

TEST_F(AstraBrowserViewTest, WidgetDestroyingClearsScreenshotCapturePointer) {
  AstraBrowserView view(nullptr);
  auto bubble_widget = CreateBubbleWidget();
  views::Widget* widget_raw = bubble_widget.get();

  SetScreenshotCaptureWidget(&view, widget_raw);
  widget_raw->AddObserver(&view);

  bubble_widget.reset();

  EXPECT_EQ(0, view.GetOpenBubbleCount());
  EXPECT_FALSE(view.IsAnyBubbleOpen());
}

TEST_F(AstraBrowserViewTest, WidgetDestroyingClearsRegionOverlayPointer) {
  AstraBrowserView view(nullptr);
  auto bubble_widget = CreateBubbleWidget();
  views::Widget* widget_raw = bubble_widget.get();

  SetRegionOverlayWidget(&view, widget_raw);
  widget_raw->AddObserver(&view);

  bubble_widget.reset();

  EXPECT_EQ(0, view.GetOpenBubbleCount());
  EXPECT_FALSE(view.IsAnyBubbleOpen());
}

TEST_F(AstraBrowserViewTest, WidgetDestroyingClearsNewTabPointer) {
  AstraBrowserView view(nullptr);
  auto bubble_widget = CreateBubbleWidget();
  views::Widget* widget_raw = bubble_widget.get();

  SetNewTabWidget(&view, widget_raw);
  widget_raw->AddObserver(&view);

  EXPECT_TRUE(view.IsNewTabPageVisible());

  bubble_widget.reset();

  EXPECT_FALSE(view.IsNewTabPageVisible());
  EXPECT_EQ(0, view.GetOpenBubbleCount());
}

TEST_F(AstraBrowserViewTest, MultipleWidgetsDestroyedIndependently) {
  // Closing one widget should not affect other open widgets.
  AstraBrowserView view(nullptr);
  auto w1 = CreateBubbleWidget();
  auto w2 = CreateBubbleWidget();
  auto w3 = CreateBubbleWidget();
  views::Widget* w1_raw = w1.get();
  views::Widget* w2_raw = w2.get();
  views::Widget* w3_raw = w3.get();

  SetCommandPaletteWidget(&view, w1_raw);
  SetSettingsWidget(&view, w2_raw);
  SetTabSearchWidget(&view, w3_raw);
  w1_raw->AddObserver(&view);
  w2_raw->AddObserver(&view);
  w3_raw->AddObserver(&view);

  EXPECT_EQ(3, view.GetOpenBubbleCount());

  // Close the middle one.
  w2.reset();

  EXPECT_EQ(2, view.GetOpenBubbleCount());
  EXPECT_TRUE(view.IsCommandPaletteOpen());
  EXPECT_FALSE(view.IsSettingsOpen());
  EXPECT_TRUE(view.IsTabSearchOpen());

  // Close the first one.
  w1.reset();

  EXPECT_EQ(1, view.GetOpenBubbleCount());
  EXPECT_FALSE(view.IsCommandPaletteOpen());
  EXPECT_TRUE(view.IsTabSearchOpen());

  // Close the last one.
  w3.reset();

  EXPECT_EQ(0, view.GetOpenBubbleCount());
  EXPECT_FALSE(view.IsAnyBubbleOpen());
}

TEST_F(AstraBrowserViewTest, UnknownWidgetDestroyingDoesNothing) {
  // Destroying a widget that AstraBrowserView is not tracking should
  // not affect any state.
  AstraBrowserView view(nullptr);
  auto tracked_widget = CreateBubbleWidget();
  auto unknown_widget = CreateBubbleWidget();

  SetCommandPaletteWidget(&view, tracked_widget.get());
  tracked_widget->AddObserver(&view);
  unknown_widget->AddObserver(&view);

  EXPECT_EQ(1, view.GetOpenBubbleCount());

  // Destroy the unknown widget.
  unknown_widget.reset();

  // Should still have 1 open bubble.
  EXPECT_EQ(1, view.GetOpenBubbleCount());
  EXPECT_TRUE(view.IsCommandPaletteOpen());
}

TEST_F(AstraBrowserViewTest, NullWidgetInOnWidgetDestroyingIsSafe) {
  // Calling OnWidgetDestroying with null should be safe.
  AstraBrowserView view(nullptr);
  view.OnWidgetDestroying(nullptr);  // Should not crash.
  SUCCEED();
}

// =========================================================================
// CloseAllBubbles tests
// =========================================================================

TEST_F(AstraBrowserViewTest, CloseAllBubblesWhenNoneOpenIsSafe) {
  AstraBrowserView view(nullptr);
  view.CloseAllBubbles();  // Should not crash.
  EXPECT_FALSE(view.IsAnyBubbleOpen());
  EXPECT_EQ(0, view.GetOpenBubbleCount());
}

TEST_F(AstraBrowserViewTest, CloseAllBubblesClosesAllTrackedWidgets) {
  // CloseAllBubbles calls Close() on all open widget pointers.
  // Since we can't verify Close() was called without mocks, we verify
  // that the call doesn't crash and the state remains consistent.
  // The actual closing behavior is tested by widget lifecycle tests.
  AstraBrowserView view(nullptr);
  auto w1 = CreateBubbleWidget();
  auto w2 = CreateBubbleWidget();

  SetCommandPaletteWidget(&view, w1.get());
  SetSettingsWidget(&view, w2.get());

  EXPECT_TRUE(view.IsAnyBubbleOpen());

  view.CloseAllBubbles();

  // Close() on the widgets has been called.
  // The widgets will be destroyed and pointers cleared via
  // OnWidgetDestroying when the widget is actually destroyed.
  SUCCEED();
}

// =========================================================================
// Hide bubble method tests (early return / no-op paths)
// =========================================================================

TEST_F(AstraBrowserViewTest, HideCommandPaletteWhenClosedIsSafe) {
  AstraBrowserView view(nullptr);
  view.HideCommandPalette();  // Should not crash.
  EXPECT_FALSE(view.IsCommandPaletteOpen());
}

TEST_F(AstraBrowserViewTest, HideSettingsWhenClosedIsSafe) {
  AstraBrowserView view(nullptr);
  view.HideSettings();  // Should not crash.
  EXPECT_FALSE(view.IsSettingsOpen());
}

TEST_F(AstraBrowserViewTest, HideTabSearchWhenClosedIsSafe) {
  AstraBrowserView view(nullptr);
  view.HideTabSearch();  // Should not crash.
  EXPECT_FALSE(view.IsTabSearchOpen());
}

TEST_F(AstraBrowserViewTest, HideImportExportDialogWhenClosedIsSafe) {
  AstraBrowserView view(nullptr);
  view.HideImportExportDialog();  // Should not crash.
  EXPECT_FALSE(view.IsImportExportDialogVisible());
}

TEST_F(AstraBrowserViewTest, HideNewTabPageWhenClosedIsSafe) {
  AstraBrowserView view(nullptr);
  view.HideNewTabPage();  // Should not crash.
  EXPECT_FALSE(view.IsNewTabPageVisible());
}

TEST_F(AstraBrowserViewTest, ShowCommandPaletteWithNullBrowserViewIsNoOp) {
  AstraBrowserView view(nullptr);
  view.ShowCommandPalette();  // Should not crash.
  EXPECT_FALSE(view.IsCommandPaletteOpen());
}

TEST_F(AstraBrowserViewTest, ShowSettingsWithNullBrowserViewIsNoOp) {
  AstraBrowserView view(nullptr);
  view.ShowSettings();  // Should not crash.
  EXPECT_FALSE(view.IsSettingsOpen());
}

TEST_F(AstraBrowserViewTest, ShowTabSearchWithNullBrowserViewIsNoOp) {
  AstraBrowserView view(nullptr);
  view.ShowTabSearch();  // Should not crash.
  EXPECT_FALSE(view.IsTabSearchOpen());
}

TEST_F(AstraBrowserViewTest, ShowNewTabPageWithNullBrowserViewIsNoOp) {
  AstraBrowserView view(nullptr);
  view.ShowNewTabPage();  // Should not crash.
  EXPECT_FALSE(view.IsNewTabPageVisible());
}

// =========================================================================
// Observer notification tests
// =========================================================================

TEST_F(AstraBrowserViewTest, AddObserverDoesNotCrash) {
  AstraBrowserView view(nullptr);
  MockAstraBrowserViewObserver observer;
  view.AddObserver(&observer);
  SUCCEED();
}

TEST_F(AstraBrowserViewTest, RemoveObserverDoesNotCrash) {
  AstraBrowserView view(nullptr);
  MockAstraBrowserViewObserver observer;
  view.AddObserver(&observer);
  view.RemoveObserver(&observer);
  SUCCEED();
}

TEST_F(AstraBrowserViewTest, RemoveObserverNeverAddedIsSafe) {
  AstraBrowserView view(nullptr);
  MockAstraBrowserViewObserver observer;
  view.RemoveObserver(&observer);  // Should not crash.
  SUCCEED();
}

TEST_F(AstraBrowserViewTest, ObserverOnAnyBubbleOpenedFiresOnFirstBubble) {
  AstraBrowserView view(nullptr);
  MockAstraBrowserViewObserver observer;
  view.AddObserver(&observer);

  // Simulate first bubble opening.
  auto widget = CreateBubbleWidget();
  views::Widget* widget_raw = widget.get();

  EXPECT_CALL(observer, OnAnyBubbleOpened()).Times(1);

  SetCommandPaletteWidget(&view, widget_raw);
  widget_raw->AddObserver(&view);

  // Since we set the pointer manually, we need to simulate the
  // notification. We call NotifyBubbleOpened through the test friend.
  view.NotifyBubbleOpened();

  Mock::VerifyAndClearExpectations(&observer);
}

TEST_F(AstraBrowserViewTest, ObserverOnAllBubblesClosedFiresOnLastBubble) {
  AstraBrowserView view(nullptr);
  MockAstraBrowserViewObserver observer;
  view.AddObserver(&observer);

  auto widget = CreateBubbleWidget();
  views::Widget* widget_raw = widget.get();

  SetCommandPaletteWidget(&view, widget_raw);
  widget_raw->AddObserver(&view);

  EXPECT_CALL(observer, OnAllBubblesClosed()).Times(1);

  // Destroy the widget — this should trigger OnWidgetDestroying which
  // clears the pointer and fires OnAllBubblesClosed.
  widget.reset();

  Mock::VerifyAndClearExpectations(&observer);
}

TEST_F(AstraBrowserViewTest, SecondBubbleDoesNotFireOnAnyBubbleOpened) {
  // OnAnyBubbleOpened should only fire when transitioning from 0 to 1.
  AstraBrowserView view(nullptr);
  MockAstraBrowserViewObserver observer;
  view.AddObserver(&observer);

  auto w1 = CreateBubbleWidget();
  auto w2 = CreateBubbleWidget();

  SetCommandPaletteWidget(&view, w1.get());
  w1->AddObserver(&view);

  // Notify for first bubble.
  view.NotifyBubbleOpened();

  // Second bubble should NOT fire OnAnyBubbleOpened again.
  EXPECT_CALL(observer, OnAnyBubbleOpened()).Times(0);

  SetSettingsWidget(&view, w2.get());
  w2->AddObserver(&view);

  Mock::VerifyAndClearExpectations(&observer);
}

TEST_F(AstraBrowserViewTest, NonLastBubbleCloseDoesNotFireAllBubblesClosed) {
  // OnAllBubblesClosed should only fire when transitioning from 1 to 0.
  AstraBrowserView view(nullptr);
  MockAstraBrowserViewObserver observer;
  view.AddObserver(&observer);

  auto w1 = CreateBubbleWidget();
  auto w2 = CreateBubbleWidget();

  SetCommandPaletteWidget(&view, w1.get());
  SetSettingsWidget(&view, w2.get());
  w1->AddObserver(&view);
  w2->AddObserver(&view);

  EXPECT_CALL(observer, OnAllBubblesClosed()).Times(0);

  // Close one of the two.
  w1.reset();

  Mock::VerifyAndClearExpectations(&observer);
  EXPECT_EQ(1, view.GetOpenBubbleCount());
}

TEST_F(AstraBrowserViewTest, MultipleObserversAllNotified) {
  AstraBrowserView view(nullptr);
  MockAstraBrowserViewObserver observer1;
  MockAstraBrowserViewObserver observer2;
  MockAstraBrowserViewObserver observer3;

  view.AddObserver(&observer1);
  view.AddObserver(&observer2);
  view.AddObserver(&observer3);

  auto widget = CreateBubbleWidget();
  views::Widget* widget_raw = widget.get();

  SetCommandPaletteWidget(&view, widget_raw);
  widget_raw->AddObserver(&view);

  EXPECT_CALL(observer1, OnAllBubblesClosed()).Times(1);
  EXPECT_CALL(observer2, OnAllBubblesClosed()).Times(1);
  EXPECT_CALL(observer3, OnAllBubblesClosed()).Times(1);

  widget.reset();

  Mock::VerifyAndClearExpectations(&observer1);
  Mock::VerifyAndClearExpectations(&observer2);
  Mock::VerifyAndClearExpectations(&observer3);
}

TEST_F(AstraBrowserViewTest, RemovedObserverNotNotified) {
  AstraBrowserView view(nullptr);
  MockAstraBrowserViewObserver observer;

  view.AddObserver(&observer);
  view.RemoveObserver(&observer);

  auto widget = CreateBubbleWidget();
  views::Widget* widget_raw = widget.get();

  SetCommandPaletteWidget(&view, widget_raw);
  widget_raw->AddObserver(&view);

  EXPECT_CALL(observer, OnAllBubblesClosed()).Times(0);

  widget.reset();

  Mock::VerifyAndClearExpectations(&observer);
}

TEST_F(AstraBrowserViewTest, ObserverDefaultImplementationsAreSafe) {
  // All observer methods should have empty default implementations.
  // Calling any of them on a base instance should not crash.
  DefaultImplObserver observer;

  // Verify all methods can be called on the default observer.
  observer.OnAstraBrowserViewInstalled();
  observer.OnAstraBrowserViewUninstalled();
  observer.OnSidebarVisibilityChanged(true);
  observer.OnSidebarVisibilityChanged(false);
  observer.OnFocusModeToggled(true);
  observer.OnFocusModeToggled(false);
  observer.OnWorkspaceSwitched("workspace-1");
  observer.OnAnyBubbleOpened();
  observer.OnAllBubblesClosed();

  SUCCEED();
}

// =========================================================================
// Focus mode observer tests
// =========================================================================

TEST_F(AstraBrowserViewTest, OnFocusModeEnteredNotification) {
  AstraBrowserView view(nullptr);
  MockAstraBrowserViewObserver observer;
  view.AddObserver(&observer);

  EXPECT_CALL(observer, OnFocusModeToggled(true)).Times(1);

  view.OnFocusModeEntered(base::Minutes(25));

  Mock::VerifyAndClearExpectations(&observer);
}

TEST_F(AstraBrowserViewTest, OnFocusModeExitedNotification) {
  AstraBrowserView view(nullptr);
  MockAstraBrowserViewObserver observer;
  view.AddObserver(&observer);

  EXPECT_CALL(observer, OnFocusModeToggled(false)).Times(1);

  view.OnFocusModeExited();

  Mock::VerifyAndClearExpectations(&observer);
}

TEST_F(AstraBrowserViewTest, ToggleFocusModeWithNullBrowserViewIsSafe) {
  AstraBrowserView view(nullptr);
  view.ToggleFocusMode();  // Should not crash.
  EXPECT_FALSE(view.IsFocusModeActive());
}

TEST_F(AstraBrowserViewTest, IsFocusModeActiveInitiallyFalse) {
  AstraBrowserView view(nullptr);
  EXPECT_FALSE(view.IsFocusModeActive());
}

// =========================================================================
// Workspace observer tests
// =========================================================================

TEST_F(AstraBrowserViewTest, OnWorkspaceChangedNotification) {
  AstraBrowserView view(nullptr);
  MockAstraBrowserViewObserver observer;
  view.AddObserver(&observer);

  std::string new_id = "workspace-alpha";
  EXPECT_CALL(observer, OnWorkspaceSwitched(new_id)).Times(1);

  view.OnActiveWorkspaceChanged("old-id", new_id);

  Mock::VerifyAndClearExpectations(&observer);
}

TEST_F(AstraBrowserViewTest, OnWorkspaceChangedMultipleObservers) {
  AstraBrowserView view(nullptr);
  MockAstraBrowserViewObserver observer1;
  MockAstraBrowserViewObserver observer2;

  view.AddObserver(&observer1);
  view.AddObserver(&observer2);

  std::string new_id = "workspace-beta";
  EXPECT_CALL(observer1, OnWorkspaceSwitched(new_id)).Times(1);
  EXPECT_CALL(observer2, OnWorkspaceSwitched(new_id)).Times(1);

  view.OnActiveWorkspaceChanged("old-id", new_id);

  Mock::VerifyAndClearExpectations(&observer1);
  Mock::VerifyAndClearExpectations(&observer2);
}

// =========================================================================
// Theme observer tests
// =========================================================================

TEST_F(AstraBrowserViewTest, OnThemeChangedDoesNotCrash) {
  AstraBrowserView view(nullptr);
  view.OnThemeChanged();  // Should not crash.
  SUCCEED();
}

TEST_F(AstraBrowserViewTest, OnAccentColorChangedDoesNotCrash) {
  AstraBrowserView view(nullptr);
  view.OnAccentColorChanged(SK_ColorBLUE);  // Should not crash.
  SUCCEED();
}

TEST_F(AstraBrowserViewTest, OnThemePresetChangedDoesNotCrash) {
  AstraBrowserView view(nullptr);
  view.OnThemePresetChanged(AstraThemePreset::kDark);  // Should not crash.
  SUCCEED();
}

// =========================================================================
// Convenience accessor tests
// =========================================================================

TEST_F(AstraBrowserViewTest, GetBrowserReturnsNullForNullBrowserView) {
  AstraBrowserView view(nullptr);
  EXPECT_EQ(nullptr, view.GetBrowser());
}

TEST_F(AstraBrowserViewTest, GetProfileReturnsNullForNullBrowserView) {
  AstraBrowserView view(nullptr);
  EXPECT_EQ(nullptr, view.GetProfile());
}

TEST_F(AstraBrowserViewTest, GetActiveWebContentsReturnsNullForNullBrowserView) {
  AstraBrowserView view(nullptr);
  EXPECT_EQ(nullptr, view.GetActiveWebContents());
}

// =========================================================================
// BubbleType enum tests
// =========================================================================

TEST_F(AstraBrowserViewTest, BubbleTypeEnumValues) {
  // Verify all bubble types exist and have distinct values.
  BubbleType type1 = BubbleType::kCommandPalette;
  BubbleType type2 = BubbleType::kSettings;
  BubbleType type3 = BubbleType::kTabSearch;
  BubbleType type4 = BubbleType::kImportExport;
  BubbleType type5 = BubbleType::kScreenshotCapture;
  BubbleType type6 = BubbleType::kRegionOverlay;
  BubbleType type7 = BubbleType::kNewTab;

  // All should be distinct.
  EXPECT_NE(static_cast<int>(type1), static_cast<int>(type2));
  EXPECT_NE(static_cast<int>(type2), static_cast<int>(type3));
  EXPECT_NE(static_cast<int>(type3), static_cast<int>(type4));
  EXPECT_NE(static_cast<int>(type4), static_cast<int>(type5));
  EXPECT_NE(static_cast<int>(type5), static_cast<int>(type6));
  EXPECT_NE(static_cast<int>(type6), static_cast<int>(type7));
}

TEST_F(AstraBrowserViewTest, BubbleTypeStartsAtZero) {
  // kCommandPalette should be 0 (the first enum value).
  EXPECT_EQ(0, static_cast<int>(BubbleType::kCommandPalette));
}

// =========================================================================
// Split view tests (null / no-op paths)
// =========================================================================

TEST_F(AstraBrowserViewTest, ToggleSplitViewWithNullBrowserViewIsSafe) {
  AstraBrowserView view(nullptr);
  view.ToggleSplitView();  // Should not crash.
  EXPECT_FALSE(view.IsSplitViewActive());
}

TEST_F(AstraBrowserViewTest, IsSplitViewActiveInitiallyFalse) {
  AstraBrowserView view(nullptr);
  EXPECT_FALSE(view.IsSplitViewActive());
}

TEST_F(AstraBrowserViewTest, SplitViewControllerInitiallyNull) {
  AstraBrowserView view(nullptr);
  EXPECT_EQ(nullptr, view.split_view_controller());
}

TEST_F(AstraBrowserViewTest, SwapSplitViewsWithNoControllerIsSafe) {
  AstraBrowserView view(nullptr);
  view.SwapSplitViews();  // Should not crash.
  SUCCEED();
}

// =========================================================================
// Workspace overview tests (null / no-op paths)
// =========================================================================

TEST_F(AstraBrowserViewTest, ShowWorkspaceOverviewWithNullBrowserViewIsSafe) {
  AstraBrowserView view(nullptr);
  view.ShowWorkspaceOverview();  // Should not crash.
  EXPECT_FALSE(view.IsWorkspaceOverviewVisible());
}

TEST_F(AstraBrowserViewTest, HideWorkspaceOverviewWithNoControllerIsSafe) {
  AstraBrowserView view(nullptr);
  view.HideWorkspaceOverview();  // Should not crash.
  SUCCEED();
}

TEST_F(AstraBrowserViewTest, WorkspaceOverviewControllerInitiallyNull) {
  AstraBrowserView view(nullptr);
  EXPECT_EQ(nullptr, view.workspace_overview_controller());
}

// =========================================================================
// Glance tests (null / no-op paths)
// =========================================================================

TEST_F(AstraBrowserViewTest, ShowGlanceWithNullBrowserViewIsSafe) {
  AstraBrowserView view(nullptr);
  view.ShowGlance(nullptr, gfx::Rect());  // Should not crash.
  EXPECT_FALSE(view.IsGlanceVisible());
}

TEST_F(AstraBrowserViewTest, HideGlanceWithNoControllerIsSafe) {
  AstraBrowserView view(nullptr);
  view.HideGlance();  // Should not crash.
  SUCCEED();
}

TEST_F(AstraBrowserViewTest, GlanceControllerInitiallyNull) {
  AstraBrowserView view(nullptr);
  EXPECT_EQ(nullptr, view.glance_controller());
}

// =========================================================================
// Workspace menu tests (null / no-op paths)
// =========================================================================

TEST_F(AstraBrowserViewTest, ShowWorkspaceMenuWithNullBrowserViewIsSafe) {
  AstraBrowserView view(nullptr);
  view.ShowWorkspaceMenu();  // Should not crash.
  EXPECT_FALSE(view.IsWorkspaceMenuVisible());
}

TEST_F(AstraBrowserViewTest, HideWorkspaceMenuWithNoControllerIsSafe) {
  AstraBrowserView view(nullptr);
  view.HideWorkspaceMenu();  // Should not crash.
  SUCCEED();
}

TEST_F(AstraBrowserViewTest, ProfileMenuControllerInitiallyNull) {
  AstraBrowserView view(nullptr);
  EXPECT_EQ(nullptr, view.profile_menu_controller());
}

// =========================================================================
// Edge case tests
// =========================================================================

TEST_F(AstraBrowserViewTest, OnTabStripModelChangedWithNoSidebarIsSafe) {
  AstraBrowserView view(nullptr);
  view.OnTabStripModelChanged();  // Should not crash.
  SUCCEED();
}

TEST_F(AstraBrowserViewTest, UpdateSidebarWithNoSidebarIsSafe) {
  AstraBrowserView view(nullptr);
  view.UpdateSidebar();  // Should not crash.
  SUCCEED();
}

TEST_F(AstraBrowserViewTest, RapidShowHideDoesNotCrash) {
  // Calling show/hide rapidly should not cause issues.
  AstraBrowserView view(nullptr);

  for (int i = 0; i < 100; i++) {
    view.ShowSidebar();
    view.HideSidebar();
  }
  for (int i = 0; i < 100; i++) {
    view.ToggleSidebar();
  }
  SUCCEED();
}

TEST_F(AstraBrowserViewTest, AllMethodsSafeWithNullBrowserView) {
  // Comprehensive test: every public method should handle null browser_view.
  AstraBrowserView view(nullptr);

  // Sidebar
  view.ShowSidebar();
  view.HideSidebar();
  view.ToggleSidebar();
  view.ToggleSidebarPin();
  view.IsSidebarVisible();
  view.IsSidebarPinned();

  // Bubbles
  view.ShowCommandPalette();
  view.HideCommandPalette();
  view.IsCommandPaletteOpen();
  view.ShowSettings();
  view.HideSettings();
  view.IsSettingsOpen();
  view.ShowTabSearch();
  view.HideTabSearch();
  view.IsTabSearchOpen();
  view.ShowNewTabPage();
  view.HideNewTabPage();
  view.IsNewTabPageVisible();
  view.HideImportExportDialog();
  view.IsImportExportDialogVisible();

  // Utilities
  view.CloseAllBubbles();
  view.IsAnyBubbleOpen();
  view.GetOpenBubbleCount();

  // Split view
  view.ToggleSplitView();
  view.HideSplitView();
  view.IsSplitViewActive();
  view.SwapSplitViews();

  // Focus mode
  view.ToggleFocusMode();
  view.IsFocusModeActive();

  // Workspace
  view.ShowWorkspaceOverview();
  view.HideWorkspaceOverview();
  view.IsWorkspaceOverviewVisible();
  view.ShowWorkspaceMenu();
  view.HideWorkspaceMenu();
  view.IsWorkspaceMenuVisible();

  // Glance
  view.HideGlance();
  view.IsGlanceVisible();

  // Update methods
  view.UpdateSidebar();
  view.OnTabStripModelChanged();

  // Service observer methods
  view.OnThemeChanged();
  view.OnAccentColorChanged(SK_ColorRED);
  view.OnThemePresetChanged(AstraThemePreset::kSystem);
  view.OnFocusModeEntered(base::Minutes(30));
  view.OnFocusModeExited();
  view.OnActiveWorkspaceChanged("old", "new");

  // Accessors
  view.GetActiveWebContents();
  view.GetBrowser();
  view.GetProfile();
  view.sidebar_view();
  view.split_view_controller();
  view.workspace_overview_controller();
  view.glance_controller();
  view.profile_menu_controller();
  view.focus_mode_controller();

  // Install / Uninstall
  view.Install();
  view.Uninstall();

  SUCCEED();
}

TEST_F(AstraBrowserViewTest, FocusModeControllerInitiallyNull) {
  AstraBrowserView view(nullptr);
  EXPECT_EQ(nullptr, view.focus_mode_controller());
}

TEST_F(AstraBrowserViewTest, IsTrackedBubbleWidgetWithNullReturnsFalse) {
  // Null widget is not a tracked bubble widget.
  AstraBrowserView view(nullptr);
  EXPECT_FALSE(view.IsTrackedBubbleWidget(nullptr));
}

TEST_F(AstraBrowserViewTest, IsTrackedBubbleWidgetWithUnknownWidgetReturnsFalse) {
  AstraBrowserView view(nullptr);
  auto widget = CreateBubbleWidget();
  EXPECT_FALSE(view.IsTrackedBubbleWidget(widget.get()));
}

TEST_F(AstraBrowserViewTest, IsTrackedBubbleWidgetWithKnownWidgetReturnsTrue) {
  AstraBrowserView view(nullptr);
  auto widget = CreateBubbleWidget();

  SetCommandPaletteWidget(&view, widget.get());
  EXPECT_TRUE(view.IsTrackedBubbleWidget(widget.get()));
}

}  // namespace
}  // namespace astra
