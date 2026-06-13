// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/app/astra_feature_list.h"

#include <set>
#include <string>
#include <vector>

#include "base/feature_list.h"
#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

namespace {

// List of all Astra features for test parametrization.
const base::Feature* kAllAstraFeatures[] = {
    &kAstraBrandedBuild,
    &kAstraSidebar,
    &kAstraWorkspaces,
    &kAstraSplitView,
    &kAstraCommandPalette,
    &kAstraFavorites,
    &kAstraTabSearch,
    &kAstraGlance,
    &kAstraFocusMode,
    &kAstraReadingList,
    &kAstraNotes,
    &kAstraMemorySaver,
    &kAstraScreenshot,
    &kAstraPip,
    &kAstraDevTools,
    &kAstraAccessibility,
};

// Expected default-enabled features (when branded build is on).
const base::Feature* kDefaultEnabledFeatures[] = {
    &kAstraBrandedBuild,
    &kAstraSidebar,
    &kAstraWorkspaces,
    &kAstraSplitView,
};

// Expected default-disabled features.
const base::Feature* kDefaultDisabledFeatures[] = {
    &kAstraCommandPalette,
    &kAstraFavorites,
    &kAstraTabSearch,
    &kAstraGlance,
    &kAstraFocusMode,
    &kAstraReadingList,
    &kAstraNotes,
    &kAstraMemorySaver,
    &kAstraScreenshot,
    &kAstraPip,
    &kAstraDevTools,
    &kAstraAccessibility,
};

// Total number of Astra features.
constexpr size_t kTotalAstraFeatures = std::size(kAllAstraFeatures);

// Helper: check if a feature is in the default-enabled list.
bool IsDefaultEnabledFeature(const base::Feature* feature) {
  for (const auto* f : kDefaultEnabledFeatures) {
    if (f == feature) {
      return true;
    }
  }
  return false;
}

}  // namespace

// =========================================================================
// AstraFeatureListTest — fixture for feature list tests
// =========================================================================

class AstraFeatureListTest : public testing::Test {
 protected:
  void SetUp() override {
    // Ensure clean state before each test.
    ResetAstraFeatureOverridesForTesting();
  }

  void TearDown() override {
    // Clean up after each test.
    ResetAstraFeatureOverridesForTesting();
  }

  base::test::ScopedFeatureList scoped_feature_list_;
};

// =========================================================================
// Feature defaults tests
// =========================================================================

TEST_F(AstraFeatureListTest, FeatureCountIsCorrect) {
  // Verify we have the expected total number of features.
  // This test fails if features are added or removed without updating tests.
  auto features = GetAllAstraFeatures();
  EXPECT_EQ(features.size(), kTotalAstraFeatures)
      << "Expected " << kTotalAstraFeatures << " Astra features, got "
      << features.size()
      << ". Update test expectations if features were added or removed.";
}

TEST_F(AstraFeatureListTest, BrandedBuildDefaultEnabled) {
  // kAstraBrandedBuild should be enabled by default.
  // (ScopedFeatureList uses default state when no overrides are set.)
  EXPECT_TRUE(base::FeatureList::IsEnabled(kAstraBrandedBuild));
}

TEST_F(AstraFeatureListTest, SidebarDefaultEnabled) {
  // kAstraSidebar should be enabled by default.
  EXPECT_TRUE(base::FeatureList::IsEnabled(kAstraSidebar));
}

TEST_F(AstraFeatureListTest, WorkspacesDefaultEnabled) {
  // kAstraWorkspaces should be enabled by default.
  EXPECT_TRUE(base::FeatureList::IsEnabled(kAstraWorkspaces));
}

TEST_F(AstraFeatureListTest, SplitViewDefaultEnabled) {
  // kAstraSplitView should be enabled by default.
  EXPECT_TRUE(base::FeatureList::IsEnabled(kAstraSplitView));
}

TEST_F(AstraFeatureListTest, CommandPaletteDefaultDisabled) {
  // kAstraCommandPalette should be disabled by default.
  EXPECT_FALSE(base::FeatureList::IsEnabled(kAstraCommandPalette));
}

TEST_F(AstraFeatureListTest, FavoritesDefaultDisabled) {
  EXPECT_FALSE(base::FeatureList::IsEnabled(kAstraFavorites));
}

TEST_F(AstraFeatureListTest, TabSearchDefaultDisabled) {
  EXPECT_FALSE(base::FeatureList::IsEnabled(kAstraTabSearch));
}

TEST_F(AstraFeatureListTest, GlanceDefaultDisabled) {
  EXPECT_FALSE(base::FeatureList::IsEnabled(kAstraGlance));
}

TEST_F(AstraFeatureListTest, FocusModeDefaultDisabled) {
  EXPECT_FALSE(base::FeatureList::IsEnabled(kAstraFocusMode));
}

TEST_F(AstraFeatureListTest, ReadingListDefaultDisabled) {
  EXPECT_FALSE(base::FeatureList::IsEnabled(kAstraReadingList));
}

TEST_F(AstraFeatureListTest, NotesDefaultDisabled) {
  EXPECT_FALSE(base::FeatureList::IsEnabled(kAstraNotes));
}

TEST_F(AstraFeatureListTest, MemorySaverDefaultDisabled) {
  EXPECT_FALSE(base::FeatureList::IsEnabled(kAstraMemorySaver));
}

TEST_F(AstraFeatureListTest, ScreenshotDefaultDisabled) {
  EXPECT_FALSE(base::FeatureList::IsEnabled(kAstraScreenshot));
}

TEST_F(AstraFeatureListTest, PipDefaultDisabled) {
  EXPECT_FALSE(base::FeatureList::IsEnabled(kAstraPip));
}

TEST_F(AstraFeatureListTest, DevToolsDefaultDisabled) {
  EXPECT_FALSE(base::FeatureList::IsEnabled(kAstraDevTools));
}

TEST_F(AstraFeatureListTest, AccessibilityDefaultDisabled) {
  EXPECT_FALSE(base::FeatureList::IsEnabled(kAstraAccessibility));
}

TEST_F(AstraFeatureListTest, DefaultEnabledCountMatchesExpected) {
  // The number of default-enabled features should match our expectations.
  int expected_enabled =
      static_cast<int>(std::size(kDefaultEnabledFeatures));
  int actual_enabled = GetEnabledAstraFeatureCount();

  // Note: GetEnabledAstraFeatureCount uses effective state, which considers
  // overrides.  With no overrides, it should match the base::FeatureList state.
  EXPECT_EQ(actual_enabled, expected_enabled);
}

TEST_F(AstraFeatureListTest, DefaultDisabledCountMatchesExpected) {
  int expected_disabled =
      static_cast<int>(kTotalAstraFeatures - std::size(kDefaultEnabledFeatures));
  int actual_enabled = GetEnabledAstraFeatureCount();
  int actual_disabled =
      static_cast<int>(kTotalAstraFeatures) - actual_enabled;

  EXPECT_EQ(actual_disabled, expected_disabled);
}

// =========================================================================
// IsAstraFeatureEnabled tests
// =========================================================================

TEST_F(AstraFeatureListTest, IsAstraFeatureEnabled_WithBranding_EnabledFeature) {
  // With branding required and an enabled feature, should return true.
  EXPECT_TRUE(IsAstraFeatureEnabled(kAstraSidebar, /*requires_branding=*/true));
}

TEST_F(AstraFeatureListTest, IsAstraFeatureEnabled_WithBranding_DisabledFeature) {
  // With branding required and a disabled feature, should return false.
  EXPECT_FALSE(
      IsAstraFeatureEnabled(kAstraCommandPalette, /*requires_branding=*/true));
}

TEST_F(AstraFeatureListTest,
       IsAstraFeatureEnabled_NoBranding_EnabledFeatureStillEnabled) {
  // Without branding requirement, enabled features are still enabled.
  EXPECT_TRUE(
      IsAstraFeatureEnabled(kAstraSidebar, /*requires_branding=*/false));
}

TEST_F(AstraFeatureListTest,
       IsAstraFeatureEnabled_NoBranding_DisabledFeatureStillDisabled) {
  // Without branding requirement, disabled features are still disabled.
  EXPECT_FALSE(
      IsAstraFeatureEnabled(kAstraCommandPalette, /*requires_branding=*/false));
}

TEST_F(AstraFeatureListTest,
       IsAstraFeatureEnabled_BrandedBuildItselfDoesNotRequireBranding) {
  // The branded build feature itself should work even with branding required
  // (it's the gate, not gated by itself).
  // Actually, with requires_branding=true, it checks kAstraBrandedBuild first.
  // If kAstraBrandedBuild is enabled, then it also checks the target feature.
  // Since the target feature IS kAstraBrandedBuild, it should return true
  // when the feature is enabled.
  EXPECT_TRUE(
      IsAstraFeatureEnabled(kAstraBrandedBuild, /*requires_branding=*/true));
}

TEST_F(AstraFeatureListTest,
       IsAstraFeatureEnabled_BrandedBuildDisabled_DisablesAllBrandedFeatures) {
  // If branded build is disabled, all features with requires_branding=true
  // should return false, even if they're individually enabled.

  // Disable the branded build feature.
  scoped_feature_list_.InitAndDisableFeature(kAstraBrandedBuild);

  // Sidebar is normally enabled, but with branding required and branded build
  // disabled, it should be disabled.
  EXPECT_FALSE(IsAstraFeatureEnabled(kAstraSidebar, /*requires_branding=*/true));
  EXPECT_FALSE(
      IsAstraFeatureEnabled(kAstraWorkspaces, /*requires_branding=*/true));
  EXPECT_FALSE(
      IsAstraFeatureEnabled(kAstraSplitView, /*requires_branding=*/true));
}

TEST_F(AstraFeatureListTest,
       IsAstraFeatureEnabled_BrandedBuildDisabled_NoBrandingStillWorks) {
  // If branded build is disabled but requires_branding=false, the feature
  // state should reflect its own flag, not the branding gate.

  scoped_feature_list_.InitWithFeatures(
      /*enabled_features=*/{},
      /*disabled_features=*/{kAstraBrandedBuild});

  // Sidebar is enabled by default.  Without branding requirement, it should
  // still be enabled even when branded build is off.
  EXPECT_TRUE(
      IsAstraFeatureEnabled(kAstraSidebar, /*requires_branding=*/false));
}

TEST_F(AstraFeatureListTest,
       IsAstraFeatureEnabled_BrandedBuildEnabled_AllowsFeatures) {
  // If branded build is explicitly enabled, features should work normally.

  scoped_feature_list_.InitWithFeatures(
      /*enabled_features=*/{kAstraBrandedBuild, kAstraCommandPalette},
      /*disabled_features=*/{});

  EXPECT_TRUE(
      IsAstraFeatureEnabled(kAstraCommandPalette, /*requires_branding=*/true));
  EXPECT_TRUE(IsAstraFeatureEnabled(kAstraSidebar, /*requires_branding=*/true));
}

TEST_F(AstraFeatureListTest,
       IsAstraFeatureEnabled_DefaultRequiresBrandingIsTrue) {
  // The default value of requires_branding should be true.
  // Both calls should return the same result.
  EXPECT_EQ(IsAstraFeatureEnabled(kAstraSidebar),
            IsAstraFeatureEnabled(kAstraSidebar, /*requires_branding=*/true));
  EXPECT_EQ(IsAstraFeatureEnabled(kAstraCommandPalette),
            IsAstraFeatureEnabled(kAstraCommandPalette,
                                  /*requires_branding=*/true));
}

// =========================================================================
// GetAllAstraFeatures tests
// =========================================================================

TEST_F(AstraFeatureListTest, GetAllAstraFeatures_ReturnsAllFeatures) {
  auto features = GetAllAstraFeatures();
  EXPECT_EQ(features.size(), kTotalAstraFeatures);
}

TEST_F(AstraFeatureListTest, GetAllAstraFeatures_HasNames) {
  auto features = GetAllAstraFeatures();
  for (const auto& feature : features) {
    EXPECT_FALSE(feature.name.empty())
        << "Feature has empty name";
  }
}

TEST_F(AstraFeatureListTest, GetAllAstraFeatures_HasDescriptions) {
  auto features = GetAllAstraFeatures();
  for (const auto& feature : features) {
    EXPECT_FALSE(feature.description.empty())
        << "Feature " << feature.name << " has empty description";
  }
}

TEST_F(AstraFeatureListTest, GetAllAstraFeatures_DescriptionsAreNonTrivial) {
  // Descriptions should be more than just a few characters.
  auto features = GetAllAstraFeatures();
  for (const auto& feature : features) {
    EXPECT_GT(feature.description.length(), 10u)
        << "Feature " << feature.name
        << " has suspiciously short description: " << feature.description;
  }
}

TEST_F(AstraFeatureListTest, GetAllAstraFeatures_NamesStartWithAstra) {
  // All Astra feature names should start with "Astra".
  auto features = GetAllAstraFeatures();
  for (const auto& feature : features) {
    EXPECT_EQ(feature.name.substr(0, 5), "Astra")
        << "Feature name does not start with 'Astra': " << feature.name;
  }
}

TEST_F(AstraFeatureListTest, GetAllAstraFeatures_NoDuplicateNames) {
  // All feature names should be unique.
  auto features = GetAllAstraFeatures();
  std::set<std::string> names;
  for (const auto& feature : features) {
    EXPECT_TRUE(names.insert(feature.name).second)
        << "Duplicate feature name: " << feature.name;
  }
}

TEST_F(AstraFeatureListTest, GetAllAstraFeatures_SortedByName) {
  // Features should be returned in sorted order.
  auto features = GetAllAstraFeatures();
  for (size_t i = 1; i < features.size(); ++i) {
    EXPECT_LT(features[i - 1].name, features[i].name)
        << "Features not sorted: " << features[i - 1].name
        << " should come before " << features[i].name;
  }
}

TEST_F(AstraFeatureListTest, GetAllAstraFeatures_EnabledStateMatchesDefaults) {
  // The enabled state in GetAllAstraFeatures should match base::FeatureList.
  auto features = GetAllAstraFeatures();
  for (const auto& feature_info : features) {
    // Find the corresponding base::Feature.
    bool found = false;
    for (const auto* feature : kAllAstraFeatures) {
      if (feature_info.name == feature->name) {
        EXPECT_EQ(feature_info.enabled,
                  base::FeatureList::IsEnabled(*feature))
            << "Feature " << feature_info.name
            << " enabled state mismatch";
        found = true;
        break;
      }
    }
    EXPECT_TRUE(found) << "Feature " << feature_info.name
                       << " not found in known feature list";
  }
}

TEST_F(AstraFeatureListTest, GetAllAstraFeatures_BrandedBuildIsPresent) {
  auto features = GetAllAstraFeatures();
  bool found = false;
  for (const auto& f : features) {
    if (f.name == "AstraBrandedBuild") {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found) << "AstraBrandedBuild not found in feature list";
}

TEST_F(AstraFeatureListTest, GetAllAstraFeatures_SidebarIsPresent) {
  auto features = GetAllAstraFeatures();
  bool found = false;
  for (const auto& f : features) {
    if (f.name == "AstraSidebar") {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

// =========================================================================
// GetEnabledAstraFeatureCount tests
// =========================================================================

TEST_F(AstraFeatureListTest, GetEnabledAstraFeatureCount_DefaultValue) {
  // With default feature states, count should match default-enabled count.
  int count = GetEnabledAstraFeatureCount();
  EXPECT_EQ(count, static_cast<int>(std::size(kDefaultEnabledFeatures)));
}

TEST_F(AstraFeatureListTest,
       GetEnabledAstraFeatureCount_EnablingOneFeatureIncreasesCount) {
  // Enable one feature and verify count increases by 1.
  int before = GetEnabledAstraFeatureCount();

  scoped_feature_list_.InitAndEnableFeature(kAstraCommandPalette);

  int after = GetEnabledAstraFeatureCount();
  EXPECT_EQ(after, before + 1);
}

TEST_F(AstraFeatureListTest,
       GetEnabledAstraFeatureCount_DisablingOneFeatureDecreasesCount) {
  // Disable one default-enabled feature and verify count decreases by 1.
  int before = GetEnabledAstraFeatureCount();

  scoped_feature_list_.InitAndDisableFeature(kAstraSidebar);

  int after = GetEnabledAstraFeatureCount();
  EXPECT_EQ(after, before - 1);
}

TEST_F(AstraFeatureListTest,
       GetEnabledAstraFeatureCount_EnablingAllFeatures) {
  // Enable all features and verify count equals total.
  std::vector<base::test::FeatureRef> all_features;
  for (const auto* f : kAllAstraFeatures) {
    all_features.push_back(*f);
  }

  scoped_feature_list_.InitWithFeatures(all_features, {});

  int count = GetEnabledAstraFeatureCount();
  EXPECT_EQ(count, static_cast<int>(kTotalAstraFeatures));
}

TEST_F(AstraFeatureListTest,
       GetEnabledAstraFeatureCount_DisablingAllFeatures) {
  // Disable all features and verify count is 0.
  std::vector<base::test::FeatureRef> all_features;
  for (const auto* f : kAllAstraFeatures) {
    all_features.push_back(*f);
  }

  scoped_feature_list_.InitWithFeatures({}, all_features);

  int count = GetEnabledAstraFeatureCount();
  EXPECT_EQ(count, 0);
}

TEST_F(AstraFeatureListTest,
       GetEnabledAstraFeatureCount_MatchesGetAllAstraFeaturesSum) {
  // The count from GetEnabledAstraFeatureCount should match the sum of
  // enabled entries from GetAllAstraFeatures.
  int count = GetEnabledAstraFeatureCount();

  auto features = GetAllAstraFeatures();
  int sum = 0;
  for (const auto& f : features) {
    if (f.enabled) {
      ++sum;
    }
  }

  EXPECT_EQ(count, sum);
}

TEST_F(AstraFeatureListTest, GetEnabledAstraFeatureCount_RangeIsValid) {
  // Count should be between 0 and total features, inclusive.
  int count = GetEnabledAstraFeatureCount();
  EXPECT_GE(count, 0);
  EXPECT_LE(count, static_cast<int>(kTotalAstraFeatures));
}

// =========================================================================
// Feature descriptions tests
// =========================================================================

TEST_F(AstraFeatureListTest, BrandedBuildDescriptionIsMeaningful) {
  auto features = GetAllAstraFeatures();
  for (const auto& f : features) {
    if (f.name == "AstraBrandedBuild") {
      EXPECT_NE(f.description.find("branding"), std::string::npos);
      return;
    }
  }
  FAIL() << "AstraBrandedBuild not found";
}

TEST_F(AstraFeatureListTest, SidebarDescriptionMentionsSidebar) {
  auto features = GetAllAstraFeatures();
  for (const auto& f : features) {
    if (f.name == "AstraSidebar") {
      EXPECT_NE(f.description.find("sidebar"), std::string::npos)
          << "Sidebar description doesn't mention sidebar: " << f.description;
      return;
    }
  }
  FAIL() << "AstraSidebar not found";
}

TEST_F(AstraFeatureListTest, WorkspacesDescriptionMentionsWorkspaces) {
  auto features = GetAllAstraFeatures();
  for (const auto& f : features) {
    if (f.name == "AstraWorkspaces") {
      EXPECT_NE(f.description.find("workspace"), std::string::npos);
      return;
    }
  }
  FAIL() << "AstraWorkspaces not found";
}

TEST_F(AstraFeatureListTest, SplitViewDescriptionMentionsSplit) {
  auto features = GetAllAstraFeatures();
  for (const auto& f : features) {
    if (f.name == "AstraSplitView") {
      EXPECT_NE(f.description.find("split"), std::string::npos);
      return;
    }
  }
  FAIL() << "AstraSplitView not found";
}

TEST_F(AstraFeatureListTest, CommandPaletteDescriptionMentionsCommand) {
  auto features = GetAllAstraFeatures();
  for (const auto& f : features) {
    if (f.name == "AstraCommandPalette") {
      EXPECT_NE(f.description.find("command"), std::string::npos);
      return;
    }
  }
  FAIL() << "AstraCommandPalette not found";
}

// =========================================================================
// Feature name pattern tests
// =========================================================================

TEST_F(AstraFeatureListTest, AllFeatureNamesFollowNamingConvention) {
  // All Astra features should be named "Astra<FeatureName>".
  auto features = GetAllAstraFeatures();
  for (const auto& feature : features) {
    // Should start with "Astra"
    EXPECT_TRUE(feature.name.size() > 5 && feature.name.substr(0, 5) == "Astra")
        << "Feature name doesn't start with Astra: " << feature.name;

    // Should not contain spaces
    EXPECT_EQ(feature.name.find(' '), std::string::npos)
        << "Feature name contains spaces: " << feature.name;
  }
}

TEST_F(AstraFeatureListTest, FeatureNamesAreAlphaNumeric) {
  // Feature names should only contain alphanumeric characters.
  auto features = GetAllAstraFeatures();
  for (const auto& feature : features) {
    for (char c : feature.name) {
      EXPECT_TRUE(std::isalnum(static_cast<unsigned char>(c)))
          << "Feature name contains non-alphanumeric character '" << c
          << "' in: " << feature.name;
    }
  }
}

// =========================================================================
// ResetAstraFeatureOverridesForTesting tests
// =========================================================================

TEST_F(AstraFeatureListTest, ResetOverrides_DoesNotCrash) {
  // Calling reset should not crash or cause issues.
  ResetAstraFeatureOverridesForTesting();
  // Should work multiple times.
  ResetAstraFeatureOverridesForTesting();
  SUCCEED();
}

TEST_F(AstraFeatureListTest, ResetOverrides_PreservesBaseFeatureListState) {
  // Resetting overrides should not change base::FeatureList state.
  bool sidebar_before = base::FeatureList::IsEnabled(kAstraSidebar);
  bool command_before = base::FeatureList::IsEnabled(kAstraCommandPalette);

  ResetAstraFeatureOverridesForTesting();

  EXPECT_EQ(base::FeatureList::IsEnabled(kAstraSidebar), sidebar_before);
  EXPECT_EQ(base::FeatureList::IsEnabled(kAstraCommandPalette), command_before);
}

TEST_F(AstraFeatureListTest, ResetOverrides_GetEnabledCountStable) {
  // Resetting overrides should result in the same enabled count (since
  // with no overrides, it reflects base::FeatureList state).
  int before = GetEnabledAstraFeatureCount();
  ResetAstraFeatureOverridesForTesting();
  int after = GetEnabledAstraFeatureCount();
  EXPECT_EQ(before, after);
}

TEST_F(AstraFeatureListTest, ResetOverrides_IsAstraFeatureEnabledStable) {
  bool sidebar_before = IsAstraFeatureEnabled(kAstraSidebar);
  bool command_before = IsAstraFeatureEnabled(kAstraCommandPalette);

  ResetAstraFeatureOverridesForTesting();

  EXPECT_EQ(IsAstraFeatureEnabled(kAstraSidebar), sidebar_before);
  EXPECT_EQ(IsAstraFeatureEnabled(kAstraCommandPalette), command_before);
}

// =========================================================================
// Edge case tests
// =========================================================================

TEST_F(AstraFeatureListTest, EdgeCase_InvalidFeatureName) {
  // Looking up a non-existent feature name shouldn't crash.
  // GetAllAstraFeatures returns all features; there's no "lookup by name"
  // function that would fail.  But we can verify that all returned features
  // have valid names.
  auto features = GetAllAstraFeatures();
  for (const auto& f : features) {
    EXPECT_FALSE(f.name.empty());
  }
}

TEST_F(AstraFeatureListTest, EdgeCase_EmptyDescriptionNotFound) {
  // No feature should have an empty description.
  auto features = GetAllAstraFeatures();
  for (const auto& f : features) {
    EXPECT_FALSE(f.description.empty())
        << "Feature " << f.name << " has empty description";
  }
}

TEST_F(AstraFeatureListTest, EdgeCase_GetAllFeaturesIsConsistent) {
  // Calling GetAllAstraFeatures multiple times should return the same result.
  auto features1 = GetAllAstraFeatures();
  auto features2 = GetAllAstraFeatures();

  ASSERT_EQ(features1.size(), features2.size());
  for (size_t i = 0; i < features1.size(); ++i) {
    EXPECT_EQ(features1[i].name, features2[i].name);
    EXPECT_EQ(features1[i].enabled, features2[i].enabled);
    EXPECT_EQ(features1[i].description, features2[i].description);
  }
}

TEST_F(AstraFeatureListTest, EdgeCase_EnabledCountIsConsistent) {
  int count1 = GetEnabledAstraFeatureCount();
  int count2 = GetEnabledAstraFeatureCount();
  EXPECT_EQ(count1, count2);
}

TEST_F(AstraFeatureListTest, EdgeCase_AllFeaturesAreAccountedFor) {
  // Every feature in kAllAstraFeatures should appear in GetAllAstraFeatures.
  auto features = GetAllAstraFeatures();

  for (const auto* feature : kAllAstraFeatures) {
    bool found = false;
    for (const auto& info : features) {
      if (info.name == feature->name) {
        found = true;
        break;
      }
    }
    EXPECT_TRUE(found) << "Feature " << feature->name
                       << " not found in GetAllAstraFeatures()";
  }
}

TEST_F(AstraFeatureListTest, EdgeCase_NoExtraFeatures) {
  // GetAllAstraFeatures should not return any features not in our known list.
  auto features = GetAllAstraFeatures();

  for (const auto& info : features) {
    bool found = false;
    for (const auto* feature : kAllAstraFeatures) {
      if (info.name == feature->name) {
        found = true;
        break;
      }
    }
    EXPECT_TRUE(found) << "Unexpected feature in GetAllAstraFeatures(): "
                       << info.name;
  }
}

TEST_F(AstraFeatureListTest, EdgeCase_FeaturePointersAreValid) {
  // All feature pointers should be non-null.
  for (const auto* feature : kAllAstraFeatures) {
    EXPECT_NE(feature, nullptr);
    EXPECT_FALSE(std::string(feature->name).empty());
  }
}

TEST_F(AstraFeatureListTest, EdgeCase_DefaultEnabledAndDisabledAreDisjoint) {
  // The default-enabled and default-disabled lists should have no overlap.
  std::set<const base::Feature*> enabled_set(
      kDefaultEnabledFeatures,
      kDefaultEnabledFeatures + std::size(kDefaultEnabledFeatures));
  std::set<const base::Feature*> disabled_set(
      kDefaultDisabledFeatures,
      kDefaultDisabledFeatures + std::size(kDefaultDisabledFeatures));

  for (const auto* f : enabled_set) {
    EXPECT_EQ(disabled_set.count(f), 0u)
        << "Feature " << f->name
        << " appears in both enabled and disabled lists";
  }
}

TEST_F(AstraFeatureListTest,
       EdgeCase_DefaultEnabledPlusDisabledEqualsTotal) {
  // All features should be in either enabled or disabled list.
  size_t total = std::size(kDefaultEnabledFeatures) +
                 std::size(kDefaultDisabledFeatures);
  EXPECT_EQ(total, kTotalAstraFeatures)
      << "Default-enabled + default-disabled count (" << total
      << ") does not equal total feature count (" << kTotalAstraFeatures
      << ")";
}

// =========================================================================
// AstraFeatureInfo struct tests
// =========================================================================

TEST_F(AstraFeatureListTest, FeatureInfo_DefaultConstruction) {
  // AstraFeatureInfo should be default-constructible with sane defaults.
  AstraFeatureInfo info;
  EXPECT_TRUE(info.name.empty());
  EXPECT_FALSE(info.enabled);
  EXPECT_TRUE(info.description.empty());
}

TEST_F(AstraFeatureListTest, FeatureInfo_Copyable) {
  AstraFeatureInfo info1;
  info1.name = "TestFeature";
  info1.enabled = true;
  info1.description = "Test description";

  AstraFeatureInfo info2 = info1;
  EXPECT_EQ(info2.name, "TestFeature");
  EXPECT_TRUE(info2.enabled);
  EXPECT_EQ(info2.description, "Test description");
}

// =========================================================================
// InitializeAstraFeaturesFromPrefs tests
// =========================================================================

TEST_F(AstraFeatureListTest, InitializeFromPrefs_NullPrefsDoesNotCrash) {
  // Calling with null prefs should not crash.
  InitializeAstraFeaturesFromPrefs(nullptr);
  SUCCEED();
}

TEST_F(AstraFeatureListTest, InitializeFromPrefs_PreservesFeatureStates) {
  // InitializeFromPrefs with no overrides should not change feature states.
  bool sidebar_before = IsAstraFeatureEnabled(kAstraSidebar);
  bool command_before = IsAstraFeatureEnabled(kAstraCommandPalette);

  InitializeAstraFeaturesFromPrefs(nullptr);

  EXPECT_EQ(IsAstraFeatureEnabled(kAstraSidebar), sidebar_before);
  EXPECT_EQ(IsAstraFeatureEnabled(kAstraCommandPalette), command_before);
}

TEST_F(AstraFeatureListTest, InitializeFromPrefs_DoesNotChangeEnabledCount) {
  int before = GetEnabledAstraFeatureCount();
  InitializeAstraFeaturesFromPrefs(nullptr);
  int after = GetEnabledAstraFeatureCount();
  EXPECT_EQ(before, after);
}

TEST_F(AstraFeatureListTest, InitializeFromPrefs_MultipleCallsAreSafe) {
  // Calling InitializeAstraFeaturesFromPrefs multiple times should be safe.
  InitializeAstraFeaturesFromPrefs(nullptr);
  InitializeAstraFeaturesFromPrefs(nullptr);
  InitializeAstraFeaturesFromPrefs(nullptr);
  SUCCEED();
}

// =========================================================================
// Per-feature presence tests
// =========================================================================

TEST_F(AstraFeatureListTest, Feature_Workspaces_IsPresent) {
  auto features = GetAllAstraFeatures();
  bool found = false;
  for (const auto& f : features) {
    if (f.name == "AstraWorkspaces") {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(AstraFeatureListTest, Feature_SplitView_IsPresent) {
  auto features = GetAllAstraFeatures();
  bool found = false;
  for (const auto& f : features) {
    if (f.name == "AstraSplitView") {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(AstraFeatureListTest, Feature_CommandPalette_IsPresent) {
  auto features = GetAllAstraFeatures();
  bool found = false;
  for (const auto& f : features) {
    if (f.name == "AstraCommandPalette") {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(AstraFeatureListTest, Feature_Favorites_IsPresent) {
  auto features = GetAllAstraFeatures();
  bool found = false;
  for (const auto& f : features) {
    if (f.name == "AstraFavorites") {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(AstraFeatureListTest, Feature_TabSearch_IsPresent) {
  auto features = GetAllAstraFeatures();
  bool found = false;
  for (const auto& f : features) {
    if (f.name == "AstraTabSearch") {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(AstraFeatureListTest, Feature_Glance_IsPresent) {
  auto features = GetAllAstraFeatures();
  bool found = false;
  for (const auto& f : features) {
    if (f.name == "AstraGlance") {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(AstraFeatureListTest, Feature_FocusMode_IsPresent) {
  auto features = GetAllAstraFeatures();
  bool found = false;
  for (const auto& f : features) {
    if (f.name == "AstraFocusMode") {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(AstraFeatureListTest, Feature_ReadingList_IsPresent) {
  auto features = GetAllAstraFeatures();
  bool found = false;
  for (const auto& f : features) {
    if (f.name == "AstraReadingList") {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(AstraFeatureListTest, Feature_Notes_IsPresent) {
  auto features = GetAllAstraFeatures();
  bool found = false;
  for (const auto& f : features) {
    if (f.name == "AstraNotes") {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(AstraFeatureListTest, Feature_MemorySaver_IsPresent) {
  auto features = GetAllAstraFeatures();
  bool found = false;
  for (const auto& f : features) {
    if (f.name == "AstraMemorySaver") {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(AstraFeatureListTest, Feature_Screenshot_IsPresent) {
  auto features = GetAllAstraFeatures();
  bool found = false;
  for (const auto& f : features) {
    if (f.name == "AstraScreenshot") {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(AstraFeatureListTest, Feature_Pip_IsPresent) {
  auto features = GetAllAstraFeatures();
  bool found = false;
  for (const auto& f : features) {
    if (f.name == "AstraPip") {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(AstraFeatureListTest, Feature_DevTools_IsPresent) {
  auto features = GetAllAstraFeatures();
  bool found = false;
  for (const auto& f : features) {
    if (f.name == "AstraDevTools") {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(AstraFeatureListTest, Feature_Accessibility_IsPresent) {
  auto features = GetAllAstraFeatures();
  bool found = false;
  for (const auto& f : features) {
    if (f.name == "AstraAccessibility") {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

// =========================================================================
// Feature description detail tests
// =========================================================================

TEST_F(AstraFeatureListTest, WorkspacesDescriptionMentionsTabs) {
  auto features = GetAllAstraFeatures();
  for (const auto& f : features) {
    if (f.name == "AstraWorkspaces") {
      EXPECT_NE(f.description.find("tab"), std::string::npos);
      return;
    }
  }
  FAIL() << "AstraWorkspaces not found";
}

TEST_F(AstraFeatureListTest, FocusModeDescriptionMentionsFocus) {
  auto features = GetAllAstraFeatures();
  for (const auto& f : features) {
    if (f.name == "AstraFocusMode") {
      EXPECT_NE(f.description.find("focus"), std::string::npos);
      return;
    }
  }
  FAIL() << "AstraFocusMode not found";
}

TEST_F(AstraFeatureListTest, NotesDescriptionMentionsNotes) {
  auto features = GetAllAstraFeatures();
  for (const auto& f : features) {
    if (f.name == "AstraNotes") {
      EXPECT_NE(f.description.find("note"), std::string::npos);
      return;
    }
  }
  FAIL() << "AstraNotes not found";
}

TEST_F(AstraFeatureListTest, ScreenshotDescriptionMentionsCapture) {
  auto features = GetAllAstraFeatures();
  for (const auto& f : features) {
    if (f.name == "AstraScreenshot") {
      EXPECT_NE(f.description.find("capture"), std::string::npos)
          << "Screenshot description: " << f.description;
      return;
    }
  }
  FAIL() << "AstraScreenshot not found";
}

TEST_F(AstraFeatureListTest, MemorySaverDescriptionMentionsMemory) {
  auto features = GetAllAstraFeatures();
  for (const auto& f : features) {
    if (f.name == "AstraMemorySaver") {
      EXPECT_NE(f.description.find("memory"), std::string::npos);
      return;
    }
  }
  FAIL() << "AstraMemorySaver not found";
}

TEST_F(AstraFeatureListTest, TabSearchDescriptionMentionsSearch) {
  auto features = GetAllAstraFeatures();
  for (const auto& f : features) {
    if (f.name == "AstraTabSearch") {
      EXPECT_NE(f.description.find("search"), std::string::npos);
      return;
    }
  }
  FAIL() << "AstraTabSearch not found";
}

// =========================================================================
// Additional IsAstraFeatureEnabled tests
// =========================================================================

TEST_F(AstraFeatureListTest,
       IsAstraFeatureEnabled_SplitViewDefaultEnabledWithBranding) {
  // Split view should be enabled by default with branding gate.
  EXPECT_TRUE(IsAstraFeatureEnabled(kAstraSplitView));
}

TEST_F(AstraFeatureListTest,
       IsAstraFeatureEnabled_WorkspacesDefaultEnabledWithBranding) {
  EXPECT_TRUE(IsAstraFeatureEnabled(kAstraWorkspaces));
}

TEST_F(AstraFeatureListTest,
       IsAstraFeatureEnabled_FavoritesDefaultDisabled) {
  EXPECT_FALSE(IsAstraFeatureEnabled(kAstraFavorites));
}

TEST_F(AstraFeatureListTest,
       IsAstraFeatureEnabled_GlanceDefaultDisabled) {
  EXPECT_FALSE(IsAstraFeatureEnabled(kAstraGlance));
}

TEST_F(AstraFeatureListTest,
       IsAstraFeatureEnabled_NotesDefaultDisabled) {
  EXPECT_FALSE(IsAstraFeatureEnabled(kAstraNotes));
}

TEST_F(AstraFeatureListTest,
       IsAstraFeatureEnabled_ScreenshotDefaultDisabled) {
  EXPECT_FALSE(IsAstraFeatureEnabled(kAstraScreenshot));
}

TEST_F(AstraFeatureListTest,
       IsAstraFeatureEnabled_AccessibilityDefaultDisabled) {
  EXPECT_FALSE(IsAstraFeatureEnabled(kAstraAccessibility));
}

// =========================================================================
// Additional edge case tests
// =========================================================================

TEST_F(AstraFeatureListTest, EdgeCase_GetAllFeaturesDoesNotChangeState) {
  // Calling GetAllAstraFeatures should not modify any feature state.
  int count_before = GetEnabledAstraFeatureCount();
  auto features = GetAllAstraFeatures();
  int count_after = GetEnabledAstraFeatureCount();

  EXPECT_EQ(count_before, count_after);
  EXPECT_EQ(features.size(), kTotalAstraFeatures);
}

TEST_F(AstraFeatureListTest, EdgeCase_AllFeaturesHaveUniqueNames) {
  auto features = GetAllAstraFeatures();
  std::set<std::string> names;
  for (const auto& f : features) {
    names.insert(f.name);
  }
  EXPECT_EQ(names.size(), features.size());
}

TEST_F(AstraFeatureListTest, EdgeCase_EnabledFeaturesAreSubsetOfAll) {
  // The set of enabled features should be a subset of all features.
  auto all = GetAllAstraFeatures();
  int enabled_count = GetEnabledAstraFeatureCount();
  EXPECT_LE(enabled_count, static_cast<int>(all.size()));
}

TEST_F(AstraFeatureListTest,
       EdgeCase_BrandedBuildFeatureHasNonTrivialDescription) {
  // The branded build feature description should be meaningful.
  auto features = GetAllAstraFeatures();
  for (const auto& f : features) {
    if (f.name == "AstraBrandedBuild") {
      EXPECT_GT(f.description.length(), 20u);
      return;
    }
  }
  FAIL() << "AstraBrandedBuild not found";
}

TEST_F(AstraFeatureListTest, EdgeCase_NoFeatureHasEmptyDescription) {
  auto features = GetAllAstraFeatures();
  for (const auto& f : features) {
    EXPECT_FALSE(f.description.empty())
        << "Feature " << f.name << " has empty description";
  }
}

TEST_F(AstraFeatureListTest, EdgeCase_FeatureNameLengthsAreReasonable) {
  // Feature names should be between, say, 10 and 50 characters.
  auto features = GetAllAstraFeatures();
  for (const auto& f : features) {
    EXPECT_GT(f.name.length(), 8u)
        << "Feature name too short: " << f.name;
    EXPECT_LT(f.name.length(), 50u)
        << "Feature name too long: " << f.name;
  }
}

}  // namespace astra
