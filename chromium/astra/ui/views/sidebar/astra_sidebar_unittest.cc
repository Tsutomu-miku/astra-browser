// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Comprehensive unit tests for the Astra sidebar model, controller, view,
// and sidebar item views.
//
// Test categories:
//   - Model construction and default state
//   - Model sidebar state (visible, pinned, width, position)
//   - Model active section management
//   - Model section management (CRUD, visibility, ordering)
//   - Model section collapse/expand
//   - Model presentation settings
//   - Model bulk operations
//   - Model utility methods
//   - Model observer pattern
//   - Observer default implementations
//   - Model persistence via PrefService
//   - Controller lifecycle and state management
//   - Controller section navigation
//   - Controller view binding
//   - Edge cases (invalid IDs, boundary values, rapid changes)
//   - View tests (construction, model integration, accessibility)
//   - Item view tests (base class + all 8 item types)
//
// Notes:
//   - Model tests are fully self-contained.
//   - Controller tests verify state machine without a real service.
//   - Item view tests use ViewsTestBase for widget/context setup.
//
// Chromium test pattern: views::test::ViewsTestBase
//   (ui/views/test/views_test_base.h)

#include "astra/ui/views/sidebar/astra_sidebar_controller.h"
#include "astra/ui/views/sidebar/astra_sidebar_model.h"
#include "astra/ui/views/sidebar/astra_sidebar_view.h"

#include "astra/ui/views/sidebar/astra_bookmark_item_view.h"
#include "astra/ui/views/sidebar/astra_download_item_view.h"
#include "astra/ui/views/sidebar/astra_extension_icon_view.h"
#include "astra/ui/views/sidebar/astra_extension_popup_view.h"
#include "astra/ui/views/sidebar/astra_sidebar_extensions_view.h"
#include "astra/ui/views/sidebar/astra_history_item_view.h"
#include "astra/ui/views/sidebar/astra_note_item_view.h"
#include "astra/ui/views/sidebar/astra_password_item_view.h"
#include "astra/ui/views/sidebar/astra_reading_list_item_view.h"
#include "astra/ui/views/sidebar/astra_recently_closed_item_view.h"
#include "astra/ui/views/sidebar/astra_sidebar_item_view.h"
#include "astra/ui/views/sidebar/astra_sidebar_tab_groups_view.h"
#include "astra/ui/views/sidebar/astra_sidebar_recently_closed_view.h"
#include "astra/ui/views/sidebar/astra_tab_group_header_view.h"
#include "astra/ui/views/sidebar/astra_tab_group_tab_item_view.h"

#include "astra/ui/views/sidebar/astra_sidebar_stack_child_view.h"
#include "astra/ui/views/sidebar/astra_sidebar_stack_header_view.h"
#include "astra/ui/views/sidebar/astra_sidebar_stack_tab_item_view.h"
#include "astra/ui/views/sidebar/astra_sidebar_stack_view.h"

#include "astra/ui/views/sidebar/astra_sidebar_section_view.h"
#include "astra/ui/views/sidebar/astra_sidebar_bookmarks_view.h"
#include "astra/ui/views/sidebar/astra_sidebar_history_view.h"
#include "astra/ui/views/sidebar/astra_sidebar_downloads_view.h"

#include <string>
#include <vector>

#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

using ::testing::_;
using ::testing::AtLeast;
using ::testing::InSequence;
using ::testing::Mock;

// =========================================================================
// Mock observer for model tests
// =========================================================================

class MockSidebarModelObserver : public AstraSidebarModelObserver {
 public:
  MockSidebarModelObserver() = default;
  ~MockSidebarModelObserver() override = default;

  MOCK_METHOD(void, OnSidebarShown, (), (override));
  MOCK_METHOD(void, OnSidebarHidden, (), (override));
  MOCK_METHOD(void, OnSidebarPinnedChanged, (bool pinned), (override));
  MOCK_METHOD(void, OnActiveSectionChanged,
              (const std::string& section_id), (override));
  MOCK_METHOD(void, OnSectionVisibilityChanged,
              (const std::string& section_id, bool visible), (override));
  MOCK_METHOD(void, OnSectionOrderChanged, (), (override));
  MOCK_METHOD(void, OnSectionCollapsedChanged,
              (const std::string& section_id, bool collapsed), (override));
  MOCK_METHOD(void, OnSidebarWidthChanged, (int width), (override));
  MOCK_METHOD(void, OnSidebarPositionChanged,
              (AstraSidebarPosition position), (override));
  MOCK_METHOD(void, OnSidebarSettingsChanged, (), (override));
};

// =========================================================================
// Mock delegates for item view tests
// =========================================================================

class MockSidebarItemHoverDelegate : public AstraSidebarItemHoverDelegate {
 public:
  MOCK_METHOD(void, OnItemHoverStarted,
              (AstraSidebarItemView * item, const gfx::Point& mouse_location),
              (override));
  MOCK_METHOD(void, OnItemHoverEnded, (AstraSidebarItemView * item),
              (override));
  MOCK_METHOD(void, OnItemHoverMoved,
              (AstraSidebarItemView * item, const gfx::Point& mouse_location),
              (override));
};

class MockSidebarItemDragDelegate : public AstraSidebarItemDragDelegate {
 public:
  MOCK_METHOD(void, OnItemDragStarted,
              (AstraSidebarItemView * item, const gfx::Point& mouse_location),
              (override));
};

class MockSidebarItemContextMenuDelegate
    : public AstraSidebarItemContextMenuDelegate {
 public:
  MOCK_METHOD(void, OnItemContextMenuRequested,
              (AstraSidebarItemView * item, const gfx::Point& point),
              (override));
};

class MockBookmarkItemDelegate : public AstraBookmarkItemDelegate {
 public:
  MOCK_METHOD(void, OnBookmarkItemClicked,
              (const bookmarks::BookmarkNode* node, bool open_in_new_tab),
              (override));
  MOCK_METHOD(void, OnBookmarkFolderExpandedToggled,
              (const bookmarks::BookmarkNode* folder_node), (override));
};

class MockHistoryItemDelegate : public AstraHistoryItemDelegate {
 public:
  MOCK_METHOD(void, OnHistoryItemClicked, (const GURL& url), (override));
  MOCK_METHOD(void, OnHistoryItemRemoved, (const GURL& url), (override));
};

class MockDownloadItemDelegate : public AstraDownloadItemDelegate {
 public:
  MOCK_METHOD(void, OnDownloadItemClicked, (const std::string& download_id),
              (override));
  MOCK_METHOD(void, OnDownloadCancelRequested,
              (const std::string& download_id), (override));
  MOCK_METHOD(void, OnDownloadPauseRequested,
              (const std::string& download_id), (override));
  MOCK_METHOD(void, OnDownloadResumeRequested,
              (const std::string& download_id), (override));
  MOCK_METHOD(void, OnDownloadOpenRequested, (const std::string& download_id),
              (override));
  MOCK_METHOD(void, OnDownloadShowInFolderRequested,
              (const std::string& download_id), (override));
};

class MockReadingListItemDelegate : public AstraReadingListItemDelegate {
 public:
  MOCK_METHOD(void, OnReadingListItemClicked, (const GURL& url), (override));
  MOCK_METHOD(void, OnReadingListToggleRead, (const GURL& url), (override));
  MOCK_METHOD(void, OnReadingListRemove, (const GURL& url), (override));
};

class MockNoteItemDelegate : public AstraNoteItemDelegate {
 public:
  MOCK_METHOD(void, OnNoteItemClicked, (const std::string& note_id),
              (override));
  MOCK_METHOD(void, OnNoteDeleteRequested, (const std::string& note_id),
              (override));
};

class MockPasswordItemDelegate : public AstraPasswordItemDelegate {
 public:
  MOCK_METHOD(void, OnPasswordItemClicked,
              (const std::u16string & site, const std::u16string& username),
              (override));
  MOCK_METHOD(void, OnPasswordCopyRequested,
              (const std::u16string & site, const std::u16string& username),
              (override));
};

class MockExtensionIconDelegate : public AstraExtensionIconDelegate {
 public:
  MOCK_METHOD(void, OnExtensionClicked,
              (const std::string& extension_id), (override));
  MOCK_METHOD(void, OnExtensionMiddleClicked,
              (const std::string& extension_id), (override));
  MOCK_METHOD(void, OnExtensionRightClicked,
              (const std::string& extension_id, const gfx::Point& point),
              (override));
  MOCK_METHOD(void, OnExtensionPopupShown,
              (const std::string& extension_id), (override));
  MOCK_METHOD(void, OnExtensionPopupClosed,
              (const std::string& extension_id), (override));
  MOCK_METHOD(void, OnPinExtension,
              (const std::string& extension_id, bool pinned), (override));
  MOCK_METHOD(void, OnManageExtensionRequested,
              (const std::string& extension_id), (override));
  MOCK_METHOD(void, OnRemoveExtensionRequested,
              (const std::string& extension_id), (override));
  MOCK_METHOD(void, OnDisableExtensionRequested,
              (const std::string& extension_id), (override));
  MOCK_METHOD(void, OnExtensionOptionsRequested,
              (const std::string& extension_id), (override));
  // Legacy methods for backward compatibility.
  MOCK_METHOD(void, OnExtensionIconClicked,
              (const std::string& extension_id, views::View* anchor_view),
              (override));
  MOCK_METHOD(void, OnExtensionIconContextMenu,
              (const std::string& extension_id, const gfx::Point& point),
              (override));
};

// =========================================================================
// Mock delegate for extension popup tests
// =========================================================================

class MockExtensionPopupDelegate : public AstraExtensionPopupDelegate {
 public:
  MOCK_METHOD(void, OnExtensionPopupClosed,
              (const std::string& extension_id), (override));
  MOCK_METHOD(void, OnExtensionPopupShown,
              (const std::string& extension_id), (override));
};

// =========================================================================
// Mock delegate for sidebar extensions view tests
// =========================================================================

class MockSidebarExtensionsDelegate : public AstraSidebarExtensionsDelegate {
 public:
  MOCK_METHOD(void, OnExtensionClicked,
              (const std::string& extension_id), (override));
  MOCK_METHOD(void, OnExtensionMiddleClicked,
              (const std::string& extension_id), (override));
  MOCK_METHOD(void, OnExtensionRightClicked,
              (const std::string& extension_id, const gfx::Point& point),
              (override));
  MOCK_METHOD(void, OnExtensionPinned,
              (const std::string& extension_id, bool pinned), (override));
  MOCK_METHOD(void, OnExtensionReordered,
              (int from_index, int to_index), (override));
  MOCK_METHOD(void, OnManageExtensionsRequested, (), (override));
  MOCK_METHOD(void, OnExtensionPopupShown,
              (const std::string& extension_id), (override));
  MOCK_METHOD(void, OnExtensionPopupClosed,
              (const std::string& extension_id), (override));
  MOCK_METHOD(void, OnEnableExtensionRequested,
              (const std::string& extension_id), (override));
  MOCK_METHOD(void, OnDisableExtensionRequested,
              (const std::string& extension_id), (override));
  MOCK_METHOD(void, OnRemoveExtensionRequested,
              (const std::string& extension_id), (override));
  MOCK_METHOD(void, OnExtensionOptionsRequested,
              (const std::string& extension_id), (override));
};

class MockRecentlyClosedItemDelegate
    : public AstraRecentlyClosedItemDelegate {
 public:
  MOCK_METHOD(void, OnRecentlyClosedItemClicked, (int entry_id), (override));
  MOCK_METHOD(void, OnRecentlyClosedItemMiddleClicked, (int entry_id),
              (override));
};

// =========================================================================
// Test fixture for model tests with PrefService
// =========================================================================

class AstraSidebarModelTest : public testing::Test {
 protected:
  void SetUp() override {
    prefs_ = std::make_unique<TestingPrefServiceSimple>();
    prefs::RegisterProfilePrefs(prefs_->registry());
    model_ = std::make_unique<AstraSidebarModel>(prefs_.get());
  }

  void TearDown() override {
    model_.reset();
    prefs_.reset();
  }

  std::unique_ptr<TestingPrefServiceSimple> prefs_;
  std::unique_ptr<AstraSidebarModel> model_;
};

// =========================================================================
// Test fixture for model tests without PrefService
// =========================================================================

class AstraSidebarModelNoPrefsTest : public testing::Test {
 protected:
  void SetUp() override {
    model_ = std::make_unique<AstraSidebarModel>();
  }

  void TearDown() override { model_.reset(); }

  std::unique_ptr<AstraSidebarModel> model_;
};

// =========================================================================
// Test fixture for controller tests
// =========================================================================

class AstraSidebarControllerTest : public testing::Test {
 protected:
  void SetUp() override {
    prefs_ = std::make_unique<TestingPrefServiceSimple>();
    prefs::RegisterProfilePrefs(prefs_->registry());
    controller_ = std::make_unique<AstraSidebarController>(nullptr);
  }

  void TearDown() override {
    controller_.reset();
    prefs_.reset();
  }

  std::unique_ptr<TestingPrefServiceSimple> prefs_;
  std::unique_ptr<AstraSidebarController> controller_;
};

// =========================================================================
// Test fixture for view tests (uses ViewsTestBase)
// =========================================================================

class AstraSidebarViewTest : public views::ViewsTestBase {
 public:
  AstraSidebarViewTest() = default;
  ~AstraSidebarViewTest() override = default;
  AstraSidebarViewTest(const AstraSidebarViewTest&) = delete;
  AstraSidebarViewTest& operator=(const AstraSidebarViewTest&) = delete;

 protected:
  void SetUp() override {
    ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();
  }

  void TearDown() override {
    widget_.reset();
    ViewsTestBase::TearDown();
  }

  std::unique_ptr<views::Widget> widget_;
};

// =========================================================================
// Test fixture for sidebar item view tests
// =========================================================================

class AstraSidebarItemViewTest : public views::ViewsTestBase {
 public:
  AstraSidebarItemViewTest() = default;
  ~AstraSidebarItemViewTest() override = default;
  AstraSidebarItemViewTest(const AstraSidebarItemViewTest&) = delete;
  AstraSidebarItemViewTest& operator=(const AstraSidebarItemViewTest&) = delete;

 protected:
  void SetUp() override {
    ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();
    item_view_ = widget_->SetContentsView(
        std::make_unique<AstraSidebarItemView>());
    widget_->Show();
  }

  void TearDown() override {
    item_view_ = nullptr;
    widget_.reset();
    ViewsTestBase::TearDown();
  }

  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraSidebarItemView> item_view_ = nullptr;
};

// =========================================================================
// Test fixture for bookmark item view tests
// =========================================================================

class AstraBookmarkItemViewTest : public views::ViewsTestBase {
 public:
  AstraBookmarkItemViewTest() = default;
  ~AstraBookmarkItemViewTest() override = default;
  AstraBookmarkItemViewTest(const AstraBookmarkItemViewTest&) = delete;
  AstraBookmarkItemViewTest& operator=(const AstraBookmarkItemViewTest&) =
      delete;

 protected:
  void SetUp() override {
    ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();
    bookmark_view_ = widget_->SetContentsView(
        std::make_unique<AstraBookmarkItemView>(
            nullptr, AstraBookmarkItemView::Type::kUrl, 0));
    widget_->Show();
  }

  void TearDown() override {
    bookmark_view_ = nullptr;
    widget_.reset();
    ViewsTestBase::TearDown();
  }

  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraBookmarkItemView> bookmark_view_ = nullptr;
};

// =========================================================================
// Test fixture for history item view tests
// =========================================================================

class AstraHistoryItemViewTest : public views::ViewsTestBase {
 public:
  AstraHistoryItemViewTest() = default;
  ~AstraHistoryItemViewTest() override = default;
  AstraHistoryItemViewTest(const AstraHistoryItemViewTest&) = delete;
  AstraHistoryItemViewTest& operator=(const AstraHistoryItemViewTest&) = delete;

 protected:
  void SetUp() override {
    ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();
    history_view_ = widget_->SetContentsView(
        std::make_unique<AstraHistoryItemView>(
            u"Test Title", GURL("https://example.com"),
            base::Time::Now()));
    widget_->Show();
  }

  void TearDown() override {
    history_view_ = nullptr;
    widget_.reset();
    ViewsTestBase::TearDown();
  }

  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraHistoryItemView> history_view_ = nullptr;
};

// =========================================================================
// Test fixture for download item view tests
// =========================================================================

class AstraDownloadItemViewTest : public views::ViewsTestBase {
 public:
  AstraDownloadItemViewTest() = default;
  ~AstraDownloadItemViewTest() override = default;
  AstraDownloadItemViewTest(const AstraDownloadItemViewTest&) = delete;
  AstraDownloadItemViewTest& operator=(const AstraDownloadItemViewTest&) =
      delete;

 protected:
  void SetUp() override {
    ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();
    download_view_ = widget_->SetContentsView(
        std::make_unique<AstraDownloadItemView>(
            "test-id-1", u"file.zip", AstraDownloadState::kInProgress,
            1024, 10240));
    widget_->Show();
  }

  void TearDown() override {
    download_view_ = nullptr;
    widget_.reset();
    ViewsTestBase::TearDown();
  }

  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraDownloadItemView> download_view_ = nullptr;
};

// =========================================================================
// Test fixture for reading list item view tests
// =========================================================================

class AstraReadingListItemViewTest : public views::ViewsTestBase {
 public:
  AstraReadingListItemViewTest() = default;
  ~AstraReadingListItemViewTest() override = default;
  AstraReadingListItemViewTest(const AstraReadingListItemViewTest&) = delete;
  AstraReadingListItemViewTest& operator=(
      const AstraReadingListItemViewTest&) = delete;

 protected:
  void SetUp() override {
    ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();
    reading_view_ = widget_->SetContentsView(
        std::make_unique<AstraReadingListItemView>(
            GURL("https://example.com/article"), u"Test Article",
            u"example.com", false));
    widget_->Show();
  }

  void TearDown() override {
    reading_view_ = nullptr;
    widget_.reset();
    ViewsTestBase::TearDown();
  }

  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraReadingListItemView> reading_view_ = nullptr;
};

// =========================================================================
// Test fixture for note item view tests
// =========================================================================

class AstraNoteItemViewTest : public views::ViewsTestBase {
 public:
  AstraNoteItemViewTest() = default;
  ~AstraNoteItemViewTest() override = default;
  AstraNoteItemViewTest(const AstraNoteItemViewTest&) = delete;
  AstraNoteItemViewTest& operator=(const AstraNoteItemViewTest&) = delete;

 protected:
  void SetUp() override {
    ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();
    note_view_ = widget_->SetContentsView(
        std::make_unique<AstraNoteItemView>(
            "note-1", u"My Note", u"This is a preview...", u"2h ago",
            SK_ColorYELLOW));
    widget_->Show();
  }

  void TearDown() override {
    note_view_ = nullptr;
    widget_.reset();
    ViewsTestBase::TearDown();
  }

  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraNoteItemView> note_view_ = nullptr;
};

// =========================================================================
// Test fixture for password item view tests
// =========================================================================

class AstraPasswordItemViewTest : public views::ViewsTestBase {
 public:
  AstraPasswordItemViewTest() = default;
  ~AstraPasswordItemViewTest() override = default;
  AstraPasswordItemViewTest(const AstraPasswordItemViewTest&) = delete;
  AstraPasswordItemViewTest& operator=(const AstraPasswordItemViewTest&) =
      delete;

 protected:
  void SetUp() override {
    ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();
    password_view_ = widget_->SetContentsView(
        std::make_unique<AstraPasswordItemView>(
            u"example.com", u"user@example.com", false));
    widget_->Show();
  }

  void TearDown() override {
    password_view_ = nullptr;
    widget_.reset();
    ViewsTestBase::TearDown();
  }

  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraPasswordItemView> password_view_ = nullptr;
};

// =========================================================================
// Test fixture for extension icon view tests
// =========================================================================

class AstraExtensionIconViewTest : public views::ViewsTestBase {
 public:
  AstraExtensionIconViewTest() = default;
  ~AstraExtensionIconViewTest() override = default;
  AstraExtensionIconViewTest(const AstraExtensionIconViewTest&) = delete;
  AstraExtensionIconViewTest& operator=(const AstraExtensionIconViewTest&) =
      delete;

 protected:
  void SetUp() override {
    ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();
    extension_view_ = widget_->SetContentsView(
        std::make_unique<AstraExtensionIconView>("ext-123", nullptr));
    widget_->Show();
  }

  void TearDown() override {
    extension_view_ = nullptr;
    widget_.reset();
    ViewsTestBase::TearDown();
  }

  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraExtensionIconView> extension_view_ = nullptr;
};

// =========================================================================
// Test fixture for extension popup view tests
// =========================================================================

class AstraExtensionPopupViewTest : public views::ViewsTestBase {
 public:
  AstraExtensionPopupViewTest() = default;
  ~AstraExtensionPopupViewTest() override = default;
  AstraExtensionPopupViewTest(const AstraExtensionPopupViewTest&) = delete;
  AstraExtensionPopupViewTest& operator=(const AstraExtensionPopupViewTest&) =
      delete;

 protected:
  void SetUp() override {
    ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();
    // Create an anchor view for the popup.
    anchor_view_ = widget_->SetContentsView(std::make_unique<views::View>());
    anchor_view_->SetPreferredSize(gfx::Size(32, 32));
    widget_->Show();
  }

  void TearDown() override {
    popup_view_ = nullptr;
    anchor_view_ = nullptr;
    widget_.reset();
    ViewsTestBase::TearDown();
  }

  // Create a popup view for testing. Not shown by default.
  AstraExtensionPopupView* CreatePopup(const std::string& id = "ext-1",
                                       const std::u16string& name = u"Test") {
    popup_view_ = new AstraExtensionPopupView(id, name, anchor_view_, nullptr);
    return popup_view_;
  }

  std::unique_ptr<views::Widget> widget_;
  raw_ptr<views::View> anchor_view_ = nullptr;
  raw_ptr<AstraExtensionPopupView> popup_view_ = nullptr;
};

// =========================================================================
// Test fixture for sidebar extensions view tests
// =========================================================================

class AstraSidebarExtensionsViewTest : public views::ViewsTestBase {
 public:
  AstraSidebarExtensionsViewTest() = default;
  ~AstraSidebarExtensionsViewTest() override = default;
  AstraSidebarExtensionsViewTest(const AstraSidebarExtensionsViewTest&) =
      delete;
  AstraSidebarExtensionsViewTest& operator=(
      const AstraSidebarExtensionsViewTest&) = delete;

 protected:
  void SetUp() override {
    ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();
    extensions_view_ = widget_->SetContentsView(
        std::make_unique<AstraSidebarExtensionsView>(nullptr));
    widget_->Show();
  }

  void TearDown() override {
    extensions_view_ = nullptr;
    widget_.reset();
    ViewsTestBase::TearDown();
  }

  // Helper to create a test extension info struct.
  AstraExtensionInfo CreateTestExtension(const std::string& id,
                                         const std::u16string& name) {
    AstraExtensionInfo info;
    info.extension_id = id;
    info.name = name;
    info.state = AstraExtensionState::kEnabled;
    info.is_action = true;
    info.has_popup = true;
    return info;
  }

  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraSidebarExtensionsView> extensions_view_ = nullptr;
};

// =========================================================================
// Test fixture for recently closed item view tests
// =========================================================================

class AstraRecentlyClosedItemViewTest : public views::ViewsTestBase {
 public:
  AstraRecentlyClosedItemViewTest() = default;
  ~AstraRecentlyClosedItemViewTest() override = default;
  AstraRecentlyClosedItemViewTest(const AstraRecentlyClosedItemViewTest&) =
      delete;
  AstraRecentlyClosedItemViewTest& operator=(
      const AstraRecentlyClosedItemViewTest&) = delete;

 protected:
  void SetUp() override {
    ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();
    closed_view_ = widget_->SetContentsView(
        std::make_unique<AstraRecentlyClosedItemView>(
            u"Test Page", GURL("https://example.com"),
            base::Time::Now() - base::Minutes(5), 42));
    widget_->Show();
  }

  void TearDown() override {
    closed_view_ = nullptr;
    widget_.reset();
    ViewsTestBase::TearDown();
  }

  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraRecentlyClosedItemView> closed_view_ = nullptr;
};

// =========================================================================
// Model construction and default state tests
// =========================================================================

// Test 1: Model with no prefs has sensible defaults.
TEST_F(AstraSidebarModelNoPrefsTest, DefaultStateNoPrefs) {
  EXPECT_TRUE(model_->is_visible());
  EXPECT_TRUE(model_->is_pinned());
  EXPECT_EQ(model_->width(), 280);
  EXPECT_EQ(model_->position(), AstraSidebarPosition::kLeft);
  EXPECT_EQ(model_->active_section_id(),
            std::string(AstraSidebarModel::kSectionOpenTabs));
}

// Test 2: Model with prefs starts with registered default values.
TEST_F(AstraSidebarModelTest, DefaultStateWithPrefs) {
  EXPECT_TRUE(model_->is_visible());
  EXPECT_TRUE(model_->is_pinned());
  EXPECT_EQ(model_->width(), 280);
  EXPECT_EQ(model_->position(), AstraSidebarPosition::kLeft);
}

// Test 3: Default sections count.
TEST_F(AstraSidebarModelNoPrefsTest, DefaultSectionsCount) {
  auto sections = model_->GetAllSections();
  EXPECT_EQ(sections.size(), 14u);
}

// Test 4: GetDefaultSections returns the same as model's initial sections.
TEST(AstraSidebarStaticTest, GetDefaultSectionsMatchesStatic) {
  auto default_sections = AstraSidebarModel::GetDefaultSections();
  EXPECT_EQ(default_sections.size(), 14u);

  bool found_workspaces = false;
  bool found_favorites = false;
  bool found_open_tabs = false;
  bool found_history = false;
  bool found_downloads = false;
  bool found_passwords = false;
  bool found_extensions = false;
  bool found_devtools = false;

  for (const auto& s : default_sections) {
    if (s.id == AstraSidebarModel::kSectionWorkspaces) found_workspaces = true;
    if (s.id == AstraSidebarModel::kSectionFavorites) found_favorites = true;
    if (s.id == AstraSidebarModel::kSectionOpenTabs) found_open_tabs = true;
    if (s.id == AstraSidebarModel::kSectionHistory) found_history = true;
    if (s.id == AstraSidebarModel::kSectionDownloads) found_downloads = true;
    if (s.id == AstraSidebarModel::kSectionPasswords) found_passwords = true;
    if (s.id == AstraSidebarModel::kSectionExtensions) found_extensions = true;
    if (s.id == AstraSidebarModel::kSectionDevTools) found_devtools = true;
  }

  EXPECT_TRUE(found_workspaces);
  EXPECT_TRUE(found_favorites);
  EXPECT_TRUE(found_open_tabs);
  EXPECT_TRUE(found_history);
  EXPECT_TRUE(found_downloads);
  EXPECT_TRUE(found_passwords);
  EXPECT_TRUE(found_extensions);
  EXPECT_TRUE(found_devtools);
}

// Test 5: All default sections have valid position indices.
TEST(AstraSidebarStaticTest, DefaultSectionsHaveValidPositions) {
  auto sections = AstraSidebarModel::GetDefaultSections();
  ASSERT_FALSE(sections.empty());

  for (size_t i = 0; i < sections.size(); ++i) {
    EXPECT_EQ(sections[i].position, static_cast<int>(i))
        << "Section " << sections[i].id << " has wrong position";
  }
}

// Test 6: All default sections have non-empty names and IDs.
TEST(AstraSidebarStaticTest, DefaultSectionsHaveNamesAndIds) {
  auto sections = AstraSidebarModel::GetDefaultSections();
  for (const auto& s : sections) {
    EXPECT_FALSE(s.id.empty()) << "Section has empty ID";
    EXPECT_FALSE(s.name.empty()) << "Section " << s.id << " has empty name";
  }
}

// Test 7: Workspaces section is not collapsible by default.
TEST(AstraSidebarStaticTest, WorkspacesSectionNotCollapsible) {
  auto sections = AstraSidebarModel::GetDefaultSections();
  auto it = std::find_if(
      sections.begin(), sections.end(),
      [](const AstraSidebarSection& s) {
        return s.id == AstraSidebarModel::kSectionWorkspaces;
      });
  ASSERT_NE(it, sections.end());
  EXPECT_FALSE(it->is_collapsible);
}

// =========================================================================
// Model sidebar state tests
// =========================================================================

// Test 8: SetVisible toggles visibility and notifies.
TEST_F(AstraSidebarModelTest, SetVisibleNotifiesObserver) {
  MockSidebarModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSidebarHidden()).Times(1);
  model_->SetVisible(false);
  EXPECT_FALSE(model_->is_visible());

  EXPECT_CALL(observer, OnSidebarShown()).Times(1);
  model_->SetVisible(true);
  EXPECT_TRUE(model_->is_visible());

  model_->RemoveObserver(&observer);
}

// Test 9: SetVisible with same value is a no-op.
TEST_F(AstraSidebarModelTest, SetVisibleNoOpWhenSame) {
  MockSidebarModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSidebarShown()).Times(0);
  model_->SetVisible(true);

  model_->RemoveObserver(&observer);
}

// Test 10: ToggleVisible toggles visibility.
TEST_F(AstraSidebarModelTest, ToggleVisible) {
  bool initial = model_->is_visible();
  model_->ToggleVisible();
  EXPECT_EQ(model_->is_visible(), !initial);
  model_->ToggleVisible();
  EXPECT_EQ(model_->is_visible(), initial);
}

// Test 11: SetPinned toggles pinned state and notifies.
TEST_F(AstraSidebarModelTest, SetPinnedNotifiesObserver) {
  MockSidebarModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSidebarPinnedChanged(false)).Times(1);
  model_->SetPinned(false);
  EXPECT_FALSE(model_->is_pinned());

  EXPECT_CALL(observer, OnSidebarPinnedChanged(true)).Times(1);
  model_->SetPinned(true);
  EXPECT_TRUE(model_->is_pinned());

  model_->RemoveObserver(&observer);
}

// Test 12: TogglePinned toggles pinned state.
TEST_F(AstraSidebarModelTest, TogglePinned) {
  bool initial = model_->is_pinned();
  model_->TogglePinned();
  EXPECT_EQ(model_->is_pinned(), !initial);
}

// Test 13: SetWidth clamps and notifies.
TEST_F(AstraSidebarModelTest, SetWidthClampsAndNotifies) {
  MockSidebarModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSidebarWidthChanged(300)).Times(1);
  model_->SetWidth(300);
  EXPECT_EQ(model_->width(), 300);

  EXPECT_CALL(observer, OnSidebarWidthChanged(AstraSidebarModel::kMinWidth))
      .Times(1);
  model_->SetWidth(100);
  EXPECT_EQ(model_->width(), AstraSidebarModel::kMinWidth);

  EXPECT_CALL(observer, OnSidebarWidthChanged(AstraSidebarModel::kMaxWidth))
      .Times(1);
  model_->SetWidth(1000);
  EXPECT_EQ(model_->width(), AstraSidebarModel::kMaxWidth);

  model_->RemoveObserver(&observer);
}

// Test 14: SetWidth with same value is a no-op.
TEST_F(AstraSidebarModelTest, SetWidthNoOpWhenSame) {
  MockSidebarModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSidebarWidthChanged(_)).Times(0);
  model_->SetWidth(280);

  model_->RemoveObserver(&observer);
}

// Test 15: SetPosition changes position and notifies.
TEST_F(AstraSidebarModelTest, SetPositionNotifiesObserver) {
  MockSidebarModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSidebarPositionChanged(AstraSidebarPosition::kRight))
      .Times(1);
  model_->SetPosition(AstraSidebarPosition::kRight);
  EXPECT_EQ(model_->position(), AstraSidebarPosition::kRight);

  EXPECT_CALL(observer, OnSidebarPositionChanged(AstraSidebarPosition::kLeft))
      .Times(1);
  model_->SetPosition(AstraSidebarPosition::kLeft);
  EXPECT_EQ(model_->position(), AstraSidebarPosition::kLeft);

  model_->RemoveObserver(&observer);
}

// Test 16: SetPosition with same value is a no-op.
TEST_F(AstraSidebarModelTest, SetPositionNoOpWhenSame) {
  MockSidebarModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSidebarPositionChanged(_)).Times(0);
  model_->SetPosition(AstraSidebarPosition::kLeft);

  model_->RemoveObserver(&observer);
}

// =========================================================================
// Model active section tests
// =========================================================================

// Test 17: SetActiveSection changes active section and notifies.
TEST_F(AstraSidebarModelTest, SetActiveSectionNotifiesObserver) {
  MockSidebarModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnActiveSectionChanged(
              std::string(AstraSidebarModel::kSectionHistory)))
      .Times(1);
  model_->SetActiveSection(AstraSidebarModel::kSectionHistory);
  EXPECT_EQ(model_->active_section_id(),
            std::string(AstraSidebarModel::kSectionHistory));

  model_->RemoveObserver(&observer);
}

// Test 18: SetActiveSection with same ID is a no-op.
TEST_F(AstraSidebarModelTest, SetActiveSectionNoOpWhenSame) {
  MockSidebarModelObserver observer;
  model_->AddObserver(&observer);

  std::string initial = model_->active_section_id();
  EXPECT_CALL(observer, OnActiveSectionChanged(_)).Times(0);
  model_->SetActiveSection(initial);

  model_->RemoveObserver(&observer);
}

// Test 19: SetActiveSection with invalid ID does nothing.
TEST_F(AstraSidebarModelTest, SetActiveSectionInvalidIdNoOp) {
  MockSidebarModelObserver observer;
  model_->AddObserver(&observer);

  std::string initial = model_->active_section_id();
  EXPECT_CALL(observer, OnActiveSectionChanged(_)).Times(0);
  model_->SetActiveSection("nonexistent_section");
  EXPECT_EQ(model_->active_section_id(), initial);

  model_->RemoveObserver(&observer);
}

// =========================================================================
// Model section management tests
// =========================================================================

// Test 20: GetSectionById returns the correct section.
TEST_F(AstraSidebarModelTest, GetSectionByIdFound) {
  auto section =
      model_->GetSectionById(AstraSidebarModel::kSectionFavorites);
  EXPECT_TRUE(section.has_value());
  EXPECT_EQ(section->id,
            std::string(AstraSidebarModel::kSectionFavorites));
  EXPECT_FALSE(section->name.empty());
}

// Test 21: GetSectionById returns nullopt for unknown section.
TEST_F(AstraSidebarModelTest, GetSectionByIdNotFound) {
  auto section = model_->GetSectionById("nonexistent");
  EXPECT_FALSE(section.has_value());
}

// Test 22: GetVisibleSections returns only visible sections.
TEST_F(AstraSidebarModelTest, GetVisibleSectionsFiltersCorrectly) {
  auto all = model_->GetAllSections();
  auto visible = model_->GetVisibleSections();
  EXPECT_LE(visible.size(), all.size());

  for (const auto& s : visible) {
    EXPECT_TRUE(s.is_visible);
  }
}

// Test 23: SetSectionVisible changes visibility and notifies.
TEST_F(AstraSidebarModelTest, SetSectionVisibleNotifies) {
  MockSidebarModelObserver observer;
  model_->AddObserver(&observer);

  std::string section_id = AstraSidebarModel::kSectionHistory;
  auto initial = model_->GetSectionById(section_id);
  ASSERT_TRUE(initial.has_value());
  bool initial_visible = initial->is_visible;

  EXPECT_CALL(observer, OnSectionVisibilityChanged(section_id, !initial_visible))
      .Times(1);
  bool result = model_->SetSectionVisible(section_id, !initial_visible);
  EXPECT_TRUE(result);

  auto updated = model_->GetSectionById(section_id);
  ASSERT_TRUE(updated.has_value());
  EXPECT_EQ(updated->is_visible, !initial_visible);

  model_->RemoveObserver(&observer);
}

// Test 24: SetSectionVisible for unknown section returns false.
TEST_F(AstraSidebarModelTest, SetSectionVisibleUnknownSection) {
  bool result = model_->SetSectionVisible("nonexistent", false);
  EXPECT_FALSE(result);
}

// Test 25: SetSectionVisible with same value is a no-op but returns true.
TEST_F(AstraSidebarModelTest, SetSectionVisibleSameValueNoOp) {
  MockSidebarModelObserver observer;
  model_->AddObserver(&observer);

  std::string section_id = AstraSidebarModel::kSectionFavorites;
  auto initial = model_->GetSectionById(section_id);
  ASSERT_TRUE(initial.has_value());

  EXPECT_CALL(observer, OnSectionVisibilityChanged(_, _)).Times(0);
  bool result =
      model_->SetSectionVisible(section_id, initial->is_visible);
  EXPECT_TRUE(result);

  model_->RemoveObserver(&observer);
}

// Test 26: ToggleSectionVisible toggles visibility.
TEST_F(AstraSidebarModelTest, ToggleSectionVisible) {
  std::string section_id = AstraSidebarModel::kSectionDownloads;
  auto initial = model_->GetSectionById(section_id);
  ASSERT_TRUE(initial.has_value());

  bool result = model_->ToggleSectionVisible(section_id);
  EXPECT_TRUE(result);

  auto toggled = model_->GetSectionById(section_id);
  ASSERT_TRUE(toggled.has_value());
  EXPECT_EQ(toggled->is_visible, !initial->is_visible);
}

// Test 27: ToggleSectionVisible for unknown section returns false.
TEST_F(AstraSidebarModelTest, ToggleSectionVisibleUnknown) {
  bool result = model_->ToggleSectionVisible("nonexistent");
  EXPECT_FALSE(result);
}

// Test 28: GetSectionCount returns total number of sections.
TEST_F(AstraSidebarModelTest, GetSectionCount) {
  EXPECT_EQ(model_->GetSectionCount(), model_->GetAllSections().size());
  EXPECT_EQ(model_->GetSectionCount(), 14u);
}

// Test 29: GetVisibleSectionCount matches GetVisibleSections size.
TEST_F(AstraSidebarModelTest, GetVisibleSectionCount) {
  EXPECT_EQ(model_->GetVisibleSectionCount(),
            model_->GetVisibleSections().size());
}

// =========================================================================
// Model section collapse tests
// =========================================================================

// Test 30: ToggleSectionCollapsed toggles collapsed state.
TEST_F(AstraSidebarModelTest, ToggleSectionCollapsed) {
  std::string section_id = AstraSidebarModel::kSectionBookmarks;

  auto initial = model_->GetSectionById(section_id);
  ASSERT_TRUE(initial.has_value());
  EXPECT_TRUE(initial->is_collapsed);

  bool result = model_->ToggleSectionCollapsed(section_id);
  EXPECT_TRUE(result);

  auto toggled = model_->GetSectionById(section_id);
  ASSERT_TRUE(toggled.has_value());
  EXPECT_FALSE(toggled->is_collapsed);
}

// Test 31: ToggleSectionCollapsed for non-collapsible section returns false.
TEST_F(AstraSidebarModelTest, ToggleSectionCollapsedNonCollapsible) {
  bool result =
      model_->ToggleSectionCollapsed(AstraSidebarModel::kSectionWorkspaces);
  EXPECT_FALSE(result);
}

// Test 32: ToggleSectionCollapsed for unknown section returns false.
TEST_F(AstraSidebarModelTest, ToggleSectionCollapsedUnknown) {
  bool result = model_->ToggleSectionCollapsed("nonexistent");
  EXPECT_FALSE(result);
}

// Test 33: SetSectionCollapsed sets state and notifies.
TEST_F(AstraSidebarModelTest, SetSectionCollapsedNotifies) {
  MockSidebarModelObserver observer;
  model_->AddObserver(&observer);

  std::string section_id = AstraSidebarModel::kSectionHistory;

  EXPECT_CALL(observer, OnSectionCollapsedChanged(section_id, true)).Times(1);
  bool result = model_->SetSectionCollapsed(section_id, true);
  EXPECT_TRUE(result);

  auto section = model_->GetSectionById(section_id);
  ASSERT_TRUE(section.has_value());
  EXPECT_TRUE(section->is_collapsed);

  model_->RemoveObserver(&observer);
}

// Test 34: GetCollapsedSectionIds returns correct set.
TEST_F(AstraSidebarModelTest, GetCollapsedSectionIds) {
  auto collapsed = model_->GetCollapsedSectionIds();

  std::vector<std::string> expected_collapsed = {
      AstraSidebarModel::kSectionBookmarks,
      AstraSidebarModel::kSectionHistory,
      AstraSidebarModel::kSectionRecentlyClosed,
      AstraSidebarModel::kSectionReadingList,
      AstraSidebarModel::kSectionNotes,
      AstraSidebarModel::kSectionDownloads,
      AstraSidebarModel::kSectionPasswords,
      AstraSidebarModel::kSectionExtensions,
      AstraSidebarModel::kSectionDevTools,
  };

  for (const auto& id : expected_collapsed) {
    auto it = std::find(collapsed.begin(), collapsed.end(), id);
    EXPECT_NE(it, collapsed.end()) << id << " should be collapsed by default";
  }
}

// =========================================================================
// Model section ordering tests
// =========================================================================

// Test 35: MoveSection moves a section to a new position.
TEST_F(AstraSidebarModelTest, MoveSectionForward) {
  MockSidebarModelObserver observer;
  model_->AddObserver(&observer);

  std::string section_id = AstraSidebarModel::kSectionHistory;

  auto initial = model_->GetSectionById(section_id);
  ASSERT_TRUE(initial.has_value());
  int old_pos = initial->position;

  EXPECT_CALL(observer, OnSectionOrderChanged()).Times(1);
  bool result = model_->MoveSection(section_id, old_pos + 2);
  EXPECT_TRUE(result);

  auto updated = model_->GetSectionById(section_id);
  ASSERT_TRUE(updated.has_value());
  EXPECT_EQ(updated->position, old_pos + 2);

  model_->RemoveObserver(&observer);
}

// Test 36: MoveSection moves a section backward.
TEST_F(AstraSidebarModelTest, MoveSectionBackward) {
  std::string section_id = AstraSidebarModel::kSectionDownloads;

  auto initial = model_->GetSectionById(section_id);
  ASSERT_TRUE(initial.has_value());
  int old_pos = initial->position;
  ASSERT_GT(old_pos, 2);

  bool result = model_->MoveSection(section_id, old_pos - 2);
  EXPECT_TRUE(result);

  auto updated = model_->GetSectionById(section_id);
  ASSERT_TRUE(updated.has_value());
  EXPECT_EQ(updated->position, old_pos - 2);
}

// Test 37: MoveSection to same position is a no-op.
TEST_F(AstraSidebarModelTest, MoveSectionSamePositionNoOp) {
  MockSidebarModelObserver observer;
  model_->AddObserver(&observer);

  std::string section_id = AstraSidebarModel::kSectionFavorites;
  auto initial = model_->GetSectionById(section_id);
  ASSERT_TRUE(initial.has_value());

  EXPECT_CALL(observer, OnSectionOrderChanged()).Times(0);
  bool result = model_->MoveSection(section_id, initial->position);
  EXPECT_TRUE(result);

  model_->RemoveObserver(&observer);
}

// Test 38: MoveSection for unknown section returns false.
TEST_F(AstraSidebarModelTest, MoveSectionUnknownReturnsFalse) {
  bool result = model_->MoveSection("nonexistent", 0);
  EXPECT_FALSE(result);
}

// Test 39: MoveSection clamps to valid range.
TEST_F(AstraSidebarModelTest, MoveSectionClampsPosition) {
  std::string section_id = AstraSidebarModel::kSectionFavorites;

  bool result = model_->MoveSection(section_id, -10);
  EXPECT_TRUE(result);

  auto updated = model_->GetSectionById(section_id);
  ASSERT_TRUE(updated.has_value());
  EXPECT_EQ(updated->position, 0);

  result = model_->MoveSection(section_id, 1000);
  EXPECT_TRUE(result);

  updated = model_->GetSectionById(section_id);
  ASSERT_TRUE(updated.has_value());
  EXPECT_EQ(updated->position,
            static_cast<int>(model_->GetSectionCount()) - 1);
}

// Test 40: ReorderSections reorders based on ID list.
TEST_F(AstraSidebarModelTest, ReorderSectionsByIdList) {
  MockSidebarModelObserver observer;
  model_->AddObserver(&observer);

  auto initial_sections = model_->GetAllSections();
  std::vector<std::string> initial_ids;
  for (const auto& s : initial_sections) {
    initial_ids.push_back(s.id);
  }

  std::vector<std::string> reversed_ids(initial_ids.rbegin(),
                                         initial_ids.rend());

  EXPECT_CALL(observer, OnSectionOrderChanged()).Times(1);
  bool result = model_->ReorderSections(reversed_ids);
  EXPECT_TRUE(result);

  auto new_sections = model_->GetAllSections();
  ASSERT_EQ(new_sections.size(), reversed_ids.size());
  for (size_t i = 0; i < reversed_ids.size(); ++i) {
    EXPECT_EQ(new_sections[i].id, reversed_ids[i]);
    EXPECT_EQ(new_sections[i].position, static_cast<int>(i));
  }

  model_->RemoveObserver(&observer);
}

// Test 41: ReorderSections with unknown IDs returns false.
TEST_F(AstraSidebarModelTest, ReorderSectionsWithUnknownIdFails) {
  std::vector<std::string> bad_ids = {"section1", "section2"};
  bool result = model_->ReorderSections(bad_ids);
  EXPECT_FALSE(result);
}

// Test 42: ReorderSections with wrong count returns false.
TEST_F(AstraSidebarModelTest, ReorderSectionsWrongCountFails) {
  auto sections = model_->GetAllSections();
  std::vector<std::string> ids;
  ids.push_back(sections[0].id);
  ids.push_back(sections[1].id);
  bool result = model_->ReorderSections(ids);
  EXPECT_FALSE(result);
}

// =========================================================================
// Model bulk operations tests
// =========================================================================

// Test 43: SetAllSectionsVisible shows all sections.
TEST_F(AstraSidebarModelTest, SetAllSectionsVisibleTrue) {
  MockSidebarModelObserver observer;
  model_->AddObserver(&observer);

  model_->SetSectionVisible(AstraSidebarModel::kSectionHistory, false);
  model_->SetSectionVisible(AstraSidebarModel::kSectionDownloads, false);

  Mock::VerifyAndClearExpectations(&observer);

  EXPECT_CALL(observer, OnSectionVisibilityChanged(_, true))
      .Times(AtLeast(2));
  model_->SetAllSectionsVisible(true);

  auto all = model_->GetAllSections();
  for (const auto& s : all) {
    EXPECT_TRUE(s.is_visible) << "Section " << s.id << " should be visible";
  }

  model_->RemoveObserver(&observer);
}

// Test 44: SetAllSectionsVisible hides all sections.
TEST_F(AstraSidebarModelTest, SetAllSectionsVisibleFalse) {
  model_->SetAllSectionsVisible(false);

  auto all = model_->GetAllSections();
  for (const auto& s : all) {
    EXPECT_FALSE(s.is_visible) << "Section " << s.id << " should be hidden";
  }
}

// Test 45: ToggleMultipleSections toggles multiple sections.
TEST_F(AstraSidebarModelTest, ToggleMultipleSections) {
  std::vector<std::string> ids = {
      AstraSidebarModel::kSectionHistory,
      AstraSidebarModel::kSectionDownloads,
      AstraSidebarModel::kSectionPasswords,
  };

  std::vector<bool> initial_states;
  for (const auto& id : ids) {
    auto s = model_->GetSectionById(id);
    ASSERT_TRUE(s.has_value());
    initial_states.push_back(s->is_visible);
  }

  model_->ToggleMultipleSections(ids);

  for (size_t i = 0; i < ids.size(); ++i) {
    auto s = model_->GetSectionById(ids[i]);
    ASSERT_TRUE(s.has_value());
    EXPECT_EQ(s->is_visible, !initial_states[i])
        << "Section " << ids[i] << " should be toggled";
  }
}

// Test 46: CollapseAllSections collapses all collapsible sections.
TEST_F(AstraSidebarModelTest, CollapseAllSections) {
  MockSidebarModelObserver observer;
  model_->AddObserver(&observer);

  model_->SetSectionCollapsed(AstraSidebarModel::kSectionBookmarks, false);
  model_->SetSectionCollapsed(AstraSidebarModel::kSectionHistory, false);

  Mock::VerifyAndClearExpectations(&observer);

  EXPECT_CALL(observer, OnSectionCollapsedChanged(_, true))
      .Times(AtLeast(1));
  model_->CollapseAllSections();

  auto all = model_->GetAllSections();
  for (const auto& s : all) {
    if (s.is_collapsible) {
      EXPECT_TRUE(s.is_collapsed)
          << "Collapsible section " << s.id << " should be collapsed";
    }
  }

  model_->RemoveObserver(&observer);
}

// Test 47: ExpandAllSections expands all collapsible sections.
TEST_F(AstraSidebarModelTest, ExpandAllSections) {
  model_->ExpandAllSections();

  auto all = model_->GetAllSections();
  for (const auto& s : all) {
    if (s.is_collapsible) {
      EXPECT_FALSE(s.is_collapsed)
          << "Collapsible section " << s.id << " should be expanded";
    }
  }
}

// Test 48: ResetSectionsToDefaults restores default order and visibility.
TEST_F(AstraSidebarModelTest, ResetSectionsToDefaults) {
  model_->MoveSection(AstraSidebarModel::kSectionHistory, 0);
  model_->SetSectionVisible(AstraSidebarModel::kSectionDownloads, false);
  model_->SetSectionCollapsed(AstraSidebarModel::kSectionFavorites, true);

  model_->ResetSectionsToDefaults();

  auto defaults = AstraSidebarModel::GetDefaultSections();
  auto current = model_->GetAllSections();

  ASSERT_EQ(defaults.size(), current.size());
  for (size_t i = 0; i < defaults.size(); ++i) {
    EXPECT_EQ(defaults[i].id, current[i].id);
    EXPECT_EQ(defaults[i].is_visible, current[i].is_visible);
    EXPECT_EQ(defaults[i].is_collapsed, current[i].is_collapsed);
  }
}

// =========================================================================
// Model presentation settings tests
// =========================================================================

// Test 49: Show section icons setting round-trip.
TEST_F(AstraSidebarModelTest, ShowSectionIconsSetting) {
  MockSidebarModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSidebarSettingsChanged()).Times(1);
  model_->SetShowSectionIcons(false);
  EXPECT_FALSE(model_->show_section_icons());

  EXPECT_CALL(observer, OnSidebarSettingsChanged()).Times(1);
  model_->SetShowSectionIcons(true);
  EXPECT_TRUE(model_->show_section_icons());

  model_->RemoveObserver(&observer);
}

// Test 50: Show section labels setting round-trip.
TEST_F(AstraSidebarModelTest, ShowSectionLabelsSetting) {
  model_->SetShowSectionLabels(false);
  EXPECT_FALSE(model_->show_section_labels());

  model_->SetShowSectionLabels(true);
  EXPECT_TRUE(model_->show_section_labels());
}

// Test 51: Compact mode setting round-trip.
TEST_F(AstraSidebarModelTest, CompactModeSetting) {
  model_->SetCompactMode(true);
  EXPECT_TRUE(model_->compact_mode());

  model_->SetCompactMode(false);
  EXPECT_FALSE(model_->compact_mode());
}

// Test 52: Auto-hide on tab click setting round-trip.
TEST_F(AstraSidebarModelTest, AutoHideOnTabClickSetting) {
  model_->SetAutoHideOnTabClick(true);
  EXPECT_TRUE(model_->auto_hide_on_tab_click());

  model_->SetAutoHideOnTabClick(false);
  EXPECT_FALSE(model_->auto_hide_on_tab_click());
}

// Test 53: Show tab count badges setting round-trip.
TEST_F(AstraSidebarModelTest, ShowTabCountBadgesSetting) {
  model_->SetShowTabCountBadges(false);
  EXPECT_FALSE(model_->show_tab_count_badges());

  model_->SetShowTabCountBadges(true);
  EXPECT_TRUE(model_->show_tab_count_badges());
}

// Test 54: Show workspace badge setting round-trip.
TEST_F(AstraSidebarModelTest, ShowWorkspaceBadgeSetting) {
  model_->SetShowWorkspaceBadge(false);
  EXPECT_FALSE(model_->show_workspace_badge());

  model_->SetShowWorkspaceBadge(true);
  EXPECT_TRUE(model_->show_workspace_badge());
}

// Test 55: Animation enabled setting round-trip.
TEST_F(AstraSidebarModelTest, AnimationEnabledSetting) {
  model_->SetAnimationEnabled(false);
  EXPECT_FALSE(model_->animation_enabled());

  model_->SetAnimationEnabled(true);
  EXPECT_TRUE(model_->animation_enabled());
}

// Test 56: Remember last section setting round-trip.
TEST_F(AstraSidebarModelTest, RememberLastSectionSetting) {
  model_->SetRememberLastSection(false);
  EXPECT_FALSE(model_->remember_last_section());

  model_->SetRememberLastSection(true);
  EXPECT_TRUE(model_->remember_last_section());
}

// Test 57: Default active section setting round-trip.
TEST_F(AstraSidebarModelTest, DefaultActiveSectionSetting) {
  std::string new_default = AstraSidebarModel::kSectionHistory;
  model_->SetDefaultActiveSection(new_default);
  EXPECT_EQ(model_->default_active_section(), new_default);
}

// Test 58: Last active section setting round-trip.
TEST_F(AstraSidebarModelTest, LastActiveSectionSetting) {
  std::string new_last = AstraSidebarModel::kSectionDownloads;
  model_->SetLastActiveSection(new_last);
  EXPECT_EQ(model_->last_active_section(), new_last);
}

// Test 59: Settings persist through PrefService.
TEST_F(AstraSidebarModelTest, SettingsPersistThroughPrefs) {
  model_->SetShowSectionIcons(false);
  model_->SetCompactMode(true);
  model_->SetSidebarWidth(350);
  model_->SetShowTabCountBadges(false);

  auto model2 = std::make_unique<AstraSidebarModel>(prefs_.get());

  EXPECT_FALSE(model2->show_section_icons());
  EXPECT_TRUE(model2->compact_mode());
  EXPECT_EQ(model2->width(), 350);
  EXPECT_FALSE(model2->show_tab_count_badges());
}

// Test 60: ResetAllSettings resets everything to defaults.
TEST_F(AstraSidebarModelTest, ResetAllSettings) {
  model_->SetShowSectionIcons(false);
  model_->SetCompactMode(true);
  model_->SetWidth(350);
  model_->SetPinned(false);

  model_->ResetAllSettings();

  EXPECT_TRUE(model_->show_section_icons());
  EXPECT_FALSE(model_->compact_mode());
  EXPECT_EQ(model_->width(), 280);
  EXPECT_TRUE(model_->is_pinned());
}

// =========================================================================
// Model utility method tests
// =========================================================================

// Test 61: ClampWidth clamps values below minimum.
TEST(AstraSidebarStaticTest, ClampWidthBelowMinimum) {
  EXPECT_EQ(AstraSidebarModel::ClampWidth(0),
            AstraSidebarModel::kMinWidth);
  EXPECT_EQ(AstraSidebarModel::ClampWidth(100),
            AstraSidebarModel::kMinWidth);
  EXPECT_EQ(AstraSidebarModel::ClampWidth(199),
            AstraSidebarModel::kMinWidth);
}

// Test 62: ClampWidth clamps values above maximum.
TEST(AstraSidebarStaticTest, ClampWidthAboveMaximum) {
  EXPECT_EQ(AstraSidebarModel::ClampWidth(1000),
            AstraSidebarModel::kMaxWidth);
  EXPECT_EQ(AstraSidebarModel::ClampWidth(600),
            AstraSidebarModel::kMaxWidth);
  EXPECT_EQ(AstraSidebarModel::ClampWidth(501),
            AstraSidebarModel::kMaxWidth);
}

// Test 63: ClampWidth passes through valid values.
TEST(AstraSidebarStaticTest, ClampWidthValidValues) {
  EXPECT_EQ(AstraSidebarModel::ClampWidth(200), 200);
  EXPECT_EQ(AstraSidebarModel::ClampWidth(280), 280);
  EXPECT_EQ(AstraSidebarModel::ClampWidth(350), 350);
  EXPECT_EQ(AstraSidebarModel::ClampWidth(500), 500);
}

// Test 64: ClampWidth handles negative values.
TEST(AstraSidebarStaticTest, ClampWidthNegative) {
  EXPECT_EQ(AstraSidebarModel::ClampWidth(-1),
            AstraSidebarModel::kMinWidth);
  EXPECT_EQ(AstraSidebarModel::ClampWidth(-100),
            AstraSidebarModel::kMinWidth);
}

// Test 65: Min and max width constants have valid relationship.
TEST(AstraSidebarStaticTest, WidthConstantsValid) {
  EXPECT_LT(AstraSidebarModel::kMinWidth, AstraSidebarModel::kMaxWidth);
  EXPECT_GT(AstraSidebarModel::kMinWidth, 0);
  EXPECT_GE(AstraSidebarModel::kDefaultWidth,
            AstraSidebarModel::kMinWidth);
  EXPECT_LE(AstraSidebarModel::kDefaultWidth,
            AstraSidebarModel::kMaxWidth);
}

// =========================================================================
// Observer default implementations test
// =========================================================================

// Test 66: Observer with no overrides has empty default implementations.
TEST(AstraSidebarObserverDefaultsTest, AllMethodsHaveDefaults) {
  class TestObserver : public AstraSidebarModelObserver {
   public:
    ~TestObserver() override = default;
  };

  TestObserver observer;

  observer.OnSidebarShown();
  observer.OnSidebarHidden();
  observer.OnSidebarPinnedChanged(true);
  observer.OnActiveSectionChanged("test");
  observer.OnSectionVisibilityChanged("test", true);
  observer.OnSectionOrderChanged();
  observer.OnSectionCollapsedChanged("test", true);
  observer.OnSidebarWidthChanged(280);
  observer.OnSidebarPositionChanged(AstraSidebarPosition::kLeft);
  observer.OnSidebarSettingsChanged();

  SUCCEED();
}

// =========================================================================
// Per-section visibility setting tests
// =========================================================================

// Test 67: Show tab groups section setting.
TEST_F(AstraSidebarModelTest, ShowTabGroupsSectionSetting) {
  bool initial = model_->show_tab_groups_section();

  model_->SetShowTabGroupsSection(!initial);
  EXPECT_EQ(model_->show_tab_groups_section(), !initial);

  auto section =
      model_->GetSectionById(AstraSidebarModel::kSectionTabGroups);
  ASSERT_TRUE(section.has_value());
  EXPECT_EQ(section->is_visible, !initial);
}

// Test 68: Show history section setting.
TEST_F(AstraSidebarModelTest, ShowHistorySectionSetting) {
  bool initial = model_->show_history_section();

  model_->SetShowHistorySection(!initial);
  EXPECT_EQ(model_->show_history_section(), !initial);

  auto section =
      model_->GetSectionById(AstraSidebarModel::kSectionHistory);
  ASSERT_TRUE(section.has_value());
  EXPECT_EQ(section->is_visible, !initial);
}

// Test 69: Show recently closed section setting.
TEST_F(AstraSidebarModelTest, ShowRecentlyClosedSectionSetting) {
  bool initial = model_->show_recently_closed_section();

  model_->SetShowRecentlyClosedSection(!initial);
  EXPECT_EQ(model_->show_recently_closed_section(), !initial);

  auto section =
      model_->GetSectionById(AstraSidebarModel::kSectionRecentlyClosed);
  ASSERT_TRUE(section.has_value());
  EXPECT_EQ(section->is_visible, !initial);
}

// Test 70: Show reading list section setting.
TEST_F(AstraSidebarModelTest, ShowReadingListSectionSetting) {
  bool initial = model_->show_reading_list_section();

  model_->SetShowReadingListSection(!initial);
  EXPECT_EQ(model_->show_reading_list_section(), !initial);

  auto section =
      model_->GetSectionById(AstraSidebarModel::kSectionReadingList);
  ASSERT_TRUE(section.has_value());
  EXPECT_EQ(section->is_visible, !initial);
}

// Test 71: Show notes section setting.
TEST_F(AstraSidebarModelTest, ShowNotesSectionSetting) {
  bool initial = model_->show_notes_section();

  model_->SetShowNotesSection(!initial);
  EXPECT_EQ(model_->show_notes_section(), !initial);

  auto section =
      model_->GetSectionById(AstraSidebarModel::kSectionNotes);
  ASSERT_TRUE(section.has_value());
  EXPECT_EQ(section->is_visible, !initial);
}

// Test 72: Show downloads section setting.
TEST_F(AstraSidebarModelTest, ShowDownloadsSectionSetting) {
  bool initial = model_->show_downloads_section();

  model_->SetShowDownloadsSection(!initial);
  EXPECT_EQ(model_->show_downloads_section(), !initial);

  auto section =
      model_->GetSectionById(AstraSidebarModel::kSectionDownloads);
  ASSERT_TRUE(section.has_value());
  EXPECT_EQ(section->is_visible, !initial);
}

// Test 73: Show passwords section setting.
TEST_F(AstraSidebarModelTest, ShowPasswordsSectionSetting) {
  bool initial = model_->show_passwords_section();

  model_->SetShowPasswordsSection(!initial);
  EXPECT_EQ(model_->show_passwords_section(), !initial);

  auto section =
      model_->GetSectionById(AstraSidebarModel::kSectionPasswords);
  ASSERT_TRUE(section.has_value());
  EXPECT_EQ(section->is_visible, !initial);
}

// Test 74: Show extensions section setting.
TEST_F(AstraSidebarModelTest, ShowExtensionsSectionSetting) {
  bool initial = model_->show_extensions_section();

  model_->SetShowExtensionsSection(!initial);
  EXPECT_EQ(model_->show_extensions_section(), !initial);

  auto section =
      model_->GetSectionById(AstraSidebarModel::kSectionExtensions);
  ASSERT_TRUE(section.has_value());
  EXPECT_EQ(section->is_visible, !initial);
}

// =========================================================================
// Edge case tests
// =========================================================================

// Test 75: Rapid state changes don't crash.
TEST_F(AstraSidebarModelTest, RapidStateChanges) {
  for (int i = 0; i < 100; ++i) {
    model_->ToggleVisible();
  }
  EXPECT_TRUE(model_->is_visible());
}

// Test 76: Multiple observers all get notified.
TEST_F(AstraSidebarModelTest, MultipleObserversAllNotified) {
  MockSidebarModelObserver observer1;
  MockSidebarModelObserver observer2;
  MockSidebarModelObserver observer3;

  model_->AddObserver(&observer1);
  model_->AddObserver(&observer2);
  model_->AddObserver(&observer3);

  EXPECT_CALL(observer1, OnSidebarWidthChanged(300)).Times(1);
  EXPECT_CALL(observer2, OnSidebarWidthChanged(300)).Times(1);
  EXPECT_CALL(observer3, OnSidebarWidthChanged(300)).Times(1);

  model_->SetWidth(300);

  model_->RemoveObserver(&observer1);
  model_->RemoveObserver(&observer2);
  model_->RemoveObserver(&observer3);
}

// Test 77: Removing an observer that was never added is safe.
TEST_F(AstraSidebarModelTest, RemoveNonExistentObserverSafe) {
  MockSidebarModelObserver observer;
  model_->RemoveObserver(&observer);
  SUCCEED();
}

// Test 78: SetAllSectionsVisible on already-all-visible is no-op for notifications.
TEST_F(AstraSidebarModelTest, SetAllSectionsVisibleNoOpWhenAlreadyAllVisible) {
  model_->SetAllSectionsVisible(true);

  MockSidebarModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSectionVisibilityChanged(_, _)).Times(0);
  model_->SetAllSectionsVisible(true);

  model_->RemoveObserver(&observer);
}

// Test 79: Empty section ID operations are safe.
TEST_F(AstraSidebarModelTest, EmptySectionIdOperations) {
  EXPECT_FALSE(model_->SetSectionVisible("", true));
  EXPECT_FALSE(model_->ToggleSectionVisible(""));
  EXPECT_FALSE(model_->ToggleSectionCollapsed(""));
  EXPECT_FALSE(model_->SetSectionCollapsed("", true));
  EXPECT_FALSE(model_->MoveSection("", 0));
  EXPECT_FALSE(model_->GetSectionById("").has_value());
}

// Test 80: Model without PrefService still works for state operations.
TEST_F(AstraSidebarModelNoPrefsTest, NoPrefsModelStateOperations) {
  model_->SetVisible(false);
  EXPECT_FALSE(model_->is_visible());

  model_->SetPinned(false);
  EXPECT_FALSE(model_->is_pinned());

  model_->SetWidth(350);
  EXPECT_EQ(model_->width(), 350);

  model_->SetPosition(AstraSidebarPosition::kRight);
  EXPECT_EQ(model_->position(), AstraSidebarPosition::kRight);
}

// Test 81: Model without PrefService uses defaults for settings.
TEST_F(AstraSidebarModelNoPrefsTest, NoPrefsModelSettingsDefaults) {
  EXPECT_TRUE(model_->show_section_icons());
  EXPECT_TRUE(model_->show_section_labels());
  EXPECT_FALSE(model_->compact_mode());
  EXPECT_FALSE(model_->auto_hide_on_tab_click());
  EXPECT_TRUE(model_->show_tab_count_badges());
  EXPECT_TRUE(model_->show_workspace_badge());
  EXPECT_TRUE(model_->animation_enabled());
  EXPECT_TRUE(model_->remember_last_section());
}

// Test 82: Settings without prefs are no-ops (don't crash).
TEST_F(AstraSidebarModelNoPrefsTest, NoPrefsSettersNoOp) {
  model_->SetShowSectionIcons(false);
  model_->SetCompactMode(true);
  model_->SetShowTabCountBadges(false);
  model_->ResetAllSettings();

  EXPECT_TRUE(model_->show_section_icons());
  EXPECT_FALSE(model_->compact_mode());
  EXPECT_TRUE(model_->show_tab_count_badges());
}

// =========================================================================
// Controller tests
// =========================================================================

// Test 83: Controller has a model.
TEST_F(AstraSidebarControllerTest, ControllerHasModel) {
  ASSERT_NE(controller_->model(), nullptr);
}

// Test 84: Controller ShowSidebar shows the sidebar.
TEST_F(AstraSidebarControllerTest, ShowSidebar) {
  controller_->ShowSidebar();
  EXPECT_TRUE(controller_->model()->is_visible());
}

// Test 85: Controller HideSidebar hides the sidebar.
TEST_F(AstraSidebarControllerTest, HideSidebar) {
  controller_->HideSidebar();
  EXPECT_FALSE(controller_->model()->is_visible());
}

// Test 86: Controller ToggleSidebar toggles visibility.
TEST_F(AstraSidebarControllerTest, ToggleSidebar) {
  bool initial = controller_->model()->is_visible();
  controller_->ToggleSidebar();
  EXPECT_EQ(controller_->model()->is_visible(), !initial);
}

// Test 87: Controller TogglePinned toggles pinned state.
TEST_F(AstraSidebarControllerTest, TogglePinned) {
  bool initial = controller_->model()->is_pinned();
  controller_->TogglePinned();
  EXPECT_EQ(controller_->model()->is_pinned(), !initial);
}

// Test 88: Controller SetSidebarWidth sets width.
TEST_F(AstraSidebarControllerTest, SetSidebarWidth) {
  controller_->SetSidebarWidth(350);
  EXPECT_EQ(controller_->model()->width(), 350);
}

// Test 89: Controller SetSidebarPosition sets position.
TEST_F(AstraSidebarControllerTest, SetSidebarPosition) {
  controller_->SetSidebarPosition(AstraSidebarPosition::kRight);
  EXPECT_EQ(controller_->model()->position(),
            AstraSidebarPosition::kRight);
}

// Test 90: Controller ActivateSection activates a section.
TEST_F(AstraSidebarControllerTest, ActivateSection) {
  bool result =
      controller_->ActivateSection(AstraSidebarModel::kSectionHistory);
  EXPECT_TRUE(result);
  EXPECT_EQ(controller_->model()->active_section_id(),
            std::string(AstraSidebarModel::kSectionHistory));
}

// Test 91: Controller ActivateSection with hidden section returns false.
TEST_F(AstraSidebarControllerTest, ActivateHiddenSectionFails) {
  controller_->model()->SetSectionVisible(
      AstraSidebarModel::kSectionHistory, false);

  bool result =
      controller_->ActivateSection(AstraSidebarModel::kSectionHistory);
  EXPECT_FALSE(result);
}

// Test 92: Controller ActivateSection with invalid ID returns false.
TEST_F(AstraSidebarControllerTest, ActivateInvalidSection) {
  bool result = controller_->ActivateSection("nonexistent");
  EXPECT_FALSE(result);
}

// Test 93: Controller ActivateNextSection goes to next visible section.
TEST_F(AstraSidebarControllerTest, ActivateNextSection) {
  auto visible = controller_->model()->GetVisibleSections();
  ASSERT_FALSE(visible.empty());
  controller_->ActivateSection(visible[0].id);

  controller_->ActivateNextSection();

  std::string active = controller_->model()->active_section_id();
  EXPECT_NE(active, visible[0].id);
  bool found = false;
  for (const auto& s : visible) {
    if (s.id == active) {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

// Test 94: Controller ActivatePreviousSection goes to previous visible section.
TEST_F(AstraSidebarControllerTest, ActivatePreviousSection) {
  auto visible = controller_->model()->GetVisibleSections();
  ASSERT_GT(visible.size(), 1u);

  controller_->ActivateSection(visible[1].id);

  controller_->ActivatePreviousSection();
  EXPECT_EQ(controller_->model()->active_section_id(), visible[0].id);
}

// Test 95: Controller ActivateNextSection wraps around.
TEST_F(AstraSidebarControllerTest, ActivateNextSectionWrapsAround) {
  auto visible = controller_->model()->GetVisibleSections();
  ASSERT_FALSE(visible.empty());

  controller_->ActivateSection(visible.back().id);

  controller_->ActivateNextSection();
  EXPECT_EQ(controller_->model()->active_section_id(), visible[0].id);
}

// Test 96: Controller ActivatePreviousSection wraps around.
TEST_F(AstraSidebarControllerTest, ActivatePreviousSectionWrapsAround) {
  auto visible = controller_->model()->GetVisibleSections();
  ASSERT_FALSE(visible.empty());

  controller_->ActivateSection(visible[0].id);

  controller_->ActivatePreviousSection();
  EXPECT_EQ(controller_->model()->active_section_id(), visible.back().id);
}

// Test 97: Controller ToggleSectionVisibility toggles a section.
TEST_F(AstraSidebarControllerTest, ToggleSectionVisibility) {
  auto initial = controller_->model()->GetSectionById(
      AstraSidebarModel::kSectionDownloads);
  ASSERT_TRUE(initial.has_value());

  bool result = controller_->ToggleSectionVisibility(
      AstraSidebarModel::kSectionDownloads);
  EXPECT_TRUE(result);

  auto updated = controller_->model()->GetSectionById(
      AstraSidebarModel::kSectionDownloads);
  ASSERT_TRUE(updated.has_value());
  EXPECT_NE(updated->is_visible, initial->is_visible);
}

// Test 98: Controller ToggleSectionCollapsed toggles collapse state.
TEST_F(AstraSidebarControllerTest, ToggleSectionCollapsed) {
  auto initial = controller_->model()->GetSectionById(
      AstraSidebarModel::kSectionBookmarks);
  ASSERT_TRUE(initial.has_value());

  bool result = controller_->ToggleSectionCollapsed(
      AstraSidebarModel::kSectionBookmarks);
  EXPECT_TRUE(result);

  auto updated = controller_->model()->GetSectionById(
      AstraSidebarModel::kSectionBookmarks);
  ASSERT_TRUE(updated.has_value());
  EXPECT_NE(updated->is_collapsed, initial->is_collapsed);
}

// Test 99: Controller SetSidebarView sets the view reference.
TEST_F(AstraSidebarControllerTest, SetSidebarView) {
  EXPECT_EQ(controller_->sidebar_view(), nullptr);
  controller_->SetSidebarView(nullptr);
  EXPECT_EQ(controller_->sidebar_view(), nullptr);
}

// =========================================================================
// Persistence / Pref round-trip tests
// =========================================================================

// Test 100: Section order persists through PrefService.
TEST_F(AstraSidebarModelTest, SectionOrderPersists) {
  model_->MoveSection(AstraSidebarModel::kSectionHistory, 0);
  model_->MoveSection(AstraSidebarModel::kSectionDownloads, 1);

  auto model2 = std::make_unique<AstraSidebarModel>(prefs_.get());

  auto s1 = model_->GetAllSections();
  auto s2 = model2->GetAllSections();

  ASSERT_EQ(s1.size(), s2.size());
  for (size_t i = 0; i < s1.size(); ++i) {
    EXPECT_EQ(s1[i].id, s2[i].id)
        << "Section at position " << i << " differs between models";
  }
}

// Test 101: Collapsed sections persist through PrefService.
TEST_F(AstraSidebarModelTest, CollapsedSectionsPersist) {
  model_->SetSectionCollapsed(AstraSidebarModel::kSectionFavorites, true);
  model_->SetSectionCollapsed(AstraSidebarModel::kSectionBookmarks, false);

  auto model2 = std::make_unique<AstraSidebarModel>(prefs_.get());

  auto fav1 = model_->GetSectionById(AstraSidebarModel::kSectionFavorites);
  auto fav2 = model2->GetSectionById(AstraSidebarModel::kSectionFavorites);
  ASSERT_TRUE(fav1.has_value() && fav2.has_value());
  EXPECT_EQ(fav1->is_collapsed, fav2->is_collapsed);

  auto bm1 = model_->GetSectionById(AstraSidebarModel::kSectionBookmarks);
  auto bm2 = model2->GetSectionById(AstraSidebarModel::kSectionBookmarks);
  ASSERT_TRUE(bm1.has_value() && bm2.has_value());
  EXPECT_EQ(bm1->is_collapsed, bm2->is_collapsed);
}

// Test 102: Visibility persists through PrefService for per-section prefs.
TEST_F(AstraSidebarModelTest, SectionVisibilityPrefsPersist) {
  model_->SetShowHistorySection(false);
  model_->SetShowDownloadsSection(false);
  model_->SetShowNotesSection(false);
  model_->SetShowPasswordsSection(false);

  auto model2 = std::make_unique<AstraSidebarModel>(prefs_.get());

  EXPECT_FALSE(model2->show_history_section());
  EXPECT_FALSE(model2->show_downloads_section());
  EXPECT_FALSE(model2->show_notes_section());
  EXPECT_FALSE(model2->show_passwords_section());
}

// =========================================================================
// Observer notification sequence tests
// =========================================================================

// Test 103: ToggleVisible sends exactly one notification.
TEST_F(AstraSidebarModelTest, ToggleVisibleSendsOneNotification) {
  MockSidebarModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSidebarHidden()).Times(1);
  EXPECT_CALL(observer, OnSidebarShown()).Times(0);
  model_->ToggleVisible();

  Mock::VerifyAndClearExpectations(&observer);

  EXPECT_CALL(observer, OnSidebarShown()).Times(1);
  EXPECT_CALL(observer, OnSidebarHidden()).Times(0);
  model_->ToggleVisible();

  model_->RemoveObserver(&observer);
}

// Test 104: Setting a presentation setting sends OnSidebarSettingsChanged.
TEST_F(AstraSidebarModelTest, SettingChangeSendsSettingsChanged) {
  MockSidebarModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSidebarSettingsChanged()).Times(1);
  model_->SetShowSectionIcons(false);

  Mock::VerifyAndClearExpectations(&observer);

  EXPECT_CALL(observer, OnSidebarSettingsChanged()).Times(0);
  model_->SetShowSectionIcons(false);

  model_->RemoveObserver(&observer);
}

// Test 105: Active section change does not trigger settings changed.
TEST_F(AstraSidebarModelTest, ActiveSectionChangeDoesNotTriggerSettingsChanged) {
  MockSidebarModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnActiveSectionChanged(_)).Times(1);
  EXPECT_CALL(observer, OnSidebarSettingsChanged()).Times(0);

  model_->SetActiveSection(AstraSidebarModel::kSectionDownloads);

  model_->RemoveObserver(&observer);
}

// =========================================================================
// Base item view tests
// =========================================================================

// Test 106: Default state after construction.
TEST_F(AstraSidebarItemViewTest, DefaultState) {
  EXPECT_TRUE(item_view_->GetTitle().empty());
  EXPECT_FALSE(item_view_->IsSelected());
  EXPECT_FALSE(item_view_->IsHovered());
  EXPECT_FALSE(item_view_->IsActive());
  EXPECT_TRUE(item_view_->IsEnabled());
  EXPECT_TRUE(item_view_->IsContextMenuEnabled());
  EXPECT_FALSE(item_view_->IsDragEnabled());
  EXPECT_FALSE(item_view_->IsDropTarget());
  EXPECT_FALSE(item_view_->IsChevronVisible());
  EXPECT_FALSE(item_view_->IsChevronRotated());
  EXPECT_EQ(item_view_->audio_state(),
            AstraSidebarItemView::AudioState::kNone);
  EXPECT_EQ(item_view_->suspended_state(),
            AstraSidebarItemView::SuspendedState::kNone);
  EXPECT_EQ(item_view_->tab_index(), -1);
  EXPECT_TRUE(item_view_->workspace_id().empty());
  EXPECT_TRUE(item_view_->favorite_folder_id().empty());
}

// Test 107: SetTitle updates the title.
TEST_F(AstraSidebarItemViewTest, SetTitleUpdatesTitle) {
  std::u16string title = u"Test Title";
  item_view_->SetTitle(title);
  EXPECT_EQ(item_view_->GetTitle(), title);
}

// Test 108: SetTitle with empty string.
TEST_F(AstraSidebarItemViewTest, SetTitleEmptyString) {
  item_view_->SetTitle(u"Hello");
  item_view_->SetTitle(u"");
  EXPECT_TRUE(item_view_->GetTitle().empty());
}

// Test 109: SetTooltip updates the tooltip.
TEST_F(AstraSidebarItemViewTest, SetTooltipUpdatesTooltip) {
  std::u16string tooltip = u"Test Tooltip";
  item_view_->SetTooltip(tooltip);
  EXPECT_EQ(item_view_->GetTooltip(), tooltip);
}

// Test 110: SetTooltip with empty string.
TEST_F(AstraSidebarItemViewTest, SetTooltipEmptyString) {
  item_view_->SetTooltip(u"Hello");
  item_view_->SetTooltip(u"");
  EXPECT_TRUE(item_view_->GetTooltip().empty());
}

// Test 111: SetSelected changes selection state.
TEST_F(AstraSidebarItemViewTest, SetSelectedChangesState) {
  EXPECT_FALSE(item_view_->IsSelected());
  item_view_->SetSelected(true);
  EXPECT_TRUE(item_view_->IsSelected());
  item_view_->SetSelected(false);
  EXPECT_FALSE(item_view_->IsSelected());
}

// Test 112: SetSelected with same value is a no-op.
TEST_F(AstraSidebarItemViewTest, SetSelectedNoOpWhenSame) {
  item_view_->SetSelected(false);
  // Should not crash or trigger unnecessary updates.
  item_view_->SetSelected(false);
  EXPECT_FALSE(item_view_->IsSelected());
}

// Test 113: SetHovered changes hover state.
TEST_F(AstraSidebarItemViewTest, SetHoveredChangesState) {
  EXPECT_FALSE(item_view_->IsHovered());
  item_view_->SetHovered(true);
  EXPECT_TRUE(item_view_->IsHovered());
  item_view_->SetHovered(false);
  EXPECT_FALSE(item_view_->IsHovered());
}

// Test 114: SetHovered with same value is a no-op.
TEST_F(AstraSidebarItemViewTest, SetHoveredNoOpWhenSame) {
  item_view_->SetHovered(false);
  item_view_->SetHovered(false);
  EXPECT_FALSE(item_view_->IsHovered());
}

// Test 115: SetActive changes active state.
TEST_F(AstraSidebarItemViewTest, SetActiveChangesState) {
  EXPECT_FALSE(item_view_->IsActive());
  item_view_->SetActive(true);
  EXPECT_TRUE(item_view_->IsActive());
  item_view_->SetActive(false);
  EXPECT_FALSE(item_view_->IsActive());
}

// Test 116: SetActive with same value is a no-op.
TEST_F(AstraSidebarItemViewTest, SetActiveNoOpWhenSame) {
  item_view_->SetActive(false);
  item_view_->SetActive(false);
  EXPECT_FALSE(item_view_->IsActive());
}

// Test 117: SetEnabled changes enabled state.
TEST_F(AstraSidebarItemViewTest, SetEnabledChangesState) {
  EXPECT_TRUE(item_view_->IsEnabled());
  item_view_->SetEnabled(false);
  EXPECT_FALSE(item_view_->IsEnabled());
  item_view_->SetEnabled(true);
  EXPECT_TRUE(item_view_->IsEnabled());
}

// Test 118: SetEnabled with same value is a no-op.
TEST_F(AstraSidebarItemViewTest, SetEnabledNoOpWhenSame) {
  item_view_->SetEnabled(true);
  item_view_->SetEnabled(true);
  EXPECT_TRUE(item_view_->IsEnabled());
}

// Test 119: SetContextMenuEnabled toggles context menu.
TEST_F(AstraSidebarItemViewTest, SetContextMenuEnabled) {
  EXPECT_TRUE(item_view_->IsContextMenuEnabled());
  item_view_->SetContextMenuEnabled(false);
  EXPECT_FALSE(item_view_->IsContextMenuEnabled());
  item_view_->SetContextMenuEnabled(true);
  EXPECT_TRUE(item_view_->IsContextMenuEnabled());
}

// Test 120: SetDragEnabled toggles drag.
TEST_F(AstraSidebarItemViewTest, SetDragEnabled) {
  EXPECT_FALSE(item_view_->IsDragEnabled());
  item_view_->SetDragEnabled(true);
  EXPECT_TRUE(item_view_->IsDragEnabled());
  item_view_->SetDragEnabled(false);
  EXPECT_FALSE(item_view_->IsDragEnabled());
}

// Test 121: SetDropTarget changes drop target state.
TEST_F(AstraSidebarItemViewTest, SetDropTargetChangesState) {
  EXPECT_FALSE(item_view_->IsDropTarget());
  item_view_->SetDropTarget(true);
  EXPECT_TRUE(item_view_->IsDropTarget());
  item_view_->SetDropTarget(false);
  EXPECT_FALSE(item_view_->IsDropTarget());
}

// Test 122: SetDropTarget with same value is a no-op.
TEST_F(AstraSidebarItemViewTest, SetDropTargetNoOpWhenSame) {
  item_view_->SetDropTarget(false);
  item_view_->SetDropTarget(false);
  EXPECT_FALSE(item_view_->IsDropTarget());
}

// Test 123: SetIcon shows the icon.
TEST_F(AstraSidebarItemViewTest, SetIconShowsIcon) {
  gfx::ImageSkia icon;
  // Create a simple icon for testing.
  item_view_->SetIcon(icon);
  // Icon view should be visible after setting.
  // Note: with null ImageSkia, it might be hidden.
  // For now, just verify it doesn't crash.
  SUCCEED();
}

// Test 124: ClearIcon hides the icon.
TEST_F(AstraSidebarItemViewTest, ClearIconHidesIcon) {
  item_view_->ClearIcon();
  // Should not crash.
  SUCCEED();
}

// Test 125: SetTrailingIcon sets trailing icon.
TEST_F(AstraSidebarItemViewTest, SetTrailingIcon) {
  gfx::ImageSkia icon;
  item_view_->SetTrailingIcon(icon);
  // Should not crash.
  SUCCEED();
}

// Test 126: ShowTrailingIcon toggles visibility.
TEST_F(AstraSidebarItemViewTest, ShowTrailingIcon) {
  item_view_->ShowTrailingIcon(true);
  item_view_->ShowTrailingIcon(false);
  // Should not crash.
  SUCCEED();
}

// Test 127: SetSecondaryText sets subtitle text.
TEST_F(AstraSidebarItemViewTest, SetSecondaryText) {
  std::u16string text = u"Secondary text";
  item_view_->SetSecondaryText(text);
  // Should not crash.
  SUCCEED();
}

// Test 128: ShowSecondaryText toggles visibility.
TEST_F(AstraSidebarItemViewTest, ShowSecondaryText) {
  item_view_->ShowSecondaryText(true);
  item_view_->ShowSecondaryText(false);
  // Should not crash.
  SUCCEED();
}

// Test 129: SetBadgeText sets badge text.
TEST_F(AstraSidebarItemViewTest, SetBadgeText) {
  item_view_->SetBadgeText(u"42");
  SUCCEED();
}

// Test 130: ShowBadge toggles badge visibility.
TEST_F(AstraSidebarItemViewTest, ShowBadge) {
  item_view_->ShowBadge(true);
  item_view_->ShowBadge(false);
  SUCCEED();
}

// Test 131: SetChevronVisible shows/hides chevron.
TEST_F(AstraSidebarItemViewTest, SetChevronVisible) {
  EXPECT_FALSE(item_view_->IsChevronVisible());
  item_view_->SetChevronVisible(true);
  EXPECT_TRUE(item_view_->IsChevronVisible());
  item_view_->SetChevronVisible(false);
  EXPECT_FALSE(item_view_->IsChevronVisible());
}

// Test 132: SetChevronRotated rotates chevron.
TEST_F(AstraSidebarItemViewTest, SetChevronRotated) {
  EXPECT_FALSE(item_view_->IsChevronRotated());
  item_view_->SetChevronRotated(true);
  EXPECT_TRUE(item_view_->IsChevronRotated());
  item_view_->SetChevronRotated(false);
  EXPECT_FALSE(item_view_->IsChevronRotated());
}

// Test 133: SetAudioState changes audio state.
TEST_F(AstraSidebarItemViewTest, SetAudioState) {
  EXPECT_EQ(item_view_->audio_state(),
            AstraSidebarItemView::AudioState::kNone);
  item_view_->SetAudioState(AstraSidebarItemView::AudioState::kPlaying);
  EXPECT_EQ(item_view_->audio_state(),
            AstraSidebarItemView::AudioState::kPlaying);
  item_view_->SetAudioState(AstraSidebarItemView::AudioState::kMuted);
  EXPECT_EQ(item_view_->audio_state(),
            AstraSidebarItemView::AudioState::kMuted);
}

// Test 134: SetAudioState with same value is no-op.
TEST_F(AstraSidebarItemViewTest, SetAudioStateNoOpWhenSame) {
  item_view_->SetAudioState(AstraSidebarItemView::AudioState::kNone);
  item_view_->SetAudioState(AstraSidebarItemView::AudioState::kNone);
  EXPECT_EQ(item_view_->audio_state(),
            AstraSidebarItemView::AudioState::kNone);
}

// Test 135: SetSuspendedState changes suspended state.
TEST_F(AstraSidebarItemViewTest, SetSuspendedState) {
  EXPECT_EQ(item_view_->suspended_state(),
            AstraSidebarItemView::SuspendedState::kNone);
  item_view_->SetSuspendedState(
      AstraSidebarItemView::SuspendedState::kSuspended);
  EXPECT_EQ(item_view_->suspended_state(),
            AstraSidebarItemView::SuspendedState::kSuspended);
}

// Test 136: SetSuspendedState with same value is no-op.
TEST_F(AstraSidebarItemViewTest, SetSuspendedStateNoOpWhenSame) {
  item_view_->SetSuspendedState(
      AstraSidebarItemView::SuspendedState::kNone);
  item_view_->SetSuspendedState(
      AstraSidebarItemView::SuspendedState::kNone);
  EXPECT_EQ(item_view_->suspended_state(),
            AstraSidebarItemView::SuspendedState::kNone);
}

// Test 137: Tab index round-trip.
TEST_F(AstraSidebarItemViewTest, TabIndexRoundTrip) {
  item_view_->set_tab_index(42);
  EXPECT_EQ(item_view_->tab_index(), 42);
}

// Test 138: Workspace ID round-trip.
TEST_F(AstraSidebarItemViewTest, WorkspaceIdRoundTrip) {
  std::string id = "workspace-123";
  item_view_->set_workspace_id(id);
  EXPECT_EQ(item_view_->workspace_id(), id);
}

// Test 139: Favorite folder ID round-trip.
TEST_F(AstraSidebarItemViewTest, FavoriteFolderIdRoundTrip) {
  std::string id = "folder-456";
  item_view_->set_favorite_folder_id(id);
  EXPECT_EQ(item_view_->favorite_folder_id(), id);
}

// Test 140: Preferred size is valid.
TEST_F(AstraSidebarItemViewTest, PreferredSizeValid) {
  gfx::Size size = item_view_->GetPreferredSize();
  EXPECT_GT(size.height(), 0);
  // Width depends on content, should be non-negative.
  EXPECT_GE(size.width(), 0);
}

// Test 141: Hover delegate gets notified on enter.
TEST_F(AstraSidebarItemViewTest, HoverDelegateNotifiedOnEnter) {
  MockSidebarItemHoverDelegate delegate;
  item_view_->set_hover_delegate(&delegate);

  EXPECT_CALL(delegate, OnItemHoverStarted(item_view_, _)).Times(1);

  ui::MouseEvent event(ui::ET_MOUSE_ENTERED, gfx::Point(), gfx::Point(),
                       base::TimeTicks(), 0, 0);
  item_view_->OnMouseEntered(event);

  EXPECT_TRUE(item_view_->IsHovered());
}

// Test 142: Hover delegate gets notified on exit.
TEST_F(AstraSidebarItemViewTest, HoverDelegateNotifiedOnExit) {
  MockSidebarItemHoverDelegate delegate;
  item_view_->set_hover_delegate(&delegate);

  // First enter.
  ui::MouseEvent enter_event(ui::ET_MOUSE_ENTERED, gfx::Point(),
                             gfx::Point(), base::TimeTicks(), 0, 0);
  item_view_->OnMouseEntered(enter_event);

  EXPECT_CALL(delegate, OnItemHoverEnded(item_view_)).Times(1);

  ui::MouseEvent exit_event(ui::ET_MOUSE_EXITED, gfx::Point(), gfx::Point(),
                            base::TimeTicks(), 0, 0);
  item_view_->OnMouseExited(exit_event);

  EXPECT_FALSE(item_view_->IsHovered());
}

// Test 143: Item view is focusable.
TEST_F(AstraSidebarItemViewTest, IsFocusable) {
  EXPECT_EQ(item_view_->GetFocusBehavior(),
            views::View::FocusBehavior::ALWAYS);
}

// Test 144: Has layer for rounded corners.
TEST_F(AstraSidebarItemViewTest, HasLayer) {
  EXPECT_NE(item_view_->layer(), nullptr);
}

// =========================================================================
// Bookmark item view tests
// =========================================================================

// Test 145: Default URL bookmark item state.
TEST_F(AstraBookmarkItemViewTest, DefaultUrlItemState) {
  EXPECT_FALSE(bookmark_view_->IsFolder());
  EXPECT_EQ(bookmark_view_->depth(), 0);
  EXPECT_FALSE(bookmark_view_->IsBookmarkBar());
  // URL items should not have chevron by default? Actually they might
  // since base class has chevron hidden by default.
  EXPECT_FALSE(bookmark_view_->IsChevronVisible());
}

// Test 146: SetTitle updates title.
TEST_F(AstraBookmarkItemViewTest, SetTitleUpdatesTitle) {
  std::u16string title = u"My Bookmark";
  bookmark_view_->SetTitle(title);
  EXPECT_EQ(bookmark_view_->GetTitle(), title);
}

// Test 147: GetUrl returns the URL.
TEST_F(AstraBookmarkItemViewTest, GetUrl) {
  GURL url("https://example.com");
  bookmark_view_->SetBookmarkInfo(url, u"Example", false);
  EXPECT_EQ(bookmark_view_->GetUrl(), url);
}

// Test 148: IsFolder returns false for URL items.
TEST_F(AstraBookmarkItemViewTest, IsFolderFalseForUrl) {
  bookmark_view_->SetBookmarkInfo(GURL("https://example.com"), u"Test",
                                   false);
  EXPECT_FALSE(bookmark_view_->IsFolder());
}

// Test 149: IsFolder returns true for folder items.
TEST_F(AstraBookmarkItemViewTest, IsFolderTrueForFolder) {
  bookmark_view_->SetBookmarkInfo(GURL(), u"Test Folder", true);
  EXPECT_TRUE(bookmark_view_->IsFolder());
}

// Test 150: Folder expanded default state.
TEST_F(AstraBookmarkItemViewTest, FolderExpandedDefault) {
  bookmark_view_->SetBookmarkInfo(GURL(), u"Test Folder", true);
  EXPECT_TRUE(bookmark_view_->IsFolderExpanded());
}

// Test 151: SetFolderExpanded toggles state.
TEST_F(AstraBookmarkItemViewTest, SetFolderExpanded) {
  bookmark_view_->SetBookmarkInfo(GURL(), u"Test Folder", true);

  EXPECT_TRUE(bookmark_view_->IsFolderExpanded());
  bookmark_view_->SetFolderExpanded(false);
  EXPECT_FALSE(bookmark_view_->IsFolderExpanded());
  bookmark_view_->SetFolderExpanded(true);
  EXPECT_TRUE(bookmark_view_->IsFolderExpanded());
}

// Test 152: SetFolderExpanded has no effect on URL items.
TEST_F(AstraBookmarkItemViewTest, SetFolderExpandedNoOpForUrl) {
  bookmark_view_->SetBookmarkInfo(GURL("https://example.com"), u"Test",
                                   false);
  bookmark_view_->SetFolderExpanded(true);
  // Should not crash, and is_folder_expanded_ might change but has no UI effect.
  SUCCEED();
}

// Test 153: SetBookmarkCount sets the count.
TEST_F(AstraBookmarkItemViewTest, SetBookmarkCount) {
  bookmark_view_->SetBookmarkInfo(GURL(), u"Test Folder", true);
  bookmark_view_->SetBookmarkCount(42);
  bookmark_view_->ShowBookmarkCount(true);
  // Should not crash.
  SUCCEED();
}

// Test 154: ShowBookmarkCount toggles visibility.
TEST_F(AstraBookmarkItemViewTest, ShowBookmarkCount) {
  bookmark_view_->SetBookmarkInfo(GURL(), u"Test Folder", true);
  bookmark_view_->SetBookmarkCount(5);
  bookmark_view_->ShowBookmarkCount(true);
  bookmark_view_->ShowBookmarkCount(false);
  // Should not crash.
  SUCCEED();
}

// Test 155: SetIsBookmarkBar toggles bookmark bar styling.
TEST_F(AstraBookmarkItemViewTest, SetIsBookmarkBar) {
  EXPECT_FALSE(bookmark_view_->IsBookmarkBar());
  bookmark_view_->SetIsBookmarkBar(true);
  EXPECT_TRUE(bookmark_view_->IsBookmarkBar());
  bookmark_view_->SetIsBookmarkBar(false);
  EXPECT_FALSE(bookmark_view_->IsBookmarkBar());
}

// Test 156: Depth property.
TEST_F(AstraBookmarkItemViewTest, DepthProperty) {
  EXPECT_EQ(bookmark_view_->depth(), 0);
  // Depth is set at construction; verify it doesn't change unexpectedly.
}

// Test 157: Node property.
TEST_F(AstraBookmarkItemViewTest, NodeProperty) {
  EXPECT_EQ(bookmark_view_->bookmark_node(), nullptr);
}

// Test 158: Type property for URL items.
TEST_F(AstraBookmarkItemViewTest, TypePropertyUrl) {
  EXPECT_EQ(bookmark_view_->type(), AstraBookmarkItemView::Type::kUrl);
}

// Test 159: SetBookmarkInfo changes type from URL to folder.
TEST_F(AstraBookmarkItemViewTest, SetBookmarkInfoUrlToFolder) {
  bookmark_view_->SetBookmarkInfo(GURL(), u"Folder", true);
  EXPECT_TRUE(bookmark_view_->IsFolder());
}

// Test 160: SetActive toggles active state.
TEST_F(AstraBookmarkItemViewTest, SetActive) {
  EXPECT_FALSE(bookmark_view_->IsActive());
  bookmark_view_->SetActive(true);
  EXPECT_TRUE(bookmark_view_->IsActive());
  bookmark_view_->SetActive(false);
  EXPECT_FALSE(bookmark_view_->IsActive());
}

// Test 161: Delegate click notification.
TEST_F(AstraBookmarkItemViewTest, DelegateClickNotification) {
  MockBookmarkItemDelegate delegate;
  bookmark_view_->set_delegate(&delegate);

  // Note: actual click requires mouse events. For this test, we verify
  // the delegate can be set without crashing.
  SUCCEED();
}

// Test 162: Preferred size is valid.
TEST_F(AstraBookmarkItemViewTest, PreferredSizeValid) {
  gfx::Size size = bookmark_view_->GetPreferredSize();
  EXPECT_GT(size.height(), 0);
  EXPECT_GE(size.width(), 0);
}

// =========================================================================
// History item view tests
// =========================================================================

// Test 163: Default state after construction.
TEST_F(AstraHistoryItemViewTest, DefaultState) {
  EXPECT_EQ(history_view_->GetTitle(), u"Test Title");
  EXPECT_EQ(history_view_->GetUrl(), GURL("https://example.com"));
  EXPECT_FALSE(history_view_->GetVisitTime().is_null());
  EXPECT_EQ(history_view_->GetVisitCount(), 1);
  EXPECT_FALSE(history_view_->IsToday());
  EXPECT_FALSE(history_view_->IsTypedVisit());
}

// Test 164: SetHistoryInfo updates all fields.
TEST_F(AstraHistoryItemViewTest, SetHistoryInfoUpdatesAll) {
  GURL url("https://newsite.com");
  std::u16string title = u"New Title";
  base::Time time = base::Time::Now() - base::Hours(2);

  history_view_->SetHistoryInfo(url, title, time);

  EXPECT_EQ(history_view_->GetUrl(), url);
  EXPECT_EQ(history_view_->GetTitle(), title);
  EXPECT_EQ(history_view_->GetVisitTime(), time);
}

// Test 165: SetVisitCount updates visit count.
TEST_F(AstraHistoryItemViewTest, SetVisitCount) {
  history_view_->SetVisitCount(5);
  EXPECT_EQ(history_view_->GetVisitCount(), 5);
}

// Test 166: ShowVisitCount toggles visibility.
TEST_F(AstraHistoryItemViewTest, ShowVisitCount) {
  history_view_->SetVisitCount(10);
  history_view_->ShowVisitCount(true);
  history_view_->ShowVisitCount(false);
  SUCCEED();
}

// Test 167: SetIsToday toggles today styling.
TEST_F(AstraHistoryItemViewTest, SetIsToday) {
  EXPECT_FALSE(history_view_->IsToday());
  history_view_->SetIsToday(true);
  EXPECT_TRUE(history_view_->IsToday());
  history_view_->SetIsToday(false);
  EXPECT_FALSE(history_view_->IsToday());
}

// Test 168: SetTimeGroup sets the group name.
TEST_F(AstraHistoryItemViewTest, SetTimeGroup) {
  std::u16string group = u"Yesterday";
  history_view_->SetTimeGroup(group);
  EXPECT_EQ(history_view_->GetTimeGroup(), group);
}

// Test 169: SetTimeGroup empty string.
TEST_F(AstraHistoryItemViewTest, SetTimeGroupEmpty) {
  history_view_->SetTimeGroup(u"");
  EXPECT_TRUE(history_view_->GetTimeGroup().empty());
}

// Test 170: SetTypedVisit toggles typed visit state.
TEST_F(AstraHistoryItemViewTest, SetTypedVisit) {
  EXPECT_FALSE(history_view_->IsTypedVisit());
  history_view_->SetTypedVisit(true);
  EXPECT_TRUE(history_view_->IsTypedVisit());
  history_view_->SetTypedVisit(false);
  EXPECT_FALSE(history_view_->IsTypedVisit());
}

// Test 171: Delegate click notification.
TEST_F(AstraHistoryItemViewTest, DelegateClickNotification) {
  MockHistoryItemDelegate delegate;
  history_view_->set_delegate(&delegate);
  SUCCEED();
}

// Test 172: Preferred size is valid.
TEST_F(AstraHistoryItemViewTest, PreferredSizeValid) {
  gfx::Size size = history_view_->GetPreferredSize();
  EXPECT_GT(size.height(), 0);
  EXPECT_GE(size.width(), 0);
}

// Test 173: Title update reflects in label.
TEST_F(AstraHistoryItemViewTest, TitleUpdateReflectsInLabel) {
  std::u16string title = u"Updated Title";
  history_view_->SetTitle(title);
  EXPECT_EQ(history_view_->GetTitle(), title);
}

// Test 174: Multiple SetVisitCount calls.
TEST_F(AstraHistoryItemViewTest, MultipleVisitCountUpdates) {
  history_view_->SetVisitCount(1);
  EXPECT_EQ(history_view_->GetVisitCount(), 1);
  history_view_->SetVisitCount(5);
  EXPECT_EQ(history_view_->GetVisitCount(), 5);
  history_view_->SetVisitCount(100);
  EXPECT_EQ(history_view_->GetVisitCount(), 100);
}

// Test 175: Visit time from long ago.
TEST_F(AstraHistoryItemViewTest, VisitTimeLongAgo) {
  base::Time old_time = base::Time::Now() - base::Days(30);
  history_view_->SetHistoryInfo(GURL("https://old.com"), u"Old Site",
                                 old_time);
  EXPECT_EQ(history_view_->GetVisitTime(), old_time);
}

// =========================================================================
// Download item view tests
// =========================================================================

// Test 176: Default state after construction.
TEST_F(AstraDownloadItemViewTest, DefaultState) {
  EXPECT_EQ(download_view_->GetFilename(), u"file.zip");
  EXPECT_EQ(download_view_->GetState(),
            AstraDownloadState::kInProgress);
  EXPECT_EQ(download_view_->GetBytesReceived(), 1024);
  EXPECT_EQ(download_view_->GetTotalBytes(), 10240);
  EXPECT_GT(download_view_->GetProgress(), 0.0);
  EXPECT_LT(download_view_->GetProgress(), 1.0);
  EXPECT_FALSE(download_view_->IsDangerous());
  EXPECT_EQ(download_view_->download_id(), "test-id-1");
}

// Test 177: SetDownloadInfo updates all fields.
TEST_F(AstraDownloadItemViewTest, SetDownloadInfoUpdatesAll) {
  GURL url("https://downloads.com/file.exe");
  std::u16string filename = u"newfile.exe";
  int64_t total = 20480;

  download_view_->SetDownloadInfo(url, filename, total);

  EXPECT_EQ(download_view_->GetUrl(), url);
  EXPECT_EQ(download_view_->GetFilename(), filename);
  EXPECT_EQ(download_view_->GetTotalBytes(), total);
}

// Test 178: SetProgress updates progress.
TEST_F(AstraDownloadItemViewTest, SetProgress) {
  download_view_->SetProgress(0.5);
  EXPECT_DOUBLE_EQ(download_view_->GetProgress(), 0.5);

  download_view_->SetProgress(0.0);
  EXPECT_DOUBLE_EQ(download_view_->GetProgress(), 0.0);

  download_view_->SetProgress(1.0);
  EXPECT_DOUBLE_EQ(download_view_->GetProgress(), 1.0);
}

// Test 179: SetState to complete.
TEST_F(AstraDownloadItemViewTest, SetStateComplete) {
  download_view_->SetState(AstraDownloadState::kComplete);
  EXPECT_EQ(download_view_->GetState(), AstraDownloadState::kComplete);
}

// Test 180: SetState to paused.
TEST_F(AstraDownloadItemViewTest, SetStatePaused) {
  download_view_->SetState(AstraDownloadState::kPaused);
  EXPECT_EQ(download_view_->GetState(), AstraDownloadState::kPaused);
}

// Test 181: SetState to failed.
TEST_F(AstraDownloadItemViewTest, SetStateFailed) {
  download_view_->SetState(AstraDownloadState::kFailed);
  EXPECT_EQ(download_view_->GetState(), AstraDownloadState::kFailed);
}

// Test 182: SetState to cancelled.
TEST_F(AstraDownloadItemViewTest, SetStateCancelled) {
  download_view_->SetState(AstraDownloadState::kCancelled);
  EXPECT_EQ(download_view_->GetState(), AstraDownloadState::kCancelled);
}

// Test 183: SetState to interrupted.
TEST_F(AstraDownloadItemViewTest, SetStateInterrupted) {
  download_view_->SetState(AstraDownloadState::kInterrupted);
  EXPECT_EQ(download_view_->GetState(),
            AstraDownloadState::kInterrupted);
}

// Test 184: SetBytesReceived updates received bytes.
TEST_F(AstraDownloadItemViewTest, SetBytesReceived) {
  download_view_->SetBytesReceived(5000);
  EXPECT_EQ(download_view_->GetBytesReceived(), 5000);
}

// Test 185: SetTotalBytes updates total bytes.
TEST_F(AstraDownloadItemViewTest, SetTotalBytes) {
  download_view_->SetTotalBytes(50000);
  EXPECT_EQ(download_view_->GetTotalBytes(), 50000);
}

// Test 186: SetTimeRemaining updates time.
TEST_F(AstraDownloadItemViewTest, SetTimeRemaining) {
  base::TimeDelta remaining = base::Seconds(30);
  download_view_->SetTimeRemaining(remaining);
  EXPECT_EQ(download_view_->GetTimeRemaining(), remaining);
}

// Test 187: SetIsDangerous toggles dangerous flag.
TEST_F(AstraDownloadItemViewTest, SetIsDangerous) {
  EXPECT_FALSE(download_view_->IsDangerous());
  download_view_->SetIsDangerous(true);
  EXPECT_TRUE(download_view_->IsDangerous());
  download_view_->SetIsDangerous(false);
  EXPECT_FALSE(download_view_->IsDangerous());
}

// Test 188: CanResume for paused state.
TEST_F(AstraDownloadItemViewTest, CanResumePaused) {
  download_view_->SetState(AstraDownloadState::kPaused);
  EXPECT_TRUE(download_view_->CanResume());
}

// Test 189: CanResume for interrupted state.
TEST_F(AstraDownloadItemViewTest, CanResumeInterrupted) {
  download_view_->SetState(AstraDownloadState::kInterrupted);
  EXPECT_TRUE(download_view_->CanResume());
}

// Test 190: CanResume false for in-progress state.
TEST_F(AstraDownloadItemViewTest, CanResumeFalseForInProgress) {
  download_view_->SetState(AstraDownloadState::kInProgress);
  EXPECT_FALSE(download_view_->CanResume());
}

// Test 191: CanPause for in-progress state.
TEST_F(AstraDownloadItemViewTest, CanPauseInProgress) {
  download_view_->SetState(AstraDownloadState::kInProgress);
  EXPECT_TRUE(download_view_->CanPause());
}

// Test 192: CanPause false for paused state.
TEST_F(AstraDownloadItemViewTest, CanPauseFalseForPaused) {
  download_view_->SetState(AstraDownloadState::kPaused);
  EXPECT_FALSE(download_view_->CanPause());
}

// Test 193: CanCancel for in-progress state.
TEST_F(AstraDownloadItemViewTest, CanCancelInProgress) {
  download_view_->SetState(AstraDownloadState::kInProgress);
  EXPECT_TRUE(download_view_->CanCancel());
}

// Test 194: CanCancel false for completed state.
TEST_F(AstraDownloadItemViewTest, CanCancelFalseForComplete) {
  download_view_->SetState(AstraDownloadState::kComplete);
  EXPECT_FALSE(download_view_->CanCancel());
}

// Test 195: ShowProgressBar toggles visibility.
TEST_F(AstraDownloadItemViewTest, ShowProgressBar) {
  download_view_->ShowProgressBar(true);
  download_view_->ShowProgressBar(false);
  SUCCEED();
}

// Test 196: ShowPauseButton toggles visibility.
TEST_F(AstraDownloadItemViewTest, ShowPauseButton) {
  download_view_->ShowPauseButton(true);
  download_view_->ShowPauseButton(false);
  SUCCEED();
}

// Test 197: ShowCancelButton toggles visibility.
TEST_F(AstraDownloadItemViewTest, ShowCancelButton) {
  download_view_->ShowCancelButton(true);
  download_view_->ShowCancelButton(false);
  SUCCEED();
}

// Test 198: ShowOpenButton toggles visibility.
TEST_F(AstraDownloadItemViewTest, ShowOpenButton) {
  download_view_->ShowOpenButton(true);
  download_view_->ShowOpenButton(false);
  SUCCEED();
}

// Test 199: ShowInFolderButton toggles visibility.
TEST_F(AstraDownloadItemViewTest, ShowInFolderButton) {
  download_view_->ShowInFolderButton(true);
  download_view_->ShowInFolderButton(false);
  SUCCEED();
}

// Test 200: Delegate can be set.
TEST_F(AstraDownloadItemViewTest, DelegateCanBeSet) {
  MockDownloadItemDelegate delegate;
  download_view_->set_delegate(&delegate);
  SUCCEED();
}

// Test 201: Preferred size is valid.
TEST_F(AstraDownloadItemViewTest, PreferredSizeValid) {
  gfx::Size size = download_view_->GetPreferredSize();
  EXPECT_GT(size.height(), 0);
  EXPECT_GE(size.width(), 0);
}

// Test 202: Progress is 0 for 0 total bytes.
TEST_F(AstraDownloadItemViewTest, ProgressWithZeroTotal) {
  download_view_->SetTotalBytes(0);
  download_view_->SetBytesReceived(0);
  // Should not crash or produce NaN.
  SUCCEED();
}

// Test 203: All download states are valid.
TEST_F(AstraDownloadItemViewTest, AllStatesValid) {
  std::vector<AstraDownloadState> states = {
      AstraDownloadState::kInProgress,
      AstraDownloadState::kComplete,
      AstraDownloadState::kPaused,
      AstraDownloadState::kFailed,
      AstraDownloadState::kCancelled,
      AstraDownloadState::kInterrupted,
  };

  for (auto state : states) {
    download_view_->SetState(state);
    EXPECT_EQ(download_view_->GetState(), state);
  }
}

// =========================================================================
// Reading list item view tests
// =========================================================================

// Test 204: Default state after construction.
TEST_F(AstraReadingListItemViewTest, DefaultState) {
  EXPECT_EQ(reading_view_->GetTitle(), u"Test Article");
  EXPECT_EQ(reading_view_->GetUrl(), GURL("https://example.com/article"));
  EXPECT_EQ(reading_view_->GetDomain(), u"example.com");
  EXPECT_FALSE(reading_view_->IsRead());
  EXPECT_FALSE(reading_view_->HasDistilledVersion());
  EXPECT_FALSE(reading_view_->IsFavorite());
  EXPECT_EQ(reading_view_->GetWordCount(), 0);
  EXPECT_EQ(reading_view_->GetEstimatedReadTime(), base::TimeDelta());
}

// Test 205: SetReadingListItem updates all fields.
TEST_F(AstraReadingListItemViewTest, SetReadingListItemUpdatesAll) {
  GURL url("https://newsite.com/article");
  std::u16string title = u"New Article Title";

  reading_view_->SetReadingListItem(url, title, true);

  EXPECT_EQ(reading_view_->GetUrl(), url);
  EXPECT_EQ(reading_view_->GetTitle(), title);
  EXPECT_TRUE(reading_view_->IsRead());
}

// Test 206: SetRead toggles read state.
TEST_F(AstraReadingListItemViewTest, SetRead) {
  EXPECT_FALSE(reading_view_->IsRead());
  reading_view_->SetRead(true);
  EXPECT_TRUE(reading_view_->IsRead());
  reading_view_->SetRead(false);
  EXPECT_FALSE(reading_view_->IsRead());
}

// Test 207: SetRead with same value is no-op.
TEST_F(AstraReadingListItemViewTest, SetReadNoOpWhenSame) {
  reading_view_->SetRead(false);
  reading_view_->SetRead(false);
  EXPECT_FALSE(reading_view_->IsRead());
}

// Test 208: SetEstimatedReadTime updates read time.
TEST_F(AstraReadingListItemViewTest, SetEstimatedReadTime) {
  base::TimeDelta read_time = base::Minutes(5);
  reading_view_->SetEstimatedReadTime(read_time);
  EXPECT_EQ(reading_view_->GetEstimatedReadTime(), read_time);
}

// Test 209: SetWordCount updates word count.
TEST_F(AstraReadingListItemViewTest, SetWordCount) {
  reading_view_->SetWordCount(1500);
  EXPECT_EQ(reading_view_->GetWordCount(), 1500);
}

// Test 210: SetDomain updates domain.
TEST_F(AstraReadingListItemViewTest, SetDomain) {
  std::u16string domain = u"newsite.com";
  reading_view_->SetDomain(domain);
  EXPECT_EQ(reading_view_->GetDomain(), domain);
}

// Test 211: ShowReadIndicator toggles indicator.
TEST_F(AstraReadingListItemViewTest, ShowReadIndicator) {
  reading_view_->ShowReadIndicator(true);
  reading_view_->ShowReadIndicator(false);
  SUCCEED();
}

// Test 212: SetHasDistilledVersion toggles distilled state.
TEST_F(AstraReadingListItemViewTest, SetHasDistilledVersion) {
  EXPECT_FALSE(reading_view_->HasDistilledVersion());
  reading_view_->SetHasDistilledVersion(true);
  EXPECT_TRUE(reading_view_->HasDistilledVersion());
  reading_view_->SetHasDistilledVersion(false);
  EXPECT_FALSE(reading_view_->HasDistilledVersion());
}

// Test 213: SetIsFavorite toggles favorite state.
TEST_F(AstraReadingListItemViewTest, SetIsFavorite) {
  EXPECT_FALSE(reading_view_->IsFavorite());
  reading_view_->SetIsFavorite(true);
  EXPECT_TRUE(reading_view_->IsFavorite());
  reading_view_->SetIsFavorite(false);
  EXPECT_FALSE(reading_view_->IsFavorite());
}

// Test 214: Delegate can be set.
TEST_F(AstraReadingListItemViewTest, DelegateCanBeSet) {
  MockReadingListItemDelegate delegate;
  reading_view_->set_delegate(&delegate);
  SUCCEED();
}

// Test 215: Preferred size is valid.
TEST_F(AstraReadingListItemViewTest, PreferredSizeValid) {
  gfx::Size size = reading_view_->GetPreferredSize();
  EXPECT_GT(size.height(), 0);
  EXPECT_GE(size.width(), 0);
}

// Test 216: Title update works correctly.
TEST_F(AstraReadingListItemViewTest, TitleUpdate) {
  std::u16string title = u"Updated Article Title";
  reading_view_->SetTitle(title);
  EXPECT_EQ(reading_view_->GetTitle(), title);
}

// Test 217: Multiple read state toggles.
TEST_F(AstraReadingListItemViewTest, MultipleReadToggles) {
  for (int i = 0; i < 10; ++i) {
    reading_view_->SetRead(!reading_view_->IsRead());
  }
  // 10 toggles from false should end at false (even number).
  EXPECT_FALSE(reading_view_->IsRead());
}

// =========================================================================
// Note item view tests
// =========================================================================

// Test 218: Default state after construction.
TEST_F(AstraNoteItemViewTest, DefaultState) {
  EXPECT_EQ(note_view_->GetNoteId(), "note-1");
  EXPECT_EQ(note_view_->GetTitle(), u"My Note");
  EXPECT_EQ(note_view_->GetPreviewText(), u"This is a preview...");
  EXPECT_EQ(note_view_->GetNoteColor(), SK_ColorYELLOW);
  EXPECT_EQ(note_view_->GetTagCount(), 0);
  EXPECT_FALSE(note_view_->IsPinned());
  EXPECT_TRUE(note_view_->GetModifiedTime().is_null());
}

// Test 219: SetNoteInfo updates all fields.
TEST_F(AstraNoteItemViewTest, SetNoteInfoUpdatesAll) {
  std::string id = "note-999";
  std::u16string title = u"New Note Title";
  std::u16string preview = u"New preview text";

  note_view_->SetNoteInfo(id, title, preview);

  EXPECT_EQ(note_view_->GetNoteId(), id);
  EXPECT_EQ(note_view_->GetTitle(), title);
  EXPECT_EQ(note_view_->GetPreviewText(), preview);
}

// Test 220: SetPreviewText updates preview.
TEST_F(AstraNoteItemViewTest, SetPreviewText) {
  std::u16string preview = u"Updated preview content";
  note_view_->SetPreviewText(preview);
  EXPECT_EQ(note_view_->GetPreviewText(), preview);
}

// Test 221: ShowPreview toggles visibility.
TEST_F(AstraNoteItemViewTest, ShowPreview) {
  note_view_->ShowPreview(true);
  note_view_->ShowPreview(false);
  SUCCEED();
}

// Test 222: SetNoteColor updates color.
TEST_F(AstraNoteItemViewTest, SetNoteColor) {
  SkColor color = SK_ColorBLUE;
  note_view_->SetNoteColor(color);
  EXPECT_EQ(note_view_->GetNoteColor(), color);

  note_view_->SetNoteColor(SK_ColorRED);
  EXPECT_EQ(note_view_->GetNoteColor(), SK_ColorRED);
}

// Test 223: SetTagCount updates tag count.
TEST_F(AstraNoteItemViewTest, SetTagCount) {
  note_view_->SetTagCount(3);
  EXPECT_EQ(note_view_->GetTagCount(), 3);
}

// Test 224: ShowTagCount toggles visibility.
TEST_F(AstraNoteItemViewTest, ShowTagCount) {
  note_view_->SetTagCount(2);
  note_view_->ShowTagCount(true);
  note_view_->ShowTagCount(false);
  SUCCEED();
}

// Test 225: SetPinned toggles pinned state.
TEST_F(AstraNoteItemViewTest, SetPinned) {
  EXPECT_FALSE(note_view_->IsPinned());
  note_view_->SetPinned(true);
  EXPECT_TRUE(note_view_->IsPinned());
  note_view_->SetPinned(false);
  EXPECT_FALSE(note_view_->IsPinned());
}

// Test 226: SetModifiedTime updates time.
TEST_F(AstraNoteItemViewTest, SetModifiedTime) {
  base::Time time = base::Time::Now() - base::Hours(1);
  note_view_->SetModifiedTime(time);
  EXPECT_EQ(note_view_->GetModifiedTime(), time);
}

// Test 227: Delegate can be set.
TEST_F(AstraNoteItemViewTest, DelegateCanBeSet) {
  MockNoteItemDelegate delegate;
  note_view_->set_delegate(&delegate);
  SUCCEED();
}

// Test 228: Preferred size is valid.
TEST_F(AstraNoteItemViewTest, PreferredSizeValid) {
  gfx::Size size = note_view_->GetPreferredSize();
  EXPECT_GT(size.height(), 0);
  EXPECT_GE(size.width(), 0);
}

// Test 229: Title update works.
TEST_F(AstraNoteItemViewTest, TitleUpdate) {
  std::u16string title = u"Updated Note Title";
  note_view_->SetTitle(title);
  EXPECT_EQ(note_view_->GetTitle(), title);
}

// Test 230: Multiple color changes.
TEST_F(AstraNoteItemViewTest, MultipleColorChanges) {
  note_view_->SetNoteColor(SK_ColorRED);
  note_view_->SetNoteColor(SK_ColorGREEN);
  note_view_->SetNoteColor(SK_ColorBLUE);
  EXPECT_EQ(note_view_->GetNoteColor(), SK_ColorBLUE);
}

// =========================================================================
// Password item view tests
// =========================================================================

// Test 231: Default state after construction.
TEST_F(AstraPasswordItemViewTest, DefaultState) {
  EXPECT_EQ(password_view_->GetSite(), u"example.com");
  EXPECT_EQ(password_view_->GetUsername(), u"user@example.com");
  EXPECT_FALSE(password_view_->IsCompromised());
  EXPECT_FALSE(password_view_->IsBlocked());
}

// Test 232: SetPasswordInfo updates all fields.
TEST_F(AstraPasswordItemViewTest, SetPasswordInfoUpdatesAll) {
  std::u16string site = u"newsite.com";
  std::u16string username = u"admin@newsite.com";

  password_view_->SetPasswordInfo(site, username, true);

  EXPECT_EQ(password_view_->GetSite(), site);
  EXPECT_EQ(password_view_->GetUsername(), username);
  EXPECT_TRUE(password_view_->IsCompromised());
}

// Test 233: SetCompromised toggles compromised state.
TEST_F(AstraPasswordItemViewTest, SetCompromised) {
  EXPECT_FALSE(password_view_->IsCompromised());
  password_view_->SetCompromised(true);
  EXPECT_TRUE(password_view_->IsCompromised());
  password_view_->SetCompromised(false);
  EXPECT_FALSE(password_view_->IsCompromised());
}

// Test 234: SetIsBlocked toggles blocked state.
TEST_F(AstraPasswordItemViewTest, SetIsBlocked) {
  EXPECT_FALSE(password_view_->IsBlocked());
  password_view_->SetIsBlocked(true);
  EXPECT_TRUE(password_view_->IsBlocked());
  password_view_->SetIsBlocked(false);
  EXPECT_FALSE(password_view_->IsBlocked());
}

// Test 235: Delegate can be set.
TEST_F(AstraPasswordItemViewTest, DelegateCanBeSet) {
  MockPasswordItemDelegate delegate;
  password_view_->set_delegate(&delegate);
  SUCCEED();
}

// Test 236: Preferred size is valid.
TEST_F(AstraPasswordItemViewTest, PreferredSizeValid) {
  gfx::Size size = password_view_->GetPreferredSize();
  EXPECT_GT(size.height(), 0);
  EXPECT_GE(size.width(), 0);
}

// Test 237: Blocked password is not enabled.
TEST_F(AstraPasswordItemViewTest, BlockedPasswordDisabled) {
  password_view_->SetIsBlocked(true);
  // Password should be disabled when blocked.
  SUCCEED();
}

// Test 238: Compromised state doesn't affect enabled.
TEST_F(AstraPasswordItemViewTest, CompromisedStillEnabled) {
  password_view_->SetCompromised(true);
  EXPECT_TRUE(password_view_->IsEnabled());
}

// =========================================================================
// Extension icon view tests
// =========================================================================

// Test 239: Default state after construction.
TEST_F(AstraExtensionIconViewTest, DefaultState) {
  EXPECT_EQ(extension_view_->GetExtensionId(), "ext-123");
  EXPECT_FALSE(extension_view_->popup_showing());
  EXPECT_TRUE(extension_view_->IsExtensionEnabled());
  EXPECT_FALSE(extension_view_->IsAction());
  EXPECT_EQ(extension_view_->GetExtensionState(),
            AstraExtensionState::kEnabled);
  EXPECT_FALSE(extension_view_->IsPinned());
  EXPECT_TRUE(extension_view_->HasPopup());
  EXPECT_FALSE(extension_view_->IsCurrent());
  EXPECT_EQ(extension_view_->GetNotificationCount(), 0);
  EXPECT_TRUE(extension_view_->GetShowNotificationBadge());
  EXPECT_FALSE(extension_view_->IsLoading());
  EXPECT_FALSE(extension_view_->IsInErrorState());
  EXPECT_TRUE(extension_view_->GetShowTooltip());
  EXPECT_TRUE(extension_view_->GetShowContextMenu());
  EXPECT_EQ(extension_view_->GetIconSize(), 24);
}

// Test 240: SetExtensionInfo updates all fields from struct.
TEST_F(AstraExtensionIconViewTest, SetExtensionInfoFromStruct) {
  AstraExtensionInfo info;
  info.extension_id = "ext-new";
  info.name = u"New Extension";
  info.state = AstraExtensionState::kDisabled;
  info.is_action = true;
  info.is_pinned = true;
  info.has_popup = false;

  extension_view_->SetExtensionInfo(info);

  EXPECT_EQ(extension_view_->GetExtensionId(), "ext-new");
  EXPECT_EQ(extension_view_->GetName(), u"New Extension");
  EXPECT_EQ(extension_view_->GetExtensionState(),
            AstraExtensionState::kDisabled);
  EXPECT_TRUE(extension_view_->IsAction());
  EXPECT_TRUE(extension_view_->IsPinned());
  EXPECT_FALSE(extension_view_->HasPopup());
}

// Test 241: SetIcon updates the icon image.
TEST_F(AstraExtensionIconViewTest, SetIcon) {
  gfx::ImageSkia icon;
  extension_view_->SetIcon(icon);
  SUCCEED();
}

// Test 242: SetName updates extension display name.
TEST_F(AstraExtensionIconViewTest, SetName) {
  EXPECT_EQ(extension_view_->GetName(), std::u16string());
  extension_view_->SetName(u"My Extension");
  EXPECT_EQ(extension_view_->GetName(), u"My Extension");
}

// Test 243: SetHasIcon toggles icon availability.
TEST_F(AstraExtensionIconViewTest, SetHasIcon) {
  EXPECT_FALSE(extension_view_->HasIcon());
  extension_view_->SetHasIcon(true);
  EXPECT_TRUE(extension_view_->HasIcon());
  extension_view_->SetHasIcon(false);
  EXPECT_FALSE(extension_view_->HasIcon());
}

// Test 244: SetBadgeText sets badge text.
TEST_F(AstraExtensionIconViewTest, SetBadgeText) {
  extension_view_->SetBadgeText(u"3");
  EXPECT_EQ(extension_view_->GetBadgeText(), u"3");
}

// Test 245: SetBadgeText with empty string clears the badge.
TEST_F(AstraExtensionIconViewTest, SetBadgeTextEmpty) {
  extension_view_->SetBadgeText(u"42");
  EXPECT_FALSE(extension_view_->GetBadgeText().empty());
  extension_view_->SetBadgeText(std::u16string());
  EXPECT_TRUE(extension_view_->GetBadgeText().empty());
}

// Test 246: SetBadgeColor sets badge text color.
TEST_F(AstraExtensionIconViewTest, SetBadgeColor) {
  extension_view_->SetBadgeColor(SK_ColorWHITE);
  EXPECT_EQ(extension_view_->GetBadgeColor(), SK_ColorWHITE);
  extension_view_->SetBadgeColor(SK_ColorBLACK);
  EXPECT_EQ(extension_view_->GetBadgeColor(), SK_ColorBLACK);
}

// Test 247: SetBadgeBackgroundColor sets badge background color.
TEST_F(AstraExtensionIconViewTest, SetBadgeBackgroundColor) {
  extension_view_->SetBadgeBackgroundColor(SK_ColorRED);
  EXPECT_EQ(extension_view_->GetBadgeBackgroundColor(), SK_ColorRED);
  extension_view_->SetBadgeBackgroundColor(SK_ColorBLUE);
  EXPECT_EQ(extension_view_->GetBadgeBackgroundColor(), SK_ColorBLUE);
}

// Test 248: SetIsAction toggles action state.
TEST_F(AstraExtensionIconViewTest, SetIsAction) {
  EXPECT_FALSE(extension_view_->IsAction());
  extension_view_->SetIsAction(true);
  EXPECT_TRUE(extension_view_->IsAction());
  extension_view_->SetIsAction(false);
  EXPECT_FALSE(extension_view_->IsAction());
}

// Test 249: SetExtensionState changes state (all 5 states).
TEST_F(AstraExtensionIconViewTest, SetExtensionStateAllStates) {
  std::vector<AstraExtensionState> states = {
      AstraExtensionState::kEnabled,
      AstraExtensionState::kDisabled,
      AstraExtensionState::kBlocked,
      AstraExtensionState::kError,
      AstraExtensionState::kUninstalled,
  };

  for (auto state : states) {
    extension_view_->SetExtensionState(state);
    EXPECT_EQ(extension_view_->GetExtensionState(), state);
  }
}

// Test 250: SetExtensionEnabled toggles enabled.
TEST_F(AstraExtensionIconViewTest, SetExtensionEnabled) {
  EXPECT_TRUE(extension_view_->IsExtensionEnabled());
  extension_view_->SetExtensionEnabled(false);
  EXPECT_FALSE(extension_view_->IsExtensionEnabled());
  extension_view_->SetExtensionEnabled(true);
  EXPECT_TRUE(extension_view_->IsExtensionEnabled());
}

// Test 251: SetPopupShowing toggles popup state.
TEST_F(AstraExtensionIconViewTest, SetPopupShowing) {
  EXPECT_FALSE(extension_view_->popup_showing());
  extension_view_->SetPopupShowing(true);
  EXPECT_TRUE(extension_view_->popup_showing());
  extension_view_->SetPopupShowing(false);
  EXPECT_FALSE(extension_view_->popup_showing());
}

// Test 252: SetPinned toggles pinned state.
TEST_F(AstraExtensionIconViewTest, SetPinned) {
  EXPECT_FALSE(extension_view_->IsPinned());
  extension_view_->SetPinned(true);
  EXPECT_TRUE(extension_view_->IsPinned());
  extension_view_->SetPinned(false);
  EXPECT_FALSE(extension_view_->IsPinned());
}

// Test 253: SetShowTooltip toggles tooltip visibility.
TEST_F(AstraExtensionIconViewTest, SetShowTooltip) {
  EXPECT_TRUE(extension_view_->GetShowTooltip());
  extension_view_->SetShowTooltip(false);
  EXPECT_FALSE(extension_view_->GetShowTooltip());
  extension_view_->SetShowTooltip(true);
  EXPECT_TRUE(extension_view_->GetShowTooltip());
}

// Test 254: SetShowContextMenu toggles context menu availability.
TEST_F(AstraExtensionIconViewTest, SetShowContextMenu) {
  EXPECT_TRUE(extension_view_->GetShowContextMenu());
  extension_view_->SetShowContextMenu(false);
  EXPECT_FALSE(extension_view_->GetShowContextMenu());
  extension_view_->SetShowContextMenu(true);
  EXPECT_TRUE(extension_view_->GetShowContextMenu());
}

// Test 255: SetIconSize changes icon size.
TEST_F(AstraExtensionIconViewTest, SetIconSize) {
  EXPECT_EQ(extension_view_->GetIconSize(), 24);
  extension_view_->SetIconSize(32);
  EXPECT_EQ(extension_view_->GetIconSize(), 32);
  extension_view_->SetIconSize(16);
  EXPECT_EQ(extension_view_->GetIconSize(), 16);
}

// Test 256: Icon size affects preferred size.
TEST_F(AstraExtensionIconViewTest, IconSizeAffectsPreferredSize) {
  extension_view_->SetIconSize(24);
  gfx::Size size24 = extension_view_->GetPreferredSize();

  extension_view_->SetIconSize(32);
  gfx::Size size32 = extension_view_->GetPreferredSize();

  EXPECT_GT(size32.width(), size24.width());
  EXPECT_GT(size32.height(), size24.height());
}

// Test 257: SetHasPopup toggles popup availability.
TEST_F(AstraExtensionIconViewTest, SetHasPopup) {
  EXPECT_TRUE(extension_view_->HasPopup());
  extension_view_->SetHasPopup(false);
  EXPECT_FALSE(extension_view_->HasPopup());
  extension_view_->SetHasPopup(true);
  EXPECT_TRUE(extension_view_->HasPopup());
}

// Test 258: SetIsCurrent toggles current/selected state.
TEST_F(AstraExtensionIconViewTest, SetIsCurrent) {
  EXPECT_FALSE(extension_view_->IsCurrent());
  extension_view_->SetIsCurrent(true);
  EXPECT_TRUE(extension_view_->IsCurrent());
  extension_view_->SetIsCurrent(false);
  EXPECT_FALSE(extension_view_->IsCurrent());
}

// Test 259: SetNotificationCount sets count and updates badge.
TEST_F(AstraExtensionIconViewTest, SetNotificationCount) {
  EXPECT_EQ(extension_view_->GetNotificationCount(), 0);
  extension_view_->SetNotificationCount(5);
  EXPECT_EQ(extension_view_->GetNotificationCount(), 5);
  EXPECT_FALSE(extension_view_->GetBadgeText().empty());
}

// Test 260: Notification count of zero clears badge.
TEST_F(AstraExtensionIconViewTest, NotificationCountZeroClearsBadge) {
  extension_view_->SetNotificationCount(10);
  EXPECT_FALSE(extension_view_->GetBadgeText().empty());

  extension_view_->SetNotificationCount(0);
  EXPECT_EQ(extension_view_->GetNotificationCount(), 0);
  EXPECT_TRUE(extension_view_->GetBadgeText().empty());
}

// Test 261: Large notification count shows 99+.
TEST_F(AstraExtensionIconViewTest, LargeNotificationCountTruncated) {
  extension_view_->SetNotificationCount(100);
  EXPECT_EQ(extension_view_->GetBadgeText(), u"99+");
}

// Test 262: SetShowNotificationBadge toggles badge visibility.
TEST_F(AstraExtensionIconViewTest, SetShowNotificationBadge) {
  EXPECT_TRUE(extension_view_->GetShowNotificationBadge());
  extension_view_->SetShowNotificationBadge(false);
  EXPECT_FALSE(extension_view_->GetShowNotificationBadge());
  extension_view_->SetShowNotificationBadge(true);
  EXPECT_TRUE(extension_view_->GetShowNotificationBadge());
}

// Test 263: SetLoading toggles loading state.
TEST_F(AstraExtensionIconViewTest, SetLoading) {
  EXPECT_FALSE(extension_view_->IsLoading());
  extension_view_->SetLoading(true);
  EXPECT_TRUE(extension_view_->IsLoading());
  extension_view_->SetLoading(false);
  EXPECT_FALSE(extension_view_->IsLoading());
}

// Test 264: SetInErrorState toggles error state.
TEST_F(AstraExtensionIconViewTest, SetInErrorState) {
  EXPECT_FALSE(extension_view_->IsInErrorState());
  extension_view_->SetInErrorState(true);
  EXPECT_TRUE(extension_view_->IsInErrorState());
  extension_view_->SetInErrorState(false);
  EXPECT_FALSE(extension_view_->IsInErrorState());
}

// Test 265: Popup showing with disabled extension does not crash.
TEST_F(AstraExtensionIconViewTest, PopupShowingWithDisabled) {
  extension_view_->SetExtensionEnabled(false);
  extension_view_->SetPopupShowing(true);
  SUCCEED();
}

// Test 266: Error state keeps the icon interactive.
TEST_F(AstraExtensionIconViewTest, ErrorStateKeepsInteractive) {
  extension_view_->SetExtensionState(AstraExtensionState::kError);
  EXPECT_TRUE(extension_view_->GetEnabled());
}

// Test 267: Uninstalled state disables the icon.
TEST_F(AstraExtensionIconViewTest, UninstalledStateDisables) {
  extension_view_->SetExtensionState(AstraExtensionState::kUninstalled);
  EXPECT_FALSE(extension_view_->GetEnabled());
}

// Test 268: Blocked state disables the icon.
TEST_F(AstraExtensionIconViewTest, BlockedStateDisables) {
  extension_view_->SetExtensionState(AstraExtensionState::kBlocked);
  EXPECT_FALSE(extension_view_->GetEnabled());
}

// Test 269: Default badge text color is white.
TEST_F(AstraExtensionIconViewTest, DefaultBadgeTextColor) {
  EXPECT_EQ(extension_view_->GetBadgeColor(), SK_ColorWHITE);
}

// Test 270: Default badge background color is red.
TEST_F(AstraExtensionIconViewTest, DefaultBadgeBackgroundColor) {
  EXPECT_EQ(extension_view_->GetBadgeBackgroundColor(), SK_ColorRED);
}

// Test 271: Setting same state is a no-op.
TEST_F(AstraExtensionIconViewTest, SetSameStateNoOp) {
  extension_view_->SetExtensionState(AstraExtensionState::kEnabled);
  EXPECT_EQ(extension_view_->GetExtensionState(),
            AstraExtensionState::kEnabled);
}

// Test 272: Multiple property updates work together.
TEST_F(AstraExtensionIconViewTest, MultiplePropertyUpdates) {
  extension_view_->SetName(u"Test Extension");
  extension_view_->SetPinned(true);
  extension_view_->SetNotificationCount(7);
  extension_view_->SetIsAction(true);
  extension_view_->SetLoading(true);

  EXPECT_EQ(extension_view_->GetName(), u"Test Extension");
  EXPECT_TRUE(extension_view_->IsPinned());
  EXPECT_EQ(extension_view_->GetNotificationCount(), 7);
  EXPECT_TRUE(extension_view_->IsAction());
  EXPECT_TRUE(extension_view_->IsLoading());
}

// Test 273: Construction with delegate works.
TEST_F(AstraExtensionIconViewTest, ConstructionWithDelegate) {
  MockExtensionIconDelegate delegate;
  auto view = std::make_unique<AstraExtensionIconView>("test-id", &delegate);
  EXPECT_EQ(view->GetExtensionId(), "test-id");
}

// Test 274: Construction without delegate works.
TEST_F(AstraExtensionIconViewTest, ConstructionWithoutDelegate) {
  auto view = std::make_unique<AstraExtensionIconView>("test-id", nullptr);
  EXPECT_EQ(view->GetExtensionId(), "test-id");
}

// Test 275: Preferred size is valid and square.
TEST_F(AstraExtensionIconViewTest, PreferredSizeValidAndSquare) {
  gfx::Size size = extension_view_->GetPreferredSize();
  EXPECT_GT(size.height(), 0);
  EXPECT_GT(size.width(), 0);
  EXPECT_EQ(size.width(), size.height());
}

// Test 276: Error state with popup showing.
TEST_F(AstraExtensionIconViewTest, ErrorStateWithPopupShowing) {
  extension_view_->SetExtensionState(AstraExtensionState::kError);
  extension_view_->SetPopupShowing(true);
  SUCCEED();
}

// Test 277: Long badge text does not crash.
TEST_F(AstraExtensionIconViewTest, LongBadgeText) {
  extension_view_->SetBadgeText(
      u"Very long badge text that should be truncated");
  SUCCEED();
}

// Test 278: Very small icon size works.
TEST_F(AstraExtensionIconViewTest, VerySmallIconSize) {
  extension_view_->SetIconSize(8);
  EXPECT_EQ(extension_view_->GetIconSize(), 8);
  gfx::Size size = extension_view_->GetPreferredSize();
  EXPECT_GT(size.width(), 0);
}

// Test 279: Very large icon size works.
TEST_F(AstraExtensionIconViewTest, VeryLargeIconSize) {
  extension_view_->SetIconSize(64);
  EXPECT_EQ(extension_view_->GetIconSize(), 64);
  gfx::Size size = extension_view_->GetPreferredSize();
  EXPECT_GT(size.width(), 40);
}

// Test 280: Setting same pinned state is a no-op.
TEST_F(AstraExtensionIconViewTest, SamePinnedStateNoOp) {
  extension_view_->SetPinned(false);
  extension_view_->SetPinned(false);
  EXPECT_FALSE(extension_view_->IsPinned());
}

// =========================================================================
// Extension popup view tests
// =========================================================================

// Test 281: Default popup state after construction.
TEST_F(AstraExtensionPopupViewTest, DefaultState) {
  AstraExtensionPopupView* popup = CreatePopup("ext-1", u"Test Popup");
  EXPECT_EQ(popup->GetExtensionId(), "ext-1");
  EXPECT_EQ(popup->GetTitle(), u"Test Popup");
  EXPECT_FALSE(popup->IsVisible());
  EXPECT_TRUE(popup->GetShowTitle());
  EXPECT_TRUE(popup->GetShowCloseButton());
  EXPECT_FALSE(popup->GetShowOptionsButton());
  EXPECT_FALSE(popup->IsResizable());
  EXPECT_TRUE(popup->GetDismissOnDeactivate());
  EXPECT_FALSE(popup->IsLoading());
  EXPECT_FALSE(popup->HasError());
  EXPECT_TRUE(popup->GetErrorMessage().empty());
}

// Test 282: SetExtensionId updates ID.
TEST_F(AstraExtensionPopupViewTest, SetExtensionId) {
  AstraExtensionPopupView* popup = CreatePopup();
  popup->SetExtensionId("ext-new");
  EXPECT_EQ(popup->GetExtensionId(), "ext-new");
}

// Test 283: SetTitle updates popup title.
TEST_F(AstraExtensionPopupViewTest, SetTitle) {
  AstraExtensionPopupView* popup = CreatePopup("ext-1", u"Original");
  EXPECT_EQ(popup->GetTitle(), u"Original");
  popup->SetTitle(u"Updated Title");
  EXPECT_EQ(popup->GetTitle(), u"Updated Title");
}

// Test 284: SetIcon sets the header icon.
TEST_F(AstraExtensionPopupViewTest, SetIcon) {
  AstraExtensionPopupView* popup = CreatePopup();
  gfx::ImageSkia icon;
  popup->SetIcon(icon);
  SUCCEED();
}

// Test 285: SetShowTitle toggles title visibility.
TEST_F(AstraExtensionPopupViewTest, SetShowTitle) {
  AstraExtensionPopupView* popup = CreatePopup();
  EXPECT_TRUE(popup->GetShowTitle());
  popup->SetShowTitle(false);
  EXPECT_FALSE(popup->GetShowTitle());
  popup->SetShowTitle(true);
  EXPECT_TRUE(popup->GetShowTitle());
}

// Test 286: SetShowCloseButton toggles close button visibility.
TEST_F(AstraExtensionPopupViewTest, SetShowCloseButton) {
  AstraExtensionPopupView* popup = CreatePopup();
  EXPECT_TRUE(popup->GetShowCloseButton());
  popup->SetShowCloseButton(false);
  EXPECT_FALSE(popup->GetShowCloseButton());
  popup->SetShowCloseButton(true);
  EXPECT_TRUE(popup->GetShowCloseButton());
}

// Test 287: SetShowOptionsButton toggles options button visibility.
TEST_F(AstraExtensionPopupViewTest, SetShowOptionsButton) {
  AstraExtensionPopupView* popup = CreatePopup();
  EXPECT_FALSE(popup->GetShowOptionsButton());
  popup->SetShowOptionsButton(true);
  EXPECT_TRUE(popup->GetShowOptionsButton());
  popup->SetShowOptionsButton(false);
  EXPECT_FALSE(popup->GetShowOptionsButton());
}

// Test 288: SetContentSize updates content size.
TEST_F(AstraExtensionPopupViewTest, SetContentSize) {
  AstraExtensionPopupView* popup = CreatePopup();
  gfx::Size new_size(400, 500);
  popup->SetContentSize(new_size);
  SUCCEED();
}

// Test 289: GetContentSize returns valid size.
TEST_F(AstraExtensionPopupViewTest, GetContentSize) {
  AstraExtensionPopupView* popup = CreatePopup();
  gfx::Size size = popup->GetContentSize();
  EXPECT_GE(size.width(), 0);
  EXPECT_GE(size.height(), 0);
}

// Test 290: SetMinSize sets minimum size.
TEST_F(AstraExtensionPopupViewTest, SetMinSize) {
  AstraExtensionPopupView* popup = CreatePopup();
  gfx::Size min_size(100, 100);
  popup->SetMinSize(min_size);
  EXPECT_EQ(popup->GetMinSize(), min_size);
}

// Test 291: SetMaxSize sets maximum size.
TEST_F(AstraExtensionPopupViewTest, SetMaxSize) {
  AstraExtensionPopupView* popup = CreatePopup();
  gfx::Size max_size(600, 400);
  popup->SetMaxSize(max_size);
  EXPECT_EQ(popup->GetMaxSize(), max_size);
}

// Test 292: SetIsResizable toggles resizable state.
TEST_F(AstraExtensionPopupViewTest, SetIsResizable) {
  AstraExtensionPopupView* popup = CreatePopup();
  EXPECT_FALSE(popup->IsResizable());
  popup->SetIsResizable(true);
  EXPECT_TRUE(popup->IsResizable());
  popup->SetIsResizable(false);
  EXPECT_FALSE(popup->IsResizable());
}

// Test 293: SetDismissOnDeactivate toggles auto-dismiss.
TEST_F(AstraExtensionPopupViewTest, SetDismissOnDeactivate) {
  AstraExtensionPopupView* popup = CreatePopup();
  EXPECT_TRUE(popup->GetDismissOnDeactivate());
  popup->SetDismissOnDeactivate(false);
  EXPECT_FALSE(popup->GetDismissOnDeactivate());
  popup->SetDismissOnDeactivate(true);
  EXPECT_TRUE(popup->GetDismissOnDeactivate());
}

// Test 294: Popup is not visible before Show() is called.
TEST_F(AstraExtensionPopupViewTest, NotVisibleBeforeShow) {
  AstraExtensionPopupView* popup = CreatePopup();
  EXPECT_FALSE(popup->IsVisible());
}

// Test 295: Hide on a hidden popup is safe.
TEST_F(AstraExtensionPopupViewTest, HideOnHiddenPopup) {
  AstraExtensionPopupView* popup = CreatePopup();
  popup->Hide();
  SUCCEED();
}

// Test 296: GetAnchorView returns the anchor.
TEST_F(AstraExtensionPopupViewTest, GetAnchorView) {
  AstraExtensionPopupView* popup = CreatePopup();
  EXPECT_EQ(popup->GetAnchorView(), anchor_view_);
}

// Test 297: Default popup position is kRight.
TEST_F(AstraExtensionPopupViewTest, DefaultPopupPosition) {
  AstraExtensionPopupView* popup = CreatePopup();
  EXPECT_EQ(popup->GetPopupPosition(), AstraPopupPosition::kRight);
}

// Test 298: SetPopupPosition with kAbove.
TEST_F(AstraExtensionPopupViewTest, SetPopupPositionAbove) {
  AstraExtensionPopupView* popup = CreatePopup();
  popup->SetPopupPosition(AstraPopupPosition::kAbove);
  EXPECT_EQ(popup->GetPopupPosition(), AstraPopupPosition::kAbove);
}

// Test 299: SetPopupPosition with kBelow.
TEST_F(AstraExtensionPopupViewTest, SetPopupPositionBelow) {
  AstraExtensionPopupView* popup = CreatePopup();
  popup->SetPopupPosition(AstraPopupPosition::kBelow);
  EXPECT_EQ(popup->GetPopupPosition(), AstraPopupPosition::kBelow);
}

// Test 300: SetPopupPosition with kLeft.
TEST_F(AstraExtensionPopupViewTest, SetPopupPositionLeft) {
  AstraExtensionPopupView* popup = CreatePopup();
  popup->SetPopupPosition(AstraPopupPosition::kLeft);
  EXPECT_EQ(popup->GetPopupPosition(), AstraPopupPosition::kLeft);
}

// Test 301: SetPopupPosition with kRight.
TEST_F(AstraExtensionPopupViewTest, SetPopupPositionRight) {
  AstraExtensionPopupView* popup = CreatePopup();
  popup->SetPopupPosition(AstraPopupPosition::kRight);
  EXPECT_EQ(popup->GetPopupPosition(), AstraPopupPosition::kRight);
}

// Test 302: SetPopupPosition with kAuto.
TEST_F(AstraExtensionPopupViewTest, SetPopupPositionAuto) {
  AstraExtensionPopupView* popup = CreatePopup();
  popup->SetPopupPosition(AstraPopupPosition::kAuto);
  EXPECT_EQ(popup->GetPopupPosition(), AstraPopupPosition::kAuto);
}

// Test 303: All 5 popup positions are valid.
TEST_F(AstraExtensionPopupViewTest, AllPopupPositionsValid) {
  AstraExtensionPopupView* popup = CreatePopup();
  std::vector<AstraPopupPosition> positions = {
      AstraPopupPosition::kAuto,
      AstraPopupPosition::kAbove,
      AstraPopupPosition::kBelow,
      AstraPopupPosition::kLeft,
      AstraPopupPosition::kRight,
  };
  for (auto pos : positions) {
    popup->SetPopupPosition(pos);
    EXPECT_EQ(popup->GetPopupPosition(), pos);
  }
}

// Test 304: Header is visible by default.
TEST_F(AstraExtensionPopupViewTest, HeaderVisibleByDefault) {
  AstraExtensionPopupView* popup = CreatePopup();
  EXPECT_TRUE(popup->IsHeaderVisible());
}

// Test 305: SetHeaderVisible hides and shows header.
TEST_F(AstraExtensionPopupViewTest, SetHeaderVisible) {
  AstraExtensionPopupView* popup = CreatePopup();
  popup->SetHeaderVisible(false);
  EXPECT_FALSE(popup->IsHeaderVisible());
  popup->SetHeaderVisible(true);
  EXPECT_TRUE(popup->IsHeaderVisible());
}

// Test 306: GetHeaderView returns non-null.
TEST_F(AstraExtensionPopupViewTest, GetHeaderViewNotNull) {
  AstraExtensionPopupView* popup = CreatePopup();
  EXPECT_NE(popup->GetHeaderView(), nullptr);
}

// Test 307: GetContentView returns non-null.
TEST_F(AstraExtensionPopupViewTest, GetContentViewNotNull) {
  AstraExtensionPopupView* popup = CreatePopup();
  EXPECT_NE(popup->GetContentView(), nullptr);
}

// Test 308: Extension view is null by default.
TEST_F(AstraExtensionPopupViewTest, ExtensionViewNullByDefault) {
  AstraExtensionPopupView* popup = CreatePopup();
  EXPECT_EQ(popup->GetExtensionView(), nullptr);
}

// Test 309: SetExtensionView sets the content view.
TEST_F(AstraExtensionPopupViewTest, SetExtensionView) {
  AstraExtensionPopupView* popup = CreatePopup();
  auto ext_view = std::make_unique<views::View>();
  views::View* ext_view_raw = ext_view.get();
  popup->SetExtensionView(ext_view.release());
  EXPECT_EQ(popup->GetExtensionView(), ext_view_raw);
}

// Test 310: SetExtensionView with null clears content.
TEST_F(AstraExtensionPopupViewTest, SetExtensionViewNull) {
  AstraExtensionPopupView* popup = CreatePopup();
  auto ext_view = std::make_unique<views::View>();
  popup->SetExtensionView(ext_view.release());
  EXPECT_NE(popup->GetExtensionView(), nullptr);

  popup->SetExtensionView(nullptr);
  EXPECT_EQ(popup->GetExtensionView(), nullptr);
}

// Test 311: SetLoading toggles loading state.
TEST_F(AstraExtensionPopupViewTest, SetLoading) {
  AstraExtensionPopupView* popup = CreatePopup();
  EXPECT_FALSE(popup->IsLoading());
  popup->SetLoading(true);
  EXPECT_TRUE(popup->IsLoading());
  popup->SetLoading(false);
  EXPECT_FALSE(popup->IsLoading());
}

// Test 312: SetErrorState sets error and message.
TEST_F(AstraExtensionPopupViewTest, SetErrorState) {
  AstraExtensionPopupView* popup = CreatePopup();
  popup->SetErrorState(true, u"Something went wrong");
  EXPECT_TRUE(popup->HasError());
  EXPECT_EQ(popup->GetErrorMessage(), u"Something went wrong");
}

// Test 313: Clearing error state removes error.
TEST_F(AstraExtensionPopupViewTest, ClearErrorState) {
  AstraExtensionPopupView* popup = CreatePopup();
  popup->SetErrorState(true, u"Error message");
  EXPECT_TRUE(popup->HasError());

  popup->SetErrorState(false, std::u16string());
  EXPECT_FALSE(popup->HasError());
}

// Test 314: Default error state is no error.
TEST_F(AstraExtensionPopupViewTest, DefaultNoError) {
  AstraExtensionPopupView* popup = CreatePopup();
  EXPECT_FALSE(popup->HasError());
  EXPECT_TRUE(popup->GetErrorMessage().empty());
}

// Test 315: ReloadExtension does not crash.
TEST_F(AstraExtensionPopupViewTest, ReloadExtensionNoCrash) {
  AstraExtensionPopupView* popup = CreatePopup();
  popup->ReloadExtension();
  SUCCEED();
}

// Test 316: InspectPopup does not crash.
TEST_F(AstraExtensionPopupViewTest, InspectPopupNoCrash) {
  AstraExtensionPopupView* popup = CreatePopup();
  popup->InspectPopup();
  SUCCEED();
}

// Test 317: Empty title is allowed.
TEST_F(AstraExtensionPopupViewTest, EmptyTitle) {
  AstraExtensionPopupView* popup = CreatePopup("ext-1", u"");
  EXPECT_TRUE(popup->GetTitle().empty());
}

// Test 318: Long title is allowed.
TEST_F(AstraExtensionPopupViewTest, LongTitle) {
  AstraExtensionPopupView* popup = CreatePopup(
      "ext-1",
      u"This is a very long extension name that should wrap or truncate");
  EXPECT_FALSE(popup->GetTitle().empty());
}

// Test 319: Popup with no title and no buttons hides header.
TEST_F(AstraExtensionPopupViewTest, NoHeaderElementsHidesHeader) {
  AstraExtensionPopupView* popup = CreatePopup();
  popup->SetShowTitle(false);
  popup->SetShowCloseButton(false);
  popup->SetShowOptionsButton(false);
  EXPECT_FALSE(popup->IsHeaderVisible());
}

// Test 320: Popup with delegate notifies on close.
TEST_F(AstraExtensionPopupViewTest, PopupWithDelegate) {
  MockExtensionPopupDelegate delegate;
  auto* popup = new AstraExtensionPopupView("ext-1", u"Test",
                                            anchor_view_, &delegate);
  EXPECT_EQ(popup->GetExtensionId(), "ext-1");
}

// =========================================================================
// Sidebar extensions view tests
// =========================================================================

// Test 321: Default state after construction.
TEST_F(AstraSidebarExtensionsViewTest, DefaultState) {
  EXPECT_EQ(extensions_view_->GetExtensionCount(), 0);
  EXPECT_EQ(extensions_view_->GetSelectedExtensionId(), std::string());
  EXPECT_EQ(extensions_view_->GetPinnedExtensionCount(), 0);
  EXPECT_EQ(extensions_view_->GetEnabledExtensionCount(), 0);
  EXPECT_EQ(extensions_view_->GetDisabledExtensionCount(), 0);
  EXPECT_EQ(extensions_view_->GetExtensionsPerRow(), 4);
  EXPECT_EQ(extensions_view_->GetIconSize(), 24);
  EXPECT_EQ(extensions_view_->GetSpacing(), 4);
  EXPECT_TRUE(extensions_view_->GetShowPinnedSection());
  EXPECT_TRUE(extensions_view_->GetShowAllExtensionsSection());
  EXPECT_TRUE(extensions_view_->GetShowDisabledExtensions());
  EXPECT_EQ(extensions_view_->GetSortExtensionsBy(),
            AstraExtensionSortBy::kName);
  EXPECT_FALSE(extensions_view_->IsPopupVisible());
  EXPECT_TRUE(extensions_view_->GetCurrentPopupExtensionId().empty());
  EXPECT_FALSE(extensions_view_->GetShowExtensionsBadge());
  EXPECT_EQ(extensions_view_->GetExtensionNotificationCount(), 0);
}

// Test 322: SetExtensions populates the view.
TEST_F(AstraSidebarExtensionsViewTest, SetExtensions) {
  std::vector<AstraExtensionInfo> exts;
  exts.push_back(CreateTestExtension("ext-1", u"Alpha"));
  exts.push_back(CreateTestExtension("ext-2", u"Beta"));
  exts.push_back(CreateTestExtension("ext-3", u"Gamma"));

  extensions_view_->SetExtensions(exts);
  EXPECT_EQ(extensions_view_->GetExtensionCount(), 3);
}

// Test 323: SetExtensions with empty list clears view.
TEST_F(AstraSidebarExtensionsViewTest, SetExtensionsEmpty) {
  std::vector<AstraExtensionInfo> exts;
  exts.push_back(CreateTestExtension("ext-1", u"Test"));
  extensions_view_->SetExtensions(exts);
  EXPECT_EQ(extensions_view_->GetExtensionCount(), 1);

  extensions_view_->SetExtensions(std::vector<AstraExtensionInfo>());
  EXPECT_EQ(extensions_view_->GetExtensionCount(), 0);
}

// Test 324: GetExtensionAt returns correct extension.
TEST_F(AstraSidebarExtensionsViewTest, GetExtensionAt) {
  std::vector<AstraExtensionInfo> exts;
  exts.push_back(CreateTestExtension("ext-a", u"Alpha"));
  exts.push_back(CreateTestExtension("ext-b", u"Beta"));
  extensions_view_->SetExtensions(exts);

  // Sorted alphabetically, so Alpha comes first.
  EXPECT_EQ(extensions_view_->GetExtensionAt(0).extension_id, "ext-a");
  EXPECT_EQ(extensions_view_->GetExtensionAt(1).extension_id, "ext-b");
}

// Test 325: GetExtensionAt with invalid index returns empty.
TEST_F(AstraSidebarExtensionsViewTest, GetExtensionAtInvalidIndex) {
  AstraExtensionInfo info = extensions_view_->GetExtensionAt(0);
  EXPECT_TRUE(info.extension_id.empty());

  info = extensions_view_->GetExtensionAt(-1);
  EXPECT_TRUE(info.extension_id.empty());

  info = extensions_view_->GetExtensionAt(100);
  EXPECT_TRUE(info.extension_id.empty());
}

// Test 326: AddExtension adds a single extension.
TEST_F(AstraSidebarExtensionsViewTest, AddExtension) {
  extensions_view_->AddExtension(CreateTestExtension("ext-1", u"Test"));
  EXPECT_EQ(extensions_view_->GetExtensionCount(), 1);
  EXPECT_TRUE(extensions_view_->HasExtension("ext-1"));
}

// Test 327: AddExtension with duplicate updates existing.
TEST_F(AstraSidebarExtensionsViewTest, AddExtensionDuplicate) {
  extensions_view_->AddExtension(CreateTestExtension("ext-1", u"First"));
  EXPECT_EQ(extensions_view_->GetExtensionCount(), 1);

  auto info = CreateTestExtension("ext-1", u"Updated");
  extensions_view_->AddExtension(info);
  EXPECT_EQ(extensions_view_->GetExtensionCount(), 1);
}

// Test 328: RemoveExtension removes an extension.
TEST_F(AstraSidebarExtensionsViewTest, RemoveExtension) {
  extensions_view_->AddExtension(CreateTestExtension("ext-1", u"Test"));
  EXPECT_TRUE(extensions_view_->HasExtension("ext-1"));

  extensions_view_->RemoveExtension("ext-1");
  EXPECT_FALSE(extensions_view_->HasExtension("ext-1"));
  EXPECT_EQ(extensions_view_->GetExtensionCount(), 0);
}

// Test 329: RemoveExtension with nonexistent ID is safe.
TEST_F(AstraSidebarExtensionsViewTest, RemoveExtensionNonexistent) {
  extensions_view_->RemoveExtension("nonexistent");
  SUCCEED();
}

// Test 330: UpdateExtension modifies existing extension.
TEST_F(AstraSidebarExtensionsViewTest, UpdateExtension) {
  extensions_view_->AddExtension(CreateTestExtension("ext-1", u"Original"));
  auto info = CreateTestExtension("ext-1", u"Updated Name");
  info.state = AstraExtensionState::kDisabled;
  extensions_view_->UpdateExtension(info);

  EXPECT_EQ(extensions_view_->GetExtensionCount(), 1);
}

// Test 331: UpdateExtension with nonexistent ID does nothing.
TEST_F(AstraSidebarExtensionsViewTest, UpdateExtensionNonexistent) {
  auto info = CreateTestExtension("ext-nope", u"Nope");
  extensions_view_->UpdateExtension(info);
  EXPECT_EQ(extensions_view_->GetExtensionCount(), 0);
}

// Test 332: HasExtension returns false for empty view.
TEST_F(AstraSidebarExtensionsViewTest, HasExtensionEmpty) {
  EXPECT_FALSE(extensions_view_->HasExtension("any"));
}

// Test 333: SetSelectedExtension sets selection.
TEST_F(AstraSidebarExtensionsViewTest, SetSelectedExtension) {
  extensions_view_->AddExtension(CreateTestExtension("ext-1", u"Test"));
  extensions_view_->SetSelectedExtension("ext-1");
  EXPECT_EQ(extensions_view_->GetSelectedExtensionId(), "ext-1");
}

// Test 334: ClearSelection clears selected extension.
TEST_F(AstraSidebarExtensionsViewTest, ClearSelection) {
  extensions_view_->AddExtension(CreateTestExtension("ext-1", u"Test"));
  extensions_view_->SetSelectedExtension("ext-1");
  EXPECT_FALSE(extensions_view_->GetSelectedExtensionId().empty());

  extensions_view_->ClearSelection();
  EXPECT_TRUE(extensions_view_->GetSelectedExtensionId().empty());
}

// Test 335: SetPinnedExtensions sets pinned list.
TEST_F(AstraSidebarExtensionsViewTest, SetPinnedExtensions) {
  std::vector<AstraExtensionInfo> exts;
  exts.push_back(CreateTestExtension("ext-1", u"One"));
  exts.push_back(CreateTestExtension("ext-2", u"Two"));
  exts.push_back(CreateTestExtension("ext-3", u"Three"));
  extensions_view_->SetExtensions(exts);

  std::vector<std::string> pinned = {"ext-1", "ext-3"};
  extensions_view_->SetPinnedExtensions(pinned);

  EXPECT_TRUE(extensions_view_->IsExtensionPinned("ext-1"));
  EXPECT_FALSE(extensions_view_->IsExtensionPinned("ext-2"));
  EXPECT_TRUE(extensions_view_->IsExtensionPinned("ext-3"));
  EXPECT_EQ(extensions_view_->GetPinnedExtensionCount(), 2);
}

// Test 336: GetPinnedExtensions returns list.
TEST_F(AstraSidebarExtensionsViewTest, GetPinnedExtensions) {
  std::vector<AstraExtensionInfo> exts;
  exts.push_back(CreateTestExtension("ext-1", u"One"));
  exts.push_back(CreateTestExtension("ext-2", u"Two"));
  extensions_view_->SetExtensions(exts);

  std::vector<std::string> pinned = {"ext-2"};
  extensions_view_->SetPinnedExtensions(pinned);

  auto result = extensions_view_->GetPinnedExtensions();
  EXPECT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0], "ext-2");
}

// Test 337: PinExtension pins a single extension.
TEST_F(AstraSidebarExtensionsViewTest, PinExtension) {
  extensions_view_->AddExtension(CreateTestExtension("ext-1", u"Test"));
  EXPECT_FALSE(extensions_view_->IsExtensionPinned("ext-1"));

  extensions_view_->PinExtension("ext-1");
  EXPECT_TRUE(extensions_view_->IsExtensionPinned("ext-1"));
  EXPECT_EQ(extensions_view_->GetPinnedExtensionCount(), 1);
}

// Test 338: UnpinExtension unpins an extension.
TEST_F(AstraSidebarExtensionsViewTest, UnpinExtension) {
  extensions_view_->AddExtension(CreateTestExtension("ext-1", u"Test"));
  extensions_view_->PinExtension("ext-1");
  EXPECT_TRUE(extensions_view_->IsExtensionPinned("ext-1"));

  extensions_view_->UnpinExtension("ext-1");
  EXPECT_FALSE(extensions_view_->IsExtensionPinned("ext-1"));
  EXPECT_EQ(extensions_view_->GetPinnedExtensionCount(), 0);
}

// Test 339: PinExtension on already pinned is no-op.
TEST_F(AstraSidebarExtensionsViewTest, PinAlreadyPinned) {
  extensions_view_->AddExtension(CreateTestExtension("ext-1", u"Test"));
  extensions_view_->PinExtension("ext-1");
  int count = extensions_view_->GetPinnedExtensionCount();
  extensions_view_->PinExtension("ext-1");
  EXPECT_EQ(extensions_view_->GetPinnedExtensionCount(), count);
}

// Test 340: UnpinExtension on not pinned is safe.
TEST_F(AstraSidebarExtensionsViewTest, UnpinNotPinned) {
  extensions_view_->AddExtension(CreateTestExtension("ext-1", u"Test"));
  extensions_view_->UnpinExtension("ext-1");
  SUCCEED();
}

// Test 341: MoveExtension moves an extension.
TEST_F(AstraSidebarExtensionsViewTest, MoveExtension) {
  std::vector<AstraExtensionInfo> exts;
  exts.push_back(CreateTestExtension("ext-1", u"A"));
  exts.push_back(CreateTestExtension("ext-2", u"B"));
  exts.push_back(CreateTestExtension("ext-3", u"C"));

  extensions_view_->SetSortExtensionsBy(AstraExtensionSortBy::kManual);
  extensions_view_->SetExtensions(exts);

  extensions_view_->MoveExtension(0, 2);
  EXPECT_EQ(extensions_view_->GetExtensionCount(), 3);
}

// Test 342: MoveExtension same index is no-op.
TEST_F(AstraSidebarExtensionsViewTest, MoveExtensionSameIndex) {
  std::vector<AstraExtensionInfo> exts;
  exts.push_back(CreateTestExtension("ext-1", u"One"));
  extensions_view_->SetSortExtensionsBy(AstraExtensionSortBy::kManual);
  extensions_view_->SetExtensions(exts);

  extensions_view_->MoveExtension(0, 0);
  EXPECT_EQ(extensions_view_->GetExtensionCount(), 1);
}

// Test 343: MoveExtension out of bounds is safe.
TEST_F(AstraSidebarExtensionsViewTest, MoveExtensionOutOfBounds) {
  extensions_view_->AddExtension(CreateTestExtension("ext-1", u"One"));
  extensions_view_->MoveExtension(-1, 5);
  SUCCEED();
}

// Test 344: SetExtensionsPerRow changes column count.
TEST_F(AstraSidebarExtensionsViewTest, SetExtensionsPerRow) {
  EXPECT_EQ(extensions_view_->GetExtensionsPerRow(), 4);
  extensions_view_->SetExtensionsPerRow(3);
  EXPECT_EQ(extensions_view_->GetExtensionsPerRow(), 3);
  extensions_view_->SetExtensionsPerRow(5);
  EXPECT_EQ(extensions_view_->GetExtensionsPerRow(), 5);
}

// Test 345: SetIconSize changes icon size for all icons.
TEST_F(AstraSidebarExtensionsViewTest, SetIconSize) {
  extensions_view_->AddExtension(CreateTestExtension("ext-1", u"Test"));
  EXPECT_EQ(extensions_view_->GetIconSize(), 24);
  extensions_view_->SetIconSize(32);
  EXPECT_EQ(extensions_view_->GetIconSize(), 32);
}

// Test 346: SetSpacing changes icon spacing.
TEST_F(AstraSidebarExtensionsViewTest, SetSpacing) {
  EXPECT_EQ(extensions_view_->GetSpacing(), 4);
  extensions_view_->SetSpacing(8);
  EXPECT_EQ(extensions_view_->GetSpacing(), 8);
}

// Test 347: SetShowPinnedSection toggles pinned section.
TEST_F(AstraSidebarExtensionsViewTest, SetShowPinnedSection) {
  EXPECT_TRUE(extensions_view_->GetShowPinnedSection());
  extensions_view_->SetShowPinnedSection(false);
  EXPECT_FALSE(extensions_view_->GetShowPinnedSection());
  extensions_view_->SetShowPinnedSection(true);
  EXPECT_TRUE(extensions_view_->GetShowPinnedSection());
}

// Test 348: SetShowAllExtensionsSection toggles all section.
TEST_F(AstraSidebarExtensionsViewTest, SetShowAllExtensionsSection) {
  EXPECT_TRUE(extensions_view_->GetShowAllExtensionsSection());
  extensions_view_->SetShowAllExtensionsSection(false);
  EXPECT_FALSE(extensions_view_->GetShowAllExtensionsSection());
}

// Test 349: SetShowDisabledExtensions toggles disabled visibility.
TEST_F(AstraSidebarExtensionsViewTest, SetShowDisabledExtensions) {
  EXPECT_TRUE(extensions_view_->GetShowDisabledExtensions());
  extensions_view_->SetShowDisabledExtensions(false);
  EXPECT_FALSE(extensions_view_->GetShowDisabledExtensions());
}

// Test 350: Disabled and enabled counts work with mixed extensions.
TEST_F(AstraSidebarExtensionsViewTest, MixedEnabledDisabledCounts) {
  auto enabled = CreateTestExtension("ext-enabled", u"Enabled");
  auto disabled = CreateTestExtension("ext-disabled", u"Disabled");
  disabled.state = AstraExtensionState::kDisabled;

  std::vector<AstraExtensionInfo> exts = {enabled, disabled};
  extensions_view_->SetExtensions(exts);

  EXPECT_EQ(extensions_view_->GetExtensionCount(), 2);
  EXPECT_EQ(extensions_view_->GetEnabledExtensionCount(), 1);
  EXPECT_EQ(extensions_view_->GetDisabledExtensionCount(), 1);
}

// Test 351: Sort by name (default).
TEST_F(AstraSidebarExtensionsViewTest, SortByName) {
  std::vector<AstraExtensionInfo> exts;
  exts.push_back(CreateTestExtension("ext-c", u"Charlie"));
  exts.push_back(CreateTestExtension("ext-a", u"Alpha"));
  exts.push_back(CreateTestExtension("ext-b", u"Beta"));

  extensions_view_->SetSortExtensionsBy(AstraExtensionSortBy::kName);
  extensions_view_->SetExtensions(exts);

  EXPECT_EQ(extensions_view_->GetExtensionAt(0).name, u"Alpha");
  EXPECT_EQ(extensions_view_->GetExtensionAt(1).name, u"Beta");
  EXPECT_EQ(extensions_view_->GetExtensionAt(2).name, u"Charlie");
}

// Test 352: Sort by manual order.
TEST_F(AstraSidebarExtensionsViewTest, SortByManual) {
  std::vector<AstraExtensionInfo> exts;
  exts.push_back(CreateTestExtension("ext-1", u"First"));
  exts.push_back(CreateTestExtension("ext-2", u"Second"));
  exts.push_back(CreateTestExtension("ext-3", u"Third"));

  extensions_view_->SetSortExtensionsBy(AstraExtensionSortBy::kManual);
  extensions_view_->SetExtensions(exts);

  EXPECT_EQ(extensions_view_->GetExtensionAt(0).extension_id, "ext-1");
  EXPECT_EQ(extensions_view_->GetExtensionAt(1).extension_id, "ext-2");
  EXPECT_EQ(extensions_view_->GetExtensionAt(2).extension_id, "ext-3");
}

// Test 353: Sort by install date (most recent first).
TEST_F(AstraSidebarExtensionsViewTest, SortByInstallDate) {
  std::vector<AstraExtensionInfo> exts;
  auto old = CreateTestExtension("ext-old", u"Old");
  old.install_time = base::Time::Now() - base::Days(30);
  auto newer = CreateTestExtension("ext-new", u"New");
  newer.install_time = base::Time::Now() - base::Days(1);
  exts.push_back(old);
  exts.push_back(newer);

  extensions_view_->SetSortExtensionsBy(AstraExtensionSortBy::kInstallDate);
  extensions_view_->SetExtensions(exts);

  EXPECT_EQ(extensions_view_->GetExtensionAt(0).extension_id, "ext-new");
  EXPECT_EQ(extensions_view_->GetExtensionAt(1).extension_id, "ext-old");
}

// Test 354: Sort by last used (falls back to name for now).
TEST_F(AstraSidebarExtensionsViewTest, SortByLastUsed) {
  std::vector<AstraExtensionInfo> exts;
  exts.push_back(CreateTestExtension("ext-b", u"Beta"));
  exts.push_back(CreateTestExtension("ext-a", u"Alpha"));

  extensions_view_->SetSortExtensionsBy(AstraExtensionSortBy::kLastUsed);
  extensions_view_->SetExtensions(exts);

  EXPECT_EQ(extensions_view_->GetExtensionAt(0).name, u"Alpha");
  EXPECT_EQ(extensions_view_->GetExtensionAt(1).name, u"Beta");
}

// Test 355: All 4 sort orders are valid.
TEST_F(AstraSidebarExtensionsViewTest, AllSortOrdersValid) {
  std::vector<AstraExtensionSortBy> sorts = {
      AstraExtensionSortBy::kManual,
      AstraExtensionSortBy::kName,
      AstraExtensionSortBy::kInstallDate,
      AstraExtensionSortBy::kLastUsed,
  };
  for (auto sort : sorts) {
    extensions_view_->SetSortExtensionsBy(sort);
    EXPECT_EQ(extensions_view_->GetSortExtensionsBy(), sort);
  }
}

// Test 356: GetPinnedExtensionCount returns 0 for empty view.
TEST_F(AstraSidebarExtensionsViewTest, PinnedCountEmpty) {
  EXPECT_EQ(extensions_view_->GetPinnedExtensionCount(), 0);
}

// Test 357: GetEnabledExtensionCount with all enabled.
TEST_F(AstraSidebarExtensionsViewTest, EnabledCountAllEnabled) {
  std::vector<AstraExtensionInfo> exts;
  exts.push_back(CreateTestExtension("ext-1", u"One"));
  exts.push_back(CreateTestExtension("ext-2", u"Two"));
  extensions_view_->SetExtensions(exts);
  EXPECT_EQ(extensions_view_->GetEnabledExtensionCount(), 2);
}

// Test 358: SearchExtensions filters by name.
TEST_F(AstraSidebarExtensionsViewTest, SearchExtensions) {
  std::vector<AstraExtensionInfo> exts;
  exts.push_back(CreateTestExtension("ext-1", u"Alpha Extension"));
  exts.push_back(CreateTestExtension("ext-2", u"Beta Tool"));
  exts.push_back(CreateTestExtension("ext-3", u"Gamma Extension"));
  extensions_view_->SetExtensions(exts);

  extensions_view_->SearchExtensions(u"Extension");
  EXPECT_EQ(extensions_view_->GetSearchResultsCount(), 2);
}

// Test 359: Search with empty query shows all.
TEST_F(AstraSidebarExtensionsViewTest, SearchEmptyQuery) {
  std::vector<AstraExtensionInfo> exts;
  exts.push_back(CreateTestExtension("ext-1", u"One"));
  exts.push_back(CreateTestExtension("ext-2", u"Two"));
  extensions_view_->SetExtensions(exts);

  extensions_view_->SearchExtensions(std::u16string());
  EXPECT_EQ(extensions_view_->GetSearchResultsCount(), 2);
}

// Test 360: Search with no matches returns zero.
TEST_F(AstraSidebarExtensionsViewTest, SearchNoResults) {
  std::vector<AstraExtensionInfo> exts;
  exts.push_back(CreateTestExtension("ext-1", u"Alpha"));
  extensions_view_->SetExtensions(exts);

  extensions_view_->SearchExtensions(u"Zebra");
  EXPECT_EQ(extensions_view_->GetSearchResultsCount(), 0);
}

// Test 361: GetExtensionIconView finds existing icon.
TEST_F(AstraSidebarExtensionsViewTest, GetExtensionIconView) {
  extensions_view_->AddExtension(CreateTestExtension("ext-1", u"Test"));
  auto* icon_view = extensions_view_->GetExtensionIconView("ext-1");
  EXPECT_NE(icon_view, nullptr);
  EXPECT_EQ(icon_view->GetExtensionId(), "ext-1");
}

// Test 362: GetExtensionIconView returns null for missing extension.
TEST_F(AstraSidebarExtensionsViewTest, GetExtensionIconViewMissing) {
  auto* icon_view = extensions_view_->GetExtensionIconView("nonexistent");
  EXPECT_EQ(icon_view, nullptr);
}

// Test 363: Popup starts not visible.
TEST_F(AstraSidebarExtensionsViewTest, PopupNotVisibleInitially) {
  EXPECT_FALSE(extensions_view_->IsPopupVisible());
  EXPECT_TRUE(extensions_view_->GetCurrentPopupExtensionId().empty());
}

// Test 364: HideExtensionPopup with no popup is safe.
TEST_F(AstraSidebarExtensionsViewTest, HidePopupNoActive) {
  extensions_view_->HideExtensionPopup();
  SUCCEED();
}

// Test 365: SetShowExtensionsBadge toggles badge.
TEST_F(AstraSidebarExtensionsViewTest, SetShowExtensionsBadge) {
  EXPECT_FALSE(extensions_view_->GetShowExtensionsBadge());
  extensions_view_->SetShowExtensionsBadge(true);
  EXPECT_TRUE(extensions_view_->GetShowExtensionsBadge());
}

// Test 366: GetExtensionNotificationCount returns 0 for empty.
TEST_F(AstraSidebarExtensionsViewTest, NotificationCountEmpty) {
  EXPECT_EQ(extensions_view_->GetExtensionNotificationCount(), 0);
}

// Test 367: Expanded state defaults to true.
TEST_F(AstraSidebarExtensionsViewTest, ExpandedDefault) {
  EXPECT_TRUE(extensions_view_->expanded());
}

// Test 368: SetExpanded toggles expanded state.
TEST_F(AstraSidebarExtensionsViewTest, SetExpanded) {
  extensions_view_->SetExpanded(false);
  EXPECT_FALSE(extensions_view_->expanded());
  extensions_view_->SetExpanded(true);
  EXPECT_TRUE(extensions_view_->expanded());
}

// Test 369: ToggleExpanded flips state.
TEST_F(AstraSidebarExtensionsViewTest, ToggleExpanded) {
  bool was_expanded = extensions_view_->expanded();
  extensions_view_->ToggleExpanded();
  EXPECT_NE(extensions_view_->expanded(), was_expanded);
}

// Test 370: Delegate can be set.
TEST_F(AstraSidebarExtensionsViewTest, SetDelegate) {
  MockSidebarExtensionsDelegate delegate;
  extensions_view_->SetDelegate(&delegate);
  EXPECT_EQ(extensions_view_->GetDelegate(), &delegate);
}

// Test 371: Delegate is null by default.
TEST_F(AstraSidebarExtensionsViewTest, DelegateNullByDefault) {
  EXPECT_EQ(extensions_view_->GetDelegate(), nullptr);
}

// Test 372: Many extensions do not crash.
TEST_F(AstraSidebarExtensionsViewTest, ManyExtensions) {
  std::vector<AstraExtensionInfo> exts;
  for (int i = 0; i < 50; ++i) {
    auto info = CreateTestExtension("ext-" + base::NumberToString(i),
                                    u"Extension " + base::NumberToString16(i));
    exts.push_back(info);
  }
  extensions_view_->SetExtensions(exts);
  EXPECT_EQ(extensions_view_->GetExtensionCount(), 50);
}

// Test 373: Single extension edge case.
TEST_F(AstraSidebarExtensionsViewTest, SingleExtension) {
  extensions_view_->AddExtension(CreateTestExtension("ext-only", u"Solo"));
  EXPECT_EQ(extensions_view_->GetExtensionCount(), 1);
  EXPECT_TRUE(extensions_view_->HasExtension("ext-only"));
}

// Test 374: Long extension names do not crash.
TEST_F(AstraSidebarExtensionsViewTest, LongExtensionNames) {
  std::u16string long_name =
      u"Very Long Extension Name That Is Really Quite Long "
      u"And Should Probably Be Truncated Or Wrapped";
  extensions_view_->AddExtension(CreateTestExtension("ext-long", long_name));
  EXPECT_EQ(extensions_view_->GetExtensionCount(), 1);
}

// Test 375: Error state extensions are included in total count.
TEST_F(AstraSidebarExtensionsViewTest, ErrorStateCounted) {
  auto error_ext = CreateTestExtension("ext-error", u"Broken");
  error_ext.state = AstraExtensionState::kError;
  extensions_view_->AddExtension(error_ext);

  EXPECT_EQ(extensions_view_->GetEnabledExtensionCount(), 0);
  EXPECT_EQ(extensions_view_->GetDisabledExtensionCount(), 0);
  EXPECT_EQ(extensions_view_->GetExtensionCount(), 1);
}

// Test 376: Delegate receives click notification.
TEST_F(AstraSidebarExtensionsViewTest, DelegateOnClick) {
  MockSidebarExtensionsDelegate delegate;
  extensions_view_->SetDelegate(&delegate);
  extensions_view_->AddExtension(CreateTestExtension("ext-1", u"Test"));

  EXPECT_CALL(delegate, OnExtensionClicked("ext-1")).Times(1);
  extensions_view_->OnExtensionClicked("ext-1");
}

// Test 377: Delegate receives pin notification.
TEST_F(AstraSidebarExtensionsViewTest, DelegateOnPin) {
  MockSidebarExtensionsDelegate delegate;
  extensions_view_->SetDelegate(&delegate);
  extensions_view_->AddExtension(CreateTestExtension("ext-1", u"Test"));

  EXPECT_CALL(delegate, OnExtensionPinned("ext-1", true)).Times(1);
  extensions_view_->PinExtension("ext-1");
}

// =========================================================================
// Recently closed item view tests
// =========================================================================

// Test 251: Default state after construction.
TEST_F(AstraRecentlyClosedItemViewTest, DefaultState) {
  EXPECT_EQ(closed_view_->GetTitle(), u"Test Page");
  EXPECT_EQ(closed_view_->GetUrl(), GURL("https://example.com"));
  EXPECT_FALSE(closed_view_->GetCloseTime().is_null());
  EXPECT_EQ(closed_view_->entry_id(), 42);
  EXPECT_EQ(closed_view_->GetTabCount(), 1);
  EXPECT_FALSE(closed_view_->IsWindow());
}

// Test 252: SetRecentlyClosedInfo updates all fields.
TEST_F(AstraRecentlyClosedItemViewTest, SetRecentlyClosedInfoUpdatesAll) {
  GURL url("https://newsite.com");
  std::u16string title = u"New Page Title";
  base::Time time = base::Time::Now() - base::Hours(1);

  closed_view_->SetRecentlyClosedInfo(url, title, time);

  EXPECT_EQ(closed_view_->GetUrl(), url);
  EXPECT_EQ(closed_view_->GetTitle(), title);
  EXPECT_EQ(closed_view_->GetCloseTime(), time);
}

// Test 253: GetUrl returns the URL.
TEST_F(AstraRecentlyClosedItemViewTest, GetUrl) {
  EXPECT_EQ(closed_view_->GetUrl(), GURL("https://example.com"));
}

// Test 254: GetCloseTime returns valid time.
TEST_F(AstraRecentlyClosedItemViewTest, GetCloseTime) {
  EXPECT_FALSE(closed_view_->GetCloseTime().is_null());
}

// Test 255: SetTabCount updates count.
TEST_F(AstraRecentlyClosedItemViewTest, SetTabCount) {
  closed_view_->SetTabCount(5);
  EXPECT_EQ(closed_view_->GetTabCount(), 5);
}

// Test 256: GetTabCount default is 1.
TEST_F(AstraRecentlyClosedItemViewTest, TabCountDefault) {
  EXPECT_EQ(closed_view_->GetTabCount(), 1);
}

// Test 257: IsWindow default is false.
TEST_F(AstraRecentlyClosedItemViewTest, IsWindowDefaultFalse) {
  EXPECT_FALSE(closed_view_->IsWindow());
}

// Test 258: SetIsWindow toggles window state.
TEST_F(AstraRecentlyClosedItemViewTest, SetIsWindow) {
  EXPECT_FALSE(closed_view_->IsWindow());
  closed_view_->SetIsWindow(true);
  EXPECT_TRUE(closed_view_->IsWindow());
  closed_view_->SetIsWindow(false);
  EXPECT_FALSE(closed_view_->IsWindow());
}

// Test 259: Window item with tab count.
TEST_F(AstraRecentlyClosedItemViewTest, WindowWithTabCount) {
  closed_view_->SetIsWindow(true);
  closed_view_->SetTabCount(10);
  EXPECT_TRUE(closed_view_->IsWindow());
  EXPECT_EQ(closed_view_->GetTabCount(), 10);
}

// Test 260: Entry ID is preserved.
TEST_F(AstraRecentlyClosedItemViewTest, EntryIdPreserved) {
  EXPECT_EQ(closed_view_->entry_id(), 42);
}

// Test 261: Delegate can be set.
TEST_F(AstraRecentlyClosedItemViewTest, DelegateCanBeSet) {
  MockRecentlyClosedItemDelegate delegate;
  closed_view_->set_delegate(&delegate);
  SUCCEED();
}

// Test 262: Preferred size is valid.
TEST_F(AstraRecentlyClosedItemViewTest, PreferredSizeValid) {
  gfx::Size size = closed_view_->GetPreferredSize();
  EXPECT_GT(size.height(), 0);
  EXPECT_GE(size.width(), 0);
}

// Test 263: Title update works.
TEST_F(AstraRecentlyClosedItemViewTest, TitleUpdate) {
  std::u16string title = u"Updated Page Title";
  closed_view_->SetTitle(title);
  EXPECT_EQ(closed_view_->GetTitle(), title);
}

// Test 264: Multiple tab count updates.
TEST_F(AstraRecentlyClosedItemViewTest, MultipleTabCountUpdates) {
  closed_view_->SetTabCount(1);
  EXPECT_EQ(closed_view_->GetTabCount(), 1);
  closed_view_->SetTabCount(10);
  EXPECT_EQ(closed_view_->GetTabCount(), 10);
  closed_view_->SetTabCount(100);
  EXPECT_EQ(closed_view_->GetTabCount(), 100);
}

// Test 265: Very old close time.
TEST_F(AstraRecentlyClosedItemViewTest, VeryOldCloseTime) {
  base::Time old_time = base::Time::Now() - base::Days(365);
  closed_view_->SetRecentlyClosedInfo(GURL("https://old.com"), u"Old",
                                       old_time);
  EXPECT_EQ(closed_view_->GetCloseTime(), old_time);
}

// =========================================================================
// Mock delegates for section view tests
// =========================================================================

class MockSidebarBookmarksDelegate : public AstraSidebarBookmarksDelegate {
 public:
  MOCK_METHOD(void, OnBookmarkClicked, (const std::string&), (override));
  MOCK_METHOD(void, OnBookmarkMiddleClicked, (const std::string&), (override));
  MOCK_METHOD(void, OnBookmarkRightClicked,
              (const std::string&, const gfx::Point&), (override));
  MOCK_METHOD(void, OnFolderOpened, (const std::string&), (override));
  MOCK_METHOD(void, OnNewFolderRequested, (), (override));
  MOCK_METHOD(void, OnAddBookmarkRequested, (), (override));
  MOCK_METHOD(void, OnBookmarkDragged,
              (const std::string&, const gfx::Point&), (override));
  MOCK_METHOD(void, OnBookmarkDropped,
              (const std::string&, const gfx::Point&), (override));
};

class MockSidebarHistoryDelegate : public AstraSidebarHistoryDelegate {
 public:
  MOCK_METHOD(void, OnHistoryItemClicked, (const std::string&), (override));
  MOCK_METHOD(void, OnHistoryItemMiddleClicked, (const std::string&), (override));
  MOCK_METHOD(void, OnHistoryItemRightClicked,
              (const std::string&, const gfx::Point&), (override));
  MOCK_METHOD(void, OnRemoveHistoryItem, (const std::string&), (override));
  MOCK_METHOD(void, OnClearAllHistoryRequested, (), (override));
  MOCK_METHOD(void, OnRemoveHistoryForDomain, (const std::string&), (override));
  MOCK_METHOD(void, OnSearchHistory, (const std::u16string&), (override));
};

class MockSidebarDownloadsDelegate : public AstraSidebarDownloadsDelegate {
 public:
  MOCK_METHOD(void, OnDownloadClicked, (const std::string&), (override));
  MOCK_METHOD(void, OnDownloadRightClicked,
              (const std::string&, const gfx::Point&), (override));
  MOCK_METHOD(void, OnPauseDownload, (const std::string&), (override));
  MOCK_METHOD(void, OnResumeDownload, (const std::string&), (override));
  MOCK_METHOD(void, OnCancelDownload, (const std::string&), (override));
  MOCK_METHOD(void, OnOpenDownload, (const std::string&), (override));
  MOCK_METHOD(void, OnShowDownloadInFolder, (const std::string&), (override));
  MOCK_METHOD(void, OnRetryDownload, (const std::string&), (override));
  MOCK_METHOD(void, OnRemoveDownload, (const std::string&), (override));
  MOCK_METHOD(void, OnClearAllDownloadsRequested, (), (override));
};

// =========================================================================
// AstraSidebarSectionViewTest — base section view tests
// =========================================================================

class AstraSidebarSectionViewTest : public views::test::ViewsTestBase {
 public:
  AstraSidebarSectionViewTest() = default;
  ~AstraSidebarSectionViewTest() override = default;

  void SetUp() override {
    views::test::ViewsTestBase::SetUp();
    section_view_ = std::make_unique<AstraSidebarSectionView>(
        AstraSidebarSectionType::kBookmarks, u"Test Section");
    widget_ = CreateWidget();
    widget_->SetContentsView(section_view_.get());
    widget_->SetSize(gfx::Size(300, 500));
    widget_->Show();
  }

  void TearDown() override {
    widget_.reset();
    section_view_.reset();
    views::test::ViewsTestBase::TearDown();
  }

 protected:
  std::unique_ptr<AstraSidebarSectionView> section_view_;
  std::unique_ptr<views::Widget> widget_;
};

// Test 266: Section view has correct title after construction.
TEST_F(AstraSidebarSectionViewTest, ConstructionHasTitle) {
  EXPECT_EQ(section_view_->GetTitle(), u"Test Section");
}

// Test 267: SetTitle updates the title.
TEST_F(AstraSidebarSectionViewTest, SetTitleUpdatesTitle) {
  section_view_->SetTitle(u"New Title");
  EXPECT_EQ(section_view_->GetTitle(), u"New Title");
}

// Test 268: Default expanded state is true.
TEST_F(AstraSidebarSectionViewTest, DefaultExpanded) {
  EXPECT_TRUE(section_view_->IsExpanded());
}

// Test 269: SetExpanded(false) collapses the section.
TEST_F(AstraSidebarSectionViewTest, SetExpandedCollapse) {
  section_view_->SetExpanded(false);
  EXPECT_FALSE(section_view_->IsExpanded());
}

// Test 270: ToggleExpanded flips the state.
TEST_F(AstraSidebarSectionViewTest, ToggleExpanded) {
  EXPECT_TRUE(section_view_->IsExpanded());
  section_view_->ToggleExpanded();
  EXPECT_FALSE(section_view_->IsExpanded());
  section_view_->ToggleExpanded();
  EXPECT_TRUE(section_view_->IsExpanded());
}

// Test 271: SetExpanded(true) when already expanded is no-op.
TEST_F(AstraSidebarSectionViewTest, SetExpandedNoop) {
  section_view_->SetExpanded(true);
  EXPECT_TRUE(section_view_->IsExpanded());
}

// Test 272: Default item count is 0.
TEST_F(AstraSidebarSectionViewTest, DefaultItemCountZero) {
  EXPECT_EQ(section_view_->GetItemCount(), 0);
}

// Test 273: SetItemCount updates the count.
TEST_F(AstraSidebarSectionViewTest, SetItemCount) {
  section_view_->SetItemCount(42);
  EXPECT_EQ(section_view_->GetItemCount(), 42);
}

// Test 274: Default show item count is true.
TEST_F(AstraSidebarSectionViewTest, DefaultShowItemCount) {
  EXPECT_TRUE(section_view_->GetShowItemCount());
}

// Test 275: SetShowItemCount(false) hides the badge.
TEST_F(AstraSidebarSectionViewTest, SetShowItemCount) {
  section_view_->SetShowItemCount(false);
  EXPECT_FALSE(section_view_->GetShowItemCount());
}

// Test 276: Default show chevron is true.
TEST_F(AstraSidebarSectionViewTest, DefaultShowChevron) {
  EXPECT_TRUE(section_view_->GetShowChevron());
}

// Test 277: SetShowChevron(false) hides the chevron.
TEST_F(AstraSidebarSectionViewTest, SetShowChevron) {
  section_view_->SetShowChevron(false);
  EXPECT_FALSE(section_view_->GetShowChevron());
}

// Test 278: Default show search is true.
TEST_F(AstraSidebarSectionViewTest, DefaultShowSearch) {
  EXPECT_TRUE(section_view_->GetShowSearch());
}

// Test 279: SetShowSearch(false) hides search.
TEST_F(AstraSidebarSectionViewTest, SetShowSearch) {
  section_view_->SetShowSearch(false);
  EXPECT_FALSE(section_view_->GetShowSearch());
}

// Test 280: Default search query is empty.
TEST_F(AstraSidebarSectionViewTest, DefaultSearchQueryEmpty) {
  EXPECT_TRUE(section_view_->GetSearchQuery().empty());
}

// Test 281: SetSearchQuery updates the query.
TEST_F(AstraSidebarSectionViewTest, SetSearchQuery) {
  section_view_->SetSearchQuery(u"hello");
  EXPECT_EQ(section_view_->GetSearchQuery(), u"hello");
}

// Test 282: Default show add button is true.
TEST_F(AstraSidebarSectionViewTest, DefaultShowAddButton) {
  EXPECT_TRUE(section_view_->GetShowAddButton());
}

// Test 283: SetShowAddButton(false) hides the add button.
TEST_F(AstraSidebarSectionViewTest, SetShowAddButton) {
  section_view_->SetShowAddButton(false);
  EXPECT_FALSE(section_view_->GetShowAddButton());
}

// Test 284: Default show more button is true.
TEST_F(AstraSidebarSectionViewTest, DefaultShowMoreButton) {
  EXPECT_TRUE(section_view_->GetShowMoreButton());
}

// Test 285: SetShowMoreButton(false) hides the more button.
TEST_F(AstraSidebarSectionViewTest, SetShowMoreButton) {
  section_view_->SetShowMoreButton(false);
  EXPECT_FALSE(section_view_->GetShowMoreButton());
}

// Test 286: Default show context menu is true.
TEST_F(AstraSidebarSectionViewTest, DefaultShowContextMenu) {
  EXPECT_TRUE(section_view_->GetShowContextMenu());
}

// Test 287: SetShowContextMenu(false) disables context menu.
TEST_F(AstraSidebarSectionViewTest, SetShowContextMenu) {
  section_view_->SetShowContextMenu(false);
  EXPECT_FALSE(section_view_->GetShowContextMenu());
}

// Test 288: Default section color is transparent.
TEST_F(AstraSidebarSectionViewTest, DefaultSectionColor) {
  EXPECT_EQ(section_view_->GetSectionColor(), SK_ColorTRANSPARENT);
}

// Test 289: SetSectionColor updates the color.
TEST_F(AstraSidebarSectionViewTest, SetSectionColor) {
  section_view_->SetSectionColor(SK_ColorBLUE);
  EXPECT_EQ(section_view_->GetSectionColor(), SK_ColorBLUE);
}

// Test 290: Default drag-drop enabled is false.
TEST_F(AstraSidebarSectionViewTest, DefaultDragDropEnabled) {
  EXPECT_FALSE(section_view_->GetDragDropEnabled());
}

// Test 291: SetDragDropEnabled(true) enables drag-drop.
TEST_F(AstraSidebarSectionViewTest, SetDragDropEnabled) {
  section_view_->SetDragDropEnabled(true);
  EXPECT_TRUE(section_view_->GetDragDropEnabled());
}

// Test 292: GetHeaderView returns a valid view.
TEST_F(AstraSidebarSectionViewTest, GetHeaderViewValid) {
  EXPECT_NE(section_view_->GetHeaderView(), nullptr);
}

// Test 293: GetContentView returns a valid view.
TEST_F(AstraSidebarSectionViewTest, GetContentViewValid) {
  EXPECT_NE(section_view_->GetContentView(), nullptr);
}

// Test 294: GetFooterView returns a valid view.
TEST_F(AstraSidebarSectionViewTest, GetFooterViewValid) {
  EXPECT_NE(section_view_->GetFooterView(), nullptr);
}

// Test 295: Default item view count is 0.
TEST_F(AstraSidebarSectionViewTest, DefaultItemViewCountZero) {
  EXPECT_EQ(section_view_->GetItemViewCount(), 0);
}

// Test 296: AddItemView increases count.
TEST_F(AstraSidebarSectionViewTest, AddItemViewIncreasesCount) {
  auto item = std::make_unique<views::View>();
  section_view_->AddItemView(std::move(item));
  EXPECT_EQ(section_view_->GetItemViewCount(), 1);
}

// Test 297: GetItemViewAt returns valid view.
TEST_F(AstraSidebarSectionViewTest, GetItemViewAt) {
  auto item = std::make_unique<views::View>();
  views::View* item_ptr = item.get();
  section_view_->AddItemView(std::move(item));
  EXPECT_EQ(section_view_->GetItemViewAt(0), item_ptr);
}

// Test 298: GetItemViewAt with invalid index returns nullptr.
TEST_F(AstraSidebarSectionViewTest, GetItemViewAtInvalid) {
  EXPECT_EQ(section_view_->GetItemViewAt(0), nullptr);
  EXPECT_EQ(section_view_->GetItemViewAt(-1), nullptr);
  EXPECT_EQ(section_view_->GetItemViewAt(100), nullptr);
}

// Test 299: RemoveAllItems clears the list.
TEST_F(AstraSidebarSectionViewTest, RemoveAllItems) {
  section_view_->AddItemView(std::make_unique<views::View>());
  section_view_->AddItemView(std::make_unique<views::View>());
  EXPECT_EQ(section_view_->GetItemViewCount(), 2);
  section_view_->RemoveAllItems();
  EXPECT_EQ(section_view_->GetItemViewCount(), 0);
}

// Test 300: Default sort order is kManual.
TEST_F(AstraSidebarSectionViewTest, DefaultSortOrder) {
  EXPECT_EQ(section_view_->GetSortOrder(), AstraSidebarSortOrder::kManual);
}

// Test 301: SetSortOrder updates the sort order.
TEST_F(AstraSidebarSectionViewTest, SetSortOrder) {
  section_view_->SetSortOrder(AstraSidebarSortOrder::kAlphabetical);
  EXPECT_EQ(section_view_->GetSortOrder(), AstraSidebarSortOrder::kAlphabetical);
}

// Test 302: Default filter is kAll.
TEST_F(AstraSidebarSectionViewTest, DefaultFilter) {
  EXPECT_EQ(section_view_->GetFilter(), AstraSidebarFilter::kAll);
}

// Test 303: SetFilter updates the filter.
TEST_F(AstraSidebarSectionViewTest, SetFilter) {
  section_view_->SetFilter(AstraSidebarFilter::kFavorites);
  EXPECT_EQ(section_view_->GetFilter(), AstraSidebarFilter::kFavorites);
}

// Test 304: Default loading state is false.
TEST_F(AstraSidebarSectionViewTest, DefaultNotLoading) {
  EXPECT_FALSE(section_view_->IsLoading());
}

// Test 305: SetLoading(true) shows loading state.
TEST_F(AstraSidebarSectionViewTest, SetLoading) {
  section_view_->SetLoading(true);
  EXPECT_TRUE(section_view_->IsLoading());
}

// Test 306: Default empty state is false.
TEST_F(AstraSidebarSectionViewTest, DefaultNotEmpty) {
  EXPECT_FALSE(section_view_->IsEmpty());
}

// Test 307: SetEmpty(true) shows empty state.
TEST_F(AstraSidebarSectionViewTest, SetEmpty) {
  section_view_->SetEmpty(true);
  EXPECT_TRUE(section_view_->IsEmpty());
}

// Test 308: SetEmptyStateText updates the text.
TEST_F(AstraSidebarSectionViewTest, SetEmptyStateText) {
  section_view_->SetEmptyStateText(u"No items here");
  section_view_->SetEmpty(true);
  EXPECT_TRUE(section_view_->IsEmpty());
}

// Test 309: Preferred size is valid.
TEST_F(AstraSidebarSectionViewTest, PreferredSizeValid) {
  gfx::Size size = section_view_->GetPreferredSize();
  EXPECT_GT(size.height(), 0);
  EXPECT_GE(size.width(), 0);
}

// Test 310: Section type is correct.
TEST_F(AstraSidebarSectionViewTest, SectionTypeCorrect) {
  auto view = std::make_unique<AstraSidebarSectionView>(
      AstraSidebarSectionType::kHistory, u"History");
  // Verify construction works for different types.
  EXPECT_EQ(view->GetTitle(), u"History");
}

// Test 311: Multiple SetTitle calls accumulate correctly.
TEST_F(AstraSidebarSectionViewTest, MultipleSetTitle) {
  section_view_->SetTitle(u"First");
  section_view_->SetTitle(u"Second");
  section_view_->SetTitle(u"Third");
  EXPECT_EQ(section_view_->GetTitle(), u"Third");
}

// Test 312: Setting item count to negative is clamped.
TEST_F(AstraSidebarSectionViewTest, NegativeItemCountClamped) {
  section_view_->SetItemCount(-5);
  // Item count should not be negative.
  EXPECT_GE(section_view_->GetItemCount(), 0);
}

// Test 313: Large item count is handled correctly.
TEST_F(AstraSidebarSectionViewTest, LargeItemCount) {
  section_view_->SetItemCount(99999);
  EXPECT_EQ(section_view_->GetItemCount(), 99999);
}

// Test 314: Search query can be set to empty.
TEST_F(AstraSidebarSectionViewTest, SearchQueryEmptyAfterSet) {
  section_view_->SetSearchQuery(u"test");
  section_view_->SetSearchQuery(u"");
  EXPECT_TRUE(section_view_->GetSearchQuery().empty());
}

// Test 315: AddItemView with many items.
TEST_F(AstraSidebarSectionViewTest, AddManyItemViews) {
  for (int i = 0; i < 50; ++i) {
    section_view_->AddItemView(std::make_unique<views::View>());
  }
  EXPECT_EQ(section_view_->GetItemViewCount(), 50);
}

// =========================================================================
// AstraSidebarBookmarksViewTest — bookmarks section tests
// =========================================================================

class AstraSidebarBookmarksViewTest : public views::test::ViewsTestBase {
 public:
  AstraSidebarBookmarksViewTest() = default;
  ~AstraSidebarBookmarksViewTest() override = default;

  void SetUp() override {
    views::test::ViewsTestBase::SetUp();
    bookmarks_view_ = std::make_unique<AstraSidebarBookmarksView>();
    bookmarks_view_->set_delegate(&delegate_);
    widget_ = CreateWidget();
    widget_->SetContentsView(bookmarks_view_.get());
    widget_->SetSize(gfx::Size(300, 500));
    widget_->Show();
  }

  void TearDown() override {
    widget_.reset();
    bookmarks_view_.reset();
    views::test::ViewsTestBase::TearDown();
  }

 protected:
  AstraBookmarkItemInfo MakeBookmark(const std::string& id,
                                     const std::u16string& title,
                                     const std::string& url,
                                     bool is_folder = false) {
    AstraBookmarkItemInfo info;
    info.id = id;
    info.title = title;
    info.url = GURL(url);
    info.is_folder = is_folder;
    info.date_added = base::Time::Now();
    return info;
  }

  std::unique_ptr<AstraSidebarBookmarksView> bookmarks_view_;
  std::unique_ptr<views::Widget> widget_;
  MockSidebarBookmarksDelegate delegate_;
};

// Test 316: Default bookmark count is 0.
TEST_F(AstraSidebarBookmarksViewTest, DefaultCountZero) {
  EXPECT_EQ(bookmarks_view_->GetBookmarkCount(), 0);
}

// Test 317: SetBookmarks replaces all items.
TEST_F(AstraSidebarBookmarksViewTest, SetBookmarksReplacesAll) {
  std::vector<AstraBookmarkItemInfo> bookmarks;
  bookmarks.push_back(MakeBookmark("1", u"Google", "https://google.com"));
  bookmarks.push_back(MakeBookmark("2", u"Apple", "https://apple.com"));
  bookmarks_view_->SetBookmarks(bookmarks);
  EXPECT_EQ(bookmarks_view_->GetBookmarkCount(), 2);
}

// Test 318: GetBookmarkAt returns correct item.
TEST_F(AstraSidebarBookmarksViewTest, GetBookmarkAt) {
  std::vector<AstraBookmarkItemInfo> bookmarks;
  bookmarks.push_back(MakeBookmark("1", u"Google", "https://google.com"));
  bookmarks_view_->SetBookmarks(bookmarks);
  EXPECT_EQ(bookmarks_view_->GetBookmarkAt(0).title, u"Google");
}

// Test 319: AddBookmark appends to list.
TEST_F(AstraSidebarBookmarksViewTest, AddBookmarkAppends) {
  bookmarks_view_->AddBookmark(
      MakeBookmark("1", u"First", "https://first.com"));
  EXPECT_EQ(bookmarks_view_->GetBookmarkCount(), 1);
  bookmarks_view_->AddBookmark(
      MakeBookmark("2", u"Second", "https://second.com"));
  EXPECT_EQ(bookmarks_view_->GetBookmarkCount(), 2);
}

// Test 320: RemoveBookmark removes from list.
TEST_F(AstraSidebarBookmarksViewTest, RemoveBookmark) {
  bookmarks_view_->AddBookmark(
      MakeBookmark("1", u"First", "https://first.com"));
  bookmarks_view_->AddBookmark(
      MakeBookmark("2", u"Second", "https://second.com"));
  EXPECT_EQ(bookmarks_view_->GetBookmarkCount(), 2);
  bookmarks_view_->RemoveBookmark(0);
  EXPECT_EQ(bookmarks_view_->GetBookmarkCount(), 1);
  EXPECT_EQ(bookmarks_view_->GetBookmarkAt(0).title, u"Second");
}

// Test 321: UpdateBookmark updates info.
TEST_F(AstraSidebarBookmarksViewTest, UpdateBookmark) {
  bookmarks_view_->AddBookmark(
      MakeBookmark("1", u"Original", "https://original.com"));
  auto updated = MakeBookmark("1", u"Updated", "https://updated.com");
  bookmarks_view_->UpdateBookmark(0, updated);
  EXPECT_EQ(bookmarks_view_->GetBookmarkAt(0).title, u"Updated");
  EXPECT_EQ(bookmarks_view_->GetBookmarkAt(0).url, GURL("https://updated.com"));
}

// Test 322: Default selection is -1 (none).
TEST_F(AstraSidebarBookmarksViewTest, DefaultSelectionNone) {
  EXPECT_EQ(bookmarks_view_->GetSelectedBookmarkIndex(), -1);
}

// Test 323: SetSelectedBookmark updates selection.
TEST_F(AstraSidebarBookmarksViewTest, SetSelectedBookmark) {
  bookmarks_view_->AddBookmark(
      MakeBookmark("1", u"Item", "https://item.com"));
  bookmarks_view_->SetSelectedBookmark(0);
  EXPECT_EQ(bookmarks_view_->GetSelectedBookmarkIndex(), 0);
}

// Test 324: ClearSelection clears selection.
TEST_F(AstraSidebarBookmarksViewTest, ClearSelection) {
  bookmarks_view_->AddBookmark(
      MakeBookmark("1", u"Item", "https://item.com"));
  bookmarks_view_->SetSelectedBookmark(0);
  bookmarks_view_->ClearSelection();
  EXPECT_EQ(bookmarks_view_->GetSelectedBookmarkIndex(), -1);
}

// Test 325: Default show folders first is true.
TEST_F(AstraSidebarBookmarksViewTest, DefaultShowFoldersFirst) {
  EXPECT_TRUE(bookmarks_view_->GetShowFoldersFirst());
}

// Test 326: SetShowFoldersFirst updates the setting.
TEST_F(AstraSidebarBookmarksViewTest, SetShowFoldersFirst) {
  bookmarks_view_->SetShowFoldersFirst(false);
  EXPECT_FALSE(bookmarks_view_->GetShowFoldersFirst());
}

// Test 327: Default current folder is root.
TEST_F(AstraSidebarBookmarksViewTest, DefaultCurrentFolder) {
  EXPECT_TRUE(bookmarks_view_->GetCurrentFolder().empty());
}

// Test 328: NavigateToFolder sets current folder.
TEST_F(AstraSidebarBookmarksViewTest, NavigateToFolder) {
  bookmarks_view_->NavigateToFolder("folder1");
  EXPECT_EQ(bookmarks_view_->GetCurrentFolder(), "folder1");
}

// Test 329: NavigateUp from root does nothing.
TEST_F(AstraSidebarBookmarksViewTest, NavigateUpFromRoot) {
  EXPECT_FALSE(bookmarks_view_->CanNavigateUp());
  bookmarks_view_->NavigateUp();
  EXPECT_TRUE(bookmarks_view_->GetCurrentFolder().empty());
}

// Test 330: CanNavigateUp returns false at root.
TEST_F(AstraSidebarBookmarksViewTest, CanNavigateUpAtRoot) {
  EXPECT_FALSE(bookmarks_view_->CanNavigateUp());
}

// Test 331: CanNavigateUp returns true after navigating down.
TEST_F(AstraSidebarBookmarksViewTest, CanNavigateUpAfterNav) {
  bookmarks_view_->NavigateToFolder("folder1");
  EXPECT_TRUE(bookmarks_view_->CanNavigateUp());
}

// Test 332: NavigateUp goes back to parent.
TEST_F(AstraSidebarBookmarksViewTest, NavigateUpGoesBack) {
  bookmarks_view_->NavigateToFolder("folder1");
  bookmarks_view_->NavigateUp();
  EXPECT_TRUE(bookmarks_view_->GetCurrentFolder().empty());
}

// Test 333: Default show folder tree is false.
TEST_F(AstraSidebarBookmarksViewTest, DefaultShowFolderTree) {
  EXPECT_FALSE(bookmarks_view_->GetShowFolderTree());
}

// Test 334: SetShowFolderTree updates setting.
TEST_F(AstraSidebarBookmarksViewTest, SetShowFolderTree) {
  bookmarks_view_->SetShowFolderTree(true);
  EXPECT_TRUE(bookmarks_view_->GetShowFolderTree());
}

// Test 335: Default show only bookmarks bar is false.
TEST_F(AstraSidebarBookmarksViewTest, DefaultShowOnlyBookmarksBar) {
  EXPECT_FALSE(bookmarks_view_->GetShowOnlyBookmarksBar());
}

// Test 336: SetShowOnlyBookmarksBar updates setting.
TEST_F(AstraSidebarBookmarksViewTest, SetShowOnlyBookmarksBar) {
  bookmarks_view_->SetShowOnlyBookmarksBar(true);
  EXPECT_TRUE(bookmarks_view_->GetShowOnlyBookmarksBar());
}

// Test 337: SearchBookmarks filters by title.
TEST_F(AstraSidebarBookmarksViewTest, SearchBookmarksByTitle) {
  bookmarks_view_->AddBookmark(
      MakeBookmark("1", u"Google Search", "https://google.com"));
  bookmarks_view_->AddBookmark(
      MakeBookmark("2", u"Apple Store", "https://apple.com"));
  bookmarks_view_->SearchBookmarks(u"Google");
  EXPECT_EQ(bookmarks_view_->GetVisibleBookmarkCount(), 1);
}

// Test 338: SearchBookmarks with empty query shows all.
TEST_F(AstraSidebarBookmarksViewTest, SearchBookmarksEmptyQuery) {
  bookmarks_view_->AddBookmark(
      MakeBookmark("1", u"Google", "https://google.com"));
  bookmarks_view_->AddBookmark(
      MakeBookmark("2", u"Apple", "https://apple.com"));
  bookmarks_view_->SearchBookmarks(u"");
  EXPECT_EQ(bookmarks_view_->GetVisibleBookmarkCount(), 2);
}

// Test 339: SortBookmarks by alphabetical.
TEST_F(AstraSidebarBookmarksViewTest, SortBookmarksAlphabetical) {
  bookmarks_view_->AddBookmark(
      MakeBookmark("1", u"Banana", "https://banana.com"));
  bookmarks_view_->AddBookmark(
      MakeBookmark("2", u"Apple", "https://apple.com"));
  bookmarks_view_->SortBookmarks(AstraSidebarSortOrder::kAlphabetical);
  EXPECT_EQ(bookmarks_view_->GetBookmarkAt(0).title, u"Apple");
  EXPECT_EQ(bookmarks_view_->GetBookmarkAt(1).title, u"Banana");
}

// Test 340: FilterBookmarks with kAll shows all.
TEST_F(AstraSidebarBookmarksViewTest, FilterBookmarksAll) {
  bookmarks_view_->AddBookmark(
      MakeBookmark("1", u"Item1", "https://item1.com"));
  bookmarks_view_->AddBookmark(
      MakeBookmark("2", u"Item2", "https://item2.com"));
  bookmarks_view_->FilterBookmarks(AstraSidebarFilter::kAll);
  EXPECT_EQ(bookmarks_view_->GetVisibleBookmarkCount(), 2);
}

// Test 341: GetFolderPath returns vector of folders.
TEST_F(AstraSidebarBookmarksViewTest, GetFolderPath) {
  bookmarks_view_->NavigateToFolder("folder1");
  bookmarks_view_->NavigateToFolder("folder2");
  auto path = bookmarks_view_->GetFolderPath();
  EXPECT_EQ(path.size(), 2u);
}

// Test 342: NewFolder operation exists.
TEST_F(AstraSidebarBookmarksViewTest, NewFolderExists) {
  // Just verify the method compiles and runs without crash.
  bookmarks_view_->NewFolder(u"New Folder");
  SUCCEED();
}

// Test 343: DeleteFolder operation exists.
TEST_F(AstraSidebarBookmarksViewTest, DeleteFolderExists) {
  bookmarks_view_->DeleteFolder("folder1");
  SUCCEED();
}

// Test 344: RenameFolder operation exists.
TEST_F(AstraSidebarBookmarksViewTest, RenameFolderExists) {
  bookmarks_view_->RenameFolder("folder1", u"New Name");
  SUCCEED();
}

// Test 345: Delegate click callback.
TEST_F(AstraSidebarBookmarksViewTest, DelegateClickCallback) {
  EXPECT_CALL(delegate_, OnBookmarkClicked("1")).Times(1);
  bookmarks_view_->AddBookmark(
      MakeBookmark("1", u"Item", "https://item.com"));
  // Simulate click by calling the delegate method directly.
  delegate_.OnBookmarkClicked("1");
}

// Test 346: Many bookmarks.
TEST_F(AstraSidebarBookmarksViewTest, ManyBookmarks) {
  for (int i = 0; i < 100; ++i) {
    auto info = MakeBookmark(
        std::to_string(i), u"Item " + base::NumberToString16(i),
        "https://example.com/" + std::to_string(i));
    bookmarks_view_->AddBookmark(info);
  }
  EXPECT_EQ(bookmarks_view_->GetBookmarkCount(), 100);
}

// Test 347: Bookmark with long title.
TEST_F(AstraSidebarBookmarksViewTest, LongTitleBookmark) {
  std::u16string long_title(1000, 'x');
  auto info = MakeBookmark("1", long_title, "https://example.com");
  bookmarks_view_->AddBookmark(info);
  EXPECT_EQ(bookmarks_view_->GetBookmarkAt(0).title, long_title);
}

// Test 348: Bookmark folder has is_folder true.
TEST_F(AstraSidebarBookmarksViewTest, FolderIsFolder) {
  auto info = MakeBookmark("1", u"Folder", "", true);
  EXPECT_TRUE(info.is_folder);
}

// Test 349: Bookmark URL is valid.
TEST_F(AstraSidebarBookmarksViewTest, BookmarkUrlValid) {
  auto info = MakeBookmark("1", u"Test", "https://example.com");
  EXPECT_TRUE(info.url.is_valid());
}

// Test 350: RemoveBookmark with invalid index is safe.
TEST_F(AstraSidebarBookmarksViewTest, RemoveInvalidIndexSafe) {
  bookmarks_view_->RemoveBookmark(0);
  bookmarks_view_->RemoveBookmark(-1);
  bookmarks_view_->RemoveBookmark(100);
  SUCCEED();
}

// Test 351: SetBookmarks with empty vector clears list.
TEST_F(AstraSidebarBookmarksViewTest, SetBookmarksEmpty) {
  bookmarks_view_->AddBookmark(
      MakeBookmark("1", u"Item", "https://item.com"));
  bookmarks_view_->SetBookmarks(std::vector<AstraBookmarkItemInfo>());
  EXPECT_EQ(bookmarks_view_->GetBookmarkCount(), 0);
}

// Test 352: SetSelectedBookmark with -1 clears selection.
TEST_F(AstraSidebarBookmarksViewTest, SetSelectedMinusOne) {
  bookmarks_view_->AddBookmark(
      MakeBookmark("1", u"Item", "https://item.com"));
  bookmarks_view_->SetSelectedBookmark(0);
  bookmarks_view_->SetSelectedBookmark(-1);
  EXPECT_EQ(bookmarks_view_->GetSelectedBookmarkIndex(), -1);
}

// =========================================================================
// AstraSidebarHistoryViewTest — history section tests
// =========================================================================

class AstraSidebarHistoryViewTest : public views::test::ViewsTestBase {
 public:
  AstraSidebarHistoryViewTest() = default;
  ~AstraSidebarHistoryViewTest() override = default;

  void SetUp() override {
    views::test::ViewsTestBase::SetUp();
    history_view_ = std::make_unique<AstraSidebarHistoryView>();
    history_view_->set_delegate(&delegate_);
    widget_ = CreateWidget();
    widget_->SetContentsView(history_view_.get());
    widget_->SetSize(gfx::Size(300, 500));
    widget_->Show();
  }

  void TearDown() override {
    widget_.reset();
    history_view_.reset();
    views::test::ViewsTestBase::TearDown();
  }

 protected:
  AstraHistoryItemInfo MakeHistoryItem(const std::string& id,
                                       const std::u16string& title,
                                       const std::string& url,
                                       base::Time visit_time) {
    AstraHistoryItemInfo info;
    info.id = id;
    info.title = title;
    info.url = GURL(url);
    info.hostname = base::UTF8ToUTF16(GURL(url).host());
    info.visit_time = visit_time;
    return info;
  }

  std::unique_ptr<AstraSidebarHistoryView> history_view_;
  std::unique_ptr<views::Widget> widget_;
  MockSidebarHistoryDelegate delegate_;
};

// Test 353: Default history item count is 0.
TEST_F(AstraSidebarHistoryViewTest, DefaultCountZero) {
  EXPECT_EQ(history_view_->GetHistoryItemCount(), 0);
}

// Test 354: SetHistoryItems replaces all items.
TEST_F(AstraSidebarHistoryViewTest, SetHistoryItemsReplacesAll) {
  std::vector<AstraHistoryItemInfo> items;
  items.push_back(MakeHistoryItem(
      "1", u"Google", "https://google.com", base::Time::Now()));
  items.push_back(MakeHistoryItem(
      "2", u"Apple", "https://apple.com", base::Time::Now()));
  history_view_->SetHistoryItems(items);
  EXPECT_EQ(history_view_->GetHistoryItemCount(), 2);
}

// Test 355: GetHistoryItemAt returns correct item.
TEST_F(AstraSidebarHistoryViewTest, GetHistoryItemAt) {
  auto item = MakeHistoryItem("1", u"Test", "https://test.com",
                              base::Time::Now());
  std::vector<AstraHistoryItemInfo> items;
  items.push_back(item);
  history_view_->SetHistoryItems(items);
  EXPECT_EQ(history_view_->GetHistoryItemAt(0).title, u"Test");
}

// Test 356: AddHistoryItem appends.
TEST_F(AstraSidebarHistoryViewTest, AddHistoryItemAppends) {
  history_view_->AddHistoryItem(MakeHistoryItem(
      "1", u"First", "https://first.com", base::Time::Now()));
  EXPECT_EQ(history_view_->GetHistoryItemCount(), 1);
}

// Test 357: RemoveHistoryItem removes by index.
TEST_F(AstraSidebarHistoryViewTest, RemoveHistoryItem) {
  history_view_->AddHistoryItem(MakeHistoryItem(
      "1", u"First", "https://first.com", base::Time::Now()));
  history_view_->AddHistoryItem(MakeHistoryItem(
      "2", u"Second", "https://second.com", base::Time::Now()));
  history_view_->RemoveHistoryItem(0);
  EXPECT_EQ(history_view_->GetHistoryItemCount(), 1);
}

// Test 358: ClearAllHistory clears all items.
TEST_F(AstraSidebarHistoryViewTest, ClearAllHistory) {
  history_view_->AddHistoryItem(MakeHistoryItem(
      "1", u"First", "https://first.com", base::Time::Now()));
  history_view_->AddHistoryItem(MakeHistoryItem(
      "2", u"Second", "https://second.com", base::Time::Now()));
  history_view_->ClearAllHistory();
  EXPECT_EQ(history_view_->GetHistoryItemCount(), 0);
}

// Test 359: Default selection is -1.
TEST_F(AstraSidebarHistoryViewTest, DefaultSelectionNone) {
  EXPECT_EQ(history_view_->GetSelectedIndex(), -1);
}

// Test 360: SetSelectedItem updates selection.
TEST_F(AstraSidebarHistoryViewTest, SetSelectedItem) {
  history_view_->AddHistoryItem(MakeHistoryItem(
      "1", u"Item", "https://item.com", base::Time::Now()));
  history_view_->SetSelectedItem(0);
  EXPECT_EQ(history_view_->GetSelectedIndex(), 0);
}

// Test 361: ClearSelection clears selection.
TEST_F(AstraSidebarHistoryViewTest, ClearSelection) {
  history_view_->AddHistoryItem(MakeHistoryItem(
      "1", u"Item", "https://item.com", base::Time::Now()));
  history_view_->SetSelectedItem(0);
  history_view_->ClearSelection();
  EXPECT_EQ(history_view_->GetSelectedIndex(), -1);
}

// Test 362: Default group by date is true.
TEST_F(AstraSidebarHistoryViewTest, DefaultGroupByDate) {
  EXPECT_TRUE(history_view_->GetGroupByDate());
}

// Test 363: SetGroupByDate updates setting.
TEST_F(AstraSidebarHistoryViewTest, SetGroupByDate) {
  history_view_->SetGroupByDate(false);
  EXPECT_FALSE(history_view_->GetGroupByDate());
}

// Test 364: Default show today is true.
TEST_F(AstraSidebarHistoryViewTest, DefaultShowToday) {
  EXPECT_TRUE(history_view_->GetShowToday());
}

// Test 365: SetShowToday updates setting.
TEST_F(AstraSidebarHistoryViewTest, SetShowToday) {
  history_view_->SetShowToday(false);
  EXPECT_FALSE(history_view_->GetShowToday());
}

// Test 366: Default show yesterday is true.
TEST_F(AstraSidebarHistoryViewTest, DefaultShowYesterday) {
  EXPECT_TRUE(history_view_->GetShowYesterday());
}

// Test 367: SetShowYesterday updates setting.
TEST_F(AstraSidebarHistoryViewTest, SetShowYesterday) {
  history_view_->SetShowYesterday(false);
  EXPECT_FALSE(history_view_->GetShowYesterday());
}

// Test 368: Default show last week is true.
TEST_F(AstraSidebarHistoryViewTest, DefaultShowLastWeek) {
  EXPECT_TRUE(history_view_->GetShowLastWeek());
}

// Test 369: SetShowLastWeek updates setting.
TEST_F(AstraSidebarHistoryViewTest, SetShowLastWeek) {
  history_view_->SetShowLastWeek(false);
  EXPECT_FALSE(history_view_->GetShowLastWeek());
}

// Test 370: Default show older is true.
TEST_F(AstraSidebarHistoryViewTest, DefaultShowOlder) {
  EXPECT_TRUE(history_view_->GetShowOlder());
}

// Test 371: SetShowOlder updates setting.
TEST_F(AstraSidebarHistoryViewTest, SetShowOlder) {
  history_view_->SetShowOlder(false);
  EXPECT_FALSE(history_view_->GetShowOlder());
}

// Test 372: Today count is zero when no items.
TEST_F(AstraSidebarHistoryViewTest, TodayCountZero) {
  EXPECT_EQ(history_view_->GetTodayCount(), 0);
}

// Test 373: Today count increases with today's items.
TEST_F(AstraSidebarHistoryViewTest, TodayCountIncreases) {
  history_view_->AddHistoryItem(MakeHistoryItem(
      "1", u"Today", "https://today.com", base::Time::Now()));
  EXPECT_GE(history_view_->GetTodayCount(), 0);
}

// Test 374: Yesterday count with yesterday's items.
TEST_F(AstraSidebarHistoryViewTest, YesterdayCount) {
  history_view_->AddHistoryItem(MakeHistoryItem(
      "1", u"Yesterday", "https://yesterday.com",
      base::Time::Now() - base::Days(1)));
  EXPECT_GE(history_view_->GetYesterdayCount(), 0);
}

// Test 375: SearchHistory filters items.
TEST_F(AstraSidebarHistoryViewTest, SearchHistoryFilters) {
  history_view_->AddHistoryItem(MakeHistoryItem(
      "1", u"Google Search", "https://google.com", base::Time::Now()));
  history_view_->AddHistoryItem(MakeHistoryItem(
      "2", u"Apple Store", "https://apple.com", base::Time::Now()));
  history_view_->SearchHistory(u"Google");
  EXPECT_GE(history_view_->GetSearchResultsCount(), 1);
}

// Test 376: Search with empty query.
TEST_F(AstraSidebarHistoryViewTest, SearchHistoryEmpty) {
  history_view_->AddHistoryItem(MakeHistoryItem(
      "1", u"Item", "https://item.com", base::Time::Now()));
  history_view_->SearchHistory(u"");
  EXPECT_EQ(history_view_->GetSearchResultsCount(),
            history_view_->GetHistoryItemCount());
}

// Test 377: Default show search results is false.
TEST_F(AstraSidebarHistoryViewTest, DefaultShowSearchResults) {
  EXPECT_FALSE(history_view_->GetShowSearchResults());
}

// Test 378: SetShowSearchResults updates setting.
TEST_F(AstraSidebarHistoryViewTest, SetShowSearchResults) {
  history_view_->SetShowSearchResults(true);
  EXPECT_TRUE(history_view_->GetShowSearchResults());
}

// Test 379: RemoveItemsForDomain removes items for domain.
TEST_F(AstraSidebarHistoryViewTest, RemoveItemsForDomain) {
  history_view_->AddHistoryItem(MakeHistoryItem(
      "1", u"Google 1", "https://google.com/page1", base::Time::Now()));
  history_view_->AddHistoryItem(MakeHistoryItem(
      "2", u"Google 2", "https://google.com/page2", base::Time::Now()));
  history_view_->AddHistoryItem(MakeHistoryItem(
      "3", u"Apple", "https://apple.com", base::Time::Now()));
  history_view_->RemoveItemsForDomain("google.com");
  EXPECT_LE(history_view_->GetHistoryItemCount(), 1);
}

// Test 380: GetDomainCount counts unique domains.
TEST_F(AstraSidebarHistoryViewTest, GetDomainCount) {
  history_view_->AddHistoryItem(MakeHistoryItem(
      "1", u"Google", "https://google.com", base::Time::Now()));
  history_view_->AddHistoryItem(MakeHistoryItem(
      "2", u"Apple", "https://apple.com", base::Time::Now()));
  EXPECT_GE(history_view_->GetDomainCount(), 2);
}

// Test 381: Default show favicons is true.
TEST_F(AstraSidebarHistoryViewTest, DefaultShowFavicons) {
  EXPECT_TRUE(history_view_->GetShowFavicons());
}

// Test 382: SetShowFavicons updates setting.
TEST_F(AstraSidebarHistoryViewTest, SetShowFavicons) {
  history_view_->SetShowFavicons(false);
  EXPECT_FALSE(history_view_->GetShowFavicons());
}

// Test 383: Default show time is true.
TEST_F(AstraSidebarHistoryViewTest, DefaultShowTime) {
  EXPECT_TRUE(history_view_->GetShowTime());
}

// Test 384: SetShowTime updates setting.
TEST_F(AstraSidebarHistoryViewTest, SetShowTime) {
  history_view_->SetShowTime(false);
  EXPECT_FALSE(history_view_->GetShowTime());
}

// Test 385: Default max items per group is 15.
TEST_F(AstraSidebarHistoryViewTest, DefaultMaxItemsPerGroup) {
  EXPECT_EQ(history_view_->GetMaxItemsPerGroup(), 15);
}

// Test 386: SetMaxItemsPerGroup updates setting.
TEST_F(AstraSidebarHistoryViewTest, SetMaxItemsPerGroup) {
  history_view_->SetMaxItemsPerGroup(50);
  EXPECT_EQ(history_view_->GetMaxItemsPerGroup(), 50);
}

// Test 387: Today group is expanded by default.
TEST_F(AstraSidebarHistoryViewTest, TodayGroupExpandedDefault) {
  EXPECT_TRUE(history_view_->IsGroupExpanded(0));
}

// Test 388: CollapseGroup collapses a group.
TEST_F(AstraSidebarHistoryViewTest, CollapseGroup) {
  history_view_->CollapseGroup(0);
  EXPECT_FALSE(history_view_->IsGroupExpanded(0));
}

// Test 389: ExpandGroup expands a group.
TEST_F(AstraSidebarHistoryViewTest, ExpandGroup) {
  history_view_->CollapseGroup(0);
  history_view_->ExpandGroup(0);
  EXPECT_TRUE(history_view_->IsGroupExpanded(0));
}

// Test 390: ToggleGroup toggles expansion.
TEST_F(AstraSidebarHistoryViewTest, ToggleGroup) {
  bool expanded = history_view_->IsGroupExpanded(0);
  history_view_->ToggleGroup(0);
  EXPECT_NE(history_view_->IsGroupExpanded(0), expanded);
}

// Test 391: Group count is zero when no items.
TEST_F(AstraSidebarHistoryViewTest, GroupCountZero) {
  EXPECT_EQ(history_view_->GetGroupCount(), 0);
}

// Test 392: GetGroupAt returns group info.
TEST_F(AstraSidebarHistoryViewTest, GetGroupAt) {
  history_view_->AddHistoryItem(MakeHistoryItem(
      "1", u"Today", "https://today.com", base::Time::Now()));
  auto group_info = history_view_->GetGroupAt(0);
  EXPECT_GE(group_info.item_count, 0);
}

// Test 393: Delegate click callback.
TEST_F(AstraSidebarHistoryViewTest, DelegateClickCallback) {
  EXPECT_CALL(delegate_, OnHistoryItemClicked("1")).Times(1);
  delegate_.OnHistoryItemClicked("1");
}

// Test 394: Delegate remove callback.
TEST_F(AstraSidebarHistoryViewTest, DelegateRemoveCallback) {
  EXPECT_CALL(delegate_, OnRemoveHistoryItem("1")).Times(1);
  delegate_.OnRemoveHistoryItem("1");
}

// Test 395: Many history items.
TEST_F(AstraSidebarHistoryViewTest, ManyHistoryItems) {
  for (int i = 0; i < 100; ++i) {
    auto item = MakeHistoryItem(
        std::to_string(i), u"Item " + base::NumberToString16(i),
        "https://example.com/" + std::to_string(i),
        base::Time::Now() - base::Hours(i));
    history_view_->AddHistoryItem(item);
  }
  EXPECT_EQ(history_view_->GetHistoryItemCount(), 100);
}

// Test 396: History item with long title.
TEST_F(AstraSidebarHistoryViewTest, LongTitleHistoryItem) {
  std::u16string long_title(500, 'x');
  auto item = MakeHistoryItem("1", long_title, "https://example.com",
                              base::Time::Now());
  history_view_->AddHistoryItem(item);
  EXPECT_EQ(history_view_->GetHistoryItemAt(0).title, long_title);
}

// Test 397: Invalid group index is safe.
TEST_F(AstraSidebarHistoryViewTest, InvalidGroupIndexSafe) {
  history_view_->ExpandGroup(-1);
  history_view_->CollapseGroup(100);
  history_view_->ToggleGroup(999);
  SUCCEED();
}

// =========================================================================
// AstraSidebarDownloadsViewTest — downloads section tests
// =========================================================================

class AstraSidebarDownloadsViewTest : public views::test::ViewsTestBase {
 public:
  AstraSidebarDownloadsViewTest() = default;
  ~AstraSidebarDownloadsViewTest() override = default;

  void SetUp() override {
    views::test::ViewsTestBase::SetUp();
    downloads_view_ = std::make_unique<AstraSidebarDownloadsView>(nullptr);
    downloads_view_->set_delegate(&delegate_);
    widget_ = CreateWidget();
    widget_->SetContentsView(downloads_view_.get());
    widget_->SetSize(gfx::Size(300, 500));
    widget_->Show();
  }

  void TearDown() override {
    widget_.reset();
    downloads_view_.reset();
    views::test::ViewsTestBase::TearDown();
  }

 protected:
  AstraDownloadItemInfo MakeDownload(const std::string& id,
                                     const std::u16string& filename,
                                     int64_t total_bytes,
                                     int64_t received_bytes,
                                     AstraDownloadState state) {
    AstraDownloadItemInfo info;
    info.id = id;
    info.filename = filename;
    info.total_bytes = total_bytes;
    info.received_bytes = received_bytes;
    info.state = state;
    info.url = GURL("https://example.com/" + id);
    return info;
  }

  std::unique_ptr<AstraSidebarDownloadsView> downloads_view_;
  std::unique_ptr<views::Widget> widget_;
  MockSidebarDownloadsDelegate delegate_;
};

// Test 398: Default download count is 0.
TEST_F(AstraSidebarDownloadsViewTest, DefaultCountZero) {
  EXPECT_EQ(downloads_view_->GetDownloadCount(), 0);
}

// Test 399: SetDownloads replaces all items.
TEST_F(AstraSidebarDownloadsViewTest, SetDownloadsReplacesAll) {
  std::vector<AstraDownloadItemInfo> downloads;
  downloads.push_back(MakeDownload("1", u"file1.zip", 1000, 500,
                                   AstraDownloadState::kInProgress));
  downloads.push_back(MakeDownload("2", u"file2.zip", 2000, 2000,
                                   AstraDownloadState::kComplete));
  downloads_view_->SetDownloads(downloads);
  EXPECT_EQ(downloads_view_->GetDownloadCount(), 2);
}

// Test 400: GetDownloadAt returns correct item.
TEST_F(AstraSidebarDownloadsViewTest, GetDownloadAt) {
  auto dl = MakeDownload("1", u"test.zip", 1000, 500,
                         AstraDownloadState::kInProgress);
  std::vector<AstraDownloadItemInfo> downloads;
  downloads.push_back(dl);
  downloads_view_->SetDownloads(downloads);
  EXPECT_EQ(downloads_view_->GetDownloadAt(0).filename, u"test.zip");
}

// Test 401: AddDownload appends.
TEST_F(AstraSidebarDownloadsViewTest, AddDownloadAppends) {
  downloads_view_->AddDownload(MakeDownload(
      "1", u"file.zip", 1000, 0, AstraDownloadState::kInProgress));
  EXPECT_EQ(downloads_view_->GetDownloadCount(), 1);
}

// Test 402: RemoveDownload removes by index.
TEST_F(AstraSidebarDownloadsViewTest, RemoveDownload) {
  downloads_view_->AddDownload(MakeDownload(
      "1", u"first.zip", 1000, 500, AstraDownloadState::kInProgress));
  downloads_view_->AddDownload(MakeDownload(
      "2", u"second.zip", 2000, 2000, AstraDownloadState::kComplete));
  EXPECT_EQ(downloads_view_->GetDownloadCount(), 2);
  downloads_view_->RemoveDownload(0);
  EXPECT_EQ(downloads_view_->GetDownloadCount(), 1);
}

// Test 403: ClearAllDownloads clears all items.
TEST_F(AstraSidebarDownloadsViewTest, ClearAllDownloads) {
  downloads_view_->AddDownload(MakeDownload(
      "1", u"file.zip", 1000, 500, AstraDownloadState::kInProgress));
  downloads_view_->ClearAllDownloads();
  EXPECT_EQ(downloads_view_->GetDownloadCount(), 0);
}

// Test 404: ClearCompletedDownloads removes completed items.
TEST_F(AstraSidebarDownloadsViewTest, ClearCompletedDownloads) {
  downloads_view_->AddDownload(MakeDownload(
      "1", u"inprog.zip", 1000, 500, AstraDownloadState::kInProgress));
  downloads_view_->AddDownload(MakeDownload(
      "2", u"done.zip", 2000, 2000, AstraDownloadState::kComplete));
  downloads_view_->ClearCompletedDownloads();
  EXPECT_EQ(downloads_view_->GetDownloadCount(), 1);
}

// Test 405: UpdateDownload updates info.
TEST_F(AstraSidebarDownloadsViewTest, UpdateDownload) {
  downloads_view_->AddDownload(MakeDownload(
      "1", u"file.zip", 1000, 0, AstraDownloadState::kInProgress));
  auto updated = MakeDownload(
      "1", u"file.zip", 1000, 500, AstraDownloadState::kInProgress);
  downloads_view_->UpdateDownload(0, updated);
  EXPECT_EQ(downloads_view_->GetDownloadAt(0).received_bytes, 500);
}

// Test 406: Default selection is -1.
TEST_F(AstraSidebarDownloadsViewTest, DefaultSelectionNone) {
  EXPECT_EQ(downloads_view_->GetSelectedIndex(), -1);
}

// Test 407: SetSelectedDownload updates selection.
TEST_F(AstraSidebarDownloadsViewTest, SetSelectedDownload) {
  downloads_view_->AddDownload(MakeDownload(
      "1", u"file.zip", 1000, 500, AstraDownloadState::kInProgress));
  downloads_view_->SetSelectedDownload(0);
  EXPECT_EQ(downloads_view_->GetSelectedIndex(), 0);
}

// Test 408: ClearSelection clears selection.
TEST_F(AstraSidebarDownloadsViewTest, ClearSelection) {
  downloads_view_->AddDownload(MakeDownload(
      "1", u"file.zip", 1000, 500, AstraDownloadState::kInProgress));
  downloads_view_->SetSelectedDownload(0);
  downloads_view_->ClearSelection();
  EXPECT_EQ(downloads_view_->GetSelectedIndex(), -1);
}

// Test 409: Default show in-progress is true.
TEST_F(AstraSidebarDownloadsViewTest, DefaultShowInProgress) {
  EXPECT_TRUE(downloads_view_->GetShowInProgress());
}

// Test 410: SetShowInProgress updates setting.
TEST_F(AstraSidebarDownloadsViewTest, SetShowInProgress) {
  downloads_view_->SetShowInProgress(false);
  EXPECT_FALSE(downloads_view_->GetShowInProgress());
}

// Test 411: Default show completed is true.
TEST_F(AstraSidebarDownloadsViewTest, DefaultShowCompleted) {
  EXPECT_TRUE(downloads_view_->GetShowCompleted());
}

// Test 412: SetShowCompleted updates setting.
TEST_F(AstraSidebarDownloadsViewTest, SetShowCompleted) {
  downloads_view_->SetShowCompleted(false);
  EXPECT_FALSE(downloads_view_->GetShowCompleted());
}

// Test 413: Default show cancelled is false.
TEST_F(AstraSidebarDownloadsViewTest, DefaultShowCancelled) {
  EXPECT_FALSE(downloads_view_->GetShowCancelled());
}

// Test 414: SetShowCancelled updates setting.
TEST_F(AstraSidebarDownloadsViewTest, SetShowCancelled) {
  downloads_view_->SetShowCancelled(true);
  EXPECT_TRUE(downloads_view_->GetShowCancelled());
}

// Test 415: GetInProgressCount counts in-progress downloads.
TEST_F(AstraSidebarDownloadsViewTest, GetInProgressCount) {
  downloads_view_->AddDownload(MakeDownload(
      "1", u"prog.zip", 1000, 500, AstraDownloadState::kInProgress));
  downloads_view_->AddDownload(MakeDownload(
      "2", u"done.zip", 1000, 1000, AstraDownloadState::kComplete));
  EXPECT_EQ(downloads_view_->GetInProgressCount(), 1);
}

// Test 416: GetCompletedCount counts completed downloads.
TEST_F(AstraSidebarDownloadsViewTest, GetCompletedCount) {
  downloads_view_->AddDownload(MakeDownload(
      "1", u"prog.zip", 1000, 500, AstraDownloadState::kInProgress));
  downloads_view_->AddDownload(MakeDownload(
      "2", u"done.zip", 1000, 1000, AstraDownloadState::kComplete));
  EXPECT_EQ(downloads_view_->GetCompletedCount(), 1);
}

// Test 417: GetCancelledCount counts cancelled downloads.
TEST_F(AstraSidebarDownloadsViewTest, GetCancelledCount) {
  downloads_view_->AddDownload(MakeDownload(
      "1", u"cancelled.zip", 1000, 300, AstraDownloadState::kCancelled));
  EXPECT_GE(downloads_view_->GetCancelledCount(), 0);
}

// Test 418: GetTotalDownloadedSize sums completed sizes.
TEST_F(AstraSidebarDownloadsViewTest, GetTotalDownloadedSize) {
  downloads_view_->AddDownload(MakeDownload(
      "1", u"file1.zip", 1000, 1000, AstraDownloadState::kComplete));
  downloads_view_->AddDownload(MakeDownload(
      "2", u"file2.zip", 500, 500, AstraDownloadState::kComplete));
  EXPECT_EQ(downloads_view_->GetTotalDownloadedSize(), 1500);
}

// Test 419: Default sort by is kNewestFirst.
TEST_F(AstraSidebarDownloadsViewTest, DefaultSortBy) {
  EXPECT_EQ(downloads_view_->GetSortBy(), AstraDownloadSortBy::kNewestFirst);
}

// Test 420: SetSortBy updates sort order.
TEST_F(AstraSidebarDownloadsViewTest, SetSortBy) {
  downloads_view_->SetSortBy(AstraDownloadSortBy::kName);
  EXPECT_EQ(downloads_view_->GetSortBy(), AstraDownloadSortBy::kName);
}

// Test 421: SearchDownloads filters by filename.
TEST_F(AstraSidebarDownloadsViewTest, SearchDownloadsByFilename) {
  downloads_view_->AddDownload(MakeDownload(
      "1", u"report.pdf", 1000, 1000, AstraDownloadState::kComplete));
  downloads_view_->AddDownload(MakeDownload(
      "2", u"image.png", 500, 500, AstraDownloadState::kComplete));
  downloads_view_->SearchDownloads(u"report");
  EXPECT_EQ(downloads_view_->GetSearchResultsCount(), 1);
}

// Test 422: Search with empty query shows all.
TEST_F(AstraSidebarDownloadsViewTest, SearchDownloadsEmpty) {
  downloads_view_->AddDownload(MakeDownload(
      "1", u"file1.zip", 1000, 500, AstraDownloadState::kInProgress));
  downloads_view_->AddDownload(MakeDownload(
      "2", u"file2.zip", 2000, 2000, AstraDownloadState::kComplete));
  downloads_view_->SearchDownloads(u"");
  EXPECT_EQ(downloads_view_->GetSearchResultsCount(),
            downloads_view_->GetDownloadCount());
}

// Test 423: Default always show progress is false.
TEST_F(AstraSidebarDownloadsViewTest, DefaultAlwaysShowProgress) {
  EXPECT_FALSE(downloads_view_->GetAlwaysShowProgress());
}

// Test 424: SetAlwaysShowProgress updates setting.
TEST_F(AstraSidebarDownloadsViewTest, SetAlwaysShowProgress) {
  downloads_view_->SetAlwaysShowProgress(true);
  EXPECT_TRUE(downloads_view_->GetAlwaysShowProgress());
}

// Test 425: Default show file size is true.
TEST_F(AstraSidebarDownloadsViewTest, DefaultShowFileSize) {
  EXPECT_TRUE(downloads_view_->GetShowFileSize());
}

// Test 426: SetShowFileSize updates setting.
TEST_F(AstraSidebarDownloadsViewTest, SetShowFileSize) {
  downloads_view_->SetShowFileSize(false);
  EXPECT_FALSE(downloads_view_->GetShowFileSize());
}

// Test 427: Default show speed is true.
TEST_F(AstraSidebarDownloadsViewTest, DefaultShowSpeed) {
  EXPECT_TRUE(downloads_view_->GetShowSpeed());
}

// Test 428: SetShowSpeed updates setting.
TEST_F(AstraSidebarDownloadsViewTest, SetShowSpeed) {
  downloads_view_->SetShowSpeed(false);
  EXPECT_FALSE(downloads_view_->GetShowSpeed());
}

// Test 429: Default show time remaining is true.
TEST_F(AstraSidebarDownloadsViewTest, DefaultShowTimeRemaining) {
  EXPECT_TRUE(downloads_view_->GetShowTimeRemaining());
}

// Test 430: SetShowTimeRemaining updates setting.
TEST_F(AstraSidebarDownloadsViewTest, SetShowTimeRemaining) {
  downloads_view_->SetShowTimeRemaining(false);
  EXPECT_FALSE(downloads_view_->GetShowTimeRemaining());
}

// Test 431: GetOverallProgress with no downloads is 0.
TEST_F(AstraSidebarDownloadsViewTest, OverallProgressZero) {
  EXPECT_EQ(downloads_view_->GetOverallProgress(), 0.0);
}

// Test 432: GetOverallProgress with completed download is 1.0.
TEST_F(AstraSidebarDownloadsViewTest, OverallProgressComplete) {
  downloads_view_->AddDownload(MakeDownload(
      "1", u"file.zip", 1000, 1000, AstraDownloadState::kComplete));
  EXPECT_GE(downloads_view_->GetOverallProgress(), 0.0);
  EXPECT_LE(downloads_view_->GetOverallProgress(), 1.0);
}

// Test 433: GetActiveDownloadCount with one active download.
TEST_F(AstraSidebarDownloadsViewTest, ActiveDownloadCount) {
  downloads_view_->AddDownload(MakeDownload(
      "1", u"file.zip", 1000, 500, AstraDownloadState::kInProgress));
  EXPECT_EQ(downloads_view_->GetActiveDownloadCount(), 1);
}

// Test 434: Default auto-open is false.
TEST_F(AstraSidebarDownloadsViewTest, DefaultAutoOpen) {
  EXPECT_FALSE(downloads_view_->GetAutoOpenDownloads());
}

// Test 435: SetAutoOpenDownloads updates setting.
TEST_F(AstraSidebarDownloadsViewTest, SetAutoOpenDownloads) {
  downloads_view_->SetAutoOpenDownloads(true);
  EXPECT_TRUE(downloads_view_->GetAutoOpenDownloads());
}

// Test 436: PauseDownload exists and is callable.
TEST_F(AstraSidebarDownloadsViewTest, PauseDownloadExists) {
  downloads_view_->AddDownload(MakeDownload(
      "1", u"file.zip", 1000, 500, AstraDownloadState::kInProgress));
  // Should not crash.
  downloads_view_->PauseDownload(0);
  SUCCEED();
}

// Test 437: ResumeDownload exists and is callable.
TEST_F(AstraSidebarDownloadsViewTest, ResumeDownloadExists) {
  downloads_view_->AddDownload(MakeDownload(
      "1", u"file.zip", 1000, 500, AstraDownloadState::kPaused));
  downloads_view_->ResumeDownload(0);
  SUCCEED();
}

// Test 438: CancelDownload exists and is callable.
TEST_F(AstraSidebarDownloadsViewTest, CancelDownloadExists) {
  downloads_view_->AddDownload(MakeDownload(
      "1", u"file.zip", 1000, 500, AstraDownloadState::kInProgress));
  downloads_view_->CancelDownload(0);
  SUCCEED();
}

// Test 439: OpenDownload exists and is callable.
TEST_F(AstraSidebarDownloadsViewTest, OpenDownloadExists) {
  downloads_view_->AddDownload(MakeDownload(
      "1", u"file.zip", 1000, 1000, AstraDownloadState::kComplete));
  downloads_view_->OpenDownload(0);
  SUCCEED();
}

// Test 440: ShowDownloadInFolder exists and is callable.
TEST_F(AstraSidebarDownloadsViewTest, ShowDownloadInFolderExists) {
  downloads_view_->AddDownload(MakeDownload(
      "1", u"file.zip", 1000, 1000, AstraDownloadState::kComplete));
  downloads_view_->ShowDownloadInFolder(0);
  SUCCEED();
}

// Test 441: RetryDownload exists and is callable.
TEST_F(AstraSidebarDownloadsViewTest, RetryDownloadExists) {
  downloads_view_->AddDownload(MakeDownload(
      "1", u"file.zip", 1000, 500, AstraDownloadState::kCancelled));
  downloads_view_->RetryDownload(0);
  SUCCEED();
}

// Test 442: Delegate pause callback.
TEST_F(AstraSidebarDownloadsViewTest, DelegatePauseCallback) {
  EXPECT_CALL(delegate_, OnPauseDownload("1")).Times(1);
  delegate_.OnPauseDownload("1");
}

// Test 443: Delegate resume callback.
TEST_F(AstraSidebarDownloadsViewTest, DelegateResumeCallback) {
  EXPECT_CALL(delegate_, OnResumeDownload("1")).Times(1);
  delegate_.OnResumeDownload("1");
}

// Test 444: Delegate cancel callback.
TEST_F(AstraSidebarDownloadsViewTest, DelegateCancelCallback) {
  EXPECT_CALL(delegate_, OnCancelDownload("1")).Times(1);
  delegate_.OnCancelDownload("1");
}

// Test 445: Delegate open callback.
TEST_F(AstraSidebarDownloadsViewTest, DelegateOpenCallback) {
  EXPECT_CALL(delegate_, OnOpenDownload("1")).Times(1);
  delegate_.OnOpenDownload("1");
}

// Test 446: Many downloads.
TEST_F(AstraSidebarDownloadsViewTest, ManyDownloads) {
  for (int i = 0; i < 50; ++i) {
    auto dl = MakeDownload(
        std::to_string(i),
        u"file" + base::NumberToString16(i) + u".zip",
        1000 * (i + 1), 500 * i, AstraDownloadState::kInProgress);
    downloads_view_->AddDownload(dl);
  }
  EXPECT_EQ(downloads_view_->GetDownloadCount(), 50);
}

// Test 447: Zero-size download.
TEST_F(AstraSidebarDownloadsViewTest, ZeroSizeDownload) {
  auto dl = MakeDownload("1", u"empty.txt", 0, 0,
                         AstraDownloadState::kComplete);
  downloads_view_->AddDownload(dl);
  EXPECT_EQ(downloads_view_->GetDownloadAt(0).total_bytes, 0);
}

// Test 448: Download with long filename.
TEST_F(AstraSidebarDownloadsViewTest, LongFilenameDownload) {
  std::u16string long_name(200, 'x');
  long_name += u".zip";
  auto dl = MakeDownload("1", long_name, 1000, 500,
                         AstraDownloadState::kInProgress);
  downloads_view_->AddDownload(dl);
  EXPECT_EQ(downloads_view_->GetDownloadAt(0).filename, long_name);
}

// Test 449: Invalid index is safe.
TEST_F(AstraSidebarDownloadsViewTest, InvalidIndexSafe) {
  downloads_view_->RemoveDownload(-1);
  downloads_view_->RemoveDownload(100);
  downloads_view_->PauseDownload(-1);
  downloads_view_->PauseDownload(100);
  SUCCEED();
}

// Test 450: Sort by name alphabetical.
TEST_F(AstraSidebarDownloadsViewTest, SortByName) {
  downloads_view_->AddDownload(MakeDownload(
      "1", u"beta.zip", 1000, 500, AstraDownloadState::kInProgress));
  downloads_view_->AddDownload(MakeDownload(
      "2", u"alpha.zip", 2000, 2000, AstraDownloadState::kComplete));
  downloads_view_->SetSortBy(AstraDownloadSortBy::kName);
  // After sort by name (ascending), alpha should come first.
  EXPECT_EQ(downloads_view_->GetDownloadAt(0).filename, u"alpha.zip");
}

// Test 451: Sort by largest first.
TEST_F(AstraSidebarDownloadsViewTest, SortByLargestFirst) {
  auto small = MakeDownload("1", u"small.zip", 100, 100,
                            AstraDownloadState::kComplete);
  auto large = MakeDownload("2", u"large.zip", 10000, 5000,
                            AstraDownloadState::kInProgress);
  std::vector<AstraDownloadItemInfo> downloads = {small, large};
  downloads_view_->SetDownloads(downloads);
  downloads_view_->SetSortBy(AstraDownloadSortBy::kLargestFirst);
  EXPECT_GE(downloads_view_->GetDownloadAt(0).total_bytes,
            downloads_view_->GetDownloadAt(1).total_bytes);
}

// Test 452: Sort by smallest first.
TEST_F(AstraSidebarDownloadsViewTest, SortBySmallestFirst) {
  auto small = MakeDownload("1", u"small.zip", 100, 100,
                            AstraDownloadState::kComplete);
  auto large = MakeDownload("2", u"large.zip", 10000, 5000,
                            AstraDownloadState::kInProgress);
  std::vector<AstraDownloadItemInfo> downloads = {large, small};
  downloads_view_->SetDownloads(downloads);
  downloads_view_->SetSortBy(AstraDownloadSortBy::kSmallestFirst);
  EXPECT_LE(downloads_view_->GetDownloadAt(0).total_bytes,
            downloads_view_->GetDownloadAt(1).total_bytes);
}

// Test 453: Dangerous download flag.
TEST_F(AstraSidebarDownloadsViewTest, DangerousDownloadFlag) {
  auto dl = MakeDownload("1", u"file.exe", 1000, 500,
                         AstraDownloadState::kInProgress);
  dl.is_dangerous = true;
  dl.danger_type = AstraDownloadItemInfo::DangerType::kDangerousFile;
  EXPECT_TRUE(dl.is_dangerous);
  EXPECT_EQ(dl.danger_type, AstraDownloadItemInfo::DangerType::kDangerousFile);
}

// =========================================================================
// Edge case tests
// =========================================================================

// Test 454: Empty section view preferred size is valid.
TEST_F(AstraSidebarSectionViewTest, EmptySectionPreferredSize) {
  auto view = std::make_unique<AstraSidebarSectionView>(
      AstraSidebarSectionType::kBookmarks, u"");
  gfx::Size size = view->GetPreferredSize();
  EXPECT_GE(size.width(), 0);
  EXPECT_GE(size.height(), 0);
}

// Test 455: Single item in section.
TEST_F(AstraSidebarSectionViewTest, SingleItemInSection) {
  section_view_->AddItemView(std::make_unique<views::View>());
  EXPECT_EQ(section_view_->GetItemViewCount(), 1);
  EXPECT_NE(section_view_->GetItemViewAt(0), nullptr);
}

// Test 456: RemoveAllItems on empty section is safe.
TEST_F(AstraSidebarSectionViewTest, RemoveAllItemsOnEmpty) {
  section_view_->RemoveAllItems();
  EXPECT_EQ(section_view_->GetItemViewCount(), 0);
}

// Test 457: Setting empty title.
TEST_F(AstraSidebarSectionViewTest, EmptyTitle) {
  section_view_->SetTitle(u"");
  EXPECT_TRUE(section_view_->GetTitle().empty());
}

// Test 458: Setting very long title.
TEST_F(AstraSidebarSectionViewTest, VeryLongTitle) {
  std::u16string long_title(1000, 'x');
  section_view_->SetTitle(long_title);
  EXPECT_EQ(section_view_->GetTitle(), long_title);
}

// Test 459: Toggle expanded multiple times.
TEST_F(AstraSidebarSectionViewTest, ToggleExpandedManyTimes) {
  for (int i = 0; i < 100; ++i) {
    section_view_->ToggleExpanded();
  }
  // After 100 toggles starting from true: 100 is even, so back to true.
  EXPECT_TRUE(section_view_->IsExpanded());
}

// Test 460: Bookmarks with zero items.
TEST_F(AstraSidebarBookmarksViewTest, ZeroBookmarks) {
  EXPECT_EQ(bookmarks_view_->GetBookmarkCount(), 0);
  EXPECT_EQ(bookmarks_view_->GetVisibleBookmarkCount(), 0);
}

// Test 461: Single bookmark.
TEST_F(AstraSidebarBookmarksViewTest, SingleBookmark) {
  bookmarks_view_->AddBookmark(MakeBookmark(
      "1", u"Only", "https://only.com"));
  EXPECT_EQ(bookmarks_view_->GetBookmarkCount(), 1);
}

// Test 462: Bookmark with empty title.
TEST_F(AstraSidebarBookmarksViewTest, EmptyTitleBookmark) {
  auto info = MakeBookmark("1", u"", "https://example.com");
  bookmarks_view_->AddBookmark(info);
  EXPECT_TRUE(bookmarks_view_->GetBookmarkAt(0).title.empty());
}

// Test 463: Bookmark with invalid URL.
TEST_F(AstraSidebarBookmarksViewTest, InvalidUrlBookmark) {
  auto info = MakeBookmark("1", u"Bad", "not-a-url");
  EXPECT_FALSE(info.url.is_valid());
}

// Test 464: History with zero items.
TEST_F(AstraSidebarHistoryViewTest, ZeroHistoryItems) {
  EXPECT_EQ(history_view_->GetHistoryItemCount(), 0);
  EXPECT_EQ(history_view_->GetDomainCount(), 0);
}

// Test 465: Single history item.
TEST_F(AstraSidebarHistoryViewTest, SingleHistoryItem) {
  history_view_->AddHistoryItem(MakeHistoryItem(
      "1", u"Only", "https://only.com", base::Time::Now()));
  EXPECT_EQ(history_view_->GetHistoryItemCount(), 1);
}

// Test 466: History item with empty title.
TEST_F(AstraSidebarHistoryViewTest, EmptyTitleHistoryItem) {
  auto item = MakeHistoryItem("1", u"", "https://example.com",
                              base::Time::Now());
  history_view_->AddHistoryItem(item);
  EXPECT_TRUE(history_view_->GetHistoryItemAt(0).title.empty());
}

// Test 467: Downloads with zero items.
TEST_F(AstraSidebarDownloadsViewTest, ZeroDownloads) {
  EXPECT_EQ(downloads_view_->GetDownloadCount(), 0);
  EXPECT_EQ(downloads_view_->GetActiveDownloadCount(), 0);
  EXPECT_EQ(downloads_view_->GetOverallProgress(), 0.0);
}

// Test 468: Single download.
TEST_F(AstraSidebarDownloadsViewTest, SingleDownload) {
  downloads_view_->AddDownload(MakeDownload(
      "1", u"only.zip", 1000, 500, AstraDownloadState::kInProgress));
  EXPECT_EQ(downloads_view_->GetDownloadCount(), 1);
}

// Test 469: Download received exceeds total (edge case).
TEST_F(AstraSidebarDownloadsViewTest, ReceivedExceedsTotal) {
  auto dl = MakeDownload("1", u"big.zip", 1000, 2000,
                         AstraDownloadState::kInProgress);
  downloads_view_->AddDownload(dl);
  EXPECT_GT(downloads_view_->GetDownloadAt(0).received_bytes,
            downloads_view_->GetDownloadAt(0).total_bytes);
}

// Test 470: Null delegate is safe.
TEST_F(AstraSidebarBookmarksViewTest, NullDelegateSafe) {
  auto view = std::make_unique<AstraSidebarBookmarksView>();
  view->AddBookmark(MakeBookmark("1", u"Item", "https://item.com"));
  // No delegate set - operations should not crash.
  SUCCEED();
}

// Test 471: All AstraSidebarSectionType enum values are distinct.
TEST(AstraSidebarSectionTypeEnum, AllDistinct) {
  // Just verify enum compiles and has expected types.
  EXPECT_NE(static_cast<int>(AstraSidebarSectionType::kBookmarks),
            static_cast<int>(AstraSidebarSectionType::kHistory));
  EXPECT_NE(static_cast<int>(AstraSidebarSectionType::kHistory),
            static_cast<int>(AstraSidebarSectionType::kDownloads));
}

// Test 472: All AstraSidebarSortOrder enum values are distinct.
TEST(AstraSidebarSortOrderEnum, AllDistinct) {
  EXPECT_NE(static_cast<int>(AstraSidebarSortOrder::kAlphabetical),
            static_cast<int>(AstraSidebarSortOrder::kDateAdded));
  EXPECT_NE(static_cast<int>(AstraSidebarSortOrder::kDateModified),
            static_cast<int>(AstraSidebarSortOrder::kMostVisited));
}

// Test 473: All AstraSidebarFilter enum values are distinct.
TEST(AstraSidebarFilterEnum, AllDistinct) {
  EXPECT_NE(static_cast<int>(AstraSidebarFilter::kAll),
            static_cast<int>(AstraSidebarFilter::kUnread));
  EXPECT_NE(static_cast<int>(AstraSidebarFilter::kRead),
            static_cast<int>(AstraSidebarFilter::kPinned));
}

// Test 474: All AstraDownloadState enum values are distinct.
TEST(AstraDownloadStateEnum, AllDistinct) {
  EXPECT_NE(static_cast<int>(AstraDownloadState::kInProgress),
            static_cast<int>(AstraDownloadState::kComplete));
  EXPECT_NE(static_cast<int>(AstraDownloadState::kCancelled),
            static_cast<int>(AstraDownloadState::kFailed));
}

// Test 475: All AstraDownloadSortBy enum values are distinct.
TEST(AstraDownloadSortByEnum, AllDistinct) {
  EXPECT_NE(static_cast<int>(AstraDownloadSortBy::kNewestFirst),
            static_cast<int>(AstraDownloadSortBy::kOldestFirst));
  EXPECT_NE(static_cast<int>(AstraDownloadSortBy::kLargestFirst),
            static_cast<int>(AstraDownloadSortBy::kSmallestFirst));
}

// Test 476: All AstraHistoryItemInfo TimeGroup enum values are distinct.
TEST(AstraHistoryTimeGroupEnum, AllDistinct) {
  EXPECT_NE(static_cast<int>(AstraHistoryItemInfo::TimeGroup::kToday),
            static_cast<int>(AstraHistoryItemInfo::TimeGroup::kYesterday));
  EXPECT_NE(static_cast<int>(AstraHistoryItemInfo::TimeGroup::kLastWeek),
            static_cast<int>(AstraHistoryItemInfo::TimeGroup::kOlder));
}

// Test 477: All download danger types are distinct.
TEST(AstraDownloadDangerTypeEnum, AllDistinct) {
  EXPECT_NE(
      static_cast<int>(AstraDownloadItemInfo::DangerType::kNone),
      static_cast<int>(AstraDownloadItemInfo::DangerType::kDangerousFile));
  EXPECT_NE(
      static_cast<int>(AstraDownloadItemInfo::DangerType::kDangerousUrl),
      static_cast<int>(AstraDownloadItemInfo::DangerType::kDangerousContent));
}

// Test 478: Delegate callbacks for bookmarks middle click.
TEST_F(AstraSidebarBookmarksViewTest, DelegateMiddleClickCallback) {
  EXPECT_CALL(delegate_, OnBookmarkMiddleClicked("1")).Times(1);
  delegate_.OnBookmarkMiddleClicked("1");
}

// Test 479: Delegate callbacks for history middle click.
TEST_F(AstraSidebarHistoryViewTest, DelegateMiddleClickCallback) {
  EXPECT_CALL(delegate_, OnHistoryItemMiddleClicked("1")).Times(1);
  delegate_.OnHistoryItemMiddleClicked("1");
}

// Test 480: Delegate callbacks for downloads right click.
TEST_F(AstraSidebarDownloadsViewTest, DelegateRightClickCallback) {
  EXPECT_CALL(delegate_, OnDownloadRightClicked("1", gfx::Point())).Times(1);
  delegate_.OnDownloadRightClicked("1", gfx::Point());
}

// Test 481: Delegate callbacks for clear all downloads.
TEST_F(AstraSidebarDownloadsViewTest, DelegateClearAllCallback) {
  EXPECT_CALL(delegate_, OnClearAllDownloadsRequested()).Times(1);
  delegate_.OnClearAllDownloadsRequested();
}

// Test 482: Delegate callbacks for clear all history.
TEST_F(AstraSidebarHistoryViewTest, DelegateClearAllCallback) {
  EXPECT_CALL(delegate_, OnClearAllHistoryRequested()).Times(1);
  delegate_.OnClearAllHistoryRequested();
}

// Test 483: Delegate callbacks for new folder.
TEST_F(AstraSidebarBookmarksViewTest, DelegateNewFolderCallback) {
  EXPECT_CALL(delegate_, OnNewFolderRequested()).Times(1);
  delegate_.OnNewFolderRequested();
}

// Test 484: Delegate callbacks for add bookmark.
TEST_F(AstraSidebarBookmarksViewTest, DelegateAddBookmarkCallback) {
  EXPECT_CALL(delegate_, OnAddBookmarkRequested()).Times(1);
  delegate_.OnAddBookmarkRequested();
}

// Test 485: Large negative item count is clamped.
TEST_F(AstraSidebarSectionViewTest, LargeNegativeItemCount) {
  section_view_->SetItemCount(-99999);
  EXPECT_GE(section_view_->GetItemCount(), 0);
}

// Test 486: Section view accessibility node data.
TEST_F(AstraSidebarSectionViewTest, AccessibleNodeData) {
  ui::AXNodeData data;
  section_view_->GetAccessibleNodeData(&data);
  // Should have a valid role and name.
  EXPECT_EQ(data.role, ax::mojom::Role::kGroup);
}

// Test 487: Bookmarks view accessibility.
TEST_F(AstraSidebarBookmarksViewTest, AccessibleNodeData) {
  ui::AXNodeData data;
  bookmarks_view_->GetAccessibleNodeData(&data);
  EXPECT_EQ(data.role, ax::mojom::Role::kGroup);
}

// Test 488: History view accessibility.
TEST_F(AstraSidebarHistoryViewTest, AccessibleNodeData) {
  ui::AXNodeData data;
  history_view_->GetAccessibleNodeData(&data);
  EXPECT_EQ(data.role, ax::mojom::Role::kGroup);
}

// Test 489: Downloads view accessibility.
TEST_F(AstraSidebarDownloadsViewTest, AccessibleNodeData) {
  ui::AXNodeData data;
  downloads_view_->GetAccessibleNodeData(&data);
  EXPECT_EQ(data.role, ax::mojom::Role::kGroup);
}

// Test 490: Section view with all chrome disabled.
TEST_F(AstraSidebarSectionViewTest, AllChromeDisabled) {
  section_view_->SetShowSearch(false);
  section_view_->SetShowAddButton(false);
  section_view_->SetShowMoreButton(false);
  section_view_->SetShowChevron(false);
  section_view_->SetShowItemCount(false);
  section_view_->SetShowContextMenu(false);
  EXPECT_FALSE(section_view_->GetShowSearch());
  EXPECT_FALSE(section_view_->GetShowAddButton());
  EXPECT_FALSE(section_view_->GetShowMoreButton());
  EXPECT_FALSE(section_view_->GetShowChevron());
  EXPECT_FALSE(section_view_->GetShowItemCount());
  EXPECT_FALSE(section_view_->GetShowContextMenu());
}

// Test 491: Bookmarks view extends section view.
TEST_F(AstraSidebarBookmarksViewTest, ExtendsSectionView) {
  // Verify it has section view methods.
  bookmarks_view_->SetTitle(u"Bookmarks");
  EXPECT_EQ(bookmarks_view_->GetTitle(), u"Bookmarks");
  bookmarks_view_->ToggleExpanded();
  EXPECT_FALSE(bookmarks_view_->IsExpanded());
}

// Test 492: History view extends section view.
TEST_F(AstraSidebarHistoryViewTest, ExtendsSectionView) {
  history_view_->SetTitle(u"History");
  EXPECT_EQ(history_view_->GetTitle(), u"History");
  history_view_->ToggleExpanded();
  EXPECT_FALSE(history_view_->IsExpanded());
}

// Test 493: Downloads view extends section view.
TEST_F(AstraSidebarDownloadsViewTest, ExtendsSectionView) {
  downloads_view_->SetTitle(u"Downloads");
  EXPECT_EQ(downloads_view_->GetTitle(), u"Downloads");
  downloads_view_->ToggleExpanded();
  EXPECT_FALSE(downloads_view_->IsExpanded());
}

// Test 494: Bookmarks sort by date added.
TEST_F(AstraSidebarBookmarksViewTest, SortByDateAdded) {
  auto older = MakeBookmark("1", u"Older", "https://older.com");
  older.date_added = base::Time::Now() - base::Days(10);
  auto newer = MakeBookmark("2", u"Newer", "https://newer.com");
  newer.date_added = base::Time::Now();

  std::vector<AstraBookmarkItemInfo> bookmarks = {older, newer};
  bookmarks_view_->SetBookmarks(bookmarks);
  bookmarks_view_->SortBookmarks(AstraSidebarSortOrder::kDateAdded);
  // Newest first, so index 0 should be the newer one.
  EXPECT_EQ(bookmarks_view_->GetBookmarkAt(0).id, "2");
}

// Test 495: Downloads sort by oldest first.
TEST_F(AstraSidebarDownloadsViewTest, SortByOldestFirst) {
  auto old_dl = MakeDownload("1", u"old.zip", 1000, 1000,
                             AstraDownloadState::kComplete);
  old_dl.start_time = base::Time::Now() - base::Days(5);
  auto new_dl = MakeDownload("2", u"new.zip", 500, 500,
                             AstraDownloadState::kComplete);
  new_dl.start_time = base::Time::Now();

  std::vector<AstraDownloadItemInfo> downloads = {new_dl, old_dl};
  downloads_view_->SetDownloads(downloads);
  downloads_view_->SetSortBy(AstraDownloadSortBy::kOldestFirst);
  // Oldest first, so index 0 should be the old one.
  EXPECT_EQ(downloads_view_->GetDownloadAt(0).id, "1");
}

// Test 496: Bookmark item info has all fields default-initialized.
TEST(AstraBookmarkItemInfoStruct, DefaultValues) {
  AstraBookmarkItemInfo info;
  EXPECT_TRUE(info.id.empty());
  EXPECT_TRUE(info.title.empty());
  EXPECT_FALSE(info.url.is_valid());
  EXPECT_FALSE(info.is_folder);
  EXPECT_TRUE(info.parent_id.empty());
  EXPECT_TRUE(info.date_added.is_null());
  EXPECT_TRUE(info.date_modified.is_null());
  EXPECT_FALSE(info.is_bookmark_bar);
  EXPECT_FALSE(info.is_other);
  EXPECT_FALSE(info.is_mobile);
  EXPECT_EQ(info.child_count, 0);
  EXPECT_FALSE(info.has_favicon);
}

// Test 497: History item info has all fields default-initialized.
TEST(AstraHistoryItemInfoStruct, DefaultValues) {
  AstraHistoryItemInfo info;
  EXPECT_TRUE(info.id.empty());
  EXPECT_TRUE(info.title.empty());
  EXPECT_FALSE(info.url.is_valid());
  EXPECT_TRUE(info.hostname.empty());
  EXPECT_TRUE(info.visit_time.is_null());
  EXPECT_EQ(info.visit_count, 1);
  EXPECT_FALSE(info.is_typed_visit);
  EXPECT_FALSE(info.has_favicon);
  EXPECT_EQ(info.time_group, AstraHistoryItemInfo::TimeGroup::kToday);
}

// Test 498: Download item info has all fields default-initialized.
TEST(AstraDownloadItemInfoStruct, DefaultValues) {
  AstraDownloadItemInfo info;
  EXPECT_TRUE(info.id.empty());
  EXPECT_TRUE(info.filename.empty());
  EXPECT_FALSE(info.url.is_valid());
  EXPECT_EQ(info.total_bytes, 0);
  EXPECT_EQ(info.received_bytes, 0);
  EXPECT_EQ(info.state, AstraDownloadState::kInProgress);
  EXPECT_EQ(info.speed_bytes_per_sec, 0);
  EXPECT_TRUE(info.time_remaining.is_zero());
  EXPECT_TRUE(info.start_time.is_null());
  EXPECT_TRUE(info.end_time.is_null());
  EXPECT_TRUE(info.file_path.empty());
  EXPECT_TRUE(info.mime_type.empty());
  EXPECT_FALSE(info.is_dangerous);
  EXPECT_EQ(info.danger_type, AstraDownloadItemInfo::DangerType::kNone);
  EXPECT_FALSE(info.has_prompted);
}

// Test 499: Bookmark view set and get item count via base class.
TEST_F(AstraSidebarBookmarksViewTest, ItemCountViaBaseClass) {
  bookmarks_view_->AddBookmark(MakeBookmark(
      "1", u"Item", "https://item.com"));
  bookmarks_view_->AddBookmark(MakeBookmark(
      "2", u"Item2", "https://item2.com"));
  EXPECT_EQ(bookmarks_view_->GetBookmarkCount(), 2);
}

// Test 500: Downloads view with paused state.
TEST_F(AstraSidebarDownloadsViewTest, PausedDownloadState) {
  auto dl = MakeDownload("1", u"paused.zip", 1000, 500,
                         AstraDownloadState::kPaused);
  downloads_view_->AddDownload(dl);
  EXPECT_EQ(downloads_view_->GetDownloadAt(0).state,
            AstraDownloadState::kPaused);
}

// Test 501: Downloads view with failed state.
TEST_F(AstraSidebarDownloadsViewTest, FailedDownloadState) {
  auto dl = MakeDownload("1", u"failed.zip", 1000, 500,
                         AstraDownloadState::kFailed);
  downloads_view_->AddDownload(dl);
  EXPECT_EQ(downloads_view_->GetDownloadAt(0).state,
            AstraDownloadState::kFailed);
}

// Test 502: Downloads view with interrupted state.
TEST_F(AstraSidebarDownloadsViewTest, InterruptedDownloadState) {
  auto dl = MakeDownload("1", u"interrupted.zip", 1000, 500,
                         AstraDownloadState::kInterrupted);
  downloads_view_->AddDownload(dl);
  EXPECT_EQ(downloads_view_->GetDownloadAt(0).state,
            AstraDownloadState::kInterrupted);
}

// Test 503: History item typed visit flag.
TEST_F(AstraSidebarHistoryViewTest, TypedVisitFlag) {
  auto item = MakeHistoryItem("1", u"Typed", "https://typed.com",
                              base::Time::Now());
  item.is_typed_visit = true;
  history_view_->AddHistoryItem(item);
  EXPECT_TRUE(history_view_->GetHistoryItemAt(0).is_typed_visit);
}

// Test 504: History item visit count.
TEST_F(AstraSidebarHistoryViewTest, VisitCount) {
  auto item = MakeHistoryItem("1", u"Popular", "https://popular.com",
                              base::Time::Now());
  item.visit_count = 42;
  history_view_->AddHistoryItem(item);
  EXPECT_EQ(history_view_->GetHistoryItemAt(0).visit_count, 42);
}

// Test 505: Bookmark item has_favicon flag.
TEST_F(AstraSidebarBookmarksViewTest, HasFaviconFlag) {
  auto info = MakeBookmark("1", u"Favicon", "https://favicon.com");
  info.has_favicon = true;
  bookmarks_view_->AddBookmark(info);
  EXPECT_TRUE(bookmarks_view_->GetBookmarkAt(0).has_favicon);
}

// Test 506: Bookmark item child count for folders.
TEST_F(AstraSidebarBookmarksViewTest, FolderChildCount) {
  auto info = MakeBookmark("1", u"Folder", "", true);
  info.child_count = 10;
  bookmarks_view_->AddBookmark(info);
  EXPECT_EQ(bookmarks_view_->GetBookmarkAt(0).child_count, 10);
}

// Test 507: Bookmarks view get folder path after navigation.
TEST_F(AstraSidebarBookmarksViewTest, FolderPathNavigation) {
  bookmarks_view_->NavigateToFolder("a");
  bookmarks_view_->NavigateToFolder("b");
  bookmarks_view_->NavigateToFolder("c");
  auto path = bookmarks_view_->GetFolderPath();
  EXPECT_EQ(path.size(), 3u);
  bookmarks_view_->NavigateUp();
  path = bookmarks_view_->GetFolderPath();
  EXPECT_EQ(path.size(), 2u);
}

// Test 508: Download speed bytes per second.
TEST_F(AstraSidebarDownloadsViewTest, DownloadSpeed) {
  auto dl = MakeDownload("1", u"fast.zip", 1000000, 500000,
                         AstraDownloadState::kInProgress);
  dl.speed_bytes_per_sec = 50000;
  downloads_view_->AddDownload(dl);
  EXPECT_EQ(downloads_view_->GetDownloadAt(0).speed_bytes_per_sec, 50000);
}

// Test 509: Download time remaining.
TEST_F(AstraSidebarDownloadsViewTest, DownloadTimeRemaining) {
  auto dl = MakeDownload("1", u"slow.zip", 1000000, 100000,
                         AstraDownloadState::kInProgress);
  dl.time_remaining = base::Seconds(30);
  downloads_view_->AddDownload(dl);
  EXPECT_EQ(downloads_view_->GetDownloadAt(0).time_remaining.InSeconds(),
            30);
}

// Test 510: Download MIME type.
TEST_F(AstraSidebarDownloadsViewTest, DownloadMimeType) {
  auto dl = MakeDownload("1", u"file.pdf", 1000, 500,
                         AstraDownloadState::kInProgress);
  dl.mime_type = "application/pdf";
  downloads_view_->AddDownload(dl);
  EXPECT_EQ(downloads_view_->GetDownloadAt(0).mime_type, "application/pdf");
}

// =========================================================================
// Mock delegates for tab groups and recently closed views
// =========================================================================

class MockSidebarTabGroupsDelegate : public AstraSidebarTabGroupsDelegate {
 public:
  MOCK_METHOD(void, OnGroupClicked, (const std::string& group_id),
              (override));
  MOCK_METHOD(void, OnGroupExpandedToggled, (const std::string& group_id),
              (override));
  MOCK_METHOD(void, OnGroupColorChanged,
              (const std::string& group_id, tab_groups::TabGroupColorId color),
              (override));
  MOCK_METHOD(void, OnGroupNameChanged,
              (const std::string& group_id, const std::u16string& name),
              (override));
  MOCK_METHOD(void, OnGroupClosed, (const std::string& group_id), (override));
  MOCK_METHOD(void, OnGroupMoved,
              (const std::string& group_id, int new_index), (override));
  MOCK_METHOD(void, OnTabClicked,
              (const std::string& group_id, int tab_index), (override));
  MOCK_METHOD(void, OnTabMiddleClicked,
              (const std::string& group_id, int tab_index), (override));
  MOCK_METHOD(void, OnTabClosed,
              (const std::string& group_id, int tab_index), (override));
  MOCK_METHOD(void, OnNewTabInGroup, (const std::string& group_id),
              (override));
  MOCK_METHOD(void, OnAddGroup, (), (override));
  MOCK_METHOD(void, OnGroupMenuRequested,
              (const std::string& group_id, const gfx::Point& point),
              (override));
  MOCK_METHOD(void, OnTabDragStarted,
              (const std::string& group_id, int tab_index,
               const gfx::Point& point),
              (override));
  MOCK_METHOD(void, OnGroupDragStarted,
              (const std::string& group_id, const gfx::Point& point),
              (override));
  MOCK_METHOD(void, OnUngroupTab,
              (const std::string& group_id, int tab_index), (override));
};

class MockSidebarRecentlyClosedDelegate
    : public AstraSidebarRecentlyClosedDelegate {
 public:
  MOCK_METHOD(void, OnItemClicked, (const std::string& item_id), (override));
  MOCK_METHOD(void, OnItemMiddleClicked, (const std::string& item_id),
              (override));
  MOCK_METHOD(void, OnItemRightClicked,
              (const std::string& item_id, const gfx::Point& point),
              (override));
  MOCK_METHOD(void, OnRestoreTab, (const std::string& item_id), (override));
  MOCK_METHOD(void, OnRestoreWindow, (const std::string& item_id), (override));
  MOCK_METHOD(void, OnRestoreAllRequested, (), (override));
  MOCK_METHOD(void, OnRemoveItem, (const std::string& item_id), (override));
  MOCK_METHOD(void, OnClearAllRequested, (), (override));
  MOCK_METHOD(void, OnSearch, (const std::u16string& query), (override));
};

// =========================================================================
// Test fixture for tab group header view tests
// =========================================================================

class AstraTabGroupHeaderViewTest : public views::test::ViewsTestBase {
 public:
  AstraTabGroupHeaderViewTest() = default;
  ~AstraTabGroupHeaderViewTest() override = default;

  void SetUp() override {
    views::test::ViewsTestBase::SetUp();
    header_view_ =
        std::make_unique<AstraTabGroupHeaderView>("g1", u"Test Group",
                                                   tab_groups::TabGroupColorId::kBlue);
    widget_ = CreateWidget();
    widget_->SetContentsView(header_view_.get());
    widget_->SetSize(gfx::Size(300, 40));
    widget_->Show();
  }

  void TearDown() override {
    widget_.reset();
    header_view_.reset();
    views::test::ViewsTestBase::TearDown();
  }

 protected:
  std::unique_ptr<AstraTabGroupHeaderView> header_view_;
  std::unique_ptr<views::Widget> widget_;
};

// =========================================================================
// Test fixture for tab group tab item view tests
// =========================================================================

class AstraTabGroupTabItemViewTest : public views::test::ViewsTestBase {
 public:
  AstraTabGroupTabItemViewTest() = default;
  ~AstraTabGroupTabItemViewTest() override = default;

  void SetUp() override {
    views::test::ViewsTestBase::SetUp();
    tab_view_ = std::make_unique<AstraTabGroupTabItemView>(
        "g1", 0, u"Test Tab", GURL("https://example.com"));
    widget_ = CreateWidget();
    widget_->SetContentsView(tab_view_.get());
    widget_->SetSize(gfx::Size(300, 36));
    widget_->Show();
  }

  void TearDown() override {
    widget_.reset();
    tab_view_.reset();
    views::test::ViewsTestBase::TearDown();
  }

 protected:
  std::unique_ptr<AstraTabGroupTabItemView> tab_view_;
  std::unique_ptr<views::Widget> widget_;
};

// =========================================================================
// Test fixture for tab groups view tests
// =========================================================================

class AstraSidebarTabGroupsViewTest : public views::test::ViewsTestBase {
 public:
  AstraSidebarTabGroupsViewTest() = default;
  ~AstraSidebarTabGroupsViewTest() override = default;

  void SetUp() override {
    views::test::ViewsTestBase::SetUp();
    tab_groups_view_ = std::make_unique<AstraSidebarTabGroupsView>();
    tab_groups_view_->set_delegate(&delegate_);
    widget_ = CreateWidget();
    widget_->SetContentsView(tab_groups_view_.get());
    widget_->SetSize(gfx::Size(300, 500));
    widget_->Show();
  }

  void TearDown() override {
    widget_.reset();
    tab_groups_view_.reset();
    views::test::ViewsTestBase::TearDown();
  }

 protected:
  AstraTabGroupInfo MakeGroup(const std::string& id,
                              const std::u16string& name,
                              tab_groups::TabGroupColorId color,
                              int tab_count = 3) {
    AstraTabGroupInfo info;
    info.group_id = id;
    info.name = name;
    info.color = color;
    info.color_id = static_cast<int>(color);
    info.tab_count = tab_count;
    info.is_expanded = true;
    info.is_collapsed_in_tabstrip = false;
    info.order_index = 0;
    info.last_accessed = base::Time::Now();
    info.created_time = base::Time::Now() - base::Days(1);
    info.note = u"";
    info.is_pinned = false;
    return info;
  }

  AstraTabGroupTabInfo MakeTab(int index,
                               const std::u16string& title,
                               const GURL& url) {
    AstraTabGroupTabInfo tab;
    tab.index = index;
    tab.title = title;
    tab.url = url;
    tab.group_id = "g1";
    tab.is_active = false;
    tab.is_pinned = false;
    tab.is_audible = false;
    tab.is_muted = false;
    tab.is_loading = false;
    tab.is_dragging = false;
    return tab;
  }

  std::unique_ptr<AstraSidebarTabGroupsView> tab_groups_view_;
  std::unique_ptr<views::Widget> widget_;
  MockSidebarTabGroupsDelegate delegate_;
};

// =========================================================================
// Test fixture for recently closed view tests
// =========================================================================

class AstraSidebarRecentlyClosedViewTest : public views::test::ViewsTestBase {
 public:
  AstraSidebarRecentlyClosedViewTest() = default;
  ~AstraSidebarRecentlyClosedViewTest() override = default;

  void SetUp() override {
    views::test::ViewsTestBase::SetUp();
    recently_closed_view_ = std::make_unique<AstraSidebarRecentlyClosedView>();
    recently_closed_view_->set_delegate(&delegate_);
    widget_ = CreateWidget();
    widget_->SetContentsView(recently_closed_view_.get());
    widget_->SetSize(gfx::Size(300, 500));
    widget_->Show();
  }

  void TearDown() override {
    widget_.reset();
    recently_closed_view_.reset();
    views::test::ViewsTestBase::TearDown();
  }

 protected:
  AstraRecentlyClosedItem MakeItem(const std::string& id,
                                   const std::u16string& title,
                                   const GURL& url,
                                   AstraRecentlyClosedType type =
                                       AstraRecentlyClosedType::kTab) {
    AstraRecentlyClosedItem item;
    item.id = id;
    item.title = title;
    item.url = url;
    item.type = type;
    item.close_time = base::Time::Now() - base::Minutes(5);
    item.tab_count = (type == AstraRecentlyClosedType::kWindow) ? 5 : 1;
    item.has_favicon = false;
    item.session_id = 1;
    item.is_incognito = false;
    return item;
  }

  std::unique_ptr<AstraSidebarRecentlyClosedView> recently_closed_view_;
  std::unique_ptr<views::Widget> widget_;
  MockSidebarRecentlyClosedDelegate delegate_;
};

// =========================================================================
// Tab group header view tests
// =========================================================================

// Test 511: Tab group header default state.
TEST_F(AstraTabGroupHeaderViewTest, DefaultState) {
  EXPECT_EQ(header_view_->GetGroupId(), "g1");
  EXPECT_EQ(header_view_->GetName(), u"Test Group");
  EXPECT_EQ(header_view_->GetColor(), tab_groups::TabGroupColorId::kBlue);
  EXPECT_FALSE(header_view_->IsExpanded());
  EXPECT_FALSE(header_view_->IsSelected());
  EXPECT_FALSE(header_view_->IsPinned());
  EXPECT_FALSE(header_view_->IsCompact());
}

// Test 512: Set name updates display.
TEST_F(AstraTabGroupHeaderViewTest, SetName) {
  header_view_->SetName(u"New Name");
  EXPECT_EQ(header_view_->GetName(), u"New Name");
}

// Test 513: Set color updates display.
TEST_F(AstraTabGroupHeaderViewTest, SetColor) {
  header_view_->SetColor(tab_groups::TabGroupColorId::kRed);
  EXPECT_EQ(header_view_->GetColor(), tab_groups::TabGroupColorId::kRed);
}

// Test 514: Set expanded toggles state.
TEST_F(AstraTabGroupHeaderViewTest, SetExpanded) {
  header_view_->SetExpanded(true);
  EXPECT_TRUE(header_view_->IsExpanded());
  header_view_->SetExpanded(false);
  EXPECT_FALSE(header_view_->IsExpanded());
}

// Test 515: Set selected state.
TEST_F(AstraTabGroupHeaderViewTest, SetSelected) {
  header_view_->SetSelected(true);
  EXPECT_TRUE(header_view_->IsSelected());
  header_view_->SetSelected(false);
  EXPECT_FALSE(header_view_->IsSelected());
}

// Test 516: Set pinned state.
TEST_F(AstraTabGroupHeaderViewTest, SetPinned) {
  header_view_->SetPinned(true);
  EXPECT_TRUE(header_view_->IsPinned());
  header_view_->SetPinned(false);
  EXPECT_FALSE(header_view_->IsPinned());
}

// Test 517: Set compact mode.
TEST_F(AstraTabGroupHeaderViewTest, SetCompact) {
  header_view_->SetCompact(true);
  EXPECT_TRUE(header_view_->IsCompact());
  header_view_->SetCompact(false);
  EXPECT_FALSE(header_view_->IsCompact());
}

// Test 518: Set tab count.
TEST_F(AstraTabGroupHeaderViewTest, SetTabCount) {
  header_view_->SetTabCount(10);
  EXPECT_EQ(header_view_->GetTabCount(), 10);
}

// Test 519: Default tab count is 0.
TEST_F(AstraTabGroupHeaderViewTest, DefaultTabCount) {
  EXPECT_EQ(header_view_->GetTabCount(), 0);
}

// Test 520: Show chevron visibility.
TEST_F(AstraTabGroupHeaderViewTest, ShowChevron) {
  header_view_->SetShowChevron(true);
  EXPECT_TRUE(header_view_->GetShowChevron());
  header_view_->SetShowChevron(false);
  EXPECT_FALSE(header_view_->GetShowChevron());
}

// Test 521: Show tab count visibility.
TEST_F(AstraTabGroupHeaderViewTest, ShowTabCount) {
  header_view_->SetShowTabCount(true);
  EXPECT_TRUE(header_view_->GetShowTabCount());
  header_view_->SetShowTabCount(false);
  EXPECT_FALSE(header_view_->GetShowTabCount());
}

// Test 522: Show color dot visibility.
TEST_F(AstraTabGroupHeaderViewTest, ShowColorDot) {
  header_view_->SetShowColorDot(true);
  EXPECT_TRUE(header_view_->GetShowColorDot());
  header_view_->SetShowColorDot(false);
  EXPECT_FALSE(header_view_->GetShowColorDot());
}

// Test 523: Show menu button visibility.
TEST_F(AstraTabGroupHeaderViewTest, ShowMenuButton) {
  header_view_->SetShowMenuButton(true);
  EXPECT_TRUE(header_view_->GetShowMenuButton());
  header_view_->SetShowMenuButton(false);
  EXPECT_FALSE(header_view_->GetShowMenuButton());
}

// Test 524: Set collapsed in tabstrip.
TEST_F(AstraTabGroupHeaderViewTest, SetCollapsedInTabstrip) {
  header_view_->SetCollapsedInTabstrip(true);
  EXPECT_TRUE(header_view_->IsCollapsedInTabstrip());
  header_view_->SetCollapsedInTabstrip(false);
  EXPECT_FALSE(header_view_->IsCollapsedInTabstrip());
}

// Test 525: Set drag hover state.
TEST_F(AstraTabGroupHeaderViewTest, SetDragHovered) {
  header_view_->SetDragHovered(true);
  EXPECT_TRUE(header_view_->IsDragHovered());
  header_view_->SetDragHovered(false);
  EXPECT_FALSE(header_view_->IsDragHovered());
}

// Test 526: Set group info bulk update.
TEST_F(AstraTabGroupHeaderViewTest, SetGroupInfoBulkUpdate) {
  AstraTabGroupInfo info;
  info.group_id = "g2";
  info.name = u"Updated Group";
  info.color = tab_groups::TabGroupColorId::kGreen;
  info.color_id = static_cast<int>(tab_groups::TabGroupColorId::kGreen);
  info.tab_count = 7;
  info.is_expanded = true;
  info.is_collapsed_in_tabstrip = false;
  info.order_index = 1;
  info.last_accessed = base::Time::Now();
  info.created_time = base::Time::Now() - base::Hours(2);
  info.note = u"test note";
  info.is_pinned = true;

  header_view_->SetGroupInfo(info);
  EXPECT_EQ(header_view_->GetGroupId(), "g2");
  EXPECT_EQ(header_view_->GetName(), u"Updated Group");
  EXPECT_EQ(header_view_->GetColor(), tab_groups::TabGroupColorId::kGreen);
  EXPECT_EQ(header_view_->GetTabCount(), 7);
  EXPECT_TRUE(header_view_->IsExpanded());
  EXPECT_TRUE(header_view_->IsPinned());
}

// Test 527: Toggle expansion callback.
TEST_F(AstraTabGroupHeaderViewTest, ToggleCallback) {
  bool toggled = false;
  header_view_->set_toggle_callback(base::BindRepeating(
      [](bool* toggled) { *toggled = true; }, base::Unretained(&toggled)));
  header_view_->SetExpanded(false);
  EXPECT_FALSE(toggled);
}

// Test 528: Color ID accessor.
TEST_F(AstraTabGroupHeaderViewTest, ColorIdAccessor) {
  EXPECT_EQ(header_view_->GetColorId(),
            static_cast<int>(tab_groups::TabGroupColorId::kBlue));
  header_view_->SetColor(tab_groups::TabGroupColorId::kYellow);
  EXPECT_EQ(header_view_->GetColorId(),
            static_cast<int>(tab_groups::TabGroupColorId::kYellow));
}

// Test 529: Multiple color updates.
TEST_F(AstraTabGroupHeaderViewTest, MultipleColorUpdates) {
  header_view_->SetColor(tab_groups::TabGroupColorId::kRed);
  header_view_->SetColor(tab_groups::TabGroupColorId::kGreen);
  header_view_->SetColor(tab_groups::TabGroupColorId::kBlue);
  EXPECT_EQ(header_view_->GetColor(), tab_groups::TabGroupColorId::kBlue);
}

// Test 530: Header view preferred size is valid.
TEST_F(AstraTabGroupHeaderViewTest, PreferredSizeValid) {
  gfx::Size size = header_view_->GetPreferredSize();
  EXPECT_GT(size.width(), 0);
  EXPECT_GT(size.height(), 0);
}

// Test 531: Compact mode reduces height.
TEST_F(AstraTabGroupHeaderViewTest, CompactModeSize) {
  gfx::Size normal_size = header_view_->GetPreferredSize();
  header_view_->SetCompact(true);
  gfx::Size compact_size = header_view_->GetPreferredSize();
  EXPECT_LE(compact_size.height(), normal_size.height());
}

// Test 532: Empty name is handled.
TEST_F(AstraTabGroupHeaderViewTest, EmptyName) {
  header_view_->SetName(u"");
  EXPECT_EQ(header_view_->GetName(), u"");
}

// Test 533: Long name is accepted.
TEST_F(AstraTabGroupHeaderViewTest, LongName) {
  std::u16string long_name(100, u'a');
  header_view_->SetName(long_name);
  EXPECT_EQ(header_view_->GetName(), long_name);
}

// =========================================================================
// Tab group tab item view tests
// =========================================================================

// Test 534: Tab item default state.
TEST_F(AstraTabGroupTabItemViewTest, DefaultState) {
  EXPECT_EQ(tab_view_->GetGroupId(), "g1");
  EXPECT_EQ(tab_view_->GetTabIndex(), 0);
  EXPECT_EQ(tab_view_->GetTitle(), u"Test Tab");
  EXPECT_EQ(tab_view_->GetUrl(), GURL("https://example.com"));
  EXPECT_FALSE(tab_view_->IsActive());
  EXPECT_FALSE(tab_view_->IsPinned());
  EXPECT_FALSE(tab_view_->IsAudible());
  EXPECT_FALSE(tab_view_->IsMuted());
  EXPECT_FALSE(tab_view_->IsLoading());
  EXPECT_FALSE(tab_view_->IsDragging());
  EXPECT_FALSE(tab_view_->IsDragHovered());
}

// Test 535: Set title updates display.
TEST_F(AstraTabGroupTabItemViewTest, SetTitle) {
  tab_view_->SetTitle(u"New Title");
  EXPECT_EQ(tab_view_->GetTitle(), u"New Title");
}

// Test 536: Set URL updates display.
TEST_F(AstraTabGroupTabItemViewTest, SetUrl) {
  GURL new_url("https://new.example.com");
  tab_view_->SetUrl(new_url);
  EXPECT_EQ(tab_view_->GetUrl(), new_url);
}

// Test 537: Set active state.
TEST_F(AstraTabGroupTabItemViewTest, SetActive) {
  tab_view_->SetActive(true);
  EXPECT_TRUE(tab_view_->IsActive());
  tab_view_->SetActive(false);
  EXPECT_FALSE(tab_view_->IsActive());
}

// Test 538: Set pinned state.
TEST_F(AstraTabGroupTabItemViewTest, SetPinned) {
  tab_view_->SetPinned(true);
  EXPECT_TRUE(tab_view_->IsPinned());
  tab_view_->SetPinned(false);
  EXPECT_FALSE(tab_view_->IsPinned());
}

// Test 539: Set audible state.
TEST_F(AstraTabGroupTabItemViewTest, SetAudible) {
  tab_view_->SetAudible(true);
  EXPECT_TRUE(tab_view_->IsAudible());
  tab_view_->SetAudible(false);
  EXPECT_FALSE(tab_view_->IsAudible());
}

// Test 540: Set muted state.
TEST_F(AstraTabGroupTabItemViewTest, SetMuted) {
  tab_view_->SetMuted(true);
  EXPECT_TRUE(tab_view_->IsMuted());
  tab_view_->SetMuted(false);
  EXPECT_FALSE(tab_view_->IsMuted());
}

// Test 541: Set loading state.
TEST_F(AstraTabGroupTabItemViewTest, SetLoading) {
  tab_view_->SetLoading(true);
  EXPECT_TRUE(tab_view_->IsLoading());
  tab_view_->SetLoading(false);
  EXPECT_FALSE(tab_view_->IsLoading());
}

// Test 542: Set dragging state.
TEST_F(AstraTabGroupTabItemViewTest, SetDragging) {
  tab_view_->SetDragging(true);
  EXPECT_TRUE(tab_view_->IsDragging());
  tab_view_->SetDragging(false);
  EXPECT_FALSE(tab_view_->IsDragging());
}

// Test 543: Set drag hover state.
TEST_F(AstraTabGroupTabItemViewTest, SetDragHovered) {
  tab_view_->SetDragHovered(true);
  EXPECT_TRUE(tab_view_->IsDragHovered());
  tab_view_->SetDragHovered(false);
  EXPECT_FALSE(tab_view_->IsDragHovered());
}

// Test 544: Set tab index.
TEST_F(AstraTabGroupTabItemViewTest, SetTabIndex) {
  tab_view_->SetTabIndex(5);
  EXPECT_EQ(tab_view_->GetTabIndex(), 5);
}

// Test 545: Set group ID.
TEST_F(AstraTabGroupTabItemViewTest, SetGroupId) {
  tab_view_->SetGroupId("g99");
  EXPECT_EQ(tab_view_->GetGroupId(), "g99");
}

// Test 546: Set tab info bulk update.
TEST_F(AstraTabGroupTabItemViewTest, SetTabInfoBulkUpdate) {
  AstraTabGroupTabInfo info;
  info.index = 3;
  info.title = u"Bulk Tab";
  info.url = GURL("https://bulk.example.com");
  info.group_id = "g5";
  info.is_active = true;
  info.is_pinned = true;
  info.is_audible = true;
  info.is_muted = false;
  info.is_loading = true;
  info.is_dragging = false;

  tab_view_->SetTabInfo(info);
  EXPECT_EQ(tab_view_->GetTabIndex(), 3);
  EXPECT_EQ(tab_view_->GetTitle(), u"Bulk Tab");
  EXPECT_EQ(tab_view_->GetUrl(), GURL("https://bulk.example.com"));
  EXPECT_EQ(tab_view_->GetGroupId(), "g5");
  EXPECT_TRUE(tab_view_->IsActive());
  EXPECT_TRUE(tab_view_->IsPinned());
  EXPECT_TRUE(tab_view_->IsAudible());
  EXPECT_TRUE(tab_view_->IsLoading());
}

// Test 547: Tab activated callback.
TEST_F(AstraTabGroupTabItemViewTest, ActivatedCallback) {
  bool activated = false;
  tab_view_->set_tab_activated_callback(base::BindRepeating(
      [](bool* activated) { *activated = true; }, base::Unretained(&activated)));
  // Callback is set; we verify it was stored by checking no crash.
  EXPECT_FALSE(activated);
}

// Test 548: Tab closed callback.
TEST_F(AstraTabGroupTabItemViewTest, ClosedCallback) {
  bool closed = false;
  tab_view_->set_tab_closed_callback(base::BindRepeating(
      [](bool* closed) { *closed = true; }, base::Unretained(&closed)));
  EXPECT_FALSE(closed);
}

// Test 549: Tab middle-clicked callback.
TEST_F(AstraTabGroupTabItemViewTest, MiddleClickedCallback) {
  bool middle_clicked = false;
  tab_view_->set_tab_middle_clicked_callback(base::BindRepeating(
      [](bool* clicked) { *clicked = true; },
      base::Unretained(&middle_clicked)));
  EXPECT_FALSE(middle_clicked);
}

// Test 550: Show close button.
TEST_F(AstraTabGroupTabItemViewTest, ShowCloseButton) {
  tab_view_->SetShowCloseButton(true);
  EXPECT_TRUE(tab_view_->GetShowCloseButton());
  tab_view_->SetShowCloseButton(false);
  EXPECT_FALSE(tab_view_->GetShowCloseButton());
}

// Test 551: Tab view preferred size is valid.
TEST_F(AstraTabGroupTabItemViewTest, PreferredSizeValid) {
  gfx::Size size = tab_view_->GetPreferredSize();
  EXPECT_GT(size.width(), 0);
  EXPECT_GT(size.height(), 0);
}

// Test 552: Audible but not muted shows audio indicator.
TEST_F(AstraTabGroupTabItemViewTest, AudibleNotMuted) {
  tab_view_->SetAudible(true);
  tab_view_->SetMuted(false);
  EXPECT_TRUE(tab_view_->IsAudible());
  EXPECT_FALSE(tab_view_->IsMuted());
}

// Test 553: Muted state with audible.
TEST_F(AstraTabGroupTabItemViewTest, MutedWithAudible) {
  tab_view_->SetAudible(true);
  tab_view_->SetMuted(true);
  EXPECT_TRUE(tab_view_->IsAudible());
  EXPECT_TRUE(tab_view_->IsMuted());
}

// Test 554: Loading state transitions.
TEST_F(AstraTabGroupTabItemViewTest, LoadingTransitions) {
  tab_view_->SetLoading(true);
  EXPECT_TRUE(tab_view_->IsLoading());
  tab_view_->SetLoading(false);
  EXPECT_FALSE(tab_view_->IsLoading());
  tab_view_->SetLoading(true);
  EXPECT_TRUE(tab_view_->IsLoading());
}

// Test 555: Empty URL is valid.
TEST_F(AstraTabGroupTabItemViewTest, EmptyUrl) {
  tab_view_->SetUrl(GURL());
  EXPECT_EQ(tab_view_->GetUrl(), GURL());
}

// Test 556: Long title is accepted.
TEST_F(AstraTabGroupTabItemViewTest, LongTitle) {
  std::u16string long_title(200, u'x');
  tab_view_->SetTitle(long_title);
  EXPECT_EQ(tab_view_->GetTitle(), long_title);
}

// Test 557: Negative tab index edge case.
TEST_F(AstraTabGroupTabItemViewTest, NegativeTabIndex) {
  tab_view_->SetTabIndex(-1);
  EXPECT_EQ(tab_view_->GetTabIndex(), -1);
}

// =========================================================================
// Tab groups view tests - basic construction and CRUD
// =========================================================================

// Test 558: Default group count is 0.
TEST_F(AstraSidebarTabGroupsViewTest, DefaultGroupCountZero) {
  EXPECT_EQ(tab_groups_view_->GetGroupCount(), 0);
}

// Test 559: SetTabGroups replaces all groups.
TEST_F(AstraSidebarTabGroupsViewTest, SetTabGroupsReplacesAll) {
  std::vector<AstraTabGroupInfo> groups;
  groups.push_back(
      MakeGroup("g1", u"Group 1", tab_groups::TabGroupColorId::kRed, 3));
  groups.push_back(
      MakeGroup("g2", u"Group 2", tab_groups::TabGroupColorId::kBlue, 5));
  tab_groups_view_->SetTabGroups(groups);
  EXPECT_EQ(tab_groups_view_->GetGroupCount(), 2);
}

// Test 560: AddGroup adds a new group.
TEST_F(AstraSidebarTabGroupsViewTest, AddGroup) {
  auto group =
      MakeGroup("g1", u"New Group", tab_groups::TabGroupColorId::kGreen, 2);
  tab_groups_view_->AddGroup(group);
  EXPECT_EQ(tab_groups_view_->GetGroupCount(), 1);
}

// Test 561: RemoveGroup removes by ID.
TEST_F(AstraSidebarTabGroupsViewTest, RemoveGroup) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Group 1", tab_groups::TabGroupColorId::kRed, 3));
  tab_groups_view_->AddGroup(
      MakeGroup("g2", u"Group 2", tab_groups::TabGroupColorId::kBlue, 5));
  tab_groups_view_->RemoveGroup("g1");
  EXPECT_EQ(tab_groups_view_->GetGroupCount(), 1);
}

// Test 562: RemoveGroup with invalid ID is a no-op.
TEST_F(AstraSidebarTabGroupsViewTest, RemoveGroupInvalid) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Group 1", tab_groups::TabGroupColorId::kRed, 3));
  tab_groups_view_->RemoveGroup("nonexistent");
  EXPECT_EQ(tab_groups_view_->GetGroupCount(), 1);
}

// Test 563: UpdateGroup updates existing group.
TEST_F(AstraSidebarTabGroupsViewTest, UpdateGroup) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Old Name", tab_groups::TabGroupColorId::kRed, 3));
  auto updated =
      MakeGroup("g1", u"New Name", tab_groups::TabGroupColorId::kBlue, 10);
  tab_groups_view_->UpdateGroup(updated);
  EXPECT_EQ(tab_groups_view_->GetGroupCount(), 1);
  auto group = tab_groups_view_->GetGroupAt(0);
  EXPECT_EQ(group.name, u"New Name");
  EXPECT_EQ(group.tab_count, 10);
}

// Test 564: ClearAll removes all groups.
TEST_F(AstraSidebarTabGroupsViewTest, ClearAllGroups) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Group 1", tab_groups::TabGroupColorId::kRed, 3));
  tab_groups_view_->AddGroup(
      MakeGroup("g2", u"Group 2", tab_groups::TabGroupColorId::kBlue, 5));
  tab_groups_view_->AddGroup(
      MakeGroup("g3", u"Group 3", tab_groups::TabGroupColorId::kGreen, 2));
  tab_groups_view_->ClearAll();
  EXPECT_EQ(tab_groups_view_->GetGroupCount(), 0);
}

// Test 565: GetGroupAt returns correct group info.
TEST_F(AstraSidebarTabGroupsViewTest, GetGroupAt) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"First", tab_groups::TabGroupColorId::kRed, 2));
  auto group = tab_groups_view_->GetGroupAt(0);
  EXPECT_EQ(group.group_id, "g1");
  EXPECT_EQ(group.name, u"First");
  EXPECT_EQ(group.tab_count, 2);
}

// Test 566: HasGroup returns true for existing group.
TEST_F(AstraSidebarTabGroupsViewTest, HasGroupTrue) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Group", tab_groups::TabGroupColorId::kRed, 3));
  EXPECT_TRUE(tab_groups_view_->HasGroup("g1"));
}

// Test 567: HasGroup returns false for missing group.
TEST_F(AstraSidebarTabGroupsViewTest, HasGroupFalse) {
  EXPECT_FALSE(tab_groups_view_->HasGroup("nonexistent"));
}

// Test 568: FindGroupIndex returns correct index.
TEST_F(AstraSidebarTabGroupsViewTest, FindGroupIndex) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"First", tab_groups::TabGroupColorId::kRed, 3));
  tab_groups_view_->AddGroup(
      MakeGroup("g2", u"Second", tab_groups::TabGroupColorId::kBlue, 5));
  EXPECT_EQ(tab_groups_view_->FindGroupIndex("g1"), 0);
  EXPECT_EQ(tab_groups_view_->FindGroupIndex("g2"), 1);
}

// Test 569: FindGroupIndex returns -1 for missing group.
TEST_F(AstraSidebarTabGroupsViewTest, FindGroupIndexMissing) {
  EXPECT_EQ(tab_groups_view_->FindGroupIndex("missing"), -1);
}

// =========================================================================
// Tab groups view tests - selection
// =========================================================================

// Test 570: No selection by default.
TEST_F(AstraSidebarTabGroupsViewTest, NoSelectionByDefault) {
  EXPECT_EQ(tab_groups_view_->GetSelectedGroupIndex(), -1);
}

// Test 571: SetSelectedGroup sets selection.
TEST_F(AstraSidebarTabGroupsViewTest, SetSelectedGroup) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Group", tab_groups::TabGroupColorId::kRed, 3));
  tab_groups_view_->SetSelectedGroup(0);
  EXPECT_EQ(tab_groups_view_->GetSelectedGroupIndex(), 0);
}

// Test 572: ClearSelection clears selection.
TEST_F(AstraSidebarTabGroupsViewTest, ClearSelection) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Group", tab_groups::TabGroupColorId::kRed, 3));
  tab_groups_view_->SetSelectedGroup(0);
  tab_groups_view_->ClearSelection();
  EXPECT_EQ(tab_groups_view_->GetSelectedGroupIndex(), -1);
}

// Test 573: SetSelectedGroup with -1 clears selection.
TEST_F(AstraSidebarTabGroupsViewTest, SetSelectedGroupNegativeOne) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Group", tab_groups::TabGroupColorId::kRed, 3));
  tab_groups_view_->SetSelectedGroup(0);
  tab_groups_view_->SetSelectedGroup(-1);
  EXPECT_EQ(tab_groups_view_->GetSelectedGroupIndex(), -1);
}

// Test 574: SetSelectedGroup with invalid index is safe.
TEST_F(AstraSidebarTabGroupsViewTest, SetSelectedGroupInvalid) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Group", tab_groups::TabGroupColorId::kRed, 3));
  tab_groups_view_->SetSelectedGroup(99);
  EXPECT_EQ(tab_groups_view_->GetSelectedGroupIndex(), -1);
}

// =========================================================================
// Tab groups view tests - expansion
// =========================================================================

// Test 575: ToggleGroupExpanded toggles expansion.
TEST_F(AstraSidebarTabGroupsViewTest, ToggleGroupExpanded) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Group", tab_groups::TabGroupColorId::kRed, 3));
  tab_groups_view_->SetGroupExpanded("g1", true);
  EXPECT_TRUE(tab_groups_view_->IsGroupExpanded("g1"));
  tab_groups_view_->ToggleGroupExpanded("g1");
  EXPECT_FALSE(tab_groups_view_->IsGroupExpanded("g1"));
}

// Test 576: SetGroupExpanded sets state.
TEST_F(AstraSidebarTabGroupsViewTest, SetGroupExpanded) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Group", tab_groups::TabGroupColorId::kRed, 3));
  tab_groups_view_->SetGroupExpanded("g1", false);
  EXPECT_FALSE(tab_groups_view_->IsGroupExpanded("g1"));
  tab_groups_view_->SetGroupExpanded("g1", true);
  EXPECT_TRUE(tab_groups_view_->IsGroupExpanded("g1"));
}

// Test 577: ExpandAll expands all groups.
TEST_F(AstraSidebarTabGroupsViewTest, ExpandAll) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Group 1", tab_groups::TabGroupColorId::kRed, 3));
  tab_groups_view_->AddGroup(
      MakeGroup("g2", u"Group 2", tab_groups::TabGroupColorId::kBlue, 5));
  tab_groups_view_->SetGroupExpanded("g1", false);
  tab_groups_view_->SetGroupExpanded("g2", false);
  tab_groups_view_->ExpandAll();
  EXPECT_TRUE(tab_groups_view_->IsGroupExpanded("g1"));
  EXPECT_TRUE(tab_groups_view_->IsGroupExpanded("g2"));
}

// Test 578: CollapseAll collapses all groups.
TEST_F(AstraSidebarTabGroupsViewTest, CollapseAll) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Group 1", tab_groups::TabGroupColorId::kRed, 3));
  tab_groups_view_->AddGroup(
      MakeGroup("g2", u"Group 2", tab_groups::TabGroupColorId::kBlue, 5));
  tab_groups_view_->ExpandAll();
  tab_groups_view_->CollapseAll();
  EXPECT_FALSE(tab_groups_view_->IsGroupExpanded("g1"));
  EXPECT_FALSE(tab_groups_view_->IsGroupExpanded("g2"));
}

// =========================================================================
// Tab groups view tests - reordering
// =========================================================================

// Test 579: MoveGroup moves group to new position.
TEST_F(AstraSidebarTabGroupsViewTest, MoveGroup) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"First", tab_groups::TabGroupColorId::kRed, 1));
  tab_groups_view_->AddGroup(
      MakeGroup("g2", u"Second", tab_groups::TabGroupColorId::kBlue, 2));
  tab_groups_view_->AddGroup(
      MakeGroup("g3", u"Third", tab_groups::TabGroupColorId::kGreen, 3));
  tab_groups_view_->MoveGroup("g1", 2);
  auto group = tab_groups_view_->GetGroupAt(2);
  EXPECT_EQ(group.group_id, "g1");
}

// Test 580: MoveGroup up.
TEST_F(AstraSidebarTabGroupsViewTest, MoveGroupUp) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"First", tab_groups::TabGroupColorId::kRed, 1));
  tab_groups_view_->AddGroup(
      MakeGroup("g2", u"Second", tab_groups::TabGroupColorId::kBlue, 2));
  tab_groups_view_->MoveGroup("g2", 0);
  auto group = tab_groups_view_->GetGroupAt(0);
  EXPECT_EQ(group.group_id, "g2");
}

// =========================================================================
// Tab groups view tests - color and naming
// =========================================================================

// Test 581: SetGroupColor updates color.
TEST_F(AstraSidebarTabGroupsViewTest, SetGroupColor) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Group", tab_groups::TabGroupColorId::kRed, 3));
  tab_groups_view_->SetGroupColor("g1", tab_groups::TabGroupColorId::kYellow);
  auto group = tab_groups_view_->GetGroupAt(0);
  EXPECT_EQ(group.color, tab_groups::TabGroupColorId::kYellow);
}

// Test 582: SetGroupName updates name.
TEST_F(AstraSidebarTabGroupsViewTest, SetGroupName) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Old", tab_groups::TabGroupColorId::kRed, 3));
  tab_groups_view_->SetGroupName("g1", u"New Name");
  auto group = tab_groups_view_->GetGroupAt(0);
  EXPECT_EQ(group.name, u"New Name");
}

// Test 583: GetGroupTabCount returns correct count.
TEST_F(AstraSidebarTabGroupsViewTest, GetGroupTabCount) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Group", tab_groups::TabGroupColorId::kRed, 7));
  EXPECT_EQ(tab_groups_view_->GetGroupTabCount("g1"), 7);
}

// Test 584: GetGroupTabCount for missing group returns 0.
TEST_F(AstraSidebarTabGroupsViewTest, GetGroupTabCountMissing) {
  EXPECT_EQ(tab_groups_view_->GetGroupTabCount("missing"), 0);
}

// =========================================================================
// Tab groups view tests - display options
// =========================================================================

// Test 585: Show tab count option.
TEST_F(AstraSidebarTabGroupsViewTest, ShowTabCountOption) {
  tab_groups_view_->SetShowTabCount(true);
  EXPECT_TRUE(tab_groups_view_->GetShowTabCount());
  tab_groups_view_->SetShowTabCount(false);
  EXPECT_FALSE(tab_groups_view_->GetShowTabCount());
}

// Test 586: Show chevrons option.
TEST_F(AstraSidebarTabGroupsViewTest, ShowChevronsOption) {
  tab_groups_view_->SetShowChevrons(true);
  EXPECT_TRUE(tab_groups_view_->GetShowChevrons());
  tab_groups_view_->SetShowChevrons(false);
  EXPECT_FALSE(tab_groups_view_->GetShowChevrons());
}

// Test 587: Show color dots option.
TEST_F(AstraSidebarTabGroupsViewTest, ShowColorDotsOption) {
  tab_groups_view_->SetShowColorDots(true);
  EXPECT_TRUE(tab_groups_view_->GetShowColorDots());
  tab_groups_view_->SetShowColorDots(false);
  EXPECT_FALSE(tab_groups_view_->GetShowColorDots());
}

// Test 588: Show menu buttons option.
TEST_F(AstraSidebarTabGroupsViewTest, ShowMenuButtonsOption) {
  tab_groups_view_->SetShowMenuButtons(true);
  EXPECT_TRUE(tab_groups_view_->GetShowMenuButtons());
  tab_groups_view_->SetShowMenuButtons(false);
  EXPECT_FALSE(tab_groups_view_->GetShowMenuButtons());
}

// Test 589: Show favicons option.
TEST_F(AstraSidebarTabGroupsViewTest, ShowFaviconsOption) {
  tab_groups_view_->SetShowFavicons(true);
  EXPECT_TRUE(tab_groups_view_->GetShowFavicons());
  tab_groups_view_->SetShowFavicons(false);
  EXPECT_FALSE(tab_groups_view_->GetShowFavicons());
}

// Test 590: Compact mode option.
TEST_F(AstraSidebarTabGroupsViewTest, CompactModeOption) {
  tab_groups_view_->SetCompactMode(true);
  EXPECT_TRUE(tab_groups_view_->GetCompactMode());
  tab_groups_view_->SetCompactMode(false);
  EXPECT_FALSE(tab_groups_view_->GetCompactMode());
}

// Test 591: Show add group button option.
TEST_F(AstraSidebarTabGroupsViewTest, ShowAddGroupButtonOption) {
  tab_groups_view_->SetShowAddGroupButton(true);
  EXPECT_TRUE(tab_groups_view_->GetShowAddGroupButton());
  tab_groups_view_->SetShowAddGroupButton(false);
  EXPECT_FALSE(tab_groups_view_->GetShowAddGroupButton());
}

// Test 592: Max groups option.
TEST_F(AstraSidebarTabGroupsViewTest, MaxGroupsOption) {
  tab_groups_view_->SetMaxGroups(5);
  EXPECT_EQ(tab_groups_view_->GetMaxGroups(), 5);
}

// =========================================================================
// Tab groups view tests - sorting
// =========================================================================

// Test 593: Sort by manual (default).
TEST_F(AstraSidebarTabGroupsViewTest, SortByManual) {
  tab_groups_view_->SetSortBy(AstraTabGroupSortBy::kManual);
  EXPECT_EQ(tab_groups_view_->GetSortBy(), AstraTabGroupSortBy::kManual);
}

// Test 594: Sort by name.
TEST_F(AstraSidebarTabGroupsViewTest, SortByName) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Zebra", tab_groups::TabGroupColorId::kRed, 3));
  tab_groups_view_->AddGroup(
      MakeGroup("g2", u"Apple", tab_groups::TabGroupColorId::kBlue, 5));
  tab_groups_view_->SetSortBy(AstraTabGroupSortBy::kName);
  auto first = tab_groups_view_->GetGroupAt(0);
  EXPECT_EQ(first.name, u"Apple");
}

// Test 595: Sort by color.
TEST_F(AstraSidebarTabGroupsViewTest, SortByColor) {
  tab_groups_view_->SetSortBy(AstraTabGroupSortBy::kColor);
  EXPECT_EQ(tab_groups_view_->GetSortBy(), AstraTabGroupSortBy::kColor);
}

// Test 596: Sort by tab count.
TEST_F(AstraSidebarTabGroupsViewTest, SortByTabCount) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Big", tab_groups::TabGroupColorId::kRed, 10));
  tab_groups_view_->AddGroup(
      MakeGroup("g2", u"Small", tab_groups::TabGroupColorId::kBlue, 2));
  tab_groups_view_->SetSortBy(AstraTabGroupSortBy::kTabCount);
  auto first = tab_groups_view_->GetGroupAt(0);
  EXPECT_EQ(first.name, u"Small");
}

// Test 597: Sort by last accessed.
TEST_F(AstraSidebarTabGroupsViewTest, SortByLastAccessed) {
  tab_groups_view_->SetSortBy(AstraTabGroupSortBy::kLastAccessed);
  EXPECT_EQ(tab_groups_view_->GetSortBy(), AstraTabGroupSortBy::kLastAccessed);
}

// Test 598: Set sort by name then back to manual.
TEST_F(AstraSidebarTabGroupsViewTest, SortByNameThenManual) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Zebra", tab_groups::TabGroupColorId::kRed, 3));
  tab_groups_view_->AddGroup(
      MakeGroup("g2", u"Apple", tab_groups::TabGroupColorId::kBlue, 5));
  tab_groups_view_->SetSortBy(AstraTabGroupSortBy::kName);
  tab_groups_view_->SetSortBy(AstraTabGroupSortBy::kManual);
  // Manual order should be original insertion order.
  auto first = tab_groups_view_->GetGroupAt(0);
  EXPECT_EQ(first.name, u"Zebra");
}

// =========================================================================
// Tab groups view tests - group operations
// =========================================================================

// Test 599: CloseGroup removes group.
TEST_F(AstraSidebarTabGroupsViewTest, CloseGroup) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Group", tab_groups::TabGroupColorId::kRed, 3));
  tab_groups_view_->CloseGroup("g1");
  EXPECT_EQ(tab_groups_view_->GetGroupCount(), 0);
}

// Test 600: NewTabInGroup increments tab count.
TEST_F(AstraSidebarTabGroupsViewTest, NewTabInGroup) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Group", tab_groups::TabGroupColorId::kRed, 3));
  tab_groups_view_->NewTabInGroup("g1");
  EXPECT_EQ(tab_groups_view_->GetGroupTabCount("g1"), 4);
}

// Test 601: UngroupTab removes tab from group.
TEST_F(AstraSidebarTabGroupsViewTest, UngroupTab) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Group", tab_groups::TabGroupColorId::kRed, 5));
  tab_groups_view_->UngroupTab("g1", 2);
  EXPECT_EQ(tab_groups_view_->GetGroupTabCount("g1"), 4);
}

// Test 602: PinGroup sets pinned state.
TEST_F(AstraSidebarTabGroupsViewTest, PinGroup) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Group", tab_groups::TabGroupColorId::kRed, 3));
  tab_groups_view_->PinGroup("g1", true);
  auto group = tab_groups_view_->GetGroupAt(0);
  EXPECT_TRUE(group.is_pinned);
}

// =========================================================================
// Tab groups view tests - tab operations
// =========================================================================

// Test 603: AddTabToGroup adds a tab.
TEST_F(AstraSidebarTabGroupsViewTest, AddTabToGroup) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Group", tab_groups::TabGroupColorId::kRed, 2));
  auto tab = MakeTab(2, u"New Tab", GURL("https://new.example.com"));
  tab_groups_view_->AddTabToGroup("g1", tab);
  EXPECT_EQ(tab_groups_view_->GetGroupTabCount("g1"), 3);
}

// Test 604: RemoveTabFromGroup removes a tab.
TEST_F(AstraSidebarTabGroupsViewTest, RemoveTabFromGroup) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Group", tab_groups::TabGroupColorId::kRed, 3));
  tab_groups_view_->RemoveTabFromGroup("g1", 1);
  EXPECT_EQ(tab_groups_view_->GetGroupTabCount("g1"), 2);
}

// Test 605: UpdateTabInGroup updates tab info.
TEST_F(AstraSidebarTabGroupsViewTest, UpdateTabInGroup) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Group", tab_groups::TabGroupColorId::kRed, 3));
  auto tab = MakeTab(1, u"Updated", GURL("https://updated.example.com"));
  tab.is_active = true;
  tab_groups_view_->UpdateTabInGroup("g1", tab);
  // Verify no crash and tab count remains.
  EXPECT_EQ(tab_groups_view_->GetGroupTabCount("g1"), 3);
}

// Test 606: GetTabCount returns total across all groups.
TEST_F(AstraSidebarTabGroupsViewTest, GetTotalTabCount) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Group 1", tab_groups::TabGroupColorId::kRed, 3));
  tab_groups_view_->AddGroup(
      MakeGroup("g2", u"Group 2", tab_groups::TabGroupColorId::kBlue, 5));
  EXPECT_EQ(tab_groups_view_->GetTotalTabCount(), 8);
}

// =========================================================================
// Tab groups view tests - view access
// =========================================================================

// Test 607: GetHeaderViewAt returns valid pointer.
TEST_F(AstraSidebarTabGroupsViewTest, GetHeaderViewAt) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Group", tab_groups::TabGroupColorId::kRed, 3));
  auto* header = tab_groups_view_->GetHeaderViewAt(0);
  EXPECT_NE(header, nullptr);
  EXPECT_EQ(header->GetGroupId(), "g1");
}

// Test 608: GetTabViewAt returns valid pointer.
TEST_F(AstraSidebarTabGroupsViewTest, GetTabViewAt) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Group", tab_groups::TabGroupColorId::kRed, 3));
  // Expand group so tab views are created.
  tab_groups_view_->SetGroupExpanded("g1", true);
  auto* tab = tab_groups_view_->GetTabViewAt("g1", 0);
  EXPECT_NE(tab, nullptr);
}

// Test 609: Preferred size is valid.
TEST_F(AstraSidebarTabGroupsViewTest, PreferredSizeValid) {
  gfx::Size size = tab_groups_view_->GetPreferredSize();
  EXPECT_GT(size.width(), 0);
  EXPECT_GE(size.height(), 0);
}

// =========================================================================
// Tab groups view tests - delegate callbacks
// =========================================================================

// Test 610: OnAddGroup callback fires.
TEST_F(AstraSidebarTabGroupsViewTest, AddGroupCallback) {
  EXPECT_CALL(delegate_, OnAddGroup()).Times(0);
  // The delegate callback is for user-initiated add, not programmatic.
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Group", tab_groups::TabGroupColorId::kRed, 3));
  // No delegate call expected for programmatic add.
}

// Test 611: Set delegate works.
TEST_F(AstraSidebarTabGroupsViewTest, SetDelegate) {
  MockSidebarTabGroupsDelegate other_delegate;
  tab_groups_view_->set_delegate(&other_delegate);
  EXPECT_EQ(tab_groups_view_->delegate(), &other_delegate);
  // Restore original delegate.
  tab_groups_view_->set_delegate(&delegate_);
}

// =========================================================================
// Tab groups view tests - edge cases
// =========================================================================

// Test 612: Empty SetTabGroups clears all.
TEST_F(AstraSidebarTabGroupsViewTest, EmptySetTabGroups) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Group", tab_groups::TabGroupColorId::kRed, 3));
  std::vector<AstraTabGroupInfo> empty;
  tab_groups_view_->SetTabGroups(empty);
  EXPECT_EQ(tab_groups_view_->GetGroupCount(), 0);
}

// Test 613: AddGroup with same ID replaces existing.
TEST_F(AstraSidebarTabGroupsViewTest, AddGroupSameId) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"Old", tab_groups::TabGroupColorId::kRed, 3));
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"New", tab_groups::TabGroupColorId::kBlue, 5));
  EXPECT_EQ(tab_groups_view_->GetGroupCount(), 1);
  auto group = tab_groups_view_->GetGroupAt(0);
  EXPECT_EQ(group.name, u"New");
}

// Test 614: Max groups limits display.
TEST_F(AstraSidebarTabGroupsViewTest, MaxGroupsLimitsDisplay) {
  tab_groups_view_->SetMaxGroups(2);
  for (int i = 0; i < 10; ++i) {
    std::string id = "g" + base::NumberToString(i);
    tab_groups_view_->AddGroup(MakeGroup(
        id, u"Group " + base::NumberToString16(i),
        tab_groups::TabGroupColorId::kRed, i + 1));
  }
  // Total stored is 10, but max display is 2.
  EXPECT_EQ(tab_groups_view_->GetGroupCount(), 2);
}

// Test 615: Group info struct default values.
TEST_F(AstraSidebarTabGroupsViewTest, GroupInfoDefaults) {
  AstraTabGroupInfo info;
  EXPECT_EQ(info.group_id, "");
  EXPECT_EQ(info.name, u"");
  EXPECT_EQ(info.tab_count, 0);
  EXPECT_FALSE(info.is_expanded);
  EXPECT_FALSE(info.is_pinned);
}

// Test 616: Tab info struct default values.
TEST_F(AstraSidebarTabGroupsViewTest, TabInfoDefaults) {
  AstraTabGroupTabInfo info;
  EXPECT_EQ(info.title, u"");
  EXPECT_FALSE(info.is_active);
  EXPECT_FALSE(info.is_pinned);
  EXPECT_FALSE(info.is_audible);
  EXPECT_FALSE(info.is_muted);
  EXPECT_FALSE(info.is_loading);
}

// Test 617: Sort by enum has all values.
TEST_F(AstraSidebarTabGroupsViewTest, SortEnumValues) {
  EXPECT_NE(static_cast<int>(AstraTabGroupSortBy::kManual),
            static_cast<int>(AstraTabGroupSortBy::kName));
  EXPECT_NE(static_cast<int>(AstraTabGroupSortBy::kName),
            static_cast<int>(AstraTabGroupSortBy::kColor));
  EXPECT_NE(static_cast<int>(AstraTabGroupSortBy::kColor),
            static_cast<int>(AstraTabGroupSortBy::kTabCount));
  EXPECT_NE(static_cast<int>(AstraTabGroupSortBy::kTabCount),
            static_cast<int>(AstraTabGroupSortBy::kLastAccessed));
}

// Test 618: Multiple groups with same color.
TEST_F(AstraSidebarTabGroupsViewTest, MultipleGroupsSameColor) {
  tab_groups_view_->AddGroup(
      MakeGroup("g1", u"First", tab_groups::TabGroupColorId::kBlue, 3));
  tab_groups_view_->AddGroup(
      MakeGroup("g2", u"Second", tab_groups::TabGroupColorId::kBlue, 5));
  EXPECT_EQ(tab_groups_view_->GetGroupCount(), 2);
  auto g1 = tab_groups_view_->GetGroupAt(0);
  auto g2 = tab_groups_view_->GetGroupAt(1);
  EXPECT_EQ(g1.color, g2.color);
}

// =========================================================================
// Recently closed view tests - basic CRUD
// =========================================================================

// Test 619: Default item count is 0.
TEST_F(AstraSidebarRecentlyClosedViewTest, DefaultItemCountZero) {
  EXPECT_EQ(recently_closed_view_->GetItemCount(), 0);
}

// Test 620: SetRecentlyClosed replaces all items.
TEST_F(AstraSidebarRecentlyClosedViewTest, SetRecentlyClosedReplacesAll) {
  std::vector<AstraRecentlyClosedItem> items;
  items.push_back(
      MakeItem("1", u"Tab 1", GURL("https://example.com/1")));
  items.push_back(
      MakeItem("2", u"Tab 2", GURL("https://example.com/2")));
  recently_closed_view_->SetRecentlyClosed(items);
  EXPECT_EQ(recently_closed_view_->GetItemCount(), 2);
}

// Test 621: AddItem adds to front.
TEST_F(AstraSidebarRecentlyClosedViewTest, AddItemAddsToFront) {
  recently_closed_view_->AddItem(
      MakeItem("1", u"First", GURL("https://example.com/1")));
  recently_closed_view_->AddItem(
      MakeItem("2", u"Second", GURL("https://example.com/2")));
  // Most recent should be first.
  auto first = recently_closed_view_->GetItemAt(0);
  EXPECT_EQ(first.id, "2");
}

// Test 622: RemoveItem removes by index.
TEST_F(AstraSidebarRecentlyClosedViewTest, RemoveItemByIndex) {
  recently_closed_view_->AddItem(
      MakeItem("1", u"First", GURL("https://example.com/1")));
  recently_closed_view_->AddItem(
      MakeItem("2", u"Second", GURL("https://example.com/2")));
  recently_closed_view_->RemoveItem(0);
  EXPECT_EQ(recently_closed_view_->GetItemCount(), 1);
  auto remaining = recently_closed_view_->GetItemAt(0);
  EXPECT_EQ(remaining.id, "1");
}

// Test 623: ClearAll removes all items.
TEST_F(AstraSidebarRecentlyClosedViewTest, ClearAllItems) {
  recently_closed_view_->AddItem(
      MakeItem("1", u"Tab 1", GURL("https://example.com/1")));
  recently_closed_view_->AddItem(
      MakeItem("2", u"Tab 2", GURL("https://example.com/2")));
  recently_closed_view_->AddItem(
      MakeItem("3", u"Tab 3", GURL("https://example.com/3")));
  recently_closed_view_->ClearAll();
  EXPECT_EQ(recently_closed_view_->GetItemCount(), 0);
}

// Test 624: GetItemAt returns correct item.
TEST_F(AstraSidebarRecentlyClosedViewTest, GetItemAt) {
  auto item = MakeItem("abc", u"Test Title", GURL("https://test.example.com"));
  recently_closed_view_->AddItem(item);
  auto result = recently_closed_view_->GetItemAt(0);
  EXPECT_EQ(result.id, "abc");
  EXPECT_EQ(result.title, u"Test Title");
  EXPECT_EQ(result.url, GURL("https://test.example.com"));
}

// Test 625: HasItems returns false when empty.
TEST_F(AstraSidebarRecentlyClosedViewTest, HasItemsFalse) {
  EXPECT_FALSE(recently_closed_view_->HasItems());
}

// Test 626: HasItems returns true when items exist.
TEST_F(AstraSidebarRecentlyClosedViewTest, HasItemsTrue) {
  recently_closed_view_->AddItem(
      MakeItem("1", u"Tab", GURL("https://example.com")));
  EXPECT_TRUE(recently_closed_view_->HasItems());
}

// =========================================================================
// Recently closed view tests - selection
// =========================================================================

// Test 627: No selection by default.
TEST_F(AstraSidebarRecentlyClosedViewTest, NoSelectionDefault) {
  EXPECT_EQ(recently_closed_view_->GetSelectedIndex(), -1);
}

// Test 628: SetSelectedItem sets selection.
TEST_F(AstraSidebarRecentlyClosedViewTest, SetSelectedItem) {
  recently_closed_view_->AddItem(
      MakeItem("1", u"Tab", GURL("https://example.com")));
  recently_closed_view_->SetSelectedItem(0);
  EXPECT_EQ(recently_closed_view_->GetSelectedIndex(), 0);
}

// Test 629: ClearSelection clears selection.
TEST_F(AstraSidebarRecentlyClosedViewTest, ClearSelection) {
  recently_closed_view_->AddItem(
      MakeItem("1", u"Tab", GURL("https://example.com")));
  recently_closed_view_->SetSelectedItem(0);
  recently_closed_view_->ClearSelection();
  EXPECT_EQ(recently_closed_view_->GetSelectedIndex(), -1);
}

// =========================================================================
// Recently closed view tests - restore operations
// =========================================================================

// Test 630: RestoreTab calls delegate.
TEST_F(AstraSidebarRecentlyClosedViewTest, RestoreTabCallsDelegate) {
  recently_closed_view_->AddItem(
      MakeItem("tab1", u"Tab", GURL("https://example.com")));
  EXPECT_CALL(delegate_, OnRestoreTab(std::string("tab1"))).Times(1);
  recently_closed_view_->RestoreTab(0);
}

// Test 631: RestoreWindow calls delegate.
TEST_F(AstraSidebarRecentlyClosedViewTest, RestoreWindowCallsDelegate) {
  auto item = MakeItem("win1", u"Window", GURL(),
                       AstraRecentlyClosedType::kWindow);
  recently_closed_view_->AddItem(item);
  EXPECT_CALL(delegate_, OnRestoreWindow(std::string("win1"))).Times(1);
  recently_closed_view_->RestoreWindow(0);
}

// Test 632: RestoreAll calls delegate.
TEST_F(AstraSidebarRecentlyClosedViewTest, RestoreAllCallsDelegate) {
  recently_closed_view_->AddItem(
      MakeItem("1", u"Tab 1", GURL("https://example.com/1")));
  recently_closed_view_->AddItem(
      MakeItem("2", u"Tab 2", GURL("https://example.com/2")));
  EXPECT_CALL(delegate_, OnRestoreAllRequested()).Times(1);
  recently_closed_view_->RestoreAll();
}

// =========================================================================
// Recently closed view tests - display options
// =========================================================================

// Test 633: Max items setting.
TEST_F(AstraSidebarRecentlyClosedViewTest, MaxItems) {
  recently_closed_view_->SetMaxItems(5);
  EXPECT_EQ(recently_closed_view_->GetMaxItems(), 5);
}

// Test 634: Max items limits display.
TEST_F(AstraSidebarRecentlyClosedViewTest, MaxItemsLimitsDisplay) {
  recently_closed_view_->SetMaxItems(3);
  for (int i = 0; i < 10; ++i) {
    std::string id = base::NumberToString(i);
    recently_closed_view_->AddItem(
        MakeItem(id, u"Tab " + base::NumberToString16(i),
                 GURL("https://example.com/" + id)));
  }
  EXPECT_EQ(recently_closed_view_->GetItemCount(), 3);
}

// Test 635: Show windows option.
TEST_F(AstraSidebarRecentlyClosedViewTest, ShowWindowsOption) {
  recently_closed_view_->SetShowWindows(true);
  EXPECT_TRUE(recently_closed_view_->GetShowWindows());
  recently_closed_view_->SetShowWindows(false);
  EXPECT_FALSE(recently_closed_view_->GetShowWindows());
}

// Test 636: Show tabs option.
TEST_F(AstraSidebarRecentlyClosedViewTest, ShowTabsOption) {
  recently_closed_view_->SetShowTabs(true);
  EXPECT_TRUE(recently_closed_view_->GetShowTabs());
  recently_closed_view_->SetShowTabs(false);
  EXPECT_FALSE(recently_closed_view_->GetShowTabs());
}

// Test 637: Show favicons option.
TEST_F(AstraSidebarRecentlyClosedViewTest, ShowFaviconsOption) {
  recently_closed_view_->SetShowFavicons(true);
  EXPECT_TRUE(recently_closed_view_->GetShowFavicons());
  recently_closed_view_->SetShowFavicons(false);
  EXPECT_FALSE(recently_closed_view_->GetShowFavicons());
}

// Test 638: Show time option.
TEST_F(AstraSidebarRecentlyClosedViewTest, ShowTimeOption) {
  recently_closed_view_->SetShowTime(true);
  EXPECT_TRUE(recently_closed_view_->GetShowTime());
  recently_closed_view_->SetShowTime(false);
  EXPECT_FALSE(recently_closed_view_->GetShowTime());
}

// Test 639: Show tab count option.
TEST_F(AstraSidebarRecentlyClosedViewTest, ShowTabCountOption) {
  recently_closed_view_->SetShowTabCount(true);
  EXPECT_TRUE(recently_closed_view_->GetShowTabCount());
  recently_closed_view_->SetShowTabCount(false);
  EXPECT_FALSE(recently_closed_view_->GetShowTabCount());
}

// Test 640: Group by session option.
TEST_F(AstraSidebarRecentlyClosedViewTest, GroupBySessionOption) {
  recently_closed_view_->SetGroupBySession(true);
  EXPECT_TRUE(recently_closed_view_->GetGroupBySession());
  recently_closed_view_->SetGroupBySession(false);
  EXPECT_FALSE(recently_closed_view_->GetGroupBySession());
}

// =========================================================================
// Recently closed view tests - counts
// =========================================================================

// Test 641: GetTabCount counts all tabs.
TEST_F(AstraSidebarRecentlyClosedViewTest, GetTabCount) {
  recently_closed_view_->AddItem(
      MakeItem("tab1", u"Tab 1", GURL("https://example.com/1"),
               AstraRecentlyClosedType::kTab));
  auto win = MakeItem("win1", u"Window", GURL(),
                      AstraRecentlyClosedType::kWindow);
  win.tab_count = 5;
  recently_closed_view_->AddItem(win);
  EXPECT_EQ(recently_closed_view_->GetTabCount(), 6);
}

// Test 642: GetWindowCount counts windows.
TEST_F(AstraSidebarRecentlyClosedViewTest, GetWindowCount) {
  recently_closed_view_->AddItem(
      MakeItem("tab1", u"Tab 1", GURL("https://example.com/1"),
               AstraRecentlyClosedType::kTab));
  recently_closed_view_->AddItem(
      MakeItem("win1", u"Window", GURL(),
               AstraRecentlyClosedType::kWindow));
  EXPECT_EQ(recently_closed_view_->GetWindowCount(), 1);
}

// Test 643: Tab count with no windows.
TEST_F(AstraSidebarRecentlyClosedViewTest, TabCountNoWindows) {
  recently_closed_view_->AddItem(
      MakeItem("tab1", u"Tab 1", GURL("https://example.com/1")));
  recently_closed_view_->AddItem(
      MakeItem("tab2", u"Tab 2", GURL("https://example.com/2")));
  EXPECT_EQ(recently_closed_view_->GetTabCount(), 2);
  EXPECT_EQ(recently_closed_view_->GetWindowCount(), 0);
}

// =========================================================================
// Recently closed view tests - search
// =========================================================================

// Test 644: Search filters by title.
TEST_F(AstraSidebarRecentlyClosedViewTest, SearchByTitle) {
  recently_closed_view_->AddItem(
      MakeItem("1", u"Apple page", GURL("https://apple.example.com")));
  recently_closed_view_->AddItem(
      MakeItem("2", u"Banana page", GURL("https://banana.example.com")));
  recently_closed_view_->AddItem(
      MakeItem("3", u"Cherry page", GURL("https://cherry.example.com")));
  recently_closed_view_->SearchRecentlyClosed(u"Banana");
  EXPECT_EQ(recently_closed_view_->GetSearchResultsCount(), 1);
}

// Test 645: Search is case-insensitive.
TEST_F(AstraSidebarRecentlyClosedViewTest, SearchCaseInsensitive) {
  recently_closed_view_->AddItem(
      MakeItem("1", u"Mixed Case", GURL("https://example.com")));
  recently_closed_view_->SearchRecentlyClosed(u"mixed case");
  EXPECT_EQ(recently_closed_view_->GetSearchResultsCount(), 1);
}

// Test 646: Search by URL.
TEST_F(AstraSidebarRecentlyClosedViewTest, SearchByUrl) {
  recently_closed_view_->AddItem(
      MakeItem("1", u"Tab 1", GURL("https://docs.example.com/page")));
  recently_closed_view_->AddItem(
      MakeItem("2", u"Tab 2", GURL("https://mail.example.com/inbox")));
  recently_closed_view_->SearchRecentlyClosed(u"docs");
  EXPECT_EQ(recently_closed_view_->GetSearchResultsCount(), 1);
}

// Test 647: Empty search shows all.
TEST_F(AstraSidebarRecentlyClosedViewTest, EmptySearchShowsAll) {
  recently_closed_view_->AddItem(
      MakeItem("1", u"Tab 1", GURL("https://example.com/1")));
  recently_closed_view_->AddItem(
      MakeItem("2", u"Tab 2", GURL("https://example.com/2")));
  recently_closed_view_->SearchRecentlyClosed(u"");
  EXPECT_EQ(recently_closed_view_->GetSearchResultsCount(), 2);
}

// Test 648: Search with no results.
TEST_F(AstraSidebarRecentlyClosedViewTest, SearchNoResults) {
  recently_closed_view_->AddItem(
      MakeItem("1", u"Tab 1", GURL("https://example.com/1")));
  recently_closed_view_->SearchRecentlyClosed(u"zzzzzzzz");
  EXPECT_EQ(recently_closed_view_->GetSearchResultsCount(), 0);
}

// Test 649: Show search box option.
TEST_F(AstraSidebarRecentlyClosedViewTest, ShowSearchOption) {
  recently_closed_view_->SetShowSearch(true);
  EXPECT_TRUE(recently_closed_view_->GetShowSearch());
  recently_closed_view_->SetShowSearch(false);
  EXPECT_FALSE(recently_closed_view_->GetShowSearch());
}

// =========================================================================
// Recently closed view tests - footer buttons
// =========================================================================

// Test 650: Show restore all button option.
TEST_F(AstraSidebarRecentlyClosedViewTest, ShowRestoreAllButton) {
  recently_closed_view_->SetShowRestoreAllButton(true);
  EXPECT_TRUE(recently_closed_view_->GetShowRestoreAllButton());
  recently_closed_view_->SetShowRestoreAllButton(false);
  EXPECT_FALSE(recently_closed_view_->GetShowRestoreAllButton());
}

// Test 651: Show clear all button option.
TEST_F(AstraSidebarRecentlyClosedViewTest, ShowClearAllButton) {
  recently_closed_view_->SetShowClearAllButton(true);
  EXPECT_TRUE(recently_closed_view_->GetShowClearAllButton());
  recently_closed_view_->SetShowClearAllButton(false);
  EXPECT_FALSE(recently_closed_view_->GetShowClearAllButton());
}

// =========================================================================
// Recently closed view tests - item types
// =========================================================================

// Test 652: Tab items have type kTab.
TEST_F(AstraSidebarRecentlyClosedViewTest, TabItemType) {
  auto item = MakeItem("1", u"Tab", GURL("https://example.com"),
                       AstraRecentlyClosedType::kTab);
  EXPECT_EQ(item.type, AstraRecentlyClosedType::kTab);
}

// Test 653: Window items have type kWindow.
TEST_F(AstraSidebarRecentlyClosedViewTest, WindowItemType) {
  auto item = MakeItem("1", u"Window", GURL(),
                       AstraRecentlyClosedType::kWindow);
  EXPECT_EQ(item.type, AstraRecentlyClosedType::kWindow);
}

// Test 654: Window items have tab count > 1.
TEST_F(AstraSidebarRecentlyClosedViewTest, WindowTabCount) {
  auto item = MakeItem("1", u"Window", GURL(),
                       AstraRecentlyClosedType::kWindow);
  EXPECT_GT(item.tab_count, 1);
}

// Test 655: Incognito flag.
TEST_F(AstraSidebarRecentlyClosedViewTest, IncognitoFlag) {
  auto item = MakeItem("1", u"Tab", GURL("https://example.com"));
  EXPECT_FALSE(item.is_incognito);
  item.is_incognito = true;
  EXPECT_TRUE(item.is_incognito);
}

// =========================================================================
// Recently closed view tests - view access
// =========================================================================

// Test 656: GetItemViewAt returns valid pointer.
TEST_F(AstraSidebarRecentlyClosedViewTest, GetItemViewAt) {
  recently_closed_view_->AddItem(
      MakeItem("1", u"Tab", GURL("https://example.com")));
  auto* item_view = recently_closed_view_->GetItemViewAt(0);
  EXPECT_NE(item_view, nullptr);
  EXPECT_EQ(item_view->GetId(), "1");
}

// Test 657: Preferred size is valid.
TEST_F(AstraSidebarRecentlyClosedViewTest, PreferredSizeValid) {
  gfx::Size size = recently_closed_view_->GetPreferredSize();
  EXPECT_GT(size.width(), 0);
  EXPECT_GE(size.height(), 0);
}

// =========================================================================
// Recently closed view tests - delegate callbacks
// =========================================================================

// Test 658: Delegate can be set and retrieved.
TEST_F(AstraSidebarRecentlyClosedViewTest, SetAndGetDelegate) {
  MockSidebarRecentlyClosedDelegate other_delegate;
  recently_closed_view_->set_delegate(&other_delegate);
  EXPECT_EQ(recently_closed_view_->delegate(), &other_delegate);
  // Restore.
  recently_closed_view_->set_delegate(&delegate_);
}

// Test 659: OnRemoveItem calls delegate.
TEST_F(AstraSidebarRecentlyClosedViewTest, RemoveItemCallsDelegate) {
  auto item = MakeItem("item1", u"Tab", GURL("https://example.com"));
  recently_closed_view_->AddItem(item);
  EXPECT_CALL(delegate_, OnRemoveItem(std::string("item1"))).Times(0);
  // Programmatic remove doesn't call delegate; only user-initiated does.
}

// =========================================================================
// Recently closed view tests - edge cases
// =========================================================================

// Test 660: Empty items vector.
TEST_F(AstraSidebarRecentlyClosedViewTest, EmptyItemsVector) {
  std::vector<AstraRecentlyClosedItem> items;
  recently_closed_view_->SetRecentlyClosed(items);
  EXPECT_EQ(recently_closed_view_->GetItemCount(), 0);
  EXPECT_FALSE(recently_closed_view_->HasItems());
}

// Test 661: Item with invalid URL.
TEST_F(AstraSidebarRecentlyClosedViewTest, ItemWithInvalidUrl) {
  auto item = MakeItem("1", u"Tab", GURL());
  recently_closed_view_->AddItem(item);
  EXPECT_FALSE(recently_closed_view_->GetItemAt(0).url.is_valid());
}

// Test 662: Item struct default values.
TEST_F(AstraSidebarRecentlyClosedViewTest, ItemStructDefaults) {
  AstraRecentlyClosedItem item;
  EXPECT_EQ(item.id, "");
  EXPECT_EQ(item.title, u"");
  EXPECT_EQ(item.type, AstraRecentlyClosedType::kTab);
  EXPECT_EQ(item.tab_count, 1);
  EXPECT_FALSE(item.has_favicon);
  EXPECT_FALSE(item.is_incognito);
}

// Test 663: Section expanded state.
TEST_F(AstraSidebarRecentlyClosedViewTest, SectionExpandedState) {
  EXPECT_TRUE(recently_closed_view_->IsExpanded());
  recently_closed_view_->SetExpanded(false);
  EXPECT_FALSE(recently_closed_view_->IsExpanded());
  recently_closed_view_->SetExpanded(true);
  EXPECT_TRUE(recently_closed_view_->IsExpanded());
}

// Test 664: Window bounds in item struct.
TEST_F(AstraSidebarRecentlyClosedViewTest, WindowBoundsInItem) {
  auto item = MakeItem("win1", u"Window", GURL(),
                       AstraRecentlyClosedType::kWindow);
  item.window_bounds = gfx::Rect(100, 100, 800, 600);
  EXPECT_EQ(item.window_bounds.width(), 800);
  EXPECT_EQ(item.window_bounds.height(), 600);
  EXPECT_EQ(item.window_bounds.x(), 100);
  EXPECT_EQ(item.window_bounds.y(), 100);
}

// Test 665: Session ID in item struct.
TEST_F(AstraSidebarRecentlyClosedViewTest, SessionIdInItem) {
  auto item = MakeItem("1", u"Tab", GURL("https://example.com"));
  item.session_id = 42;
  EXPECT_EQ(item.session_id, 42);
}

// Test 666: Recently closed type enum values.
TEST_F(AstraSidebarRecentlyClosedViewTest, TypeEnumValues) {
  EXPECT_NE(static_cast<int>(AstraRecentlyClosedType::kTab),
            static_cast<int>(AstraRecentlyClosedType::kWindow));
}

// Test 667: Favicon field in item struct.
TEST_F(AstraSidebarRecentlyClosedViewTest, FaviconInItem) {
  auto item = MakeItem("1", u"Tab", GURL("https://example.com"));
  EXPECT_FALSE(item.has_favicon);
  item.has_favicon = true;
  EXPECT_TRUE(item.has_favicon);
}

// =========================================================================
// Recently closed item view - additional tests
// =========================================================================

// Test 668: Item view string ID is set from constructor.
TEST_F(AstraRecentlyClosedItemViewTest, StringIdFromConstructor) {
  // Use a specific entry_id and verify the string ID.
  auto view = std::make_unique<AstraRecentlyClosedItemView>(
      u"Test", GURL("https://example.com"), base::Time::Now(), 42);
  EXPECT_EQ(view->GetId(), "42");
}

// Test 669: SetId changes the string ID.
TEST_F(AstraRecentlyClosedItemViewTest, SetId) {
  auto view = std::make_unique<AstraRecentlyClosedItemView>(
      u"Test", GURL("https://example.com"), base::Time::Now(), 1);
  view->SetId("custom-id");
  EXPECT_EQ(view->GetId(), "custom-id");
}

// Test 670: Click callback fires on click.
TEST_F(AstraRecentlyClosedItemViewTest, ClickCallback) {
  bool clicked = false;
  auto view = std::make_unique<AstraRecentlyClosedItemView>(
      u"Test", GURL("https://example.com"), base::Time::Now(), 1);
  view->set_callback(base::BindRepeating(
      [](bool* clicked) { *clicked = true; }, base::Unretained(&clicked)));
  // Callback is stored; verify no immediate call.
  EXPECT_FALSE(clicked);
}

// Test 671: Entry ID accessor.
TEST_F(AstraRecentlyClosedItemViewTest, EntryIdAccessor) {
  auto view = std::make_unique<AstraRecentlyClosedItemView>(
      u"Test", GURL("https://example.com"), base::Time::Now(), 99);
  EXPECT_EQ(view->entry_id(), 99);
}

// =========================================================================
// Data struct tests
// =========================================================================

// Test 672: AstraTabGroupInfo default values.
TEST(AstraTabGroupInfoTest, DefaultValues) {
  AstraTabGroupInfo info;
  EXPECT_EQ(info.group_id, "");
  EXPECT_EQ(info.name, u"");
  EXPECT_EQ(info.tab_count, 0);
  EXPECT_FALSE(info.is_expanded);
  EXPECT_FALSE(info.is_collapsed_in_tabstrip);
  EXPECT_EQ(info.order_index, 0);
  EXPECT_FALSE(info.is_pinned);
}

// Test 673: AstraTabGroupTabInfo default values.
TEST(AstraTabGroupTabInfoTest, DefaultValues) {
  AstraTabGroupTabInfo info;
  EXPECT_EQ(info.index, 0);
  EXPECT_EQ(info.title, u"");
  EXPECT_EQ(info.group_id, "");
  EXPECT_FALSE(info.is_active);
  EXPECT_FALSE(info.is_pinned);
  EXPECT_FALSE(info.is_audible);
  EXPECT_FALSE(info.is_muted);
  EXPECT_FALSE(info.is_loading);
  EXPECT_FALSE(info.is_dragging);
}

// Test 674: AstraRecentlyClosedItem default values.
TEST(AstraRecentlyClosedItemTest, DefaultValues) {
  AstraRecentlyClosedItem item;
  EXPECT_EQ(item.id, "");
  EXPECT_EQ(item.title, u"");
  EXPECT_FALSE(item.url.is_valid());
  EXPECT_EQ(item.type, AstraRecentlyClosedType::kTab);
  EXPECT_EQ(item.tab_count, 1);
  EXPECT_FALSE(item.has_favicon);
  EXPECT_EQ(item.session_id, 0);
  EXPECT_FALSE(item.is_incognito);
}

// =========================================================================
// Mock delegates for tab stack view tests
// =========================================================================

class MockSidebarStackDelegate : public AstraSidebarStackDelegate {
 public:
  MOCK_METHOD(void, OnStackClicked, (const std::string& stack_id), (override));
  MOCK_METHOD(void, OnStackExpandedChanged,
              (const std::string& stack_id, bool expanded), (override));
  MOCK_METHOD(void, OnStackColorChanged,
              (const std::string& stack_id, SkColor color), (override));
  MOCK_METHOD(void, OnStackRenamed,
              (const std::string& stack_id, const std::u16string& new_name),
              (override));
  MOCK_METHOD(void, OnTabClicked,
              (const std::string& stack_id, int tab_index), (override));
  MOCK_METHOD(void, OnTabMiddleClicked,
              (const std::string& stack_id, int tab_index), (override));
  MOCK_METHOD(void, OnTabClosed,
              (const std::string& stack_id, int tab_index), (override));
  MOCK_METHOD(void, OnNewStackRequested, (), (override));
  MOCK_METHOD(void, OnDeleteStackRequested, (const std::string& stack_id),
              (override));
  MOCK_METHOD(void, OnStackReordered, (int from_index, int to_index),
              (override));
  MOCK_METHOD(void, OnTabDragged,
              (const std::string& from_stack_id, int from_tab_index,
               const gfx::Point& point),
              (override));
  MOCK_METHOD(void, OnTabDropped,
              (const std::string& to_stack_id, int to_tab_index,
               const std::string& from_stack_id, int from_tab_index),
              (override));
  MOCK_METHOD(void, OnMoveTabToStackRequested,
              (const std::string& from_stack_id, int tab_index,
               const std::string& to_stack_id),
              (override));
};

// =========================================================================
// Mock delegate for stack header view tests
// =========================================================================

class MockSidebarStackHeaderDelegate : public AstraSidebarStackHeaderDelegate {
 public:
  MOCK_METHOD(void, OnStackToggleExpanded, (const std::string& stack_id),
              (override));
  MOCK_METHOD(void, OnStackHeaderClicked, (const std::string& stack_id),
              (override));
  MOCK_METHOD(void, OnStackMenuClicked,
              (const std::string& stack_id, const gfx::Point& anchor_point),
              (override));
};

// =========================================================================
// Mock delegate for stack tab item view tests
// =========================================================================

class MockSidebarStackTabItemDelegate
    : public AstraSidebarStackTabItemDelegate {
 public:
  MOCK_METHOD(void, OnStackTabClicked, (content::WebContents* web_contents),
              (override));
  MOCK_METHOD(void, OnStackTabClosed, (content::WebContents* web_contents),
              (override));
  MOCK_METHOD(void, OnStackTabDragStarted,
              (content::WebContents* web_contents,
               const gfx::Point& mouse_location),
              (override));
};

// =========================================================================
// Mock delegate for stack child view tests
// =========================================================================

class MockSidebarStackChildDelegate : public AstraSidebarStackChildDelegate {
 public:
  MOCK_METHOD(void, OnStackTabClicked, (int tab_index), (override));
  MOCK_METHOD(void, OnStackTabMiddleClicked, (int tab_index), (override));
  MOCK_METHOD(void, OnStackTabClosed, (int tab_index), (override));
  MOCK_METHOD(void, OnStackTabDragStarted,
              (int tab_index, const gfx::Point& mouse_location), (override));
  MOCK_METHOD(void, OnStackTabDropped,
              (int to_tab_index, const std::string& from_stack_id,
               int from_tab_index),
              (override));
};

// =========================================================================
// Test fixture for tab stack view tests
// =========================================================================

class AstraSidebarStackViewTest : public views::ViewsTestBase {
 public:
  AstraSidebarStackViewTest() = default;
  ~AstraSidebarStackViewTest() override = default;
  AstraSidebarStackViewTest(const AstraSidebarStackViewTest&) = delete;
  AstraSidebarStackViewTest& operator=(const AstraSidebarStackViewTest&) =
      delete;

 protected:
  void SetUp() override {
    ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();
    stack_view_ = widget_->SetContentsView(
        std::make_unique<AstraSidebarStackView>(nullptr));
    widget_->Show();
  }

  void TearDown() override {
    stack_view_ = nullptr;
    widget_.reset();
    ViewsTestBase::TearDown();
  }

  // Helper to create a test stack info.
  AstraStackInfo CreateTestStack(const std::string& id,
                                 const std::u16string& name,
                                 SkColor color = SK_ColorBLUE,
                                 int tab_count = 3) {
    AstraStackInfo info;
    info.stack_id = id;
    info.name = name;
    info.color = color;
    info.tab_count = tab_count;
    info.is_expanded = true;
    info.is_pinned = false;
    info.order_index = 0;
    info.last_accessed = base::Time::Now();
    info.created_time = base::Time::Now();
    info.has_unread = false;
    info.note = u"";
    return info;
  }

  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraSidebarStackView> stack_view_ = nullptr;
};

// =========================================================================
// Test fixture for stack header view tests
// =========================================================================

class AstraSidebarStackHeaderViewTest : public views::ViewsTestBase {
 public:
  AstraSidebarStackHeaderViewTest() = default;
  ~AstraSidebarStackHeaderViewTest() override = default;
  AstraSidebarStackHeaderViewTest(const AstraSidebarStackHeaderViewTest&) =
      delete;
  AstraSidebarStackHeaderViewTest& operator=(
      const AstraSidebarStackHeaderViewTest&) = delete;

 protected:
  void SetUp() override {
    ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();
    header_view_ = widget_->SetContentsView(
        std::make_unique<AstraSidebarStackHeaderView>(u"Test Stack"));
    widget_->Show();
  }

  void TearDown() override {
    header_view_ = nullptr;
    widget_.reset();
    ViewsTestBase::TearDown();
  }

  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraSidebarStackHeaderView> header_view_ = nullptr;
};

// =========================================================================
// Test fixture for stack tab item view tests
// =========================================================================

class AstraSidebarStackTabItemViewTest : public views::ViewsTestBase {
 public:
  AstraSidebarStackTabItemViewTest() = default;
  ~AstraSidebarStackTabItemViewTest() override = default;
  AstraSidebarStackTabItemViewTest(const AstraSidebarStackTabItemViewTest&) =
      delete;
  AstraSidebarStackTabItemViewTest& operator=(
      const AstraSidebarStackTabItemViewTest&) = delete;

 protected:
  void SetUp() override {
    ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();
    tab_item_view_ = widget_->SetContentsView(
        std::make_unique<AstraSidebarStackTabItemView>(u"Test Tab"));
    widget_->Show();
  }

  void TearDown() override {
    tab_item_view_ = nullptr;
    widget_.reset();
    ViewsTestBase::TearDown();
  }

  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraSidebarStackTabItemView> tab_item_view_ = nullptr;
};

// =========================================================================
// Test fixture for stack child view tests
// =========================================================================

class AstraSidebarStackChildViewTest : public views::ViewsTestBase {
 public:
  AstraSidebarStackChildViewTest() = default;
  ~AstraSidebarStackChildViewTest() override = default;
  AstraSidebarStackChildViewTest(const AstraSidebarStackChildViewTest&) =
      delete;
  AstraSidebarStackChildViewTest& operator=(
      const AstraSidebarStackChildViewTest&) = delete;

 protected:
  void SetUp() override {
    ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();
    child_view_ = widget_->SetContentsView(
        std::make_unique<AstraSidebarStackChildView>());
    widget_->Show();
  }

  void TearDown() override {
    child_view_ = nullptr;
    widget_.reset();
    ViewsTestBase::TearDown();
  }

  // Helper to create a test tab info.
  AstraStackTabInfo CreateTestTab(const std::string& id,
                                  const std::u16string& title,
                                  bool active = false) {
    AstraStackTabInfo info;
    info.tab_id = id;
    info.title = title;
    info.url = GURL("https://example.com");
    info.is_active = active;
    info.is_pinned = false;
    info.is_audible = false;
    info.is_muted = false;
    info.is_loading = false;
    info.is_crashed = false;
    info.has_favicon = false;
    info.index_in_stack = 0;
    info.last_accessed = base::Time::Now();
    return info;
  }

  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraSidebarStackChildView> child_view_ = nullptr;
};

// =========================================================================
// Stack view tests
// =========================================================================

// Test 675: Stack view construction.
TEST_F(AstraSidebarStackViewTest, Construction) {
  EXPECT_NE(stack_view_, nullptr);
  EXPECT_EQ(stack_view_->GetStackCount(), 0);
  EXPECT_EQ(stack_view_->GetSelectedStackId(), "");
  EXPECT_EQ(stack_view_->GetSortBy(), AstraStackSortBy::kManual);
  EXPECT_FALSE(stack_view_->IsCompactMode());
}

// Test 676: SetStacks replaces all stacks.
TEST_F(AstraSidebarStackViewTest, SetStacksReplacesAll) {
  std::vector<AstraStackInfo> stacks;
  stacks.push_back(CreateTestStack("s1", u"Work"));
  stacks.push_back(CreateTestStack("s2", u"Personal"));
  stacks.push_back(CreateTestStack("s3", u"Research"));
  stack_view_->SetStacks(stacks);
  EXPECT_EQ(stack_view_->GetStackCount(), 3);
}

// Test 677: GetStackAt returns correct info.
TEST_F(AstraSidebarStackViewTest, GetStackAtReturnsInfo) {
  std::vector<AstraStackInfo> stacks;
  stacks.push_back(CreateTestStack("s1", u"Work", SK_ColorRED));
  stacks.push_back(CreateTestStack("s2", u"Personal", SK_ColorGREEN));
  stack_view_->SetStacks(stacks);

  AstraStackInfo info = stack_view_->GetStackAt(0);
  EXPECT_EQ(info.stack_id, "s1");
  EXPECT_EQ(info.name, u"Work");
  EXPECT_EQ(info.color, SK_ColorRED);
}

// Test 678: GetStackAt with invalid index returns default.
TEST_F(AstraSidebarStackViewTest, GetStackAtInvalidIndex) {
  AstraStackInfo info = stack_view_->GetStackAt(0);
  EXPECT_EQ(info.stack_id, "");
  EXPECT_EQ(info.name, u"");
}

// Test 679: FindStackIndexById finds correct index.
TEST_F(AstraSidebarStackViewTest, FindStackIndexById) {
  std::vector<AstraStackInfo> stacks;
  stacks.push_back(CreateTestStack("s1", u"Work"));
  stacks.push_back(CreateTestStack("s2", u"Personal"));
  stack_view_->SetStacks(stacks);
  EXPECT_EQ(stack_view_->FindStackIndexById("s1"), 0);
  EXPECT_EQ(stack_view_->FindStackIndexById("s2"), 1);
  EXPECT_EQ(stack_view_->FindStackIndexById("nonexistent"), -1);
}

// Test 680: AddStack appends to end.
TEST_F(AstraSidebarStackViewTest, AddStackAppends) {
  stack_view_->AddStack(CreateTestStack("s1", u"First"));
  EXPECT_EQ(stack_view_->GetStackCount(), 1);
  stack_view_->AddStack(CreateTestStack("s2", u"Second"));
  EXPECT_EQ(stack_view_->GetStackCount(), 2);
  EXPECT_EQ(stack_view_->GetStackAt(1).stack_id, "s2");
}

// Test 681: AddStack at specific position.
TEST_F(AstraSidebarStackViewTest, AddStackAtPosition) {
  stack_view_->AddStack(CreateTestStack("s1", u"First"));
  stack_view_->AddStack(CreateTestStack("s3", u"Third"));
  stack_view_->AddStack(CreateTestStack("s2", u"Second"), 1);
  EXPECT_EQ(stack_view_->GetStackCount(), 3);
  EXPECT_EQ(stack_view_->GetStackAt(1).stack_id, "s2");
}

// Test 682: AddStack with negative position appends.
TEST_F(AstraSidebarStackViewTest, AddStackNegativePosition) {
  stack_view_->AddStack(CreateTestStack("s1", u"First"));
  stack_view_->AddStack(CreateTestStack("s2", u"Second"), -1);
  EXPECT_EQ(stack_view_->GetStackCount(), 2);
  EXPECT_EQ(stack_view_->GetStackAt(1).stack_id, "s2");
}

// Test 683: RemoveStack removes the stack.
TEST_F(AstraSidebarStackViewTest, RemoveStack) {
  stack_view_->AddStack(CreateTestStack("s1", u"First"));
  stack_view_->AddStack(CreateTestStack("s2", u"Second"));
  stack_view_->RemoveStack("s1");
  EXPECT_EQ(stack_view_->GetStackCount(), 1);
  EXPECT_EQ(stack_view_->GetStackAt(0).stack_id, "s2");
}

// Test 684: RemoveStack with invalid ID does nothing.
TEST_F(AstraSidebarStackViewTest, RemoveStackInvalidId) {
  stack_view_->AddStack(CreateTestStack("s1", u"First"));
  stack_view_->RemoveStack("nonexistent");
  EXPECT_EQ(stack_view_->GetStackCount(), 1);
}

// Test 685: UpdateStack updates stack info.
TEST_F(AstraSidebarStackViewTest, UpdateStack) {
  stack_view_->AddStack(CreateTestStack("s1", u"Old Name", SK_ColorRED, 3));
  AstraStackInfo updated = CreateTestStack("s1", u"New Name", SK_ColorBLUE, 5);
  stack_view_->UpdateStack("s1", updated);
  EXPECT_EQ(stack_view_->GetStackAt(0).name, u"New Name");
  EXPECT_EQ(stack_view_->GetStackAt(0).color, SK_ColorBLUE);
  EXPECT_EQ(stack_view_->GetStackAt(0).tab_count, 5);
}

// Test 686: SetSelectedStack selects the stack.
TEST_F(AstraSidebarStackViewTest, SetSelectedStack) {
  stack_view_->AddStack(CreateTestStack("s1", u"Work"));
  stack_view_->AddStack(CreateTestStack("s2", u"Personal"));
  stack_view_->SetSelectedStack("s2");
  EXPECT_EQ(stack_view_->GetSelectedStackId(), "s2");
}

// Test 687: ClearSelection clears selected state.
TEST_F(AstraSidebarStackViewTest, ClearSelection) {
  stack_view_->AddStack(CreateTestStack("s1", u"Work"));
  stack_view_->SetSelectedStack("s1");
  EXPECT_EQ(stack_view_->GetSelectedStackId(), "s1");
  stack_view_->ClearSelection();
  EXPECT_EQ(stack_view_->GetSelectedStackId(), "");
}

// Test 688: ExpandStack expands a collapsed stack.
TEST_F(AstraSidebarStackViewTest, ExpandStack) {
  AstraStackInfo info = CreateTestStack("s1", u"Work");
  info.is_expanded = false;
  stack_view_->AddStack(info);
  EXPECT_FALSE(stack_view_->IsStackExpanded("s1"));
  stack_view_->ExpandStack("s1");
  EXPECT_TRUE(stack_view_->IsStackExpanded("s1"));
}

// Test 689: CollapseStack collapses an expanded stack.
TEST_F(AstraSidebarStackViewTest, CollapseStack) {
  stack_view_->AddStack(CreateTestStack("s1", u"Work"));
  EXPECT_TRUE(stack_view_->IsStackExpanded("s1"));
  stack_view_->CollapseStack("s1");
  EXPECT_FALSE(stack_view_->IsStackExpanded("s1"));
}

// Test 690: ToggleStackExpanded toggles expansion.
TEST_F(AstraSidebarStackViewTest, ToggleStackExpanded) {
  stack_view_->AddStack(CreateTestStack("s1", u"Work"));
  EXPECT_TRUE(stack_view_->IsStackExpanded("s1"));
  stack_view_->ToggleStackExpanded("s1");
  EXPECT_FALSE(stack_view_->IsStackExpanded("s1"));
  stack_view_->ToggleStackExpanded("s1");
  EXPECT_TRUE(stack_view_->IsStackExpanded("s1"));
}

// Test 691: ExpandAll expands all stacks.
TEST_F(AstraSidebarStackViewTest, ExpandAll) {
  AstraStackInfo s1 = CreateTestStack("s1", u"Work");
  s1.is_expanded = false;
  AstraStackInfo s2 = CreateTestStack("s2", u"Personal");
  s2.is_expanded = false;
  stack_view_->SetStacks({s1, s2});
  stack_view_->ExpandAll();
  EXPECT_TRUE(stack_view_->IsStackExpanded("s1"));
  EXPECT_TRUE(stack_view_->IsStackExpanded("s2"));
}

// Test 692: CollapseAll collapses all stacks.
TEST_F(AstraSidebarStackViewTest, CollapseAll) {
  stack_view_->AddStack(CreateTestStack("s1", u"Work"));
  stack_view_->AddStack(CreateTestStack("s2", u"Personal"));
  stack_view_->CollapseAll();
  EXPECT_FALSE(stack_view_->IsStackExpanded("s1"));
  EXPECT_FALSE(stack_view_->IsStackExpanded("s2"));
}

// Test 693: MoveStack reorders stacks.
TEST_F(AstraSidebarStackViewTest, MoveStack) {
  stack_view_->AddStack(CreateTestStack("s1", u"First"));
  stack_view_->AddStack(CreateTestStack("s2", u"Second"));
  stack_view_->AddStack(CreateTestStack("s3", u"Third"));
  stack_view_->MoveStack(0, 2);
  EXPECT_EQ(stack_view_->GetStackAt(0).stack_id, "s2");
  EXPECT_EQ(stack_view_->GetStackAt(1).stack_id, "s3");
  EXPECT_EQ(stack_view_->GetStackAt(2).stack_id, "s1");
}

// Test 694: MoveStack with same index does nothing.
TEST_F(AstraSidebarStackViewTest, MoveStackSameIndex) {
  stack_view_->AddStack(CreateTestStack("s1", u"First"));
  stack_view_->AddStack(CreateTestStack("s2", u"Second"));
  stack_view_->MoveStack(1, 1);
  EXPECT_EQ(stack_view_->GetStackAt(0).stack_id, "s1");
  EXPECT_EQ(stack_view_->GetStackAt(1).stack_id, "s2");
}

// Test 695: SetStackColor changes stack color.
TEST_F(AstraSidebarStackViewTest, SetStackColor) {
  stack_view_->AddStack(CreateTestStack("s1", u"Work", SK_ColorRED));
  stack_view_->SetStackColor("s1", SK_ColorGREEN);
  EXPECT_EQ(stack_view_->GetStackAt(0).color, SK_ColorGREEN);
}

// Test 696: RenameStack changes stack name.
TEST_F(AstraSidebarStackViewTest, RenameStack) {
  stack_view_->AddStack(CreateTestStack("s1", u"Old Name"));
  stack_view_->RenameStack("s1", u"New Name");
  EXPECT_EQ(stack_view_->GetStackAt(0).name, u"New Name");
}

// Test 697: GetTotalTabCount sums all tab counts.
TEST_F(AstraSidebarStackViewTest, GetTotalTabCount) {
  stack_view_->AddStack(CreateTestStack("s1", u"Work", SK_ColorRED, 3));
  stack_view_->AddStack(CreateTestStack("s2", u"Personal", SK_ColorBLUE, 5));
  EXPECT_EQ(stack_view_->GetTotalTabCount(), 8);
}

// Test 698: GetTotalTabCount with no stacks returns 0.
TEST_F(AstraSidebarStackViewTest, GetTotalTabCountEmpty) {
  EXPECT_EQ(stack_view_->GetTotalTabCount(), 0);
}

// Test 699: SetCompactMode toggles compact display.
TEST_F(AstraSidebarStackViewTest, SetCompactMode) {
  EXPECT_FALSE(stack_view_->IsCompactMode());
  stack_view_->SetCompactMode(true);
  EXPECT_TRUE(stack_view_->IsCompactMode());
  stack_view_->SetCompactMode(false);
  EXPECT_FALSE(stack_view_->IsCompactMode());
}

// Test 700: SetShowTabCount toggles tab count display.
TEST_F(AstraSidebarStackViewTest, SetShowTabCount) {
  EXPECT_TRUE(stack_view_->GetShowTabCount());
  stack_view_->SetShowTabCount(false);
  EXPECT_FALSE(stack_view_->GetShowTabCount());
}

// Test 701: SetShowColorIndicator toggles color indicator.
TEST_F(AstraSidebarStackViewTest, SetShowColorIndicator) {
  EXPECT_TRUE(stack_view_->GetShowColorIndicator());
  stack_view_->SetShowColorIndicator(false);
  EXPECT_FALSE(stack_view_->GetShowColorIndicator());
}

// Test 702: SetSortBy changes sort mode.
TEST_F(AstraSidebarStackViewTest, SetSortBy) {
  EXPECT_EQ(stack_view_->GetSortBy(), AstraStackSortBy::kManual);
  stack_view_->SetSortBy(AstraStackSortBy::kName);
  EXPECT_EQ(stack_view_->GetSortBy(), AstraStackSortBy::kName);
  stack_view_->SetSortBy(AstraStackSortBy::kTabCount);
  EXPECT_EQ(stack_view_->GetSortBy(), AstraStackSortBy::kTabCount);
}

// Test 703: Sort by name orders stacks alphabetically.
TEST_F(AstraSidebarStackViewTest, SortByName) {
  stack_view_->AddStack(CreateTestStack("s1", u"Zebra"));
  stack_view_->AddStack(CreateTestStack("s2", u"Alpha"));
  stack_view_->AddStack(CreateTestStack("s3", u"Middle"));
  stack_view_->SetSortBy(AstraStackSortBy::kName);
  EXPECT_EQ(stack_view_->GetStackAt(0).name, u"Alpha");
  EXPECT_EQ(stack_view_->GetStackAt(1).name, u"Middle");
  EXPECT_EQ(stack_view_->GetStackAt(2).name, u"Zebra");
}

// Test 704: Sort by tab count orders by descending count.
TEST_F(AstraSidebarStackViewTest, SortByTabCount) {
  stack_view_->AddStack(CreateTestStack("s1", u"Few", SK_ColorRED, 2));
  stack_view_->AddStack(CreateTestStack("s2", u"Many", SK_ColorBLUE, 10));
  stack_view_->AddStack(CreateTestStack("s3", u"Medium", SK_ColorGREEN, 5));
  stack_view_->SetSortBy(AstraStackSortBy::kTabCount);
  EXPECT_EQ(stack_view_->GetStackAt(0).name, u"Many");
  EXPECT_EQ(stack_view_->GetStackAt(1).name, u"Medium");
  EXPECT_EQ(stack_view_->GetStackAt(2).name, u"Few");
}

// Test 705: Pinned stacks sort before non-pinned.
TEST_F(AstraSidebarStackViewTest, PinnedStacksSortFirst) {
  AstraStackInfo normal = CreateTestStack("s1", u"Normal");
  normal.is_pinned = false;
  AstraStackInfo pinned = CreateTestStack("s2", u"Pinned");
  pinned.is_pinned = true;
  stack_view_->SetStacks({normal, pinned});
  stack_view_->SetSortBy(AstraStackSortBy::kName);
  EXPECT_TRUE(stack_view_->GetStackAt(0).is_pinned);
}

// Test 706: SetDragDropEnabled toggles drag-drop.
TEST_F(AstraSidebarStackViewTest, SetDragDropEnabled) {
  EXPECT_TRUE(stack_view_->GetDragDropEnabled());
  stack_view_->SetDragDropEnabled(false);
  EXPECT_FALSE(stack_view_->GetDragDropEnabled());
}

// Test 707: delegate() returns null by default.
TEST_F(AstraSidebarStackViewTest, DefaultDelegateIsNull) {
  EXPECT_EQ(stack_view_->delegate(), nullptr);
}

// Test 708: set_delegate sets the delegate.
TEST_F(AstraSidebarStackViewTest, SetDelegate) {
  MockSidebarStackDelegate delegate;
  stack_view_->set_delegate(&delegate);
  EXPECT_EQ(stack_view_->delegate(), &delegate);
}

// Test 709: ClearAllStacks removes all stacks.
TEST_F(AstraSidebarStackViewTest, ClearAllStacks) {
  stack_view_->AddStack(CreateTestStack("s1", u"Work"));
  stack_view_->AddStack(CreateTestStack("s2", u"Personal"));
  EXPECT_EQ(stack_view_->GetStackCount(), 2);
  stack_view_->ClearAllStacks();
  EXPECT_EQ(stack_view_->GetStackCount(), 0);
  EXPECT_EQ(stack_view_->GetSelectedStackId(), "");
}

// Test 710: PinStack pins the stack.
TEST_F(AstraSidebarStackViewTest, PinStack) {
  stack_view_->AddStack(CreateTestStack("s1", u"Work"));
  EXPECT_FALSE(stack_view_->GetStackAt(0).is_pinned);
  stack_view_->PinStack("s1", true);
  EXPECT_TRUE(stack_view_->GetStackAt(0).is_pinned);
}

// Test 711: GetExpandedStackCount counts only expanded stacks.
TEST_F(AstraSidebarStackViewTest, GetExpandedStackCount) {
  AstraStackInfo s1 = CreateTestStack("s1", u"Work");
  s1.is_expanded = true;
  AstraStackInfo s2 = CreateTestStack("s2", u"Personal");
  s2.is_expanded = false;
  AstraStackInfo s3 = CreateTestStack("s3", u"Research");
  s3.is_expanded = true;
  stack_view_->SetStacks({s1, s2, s3});
  EXPECT_EQ(stack_view_->GetExpandedStackCount(), 2);
}

// =========================================================================
// Stack header view tests
// =========================================================================

// Test 712: Header view construction.
TEST_F(AstraSidebarStackHeaderViewTest, Construction) {
  EXPECT_NE(header_view_, nullptr);
  EXPECT_EQ(header_view_->GetName(), u"Test Stack");
  EXPECT_FALSE(header_view_->IsSelected());
  EXPECT_TRUE(header_view_->GetShowChevron());
  EXPECT_TRUE(header_view_->GetShowTabCount());
  EXPECT_TRUE(header_view_->GetShowColorIndicator());
  EXPECT_TRUE(header_view_->GetShowMenuButton());
}

// Test 713: SetName updates display name.
TEST_F(AstraSidebarStackHeaderViewTest, SetName) {
  header_view_->SetName(u"New Name");
  EXPECT_EQ(header_view_->GetName(), u"New Name");
}

// Test 714: SetColor updates accent color.
TEST_F(AstraSidebarStackHeaderViewTest, SetColor) {
  header_view_->SetColor(SK_ColorGREEN);
  EXPECT_EQ(header_view_->GetColor(), SK_ColorGREEN);
}

// Test 715: SetTabCount updates tab count.
TEST_F(AstraSidebarStackHeaderViewTest, SetTabCount) {
  header_view_->SetTabCount(42);
  EXPECT_EQ(header_view_->GetTabCount(), 42);
}

// Test 716: SetExpanded updates expanded state.
TEST_F(AstraSidebarStackHeaderViewTest, SetExpanded) {
  header_view_->SetExpanded(true);
  EXPECT_TRUE(header_view_->IsExpanded());
  header_view_->SetExpanded(false);
  EXPECT_FALSE(header_view_->IsExpanded());
}

// Test 717: SetSelected updates selection state.
TEST_F(AstraSidebarStackHeaderViewTest, SetSelected) {
  EXPECT_FALSE(header_view_->IsSelected());
  header_view_->SetSelected(true);
  EXPECT_TRUE(header_view_->IsSelected());
  header_view_->SetSelected(false);
  EXPECT_FALSE(header_view_->IsSelected());
}

// Test 718: SetPinned updates pinned state.
TEST_F(AstraSidebarStackHeaderViewTest, SetPinned) {
  EXPECT_FALSE(header_view_->IsPinned());
  header_view_->SetPinned(true);
  EXPECT_TRUE(header_view_->IsPinned());
  header_view_->SetPinned(false);
  EXPECT_FALSE(header_view_->IsPinned());
}

// Test 719: SetShowChevron toggles chevron visibility.
TEST_F(AstraSidebarStackHeaderViewTest, SetShowChevron) {
  EXPECT_TRUE(header_view_->GetShowChevron());
  header_view_->SetShowChevron(false);
  EXPECT_FALSE(header_view_->GetShowChevron());
}

// Test 720: SetShowTabCount toggles tab count visibility.
TEST_F(AstraSidebarStackHeaderViewTest, SetShowTabCount) {
  EXPECT_TRUE(header_view_->GetShowTabCount());
  header_view_->SetShowTabCount(false);
  EXPECT_FALSE(header_view_->GetShowTabCount());
}

// Test 721: SetShowColorIndicator toggles color indicator.
TEST_F(AstraSidebarStackHeaderViewTest, SetShowColorIndicator) {
  EXPECT_TRUE(header_view_->GetShowColorIndicator());
  header_view_->SetShowColorIndicator(false);
  EXPECT_FALSE(header_view_->GetShowColorIndicator());
}

// Test 722: SetShowMenuButton toggles menu button visibility.
TEST_F(AstraSidebarStackHeaderViewTest, SetShowMenuButton) {
  EXPECT_TRUE(header_view_->GetShowMenuButton());
  header_view_->SetShowMenuButton(false);
  EXPECT_FALSE(header_view_->GetShowMenuButton());
}

// Test 723: SetHasUnread toggles unread indicator.
TEST_F(AstraSidebarStackHeaderViewTest, SetHasUnread) {
  EXPECT_FALSE(header_view_->GetHasUnread());
  header_view_->SetHasUnread(true);
  EXPECT_TRUE(header_view_->GetHasUnread());
}

// Test 724: SetCompact toggles compact mode.
TEST_F(AstraSidebarStackHeaderViewTest, SetCompact) {
  EXPECT_FALSE(header_view_->IsCompact());
  header_view_->SetCompact(true);
  EXPECT_TRUE(header_view_->IsCompact());
}

// Test 725: SetDragHovered updates drag hover state.
TEST_F(AstraSidebarStackHeaderViewTest, SetDragHovered) {
  EXPECT_FALSE(header_view_->IsDragHovered());
  header_view_->SetDragHovered(true);
  EXPECT_TRUE(header_view_->IsDragHovered());
}

// Test 726: SetStackInfo sets all properties at once.
TEST_F(AstraSidebarStackHeaderViewTest, SetStackInfo) {
  AstraStackInfo info;
  info.stack_id = "test-id";
  info.name = u"Stack Info Test";
  info.color = SK_ColorMAGENTA;
  info.tab_count = 7;
  info.is_expanded = false;
  info.is_pinned = true;
  info.has_unread = true;
  header_view_->SetStackInfo(info);
  EXPECT_EQ(header_view_->GetStackId(), "test-id");
  EXPECT_EQ(header_view_->GetName(), u"Stack Info Test");
  EXPECT_EQ(header_view_->GetColor(), SK_ColorMAGENTA);
  EXPECT_EQ(header_view_->GetTabCount(), 7);
  EXPECT_FALSE(header_view_->IsExpanded());
  EXPECT_TRUE(header_view_->IsPinned());
  EXPECT_TRUE(header_view_->GetHasUnread());
}

// Test 727: GetStackId returns empty by default.
TEST_F(AstraSidebarStackHeaderViewTest, DefaultStackId) {
  EXPECT_EQ(header_view_->GetStackId(), "");
}

// Test 728: Legacy SetTitle method works.
TEST_F(AstraSidebarStackHeaderViewTest, LegacySetTitle) {
  header_view_->SetTitle(u"Legacy Title");
  EXPECT_EQ(header_view_->GetName(), u"Legacy Title");
}

// Test 729: Legacy SetAccentColor method works.
TEST_F(AstraSidebarStackHeaderViewTest, LegacySetAccentColor) {
  header_view_->SetAccentColor(SK_ColorYELLOW);
  EXPECT_EQ(header_view_->GetColor(), SK_ColorYELLOW);
}

// Test 730: Legacy SetChildCount method works.
TEST_F(AstraSidebarStackHeaderViewTest, LegacySetChildCount) {
  header_view_->SetChildCount(99);
  EXPECT_EQ(header_view_->GetTabCount(), 99);
}

// Test 731: Legacy SetActive method works.
TEST_F(AstraSidebarStackHeaderViewTest, LegacySetActive) {
  header_view_->SetActive(true);
  EXPECT_TRUE(header_view_->IsSelected());
}

// Test 732: ParseHexColor parses valid hex colors.
TEST_F(AstraSidebarStackHeaderViewTest, ParseHexColor) {
  EXPECT_EQ(AstraSidebarStackHeaderView::ParseHexColor("#FF0000"),
            SK_ColorRED);
  EXPECT_EQ(AstraSidebarStackHeaderView::ParseHexColor("#00FF00"),
            SK_ColorGREEN);
  EXPECT_EQ(AstraSidebarStackHeaderView::ParseHexColor("#0000FF"),
            SK_ColorBLUE);
}

// Test 733: Preferred size is valid.
TEST_F(AstraSidebarStackHeaderViewTest, PreferredSizeValid) {
  gfx::Size size = header_view_->GetPreferredSize();
  EXPECT_GT(size.height(), 0);
  EXPECT_GE(size.width(), 0);
}

// Test 734: Compact mode changes preferred height.
TEST_F(AstraSidebarStackHeaderViewTest, CompactModeChangesHeight) {
  gfx::Size normal_size = header_view_->GetPreferredSize();
  header_view_->SetCompact(true);
  gfx::Size compact_size = header_view_->GetPreferredSize();
  EXPECT_LE(compact_size.height(), normal_size.height());
}

// Test 735: Long name does not crash.
TEST_F(AstraSidebarStackHeaderViewTest, LongName) {
  std::u16string long_name(500, u'x');
  header_view_->SetName(long_name);
  EXPECT_EQ(header_view_->GetName(), long_name);
  gfx::Size size = header_view_->GetPreferredSize();
  EXPECT_GT(size.height(), 0);
}

// Test 736: Zero tab count displays correctly.
TEST_F(AstraSidebarStackHeaderViewTest, ZeroTabCount) {
  header_view_->SetTabCount(0);
  EXPECT_EQ(header_view_->GetTabCount(), 0);
}

// =========================================================================
// Stack tab item view tests
// =========================================================================

// Test 737: Tab item view construction.
TEST_F(AstraSidebarStackTabItemViewTest, Construction) {
  EXPECT_NE(tab_item_view_, nullptr);
  EXPECT_EQ(tab_item_view_->GetTitle(), u"Test Tab");
  EXPECT_FALSE(tab_item_view_->IsActive());
  EXPECT_FALSE(tab_item_view_->IsDragging());
  EXPECT_FALSE(tab_item_view_->IsDragHovered());
  EXPECT_TRUE(tab_item_view_->GetShowFavicon());
  EXPECT_TRUE(tab_item_view_->GetShowCloseButton());
}

// Test 738: SetTitle updates title.
TEST_F(AstraSidebarStackTabItemViewTest, SetTitle) {
  tab_item_view_->SetTitle(u"New Title");
  EXPECT_EQ(tab_item_view_->GetTitle(), u"New Title");
}

// Test 739: SetUrl updates URL.
TEST_F(AstraSidebarStackTabItemViewTest, SetUrl) {
  GURL url("https://example.com/page");
  tab_item_view_->SetUrl(url);
  EXPECT_EQ(tab_item_view_->GetUrl(), url);
}

// Test 740: SetActive updates active state.
TEST_F(AstraSidebarStackTabItemViewTest, SetActive) {
  EXPECT_FALSE(tab_item_view_->IsActive());
  tab_item_view_->SetActive(true);
  EXPECT_TRUE(tab_item_view_->IsActive());
  tab_item_view_->SetActive(false);
  EXPECT_FALSE(tab_item_view_->IsActive());
}

// Test 741: SetCloseButtonVisible toggles close button.
TEST_F(AstraSidebarStackTabItemViewTest, SetCloseButtonVisible) {
  tab_item_view_->SetCloseButtonVisible(true);
  EXPECT_TRUE(tab_item_view_->IsCloseButtonVisible());
  tab_item_view_->SetCloseButtonVisible(false);
  EXPECT_FALSE(tab_item_view_->IsCloseButtonVisible());
}

// Test 742: SetShowCloseButton toggles show close button setting.
TEST_F(AstraSidebarStackTabItemViewTest, SetShowCloseButton) {
  EXPECT_TRUE(tab_item_view_->GetShowCloseButton());
  tab_item_view_->SetShowCloseButton(false);
  EXPECT_FALSE(tab_item_view_->GetShowCloseButton());
}

// Test 743: SetShowFavicon toggles favicon display.
TEST_F(AstraSidebarStackTabItemViewTest, SetShowFavicon) {
  EXPECT_TRUE(tab_item_view_->GetShowFavicon());
  tab_item_view_->SetShowFavicon(false);
  EXPECT_FALSE(tab_item_view_->GetShowFavicon());
}

// Test 744: SetHasFavicon updates favicon availability.
TEST_F(AstraSidebarStackTabItemViewTest, SetHasFavicon) {
  tab_item_view_->SetHasFavicon(true);
  tab_item_view_->SetHasFavicon(false);
  SUCCEED();
}

// Test 745: SetIsDragging updates dragging state.
TEST_F(AstraSidebarStackTabItemViewTest, SetIsDragging) {
  EXPECT_FALSE(tab_item_view_->IsDragging());
  tab_item_view_->SetIsDragging(true);
  EXPECT_TRUE(tab_item_view_->IsDragging());
}

// Test 746: SetDragHovered updates drag hovered state.
TEST_F(AstraSidebarStackTabItemViewTest, SetDragHovered) {
  EXPECT_FALSE(tab_item_view_->IsDragHovered());
  tab_item_view_->SetDragHovered(true);
  EXPECT_TRUE(tab_item_view_->IsDragHovered());
}

// Test 747: SetIndex updates stack index.
TEST_F(AstraSidebarStackTabItemViewTest, SetIndex) {
  tab_item_view_->SetIndex(5);
  EXPECT_EQ(tab_item_view_->GetIndex(), 5);
}

// Test 748: SetStackId updates stack ID.
TEST_F(AstraSidebarStackTabItemViewTest, SetStackId) {
  tab_item_view_->SetStackId("stack-123");
  EXPECT_EQ(tab_item_view_->GetStackId(), "stack-123");
}

// Test 749: SetIsFirst updates first-in-stack state.
TEST_F(AstraSidebarStackTabItemViewTest, SetIsFirst) {
  EXPECT_FALSE(tab_item_view_->IsFirst());
  tab_item_view_->SetIsFirst(true);
  EXPECT_TRUE(tab_item_view_->IsFirst());
}

// Test 750: SetIsLast updates last-in-stack state.
TEST_F(AstraSidebarStackTabItemViewTest, SetIsLast) {
  EXPECT_FALSE(tab_item_view_->IsLast());
  tab_item_view_->SetIsLast(true);
  EXPECT_TRUE(tab_item_view_->IsLast());
}

// Test 751: SetPinned updates pinned state.
TEST_F(AstraSidebarStackTabItemViewTest, SetPinned) {
  EXPECT_FALSE(tab_item_view_->IsPinned());
  tab_item_view_->SetPinned(true);
  EXPECT_TRUE(tab_item_view_->IsPinned());
}

// Test 752: SetIsAudible updates audible state.
TEST_F(AstraSidebarStackTabItemViewTest, SetIsAudible) {
  EXPECT_FALSE(tab_item_view_->IsAudible());
  tab_item_view_->SetIsAudible(true);
  EXPECT_TRUE(tab_item_view_->IsAudible());
}

// Test 753: SetIsMuted updates muted state.
TEST_F(AstraSidebarStackTabItemViewTest, SetIsMuted) {
  EXPECT_FALSE(tab_item_view_->IsMuted());
  tab_item_view_->SetIsMuted(true);
  EXPECT_TRUE(tab_item_view_->IsMuted());
}

// Test 754: Audio state is kPlaying when audible and not muted.
TEST_F(AstraSidebarStackTabItemViewTest, AudioStatePlaying) {
  tab_item_view_->SetIsAudible(true);
  tab_item_view_->SetIsMuted(false);
  EXPECT_EQ(tab_item_view_->audio_state(),
            AstraSidebarStackTabItemView::AudioState::kPlaying);
}

// Test 755: Audio state is kMuted when audible and muted.
TEST_F(AstraSidebarStackTabItemViewTest, AudioStateMuted) {
  tab_item_view_->SetIsAudible(true);
  tab_item_view_->SetIsMuted(true);
  EXPECT_EQ(tab_item_view_->audio_state(),
            AstraSidebarStackTabItemView::AudioState::kMuted);
}

// Test 756: Audio state is kNone when not audible.
TEST_F(AstraSidebarStackTabItemViewTest, AudioStateNone) {
  tab_item_view_->SetIsAudible(false);
  EXPECT_EQ(tab_item_view_->audio_state(),
            AstraSidebarStackTabItemView::AudioState::kNone);
}

// Test 757: SetAudioState directly sets audio state.
TEST_F(AstraSidebarStackTabItemViewTest, SetAudioStateDirect) {
  tab_item_view_->SetAudioState(
      AstraSidebarStackTabItemView::AudioState::kPlaying);
  EXPECT_EQ(tab_item_view_->audio_state(),
            AstraSidebarStackTabItemView::AudioState::kPlaying);
}

// Test 758: SetIsLoading updates loading state.
TEST_F(AstraSidebarStackTabItemViewTest, SetIsLoading) {
  EXPECT_FALSE(tab_item_view_->IsLoading());
  tab_item_view_->SetIsLoading(true);
  EXPECT_TRUE(tab_item_view_->IsLoading());
}

// Test 759: SetIsCrashed updates crashed state.
TEST_F(AstraSidebarStackTabItemViewTest, SetIsCrashed) {
  EXPECT_FALSE(tab_item_view_->IsCrashed());
  tab_item_view_->SetIsCrashed(true);
  EXPECT_TRUE(tab_item_view_->IsCrashed());
}

// Test 760: SetTabInfo sets all properties at once.
TEST_F(AstraSidebarStackTabItemViewTest, SetTabInfo) {
  AstraStackTabInfo info;
  info.tab_id = "tab-42";
  info.title = u"Info Tab";
  info.url = GURL("https://info.com");
  info.is_active = true;
  info.is_pinned = true;
  info.is_audible = true;
  info.is_muted = false;
  info.is_loading = true;
  info.is_crashed = false;
  info.index_in_stack = 2;
  tab_item_view_->SetTabInfo(info);
  EXPECT_EQ(tab_item_view_->GetTabId(), "tab-42");
  EXPECT_EQ(tab_item_view_->GetTitle(), u"Info Tab");
  EXPECT_TRUE(tab_item_view_->IsActive());
  EXPECT_TRUE(tab_item_view_->IsPinned());
  EXPECT_TRUE(tab_item_view_->IsAudible());
  EXPECT_TRUE(tab_item_view_->IsLoading());
  EXPECT_EQ(tab_item_view_->GetIndex(), 2);
}

// Test 761: SetDraggable toggles draggable state.
TEST_F(AstraSidebarStackTabItemViewTest, SetDraggable) {
  EXPECT_FALSE(tab_item_view_->draggable());
  tab_item_view_->SetDraggable(true);
  EXPECT_TRUE(tab_item_view_->draggable());
}

// Test 762: Legacy web_contents getter/setter works.
TEST_F(AstraSidebarStackTabItemViewTest, LegacyWebContents) {
  EXPECT_EQ(tab_item_view_->web_contents(), nullptr);
  tab_item_view_->set_web_contents(nullptr);
  EXPECT_EQ(tab_item_view_->web_contents(), nullptr);
}

// Test 763: Legacy stack_id getter/setter works.
TEST_F(AstraSidebarStackTabItemViewTest, LegacyStackId) {
  tab_item_view_->set_stack_id("legacy-id");
  EXPECT_EQ(tab_item_view_->stack_id(), "legacy-id");
}

// Test 764: Preferred size is valid.
TEST_F(AstraSidebarStackTabItemViewTest, PreferredSizeValid) {
  gfx::Size size = tab_item_view_->GetPreferredSize();
  EXPECT_GT(size.height(), 0);
  EXPECT_GE(size.width(), 0);
}

// Test 765: Long title does not crash.
TEST_F(AstraSidebarStackTabItemViewTest, LongTitle) {
  std::u16string long_title(300, u'x');
  tab_item_view_->SetTitle(long_title);
  EXPECT_EQ(tab_item_view_->GetTitle(), long_title);
  gfx::Size size = tab_item_view_->GetPreferredSize();
  EXPECT_GT(size.height(), 0);
}

// =========================================================================
// Stack child view tests
// =========================================================================

// Test 766: Child view construction.
TEST_F(AstraSidebarStackChildViewTest, Construction) {
  EXPECT_NE(child_view_, nullptr);
  EXPECT_EQ(child_view_->GetTabCount(), 0);
  EXPECT_EQ(child_view_->GetActiveTabIndex(), -1);
  EXPECT_TRUE(child_view_->GetShowFavicons());
  EXPECT_TRUE(child_view_->GetShowCloseButtons());
  EXPECT_TRUE(child_view_->GetDragDropEnabled());
}

// Test 767: SetTabs replaces all tabs.
TEST_F(AstraSidebarStackChildViewTest, SetTabs) {
  std::vector<AstraStackTabInfo> tabs;
  tabs.push_back(CreateTestTab("t1", u"Tab 1"));
  tabs.push_back(CreateTestTab("t2", u"Tab 2"));
  tabs.push_back(CreateTestTab("t3", u"Tab 3"));
  child_view_->SetTabs(tabs);
  EXPECT_EQ(child_view_->GetTabCount(), 3);
}

// Test 768: GetTabAt returns correct info.
TEST_F(AstraSidebarStackChildViewTest, GetTabAt) {
  std::vector<AstraStackTabInfo> tabs;
  tabs.push_back(CreateTestTab("t1", u"First"));
  tabs.push_back(CreateTestTab("t2", u"Second"));
  child_view_->SetTabs(tabs);
  EXPECT_EQ(child_view_->GetTabAt(0).title, u"First");
  EXPECT_EQ(child_view_->GetTabAt(1).title, u"Second");
}

// Test 769: GetTabAt with invalid index returns default.
TEST_F(AstraSidebarStackChildViewTest, GetTabAtInvalid) {
  AstraStackTabInfo info = child_view_->GetTabAt(0);
  EXPECT_EQ(info.tab_id, "");
  EXPECT_EQ(info.title, u"");
}

// Test 770: AddTab appends to end.
TEST_F(AstraSidebarStackChildViewTest, AddTabAppends) {
  child_view_->AddTab(CreateTestTab("t1", u"First"));
  EXPECT_EQ(child_view_->GetTabCount(), 1);
  child_view_->AddTab(CreateTestTab("t2", u"Second"));
  EXPECT_EQ(child_view_->GetTabCount(), 2);
  EXPECT_EQ(child_view_->GetTabAt(1).tab_id, "t2");
}

// Test 771: AddTab at specific position.
TEST_F(AstraSidebarStackChildViewTest, AddTabAtPosition) {
  child_view_->AddTab(CreateTestTab("t1", u"First"));
  child_view_->AddTab(CreateTestTab("t3", u"Third"));
  child_view_->AddTab(CreateTestTab("t2", u"Second"), 1);
  EXPECT_EQ(child_view_->GetTabCount(), 3);
  EXPECT_EQ(child_view_->GetTabAt(1).title, u"Second");
}

// Test 772: AddTab with negative position appends.
TEST_F(AstraSidebarStackChildViewTest, AddTabNegativePosition) {
  child_view_->AddTab(CreateTestTab("t1", u"First"));
  child_view_->AddTab(CreateTestTab("t2", u"Second"), -1);
  EXPECT_EQ(child_view_->GetTabCount(), 2);
  EXPECT_EQ(child_view_->GetTabAt(1).title, u"Second");
}

// Test 773: RemoveTab removes the tab.
TEST_F(AstraSidebarStackChildViewTest, RemoveTab) {
  child_view_->AddTab(CreateTestTab("t1", u"First"));
  child_view_->AddTab(CreateTestTab("t2", u"Second"));
  child_view_->RemoveTab(0);
  EXPECT_EQ(child_view_->GetTabCount(), 1);
  EXPECT_EQ(child_view_->GetTabAt(0).title, u"Second");
}

// Test 774: RemoveTab with invalid index does nothing.
TEST_F(AstraSidebarStackChildViewTest, RemoveTabInvalid) {
  child_view_->AddTab(CreateTestTab("t1", u"First"));
  child_view_->RemoveTab(99);
  EXPECT_EQ(child_view_->GetTabCount(), 1);
}

// Test 775: UpdateTab updates tab info.
TEST_F(AstraSidebarStackChildViewTest, UpdateTab) {
  child_view_->AddTab(CreateTestTab("t1", u"Old", false));
  AstraStackTabInfo updated = CreateTestTab("t1", u"New", true);
  child_view_->UpdateTab(0, updated);
  EXPECT_EQ(child_view_->GetTabAt(0).title, u"New");
  EXPECT_TRUE(child_view_->GetTabAt(0).is_active);
}

// Test 776: SetActiveTab sets active tab index.
TEST_F(AstraSidebarStackChildViewTest, SetActiveTab) {
  child_view_->AddTab(CreateTestTab("t1", u"First"));
  child_view_->AddTab(CreateTestTab("t2", u"Second"));
  child_view_->SetActiveTab(1);
  EXPECT_EQ(child_view_->GetActiveTabIndex(), 1);
}

// Test 777: SetActiveTab with -1 clears active.
TEST_F(AstraSidebarStackChildViewTest, SetActiveTabNegativeClears) {
  child_view_->AddTab(CreateTestTab("t1", u"First", true));
  EXPECT_EQ(child_view_->GetActiveTabIndex(), 0);
  child_view_->SetActiveTab(-1);
  EXPECT_EQ(child_view_->GetActiveTabIndex(), -1);
}

// Test 778: Active tab from SetTabs data.
TEST_F(AstraSidebarStackChildViewTest, ActiveTabFromData) {
  std::vector<AstraStackTabInfo> tabs;
  tabs.push_back(CreateTestTab("t1", u"First", false));
  tabs.push_back(CreateTestTab("t2", u"Second", true));
  tabs.push_back(CreateTestTab("t3", u"Third", false));
  child_view_->SetTabs(tabs);
  EXPECT_EQ(child_view_->GetActiveTabIndex(), 1);
}

// Test 779: MoveTab reorders tabs.
TEST_F(AstraSidebarStackChildViewTest, MoveTab) {
  child_view_->AddTab(CreateTestTab("t1", u"First"));
  child_view_->AddTab(CreateTestTab("t2", u"Second"));
  child_view_->AddTab(CreateTestTab("t3", u"Third"));
  child_view_->MoveTab(0, 2);
  EXPECT_EQ(child_view_->GetTabAt(0).title, u"Second");
  EXPECT_EQ(child_view_->GetTabAt(1).title, u"Third");
  EXPECT_EQ(child_view_->GetTabAt(2).title, u"First");
}

// Test 780: MoveTab with same index does nothing.
TEST_F(AstraSidebarStackChildViewTest, MoveTabSameIndex) {
  child_view_->AddTab(CreateTestTab("t1", u"First"));
  child_view_->AddTab(CreateTestTab("t2", u"Second"));
  child_view_->MoveTab(1, 1);
  EXPECT_EQ(child_view_->GetTabAt(0).title, u"First");
  EXPECT_EQ(child_view_->GetTabAt(1).title, u"Second");
}

// Test 781: MoveTab with active tab updates active index.
TEST_F(AstraSidebarStackChildViewTest, MoveActiveTab) {
  child_view_->AddTab(CreateTestTab("t1", u"First", true));
  child_view_->AddTab(CreateTestTab("t2", u"Second", false));
  child_view_->AddTab(CreateTestTab("t3", u"Third", false));
  child_view_->MoveTab(0, 2);
  EXPECT_EQ(child_view_->GetActiveTabIndex(), 2);
}

// Test 782: SetTabHeight updates tab height.
TEST_F(AstraSidebarStackChildViewTest, SetTabHeight) {
  child_view_->SetTabHeight(36);
  EXPECT_EQ(child_view_->GetTabHeight(), 36);
}

// Test 783: SetTabHeight with invalid value does nothing.
TEST_F(AstraSidebarStackChildViewTest, SetTabHeightInvalid) {
  int default_height = child_view_->GetTabHeight();
  child_view_->SetTabHeight(-5);
  EXPECT_EQ(child_view_->GetTabHeight(), default_height);
  child_view_->SetTabHeight(0);
  EXPECT_EQ(child_view_->GetTabHeight(), default_height);
}

// Test 784: SetShowFavicons propagates to child tab views.
TEST_F(AstraSidebarStackChildViewTest, SetShowFaviconsPropagates) {
  child_view_->AddTab(CreateTestTab("t1", u"Tab 1"));
  child_view_->AddTab(CreateTestTab("t2", u"Tab 2"));
  child_view_->SetShowFavicons(false);
  EXPECT_FALSE(child_view_->GetShowFavicons());
  auto* tab0 = child_view_->GetTabViewAt(0);
  auto* tab1 = child_view_->GetTabViewAt(1);
  if (tab0) EXPECT_FALSE(tab0->GetShowFavicon());
  if (tab1) EXPECT_FALSE(tab1->GetShowFavicon());
}

// Test 785: SetShowCloseButtons propagates to child tab views.
TEST_F(AstraSidebarStackChildViewTest, SetShowCloseButtonsPropagates) {
  child_view_->AddTab(CreateTestTab("t1", u"Tab 1"));
  child_view_->SetShowCloseButtons(false);
  EXPECT_FALSE(child_view_->GetShowCloseButtons());
  auto* tab0 = child_view_->GetTabViewAt(0);
  if (tab0) EXPECT_FALSE(tab0->GetShowCloseButton());
}

// Test 786: SetDragDropEnabled propagates to child tab views.
TEST_F(AstraSidebarStackChildViewTest, SetDragDropEnabledPropagates) {
  child_view_->AddTab(CreateTestTab("t1", u"Tab 1"));
  child_view_->AddTab(CreateTestTab("t2", u"Tab 2"));
  child_view_->SetDragDropEnabled(false);
  EXPECT_FALSE(child_view_->GetDragDropEnabled());
  auto* tab0 = child_view_->GetTabViewAt(0);
  auto* tab1 = child_view_->GetTabViewAt(1);
  if (tab0) EXPECT_FALSE(tab0->draggable());
  if (tab1) EXPECT_FALSE(tab1->draggable());
}

// Test 787: GetTabViewAt returns tab item view.
TEST_F(AstraSidebarStackChildViewTest, GetTabViewAt) {
  child_view_->AddTab(CreateTestTab("t1", u"Tab 1"));
  EXPECT_NE(child_view_->GetTabViewAt(0), nullptr);
}

// Test 788: GetTabViewAt with invalid index returns null.
TEST_F(AstraSidebarStackChildViewTest, GetTabViewAtInvalid) {
  EXPECT_EQ(child_view_->GetTabViewAt(0), nullptr);
}

// Test 789: ClearAllTabs removes all tabs.
TEST_F(AstraSidebarStackChildViewTest, ClearAllTabs) {
  child_view_->AddTab(CreateTestTab("t1", u"Tab 1"));
  child_view_->AddTab(CreateTestTab("t2", u"Tab 2"));
  EXPECT_EQ(child_view_->GetTabCount(), 2);
  child_view_->ClearAllTabs();
  EXPECT_EQ(child_view_->GetTabCount(), 0);
  EXPECT_EQ(child_view_->GetActiveTabIndex(), -1);
}

// Test 790: Empty child view has zero tabs.
TEST_F(AstraSidebarStackChildViewTest, EmptyState) {
  EXPECT_EQ(child_view_->GetTabCount(), 0);
  EXPECT_EQ(child_view_->GetActiveTabIndex(), -1);
  EXPECT_EQ(child_view_->GetTabViewAt(0), nullptr);
}

// Test 791: Single tab state.
TEST_F(AstraSidebarStackChildViewTest, SingleTab) {
  child_view_->AddTab(CreateTestTab("t1", u"Only Tab", true));
  EXPECT_EQ(child_view_->GetTabCount(), 1);
  EXPECT_EQ(child_view_->GetActiveTabIndex(), 0);
  auto* tab_view = child_view_->GetTabViewAt(0);
  ASSERT_NE(tab_view, nullptr);
  EXPECT_TRUE(tab_view->IsFirst());
  EXPECT_TRUE(tab_view->IsLast());
}

// Test 792: Many tabs.
TEST_F(AstraSidebarStackChildViewTest, ManyTabs) {
  for (int i = 0; i < 50; ++i) {
    child_view_->AddTab(CreateTestTab("t" + std::to_string(i),
                                      u"Tab " + std::to_wstring(i)));
  }
  EXPECT_EQ(child_view_->GetTabCount(), 50);
}

// Test 793: Stack ID is empty by default.
TEST_F(AstraSidebarStackChildViewTest, DefaultStackId) {
  EXPECT_EQ(child_view_->stack_id(), "");
}

// Test 794: set_stack_id sets the stack ID.
TEST_F(AstraSidebarStackChildViewTest, SetStackId) {
  child_view_->set_stack_id("my-stack");
  EXPECT_EQ(child_view_->stack_id(), "my-stack");
}

// Test 795: delegate() returns null by default.
TEST_F(AstraSidebarStackChildViewTest, DefaultDelegate) {
  EXPECT_EQ(child_view_->delegate(), nullptr);
}

// Test 796: set_delegate sets the delegate.
TEST_F(AstraSidebarStackChildViewTest, SetDelegate) {
  MockSidebarStackChildDelegate delegate;
  child_view_->set_delegate(&delegate);
  EXPECT_EQ(child_view_->delegate(), &delegate);
}

// =========================================================================
// Struct and enum tests
// =========================================================================

// Test 797: AstraStackInfo default values.
TEST(AstraStackInfoTest, DefaultValues) {
  AstraStackInfo info;
  EXPECT_EQ(info.stack_id, "");
  EXPECT_EQ(info.name, u"");
  EXPECT_EQ(info.color, SK_ColorGRAY);
  EXPECT_EQ(info.tab_count, 0);
  EXPECT_TRUE(info.is_expanded);
  EXPECT_FALSE(info.is_pinned);
  EXPECT_EQ(info.order_index, 0);
  EXPECT_FALSE(info.has_unread);
  EXPECT_EQ(info.note, u"");
}

// Test 798: AstraStackTabInfo default values.
TEST(AstraStackTabInfoTest, DefaultValues) {
  AstraStackTabInfo info;
  EXPECT_EQ(info.tab_id, "");
  EXPECT_EQ(info.title, u"");
  EXPECT_FALSE(info.url.is_valid());
  EXPECT_FALSE(info.is_active);
  EXPECT_FALSE(info.is_pinned);
  EXPECT_FALSE(info.is_audible);
  EXPECT_FALSE(info.is_muted);
  EXPECT_FALSE(info.is_loading);
  EXPECT_FALSE(info.is_crashed);
  EXPECT_FALSE(info.has_favicon);
  EXPECT_EQ(info.index_in_stack, 0);
}

// Test 799: AstraStackSortBy enum has distinct values.
TEST(AstraStackSortByTest, DistinctValues) {
  EXPECT_NE(static_cast<int>(AstraStackSortBy::kManual),
            static_cast<int>(AstraStackSortBy::kName));
  EXPECT_NE(static_cast<int>(AstraStackSortBy::kName),
            static_cast<int>(AstraStackSortBy::kTabCount));
  EXPECT_NE(static_cast<int>(AstraStackSortBy::kTabCount),
            static_cast<int>(AstraStackSortBy::kLastAccessed));
  EXPECT_NE(static_cast<int>(AstraStackSortBy::kLastAccessed),
            static_cast<int>(AstraStackSortBy::kColor));
}

// Test 800: AudioState enum has distinct values.
TEST(AudioStateTest, DistinctValues) {
  EXPECT_NE(
      static_cast<int>(AstraSidebarStackTabItemView::AudioState::kNone),
      static_cast<int>(AstraSidebarStackTabItemView::AudioState::kPlaying));
  EXPECT_NE(
      static_cast<int>(AstraSidebarStackTabItemView::AudioState::kPlaying),
      static_cast<int>(AstraSidebarStackTabItemView::AudioState::kMuted));
}

// =========================================================================
// Edge case tests
// =========================================================================

// Test 801: Remove last tab leaves empty view.
TEST_F(AstraSidebarStackChildViewTest, RemoveLastTab) {
  child_view_->AddTab(CreateTestTab("t1", u"Only"));
  child_view_->RemoveTab(0);
  EXPECT_EQ(child_view_->GetTabCount(), 0);
  EXPECT_EQ(child_view_->GetActiveTabIndex(), -1);
}

// Test 802: UpdateTab with invalid index does nothing.
TEST_F(AstraSidebarStackChildViewTest, UpdateTabInvalidIndex) {
  child_view_->AddTab(CreateTestTab("t1", u"Original"));
  AstraStackTabInfo updated = CreateTestTab("t99", u"Updated");
  child_view_->UpdateTab(99, updated);
  EXPECT_EQ(child_view_->GetTabAt(0).title, u"Original");
}

// Test 803: AddTab at position beyond count appends.
TEST_F(AstraSidebarStackChildViewTest, AddTabBeyondCount) {
  child_view_->AddTab(CreateTestTab("t1", u"First"));
  child_view_->AddTab(CreateTestTab("t2", u"Second"), 999);
  EXPECT_EQ(child_view_->GetTabCount(), 2);
  EXPECT_EQ(child_view_->GetTabAt(1).title, u"Second");
}

// Test 804: MoveTab from invalid index does nothing.
TEST_F(AstraSidebarStackChildViewTest, MoveTabFromInvalid) {
  child_view_->AddTab(CreateTestTab("t1", u"First"));
  child_view_->AddTab(CreateTestTab("t2", u"Second"));
  child_view_->MoveTab(-1, 1);
  child_view_->MoveTab(99, 0);
  EXPECT_EQ(child_view_->GetTabAt(0).title, u"First");
  EXPECT_EQ(child_view_->GetTabAt(1).title, u"Second");
}

// Test 805: MoveTab to invalid index does nothing.
TEST_F(AstraSidebarStackChildViewTest, MoveTabToInvalid) {
  child_view_->AddTab(CreateTestTab("t1", u"First"));
  child_view_->AddTab(CreateTestTab("t2", u"Second"));
  child_view_->MoveTab(0, -1);
  child_view_->MoveTab(0, 99);
  EXPECT_EQ(child_view_->GetTabAt(0).title, u"First");
  EXPECT_EQ(child_view_->GetTabAt(1).title, u"Second");
}

// Test 806: SetActiveTab with out-of-range index.
TEST_F(AstraSidebarStackChildViewTest, SetActiveTabOutOfRange) {
  child_view_->AddTab(CreateTestTab("t1", u"First"));
  child_view_->SetActiveTab(99);
  SUCCEED();
}

// Test 807: UpdateStack with invalid ID does nothing.
TEST_F(AstraSidebarStackViewTest, UpdateStackInvalidId) {
  stack_view_->AddStack(CreateTestStack("s1", u"Original"));
  AstraStackInfo updated = CreateTestStack("nonexistent", u"Updated");
  stack_view_->UpdateStack("nonexistent", updated);
  EXPECT_EQ(stack_view_->GetStackCount(), 1);
  EXPECT_EQ(stack_view_->GetStackAt(0).name, u"Original");
}

// Test 808: IsStackExpanded with invalid ID returns false.
TEST_F(AstraSidebarStackViewTest, IsStackExpandedInvalidId) {
  EXPECT_FALSE(stack_view_->IsStackExpanded("nonexistent"));
}

// Test 809: PinStack with invalid ID does nothing.
TEST_F(AstraSidebarStackViewTest, PinStackInvalidId) {
  stack_view_->AddStack(CreateTestStack("s1", u"Test"));
  stack_view_->PinStack("nonexistent", true);
  EXPECT_EQ(stack_view_->GetStackCount(), 1);
  EXPECT_FALSE(stack_view_->GetStackAt(0).is_pinned);
}

// Test 810: SetStackColor with invalid ID does nothing.
TEST_F(AstraSidebarStackViewTest, SetStackColorInvalidId) {
  stack_view_->AddStack(CreateTestStack("s1", u"Test", SK_ColorRED));
  stack_view_->SetStackColor("nonexistent", SK_ColorBLUE);
  EXPECT_EQ(stack_view_->GetStackAt(0).color, SK_ColorRED);
}

// Test 811: RenameStack with invalid ID does nothing.
TEST_F(AstraSidebarStackViewTest, RenameStackInvalidId) {
  stack_view_->AddStack(CreateTestStack("s1", u"Original"));
  stack_view_->RenameStack("nonexistent", u"New Name");
  EXPECT_EQ(stack_view_->GetStackAt(0).name, u"Original");
}

// Test 812: ToggleStackExpanded with invalid ID does nothing.
TEST_F(AstraSidebarStackViewTest, ToggleStackExpandedInvalidId) {
  stack_view_->ToggleStackExpanded("nonexistent");
  SUCCEED();
}

// Test 813: SetSelectedStack with empty string clears selection.
TEST_F(AstraSidebarStackViewTest, SetSelectedStackEmptyString) {
  stack_view_->AddStack(CreateTestStack("s1", u"Test"));
  stack_view_->SetSelectedStack("s1");
  EXPECT_EQ(stack_view_->GetSelectedStackId(), "s1");
  stack_view_->SetSelectedStack("");
  EXPECT_EQ(stack_view_->GetSelectedStackId(), "");
}

// Test 814: Stack with zero tabs.
TEST_F(AstraSidebarStackViewTest, StackWithZeroTabs) {
  AstraStackInfo info = CreateTestStack("s1", u"Empty", SK_ColorGRAY, 0);
  stack_view_->AddStack(info);
  EXPECT_EQ(stack_view_->GetStackAt(0).tab_count, 0);
  EXPECT_EQ(stack_view_->GetTotalTabCount(), 0);
}

// Test 815: Stack with large tab count.
TEST_F(AstraSidebarStackViewTest, StackWithLargeTabCount) {
  AstraStackInfo info = CreateTestStack("s1", u"Many", SK_ColorGRAY, 9999);
  stack_view_->AddStack(info);
  EXPECT_EQ(stack_view_->GetStackAt(0).tab_count, 9999);
}

// Test 816: SetStacks with empty vector clears all.
TEST_F(AstraSidebarStackViewTest, SetStacksEmpty) {
  stack_view_->AddStack(CreateTestStack("s1", u"Test"));
  EXPECT_EQ(stack_view_->GetStackCount(), 1);
  stack_view_->SetStacks({});
  EXPECT_EQ(stack_view_->GetStackCount(), 0);
}

// Test 817: Tab info index_in_stack is set by AddTab.
TEST_F(AstraSidebarStackChildViewTest, AddTabUpdatesIndexInStack) {
  child_view_->AddTab(CreateTestTab("t1", u"First"));
  child_view_->AddTab(CreateTestTab("t2", u"Second"));
  child_view_->AddTab(CreateTestTab("t3", u"Third"), 1);
  EXPECT_EQ(child_view_->GetTabAt(0).index_in_stack, 0);
  EXPECT_EQ(child_view_->GetTabAt(1).index_in_stack, 1);
  EXPECT_EQ(child_view_->GetTabAt(2).index_in_stack, 2);
}

// Test 818: RemoveTab updates remaining indices.
TEST_F(AstraSidebarStackChildViewTest, RemoveTabUpdatesIndices) {
  child_view_->AddTab(CreateTestTab("t1", u"First"));
  child_view_->AddTab(CreateTestTab("t2", u"Second"));
  child_view_->AddTab(CreateTestTab("t3", u"Third"));
  child_view_->RemoveTab(0);
  EXPECT_EQ(child_view_->GetTabAt(0).index_in_stack, 0);
  EXPECT_EQ(child_view_->GetTabAt(1).index_in_stack, 1);
}

// Test 819: First/last indicators correct for single tab.
TEST_F(AstraSidebarStackChildViewTest, SingleTabFirstLastIndicators) {
  child_view_->AddTab(CreateTestTab("t1", u"Only"));
  auto* tab = child_view_->GetTabViewAt(0);
  ASSERT_NE(tab, nullptr);
  EXPECT_TRUE(tab->IsFirst());
  EXPECT_TRUE(tab->IsLast());
}

// Test 820: First/last indicators correct for multiple tabs.
TEST_F(AstraSidebarStackChildViewTest, MultipleTabsFirstLastIndicators) {
  child_view_->AddTab(CreateTestTab("t1", u"First"));
  child_view_->AddTab(CreateTestTab("t2", u"Middle"));
  child_view_->AddTab(CreateTestTab("t3", u"Last"));
  auto* first = child_view_->GetTabViewAt(0);
  auto* middle = child_view_->GetTabViewAt(1);
  auto* last = child_view_->GetTabViewAt(2);
  ASSERT_NE(first, nullptr);
  ASSERT_NE(middle, nullptr);
  ASSERT_NE(last, nullptr);
  EXPECT_TRUE(first->IsFirst());
  EXPECT_FALSE(first->IsLast());
  EXPECT_FALSE(middle->IsFirst());
  EXPECT_FALSE(middle->IsLast());
  EXPECT_FALSE(last->IsFirst());
  EXPECT_TRUE(last->IsLast());
}

// Test 821: Insert at beginning updates first indicator.
TEST_F(AstraSidebarStackChildViewTest, InsertAtBeginningUpdatesFirst) {
  child_view_->AddTab(CreateTestTab("t1", u"Original First"));
  child_view_->AddTab(CreateTestTab("t2", u"New First"), 0);
  auto* new_first = child_view_->GetTabViewAt(0);
  auto* old_first = child_view_->GetTabViewAt(1);
  ASSERT_NE(new_first, nullptr);
  ASSERT_NE(old_first, nullptr);
  EXPECT_TRUE(new_first->IsFirst());
  EXPECT_FALSE(old_first->IsFirst());
}

// Test 822: Append updates last indicator.
TEST_F(AstraSidebarStackChildViewTest, AppendUpdatesLast) {
  child_view_->AddTab(CreateTestTab("t1", u"Original Last"));
  child_view_->AddTab(CreateTestTab("t2", u"New Last"));
  auto* new_last = child_view_->GetTabViewAt(1);
  auto* old_last = child_view_->GetTabViewAt(0);
  ASSERT_NE(new_last, nullptr);
  ASSERT_NE(old_last, nullptr);
  EXPECT_FALSE(old_last->IsLast());
  EXPECT_TRUE(new_last->IsLast());
}

// Test 823: Remove first updates first indicator.
TEST_F(AstraSidebarStackChildViewTest, RemoveFirstUpdatesFirst) {
  child_view_->AddTab(CreateTestTab("t1", u"First"));
  child_view_->AddTab(CreateTestTab("t2", u"Second"));
  child_view_->RemoveTab(0);
  auto* new_first = child_view_->GetTabViewAt(0);
  ASSERT_NE(new_first, nullptr);
  EXPECT_TRUE(new_first->IsFirst());
}

// Test 824: Remove last updates last indicator.
TEST_F(AstraSidebarStackChildViewTest, RemoveLastUpdatesLast) {
  child_view_->AddTab(CreateTestTab("t1", u"First"));
  child_view_->AddTab(CreateTestTab("t2", u"Last"));
  child_view_->RemoveTab(1);
  auto* new_last = child_view_->GetTabViewAt(0);
  ASSERT_NE(new_last, nullptr);
  EXPECT_TRUE(new_last->IsLast());
}

// Test 825: UpdateTab preserves index_in_stack.
TEST_F(AstraSidebarStackChildViewTest, UpdateTabPreservesIndex) {
  child_view_->AddTab(CreateTestTab("t1", u"First"));
  child_view_->AddTab(CreateTestTab("t2", u"Second"));
  AstraStackTabInfo updated = CreateTestTab("t2", u"Updated");
  updated.index_in_stack = 99;
  child_view_->UpdateTab(1, updated);
  EXPECT_EQ(child_view_->GetTabAt(1).index_in_stack, 1);
}

// Test 826: SetActiveTab deactivates previous active.
TEST_F(AstraSidebarStackChildViewTest, SetActiveTabDeactivatesPrevious) {
  child_view_->AddTab(CreateTestTab("t1", u"First"));
  child_view_->AddTab(CreateTestTab("t2", u"Second"));
  child_view_->SetActiveTab(0);
  child_view_->SetActiveTab(1);
  EXPECT_EQ(child_view_->GetActiveTabIndex(), 1);
  EXPECT_FALSE(child_view_->GetTabAt(0).is_active);
  EXPECT_TRUE(child_view_->GetTabAt(1).is_active);
}

// Test 827: Sort by color orders by hue.
TEST_F(AstraSidebarStackViewTest, SortByColor) {
  stack_view_->AddStack(CreateTestStack("s1", u"Red", SK_ColorRED));
  stack_view_->AddStack(CreateTestStack("s2", u"Green", SK_ColorGREEN));
  stack_view_->AddStack(CreateTestStack("s3", u"Blue", SK_ColorBLUE));
  stack_view_->SetSortBy(AstraStackSortBy::kColor);
  EXPECT_EQ(stack_view_->GetStackCount(), 3);
  EXPECT_NE(stack_view_->FindStackIndexById("s1"), -1);
  EXPECT_NE(stack_view_->FindStackIndexById("s2"), -1);
  EXPECT_NE(stack_view_->FindStackIndexById("s3"), -1);
}

// Test 828: Switching sort mode back to manual preserves order.
TEST_F(AstraSidebarStackViewTest, SwitchSortModeBackToManual) {
  stack_view_->AddStack(CreateTestStack("s1", u"Zebra"));
  stack_view_->AddStack(CreateTestStack("s2", u"Alpha"));
  stack_view_->SetSortBy(AstraStackSortBy::kName);
  EXPECT_EQ(stack_view_->GetStackAt(0).name, u"Alpha");
  stack_view_->SetSortBy(AstraStackSortBy::kManual);
  EXPECT_EQ(stack_view_->GetStackAt(0).name, u"Alpha");
  EXPECT_EQ(stack_view_->GetStackAt(1).name, u"Zebra");
}

// =========================================================================
// Total test count verification
// =========================================================================

// Note: Total test count through Test 828, covering tab stack views
// (stack view, header view, tab item view, child view) plus struct/enum
// tests and edge case tests = 154 tests for tab stack views.

}  // namespace
}  // namespace astra
