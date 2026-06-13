// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_safety_helper.h"

#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "chrome/test/base/testing_profile.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

#include "astra/browser/astra_prefs.h"
#include "astra/browser/astra_safety_helper_factory.h"

namespace astra {

namespace {

// Test observer that records calls for verification.
class TestSafetyObserver : public AstraSafetyObserver {
 public:
  void OnThreatDetected(const AstraThreatInfo& threat) override {
    threat_detected_count_++;
    last_detected_threat_ = threat;
  }

  void OnThreatBlocked(const AstraThreatInfo& threat) override {
    threat_blocked_count_++;
    last_blocked_threat_ = threat;
  }

  void OnUserProceeded(const GURL& url) override {
    user_proceeded_count_++;
    last_proceeded_url_ = url;
  }

  void OnSecurityStatusChanged(const AstraSecurityStatus& status) override {
    security_status_changed_count_++;
    last_security_status_ = status;
  }

  void OnSafeBrowsingToggled(bool enabled) override {
    safe_browsing_toggled_count_++;
    last_safe_browsing_enabled_ = enabled;
  }

  void OnSafetySettingsChanged() override {
    safety_settings_changed_count_++;
  }

  void OnPasswordBreachDetected(const std::string& site) override {
    password_breach_count_++;
    last_breach_site_ = site;
  }

  // Counters
  int threat_detected_count_ = 0;
  int threat_blocked_count_ = 0;
  int user_proceeded_count_ = 0;
  int security_status_changed_count_ = 0;
  int safe_browsing_toggled_count_ = 0;
  int safety_settings_changed_count_ = 0;
  int password_breach_count_ = 0;

  // Last recorded values
  AstraThreatInfo last_detected_threat_;
  AstraThreatInfo last_blocked_threat_;
  GURL last_proceeded_url_;
  AstraSecurityStatus last_security_status_;
  bool last_safe_browsing_enabled_ = false;
  std::string last_breach_site_;
};

// Helper to create a threat info struct.
AstraThreatInfo MakeThreatInfo(AstraThreatType type,
                               const std::string& url,
                               int severity,
                               bool is_blocked) {
  AstraThreatInfo threat;
  threat.url = GURL(url);
  threat.threat_type = type;
  threat.severity = severity;
  threat.discovered_time = base::Time::Now();
  threat.is_blocked = is_blocked;
  return threat;
}

}  // namespace

// Test fixture for AstraSafetyHelper tests.
class SafetyHelperTest : public testing::Test {
 protected:
  SafetyHelperTest() {
    profile_ = std::make_unique<TestingProfile>();
    // Register Astra prefs on the testing profile.
    prefs::RegisterProfilePrefs(profile_->GetPrefs());
    helper_ = std::make_unique<AstraSafetyHelper>(profile_.get());
    DCHECK(helper_);
  }

  ~SafetyHelperTest() override = default;

  void SetUp() override {
    // Verify default presentation settings.
    ASSERT_TRUE(helper_->IsSafeBrowsingEnabled());
    ASSERT_TRUE(helper_->GetShowSecurityButton());
    ASSERT_TRUE(helper_->GetShowThreatNotifications());
    ASSERT_TRUE(helper_->GetBlockDangerousDownloads());
    ASSERT_TRUE(helper_->GetWarnOnDangerousDownloads());
    ASSERT_TRUE(helper_->GetMixedContentWarning());
    ASSERT_TRUE(helper_->GetShowSiteInfoButton());
    ASSERT_TRUE(helper_->GetSafetyCheckReminders());
    ASSERT_TRUE(helper_->GetCookieProtection());
    ASSERT_TRUE(helper_->WarnOnPasswordReuse());
    ASSERT_FALSE(helper_->GetAutoReportSafetyIssues());
    ASSERT_EQ(helper_->GetSafeBrowsingLevel(),
              prefs::kDefaultSafeBrowsingLevel);
    ASSERT_EQ(helper_->GetPasswordProtectionLevel(),
              prefs::kDefaultPasswordProtectionLevel);
  }

  void TearDown() override {
    // Clean up observers to avoid dangling references.
    for (auto& observer : test_observers_) {
      helper_->RemoveObserver(&observer);
    }
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<AstraSafetyHelper> helper_;
  std::vector<TestSafetyObserver> test_observers_;
};

// ---------------------------------------------------------------------------
// Construction and default state
// ---------------------------------------------------------------------------

TEST_F(SafetyHelperTest, DefaultState_SafeBrowsingEnabled) {
  EXPECT_TRUE(helper_->IsSafeBrowsingEnabled());
}

TEST_F(SafetyHelperTest, DefaultState_SafeBrowsingLevelStandard) {
  EXPECT_EQ(helper_->GetSafeBrowsingLevel(), 1);
}

TEST_F(SafetyHelperTest, DefaultState_EnhancedProtectionDisabled) {
  EXPECT_FALSE(helper_->IsEnhancedProtectionEnabled());
}

TEST_F(SafetyHelperTest, DefaultState_WarnOnPasswordReuse) {
  EXPECT_TRUE(helper_->WarnOnPasswordReuse());
}

TEST_F(SafetyHelperTest, DefaultState_PasswordProtectionLevel) {
  EXPECT_EQ(helper_->GetPasswordProtectionLevel(), 1);
}

TEST_F(SafetyHelperTest, DefaultState_ShowSecurityButton) {
  EXPECT_TRUE(helper_->GetShowSecurityButton());
}

TEST_F(SafetyHelperTest, DefaultState_ShowThreatNotifications) {
  EXPECT_TRUE(helper_->GetShowThreatNotifications());
}

TEST_F(SafetyHelperTest, DefaultState_BlockDangerousDownloads) {
  EXPECT_TRUE(helper_->GetBlockDangerousDownloads());
}

TEST_F(SafetyHelperTest, DefaultState_WarnOnDangerousDownloads) {
  EXPECT_TRUE(helper_->GetWarnOnDangerousDownloads());
}

TEST_F(SafetyHelperTest, DefaultState_AutoReportSafetyIssues) {
  EXPECT_FALSE(helper_->GetAutoReportSafetyIssues());
}

TEST_F(SafetyHelperTest, DefaultState_MixedContentWarning) {
  EXPECT_TRUE(helper_->GetMixedContentWarning());
}

TEST_F(SafetyHelperTest, DefaultState_ShowSiteInfoButton) {
  EXPECT_TRUE(helper_->GetShowSiteInfoButton());
}

TEST_F(SafetyHelperTest, DefaultState_SafetyCheckReminders) {
  EXPECT_TRUE(helper_->GetSafetyCheckReminders());
}

TEST_F(SafetyHelperTest, DefaultState_CookieProtection) {
  EXPECT_TRUE(helper_->GetCookieProtection());
}

TEST_F(SafetyHelperTest, DefaultState_ThreatCountZero) {
  EXPECT_EQ(helper_->GetThreatCount(), 0u);
}

TEST_F(SafetyHelperTest, DefaultState_BlockedThreatCountZero) {
  EXPECT_EQ(helper_->GetBlockedThreatCount(), 0u);
}

TEST_F(SafetyHelperTest, DefaultState_PasswordBreachCountZero) {
  EXPECT_EQ(helper_->GetPasswordBreachCount(), 0u);
}

TEST_F(SafetyHelperTest, DefaultState_RecentThreatsEmpty) {
  auto threats = helper_->GetRecentThreats(10);
  EXPECT_TRUE(threats.empty());
}

TEST_F(SafetyHelperTest, DefaultState_SafetyLevelSafe) {
  // With safe browsing enabled and no threats, level should be safe.
  AstraSafetyLevel level = helper_->GetSafetyLevel();
  EXPECT_EQ(level, AstraSafetyLevel::kSafe);
}

TEST_F(SafetyHelperTest, DefaultState_SecurityStatusHasDefaults) {
  auto status = helper_->GetSecurityStatus();
  EXPECT_TRUE(status.is_secure);
  EXPECT_TRUE(status.has_valid_cert);
  EXPECT_FALSE(status.has_mixed_content);
  EXPECT_EQ(status.cookie_count, 0);
  EXPECT_EQ(status.permission_count, 0);
}

TEST_F(SafetyHelperTest, DefaultState_SafetyCheckResultsEmpty) {
  auto results = helper_->GetSafetyCheckResults();
  EXPECT_TRUE(results.empty());
}

// ---------------------------------------------------------------------------
// Security status queries
// ---------------------------------------------------------------------------

TEST_F(SafetyHelperTest, GetSecurityStatus_ReturnsValidStruct) {
  auto status = helper_->GetSecurityStatus();
  EXPECT_EQ(status.security_level, AstraSafetyLevel::kSafe);
  EXPECT_TRUE(status.is_secure);
  EXPECT_TRUE(status.has_valid_cert);
  EXPECT_FALSE(status.has_mixed_content);
}

TEST_F(SafetyHelperTest, GetSecurityStatus_AfterSafeBrowsingDisabled) {
  helper_->SetSafeBrowsingEnabled(false);
  auto status = helper_->GetSecurityStatus();
  EXPECT_EQ(status.security_level, AstraSafetyLevel::kUnknown);
  EXPECT_FALSE(status.is_secure);
}

TEST_F(SafetyHelperTest, GetSecurityStatus_AfterThreat) {
  auto threat = MakeThreatInfo(AstraThreatType::kMalware,
                               "https://evil.com/malware", 8, true);
  helper_->NotifyThreatBlocked(threat);
  auto status = helper_->GetSecurityStatus();
  EXPECT_EQ(status.security_level, AstraSafetyLevel::kDangerous);
}

// ---------------------------------------------------------------------------
// Safety level queries
// ---------------------------------------------------------------------------

TEST_F(SafetyHelperTest, GetSafetyLevel_DefaultIsSafe) {
  EXPECT_EQ(helper_->GetSafetyLevel(), AstraSafetyLevel::kSafe);
}

TEST_F(SafetyHelperTest, GetSafetyLevel_WithThreatWarning) {
  auto threat = MakeThreatInfo(AstraThreatType::kMixedContent,
                               "https://example.com/mixed", 2, false);
  helper_->NotifyThreatDetected(threat);
  // Mixed content is not severe, so level stays safe.
  EXPECT_EQ(helper_->GetSafetyLevel(), AstraSafetyLevel::kSafe);
}

TEST_F(SafetyHelperTest, GetSafetyLevel_WithBlockedThreat) {
  auto threat = MakeThreatInfo(AstraThreatType::kMalware,
                               "https://evil.com", 9, true);
  helper_->NotifyThreatBlocked(threat);
  EXPECT_EQ(helper_->GetSafetyLevel(), AstraSafetyLevel::kDangerous);
}

TEST_F(SafetyHelperTest, GetSafetyLevel_AfterClearHistory) {
  auto threat = MakeThreatInfo(AstraThreatType::kMalware,
                               "https://evil.com", 9, true);
  helper_->NotifyThreatBlocked(threat);
  ASSERT_EQ(helper_->GetSafetyLevel(), AstraSafetyLevel::kDangerous);

  helper_->ClearThreatHistory();
  EXPECT_EQ(helper_->GetSafetyLevel(), AstraSafetyLevel::kSafe);
}

// ---------------------------------------------------------------------------
// Safe browsing enable/disable
// ---------------------------------------------------------------------------

TEST_F(SafetyHelperTest, SetSafeBrowsingEnabled_ChangesValue) {
  ASSERT_TRUE(helper_->IsSafeBrowsingEnabled());

  helper_->SetSafeBrowsingEnabled(false);
  EXPECT_FALSE(helper_->IsSafeBrowsingEnabled());

  helper_->SetSafeBrowsingEnabled(true);
  EXPECT_TRUE(helper_->IsSafeBrowsingEnabled());
}

TEST_F(SafetyHelperTest, SetSafeBrowsingEnabled_SameValueNoOp) {
  TestSafetyObserver observer;
  helper_->AddObserver(&observer);

  ASSERT_TRUE(helper_->IsSafeBrowsingEnabled());
  helper_->SetSafeBrowsingEnabled(true);

  EXPECT_EQ(observer.safe_browsing_toggled_count_, 0);
  EXPECT_EQ(observer.safety_settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(SafetyHelperTest, SetSafeBrowsingEnabled_FiresToggledObserver) {
  TestSafetyObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetSafeBrowsingEnabled(false);

  EXPECT_EQ(observer.safe_browsing_toggled_count_, 1);
  EXPECT_FALSE(observer.last_safe_browsing_enabled_);

  helper_->RemoveObserver(&observer);
}

TEST_F(SafetyHelperTest, SetSafeBrowsingEnabled_FiresSettingsObserver) {
  TestSafetyObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetSafeBrowsingEnabled(false);

  EXPECT_GE(observer.safety_settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Protection levels
// ---------------------------------------------------------------------------

TEST_F(SafetyHelperTest, GetSafeBrowsingLevel_DefaultIsStandard) {
  EXPECT_EQ(helper_->GetSafeBrowsingLevel(), 1);
}

TEST_F(SafetyHelperTest, SetSafeBrowsingLevel_ChangesValue) {
  helper_->SetSafeBrowsingLevel(2);
  EXPECT_EQ(helper_->GetSafeBrowsingLevel(), 2);

  helper_->SetSafeBrowsingLevel(0);
  EXPECT_EQ(helper_->GetSafeBrowsingLevel(), 0);

  helper_->SetSafeBrowsingLevel(1);
  EXPECT_EQ(helper_->GetSafeBrowsingLevel(), 1);
}

TEST_F(SafetyHelperTest, SetSafeBrowsingLevel_ClampsToZero) {
  helper_->SetSafeBrowsingLevel(-1);
  EXPECT_EQ(helper_->GetSafeBrowsingLevel(), 0);
}

TEST_F(SafetyHelperTest, SetSafeBrowsingLevel_ClampsToTwo) {
  helper_->SetSafeBrowsingLevel(100);
  EXPECT_EQ(helper_->GetSafeBrowsingLevel(), 2);
}

TEST_F(SafetyHelperTest, SetSafeBrowsingLevel_SameValueNoOp) {
  TestSafetyObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetSafeBrowsingLevel(1);
  EXPECT_EQ(observer.safety_settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(SafetyHelperTest, SetSafeBrowsingLevel_FiresSettingsObserver) {
  TestSafetyObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetSafeBrowsingLevel(2);
  EXPECT_EQ(observer.safety_settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(SafetyHelperTest, SetSafeBrowsingLevel_LevelZeroDisablesSafeBrowsing) {
  TestSafetyObserver observer;
  helper_->AddObserver(&observer);

  ASSERT_TRUE(helper_->IsSafeBrowsingEnabled());
  helper_->SetSafeBrowsingLevel(0);

  EXPECT_FALSE(helper_->IsSafeBrowsingEnabled());
  EXPECT_EQ(observer.safe_browsing_toggled_count_, 1);
  EXPECT_FALSE(observer.last_safe_browsing_enabled_);

  helper_->RemoveObserver(&observer);
}

TEST_F(SafetyHelperTest, SetSafeBrowsingLevel_LevelPositiveEnablesSafeBrowsing) {
  helper_->SetSafeBrowsingEnabled(false);
  ASSERT_FALSE(helper_->IsSafeBrowsingEnabled());

  helper_->SetSafeBrowsingLevel(1);
  EXPECT_TRUE(helper_->IsSafeBrowsingEnabled());
}

TEST_F(SafetyHelperTest, IsEnhancedProtectionEnabled_DefaultFalse) {
  EXPECT_FALSE(helper_->IsEnhancedProtectionEnabled());
}

TEST_F(SafetyHelperTest, IsEnhancedProtectionEnabled_LevelTwo) {
  helper_->SetSafeBrowsingLevel(2);
  EXPECT_TRUE(helper_->IsEnhancedProtectionEnabled());
}

TEST_F(SafetyHelperTest, SetEnhancedProtection_TrueSetsLevelTwo) {
  helper_->SetEnhancedProtection(true);
  EXPECT_TRUE(helper_->IsEnhancedProtectionEnabled());
  EXPECT_EQ(helper_->GetSafeBrowsingLevel(), 2);
}

TEST_F(SafetyHelperTest, SetEnhancedProtection_FalseSetsLevelOne) {
  helper_->SetSafeBrowsingLevel(2);
  ASSERT_TRUE(helper_->IsEnhancedProtectionEnabled());

  helper_->SetEnhancedProtection(false);
  EXPECT_FALSE(helper_->IsEnhancedProtectionEnabled());
  EXPECT_EQ(helper_->GetSafeBrowsingLevel(), 1);
}

TEST_F(SafetyHelperTest, SetEnhancedProtection_WhenAlreadyStandard) {
  // Setting false when already at level 1 should be a no-op.
  TestSafetyObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetEnhancedProtection(false);
  EXPECT_EQ(observer.safety_settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Threat detection and history
// ---------------------------------------------------------------------------

TEST_F(SafetyHelperTest, NotifyThreatDetected_IncreasesCount) {
  ASSERT_EQ(helper_->GetThreatCount(), 0u);

  auto threat = MakeThreatInfo(AstraThreatType::kMalware,
                               "https://evil.com", 8, false);
  helper_->NotifyThreatDetected(threat);

  EXPECT_EQ(helper_->GetThreatCount(), 1u);
}

TEST_F(SafetyHelperTest, NotifyThreatDetected_MultipleThreats) {
  for (int i = 0; i < 5; i++) {
    auto threat = MakeThreatInfo(AstraThreatType::kMalware,
                                 "https://evil.com/" + std::to_string(i),
                                 i + 1, false);
    helper_->NotifyThreatDetected(threat);
  }
  EXPECT_EQ(helper_->GetThreatCount(), 5u);
}

TEST_F(SafetyHelperTest, GetRecentThreats_MostRecentFirst) {
  auto threat1 = MakeThreatInfo(AstraThreatType::kMalware,
                                "https://first.com", 5, false);
  auto threat2 = MakeThreatInfo(AstraThreatType::kDeceptive,
                                "https://second.com", 7, false);

  helper_->NotifyThreatDetected(threat1);
  helper_->NotifyThreatDetected(threat2);

  auto recent = helper_->GetRecentThreats(10);
  ASSERT_GE(recent.size(), 2u);
  EXPECT_EQ(recent[0].threat_type, AstraThreatType::kDeceptive);
  EXPECT_EQ(recent[1].threat_type, AstraThreatType::kMalware);
}

TEST_F(SafetyHelperTest, GetRecentThreats_MaxCountLimit) {
  for (int i = 0; i < 20; i++) {
    auto threat = MakeThreatInfo(AstraThreatType::kMalware,
                                 "https://evil.com/" + std::to_string(i),
                                 i, false);
    helper_->NotifyThreatDetected(threat);
  }

  auto recent = helper_->GetRecentThreats(5);
  EXPECT_EQ(recent.size(), 5u);
}

TEST_F(SafetyHelperTest, GetRecentThreats_ZeroMaxCountReturnsAll) {
  for (int i = 0; i < 10; i++) {
    auto threat = MakeThreatInfo(AstraThreatType::kMalware,
                                 "https://evil.com/" + std::to_string(i),
                                 i, false);
    helper_->NotifyThreatDetected(threat);
  }

  auto recent = helper_->GetRecentThreats(0);
  EXPECT_EQ(recent.size(), 10u);
}

TEST_F(SafetyHelperTest, GetRecentThreats_NegativeMaxCountReturnsAll) {
  auto threat = MakeThreatInfo(AstraThreatType::kMalware,
                               "https://evil.com", 5, false);
  helper_->NotifyThreatDetected(threat);

  auto recent = helper_->GetRecentThreats(-1);
  EXPECT_EQ(recent.size(), 1u);
}

TEST_F(SafetyHelperTest, ClearThreatHistory_ResetsAll) {
  auto threat = MakeThreatInfo(AstraThreatType::kMalware,
                               "https://evil.com", 5, true);
  helper_->NotifyThreatBlocked(threat);
  helper_->NotifyPasswordBreachDetected("example.com");

  ASSERT_GT(helper_->GetThreatCount(), 0u);
  ASSERT_GT(helper_->GetPasswordBreachCount(), 0u);

  helper_->ClearThreatHistory();

  EXPECT_EQ(helper_->GetThreatCount(), 0u);
  EXPECT_EQ(helper_->GetBlockedThreatCount(), 0u);
  EXPECT_EQ(helper_->GetPasswordBreachCount(), 0u);
}

TEST_F(SafetyHelperTest, ClearThreatHistory_MultipleTimesNoCrash) {
  helper_->ClearThreatHistory();
  helper_->ClearThreatHistory();
  SUCCEED();
}

// ---------------------------------------------------------------------------
// Blocked threat tracking
// ---------------------------------------------------------------------------

TEST_F(SafetyHelperTest, NotifyThreatBlocked_IncrementsBlockedCount) {
  auto threat = MakeThreatInfo(AstraThreatType::kMalware,
                               "https://evil.com", 9, true);
  helper_->NotifyThreatBlocked(threat);

  EXPECT_EQ(helper_->GetBlockedThreatCount(), 1u);
}

TEST_F(SafetyHelperTest, NotifyThreatBlocked_MixedWithDetected) {
  auto detected = MakeThreatInfo(AstraThreatType::kMalware,
                                 "https://detected.com", 5, false);
  auto blocked = MakeThreatInfo(AstraThreatType::kHarmful,
                                "https://blocked.com", 8, true);

  helper_->NotifyThreatDetected(detected);
  helper_->NotifyThreatBlocked(blocked);

  EXPECT_EQ(helper_->GetThreatCount(), 2u);
  EXPECT_EQ(helper_->GetBlockedThreatCount(), 1u);
}

// ---------------------------------------------------------------------------
// Password breach tracking
// ---------------------------------------------------------------------------

TEST_F(SafetyHelperTest, PasswordBreachCount_InitiallyZero) {
  EXPECT_EQ(helper_->GetPasswordBreachCount(), 0u);
}

TEST_F(SafetyHelperTest, NotifyPasswordBreach_IncrementsCount) {
  helper_->NotifyPasswordBreachDetected("example.com");
  EXPECT_EQ(helper_->GetPasswordBreachCount(), 1u);

  helper_->NotifyPasswordBreachDetected("another.com");
  EXPECT_EQ(helper_->GetPasswordBreachCount(), 2u);
}

TEST_F(SafetyHelperTest, NotifyPasswordBreach_FiresObserver) {
  TestSafetyObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifyPasswordBreachDetected("gmail.com");

  EXPECT_EQ(observer.password_breach_count_, 1);
  EXPECT_EQ(observer.last_breach_site_, "gmail.com");

  helper_->RemoveObserver(&observer);
}

TEST_F(SafetyHelperTest, NotifyPasswordBreach_FiresSettingsChanged) {
  TestSafetyObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifyPasswordBreachDetected("example.com");
  EXPECT_GE(observer.safety_settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Site allowlist / blocklist operations
// ---------------------------------------------------------------------------

TEST_F(SafetyHelperTest, IsSiteSafe_EmptyUrl) {
  EXPECT_TRUE(helper_->IsSiteSafe(GURL()));
}

TEST_F(SafetyHelperTest, IsSiteSafe_InvalidUrl) {
  EXPECT_TRUE(helper_->IsSiteSafe(GURL("not-a-url")));
}

TEST_F(SafetyHelperTest, IsSiteSafe_ValidSite) {
  EXPECT_TRUE(helper_->IsSiteSafe(GURL("https://example.com")));
}

TEST_F(SafetyHelperTest, IsSiteAllowed_InitiallyFalse) {
  EXPECT_FALSE(helper_->IsSiteAllowed(GURL("https://example.com")));
}

TEST_F(SafetyHelperTest, IsSiteBlocked_InitiallyFalse) {
  EXPECT_FALSE(helper_->IsSiteBlocked(GURL("https://evil.com")));
}

TEST_F(SafetyHelperTest, AllowSite_EmptyUrlNoCrash) {
  helper_->AllowSite(GURL());
  SUCCEED();
}

TEST_F(SafetyHelperTest, BlockSite_EmptyUrlNoCrash) {
  helper_->BlockSite(GURL());
  SUCCEED();
}

TEST_F(SafetyHelperTest, AllowSite_Idempotent) {
  // Calling multiple times should not crash.
  helper_->AllowSite(GURL("https://example.com"));
  helper_->AllowSite(GURL("https://example.com"));
  SUCCEED();
}

TEST_F(SafetyHelperTest, BlockSite_Idempotent) {
  // Calling multiple times should not crash.
  helper_->BlockSite(GURL("https://evil.com"));
  helper_->BlockSite(GURL("https://evil.com"));
  SUCCEED();
}

TEST_F(SafetyHelperTest, IsSiteAllowed_InvalidUrl) {
  EXPECT_FALSE(helper_->IsSiteAllowed(GURL("")));
}

TEST_F(SafetyHelperTest, IsSiteBlocked_InvalidUrl) {
  EXPECT_FALSE(helper_->IsSiteBlocked(GURL("")));
}

// ---------------------------------------------------------------------------
// Threat info struct
// ---------------------------------------------------------------------------

TEST_F(SafetyHelperTest, ThreatInfo_DefaultConstructed) {
  AstraThreatInfo threat;
  EXPECT_TRUE(threat.url.is_empty());
  EXPECT_EQ(threat.threat_type, AstraThreatType::kMalware);
  EXPECT_EQ(threat.severity, 0);
  EXPECT_TRUE(threat.discovered_time.is_null());
  EXPECT_FALSE(threat.is_blocked);
  EXPECT_TRUE(threat.user_action.empty());
}

TEST_F(SafetyHelperTest, ThreatInfo_CanSetFields) {
  AstraThreatInfo threat;
  threat.url = GURL("https://evil.com/malware.exe");
  threat.threat_type = AstraThreatType::kHarmful;
  threat.severity = 9;
  threat.discovered_time = base::Time::Now();
  threat.is_blocked = true;
  threat.user_action = "blocked";

  EXPECT_EQ(threat.url.host(), "evil.com");
  EXPECT_EQ(threat.threat_type, AstraThreatType::kHarmful);
  EXPECT_EQ(threat.severity, 9);
  EXPECT_TRUE(threat.is_blocked);
  EXPECT_EQ(threat.user_action, "blocked");
}

// ---------------------------------------------------------------------------
// Security status struct
// ---------------------------------------------------------------------------

TEST_F(SafetyHelperTest, SecurityStatus_DefaultConstructed) {
  AstraSecurityStatus status;
  EXPECT_FALSE(status.is_secure);
  EXPECT_EQ(status.security_level, AstraSafetyLevel::kUnknown);
  EXPECT_FALSE(status.has_mixed_content);
  EXPECT_FALSE(status.has_valid_cert);
  EXPECT_TRUE(status.certificate_status.empty());
  EXPECT_EQ(status.cookie_count, 0);
  EXPECT_EQ(status.permission_count, 0);
}

TEST_F(SafetyHelperTest, SecurityStatus_CanSetFields) {
  AstraSecurityStatus status;
  status.is_secure = true;
  status.security_level = AstraSafetyLevel::kSafe;
  status.has_mixed_content = false;
  status.has_valid_cert = true;
  status.certificate_status = "Valid certificate";
  status.cookie_count = 12;
  status.permission_count = 3;

  EXPECT_TRUE(status.is_secure);
  EXPECT_EQ(status.security_level, AstraSafetyLevel::kSafe);
  EXPECT_TRUE(status.has_valid_cert);
  EXPECT_EQ(status.certificate_status, "Valid certificate");
  EXPECT_EQ(status.cookie_count, 12);
  EXPECT_EQ(status.permission_count, 3);
}

// ---------------------------------------------------------------------------
// Observer notifications (all observer methods)
// ---------------------------------------------------------------------------

TEST_F(SafetyHelperTest, NotifyThreatDetected_FiresObserver) {
  TestSafetyObserver observer;
  helper_->AddObserver(&observer);

  auto threat = MakeThreatInfo(AstraThreatType::kMalware,
                               "https://evil.com", 8, false);
  helper_->NotifyThreatDetected(threat);

  EXPECT_EQ(observer.threat_detected_count_, 1);
  EXPECT_EQ(observer.last_detected_threat_.threat_type, AstraThreatType::kMalware);
  EXPECT_EQ(observer.last_detected_threat_.url.host(), "evil.com");

  helper_->RemoveObserver(&observer);
}

TEST_F(SafetyHelperTest, NotifyThreatBlocked_FiresObserver) {
  TestSafetyObserver observer;
  helper_->AddObserver(&observer);

  auto threat = MakeThreatInfo(AstraThreatType::kHarmful,
                               "https://evil.com/download", 9, true);
  helper_->NotifyThreatBlocked(threat);

  EXPECT_EQ(observer.threat_blocked_count_, 1);
  EXPECT_EQ(observer.last_blocked_threat_.threat_type, AstraThreatType::kHarmful);
  EXPECT_TRUE(observer.last_blocked_threat_.is_blocked);

  helper_->RemoveObserver(&observer);
}

TEST_F(SafetyHelperTest, NotifyUserProceeded_FiresObserver) {
  TestSafetyObserver observer;
  helper_->AddObserver(&observer);

  GURL url("https://warning-site.com/page");
  helper_->NotifyUserProceeded(url);

  EXPECT_EQ(observer.user_proceeded_count_, 1);
  EXPECT_EQ(observer.last_proceeded_url_, url);

  helper_->RemoveObserver(&observer);
}

TEST_F(SafetyHelperTest, NotifySecurityStatusChanged_FiresObserver) {
  TestSafetyObserver observer;
  helper_->AddObserver(&observer);

  AstraSecurityStatus status;
  status.is_secure = true;
  status.security_level = AstraSafetyLevel::kSafe;
  helper_->NotifySecurityStatusChanged(status);

  EXPECT_EQ(observer.security_status_changed_count_, 1);
  EXPECT_TRUE(observer.last_security_status_.is_secure);

  helper_->RemoveObserver(&observer);
}

TEST_F(SafetyHelperTest, NotifySafeBrowsingToggled_FiresObserver) {
  TestSafetyObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifySafeBrowsingToggled(false);

  EXPECT_EQ(observer.safe_browsing_toggled_count_, 1);
  EXPECT_FALSE(observer.last_safe_browsing_enabled_);

  helper_->RemoveObserver(&observer);
}

TEST_F(SafetyHelperTest, NotifySafetySettingsChanged_FiresObserver) {
  TestSafetyObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifySafetySettingsChanged();

  EXPECT_EQ(observer.safety_settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(SafetyHelperTest, NotifyPasswordBreachDetected_FiresObserver) {
  TestSafetyObserver observer;
  helper_->AddObserver(&observer);

  helper_->NotifyPasswordBreachDetected("bank.com");

  EXPECT_EQ(observer.password_breach_count_, 1);
  EXPECT_EQ(observer.last_breach_site_, "bank.com");

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Observer defaults (empty implementations don't crash)
// ---------------------------------------------------------------------------

TEST_F(SafetyHelperTest, ObserverDefaults_DoNotCrash) {
  // Create a minimal observer that overrides nothing.
  // All methods have default empty implementations, so this should compile
  // and not crash.
  class DefaultObserver : public AstraSafetyObserver {};

  DefaultObserver observer;
  helper_->AddObserver(&observer);

  // Trigger all observer paths via settings changes and notifications.
  helper_->SetSafeBrowsingEnabled(false);
  helper_->SetSafeBrowsingLevel(2);
  helper_->SetShowSecurityButton(false);
  helper_->SetShowThreatNotifications(false);
  helper_->SetBlockDangerousDownloads(false);
  helper_->SetWarnOnDangerousDownloads(false);
  helper_->SetAutoReportSafetyIssues(true);
  helper_->SetMixedContentWarning(false);
  helper_->SetShowSiteInfoButton(false);
  helper_->SetSafetyCheckReminders(false);
  helper_->SetCookieProtection(false);
  helper_->SetWarnOnPasswordReuse(false);
  helper_->SetPasswordProtectionLevel(2);

  auto threat = MakeThreatInfo(AstraThreatType::kMalware,
                               "https://evil.com", 8, false);
  helper_->NotifyThreatDetected(threat);
  helper_->NotifyThreatBlocked(threat);
  helper_->NotifyUserProceeded(GURL("https://test.com"));
  helper_->NotifySecurityStatusChanged(AstraSecurityStatus());
  helper_->NotifySafeBrowsingToggled(true);
  helper_->NotifySafetySettingsChanged();
  helper_->NotifyPasswordBreachDetected("example.com");

  helper_->RemoveObserver(&observer);
  SUCCEED() << "Default observer methods do not crash.";
}

// ---------------------------------------------------------------------------
// Multiple observers
// ---------------------------------------------------------------------------

TEST_F(SafetyHelperTest, MultipleObservers_AllNotified) {
  TestSafetyObserver observer1;
  TestSafetyObserver observer2;

  helper_->AddObserver(&observer1);
  helper_->AddObserver(&observer2);

  helper_->SetSafeBrowsingEnabled(false);

  EXPECT_EQ(observer1.safe_browsing_toggled_count_, 1);
  EXPECT_EQ(observer2.safe_browsing_toggled_count_, 1);

  helper_->RemoveObserver(&observer1);
  helper_->RemoveObserver(&observer2);
}

TEST_F(SafetyHelperTest, RemoveObserver_StopsNotifications) {
  TestSafetyObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetSafeBrowsingEnabled(false);
  EXPECT_EQ(observer.safe_browsing_toggled_count_, 1);

  helper_->RemoveObserver(&observer);

  helper_->SetSafeBrowsingEnabled(true);
  // Should not have been notified after removal.
  EXPECT_EQ(observer.safe_browsing_toggled_count_, 1);
}

TEST_F(SafetyHelperTest, RemoveNonexistentObserver_NoCrash) {
  TestSafetyObserver observer;
  helper_->RemoveObserver(&observer);
  SUCCEED();
}

TEST_F(SafetyHelperTest, AddRemoveObserver_MultipleCycles) {
  TestSafetyObserver observer;

  for (int i = 0; i < 5; i++) {
    helper_->AddObserver(&observer);
    helper_->RemoveObserver(&observer);
  }
  SUCCEED();
}

// ---------------------------------------------------------------------------
// Password protection settings
// ---------------------------------------------------------------------------

TEST_F(SafetyHelperTest, GetPasswordProtectionLevel_DefaultIsStandard) {
  EXPECT_EQ(helper_->GetPasswordProtectionLevel(), 1);
}

TEST_F(SafetyHelperTest, SetPasswordProtectionLevel_ChangesValue) {
  helper_->SetPasswordProtectionLevel(2);
  EXPECT_EQ(helper_->GetPasswordProtectionLevel(), 2);

  helper_->SetPasswordProtectionLevel(0);
  EXPECT_EQ(helper_->GetPasswordProtectionLevel(), 0);
}

TEST_F(SafetyHelperTest, SetPasswordProtectionLevel_ClampsToZero) {
  helper_->SetPasswordProtectionLevel(-5);
  EXPECT_EQ(helper_->GetPasswordProtectionLevel(), 0);
}

TEST_F(SafetyHelperTest, SetPasswordProtectionLevel_ClampsToTwo) {
  helper_->SetPasswordProtectionLevel(50);
  EXPECT_EQ(helper_->GetPasswordProtectionLevel(), 2);
}

TEST_F(SafetyHelperTest, SetPasswordProtectionLevel_SameValueNoOp) {
  TestSafetyObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetPasswordProtectionLevel(1);
  EXPECT_EQ(observer.safety_settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(SafetyHelperTest, SetPasswordProtectionLevel_FiresSettingsObserver) {
  TestSafetyObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetPasswordProtectionLevel(2);
  EXPECT_EQ(observer.safety_settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(SafetyHelperTest, WarnOnPasswordReuse_DefaultTrue) {
  EXPECT_TRUE(helper_->WarnOnPasswordReuse());
}

TEST_F(SafetyHelperTest, SetWarnOnPasswordReuse_ChangesValue) {
  helper_->SetWarnOnPasswordReuse(false);
  EXPECT_FALSE(helper_->WarnOnPasswordReuse());

  helper_->SetWarnOnPasswordReuse(true);
  EXPECT_TRUE(helper_->WarnOnPasswordReuse());
}

TEST_F(SafetyHelperTest, SetWarnOnPasswordReuse_SameValueNoOp) {
  TestSafetyObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetWarnOnPasswordReuse(true);
  EXPECT_EQ(observer.safety_settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(SafetyHelperTest, SetWarnOnPasswordReuse_FiresSettingsObserver) {
  TestSafetyObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetWarnOnPasswordReuse(false);
  EXPECT_EQ(observer.safety_settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Mixed content warnings
// ---------------------------------------------------------------------------

TEST_F(SafetyHelperTest, MixedContentWarning_DefaultTrue) {
  EXPECT_TRUE(helper_->GetMixedContentWarning());
}

TEST_F(SafetyHelperTest, SetMixedContentWarning_ChangesValue) {
  helper_->SetMixedContentWarning(false);
  EXPECT_FALSE(helper_->GetMixedContentWarning());

  helper_->SetMixedContentWarning(true);
  EXPECT_TRUE(helper_->GetMixedContentWarning());
}

TEST_F(SafetyHelperTest, SetMixedContentWarning_FiresSettingsObserver) {
  TestSafetyObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetMixedContentWarning(false);
  EXPECT_EQ(observer.safety_settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(SafetyHelperTest, SetMixedContentWarning_SameValueNoOp) {
  TestSafetyObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetMixedContentWarning(true);
  EXPECT_EQ(observer.safety_settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Presentation settings (all 14 settings)
// ---------------------------------------------------------------------------

TEST_F(SafetyHelperTest, ShowSecurityButton_SetChangesValue) {
  helper_->SetShowSecurityButton(false);
  EXPECT_FALSE(helper_->GetShowSecurityButton());
}

TEST_F(SafetyHelperTest, ShowThreatNotifications_SetChangesValue) {
  helper_->SetShowThreatNotifications(false);
  EXPECT_FALSE(helper_->GetShowThreatNotifications());
}

TEST_F(SafetyHelperTest, BlockDangerousDownloads_SetChangesValue) {
  helper_->SetBlockDangerousDownloads(false);
  EXPECT_FALSE(helper_->GetBlockDangerousDownloads());
}

TEST_F(SafetyHelperTest, WarnOnDangerousDownloads_SetChangesValue) {
  helper_->SetWarnOnDangerousDownloads(false);
  EXPECT_FALSE(helper_->GetWarnOnDangerousDownloads());
}

TEST_F(SafetyHelperTest, AutoReportSafetyIssues_SetChangesValue) {
  helper_->SetAutoReportSafetyIssues(true);
  EXPECT_TRUE(helper_->GetAutoReportSafetyIssues());
}

TEST_F(SafetyHelperTest, ShowSiteInfoButton_SetChangesValue) {
  helper_->SetShowSiteInfoButton(false);
  EXPECT_FALSE(helper_->GetShowSiteInfoButton());
}

TEST_F(SafetyHelperTest, SafetyCheckReminders_SetChangesValue) {
  helper_->SetSafetyCheckReminders(false);
  EXPECT_FALSE(helper_->GetSafetyCheckReminders());
}

TEST_F(SafetyHelperTest, CookieProtection_SetChangesValue) {
  helper_->SetCookieProtection(false);
  EXPECT_FALSE(helper_->GetCookieProtection());
}

TEST_F(SafetyHelperTest, AllPresentationSettings_DefaultsAreCorrect) {
  EXPECT_TRUE(helper_->GetShowSecurityButton());
  EXPECT_TRUE(helper_->GetShowThreatNotifications());
  EXPECT_TRUE(helper_->GetBlockDangerousDownloads());
  EXPECT_TRUE(helper_->GetWarnOnDangerousDownloads());
  EXPECT_FALSE(helper_->GetAutoReportSafetyIssues());
  EXPECT_TRUE(helper_->GetMixedContentWarning());
  EXPECT_TRUE(helper_->GetShowSiteInfoButton());
  EXPECT_TRUE(helper_->GetSafetyCheckReminders());
  EXPECT_TRUE(helper_->GetCookieProtection());
  EXPECT_TRUE(helper_->WarnOnPasswordReuse());
}

TEST_F(SafetyHelperTest, AllPresentationSettings_AllFireSettingsChanged) {
  TestSafetyObserver observer;
  helper_->AddObserver(&observer);

  int initial_count = observer.safety_settings_changed_count_;

  helper_->SetShowSecurityButton(false);
  helper_->SetShowThreatNotifications(false);
  helper_->SetBlockDangerousDownloads(false);
  helper_->SetWarnOnDangerousDownloads(false);
  helper_->SetAutoReportSafetyIssues(true);
  helper_->SetMixedContentWarning(false);
  helper_->SetShowSiteInfoButton(false);
  helper_->SetSafetyCheckReminders(false);
  helper_->SetCookieProtection(false);
  helper_->SetWarnOnPasswordReuse(false);

  EXPECT_EQ(observer.safety_settings_changed_count_ - initial_count, 10);

  helper_->RemoveObserver(&observer);
}

TEST_F(SafetyHelperTest, AllPresentationSettings_SameValueNoOp) {
  TestSafetyObserver observer;
  helper_->AddObserver(&observer);

  // All these are already at default values — should be no-op.
  helper_->SetShowSecurityButton(true);
  helper_->SetShowThreatNotifications(true);
  helper_->SetBlockDangerousDownloads(true);
  helper_->SetWarnOnDangerousDownloads(true);
  helper_->SetAutoReportSafetyIssues(false);
  helper_->SetMixedContentWarning(true);
  helper_->SetShowSiteInfoButton(true);
  helper_->SetSafetyCheckReminders(true);
  helper_->SetCookieProtection(true);
  helper_->SetWarnOnPasswordReuse(true);

  EXPECT_EQ(observer.safety_settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Settings clamping (levels 0-2)
// ---------------------------------------------------------------------------

TEST_F(SafetyHelperTest, SafeBrowsingLevel_ClampNegative) {
  helper_->SetSafeBrowsingLevel(-100);
  EXPECT_EQ(helper_->GetSafeBrowsingLevel(), 0);
}

TEST_F(SafetyHelperTest, SafeBrowsingLevel_ClampAboveMax) {
  helper_->SetSafeBrowsingLevel(10);
  EXPECT_EQ(helper_->GetSafeBrowsingLevel(), 2);
}

TEST_F(SafetyHelperTest, PasswordProtectionLevel_ClampNegative) {
  helper_->SetPasswordProtectionLevel(-5);
  EXPECT_EQ(helper_->GetPasswordProtectionLevel(), 0);
}

TEST_F(SafetyHelperTest, PasswordProtectionLevel_ClampAboveMax) {
  helper_->SetPasswordProtectionLevel(99);
  EXPECT_EQ(helper_->GetPasswordProtectionLevel(), 2);
}

TEST_F(SafetyHelperTest, SafeBrowsingLevel_ValidValues) {
  for (int level : {0, 1, 2}) {
    helper_->SetSafeBrowsingLevel(level);
    EXPECT_EQ(helper_->GetSafeBrowsingLevel(), level);
  }
}

TEST_F(SafetyHelperTest, PasswordProtectionLevel_ValidValues) {
  for (int level : {0, 1, 2}) {
    helper_->SetPasswordProtectionLevel(level);
    EXPECT_EQ(helper_->GetPasswordProtectionLevel(), level);
  }
}

// ---------------------------------------------------------------------------
// Persistence round-trip via PrefService
// ---------------------------------------------------------------------------

TEST_F(SafetyHelperTest, PrefsPersist_SafeBrowsingEnabled) {
  helper_->SetSafeBrowsingEnabled(false);

  // Create a new helper with the same profile — should read persisted value.
  auto helper2 = std::make_unique<AstraSafetyHelper>(profile_.get());
  EXPECT_FALSE(helper2->IsSafeBrowsingEnabled());
}

TEST_F(SafetyHelperTest, PrefsPersist_SafeBrowsingLevel) {
  helper_->SetSafeBrowsingLevel(2);

  auto helper2 = std::make_unique<AstraSafetyHelper>(profile_.get());
  EXPECT_EQ(helper2->GetSafeBrowsingLevel(), 2);
}

TEST_F(SafetyHelperTest, PrefsPersist_PasswordProtectionLevel) {
  helper_->SetPasswordProtectionLevel(0);

  auto helper2 = std::make_unique<AstraSafetyHelper>(profile_.get());
  EXPECT_EQ(helper2->GetPasswordProtectionLevel(), 0);
}

TEST_F(SafetyHelperTest, PrefsPersist_WarnOnPasswordReuse) {
  helper_->SetWarnOnPasswordReuse(false);

  auto helper2 = std::make_unique<AstraSafetyHelper>(profile_.get());
  EXPECT_FALSE(helper2->WarnOnPasswordReuse());
}

TEST_F(SafetyHelperTest, PrefsPersist_ShowSecurityButton) {
  helper_->SetShowSecurityButton(false);

  auto helper2 = std::make_unique<AstraSafetyHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetShowSecurityButton());
}

TEST_F(SafetyHelperTest, PrefsPersist_ShowThreatNotifications) {
  helper_->SetShowThreatNotifications(false);

  auto helper2 = std::make_unique<AstraSafetyHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetShowThreatNotifications());
}

TEST_F(SafetyHelperTest, PrefsPersist_BlockDangerousDownloads) {
  helper_->SetBlockDangerousDownloads(false);

  auto helper2 = std::make_unique<AstraSafetyHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetBlockDangerousDownloads());
}

TEST_F(SafetyHelperTest, PrefsPersist_WarnOnDangerousDownloads) {
  helper_->SetWarnOnDangerousDownloads(false);

  auto helper2 = std::make_unique<AstraSafetyHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetWarnOnDangerousDownloads());
}

TEST_F(SafetyHelperTest, PrefsPersist_AutoReportSafetyIssues) {
  helper_->SetAutoReportSafetyIssues(true);

  auto helper2 = std::make_unique<AstraSafetyHelper>(profile_.get());
  EXPECT_TRUE(helper2->GetAutoReportSafetyIssues());
}

TEST_F(SafetyHelperTest, PrefsPersist_MixedContentWarning) {
  helper_->SetMixedContentWarning(false);

  auto helper2 = std::make_unique<AstraSafetyHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetMixedContentWarning());
}

TEST_F(SafetyHelperTest, PrefsPersist_ShowSiteInfoButton) {
  helper_->SetShowSiteInfoButton(false);

  auto helper2 = std::make_unique<AstraSafetyHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetShowSiteInfoButton());
}

TEST_F(SafetyHelperTest, PrefsPersist_SafetyCheckReminders) {
  helper_->SetSafetyCheckReminders(false);

  auto helper2 = std::make_unique<AstraSafetyHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetSafetyCheckReminders());
}

TEST_F(SafetyHelperTest, PrefsPersist_CookieProtection) {
  helper_->SetCookieProtection(false);

  auto helper2 = std::make_unique<AstraSafetyHelper>(profile_.get());
  EXPECT_FALSE(helper2->GetCookieProtection());
}

TEST_F(SafetyHelperTest, PrefsPersist_DefaultValues) {
  // Create a fresh helper — should have default values.
  auto helper2 = std::make_unique<AstraSafetyHelper>(profile_.get());

  EXPECT_TRUE(helper2->IsSafeBrowsingEnabled());
  EXPECT_EQ(helper2->GetSafeBrowsingLevel(), prefs::kDefaultSafeBrowsingLevel);
  EXPECT_EQ(helper2->GetPasswordProtectionLevel(),
            prefs::kDefaultPasswordProtectionLevel);
  EXPECT_TRUE(helper2->WarnOnPasswordReuse());
  EXPECT_TRUE(helper2->GetShowSecurityButton());
  EXPECT_TRUE(helper2->GetShowThreatNotifications());
  EXPECT_TRUE(helper2->GetBlockDangerousDownloads());
  EXPECT_TRUE(helper2->GetWarnOnDangerousDownloads());
  EXPECT_FALSE(helper2->GetAutoReportSafetyIssues());
  EXPECT_TRUE(helper2->GetMixedContentWarning());
  EXPECT_TRUE(helper2->GetShowSiteInfoButton());
  EXPECT_TRUE(helper2->GetSafetyCheckReminders());
  EXPECT_TRUE(helper2->GetCookieProtection());
}

// ---------------------------------------------------------------------------
// Bulk operations
// ---------------------------------------------------------------------------

TEST_F(SafetyHelperTest, AllowSites_EmptyListNoCrash) {
  helper_->AllowSites({});
  SUCCEED();
}

TEST_F(SafetyHelperTest, BlockSites_EmptyListNoCrash) {
  helper_->BlockSites({});
  SUCCEED();
}

TEST_F(SafetyHelperTest, AllowSites_MultipleUrls) {
  std::vector<GURL> urls = {
    GURL("https://site1.com"),
    GURL("https://site2.com"),
    GURL("https://site3.com"),
  };
  helper_->AllowSites(urls);
  SUCCEED();
}

TEST_F(SafetyHelperTest, BlockSites_MultipleUrls) {
  std::vector<GURL> urls = {
    GURL("https://evil1.com"),
    GURL("https://evil2.com"),
  };
  helper_->BlockSites(urls);
  SUCCEED();
}

TEST_F(SafetyHelperTest, RemoveSitesFromAllowlist_EmptyListNoCrash) {
  helper_->RemoveSitesFromAllowlist({});
  SUCCEED();
}

TEST_F(SafetyHelperTest, RemoveSitesFromAllowlist_MultipleUrls) {
  std::vector<GURL> urls = {
    GURL("https://site1.com"),
    GURL("https://site2.com"),
  };
  helper_->RemoveSitesFromAllowlist(urls);
  SUCCEED();
}

// ---------------------------------------------------------------------------
// Utility methods (all 6 utility methods)
// ---------------------------------------------------------------------------

TEST_F(SafetyHelperTest, GetSafetyLevelLabel_Safe) {
  EXPECT_EQ(AstraSafetyHelper::GetSafetyLevelLabel(AstraSafetyLevel::kSafe),
            "Safe");
}

TEST_F(SafetyHelperTest, GetSafetyLevelLabel_Warning) {
  EXPECT_EQ(AstraSafetyHelper::GetSafetyLevelLabel(AstraSafetyLevel::kWarning),
            "Warning");
}

TEST_F(SafetyHelperTest, GetSafetyLevelLabel_Dangerous) {
  EXPECT_EQ(AstraSafetyHelper::GetSafetyLevelLabel(AstraSafetyLevel::kDangerous),
            "Dangerous");
}

TEST_F(SafetyHelperTest, GetSafetyLevelLabel_Unknown) {
  EXPECT_EQ(AstraSafetyHelper::GetSafetyLevelLabel(AstraSafetyLevel::kUnknown),
            "Unknown");
}

TEST_F(SafetyHelperTest, GetThreatTypeLabel_Malware) {
  EXPECT_EQ(AstraSafetyHelper::GetThreatTypeLabel(AstraThreatType::kMalware),
            "Malware");
}

TEST_F(SafetyHelperTest, GetThreatTypeLabel_Deceptive) {
  EXPECT_EQ(AstraSafetyHelper::GetThreatTypeLabel(AstraThreatType::kDeceptive),
            "Deceptive content");
}

TEST_F(SafetyHelperTest, GetThreatTypeLabel_Unwanted) {
  EXPECT_EQ(AstraSafetyHelper::GetThreatTypeLabel(AstraThreatType::kUnwanted),
            "Unwanted software");
}

TEST_F(SafetyHelperTest, GetThreatTypeLabel_Harmful) {
  EXPECT_EQ(AstraSafetyHelper::GetThreatTypeLabel(AstraThreatType::kHarmful),
            "Harmful download");
}

TEST_F(SafetyHelperTest, GetThreatTypeLabel_PasswordReuse) {
  EXPECT_EQ(AstraSafetyHelper::GetThreatTypeLabel(
      AstraThreatType::kPasswordReuse), "Password reuse");
}

TEST_F(SafetyHelperTest, GetThreatTypeLabel_MixedContent) {
  EXPECT_EQ(AstraSafetyHelper::GetThreatTypeLabel(AstraThreatType::kMixedContent),
            "Mixed content");
}

TEST_F(SafetyHelperTest, GetThreatTypeIcon_Malware) {
  EXPECT_EQ(AstraSafetyHelper::GetThreatTypeIcon(AstraThreatType::kMalware),
            "shield_error");
}

TEST_F(SafetyHelperTest, GetThreatTypeIcon_PasswordReuse) {
  EXPECT_EQ(AstraSafetyHelper::GetThreatTypeIcon(
      AstraThreatType::kPasswordReuse), "password");
}

TEST_F(SafetyHelperTest, GetThreatTypeIcon_MixedContent) {
  EXPECT_EQ(AstraSafetyHelper::GetThreatTypeIcon(
      AstraThreatType::kMixedContent), "http");
}

TEST_F(SafetyHelperTest, IsSevereThreat_MalwareIsSevere) {
  EXPECT_TRUE(AstraSafetyHelper::IsSevereThreat(AstraThreatType::kMalware));
}

TEST_F(SafetyHelperTest, IsSevereThreat_DeceptiveIsSevere) {
  EXPECT_TRUE(AstraSafetyHelper::IsSevereThreat(AstraThreatType::kDeceptive));
}

TEST_F(SafetyHelperTest, IsSevereThreat_UnwantedIsSevere) {
  EXPECT_TRUE(AstraSafetyHelper::IsSevereThreat(AstraThreatType::kUnwanted));
}

TEST_F(SafetyHelperTest, IsSevereThreat_HarmfulIsSevere) {
  EXPECT_TRUE(AstraSafetyHelper::IsSevereThreat(AstraThreatType::kHarmful));
}

TEST_F(SafetyHelperTest, IsSevereThreat_PasswordReuseNotSevere) {
  EXPECT_FALSE(AstraSafetyHelper::IsSevereThreat(
      AstraThreatType::kPasswordReuse));
}

TEST_F(SafetyHelperTest, IsSevereThreat_MixedContentNotSevere) {
  EXPECT_FALSE(AstraSafetyHelper::IsSevereThreat(
      AstraThreatType::kMixedContent));
}

TEST_F(SafetyHelperTest, FormatThreatDescription_WithUrl) {
  auto threat = MakeThreatInfo(AstraThreatType::kMalware,
                               "https://evil.com/malware.exe", 8, false);
  std::string desc = AstraSafetyHelper::FormatThreatDescription(threat);
  EXPECT_NE(desc.find("Malware"), std::string::npos);
  EXPECT_NE(desc.find("evil.com"), std::string::npos);
}

TEST_F(SafetyHelperTest, FormatThreatDescription_Blocked) {
  auto threat = MakeThreatInfo(AstraThreatType::kHarmful,
                               "https://evil.com/bad.exe", 9, true);
  std::string desc = AstraSafetyHelper::FormatThreatDescription(threat);
  EXPECT_NE(desc.find("blocked"), std::string::npos);
}

TEST_F(SafetyHelperTest, FormatThreatDescription_WithUserAction) {
  auto threat = MakeThreatInfo(AstraThreatType::kMalware,
                               "https://evil.com", 7, false);
  threat.user_action = "proceed";
  std::string desc = AstraSafetyHelper::FormatThreatDescription(threat);
  EXPECT_NE(desc.find("proceed"), std::string::npos);
}

TEST_F(SafetyHelperTest, FormatThreatDescription_EmptyUrl) {
  AstraThreatInfo threat;
  threat.threat_type = AstraThreatType::kMalware;
  std::string desc = AstraSafetyHelper::FormatThreatDescription(threat);
  EXPECT_FALSE(desc.empty());
}

TEST_F(SafetyHelperTest, GetProtectionLevelLabel_Off) {
  EXPECT_EQ(AstraSafetyHelper::GetProtectionLevelLabel(0), "Off");
}

TEST_F(SafetyHelperTest, GetProtectionLevelLabel_Standard) {
  EXPECT_EQ(AstraSafetyHelper::GetProtectionLevelLabel(1),
            "Standard protection");
}

TEST_F(SafetyHelperTest, GetProtectionLevelLabel_Enhanced) {
  EXPECT_EQ(AstraSafetyHelper::GetProtectionLevelLabel(2),
            "Enhanced protection");
}

TEST_F(SafetyHelperTest, GetProtectionLevelLabel_Invalid) {
  EXPECT_EQ(AstraSafetyHelper::GetProtectionLevelLabel(99), "Unknown");
  EXPECT_EQ(AstraSafetyHelper::GetProtectionLevelLabel(-1), "Unknown");
}

// ---------------------------------------------------------------------------
// Edge cases
// ---------------------------------------------------------------------------

TEST_F(SafetyHelperTest, EmptyUrl_IsSiteSafe) {
  EXPECT_TRUE(helper_->IsSiteSafe(GURL()));
}

TEST_F(SafetyHelperTest, EmptyUrl_AllowSite) {
  helper_->AllowSite(GURL());
  // Should not crash and should not add anything.
  SUCCEED();
}

TEST_F(SafetyHelperTest, EmptyUrl_BlockSite) {
  helper_->BlockSite(GURL());
  SUCCEED();
}

TEST_F(SafetyHelperTest, InvalidUrl_IsSiteSafe) {
  EXPECT_TRUE(helper_->IsSiteSafe(GURL("not a valid url")));
}

TEST_F(SafetyHelperTest, InvalidThreatLevel_Clamped) {
  // All level setters clamp to 0-2.
  helper_->SetSafeBrowsingLevel(INT_MIN);
  EXPECT_EQ(helper_->GetSafeBrowsingLevel(), 0);

  helper_->SetSafeBrowsingLevel(INT_MAX);
  EXPECT_EQ(helper_->GetSafeBrowsingLevel(), 2);
}

TEST_F(SafetyHelperTest, GetRecentThreats_NoThreats) {
  auto threats = helper_->GetRecentThreats(100);
  EXPECT_TRUE(threats.empty());
}

TEST_F(SafetyHelperTest, GetRecentThreats_MaxCountZero) {
  auto threat = MakeThreatInfo(AstraThreatType::kMalware,
                               "https://evil.com", 5, false);
  helper_->NotifyThreatDetected(threat);

  auto threats = helper_->GetRecentThreats(0);
  // Zero means all — should return everything.
  EXPECT_EQ(threats.size(), 1u);
}

TEST_F(SafetyHelperTest, UnknownThreatType_NotSevere) {
  // Edge case: threat types not in the severe list are not severe.
  EXPECT_FALSE(AstraSafetyHelper::IsSevereThreat(
      static_cast<AstraThreatType>(999)));
}

TEST_F(SafetyHelperTest, UnknownSafetyLevel_Label) {
  // Out-of-range safety level returns "Unknown".
  EXPECT_EQ(AstraSafetyHelper::GetSafetyLevelLabel(
      static_cast<AstraSafetyLevel>(999)), "Unknown");
}

// ---------------------------------------------------------------------------
// Factory tests
// ---------------------------------------------------------------------------

TEST_F(SafetyHelperTest, Factory_GetForProfile) {
  // Register prefs and create via factory.
  AstraSafetyHelper* helper = AstraSafetyHelperFactory::GetForProfile(
      profile_.get());
  EXPECT_NE(helper, nullptr);
}

TEST_F(SafetyHelperTest, Factory_GetForProfileNullProfile) {
  AstraSafetyHelper* helper = AstraSafetyHelperFactory::GetForProfile(nullptr);
  EXPECT_EQ(helper, nullptr);
}

TEST_F(SafetyHelperTest, Factory_GetInstance_Singleton) {
  auto* instance1 = AstraSafetyHelperFactory::GetInstance();
  auto* instance2 = AstraSafetyHelperFactory::GetInstance();
  EXPECT_EQ(instance1, instance2);
}

TEST_F(SafetyHelperTest, Factory_RegisterProfilePrefs) {
  // Create a fresh pref registry and register prefs.
  TestingProfile profile2;
  AstraSafetyHelperFactory::RegisterProfilePrefs(
      profile2.GetPrefs());

  // Verify that prefs are registered with correct defaults.
  EXPECT_TRUE(profile2.GetPrefs()->GetBoolean(
      prefs::kPrefSafeBrowsingEnabled));
  EXPECT_EQ(profile2.GetPrefs()->GetInteger(
      prefs::kPrefSafeBrowsingLevel), 1);
  EXPECT_TRUE(profile2.GetPrefs()->GetBoolean(
      prefs::kPrefShowSecurityButton));
}

// ---------------------------------------------------------------------------
// Shutdown cleanup
// ---------------------------------------------------------------------------

TEST_F(SafetyHelperTest, Shutdown_CleansUp) {
  TestSafetyObserver observer;
  helper_->AddObserver(&observer);

  helper_->Shutdown();

  // After shutdown, profile is null so settings changes should not notify.
  helper_->SetSafeBrowsingEnabled(false);
  EXPECT_EQ(observer.safe_browsing_toggled_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(SafetyHelperTest, Shutdown_MultipleTimesNoCrash) {
  helper_->Shutdown();
  helper_->Shutdown();
  SUCCEED();
}

TEST_F(SafetyHelperTest, Shutdown_GetPrefsReturnsNull) {
  helper_->Shutdown();

  // After shutdown, settings should return defaults (no crash).
  EXPECT_TRUE(helper_->IsSafeBrowsingEnabled());  // returns default
}

// ---------------------------------------------------------------------------
// Safety check operations
// ---------------------------------------------------------------------------

TEST_F(SafetyHelperTest, RunSafetyCheck_ProducesResults) {
  helper_->RunSafetyCheck();
  auto results = helper_->GetSafetyCheckResults();
  EXPECT_FALSE(results.empty());
}

TEST_F(SafetyHelperTest, RunSafetyCheck_MultipleRuns) {
  helper_->RunSafetyCheck();
  auto results1 = helper_->GetSafetyCheckResults();

  helper_->RunSafetyCheck();
  auto results2 = helper_->GetSafetyCheckResults();

  EXPECT_EQ(results1.size(), results2.size());
}

TEST_F(SafetyHelperTest, RunSafetyCheck_FiresSecurityStatusObserver) {
  TestSafetyObserver observer;
  helper_->AddObserver(&observer);

  helper_->RunSafetyCheck();
  EXPECT_GE(observer.security_status_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(SafetyHelperTest, RunSafetyCheck_FiresSettingsObserver) {
  TestSafetyObserver observer;
  helper_->AddObserver(&observer);

  helper_->RunSafetyCheck();
  EXPECT_GE(observer.safety_settings_changed_count_, 1);

  helper_->RemoveObserver(&observer);
}

TEST_F(SafetyHelperTest, GetSafetyCheckResults_BeforeRunIsEmpty) {
  auto results = helper_->GetSafetyCheckResults();
  EXPECT_TRUE(results.empty());
}

TEST_F(SafetyHelperTest, RunSafetyCheck_WithBreaches) {
  helper_->NotifyPasswordBreachDetected("example.com");
  helper_->RunSafetyCheck();

  auto results = helper_->GetSafetyCheckResults();
  bool has_breach_info = false;
  for (const auto& result : results) {
    if (result.find("breached") != std::string::npos) {
      has_breach_info = true;
      break;
    }
  }
  EXPECT_TRUE(has_breach_info);
}

// ---------------------------------------------------------------------------
// Combined settings changes
// ---------------------------------------------------------------------------

TEST_F(SafetyHelperTest, MultipleSettingChanges_AllNotify) {
  TestSafetyObserver observer;
  helper_->AddObserver(&observer);

  int expected_changes = 0;

  helper_->SetSafeBrowsingEnabled(false);
  expected_changes++;  // also settings changed

  helper_->SetSafeBrowsingLevel(2);
  // Note: this might also toggle safe browsing (it was just disabled)
  // Let's not count this one precisely since level 2 enables SB

  helper_->SetShowSecurityButton(false);
  expected_changes++;

  helper_->SetShowThreatNotifications(false);
  expected_changes++;

  helper_->SetCookieProtection(false);
  expected_changes++;

  // At minimum, we should have these many notifications.
  EXPECT_GE(observer.safety_settings_changed_count_, expected_changes);

  helper_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Threat history ordering and limits
// ---------------------------------------------------------------------------

TEST_F(SafetyHelperTest, ThreatHistory_NewestFirst) {
  auto t1 = MakeThreatInfo(AstraThreatType::kMalware, "https://a.com", 1, false);
  auto t2 = MakeThreatInfo(AstraThreatType::kDeceptive, "https://b.com", 2, false);
  auto t3 = MakeThreatInfo(AstraThreatType::kUnwanted, "https://c.com", 3, false);

  helper_->NotifyThreatDetected(t1);
  helper_->NotifyThreatDetected(t2);
  helper_->NotifyThreatDetected(t3);

  auto recent = helper_->GetRecentThreats(3);
  ASSERT_EQ(recent.size(), 3u);
  EXPECT_EQ(recent[0].threat_type, AstraThreatType::kUnwanted);
  EXPECT_EQ(recent[1].threat_type, AstraThreatType::kDeceptive);
  EXPECT_EQ(recent[2].threat_type, AstraThreatType::kMalware);
}

TEST_F(SafetyHelperTest, ThreatHistory_BlockedCountOnlyCountsBlocked) {
  helper_->NotifyThreatDetected(MakeThreatInfo(
      AstraThreatType::kMalware, "https://a.com", 5, false));
  helper_->NotifyThreatBlocked(MakeThreatInfo(
      AstraThreatType::kHarmful, "https://b.com", 7, true));
  helper_->NotifyThreatDetected(MakeThreatInfo(
      AstraThreatType::kMixedContent, "https://c.com", 1, false));
  helper_->NotifyThreatBlocked(MakeThreatInfo(
      AstraThreatType::kDeceptive, "https://d.com", 8, true));

  EXPECT_EQ(helper_->GetThreatCount(), 4u);
  EXPECT_EQ(helper_->GetBlockedThreatCount(), 2u);
}

// ---------------------------------------------------------------------------
// Enhanced protection toggle
// ---------------------------------------------------------------------------

TEST_F(SafetyHelperTest, SetEnhancedProtection_Idempotent) {
  TestSafetyObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetEnhancedProtection(true);
  observer.safety_settings_changed_count_ = 0;

  helper_->SetEnhancedProtection(true);  // Same value
  EXPECT_EQ(observer.safety_settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

TEST_F(SafetyHelperTest, SetEnhancedProtection_DisableFromStandard) {
  // Standard (level 1) to disabled enhanced is no-op.
  ASSERT_EQ(helper_->GetSafeBrowsingLevel(), 1);
  ASSERT_FALSE(helper_->IsEnhancedProtectionEnabled());

  TestSafetyObserver observer;
  helper_->AddObserver(&observer);

  helper_->SetEnhancedProtection(false);
  EXPECT_EQ(observer.safety_settings_changed_count_, 0);

  helper_->RemoveObserver(&observer);
}

}  // namespace astra
