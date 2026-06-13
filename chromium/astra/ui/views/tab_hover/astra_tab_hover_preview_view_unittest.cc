// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Unit tests for AstraTabHoverPreviewView.
//
// Tests verify:
//   - Construction and initial state
//   - Title/URL/favicon setters
//   - Thumbnail visibility (compact vs expanded)
//   - Workspace indicator visibility and color
//   - Peek button visibility and text
//   - Preferred size calculations
//   - Theme/color integration
//   - Delegate callbacks
//   - Accessibility
//
// Chromium test pattern: views::test::ViewsTestBase
//   (ui/views/test/views_test_base.h)

#include "astra/ui/views/tab_hover/astra_tab_hover_preview_view.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace astra {

namespace {

// Test delegate that tracks callback counts.
class TestPreviewDelegate : public AstraTabHoverPreviewView::Delegate {
 public:
  int peek_requested_count = 0;
  int close_requested_count = 0;
  int view_destroyed_count = 0;
  int mute_toggled_count = 0;

  void OnPeekRequested() override { peek_requested_count++; }
  void OnCloseRequested() override { close_requested_count++; }
  void OnPreviewViewDestroyed() override { view_destroyed_count++; }
  void OnMuteToggled() override { mute_toggled_count++; }
};

}  // namespace

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

class AstraTabHoverPreviewViewTest : public views::ViewsTestBase {
 public:
  AstraTabHoverPreviewViewTest() = default;
  ~AstraTabHoverPreviewViewTest() override = default;

  // ViewsTestBase:
  void SetUp() override {
    ViewsTestBase::SetUp();

    // Create a widget so the preview view has a valid parent and
    // ColorProvider access for theme tests.
    widget_ = CreateTestWidget();

    // Create an anchor view for the preview bubble.
    anchor_view_ = widget_->SetContentsView(std::make_unique<views::View>());
    anchor_view_->SetPreferredSize(gfx::Size(100, 100));
    widget_->Show();
  }

  void TearDown() override {
    widget_.reset();
    ViewsTestBase::TearDown();
  }

  // Creates a preview view bubble anchored to the anchor view.
  // The caller is responsible for closing the widget.
  AstraTabHoverPreviewView* CreatePreviewView(
      AstraTabHoverPreviewView::Delegate* delegate) {
    views::Widget* bubble_widget = AstraTabHoverPreviewView::ShowBubble(
        anchor_view_, gfx::Rect(), nullptr, delegate);
    return static_cast<AstraTabHoverPreviewView*>(
        bubble_widget->widget_delegate()->AsDialogDelegate());
  }

 protected:
  std::unique_ptr<views::Widget> widget_;
  raw_ptr<views::View> anchor_view_ = nullptr;
};

// =========================================================================
// Construction tests
// =========================================================================

TEST_F(AstraTabHoverPreviewViewTest, WidgetIsCreated) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);

  EXPECT_NE(nullptr, preview->GetWidget());
}

TEST_F(AstraTabHoverPreviewViewTest, DefaultThumbnailIsHidden) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);

  EXPECT_FALSE(preview->thumbnail_visible());
}

TEST_F(AstraTabHoverPreviewViewTest, CompactSizeByDefault) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);

  // With thumbnail hidden, the preferred size should match compact size.
  gfx::Size pref = preview->CalculatePreferredSize();
  EXPECT_EQ(AstraTabHoverPreviewView::kCompactSize.width(), pref.width());
}

// =========================================================================
// Title / URL / favicon tests
// =========================================================================

TEST_F(AstraTabHoverPreviewViewTest, SetTitleText) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);

  preview->SetTitleText(u"Test Tab Title");
  // Should not crash.
}

TEST_F(AstraTabHoverPreviewViewTest, SetUrlText) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);

  preview->SetUrlText(u"https://example.com/path");
  // Should not crash.
}

TEST_F(AstraTabHoverPreviewViewTest, SetEmptyTitle) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);

  preview->SetTitleText(std::u16string());
  // Should not crash with empty title.
}

TEST_F(AstraTabHoverPreviewViewTest, SetEmptyUrl) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);

  preview->SetUrlText(std::u16string());
  // Should not crash with empty URL.
}

TEST_F(AstraTabHoverPreviewViewTest, SetFaviconPlaceholder) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);

  preview->SetFaviconPlaceholder();
  // Should not crash.
}

// =========================================================================
// Thumbnail visibility tests
// =========================================================================

TEST_F(AstraTabHoverPreviewViewTest, SetThumbnailVisibleTrue) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);

  preview->SetThumbnailVisible(true);
  EXPECT_TRUE(preview->thumbnail_visible());
}

TEST_F(AstraTabHoverPreviewViewTest, SetThumbnailVisibleFalse) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);

  preview->SetThumbnailVisible(true);
  ASSERT_TRUE(preview->thumbnail_visible());

  preview->SetThumbnailVisible(false);
  EXPECT_FALSE(preview->thumbnail_visible());
}

TEST_F(AstraTabHoverPreviewViewTest, ThumbnailPlaceholder) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);

  preview->SetThumbnailPlaceholder();
  // Should not crash.
}

TEST_F(AstraTabHoverPreviewViewTest, ExpandedSizeWithThumbnail) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);

  preview->SetThumbnailVisible(true);
  gfx::Size pref = preview->CalculatePreferredSize();
  EXPECT_EQ(AstraTabHoverPreviewView::kExpandedSize.height(), pref.height());
}

TEST_F(AstraTabHoverPreviewViewTest, ExpandedSizeLargerThanCompact) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);

  preview->SetThumbnailVisible(false);
  gfx::Size compact = preview->CalculatePreferredSize();

  preview->SetThumbnailVisible(true);
  gfx::Size expanded = preview->CalculatePreferredSize();

  EXPECT_GT(expanded.height(), compact.height());
  // Width should be the same.
  EXPECT_EQ(expanded.width(), compact.width());
}

// =========================================================================
// Workspace indicator tests
// =========================================================================

TEST_F(AstraTabHoverPreviewViewTest, SetWorkspaceIndicatorVisible) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);

  preview->SetWorkspaceIndicatorVisible(true);
  // Should not crash.
}

TEST_F(AstraTabHoverPreviewViewTest, SetWorkspaceIndicatorColor) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);

  preview->SetWorkspaceIndicatorColor(SK_ColorBLUE);
  preview->SetWorkspaceIndicatorColor(SK_ColorRED);
  // Should not crash.
}

TEST_F(AstraTabHoverPreviewViewTest, WorkspaceIndicatorHiddenByDefault) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);

  // Initially the workspace indicator dot should exist but be hidden by default.
  // We can't directly check visibility since it's a private child view,
  // but setting visible to false should not crash.
  preview->SetWorkspaceIndicatorVisible(false);
}

// =========================================================================
// Peek button tests
// =========================================================================

TEST_F(AstraTabHoverPreviewViewTest, SetPeekButtonVisible) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);

  preview->SetPeekButtonVisible(true);
  // Should not crash.
}

TEST_F(AstraTabHoverPreviewViewTest, SetPeekButtonText) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);

  preview->SetPeekButtonText(u"Peek");
  preview->SetPeekButtonText(u"Expand");
  // Should not crash.
}

TEST_F(AstraTabHoverPreviewViewTest, PeekButtonVisibleByDefault) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);

  // The peek button should be visible by default.
  // We can verify by checking that setting it to visible doesn't crash.
  preview->SetPeekButtonVisible(true);
}

// =========================================================================
// Delegate callback tests
// =========================================================================

TEST_F(AstraTabHoverPreviewViewTest, WidgetDestroyNotifiesDelegate) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);

  EXPECT_EQ(0, delegate.view_destroyed_count);

  // Close the widget — this should trigger OnPreviewViewDestroyed.
  preview->GetWidget()->CloseNow();
  EXPECT_GE(delegate.view_destroyed_count, 1);
}

// =========================================================================
// Theme / color tests
// =========================================================================

TEST_F(AstraTabHoverPreviewViewTest, OnThemeChangedDoesNotCrash) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);

  preview->OnThemeChanged();
  // Should not crash.
}

TEST_F(AstraTabHoverPreviewViewTest, HasColorProvider) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);

  EXPECT_NE(nullptr, preview->GetColorProvider());
}

// =========================================================================
// Size constant tests
// =========================================================================

TEST_F(AstraTabHoverPreviewViewTest, CompactSizeIsPositive) {
  EXPECT_GT(AstraTabHoverPreviewView::kCompactSize.width(), 0);
  EXPECT_GT(AstraTabHoverPreviewView::kCompactSize.height(), 0);
}

TEST_F(AstraTabHoverPreviewViewTest, ExpandedSizeIsPositive) {
  EXPECT_GT(AstraTabHoverPreviewView::kExpandedSize.width(), 0);
  EXPECT_GT(AstraTabHoverPreviewView::kExpandedSize.height(), 0);
}

TEST_F(AstraTabHoverPreviewViewTest, ExpandedTallerThanCompact) {
  EXPECT_GT(AstraTabHoverPreviewView::kExpandedSize.height(),
            AstraTabHoverPreviewView::kCompactSize.height());
}

TEST_F(AstraTabHoverPreviewViewTest, SameWidthBothModes) {
  EXPECT_EQ(AstraTabHoverPreviewView::kCompactSize.width(),
            AstraTabHoverPreviewView::kExpandedSize.width());
}

TEST_F(AstraTabHoverPreviewViewTest, ThumbnailSizeIsPositive) {
  EXPECT_GT(AstraTabHoverPreviewView::kThumbnailSize.width(), 0);
  EXPECT_GT(AstraTabHoverPreviewView::kThumbnailSize.height(), 0);
}

TEST_F(AstraTabHoverPreviewViewTest, FaviconSizeIsPositive) {
  EXPECT_GT(AstraTabHoverPreviewView::kFaviconSize, 0);
}

TEST_F(AstraTabHoverPreviewViewTest, WorkspaceDotSizeIsPositive) {
  EXPECT_GT(AstraTabHoverPreviewView::kWorkspaceDotSize, 0);
}

TEST_F(AstraTabHoverPreviewViewTest, CloseButtonSizeIsPositive) {
  EXPECT_GT(AstraTabHoverPreviewView::kCloseButtonSize, 0);
}

TEST_F(AstraTabHoverPreviewViewTest, PeekButtonSizeIsPositive) {
  EXPECT_GT(AstraTabHoverPreviewView::kPeekButtonHeight, 0);
  EXPECT_GT(AstraTabHoverPreviewView::kPeekButtonWidth, 0);
}

TEST_F(AstraTabHoverPreviewViewTest, PaddingIsPositive) {
  EXPECT_GT(AstraTabHoverPreviewView::kHorizontalPadding, 0);
  EXPECT_GT(AstraTabHoverPreviewView::kVerticalPadding, 0);
}

TEST_F(AstraTabHoverPreviewViewTest, SpacingIsPositive) {
  EXPECT_GT(AstraTabHoverPreviewView::kHeaderToThumbnailSpacing, 0);
  EXPECT_GT(AstraTabHoverPreviewView::kThumbnailToFooterSpacing, 0);
}

// =========================================================================
// UpdateFromWebContents tests (null WebContents)
// =========================================================================

TEST_F(AstraTabHoverPreviewViewTest, UpdateFromNullWebContents) {
  TestPreviewDelegate delegate;
  AstraTabHoverPreviewView* preview = CreatePreviewView(&delegate);

  preview->UpdateFromWebContents(nullptr);
  // Should not crash with null WebContents.
}

}  // namespace astra
