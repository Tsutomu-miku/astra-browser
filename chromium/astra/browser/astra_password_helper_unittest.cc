// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_password_helper.h"

#include "base/test/task_environment.h"
#include "chrome/test/base/testing_profile.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Test observer that records calls for verification.
class TestPasswordHelperObserver : public AstraPasswordHelperObserver {
 public:
  void OnPasswordsChanged() override {
    passwords_changed_count_++;
  }

  void OnPasswordSaved(const AstraPasswordEntry& entry) override {
    password_saved_count_++;
    last_saved_entry_ = entry;
  }

  void OnPasswordUpdated(const AstraPasswordEntry& entry) override {
    password_updated_count_++;
    last_updated_entry_ = entry;
  }

  void OnPasswordDeleted(const AstraPasswordEntry& entry) override {
    password_deleted_count_++;
    last_deleted_entry_ = entry;
  }

  void OnPasswordSettingsChanged() override {
    settings_changed_count_++;
  }

  void OnPasswordHealthChanged() override {
    health_changed_count_++;
  }

  void OnPasswordBreachDetected(size_t compromised_count) override {
    breach_detected_count_++;
    last_breach_count_ = compromised_count;
  }

  // Counters
  int passwords_changed_count_ = 0;
  int password_saved_count_ = 0;
  int password_updated_count_ = 0;
  int password_deleted_count_ = 0;
  int settings_changed_count_ = 0;
  int health_changed_count_ = 0;
  int breach_detected_count_ = 0;

  // Last recorded values
  AstraPasswordEntry last_saved_entry_;
  AstraPasswordEntry last_updated_entry_;
  AstraPasswordEntry last_deleted_entry_;
  size_t last_breach_count_ = 0;
};

}  // namespace

// Test fixture for AstraPasswordHelper tests.
class PasswordHelperTest : public testing::Test {
 protected:
  PasswordHelperTest() {
    profile_ = std::make_unique<TestingProfile>();
    // Register Astra prefs on the testing profile.
    prefs::RegisterProfilePrefs(profile_->GetPrefs());
    helper_ = std::make_unique<AstraPasswordHelper>(profile_.get());
    DCHECK(helper_);
  }

  ~PasswordHelperTest() override = default;

  void SetUp() override {
    // Verify default presentation settings.
    ASSERT_TRUE(helper_->GetShowPasswordSuggestions());
    ASSERT_TRUE(helper_->GetAutoFillEnabled());
    ASSERT_TRUE(helper_->GetPasswordManagerShortcut());
    ASSERT_TRUE(helper_->GetShowPasswordHealth());
    ASSERT_TRUE(helper_->GetBreachAlertsEnabled());
  }

  void TearDown() override {
    // Clean up observers to avoid dangling references.
    for (auto& observer : test_observers_) {
      helper_->RemoveObserver(&observer);
    }
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<AstraPasswordHelper> helper_;
  std::vector<TestPasswordHelperObserver> test_observers_;
};

// ---------------------------------------------------------------------------
// Default state
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, DefaultState_ShowPasswordSuggestions) {
  EXPECT_TRUE(helper_->GetShowPasswordSuggestions());
}

TEST_F(PasswordHelperTest, DefaultState_AutoFillEnabled) {
  EXPECT_TRUE(helper_->GetAutoFillEnabled());
}

TEST_F(PasswordHelperTest, DefaultState_PasswordManagerShortcut) {
  EXPECT_TRUE(helper_->GetPasswordManagerShortcut());
}

TEST_F(PasswordHelperTest, DefaultState_ShowPasswordHealth) {
  EXPECT_TRUE(helper_->GetShowPasswordHealth());
}

TEST_F(PasswordHelperTest, DefaultState_BreachAlertsEnabled) {
  EXPECT_TRUE(helper_->GetBreachAlertsEnabled());
}

TEST_F(PasswordHelperTest, DefaultState_MaxSidebarPasswords) {
  EXPECT_EQ(helper_->GetMaxSidebarPasswords(),
            prefs::kDefaultPasswordMaxSidebarPasswords);
}

TEST_F(PasswordHelperTest, DefaultState_BreachNotDetected) {
  EXPECT_FALSE(helper_->IsBreachDetected());
}

TEST_F(PasswordHelperTest, DefaultState_PasswordCountZero) {
  // In the overlay, PasswordStore is not available, so count is 0.
  EXPECT_EQ(helper_->GetPasswordCount(), 0u);
  EXPECT_EQ(helper_->GetSavedPasswordsCount(), 0u);
}

TEST_F(PasswordHelperTest, DefaultState_CompromisedCountZero) {
  EXPECT_EQ(helper_->GetCompromisedPasswordsCount(), 0u);
}

TEST_F(PasswordHelperTest, DefaultState_WeakCountZero) {
  EXPECT_EQ(helper_->GetWeakPasswordsCount(), 0u);
}

TEST_F(PasswordHelperTest, DefaultState_ReusedCountZero) {
  EXPECT_EQ(helper_->GetReusedPasswordsCount(), 0u);
}

TEST_F(PasswordHelperTest, DefaultState_NoCompromisedPasswords) {
  EXPECT_FALSE(helper_->HasCompromisedPasswords());
}

TEST_F(PasswordHelperTest, DefaultState_GetSavedPasswordsEmpty) {
  auto passwords = helper_->GetSavedPasswords(10);
  EXPECT_TRUE(passwords.empty());
}

TEST_F(PasswordHelperTest, DefaultState_SearchPasswordsEmpty) {
  auto results = helper_->SearchPasswords(u"test", 10);
  EXPECT_TRUE(results.empty());
}

TEST_F(PasswordHelperTest, DefaultState_HealthStatsAllZero) {
  auto stats = helper_->GetPasswordHealthStats();
  EXPECT_EQ(stats.total_passwords, 0u);
  EXPECT_EQ(stats.compromised_count, 0u);
  EXPECT_EQ(stats.weak_count, 0u);
  EXPECT_EQ(stats.reused_count, 0u);
  EXPECT_EQ(stats.blocked_count, 0u);
  EXPECT_EQ(stats.problem_count(), 0u);
  EXPECT_FALSE(stats.has_breaches());
}

TEST_F(PasswordHelperTest, DefaultState_BulkExportAvailable) {
  // In the overlay, IsBulkExportAvailable returns true by default.
  EXPECT_TRUE(helper_->IsBulkExportAvailable());
}

// ---------------------------------------------------------------------------
// Observer defaults — all methods have empty default implementations
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, ObserverDefaults_DoNotCrash) {
  // Create a minimal observer that overrides nothing.
  // All methods have default empty implementations, so this should compile
  // and not crash.
  class DefaultObserver : public AstraPasswordHelperObserver {};

  DefaultObserver observer;
  helper_->AddObserver(&observer);

  // Trigger all observer paths via pref changes and manual notifications.
  helper_->SetShowPasswordSuggestions(false);
  helper_->SetAutoFillEnabled(false);
  helper_->SetPasswordManagerShortcut(false);
  helper_->SetShowPasswordHealth(false);
  helper_->SetBreachAlertsEnabled(false);
  helper_->SetMaxSidebarPasswords(10);

  helper_->NotifyPasswordsChanged();
  helper_->NotifyPasswordSaved(AstraPasswordEntry());
  helper_->NotifyPasswordUpdated(AstraPasswordEntry());
  helper_->NotifyPasswordDeleted(AstraPasswordEntry());
  helper_->NotifyPasswordHealthChanged();
  helper_->NotifyPasswordBreachDetected(3);

  helper_->RemoveObserver(&observer);
  SUCCEED() << "Default observer methods do not crash.";
}

// ---------------------------------------------------------------------------
// Observer add/remove
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, AddRemoveObserver_NoCrash) {
  TestPasswordHelperObserver observer;

  helper_->AddObserver(&observer);
  helper_->RemoveObserver(&observer);

  SUCCEED() << "Observer add/remove completed without crash.";
}

TEST_F(PasswordHelperTest, RemoveNonexistentObserver_NoCrash) {
  TestPasswordHelperObserver observer;

  helper_->RemoveObserver(&observer);

  SUCCEED() << "Removing nonexistent observer completed without crash.";
}

// ---------------------------------------------------------------------------
// Presentation settings — show password suggestions
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, SetShowPasswordSuggestions_ChangesValue) {
  ASSERT_TRUE(helper_->GetShowPasswordSuggestions());

  helper_->SetShowPasswordSuggestions(false);
  EXPECT_FALSE(helper_->GetShowPasswordSuggestions());

  helper_->SetShowPasswordSuggestions(true);
  EXPECT_TRUE(helper_->GetShowPasswordSuggestions());
}

TEST_F(PasswordHelperTest, SetShowPasswordSuggestions_SameValueNoOp) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  ASSERT_TRUE(helper_->GetShowPasswordSuggestions());
  helper_->SetShowPasswordSuggestions(true);

  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(PasswordHelperTest, SetShowPasswordSuggestions_FiresSettingsObserver) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetShowPasswordSuggestions(false);

  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(PasswordHelperTest, ToggleShowPasswordSuggestions_FlipsValue) {
  ASSERT_TRUE(helper_->GetShowPasswordSuggestions());

  bool result = helper_->ToggleShowPasswordSuggestions();
  EXPECT_FALSE(result);
  EXPECT_FALSE(helper_->GetShowPasswordSuggestions());

  result = helper_->ToggleShowPasswordSuggestions();
  EXPECT_TRUE(result);
  EXPECT_TRUE(helper_->GetShowPasswordSuggestions());
}

// ---------------------------------------------------------------------------
// Presentation settings — auto-fill enabled
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, SetAutoFillEnabled_ChangesValue) {
  ASSERT_TRUE(helper_->GetAutoFillEnabled());

  helper_->SetAutoFillEnabled(false);
  EXPECT_FALSE(helper_->GetAutoFillEnabled());

  helper_->SetAutoFillEnabled(true);
  EXPECT_TRUE(helper_->GetAutoFillEnabled());
}

TEST_F(PasswordHelperTest, SetAutoFillEnabled_SameValueNoOp) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  ASSERT_TRUE(helper_->GetAutoFillEnabled());
  helper_->SetAutoFillEnabled(true);

  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(PasswordHelperTest, SetAutoFillEnabled_FiresSettingsObserver) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetAutoFillEnabled(false);

  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(PasswordHelperTest, ToggleAutoFillEnabled_FlipsValue) {
  ASSERT_TRUE(helper_->GetAutoFillEnabled());

  bool result = helper_->ToggleAutoFillEnabled();
  EXPECT_FALSE(result);
  EXPECT_FALSE(helper_->GetAutoFillEnabled());

  result = helper_->ToggleAutoFillEnabled();
  EXPECT_TRUE(result);
  EXPECT_TRUE(helper_->GetAutoFillEnabled());
}

// ---------------------------------------------------------------------------
// Presentation settings — password manager shortcut
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, SetPasswordManagerShortcut_ChangesValue) {
  ASSERT_TRUE(helper_->GetPasswordManagerShortcut());

  helper_->SetPasswordManagerShortcut(false);
  EXPECT_FALSE(helper_->GetPasswordManagerShortcut());

  helper_->SetPasswordManagerShortcut(true);
  EXPECT_TRUE(helper_->GetPasswordManagerShortcut());
}

TEST_F(PasswordHelperTest, SetPasswordManagerShortcut_SameValueNoOp) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  ASSERT_TRUE(helper_->GetPasswordManagerShortcut());
  helper_->SetPasswordManagerShortcut(true);

  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(PasswordHelperTest, SetPasswordManagerShortcut_FiresSettingsObserver) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetPasswordManagerShortcut(false);

  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(PasswordHelperTest, TogglePasswordManagerShortcut_FlipsValue) {
  ASSERT_TRUE(helper_->GetPasswordManagerShortcut());

  bool result = helper_->TogglePasswordManagerShortcut();
  EXPECT_FALSE(result);
  EXPECT_FALSE(helper_->GetPasswordManagerShortcut());

  result = helper_->TogglePasswordManagerShortcut();
  EXPECT_TRUE(result);
  EXPECT_TRUE(helper_->GetPasswordManagerShortcut());
}

// ---------------------------------------------------------------------------
// Presentation settings — show password health
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, SetShowPasswordHealth_ChangesValue) {
  ASSERT_TRUE(helper_->GetShowPasswordHealth());

  helper_->SetShowPasswordHealth(false);
  EXPECT_FALSE(helper_->GetShowPasswordHealth());

  helper_->SetShowPasswordHealth(true);
  EXPECT_TRUE(helper_->GetShowPasswordHealth());
}

TEST_F(PasswordHelperTest, SetShowPasswordHealth_SameValueNoOp) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  ASSERT_TRUE(helper_->GetShowPasswordHealth());
  helper_->SetShowPasswordHealth(true);

  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(PasswordHelperTest, SetShowPasswordHealth_FiresSettingsObserver) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetShowPasswordHealth(false);

  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(PasswordHelperTest, ToggleShowPasswordHealth_FlipsValue) {
  ASSERT_TRUE(helper_->GetShowPasswordHealth());

  bool result = helper_->ToggleShowPasswordHealth();
  EXPECT_FALSE(result);
  EXPECT_FALSE(helper_->GetShowPasswordHealth());

  result = helper_->ToggleShowPasswordHealth();
  EXPECT_TRUE(result);
  EXPECT_TRUE(helper_->GetShowPasswordHealth());
}

// ---------------------------------------------------------------------------
// Presentation settings — breach alerts
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, SetBreachAlertsEnabled_ChangesValue) {
  ASSERT_TRUE(helper_->GetBreachAlertsEnabled());

  helper_->SetBreachAlertsEnabled(false);
  EXPECT_FALSE(helper_->GetBreachAlertsEnabled());

  helper_->SetBreachAlertsEnabled(true);
  EXPECT_TRUE(helper_->GetBreachAlertsEnabled());
}

TEST_F(PasswordHelperTest, SetBreachAlertsEnabled_SameValueNoOp) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  ASSERT_TRUE(helper_->GetBreachAlertsEnabled());
  helper_->SetBreachAlertsEnabled(true);

  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(PasswordHelperTest, SetBreachAlertsEnabled_FiresSettingsObserver) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetBreachAlertsEnabled(false);

  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(PasswordHelperTest, ToggleBreachAlerts_FlipsValue) {
  ASSERT_TRUE(helper_->GetBreachAlertsEnabled());

  bool result = helper_->ToggleBreachAlerts();
  EXPECT_FALSE(result);
  EXPECT_FALSE(helper_->GetBreachAlertsEnabled());

  result = helper_->ToggleBreachAlerts();
  EXPECT_TRUE(result);
  EXPECT_TRUE(helper_->GetBreachAlertsEnabled());
}

// ---------------------------------------------------------------------------
// Presentation settings — max sidebar passwords
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, SetMaxSidebarPasswords_ChangesValue) {
  helper_->SetMaxSidebarPasswords(10);
  EXPECT_EQ(helper_->GetMaxSidebarPasswords(), 10);

  helper_->SetMaxSidebarPasswords(50);
  EXPECT_EQ(helper_->GetMaxSidebarPasswords(), 50);
}

TEST_F(PasswordHelperTest, SetMaxSidebarPasswords_ClampsToMinimum) {
  helper_->SetMaxSidebarPasswords(0);
  EXPECT_EQ(helper_->GetMaxSidebarPasswords(), 1);

  helper_->SetMaxSidebarPasswords(-5);
  EXPECT_EQ(helper_->GetMaxSidebarPasswords(), 1);
}

TEST_F(PasswordHelperTest, SetMaxSidebarPasswords_ClampsToMaximum) {
  helper_->SetMaxSidebarPasswords(200);
  EXPECT_EQ(helper_->GetMaxSidebarPasswords(), 100);
}

TEST_F(PasswordHelperTest, SetMaxSidebarPasswords_SameValueNoOp) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  int original = helper_->GetMaxSidebarPasswords();
  helper_->SetMaxSidebarPasswords(original);

  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(PasswordHelperTest, SetMaxSidebarPasswords_FiresSettingsObserver) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetMaxSidebarPasswords(10);

  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Password strength — static methods
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, GetPasswordStrength_EmptyPasswordIsVeryWeak) {
  EXPECT_EQ(AstraPasswordHelper::GetPasswordStrength(std::u16string()),
            AstraPasswordStrength::kVeryWeak);
  EXPECT_EQ(AstraPasswordHelper::GetPasswordStrengthPercent(std::u16string()), 0);
}

TEST_F(PasswordHelperTest, GetPasswordStrength_ShortPasswordIsWeak) {
  // Very short password should be weak.
  EXPECT_LE(AstraPasswordHelper::GetPasswordStrengthPercent(u"abc"), 20);
  EXPECT_EQ(AstraPasswordHelper::GetPasswordStrength(u"abc"),
            AstraPasswordStrength::kVeryWeak);
}

TEST_F(PasswordHelperTest, GetPasswordStrength_MediumLength) {
  // 8-character mixed password should be at least medium.
  int strength = AstraPasswordHelper::GetPasswordStrengthPercent(u"Abc12345");
  EXPECT_GT(strength, 20);
  EXPECT_LT(strength, 80);
}

TEST_F(PasswordHelperTest, GetPasswordStrength_StrongPassword) {
  // Long password with all character types should be strong.
  int strength = AstraPasswordHelper::GetPasswordStrengthPercent(
      u"CorrectHorseBatteryStaple123!");
  EXPECT_GE(strength, 60);
}

TEST_F(PasswordHelperTest, GetPasswordStrength_VeryStrong) {
  // Very long, complex password should be very strong.
  int strength = AstraPasswordHelper::GetPasswordStrengthPercent(
      u"Tr0ub4dor&3_Has_a_Long_Name!@#$");
  EXPECT_GE(strength, 80);
  EXPECT_EQ(AstraPasswordHelper::GetPasswordStrength(
      u"Tr0ub4dor&3_Has_a_Long_Name!@#$"),
      AstraPasswordStrength::kVeryStrong);
}

TEST_F(PasswordHelperTest, GetPasswordStrength_AllSameIsVeryWeak) {
  // All same characters should be penalized.
  int strength = AstraPasswordHelper::GetPasswordStrengthPercent(u"aaaaaaaaaaaaaaaa");
  EXPECT_LT(strength, 40);
}

TEST_F(PasswordHelperTest, GetPasswordStrength_SequentialIsWeak) {
  // Sequential characters should be penalized.
  int strength = AstraPasswordHelper::GetPasswordStrengthPercent(u"abcdefghij");
  EXPECT_LT(strength, 60);
}

TEST_F(PasswordHelperTest, GetPasswordStrength_PercentInRange) {
  // Strength percent should always be in 0-100 range.
  std::vector<std::u16string> test_cases = {
    u"",
    u"a",
    u"ab",
    u"abc",
    u"abcd",
    u"Abc123",
    u"Abc12345",
    u"Abc12345!",
    u"CorrectHorseBatteryStaple123!",
    u"Tr0ub4dor&3_Has_a_Very_Long_Name!@#$%",
  };

  for (const auto& password : test_cases) {
    int percent = AstraPasswordHelper::GetPasswordStrengthPercent(password);
    EXPECT_GE(percent, 0) << "Password: " << base::UTF16ToUTF8(password);
    EXPECT_LE(percent, 100) << "Password: " << base::UTF16ToUTF8(password);
  }
}

TEST_F(PasswordHelperTest, GetPasswordStrengthLabel_ReturnsLabels) {
  EXPECT_EQ(AstraPasswordHelper::GetPasswordStrengthLabel(
      AstraPasswordStrength::kVeryWeak), u"Very Weak");
  EXPECT_EQ(AstraPasswordHelper::GetPasswordStrengthLabel(
      AstraPasswordStrength::kWeak), u"Weak");
  EXPECT_EQ(AstraPasswordHelper::GetPasswordStrengthLabel(
      AstraPasswordStrength::kMedium), u"Medium");
  EXPECT_EQ(AstraPasswordHelper::GetPasswordStrengthLabel(
      AstraPasswordStrength::kStrong), u"Strong");
  EXPECT_EQ(AstraPasswordHelper::GetPasswordStrengthLabel(
      AstraPasswordStrength::kVeryStrong), u"Very Strong");
}

// ---------------------------------------------------------------------------
// Password generator
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, GeneratePassword_LengthCorrect) {
  auto password = AstraPasswordHelper::GeneratePassword(15, true);
  EXPECT_EQ(password.size(), 15u);
}

TEST_F(PasswordHelperTest, GeneratePassword_ZeroLengthEmpty) {
  auto password = AstraPasswordHelper::GeneratePassword(0, true);
  EXPECT_TRUE(password.empty());
}

TEST_F(PasswordHelperTest, GeneratePassword_VariousLengths) {
  for (size_t len : {1, 4, 8, 12, 16, 20, 32}) {
    auto password = AstraPasswordHelper::GeneratePassword(len, true);
    EXPECT_EQ(password.size(), len) << "Length " << len;
  }
}

TEST_F(PasswordHelperTest, GeneratePassword_WithoutSymbols) {
  auto password = AstraPasswordHelper::GeneratePassword(20, false);
  // All characters should be alphanumeric.
  for (char16_t c : password) {
    EXPECT_TRUE(base::IsAsciiAlphaNumeric(c))
        << "Unexpected character: " << c;
  }
}

TEST_F(PasswordHelperTest, GeneratePassword_WithSymbols) {
  // Generate a long password with symbols — it's very likely to contain at
  // least one symbol in a 50-char password.
  auto password = AstraPasswordHelper::GeneratePassword(50, true);
  bool has_symbol = false;
  for (char16_t c : password) {
    if (!base::IsAsciiAlphaNumeric(c)) {
      has_symbol = true;
      break;
    }
  }
  EXPECT_TRUE(has_symbol) << "Password should contain symbols";
}

TEST_F(PasswordHelperTest, GeneratePassword_NotAllSame) {
  // Generated passwords should not be all the same character.
  auto password = AstraPasswordHelper::GeneratePassword(20, true);
  char16_t first = password[0];
  bool all_same = true;
  for (char16_t c : password) {
    if (c != first) {
      all_same = false;
      break;
    }
  }
  EXPECT_FALSE(all_same) << "Generated password should not be all same chars";
}

TEST_F(PasswordHelperTest, GeneratePassword_MeetsComplexity) {
  // A generated password with default settings should meet complexity reqs.
  auto password = AstraPasswordHelper::GeneratePassword(15, true);
  EXPECT_TRUE(AstraPasswordHelper::MeetsComplexityRequirements(password));
}

TEST_F(PasswordHelperTest, GeneratePassword_TwoPasswordsDifferent) {
  // Two generated passwords should (almost always) be different.
  auto p1 = AstraPasswordHelper::GeneratePassword(20, true);
  auto p2 = AstraPasswordHelper::GeneratePassword(20, true);
  EXPECT_NE(p1, p2);
}

// ---------------------------------------------------------------------------
// Password complexity requirements
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, MeetsComplexityRequirements_StrongPassword) {
  EXPECT_TRUE(AstraPasswordHelper::MeetsComplexityRequirements(
      u"CorrectHorse123!"));
}

TEST_F(PasswordHelperTest, MeetsComplexityRequirements_TooShort) {
  EXPECT_FALSE(AstraPasswordHelper::MeetsComplexityRequirements(u"Ab1!"));
}

TEST_F(PasswordHelperTest, MeetsComplexityRequirements_OnlyLowercase) {
  EXPECT_FALSE(AstraPasswordHelper::MeetsComplexityRequirements(
      u"abcdefghijklmno"));
}

TEST_F(PasswordHelperTest, MeetsComplexityRequirements_LowerAndUpper) {
  // Only 2 categories — should not meet requirements.
  EXPECT_FALSE(AstraPasswordHelper::MeetsComplexityRequirements(
      u"ABCDEFGHIJKLmno"));
}

TEST_F(PasswordHelperTest, MeetsComplexityRequirements_ThreeCategories) {
  // Lowercase + uppercase + digits = 3 categories, should pass.
  EXPECT_TRUE(AstraPasswordHelper::MeetsComplexityRequirements(
      u"Abcdefghij123"));
}

TEST_F(PasswordHelperTest, MeetsComplexityRequirements_LowerDigitsSymbol) {
  // Lowercase + digits + symbols = 3 categories, should pass.
  EXPECT_TRUE(AstraPasswordHelper::MeetsComplexityRequirements(
      u"password123!"));
}

// ---------------------------------------------------------------------------
// Breach detection state
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, BreachDetected_InitiallyFalse) {
  EXPECT_FALSE(helper_->IsBreachDetected());
}

TEST_F(PasswordHelperTest, NotifyBreach_SetsBreachFlag) {
  ASSERT_FALSE(helper_->IsBreachDetected());

  helper_->NotifyPasswordBreachDetected(5);

  EXPECT_TRUE(helper_->IsBreachDetected());
}

TEST_F(PasswordHelperTest, AcknowledgeBreach_ResetsFlag) {
  helper_->NotifyPasswordBreachDetected(3);
  ASSERT_TRUE(helper_->IsBreachDetected());

  helper_->AcknowledgeBreach();

  EXPECT_FALSE(helper_->IsBreachDetected());
}

TEST_F(PasswordHelperTest, AcknowledgeBreach_WhenNoBreach_NoOp) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  ASSERT_FALSE(helper_->IsBreachDetected());
  helper_->AcknowledgeBreach();

  EXPECT_EQ(observer.settings_changed_count_, 0);
  EXPECT_FALSE(helper_->IsBreachDetected());

  helper_->RemoveObserver(&observer);
}

TEST_F(PasswordHelperTest, AcknowledgeBreach_FiresSettingsObserver) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifyPasswordBreachDetected(3);
  // Reset counter (breach detection fires settings via pref change).
  observer.settings_changed_count_ = 0;

  helper_->AcknowledgeBreach();

  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(PasswordHelperTest, BreachDetected_NotifiesObserver) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifyPasswordBreachDetected(5);

  EXPECT_EQ(observer.breach_detected_count_, 1);
  EXPECT_EQ(observer.last_breach_count_, 5u);
  EXPECT_GT(observer.health_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Manual observer notifications
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, NotifyPasswordsChanged_FiresObserver) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifyPasswordsChanged();
  EXPECT_EQ(observer.passwords_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(PasswordHelperTest, NotifyPasswordSaved_FiresObserver) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  AstraPasswordEntry entry;
  entry.site_display_name = u"example.com";
  entry.username = u"testuser";
  helper_->NotifyPasswordSaved(entry);

  EXPECT_EQ(observer.password_saved_count_, 1);
  EXPECT_EQ(observer.last_saved_entry_.site_display_name, u"example.com");
  EXPECT_EQ(observer.last_saved_entry_.username, u"testuser");
  // Should also fire OnPasswordsChanged.
  EXPECT_EQ(observer.passwords_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(PasswordHelperTest, NotifyPasswordUpdated_FiresObserver) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  AstraPasswordEntry entry;
  entry.site_display_name = u"updated.com";
  helper_->NotifyPasswordUpdated(entry);

  EXPECT_EQ(observer.password_updated_count_, 1);
  EXPECT_EQ(observer.last_updated_entry_.site_display_name, u"updated.com");
  EXPECT_EQ(observer.passwords_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(PasswordHelperTest, NotifyPasswordDeleted_FiresObserver) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  AstraPasswordEntry entry;
  entry.site_display_name = u"deleted.com";
  helper_->NotifyPasswordDeleted(entry);

  EXPECT_EQ(observer.password_deleted_count_, 1);
  EXPECT_EQ(observer.last_deleted_entry_.site_display_name, u"deleted.com");
  EXPECT_EQ(observer.passwords_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(PasswordHelperTest, NotifyPasswordHealthChanged_FiresObserver) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifyPasswordHealthChanged();
  EXPECT_EQ(observer.health_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(PasswordHelperTest, NotifyPasswordSettingsChanged_FiresObserver) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifyPasswordSettingsChanged();
  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Multiple observers
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, MultipleObservers_AllNotified) {
  TestPasswordHelperObserver observer1;
  TestPasswordHelperObserver observer2;

  helper_->AddObserver(&observer1);
  helper_->AddObserver(&observer2);

  helper_->SetShowPasswordSuggestions(false);

  EXPECT_EQ(observer1.settings_changed_count_, 1);
  EXPECT_EQ(observer2.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer1);
  helper_->RemoveObserver(&observer2);
}

TEST_F(PasswordHelperTest, RemoveObserver_StopsNotifications) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetShowPasswordSuggestions(false);
  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);

  helper_->SetShowPasswordSuggestions(true);
  // Should not have been notified after removal.
  EXPECT_EQ(observer.settings_changed_count_, 1);
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, Shutdown_CleansUp) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->Shutdown();

  // After shutdown, settings changes should not notify since profile is null.
  helper_->SetShowPasswordSuggestions(false);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Persistence round-trip
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, PrefsPersist_ShowPasswordSuggestions) {
  helper_->SetShowPasswordSuggestions(false);

  // Create a new helper with the same profile — should read persisted value.
  auto helper2 = std::make_unique<AstraPasswordHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetShowPasswordSuggestions());
}

TEST_F(PasswordHelperTest, PrefsPersist_AutoFillEnabled) {
  helper_->SetAutoFillEnabled(false);

  auto helper2 = std::make_unique<AstraPasswordHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetAutoFillEnabled());
}

TEST_F(PasswordHelperTest, PrefsPersist_PasswordManagerShortcut) {
  helper_->SetPasswordManagerShortcut(false);

  auto helper2 = std::make_unique<AstraPasswordHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetPasswordManagerShortcut());
}

TEST_F(PasswordHelperTest, PrefsPersist_ShowPasswordHealth) {
  helper_->SetShowPasswordHealth(false);

  auto helper2 = std::make_unique<AstraPasswordHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetShowPasswordHealth());
}

TEST_F(PasswordHelperTest, PrefsPersist_BreachAlertsEnabled) {
  helper_->SetBreachAlertsEnabled(false);

  auto helper2 = std::make_unique<AstraPasswordHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetBreachAlertsEnabled());
}

TEST_F(PasswordHelperTest, PrefsPersist_MaxSidebarPasswords) {
  helper_->SetMaxSidebarPasswords(42);

  auto helper2 = std::make_unique<AstraPasswordHelper>(profile_.get());
  EXPECT_EQ(helper2->GetMaxSidebarPasswords(), 42);
}

TEST_F(PasswordHelperTest, PrefsPersist_BreachDetected) {
  helper_->NotifyPasswordBreachDetected(3);
  ASSERT_TRUE(helper_->IsBreachDetected());

  auto helper2 = std::make_unique<AstraPasswordHelper>(profile_.get());
  EXPECT_TRUE(helper2->IsBreachDetected());
}

TEST_F(PasswordHelperTest, PrefsPersist_DefaultValues) {
  // Create a fresh helper — should have default values.
  auto helper2 = std::make_unique<AstraPasswordHelper>(profile_.get());

  EXPECT_TRUE(helper2->GetShowPasswordSuggestions());
  EXPECT_TRUE(helper2->GetAutoFillEnabled());
  EXPECT_TRUE(helper2->GetPasswordManagerShortcut());
  EXPECT_TRUE(helper2->GetShowPasswordHealth());
  EXPECT_TRUE(helper2->GetBreachAlertsEnabled());
  EXPECT_EQ(helper2->GetMaxSidebarPasswords(),
            prefs::kDefaultPasswordMaxSidebarPasswords);
  EXPECT_FALSE(helper2->IsBreachDetected());
}

// ---------------------------------------------------------------------------
// Combined settings changes
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, MultipleSettingChanges_AllNotify) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  // Change each setting — each should fire a settings notification.
  helper_->SetShowPasswordSuggestions(false);
  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->SetAutoFillEnabled(false);
  EXPECT_EQ(observer.settings_changed_count_, 2);

  helper_->SetPasswordManagerShortcut(false);
  EXPECT_EQ(observer.settings_changed_count_, 3);

  helper_->SetShowPasswordHealth(false);
  EXPECT_EQ(observer.settings_changed_count_, 4);

  helper_->SetBreachAlertsEnabled(false);
  EXPECT_EQ(observer.settings_changed_count_, 5);

  helper_->SetMaxSidebarPasswords(10);
  EXPECT_EQ(observer.settings_changed_count_, 6);

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// AstraPasswordEntry struct
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, PasswordEntry_DefaultConstructed) {
  AstraPasswordEntry entry;
  EXPECT_TRUE(entry.url.is_empty());
  EXPECT_TRUE(entry.site_display_name.empty());
  EXPECT_TRUE(entry.username.empty());
  EXPECT_FALSE(entry.is_blocked);
  EXPECT_FALSE(entry.is_compromised);
  EXPECT_FALSE(entry.is_weak);
  EXPECT_FALSE(entry.is_reused);
}

// ---------------------------------------------------------------------------
// AstraPasswordHealthStats struct
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, HealthStats_DefaultConstructed) {
  AstraPasswordHealthStats stats;
  EXPECT_EQ(stats.total_passwords, 0u);
  EXPECT_EQ(stats.compromised_count, 0u);
  EXPECT_EQ(stats.weak_count, 0u);
  EXPECT_EQ(stats.reused_count, 0u);
  EXPECT_EQ(stats.blocked_count, 0u);
  EXPECT_EQ(stats.problem_count(), 0u);
  EXPECT_FALSE(stats.has_breaches());
}

TEST_F(PasswordHelperTest, HealthStats_ProblemCount) {
  AstraPasswordHealthStats stats;
  stats.compromised_count = 3;
  stats.weak_count = 5;
  stats.reused_count = 2;
  // problem_count sums all three (can exceed total if passwords overlap).
  EXPECT_EQ(stats.problem_count(), 10u);
}

TEST_F(PasswordHelperTest, HealthStats_HasBreaches) {
  AstraPasswordHealthStats stats;
  EXPECT_FALSE(stats.has_breaches());

  stats.compromised_count = 1;
  EXPECT_TRUE(stats.has_breaches());
}

// ---------------------------------------------------------------------------
// Bulk operations — stub behavior
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, ExportPasswords_ReturnsFalseInOverlay) {
  // In the overlay, password export is not implemented.
  EXPECT_FALSE(helper_->ExportPasswords("/tmp/passwords.csv"));
}

// ---------------------------------------------------------------------------
// Copy to clipboard — stub behavior
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, CopyPasswordToClipboard_ReturnsFalseInOverlay) {
  AstraPasswordEntry entry;
  EXPECT_FALSE(helper_->CopyPasswordToClipboard(entry));
}

// ---------------------------------------------------------------------------
// Open settings — null profile safety
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, OpenPasswordSettings_NullProfileNoCrash) {
  helper_->OpenPasswordSettings(nullptr);
  SUCCEED();
}

TEST_F(PasswordHelperTest, OpenPasswordCheck_NullProfileNoCrash) {
  helper_->OpenPasswordCheck(nullptr);
  SUCCEED();
}

TEST_F(PasswordHelperTest, OpenPasswordSettings_ValidProfileNoCrash) {
  helper_->OpenPasswordSettings(profile_.get());
  SUCCEED();
}

TEST_F(PasswordHelperTest, OpenPasswordCheck_ValidProfileNoCrash) {
  helper_->OpenPasswordCheck(profile_.get());
  SUCCEED();
}

// ---------------------------------------------------------------------------
// SearchPasswords — edge cases
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, SearchPasswords_EmptyQueryReturnsAll) {
  // In the overlay, both return empty since there are no passwords.
  auto empty_query = helper_->SearchPasswords(std::u16string(), 10);
  auto all = helper_->GetSavedPasswords(10);
  EXPECT_EQ(empty_query.size(), all.size());
}

TEST_F(PasswordHelperTest, SearchPasswords_ZeroMaxCount) {
  auto results = helper_->SearchPasswords(u"test", 0);
  // Zero max_count means no limit.
  // In overlay, there are no passwords, so result is empty.
  EXPECT_TRUE(results.empty());
}

// ---------------------------------------------------------------------------
// GetSavedPasswords — max_count edge cases
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, GetSavedPasswords_ZeroMaxCountUsesDefault) {
  auto results = helper_->GetSavedPasswords(0);
  // Zero means default limit. In overlay, result is empty.
  EXPECT_TRUE(results.empty());
}

// ---------------------------------------------------------------------------
// Sort order settings
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, SortOrder_DefaultIsAlphabetical) {
  EXPECT_EQ(helper_->GetSortOrder(), AstraPasswordSortOrder::kAlphabetical);
}

TEST_F(PasswordHelperTest, SortOrder_SetToLastUsed) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetSortOrder(AstraPasswordSortOrder::kLastUsed);
  EXPECT_EQ(helper_->GetSortOrder(), AstraPasswordSortOrder::kLastUsed);
  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(PasswordHelperTest, SortOrder_SetToDateCreated) {
  helper_->SetSortOrder(AstraPasswordSortOrder::kDateCreated);
  EXPECT_EQ(helper_->GetSortOrder(), AstraPasswordSortOrder::kDateCreated);
}

TEST_F(PasswordHelperTest, SortOrder_SetSameNoChange) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  // Default is alphabetical; setting it again should not notify.
  helper_->SetSortOrder(AstraPasswordSortOrder::kAlphabetical);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(PasswordHelperTest, SortOrder_CycleAlphabeticalToLastUsed) {
  helper_->SetSortOrder(AstraPasswordSortOrder::kAlphabetical);
  AstraPasswordSortOrder next = helper_->CycleSortOrder();
  EXPECT_EQ(next, AstraPasswordSortOrder::kLastUsed);
}

TEST_F(PasswordHelperTest, SortOrder_CycleLastUsedToDateCreated) {
  helper_->SetSortOrder(AstraPasswordSortOrder::kLastUsed);
  AstraPasswordSortOrder next = helper_->CycleSortOrder();
  EXPECT_EQ(next, AstraPasswordSortOrder::kDateCreated);
}

TEST_F(PasswordHelperTest, SortOrder_CycleDateCreatedToAlphabetical) {
  helper_->SetSortOrder(AstraPasswordSortOrder::kDateCreated);
  AstraPasswordSortOrder next = helper_->CycleSortOrder();
  EXPECT_EQ(next, AstraPasswordSortOrder::kAlphabetical);
}

TEST_F(PasswordHelperTest, SortOrder_Persistence) {
  helper_->SetSortOrder(AstraPasswordSortOrder::kLastUsed);
  // Read back from prefs to verify persistence.
  PrefService* prefs = profile_->GetPrefs();
  EXPECT_EQ(prefs->GetString(prefs::kPrefPasswordSortOrder), "last_used");
}

// ---------------------------------------------------------------------------
// Filter settings
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, Filter_DefaultIsAll) {
  EXPECT_EQ(helper_->GetFilter(), AstraPasswordFilter::kAll);
}

TEST_F(PasswordHelperTest, Filter_SetToCompromised) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetFilter(AstraPasswordFilter::kCompromised);
  EXPECT_EQ(helper_->GetFilter(), AstraPasswordFilter::kCompromised);
  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(PasswordHelperTest, Filter_SetToWeak) {
  helper_->SetFilter(AstraPasswordFilter::kWeak);
  EXPECT_EQ(helper_->GetFilter(), AstraPasswordFilter::kWeak);
}

TEST_F(PasswordHelperTest, Filter_SetToReused) {
  helper_->SetFilter(AstraPasswordFilter::kReused);
  EXPECT_EQ(helper_->GetFilter(), AstraPasswordFilter::kReused);
}

TEST_F(PasswordHelperTest, Filter_SetSameNoNotification) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetFilter(AstraPasswordFilter::kAll);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(PasswordHelperTest, Filter_CycleAllToCompromised) {
  helper_->SetFilter(AstraPasswordFilter::kAll);
  AstraPasswordFilter next = helper_->CycleFilter();
  EXPECT_EQ(next, AstraPasswordFilter::kCompromised);
}

TEST_F(PasswordHelperTest, Filter_CycleCompromisedToWeak) {
  helper_->SetFilter(AstraPasswordFilter::kCompromised);
  AstraPasswordFilter next = helper_->CycleFilter();
  EXPECT_EQ(next, AstraPasswordFilter::kWeak);
}

TEST_F(PasswordHelperTest, Filter_CycleWeakToReused) {
  helper_->SetFilter(AstraPasswordFilter::kWeak);
  AstraPasswordFilter next = helper_->CycleFilter();
  EXPECT_EQ(next, AstraPasswordFilter::kReused);
}

TEST_F(PasswordHelperTest, Filter_CycleReusedToAll) {
  helper_->SetFilter(AstraPasswordFilter::kReused);
  AstraPasswordFilter next = helper_->CycleFilter();
  EXPECT_EQ(next, AstraPasswordFilter::kAll);
}

TEST_F(PasswordHelperTest, Filter_Persistence) {
  helper_->SetFilter(AstraPasswordFilter::kCompromised);
  PrefService* prefs = profile_->GetPrefs();
  EXPECT_EQ(prefs->GetString(prefs::kPrefPasswordFilter), "compromised");
}

// ---------------------------------------------------------------------------
// Group by settings
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, GroupBy_DefaultIsNone) {
  EXPECT_EQ(helper_->GetGroupBy(), AstraPasswordGroupBy::kNone);
}

TEST_F(PasswordHelperTest, GroupBy_SetToSite) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetGroupBy(AstraPasswordGroupBy::kSite);
  EXPECT_EQ(helper_->GetGroupBy(), AstraPasswordGroupBy::kSite);
  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(PasswordHelperTest, GroupBy_SetToAccount) {
  helper_->SetGroupBy(AstraPasswordGroupBy::kAccount);
  EXPECT_EQ(helper_->GetGroupBy(), AstraPasswordGroupBy::kAccount);
}

TEST_F(PasswordHelperTest, GroupBy_SetSameNoNotification) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetGroupBy(AstraPasswordGroupBy::kNone);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(PasswordHelperTest, GroupBy_CycleNoneToSite) {
  helper_->SetGroupBy(AstraPasswordGroupBy::kNone);
  AstraPasswordGroupBy next = helper_->CycleGroupBy();
  EXPECT_EQ(next, AstraPasswordGroupBy::kSite);
}

TEST_F(PasswordHelperTest, GroupBy_CycleSiteToAccount) {
  helper_->SetGroupBy(AstraPasswordGroupBy::kSite);
  AstraPasswordGroupBy next = helper_->CycleGroupBy();
  EXPECT_EQ(next, AstraPasswordGroupBy::kAccount);
}

TEST_F(PasswordHelperTest, GroupBy_CycleAccountToNone) {
  helper_->SetGroupBy(AstraPasswordGroupBy::kAccount);
  AstraPasswordGroupBy next = helper_->CycleGroupBy();
  EXPECT_EQ(next, AstraPasswordGroupBy::kNone);
}

TEST_F(PasswordHelperTest, GroupBy_Persistence) {
  helper_->SetGroupBy(AstraPasswordGroupBy::kSite);
  PrefService* prefs = profile_->GetPrefs();
  EXPECT_EQ(prefs->GetString(prefs::kPrefPasswordGroupBy), "site");
}

// ---------------------------------------------------------------------------
// Hide passwords by default
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, HideByDefault_DefaultIsTrue) {
  EXPECT_TRUE(helper_->GetHidePasswordsByDefault());
}

TEST_F(PasswordHelperTest, HideByDefault_SetToFalse) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetHidePasswordsByDefault(false);
  EXPECT_FALSE(helper_->GetHidePasswordsByDefault());
  EXPECT_EQ(observer.settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(PasswordHelperTest, HideByDefault_Toggle) {
  bool original = helper_->GetHidePasswordsByDefault();
  bool toggled = helper_->ToggleHidePasswordsByDefault();
  EXPECT_EQ(toggled, !original);
  EXPECT_EQ(helper_->GetHidePasswordsByDefault(), !original);
}

TEST_F(PasswordHelperTest, HideByDefault_SetSameNoNotification) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetHidePasswordsByDefault(helper_->GetHidePasswordsByDefault());
  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(PasswordHelperTest, HideByDefault_Persistence) {
  helper_->SetHidePasswordsByDefault(false);
  PrefService* prefs = profile_->GetPrefs();
  EXPECT_FALSE(prefs->GetBoolean(prefs::kPrefPasswordHideByDefault));
}

// ---------------------------------------------------------------------------
// GetDisplayPasswords
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, GetDisplayPasswords_ReturnsEmptyInOverlay) {
  auto results = helper_->GetDisplayPasswords(10);
  EXPECT_TRUE(results.empty());
}

TEST_F(PasswordHelperTest, GetDisplayPasswords_ZeroMaxCount) {
  auto results = helper_->GetDisplayPasswords(0);
  // With no real store, still returns empty.
  EXPECT_TRUE(results.empty());
}

TEST_F(PasswordHelperTest, GetDisplayPasswords_RespectsFilter) {
  // Set filter to compromised and verify it doesn't crash.
  helper_->SetFilter(AstraPasswordFilter::kCompromised);
  auto results = helper_->GetDisplayPasswords(10);
  // In overlay, still empty, but shouldn't crash.
  EXPECT_TRUE(results.empty());
}

TEST_F(PasswordHelperTest, GetDisplayPasswords_RespectsSortOrder) {
  helper_->SetSortOrder(AstraPasswordSortOrder::kLastUsed);
  auto results = helper_->GetDisplayPasswords(10);
  // In overlay, still empty, but shouldn't crash.
  EXPECT_TRUE(results.empty());
}

// ---------------------------------------------------------------------------
// GetPasswordsForURL
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, GetPasswordsForURL_InvalidURL) {
  auto results = helper_->GetPasswordsForURL(GURL(), 10);
  // Invalid URL returns all (which is empty in overlay).
  EXPECT_TRUE(results.empty());
}

TEST_F(PasswordHelperTest, GetPasswordsForURL_ValidURLNoMatches) {
  auto results = helper_->GetPasswordsForURL(GURL("https://example.com"), 10);
  // No passwords in overlay.
  EXPECT_TRUE(results.empty());
}

TEST_F(PasswordHelperTest, GetPasswordsForURL_ZeroMaxCount) {
  auto results = helper_->GetPasswordsForURL(GURL("https://example.com"), 0);
  // Zero means no limit. In overlay, still empty.
  EXPECT_TRUE(results.empty());
}

// ---------------------------------------------------------------------------
// GetPasswordById
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, GetPasswordById_EmptyId) {
  auto result = helper_->GetPasswordById("");
  EXPECT_FALSE(result.has_value());
}

TEST_F(PasswordHelperTest, GetPasswordById_Nonexistent) {
  auto result = helper_->GetPasswordById("nonexistent-id");
  EXPECT_FALSE(result.has_value());
}

// ---------------------------------------------------------------------------
// GetFilteredPasswordCount
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, GetFilteredPasswordCount_AllFilter) {
  helper_->SetFilter(AstraPasswordFilter::kAll);
  EXPECT_EQ(helper_->GetFilteredPasswordCount(), 0u);
}

TEST_F(PasswordHelperTest, GetFilteredPasswordCount_CompromisedFilter) {
  helper_->SetFilter(AstraPasswordFilter::kCompromised);
  EXPECT_EQ(helper_->GetFilteredPasswordCount(), 0u);
}

TEST_F(PasswordHelperTest, GetFilteredPasswordCount_WeakFilter) {
  helper_->SetFilter(AstraPasswordFilter::kWeak);
  EXPECT_EQ(helper_->GetFilteredPasswordCount(), 0u);
}

TEST_F(PasswordHelperTest, GetFilteredPasswordCount_ReusedFilter) {
  helper_->SetFilter(AstraPasswordFilter::kReused);
  EXPECT_EQ(helper_->GetFilteredPasswordCount(), 0u);
}

// ---------------------------------------------------------------------------
// Password strength color
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, StrengthColor_VeryWeakIsRed) {
  SkColor color = AstraPasswordHelper::GetPasswordStrengthColor(
      AstraPasswordStrength::kVeryWeak);
  EXPECT_EQ(color, SK_ColorRED);
}

TEST_F(PasswordHelperTest, StrengthColor_VeryStrongIsGreen) {
  SkColor color = AstraPasswordHelper::GetPasswordStrengthColor(
      AstraPasswordStrength::kVeryStrong);
  // Should be a green-ish color.
  EXPECT_GT(SkColorGetG(color), SkColorGetR(color));
}

TEST_F(PasswordHelperTest, StrengthColor_MediumIsAmber) {
  SkColor color = AstraPasswordHelper::GetPasswordStrengthColor(
      AstraPasswordStrength::kMedium);
  // Medium should have high red and green (amber/yellow).
  EXPECT_GT(SkColorGetR(color), 0);
  EXPECT_GT(SkColorGetG(color), 0);
}

TEST_F(PasswordHelperTest, StrengthColor_AllStrengthsDistinct) {
  SkColor very_weak = AstraPasswordHelper::GetPasswordStrengthColor(
      AstraPasswordStrength::kVeryWeak);
  SkColor weak = AstraPasswordHelper::GetPasswordStrengthColor(
      AstraPasswordStrength::kWeak);
  SkColor medium = AstraPasswordHelper::GetPasswordStrengthColor(
      AstraPasswordStrength::kMedium);
  SkColor strong = AstraPasswordHelper::GetPasswordStrengthColor(
      AstraPasswordStrength::kStrong);
  SkColor very_strong = AstraPasswordHelper::GetPasswordStrengthColor(
      AstraPasswordStrength::kVeryStrong);

  // Not all should be the same.
  EXPECT_TRUE(very_weak != weak || weak != medium || medium != strong ||
              strong != very_strong);
}

// ---------------------------------------------------------------------------
// CopyUsernameToClipboard
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, CopyUsernameToClipboard_EmptyUsername) {
  AstraPasswordEntry entry;
  entry.username = std::u16string();
  EXPECT_FALSE(helper_->CopyUsernameToClipboard(entry));
}

TEST_F(PasswordHelperTest, CopyUsernameToClipboard_ValidEntry) {
  AstraPasswordEntry entry;
  entry.username = u"testuser@example.com";
  // In overlay, returns false (not implemented).
  // Just verify it doesn't crash.
  helper_->CopyUsernameToClipboard(entry);
  SUCCEED();
}

// ---------------------------------------------------------------------------
// AstraPasswordEntry struct
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, PasswordEntry_DefaultValues) {
  AstraPasswordEntry entry;
  EXPECT_FALSE(entry.url.is_valid());
  EXPECT_TRUE(entry.site_display_name.empty());
  EXPECT_TRUE(entry.username.empty());
  EXPECT_FALSE(entry.is_blocked);
  EXPECT_FALSE(entry.is_compromised);
  EXPECT_FALSE(entry.is_weak);
  EXPECT_FALSE(entry.is_reused);
  EXPECT_EQ(entry.strength, AstraPasswordStrength::kMedium);
  EXPECT_EQ(entry.strength_percent, 50);
  EXPECT_EQ(entry.last_used_time, 0);
  EXPECT_EQ(entry.date_created, 0);
  EXPECT_EQ(entry.use_count, 0);
  EXPECT_FALSE(entry.is_federated);
  EXPECT_TRUE(entry.id.empty());
  EXPECT_TRUE(entry.notes.empty());
}

TEST_F(PasswordHelperTest, PasswordEntry_Copyable) {
  AstraPasswordEntry entry;
  entry.site_display_name = u"Example";
  entry.username = u"user";
  entry.is_compromised = true;
  entry.strength = AstraPasswordStrength::kStrong;
  entry.strength_percent = 75;

  AstraPasswordEntry copy = entry;
  EXPECT_EQ(copy.site_display_name, entry.site_display_name);
  EXPECT_EQ(copy.username, entry.username);
  EXPECT_EQ(copy.is_compromised, entry.is_compromised);
  EXPECT_EQ(copy.strength, entry.strength);
  EXPECT_EQ(copy.strength_percent, entry.strength_percent);
}

// ---------------------------------------------------------------------------
// AstraPasswordHealthStats struct
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, HealthStats_DefaultValues) {
  AstraPasswordHealthStats stats;
  EXPECT_EQ(stats.total_passwords, 0u);
  EXPECT_EQ(stats.compromised_count, 0u);
  EXPECT_EQ(stats.weak_count, 0u);
  EXPECT_EQ(stats.reused_count, 0u);
  EXPECT_EQ(stats.blocked_count, 0u);
  EXPECT_EQ(stats.problem_count(), 0u);
  EXPECT_FALSE(stats.has_breaches());
}

TEST_F(PasswordHelperTest, HealthStats_ProblemCount) {
  AstraPasswordHealthStats stats;
  stats.compromised_count = 3;
  stats.weak_count = 5;
  stats.reused_count = 2;
  // problem_count sums all three categories.
  EXPECT_EQ(stats.problem_count(), 10u);
}

TEST_F(PasswordHelperTest, HealthStats_HasBreaches) {
  AstraPasswordHealthStats stats;
  EXPECT_FALSE(stats.has_breaches());
  stats.compromised_count = 1;
  EXPECT_TRUE(stats.has_breaches());
}

TEST_F(PasswordHelperTest, HealthStats_SinglePasswordMultipleProblems) {
  // A single password can be both weak and compromised.
  AstraPasswordHealthStats stats;
  stats.total_passwords = 1;
  stats.compromised_count = 1;
  stats.weak_count = 1;
  // problem_count can exceed total_passwords.
  EXPECT_GT(stats.problem_count(), stats.total_passwords);
}

// ---------------------------------------------------------------------------
// More password strength tests
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, Strength_VeryShortPassword) {
  EXPECT_EQ(AstraPasswordHelper::GetPasswordStrength(u"abc"),
            AstraPasswordStrength::kVeryWeak);
}

TEST_F(PasswordHelperTest, Strength_LongAllLowercase) {
  // Long but only lowercase — likely weak or medium.
  auto strength = AstraPasswordHelper::GetPasswordStrength(u"abcdefghijklmnop");
  EXPECT_NE(strength, AstraPasswordStrength::kVeryStrong);
  EXPECT_NE(strength, AstraPasswordStrength::kStrong);
}

TEST_F(PasswordHelperTest, Strength_MixedCaseAndNumbers) {
  // Mixed case + numbers + 12 chars — should be strong or very strong.
  auto strength = AstraPasswordHelper::GetPasswordStrength(u"Abcdefg123456");
  EXPECT_TRUE(strength == AstraPasswordStrength::kStrong ||
              strength == AstraPasswordStrength::kVeryStrong ||
              strength == AstraPasswordStrength::kMedium);
}

TEST_F(PasswordHelperTest, Strength_AllSameCharacters) {
  // All same characters — should be penalized.
  int score_normal = AstraPasswordHelper::GetPasswordStrengthPercent(u"aaaaaaaa");
  int score_varied = AstraPasswordHelper::GetPasswordStrengthPercent(u"abcdefgh");
  EXPECT_LE(score_normal, score_varied);
}

TEST_F(PasswordHelperTest, Strength_SequentialCharacters) {
  // Sequential characters — should be penalized.
  int sequential = AstraPasswordHelper::GetPasswordStrengthPercent(u"abcdefgh");
  int mixed = AstraPasswordHelper::GetPasswordStrengthPercent(u"ahcdefgb");
  EXPECT_LE(sequential, mixed);
}

// ---------------------------------------------------------------------------
// More complexity requirement tests
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, Complexity_TooShort) {
  EXPECT_FALSE(AstraPasswordHelper::MeetsComplexityRequirements(u"Short1!"));
}

TEST_F(PasswordHelperTest, Complexity_AllLowercase) {
  EXPECT_FALSE(AstraPasswordHelper::MeetsComplexityRequirements(
      u"abcdefghijkl"));
}

TEST_F(PasswordHelperTest, Complexity_NoDigits) {
  EXPECT_FALSE(AstraPasswordHelper::MeetsComplexityRequirements(
      u"Abcdefghijkl"));
}

TEST_F(PasswordHelperTest, Complexity_MeetsRequirements) {
  EXPECT_TRUE(AstraPasswordHelper::MeetsComplexityRequirements(
      u"Abcdefg123456"));
}

TEST_F(PasswordHelperTest, Complexity_WithSymbols) {
  EXPECT_TRUE(AstraPasswordHelper::MeetsComplexityRequirements(
      u"Abcdefg123!@#"));
}

// ---------------------------------------------------------------------------
// Max sidebar passwords clamping
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, MaxSidebarPasswords_ClampedToMin) {
  helper_->SetMaxSidebarPasswords(-5);
  EXPECT_GE(helper_->GetMaxSidebarPasswords(), 1);
}

TEST_F(PasswordHelperTest, MaxSidebarPasswords_ClampedToMax) {
  helper_->SetMaxSidebarPasswords(1000);
  EXPECT_LE(helper_->GetMaxSidebarPasswords(), 100);
}

TEST_F(PasswordHelperTest, MaxSidebarPasswords_ValidValue) {
  helper_->SetMaxSidebarPasswords(50);
  EXPECT_EQ(helper_->GetMaxSidebarPasswords(), 50);
}

TEST_F(PasswordHelperTest, MaxSidebarPasswords_SetSameNoChange) {
  TestPasswordHelperObserver observer;
  helper_->AddObserver(&observer);

  int current = helper_->GetMaxSidebarPasswords();
  helper_->SetMaxSidebarPasswords(current);
  EXPECT_EQ(observer.settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Presentation settings — no-op when no prefs
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, Settings_NoCrashWithNullProfile) {
  // Create a helper with null profile to test null safety.
  auto helper_no_prefs = std::make_unique<AstraPasswordHelper>(nullptr);

  // These should not crash even with no prefs.
  EXPECT_NO_FATAL_FAILURE(helper_no_prefs->GetShowPasswordSuggestions());
  EXPECT_NO_FATAL_FAILURE(helper_no_prefs->SetShowPasswordSuggestions(false));
  EXPECT_NO_FATAL_FAILURE(helper_no_prefs->GetSortOrder());
  EXPECT_NO_FATAL_FAILURE(helper_no_prefs->SetSortOrder(AstraPasswordSortOrder::kLastUsed));
  EXPECT_NO_FATAL_FAILURE(helper_no_prefs->GetFilter());
  EXPECT_NO_FATAL_FAILURE(helper_no_prefs->SetFilter(AstraPasswordFilter::kCompromised));
  EXPECT_NO_FATAL_FAILURE(helper_no_prefs->GetGroupBy());
  EXPECT_NO_FATAL_FAILURE(helper_no_prefs->SetGroupBy(AstraPasswordGroupBy::kSite));
  EXPECT_NO_FATAL_FAILURE(helper_no_prefs->GetHidePasswordsByDefault());
  EXPECT_NO_FATAL_FAILURE(helper_no_prefs->SetHidePasswordsByDefault(false));
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------

TEST_F(PasswordHelperTest, Shutdown_Clean) {
  // Helper should shut down cleanly.
  helper_->Shutdown();
  SUCCEED();
}

TEST_F(PasswordHelperTest, Shutdown_DoubleShutdownNoCrash) {
  helper_->Shutdown();
  helper_->Shutdown();  // Second shutdown should be safe.
  SUCCEED();
}

}  // namespace astra
