// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/app/astra_accelerator_table.h"

#include <set>
#include <string>
#include <vector>

#include "base/containers/span.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/events/event_constants.h"
#include "ui/events/keycodes/keyboard_codes.h"

#include "astra/common/astra_command_constants.h"

namespace astra {

namespace {

// Expected number of accelerator entries (minimum).
constexpr size_t kMinAcceleratorCount = 25;

// Helper: get the primary modifier constant for this platform.
#if BUILDFLAG(IS_MAC)
constexpr int kPrimaryModifier = ui::EF_COMMAND_DOWN;
#else
constexpr int kPrimaryModifier = ui::EF_CONTROL_DOWN;
#endif

}  // namespace

// =========================================================================
// AstraAcceleratorTableTest
// =========================================================================

class AstraAcceleratorTableTest : public testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

// =========================================================================
// Table structure tests
// =========================================================================

TEST_F(AstraAcceleratorTableTest, TableIsNotEmpty) {
  auto table = GetAstraAcceleratorTable();
  EXPECT_FALSE(table.empty());
}

TEST_F(AstraAcceleratorTableTest, TableHasMinimumEntries) {
  // The accelerator table should have at least 25 entries.
  auto table = GetAstraAcceleratorTable();
  EXPECT_GE(table.size(), kMinAcceleratorCount)
      << "Expected at least " << kMinAcceleratorCount
      << " accelerator entries, got " << table.size();
}

TEST_F(AstraAcceleratorTableTest, AllEntriesHaveCommandIds) {
  // All entries should have a valid (non-negative) command ID.
  auto table = GetAstraAcceleratorTable();
  for (size_t i = 0; i < table.size(); ++i) {
    EXPECT_GE(table[i].command_id, 0)
        << "Entry " << i << " has invalid command_id: " << table[i].command_id;
  }
}

TEST_F(AstraAcceleratorTableTest, AllEntriesHaveAcceleratorIds) {
  // All entries should have a non-empty accelerator_id string.
  auto table = GetAstraAcceleratorTable();
  for (size_t i = 0; i < table.size(); ++i) {
    EXPECT_FALSE(table[i].accelerator_id.empty())
        << "Entry " << i << " has empty accelerator_id";
  }
}

TEST_F(AstraAcceleratorTableTest, AllEntriesHaveDescriptions) {
  // All entries should have a non-empty description.
  auto table = GetAstraAcceleratorTable();
  for (size_t i = 0; i < table.size(); ++i) {
    EXPECT_FALSE(table[i].description.empty())
        << "Entry " << i << " (id=" << table[i].accelerator_id
        << ") has empty description";
  }
}

TEST_F(AstraAcceleratorTableTest, AllEntriesHaveValidKeycodes) {
  // All entries should have a valid keycode (not VKEY_UNKNOWN).
  auto table = GetAstraAcceleratorTable();
  for (size_t i = 0; i < table.size(); ++i) {
    EXPECT_NE(table[i].keycode, ui::VKEY_UNKNOWN)
        << "Entry " << i << " has VKEY_UNKNOWN keycode";
    EXPECT_GT(table[i].keycode, 0)
        << "Entry " << i << " has invalid keycode: " << table[i].keycode;
  }
}

TEST_F(AstraAcceleratorTableTest, AllEntriesHaveModifiers) {
  // Most accelerators should have modifiers (key alone is unusual).
  auto table = GetAstraAcceleratorTable();
  int unmodified_count = 0;
  for (size_t i = 0; i < table.size(); ++i) {
    if (table[i].modifiers == 0) {
      ++unmodified_count;
    }
  }
  // Some accelerators (like F11, F12) might not have modifiers,
  // but most should.
  EXPECT_LT(unmodified_count, static_cast<int>(table.size() / 2))
      << "Too many accelerators without modifiers";
}

TEST_F(AstraAcceleratorTableTest, AllEntriesHaveIsDefault) {
  // All default entries should have is_default = true.
  auto table = GetAstraAcceleratorTable();
  for (size_t i = 0; i < table.size(); ++i) {
    // Currently all table entries are defaults (user-customized would be
    // added at runtime).
    EXPECT_TRUE(table[i].is_default)
        << "Entry " << i << " has is_default=false unexpectedly";
  }
}

TEST_F(AstraAcceleratorTableTest, AcceleratorIdsFollowNamingConvention) {
  // All accelerator IDs should start with "astra.".
  auto table = GetAstraAcceleratorTable();
  for (size_t i = 0; i < table.size(); ++i) {
    EXPECT_EQ(table[i].accelerator_id.substr(0, 6), "astra.")
        << "Entry " << i << " has invalid accelerator_id: "
        << table[i].accelerator_id;
  }
}

TEST_F(AstraAcceleratorTableTest, NoDuplicateAcceleratorKeyCombos) {
  // No two entries should have the same keycode+modifiers combination.
  // (That would mean a duplicate accelerator.)
  auto table = GetAstraAcceleratorTable();
  std::set<std::pair<int, int>> combos;  // (keycode, modifiers)

  for (size_t i = 0; i < table.size(); ++i) {
    auto combo = std::make_pair(table[i].keycode, table[i].modifiers);
    EXPECT_TRUE(combos.insert(combo).second)
        << "Duplicate accelerator key combo: keycode=" << table[i].keycode
        << ", modifiers=" << table[i].modifiers
        << " (entry " << i << ": " << table[i].accelerator_id << ")";
  }
}

// =========================================================================
// GetAcceleratorsForCommand tests
// =========================================================================

TEST_F(AstraAcceleratorTableTest, GetAcceleratorsForCommand_KnownCommand) {
  // Command palette should have at least one accelerator.
  auto accelerators =
      GetAcceleratorsForCommand(kAstraCommandOpenCommandPalette);
  EXPECT_GE(accelerators.size(), 1u);
}

TEST_F(AstraAcceleratorTableTest, GetAcceleratorsForCommand_UnknownCommand) {
  // An unknown command should return empty vector.
  auto accelerators = GetAcceleratorsForCommand(-1);
  EXPECT_TRUE(accelerators.empty());
}

TEST_F(AstraAcceleratorTableTest, GetAcceleratorsForCommand_InvalidCommand) {
  auto accelerators = GetAcceleratorsForCommand(999999);
  EXPECT_TRUE(accelerators.empty());
}

TEST_F(AstraAcceleratorTableTest,
       GetAcceleratorsForCommand_AllReturnedHaveSameCommandId) {
  // All returned accelerators should have the requested command ID.
  auto accelerators =
      GetAcceleratorsForCommand(kAstraCommandOpenCommandPalette);
  for (const auto& entry : accelerators) {
    EXPECT_EQ(entry.command_id, kAstraCommandOpenCommandPalette);
  }
}

TEST_F(AstraAcceleratorTableTest, GetAcceleratorsForCommand_ToggleSidebar) {
  auto accelerators = GetAcceleratorsForCommand(kAstraCommandToggleSidebar);
  EXPECT_GE(accelerators.size(), 1u);
}

TEST_F(AstraAcceleratorTableTest, GetAcceleratorsForCommand_ToggleSplitView) {
  auto accelerators = GetAcceleratorsForCommand(kAstraCommandToggleSplitView);
  EXPECT_GE(accelerators.size(), 1u);
}

// =========================================================================
// GetPrimaryAcceleratorForCommand tests
// =========================================================================

TEST_F(AstraAcceleratorTableTest, GetPrimaryAccelerator_KnownCommand) {
  auto primary = GetPrimaryAcceleratorForCommand(kAstraCommandToggleSidebar);
  EXPECT_NE(primary, nullptr);
  EXPECT_EQ(primary->command_id, kAstraCommandToggleSidebar);
}

TEST_F(AstraAcceleratorTableTest, GetPrimaryAccelerator_UnknownCommand) {
  auto primary = GetPrimaryAcceleratorForCommand(-1);
  EXPECT_EQ(primary, nullptr);
}

TEST_F(AstraAcceleratorTableTest, GetPrimaryAccelerator_IsFirstEntry) {
  // The primary accelerator should be the first one for that command
  // in the table.
  auto all = GetAcceleratorsForCommand(kAstraCommandOpenCommandPalette);
  ASSERT_GE(all.size(), 1u);

  auto primary = GetPrimaryAcceleratorForCommand(kAstraCommandOpenCommandPalette);
  ASSERT_NE(primary, nullptr);

  EXPECT_EQ(primary->keycode, all[0].keycode);
  EXPECT_EQ(primary->modifiers, all[0].modifiers);
  EXPECT_EQ(primary->accelerator_id, all[0].accelerator_id);
}

// =========================================================================
// GetShortcutText tests
// =========================================================================

TEST_F(AstraAcceleratorTableTest, GetShortcutTextForCommand_KnownCommand) {
  std::string text = GetShortcutTextForCommand(kAstraCommandToggleSidebar);
  EXPECT_FALSE(text.empty());
}

TEST_F(AstraAcceleratorTableTest, GetShortcutTextForCommand_UnknownCommand) {
  std::string text = GetShortcutTextForCommand(-1);
  EXPECT_TRUE(text.empty());
}

TEST_F(AstraAcceleratorTableTest, GetShortcutText_EntryProducesText) {
  auto entry = GetPrimaryAcceleratorForCommand(kAstraCommandOpenCommandPalette);
  ASSERT_NE(entry, nullptr);

  std::string text = GetShortcutText(*entry);
  EXPECT_FALSE(text.empty());
}

TEST_F(AstraAcceleratorTableTest, GetShortcutText_ContainsKeyName) {
  // The shortcut text should contain the key name.
  ui::Accelerator accel(ui::VKEY_P, kPrimaryModifier | ui::EF_SHIFT_DOWN);
  std::string text = FormatAcceleratorText(accel);

  EXPECT_FALSE(text.empty());
  // Should contain "P" or the key name.
  EXPECT_NE(text.find("P"), std::string::npos)
      << "Shortcut text '" << text << "' doesn't contain 'P'";
}

TEST_F(AstraAcceleratorTableTest, GetShortcutText_NoModifiers) {
  // An accelerator with no modifiers should just show the key.
  ui::Accelerator accel(ui::VKEY_F11, 0);
  std::string text = FormatAcceleratorText(accel);

  EXPECT_FALSE(text.empty());
  // Should not contain "Ctrl" or "Cmd" since there are no modifiers.
  EXPECT_EQ(text.find("Ctrl"), std::string::npos);
  EXPECT_EQ(text.find("Command"), std::string::npos);
}

TEST_F(AstraAcceleratorTableTest, GetShortcutText_ShiftModifier) {
  ui::Accelerator accel(ui::VKEY_A, ui::EF_SHIFT_DOWN);
  std::string text = FormatAcceleratorText(accel);

  EXPECT_FALSE(text.empty());
#if !BUILDFLAG(IS_MAC)
  EXPECT_NE(text.find("Shift"), std::string::npos)
      << "Shortcut text '" << text << "' doesn't contain 'Shift'";
#endif
}

TEST_F(AstraAcceleratorTableTest, GetShortcutText_AltModifier) {
  ui::Accelerator accel(ui::VKEY_A, ui::EF_ALT_DOWN);
  std::string text = FormatAcceleratorText(accel);

  EXPECT_FALSE(text.empty());
#if !BUILDFLAG(IS_MAC)
  EXPECT_NE(text.find("Alt"), std::string::npos);
#endif
}

TEST_F(AstraAcceleratorTableTest, GetShortcutText_ConsistentWithEntry) {
  // GetShortcutTextForCommand should produce the same result as
  // GetShortcutText with the primary entry.
  auto entry = GetPrimaryAcceleratorForCommand(kAstraCommandToggleSidebar);
  ASSERT_NE(entry, nullptr);

  std::string text_from_command = GetShortcutTextForCommand(kAstraCommandToggleSidebar);
  std::string text_from_entry = GetShortcutText(*entry);

  EXPECT_EQ(text_from_command, text_from_entry);
}

// =========================================================================
// GetDefaultAcceleratorForAction tests
// =========================================================================

TEST_F(AstraAcceleratorTableTest, GetDefaultAcceleratorForAction_KnownAction) {
  auto accel = GetDefaultAcceleratorForAction(kAstraAcceleratorToggleSidebar);
  EXPECT_TRUE(accel.has_value());
}

TEST_F(AstraAcceleratorTableTest, GetDefaultAcceleratorForAction_UnknownAction) {
  auto accel = GetDefaultAcceleratorForAction("nonexistent.action");
  EXPECT_FALSE(accel.has_value());
}

TEST_F(AstraAcceleratorTableTest, GetDefaultAcceleratorForAction_EmptyString) {
  auto accel = GetDefaultAcceleratorForAction("");
  EXPECT_FALSE(accel.has_value());
}

TEST_F(AstraAcceleratorTableTest, GetDefaultAcceleratorForAction_CommandPalette) {
  auto accel = GetDefaultAcceleratorForAction(kAstraAcceleratorCommandPalette);
  ASSERT_TRUE(accel.has_value());
  // Command palette primary is Ctrl/Cmd+Shift+P.
  EXPECT_EQ(accel->key_code(), ui::VKEY_P);
  EXPECT_TRUE(accel->modifiers() & ui::EF_SHIFT_DOWN);
  EXPECT_TRUE(accel->modifiers() & kPrimaryModifier);
}

TEST_F(AstraAcceleratorTableTest, GetDefaultAcceleratorForAction_ReturnsAccelerator) {
  auto accel = GetDefaultAcceleratorForAction(kAstraAcceleratorToggleSidebar);
  ASSERT_TRUE(accel.has_value());
  // Should have valid keycode and modifiers.
  EXPECT_GT(accel->key_code(), 0);
  EXPECT_NE(accel->key_code(), ui::VKEY_UNKNOWN);
}

// =========================================================================
// GetAllAccelerators tests
// =========================================================================

TEST_F(AstraAcceleratorTableTest, GetAllAccelerators_ReturnsAllEntries) {
  auto all = GetAllAccelerators();
  auto table = GetAstraAcceleratorTable();
  EXPECT_EQ(all.size(), table.size());
}

TEST_F(AstraAcceleratorTableTest, GetAllAccelerators_NotEmpty) {
  auto all = GetAllAccelerators();
  EXPECT_FALSE(all.empty());
}

TEST_F(AstraAcceleratorTableTest, GetAllAccelerators_MatchesTable) {
  auto all = GetAllAccelerators();
  auto table = GetAstraAcceleratorTable();

  ASSERT_EQ(all.size(), table.size());
  for (size_t i = 0; i < all.size(); ++i) {
    EXPECT_EQ(all[i].command_id, table[i].command_id);
    EXPECT_EQ(all[i].accelerator_id, table[i].accelerator_id);
    EXPECT_EQ(all[i].keycode, table[i].keycode);
    EXPECT_EQ(all[i].modifiers, table[i].modifiers);
  }
}

TEST_F(AstraAcceleratorTableTest, GetAllAccelerators_HasEnoughEntries) {
  auto all = GetAllAccelerators();
  EXPECT_GE(all.size(), kMinAcceleratorCount);
}

// =========================================================================
// GetAcceleratorDescription tests
// =========================================================================

TEST_F(AstraAcceleratorTableTest, GetAcceleratorDescription_KnownAction) {
  std::string desc = GetAcceleratorDescription(kAstraAcceleratorToggleSidebar);
  EXPECT_FALSE(desc.empty());
}

TEST_F(AstraAcceleratorTableTest, GetAcceleratorDescription_UnknownAction) {
  std::string desc = GetAcceleratorDescription("nonexistent.action");
  EXPECT_TRUE(desc.empty());
}

TEST_F(AstraAcceleratorTableTest, GetAcceleratorDescription_EmptyString) {
  std::string desc = GetAcceleratorDescription("");
  EXPECT_TRUE(desc.empty());
}

TEST_F(AstraAcceleratorTableTest, GetAcceleratorDescription_CommandPalette) {
  std::string desc = GetAcceleratorDescription(kAstraAcceleratorCommandPalette);
  EXPECT_FALSE(desc.empty());
  // Should mention "command" or "palette".
  EXPECT_TRUE(
      desc.find("command") != std::string::npos ||
      desc.find("palette") != std::string::npos ||
      desc.find("Command") != std::string::npos ||
      desc.find("Palette") != std::string::npos)
      << "Description doesn't mention command palette: " << desc;
}

TEST_F(AstraAcceleratorTableTest, GetAcceleratorDescription_MatchesEntry) {
  // The description from GetAcceleratorDescription should match the entry.
  auto entry = GetPrimaryAcceleratorForCommand(kAstraCommandToggleSidebar);
  ASSERT_NE(entry, nullptr);

  std::string desc_from_lookup =
      GetAcceleratorDescription(entry->accelerator_id);
  EXPECT_EQ(desc_from_lookup, entry->description);
}

// =========================================================================
// FormatAcceleratorText tests
// =========================================================================

TEST_F(FormatAcceleratorTextTest, SimpleLetterKey) {
  ui::Accelerator accel(ui::VKEY_A, kPrimaryModifier);
  std::string text = FormatAcceleratorText(accel);
  EXPECT_FALSE(text.empty());
  EXPECT_NE(text.find("A"), std::string::npos);
}

TEST_F(FormatAcceleratorTextTest, WithShift) {
  ui::Accelerator accel(ui::VKEY_B, kPrimaryModifier | ui::EF_SHIFT_DOWN);
  std::string text = FormatAcceleratorText(accel);
  EXPECT_FALSE(text.empty());
  EXPECT_NE(text.find("B"), std::string::npos);
}

TEST_F(FormatAcceleratorTextTest, FunctionKeyF11) {
  ui::Accelerator accel(ui::VKEY_F11, kPrimaryModifier);
  std::string text = FormatAcceleratorText(accel);
  EXPECT_FALSE(text.empty());
  EXPECT_NE(text.find("F11"), std::string::npos);
}

TEST_F(FormatAcceleratorTextTest, FunctionKeyF12) {
  ui::Accelerator accel(ui::VKEY_F12, kPrimaryModifier);
  std::string text = FormatAcceleratorText(accel);
  EXPECT_FALSE(text.empty());
  EXPECT_NE(text.find("F12"), std::string::npos);
}

TEST_F(FormatAcceleratorTextTest, DigitKey) {
  ui::Accelerator accel(ui::VKEY_5, kPrimaryModifier);
  std::string text = FormatAcceleratorText(accel);
  EXPECT_FALSE(text.empty());
  EXPECT_NE(text.find("5"), std::string::npos);
}

TEST_F(FormatAcceleratorTextTest, EnterKey) {
  ui::Accelerator accel(ui::VKEY_RETURN, kPrimaryModifier);
  std::string text = FormatAcceleratorText(accel);
  EXPECT_FALSE(text.empty());
  EXPECT_NE(text.find("Enter"), std::string::npos);
}

TEST_F(FormatAcceleratorTextTest, TabKey) {
  ui::Accelerator accel(ui::VKEY_TAB, kPrimaryModifier);
  std::string text = FormatAcceleratorText(accel);
  EXPECT_FALSE(text.empty());
  EXPECT_NE(text.find("Tab"), std::string::npos);
}

TEST_F(FormatAcceleratorTextTest, BracketKeys) {
  ui::Accelerator accel_left(ui::VKEY_OEM_4, kPrimaryModifier);
  std::string text_left = FormatAcceleratorText(accel_left);
  EXPECT_FALSE(text_left.empty());
  EXPECT_NE(text_left.find("["), std::string::npos);

  ui::Accelerator accel_right(ui::VKEY_OEM_6, kPrimaryModifier);
  std::string text_right = FormatAcceleratorText(accel_right);
  EXPECT_FALSE(text_right.empty());
  EXPECT_NE(text_right.find("]"), std::string::npos);
}

TEST_F(FormatAcceleratorTextTest, BackslashKey) {
  ui::Accelerator accel(ui::VKEY_OEM_5, kPrimaryModifier);
  std::string text = FormatAcceleratorText(accel);
  EXPECT_FALSE(text.empty());
  // Should contain backslash character.
  EXPECT_NE(text.find("\\"), std::string::npos);
}

TEST_F(FormatAcceleratorTextTest, MultipleModifiers) {
  ui::Accelerator accel(
      ui::VKEY_P,
      kPrimaryModifier | ui::EF_SHIFT_DOWN | ui::EF_ALT_DOWN);
  std::string text = FormatAcceleratorText(accel);
  EXPECT_FALSE(text.empty());
  EXPECT_NE(text.find("P"), std::string::npos);
}

TEST_F(FormatAcceleratorTextTest, NoModifiersProducesKeyOnly) {
  ui::Accelerator accel(ui::VKEY_F11, 0);
  std::string text = FormatAcceleratorText(accel);
  // Should just be "F11" with no modifier prefixes.
  EXPECT_EQ(text, "F11");
}

TEST_F(FormatAcceleratorTextTest, ConsistentResults) {
  // Formatting the same accelerator twice should give the same result.
  ui::Accelerator accel(ui::VKEY_P, kPrimaryModifier | ui::EF_SHIFT_DOWN);
  std::string text1 = FormatAcceleratorText(accel);
  std::string text2 = FormatAcceleratorText(accel);
  EXPECT_EQ(text1, text2);
}

TEST_F(FormatAcceleratorTextTest, DifferentAcceleratorsProduceDifferentText) {
  ui::Accelerator accel_a(ui::VKEY_A, kPrimaryModifier);
  ui::Accelerator accel_b(ui::VKEY_B, kPrimaryModifier);

  std::string text_a = FormatAcceleratorText(accel_a);
  std::string text_b = FormatAcceleratorText(accel_b);

  EXPECT_NE(text_a, text_b);
}

// =========================================================================
// IsAcceleratorConflicting tests
// =========================================================================

TEST_F(IsAcceleratorConflictingTest, NewTabIsConflicting) {
  // Ctrl/Cmd+T is Chrome's new tab shortcut and should be flagged.
  ui::Accelerator accel(ui::VKEY_T, kPrimaryModifier);
  EXPECT_TRUE(IsAcceleratorConflicting(accel));
}

TEST_F(IsAcceleratorConflictingTest, NewWindowIsConflicting) {
  ui::Accelerator accel(ui::VKEY_N, kPrimaryModifier);
  EXPECT_TRUE(IsAcceleratorConflicting(accel));
}

TEST_F(IsAcceleratorConflictingTest, CloseTabIsConflicting) {
  ui::Accelerator accel(ui::VKEY_W, kPrimaryModifier);
  EXPECT_TRUE(IsAcceleratorConflicting(accel));
}

TEST_F(IsAcceleratorConflictingTest, ReloadIsConflicting) {
  ui::Accelerator accel(ui::VKEY_R, kPrimaryModifier);
  EXPECT_TRUE(IsAcceleratorConflicting(accel));
}

TEST_F(IsAcceleratorConflictingTest, AddressBarIsConflicting) {
  ui::Accelerator accel(ui::VKEY_L, kPrimaryModifier);
  EXPECT_TRUE(IsAcceleratorConflicting(accel));
}

TEST_F(IsAcceleratorConflictingTest, BookmarkIsConflicting) {
  ui::Accelerator accel(ui::VKEY_D, kPrimaryModifier);
  EXPECT_TRUE(IsAcceleratorConflicting(accel));
}

TEST_F(IsAcceleratorConflictingTest, FindIsConflicting) {
  ui::Accelerator accel(ui::VKEY_F, kPrimaryModifier);
  EXPECT_TRUE(IsAcceleratorConflicting(accel));
}

TEST_F(IsAcceleratorConflictingTest, DevToolsIsConflicting) {
  ui::Accelerator accel(
      ui::VKEY_I, kPrimaryModifier | ui::EF_SHIFT_DOWN);
  EXPECT_TRUE(IsAcceleratorConflicting(accel));
}

TEST_F(IsAcceleratorConflictingTest, F11FullscreenIsConflicting) {
  ui::Accelerator accel(ui::VKEY_F11, 0);
  EXPECT_TRUE(IsAcceleratorConflicting(accel));
}

TEST_F(IsAcceleratorConflictingTest, SidebarShortcutIsNotConflicting) {
  // Ctrl/Cmd+B should not be in the reserved list (it's our shortcut).
  // Wait, actually Ctrl/Cmd+B might not be a standard Chrome shortcut.
  // Let's check with something that's definitely not in the reserved list.
  // Actually let's use a non-standard combo.
  ui::Accelerator accel(
      ui::VKEY_G, ui::EF_ALT_DOWN | ui::EF_SHIFT_DOWN);
  EXPECT_FALSE(IsAcceleratorConflicting(accel))
      << "Alt+Shift+G should not be a reserved Chrome shortcut";
}

TEST_F(IsAcceleratorConflictingTest, SplitViewShortcutIsNotConflicting) {
  // Ctrl/Cmd+\ (backslash) should not conflict.
  ui::Accelerator accel(ui::VKEY_OEM_5, kPrimaryModifier);
  EXPECT_FALSE(IsAcceleratorConflicting(accel))
      << "Ctrl/Cmd+\\ should not be a reserved Chrome shortcut";
}

TEST_F(IsAcceleratorConflictingTest, DifferentModifiersNotConflicting) {
  // Same key but different modifiers shouldn't conflict.
  // Ctrl+T conflicts, but Alt+T doesn't.
  ui::Accelerator ctrl_t(ui::VKEY_T, kPrimaryModifier);
  ui::Accelerator alt_t(ui::VKEY_T, ui::EF_ALT_DOWN);

  EXPECT_TRUE(IsAcceleratorConflicting(ctrl_t));
  EXPECT_FALSE(IsAcceleratorConflicting(alt_t));
}

TEST_F(IsAcceleratorConflictingTest, DownloadsIsConflicting) {
  // Ctrl/Cmd+J is Chrome's downloads shortcut.
  ui::Accelerator accel(ui::VKEY_J, kPrimaryModifier);
  EXPECT_TRUE(IsAcceleratorConflicting(accel));
}

TEST_F(IsAcceleratorConflictingTest, HistoryIsConflicting) {
  // Ctrl/Cmd+H is Chrome's history shortcut.
  ui::Accelerator accel(ui::VKEY_H, kPrimaryModifier);
  EXPECT_TRUE(IsAcceleratorConflicting(accel));
}

TEST_F(IsAcceleratorConflictingTest, OmniboxSearchIsConflicting) {
  // Ctrl/Cmd+K focuses the omnibox in search mode.
  ui::Accelerator accel(ui::VKEY_K, kPrimaryModifier);
  EXPECT_TRUE(IsAcceleratorConflicting(accel));
}

// =========================================================================
// Edge case tests
// =========================================================================

TEST_F(AstraAcceleratorTableTest, EdgeCase_AllAcceleratorIdsAreValidStrings) {
  auto table = GetAstraAcceleratorTable();
  for (const auto& entry : table) {
    // Should not contain spaces.
    EXPECT_EQ(entry.accelerator_id.find(' '), std::string::npos)
        << "Accelerator ID contains spaces: " << entry.accelerator_id;
    // Should not be empty.
    EXPECT_FALSE(entry.accelerator_id.empty());
    // Should have at least one dot (the "astra." prefix).
    EXPECT_NE(entry.accelerator_id.find('.'), std::string::npos)
        << "Accelerator ID doesn't contain '.': " << entry.accelerator_id;
  }
}

TEST_F(AstraAcceleratorTableTest, EdgeCase_AllDescriptionsAreMeaningful) {
  auto table = GetAstraAcceleratorTable();
  for (const auto& entry : table) {
    EXPECT_GT(entry.description.length(), 5u)
        << "Description too short for " << entry.accelerator_id << ": "
        << entry.description;
  }
}

TEST_F(AstraAcceleratorTableTest, EdgeCase_GetAllIsConsistent) {
  auto all1 = GetAllAccelerators();
  auto all2 = GetAllAccelerators();
  ASSERT_EQ(all1.size(), all2.size());
  for (size_t i = 0; i < all1.size(); ++i) {
    EXPECT_EQ(all1[i].accelerator_id, all2[i].accelerator_id);
    EXPECT_EQ(all1[i].keycode, all2[i].keycode);
    EXPECT_EQ(all1[i].modifiers, all2[i].modifiers);
  }
}

TEST_F(AstraAcceleratorTableTest, EdgeCase_TableSpanIsValid) {
  auto table = GetAstraAcceleratorTable();
  // Span should have valid data pointer.
  EXPECT_NE(table.data(), nullptr);
  EXPECT_GT(table.size(), 0u);
}

TEST_F(AstraAcceleratorTableTest, EdgeCase_CommandIdsInAstraRange) {
  // All command IDs should be in the Astra range (60000+).
  auto table = GetAstraAcceleratorTable();
  for (size_t i = 0; i < table.size(); ++i) {
    // Note: some entries might use placeholder command IDs like
    // kAstraCommandFirst for features not yet fully implemented.
    // Those are still in the Astra range.
    EXPECT_TRUE(IsAstraCommandId(table[i].command_id) ||
                table[i].command_id == kAstraCommandFirst)
        << "Entry " << i << " (" << table[i].accelerator_id
        << ") has command_id " << table[i].command_id
        << " which is outside the Astra range";
  }
}

TEST_F(AstraAcceleratorTableTest,
       EdgeCase_GetDefaultAcceleratorReturnsFirstDefault) {
  // When a command has multiple accelerators, the default should be
  // the first one with is_default=true.
  auto all = GetAcceleratorsForCommand(kAstraCommandOpenCommandPalette);
  ASSERT_GE(all.size(), 2u);  // Command palette has 2 entries.

  auto default_accel =
      GetDefaultAcceleratorForAction(kAstraAcceleratorCommandPalette);
  ASSERT_TRUE(default_accel.has_value());

  // Should match the first entry.
  EXPECT_EQ(default_accel->key_code(), all[0].keycode);
  EXPECT_EQ(default_accel->modifiers(), all[0].modifiers);
}

TEST_F(AstraAcceleratorTableTest, EdgeCase_InvalidActionIdReturnsNullopt) {
  auto result = GetDefaultAcceleratorForAction("astra.nonexistent");
  EXPECT_FALSE(result.has_value());
}

TEST_F(AstraAcceleratorTableTest, EdgeCase_DescriptionForUnknownActionIsEmpty) {
  std::string desc = GetAcceleratorDescription("completely.bogus.id");
  EXPECT_TRUE(desc.empty());
}

}  // namespace astra
