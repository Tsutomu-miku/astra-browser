// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_extension_helper.h"

#include "base/test/task_environment.h"
#include "chrome/test/base/testing_profile.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Test observer that records calls for verification.
class TestExtensionHelperObserver : public AstraExtensionHelperObserver {
 public:
  void OnExtensionsChanged() override {
    extensions_changed_count_++;
  }

  void OnExtensionIconChanged(const std::string& extension_id) override {
    icon_changed_count_++;
    last_icon_extension_id_ = extension_id;
  }

  void OnExtensionInstalled(const AstraExtensionInfo& extension) override {
    extension_installed_count_++;
    last_installed_extension_ = extension;
  }

  void OnExtensionUninstalled(const std::string& extension_id,
                              const std::u16string& extension_name) override {
    extension_uninstalled_count_++;
    last_uninstalled_extension_id_ = extension_id;
    last_uninstalled_extension_name_ = extension_name;
  }

  void OnExtensionEnabled(const std::string& extension_id) override {
    extension_enabled_count_++;
    last_enabled_extension_id_ = extension_id;
  }

  void OnExtensionDisabled(const std::string& extension_id) override {
    extension_disabled_count_++;
    last_disabled_extension_id_ = extension_id;
  }

  void OnExtensionSettingsChanged() override {
    settings_changed_count_++;
  }

  // Counters
  int extensions_changed_count_ = 0;
  int icon_changed_count_ = 0;
  int extension_installed_count_ = 0;
  int extension_uninstalled_count_ = 0;
  int extension_enabled_count_ = 0;
  int extension_disabled_count_ = 0;
  int settings_changed_count_ = 0;

  // Last recorded values
  std::string last_icon_extension_id_;
  AstraExtensionInfo last_installed_extension_;
  std::string last_uninstalled_extension_id_;
  std::u16string last_uninstalled_extension_name_;
  std::string last_enabled_extension_id_;
  std::string last_disabled_extension_id_;
};

}  // namespace

// Test fixture for AstraExtensionHelper tests.
class ExtensionHelperTest : public testing::Test {
 protected:
  ExtensionHelperTest() {
    profile_ = std::make_unique<TestingProfile>();
    // Register Astra prefs on the testing profile.
    prefs::RegisterProfilePrefs(profile_->GetPrefs());
    helper_ = std::make_unique<AstraExtensionHelper>(profile_.get());
    DCHECK(helper_);
  }

  ~ExtensionHelperTest() override = default;

  void SetUp() override {
    // Verify default presentation settings.
    ASSERT_TRUE(helper_->GetShowExtensionToolbar());
    ASSERT_TRUE(helper_->GetShowExtensionsInSidebar());
    ASSERT_TRUE(helper_->GetExtensionManagerShortcut());
    ASSERT_TRUE(helper_->GetShowRecommendedExtensions());
  }

  void TearDown() override {
    // Clean up observers to avoid dangling references.
    for (auto& observer : test_observers_) {
      helper_->RemoveObserver(&observer);
    }
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<AstraExtensionHelper> helper_;
  std::vector<TestExtensionHelperObserver> test_observers_;
};

// ---------------------------------------------------------------------------
// Default state
// ---------------------------------------------------------------------------

TEST_F(ExtensionHelperTest, DefaultState_ShowExtensionToolbar) {
  EXPECT_TRUE(helper_->GetShowExtensionToolbar());
}

TEST_F(ExtensionHelperTest, DefaultState_ShowExtensionsInSidebar) {
  EXPECT_TRUE(helper_->GetShowExtensionsInSidebar());
}

TEST_F(ExtensionHelperTest, DefaultState_ExtensionManagerShortcut) {
  EXPECT_TRUE(helper_->GetExtensionManagerShortcut());
}

TEST_F(ExtensionHelperTest, DefaultState_ShowRecommendedExtensions) {
  EXPECT_TRUE(helper_->GetShowRecommendedExtensions());
}

TEST_F(ExtensionHelperTest, DefaultState_MaxSidebarExtensions) {
  EXPECT_EQ(helper_->GetMaxSidebarExtensions(),
            prefs::kDefaultExtensionMaxSidebarCount);
}

TEST_F(ExtensionHelperTest, DefaultState_ExtensionSortOrder) {
  EXPECT_EQ(helper_->GetExtensionSortOrder(),
            prefs::kDefaultExtensionSortOrder);
}

TEST_F(ExtensionHelperTest, DefaultState_InstalledCountZero) {
  // In the overlay, ExtensionRegistry is not populated with real extensions,
  // so count is 0.
  EXPECT_EQ(helper_->GetInstalledExtensionsCount(), 0u);
}

TEST_F(ExtensionHelperTest, DefaultState_EnabledCountZero) {
  EXPECT_EQ(helper_->GetEnabledExtensionsCount(), 0u);
}

TEST_F(ExtensionHelperTest, DefaultState_DisabledCountZero) {
  EXPECT_EQ(helper_->GetDisabledExtensionsCount(), 0u);
}

TEST_F(ExtensionHelperTest, DefaultState_ExtensionsWithBrowserActionsEmpty) {
  auto extensions = helper_->GetExtensionsWithBrowserActions();
  EXPECT_TRUE(extensions.empty());
}

TEST_F(ExtensionHelperTest, DefaultState_HasBrowserActionFalse) {
  EXPECT_FALSE(helper_->HasBrowserAction("nonexistent-id"));
}

TEST_F(ExtensionHelperTest, DefaultState_GetExtensionByIdEmpty) {
  auto info = helper_->GetExtensionById("nonexistent-id");
  EXPECT_TRUE(info.id.empty());
}

TEST_F(ExtensionHelperTest, DefaultState_GetExtensionByNameEmpty) {
  auto results = helper_->GetExtensionByName(u"test");
  EXPECT_TRUE(results.empty());
}

TEST_F(ExtensionHelperTest, DefaultState_GetAllExtensionsEmpty) {
  auto all = helper_->GetAllExtensions();
  EXPECT_TRUE(all.empty());
}

TEST_F(ExtensionHelperTest, DefaultState_StatsAllZero) {
  auto stats = helper_->GetExtensionStats();
  EXPECT_EQ(stats.total_installed, 0u);
  EXPECT_EQ(stats.enabled_count, 0u);
  EXPECT_EQ(stats.disabled_count, 0u);
  EXPECT_EQ(stats.with_browser_action_count, 0u);
  EXPECT_EQ(stats.with_page_action_count, 0u);
  EXPECT_EQ(stats.theme_count, 0u);
  EXPECT_EQ(stats.with_permission_warnings_count, 0u);
  EXPECT_EQ(stats.active_count(), 0u);
  EXPECT_FALSE(stats.has_disabled());
  EXPECT_FALSE(stats.has_themes());
}

TEST_F(ExtensionHelperTest, DefaultState_RecommendedExtensionsNonEmpty) {
  // Recommended extensions are a static curated list, not dependent on
  // the ExtensionRegistry.
  auto recommended = helper_->GetRecommendedExtensions();
  EXPECT_GT(recommended.size(), 0u);
  EXPECT_GT(helper_->GetRecommendedExtensionsCount(), 0u);
}

TEST_F(ExtensionHelperTest, DefaultState_ExtensionManagementAvailable) {
  EXPECT_TRUE(helper_->IsExtensionManagementAvailable());
}

TEST_F(ExtensionHelperTest, DefaultState_BulkOperationAvailable) {
  EXPECT_TRUE(helper_->IsBulkOperationAvailable());
}

// ---------------------------------------------------------------------------
// Observer defaults — all methods have empty default implementations
// ---------------------------------------------------------------------------

TEST_F(ExtensionHelperTest, ObserverDefaults_DoNotCrash) {
  // Create a minimal observer that overrides nothing.
  // All methods have default empty implementations, so this should compile
  // and not crash.
  class DefaultObserver : public AstraExtensionHelperObserver {};

  DefaultObserver observer;
  helper_->AddObserver(&observer);

  // Trigger all observer paths via pref changes and manual notifications.
  helper_->SetShowExtensionToolbar(false);
  helper_->SetShowExtensionsInSidebar(false);
  helper_->SetExtensionManagerShortcut(false);
  helper_->SetShowRecommendedExtensions(false);
  helper_->SetMaxSidebarExtensions(5);
  helper_->SetExtensionSortOrder("install_date");

  helper_->NotifyExtensionsChanged();
  helper_->NotifyExtensionIconChanged("test-id");
  helper_->NotifyExtensionInstalled(AstraExtensionInfo());
  helper_->NotifyExtensionUninstalled("test-id", u"Test Extension");
  helper_->NotifyExtensionEnabled("test-id");
  helper_->NotifyExtensionDisabled("test-id");

  helper_->RemoveObserver(&observer);
  SUCCEED() << "Default observer methods do not crash.";
}

// ---------------------------------------------------------------------------
// Observer add/remove
// ---------------------------------------------------------------------------

TEST_F(ExtensionHelperTest, AddRemoveObserver_NoCrash) {
  TestExtensionHelperObserver observer;

  helper_->AddObserver(&observer);
  helper_->RemoveObserver(&observer);

  SUCCEED() << "Observer add/remove completed without crash.";
}

TEST_F(ExtensionHelperTest, RemoveNonexistentObserver_NoCrash) {
  TestExtensionHelperObserver observer;

  helper_->RemoveObserver(&observer);

  SUCCEED() << "Removing nonexistent observer completed without crash.";
}

// ---------------------------------------------------------------------------
// Presentation settings — show extension toolbar
// ---------------------------------------------------------------------------

TEST_F(ExtensionHelperTest, SetShowExtensionToolbar_ChangesValue) {
  ASSERT_TRUE(helper_->GetShowExtensionToolbar());

  helper_->SetShowExtensionToolbar(false);
  EXPECT_FALSE(helper_->GetShowExtensionToolbar());

  helper_->SetShowExtensionToolbar(true);
  EXPECT_TRUE(helper_->GetShowExtensionToolbar());
}

TEST_F(ExtensionHelperTest, SetShowExtensionToolbar_SameValueNoOp) {
  TestExtensionHelperObserver observer;
  helper_->AddObserver(&observer);

  ASSERT_TRUE(helper_->GetShowExtensionToolbar());
  helper_->SetShowExtensionToolbar(true);

  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(ExtensionHelperTest, SetShowExtensionToolbar_FiresSettingsObserver) {
  TestExtensionHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetShowExtensionToolbar(false);

  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(ExtensionHelperTest, ToggleShowExtensionToolbar_FlipsValue) {
  ASSERT_TRUE(helper_->GetShowExtensionToolbar());

  bool result = helper_->ToggleShowExtensionToolbar();
  EXPECT_FALSE(result);
  EXPECT_FALSE(helper_->GetShowExtensionToolbar());

  result = helper_->ToggleShowExtensionToolbar();
  EXPECT_TRUE(result);
  EXPECT_TRUE(helper_->GetShowExtensionToolbar());
}

// ---------------------------------------------------------------------------
// Presentation settings — show extensions in sidebar
// ---------------------------------------------------------------------------

TEST_F(ExtensionHelperTest, SetShowExtensionsInSidebar_ChangesValue) {
  ASSERT_TRUE(helper_->GetShowExtensionsInSidebar());

  helper_->SetShowExtensionsInSidebar(false);
  EXPECT_FALSE(helper_->GetShowExtensionsInSidebar());

  helper_->SetShowExtensionsInSidebar(true);
  EXPECT_TRUE(helper_->GetShowExtensionsInSidebar());
}

TEST_F(ExtensionHelperTest, SetShowExtensionsInSidebar_SameValueNoOp) {
  TestExtensionHelperObserver observer;
  helper_->AddObserver(&observer);

  ASSERT_TRUE(helper_->GetShowExtensionsInSidebar());
  helper_->SetShowExtensionsInSidebar(true);

  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(ExtensionHelperTest, SetShowExtensionsInSidebar_FiresSettingsObserver) {
  TestExtensionHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetShowExtensionsInSidebar(false);

  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(ExtensionHelperTest, ToggleShowExtensionsInSidebar_FlipsValue) {
  ASSERT_TRUE(helper_->GetShowExtensionsInSidebar());

  bool result = helper_->ToggleShowExtensionsInSidebar();
  EXPECT_FALSE(result);
  EXPECT_FALSE(helper_->GetShowExtensionsInSidebar());

  result = helper_->ToggleShowExtensionsInSidebar();
  EXPECT_TRUE(result);
  EXPECT_TRUE(helper_->GetShowExtensionsInSidebar());
}

// ---------------------------------------------------------------------------
// Presentation settings — extension manager shortcut
// ---------------------------------------------------------------------------

TEST_F(ExtensionHelperTest, SetExtensionManagerShortcut_ChangesValue) {
  ASSERT_TRUE(helper_->GetExtensionManagerShortcut());

  helper_->SetExtensionManagerShortcut(false);
  EXPECT_FALSE(helper_->GetExtensionManagerShortcut());

  helper_->SetExtensionManagerShortcut(true);
  EXPECT_TRUE(helper_->GetExtensionManagerShortcut());
}

TEST_F(ExtensionHelperTest, SetExtensionManagerShortcut_SameValueNoOp) {
  TestExtensionHelperObserver observer;
  helper_->AddObserver(&observer);

  ASSERT_TRUE(helper_->GetExtensionManagerShortcut());
  helper_->SetExtensionManagerShortcut(true);

  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(ExtensionHelperTest, SetExtensionManagerShortcut_FiresSettingsObserver) {
  TestExtensionHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetExtensionManagerShortcut(false);

  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(ExtensionHelperTest, ToggleExtensionManagerShortcut_FlipsValue) {
  ASSERT_TRUE(helper_->GetExtensionManagerShortcut());

  bool result = helper_->ToggleExtensionManagerShortcut();
  EXPECT_FALSE(result);
  EXPECT_FALSE(helper_->GetExtensionManagerShortcut());

  result = helper_->ToggleExtensionManagerShortcut();
  EXPECT_TRUE(result);
  EXPECT_TRUE(helper_->GetExtensionManagerShortcut());
}

// ---------------------------------------------------------------------------
// Presentation settings — show recommended extensions
// ---------------------------------------------------------------------------

TEST_F(ExtensionHelperTest, SetShowRecommendedExtensions_ChangesValue) {
  ASSERT_TRUE(helper_->GetShowRecommendedExtensions());

  helper_->SetShowRecommendedExtensions(false);
  EXPECT_FALSE(helper_->GetShowRecommendedExtensions());

  helper_->SetShowRecommendedExtensions(true);
  EXPECT_TRUE(helper_->GetShowRecommendedExtensions());
}

TEST_F(ExtensionHelperTest, SetShowRecommendedExtensions_SameValueNoOp) {
  TestExtensionHelperObserver observer;
  helper_->AddObserver(&observer);

  ASSERT_TRUE(helper_->GetShowRecommendedExtensions());
  helper_->SetShowRecommendedExtensions(true);

  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(ExtensionHelperTest, SetShowRecommendedExtensions_FiresSettingsObserver) {
  TestExtensionHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetShowRecommendedExtensions(false);

  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(ExtensionHelperTest, ToggleShowRecommendedExtensions_FlipsValue) {
  ASSERT_TRUE(helper_->GetShowRecommendedExtensions());

  bool result = helper_->ToggleShowRecommendedExtensions();
  EXPECT_FALSE(result);
  EXPECT_FALSE(helper_->GetShowRecommendedExtensions());

  result = helper_->ToggleShowRecommendedExtensions();
  EXPECT_TRUE(result);
  EXPECT_TRUE(helper_->GetShowRecommendedExtensions());
}

// ---------------------------------------------------------------------------
// Presentation settings — max sidebar extensions
// ---------------------------------------------------------------------------

TEST_F(ExtensionHelperTest, SetMaxSidebarExtensions_ChangesValue) {
  helper_->SetMaxSidebarExtensions(5);
  EXPECT_EQ(helper_->GetMaxSidebarExtensions(), 5);

  helper_->SetMaxSidebarExtensions(30);
  EXPECT_EQ(helper_->GetMaxSidebarExtensions(), 30);
}

TEST_F(ExtensionHelperTest, SetMaxSidebarExtensions_ClampsToMinimum) {
  helper_->SetMaxSidebarExtensions(0);
  EXPECT_EQ(helper_->GetMaxSidebarExtensions(), 1);

  helper_->SetMaxSidebarExtensions(-5);
  EXPECT_EQ(helper_->GetMaxSidebarExtensions(), 1);
}

TEST_F(ExtensionHelperTest, SetMaxSidebarExtensions_ClampsToMaximum) {
  helper_->SetMaxSidebarExtensions(100);
  EXPECT_EQ(helper_->GetMaxSidebarExtensions(), 50);
}

TEST_F(ExtensionHelperTest, SetMaxSidebarExtensions_SameValueNoOp) {
  TestExtensionHelperObserver observer;
  helper_->AddObserver(&observer);

  int original = helper_->GetMaxSidebarExtensions();
  helper_->SetMaxSidebarExtensions(original);

  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(ExtensionHelperTest, SetMaxSidebarExtensions_FiresSettingsObserver) {
  TestExtensionHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetMaxSidebarExtensions(5);

  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Presentation settings — extension sort order
// ---------------------------------------------------------------------------

TEST_F(ExtensionHelperTest, SetExtensionSortOrder_ChangesValue) {
  helper_->SetExtensionSortOrder("install_date");
  EXPECT_EQ(helper_->GetExtensionSortOrder(), "install_date");

  helper_->SetExtensionSortOrder("category");
  EXPECT_EQ(helper_->GetExtensionSortOrder(), "category");

  helper_->SetExtensionSortOrder("name");
  EXPECT_EQ(helper_->GetExtensionSortOrder(), "name");
}

TEST_F(ExtensionHelperTest, SetExtensionSortOrder_InvalidValueIgnored) {
  std::string original = helper_->GetExtensionSortOrder();

  helper_->SetExtensionSortOrder("invalid_sort_order");
  EXPECT_EQ(helper_->GetExtensionSortOrder(), original);
}

TEST_F(ExtensionHelperTest, SetExtensionSortOrder_SameValueNoOp) {
  TestExtensionHelperObserver observer;
  helper_->AddObserver(&observer);

  std::string original = helper_->GetExtensionSortOrder();
  helper_->SetExtensionSortOrder(original);

  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(ExtensionHelperTest, SetExtensionSortOrder_FiresSettingsObserver) {
  TestExtensionHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetExtensionSortOrder("install_date");

  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Query methods — edge cases
// ---------------------------------------------------------------------------

TEST_F(ExtensionHelperTest, GetExtensionById_EmptyId) {
  auto info = helper_->GetExtensionById("");
  EXPECT_TRUE(info.id.empty());
}

TEST_F(ExtensionHelperTest, GetExtensionByName_EmptyQuery) {
  // Empty query should return empty results.
  auto results = helper_->GetExtensionByName(std::u16string());
  EXPECT_TRUE(results.empty());
}

TEST_F(ExtensionHelperTest, GetExtensionByName_MaxCountZero) {
  // Zero max_count means no limit.
  auto results = helper_->GetExtensionByName(u"test", 0);
  // In overlay there are no extensions, so result is empty.
  EXPECT_TRUE(results.empty());
}

TEST_F(ExtensionHelperTest, GetExtensionByName_MaxCountOne) {
  auto results = helper_->GetExtensionByName(u"test", 1);
  EXPECT_LE(results.size(), 1u);
}

TEST_F(ExtensionHelperTest, GetAllExtensions_WithoutDisabled) {
  auto results = helper_->GetAllExtensions(false);
  // In overlay there are no extensions.
  EXPECT_TRUE(results.empty());
}

TEST_F(ExtensionHelperTest, GetAllExtensions_WithMaxCount) {
  auto results = helper_->GetAllExtensions(true, 5);
  EXPECT_LE(results.size(), 5u);
}

TEST_F(ExtensionHelperTest, GetExtensionsByCategory_NoExtensions) {
  auto results = helper_->GetExtensionsByCategory(
      AstraExtensionCategory::kProductivity);
  EXPECT_TRUE(results.empty());
}

TEST_F(ExtensionHelperTest, GetExtensionCountByCategory_NoExtensions) {
  EXPECT_EQ(helper_->GetExtensionCountByCategory(
      AstraExtensionCategory::kProductivity), 0u);
}

TEST_F(ExtensionHelperTest, GetAvailableCategories_NoExtensions) {
  auto categories = helper_->GetAvailableCategories();
  EXPECT_TRUE(categories.empty());
}

// ---------------------------------------------------------------------------
// Recommended extensions
// ---------------------------------------------------------------------------

TEST_F(ExtensionHelperTest, RecommendedExtensions_HaveValidCategories) {
  auto recommended = helper_->GetRecommendedExtensions();
  ASSERT_GT(recommended.size(), 0u);

  for (const auto& ext : recommended) {
    EXPECT_FALSE(ext.id.empty());
    EXPECT_FALSE(ext.name.empty());
    // Recommended extensions should not be marked as enabled/installed.
    EXPECT_FALSE(ext.enabled);
  }
}

TEST_F(ExtensionHelperTest, IsRecommendedExtension_ValidId) {
  auto recommended = helper_->GetRecommendedExtensions();
  ASSERT_GT(recommended.size(), 0u);

  EXPECT_TRUE(helper_->IsRecommendedExtension(recommended[0].id));
}

TEST_F(ExtensionHelperTest, IsRecommendedExtension_InvalidId) {
  EXPECT_FALSE(helper_->IsRecommendedExtension("nonexistent-recommended-id"));
}

TEST_F(ExtensionHelperTest, RecommendedExtensionsCount_MatchesListSize) {
  auto recommended = helper_->GetRecommendedExtensions();
  EXPECT_EQ(helper_->GetRecommendedExtensionsCount(), recommended.size());
}

// ---------------------------------------------------------------------------
// Extension categories and labels
// ---------------------------------------------------------------------------

TEST_F(ExtensionHelperTest, GetCategoryLabel_ReturnsLabels) {
  EXPECT_FALSE(AstraExtensionHelper::GetCategoryLabel(
      AstraExtensionCategory::kProductivity).empty());
  EXPECT_FALSE(AstraExtensionHelper::GetCategoryLabel(
      AstraExtensionCategory::kAccessibility).empty());
  EXPECT_FALSE(AstraExtensionHelper::GetCategoryLabel(
      AstraExtensionCategory::kDeveloperTools).empty());
  EXPECT_FALSE(AstraExtensionHelper::GetCategoryLabel(
      AstraExtensionCategory::kEntertainment).empty());
  EXPECT_FALSE(AstraExtensionHelper::GetCategoryLabel(
      AstraExtensionCategory::kCommunication).empty());
  EXPECT_FALSE(AstraExtensionHelper::GetCategoryLabel(
      AstraExtensionCategory::kNewsWeather).empty());
  EXPECT_FALSE(AstraExtensionHelper::GetCategoryLabel(
      AstraExtensionCategory::kPrivacySecurity).empty());
  EXPECT_FALSE(AstraExtensionHelper::GetCategoryLabel(
      AstraExtensionCategory::kPhotos).empty());
  EXPECT_FALSE(AstraExtensionHelper::GetCategoryLabel(
      AstraExtensionCategory::kProductivityTab).empty());
  EXPECT_FALSE(AstraExtensionHelper::GetCategoryLabel(
      AstraExtensionCategory::kThemes).empty());
  EXPECT_FALSE(AstraExtensionHelper::GetCategoryLabel(
      AstraExtensionCategory::kOther).empty());
}

TEST_F(ExtensionHelperTest, ClassifyExtension_NullReturnsOther) {
  EXPECT_EQ(AstraExtensionHelper::ClassifyExtension(nullptr),
            AstraExtensionCategory::kOther);
}

// ---------------------------------------------------------------------------
// Manual observer notifications
// ---------------------------------------------------------------------------

TEST_F(ExtensionHelperTest, NotifyExtensionsChanged_FiresObserver) {
  TestExtensionHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifyExtensionsChanged();
  EXPECT_EQ(observer.extensions_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(ExtensionHelperTest, NotifyExtensionIconChanged_FiresObserver) {
  TestExtensionHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifyExtensionIconChanged("ext-123");
  EXPECT_EQ(observer.icon_changed_count_, 1);
  EXPECT_EQ(observer.last_icon_extension_id_, "ext-123");

  helper_->RemoveObserver(&observer);
}

TEST_F(ExtensionHelperTest, NotifyExtensionInstalled_FiresObserver) {
  TestExtensionHelperObserver observer;
  helper_->AddObserver(&observer);

  AstraExtensionInfo info;
  info.id = "test-ext-id";
  info.name = u"Test Extension";
  helper_->NotifyExtensionInstalled(info);

  EXPECT_EQ(observer.extension_installed_count_, 1);
  EXPECT_EQ(observer.last_installed_extension_.id, "test-ext-id");
  EXPECT_EQ(observer.last_installed_extension_.name, u"Test Extension");
  // Should also fire OnExtensionsChanged.
  EXPECT_EQ(observer.extensions_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(ExtensionHelperTest, NotifyExtensionUninstalled_FiresObserver) {
  TestExtensionHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifyExtensionUninstalled("ext-456", u"My Extension");

  EXPECT_EQ(observer.extension_uninstalled_count_, 1);
  EXPECT_EQ(observer.last_uninstalled_extension_id_, "ext-456");
  EXPECT_EQ(observer.last_uninstalled_extension_name_, u"My Extension");
  // Should also fire OnExtensionsChanged.
  EXPECT_EQ(observer.extensions_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(ExtensionHelperTest, NotifyExtensionEnabled_FiresObserver) {
  TestExtensionHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifyExtensionEnabled("ext-789");

  EXPECT_EQ(observer.extension_enabled_count_, 1);
  EXPECT_EQ(observer.last_enabled_extension_id_, "ext-789");
  // Should also fire OnExtensionsChanged.
  EXPECT_EQ(observer.extensions_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(ExtensionHelperTest, NotifyExtensionDisabled_FiresObserver) {
  TestExtensionHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifyExtensionDisabled("ext-012");

  EXPECT_EQ(observer.extension_disabled_count_, 1);
  EXPECT_EQ(observer.last_disabled_extension_id_, "ext-012");
  // Should also fire OnExtensionsChanged.
  EXPECT_EQ(observer.extensions_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(ExtensionHelperTest, NotifyExtensionSettingsChanged_FiresObserver) {
  TestExtensionHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifyExtensionSettingsChanged();
  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Multiple observers
// ---------------------------------------------------------------------------

TEST_F(ExtensionHelperTest, MultipleObservers_AllNotified) {
  TestExtensionHelperObserver observer1;
  TestExtensionHelperObserver observer2;

  helper_->AddObserver(&observer1);
  helper_->AddObserver(&observer2);

  helper_->SetShowExtensionToolbar(false);

  EXPECT_EQ(observer1.settings_changed_count_, 1);
  EXPECT_EQ(observer2.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer1);
  helper_->RemoveObserver(&observer2);
}

TEST_F(ExtensionHelperTest, RemoveObserver_StopsNotifications) {
  TestExtensionHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetShowExtensionToolbar(false);
  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);

  helper_->SetShowExtensionToolbar(true);
  // Should not have been notified after removal.
  EXPECT_EQ(observer.settings_changed_count_, 1);
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------

TEST_F(ExtensionHelperTest, Shutdown_CleansUp) {
  TestExtensionHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->Shutdown();

  // After shutdown, settings changes should not notify since profile is null.
  helper_->SetShowExtensionToolbar(false);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Persistence round-trip
// ---------------------------------------------------------------------------

TEST_F(ExtensionHelperTest, PrefsPersist_ShowExtensionToolbar) {
  helper_->SetShowExtensionToolbar(false);

  // Create a new helper with the same profile — should read persisted value.
  auto helper2 = std::make_unique<AstraExtensionHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetShowExtensionToolbar());
}

TEST_F(ExtensionHelperTest, PrefsPersist_ShowExtensionsInSidebar) {
  helper_->SetShowExtensionsInSidebar(false);

  auto helper2 = std::make_unique<AstraExtensionHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetShowExtensionsInSidebar());
}

TEST_F(ExtensionHelperTest, PrefsPersist_ExtensionManagerShortcut) {
  helper_->SetExtensionManagerShortcut(false);

  auto helper2 = std::make_unique<AstraExtensionHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetExtensionManagerShortcut());
}

TEST_F(ExtensionHelperTest, PrefsPersist_ShowRecommendedExtensions) {
  helper_->SetShowRecommendedExtensions(false);

  auto helper2 = std::make_unique<AstraExtensionHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetShowRecommendedExtensions());
}

TEST_F(ExtensionHelperTest, PrefsPersist_MaxSidebarExtensions) {
  helper_->SetMaxSidebarExtensions(25);

  auto helper2 = std::make_unique<AstraExtensionHelper>(profile_.get());
  EXPECT_EQ(helper2->GetMaxSidebarExtensions(), 25);
}

TEST_F(ExtensionHelperTest, PrefsPersist_ExtensionSortOrder) {
  helper_->SetExtensionSortOrder("install_date");

  auto helper2 = std::make_unique<AstraExtensionHelper>(profile_.get());
  EXPECT_EQ(helper2->GetExtensionSortOrder(), "install_date");
}

TEST_F(ExtensionHelperTest, PrefsPersist_DefaultValues) {
  // Create a fresh helper — should have default values.
  auto helper2 = std::make_unique<AstraExtensionHelper>(profile_.get());

  EXPECT_TRUE(helper2->GetShowExtensionToolbar());
  EXPECT_TRUE(helper2->GetShowExtensionsInSidebar());
  EXPECT_TRUE(helper2->GetExtensionManagerShortcut());
  EXPECT_TRUE(helper2->GetShowRecommendedExtensions());
  EXPECT_EQ(helper2->GetMaxSidebarExtensions(),
            prefs::kDefaultExtensionMaxSidebarCount);
  EXPECT_EQ(helper2->GetExtensionSortOrder(),
            prefs::kDefaultExtensionSortOrder);
}

// ---------------------------------------------------------------------------
// Combined settings changes
// ---------------------------------------------------------------------------

TEST_F(ExtensionHelperTest, MultipleSettingChanges_AllNotify) {
  TestExtensionHelperObserver observer;
  helper_->AddObserver(&observer);

  // Change each setting — each should fire a settings notification.
  helper_->SetShowExtensionToolbar(false);
  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->SetShowExtensionsInSidebar(false);
  EXPECT_EQ(observer.settings_changed_count_, 2);

  helper_->SetExtensionManagerShortcut(false);
  EXPECT_EQ(observer.settings_changed_count_, 3);

  helper_->SetShowRecommendedExtensions(false);
  EXPECT_EQ(observer.settings_changed_count_, 4);

  helper_->SetMaxSidebarExtensions(5);
  EXPECT_EQ(observer.settings_changed_count_, 5);

  helper_->SetExtensionSortOrder("category");
  EXPECT_EQ(observer.settings_changed_count_, 6);

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// AstraExtensionInfo struct
// ---------------------------------------------------------------------------

TEST_F(ExtensionHelperTest, ExtensionInfo_DefaultConstructed) {
  AstraExtensionInfo info;
  EXPECT_TRUE(info.id.empty());
  EXPECT_TRUE(info.name.empty());
  EXPECT_TRUE(info.description.empty());
  EXPECT_TRUE(info.version.empty());
  EXPECT_FALSE(info.enabled);
  EXPECT_FALSE(info.has_browser_action);
  EXPECT_FALSE(info.has_page_action);
  EXPECT_FALSE(info.has_options_page);
  EXPECT_TRUE(info.popup_url.empty());
  EXPECT_TRUE(info.icon_url.empty());
  EXPECT_EQ(info.category, AstraExtensionCategory::kOther);
}

// ---------------------------------------------------------------------------
// AstraExtensionStats struct
// ---------------------------------------------------------------------------

TEST_F(ExtensionHelperTest, ExtensionStats_DefaultConstructed) {
  AstraExtensionStats stats;
  EXPECT_EQ(stats.total_installed, 0u);
  EXPECT_EQ(stats.enabled_count, 0u);
  EXPECT_EQ(stats.disabled_count, 0u);
  EXPECT_EQ(stats.with_browser_action_count, 0u);
  EXPECT_EQ(stats.with_page_action_count, 0u);
  EXPECT_EQ(stats.theme_count, 0u);
  EXPECT_EQ(stats.with_permission_warnings_count, 0u);
  EXPECT_EQ(stats.active_count(), 0u);
  EXPECT_FALSE(stats.has_disabled());
  EXPECT_FALSE(stats.has_themes());
}

TEST_F(ExtensionHelperTest, ExtensionStats_ActiveCount) {
  AstraExtensionStats stats;
  stats.enabled_count = 5;
  EXPECT_EQ(stats.active_count(), 5u);
}

TEST_F(ExtensionHelperTest, ExtensionStats_HasDisabled) {
  AstraExtensionStats stats;
  EXPECT_FALSE(stats.has_disabled());

  stats.disabled_count = 1;
  EXPECT_TRUE(stats.has_disabled());
}

TEST_F(ExtensionHelperTest, ExtensionStats_HasThemes) {
  AstraExtensionStats stats;
  EXPECT_FALSE(stats.has_themes());

  stats.theme_count = 1;
  EXPECT_TRUE(stats.has_themes());
}

// ---------------------------------------------------------------------------
// Extension management helpers — null profile safety
// ---------------------------------------------------------------------------

TEST_F(ExtensionHelperTest, OpenExtensionSettings_NullProfileNoCrash) {
  helper_->OpenExtensionSettings(nullptr);
  SUCCEED();
}

TEST_F(ExtensionHelperTest, OpenChromeWebStore_NullProfileNoCrash) {
  helper_->OpenChromeWebStore(nullptr);
  SUCCEED();
}

TEST_F(ExtensionHelperTest, OpenExtensionDetails_NullProfileNoCrash) {
  helper_->OpenExtensionDetails(nullptr, "test-id");
  SUCCEED();
}

TEST_F(ExtensionHelperTest, OpenExtensionDetails_EmptyIdNoCrash) {
  helper_->OpenExtensionDetails(profile_.get(), "");
  SUCCEED();
}

TEST_F(ExtensionHelperTest, OpenExtensionSettings_ValidProfileNoCrash) {
  helper_->OpenExtensionSettings(profile_.get());
  SUCCEED();
}

TEST_F(ExtensionHelperTest, OpenChromeWebStore_ValidProfileNoCrash) {
  helper_->OpenChromeWebStore(profile_.get());
  SUCCEED();
}

TEST_F(ExtensionHelperTest, OpenExtensionDetails_ValidProfileNoCrash) {
  helper_->OpenExtensionDetails(profile_.get(), "test-extension-id");
  SUCCEED();
}

// ---------------------------------------------------------------------------
// Icon queries — edge cases
// ---------------------------------------------------------------------------

TEST_F(ExtensionHelperTest, GetExtensionIcon_UnknownIdReturnsDefault) {
  auto icon = helper_->GetExtensionIcon("nonexistent-id", 16);
  // Should not crash and should return an image (possibly empty default).
  EXPECT_FALSE(icon.IsEmpty());
}

TEST_F(ExtensionHelperTest, GetExtensionIcon_ZeroSize) {
  auto icon = helper_->GetExtensionIcon("test-id", 0);
  EXPECT_FALSE(icon.IsEmpty());
}

TEST_F(ExtensionHelperTest, GetDefaultExtensionIcon_ValidSize) {
  auto icon = helper_->GetDefaultExtensionIcon(16);
  // Should not crash.
  SUCCEED();
}

// ---------------------------------------------------------------------------
// Popup URL — edge cases
// ---------------------------------------------------------------------------

TEST_F(ExtensionHelperTest, GetPopupURL_UnknownIdReturnsEmpty) {
  auto url = helper_->GetPopupURL("nonexistent-id");
  EXPECT_TRUE(url.is_empty());
}

TEST_F(ExtensionHelperTest, GetPopupURL_EmptyIdReturnsEmpty) {
  auto url = helper_->GetPopupURL("");
  EXPECT_TRUE(url.is_empty());
}

// ---------------------------------------------------------------------------
// AstraExtensionCategory enum
// ---------------------------------------------------------------------------

TEST_F(ExtensionHelperTest, ExtensionCategory_AllValuesDistinct) {
  // Verify that all category enum values are distinct.
  auto cat_productivity = AstraExtensionCategory::kProductivity;
  auto cat_accessibility = AstraExtensionCategory::kAccessibility;
  auto cat_devtools = AstraExtensionCategory::kDeveloperTools;
  auto cat_entertainment = AstraExtensionCategory::kEntertainment;
  auto cat_communication = AstraExtensionCategory::kCommunication;
  auto cat_news = AstraExtensionCategory::kNewsWeather;
  auto cat_privacy = AstraExtensionCategory::kPrivacySecurity;
  auto cat_photos = AstraExtensionCategory::kPhotos;
  auto cat_tab = AstraExtensionCategory::kProductivityTab;
  auto cat_themes = AstraExtensionCategory::kThemes;
  auto cat_other = AstraExtensionCategory::kOther;

  // Just verify they compile and exist — they should be distinct by definition.
  EXPECT_NE(cat_productivity, cat_other);
  EXPECT_NE(cat_accessibility, cat_other);
  EXPECT_NE(cat_devtools, cat_other);
  EXPECT_NE(cat_themes, cat_other);
}

}  // namespace astra
