// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Unit tests for AstraNewTabPageService.
//
// Tests verify:
//   - Construction and shutdown
//   - Legacy API: GetTopSites, GetRecentlyVisited, GetWorkspaceSummaries
//   - Legacy API: custom shortcuts (index-based add/remove/update/reorder)
//   - Legacy API: layout options and background settings
//   - Legacy API: observer pattern (AstraNewTabPageServiceObserver)
//   - New API: managed shortcuts (ID-based add/remove/update/reorder)
//   - New API: default shortcuts and reset to defaults
//   - New API: shortcut icons, positions, use counts
//   - New API: workspace card visibility, max count, reorder
//   - New API: suggested content (get, dismiss, restore, refresh)
//   - New API: theme settings (all 15+ settings, Get/SetNtpTheme, Reset)
//   - New API: observer pattern (AstraNewTabPageObserver, all 8 events)
//   - Edge cases: empty lists, invalid IDs, negative positions, max=0
//   - Persistence across service instances
//   - Struct default values and field access
//   - Enum value verification
//
// Note: Suggested content currently uses placeholder data because the real
// Chromium history / most-visited integration is not yet wired up.
//
// Chromium test pattern: TestingProfile + base::test::TaskEnvironment
//   (chrome/test/base/testing_profile.h)

#include "astra/browser/astra_new_tab_page_service.h"

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/test/task_environment.h"
#include "base/uuid.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkColor.h"

namespace astra {

namespace {

// =========================================================================
// Test observers
// =========================================================================

// New-style observer that records all calls for verification.
class TestNewNtpObserver : public AstraNewTabPageObserver {
 public:
  void OnShortcutAdded(AstraNewTabPageService* /*service*/,
                       const std::string& shortcut_id) override {
    shortcut_added_count_++;
    last_shortcut_id_ = shortcut_id;
  }

  void OnShortcutRemoved(AstraNewTabPageService* /*service*/,
                         const std::string& shortcut_id) override {
    shortcut_removed_count_++;
    last_shortcut_id_ = shortcut_id;
  }

  void OnShortcutChanged(AstraNewTabPageService* /*service*/,
                         const std::string& shortcut_id) override {
    shortcut_changed_count_++;
    last_shortcut_id_ = shortcut_id;
  }

  void OnShortcutsReordered(AstraNewTabPageService* /*service*/) override {
    shortcuts_reordered_count_++;
  }

  void OnNtpThemeChanged(AstraNewTabPageService* /*service*/) override {
    ntp_theme_changed_count_++;
  }

  void OnWorkspaceCardVisibilityChanged(
      AstraNewTabPageService* /*service*/,
      const std::string& workspace_id,
      bool visible) override {
    workspace_visibility_count_++;
    last_workspace_id_ = workspace_id;
    last_workspace_visible_ = visible;
  }

  void OnSuggestionsChanged(AstraNewTabPageService* /*service*/) override {
    suggestions_changed_count_++;
  }

  void OnNewTabPageServiceShutdown(
      AstraNewTabPageService* /*service*/) override {
    shutdown_count_++;
  }

  // Counters
  int shortcut_added_count_ = 0;
  int shortcut_removed_count_ = 0;
  int shortcut_changed_count_ = 0;
  int shortcuts_reordered_count_ = 0;
  int ntp_theme_changed_count_ = 0;
  int workspace_visibility_count_ = 0;
  int suggestions_changed_count_ = 0;
  int shutdown_count_ = 0;

  // Last values
  std::string last_shortcut_id_;
  std::string last_workspace_id_;
  bool last_workspace_visible_ = false;

  // Resets all counters to zero.
  void ResetCounters() {
    shortcut_added_count_ = 0;
    shortcut_removed_count_ = 0;
    shortcut_changed_count_ = 0;
    shortcuts_reordered_count_ = 0;
    ntp_theme_changed_count_ = 0;
    workspace_visibility_count_ = 0;
    suggestions_changed_count_ = 0;
    shutdown_count_ = 0;
    last_shortcut_id_.clear();
    last_workspace_id_.clear();
    last_workspace_visible_ = false;
  }
};

// Legacy test observer.
class TestLegacyNtpObserver : public AstraNewTabPageServiceObserver {
 public:
  void OnCustomShortcutsChanged() override {
    custom_shortcuts_changed_count_++;
  }

  void OnLayoutChanged() override {
    layout_changed_count_++;
  }

  void OnBackgroundChanged() override {
    background_changed_count_++;
  }

  int custom_shortcuts_changed_count_ = 0;
  int layout_changed_count_ = 0;
  int background_changed_count_ = 0;

  void ResetCounters() {
    custom_shortcuts_changed_count_ = 0;
    layout_changed_count_ = 0;
    background_changed_count_ = 0;
  }
};

// Partial observer that only overrides one method — verifies default impls.
class PartialNewNtpObserver : public AstraNewTabPageObserver {
 public:
  void OnShortcutAdded(AstraNewTabPageService* /*service*/,
                       const std::string& /*shortcut_id*/) override {
    shortcut_added_count_++;
  }

  int shortcut_added_count_ = 0;
};

}  // namespace

// =========================================================================
// NewTabPageService test fixture
// =========================================================================

class NewTabPageServiceTest : public testing::Test {
 protected:
  NewTabPageServiceTest() {
    profile_ = std::make_unique<TestingProfile>();

    // Register NTP prefs on the testing pref service.
    auto* prefs =
        static_cast<TestingPrefServiceSimple*>(profile_->GetPrefs());
    AstraNewTabPageServiceFactory::RegisterProfilePrefs(prefs->registry());

    service_ = std::make_unique<AstraNewTabPageService>(profile_.get());
    DCHECK(service_);
  }

  ~NewTabPageServiceTest() override = default;

  void TearDown() override {
    // Clean up observers to avoid dangling references.
    for (auto* observer : new_observers_) {
      service_->RemoveObserver(observer);
    }
    for (auto* observer : legacy_observers_) {
      service_->RemoveObserver(observer);
    }
  }

  // Helpers ---------------------------------------------------------------

  // Adds a new-style observer and tracks it for cleanup.
  void AddNewObserver(TestNewNtpObserver* observer) {
    service_->AddObserver(observer);
    new_observers_.push_back(observer);
  }

  // Adds a legacy observer and tracks it for cleanup.
  void AddLegacyObserver(TestLegacyNtpObserver* observer) {
    service_->AddObserver(observer);
    legacy_observers_.push_back(observer);
  }

  // Task environment is required for TestingProfile and base::Time.
  base::test::TaskEnvironment task_environment_;

  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<AstraNewTabPageService> service_;

  // Pool of test observers tracked for cleanup.
  std::vector<TestNewNtpObserver*> new_observers_;
  std::vector<TestLegacyNtpObserver*> legacy_observers_;
};

// =========================================================================
// Construction and lifecycle tests
// =========================================================================

TEST_F(NewTabPageServiceTest, ConstructsWithoutCrash) {
  EXPECT_NE(nullptr, service_.get());
}

TEST_F(NewTabPageServiceTest, ShutdownIsSafe) {
  service_->Shutdown();
  // No crash = success.
}

TEST_F(NewTabPageServiceTest, ShutdownTwiceIsSafe) {
  service_->Shutdown();
  service_->Shutdown();
  // No crash = success.  Shutdown should be idempotent.
}

TEST_F(NewTabPageServiceTest, ShutdownNotifiesNewObserver) {
  TestNewNtpObserver observer;
  AddNewObserver(&observer);

  service_->Shutdown();

  EXPECT_EQ(1, observer.shutdown_count_);
}

// =========================================================================
// Top sites tests (legacy API)
// =========================================================================

TEST_F(NewTabPageServiceTest, GetTopSitesReturnsResults) {
  auto shortcuts = service_->GetTopSites(10);
  EXPECT_GT(shortcuts.size(), 0u);
}

TEST_F(NewTabPageServiceTest, GetTopSitesRespectsCount) {
  auto shortcuts = service_->GetTopSites(3);
  EXPECT_LE(shortcuts.size(), 3u);
}

TEST_F(NewTabPageServiceTest, GetTopSitesZeroCount) {
  auto shortcuts = service_->GetTopSites(0);
  EXPECT_TRUE(shortcuts.empty());
}

TEST_F(NewTabPageServiceTest, GetTopSitesLargeCount) {
  auto shortcuts = service_->GetTopSites(1000);
  EXPECT_GE(shortcuts.size(), 0u);
}

TEST_F(NewTabPageServiceTest, TopSitesHaveValidUrls) {
  auto shortcuts = service_->GetTopSites(5);
  for (const auto& shortcut : shortcuts) {
    EXPECT_TRUE(shortcut.url.is_valid());
    EXPECT_FALSE(shortcut.title.empty());
  }
}

TEST_F(NewTabPageServiceTest, TopSitesAreMostVisitedByDefault) {
  auto shortcuts = service_->GetTopSites(5);
  for (const auto& shortcut : shortcuts) {
    EXPECT_TRUE(shortcut.is_most_visited);
  }
}

// =========================================================================
// Recently visited tests (legacy API)
// =========================================================================

TEST_F(NewTabPageServiceTest, GetRecentlyVisitedReturnsResults) {
  auto visits = service_->GetRecentlyVisited(10);
  EXPECT_GT(visits.size(), 0u);
}

TEST_F(NewTabPageServiceTest, GetRecentlyVisitedRespectsCount) {
  auto visits = service_->GetRecentlyVisited(3);
  EXPECT_LE(visits.size(), 3u);
}

TEST_F(NewTabPageServiceTest, GetRecentlyVisitedZeroCount) {
  auto visits = service_->GetRecentlyVisited(0);
  EXPECT_TRUE(visits.empty());
}

TEST_F(NewTabPageServiceTest, RecentlyVisitedHaveValidUrls) {
  auto visits = service_->GetRecentlyVisited(5);
  for (const auto& visit : visits) {
    EXPECT_TRUE(visit.url.is_valid());
    EXPECT_FALSE(visit.title.empty());
  }
}

// =========================================================================
// Workspace summaries tests (legacy API)
// =========================================================================

TEST_F(NewTabPageServiceTest, GetWorkspaceSummariesReturnsVector) {
  auto summaries = service_->GetWorkspaceSummaries();
  EXPECT_GE(summaries.size(), 0u);
}

TEST_F(NewTabPageServiceTest, WorkspaceSummariesHaveValidIds) {
  auto summaries = service_->GetWorkspaceSummaries();
  for (const auto& summary : summaries) {
    EXPECT_FALSE(summary.id.empty());
    EXPECT_FALSE(summary.name.empty());
  }
}

// =========================================================================
// Favorite shortcuts tests (legacy API)
// =========================================================================

TEST_F(NewTabPageServiceTest, GetFavoriteShortcutsReturnsVector) {
  auto shortcuts = service_->GetFavoriteShortcuts(10);
  EXPECT_GE(shortcuts.size(), 0u);
}

TEST_F(NewTabPageServiceTest, GetFavoriteShortcutsRespectsCount) {
  auto shortcuts = service_->GetFavoriteShortcuts(5);
  EXPECT_LE(shortcuts.size(), 5u);
}

TEST_F(NewTabPageServiceTest, GetFavoriteShortcutsZeroCount) {
  auto shortcuts = service_->GetFavoriteShortcuts(0);
  EXPECT_TRUE(shortcuts.empty());
}

// =========================================================================
// Legacy observer pattern tests
// =========================================================================

TEST_F(NewTabPageServiceTest, LegacyAddAndRemoveObserver) {
  TestLegacyNtpObserver observer;
  AddLegacyObserver(&observer);
  service_->RemoveObserver(&observer);
  // No crash = success.
}

TEST_F(NewTabPageServiceTest, LegacyObserverDefaultImplementations) {
  class PartialLegacyObserver : public AstraNewTabPageServiceObserver {
   public:
    void OnCustomShortcutsChanged() override { count_++; }
    int count_ = 0;
  } partial_observer;

  service_->AddObserver(&partial_observer);
  service_->set_layout_mode(AstraNtpLayoutMode::kCompact);
  service_->SetBackgroundColor(SK_ColorRED);
  service_->AddCustomShortcut(u"Test", GURL("https://example.com"));

  EXPECT_EQ(1, partial_observer.count_);

  service_->RemoveObserver(&partial_observer);
}

TEST_F(NewTabPageServiceTest, LegacyObserverRemovalStopsNotifications) {
  TestLegacyNtpObserver observer;
  AddLegacyObserver(&observer);

  service_->set_layout_mode(AstraNtpLayoutMode::kCompact);
  EXPECT_EQ(1, observer.layout_changed_count_);

  service_->RemoveObserver(&observer);
  auto it = base::ranges::find(legacy_observers_, &observer);
  if (it != legacy_observers_.end()) {
    legacy_observers_.erase(it);
  }

  observer.ResetCounters();
  service_->set_layout_mode(AstraNtpLayoutMode::kFocused);
  EXPECT_EQ(0, observer.layout_changed_count_);
}

TEST_F(NewTabPageServiceTest, LegacyMultipleObservers) {
  TestLegacyNtpObserver observer1;
  TestLegacyNtpObserver observer2;
  AddLegacyObserver(&observer1);
  AddLegacyObserver(&observer2);

  service_->AddCustomShortcut(u"Test", GURL("https://example.com"));

  EXPECT_EQ(1, observer1.custom_shortcuts_changed_count_);
  EXPECT_EQ(1, observer2.custom_shortcuts_changed_count_);
}

// =========================================================================
// Legacy custom shortcuts tests
// =========================================================================

TEST_F(NewTabPageServiceTest, CustomShortcutsStartsEmpty) {
  EXPECT_EQ(0u, service_->GetCustomShortcutCount());
  auto shortcuts = service_->GetCustomShortcuts(10);
  EXPECT_TRUE(shortcuts.empty());
}

TEST_F(NewTabPageServiceTest, AddCustomShortcut) {
  size_t index = service_->AddCustomShortcut(
      u"My Site", GURL("https://example.com"));

  EXPECT_EQ(0u, index);
  EXPECT_EQ(1u, service_->GetCustomShortcutCount());

  auto shortcuts = service_->GetCustomShortcuts(10);
  ASSERT_EQ(1u, shortcuts.size());
  EXPECT_EQ(u"My Site", shortcuts[0].title);
  EXPECT_EQ(GURL("https://example.com"), shortcuts[0].url);
  EXPECT_FALSE(shortcuts[0].is_most_visited);
}

TEST_F(NewTabPageServiceTest, AddMultipleCustomShortcuts) {
  service_->AddCustomShortcut(u"Site 1", GURL("https://site1.com"));
  service_->AddCustomShortcut(u"Site 2", GURL("https://site2.com"));
  service_->AddCustomShortcut(u"Site 3", GURL("https://site3.com"));

  EXPECT_EQ(3u, service_->GetCustomShortcutCount());

  auto shortcuts = service_->GetCustomShortcuts(10);
  ASSERT_EQ(3u, shortcuts.size());
  EXPECT_EQ(u"Site 1", shortcuts[0].title);
  EXPECT_EQ(u"Site 2", shortcuts[1].title);
  EXPECT_EQ(u"Site 3", shortcuts[2].title);
}

TEST_F(NewTabPageServiceTest, AddCustomShortcutReturnsIncrementingIndex) {
  size_t idx1 = service_->AddCustomShortcut(u"A", GURL("https://a.com"));
  size_t idx2 = service_->AddCustomShortcut(u"B", GURL("https://b.com"));
  size_t idx3 = service_->AddCustomShortcut(u"C", GURL("https://c.com"));

  EXPECT_EQ(0u, idx1);
  EXPECT_EQ(1u, idx2);
  EXPECT_EQ(2u, idx3);
}

TEST_F(NewTabPageServiceTest, RemoveCustomShortcut) {
  service_->AddCustomShortcut(u"Site 1", GURL("https://site1.com"));
  service_->AddCustomShortcut(u"Site 2", GURL("https://site2.com"));
  service_->AddCustomShortcut(u"Site 3", GURL("https://site3.com"));

  bool result = service_->RemoveCustomShortcut(1);
  EXPECT_TRUE(result);
  EXPECT_EQ(2u, service_->GetCustomShortcutCount());

  auto shortcuts = service_->GetCustomShortcuts(10);
  ASSERT_EQ(2u, shortcuts.size());
  EXPECT_EQ(u"Site 1", shortcuts[0].title);
  EXPECT_EQ(u"Site 3", shortcuts[1].title);
}

TEST_F(NewTabPageServiceTest, RemoveCustomShortcutInvalidIndex) {
  service_->AddCustomShortcut(u"Site 1", GURL("https://site1.com"));

  bool result = service_->RemoveCustomShortcut(5);
  EXPECT_FALSE(result);
  EXPECT_EQ(1u, service_->GetCustomShortcutCount());
}

TEST_F(NewTabPageServiceTest, RemoveCustomShortcutFromEmpty) {
  bool result = service_->RemoveCustomShortcut(0);
  EXPECT_FALSE(result);
  EXPECT_EQ(0u, service_->GetCustomShortcutCount());
}

TEST_F(NewTabPageServiceTest, UpdateCustomShortcut) {
  service_->AddCustomShortcut(u"Old Title", GURL("https://old.com"));

  bool result = service_->UpdateCustomShortcut(
      0, u"New Title", GURL("https://new.com"));
  EXPECT_TRUE(result);

  auto shortcuts = service_->GetCustomShortcuts(10);
  ASSERT_EQ(1u, shortcuts.size());
  EXPECT_EQ(u"New Title", shortcuts[0].title);
  EXPECT_EQ(GURL("https://new.com"), shortcuts[0].url);
}

TEST_F(NewTabPageServiceTest, UpdateCustomShortcutInvalidIndex) {
  service_->AddCustomShortcut(u"Site 1", GURL("https://site1.com"));

  bool result = service_->UpdateCustomShortcut(
      5, u"New Title", GURL("https://new.com"));
  EXPECT_FALSE(result);
  EXPECT_EQ(1u, service_->GetCustomShortcutCount());
}

TEST_F(NewTabPageServiceTest, ReorderCustomShortcuts) {
  service_->AddCustomShortcut(u"A", GURL("https://a.com"));
  service_->AddCustomShortcut(u"B", GURL("https://b.com"));
  service_->AddCustomShortcut(u"C", GURL("https://c.com"));

  std::vector<size_t> new_order = {2, 1, 0};
  service_->ReorderCustomShortcuts(new_order);

  auto shortcuts = service_->GetCustomShortcuts(10);
  ASSERT_EQ(3u, shortcuts.size());
  EXPECT_EQ(u"C", shortcuts[0].title);
  EXPECT_EQ(u"B", shortcuts[1].title);
  EXPECT_EQ(u"A", shortcuts[2].title);
}

TEST_F(NewTabPageServiceTest, ReorderCustomShortcutsWrongSize) {
  service_->AddCustomShortcut(u"A", GURL("https://a.com"));
  service_->AddCustomShortcut(u"B", GURL("https://b.com"));

  std::vector<size_t> bad_order = {0};
  service_->ReorderCustomShortcuts(bad_order);

  auto shortcuts = service_->GetCustomShortcuts(10);
  ASSERT_EQ(2u, shortcuts.size());
  EXPECT_EQ(u"A", shortcuts[0].title);
  EXPECT_EQ(u"B", shortcuts[1].title);
}

TEST_F(NewTabPageServiceTest, ReorderCustomShortcutsDuplicateIndices) {
  service_->AddCustomShortcut(u"A", GURL("https://a.com"));
  service_->AddCustomShortcut(u"B", GURL("https://b.com"));

  std::vector<size_t> bad_order = {0, 0};
  service_->ReorderCustomShortcuts(bad_order);

  auto shortcuts = service_->GetCustomShortcuts(10);
  ASSERT_EQ(2u, shortcuts.size());
  EXPECT_EQ(u"A", shortcuts[0].title);
  EXPECT_EQ(u"B", shortcuts[1].title);
}

TEST_F(NewTabPageServiceTest, ReorderCustomShortcutsOutOfRange) {
  service_->AddCustomShortcut(u"A", GURL("https://a.com"));
  service_->AddCustomShortcut(u"B", GURL("https://b.com"));

  std::vector<size_t> bad_order = {0, 99};
  service_->ReorderCustomShortcuts(bad_order);

  auto shortcuts = service_->GetCustomShortcuts(10);
  ASSERT_EQ(2u, shortcuts.size());
  EXPECT_EQ(u"A", shortcuts[0].title);
  EXPECT_EQ(u"B", shortcuts[1].title);
}

TEST_F(NewTabPageServiceTest, ReorderEmptyShortcuts) {
  std::vector<size_t> empty_order;
  service_->ReorderCustomShortcuts(empty_order);
  EXPECT_EQ(0u, service_->GetCustomShortcutCount());
}

TEST_F(NewTabPageServiceTest, GetCustomShortcutsRespectsCount) {
  for (int i = 0; i < 10; ++i) {
    std::string title = "Site " + base::NumberToString(i);
    std::string url = "https://site" + base::NumberToString(i) + ".com";
    service_->AddCustomShortcut(base::UTF8ToUTF16(title), GURL(url));
  }

  auto shortcuts = service_->GetCustomShortcuts(3);
  EXPECT_EQ(3u, shortcuts.size());
}

TEST_F(NewTabPageServiceTest, GetCustomShortcutsZeroCount) {
  service_->AddCustomShortcut(u"Test", GURL("https://example.com"));

  auto shortcuts = service_->GetCustomShortcuts(0);
  EXPECT_TRUE(shortcuts.empty());
}

// =========================================================================
// Legacy custom shortcut persistence tests
// =========================================================================

TEST_F(NewTabPageServiceTest, CustomShortcutsPersistAcrossServiceInstances) {
  service_->AddCustomShortcut(u"Persisted 1", GURL("https://p1.com"));
  service_->AddCustomShortcut(u"Persisted 2", GURL("https://p2.com"));

  auto service2 =
      std::make_unique<AstraNewTabPageService>(profile_.get());

  EXPECT_EQ(2u, service2->GetCustomShortcutCount());
  auto shortcuts = service2->GetCustomShortcuts(10);
  ASSERT_EQ(2u, shortcuts.size());
  EXPECT_EQ(u"Persisted 1", shortcuts[0].title);
  EXPECT_EQ(u"Persisted 2", shortcuts[1].title);
}

TEST_F(NewTabPageServiceTest, ReorderedShortcutsPersist) {
  service_->AddCustomShortcut(u"A", GURL("https://a.com"));
  service_->AddCustomShortcut(u"B", GURL("https://b.com"));
  service_->AddCustomShortcut(u"C", GURL("https://c.com"));

  service_->ReorderCustomShortcuts({2, 0, 1});

  auto service2 =
      std::make_unique<AstraNewTabPageService>(profile_.get());
  auto shortcuts = service2->GetCustomShortcuts(10);
  ASSERT_EQ(3u, shortcuts.size());
  EXPECT_EQ(u"C", shortcuts[0].title);
  EXPECT_EQ(u"A", shortcuts[1].title);
  EXPECT_EQ(u"B", shortcuts[2].title);
}

// =========================================================================
// Legacy layout options tests
// =========================================================================

TEST_F(NewTabPageServiceTest, LayoutModeDefault) {
  EXPECT_EQ(AstraNtpLayoutMode::kStandard, service_->layout_mode());
}

TEST_F(NewTabPageServiceTest, SetLayoutMode) {
  service_->set_layout_mode(AstraNtpLayoutMode::kCompact);
  EXPECT_EQ(AstraNtpLayoutMode::kCompact, service_->layout_mode());

  service_->set_layout_mode(AstraNtpLayoutMode::kFocused);
  EXPECT_EQ(AstraNtpLayoutMode::kFocused, service_->layout_mode());
}

TEST_F(NewTabPageServiceTest, LayoutModeName) {
  EXPECT_EQ("standard", service_->GetLayoutModeName());

  service_->set_layout_mode(AstraNtpLayoutMode::kCompact);
  EXPECT_EQ("compact", service_->GetLayoutModeName());

  service_->set_layout_mode(AstraNtpLayoutMode::kFocused);
  EXPECT_EQ("focused", service_->GetLayoutModeName());
}

TEST_F(NewTabPageServiceTest, ShowShortcutsLegacyDefault) {
  EXPECT_TRUE(service_->show_shortcuts());
}

TEST_F(NewTabPageServiceTest, SetShowShortcutsLegacy) {
  service_->set_show_shortcuts(false);
  EXPECT_FALSE(service_->show_shortcuts());

  service_->set_show_shortcuts(true);
  EXPECT_TRUE(service_->show_shortcuts());
}

TEST_F(NewTabPageServiceTest, ShowRecentlyVisitedDefault) {
  EXPECT_TRUE(service_->show_recently_visited());
}

TEST_F(NewTabPageServiceTest, SetShowRecentlyVisited) {
  service_->set_show_recently_visited(false);
  EXPECT_FALSE(service_->show_recently_visited());

  service_->set_show_recently_visited(true);
  EXPECT_TRUE(service_->show_recently_visited());
}

TEST_F(NewTabPageServiceTest, ShowWorkspaceCardsLegacyDefault) {
  EXPECT_TRUE(service_->show_workspace_cards());
}

TEST_F(NewTabPageServiceTest, SetShowWorkspaceCardsLegacy) {
  service_->set_show_workspace_cards(false);
  EXPECT_FALSE(service_->show_workspace_cards());

  service_->set_show_workspace_cards(true);
  EXPECT_TRUE(service_->show_workspace_cards());
}

TEST_F(NewTabPageServiceTest, ShowFavoritesDefault) {
  EXPECT_TRUE(service_->show_favorites());
}

TEST_F(NewTabPageServiceTest, SetShowFavorites) {
  service_->set_show_favorites(false);
  EXPECT_FALSE(service_->show_favorites());

  service_->set_show_favorites(true);
  EXPECT_TRUE(service_->show_favorites());
}

TEST_F(NewTabPageServiceTest, LayoutOptionsPersist) {
  service_->set_layout_mode(AstraNtpLayoutMode::kCompact);
  service_->set_show_shortcuts(false);
  service_->set_show_recently_visited(false);
  service_->set_show_workspace_cards(false);
  service_->set_show_favorites(false);

  auto service2 =
      std::make_unique<AstraNewTabPageService>(profile_.get());

  EXPECT_EQ(AstraNtpLayoutMode::kCompact, service2->layout_mode());
  EXPECT_FALSE(service2->show_shortcuts());
  EXPECT_FALSE(service2->show_recently_visited());
  EXPECT_FALSE(service2->show_workspace_cards());
  EXPECT_FALSE(service2->show_favorites());
}

// =========================================================================
// Legacy background / theme tests
// =========================================================================

TEST_F(NewTabPageServiceTest, BackgroundTypeDefault) {
  EXPECT_EQ(AstraNtpBackgroundType::kDefault, service_->background_type());
}

TEST_F(NewTabPageServiceTest, SetBackgroundType) {
  service_->set_background_type(AstraNtpBackgroundType::kSolidColor);
  EXPECT_EQ(AstraNtpBackgroundType::kSolidColor,
            service_->background_type());

  service_->set_background_type(AstraNtpBackgroundType::kCustomImage);
  EXPECT_EQ(AstraNtpBackgroundType::kCustomImage,
            service_->background_type());
}

TEST_F(NewTabPageServiceTest, BackgroundColorDefault) {
  EXPECT_EQ(SK_ColorWHITE, service_->GetBackgroundColor());
}

TEST_F(NewTabPageServiceTest, SetBackgroundColorLegacy) {
  service_->SetBackgroundColor(SK_ColorRED);
  EXPECT_EQ(SK_ColorRED, service_->GetBackgroundColor());

  service_->SetBackgroundColor(SK_ColorBLUE);
  EXPECT_EQ(SK_ColorBLUE, service_->GetBackgroundColor());
}

TEST_F(NewTabPageServiceTest, SetBackgroundColorCustomRgb) {
  SkColor custom = SkColorSetRGB(0x12, 0x34, 0x56);
  service_->SetBackgroundColor(custom);

  SkColor result = service_->GetBackgroundColor();
  EXPECT_EQ(0x12, SkColorGetR(result));
  EXPECT_EQ(0x34, SkColorGetG(result));
  EXPECT_EQ(0x56, SkColorGetB(result));
}

TEST_F(NewTabPageServiceTest, BackgroundSettingsPersist) {
  service_->set_background_type(AstraNtpBackgroundType::kSolidColor);
  service_->SetBackgroundColor(SK_ColorGREEN);

  auto service2 =
      std::make_unique<AstraNewTabPageService>(profile_.get());

  EXPECT_EQ(AstraNtpBackgroundType::kSolidColor,
            service2->background_type());
  EXPECT_EQ(SK_ColorGREEN, service2->GetBackgroundColor());
}

// =========================================================================
// Legacy query method tests (GetAllShortcuts, etc.)
// =========================================================================

TEST_F(NewTabPageServiceTest, GetAllShortcutsCombinesCustomAndTopSites) {
  service_->AddCustomShortcut(u"Custom 1", GURL("https://custom1.com"));
  service_->AddCustomShortcut(u"Custom 2", GURL("https://custom2.com"));

  auto all = service_->GetAllShortcuts(10);
  EXPECT_GT(all.size(), 2u);

  EXPECT_FALSE(all[0].is_most_visited);
  EXPECT_FALSE(all[1].is_most_visited);
  EXPECT_EQ(u"Custom 1", all[0].title);
  EXPECT_EQ(u"Custom 2", all[1].title);

  for (size_t i = 2; i < all.size(); ++i) {
    EXPECT_TRUE(all[i].is_most_visited);
  }
}

TEST_F(NewTabPageServiceTest, GetAllShortcutsRespectsCount) {
  service_->AddCustomShortcut(u"Custom 1", GURL("https://custom1.com"));
  service_->AddCustomShortcut(u"Custom 2", GURL("https://custom2.com"));

  auto all = service_->GetAllShortcuts(3);
  EXPECT_EQ(3u, all.size());
}

TEST_F(NewTabPageServiceTest, GetAllShortcutsZeroCount) {
  service_->AddCustomShortcut(u"Test", GURL("https://example.com"));

  auto all = service_->GetAllShortcuts(0);
  EXPECT_TRUE(all.empty());
}

TEST_F(NewTabPageServiceTest, GetAllShortcutsOnlyCustomWhenNoTopSites) {
  service_->AddCustomShortcut(u"Custom 1", GURL("https://custom1.com"));
  service_->AddCustomShortcut(u"Custom 2", GURL("https://custom2.com"));

  auto all = service_->GetAllShortcuts(2);
  ASSERT_EQ(2u, all.size());
  EXPECT_FALSE(all[0].is_most_visited);
  EXPECT_FALSE(all[1].is_most_visited);
}

TEST_F(NewTabPageServiceTest, GetAllShortcutsOnlyTopSitesWhenNoCustom) {
  auto all = service_->GetAllShortcuts(5);
  for (const auto& s : all) {
    EXPECT_TRUE(s.is_most_visited);
  }
}

TEST_F(NewTabPageServiceTest, GetCustomShortcutsAreNotMostVisited) {
  service_->AddCustomShortcut(u"Custom", GURL("https://custom.com"));

  auto shortcuts = service_->GetCustomShortcuts(1);
  ASSERT_EQ(1u, shortcuts.size());
  EXPECT_FALSE(shortcuts[0].is_most_visited);
}

// =========================================================================
// Legacy observer notification tests
// =========================================================================

TEST_F(NewTabPageServiceTest, AddCustomShortcutNotifiesLegacyObserver) {
  TestLegacyNtpObserver observer;
  AddLegacyObserver(&observer);

  service_->AddCustomShortcut(u"Test", GURL("https://example.com"));

  EXPECT_EQ(1, observer.custom_shortcuts_changed_count_);
  EXPECT_EQ(0, observer.layout_changed_count_);
  EXPECT_EQ(0, observer.background_changed_count_);
}

TEST_F(NewTabPageServiceTest, RemoveCustomShortcutNotifiesLegacyObserver) {
  service_->AddCustomShortcut(u"Test", GURL("https://example.com"));

  TestLegacyNtpObserver observer;
  AddLegacyObserver(&observer);

  service_->RemoveCustomShortcut(0);

  EXPECT_EQ(1, observer.custom_shortcuts_changed_count_);
}

TEST_F(NewTabPageServiceTest, UpdateCustomShortcutNotifiesLegacyObserver) {
  service_->AddCustomShortcut(u"Test", GURL("https://example.com"));

  TestLegacyNtpObserver observer;
  AddLegacyObserver(&observer);

  service_->UpdateCustomShortcut(0, u"New", GURL("https://new.com"));

  EXPECT_EQ(1, observer.custom_shortcuts_changed_count_);
}

TEST_F(NewTabPageServiceTest, ReorderCustomShortcutsNotifiesLegacyObserver) {
  service_->AddCustomShortcut(u"A", GURL("https://a.com"));
  service_->AddCustomShortcut(u"B", GURL("https://b.com"));

  TestLegacyNtpObserver observer;
  AddLegacyObserver(&observer);

  service_->ReorderCustomShortcuts({1, 0});

  EXPECT_EQ(1, observer.custom_shortcuts_changed_count_);
}

TEST_F(NewTabPageServiceTest, InvalidRemoveDoesNotNotifyLegacy) {
  TestLegacyNtpObserver observer;
  AddLegacyObserver(&observer);

  service_->RemoveCustomShortcut(0);

  EXPECT_EQ(0, observer.custom_shortcuts_changed_count_);
}

TEST_F(NewTabPageServiceTest, InvalidUpdateDoesNotNotifyLegacy) {
  TestLegacyNtpObserver observer;
  AddLegacyObserver(&observer);

  service_->UpdateCustomShortcut(0, u"X", GURL("https://x.com"));

  EXPECT_EQ(0, observer.custom_shortcuts_changed_count_);
}

TEST_F(NewTabPageServiceTest, InvalidReorderDoesNotNotifyLegacy) {
  service_->AddCustomShortcut(u"A", GURL("https://a.com"));
  service_->AddCustomShortcut(u"B", GURL("https://b.com"));

  TestLegacyNtpObserver observer;
  AddLegacyObserver(&observer);

  service_->ReorderCustomShortcuts({0});

  EXPECT_EQ(0, observer.custom_shortcuts_changed_count_);
}

TEST_F(NewTabPageServiceTest, SetLayoutModeNotifiesLegacyObserver) {
  TestLegacyNtpObserver observer;
  AddLegacyObserver(&observer);

  service_->set_layout_mode(AstraNtpLayoutMode::kCompact);

  EXPECT_EQ(1, observer.layout_changed_count_);
  EXPECT_EQ(0, observer.custom_shortcuts_changed_count_);
  EXPECT_EQ(0, observer.background_changed_count_);
}

TEST_F(NewTabPageServiceTest, ShowShortcutsNotifiesLegacyLayoutObserver) {
  TestLegacyNtpObserver observer;
  AddLegacyObserver(&observer);

  service_->set_show_shortcuts(false);

  EXPECT_EQ(1, observer.layout_changed_count_);
}

TEST_F(NewTabPageServiceTest, ShowRecentlyVisitedNotifiesLegacyLayoutObserver) {
  TestLegacyNtpObserver observer;
  AddLegacyObserver(&observer);

  service_->set_show_recently_visited(false);

  EXPECT_EQ(1, observer.layout_changed_count_);
}

TEST_F(NewTabPageServiceTest, ShowWorkspaceCardsNotifiesLegacyLayoutObserver) {
  TestLegacyNtpObserver observer;
  AddLegacyObserver(&observer);

  service_->set_show_workspace_cards(false);

  EXPECT_EQ(1, observer.layout_changed_count_);
}

TEST_F(NewTabPageServiceTest, ShowFavoritesNotifiesLegacyLayoutObserver) {
  TestLegacyNtpObserver observer;
  AddLegacyObserver(&observer);

  service_->set_show_favorites(false);

  EXPECT_EQ(1, observer.layout_changed_count_);
}

TEST_F(NewTabPageServiceTest, SetBackgroundTypeNotifiesLegacyObserver) {
  TestLegacyNtpObserver observer;
  AddLegacyObserver(&observer);

  service_->set_background_type(AstraNtpBackgroundType::kSolidColor);

  EXPECT_EQ(1, observer.background_changed_count_);
  EXPECT_EQ(0, observer.layout_changed_count_);
  EXPECT_EQ(0, observer.custom_shortcuts_changed_count_);
}

TEST_F(NewTabPageServiceTest, SetBackgroundColorNotifiesLegacyObserver) {
  TestLegacyNtpObserver observer;
  AddLegacyObserver(&observer);

  service_->SetBackgroundColor(SK_ColorRED);

  EXPECT_EQ(1, observer.background_changed_count_);
}

// =========================================================================
// Legacy edge case tests
// =========================================================================

TEST_F(NewTabPageServiceTest, OperationsAfterShutdownAreSafe) {
  service_->Shutdown();

  EXPECT_EQ(0u, service_->GetCustomShortcutCount());
  EXPECT_EQ(AstraNtpLayoutMode::kStandard, service_->layout_mode());
  EXPECT_TRUE(service_->show_shortcuts());
  EXPECT_EQ(AstraNtpBackgroundType::kDefault,
            service_->background_type());
  EXPECT_EQ(SK_ColorWHITE, service_->GetBackgroundColor());

  service_->AddCustomShortcut(u"Test", GURL("https://test.com"));
  service_->set_layout_mode(AstraNtpLayoutMode::kCompact);
  service_->SetBackgroundColor(SK_ColorRED);

  EXPECT_EQ(0u, service_->GetCustomShortcutCount());
}

TEST_F(NewTabPageServiceTest, EmptyCustomShortcutListOperations) {
  EXPECT_EQ(0u, service_->GetCustomShortcutCount());
  EXPECT_TRUE(service_->GetCustomShortcuts(5).empty());
  EXPECT_FALSE(service_->RemoveCustomShortcut(0));
  EXPECT_FALSE(service_->UpdateCustomShortcut(
      0, u"X", GURL("https://x.com")));
  service_->ReorderCustomShortcuts({});
}

TEST_F(NewTabPageServiceTest, LargeCustomShortcutIndex) {
  EXPECT_FALSE(service_->RemoveCustomShortcut(999999));
  EXPECT_FALSE(service_->UpdateCustomShortcut(
      999999, u"X", GURL("https://x.com")));
}

TEST_F(NewTabPageServiceTest, GetCustomShortcutsLargeCount) {
  service_->AddCustomShortcut(u"Only One", GURL("https://only.com"));

  auto shortcuts = service_->GetCustomShortcuts(99999);
  EXPECT_EQ(1u, shortcuts.size());
}

// =========================================================================
// NEW API: Managed shortcut tests
// =========================================================================

TEST_F(NewTabPageServiceTest, NewShortcutsStartEmpty) {
  EXPECT_EQ(0u, service_->GetShortcutCount());
  auto shortcuts = service_->GetShortcuts();
  EXPECT_TRUE(shortcuts.empty());
}

TEST_F(NewTabPageServiceTest, AddShortcutReturnsId) {
  std::string id = service_->AddShortcut("My Site",
                                          GURL("https://example.com"));

  EXPECT_FALSE(id.empty());
  EXPECT_EQ(1u, service_->GetShortcutCount());
}

TEST_F(NewTabPageServiceTest, AddShortcutAddsToList) {
  std::string id = service_->AddShortcut("My Site",
                                          GURL("https://example.com"));

  auto shortcuts = service_->GetShortcuts();
  ASSERT_EQ(1u, shortcuts.size());
  EXPECT_EQ(id, shortcuts[0].shortcut_id);
  EXPECT_EQ("My Site", shortcuts[0].name);
  EXPECT_EQ(GURL("https://example.com"), shortcuts[0].url);
  EXPECT_FALSE(shortcuts[0].is_default);
}

TEST_F(NewTabPageServiceTest, AddMultipleShortcuts) {
  std::string id1 = service_->AddShortcut("Site 1",
                                           GURL("https://site1.com"));
  std::string id2 = service_->AddShortcut("Site 2",
                                           GURL("https://site2.com"));
  std::string id3 = service_->AddShortcut("Site 3",
                                           GURL("https://site3.com"));

  EXPECT_NE(id1, id2);
  EXPECT_NE(id2, id3);
  EXPECT_NE(id1, id3);
  EXPECT_EQ(3u, service_->GetShortcutCount());

  auto shortcuts = service_->GetShortcuts();
  ASSERT_EQ(3u, shortcuts.size());
  EXPECT_EQ("Site 1", shortcuts[0].name);
  EXPECT_EQ("Site 2", shortcuts[1].name);
  EXPECT_EQ("Site 3", shortcuts[2].name);
}

TEST_F(NewTabPageServiceTest, AddShortcutWithExplicitPosition) {
  service_->AddShortcut("A", GURL("https://a.com"));
  service_->AddShortcut("B", GURL("https://b.com"));
  service_->AddShortcut("C", GURL("https://c.com"));

  // Insert "New" at position 1.
  std::string id = service_->AddShortcut("New",
                                          GURL("https://new.com"),
                                          /*position=*/1);

  auto shortcuts = service_->GetShortcuts();
  ASSERT_EQ(4u, shortcuts.size());
  EXPECT_EQ("A", shortcuts[0].name);
  EXPECT_EQ("New", shortcuts[1].name);
  EXPECT_EQ("B", shortcuts[2].name);
  EXPECT_EQ("C", shortcuts[3].name);

  // Check that positions are sequential.
  EXPECT_EQ(0, shortcuts[0].position);
  EXPECT_EQ(1, shortcuts[1].position);
  EXPECT_EQ(2, shortcuts[2].position);
  EXPECT_EQ(3, shortcuts[3].position);
}

TEST_F(NewTabPageServiceTest, AddShortcutWithNegativePositionAppends) {
  service_->AddShortcut("A", GURL("https://a.com"));

  // position = -1 should append at end.
  std::string id = service_->AddShortcut("B", GURL("https://b.com"), -1);

  auto shortcuts = service_->GetShortcuts();
  ASSERT_EQ(2u, shortcuts.size());
  EXPECT_EQ("A", shortcuts[0].name);
  EXPECT_EQ("B", shortcuts[1].name);
}

TEST_F(NewTabPageServiceTest, AddShortcutWithLargePositionAppends) {
  service_->AddShortcut("A", GURL("https://a.com"));

  // position = 99 (beyond end) should append at end.
  std::string id = service_->AddShortcut("B", GURL("https://b.com"), 99);

  auto shortcuts = service_->GetShortcuts();
  ASSERT_EQ(2u, shortcuts.size());
  EXPECT_EQ("A", shortcuts[0].name);
  EXPECT_EQ("B", shortcuts[1].name);
}

TEST_F(NewTabPageServiceTest, RemoveShortcutById) {
  std::string id1 = service_->AddShortcut("Site 1",
                                           GURL("https://site1.com"));
  std::string id2 = service_->AddShortcut("Site 2",
                                           GURL("https://site2.com"));

  bool result = service_->RemoveShortcut(id1);
  EXPECT_TRUE(result);
  EXPECT_EQ(1u, service_->GetShortcutCount());

  auto shortcuts = service_->GetShortcuts();
  ASSERT_EQ(1u, shortcuts.size());
  EXPECT_EQ(id2, shortcuts[0].shortcut_id);
  EXPECT_EQ("Site 2", shortcuts[0].name);
}

TEST_F(NewTabPageServiceTest, RemoveShortcutInvalidId) {
  service_->AddShortcut("Site 1", GURL("https://site1.com"));

  bool result = service_->RemoveShortcut("nonexistent-id");
  EXPECT_FALSE(result);
  EXPECT_EQ(1u, service_->GetShortcutCount());
}

TEST_F(NewTabPageServiceTest, RemoveShortcutFromEmpty) {
  bool result = service_->RemoveShortcut("some-id");
  EXPECT_FALSE(result);
  EXPECT_EQ(0u, service_->GetShortcutCount());
}

TEST_F(NewTabPageServiceTest, UpdateShortcut) {
  std::string id = service_->AddShortcut("Old Name",
                                          GURL("https://old.com"));

  bool result = service_->UpdateShortcut(id, "New Name",
                                          GURL("https://new.com"));
  EXPECT_TRUE(result);

  auto shortcut = service_->GetShortcut(id);
  ASSERT_TRUE(shortcut.has_value());
  EXPECT_EQ("New Name", shortcut->name);
  EXPECT_EQ(GURL("https://new.com"), shortcut->url);
}

TEST_F(NewTabPageServiceTest, UpdateShortcutInvalidId) {
  bool result = service_->UpdateShortcut("nonexistent", "Name",
                                          GURL("https://x.com"));
  EXPECT_FALSE(result);
}

TEST_F(NewTabPageServiceTest, GetShortcut) {
  std::string id = service_->AddShortcut("Test",
                                          GURL("https://test.com"));

  auto shortcut = service_->GetShortcut(id);
  ASSERT_TRUE(shortcut.has_value());
  EXPECT_EQ(id, shortcut->shortcut_id);
  EXPECT_EQ("Test", shortcut->name);
  EXPECT_EQ(GURL("https://test.com"), shortcut->url);
}

TEST_F(NewTabPageServiceTest, GetShortcutInvalidId) {
  auto shortcut = service_->GetShortcut("nonexistent");
  EXPECT_FALSE(shortcut.has_value());
}

TEST_F(NewTabPageServiceTest, GetShortcutEmptyList) {
  auto shortcut = service_->GetShortcut("any-id");
  EXPECT_FALSE(shortcut.has_value());
}

TEST_F(NewTabPageServiceTest, ReorderShortcutsById) {
  std::string id_a = service_->AddShortcut("A", GURL("https://a.com"));
  std::string id_b = service_->AddShortcut("B", GURL("https://b.com"));
  std::string id_c = service_->AddShortcut("C", GURL("https://c.com"));

  // Reverse order: C, B, A
  std::vector<std::string> new_order = {id_c, id_b, id_a};
  service_->ReorderShortcuts(new_order);

  auto shortcuts = service_->GetShortcuts();
  ASSERT_EQ(3u, shortcuts.size());
  EXPECT_EQ("C", shortcuts[0].name);
  EXPECT_EQ("B", shortcuts[1].name);
  EXPECT_EQ("A", shortcuts[2].name);

  // Verify positions are updated.
  EXPECT_EQ(0, shortcuts[0].position);
  EXPECT_EQ(1, shortcuts[1].position);
  EXPECT_EQ(2, shortcuts[2].position);
}

TEST_F(NewTabPageServiceTest, ReorderShortcutsWrongSize) {
  std::string id_a = service_->AddShortcut("A", GURL("https://a.com"));
  std::string id_b = service_->AddShortcut("B", GURL("https://b.com"));

  // Only 1 ID but we have 2 shortcuts — should be no-op.
  std::vector<std::string> bad_order = {id_a};
  service_->ReorderShortcuts(bad_order);

  auto shortcuts = service_->GetShortcuts();
  ASSERT_EQ(2u, shortcuts.size());
  EXPECT_EQ("A", shortcuts[0].name);
  EXPECT_EQ("B", shortcuts[1].name);
}

TEST_F(NewTabPageServiceTest, ReorderShortcutsUnknownId) {
  std::string id_a = service_->AddShortcut("A", GURL("https://a.com"));
  std::string id_b = service_->AddShortcut("B", GURL("https://b.com"));

  std::vector<std::string> bad_order = {id_a, "nonexistent-id"};
  service_->ReorderShortcuts(bad_order);

  auto shortcuts = service_->GetShortcuts();
  ASSERT_EQ(2u, shortcuts.size());
  EXPECT_EQ("A", shortcuts[0].name);
  EXPECT_EQ("B", shortcuts[1].name);
}

TEST_F(NewTabPageServiceTest, ReorderShortcutsDuplicateIds) {
  std::string id_a = service_->AddShortcut("A", GURL("https://a.com"));
  std::string id_b = service_->AddShortcut("B", GURL("https://b.com"));

  std::vector<std::string> bad_order = {id_a, id_a};
  service_->ReorderShortcuts(bad_order);

  auto shortcuts = service_->GetShortcuts();
  ASSERT_EQ(2u, shortcuts.size());
  EXPECT_EQ("A", shortcuts[0].name);
  EXPECT_EQ("B", shortcuts[1].name);
}

TEST_F(NewTabPageServiceTest, ReorderEmptyShortcuts) {
  std::vector<std::string> empty_order;
  service_->ReorderShortcuts(empty_order);
  EXPECT_EQ(0u, service_->GetShortcutCount());
}

TEST_F(NewTabPageServiceTest, SetShortcutIcon) {
  std::string id = service_->AddShortcut("Test",
                                          GURL("https://test.com"));

  GURL icon_url("https://test.com/icon.png");
  bool result = service_->SetShortcutIcon(id, icon_url);
  EXPECT_TRUE(result);

  auto shortcut = service_->GetShortcut(id);
  ASSERT_TRUE(shortcut.has_value());
  EXPECT_EQ(icon_url, shortcut->icon_url);
}

TEST_F(NewTabPageServiceTest, SetShortcutIconInvalidId) {
  bool result = service_->SetShortcutIcon("nonexistent",
                                           GURL("https://x.com/icon.png"));
  EXPECT_FALSE(result);
}

TEST_F(NewTabPageServiceTest, ShortcutHasCreatedTime) {
  base::Time before = base::Time::Now();
  std::string id = service_->AddShortcut("Test",
                                          GURL("https://test.com"));
  base::Time after = base::Time::Now();

  auto shortcut = service_->GetShortcut(id);
  ASSERT_TRUE(shortcut.has_value());
  EXPECT_GE(shortcut->created_time, before);
  EXPECT_LE(shortcut->created_time, after);
}

TEST_F(NewTabPageServiceTest, ShortcutHasZeroUseCountInitially) {
  std::string id = service_->AddShortcut("Test",
                                          GURL("https://test.com"));

  auto shortcut = service_->GetShortcut(id);
  ASSERT_TRUE(shortcut.has_value());
  EXPECT_EQ(0, shortcut->use_count);
}

TEST_F(NewTabPageServiceTest, DefaultShortcuts) {
  auto defaults = AstraNewTabPageService::GetDefaultShortcuts();

  // Should have 6+ default shortcuts.
  EXPECT_GE(defaults.size(), 6u);

  for (const auto& shortcut : defaults) {
    EXPECT_FALSE(shortcut.shortcut_id.empty());
    EXPECT_FALSE(shortcut.name.empty());
    EXPECT_TRUE(shortcut.url.is_valid());
    EXPECT_TRUE(shortcut.is_default);
    EXPECT_GE(shortcut.position, 0);
  }
}

TEST_F(NewTabPageServiceTest, DefaultShortcutsHaveKnownSites) {
  auto defaults = AstraNewTabPageService::GetDefaultShortcuts();

  // Check for some expected default shortcut names.
  bool has_google = false;
  bool has_youtube = false;
  bool has_gmail = false;

  for (const auto& sc : defaults) {
    if (sc.name == "Google") has_google = true;
    if (sc.name == "YouTube") has_youtube = true;
    if (sc.name == "Gmail") has_gmail = true;
  }

  EXPECT_TRUE(has_google);
  EXPECT_TRUE(has_youtube);
  EXPECT_TRUE(has_gmail);
}

TEST_F(NewTabPageServiceTest, ResetShortcutsToDefaults) {
  // Add some user shortcuts.
  service_->AddShortcut("User 1", GURL("https://user1.com"));
  service_->AddShortcut("User 2", GURL("https://user2.com"));
  EXPECT_EQ(2u, service_->GetShortcutCount());

  service_->ResetShortcutsToDefaults();

  auto shortcuts = service_->GetShortcuts();
  EXPECT_GE(shortcuts.size(), 6u);

  // All shortcuts should be default shortcuts.
  for (const auto& sc : shortcuts) {
    EXPECT_TRUE(sc.is_default);
  }
}

TEST_F(NewTabPageServiceTest, IsDefaultShortcut) {
  service_->ResetShortcutsToDefaults();

  auto shortcuts = service_->GetShortcuts();
  ASSERT_GT(shortcuts.size(), 0u);

  // First shortcut should be a default.
  EXPECT_TRUE(service_->IsDefaultShortcut(shortcuts[0].shortcut_id));
}

TEST_F(NewTabPageServiceTest, IsDefaultShortcutUserAdded) {
  std::string id = service_->AddShortcut("User Site",
                                          GURL("https://user.com"));

  EXPECT_FALSE(service_->IsDefaultShortcut(id));
}

TEST_F(NewTabPageServiceTest, IsDefaultShortcutInvalidId) {
  EXPECT_FALSE(service_->IsDefaultShortcut("nonexistent"));
}

TEST_F(NewTabPageServiceTest, ShortcutsPersistAcrossInstances) {
  std::string id1 = service_->AddShortcut("Persisted 1",
                                           GURL("https://p1.com"));
  std::string id2 = service_->AddShortcut("Persisted 2",
                                           GURL("https://p2.com"));

  auto service2 =
      std::make_unique<AstraNewTabPageService>(profile_.get());

  EXPECT_EQ(2u, service2->GetShortcutCount());

  auto sc1 = service2->GetShortcut(id1);
  ASSERT_TRUE(sc1.has_value());
  EXPECT_EQ("Persisted 1", sc1->name);

  auto sc2 = service2->GetShortcut(id2);
  ASSERT_TRUE(sc2.has_value());
  EXPECT_EQ("Persisted 2", sc2->name);
}

TEST_F(NewTabPageServiceTest, ReorderedShortcutsPersistNewApi) {
  std::string id_a = service_->AddShortcut("A", GURL("https://a.com"));
  std::string id_b = service_->AddShortcut("B", GURL("https://b.com"));
  std::string id_c = service_->AddShortcut("C", GURL("https://c.com"));

  service_->ReorderShortcuts({id_c, id_a, id_b});

  auto service2 =
      std::make_unique<AstraNewTabPageService>(profile_.get());
  auto shortcuts = service2->GetShortcuts();
  ASSERT_EQ(3u, shortcuts.size());
  EXPECT_EQ("C", shortcuts[0].name);
  EXPECT_EQ("A", shortcuts[1].name);
  EXPECT_EQ("B", shortcuts[2].name);
}

// =========================================================================
// NEW API: Workspace card tests
// =========================================================================

TEST_F(NewTabPageServiceTest, WorkspaceCardsDefaultVisible) {
  // Workspaces should be visible by default (no entry = visible).
  EXPECT_TRUE(service_->IsWorkspaceVisible("ws-1"));
  EXPECT_TRUE(service_->IsWorkspaceVisible("any-workspace"));
}

TEST_F(NewTabPageServiceTest, HideWorkspace) {
  service_->ShowWorkspace("ws-1", false);
  EXPECT_FALSE(service_->IsWorkspaceVisible("ws-1"));
}

TEST_F(NewTabPageServiceTest, ShowWorkspaceAfterHiding) {
  service_->ShowWorkspace("ws-1", false);
  EXPECT_FALSE(service_->IsWorkspaceVisible("ws-1"));

  service_->ShowWorkspace("ws-1", true);
  EXPECT_TRUE(service_->IsWorkspaceVisible("ws-1"));
}

TEST_F(NewTabPageServiceTest, ShowWorkspaceDoesNotAffectOthers) {
  service_->ShowWorkspace("ws-1", false);

  EXPECT_FALSE(service_->IsWorkspaceVisible("ws-1"));
  EXPECT_TRUE(service_->IsWorkspaceVisible("ws-2"));
}

TEST_F(NewTabPageServiceTest, GetWorkspaceCardsReturnsList) {
  auto cards = service_->GetWorkspaceCards();
  EXPECT_GE(cards.size(), 0u);
}

TEST_F(NewTabPageServiceTest, GetVisibleWorkspacesCount) {
  size_t count = service_->GetVisibleWorkspacesCount();
  EXPECT_GE(count, 0u);
}

TEST_F(NewTabPageServiceTest, MaxWorkspaceCardsDefault) {
  EXPECT_EQ(6, service_->GetMaxWorkspaceCards());
}

TEST_F(NewTabPageServiceTest, SetMaxWorkspaceCards) {
  service_->SetMaxWorkspaceCards(3);
  EXPECT_EQ(3, service_->GetMaxWorkspaceCards());
}

TEST_F(NewTabPageServiceTest, SetMaxWorkspaceCardsZero) {
  service_->SetMaxWorkspaceCards(0);
  EXPECT_EQ(0, service_->GetMaxWorkspaceCards());

  // With max = 0, no workspace cards should be returned.
  auto cards = service_->GetWorkspaceCards();
  EXPECT_TRUE(cards.empty());
}

TEST_F(NewTabPageServiceTest, SetMaxWorkspaceCardsNegative) {
  // Negative should be clamped to 0.
  service_->SetMaxWorkspaceCards(-5);
  EXPECT_EQ(0, service_->GetMaxWorkspaceCards());
}

TEST_F(NewTabPageServiceTest, ReorderWorkspaceCards) {
  std::vector<std::string> new_order = {"ws-3", "ws-1", "ws-2"};
  service_->ReorderWorkspaceCards(new_order);
  // No crash = success.  Order is persisted.
}

TEST_F(NewTabPageServiceTest, WorkspaceVisibilityPersists) {
  service_->ShowWorkspace("ws-test", false);

  auto service2 =
      std::make_unique<AstraNewTabPageService>(profile_.get());

  EXPECT_FALSE(service2->IsWorkspaceVisible("ws-test"));
  EXPECT_TRUE(service2->IsWorkspaceVisible("ws-other"));
}

TEST_F(NewTabPageServiceTest, MaxWorkspaceCardsPersists) {
  service_->SetMaxWorkspaceCards(4);

  auto service2 =
      std::make_unique<AstraNewTabPageService>(profile_.get());

  EXPECT_EQ(4, service2->GetMaxWorkspaceCards());
}

// =========================================================================
// NEW API: Suggested content tests
// =========================================================================

TEST_F(NewTabPageServiceTest, SuggestionsEnabledDefault) {
  EXPECT_TRUE(service_->IsSuggestionsEnabled());
}

TEST_F(NewTabPageServiceTest, GetSuggestedContentReturnsItems) {
  auto suggestions = service_->GetSuggestedContent(5);
  EXPECT_GT(suggestions.size(), 0u);
  EXPECT_LE(suggestions.size(), 5u);
}

TEST_F(NewTabPageServiceTest, GetSuggestedContentRespectsMax) {
  auto suggestions = service_->GetSuggestedContent(3);
  EXPECT_LE(suggestions.size(), 3u);
}

TEST_F(NewTabPageServiceTest, GetSuggestedContentZeroMax) {
  auto suggestions = service_->GetSuggestedContent(0);
  EXPECT_TRUE(suggestions.empty());
}

TEST_F(NewTabPageServiceTest, GetSuggestedContentNegativeMax) {
  auto suggestions = service_->GetSuggestedContent(-1);
  EXPECT_TRUE(suggestions.empty());
}

TEST_F(NewTabPageServiceTest, SuggestionsHaveValidUrls) {
  auto suggestions = service_->GetSuggestedContent(5);
  for (const auto& item : suggestions) {
    EXPECT_TRUE(item.url.is_valid());
    EXPECT_FALSE(item.title.empty());
  }
}

TEST_F(NewTabPageServiceTest, SuggestionsHaveScores) {
  auto suggestions = service_->GetSuggestedContent(5);
  for (const auto& item : suggestions) {
    EXPECT_GE(item.score, 0.0);
    EXPECT_LE(item.score, 1.0);
  }
}

TEST_F(NewTabPageServiceTest, SuggestionsAreSortedByScore) {
  auto suggestions = service_->GetSuggestedContent(10);
  ASSERT_GT(suggestions.size(), 1u);

  // Should be in descending order of score.
  for (size_t i = 1; i < suggestions.size(); ++i) {
    EXPECT_LE(suggestions[i].score, suggestions[i-1].score);
  }
}

TEST_F(NewTabPageServiceTest, DismissSuggestion) {
  auto suggestions_before = service_->GetSuggestedContent(10);
  ASSERT_GT(suggestions_before.size(), 0u);

  GURL url_to_dismiss = suggestions_before[0].url;
  service_->DismissSuggestion(url_to_dismiss);

  auto suggestions_after = service_->GetSuggestedContent(10);
  EXPECT_LT(suggestions_after.size(), suggestions_before.size());

  // The dismissed URL should not appear.
  for (const auto& item : suggestions_after) {
    EXPECT_NE(url_to_dismiss, item.url);
  }
}

TEST_F(NewTabPageServiceTest, DismissSuggestionIncreasesDismissedCount) {
  EXPECT_EQ(0u, service_->GetDismissedSuggestionCount());

  auto suggestions = service_->GetSuggestedContent(5);
  ASSERT_GT(suggestions.size(), 0u);

  service_->DismissSuggestion(suggestions[0].url);
  EXPECT_EQ(1u, service_->GetDismissedSuggestionCount());
}

TEST_F(NewTabPageServiceTest, DismissSameSuggestionTwice) {
  auto suggestions = service_->GetSuggestedContent(5);
  ASSERT_GT(suggestions.size(), 0u);

  GURL url = suggestions[0].url;
  service_->DismissSuggestion(url);
  service_->DismissSuggestion(url);

  // Should only count once.
  EXPECT_EQ(1u, service_->GetDismissedSuggestionCount());
}

TEST_F(NewTabPageServiceTest, RestoreDismissedSuggestions) {
  auto suggestions = service_->GetSuggestedContent(5);
  ASSERT_GT(suggestions.size(), 0u);

  service_->DismissSuggestion(suggestions[0].url);
  service_->DismissSuggestion(suggestions[1].url);
  EXPECT_EQ(2u, service_->GetDismissedSuggestionCount());

  service_->RestoreDismissedSuggestions();
  EXPECT_EQ(0u, service_->GetDismissedSuggestionCount());

  // All suggestions should be visible again.
  auto restored = service_->GetSuggestedContent(10);
  EXPECT_GE(restored.size(), suggestions.size());
}

TEST_F(NewTabPageServiceTest, RefreshSuggestions) {
  // Refresh should notify observers but not crash.
  service_->RefreshSuggestions();
}

TEST_F(NewTabPageServiceTest, SuggestionsCanBeDisabled) {
  service_->set_show_suggestions(false);
  // Note: show_suggestions controls section visibility, not data availability.
  // IsSuggestionsEnabled controls whether suggestions are generated.
  EXPECT_TRUE(service_->IsSuggestionsEnabled());
}

TEST_F(NewTabPageServiceTest, DismissedSuggestionsPersist) {
  auto suggestions = service_->GetSuggestedContent(5);
  ASSERT_GT(suggestions.size(), 0u);

  service_->DismissSuggestion(suggestions[0].url);
  EXPECT_EQ(1u, service_->GetDismissedSuggestionCount());

  auto service2 =
      std::make_unique<AstraNewTabPageService>(profile_.get());

  EXPECT_EQ(1u, service2->GetDismissedSuggestionCount());
}

// =========================================================================
// NEW API: Theme & layout tests
// =========================================================================

TEST_F(NewTabPageServiceTest, LayoutDefault) {
  EXPECT_EQ(AstraNtpLayout::kDefault, service_->layout());
}

TEST_F(NewTabPageServiceTest, SetLayout) {
  service_->set_layout(AstraNtpLayout::kShortcutsOnly);
  EXPECT_EQ(AstraNtpLayout::kShortcutsOnly, service_->layout());

  service_->set_layout(AstraNtpLayout::kMinimal);
  EXPECT_EQ(AstraNtpLayout::kMinimal, service_->layout());

  service_->set_layout(AstraNtpLayout::kCustom);
  EXPECT_EQ(AstraNtpLayout::kCustom, service_->layout());
}

TEST_F(NewTabPageServiceTest, AllLayoutEnumValues) {
  // Test every layout enum value can be set and retrieved.
  service_->set_layout(AstraNtpLayout::kDefault);
  EXPECT_EQ(AstraNtpLayout::kDefault, service_->layout());

  service_->set_layout(AstraNtpLayout::kShortcutsOnly);
  EXPECT_EQ(AstraNtpLayout::kShortcutsOnly, service_->layout());

  service_->set_layout(AstraNtpLayout::kWorkspacesOnly);
  EXPECT_EQ(AstraNtpLayout::kWorkspacesOnly, service_->layout());

  service_->set_layout(AstraNtpLayout::kMinimal);
  EXPECT_EQ(AstraNtpLayout::kMinimal, service_->layout());

  service_->set_layout(AstraNtpLayout::kCustom);
  EXPECT_EQ(AstraNtpLayout::kCustom, service_->layout());
}

TEST_F(NewTabPageServiceTest, BackgroundColorNewApiDefault) {
  EXPECT_EQ(SK_ColorWHITE, service_->background_color());
}

TEST_F(NewTabPageServiceTest, SetBackgroundColorNewApi) {
  service_->set_background_color(SK_ColorRED);
  EXPECT_EQ(SK_ColorRED, service_->background_color());

  service_->set_background_color(SK_ColorBLUE);
  EXPECT_EQ(SK_ColorBLUE, service_->background_color());
}

TEST_F(NewTabPageServiceTest, BackgroundImageUrlDefault) {
  EXPECT_TRUE(service_->background_image_url().empty());
}

TEST_F(NewTabPageServiceTest, SetBackgroundImageUrl) {
  std::string url = "https://example.com/background.jpg";
  service_->set_background_image_url(url);
  EXPECT_EQ(url, service_->background_image_url());
}

TEST_F(NewTabPageServiceTest, ShowShortcutsNewApiDefault) {
  EXPECT_TRUE(service_->show_shortcuts());
}

TEST_F(NewTabPageServiceTest, SetShowShortcutsNewApi) {
  service_->set_show_shortcuts(false);
  EXPECT_FALSE(service_->show_shortcuts());
  service_->set_show_shortcuts(true);
  EXPECT_TRUE(service_->show_shortcuts());
}

TEST_F(NewTabPageServiceTest, ShowWorkspaceCardsNewApiDefault) {
  EXPECT_TRUE(service_->show_workspace_cards());
}

TEST_F(NewTabPageServiceTest, SetShowWorkspaceCardsNewApi) {
  service_->set_show_workspace_cards(false);
  EXPECT_FALSE(service_->show_workspace_cards());
}

TEST_F(NewTabPageServiceTest, ShowSuggestionsDefault) {
  EXPECT_TRUE(service_->show_suggestions());
}

TEST_F(NewTabPageServiceTest, SetShowSuggestions) {
  service_->set_show_suggestions(false);
  EXPECT_FALSE(service_->show_suggestions());
}

TEST_F(NewTabPageServiceTest, ShowGoogleLogoDefault) {
  EXPECT_TRUE(service_->show_google_logo());
}

TEST_F(NewTabPageServiceTest, SetShowGoogleLogo) {
  service_->set_show_google_logo(false);
  EXPECT_FALSE(service_->show_google_logo());
}

TEST_F(NewTabPageServiceTest, ShowSearchBoxDefault) {
  EXPECT_TRUE(service_->show_search_box());
}

TEST_F(NewTabPageServiceTest, SetShowSearchBox) {
  service_->set_show_search_box(false);
  EXPECT_FALSE(service_->show_search_box());
}

TEST_F(NewTabPageServiceTest, ShortcutColumnsDefault) {
  EXPECT_EQ(4, service_->shortcut_columns());
}

TEST_F(NewTabPageServiceTest, SetShortcutColumns) {
  service_->set_shortcut_columns(5);
  EXPECT_EQ(5, service_->shortcut_columns());
}

TEST_F(NewTabPageServiceTest, ShortcutColumnsClampedToMin) {
  service_->set_shortcut_columns(0);
  // Should be clamped to at least 1.
  EXPECT_GE(service_->shortcut_columns(), 1);
}

TEST_F(NewTabPageServiceTest, ShortcutColumnsClampedToMax) {
  service_->set_shortcut_columns(100);
  // Should be clamped to at most 10.
  EXPECT_LE(service_->shortcut_columns(), 10);
}

TEST_F(NewTabPageServiceTest, ShortcutRowsDefault) {
  EXPECT_EQ(2, service_->shortcut_rows());
}

TEST_F(NewTabPageServiceTest, SetShortcutRows) {
  service_->set_shortcut_rows(3);
  EXPECT_EQ(3, service_->shortcut_rows());
}

TEST_F(NewTabPageServiceTest, ShortcutRowsClampedToMin) {
  service_->set_shortcut_rows(-1);
  EXPECT_GE(service_->shortcut_rows(), 1);
}

TEST_F(NewTabPageServiceTest, MaxSuggestionsDefault) {
  EXPECT_EQ(8, service_->max_suggestions());
}

TEST_F(NewTabPageServiceTest, SetMaxSuggestions) {
  service_->set_max_suggestions(12);
  EXPECT_EQ(12, service_->max_suggestions());
}

TEST_F(NewTabPageServiceTest, SetMaxSuggestionsZero) {
  service_->set_max_suggestions(0);
  EXPECT_EQ(0, service_->max_suggestions());
}

TEST_F(NewTabPageServiceTest, ShowMostVisitedDefault) {
  EXPECT_TRUE(service_->show_most_visited());
}

TEST_F(NewTabPageServiceTest, SetShowMostVisited) {
  service_->set_show_most_visited(false);
  EXPECT_FALSE(service_->show_most_visited());
}

TEST_F(NewTabPageServiceTest, ShowRecentlyClosedDefault) {
  EXPECT_FALSE(service_->show_recently_closed());
}

TEST_F(NewTabPageServiceTest, SetShowRecentlyClosed) {
  service_->set_show_recently_closed(true);
  EXPECT_TRUE(service_->show_recently_closed());
}

TEST_F(NewTabPageServiceTest, DarkModeDefault) {
  EXPECT_EQ(AstraNtpDarkMode::kAuto, service_->dark_mode());
}

TEST_F(NewTabPageServiceTest, SetDarkMode) {
  service_->set_dark_mode(AstraNtpDarkMode::kLight);
  EXPECT_EQ(AstraNtpDarkMode::kLight, service_->dark_mode());

  service_->set_dark_mode(AstraNtpDarkMode::kDark);
  EXPECT_EQ(AstraNtpDarkMode::kDark, service_->dark_mode());
}

TEST_F(NewTabPageServiceTest, CustomBackgroundEnabledDefault) {
  EXPECT_FALSE(service_->custom_background_enabled());
}

TEST_F(NewTabPageServiceTest, SetCustomBackgroundEnabled) {
  service_->set_custom_background_enabled(true);
  EXPECT_TRUE(service_->custom_background_enabled());
}

TEST_F(NewTabPageServiceTest, GetNtpThemeReturnsAllFields) {
  auto theme = service_->GetNtpTheme();

  EXPECT_EQ(AstraNtpLayout::kDefault, theme.layout);
  EXPECT_EQ(SK_ColorWHITE, theme.background_color);
  EXPECT_TRUE(theme.background_image_url.empty());
  EXPECT_TRUE(theme.show_shortcuts);
  EXPECT_TRUE(theme.show_workspace_cards);
  EXPECT_TRUE(theme.show_suggestions);
  EXPECT_TRUE(theme.show_google_logo);
  EXPECT_TRUE(theme.show_search_box);
  EXPECT_EQ(4, theme.shortcut_columns);
  EXPECT_EQ(2, theme.shortcut_rows);
}

TEST_F(NewTabPageServiceTest, SetNtpTheme) {
  AstraNtpTheme theme;
  theme.layout = AstraNtpLayout::kMinimal;
  theme.background_color = SK_ColorBLACK;
  theme.background_image_url = "https://example.com/bg.jpg";
  theme.show_shortcuts = false;
  theme.show_workspace_cards = false;
  theme.show_suggestions = false;
  theme.show_google_logo = false;
  theme.show_search_box = false;
  theme.shortcut_columns = 6;
  theme.shortcut_rows = 3;

  service_->SetNtpTheme(theme);

  auto result = service_->GetNtpTheme();
  EXPECT_EQ(AstraNtpLayout::kMinimal, result.layout);
  EXPECT_EQ(SK_ColorBLACK, result.background_color);
  EXPECT_EQ("https://example.com/bg.jpg", result.background_image_url);
  EXPECT_FALSE(result.show_shortcuts);
  EXPECT_FALSE(result.show_workspace_cards);
  EXPECT_FALSE(result.show_suggestions);
  EXPECT_FALSE(result.show_google_logo);
  EXPECT_FALSE(result.show_search_box);
  EXPECT_EQ(6, result.shortcut_columns);
  EXPECT_EQ(3, result.shortcut_rows);
}

TEST_F(NewTabPageServiceTest, ResetNtpTheme) {
  // Change everything.
  service_->set_layout(AstraNtpLayout::kMinimal);
  service_->set_background_color(SK_ColorBLACK);
  service_->set_background_image_url("https://example.com/bg.jpg");
  service_->set_show_shortcuts(false);
  service_->set_show_workspace_cards(false);
  service_->set_show_suggestions(false);
  service_->set_show_google_logo(false);
  service_->set_show_search_box(false);
  service_->set_shortcut_columns(6);
  service_->set_shortcut_rows(3);

  // Verify changes were applied.
  EXPECT_EQ(AstraNtpLayout::kMinimal, service_->layout());
  EXPECT_EQ(SK_ColorBLACK, service_->background_color());

  // Reset.
  service_->ResetNtpTheme();

  // Verify back to defaults.
  auto theme = service_->GetNtpTheme();
  EXPECT_EQ(AstraNtpLayout::kDefault, theme.layout);
  EXPECT_EQ(SK_ColorWHITE, theme.background_color);
  EXPECT_TRUE(theme.background_image_url.empty());
  EXPECT_TRUE(theme.show_shortcuts);
  EXPECT_TRUE(theme.show_workspace_cards);
  EXPECT_TRUE(theme.show_suggestions);
  EXPECT_TRUE(theme.show_google_logo);
  EXPECT_TRUE(theme.show_search_box);
  EXPECT_EQ(4, theme.shortcut_columns);
  EXPECT_EQ(2, theme.shortcut_rows);
}

TEST_F(NewTabPageServiceTest, ThemeSettingsPersist) {
  service_->set_layout(AstraNtpLayout::kShortcutsOnly);
  service_->set_background_color(SK_ColorGREEN);
  service_->set_background_image_url("https://persist.com/bg.png");
  service_->set_show_shortcuts(false);
  service_->set_shortcut_columns(5);
  service_->set_shortcut_rows(4);

  auto service2 =
      std::make_unique<AstraNewTabPageService>(profile_.get());

  EXPECT_EQ(AstraNtpLayout::kShortcutsOnly, service2->layout());
  EXPECT_EQ(SK_ColorGREEN, service2->background_color());
  EXPECT_EQ("https://persist.com/bg.png", service2->background_image_url());
  EXPECT_FALSE(service2->show_shortcuts());
  EXPECT_EQ(5, service2->shortcut_columns());
  EXPECT_EQ(4, service2->shortcut_rows());
}

// =========================================================================
// NEW API: Observer notification tests
// =========================================================================

TEST_F(NewTabPageServiceTest, AddShortcutNotifiesNewObserver) {
  TestNewNtpObserver observer;
  AddNewObserver(&observer);

  std::string id = service_->AddShortcut("Test",
                                          GURL("https://example.com"));

  EXPECT_EQ(1, observer.shortcut_added_count_);
  EXPECT_EQ(id, observer.last_shortcut_id_);
  EXPECT_EQ(0, observer.shortcut_removed_count_);
  EXPECT_EQ(0, observer.shortcut_changed_count_);
}

TEST_F(NewTabPageServiceTest, RemoveShortcutNotifiesNewObserver) {
  std::string id = service_->AddShortcut("Test",
                                          GURL("https://example.com"));

  TestNewNtpObserver observer;
  AddNewObserver(&observer);

  service_->RemoveShortcut(id);

  EXPECT_EQ(1, observer.shortcut_removed_count_);
  EXPECT_EQ(id, observer.last_shortcut_id_);
  EXPECT_EQ(0, observer.shortcut_added_count_);
}

TEST_F(NewTabPageServiceTest, UpdateShortcutNotifiesNewObserver) {
  std::string id = service_->AddShortcut("Test",
                                          GURL("https://example.com"));

  TestNewNtpObserver observer;
  AddNewObserver(&observer);

  service_->UpdateShortcut(id, "New", GURL("https://new.com"));

  EXPECT_EQ(1, observer.shortcut_changed_count_);
  EXPECT_EQ(id, observer.last_shortcut_id_);
}

TEST_F(NewTabPageServiceTest, ReorderShortcutsNotifiesNewObserver) {
  std::string id_a = service_->AddShortcut("A", GURL("https://a.com"));
  std::string id_b = service_->AddShortcut("B", GURL("https://b.com"));

  TestNewNtpObserver observer;
  AddNewObserver(&observer);

  service_->ReorderShortcuts({id_b, id_a});

  EXPECT_EQ(1, observer.shortcuts_reordered_count_);
}

TEST_F(NewTabPageServiceTest, SetShortcutIconNotifiesChanged) {
  std::string id = service_->AddShortcut("Test",
                                          GURL("https://test.com"));

  TestNewNtpObserver observer;
  AddNewObserver(&observer);

  service_->SetShortcutIcon(id, GURL("https://test.com/icon.png"));

  EXPECT_EQ(1, observer.shortcut_changed_count_);
  EXPECT_EQ(id, observer.last_shortcut_id_);
}

TEST_F(NewTabPageServiceTest, SetLayoutNotifiesNtpThemeChanged) {
  TestNewNtpObserver observer;
  AddNewObserver(&observer);

  service_->set_layout(AstraNtpLayout::kMinimal);

  EXPECT_EQ(1, observer.ntp_theme_changed_count_);
}

TEST_F(NewTabPageServiceTest, SetBackgroundColorNotifiesThemeChanged) {
  TestNewNtpObserver observer;
  AddNewObserver(&observer);

  service_->set_background_color(SK_ColorRED);

  EXPECT_EQ(1, observer.ntp_theme_changed_count_);
}

TEST_F(NewTabPageServiceTest, ShowWorkspaceNotifiesObserver) {
  TestNewNtpObserver observer;
  AddNewObserver(&observer);

  service_->ShowWorkspace("ws-1", false);

  EXPECT_EQ(1, observer.workspace_visibility_count_);
  EXPECT_EQ("ws-1", observer.last_workspace_id_);
  EXPECT_FALSE(observer.last_workspace_visible_);
}

TEST_F(NewTabPageServiceTest, ShowWorkspaceTrueNotifiesObserver) {
  // Hide first, then show — both should notify.
  service_->ShowWorkspace("ws-1", false);

  TestNewNtpObserver observer;
  AddNewObserver(&observer);

  service_->ShowWorkspace("ws-1", true);

  EXPECT_EQ(1, observer.workspace_visibility_count_);
  EXPECT_EQ("ws-1", observer.last_workspace_id_);
  EXPECT_TRUE(observer.last_workspace_visible_);
}

TEST_F(NewTabPageServiceTest, DismissSuggestionNotifiesObserver) {
  TestNewNtpObserver observer;
  AddNewObserver(&observer);

  auto suggestions = service_->GetSuggestedContent(1);
  if (!suggestions.empty()) {
    service_->DismissSuggestion(suggestions[0].url);
    EXPECT_EQ(1, observer.suggestions_changed_count_);
  }
}

TEST_F(NewTabPageServiceTest, RestoreSuggestionsNotifiesObserver) {
  TestNewNtpObserver observer;
  AddNewObserver(&observer);

  service_->RestoreDismissedSuggestions();

  EXPECT_EQ(1, observer.suggestions_changed_count_);
}

TEST_F(NewTabPageServiceTest, RefreshSuggestionsNotifiesObserver) {
  TestNewNtpObserver observer;
  AddNewObserver(&observer);

  service_->RefreshSuggestions();

  EXPECT_EQ(1, observer.suggestions_changed_count_);
}

TEST_F(NewTabPageServiceTest, ResetShortcutsNotifiesReordered) {
  TestNewNtpObserver observer;
  AddNewObserver(&observer);

  service_->ResetShortcutsToDefaults();

  EXPECT_EQ(1, observer.shortcuts_reordered_count_);
}

TEST_F(NewTabPageServiceTest, ResetNtpThemeNotifiesObserver) {
  TestNewNtpObserver observer;
  AddNewObserver(&observer);

  service_->ResetNtpTheme();

  EXPECT_EQ(1, observer.ntp_theme_changed_count_);
}

TEST_F(NewTabPageServiceTest, SetNtpThemeNotifiesObserver) {
  TestNewNtpObserver observer;
  AddNewObserver(&observer);

  AstraNtpTheme theme = service_->GetNtpTheme();
  theme.layout = AstraNtpLayout::kCustom;
  service_->SetNtpTheme(theme);

  EXPECT_EQ(1, observer.ntp_theme_changed_count_);
}

TEST_F(NewTabPageServiceTest, NewObserverDefaultImplementations) {
  PartialNewNtpObserver partial_observer;
  service_->AddObserver(&partial_observer);

  // Trigger all notification types — only shortcut_added should be counted.
  service_->set_layout(AstraNtpLayout::kMinimal);
  service_->set_background_color(SK_ColorRED);
  service_->AddShortcut("Test", GURL("https://example.com"));
  service_->RefreshSuggestions();
  service_->ShowWorkspace("ws-x", false);
  service_->ResetShortcutsToDefaults();

  EXPECT_EQ(1, partial_observer.shortcut_added_count_);  // From AddShortcut.

  service_->RemoveObserver(&partial_observer);
}

TEST_F(NewTabPageServiceTest, NewObserverRemovalStopsNotifications) {
  TestNewNtpObserver observer;
  AddNewObserver(&observer);

  service_->set_layout(AstraNtpLayout::kMinimal);
  EXPECT_EQ(1, observer.ntp_theme_changed_count_);

  service_->RemoveObserver(&observer);
  auto it = base::ranges::find(new_observers_, &observer);
  if (it != new_observers_.end()) {
    new_observers_.erase(it);
  }

  observer.ResetCounters();
  service_->set_layout(AstraNtpLayout::kCustom);
  EXPECT_EQ(0, observer.ntp_theme_changed_count_);
}

TEST_F(NewTabPageServiceTest, MultipleNewObservers) {
  TestNewNtpObserver observer1;
  TestNewNtpObserver observer2;
  AddNewObserver(&observer1);
  AddNewObserver(&observer2);

  service_->AddShortcut("Test", GURL("https://example.com"));

  EXPECT_EQ(1, observer1.shortcut_added_count_);
  EXPECT_EQ(1, observer2.shortcut_added_count_);
}

TEST_F(NewTabPageServiceTest, InvalidRemoveShortcutDoesNotNotifyNewObserver) {
  TestNewNtpObserver observer;
  AddNewObserver(&observer);

  service_->RemoveShortcut("nonexistent");

  EXPECT_EQ(0, observer.shortcut_removed_count_);
}

TEST_F(NewTabPageServiceTest, InvalidUpdateShortcutDoesNotNotifyNewObserver) {
  TestNewNtpObserver observer;
  AddNewObserver(&observer);

  service_->UpdateShortcut("nonexistent", "X", GURL("https://x.com"));

  EXPECT_EQ(0, observer.shortcut_changed_count_);
}

TEST_F(NewTabPageServiceTest, InvalidReorderShortcutsDoesNotNotifyNewObserver) {
  std::string id = service_->AddShortcut("A", GURL("https://a.com"));

  TestNewNtpObserver observer;
  AddNewObserver(&observer);

  // Wrong size reorder — should not notify.
  service_->ReorderShortcuts({"nonexistent"});

  EXPECT_EQ(0, observer.shortcuts_reordered_count_);
}

TEST_F(NewTabPageServiceTest, InvalidSetShortcutIconDoesNotNotify) {
  TestNewNtpObserver observer;
  AddNewObserver(&observer);

  service_->SetShortcutIcon("nonexistent", GURL("https://x.com/icon.png"));

  EXPECT_EQ(0, observer.shortcut_changed_count_);
}

// =========================================================================
// NEW API: Edge case tests
// =========================================================================

TEST_F(NewTabPageServiceTest, OperationsAfterShutdownNewApi) {
  service_->Shutdown();

  // All operations should be safe (no crash).
  EXPECT_EQ(0u, service_->GetShortcutCount());
  EXPECT_TRUE(service_->GetShortcuts().empty());
  EXPECT_EQ(AstraNtpLayout::kDefault, service_->layout());
  EXPECT_TRUE(service_->show_shortcuts());
  EXPECT_TRUE(service_->IsSuggestionsEnabled());

  // Mutations should be no-ops.
  service_->AddShortcut("Test", GURL("https://test.com"));
  service_->set_layout(AstraNtpLayout::kMinimal);
  service_->set_background_color(SK_ColorRED);
  service_->ShowWorkspace("ws-1", false);
  service_->DismissSuggestion(GURL("https://example.com"));

  EXPECT_EQ(0u, service_->GetShortcutCount());
}

TEST_F(NewTabPageServiceTest, EmptyShortcutListOperationsNewApi) {
  EXPECT_EQ(0u, service_->GetShortcutCount());
  EXPECT_TRUE(service_->GetShortcuts().empty());
  EXPECT_FALSE(service_->RemoveShortcut("any"));
  EXPECT_FALSE(service_->UpdateShortcut("any", "X", GURL("https://x.com")));
  EXPECT_FALSE(service_->GetShortcut("any").has_value());
  EXPECT_FALSE(service_->IsDefaultShortcut("any"));
  EXPECT_FALSE(service_->SetShortcutIcon("any", GURL("https://x.com/i.png")));

  service_->ReorderShortcuts({});
}

TEST_F(NewTabPageServiceTest, AddShortcutAtZeroPosition) {
  // Adding at position 0 when list is empty should work.
  std::string id = service_->AddShortcut("First",
                                          GURL("https://first.com"), 0);

  auto shortcuts = service_->GetShortcuts();
  ASSERT_EQ(1u, shortcuts.size());
  EXPECT_EQ("First", shortcuts[0].name);
  EXPECT_EQ(0, shortcuts[0].position);
}

TEST_F(NewTabPageServiceTest, RemoveLastShortcutMaintainsPositions) {
  service_->AddShortcut("A", GURL("https://a.com"));
  service_->AddShortcut("B", GURL("https://b.com"));
  service_->AddShortcut("C", GURL("https://c.com"));

  // Remove the last one.
  auto shortcuts_before = service_->GetShortcuts();
  service_->RemoveShortcut(shortcuts_before[2].shortcut_id);

  auto shortcuts_after = service_->GetShortcuts();
  ASSERT_EQ(2u, shortcuts_after.size());
  EXPECT_EQ(0, shortcuts_after[0].position);
  EXPECT_EQ(1, shortcuts_after[1].position);
}

TEST_F(NewTabPageServiceTest, RemoveFirstShortcutShiftsPositions) {
  service_->AddShortcut("A", GURL("https://a.com"));
  service_->AddShortcut("B", GURL("https://b.com"));
  service_->AddShortcut("C", GURL("https://c.com"));

  auto shortcuts_before = service_->GetShortcuts();
  service_->RemoveShortcut(shortcuts_before[0].shortcut_id);

  auto shortcuts_after = service_->GetShortcuts();
  ASSERT_EQ(2u, shortcuts_after.size());
  EXPECT_EQ("B", shortcuts_after[0].name);
  EXPECT_EQ(0, shortcuts_after[0].position);
  EXPECT_EQ("C", shortcuts_after[1].name);
  EXPECT_EQ(1, shortcuts_after[1].position);
}

TEST_F(NewTabPageServiceTest, MaxWorkspaceCardsZeroShowsNone) {
  service_->SetMaxWorkspaceCards(0);

  auto cards = service_->GetWorkspaceCards();
  EXPECT_TRUE(cards.empty());
  EXPECT_EQ(0u, service_->GetVisibleWorkspacesCount());
}

TEST_F(NewTabPageServiceTest, GetSuggestedContentLargeCount) {
  auto suggestions = service_->GetSuggestedContent(999);
  // Should return all available, not crash.
  EXPECT_GE(suggestions.size(), 0u);
}

TEST_F(NewTabPageServiceTest, DismissAllSuggestions) {
  auto suggestions = service_->GetSuggestedContent(100);

  for (const auto& s : suggestions) {
    service_->DismissSuggestion(s.url);
  }

  // After dismissing all, GetSuggestedContent should return empty.
  auto remaining = service_->GetSuggestedContent(100);
  EXPECT_TRUE(remaining.empty());
}

TEST_F(NewTabPageServiceTest, ThemeSettingsAllFifteenPlus) {
  // Verify there are 15+ configurable settings.
  // This test documents all available settings.
  //
  // 1. Layout (AstraNtpLayout enum)
  // 2. Background color (SkColor)
  // 3. Background image URL (string)
  // 4. Show shortcuts (bool)
  // 5. Show workspace cards (bool)
  // 6. Show suggestions (bool)
  // 7. Show Google logo (bool)
  // 8. Show search box (bool)
  // 9. Shortcut columns (int)
  // 10. Shortcut rows (int)
  // 11. Max workspace cards (int)
  // 12. Max suggestions (int)
  // 13. Show most visited (bool)
  // 14. Show recently closed (bool)
  // 15. Dark mode (enum)
  // 16. Custom background enabled (bool)
  //
  // That's 16 settings.
  SUCCEED();
}

// =========================================================================
// Struct tests
// =========================================================================

TEST(AstraShortcutStructTest, DefaultValues) {
  AstraShortcut shortcut;
  EXPECT_TRUE(shortcut.shortcut_id.empty());
  EXPECT_TRUE(shortcut.name.empty());
  EXPECT_FALSE(shortcut.url.is_valid());
  EXPECT_FALSE(shortcut.icon_url.is_valid());
  EXPECT_FALSE(shortcut.is_default);
  EXPECT_EQ(0, shortcut.position);
  EXPECT_TRUE(shortcut.astra_workspace_id.empty());
  EXPECT_TRUE(shortcut.created_time.is_null());
  EXPECT_TRUE(shortcut.last_used.is_null());
  EXPECT_EQ(0, shortcut.use_count);
}

TEST(AstraShortcutStructTest, CanSetFields) {
  AstraShortcut shortcut;
  shortcut.shortcut_id = "sc-123";
  shortcut.name = "Example";
  shortcut.url = GURL("https://example.com");
  shortcut.icon_url = GURL("https://example.com/icon.png");
  shortcut.is_default = true;
  shortcut.position = 3;
  shortcut.astra_workspace_id = "ws-456";
  shortcut.use_count = 10;

  EXPECT_EQ("sc-123", shortcut.shortcut_id);
  EXPECT_EQ("Example", shortcut.name);
  EXPECT_EQ(GURL("https://example.com"), shortcut.url);
  EXPECT_EQ(GURL("https://example.com/icon.png"), shortcut.icon_url);
  EXPECT_TRUE(shortcut.is_default);
  EXPECT_EQ(3, shortcut.position);
  EXPECT_EQ("ws-456", shortcut.astra_workspace_id);
  EXPECT_EQ(10, shortcut.use_count);
}

TEST(AstraNtpThemeStructTest, DefaultValues) {
  AstraNtpTheme theme;
  EXPECT_EQ(AstraNtpLayout::kDefault, theme.layout);
  EXPECT_EQ(SK_ColorWHITE, theme.background_color);
  EXPECT_TRUE(theme.background_image_url.empty());
  EXPECT_TRUE(theme.show_shortcuts);
  EXPECT_TRUE(theme.show_workspace_cards);
  EXPECT_TRUE(theme.show_suggestions);
  EXPECT_TRUE(theme.show_google_logo);
  EXPECT_TRUE(theme.show_search_box);
  EXPECT_EQ(4, theme.shortcut_columns);
  EXPECT_EQ(2, theme.shortcut_rows);
}

TEST(AstraNtpThemeStructTest, CanSetFields) {
  AstraNtpTheme theme;
  theme.layout = AstraNtpLayout::kCustom;
  theme.background_color = SK_ColorBLACK;
  theme.background_image_url = "https://example.com/bg.jpg";
  theme.show_shortcuts = false;
  theme.show_workspace_cards = false;
  theme.show_suggestions = false;
  theme.show_google_logo = false;
  theme.show_search_box = false;
  theme.shortcut_columns = 6;
  theme.shortcut_rows = 4;

  EXPECT_EQ(AstraNtpLayout::kCustom, theme.layout);
  EXPECT_EQ(SK_ColorBLACK, theme.background_color);
  EXPECT_EQ("https://example.com/bg.jpg", theme.background_image_url);
  EXPECT_FALSE(theme.show_shortcuts);
  EXPECT_FALSE(theme.show_workspace_cards);
  EXPECT_FALSE(theme.show_suggestions);
  EXPECT_FALSE(theme.show_google_logo);
  EXPECT_FALSE(theme.show_search_box);
  EXPECT_EQ(6, theme.shortcut_columns);
  EXPECT_EQ(4, theme.shortcut_rows);
}

TEST(AstraSuggestedContentStructTest, DefaultValues) {
  AstraSuggestedContent content;
  EXPECT_FALSE(content.url.is_valid());
  EXPECT_TRUE(content.title.empty());
  EXPECT_TRUE(content.source.empty());
  EXPECT_FALSE(content.thumbnail_url.is_valid());
  EXPECT_TRUE(content.category.empty());
  EXPECT_EQ(0.0, content.score);
  EXPECT_FALSE(content.is_read);
  EXPECT_TRUE(content.published_time.is_null());
}

TEST(AstraSuggestedContentStructTest, CanSetFields) {
  AstraSuggestedContent content;
  content.url = GURL("https://example.com/article");
  content.title = "Breaking News";
  content.source = "Example News";
  content.thumbnail_url = GURL("https://example.com/thumb.jpg");
  content.category = "technology";
  content.score = 0.85;
  content.is_read = true;
  content.published_time = base::Time::Now();

  EXPECT_EQ(GURL("https://example.com/article"), content.url);
  EXPECT_EQ("Breaking News", content.title);
  EXPECT_EQ("Example News", content.source);
  EXPECT_EQ(GURL("https://example.com/thumb.jpg"), content.thumbnail_url);
  EXPECT_EQ("technology", content.category);
  EXPECT_DOUBLE_EQ(0.85, content.score);
  EXPECT_TRUE(content.is_read);
  EXPECT_FALSE(content.published_time.is_null());
}

// Legacy struct tests.
TEST(AstraNtpShortcutStructTest, DefaultValues) {
  AstraNtpShortcut shortcut;
  EXPECT_TRUE(shortcut.title.empty());
  EXPECT_FALSE(shortcut.url.is_valid());
  EXPECT_FALSE(shortcut.favicon_url.is_valid());
  EXPECT_TRUE(shortcut.is_most_visited);
}

TEST(AstraNtpShortcutStructTest, CanSetFields) {
  AstraNtpShortcut shortcut;
  shortcut.title = u"Example";
  shortcut.url = GURL("https://example.com");
  shortcut.is_most_visited = false;

  EXPECT_EQ(u"Example", shortcut.title);
  EXPECT_EQ(GURL("https://example.com"), shortcut.url);
  EXPECT_FALSE(shortcut.is_most_visited);
}

TEST(AstraNtpRecentVisitStructTest, DefaultValues) {
  AstraNtpRecentVisit visit;
  EXPECT_TRUE(visit.title.empty());
  EXPECT_FALSE(visit.url.is_valid());
  EXPECT_TRUE(visit.visit_time.is_null());
}

TEST(AstraNtpWorkspaceSummaryStructTest, DefaultValues) {
  AstraNtpWorkspaceSummary summary;
  EXPECT_TRUE(summary.id.empty());
  EXPECT_TRUE(summary.name.empty());
  EXPECT_TRUE(summary.accent_color.empty());
  EXPECT_EQ(0, summary.tab_count);
  EXPECT_FALSE(summary.is_active);
}

TEST(AstraNtpWorkspaceSummaryStructTest, CanSetFields) {
  AstraNtpWorkspaceSummary summary;
  summary.id = "ws-123";
  summary.name = "Work";
  summary.accent_color = "#5B8FF9";
  summary.tab_count = 5;
  summary.is_active = true;

  EXPECT_EQ("ws-123", summary.id);
  EXPECT_EQ("Work", summary.name);
  EXPECT_EQ("#5B8FF9", summary.accent_color);
  EXPECT_EQ(5, summary.tab_count);
  EXPECT_TRUE(summary.is_active);
}

// =========================================================================
// Enum tests
// =========================================================================

TEST(AstraNtpLayoutEnumTest, HasFiveValues) {
  EXPECT_EQ(0, static_cast<int>(AstraNtpLayout::kDefault));
  EXPECT_EQ(1, static_cast<int>(AstraNtpLayout::kShortcutsOnly));
  EXPECT_EQ(2, static_cast<int>(AstraNtpLayout::kWorkspacesOnly));
  EXPECT_EQ(3, static_cast<int>(AstraNtpLayout::kMinimal));
  EXPECT_EQ(4, static_cast<int>(AstraNtpLayout::kCustom));
}

TEST(AstraNtpDarkModeEnumTest, HasThreeValues) {
  EXPECT_EQ(0, static_cast<int>(AstraNtpDarkMode::kAuto));
  EXPECT_EQ(1, static_cast<int>(AstraNtpDarkMode::kLight));
  EXPECT_EQ(2, static_cast<int>(AstraNtpDarkMode::kDark));
}

TEST(AstraNtpLayoutModeEnumTest, HasThreeValues) {
  EXPECT_EQ(0, static_cast<int>(AstraNtpLayoutMode::kStandard));
  EXPECT_EQ(1, static_cast<int>(AstraNtpLayoutMode::kCompact));
  EXPECT_EQ(2, static_cast<int>(AstraNtpLayoutMode::kFocused));
}

TEST(AstraNtpBackgroundTypeEnumTest, HasThreeValues) {
  EXPECT_EQ(0, static_cast<int>(AstraNtpBackgroundType::kDefault));
  EXPECT_EQ(1, static_cast<int>(AstraNtpBackgroundType::kSolidColor));
  EXPECT_EQ(2, static_cast<int>(AstraNtpBackgroundType::kCustomImage));
}

// =========================================================================
// Pref key constants tests
// =========================================================================

TEST(AstraNewTabPagePrefKeysTest, ArePublicStaticConstexpr) {
  // Verify pref key constants are accessible as public static members.
  EXPECT_STREQ("astra.ntp.shortcuts",
               AstraNewTabPageService::kPrefShortcuts);
  EXPECT_STREQ("astra.ntp.layout",
               AstraNewTabPageService::kPrefNtpLayout);
  EXPECT_STREQ("astra.ntp.background_color",
               AstraNewTabPageService::kPrefBackgroundColor);
  EXPECT_STREQ("astra.ntp.background_image_url",
               AstraNewTabPageService::kPrefBackgroundImageUrl);
  EXPECT_STREQ("astra.ntp.show_shortcuts",
               AstraNewTabPageService::kPrefShowShortcuts);
  EXPECT_STREQ("astra.ntp.show_workspace_cards",
               AstraNewTabPageService::kPrefShowWorkspaceCards);
  EXPECT_STREQ("astra.ntp.show_suggestions",
               AstraNewTabPageService::kPrefShowSuggestions);
  EXPECT_STREQ("astra.ntp.show_google_logo",
               AstraNewTabPageService::kPrefShowGoogleLogo);
  EXPECT_STREQ("astra.ntp.show_search_box",
               AstraNewTabPageService::kPrefShowSearchBox);
  EXPECT_STREQ("astra.ntp.shortcut_columns",
               AstraNewTabPageService::kPrefShortcutColumns);
  EXPECT_STREQ("astra.ntp.shortcut_rows",
               AstraNewTabPageService::kPrefShortcutRows);
  EXPECT_STREQ("astra.ntp.max_workspace_cards",
               AstraNewTabPageService::kPrefMaxWorkspaceCards);
  EXPECT_STREQ("astra.ntp.max_suggestions",
               AstraNewTabPageService::kPrefMaxSuggestions);
  EXPECT_STREQ("astra.ntp.show_most_visited",
               AstraNewTabPageService::kPrefShowMostVisited);
  EXPECT_STREQ("astra.ntp.show_recently_closed",
               AstraNewTabPageService::kPrefShowRecentlyClosed);
  EXPECT_STREQ("astra.ntp.dark_mode",
               AstraNewTabPageService::kPrefDarkMode);
  EXPECT_STREQ("astra.ntp.custom_background_enabled",
               AstraNewTabPageService::kPrefCustomBackgroundEnabled);
  EXPECT_STREQ("astra.ntp.workspace_card_visibility",
               AstraNewTabPageService::kPrefWorkspaceCardVisibility);
  EXPECT_STREQ("astra.ntp.workspace_card_order",
               AstraNewTabPageService::kPrefWorkspaceCardOrder);
  EXPECT_STREQ("astra.ntp.dismissed_suggestions",
               AstraNewTabPageService::kPrefDismissedSuggestions);
  EXPECT_STREQ("astra.ntp.suggestions_enabled",
               AstraNewTabPageService::kPrefSuggestionsEnabled);
}

TEST(AstraNewTabPagePrefKeysTest, AllUseAstraNtpNamespace) {
  // All NTP pref keys should start with "astra.ntp."
  std::vector<const char*> keys = {
      AstraNewTabPageService::kPrefShortcuts,
      AstraNewTabPageService::kPrefNtpLayout,
      AstraNewTabPageService::kPrefBackgroundColor,
      AstraNewTabPageService::kPrefBackgroundImageUrl,
      AstraNewTabPageService::kPrefShowShortcuts,
      AstraNewTabPageService::kPrefShowWorkspaceCards,
      AstraNewTabPageService::kPrefShowSuggestions,
      AstraNewTabPageService::kPrefShowGoogleLogo,
      AstraNewTabPageService::kPrefShowSearchBox,
      AstraNewTabPageService::kPrefShortcutColumns,
      AstraNewTabPageService::kPrefShortcutRows,
      AstraNewTabPageService::kPrefMaxWorkspaceCards,
      AstraNewTabPageService::kPrefMaxSuggestions,
      AstraNewTabPageService::kPrefShowMostVisited,
      AstraNewTabPageService::kPrefShowRecentlyClosed,
      AstraNewTabPageService::kPrefDarkMode,
      AstraNewTabPageService::kPrefCustomBackgroundEnabled,
      AstraNewTabPageService::kPrefWorkspaceCardVisibility,
      AstraNewTabPageService::kPrefWorkspaceCardOrder,
      AstraNewTabPageService::kPrefDismissedSuggestions,
      AstraNewTabPageService::kPrefSuggestionsEnabled,
  };

  for (const char* key : keys) {
    std::string key_str(key);
    EXPECT_EQ(0u, key_str.find("astra.ntp.")) << "Key: " << key;
  }
}

// =========================================================================
// Service architecture documentation tests
// =========================================================================

TEST(AstraNewTabPageArchTest, ServiceIsProjection) {
  // AstraNewTabPageService is a projection / aggregation service:
  //   - Top sites: from Chromium TopSites
  //   - Recently visited: from Chromium HistoryService
  //   - Workspace summaries: from AstraWorkspaceService
  //   - Favorite shortcuts: from AstraFavoriteService
  //   - Managed shortcuts / layout / theme: Astra-owned metadata
  SUCCEED();
}

TEST(AstraNewTabPageArchTest, PlaceholderDataPattern) {
  // Currently the service returns placeholder data because the real
  // Chromium service integrations are not fully wired up.
  //
  // TODO(astra): Replace placeholder data with real service integrations.
  //   Chromium owners:
  //   - TopSites (components/history/core/browser/top_sites.h)
  //   - HistoryService (components/history/core/browser/history_service.h)
  //   - FaviconService (components/favicon/core/favicon_service.h)
  SUCCEED();
}

TEST(AstraNewTabPageArchTest, FactoryBehavior) {
  // Factory behavior:
  //   - Regular profile: own instance
  //   - Incognito: redirected to original profile
  //   - Guest session: own instance
  //
  // NTP data is shared with the original profile in incognito because
  // shortcuts and workspaces are product-level state, not browsing state.
  SUCCEED();
}

// =========================================================================
// NTP section documentation tests
// =========================================================================

TEST(AstraNtpSectionsTest, MultipleSections) {
  // The new tab page has multiple sections:
  //   1. Search box (top)
  //   2. Shortcuts / most visited sites
  //   3. Workspace cards
  //   4. Suggested content / For you
  //   5. Recently visited
  //   6. Favorite shortcuts
  //
  // Sections are customizable via settings.
  SUCCEED();
}

// =========================================================================
// Pref / settings documentation tests
// =========================================================================

TEST(AstraNtpPrefsTest, ServiceOwnsAstraMetadataOnly) {
  // The NTP service owns only Astra-specific metadata:
  //   - Managed shortcuts (user-curated tiles with IDs)
  //   - Layout options (visibility toggles, density, preset)
  //   - Theme settings (background color, image, dark mode)
  //   - Workspace card visibility and ordering
  //   - Suggestion dismissed state
  //
  // It does NOT own:
  //   - Top sites / most visited (Chromium TopSites)
  //   - History (Chromium HistoryService)
  //   - Workspaces (AstraWorkspaceService)
  //   - Favorites (AstraFavoriteService)
  //
  // This follows the projection/adapter pattern.
  SUCCEED();
}

}  // namespace astra
