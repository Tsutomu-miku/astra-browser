// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/site_settings/astra_site_settings_model.h"
#include "astra/ui/views/site_settings/astra_site_settings_view.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

// ===========================================================================
// AstraSiteSettingsModelTest
// ===========================================================================

class AstraSiteSettingsModelTest : public testing::Test {
 protected:
  void SetUp() override {
    model_ = std::make_unique<AstraSiteSettingsModel>();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<AstraSiteSettingsModel> model_;
};

// Test model creation.
TEST_F(AstraSiteSettingsModelTest, Creation) {
  EXPECT_EQ(0u, model_->GetSiteCount());
  EXPECT_TRUE(model_->GetAllSites().empty());
  EXPECT_EQ(AstraSiteSettingsCategory::kAll, model_->GetCategory());
  EXPECT_EQ(AstraSiteSettingsFilter::kAll, model_->GetFilter());
  EXPECT_EQ(AstraSiteSettingsSort::kLastVisited, model_->GetSort());
  EXPECT_TRUE(model_->GetSearchQuery().empty());
  EXPECT_FALSE(model_->IsLoading());
}

// Test populate sample sites.
TEST_F(AstraSiteSettingsModelTest, PopulateSampleSites) {
  model_->PopulateSampleSites();
  EXPECT_GT(model_->GetSiteCount(), 5u);
  EXPECT_FALSE(model_->GetAllSites().empty());
}

// Test get site by ID.
TEST_F(AstraSiteSettingsModelTest, GetSite) {
  model_->PopulateSampleSites();
  auto all = model_->GetAllSites();
  ASSERT_FALSE(all.empty());

  const auto& first = all[0];
  const auto* found = model_->GetSite(first.id);
  ASSERT_NE(nullptr, found);
  EXPECT_EQ(first.id, found->id);
  EXPECT_EQ(first.origin, found->origin);

  // Non-existent ID.
  EXPECT_EQ(nullptr, model_->GetSite("nonexistent"));
}

// Test remove site.
TEST_F(AstraSiteSettingsModelTest, RemoveSite) {
  model_->PopulateSampleSites();
  size_t initial_count = model_->GetSiteCount();
  auto all = model_->GetAllSites();
  ASSERT_FALSE(all.empty());

  std::string id = all[0].id;
  model_->RemoveSite(id);
  EXPECT_EQ(initial_count - 1, model_->GetSiteCount());
  EXPECT_EQ(nullptr, model_->GetSite(id));

  // Remove non-existent should do nothing.
  model_->RemoveSite("nonexistent");
  EXPECT_EQ(initial_count - 1, model_->GetSiteCount());
}

// Test clear site data.
TEST_F(AstraSiteSettingsModelTest, ClearSiteData) {
  model_->PopulateSampleSites();
  auto all = model_->GetAllSites();
  auto it = std::find_if(all.begin(), all.end(),
                         [](const AstraSiteSettingsEntry& s) {
                           return s.storage_bytes > 0;
                         });
  ASSERT_NE(it, all.end());

  std::string id = it->id;
  model_->ClearSiteData(id);

  const auto* site = model_->GetSite(id);
  ASSERT_NE(nullptr, site);
  EXPECT_EQ(0, site->storage_bytes);
  EXPECT_EQ(0u, site->cookies_count);
}

// Test reset site permissions.
TEST_F(AstraSiteSettingsModelTest, ResetSitePermissions) {
  model_->PopulateSampleSites();
  auto all = model_->GetAllSites();
  auto it = std::find_if(all.begin(), all.end(),
                         [](const AstraSiteSettingsEntry& s) {
                           return !s.permissions.empty();
                         });
  ASSERT_NE(it, all.end());

  std::string id = it->id;
  model_->ResetSitePermissions(id);

  const auto* site = model_->GetSite(id);
  ASSERT_NE(nullptr, site);
  // After reset, all permissions should be default.
  for (const auto& p : site->permissions) {
    EXPECT_TRUE(p.is_default);
  }
}

// Test set site permission.
TEST_F(AstraSiteSettingsModelTest, SetSitePermission) {
  model_->PopulateSampleSites();
  auto all = model_->GetAllSites();
  ASSERT_FALSE(all.empty());

  std::string id = all[0].id;
  model_->SetSitePermission(id, AstraSitePermissionType::kCamera,
                            AstraContentSetting::kBlock);

  const auto* site = model_->GetSite(id);
  ASSERT_NE(nullptr, site);

  auto perm_it = std::find_if(site->permissions.begin(),
                              site->permissions.end(),
                              [](const AstraSitePermission& p) {
                                return p.type ==
                                    AstraSitePermissionType::kCamera;
                              });
  ASSERT_NE(perm_it, site->permissions.end());
  EXPECT_EQ(AstraContentSetting::kBlock, perm_it->setting);
  EXPECT_FALSE(perm_it->is_default);
}

// Test default permissions.
TEST_F(AstraSiteSettingsModelTest, DefaultPermissions) {
  // Camera should default to Ask.
  EXPECT_EQ(AstraContentSetting::kAsk,
      model_->GetDefaultPermission(AstraSitePermissionType::kCamera));

  // JavaScript should default to Allow.
  EXPECT_EQ(AstraContentSetting::kAllow,
      model_->GetDefaultPermission(AstraSitePermissionType::kJavaScript));

  // Popups should default to Block.
  EXPECT_EQ(AstraContentSetting::kBlock,
      model_->GetDefaultPermission(AstraSitePermissionType::kPopups));
}

// Test set default permission.
TEST_F(AstraSiteSettingsModelTest, SetDefaultPermission) {
  model_->SetDefaultPermission(AstraSitePermissionType::kCamera,
                                AstraContentSetting::kAllow);
  EXPECT_EQ(AstraContentSetting::kAllow,
      model_->GetDefaultPermission(AstraSitePermissionType::kCamera));
}

// Test search filter.
TEST_F(AstraSiteSettingsModelTest, SearchFilter) {
  model_->PopulateSampleSites();
  size_t all_count = model_->GetFilteredSites().size();
  EXPECT_GT(all_count, 0u);

  model_->SetSearchQuery("google");
  auto filtered = model_->GetFilteredSites();
  EXPECT_LT(filtered.size(), all_count);
  for (const auto& s : filtered) {
    std::string origin = base::ToLowerASCII(s.origin);
    std::string name = base::UTF16ToUTF8(
        base::ToLowerASCII(s.display_name));
    EXPECT_TRUE(origin.find("google") != std::string::npos ||
                name.find("google") != std::string::npos);
  }

  model_->SetSearchQuery("");
  EXPECT_EQ(all_count, model_->GetFilteredSites().size());
}

// Test category filter.
TEST_F(AstraSiteSettingsModelTest, CategoryFilter) {
  model_->PopulateSampleSites();
  size_t all_count = model_->GetFilteredSites().size();

  model_->SetCategory(AstraSiteSettingsCategory::kNotifications);
  auto notification_sites = model_->GetFilteredSites();
  for (const auto& s : notification_sites) {
    bool has_notification = std::any_of(
        s.permissions.begin(), s.permissions.end(),
        [](const AstraSitePermission& p) {
          return p.type == AstraSitePermissionType::kNotifications;
        });
    EXPECT_TRUE(has_notification);
  }

  model_->SetCategory(AstraSiteSettingsCategory::kAll);
  EXPECT_EQ(all_count, model_->GetFilteredSites().size());
}

// Test sort by name.
TEST_F(AstraSiteSettingsModelTest, SortByName) {
  model_->PopulateSampleSites();
  model_->SetSort(AstraSiteSettingsSort::kName);
  auto sorted = model_->GetFilteredSites();

  for (size_t i = 1; i < sorted.size(); ++i) {
    EXPECT_LE(sorted[i - 1].display_name, sorted[i].display_name);
  }
}

// Test sort by storage.
TEST_F(AstraSiteSettingsModelTest, SortByStorage) {
  model_->PopulateSampleSites();
  model_->SetSort(AstraSiteSettingsSort::kStorage);
  auto sorted = model_->GetFilteredSites();

  for (size_t i = 1; i < sorted.size(); ++i) {
    EXPECT_GE(sorted[i - 1].storage_bytes, sorted[i].storage_bytes);
  }
}

// Test filter by allowed.
TEST_F(AstraSiteSettingsModelTest, FilterByAllowed) {
  model_->PopulateSampleSites();
  model_->SetFilter(AstraSiteSettingsFilter::kAllowed);
  auto filtered = model_->GetFilteredSites();

  for (const auto& s : filtered) {
    bool has_allowed = std::any_of(
        s.permissions.begin(), s.permissions.end(),
        [](const AstraSitePermission& p) {
          return p.setting == AstraContentSetting::kAllow && !p.is_default;
        });
    EXPECT_TRUE(has_allowed);
  }
}

// Test filter by blocked.
TEST_F(AstraSiteSettingsModelTest, FilterByBlocked) {
  model_->PopulateSampleSites();
  model_->SetFilter(AstraSiteSettingsFilter::kBlocked);
  auto filtered = model_->GetFilteredSites();

  for (const auto& s : filtered) {
    bool has_blocked = std::any_of(
        s.permissions.begin(), s.permissions.end(),
        [](const AstraSitePermission& p) {
          return p.setting == AstraContentSetting::kBlock && !p.is_default;
        });
    EXPECT_TRUE(has_blocked);
  }
}

// Test filter by with data.
TEST_F(AstraSiteSettingsModelTest, FilterByWithData) {
  model_->PopulateSampleSites();
  model_->SetFilter(AstraSiteSettingsFilter::kWithData);
  auto filtered = model_->GetFilteredSites();

  for (const auto& s : filtered) {
    EXPECT_TRUE(s.storage_bytes > 0 || s.cookies_count > 0);
  }
}

// Test grouped sites.
TEST_F(AstraSiteSettingsModelTest, GroupedSites) {
  model_->PopulateSampleSites();
  auto groups = model_->GetGroupedSites();
  EXPECT_FALSE(groups.empty());

  size_t total = 0;
  for (const auto& g : groups) {
    total += g.sites.size();
  }
  EXPECT_EQ(model_->GetFilteredSites().size(), total);
}

// Test permission names.
TEST_F(AstraSiteSettingsModelTest, PermissionNames) {
  EXPECT_FALSE(AstraSiteSettingsModel::GetPermissionName(
      AstraSitePermissionType::kCamera).empty());
  EXPECT_FALSE(AstraSiteSettingsModel::GetPermissionName(
      AstraSitePermissionType::kGeolocation).empty());
  EXPECT_FALSE(AstraSiteSettingsModel::GetPermissionName(
      AstraSitePermissionType::kNotifications).empty());
}

// Test permission descriptions.
TEST_F(AstraSiteSettingsModelTest, PermissionDescriptions) {
  EXPECT_FALSE(AstraSiteSettingsModel::GetPermissionDescription(
      AstraSitePermissionType::kCamera).empty());
}

// Test permission icon names.
TEST_F(AstraSiteSettingsModelTest, PermissionIconNames) {
  EXPECT_FALSE(AstraSiteSettingsModel::GetPermissionIconName(
      AstraSitePermissionType::kCamera).empty());
}

// Test categories list.
TEST_F(AstraSiteSettingsModelTest, CategoriesList) {
  auto categories = AstraSiteSettingsModel::GetCategories();
  EXPECT_FALSE(categories.empty());
  EXPECT_GT(categories.size(), 5u);
}

// Test loading state.
TEST_F(AstraSiteSettingsModelTest, LoadingState) {
  EXPECT_FALSE(model_->IsLoading());
  model_->SetLoading(true);
  EXPECT_TRUE(model_->IsLoading());
  model_->SetLoading(false);
  EXPECT_FALSE(model_->IsLoading());
}

// ===========================================================================
// AstraSiteSettingsPageViewTest
// ===========================================================================

class AstraSiteSettingsPageViewTest : public testing::Test {
 protected:
  void SetUp() override {
    model_ = std::make_unique<AstraSiteSettingsModel>();
    model_->PopulateSampleSites();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<AstraSiteSettingsModel> model_;
};

// Test model has sample data.
TEST_F(AstraSiteSettingsPageViewTest, SampleData) {
  EXPECT_GT(model_->GetSiteCount(), 5u);
  EXPECT_FALSE(model_->GetAllSites().empty());
}

// Test filtered sites work.
TEST_F(AstraSiteSettingsPageViewTest, FilteredSites) {
  auto all = model_->GetFilteredSites();
  EXPECT_EQ(model_->GetSiteCount(), all.size());

  model_->SetSearchQuery("nonexistent_site_xyz");
  auto filtered = model_->GetFilteredSites();
  EXPECT_TRUE(filtered.empty());
}

// Test categories.
TEST_F(AstraSiteSettingsPageViewTest, Categories) {
  auto categories = AstraSiteSettingsModel::GetCategories();
  EXPECT_FALSE(categories.empty());

  // All category should have names.
  for (const auto& [cat, name] : categories) {
    EXPECT_FALSE(name.empty());
  }
}

// Test site entry structure.
TEST_F(AstraSiteSettingsPageViewTest, SiteEntryStructure) {
  auto all = model_->GetAllSites();
  ASSERT_FALSE(all.empty());

  const auto& first = all[0];
  EXPECT_FALSE(first.id.empty());
  EXPECT_FALSE(first.origin.empty());
  EXPECT_FALSE(first.display_name.empty());
  EXPECT_GE(first.storage_bytes, 0);
}

}  // namespace astra
