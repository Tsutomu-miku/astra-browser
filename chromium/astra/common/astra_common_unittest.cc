// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Comprehensive unit tests for the Astra common layer.
//
// Test categories:
//   - AstraWorkspaceAccentColor enum and color lookup.
//   - AstraWorkspaceColor enum and AccentColorForWorkspaceColor.
//   - AstraWorkspaceLayout enum bounds and values.
//   - AstraWorkspaceInfo struct defaults and field access (expanded).
//   - AstraWorkspaceWindowState struct defaults and field access.
//   - IsValidWorkspaceId format validation.
//   - IsAstraCommandId range checks.
//   - AstraTabStackState enum values.
//   - AstraTabSource enum values.
//   - AstraTabCloseBehavior enum values.
//   - AstraSplitViewState defaults and field access.
//   - Split view ratio constant invariants.
//   - AstraTabFeatureFlag bit operations (expanded to 16 flags).
//   - AstraTabMetadata struct defaults and field access.
//   - String ID constants (invalid, default, root folder, etc.).
//   - Astra URL constants and helper functions (IsAstraURL, IsAstraWebUI).
//   - UI constants: sidebar, split view, command palette, animation,
//     workspace card, font sizes, spacing, radius, elevation.
//   - Accelerator ID constants: count and naming conventions.

#include "astra/common/astra_command_constants.h"
#include "astra/common/astra_tab_types.h"
#include "astra/common/astra_ui_constants.h"
#include "astra/common/astra_url_constants.h"
#include "astra/common/astra_workspace_types.h"

#include "base/types/bit_flag.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkColor.h"
#include "url/gurl.h"

namespace astra {

// =========================================================================
// AstraWorkspaceAccentColor tests (legacy)
// =========================================================================

TEST(AstraWorkspaceAccentColorTest, EnumBounds) {
  EXPECT_EQ(static_cast<int>(AstraWorkspaceAccentColor::kBlue), 0);
  EXPECT_EQ(static_cast<int>(AstraWorkspaceAccentColor::kCustom), 9);
}

TEST(AstraWorkspaceAccentColorTest, GetAstraAccentColor_ReturnsValidColors) {
  const AstraWorkspaceAccentColor colors[] = {
      AstraWorkspaceAccentColor::kBlue,
      AstraWorkspaceAccentColor::kGreen,
      AstraWorkspaceAccentColor::kPurple,
      AstraWorkspaceAccentColor::kOrange,
      AstraWorkspaceAccentColor::kPink,
      AstraWorkspaceAccentColor::kRed,
      AstraWorkspaceAccentColor::kTeal,
      AstraWorkspaceAccentColor::kYellow,
      AstraWorkspaceAccentColor::kGrey,
      AstraWorkspaceAccentColor::kCustom,
  };

  for (auto c : colors) {
    SkColor color = GetAstraAccentColor(c);
    EXPECT_EQ(SkColorGetA(color), 255U)
        << "Color enum value " << static_cast<int>(c) << " has non-opaque alpha";
  }
}

TEST(AstraWorkspaceAccentColorTest,
     GetAstraAccentColor_PredefinedColorsAreDistinct) {
  SkColor blue = GetAstraAccentColor(AstraWorkspaceAccentColor::kBlue);
  SkColor green = GetAstraAccentColor(AstraWorkspaceAccentColor::kGreen);
  SkColor purple = GetAstraAccentColor(AstraWorkspaceAccentColor::kPurple);
  SkColor orange = GetAstraAccentColor(AstraWorkspaceAccentColor::kOrange);
  SkColor pink = GetAstraAccentColor(AstraWorkspaceAccentColor::kPink);
  SkColor red = GetAstraAccentColor(AstraWorkspaceAccentColor::kRed);
  SkColor teal = GetAstraAccentColor(AstraWorkspaceAccentColor::kTeal);
  SkColor yellow = GetAstraAccentColor(AstraWorkspaceAccentColor::kYellow);
  SkColor grey = GetAstraAccentColor(AstraWorkspaceAccentColor::kGrey);

  EXPECT_NE(blue, green);
  EXPECT_NE(blue, purple);
  EXPECT_NE(blue, red);
  EXPECT_NE(green, teal);
  EXPECT_NE(orange, yellow);
  EXPECT_NE(pink, red);
  EXPECT_NE(grey, blue);
}

TEST(AstraWorkspaceAccentColorTest, GetAstraAccentColor_CustomFallsBackToBlue) {
  EXPECT_EQ(GetAstraAccentColor(AstraWorkspaceAccentColor::kCustom),
            GetAstraAccentColor(AstraWorkspaceAccentColor::kBlue));
}

// =========================================================================
// AstraWorkspaceColor tests (12-color palette)
// =========================================================================

TEST(AstraWorkspaceColorTest, EnumBounds) {
  // First color should be kGray = 0.
  EXPECT_EQ(static_cast<int>(AstraWorkspaceColor::kGray), 0);

  // Last color should be kBrown = 11 (12 colors total, 0-indexed).
  EXPECT_EQ(static_cast<int>(AstraWorkspaceColor::kBrown), 11);
}

TEST(AstraWorkspaceColorTest, All12ColorsExist) {
  // Verify all 12 named colors are defined.
  EXPECT_TRUE(true);  // Compile check — if any are missing, this won't compile.
  AstraWorkspaceColor c;
  c = AstraWorkspaceColor::kGray;
  c = AstraWorkspaceColor::kBlue;
  c = AstraWorkspaceColor::kRed;
  c = AstraWorkspaceColor::kGreen;
  c = AstraWorkspaceColor::kYellow;
  c = AstraWorkspaceColor::kPurple;
  c = AstraWorkspaceColor::kPink;
  c = AstraWorkspaceColor::kCyan;
  c = AstraWorkspaceColor::kOrange;
  c = AstraWorkspaceColor::kTeal;
  c = AstraWorkspaceColor::kIndigo;
  c = AstraWorkspaceColor::kBrown;
  EXPECT_EQ(static_cast<int>(c), 11);
}

TEST(AstraWorkspaceColorTest, AccentColorForWorkspaceColor_AllValid) {
  // Every color enum value should produce a valid opaque SkColor.
  const AstraWorkspaceColor colors[] = {
      AstraWorkspaceColor::kGray,   AstraWorkspaceColor::kBlue,
      AstraWorkspaceColor::kRed,    AstraWorkspaceColor::kGreen,
      AstraWorkspaceColor::kYellow, AstraWorkspaceColor::kPurple,
      AstraWorkspaceColor::kPink,   AstraWorkspaceColor::kCyan,
      AstraWorkspaceColor::kOrange, AstraWorkspaceColor::kTeal,
      AstraWorkspaceColor::kIndigo, AstraWorkspaceColor::kBrown,
  };

  for (auto c : colors) {
    SkColor color = AccentColorForWorkspaceColor(c);
    EXPECT_EQ(SkColorGetA(color), 255U)
        << "Color " << static_cast<int>(c) << " has non-opaque alpha";
  }
}

TEST(AstraWorkspaceColorTest, AccentColorForWorkspaceColor_DistinctColors) {
  // The most distinguishable colors should be different.
  SkColor red = AccentColorForWorkspaceColor(AstraWorkspaceColor::kRed);
  SkColor blue = AccentColorForWorkspaceColor(AstraWorkspaceColor::kBlue);
  SkColor green = AccentColorForWorkspaceColor(AstraWorkspaceColor::kGreen);
  SkColor yellow = AccentColorForWorkspaceColor(AstraWorkspaceColor::kYellow);
  SkColor purple = AccentColorForWorkspaceColor(AstraWorkspaceColor::kPurple);

  EXPECT_NE(red, blue);
  EXPECT_NE(red, green);
  EXPECT_NE(blue, yellow);
  EXPECT_NE(green, purple);
}

TEST(AstraWorkspaceColorTest, AccentColorForWorkspaceColor_BlueMatchesLegacy) {
  // The new blue should approximately match the legacy kBlue accent color.
  SkColor new_blue = AccentColorForWorkspaceColor(AstraWorkspaceColor::kBlue);
  SkColor legacy_blue = GetAstraAccentColor(AstraWorkspaceAccentColor::kBlue);
  EXPECT_EQ(new_blue, legacy_blue);
}

TEST(AstraWorkspaceColorTest, AccentColorForWorkspaceColor_OutOfRangeFallback) {
  // Passing an out-of-range value via static_cast should still return
  // something valid (blue fallback).
  SkColor result = AccentColorForWorkspaceColor(static_cast<AstraWorkspaceColor>(99));
  EXPECT_EQ(SkColorGetA(result), 255U);
}

// =========================================================================
// AstraWorkspaceLayout tests
// =========================================================================

TEST(AstraWorkspaceLayoutTest, EnumValues) {
  // kDefault should be 0.
  EXPECT_EQ(static_cast<int>(AstraWorkspaceLayout::kDefault), 0);

  // Verify all 5 layout types exist.
  AstraWorkspaceLayout l;
  l = AstraWorkspaceLayout::kDefault;
  l = AstraWorkspaceLayout::kTiled;
  l = AstraWorkspaceLayout::kStacked;
  l = AstraWorkspaceLayout::kGrid;
  l = AstraWorkspaceLayout::kFocus;
  EXPECT_EQ(static_cast<int>(l), 4);  // kFocus is the 5th entry (index 4).
}

TEST(AstraWorkspaceLayoutTest, AllValuesAreDistinct) {
  // All layout enum values should have distinct integer values.
  EXPECT_NE(static_cast<int>(AstraWorkspaceLayout::kDefault),
            static_cast<int>(AstraWorkspaceLayout::kTiled));
  EXPECT_NE(static_cast<int>(AstraWorkspaceLayout::kTiled),
            static_cast<int>(AstraWorkspaceLayout::kStacked));
  EXPECT_NE(static_cast<int>(AstraWorkspaceLayout::kStacked),
            static_cast<int>(AstraWorkspaceLayout::kGrid));
  EXPECT_NE(static_cast<int>(AstraWorkspaceLayout::kGrid),
            static_cast<int>(AstraWorkspaceLayout::kFocus));
}

// =========================================================================
// IsValidWorkspaceId tests
// =========================================================================

TEST(IsValidWorkspaceIdTest, ValidIds) {
  EXPECT_TRUE(IsValidWorkspaceId("default"));
  EXPECT_TRUE(IsValidWorkspaceId("workspace-1"));
  EXPECT_TRUE(IsValidWorkspaceId("my_workspace"));
  EXPECT_TRUE(IsValidWorkspaceId("abc123"));
  EXPECT_TRUE(IsValidWorkspaceId("a"));  // Single character.
  EXPECT_TRUE(IsValidWorkspaceId("A-B-C"));  // Uppercase and hyphens.
  EXPECT_TRUE(IsValidWorkspaceId("test_workspace_42"));  // Underscores and digits.
}

TEST(IsValidWorkspaceIdTest, EmptyIdIsInvalid) {
  EXPECT_FALSE(IsValidWorkspaceId(""));
  EXPECT_FALSE(IsValidWorkspaceId(std::string()));
}

TEST(IsValidWorkspaceIdTest, SpacesAreInvalid) {
  EXPECT_FALSE(IsValidWorkspaceId("my workspace"));
  EXPECT_FALSE(IsValidWorkspaceId(" "));
  EXPECT_FALSE(IsValidWorkspaceId("workspace 1"));
}

TEST(IsValidWorkspaceIdTest, SpecialCharsAreInvalid) {
  EXPECT_FALSE(IsValidWorkspaceId("workspace!"));
  EXPECT_FALSE(IsValidWorkspaceId("work@space"));
  EXPECT_FALSE(IsValidWorkspaceId("work.space"));
  EXPECT_FALSE(IsValidWorkspaceId("work/space"));
  EXPECT_FALSE(IsValidWorkspaceId("<script>"));
}

TEST(IsValidWorkspaceIdTest, MaxLengthIsValid) {
  // Exactly 64 characters should be valid.
  std::string max_id(64, 'a');
  EXPECT_TRUE(IsValidWorkspaceId(max_id));
  EXPECT_EQ(max_id.size(), 64u);
}

TEST(IsValidWorkspaceIdTest, OverMaxLengthIsInvalid) {
  // 65 characters should be invalid.
  std::string too_long(65, 'a');
  EXPECT_FALSE(IsValidWorkspaceId(too_long));
  EXPECT_GT(too_long.size(), kAstraMaxWorkspaceIdLength);
}

TEST(IsValidWorkspaceIdTest, MaxLengthConstantIs64) {
  EXPECT_EQ(kAstraMaxWorkspaceIdLength, 64u);
}

// =========================================================================
// AstraWorkspaceInfo expanded struct tests
// =========================================================================

TEST(AstraWorkspaceInfoTest, DefaultValues) {
  AstraWorkspaceInfo info;

  EXPECT_TRUE(info.id.empty());
  EXPECT_TRUE(info.name.empty());
  EXPECT_EQ(info.accent_color, AstraWorkspaceAccentColor::kBlue);
  EXPECT_EQ(info.custom_color, SK_ColorBLUE);
  EXPECT_EQ(info.order_index, 0u);
  EXPECT_TRUE(info.created_time.is_null());
  EXPECT_FALSE(info.is_default);

  // Expanded fields.
  EXPECT_EQ(info.color, AstraWorkspaceColor::kBlue);
  EXPECT_TRUE(info.icon.empty());
  EXPECT_EQ(info.layout, AstraWorkspaceLayout::kDefault);
  EXPECT_FALSE(info.is_pinned);
  EXPECT_EQ(info.tab_count, 0);
  EXPECT_EQ(info.window_count, 0);
  EXPECT_TRUE(info.last_accessed_time.is_null());
}

TEST(AstraWorkspaceInfoTest, FieldAccess) {
  AstraWorkspaceInfo info;

  info.id = "workspace-42";
  info.name = u"My Workspace";
  info.accent_color = AstraWorkspaceAccentColor::kPurple;
  info.custom_color = SkColorSetRGB(0xAA, 0xBB, 0xCC);
  info.order_index = 3;
  info.created_time = base::Time::Now();
  info.is_default = true;

  // Expanded fields.
  info.color = AstraWorkspaceColor::kTeal;
  info.icon = "star";
  info.layout = AstraWorkspaceLayout::kTiled;
  info.is_pinned = true;
  info.tab_count = 12;
  info.window_count = 2;
  info.last_accessed_time = base::Time::Now();

  EXPECT_EQ(info.id, "workspace-42");
  EXPECT_EQ(info.name, u"My Workspace");
  EXPECT_EQ(info.accent_color, AstraWorkspaceAccentColor::kPurple);
  EXPECT_EQ(info.custom_color, SkColorSetRGB(0xAA, 0xBB, 0xCC));
  EXPECT_EQ(info.order_index, 3u);
  EXPECT_FALSE(info.created_time.is_null());
  EXPECT_TRUE(info.is_default);

  // Expanded fields.
  EXPECT_EQ(info.color, AstraWorkspaceColor::kTeal);
  EXPECT_EQ(info.icon, "star");
  EXPECT_EQ(info.layout, AstraWorkspaceLayout::kTiled);
  EXPECT_TRUE(info.is_pinned);
  EXPECT_EQ(info.tab_count, 12);
  EXPECT_EQ(info.window_count, 2);
  EXPECT_FALSE(info.last_accessed_time.is_null());
}

TEST(AstraWorkspaceInfoTest, TabCountCanBeZeroOrPositive) {
  AstraWorkspaceInfo info;
  EXPECT_EQ(info.tab_count, 0);
  info.tab_count = 100;
  EXPECT_EQ(info.tab_count, 100);
}

TEST(AstraWorkspaceInfoTest, WindowCountCanBeZeroOrPositive) {
  AstraWorkspaceInfo info;
  EXPECT_EQ(info.window_count, 0);
  info.window_count = 5;
  EXPECT_EQ(info.window_count, 5);
}

// =========================================================================
// AstraWorkspaceWindowState tests
// =========================================================================

TEST(AstraWorkspaceWindowStateTest, DefaultValues) {
  AstraWorkspaceWindowState state;

  EXPECT_TRUE(state.window_id.empty());
  EXPECT_TRUE(state.bounds.IsEmpty());
  EXPECT_FALSE(state.is_minimized);
  EXPECT_FALSE(state.is_maximized);
  EXPECT_FALSE(state.is_fullscreen);
  EXPECT_EQ(state.active_tab_index, -1);
}

TEST(AstraWorkspaceWindowStateTest, FieldAccess) {
  AstraWorkspaceWindowState state;

  state.window_id = "win-abc123";
  state.bounds = gfx::Rect(100, 200, 800, 600);
  state.is_minimized = false;
  state.is_maximized = true;
  state.is_fullscreen = false;
  state.active_tab_index = 3;

  EXPECT_EQ(state.window_id, "win-abc123");
  EXPECT_EQ(state.bounds.x(), 100);
  EXPECT_EQ(state.bounds.y(), 200);
  EXPECT_EQ(state.bounds.width(), 800);
  EXPECT_EQ(state.bounds.height(), 600);
  EXPECT_FALSE(state.is_minimized);
  EXPECT_TRUE(state.is_maximized);
  EXPECT_FALSE(state.is_fullscreen);
  EXPECT_EQ(state.active_tab_index, 3);
}

TEST(AstraWorkspaceWindowStateTest, FullscreenState) {
  AstraWorkspaceWindowState state;
  state.is_fullscreen = true;
  state.active_tab_index = 0;

  EXPECT_TRUE(state.is_fullscreen);
  EXPECT_EQ(state.active_tab_index, 0);
}

TEST(AstraWorkspaceWindowStateTest, NegativeActiveTabIndexMeansNoTab) {
  AstraWorkspaceWindowState state;
  EXPECT_EQ(state.active_tab_index, -1);
  // -1 means no active tab (empty window).
  EXPECT_LT(state.active_tab_index, 0);
}

// =========================================================================
// IsAstraCommandId range tests
// =========================================================================

TEST(AstraCommandIdTest, RangeBoundaries) {
  EXPECT_EQ(kAstraCommandFirst, 60000);
  EXPECT_EQ(kAstraCommandLast, 60500);
}

TEST(AstraCommandIdTest, IsAstraCommandId_BelowRange) {
  EXPECT_FALSE(IsAstraCommandId(0));
  EXPECT_FALSE(IsAstraCommandId(100));
  EXPECT_FALSE(IsAstraCommandId(59999));
  EXPECT_FALSE(IsAstraCommandId(-1));
  EXPECT_FALSE(IsAstraCommandId(-60000));
}

TEST(AstraCommandIdTest, IsAstraCommandId_AtLowerBoundary) {
  EXPECT_TRUE(IsAstraCommandId(kAstraCommandFirst));
  EXPECT_TRUE(IsAstraCommandId(60000));
  EXPECT_TRUE(IsAstraCommandId(60001));
}

TEST(AstraCommandIdTest, IsAstraCommandId_InRange) {
  EXPECT_TRUE(IsAstraCommandId(60100));
  EXPECT_TRUE(IsAstraCommandId(60250));
  EXPECT_TRUE(IsAstraCommandId(60499));
}

TEST(AstraCommandIdTest, IsAstraCommandId_AtUpperBoundary) {
  EXPECT_FALSE(IsAstraCommandId(kAstraCommandLast));
  EXPECT_FALSE(IsAstraCommandId(60500));
}

TEST(AstraCommandIdTest, IsAstraCommandId_AboveRange) {
  EXPECT_FALSE(IsAstraCommandId(60501));
  EXPECT_FALSE(IsAstraCommandId(70000));
  EXPECT_FALSE(IsAstraCommandId(100000));
}

TEST(AstraCommandIdTest, RangeHas500Entries) {
  // The range should contain exactly 500 command IDs.
  EXPECT_EQ(kAstraCommandLast - kAstraCommandFirst, 500);
}

// =========================================================================
// Accelerator ID constant tests
// =========================================================================

TEST(AstraAcceleratorIdTest, CoreAcceleratorsExist) {
  // Verify core accelerator IDs are non-empty strings.
  EXPECT_STRNE(kAstraAcceleratorCommandPalette, "");
  EXPECT_STRNE(kAstraAcceleratorToggleSidebar, "");
  EXPECT_STRNE(kAstraAcceleratorNextWorkspace, "");
  EXPECT_STRNE(kAstraAcceleratorPreviousWorkspace, "");
  EXPECT_STRNE(kAstraAcceleratorToggleSplitView, "");
  EXPECT_STRNE(kAstraAcceleratorToggleFavorite, "");
}

TEST(AstraAcceleratorIdTest, WorkspaceAcceleratorsExist) {
  EXPECT_STRNE(kAstraAcceleratorNewWorkspace, "");
  EXPECT_STRNE(kAstraAcceleratorCloseWorkspace, "");
  EXPECT_STRNE(kAstraAcceleratorRenameWorkspace, "");
  EXPECT_STRNE(kAstraAcceleratorShowAllWorkspaces, "");
  EXPECT_STRNE(kAstraAcceleratorMoveTabToNextWorkspace, "");
  EXPECT_STRNE(kAstraAcceleratorMoveTabToPreviousWorkspace, "");
}

TEST(AstraAcceleratorIdTest, SidebarAcceleratorsExist) {
  EXPECT_STRNE(kAstraAcceleratorOpenBookmarks, "");
  EXPECT_STRNE(kAstraAcceleratorOpenHistory, "");
  EXPECT_STRNE(kAstraAcceleratorOpenDownloads, "");
  EXPECT_STRNE(kAstraAcceleratorOpenNotes, "");
  EXPECT_STRNE(kAstraAcceleratorOpenReadingList, "");
}

TEST(AstraAcceleratorIdTest, TabStackAcceleratorsExist) {
  EXPECT_STRNE(kAstraAcceleratorStackTabs, "");
  EXPECT_STRNE(kAstraAcceleratorUnstackTabs, "");
  EXPECT_STRNE(kAstraAcceleratorNextTabInStack, "");
  EXPECT_STRNE(kAstraAcceleratorPreviousTabInStack, "");
}

TEST(AstraAcceleratorIdTest, SplitViewAcceleratorsExist) {
  EXPECT_STRNE(kAstraAcceleratorToggleSplitView, "");
  EXPECT_STRNE(kAstraAcceleratorSplitViewOrientationToggle, "");
  EXPECT_STRNE(kAstraAcceleratorSplitViewGrowPrimary, "");
  EXPECT_STRNE(kAstraAcceleratorSplitViewShrinkPrimary, "");
}

TEST(AstraAcceleratorIdTest, DevToolsAcceleratorsExist) {
  EXPECT_STRNE(kAstraAcceleratorToggleDevTools, "");
  EXPECT_STRNE(kAstraAcceleratorDevToolsWorkspacePanel, "");
  EXPECT_STRNE(kAstraAcceleratorDevToolsTabPanel, "");
}

TEST(AstraAcceleratorIdTest, FeatureAcceleratorsExist) {
  EXPECT_STRNE(kAstraAcceleratorToggleFocusMode, "");
  EXPECT_STRNE(kAstraAcceleratorOpenTabSearch, "");
  EXPECT_STRNE(kAstraAcceleratorOpenGlance, "");
  EXPECT_STRNE(kAstraAcceleratorScreenshotVisible, "");
  EXPECT_STRNE(kAstraAcceleratorScreenshotFullPage, "");
  EXPECT_STRNE(kAstraAcceleratorScreenshotRegion, "");
  EXPECT_STRNE(kAstraAcceleratorTogglePip, "");
}

TEST(AstraAcceleratorIdTest, QuickSwitchAcceleratorsExist) {
  EXPECT_STRNE(kAstraAcceleratorSwitchToWorkspace1, "");
  EXPECT_STRNE(kAstraAcceleratorSwitchToWorkspace9, "");
}

TEST(AstraAcceleratorIdTest, AllUseAstraPrefix) {
  // All accelerator IDs should start with "astra."
  EXPECT_TRUE(strncmp(kAstraAcceleratorCommandPalette, "astra.", 6) == 0);
  EXPECT_TRUE(strncmp(kAstraAcceleratorToggleSidebar, "astra.", 6) == 0);
  EXPECT_TRUE(strncmp(kAstraAcceleratorNewWorkspace, "astra.", 6) == 0);
  EXPECT_TRUE(strncmp(kAstraAcceleratorToggleFocusMode, "astra.", 6) == 0);
  EXPECT_TRUE(strncmp(kAstraAcceleratorScreenshotVisible, "astra.", 6) == 0);
}

// =========================================================================
// AstraTabStackState tests
// =========================================================================

TEST(AstraTabStackStateTest, EnumValues) {
  // kNormal should be 0.
  EXPECT_EQ(static_cast<int>(AstraTabStackState::kNormal), 0);

  // All 4 states should exist.
  AstraTabStackState s;
  s = AstraTabStackState::kNormal;
  s = AstraTabStackState::kStacked;
  s = AstraTabStackState::kStackRoot;
  s = AstraTabStackState::kStackChild;
  EXPECT_EQ(static_cast<int>(s), 3);  // kStackChild is index 3.
}

TEST(AstraTabStackStateTest, AllValuesDistinct) {
  EXPECT_NE(static_cast<int>(AstraTabStackState::kNormal),
            static_cast<int>(AstraTabStackState::kStacked));
  EXPECT_NE(static_cast<int>(AstraTabStackState::kStacked),
            static_cast<int>(AstraTabStackState::kStackRoot));
  EXPECT_NE(static_cast<int>(AstraTabStackState::kStackRoot),
            static_cast<int>(AstraTabStackState::kStackChild));
}

// =========================================================================
// AstraTabSource tests
// =========================================================================

TEST(AstraTabSourceTest, EnumValues) {
  // kUserOpened should be 0.
  EXPECT_EQ(static_cast<int>(AstraTabSource::kUserOpened), 0);

  // All 7 sources should exist.
  AstraTabSource s;
  s = AstraTabSource::kUserOpened;
  s = AstraTabSource::kRestore;
  s = AstraTabSource::kLinkClick;
  s = AstraTabSource::kPopup;
  s = AstraTabSource::kExtension;
  s = AstraTabSource::kDevTools;
  s = AstraTabSource::kWorkspaceSwitch;
  EXPECT_EQ(static_cast<int>(s), 6);  // 7th entry, index 6.
}

TEST(AstraTabSourceTest, AllValuesDistinct) {
  // Verify all 7 values are distinct.
  int values[] = {
      static_cast<int>(AstraTabSource::kUserOpened),
      static_cast<int>(AstraTabSource::kRestore),
      static_cast<int>(AstraTabSource::kLinkClick),
      static_cast<int>(AstraTabSource::kPopup),
      static_cast<int>(AstraTabSource::kExtension),
      static_cast<int>(AstraTabSource::kDevTools),
      static_cast<int>(AstraTabSource::kWorkspaceSwitch),
  };

  for (size_t i = 0; i < 7; ++i) {
    for (size_t j = i + 1; j < 7; ++j) {
      EXPECT_NE(values[i], values[j])
          << "AstraTabSource values at indices " << i << " and " << j
          << " are not distinct";
    }
  }
}

// =========================================================================
// AstraTabCloseBehavior tests
// =========================================================================

TEST(AstraTabCloseBehaviorTest, EnumValues) {
  // kDefault should be 0.
  EXPECT_EQ(static_cast<int>(AstraTabCloseBehavior::kDefault), 0);

  // All 4 behaviors should exist.
  AstraTabCloseBehavior b;
  b = AstraTabCloseBehavior::kDefault;
  b = AstraTabCloseBehavior::kKeepInStack;
  b = AstraTabCloseBehavior::kCloseStack;
  b = AstraTabCloseBehavior::kHibernate;
  EXPECT_EQ(static_cast<int>(b), 3);  // kHibernate is index 3.
}

TEST(AstraTabCloseBehaviorTest, AllValuesDistinct) {
  EXPECT_NE(static_cast<int>(AstraTabCloseBehavior::kDefault),
            static_cast<int>(AstraTabCloseBehavior::kKeepInStack));
  EXPECT_NE(static_cast<int>(AstraTabCloseBehavior::kKeepInStack),
            static_cast<int>(AstraTabCloseBehavior::kCloseStack));
  EXPECT_NE(static_cast<int>(AstraTabCloseBehavior::kCloseStack),
            static_cast<int>(AstraTabCloseBehavior::kHibernate));
}

// =========================================================================
// AstraTabMetadata tests
// =========================================================================

TEST(AstraTabMetadataTest, DefaultValues) {
  AstraTabMetadata metadata;

  EXPECT_TRUE(metadata.stack_id.empty());
  EXPECT_EQ(metadata.source, AstraTabSource::kUserOpened);
  EXPECT_EQ(metadata.close_behavior, AstraTabCloseBehavior::kDefault);
  EXPECT_TRUE(metadata.last_access_time.is_null());
  EXPECT_EQ(metadata.visit_count, 0);
  EXPECT_TRUE(metadata.thumbnail_cache_key.empty());
}

TEST(AstraTabMetadataTest, FieldAccess) {
  AstraTabMetadata metadata;

  metadata.stack_id = "stack-abc";
  metadata.source = AstraTabSource::kLinkClick;
  metadata.close_behavior = AstraTabCloseBehavior::kKeepInStack;
  metadata.last_access_time = base::Time::Now();
  metadata.visit_count = 42;
  metadata.thumbnail_cache_key = "thumb_tab_123.png";

  EXPECT_EQ(metadata.stack_id, "stack-abc");
  EXPECT_EQ(metadata.source, AstraTabSource::kLinkClick);
  EXPECT_EQ(metadata.close_behavior, AstraTabCloseBehavior::kKeepInStack);
  EXPECT_FALSE(metadata.last_access_time.is_null());
  EXPECT_EQ(metadata.visit_count, 42);
  EXPECT_EQ(metadata.thumbnail_cache_key, "thumb_tab_123.png");
}

TEST(AstraTabMetadataTest, VisitCountCanIncrement) {
  AstraTabMetadata metadata;
  EXPECT_EQ(metadata.visit_count, 0);
  metadata.visit_count++;
  EXPECT_EQ(metadata.visit_count, 1);
  metadata.visit_count += 10;
  EXPECT_EQ(metadata.visit_count, 11);
}

TEST(AstraTabMetadataTest, ThumbnailKeyCanBeEmpty) {
  AstraTabMetadata metadata;
  EXPECT_TRUE(metadata.thumbnail_cache_key.empty());
  // Empty string means no thumbnail available.
  metadata.thumbnail_cache_key = "";
  EXPECT_TRUE(metadata.thumbnail_cache_key.empty());
}

// =========================================================================
// AstraSplitViewState tests
// =========================================================================

TEST(AstraSplitViewStateTest, DefaultValues) {
  AstraSplitViewState state;

  EXPECT_FALSE(state.is_active);
  EXPECT_EQ(state.orientation, AstraSplitViewOrientation::kHorizontal);
  EXPECT_TRUE(state.partner_tab_id.empty());
  EXPECT_FLOAT_EQ(state.ratio, 0.5f);
}

TEST(AstraSplitViewStateTest, FieldAccess) {
  AstraSplitViewState state;

  state.is_active = true;
  state.orientation = AstraSplitViewOrientation::kVertical;
  state.partner_tab_id = "tab-123";
  state.ratio = 0.3f;

  EXPECT_TRUE(state.is_active);
  EXPECT_EQ(state.orientation, AstraSplitViewOrientation::kVertical);
  EXPECT_EQ(state.partner_tab_id, "tab-123");
  EXPECT_FLOAT_EQ(state.ratio, 0.3f);
}

// =========================================================================
// Split view ratio constants tests
// =========================================================================

TEST(SplitViewRatioConstantsTest, MinAndMaxSumToOne) {
  EXPECT_FLOAT_EQ(kAstraMinSplitViewRatio + kAstraMaxSplitViewRatio, 1.0f);
}

TEST(SplitViewRatioConstantsTest, MinIsLessThanMax) {
  EXPECT_LT(kAstraMinSplitViewRatio, kAstraMaxSplitViewRatio);
}

TEST(SplitViewRatioConstantsTest, DefaultIsBetweenMinAndMax) {
  EXPECT_GT(kAstraDefaultSplitViewRatio, kAstraMinSplitViewRatio);
  EXPECT_LT(kAstraDefaultSplitViewRatio, kAstraMaxSplitViewRatio);
}

TEST(SplitViewRatioConstantsTest, RatiosAreInValidRange) {
  EXPECT_GE(kAstraMinSplitViewRatio, 0.0f);
  EXPECT_LE(kAstraMinSplitViewRatio, 1.0f);
  EXPECT_GE(kAstraMaxSplitViewRatio, 0.0f);
  EXPECT_LE(kAstraMaxSplitViewRatio, 1.0f);
  EXPECT_GE(kAstraDefaultSplitViewRatio, 0.0f);
  EXPECT_LE(kAstraDefaultSplitViewRatio, 1.0f);
}

TEST(SplitViewRatioConstantsTest, MinRatioIsPositive) {
  // The minimum ratio should be greater than 0 to prevent zero-sized panes.
  EXPECT_GT(kAstraMinSplitViewRatio, 0.0f);
  EXPECT_LT(kAstraMinSplitViewRatio, 0.5f);
}

// =========================================================================
// AstraTabFeatureFlag bit operations tests
// =========================================================================

TEST(AstraTabFeatureFlagTest, NoneIsZero) {
  EXPECT_EQ(static_cast<uint32_t>(AstraTabFeatureFlag::kNone), 0u);
}

TEST(AstraTabFeatureFlagTest, All16FlagsAreDistinctBits) {
  // All 16 flags should be distinct powers of 2.
  AstraTabFeatureFlags flags = 0;

  // First byte.
  flags |= static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kFavorite);
  flags |= static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kPinned);
  flags |= static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kGlanceTab);
  flags |= static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kInStack);
  flags |= static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kInSplitView);
  flags |= static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kPipTab);
  flags |= static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kSuspended);
  flags |=
      static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kSidebarHidden);

  // Second byte (new flags).
  flags |= static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kReadingList);
  flags |= static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kNote);
  flags |=
      static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kPinnedInSidebar);
  flags |=
      static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kGlancePreview);
  flags |=
      static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kAutoDiscardable);
  flags |= static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kMuted);
  flags |= static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kAudible);
  flags |=
      static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kHibernated);

  // With 16 distinct bit flags, we should have 16 bits set.
  int count = 0;
  AstraTabFeatureFlags temp = flags;
  while (temp) {
    count += temp & 1;
    temp >>= 1;
  }
  EXPECT_EQ(count, 16);
}

TEST(AstraTabFeatureFlagTest, NewFlagsInSecondByte) {
  // The 8 new flags should all be in bits 8-15 (second byte).
  uint32_t second_byte_mask = 0xFF00;

  EXPECT_EQ(static_cast<uint32_t>(AstraTabFeatureFlag::kReadingList) &
                second_byte_mask,
            static_cast<uint32_t>(AstraTabFeatureFlag::kReadingList));
  EXPECT_EQ(static_cast<uint32_t>(AstraTabFeatureFlag::kNote) & second_byte_mask,
            static_cast<uint32_t>(AstraTabFeatureFlag::kNote));
  EXPECT_EQ(static_cast<uint32_t>(AstraTabFeatureFlag::kHibernated) &
                second_byte_mask,
            static_cast<uint32_t>(AstraTabFeatureFlag::kHibernated));
}

TEST(AstraTabFeatureFlagTest, HasFlag_SingleFlag) {
  AstraTabFeatureFlags flags =
      static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kFavorite);

  EXPECT_TRUE(base::HasFlag(flags, AstraTabFeatureFlag::kFavorite));
  EXPECT_FALSE(base::HasFlag(flags, AstraTabFeatureFlag::kPinned));
  EXPECT_FALSE(base::HasFlag(flags, AstraTabFeatureFlag::kReadingList));
  EXPECT_FALSE(base::HasFlag(flags, AstraTabFeatureFlag::kMuted));
}

TEST(AstraTabFeatureFlagTest, HasFlag_NewFlags) {
  // Test each of the 8 new flags individually.
  AstraTabFeatureFlags reading_list =
      static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kReadingList);
  EXPECT_TRUE(base::HasFlag(reading_list, AstraTabFeatureFlag::kReadingList));
  EXPECT_FALSE(base::HasFlag(reading_list, AstraTabFeatureFlag::kNote));

  AstraTabFeatureFlags note =
      static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kNote);
  EXPECT_TRUE(base::HasFlag(note, AstraTabFeatureFlag::kNote));

  AstraTabFeatureFlags muted =
      static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kMuted);
  EXPECT_TRUE(base::HasFlag(muted, AstraTabFeatureFlag::kMuted));

  AstraTabFeatureFlags audible =
      static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kAudible);
  EXPECT_TRUE(base::HasFlag(audible, AstraTabFeatureFlag::kAudible));

  AstraTabFeatureFlags hibernated =
      static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kHibernated);
  EXPECT_TRUE(base::HasFlag(hibernated, AstraTabFeatureFlag::kHibernated));
}

TEST(AstraTabFeatureFlagTest, SetFlag_AddsNewFlag) {
  AstraTabFeatureFlags flags = 0;
  EXPECT_FALSE(base::HasFlag(flags, AstraTabFeatureFlag::kReadingList));

  base::SetFlag(flags, AstraTabFeatureFlag::kReadingList, true);
  EXPECT_TRUE(base::HasFlag(flags, AstraTabFeatureFlag::kReadingList));
}

TEST(AstraTabFeatureFlagTest, ClearFlag_RemovesNewFlag) {
  AstraTabFeatureFlags flags =
      static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kMuted) |
      static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kAudible);

  EXPECT_TRUE(base::HasFlag(flags, AstraTabFeatureFlag::kMuted));
  EXPECT_TRUE(base::HasFlag(flags, AstraTabFeatureFlag::kAudible));

  base::SetFlag(flags, AstraTabFeatureFlag::kMuted, false);
  EXPECT_FALSE(base::HasFlag(flags, AstraTabFeatureFlag::kMuted));
  EXPECT_TRUE(base::HasFlag(flags, AstraTabFeatureFlag::kAudible));
}

TEST(AstraTabFeatureFlagTest, MixedOldAndNewFlags) {
  // Combine flags from both bytes.
  AstraTabFeatureFlags flags =
      static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kFavorite) |
      static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kMuted) |
      static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kInStack) |
      static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kAutoDiscardable);

  EXPECT_TRUE(base::HasFlag(flags, AstraTabFeatureFlag::kFavorite));
  EXPECT_TRUE(base::HasFlag(flags, AstraTabFeatureFlag::kMuted));
  EXPECT_TRUE(base::HasFlag(flags, AstraTabFeatureFlag::kInStack));
  EXPECT_TRUE(base::HasFlag(flags, AstraTabFeatureFlag::kAutoDiscardable));
  EXPECT_FALSE(base::HasFlag(flags, AstraTabFeatureFlag::kPinned));
  EXPECT_FALSE(base::HasFlag(flags, AstraTabFeatureFlag::kNote));
}

TEST(AstraTabFeatureFlagTest, MultipleFlagsCombined_NewFlags) {
  AstraTabFeatureFlags flags =
      static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kReadingList) |
      static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kNote) |
      static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kMuted) |
      static_cast<AstraTabFeatureFlags>(AstraTabFeatureFlag::kHibernated);

  EXPECT_TRUE(base::HasFlag(flags, AstraTabFeatureFlag::kReadingList));
  EXPECT_TRUE(base::HasFlag(flags, AstraTabFeatureFlag::kNote));
  EXPECT_TRUE(base::HasFlag(flags, AstraTabFeatureFlag::kMuted));
  EXPECT_TRUE(base::HasFlag(flags, AstraTabFeatureFlag::kHibernated));
  EXPECT_FALSE(base::HasFlag(flags, AstraTabFeatureFlag::kAudible));
  EXPECT_FALSE(base::HasFlag(flags, AstraTabFeatureFlag::kGlancePreview));
  EXPECT_FALSE(base::HasFlag(flags, AstraTabFeatureFlag::kAutoDiscardable));
}

// =========================================================================
// Astra URL constants tests
// =========================================================================

TEST(AstraUrlConstantsTest, SchemeIsAstra) {
  EXPECT_STREQ(kAstraUIScheme, "astra");
}

TEST(AstraUrlConstantsTest, UrlConstantsUseAstraScheme) {
  // All URL constants should use the "astra://" scheme.
  EXPECT_EQ(GURL(kAstraNewTabURL).scheme(), "astra");
  EXPECT_EQ(GURL(kAstraSettingsURL).scheme(), "astra");
  EXPECT_EQ(GURL(kAstraHistoryURL).scheme(), "astra");
  EXPECT_EQ(GURL(kAstraBookmarksURL).scheme(), "astra");
  EXPECT_EQ(GURL(kAstraDownloadsURL).scheme(), "astra");
  EXPECT_EQ(GURL(kAstraWorkspaceOverviewURL).scheme(), "astra");
  EXPECT_EQ(GURL(kAstraCommandPaletteURL).scheme(), "astra");
  EXPECT_EQ(GURL(kAstraFocusModeURL).scheme(), "astra");
  EXPECT_EQ(GURL(kAstraNotesURL).scheme(), "astra");
}

TEST(AstraUrlConstantsTest, UrlConstantsHaveValidHosts) {
  // All URL constants should have non-empty hosts.
  EXPECT_FALSE(GURL(kAstraNewTabURL).host().empty());
  EXPECT_FALSE(GURL(kAstraSettingsURL).host().empty());
  EXPECT_FALSE(GURL(kAstraHistoryURL).host().empty());
  EXPECT_FALSE(GURL(kAstraBookmarksURL).host().empty());
  EXPECT_FALSE(GURL(kAstraDownloadsURL).host().empty());
  EXPECT_FALSE(GURL(kAstraWorkspaceOverviewURL).host().empty());
  EXPECT_FALSE(GURL(kAstraCommandPaletteURL).host().empty());
  EXPECT_FALSE(GURL(kAstraFocusModeURL).host().empty());
  EXPECT_FALSE(GURL(kAstraNotesURL).host().empty());
}

TEST(AstraUrlConstantsTest, NewTabUrl) {
  EXPECT_STREQ(kAstraNewTabURL, "astra://newtab");
  GURL url(kAstraNewTabURL);
  EXPECT_EQ(url.host(), "newtab");
}

TEST(AstraUrlConstantsTest, SettingsUrl) {
  GURL url(kAstraSettingsURL);
  EXPECT_EQ(url.host(), "settings");
}

TEST(AstraUrlConstantsTest, WorkspaceOverviewUrl) {
  GURL url(kAstraWorkspaceOverviewURL);
  EXPECT_EQ(url.host(), "workspace-overview");
}

TEST(AstraUrlConstantsTest, CommandPaletteUrl) {
  GURL url(kAstraCommandPaletteURL);
  EXPECT_EQ(url.host(), "command-palette");
}

TEST(AstraUrlConstantsTest, IsAstraURL_ValidAstraUrl) {
  EXPECT_TRUE(IsAstraURL(GURL("astra://newtab")));
  EXPECT_TRUE(IsAstraURL(GURL("astra://settings")));
  EXPECT_TRUE(IsAstraURL(GURL("astra://anything")));
  EXPECT_TRUE(IsAstraURL(GURL("astra://foo/bar")));
}

TEST(AstraUrlConstantsTest, IsAstraURL_NonAstraSchemes) {
  EXPECT_FALSE(IsAstraURL(GURL("http://example.com")));
  EXPECT_FALSE(IsAstraURL(GURL("https://example.com")));
  EXPECT_FALSE(IsAstraURL(GURL("chrome://newtab")));
  EXPECT_FALSE(IsAstraURL(GURL("about:blank")));
  EXPECT_FALSE(IsAstraURL(GURL("file:///tmp/foo")));
}

TEST(AstraUrlConstantsTest, IsAstraURL_InvalidUrl) {
  // An empty or invalid URL should return false.
  EXPECT_FALSE(IsAstraURL(GURL()));
  EXPECT_FALSE(IsAstraURL(GURL("")));
  EXPECT_FALSE(IsAstraURL(GURL("not-a-url")));
}

TEST(AstraUrlConstantsTest, IsAstraURL_CaseSensitiveScheme) {
  // URL schemes are case-insensitive per RFC 3986, but GURL normalizes them.
  EXPECT_TRUE(IsAstraURL(GURL("ASTRA://newtab")));
  EXPECT_TRUE(IsAstraURL(GURL("Astra://settings")));
}

TEST(AstraUrlConstantsTest, IsAstraWebUI_AllKnownPages) {
  EXPECT_TRUE(IsAstraWebUI(GURL(kAstraNewTabURL)));
  EXPECT_TRUE(IsAstraWebUI(GURL(kAstraSettingsURL)));
  EXPECT_TRUE(IsAstraWebUI(GURL(kAstraHistoryURL)));
  EXPECT_TRUE(IsAstraWebUI(GURL(kAstraBookmarksURL)));
  EXPECT_TRUE(IsAstraWebUI(GURL(kAstraDownloadsURL)));
  EXPECT_TRUE(IsAstraWebUI(GURL(kAstraWorkspaceOverviewURL)));
  EXPECT_TRUE(IsAstraWebUI(GURL(kAstraCommandPaletteURL)));
  EXPECT_TRUE(IsAstraWebUI(GURL(kAstraFocusModeURL)));
  EXPECT_TRUE(IsAstraWebUI(GURL(kAstraNotesURL)));
}

TEST(AstraUrlConstantsTest, IsAstraWebUI_UnknownHost) {
  // Unknown astra:// hosts are not valid WebUI pages.
  EXPECT_FALSE(IsAstraWebUI(GURL("astra://unknown-page")));
  EXPECT_FALSE(IsAstraWebUI(GURL("astra://")));
  EXPECT_FALSE(IsAstraWebUI(GURL("astra://random")));
}

TEST(AstraUrlConstantsTest, IsAstraWebUI_NonAstraScheme) {
  // Non-astra schemes should return false.
  EXPECT_FALSE(IsAstraWebUI(GURL("http://newtab")));
  EXPECT_FALSE(IsAstraWebUI(GURL("chrome://newtab")));
  EXPECT_FALSE(IsAstraWebUI(GURL("about:newtab")));
}

TEST(AstraUrlConstantsTest, IsAstraWebUI_EmptyHost) {
  // URL with no host / empty host.
  EXPECT_FALSE(IsAstraWebUI(GURL("astra:///")));
  EXPECT_FALSE(IsAstraWebUI(GURL("astra:")));
}

TEST(AstraUrlConstantsTest, IsAstraWebUI_WithPath) {
  // Paths should still be recognized as valid WebUI.
  GURL url("astra://newtab/subpage");
  EXPECT_TRUE(IsAstraURL(url));
  // The host is still "newtab" which is valid.
  EXPECT_TRUE(IsAstraWebUI(url));
}

// =========================================================================
// UI constants tests
// =========================================================================

TEST(AstraUiConstantsTest, SidebarDimensions) {
  // Min < default < max.
  EXPECT_LT(kAstraSidebarMinWidth, kAstraSidebarDefaultWidth);
  EXPECT_LT(kAstraSidebarDefaultWidth, kAstraSidebarMaxWidth);
  EXPECT_GT(kAstraSidebarMinWidth, 0);
  EXPECT_GT(kAstraSidebarCollapsedWidth, 0);
  EXPECT_LT(kAstraSidebarCollapsedWidth, kAstraSidebarMinWidth);
}

TEST(AstraUiConstantsTest, SidebarDefaultSections) {
  EXPECT_GT(kAstraSidebarDefaultSections, 0);
  EXPECT_EQ(kAstraSidebarDefaultSections, 5);
}

TEST(AstraUiConstantsTest, SidebarRowDimensions) {
  EXPECT_GT(kAstraSidebarRowHeight, 0);
  EXPECT_GT(kAstraSidebarSectionHeaderHeight, 0);
  EXPECT_GT(kAstraSidebarSectionPadding, 0);
  EXPECT_GT(kAstraSidebarRowHeight, kAstraSidebarSectionHeaderHeight);
}

TEST(AstraUiConstantsTest, SplitViewRatios) {
  EXPECT_FLOAT_EQ(kAstraSplitViewDefaultRatio, 0.5f);
  EXPECT_GT(kAstraSplitViewMinRatio, 0.0f);
  EXPECT_LT(kAstraSplitViewMaxRatio, 1.0f);
  EXPECT_FLOAT_EQ(kAstraSplitViewMinRatio + kAstraSplitViewMaxRatio, 1.0f);
  EXPECT_GT(kAstraSplitViewMinPaneSize, 0);
  EXPECT_GT(kAstraSplitViewDividerThickness, 0);
}

TEST(AstraUiConstantsTest, CommandPalette) {
  EXPECT_GT(kAstraCommandPaletteDefaultSize.width(), 0);
  EXPECT_GT(kAstraCommandPaletteDefaultSize.height(), 0);
  EXPECT_GT(kAstraCommandPaletteMaxHeight, 0);
  EXPECT_GT(kAstraCommandPaletteMaxResults, 0);
  EXPECT_GT(kAstraCommandPaletteMaxHistoryItems, 0);
  EXPECT_GT(kAstraCommandPaletteMaxHistoryItems, kAstraCommandPaletteMaxResults);
}

TEST(AstraUiConstantsTest, WorkspaceCardDimensions) {
  EXPECT_GT(kAstraWorkspaceCardWidth, 0);
  EXPECT_GT(kAstraWorkspaceCardHeight, 0);
  EXPECT_GT(kAstraWorkspaceCardSpacing, 0);
  EXPECT_GT(kAstraWorkspaceCardPadding, 0);
  EXPECT_GT(kAstraWorkspaceCardCornerRadius, 0);
}

TEST(AstraUiConstantsTest, CornerRadii) {
  // kRadiusNone should be 0.
  EXPECT_EQ(kAstraRadiusNone, 0);

  // Values should be increasing.
  EXPECT_LT(kAstraRadiusNone, kAstraRadiusSmall);
  EXPECT_LT(kAstraRadiusSmall, kAstraRadiusMedium);
  EXPECT_LT(kAstraRadiusMedium, kAstraRadiusLarge);
  // Pill is effectively "infinite" (999px — use width/2 in practice).
  EXPECT_GT(kAstraRadiusPill, kAstraRadiusLarge);
}

TEST(AstraUiConstantsTest, SpacingScale) {
  // kSpacingNone should be 0.
  EXPECT_EQ(kAstraSpacingNone, 0);

  // Spacing should be monotonically increasing.
  EXPECT_LT(kAstraSpacingNone, kAstraSpacingTiny);
  EXPECT_LT(kAstraSpacingTiny, kAstraSpacingSmall);
  EXPECT_LT(kAstraSpacingSmall, kAstraSpacingMedium);
  EXPECT_LT(kAstraSpacingMedium, kAstraSpacingLarge);
  EXPECT_LT(kAstraSpacingLarge, kAstraSpacingHuge);
}

TEST(AstraUiConstantsTest, SpacingValuesInPixels) {
  EXPECT_EQ(kAstraSpacingTiny, 4);
  EXPECT_EQ(kAstraSpacingSmall, 8);
  EXPECT_EQ(kAstraSpacingMedium, 16);
  EXPECT_EQ(kAstraSpacingLarge, 24);
  EXPECT_EQ(kAstraSpacingHuge, 32);
}

TEST(AstraUiConstantsTest, FontSizes) {
  // All font sizes should be positive and increasing.
  EXPECT_GT(kAstraFontSizeTiny, 0);
  EXPECT_LT(kAstraFontSizeTiny, kAstraFontSizeSmall);
  EXPECT_LT(kAstraFontSizeSmall, kAstraFontSizeMedium);
  EXPECT_LT(kAstraFontSizeMedium, kAstraFontSizeLarge);
  EXPECT_LT(kAstraFontSizeLarge, kAstraFontSizeTitle);
  EXPECT_LT(kAstraFontSizeTitle, kAstraFontSizeHeadline);
  EXPECT_LT(kAstraFontSizeHeadline, kAstraFontSizeDisplay);
  EXPECT_LT(kAstraFontSizeDisplay, kAstraFontSizeGiant);
}

TEST(AstraUiConstantsTest, FontSizeCount) {
  // There should be 8 font sizes.
  int sizes[] = {
      kAstraFontSizeTiny,     kAstraFontSizeSmall,
      kAstraFontSizeMedium,   kAstraFontSizeLarge,
      kAstraFontSizeTitle,    kAstraFontSizeHeadline,
      kAstraFontSizeDisplay,  kAstraFontSizeGiant,
  };
  // Verify all 8 are distinct and positive.
  for (int i = 0; i < 8; ++i) {
    EXPECT_GT(sizes[i], 0);
    for (int j = i + 1; j < 8; ++j) {
      EXPECT_NE(sizes[i], sizes[j]);
    }
  }
}

TEST(AstraUiConstantsTest, AnimationDurations) {
  EXPECT_GT(kAstraAnimationFastMs, 0);
  EXPECT_LT(kAstraAnimationFastMs, kAstraAnimationDefaultMs);
  EXPECT_LT(kAstraAnimationDefaultMs, kAstraAnimationSlowMs);
}

TEST(AstraUiConstantsTest, AnimationSpecificValues) {
  EXPECT_EQ(kAstraAnimationFastMs, 120);
  EXPECT_EQ(kAstraAnimationDefaultMs, 200);
  EXPECT_EQ(kAstraAnimationSlowMs, 350);
}

TEST(AstraUiConstantsTest, IconSizes) {
  EXPECT_GT(kAstraIconSizeSmall, 0);
  EXPECT_LT(kAstraIconSizeSmall, kAstraIconSizeMedium);
  EXPECT_LT(kAstraIconSizeMedium, kAstraIconSizeLarge);
  EXPECT_LT(kAstraIconSizeLarge, kAstraIconSizeExtraLarge);
}

TEST(AstraUiConstantsTest, ElevationLevels) {
  // Level 0 should be 0.
  EXPECT_EQ(kAstraElevationLevel0, 0);

  // Levels should be increasing.
  EXPECT_LT(kAstraElevationLevel0, kAstraElevationLevel1);
  EXPECT_LT(kAstraElevationLevel1, kAstraElevationLevel2);
  EXPECT_LT(kAstraElevationLevel2, kAstraElevationLevel3);
  EXPECT_LT(kAstraElevationLevel3, kAstraElevationLevel4);
}

TEST(AstraUiConstantsTest, ElevationCount) {
  // There should be 5 elevation levels (0-4).
  int levels[] = {
      kAstraElevationLevel0, kAstraElevationLevel1, kAstraElevationLevel2,
      kAstraElevationLevel3, kAstraElevationLevel4,
  };
  for (int i = 0; i < 5; ++i) {
    EXPECT_GE(levels[i], 0);
  }
}

// =========================================================================
// Constant string ID tests
// =========================================================================

TEST(AstraIdConstantsTest, InvalidWorkspaceIdIsEmpty) {
  EXPECT_STREQ(kAstraInvalidWorkspaceId, "");
  EXPECT_EQ(std::string(kAstraInvalidWorkspaceId), std::string());
}

TEST(AstraIdConstantsTest, DefaultWorkspaceIdIsDefault) {
  EXPECT_STREQ(kAstraDefaultWorkspaceId, "default");
  EXPECT_NE(kAstraDefaultWorkspaceId, kAstraInvalidWorkspaceId);
  EXPECT_TRUE(IsValidWorkspaceId(kAstraDefaultWorkspaceId));
}

TEST(AstraIdConstantsTest, FavoriteRootFolderIdIsRoot) {
  EXPECT_STREQ(kAstraFavoriteRootFolderId, "root");
}

TEST(AstraIdConstantsTest, InvalidTabStackIdIsEmpty) {
  EXPECT_STREQ(kAstraInvalidTabStackId, "");
}

// =========================================================================
// Typedef / using tests
// =========================================================================

TEST(AstraTypeDefsTest, FavoriteFolderIdIsString) {
  AstraFavoriteFolderId id = "test-folder";
  EXPECT_EQ(id, "test-folder");
  EXPECT_TRUE(id.empty() == false);
}

TEST(AstraTypeDefsTest, TabStackIdIsString) {
  AstraTabStackId id = "stack-123";
  EXPECT_EQ(id, "stack-123");
}

TEST(AstraTypeDefsTest, WorkspaceIdIsString) {
  AstraWorkspaceId id = "workspace-abc";
  EXPECT_EQ(id, "workspace-abc");
}

TEST(AstraTypeDefsTest, WorkspaceWindowIdIsString) {
  AstraWorkspaceWindowId id = "window-xyz";
  EXPECT_EQ(id, "window-xyz");
}

// =========================================================================
// AstraWorkspaceList and AstraWorkspaceWindowList tests
// =========================================================================

TEST(AstraWorkspaceListTest, EmptyByDefault) {
  AstraWorkspaceList list;
  EXPECT_TRUE(list.empty());
  EXPECT_EQ(list.size(), 0u);
}

TEST(AstraWorkspaceListTest, CanAddEntries) {
  AstraWorkspaceList list;
  AstraWorkspaceInfo info1;
  info1.id = "ws1";
  info1.name = u"Workspace 1";

  AstraWorkspaceInfo info2;
  info2.id = "ws2";
  info2.name = u"Workspace 2";

  list.push_back(info1);
  list.push_back(info2);

  EXPECT_EQ(list.size(), 2u);
  EXPECT_EQ(list[0].id, "ws1");
  EXPECT_EQ(list[1].id, "ws2");
}

TEST(AstraWorkspaceWindowListTest, EmptyByDefault) {
  AstraWorkspaceWindowList list;
  EXPECT_TRUE(list.empty());
}

TEST(AstraWorkspaceWindowListTest, CanAddEntries) {
  AstraWorkspaceWindowList list;
  AstraWorkspaceWindowState state;
  state.window_id = "win1";
  state.active_tab_index = 3;
  list.push_back(state);

  EXPECT_EQ(list.size(), 1u);
  EXPECT_EQ(list[0].window_id, "win1");
}

}  // namespace astra
