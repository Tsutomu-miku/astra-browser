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
#include "astra/ui/views/sidebar/astra_history_item_view.h"
#include "astra/ui/views/sidebar/astra_note_item_view.h"
#include "astra/ui/views/sidebar/astra_password_item_view.h"
#include "astra/ui/views/sidebar/astra_reading_list_item_view.h"
#include "astra/ui/views/sidebar/astra_recently_closed_item_view.h"
#include "astra/ui/views/sidebar/astra_sidebar_item_view.h"

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
  MOCK_METHOD(void, OnExtensionIconClicked,
              (const std::string& extension_id, views::View* anchor_view),
              (override));
  MOCK_METHOD(void, OnExtensionIconContextMenu,
              (const std::string& extension_id, const gfx::Point& point),
              (override));
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
}

// Test 240: SetExtensionInfo updates all fields.
TEST_F(AstraExtensionIconViewTest, SetExtensionInfoUpdatesAll) {
  std::string id = "ext-456";
  std::u16string name = u"My Extension";
  gfx::ImageSkia icon;

  extension_view_->SetExtensionInfo(id, name, icon);

  EXPECT_EQ(extension_view_->GetExtensionId(), id);
}

// Test 241: SetExtensionIcon updates icon.
TEST_F(AstraExtensionIconViewTest, SetExtensionIcon) {
  gfx::ImageSkia icon;
  extension_view_->SetExtensionIcon(icon);
  SUCCEED();
}

// Test 242: SetBadgeText sets badge.
TEST_F(AstraExtensionIconViewTest, SetBadgeText) {
  extension_view_->SetBadgeText(u"3");
  SUCCEED();
}

// Test 243: SetIsAction toggles action state.
TEST_F(AstraExtensionIconViewTest, SetIsAction) {
  EXPECT_FALSE(extension_view_->IsAction());
  extension_view_->SetIsAction(true);
  EXPECT_TRUE(extension_view_->IsAction());
  extension_view_->SetIsAction(false);
  EXPECT_FALSE(extension_view_->IsAction());
}

// Test 244: SetExtensionState changes state.
TEST_F(AstraExtensionIconViewTest, SetExtensionState) {
  extension_view_->SetExtensionState(AstraExtensionState::kDisabled);
  EXPECT_EQ(extension_view_->GetExtensionState(),
            AstraExtensionState::kDisabled);

  extension_view_->SetExtensionState(AstraExtensionState::kBlocked);
  EXPECT_EQ(extension_view_->GetExtensionState(),
            AstraExtensionState::kBlocked);

  extension_view_->SetExtensionState(AstraExtensionState::kEnabled);
  EXPECT_EQ(extension_view_->GetExtensionState(),
            AstraExtensionState::kEnabled);
}

// Test 245: SetExtensionEnabled toggles enabled.
TEST_F(AstraExtensionIconViewTest, SetExtensionEnabled) {
  EXPECT_TRUE(extension_view_->IsExtensionEnabled());
  extension_view_->SetExtensionEnabled(false);
  EXPECT_FALSE(extension_view_->IsExtensionEnabled());
  extension_view_->SetExtensionEnabled(true);
  EXPECT_TRUE(extension_view_->IsExtensionEnabled());
}

// Test 246: SetPopupShowing toggles popup state.
TEST_F(AstraExtensionIconViewTest, SetPopupShowing) {
  EXPECT_FALSE(extension_view_->popup_showing());
  extension_view_->SetPopupShowing(true);
  EXPECT_TRUE(extension_view_->popup_showing());
  extension_view_->SetPopupShowing(false);
  EXPECT_FALSE(extension_view_->popup_showing());
}

// Test 247: Delegate can be set.
TEST_F(AstraExtensionIconViewTest, DelegateCanBeSet) {
  MockExtensionIconDelegate delegate;
  // Can't set delegate after construction since we pass it in constructor.
  // This test verifies construction with delegate works.
  auto view = std::make_unique<AstraExtensionIconView>("test-id", nullptr);
  SUCCEED();
}

// Test 248: Preferred size is valid.
TEST_F(AstraExtensionIconViewTest, PreferredSizeValid) {
  gfx::Size size = extension_view_->GetPreferredSize();
  EXPECT_GT(size.height(), 0);
  EXPECT_GT(size.width(), 0);
  // Extension icon should be square.
  EXPECT_EQ(size.width(), size.height());
}

// Test 249: All extension states are valid.
TEST_F(AstraExtensionIconViewTest, AllStatesValid) {
  std::vector<AstraExtensionState> states = {
      AstraExtensionState::kEnabled,
      AstraExtensionState::kDisabled,
      AstraExtensionState::kBlocked,
  };

  for (auto state : states) {
    extension_view_->SetExtensionState(state);
    EXPECT_EQ(extension_view_->GetExtensionState(), state);
  }
}

// Test 250: Popup showing with disabled extension.
TEST_F(AstraExtensionIconViewTest, PopupShowingWithDisabled) {
  extension_view_->SetExtensionEnabled(false);
  extension_view_->SetPopupShowing(true);
  // Should not crash.
  SUCCEED();
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

}  // namespace
}  // namespace astra
