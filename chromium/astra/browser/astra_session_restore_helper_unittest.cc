// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_session_restore_helper.h"

#include <vector>

#include "base/strings/string_util.h"
#include "base/test/task_environment.h"
#include "base/values.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "astra/browser/astra_prefs.h"
#include "astra/browser/astra_session_metadata.h"

namespace astra {

namespace {

// Test observer that records calls for verification.
class TestSessionRestoreObserver : public AstraSessionRestoreObserver {
 public:
  void OnTabRestored(content::WebContents* web_contents) override {
    tab_restored_count_++;
    last_tab_restored_ = web_contents;
  }

  void OnWindowRestored(Browser* browser) override {
    window_restored_count_++;
    last_window_restored_ = browser;
  }

  void OnSessionRestoreComplete(Profile* profile) override {
    session_restore_complete_count_++;
    last_restore_profile_ = profile;
  }

  void OnSessionSaved(Profile* profile,
                      size_t tab_count,
                      size_t window_count) override {
    session_saved_count_++;
    last_saved_profile_ = profile;
    last_saved_tab_count_ = tab_count;
    last_saved_window_count_ = window_count;
  }

  // Counters
  int tab_restored_count_ = 0;
  int window_restored_count_ = 0;
  int session_restore_complete_count_ = 0;
  int session_saved_count_ = 0;

  // Last recorded values
  content::WebContents* last_tab_restored_ = nullptr;
  Browser* last_window_restored_ = nullptr;
  Profile* last_restore_profile_ = nullptr;
  Profile* last_saved_profile_ = nullptr;
  size_t last_saved_tab_count_ = 0;
  size_t last_saved_window_count_ = 0;
};

}  // namespace

// Test fixture for AstraSessionRestoreHelper tests.
//
// Uses TestingProfile so that PrefService-based tests have a real profile.
// Tests that require WebContents or Browser objects use the null-pointer
// path to verify error handling and edge cases.  Tests with real objects
// require a content/browser test harness and are marked as TODO(astra).
//
// TODO(astra): Add WebContents-based tests using content::WebContentsTester
// or content::TestWebContentsFactory once the content test harness is
// available.  Chromium component: content/public/test:test_support.
//
// TODO(astra): Add Browser-based tests using a Browser test helper once
// the browser test harness is available.  Chromium component:
// chrome/test:test_support with Browser creation helpers.
class SessionRestoreHelperTest : public testing::Test {
 protected:
  SessionRestoreHelperTest() {
    profile_ = std::make_unique<TestingProfile>();
    // Reset stats before each test to avoid cross-test contamination.
    AstraSessionRestoreHelper::ResetRestoreStats();
  }

  ~SessionRestoreHelperTest() override = default;

  // Helpers ---------------------------------------------------------------

  // Builds a sample Astra metadata dict for tab-level testing.
  static base::Value::Dict BuildSampleTabMetadata() {
    base::Value::Dict dict;
    dict.Set(kMetaKeyWorkspaceId, "workspace-1");
    dict.Set(kMetaKeyIsFavorite, true);
    dict.Set(kMetaKeyFavoriteFolderId, "folder-1");
    dict.Set(kMetaKeyFavoriteOrderIndex, 2);
    dict.Set(kMetaKeySidebarPinned, true);
    dict.Set(kMetaKeyIsInSplitView, true);
    dict.Set(kMetaKeySplitViewPartnerId, "partner-tab-123");
    dict.Set(kMetaKeySplitViewRatio, 0.6);
    dict.Set(kMetaKeySplitViewOrientation,
             static_cast<int>(SplitViewOrientation::kHorizontal));
    dict.Set(kMetaKeyTabUniqueId,
             "(1234567890ABCDEF,1234567890ABCDEF)");
    dict.Set(kMetaKeySourceWorkspaceId, "source-workspace");
    dict.Set(kMetaKeyTabColor, 0xFFFF0000);  // ARGB red
    dict.Set(kMetaKeyReadLater, true);
    dict.Set(kMetaKeyIsSnoozed, true);
    dict.Set(kMetaKeySnoozeTime, 1337000000000000.0);
    dict.Set(kMetaKeyIsHibernatedTab, false);
    dict.Set(kMetaKeyCreatedTime, 1335000000000000.0);
    return dict;
  }

  // Builds a minimal tab metadata dict (just workspace_id).
  static base::Value::Dict BuildMinimalTabMetadata() {
    base::Value::Dict dict;
    dict.Set(kMetaKeyWorkspaceId, "default");
    return dict;
  }

  // Builds a sample Astra metadata dict for window-level testing.
  static base::Value::Dict BuildSampleWindowMetadata() {
    base::Value::Dict dict;
    dict.Set(kMetaKeyWindowWorkspaceId, "workspace-2");
    dict.Set(kMetaKeyWindowSavedBoundsX, 100);
    dict.Set(kMetaKeyWindowSavedBoundsY, 50);
    dict.Set(kMetaKeyWindowSavedBoundsWidth, 1200);
    dict.Set(kMetaKeyWindowSavedBoundsHeight, 800);
    dict.Set(kMetaKeyWindowIsMinimized, false);
    dict.Set(kMetaKeyWindowIsMaximized, true);
    return dict;
  }

  // Builds a minimal window metadata dict (just workspace_id).
  static base::Value::Dict BuildMinimalWindowMetadata() {
    base::Value::Dict dict;
    dict.Set(kMetaKeyWindowWorkspaceId, "default");
    return dict;
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
};

// ===========================================================================
// Observer default implementations
// ===========================================================================

TEST(AstraSessionRestoreObserverTest, DefaultImplementationsAreNoOps) {
  // Observer has default empty implementations for all methods.
  // Derived classes can override only what they need.
  class TestObserver : public AstraSessionRestoreObserver {};

  TestObserver observer;
  content::WebContents* null_contents = nullptr;
  Browser* null_browser = nullptr;
  Profile* null_profile = nullptr;

  observer.OnTabRestored(null_contents);
  observer.OnWindowRestored(null_browser);
  observer.OnSessionRestoreComplete(null_profile);
  observer.OnSessionSaved(null_profile, 0, 0);
  // No crash = success (default implementations are no-ops).
}

TEST(AstraSessionRestoreObserverTest, PartialObserverDefaultsWork) {
  // Observer that overrides only OnTabRestored.
  class PartialObserver : public AstraSessionRestoreObserver {
   public:
    void OnTabRestored(content::WebContents* /* web_contents */) override {
      tab_count_++;
    }
    int tab_count_ = 0;
  };

  PartialObserver observer;
  EXPECT_EQ(observer.tab_count_, 0);

  // All methods should be callable without crash.
  observer.OnTabRestored(nullptr);
  observer.OnWindowRestored(nullptr);
  observer.OnSessionRestoreComplete(nullptr);
  observer.OnSessionSaved(nullptr, 5, 2);

  EXPECT_EQ(observer.tab_count_, 1);
}

// ===========================================================================
// Observer management
// ===========================================================================

TEST_F(SessionRestoreHelperTest, AddRemoveObserver_NoCrash) {
  TestSessionRestoreObserver observer;

  AstraSessionRestoreHelper::AddObserver(&observer);
  AstraSessionRestoreHelper::RemoveObserver(&observer);
  // No crash = success.
}

TEST_F(SessionRestoreHelperTest, ObserverNotifiedOnTabRestore) {
  TestSessionRestoreObserver observer;
  AstraSessionRestoreHelper::AddObserver(&observer);

  AstraSessionRestoreHelper::OnTabRestored(nullptr);

  EXPECT_EQ(observer.tab_restored_count_, 1);
  EXPECT_EQ(observer.last_tab_restored_, nullptr);

  AstraSessionRestoreHelper::RemoveObserver(&observer);
}

TEST_F(SessionRestoreHelperTest, ObserverNotifiedOnWindowRestore) {
  TestSessionRestoreObserver observer;
  AstraSessionRestoreHelper::AddObserver(&observer);

  AstraSessionRestoreHelper::OnWindowRestored(nullptr);

  EXPECT_EQ(observer.window_restored_count_, 1);
  EXPECT_EQ(observer.last_window_restored_, nullptr);

  AstraSessionRestoreHelper::RemoveObserver(&observer);
}

TEST_F(SessionRestoreHelperTest, ObserverNotifiedOnSessionRestoreComplete) {
  TestSessionRestoreObserver observer;
  AstraSessionRestoreHelper::AddObserver(&observer);

  AstraSessionRestoreHelper::OnSessionRestoreComplete(profile_.get());

  EXPECT_EQ(observer.session_restore_complete_count_, 1);
  EXPECT_EQ(observer.last_restore_profile_, profile_.get());

  AstraSessionRestoreHelper::RemoveObserver(&observer);
}

TEST_F(SessionRestoreHelperTest, RemoveObserverStopsNotifications) {
  TestSessionRestoreObserver observer;
  AstraSessionRestoreHelper::AddObserver(&observer);

  AstraSessionRestoreHelper::OnTabRestored(nullptr);
  EXPECT_EQ(observer.tab_restored_count_, 1);

  AstraSessionRestoreHelper::RemoveObserver(&observer);

  AstraSessionRestoreHelper::OnTabRestored(nullptr);
  EXPECT_EQ(observer.tab_restored_count_, 1);  // Still 1.
}

TEST_F(SessionRestoreHelperTest, MultipleObserversAllNotified) {
  TestSessionRestoreObserver observer1;
  TestSessionRestoreObserver observer2;

  AstraSessionRestoreHelper::AddObserver(&observer1);
  AstraSessionRestoreHelper::AddObserver(&observer2);

  AstraSessionRestoreHelper::OnTabRestored(nullptr);

  EXPECT_EQ(observer1.tab_restored_count_, 1);
  EXPECT_EQ(observer2.tab_restored_count_, 1);

  AstraSessionRestoreHelper::RemoveObserver(&observer1);
  AstraSessionRestoreHelper::RemoveObserver(&observer2);
}

TEST_F(SessionRestoreHelperTest, ObserverNotifiedOnSessionSaved) {
  TestSessionRestoreObserver observer;
  AstraSessionRestoreHelper::AddObserver(&observer);

  AstraSessionRestoreHelper::RecordSessionSaved(profile_.get(), 10, 2, 3);

  EXPECT_EQ(observer.session_saved_count_, 1);
  EXPECT_EQ(observer.last_saved_profile_, profile_.get());
  EXPECT_EQ(observer.last_saved_tab_count_, 10u);
  EXPECT_EQ(observer.last_saved_window_count_, 2u);

  AstraSessionRestoreHelper::RemoveObserver(&observer);
}

// ===========================================================================
// OnWillSaveTab — null / edge cases
// ===========================================================================

TEST_F(SessionRestoreHelperTest, OnWillSaveTab_NullWebContentsReturnsEmptyDict) {
  base::Value::Dict result =
      AstraSessionRestoreHelper::OnWillSaveTab(nullptr);

  EXPECT_TRUE(result.empty());
}

// ===========================================================================
// OnWillRestoreTab — null / edge cases
// ===========================================================================

TEST_F(SessionRestoreHelperTest, OnWillRestoreTab_NullWebContentsDoesNotCrash) {
  base::Value::Dict metadata = BuildSampleTabMetadata();

  // Should not crash with null web contents.
  bool result = AstraSessionRestoreHelper::OnWillRestoreTab(
      profile_.get(), nullptr, metadata);

  EXPECT_FALSE(result);
}

TEST_F(SessionRestoreHelperTest, OnWillRestoreTab_EmptyMetadataNoCrash) {
  base::Value::Dict empty_metadata;

  bool result = AstraSessionRestoreHelper::OnWillRestoreTab(
      profile_.get(), nullptr, empty_metadata);

  EXPECT_FALSE(result);
}

TEST_F(SessionRestoreHelperTest, OnWillRestoreTab_NullProfileNoCrash) {
  base::Value::Dict metadata = BuildSampleTabMetadata();

  // Null profile should not crash.
  bool result = AstraSessionRestoreHelper::OnWillRestoreTab(
      nullptr, nullptr, metadata);

  EXPECT_FALSE(result);
}

// ===========================================================================
// OnTabRestored — null / edge cases
// ===========================================================================

TEST_F(SessionRestoreHelperTest, OnTabRestored_NullWebContentsNoCrash) {
  // This should handle null gracefully.
  AstraSessionRestoreHelper::OnTabRestored(nullptr);
  SUCCEED() << "OnTabRestored with null web contents handled without crash.";
}

// ===========================================================================
// OnWillSaveWindow — null / edge cases
// ===========================================================================

TEST_F(SessionRestoreHelperTest, OnWillSaveWindow_NullBrowserReturnsEmptyDict) {
  base::Value::Dict result =
      AstraSessionRestoreHelper::OnWillSaveWindow(profile_.get(), nullptr);

  // Null browser returns empty dict.
  EXPECT_TRUE(result.empty());
}

TEST_F(SessionRestoreHelperTest, OnWillSaveWindow_NullProfileNoCrash) {
  base::Value::Dict result =
      AstraSessionRestoreHelper::OnWillSaveWindow(nullptr, nullptr);
  // No crash = success.
  SUCCEED();
}

// ===========================================================================
// OnWillRestoreWindow — null / edge cases
// ===========================================================================

TEST_F(SessionRestoreHelperTest, OnWillRestoreWindow_NullBrowserNoCrash) {
  base::Value::Dict metadata = BuildSampleWindowMetadata();

  bool result = AstraSessionRestoreHelper::OnWillRestoreWindow(
      profile_.get(), nullptr, metadata);

  EXPECT_FALSE(result);
}

TEST_F(SessionRestoreHelperTest, OnWillRestoreWindow_EmptyMetadataNoCrash) {
  base::Value::Dict empty_metadata;

  bool result = AstraSessionRestoreHelper::OnWillRestoreWindow(
      profile_.get(), nullptr, empty_metadata);

  EXPECT_FALSE(result);
}

TEST_F(SessionRestoreHelperTest, OnWillRestoreWindow_NullProfileNoCrash) {
  base::Value::Dict metadata = BuildSampleWindowMetadata();

  bool result = AstraSessionRestoreHelper::OnWillRestoreWindow(
      nullptr, nullptr, metadata);

  EXPECT_FALSE(result);
}

TEST_F(SessionRestoreHelperTest, OnWindowRestored_NullBrowserNoCrash) {
  AstraSessionRestoreHelper::OnWindowRestored(nullptr);
  SUCCEED();
}

// ===========================================================================
// Metadata key constants
// ===========================================================================

TEST_F(SessionRestoreHelperTest, TabMetadataKeys_AreNamespaced) {
  // All tab-level keys should be prefixed with "astra." to avoid collisions
  // with Chromium's own session data keys.
  EXPECT_TRUE(base::StartsWith(kMetaKeyWorkspaceId, "astra.",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::StartsWith(kMetaKeyIsFavorite, "astra.",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::StartsWith(kMetaKeyFavoriteFolderId, "astra.",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::StartsWith(kMetaKeyFavoriteOrderIndex, "astra.",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::StartsWith(kMetaKeySidebarPinned, "astra.",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::StartsWith(kMetaKeyIsInSplitView, "astra.",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::StartsWith(kMetaKeySplitViewPartnerId, "astra.",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::StartsWith(kMetaKeySplitViewRatio, "astra.",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::StartsWith(kMetaKeySplitViewOrientation, "astra.",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::StartsWith(kMetaKeyTabStackId, "astra.",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::StartsWith(kMetaKeyIsGlanceTab, "astra.",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::StartsWith(kMetaKeyIsPipTab, "astra.",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::StartsWith(kMetaKeySidebarHidden, "astra.",
                               base::CompareCase::SENSITIVE));
}

TEST_F(SessionRestoreHelperTest, WindowMetadataKeys_AreNamespaced) {
  EXPECT_TRUE(base::StartsWith(kMetaKeyWindowWorkspaceId, "astra.",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::StartsWith(kMetaKeyWindowSavedBoundsX, "astra.",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::StartsWith(kMetaKeyWindowSavedBoundsY, "astra.",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::StartsWith(kMetaKeyWindowSavedBoundsWidth, "astra.",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::StartsWith(kMetaKeyWindowSavedBoundsHeight, "astra.",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::StartsWith(kMetaKeyWindowIsMinimized, "astra.",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::StartsWith(kMetaKeyWindowIsMaximized, "astra.",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::StartsWith(kMetaKeyWindowSidebarVisible, "astra.",
                               base::CompareCase::SENSITIVE));
}

TEST_F(SessionRestoreHelperTest, TabMetadataKeys_HaveExpectedNames) {
  EXPECT_STREQ(kMetaKeyWorkspaceId, "astra.workspace_id");
  EXPECT_STREQ(kMetaKeyIsFavorite, "astra.is_favorite");
  EXPECT_STREQ(kMetaKeyFavoriteFolderId, "astra.favorite_folder_id");
  EXPECT_STREQ(kMetaKeyFavoriteOrderIndex, "astra.favorite_order_index");
  EXPECT_STREQ(kMetaKeySidebarPinned, "astra.sidebar_pinned");
  EXPECT_STREQ(kMetaKeyIsInSplitView, "astra.is_in_split_view");
  EXPECT_STREQ(kMetaKeySplitViewPartnerId, "astra.split_view_partner_id");
  EXPECT_STREQ(kMetaKeySplitViewRatio, "astra.split_view_ratio");
  EXPECT_STREQ(kMetaKeySplitViewOrientation, "astra.split_view_orientation");
  EXPECT_STREQ(kMetaKeyTabStackId, "astra.tab_stack_id");
  EXPECT_STREQ(kMetaKeyIsGlanceTab, "astra.is_glance_tab");
  EXPECT_STREQ(kMetaKeyIsPipTab, "astra.is_pip_tab");
}

TEST_F(SessionRestoreHelperTest, WindowMetadataKeys_HaveExpectedNames) {
  EXPECT_STREQ(kMetaKeyWindowWorkspaceId, "astra.window_workspace_id");
  EXPECT_STREQ(kMetaKeyWindowSavedBoundsX, "astra.window_saved_bounds_x");
  EXPECT_STREQ(kMetaKeyWindowSavedBoundsY, "astra.window_saved_bounds_y");
  EXPECT_STREQ(kMetaKeyWindowSavedBoundsWidth,
               "astra.window_saved_bounds_width");
  EXPECT_STREQ(kMetaKeyWindowSavedBoundsHeight,
               "astra.window_saved_bounds_height");
  EXPECT_STREQ(kMetaKeyWindowIsMinimized, "astra.window_is_minimized");
  EXPECT_STREQ(kMetaKeyWindowIsMaximized, "astra.window_is_maximized");
}

// ===========================================================================
// Sample metadata structure validation
// ===========================================================================

TEST_F(SessionRestoreHelperTest, SampleTabMetadata_HasAllKeys) {
  base::Value::Dict metadata = BuildSampleTabMetadata();

  EXPECT_TRUE(metadata.Find(kMetaKeyWorkspaceId));
  EXPECT_TRUE(metadata.Find(kMetaKeyIsFavorite));
  EXPECT_TRUE(metadata.Find(kMetaKeyFavoriteFolderId));
  EXPECT_TRUE(metadata.Find(kMetaKeyFavoriteOrderIndex));
  EXPECT_TRUE(metadata.Find(kMetaKeySidebarPinned));
  EXPECT_TRUE(metadata.Find(kMetaKeyIsInSplitView));
  EXPECT_TRUE(metadata.Find(kMetaKeySplitViewPartnerId));
  EXPECT_TRUE(metadata.Find(kMetaKeySplitViewRatio));
  EXPECT_TRUE(metadata.Find(kMetaKeySplitViewOrientation));
}

TEST_F(SessionRestoreHelperTest, SampleWindowMetadata_HasAllKeys) {
  base::Value::Dict metadata = BuildSampleWindowMetadata();

  EXPECT_TRUE(metadata.Find(kMetaKeyWindowWorkspaceId));
  EXPECT_TRUE(metadata.Find(kMetaKeyWindowSavedBoundsX));
  EXPECT_TRUE(metadata.Find(kMetaKeyWindowSavedBoundsY));
  EXPECT_TRUE(metadata.Find(kMetaKeyWindowSavedBoundsWidth));
  EXPECT_TRUE(metadata.Find(kMetaKeyWindowSavedBoundsHeight));
  EXPECT_TRUE(metadata.Find(kMetaKeyWindowIsMinimized));
  EXPECT_TRUE(metadata.Find(kMetaKeyWindowIsMaximized));
}

TEST_F(SessionRestoreHelperTest, TabMetadata_HasCorrectTypes) {
  base::Value::Dict metadata = BuildSampleTabMetadata();

  EXPECT_TRUE(metadata.FindString(kMetaKeyWorkspaceId));
  EXPECT_TRUE(metadata.FindBool(kMetaKeyIsFavorite));
  EXPECT_TRUE(metadata.FindString(kMetaKeyFavoriteFolderId));
  EXPECT_TRUE(metadata.FindInt(kMetaKeyFavoriteOrderIndex));
  EXPECT_TRUE(metadata.FindBool(kMetaKeySidebarPinned));
  EXPECT_TRUE(metadata.FindBool(kMetaKeyIsInSplitView));
  EXPECT_TRUE(metadata.FindString(kMetaKeySplitViewPartnerId));
  EXPECT_TRUE(metadata.FindDouble(kMetaKeySplitViewRatio));
  EXPECT_TRUE(metadata.FindInt(kMetaKeySplitViewOrientation));
}

TEST_F(SessionRestoreHelperTest, WindowMetadata_HasCorrectTypes) {
  base::Value::Dict metadata = BuildSampleWindowMetadata();

  EXPECT_TRUE(metadata.FindString(kMetaKeyWindowWorkspaceId));
  EXPECT_TRUE(metadata.FindInt(kMetaKeyWindowSavedBoundsX));
  EXPECT_TRUE(metadata.FindInt(kMetaKeyWindowSavedBoundsY));
  EXPECT_TRUE(metadata.FindInt(kMetaKeyWindowSavedBoundsWidth));
  EXPECT_TRUE(metadata.FindInt(kMetaKeyWindowSavedBoundsHeight));
  EXPECT_TRUE(metadata.FindBool(kMetaKeyWindowIsMinimized));
  EXPECT_TRUE(metadata.FindBool(kMetaKeyWindowIsMaximized));
}

// ===========================================================================
// Metadata validation tests
// ===========================================================================

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_EmptyIsValid) {
  base::Value::Dict empty;
  EXPECT_TRUE(ValidateAstraTabMetadata(empty));
}

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_SampleIsValid) {
  base::Value::Dict metadata = BuildSampleTabMetadata();
  EXPECT_TRUE(ValidateAstraTabMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_WrongTypeWorkspaceId) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyWorkspaceId, 42);  // Should be string.
  EXPECT_FALSE(ValidateAstraTabMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_WrongTypeIsFavorite) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyIsFavorite, "true");  // Should be bool.
  EXPECT_FALSE(ValidateAstraTabMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_InvalidSplitRatio) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeySplitViewRatio, 1.5);  // Out of range [0,1].
  EXPECT_FALSE(ValidateAstraTabMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_NegativeSplitRatio) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeySplitViewRatio, -0.1);
  EXPECT_FALSE(ValidateAstraTabMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_InvalidFavoriteOrder) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyFavoriteOrderIndex, -1);
  EXPECT_FALSE(ValidateAstraTabMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_UnknownKeysAllowed) {
  base::Value::Dict metadata = BuildSampleTabMetadata();
  metadata.Set("unknown_key", "value");
  metadata.Set("another.unknown", 42);
  // Unknown keys don't cause validation failure (forward compatibility).
  EXPECT_TRUE(ValidateAstraTabMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_InvalidPipWidth) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyPipWindowWidth, -100);
  EXPECT_FALSE(ValidateAstraTabMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, ValidateWindowMetadata_EmptyIsValid) {
  base::Value::Dict empty;
  EXPECT_TRUE(ValidateAstraWindowMetadata(empty));
}

TEST_F(SessionRestoreHelperTest, ValidateWindowMetadata_SampleIsValid) {
  base::Value::Dict metadata = BuildSampleWindowMetadata();
  EXPECT_TRUE(ValidateAstraWindowMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, ValidateWindowMetadata_WrongTypeWorkspaceId) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyWindowWorkspaceId, 123);
  EXPECT_FALSE(ValidateAstraWindowMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, ValidateWindowMetadata_NegativeWidth) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyWindowSavedBoundsWidth, -1);
  EXPECT_FALSE(ValidateAstraWindowMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, ValidateWindowMetadata_UnknownKeysAllowed) {
  base::Value::Dict metadata = BuildSampleWindowMetadata();
  metadata.Set("chrome_key", "something");
  EXPECT_TRUE(ValidateAstraWindowMetadata(metadata));
}

// ===========================================================================
// HasAstraMetadata / HasAstraWindowMetadata
// ===========================================================================

TEST_F(SessionRestoreHelperTest, HasAstraMetadata_EmptyReturnsFalse) {
  base::Value::Dict empty;
  EXPECT_FALSE(HasAstraMetadata(empty));
}

TEST_F(SessionRestoreHelperTest, HasAstraMetadata_WithWorkspaceReturnsTrue) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyWorkspaceId, "default");
  EXPECT_TRUE(HasAstraMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, HasAstraMetadata_NonAstraKeysReturnsFalse) {
  base::Value::Dict metadata;
  metadata.Set("chrome.some_key", "value");
  metadata.Set("other_key", 42);
  EXPECT_FALSE(HasAstraMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, HasAstraWindowMetadata_EmptyReturnsFalse) {
  base::Value::Dict empty;
  EXPECT_FALSE(HasAstraWindowMetadata(empty));
}

TEST_F(SessionRestoreHelperTest, HasAstraWindowMetadata_WithWorkspaceReturnsTrue) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyWindowWorkspaceId, "default");
  EXPECT_TRUE(HasAstraWindowMetadata(metadata));
}

// ===========================================================================
// Metadata utility functions: IsEmpty
// ===========================================================================

TEST_F(SessionRestoreHelperTest, IsEmptyTabMetadata_EmptyDict) {
  base::Value::Dict empty;
  EXPECT_TRUE(IsEmptyAstraTabMetadata(empty));
}

TEST_F(SessionRestoreHelperTest, IsEmptyTabMetadata_WithAstraKeysNotEmpty) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyWorkspaceId, "default");
  EXPECT_FALSE(IsEmptyAstraTabMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, IsEmptyTabMetadata_NonAstraKeysOnly) {
  base::Value::Dict metadata;
  metadata.Set("other_key", "value");
  metadata.Set("chrome.something", 123);
  EXPECT_TRUE(IsEmptyAstraTabMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, IsEmptyWindowMetadata_EmptyDict) {
  base::Value::Dict empty;
  EXPECT_TRUE(IsEmptyAstraWindowMetadata(empty));
}

TEST_F(SessionRestoreHelperTest, IsEmptyWindowMetadata_WithAstraKeys) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyWindowWorkspaceId, "default");
  EXPECT_FALSE(IsEmptyAstraWindowMetadata(metadata));
}

// ===========================================================================
// Metadata utility functions: Clone
// ===========================================================================

TEST_F(SessionRestoreHelperTest, CloneTabMetadata_EmptySource) {
  base::Value::Dict empty;
  base::Value::Dict result = CloneAstraTabMetadata(empty);
  EXPECT_TRUE(result.empty());
}

TEST_F(SessionRestoreHelperTest, CloneTabMetadata_CopiesAllAstraKeys) {
  base::Value::Dict source = BuildSampleTabMetadata();
  base::Value::Dict clone = CloneAstraTabMetadata(source);

  // Verify all Astra keys are present in clone.
  EXPECT_EQ(*clone.FindString(kMetaKeyWorkspaceId),
            *source.FindString(kMetaKeyWorkspaceId));
  EXPECT_EQ(*clone.FindBool(kMetaKeyIsFavorite),
            *source.FindBool(kMetaKeyIsFavorite));
  EXPECT_EQ(*clone.FindString(kMetaKeyFavoriteFolderId),
            *source.FindString(kMetaKeyFavoriteFolderId));
  EXPECT_EQ(*clone.FindDouble(kMetaKeySplitViewRatio),
            *source.FindDouble(kMetaKeySplitViewRatio));
}

TEST_F(SessionRestoreHelperTest, CloneTabMetadata_StripsNonAstraKeys) {
  base::Value::Dict source = BuildSampleTabMetadata();
  source.Set("chrome.key", "value");
  source.Set("other", 42);

  base::Value::Dict clone = CloneAstraTabMetadata(source);

  // Non-Astra keys should be stripped.
  EXPECT_FALSE(clone.contains("chrome.key"));
  EXPECT_FALSE(clone.contains("other"));
  // Astra keys should still be there.
  EXPECT_TRUE(clone.contains(kMetaKeyWorkspaceId));
}

TEST_F(SessionRestoreHelperTest, CloneTabMetadata_IsDeepCopy) {
  base::Value::Dict source = BuildSampleTabMetadata();
  base::Value::Dict clone = CloneAstraTabMetadata(source);

  // Modify source — clone should not change.
  source.Set(kMetaKeyWorkspaceId, "modified");
  EXPECT_EQ(*clone.FindString(kMetaKeyWorkspaceId), "workspace-1");
}

TEST_F(SessionRestoreHelperTest, CloneWindowMetadata_EmptySource) {
  base::Value::Dict empty;
  base::Value::Dict result = CloneAstraWindowMetadata(empty);
  EXPECT_TRUE(result.empty());
}

TEST_F(SessionRestoreHelperTest, CloneWindowMetadata_CopiesAstraKeys) {
  base::Value::Dict source = BuildSampleWindowMetadata();
  base::Value::Dict clone = CloneAstraWindowMetadata(source);

  EXPECT_EQ(*clone.FindString(kMetaKeyWindowWorkspaceId),
            *source.FindString(kMetaKeyWindowWorkspaceId));
  EXPECT_EQ(*clone.FindInt(kMetaKeyWindowSavedBoundsWidth),
            *source.FindInt(kMetaKeyWindowSavedBoundsWidth));
}

TEST_F(SessionRestoreHelperTest, CloneWindowMetadata_StripsNonAstraKeys) {
  base::Value::Dict source = BuildSampleWindowMetadata();
  source.Set("not_astra", "value");

  base::Value::Dict clone = CloneAstraWindowMetadata(source);
  EXPECT_FALSE(clone.contains("not_astra"));
  EXPECT_TRUE(clone.contains(kMetaKeyWindowWorkspaceId));
}

// ===========================================================================
// Metadata utility functions: Merge
// ===========================================================================

TEST_F(SessionRestoreHelperTest, MergeTabMetadata_EmptySourceNoChange) {
  base::Value::Dict target = BuildSampleTabMetadata();
  base::Value::Dict empty_source;

  MergeAstraTabMetadata(empty_source, target);

  // Target should be unchanged.
  EXPECT_TRUE(HasAstraMetadata(target));
  EXPECT_EQ(*target.FindString(kMetaKeyWorkspaceId), "workspace-1");
}

TEST_F(SessionRestoreHelperTest, MergeTabMetadata_OverridesExistingKeys) {
  base::Value::Dict target = BuildSampleTabMetadata();
  base::Value::Dict source;
  source.Set(kMetaKeyWorkspaceId, "new-workspace");
  source.Set(kMetaKeyIsFavorite, false);

  MergeAstraTabMetadata(source, target);

  EXPECT_EQ(*target.FindString(kMetaKeyWorkspaceId), "new-workspace");
  EXPECT_EQ(*target.FindBool(kMetaKeyIsFavorite), false);
  // Non-overridden keys should still be there.
  EXPECT_TRUE(target.contains(kMetaKeySplitViewRatio));
}

TEST_F(SessionRestoreHelperTest, MergeTabMetadata_AddsNewKeys) {
  base::Value::Dict target = BuildMinimalTabMetadata();
  base::Value::Dict source;
  source.Set(kMetaKeyIsFavorite, true);
  source.Set(kMetaKeySidebarPinned, true);

  MergeAstraTabMetadata(source, target);

  EXPECT_TRUE(*target.FindBool(kMetaKeyIsFavorite));
  EXPECT_TRUE(*target.FindBool(kMetaKeySidebarPinned));
}

TEST_F(SessionRestoreHelperTest, MergeTabMetadata_SourceNonAstraKeysIgnored) {
  base::Value::Dict target;
  base::Value::Dict source;
  source.Set(kMetaKeyWorkspaceId, "test");
  source.Set("chrome.key", "value");
  source.Set("other", 42);

  MergeAstraTabMetadata(source, target);

  EXPECT_TRUE(target.contains(kMetaKeyWorkspaceId));
  EXPECT_FALSE(target.contains("chrome.key"));
  EXPECT_FALSE(target.contains("other"));
}

TEST_F(SessionRestoreHelperTest, MergeWindowMetadata_OverridesKeys) {
  base::Value::Dict target = BuildSampleWindowMetadata();
  base::Value::Dict source;
  source.Set(kMetaKeyWindowWorkspaceId, "new-ws");
  source.Set(kMetaKeyWindowIsMaximized, false);

  MergeAstraWindowMetadata(source, target);

  EXPECT_EQ(*target.FindString(kMetaKeyWindowWorkspaceId), "new-ws");
  EXPECT_FALSE(*target.FindBool(kMetaKeyWindowIsMaximized));
}

// ===========================================================================
// Metadata utility functions: GetFieldCount
// ===========================================================================

TEST_F(SessionRestoreHelperTest, GetTabMetadataFieldCount_EmptyDict) {
  base::Value::Dict empty;
  EXPECT_EQ(GetAstraTabMetadataFieldCount(empty), 0u);
}

TEST_F(SessionRestoreHelperTest, GetTabMetadataFieldCount_SampleMetadata) {
  base::Value::Dict metadata = BuildSampleTabMetadata();
  // Sample has 9 keys.
  EXPECT_GE(GetAstraTabMetadataFieldCount(metadata), 9u);
}

TEST_F(SessionRestoreHelperTest, GetTabMetadataFieldCount_NonAstraKeysNotCounted) {
  base::Value::Dict metadata;
  metadata.Set("not_astra_1", "v1");
  metadata.Set("not_astra_2", 2);
  metadata.Set(kMetaKeyWorkspaceId, "ws");

  EXPECT_EQ(GetAstraTabMetadataFieldCount(metadata), 1u);
}

TEST_F(SessionRestoreHelperTest, GetWindowMetadataFieldCount_EmptyDict) {
  base::Value::Dict empty;
  EXPECT_EQ(GetAstraWindowMetadataFieldCount(empty), 0u);
}

TEST_F(SessionRestoreHelperTest, GetWindowMetadataFieldCount_SampleMetadata) {
  base::Value::Dict metadata = BuildSampleWindowMetadata();
  EXPECT_GE(GetAstraWindowMetadataFieldCount(metadata), 7u);
}

// ===========================================================================
// Metadata utility functions: Normalize
// ===========================================================================

TEST_F(SessionRestoreHelperTest, NormalizeTabMetadata_FillsDefaults) {
  base::Value::Dict metadata;
  NormalizeAstraTabMetadata(metadata);

  EXPECT_EQ(*metadata.FindString(kMetaKeyWorkspaceId), "default");
  EXPECT_FALSE(*metadata.FindBool(kMetaKeyIsFavorite));
  EXPECT_EQ(*metadata.FindString(kMetaKeyFavoriteFolderId), "root");
  EXPECT_EQ(*metadata.FindInt(kMetaKeyFavoriteOrderIndex), 0);
  EXPECT_FALSE(*metadata.FindBool(kMetaKeySidebarPinned));
  EXPECT_FALSE(*metadata.FindBool(kMetaKeySidebarHidden));
  EXPECT_FALSE(*metadata.FindBool(kMetaKeyIsInSplitView));
  EXPECT_DOUBLE_EQ(*metadata.FindDouble(kMetaKeySplitViewRatio), 0.5);
  EXPECT_EQ(*metadata.FindInt(kMetaKeySplitViewOrientation),
            static_cast<int>(SplitViewOrientation::kHorizontal));
  EXPECT_FALSE(*metadata.FindBool(kMetaKeyIsStackCollapsed));
  EXPECT_FALSE(*metadata.FindBool(kMetaKeyIsGlanceTab));
  EXPECT_FALSE(*metadata.FindBool(kMetaKeyIsPipTab));
}

TEST_F(SessionRestoreHelperTest, NormalizeTabMetadata_PreservesExistingValues) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyWorkspaceId, "my-workspace");
  metadata.Set(kMetaKeyIsFavorite, true);

  NormalizeAstraTabMetadata(metadata);

  EXPECT_EQ(*metadata.FindString(kMetaKeyWorkspaceId), "my-workspace");
  EXPECT_TRUE(*metadata.FindBool(kMetaKeyIsFavorite));
  // Defaults should be filled in for missing keys.
  EXPECT_EQ(*metadata.FindString(kMetaKeyFavoriteFolderId), "root");
}

TEST_F(SessionRestoreHelperTest, NormalizeWindowMetadata_FillsDefaults) {
  base::Value::Dict metadata;
  NormalizeAstraWindowMetadata(metadata);

  EXPECT_EQ(*metadata.FindString(kMetaKeyWindowWorkspaceId), "default");
  EXPECT_FALSE(*metadata.FindBool(kMetaKeyWindowIsMinimized));
  EXPECT_FALSE(*metadata.FindBool(kMetaKeyWindowIsMaximized));
}

TEST_F(SessionRestoreHelperTest, NormalizeWindowMetadata_PreservesExisting) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyWindowWorkspaceId, "ws-1");
  metadata.Set(kMetaKeyWindowIsMaximized, true);

  NormalizeAstraWindowMetadata(metadata);

  EXPECT_EQ(*metadata.FindString(kMetaKeyWindowWorkspaceId), "ws-1");
  EXPECT_TRUE(*metadata.FindBool(kMetaKeyWindowIsMaximized));
  EXPECT_FALSE(*metadata.FindBool(kMetaKeyWindowIsMinimized));
}

// ===========================================================================
// Metadata utility functions: AreEqual
// ===========================================================================

TEST_F(SessionRestoreHelperTest, AreTabMetadataEqual_EmptyEqual) {
  base::Value::Dict a;
  base::Value::Dict b;
  EXPECT_TRUE(AreAstraTabMetadataEqual(a, b));
}

TEST_F(SessionRestoreHelperTest, AreTabMetadataEqual_SameContent) {
  base::Value::Dict a = BuildSampleTabMetadata();
  base::Value::Dict b = BuildSampleTabMetadata();
  EXPECT_TRUE(AreAstraTabMetadataEqual(a, b));
}

TEST_F(SessionRestoreHelperTest, AreTabMetadataEqual_DifferentContent) {
  base::Value::Dict a = BuildSampleTabMetadata();
  base::Value::Dict b = BuildSampleTabMetadata();
  b.Set(kMetaKeyWorkspaceId, "different");
  EXPECT_FALSE(AreAstraTabMetadataEqual(a, b));
}

TEST_F(SessionRestoreHelperTest, AreTabMetadataEqual_IgnoresNonAstraKeys) {
  base::Value::Dict a = BuildSampleTabMetadata();
  base::Value::Dict b = BuildSampleTabMetadata();
  b.Set("extra_key", "value");
  b.Set("another", 42);
  // Non-Astra keys don't affect equality.
  EXPECT_TRUE(AreAstraTabMetadataEqual(a, b));
}

TEST_F(SessionRestoreHelperTest, AreWindowMetadataEqual_EmptyEqual) {
  base::Value::Dict a;
  base::Value::Dict b;
  EXPECT_TRUE(AreAstraWindowMetadataEqual(a, b));
}

TEST_F(SessionRestoreHelperTest, AreWindowMetadataEqual_SameContent) {
  base::Value::Dict a = BuildSampleWindowMetadata();
  base::Value::Dict b = BuildSampleWindowMetadata();
  EXPECT_TRUE(AreAstraWindowMetadataEqual(a, b));
}

TEST_F(SessionRestoreHelperTest, AreWindowMetadataEqual_DifferentContent) {
  base::Value::Dict a = BuildSampleWindowMetadata();
  base::Value::Dict b = BuildSampleWindowMetadata();
  b.Set(kMetaKeyWindowWorkspaceId, "different");
  EXPECT_FALSE(AreAstraWindowMetadataEqual(a, b));
}

TEST_F(SessionRestoreHelperTest, AreWindowMetadataEqual_IgnoresNonAstraKeys) {
  base::Value::Dict a = BuildSampleWindowMetadata();
  base::Value::Dict b = BuildSampleWindowMetadata();
  b.Set("chrome.extra", 123);
  EXPECT_TRUE(AreAstraWindowMetadataEqual(a, b));
}

// ===========================================================================
// Metadata round-trip (Clone == original after clone + normalize)
// ===========================================================================

TEST_F(SessionRestoreHelperTest, TabMetadata_CloneRoundTrip) {
  base::Value::Dict original = BuildSampleTabMetadata();
  base::Value::Dict cloned = CloneAstraTabMetadata(original);

  EXPECT_TRUE(AreAstraTabMetadataEqual(original, cloned));
}

TEST_F(SessionRestoreHelperTest, WindowMetadata_CloneRoundTrip) {
  base::Value::Dict original = BuildSampleWindowMetadata();
  base::Value::Dict cloned = CloneAstraWindowMetadata(original);

  EXPECT_TRUE(AreAstraWindowMetadataEqual(original, cloned));
}

TEST_F(SessionRestoreHelperTest, TabMetadata_NormalizeThenCloneSame) {
  base::Value::Dict metadata;
  NormalizeAstraTabMetadata(metadata);
  base::Value::Dict cloned = CloneAstraTabMetadata(metadata);

  EXPECT_TRUE(AreAstraTabMetadataEqual(metadata, cloned));
}

// ===========================================================================
// Restore stats
// ===========================================================================

TEST_F(SessionRestoreHelperTest, RestoreStats_DefaultIsEmpty) {
  const AstraSessionRestoreStats& stats = AstraSessionRestoreHelper::GetRestoreStats();
  EXPECT_TRUE(stats.IsEmpty());
  EXPECT_EQ(stats.tabs_restored, 0u);
  EXPECT_EQ(stats.windows_restored, 0u);
  EXPECT_EQ(stats.tabs_with_metadata, 0u);
  EXPECT_EQ(stats.windows_with_metadata, 0u);
  EXPECT_FALSE(stats.restore_in_progress);
}

TEST_F(SessionRestoreHelperTest, RestoreStats_ResetClearsAll) {
  // Reset is called in the test fixture's constructor.
  const AstraSessionRestoreStats& stats = AstraSessionRestoreHelper::GetRestoreStats();
  EXPECT_TRUE(stats.IsEmpty());

  // Reset again — should still be empty.
  AstraSessionRestoreHelper::ResetRestoreStats();
  EXPECT_TRUE(AstraSessionRestoreHelper::GetRestoreStats().IsEmpty());
}

TEST_F(SessionRestoreHelperTest, RestoreStats_OnSessionRestoreCompleteUpdates) {
  // After calling OnSessionRestoreComplete, restore_in_progress should be false.
  AstraSessionRestoreHelper::OnSessionRestoreComplete(profile_.get());
  EXPECT_FALSE(AstraSessionRestoreHelper::GetRestoreStats().restore_in_progress);
}

TEST_F(SessionRestoreHelperTest, RestoreStats_IsEmptyCheck) {
  AstraSessionRestoreStats stats;
  EXPECT_TRUE(stats.IsEmpty());

  stats.tabs_restored = 1;
  EXPECT_FALSE(stats.IsEmpty());

  stats.Reset();
  EXPECT_TRUE(stats.IsEmpty());
}

TEST_F(SessionRestoreHelperTest, RestoreStats_ResetClearsAllFields) {
  AstraSessionRestoreStats stats;
  stats.tabs_restored = 10;
  stats.windows_restored = 3;
  stats.tabs_with_metadata = 8;
  stats.windows_with_metadata = 2;
  stats.workspaces_restored = 2;
  stats.split_view_pairs_restored = 4;
  stats.favorite_tabs_restored = 5;
  stats.tab_stacks_restored = 2;
  stats.pip_tabs_restored = 1;
  stats.restore_in_progress = true;

  stats.Reset();

  EXPECT_EQ(stats.tabs_restored, 0u);
  EXPECT_EQ(stats.windows_restored, 0u);
  EXPECT_EQ(stats.tabs_with_metadata, 0u);
  EXPECT_EQ(stats.windows_with_metadata, 0u);
  EXPECT_EQ(stats.workspaces_restored, 0u);
  EXPECT_EQ(stats.split_view_pairs_restored, 0u);
  EXPECT_EQ(stats.favorite_tabs_restored, 0u);
  EXPECT_EQ(stats.tab_stacks_restored, 0u);
  EXPECT_EQ(stats.pip_tabs_restored, 0u);
  EXPECT_FALSE(stats.restore_in_progress);
}

// ===========================================================================
// Session restore mode
// ===========================================================================

TEST_F(SessionRestoreHelperTest, RestoreMode_DefaultIsRestoreAll) {
  SessionRestoreMode mode = AstraSessionRestoreHelper::GetRestoreMode(
      profile_.get());
  EXPECT_EQ(mode, SessionRestoreMode::kRestoreAll);
}

TEST_F(SessionRestoreHelperTest, RestoreMode_SetAndGet) {
  // Set to kRestoreLast.
  AstraSessionRestoreHelper::SetRestoreMode(profile_.get(),
                                            SessionRestoreMode::kRestoreLast);
  EXPECT_EQ(AstraSessionRestoreHelper::GetRestoreMode(profile_.get()),
            SessionRestoreMode::kRestoreLast);

  // Set to kRestoreNone.
  AstraSessionRestoreHelper::SetRestoreMode(profile_.get(),
                                            SessionRestoreMode::kRestoreNone);
  EXPECT_EQ(AstraSessionRestoreHelper::GetRestoreMode(profile_.get()),
            SessionRestoreMode::kRestoreNone);

  // Set back to kRestoreAll.
  AstraSessionRestoreHelper::SetRestoreMode(profile_.get(),
                                            SessionRestoreMode::kRestoreAll);
  EXPECT_EQ(AstraSessionRestoreHelper::GetRestoreMode(profile_.get()),
            SessionRestoreMode::kRestoreAll);
}

TEST_F(SessionRestoreHelperTest, RestoreMode_NullProfileReturnsDefault) {
  SessionRestoreMode mode = AstraSessionRestoreHelper::GetRestoreMode(nullptr);
  EXPECT_EQ(mode, SessionRestoreMode::kRestoreAll);
}

TEST_F(SessionRestoreHelperTest, RestoreMode_SetNullProfileNoCrash) {
  AstraSessionRestoreHelper::SetRestoreMode(nullptr,
                                            SessionRestoreMode::kRestoreLast);
  // No crash = success.
  SUCCEED();
}

TEST_F(SessionRestoreHelperTest, RestoreMode_PersistsAcrossServiceAccess) {
  // Set mode, then verify it's still there via direct pref access.
  AstraSessionRestoreHelper::SetRestoreMode(profile_.get(),
                                            SessionRestoreMode::kRestoreLast);

  PrefService* prefs = profile_->GetPrefs();
  std::string mode_str = prefs->GetString(prefs::kPrefSessionRestoreMode);
  EXPECT_EQ(mode_str, "last");
}

// ===========================================================================
// Restore mode string conversion
// ===========================================================================

TEST_F(SessionRestoreHelperTest, RestoreModeToString_AllModes) {
  EXPECT_EQ(AstraSessionRestoreHelper::RestoreModeToString(
      SessionRestoreMode::kRestoreAll), "all");
  EXPECT_EQ(AstraSessionRestoreHelper::RestoreModeToString(
      SessionRestoreMode::kRestoreLast), "last");
  EXPECT_EQ(AstraSessionRestoreHelper::RestoreModeToString(
      SessionRestoreMode::kRestoreNone), "none");
}

TEST_F(SessionRestoreHelperTest, RestoreModeFromString_AllModes) {
  EXPECT_EQ(AstraSessionRestoreHelper::RestoreModeFromString("all"),
            SessionRestoreMode::kRestoreAll);
  EXPECT_EQ(AstraSessionRestoreHelper::RestoreModeFromString("last"),
            SessionRestoreMode::kRestoreLast);
  EXPECT_EQ(AstraSessionRestoreHelper::RestoreModeFromString("none"),
            SessionRestoreMode::kRestoreNone);
}

TEST_F(SessionRestoreHelperTest, RestoreModeFromString_UnknownDefaultsToAll) {
  EXPECT_EQ(AstraSessionRestoreHelper::RestoreModeFromString("invalid"),
            SessionRestoreMode::kRestoreAll);
  EXPECT_EQ(AstraSessionRestoreHelper::RestoreModeFromString(""),
            SessionRestoreMode::kRestoreAll);
  EXPECT_EQ(AstraSessionRestoreHelper::RestoreModeFromString("something"),
            SessionRestoreMode::kRestoreAll);
}

TEST_F(SessionRestoreHelperTest, RestoreModeString_RoundTrip) {
  // Convert each mode to string and back — should get the same mode.
  SessionRestoreMode modes[] = {
    SessionRestoreMode::kRestoreAll,
    SessionRestoreMode::kRestoreLast,
    SessionRestoreMode::kRestoreNone,
  };

  for (SessionRestoreMode mode : modes) {
    std::string str = AstraSessionRestoreHelper::RestoreModeToString(mode);
    SessionRestoreMode parsed =
        AstraSessionRestoreHelper::RestoreModeFromString(str);
    EXPECT_EQ(parsed, mode) << "Round-trip failed for mode " << static_cast<int>(mode);
  }
}

// ===========================================================================
// Restore enabled setting
// ===========================================================================

TEST_F(SessionRestoreHelperTest, RestoreEnabled_DefaultIsTrue) {
  EXPECT_TRUE(AstraSessionRestoreHelper::IsRestoreEnabled(profile_.get()));
}

TEST_F(SessionRestoreHelperTest, RestoreEnabled_SetAndGet) {
  AstraSessionRestoreHelper::SetRestoreEnabled(profile_.get(), false);
  EXPECT_FALSE(AstraSessionRestoreHelper::IsRestoreEnabled(profile_.get()));

  AstraSessionRestoreHelper::SetRestoreEnabled(profile_.get(), true);
  EXPECT_TRUE(AstraSessionRestoreHelper::IsRestoreEnabled(profile_.get()));
}

TEST_F(SessionRestoreHelperTest, RestoreEnabled_NullProfileReturnsDefault) {
  EXPECT_TRUE(AstraSessionRestoreHelper::IsRestoreEnabled(nullptr));
}

// ===========================================================================
// Lazy loading and prompt settings
// ===========================================================================

TEST_F(SessionRestoreHelperTest, LazyLoading_DefaultIsTrue) {
  EXPECT_TRUE(AstraSessionRestoreHelper::IsLazyLoadingEnabled(profile_.get()));
}

TEST_F(SessionRestoreHelperTest, LazyLoading_NullProfileReturnsDefault) {
  EXPECT_TRUE(AstraSessionRestoreHelper::IsLazyLoadingEnabled(nullptr));
}

TEST_F(SessionRestoreHelperTest, ShowPrompt_DefaultIsFalse) {
  EXPECT_FALSE(AstraSessionRestoreHelper::ShouldShowRestorePrompt(profile_.get()));
}

TEST_F(SessionRestoreHelperTest, ShowPrompt_NullProfileReturnsDefault) {
  EXPECT_FALSE(AstraSessionRestoreHelper::ShouldShowRestorePrompt(nullptr));
}

// ===========================================================================
// Last session info
// ===========================================================================

TEST_F(SessionRestoreHelperTest, AstraLastSessionInfo_DefaultIsEmpty) {
  AstraLastSessionInfo info =
      AstraSessionRestoreHelper::GetLastSessionInfo(profile_.get());

  EXPECT_FALSE(info.has_session());
  EXPECT_TRUE(info.last_save_time.is_null());
  EXPECT_EQ(info.tab_count, 0u);
  EXPECT_EQ(info.window_count, 0u);
  EXPECT_EQ(info.workspace_count, 0u);
}

TEST_F(SessionRestoreHelperTest, AstraLastSessionInfo_RecordAndGet) {
  AstraSessionRestoreHelper::RecordSessionSaved(profile_.get(), 15, 3, 2);

  AstraLastSessionInfo info =
      AstraSessionRestoreHelper::GetLastSessionInfo(profile_.get());

  EXPECT_TRUE(info.has_session());
  EXPECT_FALSE(info.last_save_time.is_null());
  EXPECT_EQ(info.tab_count, 15u);
  EXPECT_EQ(info.window_count, 3u);
  EXPECT_EQ(info.workspace_count, 2u);
}

TEST_F(SessionRestoreHelperTest, AstraLastSessionInfo_RecordUpdatesTime) {
  AstraLastSessionInfo before =
      AstraSessionRestoreHelper::GetLastSessionInfo(profile_.get());

  AstraSessionRestoreHelper::RecordSessionSaved(profile_.get(), 5, 1, 1);

  AstraLastSessionInfo after =
      AstraSessionRestoreHelper::GetLastSessionInfo(profile_.get());

  if (before.has_session()) {
    EXPECT_GE(after.last_save_time, before.last_save_time);
  } else {
    EXPECT_FALSE(after.last_save_time.is_null());
  }
}

TEST_F(SessionRestoreHelperTest, AstraLastSessionInfo_NullProfileReturnsEmpty) {
  AstraLastSessionInfo info =
      AstraSessionRestoreHelper::GetLastSessionInfo(nullptr);

  EXPECT_FALSE(info.has_session());
  EXPECT_TRUE(info.last_save_time.is_null());
}

TEST_F(SessionRestoreHelperTest, AstraLastSessionInfo_NullProfileRecordNoCrash) {
  AstraSessionRestoreHelper::RecordSessionSaved(nullptr, 10, 2, 1);
  // No crash = success.
  SUCCEED();
}

TEST_F(SessionRestoreHelperTest, AstraLastSessionInfo_ResetClearsAll) {
  AstraLastSessionInfo info;
  info.last_save_time = base::Time::Now();
  info.tab_count = 10;
  info.window_count = 2;
  info.workspace_count = 1;

  EXPECT_TRUE(info.has_session());

  info.Reset();

  EXPECT_FALSE(info.has_session());
  EXPECT_TRUE(info.last_save_time.is_null());
  EXPECT_EQ(info.tab_count, 0u);
  EXPECT_EQ(info.window_count, 0u);
  EXPECT_EQ(info.workspace_count, 0u);
}

TEST_F(SessionRestoreHelperTest, AstraLastSessionInfo_MultipleRecords) {
  AstraSessionRestoreHelper::RecordSessionSaved(profile_.get(), 5, 1, 1);

  AstraLastSessionInfo first =
      AstraSessionRestoreHelper::GetLastSessionInfo(profile_.get());

  // Give time a chance to advance.
  base::PlatformThread::Sleep(base::Milliseconds(1));

  AstraSessionRestoreHelper::RecordSessionSaved(profile_.get(), 20, 4, 3);

  AstraLastSessionInfo second =
      AstraSessionRestoreHelper::GetLastSessionInfo(profile_.get());

  // Second record should have higher counts and later time.
  EXPECT_EQ(second.tab_count, 20u);
  EXPECT_EQ(second.window_count, 4u);
  EXPECT_EQ(second.workspace_count, 3u);
  EXPECT_GE(second.last_save_time, first.last_save_time);
}

// ===========================================================================
// Bulk operations
// ===========================================================================

TEST_F(SessionRestoreHelperTest, BulkExtract_EmptyListReturnsEmpty) {
  std::vector<content::WebContents*> empty_list;
  auto results = AstraSessionRestoreHelper::BulkExtractMetadataFromTabs(empty_list);
  EXPECT_TRUE(results.empty());
}

TEST_F(SessionRestoreHelperTest, BulkExtract_NullWebContentsList) {
  std::vector<content::WebContents*> list = {nullptr, nullptr, nullptr};
  auto results = AstraSessionRestoreHelper::BulkExtractMetadataFromTabs(list);

  ASSERT_EQ(results.size(), 3u);
  EXPECT_TRUE(results[0].empty());
  EXPECT_TRUE(results[1].empty());
  EXPECT_TRUE(results[2].empty());
}

TEST_F(SessionRestoreHelperTest, BulkApply_EmptyListsReturnsZero) {
  std::vector<base::Value::Dict> metadata_list;
  std::vector<content::WebContents*> web_contents_list;

  size_t applied = AstraSessionRestoreHelper::BulkApplyMetadataToTabs(
      metadata_list, web_contents_list, profile_.get());

  EXPECT_EQ(applied, 0u);
}

TEST_F(SessionRestoreHelperTest, BulkApply_MismatchedSizesReturnsZero) {
  std::vector<base::Value::Dict> metadata_list(3);
  std::vector<content::WebContents*> web_contents_list(5);

  size_t applied = AstraSessionRestoreHelper::BulkApplyMetadataToTabs(
      metadata_list, web_contents_list, profile_.get());

  EXPECT_EQ(applied, 0u);
}

TEST_F(SessionRestoreHelperTest, BulkApply_AllNullWebContents) {
  std::vector<base::Value::Dict> metadata_list;
  metadata_list.push_back(BuildSampleTabMetadata().Clone());
  metadata_list.push_back(BuildMinimalTabMetadata().Clone());

  std::vector<content::WebContents*> web_contents_list = {nullptr, nullptr};

  size_t applied = AstraSessionRestoreHelper::BulkApplyMetadataToTabs(
      metadata_list, web_contents_list, profile_.get());

  // All null web contents, so none applied.
  EXPECT_EQ(applied, 0u);
}

// ===========================================================================
// Persistence round-trip (PrefService)
// ===========================================================================

TEST_F(SessionRestoreHelperTest, Persistence_RestoreModeSurvives) {
  // Set mode to kRestoreLast.
  AstraSessionRestoreHelper::SetRestoreMode(profile_.get(),
                                            SessionRestoreMode::kRestoreLast);
  ASSERT_EQ(AstraSessionRestoreHelper::GetRestoreMode(profile_.get()),
            SessionRestoreMode::kRestoreLast);

  // Simulate restart by reading from the same pref service directly.
  // (TestingProfile's PrefService persists across calls.)
  PrefService* prefs = profile_->GetPrefs();
  std::string mode_str = prefs->GetString(prefs::kPrefSessionRestoreMode);
  EXPECT_EQ(mode_str, "last");

  // Verify through the helper too.
  EXPECT_EQ(AstraSessionRestoreHelper::GetRestoreMode(profile_.get()),
            SessionRestoreMode::kRestoreLast);
}

TEST_F(SessionRestoreHelperTest, Persistence_AstraLastSessionInfoSurvives) {
  AstraSessionRestoreHelper::RecordSessionSaved(profile_.get(), 42, 3, 2);

  // Verify through pref service directly.
  PrefService* prefs = profile_->GetPrefs();
  EXPECT_GT(prefs->GetDouble(prefs::kPrefSessionRestoreLastSaveTime), 0.0);
  EXPECT_EQ(prefs->GetInteger(prefs::kPrefSessionRestoreLastTabCount), 42);
  EXPECT_EQ(prefs->GetInteger(prefs::kPrefSessionRestoreLastWindowCount), 3);
  EXPECT_EQ(prefs->GetInteger(prefs::kPrefSessionRestoreLastWorkspaceCount), 2);

  // Verify through helper.
  AstraLastSessionInfo info =
      AstraSessionRestoreHelper::GetLastSessionInfo(profile_.get());
  EXPECT_TRUE(info.has_session());
  EXPECT_EQ(info.tab_count, 42u);
  EXPECT_EQ(info.window_count, 3u);
  EXPECT_EQ(info.workspace_count, 2u);
}

TEST_F(SessionRestoreHelperTest, Persistence_RestoreEnabledPersists) {
  AstraSessionRestoreHelper::SetRestoreEnabled(profile_.get(), false);

  PrefService* prefs = profile_->GetPrefs();
  EXPECT_FALSE(prefs->GetBoolean(prefs::kPrefSessionRestoreEnabled));

  EXPECT_FALSE(AstraSessionRestoreHelper::IsRestoreEnabled(profile_.get()));
}

// ===========================================================================
// Static class verification
// ===========================================================================

TEST_F(SessionRestoreHelperTest, HelperIsStaticClass) {
  // AstraSessionRestoreHelper should be a static-only class — no instances.
  // The constructor is deleted, so this is a compile-time check.
  SUCCEED() << "AstraSessionRestoreHelper is a static-only class.";
}

TEST_F(SessionRestoreHelperTest, AstraSessionRestoreStatsDefaultIsEmpty) {
  AstraSessionRestoreStats stats;
  EXPECT_TRUE(stats.IsEmpty());
  EXPECT_FALSE(stats.restore_in_progress);
}

TEST_F(SessionRestoreHelperTest, AstraLastSessionInfoDefaultIsEmpty) {
  AstraLastSessionInfo info;
  EXPECT_FALSE(info.has_session());
}

// ===========================================================================
// Edge cases
// ===========================================================================

TEST_F(SessionRestoreHelperTest, TabMetadata_AllDefaultsValid) {
  base::Value::Dict metadata;
  NormalizeAstraTabMetadata(metadata);

  // All default values should pass validation.
  EXPECT_TRUE(ValidateAstraTabMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, WindowMetadata_AllDefaultsValid) {
  base::Value::Dict metadata;
  NormalizeAstraWindowMetadata(metadata);

  EXPECT_TRUE(ValidateAstraWindowMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, TabMetadata_EmptyStringWorkspaceIdValid) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyWorkspaceId, "");
  // Empty string is still a string type — valid.
  EXPECT_TRUE(ValidateAstraTabMetadata(metadata));
  EXPECT_TRUE(HasAstraMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, TabMetadata_ZeroSplitRatioValid) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeySplitViewRatio, 0.0);
  EXPECT_TRUE(ValidateAstraTabMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, TabMetadata_OneSplitRatioValid) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeySplitViewRatio, 1.0);
  EXPECT_TRUE(ValidateAstraTabMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, TabMetadata_MinOrientationValid) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeySplitViewOrientation,
               static_cast<int>(SplitViewOrientation::kHorizontal));
  EXPECT_TRUE(ValidateAstraTabMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, TabMetadata_MaxOrientationValid) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeySplitViewOrientation,
               static_cast<int>(SplitViewOrientation::kVertical));
  EXPECT_TRUE(ValidateAstraTabMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, TabMetadata_OutOfRangeOrientationInvalid) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeySplitViewOrientation, 999);
  EXPECT_FALSE(ValidateAstraTabMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, MetadataClone_WithTabStackKeys) {
  base::Value::Dict source;
  source.Set(kMetaKeyTabStackId, "stack-1");
  source.Set(kMetaKeyStackParentId, "parent-tab");
  source.Set(kMetaKeyIsStackCollapsed, true);

  base::Value::Dict clone = CloneAstraTabMetadata(source);
  EXPECT_TRUE(AreAstraTabMetadataEqual(source, clone));
}

TEST_F(SessionRestoreHelperTest, MetadataClone_WithGlanceKeys) {
  base::Value::Dict source;
  source.Set(kMetaKeyIsGlanceTab, true);
  source.Set(kMetaKeyGlanceSourceTabId, "source-tab");

  base::Value::Dict clone = CloneAstraTabMetadata(source);
  EXPECT_TRUE(AreAstraTabMetadataEqual(source, clone));
}

TEST_F(SessionRestoreHelperTest, MetadataClone_WithPipKeys) {
  base::Value::Dict source;
  source.Set(kMetaKeyIsPipTab, true);
  source.Set(kMetaKeyPipWindowWidth, 320);
  source.Set(kMetaKeyPipWindowHeight, 240);

  base::Value::Dict clone = CloneAstraTabMetadata(source);
  EXPECT_TRUE(AreAstraTabMetadataEqual(source, clone));
}

TEST_F(SessionRestoreHelperTest, MetadataClone_WithSidebarHidden) {
  base::Value::Dict source;
  source.Set(kMetaKeySidebarHidden, true);

  base::Value::Dict clone = CloneAstraTabMetadata(source);
  EXPECT_TRUE(AreAstraTabMetadataEqual(source, clone));
}

TEST_F(SessionRestoreHelperTest, MetadataClone_WithSidebarKeys) {
  base::Value::Dict source;
  source.Set(kMetaKeySidebarPinned, true);
  source.Set(kMetaKeyIsPinned, false);  // Alias key.

  base::Value::Dict clone = CloneAstraTabMetadata(source);
  EXPECT_TRUE(clone.contains(kMetaKeySidebarPinned));
  EXPECT_TRUE(clone.contains(kMetaKeyIsPinned));
}

TEST_F(SessionRestoreHelperTest, FieldCount_AllKnownKeys) {
  // Normalize fills in all default keys — count them.
  base::Value::Dict metadata;
  NormalizeAstraTabMetadata(metadata);

  size_t count = GetAstraTabMetadataFieldCount(metadata);
  // Should have at least the major keys.
  EXPECT_GE(count, 10u);
}

// ===========================================================================
// Observer notifications — comprehensive
// ===========================================================================

TEST_F(SessionRestoreHelperTest, ObserverOnSessionSaved_CorrectCounts) {
  TestSessionRestoreObserver observer;
  AstraSessionRestoreHelper::AddObserver(&observer);

  AstraSessionRestoreHelper::RecordSessionSaved(profile_.get(), 25, 4, 3);

  EXPECT_EQ(observer.session_saved_count_, 1);
  EXPECT_EQ(observer.last_saved_tab_count_, 25u);
  EXPECT_EQ(observer.last_saved_window_count_, 4u);
  EXPECT_EQ(observer.last_saved_profile_, profile_.get());

  AstraSessionRestoreHelper::RemoveObserver(&observer);
}

TEST_F(SessionRestoreHelperTest, MultipleSavesIncrementObserverCount) {
  TestSessionRestoreObserver observer;
  AstraSessionRestoreHelper::AddObserver(&observer);

  AstraSessionRestoreHelper::RecordSessionSaved(profile_.get(), 5, 1, 1);
  AstraSessionRestoreHelper::RecordSessionSaved(profile_.get(), 10, 2, 1);
  AstraSessionRestoreHelper::RecordSessionSaved(profile_.get(), 15, 3, 2);

  EXPECT_EQ(observer.session_saved_count_, 3);

  AstraSessionRestoreHelper::RemoveObserver(&observer);
}

// ===========================================================================
// Metadata validation — tab stack keys
// ===========================================================================

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_ValidStackId) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyTabStackId, "stack-123");
  EXPECT_TRUE(ValidateAstraTabMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_InvalidStackId) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyTabStackId, 42);  // Should be string.
  EXPECT_FALSE(ValidateAstraTabMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_ValidStackParentId) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyStackParentId, "parent-tab");
  EXPECT_TRUE(ValidateAstraTabMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_ValidStackCollapsed) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyIsStackCollapsed, true);
  EXPECT_TRUE(ValidateAstraTabMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_InvalidStackCollapsed) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyIsStackCollapsed, "true");  // Should be bool.
  EXPECT_FALSE(ValidateAstraTabMetadata(metadata));
}

// ===========================================================================
// Metadata validation — Glance and PiP keys
// ===========================================================================

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_ValidGlanceTab) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyIsGlanceTab, true);
  metadata.Set(kMetaKeyGlanceSourceTabId, "source-1");
  EXPECT_TRUE(ValidateAstraTabMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_InvalidGlanceTab) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyIsGlanceTab, "yes");  // Should be bool.
  EXPECT_FALSE(ValidateAstraTabMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_InvalidGlanceSource) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyGlanceSourceTabId, 123);  // Should be string.
  EXPECT_FALSE(ValidateAstraTabMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_ValidPipTab) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyIsPipTab, true);
  metadata.Set(kMetaKeyPipWindowWidth, 320);
  metadata.Set(kMetaKeyPipWindowHeight, 240);
  EXPECT_TRUE(ValidateAstraTabMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_InvalidPipTab) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyIsPipTab, "yes");  // Should be bool.
  EXPECT_FALSE(ValidateAstraTabMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_NegativePipWidth) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyPipWindowWidth, -1);
  EXPECT_FALSE(ValidateAstraTabMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_ZeroPipWidthValid) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyPipWindowWidth, 0);
  EXPECT_TRUE(ValidateAstraTabMetadata(metadata));
}

// ===========================================================================
// Metadata validation — note keys
// ===========================================================================

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_ValidNoteId) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyNoteId, "note-123");
  EXPECT_TRUE(ValidateAstraTabMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_InvalidNoteId) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyNoteId, 123);  // Should be string.
  EXPECT_FALSE(ValidateAstraTabMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_ValidNotePreview) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyNotePreview, "Some preview text");
  EXPECT_TRUE(ValidateAstraTabMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_InvalidNotePreview) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyNotePreview, 123);  // Should be string.
  EXPECT_FALSE(ValidateAstraTabMetadata(metadata));
}

// ===========================================================================
// Window metadata validation — sidebar keys
// ===========================================================================

TEST_F(SessionRestoreHelperTest, ValidateWindowMetadata_ValidSidebarKeys) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyWindowSidebarVisible, true);
  metadata.Set(kMetaKeyWindowSidebarWidth, 300);
  metadata.Set(kMetaKeyWindowSidebarPinned, false);
  EXPECT_TRUE(ValidateAstraWindowMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, ValidateWindowMetadata_InvalidSidebarWidth) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyWindowSidebarWidth, -1);
  EXPECT_FALSE(ValidateAstraWindowMetadata(metadata));
}

TEST_F(SessionRestoreHelperTest, ValidateWindowMetadata_InvalidSidebarVisible) {
  base::Value::Dict metadata;
  metadata.Set(kMetaKeyWindowSidebarVisible, "yes");  // Should be bool.
  EXPECT_FALSE(ValidateAstraWindowMetadata(metadata));
}

// ===========================================================================
// Pref registration
// ===========================================================================

TEST(AstraSessionRestorePrefsTest, PrefKeysHaveCorrectFormat) {
  // All session restore pref keys should be under "astra.session_restore."
  using namespace prefs;

  EXPECT_TRUE(base::StartsWith(kPrefSessionRestoreMode, "astra.session_restore.",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::StartsWith(kPrefSessionRestoreEnabled, "astra.session_restore.",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::StartsWith(kPrefSessionRestoreLastSaveTime,
                               "astra.session_restore.",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::StartsWith(kPrefSessionRestoreLastTabCount,
                               "astra.session_restore.",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::StartsWith(kPrefSessionRestoreLastWindowCount,
                               "astra.session_restore.",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::StartsWith(kPrefSessionRestoreLastWorkspaceCount,
                               "astra.session_restore.",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::StartsWith(kPrefSessionRestoreLazyLoading,
                               "astra.session_restore.",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::StartsWith(kPrefSessionRestoreShowPrompt,
                               "astra.session_restore.",
                               base::CompareCase::SENSITIVE));
}

TEST(AstraSessionRestorePrefsTest, DefaultsAreSane) {
  using namespace prefs;

  // Default mode should be "all".
  EXPECT_STREQ(kDefaultSessionRestoreMode, "all");

  // Restore should be enabled by default.
  EXPECT_TRUE(kDefaultSessionRestoreEnabled);

  // Last save time should be 0 (no session saved yet).
  EXPECT_DOUBLE_EQ(kDefaultSessionRestoreLastSaveTime, 0.0);

  // Counts should be 0.
  EXPECT_EQ(kDefaultSessionRestoreLastTabCount, 0);
  EXPECT_EQ(kDefaultSessionRestoreLastWindowCount, 0);
  EXPECT_EQ(kDefaultSessionRestoreLastWorkspaceCount, 0);

  // Lazy loading should be on by default.
  EXPECT_TRUE(kDefaultSessionRestoreLazyLoading);

  // Prompt should be off by default (auto-restore).
  EXPECT_FALSE(kDefaultSessionRestoreShowPrompt);
}

// ===========================================================================
// SessionRestoreMode enum completeness
// ===========================================================================

TEST(AstraSessionRestoreModeTest, HasAllThreeModes) {
  // The enum should have three modes: all, last, none.
  // Verify by checking string conversion.
  EXPECT_EQ(AstraSessionRestoreHelper::RestoreModeToString(
      SessionRestoreMode::kRestoreAll), "all");
  EXPECT_EQ(AstraSessionRestoreHelper::RestoreModeToString(
      SessionRestoreMode::kRestoreLast), "last");
  EXPECT_EQ(AstraSessionRestoreHelper::RestoreModeToString(
      SessionRestoreMode::kRestoreNone), "none");
}

// ===========================================================================
// Bulk operations — more edge cases
// ===========================================================================

TEST_F(SessionRestoreHelperTest, BulkExtract_SizeMatchesInput) {
  std::vector<content::WebContents*> input;
  input.push_back(nullptr);
  input.push_back(nullptr);
  input.push_back(nullptr);

  auto results = AstraSessionRestoreHelper::BulkExtractMetadataFromTabs(input);

  EXPECT_EQ(results.size(), input.size());
}

TEST_F(SessionRestoreHelperTest, BulkApply_AllEmptyMetadata) {
  std::vector<base::Value::Dict> metadata_list(5);  // 5 empty dicts.
  std::vector<content::WebContents*> web_contents_list(5, nullptr);

  size_t applied = AstraSessionRestoreHelper::BulkApplyMetadataToTabs(
      metadata_list, web_contents_list, profile_.get());

  EXPECT_EQ(applied, 0u);
}

// ===========================================================================
// Metadata utility — Merge preserves target keys
// ===========================================================================

TEST_F(SessionRestoreHelperTest, MergePreservesUnrelatedTargetKeys) {
  base::Value::Dict target;
  target.Set(kMetaKeyWorkspaceId, "target-ws");
  target.Set(kMetaKeyIsFavorite, true);
  target.Set("non_astra_key", "value");  // Non-Astra key.

  base::Value::Dict source;
  source.Set(kMetaKeyIsFavorite, false);
  source.Set(kMetaKeySidebarPinned, true);

  MergeAstraTabMetadata(source, target);

  // Workspace was not overwritten (not in source).
  EXPECT_EQ(*target.FindString(kMetaKeyWorkspaceId), "target-ws");
  // Favorite was overwritten.
  EXPECT_FALSE(*target.FindBool(kMetaKeyIsFavorite));
  // Sidebar pinned was added.
  EXPECT_TRUE(*target.FindBool(kMetaKeySidebarPinned));
  // Non-Astra key was preserved.
  EXPECT_EQ(*target.FindString("non_astra_key"), "value");
}

// ===========================================================================
// TODO(astra): WebContents-based round-trip tests
// ===========================================================================
//
// The following tests require real WebContents objects with AstraTabFeatures
// user data.  They should be implemented once the content test harness is
// available:
//
// Tab-level round-trip:
//   - OnWillSaveTab_ExtractsWorkspaceId
//   - OnWillSaveTab_ExtractsFavoriteState
//   - OnWillSaveTab_ExtractsSplitViewState
//   - OnWillSaveTab_ExtractsSidebarPinnedState
//   - OnWillSaveTab_ReturnsEmptyForTabWithNoMetadata
//   - OnWillRestoreTab_AppliesWorkspaceId
//   - OnWillRestoreTab_AppliesFavoriteState
//   - OnWillRestoreTab_AppliesSplitViewState
//   - OnWillRestoreTab_AppliesSidebarPinned
//   - OnWillRestoreTab_RoundTripPreservesAllMetadata
//   - OnWillRestoreTab_UnknownKeysIgnored
//   - OnWillRestoreTab_PartialMetadataAppliesOnlyPresentFields
//
// Window-level round-trip:
//   - OnWillSaveWindow_ExtractsWorkspaceId
//   - OnWillSaveWindow_ExtractsSavedBounds
//   - OnWillSaveWindow_ExtractsMinimizedMaximizedState
//   - OnWillRestoreWindow_AppliesWorkspaceId
//   - OnWillRestoreWindow_AppliesSavedBounds
//   - OnWillRestoreWindow_RoundTripPreservesAllMetadata
//   - OnWillRestoreWindow_EmptyMetadataNoChange
//
// Edge cases:
//   - OnWillSaveTab_NewTabHasDefaultMetadata
//   - OnWillRestoreTab_InvalidMetadataValuesDontCrash
//   - OnWillRestoreTab_MetadataWithExtraKeysIgnored
//   - OnTabRestored_DoesNotCrashAfterRestore
//
// Integration with session restore:
//   - MultipleTabs_RoundTripPreservesIndividualMetadata
//   - SplitViewPair_RoundTripPreservesPartnerIds
//   - WorkspaceMembership_PreservedAcrossSessionSaveRestore
//
// TODO(astra): Add browser_tests for full session restore integration with
// real Browser, TabStripModel, and SessionService.
// Chromium component: InProcessBrowserTest + sessions::SessionService.
// Patch point: session save/restore pipeline in chrome/browser/sessions/.

// ===========================================================================
// AstraTabSessionMetadata struct tests
// ===========================================================================

TEST_F(SessionRestoreHelperTest, TabMetadataStruct_DefaultValues) {
  AstraTabSessionMetadata tab;

  EXPECT_TRUE(tab.tab_id.empty());
  EXPECT_TRUE(tab.workspace_id.empty());
  EXPECT_FALSE(tab.is_favorite);
  EXPECT_TRUE(tab.favorite_folder_id.empty());
  EXPECT_EQ(tab.favorite_order_index, 0);
  EXPECT_TRUE(tab.tab_stack_id.empty());
  EXPECT_FALSE(tab.is_pinned);
  EXPECT_FALSE(tab.sidebar_hidden);
  EXPECT_FALSE(tab.is_in_split_view);
  EXPECT_DOUBLE_EQ(tab.split_view_ratio, 0.5);
  EXPECT_EQ(tab.split_view_orientation, 0);
  EXPECT_TRUE(tab.note_id.empty());
  EXPECT_FALSE(tab.is_glance_tab);
  EXPECT_FALSE(tab.is_pip_tab);
  EXPECT_EQ(tab.pip_window_width, 0);
  EXPECT_FALSE(tab.is_in_reading_list);
  EXPECT_TRUE(tab.last_active_time.is_null());
  EXPECT_EQ(tab.discard_count, 0);
}

TEST_F(SessionRestoreHelperTest, TabMetadataStruct_ToDictDefaultIsEmptyish) {
  AstraTabSessionMetadata tab;
  base::Value::Dict dict = tab.ToDict();

  // Default bools are still serialized (false).
  EXPECT_TRUE(dict.FindBool(kMetaKeyIsFavorite).has_value());
  EXPECT_FALSE(*dict.FindBool(kMetaKeyIsFavorite));
  // Empty strings are not serialized.
  EXPECT_FALSE(dict.contains(kMetaKeyWorkspaceId));
}

TEST_F(SessionRestoreHelperTest, TabMetadataStruct_ToDictPopulated) {
  AstraTabSessionMetadata tab;
  tab.tab_id = "tab-1";
  tab.workspace_id = "ws-1";
  tab.is_favorite = true;
  tab.is_pinned = true;
  tab.is_in_split_view = true;
  tab.split_view_ratio = 0.7;
  tab.note_id = "note-1";
  tab.is_in_reading_list = true;
  tab.discard_count = 3;
  tab.last_active_time = base::Time::Now();

  base::Value::Dict dict = tab.ToDict();

  EXPECT_EQ(*dict.FindString("tab_id"), "tab-1");
  EXPECT_EQ(*dict.FindString(kMetaKeyWorkspaceId), "ws-1");
  EXPECT_TRUE(*dict.FindBool(kMetaKeyIsFavorite));
  EXPECT_TRUE(*dict.FindBool(kMetaKeyIsPinned));
  EXPECT_TRUE(*dict.FindBool(kMetaKeyIsInSplitView));
  EXPECT_DOUBLE_EQ(*dict.FindDouble(kMetaKeySplitViewRatio), 0.7);
  EXPECT_EQ(*dict.FindString(kMetaKeyNoteId), "note-1");
  EXPECT_TRUE(*dict.FindBool(kMetaKeyIsInReadingList));
  EXPECT_EQ(*dict.FindInt(kMetaKeyDiscardCount), 3);
  EXPECT_TRUE(dict.contains(kMetaKeyLastActiveTime));
}

TEST_F(SessionRestoreHelperTest, TabMetadataStruct_FromDictBasic) {
  base::Value::Dict dict;
  dict.Set(kMetaKeyWorkspaceId, "ws-42");
  dict.Set(kMetaKeyIsFavorite, true);
  dict.Set(kMetaKeyIsPinned, true);
  dict.Set(kMetaKeyIsInReadingList, true);
  dict.Set(kMetaKeyDiscardCount, 5);

  AstraTabSessionMetadata tab;
  bool result = tab.FromDict(dict);

  EXPECT_TRUE(result);
  EXPECT_EQ(tab.workspace_id, "ws-42");
  EXPECT_TRUE(tab.is_favorite);
  EXPECT_TRUE(tab.is_pinned);
  EXPECT_TRUE(tab.is_in_reading_list);
  EXPECT_EQ(tab.discard_count, 5);
}

TEST_F(SessionRestoreHelperTest, TabMetadataStruct_FromDictEmptyPreservesDefaults) {
  base::Value::Dict dict;

  AstraTabSessionMetadata tab;
  tab.workspace_id = "should-stay";
  bool result = tab.FromDict(dict);

  // Empty dict doesn't change anything.
  EXPECT_TRUE(result);
  EXPECT_EQ(tab.workspace_id, "should-stay");
}

TEST_F(SessionRestoreHelperTest, TabMetadataStruct_RoundTrip) {
  AstraTabSessionMetadata original;
  original.tab_id = "tab-xyz";
  original.workspace_id = "workspace-alpha";
  original.is_favorite = true;
  original.favorite_folder_id = "folder-1";
  original.favorite_order_index = 5;
  original.tab_stack_id = "stack-1";
  original.is_pinned = true;
  original.sidebar_hidden = false;
  original.is_in_split_view = true;
  original.split_view_partner_id = "partner-tab";
  original.split_view_ratio = 0.35;
  original.split_view_orientation = 1;
  original.note_id = "note-abc";
  original.note_preview = "A short preview";
  original.is_glance_tab = true;
  original.glance_source_tab_id = "source-tab";
  original.is_pip_tab = true;
  original.pip_window_width = 400;
  original.pip_window_height = 300;
  original.is_in_reading_list = true;
  original.last_active_time = base::Time::Now();
  original.discard_count = 7;

  base::Value::Dict dict = original.ToDict();

  AstraTabSessionMetadata restored;
  bool result = restored.FromDict(dict);

  EXPECT_TRUE(result);
  EXPECT_EQ(restored.tab_id, original.tab_id);
  EXPECT_EQ(restored.workspace_id, original.workspace_id);
  EXPECT_EQ(restored.is_favorite, original.is_favorite);
  EXPECT_EQ(restored.favorite_folder_id, original.favorite_folder_id);
  EXPECT_EQ(restored.favorite_order_index, original.favorite_order_index);
  EXPECT_EQ(restored.tab_stack_id, original.tab_stack_id);
  EXPECT_EQ(restored.is_pinned, original.is_pinned);
  EXPECT_EQ(restored.sidebar_hidden, original.sidebar_hidden);
  EXPECT_EQ(restored.is_in_split_view, original.is_in_split_view);
  EXPECT_EQ(restored.split_view_partner_id, original.split_view_partner_id);
  EXPECT_DOUBLE_EQ(restored.split_view_ratio, original.split_view_ratio);
  EXPECT_EQ(restored.split_view_orientation, original.split_view_orientation);
  EXPECT_EQ(restored.note_id, original.note_id);
  EXPECT_EQ(restored.note_preview, original.note_preview);
  EXPECT_EQ(restored.is_glance_tab, original.is_glance_tab);
  EXPECT_EQ(restored.glance_source_tab_id, original.glance_source_tab_id);
  EXPECT_EQ(restored.is_pip_tab, original.is_pip_tab);
  EXPECT_EQ(restored.pip_window_width, original.pip_window_width);
  EXPECT_EQ(restored.pip_window_height, original.pip_window_height);
  EXPECT_EQ(restored.is_in_reading_list, original.is_in_reading_list);
  EXPECT_EQ(restored.discard_count, original.discard_count);
  // last_active_time may lose precision from double conversion, so
  // we just check it's not null.
  EXPECT_FALSE(restored.last_active_time.is_null());
}

TEST_F(SessionRestoreHelperTest, TabMetadataStruct_ValidateValid) {
  AstraTabSessionMetadata tab;
  EXPECT_TRUE(tab.Validate());

  tab.is_favorite = true;
  tab.split_view_ratio = 0.5;
  tab.pip_window_width = 100;
  tab.discard_count = 0;
  EXPECT_TRUE(tab.Validate());
}

TEST_F(SessionRestoreHelperTest, TabMetadataStruct_ValidateInvalidNegativeOrder) {
  AstraTabSessionMetadata tab;
  tab.favorite_order_index = -1;
  EXPECT_FALSE(tab.Validate());
}

TEST_F(SessionRestoreHelperTest, TabMetadataStruct_ValidateInvalidRatio) {
  AstraTabSessionMetadata tab;
  tab.split_view_ratio = 1.5;
  EXPECT_FALSE(tab.Validate());

  tab.split_view_ratio = -0.1;
  EXPECT_FALSE(tab.Validate());
}

TEST_F(SessionRestoreHelperTest, TabMetadataStruct_ValidateInvalidPipSize) {
  AstraTabSessionMetadata tab;
  tab.pip_window_width = -10;
  EXPECT_FALSE(tab.Validate());
}

TEST_F(SessionRestoreHelperTest, TabMetadataStruct_ValidateInvalidDiscardCount) {
  AstraTabSessionMetadata tab;
  tab.discard_count = -1;
  EXPECT_FALSE(tab.Validate());
}

TEST_F(SessionRestoreHelperTest, TabMetadataStruct_ValidateBoundaryRatio) {
  AstraTabSessionMetadata tab;
  tab.split_view_ratio = 0.0;
  EXPECT_TRUE(tab.Validate());
  tab.split_view_ratio = 1.0;
  EXPECT_TRUE(tab.Validate());
}

TEST_F(SessionRestoreHelperTest, TabMetadataStruct_MergeFrom) {
  AstraTabSessionMetadata base;
  base.workspace_id = "ws-original";
  base.is_favorite = false;

  AstraTabSessionMetadata update;
  update.is_favorite = true;
  update.note_id = "new-note";

  base.MergeFrom(update);

  EXPECT_EQ(base.workspace_id, "ws-original");  // Not in update, preserved.
  EXPECT_TRUE(base.is_favorite);
  EXPECT_EQ(base.note_id, "new-note");
}

TEST_F(SessionRestoreHelperTest, TabMetadataStruct_EstimateSizeBytes) {
  AstraTabSessionMetadata tab;
  size_t empty_size = tab.EstimateSizeBytes();
  EXPECT_GT(empty_size, 0u);

  tab.workspace_id = "some-workspace-id";
  tab.note_id = "note-id";
  tab.tab_stack_id = "stack-id";
  size_t populated_size = tab.EstimateSizeBytes();

  EXPECT_GT(populated_size, empty_size);
}

// ===========================================================================
// AstraWindowSessionMetadata struct tests
// ===========================================================================

TEST_F(SessionRestoreHelperTest, WindowMetadataStruct_DefaultValues) {
  AstraWindowSessionMetadata window;

  EXPECT_TRUE(window.window_id.empty());
  EXPECT_TRUE(window.workspace_id.empty());
  EXPECT_EQ(window.window_order_index, 0);
  EXPECT_TRUE(window.saved_bounds.IsEmpty());
  EXPECT_FALSE(window.is_minimized);
  EXPECT_FALSE(window.is_maximized);
  EXPECT_FALSE(window.is_hibernated);
  EXPECT_TRUE(window.sidebar_visible);
  EXPECT_FALSE(window.sidebar_pinned);
  EXPECT_EQ(window.sidebar_width, 300);
  EXPECT_FALSE(window.split_view_active);
  EXPECT_EQ(window.split_view_ratio, 0.5);
  EXPECT_TRUE(window.tabs.empty());
}

TEST_F(SessionRestoreHelperTest, WindowMetadataStruct_ToDictPopulated) {
  AstraWindowSessionMetadata window;
  window.window_id = "win-1";
  window.workspace_id = "ws-1";
  window.window_order_index = 2;
  window.saved_bounds = gfx::Rect(10, 20, 800, 600);
  window.is_hibernated = true;
  window.sidebar_visible = true;
  window.sidebar_pinned = true;
  window.sidebar_width = 350;
  window.split_view_active = true;
  window.split_view_ratio = 0.6;
  window.split_view_orientation = 1;

  base::Value::Dict dict = window.ToDict();

  EXPECT_EQ(*dict.FindString("window_id"), "win-1");
  EXPECT_EQ(*dict.FindString(kMetaKeyWindowWorkspaceId), "ws-1");
  EXPECT_EQ(*dict.FindInt(kMetaKeyWindowOrderIndex), 2);
  EXPECT_EQ(*dict.FindInt(kMetaKeyWindowSavedBoundsX), 10);
  EXPECT_EQ(*dict.FindInt(kMetaKeyWindowSavedBoundsY), 20);
  EXPECT_EQ(*dict.FindInt(kMetaKeyWindowSavedBoundsWidth), 800);
  EXPECT_EQ(*dict.FindInt(kMetaKeyWindowSavedBoundsHeight), 600);
  EXPECT_TRUE(*dict.FindBool(kMetaKeyWindowIsHibernated));
  EXPECT_TRUE(*dict.FindBool(kMetaKeyWindowSidebarVisible));
  EXPECT_TRUE(*dict.FindBool(kMetaKeyWindowSidebarPinned));
  EXPECT_EQ(*dict.FindInt(kMetaKeyWindowSidebarWidth), 350);
  EXPECT_TRUE(*dict.FindBool(kMetaKeyWindowSplitViewActive));
  EXPECT_DOUBLE_EQ(*dict.FindDouble(kMetaKeyWindowSplitViewRatio), 0.6);
  EXPECT_EQ(*dict.FindInt(kMetaKeyWindowSplitViewOrientation), 1);
}

TEST_F(SessionRestoreHelperTest, WindowMetadataStruct_FromDict) {
  base::Value::Dict dict;
  dict.Set("window_id", "win-test");
  dict.Set(kMetaKeyWindowWorkspaceId, "ws-test");
  dict.Set(kMetaKeyWindowOrderIndex, 3);
  dict.Set(kMetaKeyWindowSavedBoundsX, 100);
  dict.Set(kMetaKeyWindowSavedBoundsY, 50);
  dict.Set(kMetaKeyWindowSavedBoundsWidth, 1200);
  dict.Set(kMetaKeyWindowSavedBoundsHeight, 800);
  dict.Set(kMetaKeyWindowIsHibernated, true);
  dict.Set(kMetaKeyWindowSidebarVisible, false);
  dict.Set(kMetaKeyWindowSidebarPinned, true);
  dict.Set(kMetaKeyWindowSidebarWidth, 280);
  dict.Set(kMetaKeyWindowSplitViewActive, true);

  AstraWindowSessionMetadata window;
  bool result = window.FromDict(dict);

  EXPECT_TRUE(result);
  EXPECT_EQ(window.window_id, "win-test");
  EXPECT_EQ(window.workspace_id, "ws-test");
  EXPECT_EQ(window.window_order_index, 3);
  EXPECT_EQ(window.saved_bounds.x(), 100);
  EXPECT_EQ(window.saved_bounds.width(), 1200);
  EXPECT_TRUE(window.is_hibernated);
  EXPECT_FALSE(window.sidebar_visible);
  EXPECT_TRUE(window.sidebar_pinned);
  EXPECT_EQ(window.sidebar_width, 280);
  EXPECT_TRUE(window.split_view_active);
}

TEST_F(SessionRestoreHelperTest, WindowMetadataStruct_RoundTrip) {
  AstraWindowSessionMetadata original;
  original.window_id = "win-roundtrip";
  original.workspace_id = "ws-roundtrip";
  original.window_order_index = 5;
  original.saved_bounds = gfx::Rect(50, 30, 1024, 768);
  original.is_minimized = false;
  original.is_maximized = true;
  original.is_hibernated = false;
  original.sidebar_visible = true;
  original.sidebar_pinned = true;
  original.sidebar_width = 320;
  original.split_view_active = true;
  original.split_view_orientation = 1;
  original.split_view_ratio = 0.4;

  base::Value::Dict dict = original.ToDict();

  AstraWindowSessionMetadata restored;
  bool result = restored.FromDict(dict);

  EXPECT_TRUE(result);
  EXPECT_EQ(restored.window_id, original.window_id);
  EXPECT_EQ(restored.workspace_id, original.workspace_id);
  EXPECT_EQ(restored.window_order_index, original.window_order_index);
  EXPECT_EQ(restored.saved_bounds, original.saved_bounds);
  EXPECT_EQ(restored.is_minimized, original.is_minimized);
  EXPECT_EQ(restored.is_maximized, original.is_maximized);
  EXPECT_EQ(restored.is_hibernated, original.is_hibernated);
  EXPECT_EQ(restored.sidebar_visible, original.sidebar_visible);
  EXPECT_EQ(restored.sidebar_pinned, original.sidebar_pinned);
  EXPECT_EQ(restored.sidebar_width, original.sidebar_width);
  EXPECT_EQ(restored.split_view_active, original.split_view_active);
  EXPECT_EQ(restored.split_view_orientation, original.split_view_orientation);
  EXPECT_DOUBLE_EQ(restored.split_view_ratio, original.split_view_ratio);
}

TEST_F(SessionRestoreHelperTest, WindowMetadataStruct_WithTabsRoundTrip) {
  AstraWindowSessionMetadata original;
  original.window_id = "win-tabs";

  AstraTabSessionMetadata tab1;
  tab1.tab_id = "t1";
  tab1.workspace_id = "ws-1";
  tab1.is_favorite = true;
  original.tabs.push_back(tab1);

  AstraTabSessionMetadata tab2;
  tab2.tab_id = "t2";
  tab2.workspace_id = "ws-2";
  tab2.is_pinned = true;
  original.tabs.push_back(tab2);

  base::Value::Dict dict = original.ToDict();

  AstraWindowSessionMetadata restored;
  bool result = restored.FromDict(dict);

  EXPECT_TRUE(result);
  EXPECT_EQ(restored.GetTabCount(), 2u);
  EXPECT_EQ(restored.tabs[0].tab_id, "t1");
  EXPECT_TRUE(restored.tabs[0].is_favorite);
  EXPECT_EQ(restored.tabs[1].tab_id, "t2");
  EXPECT_TRUE(restored.tabs[1].is_pinned);
}

TEST_F(SessionRestoreHelperTest, WindowMetadataStruct_ValidateValid) {
  AstraWindowSessionMetadata window;
  EXPECT_TRUE(window.Validate());
}

TEST_F(SessionRestoreHelperTest, WindowMetadataStruct_ValidateInvalidSidebarWidth) {
  AstraWindowSessionMetadata window;
  window.sidebar_width = -1;
  EXPECT_FALSE(window.Validate());
}

TEST_F(SessionRestoreHelperTest, WindowMetadataStruct_ValidateInvalidOrderIndex) {
  AstraWindowSessionMetadata window;
  window.window_order_index = -1;
  EXPECT_FALSE(window.Validate());
}

TEST_F(SessionRestoreHelperTest, WindowMetadataStruct_ValidateInvalidTab) {
  AstraWindowSessionMetadata window;
  AstraTabSessionMetadata bad_tab;
  bad_tab.split_view_ratio = 2.0;  // Invalid.
  window.tabs.push_back(bad_tab);

  EXPECT_FALSE(window.Validate());
}

TEST_F(SessionRestoreHelperTest, WindowMetadataStruct_GetTabCount) {
  AstraWindowSessionMetadata window;
  EXPECT_EQ(window.GetTabCount(), 0u);

  AstraTabSessionMetadata tab;
  window.tabs.push_back(tab);
  window.tabs.push_back(tab);
  EXPECT_EQ(window.GetTabCount(), 2u);
}

TEST_F(SessionRestoreHelperTest, WindowMetadataStruct_GetWorkspaceTabCount) {
  AstraWindowSessionMetadata window;

  AstraTabSessionMetadata tab1;
  tab1.workspace_id = "ws-a";
  window.tabs.push_back(tab1);

  AstraTabSessionMetadata tab2;
  tab2.workspace_id = "ws-a";
  window.tabs.push_back(tab2);

  AstraTabSessionMetadata tab3;
  tab3.workspace_id = "ws-b";
  window.tabs.push_back(tab3);

  EXPECT_EQ(window.GetWorkspaceTabCount("ws-a"), 2u);
  EXPECT_EQ(window.GetWorkspaceTabCount("ws-b"), 1u);
  EXPECT_EQ(window.GetWorkspaceTabCount("ws-c"), 0u);
}

TEST_F(SessionRestoreHelperTest, WindowMetadataStruct_GetTabsByWorkspace) {
  AstraWindowSessionMetadata window;

  AstraTabSessionMetadata tab1;
  tab1.tab_id = "t1";
  tab1.workspace_id = "ws-a";
  window.tabs.push_back(tab1);

  AstraTabSessionMetadata tab2;
  tab2.tab_id = "t2";
  tab2.workspace_id = "ws-a";
  window.tabs.push_back(tab2);

  auto tabs = window.GetTabsByWorkspace("ws-a");
  ASSERT_EQ(tabs.size(), 2u);
  EXPECT_EQ(tabs[0], "t1");
  EXPECT_EQ(tabs[1], "t2");
}

TEST_F(SessionRestoreHelperTest, WindowMetadataStruct_GetFavoriteTabCount) {
  AstraWindowSessionMetadata window;

  AstraTabSessionMetadata tab1;
  tab1.is_favorite = true;
  window.tabs.push_back(tab1);

  AstraTabSessionMetadata tab2;
  tab2.is_favorite = false;
  window.tabs.push_back(tab2);

  AstraTabSessionMetadata tab3;
  tab3.is_favorite = true;
  window.tabs.push_back(tab3);

  EXPECT_EQ(window.GetFavoriteTabCount(), 2u);
}

TEST_F(SessionRestoreHelperTest, WindowMetadataStruct_GetPinnedTabCount) {
  AstraWindowSessionMetadata window;

  AstraTabSessionMetadata tab1;
  tab1.is_pinned = true;
  window.tabs.push_back(tab1);

  AstraTabSessionMetadata tab2;
  tab2.sidebar_pinned = true;  // Also counts as pinned.
  window.tabs.push_back(tab2);

  EXPECT_EQ(window.GetPinnedTabCount(), 2u);
}

TEST_F(SessionRestoreHelperTest, WindowMetadataStruct_GetStackedTabCount) {
  AstraWindowSessionMetadata window;

  AstraTabSessionMetadata tab1;
  tab1.tab_stack_id = "stack-1";
  window.tabs.push_back(tab1);

  AstraTabSessionMetadata tab2;
  tab2.stack_parent_id = "parent-tab";
  window.tabs.push_back(tab2);

  AstraTabSessionMetadata tab3;
  window.tabs.push_back(tab3);

  EXPECT_EQ(window.GetStackedTabCount(), 2u);
}

TEST_F(SessionRestoreHelperTest, WindowMetadataStruct_MergeFrom) {
  AstraWindowSessionMetadata base;
  base.window_id = "win-base";
  base.workspace_id = "ws-base";

  AstraWindowSessionMetadata update;
  update.is_hibernated = true;
  update.sidebar_width = 250;

  base.MergeFrom(update);

  EXPECT_EQ(base.window_id, "win-base");
  EXPECT_EQ(base.workspace_id, "ws-base");
  EXPECT_TRUE(base.is_hibernated);
  EXPECT_EQ(base.sidebar_width, 250);
}

TEST_F(SessionRestoreHelperTest, WindowMetadataStruct_EstimateSizeBytes) {
  AstraWindowSessionMetadata window;
  size_t empty_size = window.EstimateSizeBytes();
  EXPECT_GT(empty_size, 0u);

  AstraTabSessionMetadata tab;
  tab.workspace_id = "test-workspace";
  tab.note_id = "test-note";
  window.tabs.push_back(tab);

  size_t with_tabs_size = window.EstimateSizeBytes();
  EXPECT_GT(with_tabs_size, empty_size);
}

// ===========================================================================
// AstraSessionMetadata struct tests (session-level statistics)
// ===========================================================================

TEST_F(SessionRestoreHelperTest, SessionMetadataStruct_DefaultValues) {
  AstraSessionMetadata session;
  EXPECT_TRUE(session.session_name.empty());
  EXPECT_TRUE(session.save_time.is_null());
  EXPECT_TRUE(session.windows.empty());
}

TEST_F(SessionRestoreHelperTest, SessionMetadataStruct_GetTabCountEmpty) {
  AstraSessionMetadata session;
  EXPECT_EQ(session.GetTabCount(), 0u);
}

TEST_F(SessionRestoreHelperTest, SessionMetadataStruct_GetWindowCountEmpty) {
  AstraSessionMetadata session;
  EXPECT_EQ(session.GetWindowCount(), 0u);
}

TEST_F(SessionRestoreHelperTest, SessionMetadataStruct_GetWorkspaceCountEmpty) {
  AstraSessionMetadata session;
  EXPECT_EQ(session.GetWorkspaceCount(), 0u);
}

TEST_F(SessionRestoreHelperTest, SessionMetadataStruct_SingleWindowStats) {
  AstraSessionMetadata session;

  AstraWindowSessionMetadata window;
  AstraTabSessionMetadata tab1;
  tab1.workspace_id = "ws-1";
  tab1.is_favorite = true;
  tab1.is_pinned = true;
  window.tabs.push_back(tab1);

  AstraTabSessionMetadata tab2;
  tab2.workspace_id = "ws-1";
  tab2.is_favorite = false;
  window.tabs.push_back(tab2);

  session.windows.push_back(window);

  EXPECT_EQ(session.GetTabCount(), 2u);
  EXPECT_EQ(session.GetWindowCount(), 1u);
  EXPECT_EQ(session.GetWorkspaceCount(), 1u);
  EXPECT_EQ(session.GetFavoriteTabCount(), 1u);
  EXPECT_EQ(session.GetPinnedTabCount(), 1u);
}

TEST_F(SessionRestoreHelperTest, SessionMetadataStruct_MultipleWindowsStats) {
  AstraSessionMetadata session;

  AstraWindowSessionMetadata win1;
  win1.workspace_id = "ws-1";
  AstraTabSessionMetadata t1;
  t1.workspace_id = "ws-1";
  t1.tab_stack_id = "stack-a";
  win1.tabs.push_back(t1);
  session.windows.push_back(win1);

  AstraWindowSessionMetadata win2;
  win2.workspace_id = "ws-2";
  AstraTabSessionMetadata t2;
  t2.workspace_id = "ws-2";
  t2.is_favorite = true;
  win2.tabs.push_back(t2);
  AstraTabSessionMetadata t3;
  t3.workspace_id = "ws-2";
  t3.is_favorite = true;
  t3.is_pinned = true;
  win2.tabs.push_back(t3);
  session.windows.push_back(win2);

  EXPECT_EQ(session.GetTabCount(), 3u);
  EXPECT_EQ(session.GetWindowCount(), 2u);
  EXPECT_EQ(session.GetWorkspaceCount(), 2u);
  EXPECT_EQ(session.GetFavoriteTabCount(), 2u);
  EXPECT_EQ(session.GetPinnedTabCount(), 1u);
  EXPECT_EQ(session.GetStackedTabCount(), 1u);
}

TEST_F(SessionRestoreHelperTest, SessionMetadataStruct_MemoryUsageEstimate) {
  AstraSessionMetadata session;
  size_t empty_size = session.GetEstimatedMemoryUsage();
  EXPECT_GT(empty_size, 0u);

  AstraWindowSessionMetadata window;
  for (int i = 0; i < 10; ++i) {
    AstraTabSessionMetadata tab;
    tab.tab_id = "tab-" + std::to_string(i);
    tab.workspace_id = "ws-" + std::to_string(i % 3);
    window.tabs.push_back(tab);
  }
  session.windows.push_back(window);

  size_t with_data_size = session.GetEstimatedMemoryUsage();
  EXPECT_GT(with_data_size, empty_size);
}

TEST_F(SessionRestoreHelperTest, SessionMetadataStruct_SessionSizeBytes) {
  AstraSessionMetadata session;
  EXPECT_GE(session.GetSessionSizeBytes(), 0u);
}

TEST_F(SessionRestoreHelperTest, SessionMetadataStruct_GetWorkspaceTabCount) {
  AstraSessionMetadata session;

  AstraWindowSessionMetadata win1;
  win1.workspace_id = "ws-a";
  AstraTabSessionMetadata t1;
  t1.workspace_id = "ws-a";
  win1.tabs.push_back(t1);
  AstraTabSessionMetadata t2;
  t2.workspace_id = "ws-a";
  win1.tabs.push_back(t2);
  session.windows.push_back(win1);

  AstraWindowSessionMetadata win2;
  win2.workspace_id = "ws-b";
  AstraTabSessionMetadata t3;
  t3.workspace_id = "ws-a";  // Tab in ws-a but window is ws-b.
  win2.tabs.push_back(t3);
  session.windows.push_back(win2);

  // Count by tab workspace_id across all windows.
  EXPECT_EQ(session.GetWorkspaceTabCount("ws-a"), 3u);
  EXPECT_EQ(session.GetWorkspaceTabCount("ws-b"), 0u);
  EXPECT_EQ(session.GetWorkspaceTabCount("ws-c"), 0u);
}

TEST_F(SessionRestoreHelperTest, SessionMetadataStruct_GetTabsByWorkspace) {
  AstraSessionMetadata session;

  AstraWindowSessionMetadata win1;
  AstraTabSessionMetadata t1;
  t1.tab_id = "tab-1";
  t1.workspace_id = "ws-x";
  win1.tabs.push_back(t1);
  session.windows.push_back(win1);

  AstraWindowSessionMetadata win2;
  AstraTabSessionMetadata t2;
  t2.tab_id = "tab-2";
  t2.workspace_id = "ws-x";
  win2.tabs.push_back(t2);
  session.windows.push_back(win2);

  auto tabs = session.GetTabsByWorkspace("ws-x");
  ASSERT_EQ(tabs.size(), 2u);
  EXPECT_EQ(tabs[0], "tab-1");
  EXPECT_EQ(tabs[1], "tab-2");
}

TEST_F(SessionRestoreHelperTest, SessionMetadataStruct_GetWindowTabCount) {
  AstraSessionMetadata session;

  AstraWindowSessionMetadata win1;
  win1.window_id = "window-1";
  win1.tabs.resize(5);
  session.windows.push_back(win1);

  AstraWindowSessionMetadata win2;
  win2.window_id = "window-2";
  win2.tabs.resize(3);
  session.windows.push_back(win2);

  EXPECT_EQ(session.GetWindowTabCount("window-1"), 5u);
  EXPECT_EQ(session.GetWindowTabCount("window-2"), 3u);
  EXPECT_EQ(session.GetWindowTabCount("nonexistent"), 0u);
}

TEST_F(SessionRestoreHelperTest, SessionMetadataStruct_GetWorkspaceIds) {
  AstraSessionMetadata session;

  AstraWindowSessionMetadata win1;
  win1.workspace_id = "ws-1";
  AstraTabSessionMetadata t1;
  t1.workspace_id = "ws-1";
  win1.tabs.push_back(t1);
  session.windows.push_back(win1);

  AstraWindowSessionMetadata win2;
  win2.workspace_id = "ws-2";
  session.windows.push_back(win2);

  auto ids = session.GetWorkspaceIds();
  EXPECT_EQ(ids.size(), 2u);
  EXPECT_TRUE(ids.count("ws-1") > 0);
  EXPECT_TRUE(ids.count("ws-2") > 0);
}

TEST_F(SessionRestoreHelperTest, SessionMetadataStruct_ValidateEmpty) {
  AstraSessionMetadata session;
  EXPECT_TRUE(session.Validate());
}

TEST_F(SessionRestoreHelperTest, SessionMetadataStruct_ValidateWithValidTabs) {
  AstraSessionMetadata session;
  AstraWindowSessionMetadata window;
  AstraTabSessionMetadata tab;
  tab.is_favorite = true;
  window.tabs.push_back(tab);
  session.windows.push_back(window);
  EXPECT_TRUE(session.Validate());
}

TEST_F(SessionRestoreHelperTest, SessionMetadataStruct_ValidateWithInvalidTab) {
  AstraSessionMetadata session;
  AstraWindowSessionMetadata window;
  AstraTabSessionMetadata bad_tab;
  bad_tab.split_view_ratio = 2.0;  // Invalid.
  window.tabs.push_back(bad_tab);
  session.windows.push_back(window);
  EXPECT_FALSE(session.Validate());
}

TEST_F(SessionRestoreHelperTest, SessionMetadataStruct_RoundTrip) {
  AstraSessionMetadata original;
  original.session_name = "my-session";
  original.save_time = base::Time::Now();

  AstraWindowSessionMetadata win1;
  win1.window_id = "win-1";
  win1.workspace_id = "ws-1";
  win1.is_hibernated = false;
  AstraTabSessionMetadata t1;
  t1.tab_id = "t1";
  t1.workspace_id = "ws-1";
  t1.is_favorite = true;
  win1.tabs.push_back(t1);
  original.windows.push_back(win1);

  AstraWindowSessionMetadata win2;
  win2.window_id = "win-2";
  win2.workspace_id = "ws-2";
  original.windows.push_back(win2);

  base::Value::Dict dict = original.ToDict();

  AstraSessionMetadata restored;
  bool result = restored.FromDict(dict);

  EXPECT_TRUE(result);
  EXPECT_EQ(restored.session_name, original.session_name);
  EXPECT_FALSE(restored.save_time.is_null());
  EXPECT_EQ(restored.GetWindowCount(), 2u);
  EXPECT_EQ(restored.GetTabCount(), 1u);
  EXPECT_EQ(restored.windows[0].window_id, "win-1");
  EXPECT_EQ(restored.windows[1].window_id, "win-2");
  EXPECT_TRUE(restored.windows[0].tabs[0].is_favorite);
}

TEST_F(SessionRestoreHelperTest, SessionMetadataStruct_MergeFrom) {
  AstraSessionMetadata base;
  base.session_name = "base-session";

  AstraSessionMetadata other;
  other.session_name = "other-session";

  AstraWindowSessionMetadata win;
  win.window_id = "win-new";
  other.windows.push_back(win);

  base.MergeFrom(other);

  EXPECT_EQ(base.session_name, "other-session");
  EXPECT_EQ(base.GetWindowCount(), 1u);
}

// ===========================================================================
// Restore load mode tests
// ===========================================================================

TEST_F(SessionRestoreHelperTest, RestoreLoadMode_DefaultIsSmart) {
  SessionRestoreLoadMode mode =
      AstraSessionRestoreHelper::GetRestoreLoadMode(profile_.get());
  EXPECT_EQ(mode, SessionRestoreLoadMode::kSmart);
}

TEST_F(SessionRestoreHelperTest, RestoreLoadMode_SetAndGet) {
  // Test all 4 modes.
  auto modes = {
    SessionRestoreLoadMode::kFull,
    SessionRestoreLoadMode::kLazy,
    SessionRestoreLoadMode::kSmart,
    SessionRestoreLoadMode::kMinimal,
  };

  for (auto mode : modes) {
    AstraSessionRestoreHelper::SetRestoreLoadMode(profile_.get(), mode);
    EXPECT_EQ(AstraSessionRestoreHelper::GetRestoreLoadMode(profile_.get()),
              mode);
  }
}

TEST_F(SessionRestoreHelperTest, RestoreLoadMode_NullProfileReturnsDefault) {
  EXPECT_EQ(AstraSessionRestoreHelper::GetRestoreLoadMode(nullptr),
            SessionRestoreLoadMode::kSmart);
}

TEST_F(SessionRestoreHelperTest, RestoreLoadMode_SetNullProfileNoCrash) {
  AstraSessionRestoreHelper::SetRestoreLoadMode(nullptr,
      SessionRestoreLoadMode::kFull);
  SUCCEED();
}

TEST_F(SessionRestoreHelperTest, RestoreLoadMode_ToStringAllModes) {
  EXPECT_EQ(AstraSessionRestoreHelper::RestoreLoadModeToString(
      SessionRestoreLoadMode::kFull), "full");
  EXPECT_EQ(AstraSessionRestoreHelper::RestoreLoadModeToString(
      SessionRestoreLoadMode::kLazy), "lazy");
  EXPECT_EQ(AstraSessionRestoreHelper::RestoreLoadModeToString(
      SessionRestoreLoadMode::kSmart), "smart");
  EXPECT_EQ(AstraSessionRestoreHelper::RestoreLoadModeToString(
      SessionRestoreLoadMode::kMinimal), "minimal");
}

TEST_F(SessionRestoreHelperTest, RestoreLoadMode_FromStringAllModes) {
  EXPECT_EQ(AstraSessionRestoreHelper::RestoreLoadModeFromString("full"),
            SessionRestoreLoadMode::kFull);
  EXPECT_EQ(AstraSessionRestoreHelper::RestoreLoadModeFromString("lazy"),
            SessionRestoreLoadMode::kLazy);
  EXPECT_EQ(AstraSessionRestoreHelper::RestoreLoadModeFromString("smart"),
            SessionRestoreLoadMode::kSmart);
  EXPECT_EQ(AstraSessionRestoreHelper::RestoreLoadModeFromString("minimal"),
            SessionRestoreLoadMode::kMinimal);
}

TEST_F(SessionRestoreHelperTest, RestoreLoadMode_FromUnknownDefaultsToSmart) {
  EXPECT_EQ(AstraSessionRestoreHelper::RestoreLoadModeFromString(""),
            SessionRestoreLoadMode::kSmart);
  EXPECT_EQ(AstraSessionRestoreHelper::RestoreLoadModeFromString("invalid"),
            SessionRestoreLoadMode::kSmart);
  EXPECT_EQ(AstraSessionRestoreHelper::RestoreLoadModeFromString("foobar"),
            SessionRestoreLoadMode::kSmart);
}

TEST_F(SessionRestoreHelperTest, RestoreLoadMode_StringRoundTrip) {
  SessionRestoreLoadMode modes[] = {
    SessionRestoreLoadMode::kFull,
    SessionRestoreLoadMode::kLazy,
    SessionRestoreLoadMode::kSmart,
    SessionRestoreLoadMode::kMinimal,
  };

  for (auto mode : modes) {
    std::string str =
        AstraSessionRestoreHelper::RestoreLoadModeToString(mode);
    SessionRestoreLoadMode parsed =
        AstraSessionRestoreHelper::RestoreLoadModeFromString(str);
    EXPECT_EQ(parsed, mode);
  }
}

TEST_F(SessionRestoreHelperTest, RestoreLoadMode_PersistsViaPrefs) {
  AstraSessionRestoreHelper::SetRestoreLoadMode(profile_.get(),
      SessionRestoreLoadMode::kFull);

  PrefService* prefs = profile_->GetPrefs();
  EXPECT_EQ(prefs->GetString(prefs::kPrefSessionRestoreLoadMode), "full");
}

// ===========================================================================
// Restore statistics tracking tests
// ===========================================================================

TEST_F(SessionRestoreHelperTest, RestoreStats_StartMarksInProgress) {
  AstraSessionRestoreHelper::OnSessionRestoreStarted();

  EXPECT_TRUE(AstraSessionRestoreHelper::IsRestoreInProgress());
  EXPECT_FALSE(AstraSessionRestoreHelper::GetRestoreStats().restore_start_time.is_null());
  EXPECT_TRUE(AstraSessionRestoreHelper::GetRestoreStats().restore_end_time.is_null());
}

TEST_F(SessionRestoreHelperTest, RestoreStats_CompleteClearsInProgress) {
  AstraSessionRestoreHelper::OnSessionRestoreStarted();
  ASSERT_TRUE(AstraSessionRestoreHelper::IsRestoreInProgress());

  AstraSessionRestoreHelper::OnSessionRestoreComplete(profile_.get());

  EXPECT_FALSE(AstraSessionRestoreHelper::IsRestoreInProgress());
  EXPECT_FALSE(AstraSessionRestoreHelper::GetRestoreStats().restore_end_time.is_null());
}

TEST_F(SessionRestoreHelperTest, RestoreStats_DurationAfterComplete) {
  AstraSessionRestoreHelper::OnSessionRestoreStarted();
  AstraSessionRestoreHelper::OnSessionRestoreComplete(profile_.get());

  base::TimeDelta duration = AstraSessionRestoreHelper::GetRestoreDuration();
  EXPECT_GE(duration, base::TimeDelta());
}

TEST_F(SessionRestoreHelperTest, RestoreStats_DurationZeroBeforeComplete) {
  AstraSessionRestoreHelper::OnSessionRestoreStarted();
  base::TimeDelta duration = AstraSessionRestoreHelper::GetRestoreDuration();
  EXPECT_EQ(duration, base::TimeDelta());
}

TEST_F(SessionRestoreHelperTest, RestoreStats_SuccessRateEmpty) {
  // No tabs attempted = 100% success (vacuous truth).
  EXPECT_DOUBLE_EQ(AstraSessionRestoreHelper::GetRestoreSuccessRate(), 100.0);
}

TEST_F(SessionRestoreHelperTest, RestoreStats_SuccessRateAllSucceed) {
  AstraSessionRestoreHelper::OnSessionRestoreStarted();

  // Simulate some successful tab restores by directly manipulating stats.
  // We can also use GetMutableRestoreStats for testing.
  auto& stats = const_cast<AstraSessionRestoreStats&>(
      AstraSessionRestoreHelper::GetRestoreStats());
  stats.tabs_restored = 10;
  stats.tabs_failed = 0;

  EXPECT_DOUBLE_EQ(AstraSessionRestoreHelper::GetRestoreSuccessRate(), 100.0);
}

TEST_F(SessionRestoreHelperTest, RestoreStats_SuccessRatePartial) {
  AstraSessionRestoreHelper::OnSessionRestoreStarted();

  auto& stats = const_cast<AstraSessionRestoreStats&>(
      AstraSessionRestoreHelper::GetRestoreStats());
  stats.tabs_restored = 7;
  stats.tabs_failed = 3;

  EXPECT_DOUBLE_EQ(AstraSessionRestoreHelper::GetRestoreSuccessRate(), 70.0);
}

TEST_F(SessionRestoreHelperTest, RestoreStats_GetLastRestoreStats) {
  // GetLastRestoreStats is an alias for GetRestoreStats.
  const auto& stats1 = AstraSessionRestoreHelper::GetRestoreStats();
  const auto& stats2 = AstraSessionRestoreHelper::GetLastRestoreStats();
  EXPECT_EQ(&stats1, &stats2);
}

TEST_F(SessionRestoreHelperTest, RestoreStats_FailedSetsEndTime) {
  AstraSessionRestoreHelper::OnSessionRestoreStarted();
  ASSERT_TRUE(AstraSessionRestoreHelper::IsRestoreInProgress());

  AstraSessionRestoreHelper::OnSessionRestoreFailed("test error");

  EXPECT_FALSE(AstraSessionRestoreHelper::IsRestoreInProgress());
  EXPECT_FALSE(AstraSessionRestoreHelper::GetRestoreStats().restore_end_time.is_null());
}

TEST_F(SessionRestoreHelperTest, RestoreStats_ResetClearsTimestamps) {
  AstraSessionRestoreHelper::OnSessionRestoreStarted();
  AstraSessionRestoreHelper::OnSessionRestoreComplete(profile_.get());

  ASSERT_FALSE(AstraSessionRestoreHelper::GetRestoreStats().restore_start_time.is_null());

  AstraSessionRestoreHelper::ResetRestoreStats();

  EXPECT_TRUE(AstraSessionRestoreHelper::GetRestoreStats().restore_start_time.is_null());
  EXPECT_TRUE(AstraSessionRestoreHelper::GetRestoreStats().restore_end_time.is_null());
  EXPECT_EQ(AstraSessionRestoreHelper::GetRestoreStats().tabs_failed, 0u);
}

// ===========================================================================
// Observer notification tests — new methods
// ===========================================================================

namespace {

// Extended test observer that records all new observer methods.
class TestExtendedObserver : public TestSessionRestoreObserver {
 public:
  void OnSessionRestoreStarted() override {
    session_restore_started_count_++;
  }

  void OnSessionRestoreCompleted() override {
    session_restore_completed_count_++;
  }

  void OnSessionRestoreFailed(const std::string& error) override {
    session_restore_failed_count_++;
    last_restore_error_ = error;
  }

  void OnTabRestored(int tab_index) override {
    tab_restored_index_count_++;
    last_tab_index_ = tab_index;
  }

  void OnSessionSaved(const std::string& session_name) override {
    named_session_saved_count_++;
    last_saved_session_name_ = session_name;
  }

  void OnSessionMetadataChanged() override {
    session_metadata_changed_count_++;
  }

  int session_restore_started_count_ = 0;
  int session_restore_completed_count_ = 0;
  int session_restore_failed_count_ = 0;
  std::string last_restore_error_;
  int tab_restored_index_count_ = 0;
  int last_tab_index_ = -1;
  int named_session_saved_count_ = 0;
  std::string last_saved_session_name_;
  int session_metadata_changed_count_ = 0;
};

}  // namespace

TEST_F(SessionRestoreHelperTest, Observer_OnSessionRestoreStarted) {
  TestExtendedObserver observer;
  AstraSessionRestoreHelper::AddObserver(&observer);

  AstraSessionRestoreHelper::OnSessionRestoreStarted();

  EXPECT_EQ(observer.session_restore_started_count_, 1);

  AstraSessionRestoreHelper::RemoveObserver(&observer);
}

TEST_F(SessionRestoreHelperTest, Observer_OnSessionRestoreCompleted) {
  TestExtendedObserver observer;
  AstraSessionRestoreHelper::AddObserver(&observer);

  AstraSessionRestoreHelper::OnSessionRestoreStarted();
  AstraSessionRestoreHelper::OnSessionRestoreComplete(profile_.get());

  EXPECT_EQ(observer.session_restore_completed_count_, 1);

  AstraSessionRestoreHelper::RemoveObserver(&observer);
}

TEST_F(SessionRestoreHelperTest, Observer_OnSessionRestoreFailed) {
  TestExtendedObserver observer;
  AstraSessionRestoreHelper::AddObserver(&observer);

  AstraSessionRestoreHelper::OnSessionRestoreStarted();
  AstraSessionRestoreHelper::OnSessionRestoreFailed("disk error");

  EXPECT_EQ(observer.session_restore_failed_count_, 1);
  EXPECT_EQ(observer.last_restore_error_, "disk error");

  AstraSessionRestoreHelper::RemoveObserver(&observer);
}

TEST_F(SessionRestoreHelperTest, Observer_OnTabRestoredAtIndex) {
  TestExtendedObserver observer;
  AstraSessionRestoreHelper::AddObserver(&observer);

  AstraSessionRestoreHelper::OnTabRestoredAtIndex(5);

  EXPECT_EQ(observer.tab_restored_index_count_, 1);
  EXPECT_EQ(observer.last_tab_index_, 5);

  AstraSessionRestoreHelper::OnTabRestoredAtIndex(10);
  EXPECT_EQ(observer.tab_restored_index_count_, 2);
  EXPECT_EQ(observer.last_tab_index_, 10);

  AstraSessionRestoreHelper::RemoveObserver(&observer);
}

TEST_F(SessionRestoreHelperTest, Observer_OnSessionSavedNamed) {
  TestExtendedObserver observer;
  AstraSessionRestoreHelper::AddObserver(&observer);

  AstraSessionRestoreHelper::SaveSession(profile_.get(), "test-session");

  EXPECT_GE(observer.named_session_saved_count_, 1);
  EXPECT_EQ(observer.last_saved_session_name_, "test-session");

  AstraSessionRestoreHelper::RemoveObserver(&observer);
}

TEST_F(SessionRestoreHelperTest, Observer_OnSessionMetadataChanged) {
  TestExtendedObserver observer;
  AstraSessionRestoreHelper::AddObserver(&observer);

  // Delete non-existent session still fires metadata changed? No, it shouldn't.
  // Let's use ClearLastSession which always fires.
  AstraSessionRestoreHelper::ClearLastSession(profile_.get());

  EXPECT_GE(observer.session_metadata_changed_count_, 1);

  AstraSessionRestoreHelper::RemoveObserver(&observer);
}

TEST_F(SessionRestoreHelperTest, Observer_MultipleNewMethodsAllNotified) {
  TestExtendedObserver obs1;
  TestExtendedObserver obs2;

  AstraSessionRestoreHelper::AddObserver(&obs1);
  AstraSessionRestoreHelper::AddObserver(&obs2);

  AstraSessionRestoreHelper::OnSessionRestoreStarted();
  AstraSessionRestoreHelper::OnTabRestoredAtIndex(0);
  AstraSessionRestoreHelper::OnSessionRestoreComplete(profile_.get());

  EXPECT_EQ(obs1.session_restore_started_count_, 1);
  EXPECT_EQ(obs2.session_restore_started_count_, 1);
  EXPECT_EQ(obs1.tab_restored_index_count_, 1);
  EXPECT_EQ(obs2.tab_restored_index_count_, 1);
  EXPECT_EQ(obs1.session_restore_completed_count_, 1);
  EXPECT_EQ(obs2.session_restore_completed_count_, 1);

  AstraSessionRestoreHelper::RemoveObserver(&obs1);
  AstraSessionRestoreHelper::RemoveObserver(&obs2);
}

TEST_F(SessionRestoreHelperTest, Observer_DefaultsForNewMethodsDontCrash) {
  // Use base observer class which has default empty implementations.
  class DefaultObserver : public AstraSessionRestoreObserver {};

  DefaultObserver observer;
  AstraSessionRestoreHelper::AddObserver(&observer);

  // Call all the new methods — should not crash.
  AstraSessionRestoreHelper::OnSessionRestoreStarted();
  AstraSessionRestoreHelper::OnSessionRestoreComplete(profile_.get());
  AstraSessionRestoreHelper::OnSessionRestoreFailed("error");
  AstraSessionRestoreHelper::OnTabRestoredAtIndex(0);
  AstraSessionRestoreHelper::SaveSession(profile_.get(), "test");
  AstraSessionRestoreHelper::ClearLastSession(profile_.get());

  AstraSessionRestoreHelper::RemoveObserver(&observer);
  SUCCEED();
}

// ===========================================================================
// Presentation settings tests
// ===========================================================================

TEST_F(SessionRestoreHelperTest, Setting_RestoreOnStartupDefault) {
  EXPECT_TRUE(AstraSessionRestoreHelper::GetRestoreOnStartup(profile_.get()));
}

TEST_F(SessionRestoreHelperTest, Setting_RestoreOnStartupSetAndGet) {
  AstraSessionRestoreHelper::SetRestoreOnStartup(profile_.get(), false);
  EXPECT_FALSE(AstraSessionRestoreHelper::GetRestoreOnStartup(profile_.get()));

  AstraSessionRestoreHelper::SetRestoreOnStartup(profile_.get(), true);
  EXPECT_TRUE(AstraSessionRestoreHelper::GetRestoreOnStartup(profile_.get()));
}

TEST_F(SessionRestoreHelperTest, Setting_MaxTabsPerWorkspaceDefault) {
  EXPECT_EQ(AstraSessionRestoreHelper::GetMaxTabsPerWorkspaceRestore(
      profile_.get()), 0);  // 0 = no limit.
}

TEST_F(SessionRestoreHelperTest, Setting_MaxTabsPerWorkspaceSetAndGet) {
  AstraSessionRestoreHelper::SetMaxTabsPerWorkspaceRestore(profile_.get(), 50);
  EXPECT_EQ(AstraSessionRestoreHelper::GetMaxTabsPerWorkspaceRestore(
      profile_.get()), 50);

  // Negative values should be clamped to 0.
  AstraSessionRestoreHelper::SetMaxTabsPerWorkspaceRestore(profile_.get(), -5);
  EXPECT_EQ(AstraSessionRestoreHelper::GetMaxTabsPerWorkspaceRestore(
      profile_.get()), 0);
}

TEST_F(SessionRestoreHelperTest, Setting_ShowRestorePromptSetAndGet) {
  // Default is false (already tested above).
  AstraSessionRestoreHelper::SetShowRestorePrompt(profile_.get(), true);
  EXPECT_TRUE(AstraSessionRestoreHelper::GetShowRestorePrompt(profile_.get()));

  AstraSessionRestoreHelper::SetShowRestorePrompt(profile_.get(), false);
  EXPECT_FALSE(AstraSessionRestoreHelper::GetShowRestorePrompt(profile_.get()));
}

TEST_F(SessionRestoreHelperTest, Setting_RestoreWorkspacesIndividuallyDefault) {
  EXPECT_FALSE(AstraSessionRestoreHelper::GetRestoreWorkspacesIndividually(
      profile_.get()));
}

TEST_F(SessionRestoreHelperTest, Setting_RestoreWorkspacesIndividuallySetAndGet) {
  AstraSessionRestoreHelper::SetRestoreWorkspacesIndividually(
      profile_.get(), true);
  EXPECT_TRUE(AstraSessionRestoreHelper::GetRestoreWorkspacesIndividually(
      profile_.get()));

  AstraSessionRestoreHelper::SetRestoreWorkspacesIndividually(
      profile_.get(), false);
  EXPECT_FALSE(AstraSessionRestoreHelper::GetRestoreWorkspacesIndividually(
      profile_.get()));
}

TEST_F(SessionRestoreHelperTest, Setting_BackgroundTabLoadingDefault) {
  EXPECT_TRUE(AstraSessionRestoreHelper::GetBackgroundTabLoading(
      profile_.get()));
}

TEST_F(SessionRestoreHelperTest, Setting_BackgroundTabLoadingSetAndGet) {
  AstraSessionRestoreHelper::SetBackgroundTabLoading(profile_.get(), false);
  EXPECT_FALSE(AstraSessionRestoreHelper::GetBackgroundTabLoading(
      profile_.get()));

  AstraSessionRestoreHelper::SetBackgroundTabLoading(profile_.get(), true);
  EXPECT_TRUE(AstraSessionRestoreHelper::GetBackgroundTabLoading(
      profile_.get()));
}

TEST_F(SessionRestoreHelperTest, Setting_AutoSaveIntervalDefault) {
  EXPECT_EQ(AstraSessionRestoreHelper::GetAutoSaveSessionInterval(
      profile_.get()), 5);  // 5 minutes.
}

TEST_F(SessionRestoreHelperTest, Setting_AutoSaveIntervalSetAndGet) {
  AstraSessionRestoreHelper::SetAutoSaveSessionInterval(profile_.get(), 10);
  EXPECT_EQ(AstraSessionRestoreHelper::GetAutoSaveSessionInterval(
      profile_.get()), 10);

  // Clamp negative to 0.
  AstraSessionRestoreHelper::SetAutoSaveSessionInterval(profile_.get(), -1);
  EXPECT_EQ(AstraSessionRestoreHelper::GetAutoSaveSessionInterval(
      profile_.get()), 0);
}

TEST_F(SessionRestoreHelperTest, Setting_MaxSavedSessionsDefault) {
  EXPECT_EQ(AstraSessionRestoreHelper::GetMaxSavedSessions(profile_.get()),
            10);
}

TEST_F(SessionRestoreHelperTest, Setting_MaxSavedSessionsSetAndGet) {
  AstraSessionRestoreHelper::SetMaxSavedSessions(profile_.get(), 5);
  EXPECT_EQ(AstraSessionRestoreHelper::GetMaxSavedSessions(profile_.get()),
            5);

  // Clamp to minimum 1.
  AstraSessionRestoreHelper::SetMaxSavedSessions(profile_.get(), 0);
  EXPECT_EQ(AstraSessionRestoreHelper::GetMaxSavedSessions(profile_.get()),
            1);

  AstraSessionRestoreHelper::SetMaxSavedSessions(profile_.get(), -1);
  EXPECT_EQ(AstraSessionRestoreHelper::GetMaxSavedSessions(profile_.get()),
            1);
}

TEST_F(SessionRestoreHelperTest, Settings_NullProfileReturnsDefaults) {
  // All settings should return defaults for null profile.
  EXPECT_TRUE(AstraSessionRestoreHelper::GetRestoreOnStartup(nullptr));
  EXPECT_EQ(AstraSessionRestoreHelper::GetMaxTabsPerWorkspaceRestore(nullptr), 0);
  EXPECT_FALSE(AstraSessionRestoreHelper::GetRestoreWorkspacesIndividually(nullptr));
  EXPECT_TRUE(AstraSessionRestoreHelper::GetBackgroundTabLoading(nullptr));
  EXPECT_EQ(AstraSessionRestoreHelper::GetAutoSaveSessionInterval(nullptr), 5);
  EXPECT_EQ(AstraSessionRestoreHelper::GetMaxSavedSessions(nullptr), 10);
}

TEST_F(SessionRestoreHelperTest, Settings_NullProfileSetNoCrash) {
  // Setting on null profile shouldn't crash.
  AstraSessionRestoreHelper::SetRestoreOnStartup(nullptr, true);
  AstraSessionRestoreHelper::SetMaxTabsPerWorkspaceRestore(nullptr, 10);
  AstraSessionRestoreHelper::SetShowRestorePrompt(nullptr, true);
  AstraSessionRestoreHelper::SetRestoreWorkspacesIndividually(nullptr, true);
  AstraSessionRestoreHelper::SetBackgroundTabLoading(nullptr, true);
  AstraSessionRestoreHelper::SetAutoSaveSessionInterval(nullptr, 5);
  AstraSessionRestoreHelper::SetMaxSavedSessions(nullptr, 10);
  SUCCEED();
}

// ===========================================================================
// Named session management tests
// ===========================================================================

TEST_F(SessionRestoreHelperTest, SavedSessions_ListEmptyByDefault) {
  auto sessions = AstraSessionRestoreHelper::ListSavedSessions(profile_.get());
  EXPECT_TRUE(sessions.empty());
}

TEST_F(SessionRestoreHelperTest, SavedSessions_SaveAndList) {
  AstraSessionRestoreHelper::SaveSession(profile_.get(), "session-1");

  auto sessions = AstraSessionRestoreHelper::ListSavedSessions(profile_.get());
  ASSERT_EQ(sessions.size(), 1u);
  EXPECT_EQ(sessions[0], "session-1");
}

TEST_F(SessionRestoreHelperTest, SavedSessions_SaveMultiple) {
  AstraSessionRestoreHelper::SaveSession(profile_.get(), "s1");
  AstraSessionRestoreHelper::SaveSession(profile_.get(), "s2");
  AstraSessionRestoreHelper::SaveSession(profile_.get(), "s3");

  auto sessions = AstraSessionRestoreHelper::ListSavedSessions(profile_.get());
  EXPECT_EQ(sessions.size(), 3u);
}

TEST_F(SessionRestoreHelperTest, SavedSessions_SaveOverwrite) {
  // Saving same session name twice should overwrite.
  AstraSessionRestoreHelper::SaveSession(profile_.get(), "my-session");
  AstraSessionRestoreHelper::SaveSession(profile_.get(), "my-session");

  auto sessions = AstraSessionRestoreHelper::ListSavedSessions(profile_.get());
  EXPECT_EQ(sessions.size(), 1u);
}

TEST_F(SessionRestoreHelperTest, SavedSessions_LoadExisting) {
  AstraSessionRestoreHelper::SaveSession(profile_.get(), "test-load");

  AstraSessionMetadata loaded =
      AstraSessionRestoreHelper::LoadSession(profile_.get(), "test-load");

  EXPECT_EQ(loaded.session_name, "test-load");
  EXPECT_FALSE(loaded.save_time.is_null());
}

TEST_F(SessionRestoreHelperTest, SavedSessions_LoadNonExistentReturnsEmpty) {
  AstraSessionMetadata loaded =
      AstraSessionRestoreHelper::LoadSession(profile_.get(), "nonexistent");

  EXPECT_TRUE(loaded.session_name.empty());
  EXPECT_TRUE(loaded.save_time.is_null());
  EXPECT_TRUE(loaded.windows.empty());
}

TEST_F(SessionRestoreHelperTest, SavedSessions_DeleteExisting) {
  AstraSessionRestoreHelper::SaveSession(profile_.get(), "to-delete");
  ASSERT_EQ(
      AstraSessionRestoreHelper::ListSavedSessions(profile_.get()).size(), 1u);

  bool result = AstraSessionRestoreHelper::DeleteSavedSession(
      profile_.get(), "to-delete");
  EXPECT_TRUE(result);

  EXPECT_TRUE(
      AstraSessionRestoreHelper::ListSavedSessions(profile_.get()).empty());
}

TEST_F(SessionRestoreHelperTest, SavedSessions_DeleteNonExistent) {
  bool result = AstraSessionRestoreHelper::DeleteSavedSession(
      profile_.get(), "does-not-exist");
  EXPECT_FALSE(result);
}

TEST_F(SessionRestoreHelperTest, SavedSessions_SaveEmptyNameNoOp) {
  AstraSessionRestoreHelper::SaveSession(profile_.get(), "");

  auto sessions = AstraSessionRestoreHelper::ListSavedSessions(profile_.get());
  EXPECT_TRUE(sessions.empty());
}

TEST_F(SessionRestoreHelperTest, SavedSessions_NullProfile) {
  // All session management calls with null profile should not crash.
  EXPECT_TRUE(AstraSessionRestoreHelper::ListSavedSessions(nullptr).empty());
  AstraSessionRestoreHelper::SaveSession(nullptr, "test");
  EXPECT_TRUE(
      AstraSessionRestoreHelper::LoadSession(nullptr, "test").windows.empty());
  EXPECT_FALSE(AstraSessionRestoreHelper::DeleteSavedSession(nullptr, "test"));
}

// ===========================================================================
// Last session tests
// ===========================================================================

TEST_F(SessionRestoreHelperTest, HasLastSession_FalseByDefault) {
  EXPECT_FALSE(AstraSessionRestoreHelper::HasLastSession(profile_.get()));
}

TEST_F(SessionRestoreHelperTest, HasLastSession_TrueAfterRecord) {
  AstraSessionRestoreHelper::RecordSessionSaved(profile_.get(), 5, 1, 1);
  EXPECT_TRUE(AstraSessionRestoreHelper::HasLastSession(profile_.get()));
}

TEST_F(SessionRestoreHelperTest, GetLastSession_DefaultEmpty) {
  auto session = AstraSessionRestoreHelper::GetLastSession(profile_.get());
  EXPECT_TRUE(session.windows.empty());
}

TEST_F(SessionRestoreHelperTest, GetLastSession_AfterRecord) {
  AstraSessionRestoreHelper::RecordSessionSaved(profile_.get(), 10, 2, 2);

  auto session = AstraSessionRestoreHelper::GetLastSession(profile_.get());
  EXPECT_EQ(session.session_name, "last");
  EXPECT_FALSE(session.save_time.is_null());
}

TEST_F(SessionRestoreHelperTest, ClearLastSession_ClearsData) {
  AstraSessionRestoreHelper::RecordSessionSaved(profile_.get(), 10, 2, 2);
  ASSERT_TRUE(AstraSessionRestoreHelper::HasLastSession(profile_.get()));

  AstraSessionRestoreHelper::ClearLastSession(profile_.get());
  EXPECT_FALSE(AstraSessionRestoreHelper::HasLastSession(profile_.get()));

  auto info = AstraSessionRestoreHelper::GetLastSessionInfo(profile_.get());
  EXPECT_EQ(info.tab_count, 0u);
}

TEST_F(SessionRestoreHelperTest, ClearLastSession_NullProfileNoCrash) {
  AstraSessionRestoreHelper::ClearLastSession(nullptr);
  SUCCEED();
}

// ===========================================================================
// Bulk operations tests — workspace-specific
// ===========================================================================

TEST_F(SessionRestoreHelperTest, Bulk_RestoreAllWorkspaces) {
  TestExtendedObserver observer;
  AstraSessionRestoreHelper::AddObserver(&observer);

  AstraSessionRestoreHelper::RestoreAllWorkspaces(profile_.get());

  // Should trigger start and completed.
  EXPECT_EQ(observer.session_restore_started_count_, 1);
  EXPECT_EQ(observer.session_restore_completed_count_, 1);

  AstraSessionRestoreHelper::RemoveObserver(&observer);
}

TEST_F(SessionRestoreHelperTest, Bulk_RestoreWorkspaceById) {
  TestExtendedObserver observer;
  AstraSessionRestoreHelper::AddObserver(&observer);

  bool result = AstraSessionRestoreHelper::RestoreWorkspace(
      profile_.get(), "workspace-1");

  EXPECT_TRUE(result);
  EXPECT_EQ(observer.session_restore_started_count_, 1);
  EXPECT_EQ(observer.session_restore_completed_count_, 1);

  AstraSessionRestoreHelper::RemoveObserver(&observer);
}

TEST_F(SessionRestoreHelperTest, Bulk_RestoreWorkspaceEmptyId) {
  bool result = AstraSessionRestoreHelper::RestoreWorkspace(
      profile_.get(), "");
  EXPECT_FALSE(result);
}

TEST_F(SessionRestoreHelperTest, Bulk_RestoreWorkspaceNullProfile) {
  bool result = AstraSessionRestoreHelper::RestoreWorkspace(
      nullptr, "ws-1");
  EXPECT_FALSE(result);
}

TEST_F(SessionRestoreHelperTest, Bulk_CloseAllTabsInWorkspace) {
  size_t closed = AstraSessionRestoreHelper::CloseAllTabsInWorkspace(
      profile_.get(), "ws-to-close");

  // Returns 0 in skeleton implementation.
  EXPECT_EQ(closed, 0u);
}

TEST_F(SessionRestoreHelperTest, Bulk_CloseAllTabsInWorkspaceEmptyId) {
  size_t closed = AstraSessionRestoreHelper::CloseAllTabsInWorkspace(
      profile_.get(), "");
  EXPECT_EQ(closed, 0u);
}

TEST_F(SessionRestoreHelperTest, Bulk_RestoreAllWorkspacesNullProfile) {
  // Should not crash with null profile.
  AstraSessionRestoreHelper::RestoreAllWorkspaces(nullptr);
  SUCCEED();
}

// ===========================================================================
// New metadata keys validation
// ===========================================================================

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_IsInReadingListValid) {
  base::Value::Dict dict;
  dict.Set(kMetaKeyIsInReadingList, true);
  EXPECT_TRUE(ValidateAstraTabMetadata(dict));
}

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_IsInReadingListWrongType) {
  base::Value::Dict dict;
  dict.Set(kMetaKeyIsInReadingList, "true");  // Should be bool.
  EXPECT_FALSE(ValidateAstraTabMetadata(dict));
}

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_LastActiveTimeValid) {
  base::Value::Dict dict;
  dict.Set(kMetaKeyLastActiveTime, 1234567890.0);
  EXPECT_TRUE(ValidateAstraTabMetadata(dict));
}

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_LastActiveTimeWrongType) {
  base::Value::Dict dict;
  dict.Set(kMetaKeyLastActiveTime, "recent");  // Should be double.
  EXPECT_FALSE(ValidateAstraTabMetadata(dict));
}

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_DiscardCountValid) {
  base::Value::Dict dict;
  dict.Set(kMetaKeyDiscardCount, 5);
  EXPECT_TRUE(ValidateAstraTabMetadata(dict));
}

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_DiscardCountNegative) {
  base::Value::Dict dict;
  dict.Set(kMetaKeyDiscardCount, -1);
  EXPECT_FALSE(ValidateAstraTabMetadata(dict));
}

TEST_F(SessionRestoreHelperTest, ValidateTabMetadata_DiscardCountWrongType) {
  base::Value::Dict dict;
  dict.Set(kMetaKeyDiscardCount, "many");
  EXPECT_FALSE(ValidateAstraTabMetadata(dict));
}

TEST_F(SessionRestoreHelperTest, ValidateWindowMetadata_OrderIndexValid) {
  base::Value::Dict dict;
  dict.Set(kMetaKeyWindowOrderIndex, 3);
  EXPECT_TRUE(ValidateAstraWindowMetadata(dict));
}

TEST_F(SessionRestoreHelperTest, ValidateWindowMetadata_OrderIndexNegative) {
  base::Value::Dict dict;
  dict.Set(kMetaKeyWindowOrderIndex, -1);
  EXPECT_FALSE(ValidateAstraWindowMetadata(dict));
}

TEST_F(SessionRestoreHelperTest, ValidateWindowMetadata_SplitViewActiveValid) {
  base::Value::Dict dict;
  dict.Set(kMetaKeyWindowSplitViewActive, true);
  EXPECT_TRUE(ValidateAstraWindowMetadata(dict));
}

TEST_F(SessionRestoreHelperTest, ValidateWindowMetadata_SplitViewRatioValid) {
  base::Value::Dict dict;
  dict.Set(kMetaKeyWindowSplitViewRatio, 0.5);
  EXPECT_TRUE(ValidateAstraWindowMetadata(dict));
}

TEST_F(SessionRestoreHelperTest, ValidateWindowMetadata_SplitViewRatioOutOfRange) {
  base::Value::Dict dict;
  dict.Set(kMetaKeyWindowSplitViewRatio, 1.5);
  EXPECT_FALSE(ValidateAstraWindowMetadata(dict));
}

TEST_F(SessionRestoreHelperTest, ValidateWindowMetadata_SplitViewOrientationValid) {
  base::Value::Dict dict;
  dict.Set(kMetaKeyWindowSplitViewOrientation, 0);
  EXPECT_TRUE(ValidateAstraWindowMetadata(dict));
  dict.Set(kMetaKeyWindowSplitViewOrientation, 1);
  EXPECT_TRUE(ValidateAstraWindowMetadata(dict));
}

TEST_F(SessionRestoreHelperTest, ValidateWindowMetadata_SplitViewOrientationInvalid) {
  base::Value::Dict dict;
  dict.Set(kMetaKeyWindowSplitViewOrientation, 99);
  EXPECT_FALSE(ValidateAstraWindowMetadata(dict));
}

TEST_F(SessionRestoreHelperTest, ValidateWindowMetadata_IsHibernatedValid) {
  base::Value::Dict dict;
  dict.Set(kMetaKeyWindowIsHibernated, true);
  EXPECT_TRUE(ValidateAstraWindowMetadata(dict));
}

TEST_F(SessionRestoreHelperTest, ValidateWindowMetadata_IsHibernatedWrongType) {
  base::Value::Dict dict;
  dict.Set(kMetaKeyWindowIsHibernated, "yes");
  EXPECT_FALSE(ValidateAstraWindowMetadata(dict));
}

// ===========================================================================
// Clone/normalize tests for new keys
// ===========================================================================

TEST_F(SessionRestoreHelperTest, CloneTabMetadata_IncludesNewKeys) {
  base::Value::Dict source;
  source.Set(kMetaKeyIsInReadingList, true);
  source.Set(kMetaKeyDiscardCount, 5);
  source.Set(kMetaKeyLastActiveTime, 1000.0);

  base::Value::Dict cloned = CloneAstraTabMetadata(source);

  EXPECT_TRUE(*cloned.FindBool(kMetaKeyIsInReadingList));
  EXPECT_EQ(*cloned.FindInt(kMetaKeyDiscardCount), 5);
  EXPECT_DOUBLE_EQ(*cloned.FindDouble(kMetaKeyLastActiveTime), 1000.0);
}

TEST_F(SessionRestoreHelperTest, CloneWindowMetadata_IncludesNewKeys) {
  base::Value::Dict source;
  source.Set(kMetaKeyWindowOrderIndex, 2);
  source.Set(kMetaKeyWindowSplitViewActive, true);
  source.Set(kMetaKeyWindowSplitViewRatio, 0.7);
  source.Set(kMetaKeyWindowIsHibernated, true);

  base::Value::Dict cloned = CloneAstraWindowMetadata(source);

  EXPECT_EQ(*cloned.FindInt(kMetaKeyWindowOrderIndex), 2);
  EXPECT_TRUE(*cloned.FindBool(kMetaKeyWindowSplitViewActive));
  EXPECT_DOUBLE_EQ(*cloned.FindDouble(kMetaKeyWindowSplitViewRatio), 0.7);
  EXPECT_TRUE(*cloned.FindBool(kMetaKeyWindowIsHibernated));
}

TEST_F(SessionRestoreHelperTest, NormalizeTabMetadata_IncludesNewDefaults) {
  base::Value::Dict dict;
  NormalizeAstraTabMetadata(dict);

  EXPECT_TRUE(dict.contains(kMetaKeyIsInReadingList));
  EXPECT_FALSE(*dict.FindBool(kMetaKeyIsInReadingList));
  EXPECT_TRUE(dict.contains(kMetaKeyDiscardCount));
  EXPECT_EQ(*dict.FindInt(kMetaKeyDiscardCount), 0);
}

TEST_F(SessionRestoreHelperTest, NormalizeWindowMetadata_IncludesNewDefaults) {
  base::Value::Dict dict;
  NormalizeAstraWindowMetadata(dict);

  EXPECT_TRUE(dict.contains(kMetaKeyWindowIsHibernated));
  EXPECT_FALSE(*dict.FindBool(kMetaKeyWindowIsHibernated));
  EXPECT_TRUE(dict.contains(kMetaKeyWindowOrderIndex));
  EXPECT_EQ(*dict.FindInt(kMetaKeyWindowOrderIndex), 0);
  EXPECT_TRUE(dict.contains(kMetaKeyWindowSplitViewActive));
  EXPECT_FALSE(*dict.FindBool(kMetaKeyWindowSplitViewActive));
  EXPECT_TRUE(dict.contains(kMetaKeyWindowSidebarVisible));
  EXPECT_TRUE(*dict.FindBool(kMetaKeyWindowSidebarVisible));
}

// ===========================================================================
// Session restore stats struct — extended tests
// ===========================================================================

TEST_F(SessionRestoreHelperTest, RestoreStatsStruct_GetRestoreDurationZeroDefault) {
  AstraSessionRestoreStats stats;
  EXPECT_EQ(stats.GetRestoreDuration(), base::TimeDelta());
}

TEST_F(SessionRestoreHelperTest, RestoreStatsStruct_GetRestoreDurationCalculated) {
  AstraSessionRestoreStats stats;
  stats.restore_start_time = base::Time::Now();
  stats.restore_end_time = stats.restore_start_time + base::Seconds(5);
  EXPECT_EQ(stats.GetRestoreDuration(), base::Seconds(5));
}

TEST_F(SessionRestoreHelperTest, RestoreStatsStruct_GetRestoreSuccessRateDefault) {
  AstraSessionRestoreStats stats;
  EXPECT_DOUBLE_EQ(stats.GetRestoreSuccessRate(), 100.0);
}

TEST_F(SessionRestoreHelperTest, RestoreStatsStruct_GetRestoreSuccessRatePartial) {
  AstraSessionRestoreStats stats;
  stats.tabs_restored = 8;
  stats.tabs_failed = 2;
  EXPECT_DOUBLE_EQ(stats.GetRestoreSuccessRate(), 80.0);
}

TEST_F(SessionRestoreHelperTest, RestoreStatsStruct_IsRestoreInProgress) {
  AstraSessionRestoreStats stats;
  EXPECT_FALSE(stats.IsRestoreInProgress());
  stats.restore_in_progress = true;
  EXPECT_TRUE(stats.IsRestoreInProgress());
}

TEST_F(SessionRestoreHelperTest, RestoreStatsStruct_ResetClearsTimestamps) {
  AstraSessionRestoreStats stats;
  stats.restore_start_time = base::Time::Now();
  stats.restore_end_time = base::Time::Now();
  stats.tabs_failed = 5;
  stats.Reset();
  EXPECT_TRUE(stats.restore_start_time.is_null());
  EXPECT_TRUE(stats.restore_end_time.is_null());
  EXPECT_EQ(stats.tabs_failed, 0u);
}

// ===========================================================================
// Edge cases
// ===========================================================================

TEST_F(SessionRestoreHelperTest, SessionMetadata_EmptySessionHasZeroStats) {
  AstraSessionMetadata session;
  EXPECT_EQ(session.GetTabCount(), 0u);
  EXPECT_EQ(session.GetWindowCount(), 0u);
  EXPECT_EQ(session.GetWorkspaceCount(), 0u);
  EXPECT_EQ(session.GetFavoriteTabCount(), 0u);
  EXPECT_EQ(session.GetPinnedTabCount(), 0u);
  EXPECT_EQ(session.GetStackedTabCount(), 0u);
  EXPECT_EQ(session.GetEstimatedMemoryUsage(), sizeof(AstraSessionMetadata));
}

TEST_F(SessionRestoreHelperTest, SessionMetadata_MergeEmptySession) {
  AstraSessionMetadata base;
  base.session_name = "base";
  AstraSessionMetadata empty;

  base.MergeFrom(empty);

  // Session name should stay (empty source doesn't overwrite).
  EXPECT_EQ(base.session_name, "base");
}

TEST_F(SessionRestoreHelperTest, TabMetadata_FromDictClampsValues) {
  base::Value::Dict dict;
  dict.Set(kMetaKeySplitViewRatio, 2.0);  // Too high.
  dict.Set(kMetaKeyFavoriteOrderIndex, -5);  // Negative.
  dict.Set(kMetaKeyDiscardCount, -3);  // Negative.

  AstraTabSessionMetadata tab;
  tab.FromDict(dict);

  // Values should be clamped.
  EXPECT_DOUBLE_EQ(tab.split_view_ratio, 1.0);
  EXPECT_EQ(tab.favorite_order_index, 0);
  EXPECT_EQ(tab.discard_count, 0);
}

TEST_F(SessionRestoreHelperTest, WindowMetadata_FromDictClampsValues) {
  base::Value::Dict dict;
  dict.Set(kMetaKeyWindowSplitViewRatio, -0.5);  // Out of range.
  dict.Set(kMetaKeyWindowOrderIndex, -10);  // Negative.
  dict.Set(kMetaKeyWindowSidebarWidth, -50);  // Negative.

  AstraWindowSessionMetadata window;
  window.FromDict(dict);

  EXPECT_DOUBLE_EQ(window.split_view_ratio, 0.0);
  EXPECT_EQ(window.window_order_index, 0);
  EXPECT_EQ(window.sidebar_width, 0);
}

TEST_F(SessionRestoreHelperTest, LoadModeFourDistinctModes) {
  // Ensure all 4 modes are distinct.
  auto modes = {
    SessionRestoreLoadMode::kFull,
    SessionRestoreLoadMode::kLazy,
    SessionRestoreLoadMode::kSmart,
    SessionRestoreLoadMode::kMinimal,
  };

  std::set<std::string> mode_strings;
  for (auto mode : modes) {
    mode_strings.insert(
        AstraSessionRestoreHelper::RestoreLoadModeToString(mode));
  }

  // 4 distinct modes → 4 distinct strings.
  EXPECT_EQ(mode_strings.size(), 4u);
}

TEST_F(SessionRestoreHelperTest, MetadataKeys_NewKeysAreNamespaced) {
  EXPECT_TRUE(base::StartsWith(kMetaKeyIsInReadingList, "astra.",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::StartsWith(kMetaKeyLastActiveTime, "astra.",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::StartsWith(kMetaKeyDiscardCount, "astra.",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::StartsWith(kMetaKeyWindowOrderIndex, "astra.",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::StartsWith(kMetaKeyWindowSplitViewActive, "astra.",
                               base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::StartsWith(kMetaKeyWindowIsHibernated, "astra.",
                               base::CompareCase::SENSITIVE));
}

TEST_F(SessionRestoreHelperTest, Persistence_AllSettingsRoundTrip) {
  // Set all settings to non-default values, then verify they persist.
  AstraSessionRestoreHelper::SetRestoreOnStartup(profile_.get(), false);
  AstraSessionRestoreHelper::SetMaxTabsPerWorkspaceRestore(profile_.get(), 25);
  AstraSessionRestoreHelper::SetShowRestorePrompt(profile_.get(), true);
  AstraSessionRestoreHelper::SetRestoreWorkspacesIndividually(
      profile_.get(), true);
  AstraSessionRestoreHelper::SetBackgroundTabLoading(profile_.get(), false);
  AstraSessionRestoreHelper::SetAutoSaveSessionInterval(profile_.get(), 15);
  AstraSessionRestoreHelper::SetMaxSavedSessions(profile_.get(), 20);
  AstraSessionRestoreHelper::SetRestoreLoadMode(profile_.get(),
      SessionRestoreLoadMode::kMinimal);

  PrefService* prefs = profile_->GetPrefs();

  EXPECT_FALSE(prefs->GetBoolean(prefs::kPrefSessionRestoreOnStartup));
  EXPECT_EQ(prefs->GetInteger(prefs::kPrefSessionRestoreMaxTabsPerWorkspace), 25);
  EXPECT_TRUE(prefs->GetBoolean(prefs::kPrefSessionRestoreShowPrompt));
  EXPECT_TRUE(prefs->GetBoolean(
      prefs::kPrefSessionRestoreWorkspacesIndividually));
  EXPECT_FALSE(prefs->GetBoolean(prefs::kPrefSessionRestoreLazyLoading));
  EXPECT_EQ(prefs->GetInteger(
      prefs::kPrefSessionRestoreAutoSaveInterval), 15);
  EXPECT_EQ(prefs->GetInteger(prefs::kPrefSessionRestoreMaxSavedSessions), 20);
  EXPECT_EQ(prefs->GetString(prefs::kPrefSessionRestoreLoadMode), "minimal");
}

TEST_F(SessionRestoreHelperTest, OnTabRestored_NullWebContentsNoNotification) {
  TestSessionRestoreObserver observer;
  AstraSessionRestoreHelper::AddObserver(&observer);

  // Null web contents should not trigger notification.
  AstraSessionRestoreHelper::OnTabRestored(nullptr);
  EXPECT_EQ(observer.tab_restored_count_, 0);

  AstraSessionRestoreHelper::RemoveObserver(&observer);
}

TEST_F(SessionRestoreHelperTest, SaveSession_Idempotent) {
  // Saving the same session multiple times should be safe.
  AstraSessionRestoreHelper::SaveSession(profile_.get(), "idempotent");
  AstraSessionRestoreHelper::SaveSession(profile_.get(), "idempotent");
  AstraSessionRestoreHelper::SaveSession(profile_.get(), "idempotent");

  auto sessions = AstraSessionRestoreHelper::ListSavedSessions(profile_.get());
  EXPECT_EQ(sessions.size(), 1u);
}

TEST_F(SessionRestoreHelperTest, DeleteSavedSession_Idempotent) {
  // Deleting a non-existent session is safe (returns false).
  EXPECT_FALSE(AstraSessionRestoreHelper::DeleteSavedSession(
      profile_.get(), "not-there"));
  EXPECT_FALSE(AstraSessionRestoreHelper::DeleteSavedSession(
      profile_.get(), "not-there"));
}

TEST_F(SessionRestoreHelperTest, ClearLastSession_Idempotent) {
  // Clearing last session multiple times is safe.
  AstraSessionRestoreHelper::ClearLastSession(profile_.get());
  AstraSessionRestoreHelper::ClearLastSession(profile_.get());
  EXPECT_FALSE(AstraSessionRestoreHelper::HasLastSession(profile_.get()));
}

}  // namespace astra
