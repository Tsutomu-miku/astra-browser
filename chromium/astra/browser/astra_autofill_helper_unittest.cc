// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_autofill_helper.h"

#include <string>
#include <vector>

#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "chrome/test/base/testing_profile.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "astra/browser/astra_autofill_helper_factory.h"
#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Test observer that records calls for verification.
class TestAutofillObserver : public AstraAutofillObserver {
 public:
  void OnAutofillSuggestionShown() override {
    suggestion_shown_count_++;
  }

  void OnAutofillEntrySelected(const std::string& entry_id) override {
    entry_selected_count_++;
    last_selected_id_ = entry_id;
  }

  void OnAutofillEntryAdded(const std::string& entry_id) override {
    entry_added_count_++;
    last_added_id_ = entry_id;
  }

  void OnAutofillEntryRemoved(const std::string& entry_id) override {
    entry_removed_count_++;
    last_removed_id_ = entry_id;
  }

  void OnAutofillEntryUpdated(const std::string& entry_id) override {
    entry_updated_count_++;
    last_updated_id_ = entry_id;
  }

  void OnAutofillSettingsChanged() override {
    settings_changed_count_++;
  }

  void OnAutofillProfileChanged() override {
    profile_changed_count_++;
  }

  void OnAutofillDataCleared() override {
    data_cleared_count_++;
  }

  // Counters
  int suggestion_shown_count_ = 0;
  int entry_selected_count_ = 0;
  int entry_added_count_ = 0;
  int entry_removed_count_ = 0;
  int entry_updated_count_ = 0;
  int settings_changed_count_ = 0;
  int profile_changed_count_ = 0;
  int data_cleared_count_ = 0;

  // Last recorded values
  std::string last_selected_id_;
  std::string last_added_id_;
  std::string last_removed_id_;
  std::string last_updated_id_;
};

// Helper to create a test address entry.
AstraAutofillEntry CreateTestAddress(const std::string& id,
                                     const std::string& label,
                                     const std::string& value) {
  AstraAutofillEntry entry;
  entry.id = id;
  entry.type = AstraAutofillEntryType::kAddress;
  entry.label = label;
  entry.value = value;
  entry.last_used = base::Time::Now();
  entry.use_count = 0;
  entry.is_blocked = false;
  entry.profile_id = "profile1";
  return entry;
}

// Helper to create a test credit card entry.
AstraAutofillEntry CreateTestCard(const std::string& id,
                                  const std::string& label,
                                  const std::string& last_four) {
  AstraAutofillEntry entry;
  entry.id = id;
  entry.type = AstraAutofillEntryType::kCreditCard;
  entry.label = label;
  entry.value = last_four;
  entry.last_used = base::Time::Now();
  entry.use_count = 0;
  entry.is_blocked = false;
  entry.profile_id = "card_profile1";
  return entry;
}

// Helper to create a test email entry.
AstraAutofillEntry CreateTestEmail(const std::string& id,
                                   const std::string& email) {
  AstraAutofillEntry entry;
  entry.id = id;
  entry.type = AstraAutofillEntryType::kEmail;
  entry.label = email;
  entry.value = email;
  entry.last_used = base::Time::Now();
  entry.use_count = 0;
  return entry;
}

}  // namespace

// Test fixture for AstraAutofillHelper tests.
class AutofillHelperTest : public testing::Test {
 protected:
  AutofillHelperTest() {
    profile_ = std::make_unique<TestingProfile>();
    // Register Astra prefs on the testing profile.
    prefs::RegisterProfilePrefs(profile_->GetPrefs());
    helper_ = std::make_unique<AstraAutofillHelper>(profile_.get());
    DCHECK(helper_);
  }

  ~AutofillHelperTest() override = default;

  void SetUp() override {
    // Verify default presentation settings.
    ASSERT_TRUE(helper_->IsAutofillEnabled());
    ASSERT_TRUE(helper_->IsAddressAutofillEnabled());
    ASSERT_TRUE(helper_->IsCreditCardAutofillEnabled());
    ASSERT_TRUE(helper_->IsAutosignInEnabled());
  }

  void TearDown() override {
    // Clean up observers to avoid dangling references.
    for (auto& observer : test_observers_) {
      helper_->RemoveObserver(&observer);
    }
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<AstraAutofillHelper> helper_;
  std::vector<std::unique_ptr<TestAutofillObserver>> test_observers_;
};

// ---------------------------------------------------------------------------
// Construction and default state
// ---------------------------------------------------------------------------

TEST_F(AutofillHelperTest, DefaultState_EntryCountZero) {
  EXPECT_EQ(helper_->GetEntryCount(), 0u);
}

TEST_F(AutofillHelperTest, DefaultState_AddressCountZero) {
  EXPECT_EQ(helper_->GetAddressCount(), 0u);
}

TEST_F(AutofillHelperTest, DefaultState_CreditCardCountZero) {
  EXPECT_EQ(helper_->GetCreditCardCount(), 0u);
}

TEST_F(AutofillHelperTest, DefaultState_AllEntriesEmpty) {
  auto entries = helper_->GetAllEntries();
  EXPECT_TRUE(entries.empty());
}

TEST_F(AutofillHelperTest, DefaultState_AddressesEmpty) {
  auto entries = helper_->GetAddresses();
  EXPECT_TRUE(entries.empty());
}

TEST_F(AutofillHelperTest, DefaultState_CreditCardsEmpty) {
  auto entries = helper_->GetCreditCards();
  EXPECT_TRUE(entries.empty());
}

TEST_F(AutofillHelperTest, DefaultState_GetEntryReturnsDefault) {
  auto entry = helper_->GetEntry("nonexistent");
  EXPECT_TRUE(entry.id.empty());
  EXPECT_EQ(entry.type, AstraAutofillEntryType::kCustom);
  EXPECT_TRUE(entry.label.empty());
  EXPECT_TRUE(entry.value.empty());
  EXPECT_EQ(entry.use_count, 0);
  EXPECT_FALSE(entry.is_blocked);
}

TEST_F(AutofillHelperTest, DefaultState_AutofillEnabled) {
  EXPECT_TRUE(helper_->IsAutofillEnabled());
}

TEST_F(AutofillHelperTest, DefaultState_AddressAutofillEnabled) {
  EXPECT_TRUE(helper_->IsAddressAutofillEnabled());
}

TEST_F(AutofillHelperTest, DefaultState_CreditCardAutofillEnabled) {
  EXPECT_TRUE(helper_->IsCreditCardAutofillEnabled());
}

TEST_F(AutofillHelperTest, DefaultState_AutosignInEnabled) {
  EXPECT_TRUE(helper_->IsAutosignInEnabled());
}

TEST_F(AutofillHelperTest, DefaultState_ShowAutofillPopup) {
  EXPECT_TRUE(helper_->GetShowAutofillPopup());
}

TEST_F(AutofillHelperTest, DefaultState_PopupPosition) {
  EXPECT_EQ(helper_->GetAutofillPopupPosition(),
            prefs::kDefaultAutofillPopupPosition);
}

TEST_F(AutofillHelperTest, DefaultState_MaxSuggestions) {
  EXPECT_EQ(helper_->GetMaxSuggestions(),
            prefs::kDefaultAutofillMaxSuggestions);
}

TEST_F(AutofillHelperTest, DefaultState_ShowSuggestionIcons) {
  EXPECT_TRUE(helper_->GetShowSuggestionIcons());
}

TEST_F(AutofillHelperTest, DefaultState_ShowSuggestionLabels) {
  EXPECT_TRUE(helper_->GetShowSuggestionLabels());
}

TEST_F(AutofillHelperTest, DefaultState_ShowSuggestionSubtext) {
  EXPECT_TRUE(helper_->GetShowSuggestionSubtext());
}

TEST_F(AutofillHelperTest, DefaultState_AutofillOnTap) {
  EXPECT_TRUE(helper_->GetAutofillOnTap());
}

TEST_F(AutofillHelperTest, DefaultState_SuggestionsSortOrder) {
  EXPECT_EQ(helper_->GetSuggestionsSortOrder(),
            prefs::kDefaultAutofillSuggestionsSortOrder);
}

TEST_F(AutofillHelperTest, DefaultState_ShowCreditCardIcons) {
  EXPECT_TRUE(helper_->GetShowCreditCardIcons());
}

TEST_F(AutofillHelperTest, DefaultState_QuickCheckout) {
  EXPECT_FALSE(helper_->GetAutofillQuickCheckout());
}

TEST_F(AutofillHelperTest, DefaultState_GetSuggestionsEmpty) {
  auto suggestions = helper_->GetSuggestions("email", 5);
  EXPECT_TRUE(suggestions.empty());
}

TEST_F(AutofillHelperTest, DefaultState_GetRecentlyUsedEmpty) {
  auto entries = helper_->GetRecentlyUsed(5);
  EXPECT_TRUE(entries.empty());
}

TEST_F(AutofillHelperTest, DefaultState_GetMostUsedEmpty) {
  auto entries = helper_->GetMostUsed(5);
  EXPECT_TRUE(entries.empty());
}

// ---------------------------------------------------------------------------
// Entry CRUD operations
// ---------------------------------------------------------------------------

TEST_F(AutofillHelperTest, AddEntry_SingleAddress) {
  auto entry = CreateTestAddress("addr1", "Home", "123 Main St");
  helper_->AddEntry(entry);

  EXPECT_EQ(helper_->GetEntryCount(), 1u);
  EXPECT_EQ(helper_->GetAddressCount(), 1u);

  auto retrieved = helper_->GetEntry("addr1");
  EXPECT_EQ(retrieved.id, "addr1");
  EXPECT_EQ(retrieved.type, AstraAutofillEntryType::kAddress);
  EXPECT_EQ(retrieved.label, "Home");
  EXPECT_EQ(retrieved.value, "123 Main St");
}

TEST_F(AutofillHelperTest, AddEntry_SingleCreditCard) {
  auto entry = CreateTestCard("card1", "Visa", "1234");
  helper_->AddEntry(entry);

  EXPECT_EQ(helper_->GetEntryCount(), 1u);
  EXPECT_EQ(helper_->GetCreditCardCount(), 1u);

  auto retrieved = helper_->GetEntry("card1");
  EXPECT_EQ(retrieved.id, "card1");
  EXPECT_EQ(retrieved.type, AstraAutofillEntryType::kCreditCard);
  EXPECT_EQ(retrieved.label, "Visa");
}

TEST_F(AutofillHelperTest, AddEntry_MultipleTypes) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));
  helper_->AddEntry(CreateTestCard("card1", "Visa", "1234"));
  helper_->AddEntry(CreateTestEmail("email1", "user@example.com"));

  EXPECT_EQ(helper_->GetEntryCount(), 3u);
  EXPECT_EQ(helper_->GetAddressCount(), 1u);
  EXPECT_EQ(helper_->GetCreditCardCount(), 1u);
}

TEST_F(AutofillHelperTest, AddEntry_DuplicateIdUpdates) {
  auto entry1 = CreateTestAddress("addr1", "Home", "123 Main St");
  entry1.use_count = 5;
  helper_->AddEntry(entry1);

  auto entry2 = CreateTestAddress("addr1", "Updated Home", "456 Oak Ave");
  entry2.use_count = 10;
  helper_->AddEntry(entry2);

  EXPECT_EQ(helper_->GetEntryCount(), 1u);

  auto retrieved = helper_->GetEntry("addr1");
  EXPECT_EQ(retrieved.label, "Updated Home");
  EXPECT_EQ(retrieved.value, "456 Oak Ave");
  EXPECT_EQ(retrieved.use_count, 10);
}

TEST_F(AutofillHelperTest, AddEntry_EmptyIdIgnored) {
  AstraAutofillEntry entry;
  entry.id = "";
  entry.label = "Test";
  helper_->AddEntry(entry);

  EXPECT_EQ(helper_->GetEntryCount(), 0u);
}

TEST_F(AutofillHelperTest, GetAllEntries_ReturnsAll) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));
  helper_->AddEntry(CreateTestAddress("addr2", "Work", "456 Oak Ave"));
  helper_->AddEntry(CreateTestCard("card1", "Visa", "1234"));
  helper_->AddEntry(CreateTestEmail("email1", "user@example.com"));

  auto all = helper_->GetAllEntries();
  EXPECT_EQ(all.size(), 4u);
}

TEST_F(AutofillHelperTest, GetAddresses_ReturnsOnlyAddresses) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));
  helper_->AddEntry(CreateTestAddress("addr2", "Work", "456 Oak Ave"));
  helper_->AddEntry(CreateTestCard("card1", "Visa", "1234"));

  auto addresses = helper_->GetAddresses();
  EXPECT_EQ(addresses.size(), 2u);
  for (const auto& entry : addresses) {
    EXPECT_EQ(entry.type, AstraAutofillEntryType::kAddress);
  }
}

TEST_F(AutofillHelperTest, GetCreditCards_ReturnsOnlyCards) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));
  helper_->AddEntry(CreateTestCard("card1", "Visa", "1234"));
  helper_->AddEntry(CreateTestCard("card2", "Mastercard", "5678"));

  auto cards = helper_->GetCreditCards();
  EXPECT_EQ(cards.size(), 2u);
  for (const auto& entry : cards) {
    EXPECT_EQ(entry.type, AstraAutofillEntryType::kCreditCard);
  }
}

TEST_F(AutofillHelperTest, UpdateEntry_ExistingEntry) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));

  AstraAutofillEntry updated;
  updated.id = "addr1";
  updated.type = AstraAutofillEntryType::kAddress;
  updated.label = "New Home";
  updated.value = "789 Pine Rd";
  updated.use_count = 42;

  helper_->UpdateEntry("addr1", updated);

  auto retrieved = helper_->GetEntry("addr1");
  EXPECT_EQ(retrieved.label, "New Home");
  EXPECT_EQ(retrieved.value, "789 Pine Rd");
  EXPECT_EQ(retrieved.use_count, 42);
}

TEST_F(AutofillHelperTest, UpdateEntry_NonexistentIdNoOp) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));

  AstraAutofillEntry updated;
  updated.id = "nonexistent";
  updated.label = "Test";

  // Should not crash and should not add a new entry.
  helper_->UpdateEntry("nonexistent", updated);
  EXPECT_EQ(helper_->GetEntryCount(), 1u);
}

TEST_F(AutofillHelperTest, UpdateEntry_EmptyIdNoOp) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));

  AstraAutofillEntry updated;
  updated.id = "";
  helper_->UpdateEntry("", updated);

  EXPECT_EQ(helper_->GetEntryCount(), 1u);
}

TEST_F(AutofillHelperTest, UpdateEntry_PreservesId) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));

  AstraAutofillEntry updated;
  updated.id = "different_id";  // Should be ignored
  updated.type = AstraAutofillEntryType::kAddress;
  updated.label = "Updated";

  helper_->UpdateEntry("addr1", updated);

  auto retrieved = helper_->GetEntry("addr1");
  EXPECT_EQ(retrieved.id, "addr1");
  EXPECT_EQ(retrieved.label, "Updated");
}

TEST_F(AutofillHelperTest, RemoveEntry_ExistingEntry) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));
  helper_->AddEntry(CreateTestCard("card1", "Visa", "1234"));

  EXPECT_EQ(helper_->GetEntryCount(), 2u);

  helper_->RemoveEntry("addr1");
  EXPECT_EQ(helper_->GetEntryCount(), 1u);
  EXPECT_EQ(helper_->GetAddressCount(), 0u);
  EXPECT_EQ(helper_->GetCreditCardCount(), 1u);

  auto retrieved = helper_->GetEntry("addr1");
  EXPECT_TRUE(retrieved.id.empty());
}

TEST_F(AutofillHelperTest, RemoveEntry_NonexistentIdNoOp) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));

  helper_->RemoveEntry("nonexistent");
  EXPECT_EQ(helper_->GetEntryCount(), 1u);
}

TEST_F(AutofillHelperTest, RemoveEntry_EmptyIdNoOp) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));

  helper_->RemoveEntry("");
  EXPECT_EQ(helper_->GetEntryCount(), 1u);
}

TEST_F(AutofillHelperTest, BlockEntry_BlocksEntry) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));

  helper_->BlockEntry("addr1");

  auto entry = helper_->GetEntry("addr1");
  EXPECT_TRUE(entry.is_blocked);
}

TEST_F(AutofillHelperTest, BlockEntry_Idempotent) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));

  helper_->BlockEntry("addr1");
  helper_->BlockEntry("addr1");  // Second call should be safe

  auto entry = helper_->GetEntry("addr1");
  EXPECT_TRUE(entry.is_blocked);
}

TEST_F(AutofillHelperTest, BlockEntry_NonexistentIdNoOp) {
  // Should not crash.
  helper_->BlockEntry("nonexistent");
}

TEST_F(AutofillHelperTest, UnblockEntry_UnblocksEntry) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));
  helper_->BlockEntry("addr1");
  EXPECT_TRUE(helper_->GetEntry("addr1").is_blocked);

  helper_->UnblockEntry("addr1");
  EXPECT_FALSE(helper_->GetEntry("addr1").is_blocked);
}

TEST_F(AutofillHelperTest, UnblockEntry_Idempotent) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));

  helper_->UnblockEntry("addr1");  // Already unblocked
  helper_->UnblockEntry("addr1");  // Second call should be safe

  EXPECT_FALSE(helper_->GetEntry("addr1").is_blocked);
}

TEST_F(AutofillHelperTest, UnblockEntry_NonexistentIdNoOp) {
  // Should not crash.
  helper_->UnblockEntry("nonexistent");
}

// ---------------------------------------------------------------------------
// Clear operations
// ---------------------------------------------------------------------------

TEST_F(AutofillHelperTest, ClearAllEntries_ClearsEverything) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));
  helper_->AddEntry(CreateTestCard("card1", "Visa", "1234"));
  helper_->AddEntry(CreateTestEmail("email1", "user@example.com"));

  EXPECT_EQ(helper_->GetEntryCount(), 3u);

  helper_->ClearAllEntries();
  EXPECT_EQ(helper_->GetEntryCount(), 0u);
  EXPECT_EQ(helper_->GetAddressCount(), 0u);
  EXPECT_EQ(helper_->GetCreditCardCount(), 0u);
}

TEST_F(AutofillHelperTest, ClearAllEntries_EmptyStateNoOp) {
  // Should not crash.
  helper_->ClearAllEntries();
  EXPECT_EQ(helper_->GetEntryCount(), 0u);
}

TEST_F(AutofillHelperTest, ClearAddresses_ClearsOnlyAddresses) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));
  helper_->AddEntry(CreateTestAddress("addr2", "Work", "456 Oak Ave"));
  helper_->AddEntry(CreateTestCard("card1", "Visa", "1234"));
  helper_->AddEntry(CreateTestEmail("email1", "user@example.com"));

  EXPECT_EQ(helper_->GetEntryCount(), 4u);

  helper_->ClearAddresses();
  EXPECT_EQ(helper_->GetEntryCount(), 2u);
  EXPECT_EQ(helper_->GetAddressCount(), 0u);
  EXPECT_EQ(helper_->GetCreditCardCount(), 1u);
}

TEST_F(AutofillHelperTest, ClearCreditCards_ClearsOnlyCards) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));
  helper_->AddEntry(CreateTestCard("card1", "Visa", "1234"));
  helper_->AddEntry(CreateTestCard("card2", "Mastercard", "5678"));

  EXPECT_EQ(helper_->GetEntryCount(), 3u);

  helper_->ClearCreditCards();
  EXPECT_EQ(helper_->GetEntryCount(), 1u);
  EXPECT_EQ(helper_->GetAddressCount(), 1u);
  EXPECT_EQ(helper_->GetCreditCardCount(), 0u);
}

// ---------------------------------------------------------------------------
// Suggestion queries
// ---------------------------------------------------------------------------

TEST_F(AutofillHelperTest, GetSuggestions_ByAddressType) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));
  helper_->AddEntry(CreateTestAddress("addr2", "Work", "456 Oak Ave"));
  helper_->AddEntry(CreateTestCard("card1", "Visa", "1234"));

  auto suggestions = helper_->GetSuggestions("address", 10);
  EXPECT_EQ(suggestions.size(), 2u);
  for (const auto& s : suggestions) {
    EXPECT_EQ(s.type, AstraAutofillEntryType::kAddress);
  }
}

TEST_F(AutofillHelperTest, GetSuggestions_ByEmailType) {
  helper_->AddEntry(CreateTestEmail("email1", "user@example.com"));
  helper_->AddEntry(CreateTestEmail("email2", "admin@example.org"));
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));

  auto suggestions = helper_->GetSuggestions("email", 10);
  EXPECT_EQ(suggestions.size(), 2u);
  for (const auto& s : suggestions) {
    EXPECT_EQ(s.type, AstraAutofillEntryType::kEmail);
  }
}

TEST_F(AutofillHelperTest, GetSuggestions_ByCreditCardType) {
  helper_->AddEntry(CreateTestCard("card1", "Visa", "1234"));
  helper_->AddEntry(CreateTestCard("card2", "Mastercard", "5678"));
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));

  auto suggestions = helper_->GetSuggestions("credit-card", 10);
  EXPECT_EQ(suggestions.size(), 2u);
  for (const auto& s : suggestions) {
    EXPECT_EQ(s.type, AstraAutofillEntryType::kCreditCard);
  }
}

TEST_F(AutofillHelperTest, GetSuggestions_RespectsMaxCount) {
  for (int i = 0; i < 10; ++i) {
    helper_->AddEntry(CreateTestEmail("email" + std::to_string(i),
                                      "user" + std::to_string(i) + "@example.com"));
  }

  auto suggestions = helper_->GetSuggestions("email", 3);
  EXPECT_EQ(suggestions.size(), 3u);
}

TEST_F(AutofillHelperTest, GetSuggestions_BlockedEntriesExcluded) {
  auto entry = CreateTestEmail("email1", "blocked@example.com");
  entry.is_blocked = true;
  helper_->AddEntry(entry);
  helper_->AddEntry(CreateTestEmail("email2", "active@example.com"));

  auto suggestions = helper_->GetSuggestions("email", 10);
  EXPECT_EQ(suggestions.size(), 1u);
  EXPECT_EQ(suggestions[0].value, "active@example.com");
}

TEST_F(AutofillHelperTest, GetSuggestions_ZeroMaxUsesDefault) {
  helper_->AddEntry(CreateTestEmail("email1", "user@example.com"));

  // Zero max should use default max suggestions.
  auto suggestions = helper_->GetSuggestions("email", 0);
  EXPECT_LE(suggestions.size(),
            static_cast<size_t>(prefs::kDefaultAutofillMaxSuggestions));
}

TEST_F(AutofillHelperTest, GetSuggestions_NegativeMaxUsesDefault) {
  helper_->AddEntry(CreateTestEmail("email1", "user@example.com"));

  auto suggestions = helper_->GetSuggestions("email", -5);
  EXPECT_LE(suggestions.size(),
            static_cast<size_t>(prefs::kDefaultAutofillMaxSuggestions));
}

TEST_F(AutofillHelperTest, GetSuggestions_UnknownTypeReturnsAllNonBlocked) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));
  helper_->AddEntry(CreateTestCard("card1", "Visa", "1234"));
  helper_->AddEntry(CreateTestEmail("email1", "user@example.com"));

  auto blocked = CreateTestEmail("email2", "blocked@example.com");
  blocked.is_blocked = true;
  helper_->AddEntry(blocked);

  auto suggestions = helper_->GetSuggestions("unknown_type", 10);
  EXPECT_EQ(suggestions.size(), 3u);
}

// ---------------------------------------------------------------------------
// Recently used / most used queries
// ---------------------------------------------------------------------------

TEST_F(AutofillHelperTest, GetRecentlyUsed_ReturnsMostRecentFirst) {
  auto entry1 = CreateTestEmail("email1", "old@example.com");
  entry1.last_used = base::Time::Now() - base::Days(10);
  helper_->AddEntry(entry1);

  auto entry2 = CreateTestEmail("email2", "recent@example.com");
  entry2.last_used = base::Time::Now() - base::Hours(1);
  helper_->AddEntry(entry2);

  auto entry3 = CreateTestEmail("email3", "newest@example.com");
  entry3.last_used = base::Time::Now();
  helper_->AddEntry(entry3);

  auto recent = helper_->GetRecentlyUsed(10);
  ASSERT_EQ(recent.size(), 3u);
  EXPECT_EQ(recent[0].id, "email3");
  EXPECT_EQ(recent[1].id, "email2");
  EXPECT_EQ(recent[2].id, "email1");
}

TEST_F(AutofillHelperTest, GetRecentlyUsed_RespectsMaxCount) {
  for (int i = 0; i < 10; ++i) {
    auto entry = CreateTestEmail("email" + std::to_string(i),
                                 "user" + std::to_string(i) + "@example.com");
    entry.last_used = base::Time::Now() - base::Hours(i);
    helper_->AddEntry(entry);
  }

  auto recent = helper_->GetRecentlyUsed(3);
  EXPECT_EQ(recent.size(), 3u);
}

TEST_F(AutofillHelperTest, GetRecentlyUsed_ExcludesBlocked) {
  auto entry = CreateTestEmail("email1", "blocked@example.com");
  entry.is_blocked = true;
  entry.last_used = base::Time::Now();
  helper_->AddEntry(entry);

  helper_->AddEntry(CreateTestEmail("email2", "active@example.com"));

  auto recent = helper_->GetRecentlyUsed(10);
  EXPECT_EQ(recent.size(), 1u);
}

TEST_F(AutofillHelperTest, GetMostUsed_ReturnsMostUsedFirst) {
  auto entry1 = CreateTestEmail("email1", "low@example.com");
  entry1.use_count = 1;
  helper_->AddEntry(entry1);

  auto entry2 = CreateTestEmail("email2", "medium@example.com");
  entry2.use_count = 5;
  helper_->AddEntry(entry2);

  auto entry3 = CreateTestEmail("email3", "high@example.com");
  entry3.use_count = 100;
  helper_->AddEntry(entry3);

  auto most_used = helper_->GetMostUsed(10);
  ASSERT_EQ(most_used.size(), 3u);
  EXPECT_EQ(most_used[0].id, "email3");
  EXPECT_EQ(most_used[1].id, "email2");
  EXPECT_EQ(most_used[2].id, "email1");
}

TEST_F(AutofillHelperTest, GetMostUsed_RespectsMaxCount) {
  for (int i = 0; i < 10; ++i) {
    auto entry = CreateTestEmail("email" + std::to_string(i),
                                 "user" + std::to_string(i) + "@example.com");
    entry.use_count = i;
    helper_->AddEntry(entry);
  }

  auto most_used = helper_->GetMostUsed(3);
  EXPECT_EQ(most_used.size(), 3u);
}

TEST_F(AutofillHelperTest, GetMostUsed_ExcludesBlocked) {
  auto entry = CreateTestEmail("email1", "blocked@example.com");
  entry.is_blocked = true;
  entry.use_count = 999;
  helper_->AddEntry(entry);

  helper_->AddEntry(CreateTestEmail("email2", "active@example.com"));

  auto most_used = helper_->GetMostUsed(10);
  EXPECT_EQ(most_used.size(), 1u);
  EXPECT_EQ(most_used[0].id, "email2");
}

// ---------------------------------------------------------------------------
// Autofill enable / disable settings
// ---------------------------------------------------------------------------

TEST_F(AutofillHelperTest, SetAutofillEnabled_ToggleOff) {
  helper_->SetAutofillEnabled(false);
  EXPECT_FALSE(helper_->IsAutofillEnabled());
}

TEST_F(AutofillHelperTest, SetAutofillEnabled_ToggleOn) {
  helper_->SetAutofillEnabled(false);
  helper_->SetAutofillEnabled(true);
  EXPECT_TRUE(helper_->IsAutofillEnabled());
}

TEST_F(AutofillHelperTest, SetAutofillEnabled_Idempotent) {
  TestAutofillObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetAutofillEnabled(true);  // Already true
  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(AutofillHelperTest, SetAddressAutofillEnabled_Toggle) {
  helper_->SetAddressAutofillEnabled(false);
  EXPECT_FALSE(helper_->IsAddressAutofillEnabled());

  helper_->SetAddressAutofillEnabled(true);
  EXPECT_TRUE(helper_->IsAddressAutofillEnabled());
}

TEST_F(AutofillHelperTest, SetCreditCardAutofillEnabled_Toggle) {
  helper_->SetCreditCardAutofillEnabled(false);
  EXPECT_FALSE(helper_->IsCreditCardAutofillEnabled());

  helper_->SetCreditCardAutofillEnabled(true);
  EXPECT_TRUE(helper_->IsCreditCardAutofillEnabled());
}

TEST_F(AutofillHelperTest, SetAutosignInEnabled_Toggle) {
  helper_->SetAutosignInEnabled(false);
  EXPECT_FALSE(helper_->IsAutosignInEnabled());

  helper_->SetAutosignInEnabled(true);
  EXPECT_TRUE(helper_->IsAutosignInEnabled());
}

// ---------------------------------------------------------------------------
// Presentation settings
// ---------------------------------------------------------------------------

TEST_F(AutofillHelperTest, SetShowAutofillPopup_Toggle) {
  helper_->SetShowAutofillPopup(false);
  EXPECT_FALSE(helper_->GetShowAutofillPopup());

  helper_->SetShowAutofillPopup(true);
  EXPECT_TRUE(helper_->GetShowAutofillPopup());
}

TEST_F(AutofillHelperTest, SetAutofillPopupPosition_ValidValues) {
  helper_->SetAutofillPopupPosition("below_field");
  EXPECT_EQ(helper_->GetAutofillPopupPosition(), "below_field");

  helper_->SetAutofillPopupPosition("above_field");
  EXPECT_EQ(helper_->GetAutofillPopupPosition(), "above_field");

  helper_->SetAutofillPopupPosition("auto");
  EXPECT_EQ(helper_->GetAutofillPopupPosition(), "auto");
}

TEST_F(AutofillHelperTest, SetAutofillPopupPosition_InvalidValueDefaults) {
  helper_->SetAutofillPopupPosition("invalid_position");
  EXPECT_EQ(helper_->GetAutofillPopupPosition(),
            prefs::kDefaultAutofillPopupPosition);
}

TEST_F(AutofillHelperTest, SetMaxSuggestions_ValidValue) {
  helper_->SetMaxSuggestions(10);
  EXPECT_EQ(helper_->GetMaxSuggestions(), 10);
}

TEST_F(AutofillHelperTest, SetMaxSuggestions_ClampedToMin) {
  helper_->SetMaxSuggestions(0);
  EXPECT_EQ(helper_->GetMaxSuggestions(), 1);
}

TEST_F(AutofillHelperTest, SetMaxSuggestions_ClampedToMax) {
  helper_->SetMaxSuggestions(100);
  EXPECT_EQ(helper_->GetMaxSuggestions(), 20);
}

TEST_F(AutofillHelperTest, SetMaxSuggestions_NegativeClamped) {
  helper_->SetMaxSuggestions(-5);
  EXPECT_EQ(helper_->GetMaxSuggestions(), 1);
}

TEST_F(AutofillHelperTest, SetMaxSuggestions_Idempotent) {
  TestAutofillObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetMaxSuggestions(6);  // Already default
  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(AutofillHelperTest, SetShowSuggestionIcons_Toggle) {
  helper_->SetShowSuggestionIcons(false);
  EXPECT_FALSE(helper_->GetShowSuggestionIcons());

  helper_->SetShowSuggestionIcons(true);
  EXPECT_TRUE(helper_->GetShowSuggestionIcons());
}

TEST_F(AutofillHelperTest, SetShowSuggestionLabels_Toggle) {
  helper_->SetShowSuggestionLabels(false);
  EXPECT_FALSE(helper_->GetShowSuggestionLabels());

  helper_->SetShowSuggestionLabels(true);
  EXPECT_TRUE(helper_->GetShowSuggestionLabels());
}

TEST_F(AutofillHelperTest, SetShowSuggestionSubtext_Toggle) {
  helper_->SetShowSuggestionSubtext(false);
  EXPECT_FALSE(helper_->GetShowSuggestionSubtext());

  helper_->SetShowSuggestionSubtext(true);
  EXPECT_TRUE(helper_->GetShowSuggestionSubtext());
}

TEST_F(AutofillHelperTest, SetAutofillOnTap_Toggle) {
  helper_->SetAutofillOnTap(false);
  EXPECT_FALSE(helper_->GetAutofillOnTap());

  helper_->SetAutofillOnTap(true);
  EXPECT_TRUE(helper_->GetAutofillOnTap());
}

TEST_F(AutofillHelperTest, SetSuggestionsSortOrder_ValidValues) {
  helper_->SetSuggestionsSortOrder("most_recent");
  EXPECT_EQ(helper_->GetSuggestionsSortOrder(), "most_recent");

  helper_->SetSuggestionsSortOrder("most_used");
  EXPECT_EQ(helper_->GetSuggestionsSortOrder(), "most_used");

  helper_->SetSuggestionsSortOrder("alphabetical");
  EXPECT_EQ(helper_->GetSuggestionsSortOrder(), "alphabetical");
}

TEST_F(AutofillHelperTest, SetSuggestionsSortOrder_InvalidValueDefaults) {
  helper_->SetSuggestionsSortOrder("invalid_order");
  EXPECT_EQ(helper_->GetSuggestionsSortOrder(),
            prefs::kDefaultAutofillSuggestionsSortOrder);
}

TEST_F(AutofillHelperTest, SetShowCreditCardIcons_Toggle) {
  helper_->SetShowCreditCardIcons(false);
  EXPECT_FALSE(helper_->GetShowCreditCardIcons());

  helper_->SetShowCreditCardIcons(true);
  EXPECT_TRUE(helper_->GetShowCreditCardIcons());
}

TEST_F(AutofillHelperTest, SetAutofillQuickCheckout_Toggle) {
  helper_->SetAutofillQuickCheckout(true);
  EXPECT_TRUE(helper_->GetAutofillQuickCheckout());

  helper_->SetAutofillQuickCheckout(false);
  EXPECT_FALSE(helper_->GetAutofillQuickCheckout());
}

// ---------------------------------------------------------------------------
// Observer notifications
// ---------------------------------------------------------------------------

TEST_F(AutofillHelperTest, Observer_OnSuggestionShown) {
  TestAutofillObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifySuggestionShown();
  EXPECT_EQ(observer.suggestion_shown_count_, 1);

  helper_->NotifySuggestionShown();
  EXPECT_EQ(observer.suggestion_shown_count_, 2);

  helper_->RemoveObserver(&observer);
}

TEST_F(AutofillHelperTest, Observer_OnEntrySelected) {
  TestAutofillObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifyEntrySelected("entry1");
  EXPECT_EQ(observer.entry_selected_count_, 1);
  EXPECT_EQ(observer.last_selected_id_, "entry1");

  helper_->NotifyEntrySelected("entry2");
  EXPECT_EQ(observer.entry_selected_count_, 2);
  EXPECT_EQ(observer.last_selected_id_, "entry2");

  helper_->RemoveObserver(&observer);
}

TEST_F(AutofillHelperTest, Observer_OnEntryAdded) {
  TestAutofillObserver observer;
  helper_->AddObserver(&observer);

  auto entry = CreateTestAddress("addr1", "Home", "123 Main St");
  helper_->AddEntry(entry);

  EXPECT_EQ(observer.entry_added_count_, 1);
  EXPECT_EQ(observer.last_added_id_, "addr1");

  helper_->RemoveObserver(&observer);
}

TEST_F(AutofillHelperTest, Observer_OnEntryRemoved) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));

  TestAutofillObserver observer;
  helper_->AddObserver(&observer);

  helper_->RemoveEntry("addr1");
  EXPECT_EQ(observer.entry_removed_count_, 1);
  EXPECT_EQ(observer.last_removed_id_, "addr1");

  helper_->RemoveObserver(&observer);
}

TEST_F(AutofillHelperTest, Observer_OnEntryUpdated) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));

  TestAutofillObserver observer;
  helper_->AddObserver(&observer);

  auto updated = CreateTestAddress("addr1", "Updated", "456 Oak");
  helper_->UpdateEntry("addr1", updated);
  EXPECT_EQ(observer.entry_updated_count_, 1);
  EXPECT_EQ(observer.last_updated_id_, "addr1");

  helper_->RemoveObserver(&observer);
}

TEST_F(AutofillHelperTest, Observer_OnSettingsChanged) {
  TestAutofillObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetAutofillEnabled(false);
  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->SetAutofillEnabled(true);
  EXPECT_EQ(observer.settings_changed_count_, 2);

  helper_->RemoveObserver(&observer);
}

TEST_F(AutofillHelperTest, Observer_OnProfileChanged) {
  TestAutofillObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifyProfileChanged();
  EXPECT_EQ(observer.profile_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(AutofillHelperTest, Observer_OnDataCleared) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));

  TestAutofillObserver observer;
  helper_->AddObserver(&observer);

  helper_->ClearAllEntries();
  EXPECT_EQ(observer.data_cleared_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(AutofillHelperTest, Observer_AddEntryWithDuplicateIdFiresUpdated) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));

  TestAutofillObserver observer;
  helper_->AddObserver(&observer);

  auto updated = CreateTestAddress("addr1", "Updated", "456 Oak");
  helper_->AddEntry(updated);

  EXPECT_EQ(observer.entry_added_count_, 0);
  EXPECT_EQ(observer.entry_updated_count_, 1);
  EXPECT_EQ(observer.last_updated_id_, "addr1");

  helper_->RemoveObserver(&observer);
}

TEST_F(AutofillHelperTest, Observer_BlockEntryFiresUpdated) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));

  TestAutofillObserver observer;
  helper_->AddObserver(&observer);

  helper_->BlockEntry("addr1");
  EXPECT_EQ(observer.entry_updated_count_, 1);
  EXPECT_EQ(observer.last_updated_id_, "addr1");

  helper_->RemoveObserver(&observer);
}

TEST_F(AutofillHelperTest, Observer_UnblockEntryFiresUpdated) {
  auto entry = CreateTestAddress("addr1", "Home", "123 Main St");
  entry.is_blocked = true;
  helper_->AddEntry(entry);

  TestAutofillObserver observer;
  helper_->AddObserver(&observer);

  helper_->UnblockEntry("addr1");
  EXPECT_EQ(observer.entry_updated_count_, 1);
  EXPECT_EQ(observer.last_updated_id_, "addr1");

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Observer defaults — empty implementations don't crash
// ---------------------------------------------------------------------------

TEST_F(AutofillHelperTest, ObserverDefaults_DoNotCrash) {
  // Create a minimal observer that overrides nothing.
  // All methods have default empty implementations, so this should compile
  // and not crash.
  class DefaultObserver : public AstraAutofillObserver {};

  DefaultObserver observer;
  helper_->AddObserver(&observer);

  // Trigger all observer paths.
  helper_->NotifySuggestionShown();
  helper_->NotifyEntrySelected("test");
  helper_->NotifyEntryAdded("test");
  helper_->NotifyEntryRemoved("test");
  helper_->NotifyEntryUpdated("test");
  helper_->NotifySettingsChanged();
  helper_->NotifyProfileChanged();
  helper_->NotifyDataCleared();

  // Also trigger via settings change.
  helper_->SetAutofillEnabled(false);

  // Also trigger via entry operations.
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));
  helper_->UpdateEntry("addr1", CreateTestAddress("addr1", "Updated", ""));
  helper_->RemoveEntry("addr1");
  helper_->ClearAllEntries();

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Multiple observers
// ---------------------------------------------------------------------------

TEST_F(AutofillHelperTest, MultipleObservers_AllNotified) {
  TestAutofillObserver observer1;
  TestAutofillObserver observer2;
  TestAutofillObserver observer3;

  helper_->AddObserver(&observer1);
  helper_->AddObserver(&observer2);
  helper_->AddObserver(&observer3);

  helper_->NotifySuggestionShown();

  EXPECT_EQ(observer1.suggestion_shown_count_, 1);
  EXPECT_EQ(observer2.suggestion_shown_count_, 1);
  EXPECT_EQ(observer3.suggestion_shown_count_, 1);

  helper_->RemoveObserver(&observer1);
  helper_->RemoveObserver(&observer2);
  helper_->RemoveObserver(&observer3);
}

TEST_F(AutofillHelperTest, RemoveObserver_StopsNotifications) {
  TestAutofillObserver observer1;
  TestAutofillObserver observer2;

  helper_->AddObserver(&observer1);
  helper_->AddObserver(&observer2);

  helper_->NotifySuggestionShown();
  EXPECT_EQ(observer1.suggestion_shown_count_, 1);
  EXPECT_EQ(observer2.suggestion_shown_count_, 1);

  helper_->RemoveObserver(&observer1);

  helper_->NotifySuggestionShown();
  EXPECT_EQ(observer1.suggestion_shown_count_, 1);  // Unchanged
  EXPECT_EQ(observer2.suggestion_shown_count_, 2);  // Incremented

  helper_->RemoveObserver(&observer2);
}

// ---------------------------------------------------------------------------
// Bulk operations
// ---------------------------------------------------------------------------

TEST_F(AutofillHelperTest, ImportEntries_AddsMultiple) {
  std::vector<AstraAutofillEntry> entries;
  entries.push_back(CreateTestAddress("addr1", "Home", "123 Main St"));
  entries.push_back(CreateTestCard("card1", "Visa", "1234"));
  entries.push_back(CreateTestEmail("email1", "user@example.com"));

  helper_->ImportEntries(entries);
  EXPECT_EQ(helper_->GetEntryCount(), 3u);
  EXPECT_EQ(helper_->GetAddressCount(), 1u);
  EXPECT_EQ(helper_->GetCreditCardCount(), 1u);
}

TEST_F(AutofillHelperTest, ImportEntries_EmptyList) {
  std::vector<AstraAutofillEntry> entries;
  helper_->ImportEntries(entries);
  EXPECT_EQ(helper_->GetEntryCount(), 0u);
}

TEST_F(AutofillHelperTest, ImportEntries_WithDuplicatesUpdates) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));

  std::vector<AstraAutofillEntry> entries;
  auto updated = CreateTestAddress("addr1", "Updated", "456 Oak");
  updated.use_count = 42;
  entries.push_back(updated);
  entries.push_back(CreateTestAddress("addr2", "Work", "789 Pine"));

  helper_->ImportEntries(entries);
  EXPECT_EQ(helper_->GetEntryCount(), 2u);
  EXPECT_EQ(helper_->GetEntry("addr1").use_count, 42);
  EXPECT_EQ(helper_->GetEntry("addr1").label, "Updated");
}

TEST_F(AutofillHelperTest, RemoveEntries_BulkRemove) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));
  helper_->AddEntry(CreateTestCard("card1", "Visa", "1234"));
  helper_->AddEntry(CreateTestEmail("email1", "user@example.com"));
  helper_->AddEntry(CreateTestAddress("addr2", "Work", "456 Oak"));

  EXPECT_EQ(helper_->GetEntryCount(), 4u);

  std::vector<std::string> ids = {"addr1", "card1"};
  helper_->RemoveEntries(ids);

  EXPECT_EQ(helper_->GetEntryCount(), 2u);
  EXPECT_TRUE(helper_->GetEntry("addr1").id.empty());
  EXPECT_TRUE(helper_->GetEntry("card1").id.empty());
  EXPECT_FALSE(helper_->GetEntry("email1").id.empty());
  EXPECT_FALSE(helper_->GetEntry("addr2").id.empty());
}

TEST_F(AutofillHelperTest, RemoveEntries_EmptyList) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));

  std::vector<std::string> ids;
  helper_->RemoveEntries(ids);

  EXPECT_EQ(helper_->GetEntryCount(), 1u);
}

TEST_F(AutofillHelperTest, RemoveEntries_WithNonexistentIds) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));

  std::vector<std::string> ids = {"addr1", "nonexistent", "also_missing"};
  helper_->RemoveEntries(ids);

  EXPECT_EQ(helper_->GetEntryCount(), 0u);
}

TEST_F(AutofillHelperTest, BlockEntries_BulkBlock) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));
  helper_->AddEntry(CreateTestCard("card1", "Visa", "1234"));
  helper_->AddEntry(CreateTestEmail("email1", "user@example.com"));

  std::vector<std::string> ids = {"addr1", "card1"};
  helper_->BlockEntries(ids);

  EXPECT_TRUE(helper_->GetEntry("addr1").is_blocked);
  EXPECT_TRUE(helper_->GetEntry("card1").is_blocked);
  EXPECT_FALSE(helper_->GetEntry("email1").is_blocked);
}

TEST_F(AutofillHelperTest, BlockEntries_EmptyList) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));

  std::vector<std::string> ids;
  helper_->BlockEntries(ids);

  EXPECT_FALSE(helper_->GetEntry("addr1").is_blocked);
}

TEST_F(AutofillHelperTest, MergeDuplicateAddresses_NoDuplicates) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));
  helper_->AddEntry(CreateTestAddress("addr2", "Work", "456 Oak Ave"));

  int merges = helper_->MergeDuplicateAddresses();
  EXPECT_EQ(merges, 0);
  EXPECT_EQ(helper_->GetAddressCount(), 2u);
}

TEST_F(AutofillHelperTest, MergeDuplicateAddresses_WithDuplicates) {
  auto addr1 = CreateTestAddress("addr1", "Home", "123 Main St");
  addr1.use_count = 5;
  helper_->AddEntry(addr1);

  auto addr2 = CreateTestAddress("addr2", "Duplicate", "123 Main St");
  addr2.use_count = 3;
  helper_->AddEntry(addr2);

  int merges = helper_->MergeDuplicateAddresses();
  EXPECT_EQ(merges, 1);
  EXPECT_EQ(helper_->GetAddressCount(), 1u);

  auto merged = helper_->GetAddresses()[0];
  EXPECT_EQ(merged.use_count, 8);  // 5 + 3
}

TEST_F(AutofillHelperTest, MergeDuplicateAddresses_CaseInsensitive) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));
  helper_->AddEntry(CreateTestAddress("addr2", "Dup", "123 MAIN ST"));

  int merges = helper_->MergeDuplicateAddresses();
  EXPECT_EQ(merges, 1);
  EXPECT_EQ(helper_->GetAddressCount(), 1u);
}

TEST_F(AutofillHelperTest, MergeDuplicateAddresses_TrimsWhitespace) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "  123 Main St  "));
  helper_->AddEntry(CreateTestAddress("addr2", "Dup", "123 Main St"));

  int merges = helper_->MergeDuplicateAddresses();
  EXPECT_EQ(merges, 1);
}

TEST_F(AutofillHelperTest, MergeDuplicateAddresses_EmptyAddresses) {
  int merges = helper_->MergeDuplicateAddresses();
  EXPECT_EQ(merges, 0);
}

TEST_F(AutofillHelperTest, MergeDuplicateAddresses_OnlyNonAddressTypes) {
  helper_->AddEntry(CreateTestCard("card1", "Visa", "1234"));
  helper_->AddEntry(CreateTestEmail("email1", "user@example.com"));

  int merges = helper_->MergeDuplicateAddresses();
  EXPECT_EQ(merges, 0);
}

// ---------------------------------------------------------------------------
// Utility methods
// ---------------------------------------------------------------------------

TEST_F(AutofillHelperTest, FormatCardNumberLastFour_Basic) {
  std::string result = AstraAutofillHelper::FormatCardNumberLastFour("1234");
  EXPECT_FALSE(result.empty());
  EXPECT_NE(result.find("1234"), std::string::npos);
}

TEST_F(AutofillHelperTest, FormatCardNumberLastFour_EmptyString) {
  std::string result = AstraAutofillHelper::FormatCardNumberLastFour("");
  EXPECT_TRUE(result.empty());
}

TEST_F(AutofillHelperTest, GetCardNetworkLabel_KnownNetworks) {
  EXPECT_EQ(AstraAutofillHelper::GetCardNetworkLabel("visa"), "Visa");
  EXPECT_EQ(AstraAutofillHelper::GetCardNetworkLabel("mastercard"), "Mastercard");
  EXPECT_EQ(AstraAutofillHelper::GetCardNetworkLabel("amex"), "American Express");
  EXPECT_EQ(AstraAutofillHelper::GetCardNetworkLabel("discover"), "Discover");
  EXPECT_EQ(AstraAutofillHelper::GetCardNetworkLabel("jcb"), "JCB");
  EXPECT_EQ(AstraAutofillHelper::GetCardNetworkLabel("diners"), "Diners Club");
  EXPECT_EQ(AstraAutofillHelper::GetCardNetworkLabel("unionpay"), "UnionPay");
}

TEST_F(AutofillHelperTest, GetCardNetworkLabel_CaseInsensitive) {
  EXPECT_EQ(AstraAutofillHelper::GetCardNetworkLabel("VISA"), "Visa");
  EXPECT_EQ(AstraAutofillHelper::GetCardNetworkLabel("Visa"), "Visa");
  EXPECT_EQ(AstraAutofillHelper::GetCardNetworkLabel("MasterCard"), "Mastercard");
}

TEST_F(AutofillHelperTest, GetCardNetworkLabel_UnknownNetwork) {
  EXPECT_EQ(AstraAutofillHelper::GetCardNetworkLabel("unknown"), "unknown");
  EXPECT_EQ(AstraAutofillHelper::GetCardNetworkLabel(""), "");
}

TEST_F(AutofillHelperTest, GetEntryTypeLabel_AllTypes) {
  EXPECT_EQ(AstraAutofillHelper::GetEntryTypeLabel(
      AstraAutofillEntryType::kAddress), "Address");
  EXPECT_EQ(AstraAutofillHelper::GetEntryTypeLabel(
      AstraAutofillEntryType::kCreditCard), "Credit Card");
  EXPECT_EQ(AstraAutofillHelper::GetEntryTypeLabel(
      AstraAutofillEntryType::kPassword), "Password");
  EXPECT_EQ(AstraAutofillHelper::GetEntryTypeLabel(
      AstraAutofillEntryType::kEmail), "Email");
  EXPECT_EQ(AstraAutofillHelper::GetEntryTypeLabel(
      AstraAutofillEntryType::kPhone), "Phone");
  EXPECT_EQ(AstraAutofillHelper::GetEntryTypeLabel(
      AstraAutofillEntryType::kName), "Name");
  EXPECT_EQ(AstraAutofillHelper::GetEntryTypeLabel(
      AstraAutofillEntryType::kCustom), "Custom");
}

TEST_F(AutofillHelperTest, MaskCardNumber_Standard16Digit) {
  std::string result = AstraAutofillHelper::MaskCardNumber("4111111111111234");
  EXPECT_NE(result.find("1234"), std::string::npos);
  EXPECT_EQ(result.front(), '\xE2');  // Bullet character starts with E2
}

TEST_F(AutofillHelperTest, MaskCardNumber_WithSpaces) {
  std::string result = AstraAutofillHelper::MaskCardNumber("4111 1111 1111 1234");
  EXPECT_NE(result.find("1234"), std::string::npos);
  EXPECT_FALSE(result.empty());
}

TEST_F(AutofillHelperTest, MaskCardNumber_ShortNumber) {
  std::string result = AstraAutofillHelper::MaskCardNumber("1234");
  // 4 digits should show as-is (no masking since all are last 4)
  EXPECT_EQ(result, "1234");
}

TEST_F(AutofillHelperTest, MaskCardNumber_EmptyString) {
  std::string result = AstraAutofillHelper::MaskCardNumber("");
  EXPECT_TRUE(result.empty());
}

TEST_F(AutofillHelperTest, MaskCardNumber_NonDigitCharacters) {
  std::string result = AstraAutofillHelper::MaskCardNumber("abcd-efgh-ijkl-1234");
  // Should still mask based on digit count and show last 4 digits
  EXPECT_NE(result.find("1234"), std::string::npos);
}

TEST_F(AutofillHelperTest, FormatExpiration_ValidDate) {
  std::string result = AstraAutofillHelper::FormatExpiration(12, 2025);
  EXPECT_EQ(result, "12/25");
}

TEST_F(AutofillHelperTest, FormatExpiration_SingleDigitMonth) {
  std::string result = AstraAutofillHelper::FormatExpiration(3, 2025);
  EXPECT_EQ(result, "03/25");
}

TEST_F(AutofillHelperTest, FormatExpiration_SingleDigitYear) {
  std::string result = AstraAutofillHelper::FormatExpiration(12, 2005);
  EXPECT_EQ(result, "12/05");
}

TEST_F(AutofillHelperTest, FormatExpiration_InvalidMonth) {
  EXPECT_TRUE(AstraAutofillHelper::FormatExpiration(0, 2025).empty());
  EXPECT_TRUE(AstraAutofillHelper::FormatExpiration(13, 2025).empty());
}

TEST_F(AutofillHelperTest, FormatExpiration_InvalidYear) {
  EXPECT_TRUE(AstraAutofillHelper::FormatExpiration(5, 0).empty());
  EXPECT_TRUE(AstraAutofillHelper::FormatExpiration(5, -1).empty());
}

TEST_F(AutofillHelperTest, TruncateSuggestion_ShortText) {
  std::string result = AstraAutofillHelper::TruncateSuggestion("Hello", 100);
  EXPECT_EQ(result, "Hello");
}

TEST_F(AutofillHelperTest, TruncateSuggestion_ExactLength) {
  std::string result = AstraAutofillHelper::TruncateSuggestion("Hello", 5);
  EXPECT_EQ(result, "Hello");
}

TEST_F(AutofillHelperTest, TruncateSuggestion_LongText) {
  std::string result = AstraAutofillHelper::TruncateSuggestion(
      "This is a very long suggestion text", 10);
  EXPECT_LT(result.size(), std::string("This is a very long suggestion text").size());
  // Should end with ellipsis
  EXPECT_TRUE(result.size() > 10);  // Ellipsis adds some chars
}

TEST_F(AutofillHelperTest, TruncateSuggestion_ZeroMaxLength) {
  std::string result = AstraAutofillHelper::TruncateSuggestion("Hello", 0);
  EXPECT_EQ(result, "Hello");
}

TEST_F(AutofillHelperTest, TruncateSuggestion_NegativeMaxLength) {
  std::string result = AstraAutofillHelper::TruncateSuggestion("Hello", -5);
  EXPECT_EQ(result, "Hello");
}

TEST_F(AutofillHelperTest, TruncateSuggestion_EmptyString) {
  std::string result = AstraAutofillHelper::TruncateSuggestion("", 10);
  EXPECT_TRUE(result.empty());
}

// ---------------------------------------------------------------------------
// Persistence round-trip via PrefService
// ---------------------------------------------------------------------------

TEST_F(AutofillHelperTest, Persistence_SettingPersistsAfterHelperRecreate) {
  helper_->SetAutofillEnabled(false);
  helper_->SetMaxSuggestions(10);
  helper_->SetSuggestionsSortOrder("alphabetical");
  helper_->SetAutofillQuickCheckout(true);

  // Create a new helper using the same profile.
  auto helper2 = std::make_unique<AstraAutofillHelper>(profile_.get());

  EXPECT_FALSE(helper2->IsAutofillEnabled());
  EXPECT_EQ(helper2->GetMaxSuggestions(), 10);
  EXPECT_EQ(helper2->GetSuggestionsSortOrder(), "alphabetical");
  EXPECT_TRUE(helper2->GetAutofillQuickCheckout());
}

TEST_F(AutofillHelperTest, Persistence_AllBooleanSettingsPersist) {
  helper_->SetAutofillEnabled(false);
  helper_->SetAddressAutofillEnabled(false);
  helper_->SetCreditCardAutofillEnabled(false);
  helper_->SetAutosignInEnabled(false);
  helper_->SetShowAutofillPopup(false);
  helper_->SetShowSuggestionIcons(false);
  helper_->SetShowSuggestionLabels(false);
  helper_->SetShowSuggestionSubtext(false);
  helper_->SetAutofillOnTap(false);
  helper_->SetShowCreditCardIcons(false);
  helper_->SetAutofillQuickCheckout(true);

  auto helper2 = std::make_unique<AstraAutofillHelper>(profile_.get());

  EXPECT_FALSE(helper2->IsAutofillEnabled());
  EXPECT_FALSE(helper2->IsAddressAutofillEnabled());
  EXPECT_FALSE(helper2->IsCreditCardAutofillEnabled());
  EXPECT_FALSE(helper2->IsAutosignInEnabled());
  EXPECT_FALSE(helper2->GetShowAutofillPopup());
  EXPECT_FALSE(helper2->GetShowSuggestionIcons());
  EXPECT_FALSE(helper2->GetShowSuggestionLabels());
  EXPECT_FALSE(helper2->GetShowSuggestionSubtext());
  EXPECT_FALSE(helper2->GetAutofillOnTap());
  EXPECT_FALSE(helper2->GetShowCreditCardIcons());
  EXPECT_TRUE(helper2->GetAutofillQuickCheckout());
}

// ---------------------------------------------------------------------------
// Edge cases
// ---------------------------------------------------------------------------

TEST_F(AutofillHelperTest, EdgeCase_EmptyEntriesList) {
  EXPECT_EQ(helper_->GetEntryCount(), 0u);
  EXPECT_TRUE(helper_->GetAllEntries().empty());
  EXPECT_TRUE(helper_->GetAddresses().empty());
  EXPECT_TRUE(helper_->GetCreditCards().empty());
  EXPECT_TRUE(helper_->GetSuggestions("email", 5).empty());
  EXPECT_TRUE(helper_->GetRecentlyUsed(5).empty());
  EXPECT_TRUE(helper_->GetMostUsed(5).empty());
}

TEST_F(AutofillHelperTest, EdgeCase_InvalidIds) {
  // Should not crash with empty/null IDs.
  helper_->AddEntry(AstraAutofillEntry());
  helper_->GetEntry("");
  helper_->UpdateEntry("", AstraAutofillEntry());
  helper_->RemoveEntry("");
  helper_->BlockEntry("");
  helper_->UnblockEntry("");

  EXPECT_EQ(helper_->GetEntryCount(), 0u);
}

TEST_F(AutofillHelperTest, EdgeCase_ZeroMaxSuggestions) {
  auto suggestions = helper_->GetSuggestions("email", 0);
  // Zero should use default, which is > 0.
  EXPECT_LE(suggestions.size(),
            static_cast<size_t>(prefs::kDefaultAutofillMaxSuggestions));
}

TEST_F(AutofillHelperTest, EdgeCase_NegativeMaxSuggestions) {
  auto suggestions = helper_->GetSuggestions("email", -10);
  EXPECT_LE(suggestions.size(),
            static_cast<size_t>(prefs::kDefaultAutofillMaxSuggestions));
}

TEST_F(AutofillHelperTest, EdgeCase_LargeMaxSuggestionsClamped) {
  helper_->AddEntry(CreateTestEmail("email1", "user@example.com"));

  auto suggestions = helper_->GetSuggestions("email", 1000);
  // Should be clamped to max (20).
  EXPECT_LE(suggestions.size(), 20u);
}

TEST_F(AutofillHelperTest, EdgeCase_DuplicateEntriesHandled) {
  // Adding same ID multiple times is handled as update.
  for (int i = 0; i < 10; ++i) {
    auto entry = CreateTestAddress("addr1", "Home", "123 Main St");
    entry.use_count = i;
    helper_->AddEntry(entry);
  }

  EXPECT_EQ(helper_->GetEntryCount(), 1u);
  EXPECT_EQ(helper_->GetEntry("addr1").use_count, 9);
}

TEST_F(AutofillHelperTest, EdgeCase_ClearEmptyNoCrash) {
  helper_->ClearAllEntries();
  helper_->ClearAddresses();
  helper_->ClearCreditCards();
  // Should not crash.
}

TEST_F(AutofillHelperTest, EdgeCase_LargeNumberOfEntries) {
  for (int i = 0; i < 1000; ++i) {
    helper_->AddEntry(CreateTestEmail(
        "email" + std::to_string(i),
        "user" + std::to_string(i) + "@example.com"));
  }

  EXPECT_EQ(helper_->GetEntryCount(), 1000u);

  auto suggestions = helper_->GetSuggestions("email", 10);
  EXPECT_EQ(suggestions.size(), 10u);
}

// ---------------------------------------------------------------------------
// Sort order behavior
// ---------------------------------------------------------------------------

TEST_F(AutofillHelperTest, SortOrder_MostRecentDefault) {
  // Default sort order should be "most_recent".
  EXPECT_EQ(helper_->GetSuggestionsSortOrder(), "most_recent");
}

TEST_F(AutofillHelperTest, SortOrder_MostUsed) {
  helper_->SetSuggestionsSortOrder("most_used");

  auto entry1 = CreateTestEmail("email1", "low@example.com");
  entry1.use_count = 1;
  entry1.last_used = base::Time::Now();  // Very recent
  helper_->AddEntry(entry1);

  auto entry2 = CreateTestEmail("email2", "high@example.com");
  entry2.use_count = 100;
  entry2.last_used = base::Time::Now() - base::Days(365);  // Very old
  helper_->AddEntry(entry2);

  auto suggestions = helper_->GetSuggestions("email", 10);
  ASSERT_EQ(suggestions.size(), 2u);
  // Most used should be first.
  EXPECT_EQ(suggestions[0].id, "email2");
  EXPECT_EQ(suggestions[1].id, "email1");
}

TEST_F(AutofillHelperTest, SortOrder_Alphabetical) {
  helper_->SetSuggestionsSortOrder("alphabetical");

  helper_->AddEntry(CreateTestEmail("email1", "zebra@example.com"));
  helper_->AddEntry(CreateTestEmail("email2", "apple@example.com"));
  helper_->AddEntry(CreateTestEmail("email3", "mango@example.com"));

  auto suggestions = helper_->GetSuggestions("email", 10);
  ASSERT_EQ(suggestions.size(), 3u);
  EXPECT_EQ(suggestions[0].label, "apple@example.com");
  EXPECT_EQ(suggestions[1].label, "mango@example.com");
  EXPECT_EQ(suggestions[2].label, "zebra@example.com");
}

TEST_F(AutofillHelperTest, SortOrder_MostRecent) {
  helper_->SetSuggestionsSortOrder("most_recent");

  auto entry1 = CreateTestEmail("email1", "old@example.com");
  entry1.last_used = base::Time::Now() - base::Days(30);
  helper_->AddEntry(entry1);

  auto entry2 = CreateTestEmail("email2", "new@example.com");
  entry2.last_used = base::Time::Now();
  helper_->AddEntry(entry2);

  auto suggestions = helper_->GetSuggestions("email", 10);
  ASSERT_EQ(suggestions.size(), 2u);
  EXPECT_EQ(suggestions[0].id, "email2");
  EXPECT_EQ(suggestions[1].id, "email1");
}

// ---------------------------------------------------------------------------
// Struct defaults
// ---------------------------------------------------------------------------

TEST_F(AutofillHelperTest, StructDefaults_Entry) {
  AstraAutofillEntry entry;
  EXPECT_TRUE(entry.id.empty());
  EXPECT_EQ(entry.type, AstraAutofillEntryType::kCustom);
  EXPECT_TRUE(entry.label.empty());
  EXPECT_TRUE(entry.value.empty());
  EXPECT_TRUE(entry.last_used.is_null());
  EXPECT_EQ(entry.use_count, 0);
  EXPECT_FALSE(entry.is_blocked);
  EXPECT_TRUE(entry.profile_id.empty());
}

TEST_F(AutofillHelperTest, StructDefaults_AddressEntry) {
  AstraAutofillAddressEntry entry;
  EXPECT_TRUE(entry.base.id.empty());
  EXPECT_TRUE(entry.full_name.empty());
  EXPECT_TRUE(entry.company.empty());
  EXPECT_TRUE(entry.street_address.empty());
  EXPECT_TRUE(entry.city.empty());
  EXPECT_TRUE(entry.state.empty());
  EXPECT_TRUE(entry.zip_code.empty());
  EXPECT_TRUE(entry.country.empty());
  EXPECT_TRUE(entry.phone_number.empty());
  EXPECT_TRUE(entry.email.empty());
}

TEST_F(AutofillHelperTest, StructDefaults_CreditCardEntry) {
  AstraAutofillCreditCardEntry entry;
  EXPECT_TRUE(entry.base.id.empty());
  EXPECT_TRUE(entry.card_number_last_four.empty());
  EXPECT_TRUE(entry.cardholder_name.empty());
  EXPECT_EQ(entry.expiration_month, 0);
  EXPECT_EQ(entry.expiration_year, 0);
  EXPECT_TRUE(entry.card_network.empty());
  EXPECT_FALSE(entry.is_virtual);
  EXPECT_TRUE(entry.billing_address_id.empty());
}

// ---------------------------------------------------------------------------
// Shutdown cleanup
// ---------------------------------------------------------------------------

TEST_F(AutofillHelperTest, Shutdown_ClearsProfile) {
  // After shutdown, settings operations should not crash but should use defaults.
  helper_->Shutdown();

  // These should return defaults since profile_ is nullptr after shutdown.
  EXPECT_EQ(helper_->IsAutofillEnabled(), prefs::kDefaultAutofillEnabled);
  EXPECT_EQ(helper_->GetMaxSuggestions(),
            prefs::kDefaultAutofillMaxSuggestions);
}

TEST_F(AutofillHelperTest, Shutdown_DoesNotCrashMultipleTimes) {
  helper_->Shutdown();
  helper_->Shutdown();  // Second call should be safe
}

TEST_F(AutofillHelperTest, Shutdown_SettingChangesNoOp) {
  helper_->Shutdown();

  // Setting changes after shutdown should not crash.
  helper_->SetAutofillEnabled(false);
  helper_->SetMaxSuggestions(10);
  helper_->SetSuggestionsSortOrder("alphabetical");
}

// ---------------------------------------------------------------------------
// Factory tests
// ---------------------------------------------------------------------------

TEST(AutofillHelperFactoryTest, GetInstance_ReturnsValid) {
  auto* factory = AstraAutofillHelperFactory::GetInstance();
  EXPECT_NE(factory, nullptr);
}

TEST(AutofillHelperFactoryTest, GetInstance_Singleton) {
  auto* factory1 = AstraAutofillHelperFactory::GetInstance();
  auto* factory2 = AstraAutofillHelperFactory::GetInstance();
  EXPECT_EQ(factory1, factory2);
}

TEST(AutofillHelperFactoryTest, GetForProfile_WithNullProfile) {
  auto* helper = AstraAutofillHelperFactory::GetForProfile(nullptr);
  EXPECT_EQ(helper, nullptr);
}

TEST(AutofillHelperFactoryTest, GetForProfile_WithTestingProfile) {
  base::test::TaskEnvironment task_environment;
  auto profile = std::make_unique<TestingProfile>();
  prefs::RegisterProfilePrefs(profile->GetPrefs());

  auto* helper = AstraAutofillHelperFactory::GetForProfile(profile.get());
  EXPECT_NE(helper, nullptr);
}

TEST(AutofillHelperFactoryTest, RegisterProfilePrefs_DoesNotCrash) {
  auto registry = base::MakeRefCounted<PrefRegistrySimple>();
  AstraAutofillHelperFactory::RegisterProfilePrefs(registry.get());
  // Should not crash and should register all prefs.
}

TEST(AutofillHelperFactoryTest, RegisterProfilePrefs_SetsDefaults) {
  auto registry = base::MakeRefCounted<PrefRegistrySimple>();
  AstraAutofillHelperFactory::RegisterProfilePrefs(registry.get());

  // Check that defaults match the expected values.
  TestingProfile::Builder builder;
  builder.SetPrefRegistry(registry);
  auto profile = builder.Build();

  auto* prefs = profile->GetPrefs();
  EXPECT_TRUE(prefs->GetBoolean(prefs::kPrefAutofillEnabled));
  EXPECT_TRUE(prefs->GetBoolean(prefs::kPrefAddressAutofillEnabled));
  EXPECT_TRUE(prefs->GetBoolean(prefs::kPrefCreditCardAutofillEnabled));
  EXPECT_TRUE(prefs->GetBoolean(prefs::kPrefAutosignInEnabled));
  EXPECT_TRUE(prefs->GetBoolean(prefs::kPrefShowAutofillPopup));
  EXPECT_EQ(prefs->GetString(prefs::kPrefAutofillPopupPosition), "auto");
  EXPECT_EQ(prefs->GetInteger(prefs::kPrefAutofillMaxSuggestions), 6);
  EXPECT_TRUE(prefs->GetBoolean(prefs::kPrefShowSuggestionIcons));
  EXPECT_TRUE(prefs->GetBoolean(prefs::kPrefShowSuggestionLabels));
  EXPECT_TRUE(prefs->GetBoolean(prefs::kPrefShowSuggestionSubtext));
  EXPECT_TRUE(prefs->GetBoolean(prefs::kPrefAutofillOnTap));
  EXPECT_EQ(prefs->GetString(prefs::kPrefAutofillSuggestionsSortOrder),
            "most_recent");
  EXPECT_TRUE(prefs->GetBoolean(prefs::kPrefShowCreditCardIcons));
  EXPECT_FALSE(prefs->GetBoolean(prefs::kPrefAutofillQuickCheckout));
}

// ---------------------------------------------------------------------------
// Additional edge case tests
// ---------------------------------------------------------------------------

TEST_F(AutofillHelperTest, ClearAddresses_NoAddressesNoCrash) {
  helper_->AddEntry(CreateTestCard("card1", "Visa", "1234"));
  helper_->AddEntry(CreateTestEmail("email1", "user@example.com"));

  helper_->ClearAddresses();
  EXPECT_EQ(helper_->GetEntryCount(), 2u);
}

TEST_F(AutofillHelperTest, ClearCreditCards_NoCardsNoCrash) {
  helper_->AddEntry(CreateTestAddress("addr1", "Home", "123 Main St"));
  helper_->AddEntry(CreateTestEmail("email1", "user@example.com"));

  helper_->ClearCreditCards();
  EXPECT_EQ(helper_->GetEntryCount(), 2u);
}

TEST_F(AutofillHelperTest, GetEntry_NonexistentReturnsDefault) {
  auto entry = helper_->GetEntry("does_not_exist");
  EXPECT_TRUE(entry.id.empty());
  EXPECT_EQ(entry.type, AstraAutofillEntryType::kCustom);
}

TEST_F(AutofillHelperTest, AllEntryTypes) {
  // Test that all entry types can be added and retrieved.
  std::vector<std::pair<AstraAutofillEntryType, std::string>> types = {
      {AstraAutofillEntryType::kAddress, "addr"},
      {AstraAutofillEntryType::kCreditCard, "card"},
      {AstraAutofillEntryType::kPassword, "pass"},
      {AstraAutofillEntryType::kEmail, "email"},
      {AstraAutofillEntryType::kPhone, "phone"},
      {AstraAutofillEntryType::kName, "name"},
      {AstraAutofillEntryType::kCustom, "custom"},
  };

  for (const auto& [type, id] : types) {
    AstraAutofillEntry entry;
    entry.id = id;
    entry.type = type;
    entry.label = id + "_label";
    entry.value = id + "_value";
    helper_->AddEntry(entry);
  }

  EXPECT_EQ(helper_->GetEntryCount(), 7u);

  for (const auto& [type, id] : types) {
    auto entry = helper_->GetEntry(id);
    EXPECT_EQ(entry.id, id);
    EXPECT_EQ(entry.type, type);
  }
}

TEST_F(AutofillHelperTest, BlockEntry_BlockedExcludedFromSuggestions) {
  auto entry = CreateTestEmail("email1", "blocked@example.com");
  helper_->AddEntry(entry);
  helper_->BlockEntry("email1");

  auto suggestions = helper_->GetSuggestions("email", 10);
  EXPECT_TRUE(suggestions.empty());
}

TEST_F(AutofillHelperTest, UnblockEntry_ReappearsInSuggestions) {
  auto entry = CreateTestEmail("email1", "test@example.com");
  entry.is_blocked = true;
  helper_->AddEntry(entry);

  EXPECT_TRUE(helper_->GetSuggestions("email", 10).empty());

  helper_->UnblockEntry("email1");
  EXPECT_EQ(helper_->GetSuggestions("email", 10).size(), 1u);
}

TEST_F(AutofillHelperTest, SettingsChanged_OnlyOnActualChange) {
  TestAutofillObserver observer;
  helper_->AddObserver(&observer);

  // Same value should not trigger notification.
  helper_->SetAutofillEnabled(true);  // Already true
  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->SetMaxSuggestions(6);  // Already default
  EXPECT_EQ(observer.settings_changed_count_, 0);

  // Changing value should trigger notification.
  helper_->SetAutofillEnabled(false);
  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

}  // namespace astra
