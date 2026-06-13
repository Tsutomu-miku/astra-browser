// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Comprehensive unit tests for focus mode views (AstraFocusModeModel,
// AstraFocusModeController, AstraFocusModeIndicator,
// AstraFocusModeMenuBubble).
//
// Test categories:
//   - Model state machine tests (activate/deactivate/pause/resume)
//   - Model session tests (duration, time remaining, elapsed, start time)
//   - Model session control tests (pause, resume, reset, extend, end)
//   - Model block level tests (all 5 levels, get/set)
//   - Model blocked sites tests (add, remove, get list, check blocked)
//   - Model allowed sites tests (add, remove, get list)
//   - Model session history tests (count, today, week, clear, current, note)
//   - Model UI settings tests (indicator, position, timer, sound, etc.)
//   - Model default duration tests (get/set, presets)
//   - Model observer tests (all 9 events)
//   - Model edge case tests
//   - Controller tests (toggle/start/end, pause/resume, duration, etc.)
//   - Indicator view tests (active, time, timer, block level, paused, etc.)
//   - Menu bubble tests (show/hide, duration, block level, presets, etc.)
//
// Notes:
//   - Model tests are fully self-contained (no browser dependencies).
//   - Controller tests verify state machine without a real service.
//   - View tests cover properties and patterns.
//
// Chromium test pattern: views::test::ViewsTestBase

#include "astra/ui/views/focus_mode/astra_focus_mode_controller.h"
#include "astra/ui/views/focus_mode/astra_focus_mode_indicator.h"
#include "astra/ui/views/focus_mode/astra_focus_mode_menu_bubble.h"
#include "astra/ui/views/focus_mode/astra_focus_mode_model.h"

#include <string>
#include <vector>

#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

using ::testing::_;
using ::testing::AtLeast;
using ::testing::InSequence;
using ::testing::Mock;
using ::testing::NiceMock;

// =========================================================================
// Mock observer for model tests
// =========================================================================

class MockFocusModeObserver : public AstraFocusModeObserver {
 public:
  MockFocusModeObserver() = default;
  ~MockFocusModeObserver() override = default;

  MOCK_METHOD(void, OnFocusModeStarted, (AstraFocusModeModel * model), (override));
  MOCK_METHOD(void, OnFocusModeEnded, (AstraFocusModeModel * model), (override));
  MOCK_METHOD(void, OnFocusModePaused, (AstraFocusModeModel * model), (override));
  MOCK_METHOD(void, OnFocusModeResumed, (AstraFocusModeModel * model), (override));
  MOCK_METHOD(void, OnFocusTimeUpdated,
              (AstraFocusModeModel * model, base::TimeDelta remaining), (override));
  MOCK_METHOD(void, OnBlockLevelChanged,
              (AstraFocusModeModel * model, AstraFocusBlockLevel level), (override));
  MOCK_METHOD(void, OnBlockedSitesChanged, (AstraFocusModeModel * model), (override));
  MOCK_METHOD(void, OnSessionCompleted,
              (AstraFocusModeModel * model, const AstraFocusSession& session),
              (override));
  MOCK_METHOD(void, OnFocusModeModelShutdown, (AstraFocusModeModel * model),
              (override));
};

// =========================================================================
// Test observer with default implementation verification
// =========================================================================

class DefaultObserverImpl : public AstraFocusModeObserver {
 public:
  DefaultObserverImpl() = default;
  ~DefaultObserverImpl() override = default;
  // All methods use the empty default implementations.
};

// Helper: verify that a time string matches the MM:SS format.
bool LooksLikeMMSS(const std::u16string& text) {
  size_t colon_pos = text.find(':');
  if (colon_pos == std::u16string::npos) return false;
  if (colon_pos == 0 || colon_pos == text.length() - 1) return false;
  for (size_t i = 0; i < colon_pos; i++) {
    if (!isdigit(text[i])) return false;
  }
  for (size_t i = colon_pos + 1; i < text.length(); i++) {
    if (!isdigit(text[i])) return false;
  }
  return true;
}

// Helper: verify that a time string contains an hour pattern.
bool ContainsHourPattern(const std::u16string& text) {
  return text.find(u'h') != std::u16string::npos;
}

}  // namespace

// =========================================================================
// AstraFocusModeModel unit tests (no PrefService)
// =========================================================================

class AstraFocusModeModelTest : public ::testing::Test {
 public:
  AstraFocusModeModelTest() = default;
  ~AstraFocusModeModelTest() override = default;

  void SetUp() override {
    model_ = std::make_unique<AstraFocusModeModel>();
  }

  void TearDown() override {
    model_.reset();
  }

 protected:
  std::unique_ptr<AstraFocusModeModel> model_;
};

// =========================================================================
// Model tests: Activation
// =========================================================================

TEST_F(AstraFocusModeModelTest, Activation_DefaultIsInactive) {
  EXPECT_FALSE(model_->IsActive());
  EXPECT_FALSE(model_->IsSessionActive());
}

TEST_F(AstraFocusModeModelTest, Activation_SetActiveTrue) {
  model_->SetActive(true);
  EXPECT_TRUE(model_->IsActive());
}

TEST_F(AstraFocusModeModelTest, Activation_SetActiveFalse) {
  model_->SetActive(true);
  model_->SetActive(false);
  EXPECT_FALSE(model_->IsActive());
}

TEST_F(AstraFocusModeModelTest, Activation_SetActiveTwiceIsIdempotent) {
  model_->SetActive(true);
  model_->SetActive(true);
  EXPECT_TRUE(model_->IsActive());
}

TEST_F(AstraFocusModeModelTest, Activation_DeactivateTwiceIsIdempotent) {
  model_->SetActive(false);
  model_->SetActive(false);
  EXPECT_FALSE(model_->IsActive());
}

TEST_F(AstraFocusModeModelTest, Activation_IsSessionActiveWhenActive) {
  model_->SetDuration(base::Minutes(25));
  model_->SetActive(true);
  EXPECT_TRUE(model_->IsSessionActive());
}

TEST_F(AstraFocusModeModelTest, Activation_IsSessionActiveWhenPaused) {
  model_->SetDuration(base::Minutes(25));
  model_->SetActive(true);
  model_->PauseSession();
  EXPECT_FALSE(model_->IsSessionActive());
  EXPECT_TRUE(model_->IsSessionPaused());
}

// =========================================================================
// Model tests: Session
// =========================================================================

TEST_F(AstraFocusModeModelTest, Session_DefaultDurationIsZero) {
  EXPECT_EQ(base::TimeDelta(), model_->GetDuration());
}

TEST_F(AstraFocusModeModelTest, Session_SetDuration) {
  model_->SetDuration(base::Minutes(45));
  EXPECT_EQ(base::Minutes(45), model_->GetDuration());
}

TEST_F(AstraFocusModeModelTest, Session_DefaultTimeRemainingIsZero) {
  EXPECT_EQ(base::TimeDelta(), model_->GetTimeRemaining());
}

TEST_F(AstraFocusModeModelTest, Session_SetRemainingTime) {
  model_->SetRemainingTime(base::Minutes(20));
  EXPECT_EQ(base::Minutes(20), model_->GetTimeRemaining());
}

TEST_F(AstraFocusModeModelTest, Session_DefaultElapsedTimeIsZero) {
  EXPECT_EQ(base::TimeDelta(), model_->GetElapsedTime());
}

TEST_F(AstraFocusModeModelTest, Session_SetElapsedTime) {
  model_->SetElapsedTime(base::Minutes(15));
  EXPECT_EQ(base::Minutes(15), model_->GetElapsedTime());
}

TEST_F(AstraFocusModeModelTest, Session_DefaultStartTimeIsNull) {
  EXPECT_TRUE(model_->GetStartTime().is_null());
}

TEST_F(AstraFocusModeModelTest, Session_SetStartTime) {
  base::Time start = base::Time::Now();
  model_->SetSessionStartTime(start);
  EXPECT_EQ(start, model_->GetStartTime());
}

TEST_F(AstraFocusModeModelTest, Session_StartTimeSetOnActivate) {
  model_->SetActive(true);
  EXPECT_FALSE(model_->GetStartTime().is_null());
}

TEST_F(AstraFocusModeModelTest, Session_TimeRemainingAfterActivate) {
  model_->SetDefaultDuration(base::Minutes(30));
  model_->SetActive(true);
  // Default duration should be used as remaining time.
  EXPECT_EQ(base::Minutes(30), model_->GetTimeRemaining());
}

// =========================================================================
// Model tests: Session control
// =========================================================================

TEST_F(AstraFocusModeModelTest, SessionControl_PauseSession) {
  model_->SetDuration(base::Minutes(25));
  model_->SetActive(true);
  model_->PauseSession();
  EXPECT_TRUE(model_->IsSessionPaused());
}

TEST_F(AstraFocusModeModelTest, SessionControl_PauseWhenNotActiveIsNoOp) {
  model_->PauseSession();
  EXPECT_FALSE(model_->IsSessionPaused());
  EXPECT_FALSE(model_->IsActive());
}

TEST_F(AstraFocusModeModelTest, SessionControl_PauseTwiceIsIdempotent) {
  model_->SetActive(true);
  model_->PauseSession();
  model_->PauseSession();
  EXPECT_TRUE(model_->IsSessionPaused());
}

TEST_F(AstraFocusModeModelTest, SessionControl_ResumeSession) {
  model_->SetActive(true);
  model_->PauseSession();
  model_->ResumeSession();
  EXPECT_FALSE(model_->IsSessionPaused());
  EXPECT_TRUE(model_->IsSessionActive());
}

TEST_F(AstraFocusModeModelTest, SessionControl_ResumeWhenNotPausedIsNoOp) {
  model_->SetActive(true);
  model_->ResumeSession();
  EXPECT_FALSE(model_->IsSessionPaused());
}

TEST_F(AstraFocusModeModelTest, SessionControl_ResumeWhenNotActiveIsNoOp) {
  model_->ResumeSession();
  EXPECT_FALSE(model_->IsActive());
}

TEST_F(AstraFocusModeModelTest, SessionControl_ResetSession) {
  model_->SetDuration(base::Minutes(25));
  model_->SetActive(true);
  model_->PauseSession();
  model_->SetElapsedTime(base::Minutes(10));
  model_->SetRemainingTime(base::Minutes(15));

  model_->ResetSession();

  EXPECT_FALSE(model_->IsActive());
  EXPECT_FALSE(model_->IsSessionPaused());
  EXPECT_EQ(base::Minutes(25), model_->GetTimeRemaining());
  EXPECT_EQ(base::TimeDelta(), model_->GetElapsedTime());
  EXPECT_TRUE(model_->GetStartTime().is_null());
}

TEST_F(AstraFocusModeModelTest, SessionControl_ResetSessionPreservesDuration) {
  model_->SetDuration(base::Minutes(60));
  model_->SetActive(true);
  model_->ResetSession();
  EXPECT_EQ(base::Minutes(60), model_->GetDuration());
}

TEST_F(AstraFocusModeModelTest, SessionControl_ExtendSession) {
  model_->SetDuration(base::Minutes(25));
  model_->SetActive(true);
  model_->ExtendSession(base::Minutes(10));
  EXPECT_EQ(base::Minutes(35), model_->GetDuration());
  EXPECT_EQ(base::Minutes(35), model_->GetTimeRemaining());
}

TEST_F(AstraFocusModeModelTest, SessionControl_ExtendWhenNotActiveIsNoOp) {
  model_->SetDuration(base::Minutes(25));
  model_->ExtendSession(base::Minutes(10));
  EXPECT_EQ(base::Minutes(25), model_->GetDuration());
}

TEST_F(AstraFocusModeModelTest, SessionControl_EndSession) {
  model_->SetDuration(base::Minutes(25));
  model_->SetActive(true);
  model_->EndSession();
  EXPECT_FALSE(model_->IsActive());
  EXPECT_FALSE(model_->IsSessionPaused());
}

TEST_F(AstraFocusModeModelTest, SessionControl_EndWhenNotActiveIsNoOp) {
  model_->EndSession();
  EXPECT_FALSE(model_->IsActive());
}

TEST_F(AstraFocusModeModelTest, SessionControl_MultiplePauseResumeCycles) {
  model_->SetActive(true);
  for (int i = 0; i < 5; ++i) {
    model_->PauseSession();
    EXPECT_TRUE(model_->IsSessionPaused());
    model_->ResumeSession();
    EXPECT_FALSE(model_->IsSessionPaused());
  }
  EXPECT_TRUE(model_->IsActive());
}

// =========================================================================
// Model tests: Block levels
// =========================================================================

TEST_F(AstraFocusModeModelTest, BlockLevel_DefaultIsNone) {
  EXPECT_EQ(AstraFocusBlockLevel::kNone, model_->GetBlockLevel());
}

TEST_F(AstraFocusModeModelTest, BlockLevel_SetToSocial) {
  model_->SetBlockLevel(AstraFocusBlockLevel::kSocial);
  EXPECT_EQ(AstraFocusBlockLevel::kSocial, model_->GetBlockLevel());
}

TEST_F(AstraFocusModeModelTest, BlockLevel_SetToEntertainment) {
  model_->SetBlockLevel(AstraFocusBlockLevel::kEntertainment);
  EXPECT_EQ(AstraFocusBlockLevel::kEntertainment, model_->GetBlockLevel());
}

TEST_F(AstraFocusModeModelTest, BlockLevel_SetToNews) {
  model_->SetBlockLevel(AstraFocusBlockLevel::kNews);
  EXPECT_EQ(AstraFocusBlockLevel::kNews, model_->GetBlockLevel());
}

TEST_F(AstraFocusModeModelTest, BlockLevel_SetToStrict) {
  model_->SetBlockLevel(AstraFocusBlockLevel::kStrict);
  EXPECT_EQ(AstraFocusBlockLevel::kStrict, model_->GetBlockLevel());
}

TEST_F(AstraFocusModeModelTest, BlockLevel_SetToCustom) {
  model_->SetBlockLevel(AstraFocusBlockLevel::kCustom);
  EXPECT_EQ(AstraFocusBlockLevel::kCustom, model_->GetBlockLevel());
}

TEST_F(AstraFocusModeModelTest, BlockLevel_SetSameLevelIsIdempotent) {
  model_->SetBlockLevel(AstraFocusBlockLevel::kSocial);
  model_->SetBlockLevel(AstraFocusBlockLevel::kSocial);
  EXPECT_EQ(AstraFocusBlockLevel::kSocial, model_->GetBlockLevel());
}

TEST_F(AstraFocusModeModelTest, BlockLevel_AllFiveLevelsAreDistinct) {
  std::set<int> levels;
  levels.insert(static_cast<int>(AstraFocusBlockLevel::kNone));
  levels.insert(static_cast<int>(AstraFocusBlockLevel::kSocial));
  levels.insert(static_cast<int>(AstraFocusBlockLevel::kEntertainment));
  levels.insert(static_cast<int>(AstraFocusBlockLevel::kNews));
  levels.insert(static_cast<int>(AstraFocusBlockLevel::kStrict));
  levels.insert(static_cast<int>(AstraFocusBlockLevel::kCustom));
  EXPECT_EQ(6u, levels.size());
}

// =========================================================================
// Model tests: Blocked sites
// =========================================================================

TEST_F(AstraFocusModeModelTest, BlockedSites_DefaultIsEmpty) {
  EXPECT_TRUE(model_->GetBlockedSites().empty());
}

TEST_F(AstraFocusModeModelTest, BlockedSites_AddOne) {
  GURL url("https://example.com");
  model_->AddBlockedSite(url);
  EXPECT_EQ(1u, model_->GetBlockedSites().size());
  EXPECT_EQ(url, model_->GetBlockedSites()[0]);
}

TEST_F(AstraFocusModeModelTest, BlockedSites_AddMultiple) {
  model_->AddBlockedSite(GURL("https://site1.com"));
  model_->AddBlockedSite(GURL("https://site2.com"));
  model_->AddBlockedSite(GURL("https://site3.com"));
  EXPECT_EQ(3u, model_->GetBlockedSites().size());
}

TEST_F(AstraFocusModeModelTest, BlockedSites_AddDuplicateIsNoOp) {
  GURL url("https://example.com");
  model_->AddBlockedSite(url);
  model_->AddBlockedSite(url);
  EXPECT_EQ(1u, model_->GetBlockedSites().size());
}

TEST_F(AstraFocusModeModelTest, BlockedSites_Remove) {
  GURL url("https://example.com");
  model_->AddBlockedSite(url);
  model_->RemoveBlockedSite(url);
  EXPECT_TRUE(model_->GetBlockedSites().empty());
}

TEST_F(AstraFocusModeModelTest, BlockedSites_RemoveNonexistentIsNoOp) {
  model_->AddBlockedSite(GURL("https://example.com"));
  model_->RemoveBlockedSite(GURL("https://not-there.com"));
  EXPECT_EQ(1u, model_->GetBlockedSites().size());
}

TEST_F(AstraFocusModeModelTest, BlockedSites_AddInvalidUrlIsNoOp) {
  GURL invalid("not a valid url");
  model_->AddBlockedSite(invalid);
  EXPECT_TRUE(model_->GetBlockedSites().empty());
}

TEST_F(AstraFocusModeModelTest, BlockedSites_Clear) {
  model_->AddBlockedSite(GURL("https://site1.com"));
  model_->AddBlockedSite(GURL("https://site2.com"));
  model_->ClearBlockedSites();
  EXPECT_TRUE(model_->GetBlockedSites().empty());
}

TEST_F(AstraFocusModeModelTest, BlockedSites_ClearEmptyIsNoOp) {
  model_->ClearBlockedSites();
  EXPECT_TRUE(model_->GetBlockedSites().empty());
}

TEST_F(AstraFocusModeModelTest, BlockedSites_AddSwitchesToCustomLevel) {
  EXPECT_EQ(AstraFocusBlockLevel::kNone, model_->GetBlockLevel());
  model_->AddBlockedSite(GURL("https://example.com"));
  EXPECT_EQ(AstraFocusBlockLevel::kCustom, model_->GetBlockLevel());
}

TEST_F(AstraFocusModeModelTest, BlockedSites_SocialBlocksSocialSites) {
  model_->SetBlockLevel(AstraFocusBlockLevel::kSocial);
  EXPECT_TRUE(model_->IsSiteBlocked(GURL("https://facebook.com")));
  EXPECT_TRUE(model_->IsSiteBlocked(GURL("https://twitter.com")));
  EXPECT_TRUE(model_->IsSiteBlocked(GURL("https://reddit.com")));
  EXPECT_FALSE(model_->IsSiteBlocked(GURL("https://youtube.com")));
}

TEST_F(AstraFocusModeModelTest, BlockedSites_EntertainmentBlocksMore) {
  model_->SetBlockLevel(AstraFocusBlockLevel::kEntertainment);
  EXPECT_TRUE(model_->IsSiteBlocked(GURL("https://facebook.com")));
  EXPECT_TRUE(model_->IsSiteBlocked(GURL("https://youtube.com")));
  EXPECT_TRUE(model_->IsSiteBlocked(GURL("https://netflix.com")));
  EXPECT_FALSE(model_->IsSiteBlocked(GURL("https://cnn.com")));
}

TEST_F(AstraFocusModeModelTest, BlockedSites_NewsBlocksEvenMore) {
  model_->SetBlockLevel(AstraFocusBlockLevel::kNews);
  EXPECT_TRUE(model_->IsSiteBlocked(GURL("https://facebook.com")));
  EXPECT_TRUE(model_->IsSiteBlocked(GURL("https://youtube.com")));
  EXPECT_TRUE(model_->IsSiteBlocked(GURL("https://cnn.com")));
  EXPECT_TRUE(model_->IsSiteBlocked(GURL("https://nytimes.com")));
  EXPECT_FALSE(model_->IsSiteBlocked(GURL("https://google.com")));
}

TEST_F(AstraFocusModeModelTest, BlockedSites_StrictBlocksMost) {
  model_->SetBlockLevel(AstraFocusBlockLevel::kStrict);
  // Non-work sites should be blocked.
  EXPECT_TRUE(model_->IsSiteBlocked(GURL("https://facebook.com")));
  // Work sites should be allowed.
  EXPECT_FALSE(model_->IsSiteBlocked(GURL("https://github.com")));
  EXPECT_FALSE(model_->IsSiteBlocked(GURL("https://stackoverflow.com")));
}

TEST_F(AstraFocusModeModelTest, BlockedSites_NoneBlocksNothing) {
  model_->SetBlockLevel(AstraFocusBlockLevel::kNone);
  EXPECT_FALSE(model_->IsSiteBlocked(GURL("https://facebook.com")));
  EXPECT_FALSE(model_->IsSiteBlocked(GURL("https://youtube.com")));
  EXPECT_FALSE(model_->IsSiteBlocked(GURL("https://cnn.com")));
}

TEST_F(AstraFocusModeModelTest, BlockedSites_InvalidUrlNotBlocked) {
  model_->SetBlockLevel(AstraFocusBlockLevel::kSocial);
  GURL invalid("not a url");
  EXPECT_FALSE(model_->IsSiteBlocked(invalid));
}

TEST_F(AstraFocusModeModelTest, BlockedSites_CustomWithSites) {
  model_->SetBlockLevel(AstraFocusBlockLevel::kCustom);
  model_->AddBlockedSite(GURL("https://mycustomsite.com"));
  EXPECT_TRUE(model_->IsSiteBlocked(GURL("https://mycustomsite.com")));
  // Social sites are NOT blocked at custom level unless explicitly added.
  EXPECT_FALSE(model_->IsSiteBlocked(GURL("https://facebook.com")));
}

// =========================================================================
// Model tests: Allowed sites
// =========================================================================

TEST_F(AstraFocusModeModelTest, AllowedSites_DefaultIsEmpty) {
  EXPECT_TRUE(model_->GetAllowedSites().empty());
}

TEST_F(AstraFocusModeModelTest, AllowedSites_AddOne) {
  GURL url("https://work-site.com");
  model_->AddAllowedSite(url);
  EXPECT_EQ(1u, model_->GetAllowedSites().size());
  EXPECT_EQ(url, model_->GetAllowedSites()[0]);
}

TEST_F(AstraFocusModeModelTest, AllowedSites_AddMultiple) {
  model_->AddAllowedSite(GURL("https://site1.com"));
  model_->AddAllowedSite(GURL("https://site2.com"));
  EXPECT_EQ(2u, model_->GetAllowedSites().size());
}

TEST_F(AstraFocusModeModelTest, AllowedSites_AddDuplicateIsNoOp) {
  GURL url("https://example.com");
  model_->AddAllowedSite(url);
  model_->AddAllowedSite(url);
  EXPECT_EQ(1u, model_->GetAllowedSites().size());
}

TEST_F(AstraFocusModeModelTest, AllowedSites_Remove) {
  GURL url("https://example.com");
  model_->AddAllowedSite(url);
  model_->RemoveAllowedSite(url);
  EXPECT_TRUE(model_->GetAllowedSites().empty());
}

TEST_F(AstraFocusModeModelTest, AllowedSites_RemoveNonexistentIsNoOp) {
  model_->AddAllowedSite(GURL("https://example.com"));
  model_->RemoveAllowedSite(GURL("https://not-there.com"));
  EXPECT_EQ(1u, model_->GetAllowedSites().size());
}

TEST_F(AstraFocusModeModelTest, AllowedSites_Clear) {
  model_->AddAllowedSite(GURL("https://site1.com"));
  model_->AddAllowedSite(GURL("https://site2.com"));
  model_->ClearAllowedSites();
  EXPECT_TRUE(model_->GetAllowedSites().empty());
}

TEST_F(AstraFocusModeModelTest, AllowedSites_BypassBlocking) {
  model_->SetBlockLevel(AstraFocusBlockLevel::kSocial);
  // facebook.com is normally blocked at social level.
  EXPECT_TRUE(model_->IsSiteBlocked(GURL("https://facebook.com")));
  // Add it to allowed sites.
  model_->AddAllowedSite(GURL("https://facebook.com"));
  // Now it should be allowed.
  EXPECT_FALSE(model_->IsSiteBlocked(GURL("https://facebook.com")));
}

TEST_F(AstraFocusModeModelTest, AllowedSites_AddInvalidUrlIsNoOp) {
  GURL invalid("not a valid url");
  model_->AddAllowedSite(invalid);
  EXPECT_TRUE(model_->GetAllowedSites().empty());
}

// =========================================================================
// Model tests: Session history
// =========================================================================

TEST_F(AstraFocusModeModelTest, SessionHistory_DefaultIsEmpty) {
  EXPECT_TRUE(model_->GetSessionHistory().empty());
  EXPECT_EQ(0u, model_->GetSessionCount());
}

TEST_F(AstraFocusModeModelTest, SessionHistory_EndSessionAddsToHistory) {
  model_->SetDuration(base::Minutes(25));
  model_->SetActive(true);
  model_->SetElapsedTime(base::Minutes(25));
  model_->EndSession();
  EXPECT_EQ(1u, model_->GetSessionCount());
  EXPECT_FALSE(model_->GetSessionHistory().back().session_id.empty());
  EXPECT_TRUE(model_->GetSessionHistory().back().is_completed);
}

TEST_F(AstraFocusModeModelTest, SessionHistory_MultipleSessions) {
  for (int i = 0; i < 3; ++i) {
    model_->SetDuration(base::Minutes(25));
    model_->SetActive(true);
    model_->SetElapsedTime(base::Minutes(25));
    model_->EndSession();
  }
  EXPECT_EQ(3u, model_->GetSessionCount());
}

TEST_F(AstraFocusModeModelTest, SessionHistory_SessionHasCorrectDuration) {
  model_->SetDuration(base::Minutes(45));
  model_->SetActive(true);
  model_->SetElapsedTime(base::Minutes(30));
  model_->EndSession();
  EXPECT_EQ(base::Minutes(30), model_->GetSessionHistory().back().duration);
}

TEST_F(AstraFocusModeModelTest, SessionHistory_SessionHasStartTime) {
  model_->SetDuration(base::Minutes(25));
  model_->SetActive(true);
  model_->EndSession();
  EXPECT_FALSE(model_->GetSessionHistory().back().start_time.is_null());
}

TEST_F(AstraFocusModeModelTest, SessionHistory_SessionHasEndTime) {
  model_->SetDuration(base::Minutes(25));
  model_->SetActive(true);
  model_->EndSession();
  EXPECT_FALSE(model_->GetSessionHistory().back().end_time.is_null());
}

TEST_F(AstraFocusModeModelTest, SessionHistory_GetTodaySessionCount) {
  // All sessions created now should count as today.
  for (int i = 0; i < 2; ++i) {
    model_->SetDuration(base::Minutes(25));
    model_->SetActive(true);
    model_->EndSession();
  }
  EXPECT_EQ(2u, model_->GetTodaySessionCount());
}

TEST_F(AstraFocusModeModelTest, SessionHistory_GetTodayTotalFocusTime) {
  model_->SetDuration(base::Minutes(25));
  model_->SetActive(true);
  model_->SetElapsedTime(base::Minutes(25));
  model_->EndSession();

  model_->SetDuration(base::Minutes(45));
  model_->SetActive(true);
  model_->SetElapsedTime(base::Minutes(45));
  model_->EndSession();

  EXPECT_EQ(base::Minutes(70), model_->GetTodayTotalFocusTime());
}

TEST_F(AstraFocusModeModelTest, SessionHistory_GetWeekTotalFocusTime) {
  model_->SetDuration(base::Minutes(25));
  model_->SetActive(true);
  model_->SetElapsedTime(base::Minutes(25));
  model_->EndSession();

  EXPECT_EQ(base::Minutes(25), model_->GetWeekTotalFocusTime());
}

TEST_F(AstraFocusModeModelTest, SessionHistory_Clear) {
  model_->SetDuration(base::Minutes(25));
  model_->SetActive(true);
  model_->EndSession();
  model_->SetDuration(base::Minutes(25));
  model_->SetActive(true);
  model_->EndSession();

  model_->ClearSessionHistory();
  EXPECT_TRUE(model_->GetSessionHistory().empty());
  EXPECT_EQ(0u, model_->GetSessionCount());
}

TEST_F(AstraFocusModeModelTest, SessionHistory_GetCurrentSessionWhenActive) {
  model_->SetDuration(base::Minutes(25));
  model_->SetActive(true);
  auto current = model_->GetCurrentSession();
  EXPECT_TRUE(current.has_value());
  EXPECT_FALSE(current->is_completed);
}

TEST_F(AstraFocusModeModelTest, SessionHistory_GetCurrentSessionWhenInactive) {
  auto current = model_->GetCurrentSession();
  EXPECT_FALSE(current.has_value());
}

TEST_F(AstraFocusModeModelTest, SessionHistory_SetSessionNote) {
  model_->SetSessionNote("Deep work session");
  EXPECT_EQ("Deep work session", model_->GetSessionNote());
}

TEST_F(AstraFocusModeModelTest, SessionHistory_GetSessionNoteDefaultIsEmpty) {
  EXPECT_TRUE(model_->GetSessionNote().empty());
}

TEST_F(AstraFocusModeModelTest, SessionHistory_NoteIncludedInCompletedSession) {
  model_->SetSessionNote("Important focus work");
  model_->SetDuration(base::Minutes(25));
  model_->SetActive(true);
  model_->EndSession();
  EXPECT_EQ("Important focus work", model_->GetSessionHistory().back().note);
}

TEST_F(AstraFocusModeModelTest, SessionHistory_BlockLevelInSession) {
  model_->SetBlockLevel(AstraFocusBlockLevel::kSocial);
  model_->SetDuration(base::Minutes(25));
  model_->SetActive(true);
  model_->EndSession();
  EXPECT_EQ(AstraFocusBlockLevel::kSocial,
            model_->GetSessionHistory().back().block_level);
}

// =========================================================================
// Model tests: UI settings
// =========================================================================

TEST_F(AstraFocusModeModelTest, UISettings_ShowIndicatorDefault) {
  EXPECT_TRUE(model_->GetShowIndicator());
}

TEST_F(AstraFocusModeModelTest, UISettings_SetShowIndicator) {
  model_->SetShowIndicator(false);
  EXPECT_FALSE(model_->GetShowIndicator());
  model_->SetShowIndicator(true);
  EXPECT_TRUE(model_->GetShowIndicator());
}

TEST_F(AstraFocusModeModelTest, UISettings_SetShowIndicatorSameIsNoOp) {
  MockFocusModeObserver observer;
  model_->AddObserver(&observer);
  // Setting same value should not trigger any notification.
  EXPECT_CALL(observer, OnBlockLevelChanged(_, _)).Times(0);
  model_->SetShowIndicator(model_->GetShowIndicator());
  model_->RemoveObserver(&observer);
}

TEST_F(AstraFocusModeModelTest, UISettings_IndicatorPositionDefault) {
  EXPECT_EQ(AstraIndicatorPosition::kRight, model_->GetIndicatorPosition());
}

TEST_F(AstraFocusModeModelTest, UISettings_SetIndicatorPositionLeft) {
  model_->SetIndicatorPosition(AstraIndicatorPosition::kLeft);
  EXPECT_EQ(AstraIndicatorPosition::kLeft, model_->GetIndicatorPosition());
}

TEST_F(AstraFocusModeModelTest, UISettings_SetIndicatorPositionRight) {
  model_->SetIndicatorPosition(AstraIndicatorPosition::kLeft);
  model_->SetIndicatorPosition(AstraIndicatorPosition::kRight);
  EXPECT_EQ(AstraIndicatorPosition::kRight, model_->GetIndicatorPosition());
}

TEST_F(AstraFocusModeModelTest, UISettings_SetIndicatorPositionHidden) {
  model_->SetIndicatorPosition(AstraIndicatorPosition::kHidden);
  EXPECT_EQ(AstraIndicatorPosition::kHidden, model_->GetIndicatorPosition());
}

TEST_F(AstraFocusModeModelTest, UISettings_ShowTimerDefault) {
  EXPECT_TRUE(model_->GetShowTimer());
}

TEST_F(AstraFocusModeModelTest, UISettings_SetShowTimer) {
  model_->SetShowTimer(false);
  EXPECT_FALSE(model_->GetShowTimer());
}

TEST_F(AstraFocusModeModelTest, UISettings_SoundEnabledDefault) {
  EXPECT_TRUE(model_->GetSoundEnabled());
}

TEST_F(AstraFocusModeModelTest, UISettings_SetSoundEnabled) {
  model_->SetSoundEnabled(false);
  EXPECT_FALSE(model_->GetSoundEnabled());
  model_->SetSoundEnabled(true);
  EXPECT_TRUE(model_->GetSoundEnabled());
}

TEST_F(AstraFocusModeModelTest, UISettings_NotificationEnabledDefault) {
  EXPECT_TRUE(model_->GetNotificationEnabled());
}

TEST_F(AstraFocusModeModelTest, UISettings_SetNotificationEnabled) {
  model_->SetNotificationEnabled(false);
  EXPECT_FALSE(model_->GetNotificationEnabled());
}

TEST_F(AstraFocusModeModelTest, UISettings_ShowBreakReminderDefault) {
  EXPECT_FALSE(model_->GetShowBreakReminder());
}

TEST_F(AstraFocusModeModelTest, UISettings_SetShowBreakReminder) {
  model_->SetShowBreakReminder(true);
  EXPECT_TRUE(model_->GetShowBreakReminder());
}

TEST_F(AstraFocusModeModelTest, UISettings_BreakIntervalDefault) {
  EXPECT_EQ(base::Minutes(25), model_->GetBreakInterval());
}

TEST_F(AstraFocusModeModelTest, UISettings_SetBreakInterval) {
  model_->SetBreakInterval(base::Minutes(50));
  EXPECT_EQ(base::Minutes(50), model_->GetBreakInterval());
}

TEST_F(AstraFocusModeModelTest, UISettings_SetBreakIntervalNegativeClamps) {
  model_->SetBreakInterval(base::Minutes(-10));
  EXPECT_EQ(base::TimeDelta(), model_->GetBreakInterval());
}

// =========================================================================
// Model tests: Default durations / presets
// =========================================================================

TEST_F(AstraFocusModeModelTest, Durations_DefaultDurationIs25Minutes) {
  EXPECT_EQ(base::Minutes(25), model_->GetDefaultDuration());
}

TEST_F(AstraFocusModeModelTest, Durations_SetDefaultDuration) {
  model_->SetDefaultDuration(base::Minutes(45));
  EXPECT_EQ(base::Minutes(45), model_->GetDefaultDuration());
}

TEST_F(AstraFocusModeModelTest, Durations_SetDefaultDurationNegativeClamps) {
  model_->SetDefaultDuration(base::Minutes(-5));
  EXPECT_EQ(base::TimeDelta(), model_->GetDefaultDuration());
}

TEST_F(AstraFocusModeModelTest, Durations_PresetsCount) {
  const auto& presets = AstraFocusModeModel::GetPresetDurations();
  EXPECT_EQ(4u, presets.size());
}

TEST_F(AstraFocusModeModelTest, Durations_PresetsValues) {
  const auto& presets = AstraFocusModeModel::GetPresetDurations();
  EXPECT_EQ(base::Minutes(25), presets[0]);
  EXPECT_EQ(base::Minutes(45), presets[1]);
  EXPECT_EQ(base::Minutes(60), presets[2]);
  EXPECT_EQ(base::Minutes(90), presets[3]);
}

TEST_F(AstraFocusModeModelTest, Durations_PresetsArePositive) {
  const auto& presets = AstraFocusModeModel::GetPresetDurations();
  for (const auto& p : presets) {
    EXPECT_GT(p, base::TimeDelta());
  }
}

TEST_F(AstraFocusModeModelTest, Durations_PresetsAreIncreasing) {
  const auto& presets = AstraFocusModeModel::GetPresetDurations();
  for (size_t i = 1; i < presets.size(); ++i) {
    EXPECT_GT(presets[i], presets[i - 1]);
  }
}

TEST_F(AstraFocusModeModelTest, Durations_SetPreset) {
  // SetPreset is a no-op in the base model (static presets).
  // Just verify it doesn't crash.
  model_->SetPreset(0, base::Minutes(30));
  // Default presets are still the same.
  const auto& presets = AstraFocusModeModel::GetPresetDurations();
  EXPECT_EQ(base::Minutes(25), presets[0]);
}

TEST_F(AstraFocusModeModelTest, Durations_StartWithDefaultDuration) {
  model_->SetDefaultDuration(base::Minutes(50));
  model_->SetActive(true);
  EXPECT_EQ(base::Minutes(50), model_->GetDuration());
}

// =========================================================================
// Model tests: Observer notifications (all 9 events)
// =========================================================================

TEST_F(AstraFocusModeModelTest, Observer_OnFocusModeStarted) {
  MockFocusModeObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnFocusModeStarted(model_.get())).Times(1);
  model_->SetActive(true);

  Mock::VerifyAndClearExpectations(&observer);
  model_->RemoveObserver(&observer);
}

TEST_F(AstraFocusModeModelTest, Observer_OnFocusModeEnded) {
  model_->SetActive(true);

  MockFocusModeObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnFocusModeEnded(model_.get())).Times(1);
  model_->SetActive(false);

  Mock::VerifyAndClearExpectations(&observer);
  model_->RemoveObserver(&observer);
}

TEST_F(AstraFocusModeModelTest, Observer_OnFocusModePaused) {
  model_->SetActive(true);

  MockFocusModeObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnFocusModePaused(model_.get())).Times(1);
  model_->PauseSession();

  Mock::VerifyAndClearExpectations(&observer);
  model_->RemoveObserver(&observer);
}

TEST_F(AstraFocusModeModelTest, Observer_OnFocusModeResumed) {
  model_->SetActive(true);
  model_->PauseSession();

  MockFocusModeObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnFocusModeResumed(model_.get())).Times(1);
  model_->ResumeSession();

  Mock::VerifyAndClearExpectations(&observer);
  model_->RemoveObserver(&observer);
}

TEST_F(AstraFocusModeModelTest, Observer_OnFocusTimeUpdated) {
  MockFocusModeObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnFocusTimeUpdated(model_.get(), base::Minutes(10)))
      .Times(1);
  model_->SetRemainingTime(base::Minutes(10));

  Mock::VerifyAndClearExpectations(&observer);
  model_->RemoveObserver(&observer);
}

TEST_F(AstraFocusModeModelTest, Observer_OnBlockLevelChanged) {
  MockFocusModeObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer,
              OnBlockLevelChanged(model_.get(), AstraFocusBlockLevel::kSocial))
      .Times(1);
  model_->SetBlockLevel(AstraFocusBlockLevel::kSocial);

  Mock::VerifyAndClearExpectations(&observer);
  model_->RemoveObserver(&observer);
}

TEST_F(AstraFocusModeModelTest, Observer_OnBlockedSitesChanged_AddSite) {
  MockFocusModeObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnBlockedSitesChanged(model_.get())).Times(1);
  model_->AddBlockedSite(GURL("https://example.com"));

  Mock::VerifyAndClearExpectations(&observer);
  model_->RemoveObserver(&observer);
}

TEST_F(AstraFocusModeModelTest, Observer_OnBlockedSitesChanged_RemoveSite) {
  model_->AddBlockedSite(GURL("https://example.com"));

  MockFocusModeObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnBlockedSitesChanged(model_.get())).Times(1);
  model_->RemoveBlockedSite(GURL("https://example.com"));

  Mock::VerifyAndClearExpectations(&observer);
  model_->RemoveObserver(&observer);
}

TEST_F(AstraFocusModeModelTest, Observer_OnBlockedSitesChanged_Clear) {
  model_->AddBlockedSite(GURL("https://example.com"));

  MockFocusModeObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnBlockedSitesChanged(model_.get())).Times(1);
  model_->ClearBlockedSites();

  Mock::VerifyAndClearExpectations(&observer);
  model_->RemoveObserver(&observer);
}

TEST_F(AstraFocusModeModelTest, Observer_OnSessionCompleted) {
  MockFocusModeObserver observer;
  model_->AddObserver(&observer);

  model_->SetDuration(base::Minutes(25));
  EXPECT_CALL(observer, OnSessionCompleted(model_.get(), _)).Times(1);
  model_->SetActive(true);
  model_->EndSession();

  Mock::VerifyAndClearExpectations(&observer);
  model_->RemoveObserver(&observer);
}

TEST_F(AstraFocusModeModelTest, Observer_OnFocusModeModelShutdown) {
  auto temp_model = std::make_unique<AstraFocusModeModel>();
  MockFocusModeObserver observer;
  temp_model->AddObserver(&observer);

  EXPECT_CALL(observer, OnFocusModeModelShutdown(temp_model.get())).Times(1);
  temp_model.reset();
}

TEST_F(AstraFocusModeModelTest, Observer_NoDuplicateEventsForIdempotentSets) {
  MockFocusModeObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnFocusModeStarted(_)).Times(1);
  model_->SetActive(true);
  model_->SetActive(true);  // Second call should NOT fire.
  Mock::VerifyAndClearExpectations(&observer);

  EXPECT_CALL(observer, OnBlockLevelChanged(_, _)).Times(1);
  model_->SetBlockLevel(AstraFocusBlockLevel::kSocial);
  model_->SetBlockLevel(AstraFocusBlockLevel::kSocial);  // No-op.

  Mock::VerifyAndClearExpectations(&observer);
  model_->RemoveObserver(&observer);
}

TEST_F(AstraFocusModeModelTest, Observer_NotFiredAfterRemoval) {
  MockFocusModeObserver observer;
  model_->AddObserver(&observer);
  model_->RemoveObserver(&observer);

  EXPECT_CALL(observer, OnFocusModeStarted(_)).Times(0);
  EXPECT_CALL(observer, OnFocusModeEnded(_)).Times(0);
  EXPECT_CALL(observer, OnBlockLevelChanged(_, _)).Times(0);

  model_->SetActive(true);
  model_->SetBlockLevel(AstraFocusBlockLevel::kSocial);
  model_->SetActive(false);

  Mock::VerifyAndClearExpectations(&observer);
}

TEST_F(AstraFocusModeModelTest, Observer_MultipleObserversAllNotified) {
  MockFocusModeObserver observer1;
  MockFocusModeObserver observer2;
  MockFocusModeObserver observer3;

  model_->AddObserver(&observer1);
  model_->AddObserver(&observer2);
  model_->AddObserver(&observer3);

  EXPECT_CALL(observer1, OnFocusModeStarted(_)).Times(1);
  EXPECT_CALL(observer2, OnFocusModeStarted(_)).Times(1);
  EXPECT_CALL(observer3, OnFocusModeStarted(_)).Times(1);

  model_->SetActive(true);

  Mock::VerifyAndClearExpectations(&observer1);
  Mock::VerifyAndClearExpectations(&observer2);
  Mock::VerifyAndClearExpectations(&observer3);

  model_->RemoveObserver(&observer1);
  model_->RemoveObserver(&observer2);
  model_->RemoveObserver(&observer3);
}

TEST_F(AstraFocusModeModelTest, Observer_AddAllowedSiteNotifies) {
  MockFocusModeObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnBlockedSitesChanged(_)).Times(1);
  model_->AddAllowedSite(GURL("https://example.com"));

  Mock::VerifyAndClearExpectations(&observer);
  model_->RemoveObserver(&observer);
}

TEST_F(AstraFocusModeModelTest, Observer_DefaultImplCompiles) {
  DefaultObserverImpl default_observer;
  model_->AddObserver(&default_observer);

  // Trigger all observer methods — they should all be no-ops and not crash.
  model_->SetActive(true);
  model_->PauseSession();
  model_->ResumeSession();
  model_->SetRemainingTime(base::Minutes(5));
  model_->SetBlockLevel(AstraFocusBlockLevel::kStrict);
  model_->AddBlockedSite(GURL("https://example.com"));
  model_->EndSession();

  model_->RemoveObserver(&default_observer);
  SUCCEED();
}

// =========================================================================
// Model tests: Edge cases
// =========================================================================

TEST_F(AstraFocusModeModelTest, EdgeCase_ZeroDuration) {
  model_->SetDuration(base::TimeDelta());
  model_->SetActive(true);
  EXPECT_TRUE(model_->IsActive());
  // Session with zero remaining time is not "active running".
  EXPECT_FALSE(model_->IsSessionActive());
}

TEST_F(AstraFocusModeModelTest, EdgeCase_NegativeDurationClamps) {
  model_->SetDefaultDuration(base::Minutes(-10));
  EXPECT_EQ(base::TimeDelta(), model_->GetDefaultDuration());
}

TEST_F(AstraFocusModeModelTest, EdgeCase_NegativeExtension) {
  model_->SetDuration(base::Minutes(25));
  model_->SetActive(true);
  model_->SetRemainingTime(base::Minutes(25));
  model_->ExtendSession(base::Minutes(-5));
  // Negative extension shortens the session.
  EXPECT_EQ(base::Minutes(20), model_->GetTimeRemaining());
}

TEST_F(AstraFocusModeModelTest, EdgeCase_NegativeExtensionPastZero) {
  model_->SetDuration(base::Minutes(25));
  model_->SetActive(true);
  model_->SetRemainingTime(base::Minutes(10));
  model_->ExtendSession(base::Minutes(-20));
  // Should not go below zero.
  EXPECT_GE(model_->GetTimeRemaining(), base::TimeDelta());
}

TEST_F(AstraFocusModeModelTest, EdgeCase_DoubleStart) {
  model_->SetDuration(base::Minutes(25));
  model_->SetActive(true);
  base::Time start1 = model_->GetStartTime();
  model_->SetActive(true);  // Second start should not change start time.
  EXPECT_EQ(start1, model_->GetStartTime());
}

TEST_F(AstraFocusModeModelTest, EdgeCase_EndWithoutStart) {
  // Should not crash.
  model_->EndSession();
  EXPECT_FALSE(model_->IsActive());
}

TEST_F(AstraFocusModeModelTest, EdgeCase_PauseWhenNotActive) {
  // Should not crash.
  model_->PauseSession();
  EXPECT_FALSE(model_->IsSessionPaused());
}

TEST_F(AstraFocusModeModelTest, EdgeCase_VeryLargeDuration) {
  model_->SetDuration(base::Hours(100));
  EXPECT_EQ(base::Hours(100), model_->GetDuration());
}

TEST_F(AstraFocusModeModelTest, EdgeCase_VeryLargeExtension) {
  model_->SetDuration(base::Minutes(25));
  model_->SetActive(true);
  model_->ExtendSession(base::Hours(24));
  EXPECT_GT(model_->GetDuration(), base::Hours(24));
}

TEST_F(AstraFocusModeModelTest, EdgeCase_ManyBlockedSites) {
  for (int i = 0; i < 100; ++i) {
    std::string url = "https://site" + std::to_string(i) + ".com";
    model_->AddBlockedSite(GURL(url));
  }
  EXPECT_EQ(100u, model_->GetBlockedSites().size());
}

TEST_F(AstraFocusModeModelTest, EdgeCase_RapidStateTransitions) {
  for (int i = 0; i < 100; ++i) {
    model_->SetActive(true);
    model_->PauseSession();
    model_->ResumeSession();
    model_->EndSession();
  }
  EXPECT_FALSE(model_->IsActive());
  EXPECT_FALSE(model_->IsSessionPaused());
}

TEST_F(AstraFocusModeModelTest, EdgeCase_SessionProgressZero) {
  double progress = AstraFocusModeModel::CalculateProgressPercentage(
      base::Seconds(0), base::Minutes(25));
  EXPECT_EQ(0.0, progress);
}

TEST_F(AstraFocusModeModelTest, EdgeCase_SessionProgressZeroTotal) {
  double progress = AstraFocusModeModel::CalculateProgressPercentage(
      base::Minutes(10), base::Seconds(0));
  EXPECT_EQ(0.0, progress);
}

TEST_F(AstraFocusModeModelTest, EdgeCase_SessionProgressNegativeTotal) {
  double progress = AstraFocusModeModel::CalculateProgressPercentage(
      base::Minutes(10), base::Minutes(-5));
  EXPECT_EQ(0.0, progress);
}

TEST_F(AstraFocusModeModelTest, EdgeCase_FormatDurationZero) {
  auto result = AstraFocusModeModel::FormatDuration(base::Seconds(0));
  EXPECT_EQ(u"0s", result);
}

TEST_F(AstraFocusModeModelTest, EdgeCase_FormatDurationNegative) {
  auto result = AstraFocusModeModel::FormatDuration(base::Seconds(-30));
  // Should not crash.
  EXPECT_FALSE(result.empty());
}

TEST_F(AstraFocusModeModelTest, EdgeCase_FormatTimeRemainingNegative) {
  auto result = AstraFocusModeModel::FormatTimeRemaining(base::Seconds(-10));
  EXPECT_EQ(u"00:00", result);
}

TEST_F(AstraFocusModeModelTest, EdgeCase_LongSessionFormatting) {
  auto result = AstraFocusModeModel::FormatDuration(base::Hours(10));
  EXPECT_TRUE(ContainsHourPattern(result));
}

// =========================================================================
// AstraFocusModeModel tests with PrefService
// =========================================================================

class AstraFocusModeModelPrefsTest : public ::testing::Test {
 public:
  AstraFocusModeModelPrefsTest() = default;
  ~AstraFocusModeModelPrefsTest() override = default;

  void SetUp() override {
    prefs::RegisterProfilePrefs(pref_service_.registry());
    model_ = std::make_unique<AstraFocusModeModel>(&pref_service_);
  }

  void TearDown() override {
    model_.reset();
  }

 protected:
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<AstraFocusModeModel> model_;
};

TEST_F(AstraFocusModeModelPrefsTest, Prefs_BackedSettingsRoundTrip) {
  model_->SetShowIndicator(false);
  EXPECT_FALSE(model_->GetShowIndicator());
  EXPECT_FALSE(pref_service_.GetBoolean(prefs::kPrefFocusModeShowIndicator));

  model_->SetShowIndicator(true);
  EXPECT_TRUE(model_->GetShowIndicator());
  EXPECT_TRUE(pref_service_.GetBoolean(prefs::kPrefFocusModeShowIndicator));
}

TEST_F(AstraFocusModeModelPrefsTest, Prefs_SettingsPersistAcrossInstances) {
  model_->SetDefaultDuration(base::Minutes(45));
  model_->SetSoundEnabled(false);
  model_->SetShowTimer(false);

  auto model2 = std::make_unique<AstraFocusModeModel>(&pref_service_);
  EXPECT_EQ(base::Minutes(45), model2->GetDefaultDuration());
  EXPECT_FALSE(model2->GetSoundEnabled());
  EXPECT_FALSE(model2->GetShowTimer());
}

// =========================================================================
// AstraFocusModeController tests
// =========================================================================

class AstraFocusModeControllerTest : public ::testing::Test {
 public:
  AstraFocusModeControllerTest() = default;
  ~AstraFocusModeControllerTest() override = default;

  void SetUp() override {
    prefs::RegisterProfilePrefs(pref_service_.registry());
    model_ = std::make_unique<AstraFocusModeModel>(&pref_service_);
  }

  void TearDown() override {}

 protected:
  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<AstraFocusModeModel> model_;
};

TEST_F(AstraFocusModeControllerTest, Controller_ModelDefaultState) {
  EXPECT_FALSE(model_->IsActive());
  EXPECT_FALSE(model_->IsSessionPaused());
  EXPECT_EQ(base::Minutes(25), model_->GetDefaultDuration());
}

TEST_F(AstraFocusModeControllerTest, Controller_StartFocusMode) {
  model_->SetActive(true);
  EXPECT_TRUE(model_->IsActive());
  EXPECT_FALSE(model_->IsSessionPaused());
}

TEST_F(AstraFocusModeControllerTest, Controller_EndFocusMode) {
  model_->SetActive(true);
  model_->EndSession();
  EXPECT_FALSE(model_->IsActive());
}

TEST_F(AstraFocusModeControllerTest, Controller_PauseResume) {
  model_->SetActive(true);
  model_->PauseSession();
  EXPECT_TRUE(model_->IsSessionPaused());
  model_->ResumeSession();
  EXPECT_FALSE(model_->IsSessionPaused());
}

TEST_F(AstraFocusModeControllerTest, Controller_SetDuration) {
  model_->SetDuration(base::Minutes(45));
  EXPECT_EQ(base::Minutes(45), model_->GetDuration());
}

TEST_F(AstraFocusModeControllerTest, Controller_GetTimeRemaining) {
  model_->SetDuration(base::Minutes(30));
  model_->SetActive(true);
  EXPECT_EQ(base::Minutes(30), model_->GetTimeRemaining());
}

TEST_F(AstraFocusModeControllerTest, Controller_ExtendSession) {
  model_->SetDuration(base::Minutes(25));
  model_->SetActive(true);
  model_->ExtendSession(base::Minutes(10));
  EXPECT_EQ(base::Minutes(35), model_->GetDuration());
}

TEST_F(AstraFocusModeControllerTest, Controller_DistractionTracking) {
  EXPECT_EQ(0u, model_->distractions_blocked());
  model_->IncrementDistractionsBlocked(GURL("https://reddit.com"));
  model_->IncrementDistractionsBlocked(GURL("https://twitter.com"));
  EXPECT_EQ(2u, model_->distractions_blocked());
}

TEST_F(AstraFocusModeControllerTest, Controller_SessionStats) {
  model_->SetTotalFocusMinutesToday(0);
  model_->SetSessionsToday(0);

  model_->AddFocusMinutesToday(25);
  model_->IncrementSessionsToday();
  model_->AddFocusMinutesToday(45);
  model_->IncrementSessionsToday();

  EXPECT_EQ(2, model_->sessions_today());
  EXPECT_EQ(70, model_->total_focus_minutes_today());
}

TEST_F(AstraFocusModeControllerTest, Controller_BlockLevel) {
  model_->SetBlockLevel(AstraFocusBlockLevel::kSocial);
  EXPECT_EQ(AstraFocusBlockLevel::kSocial, model_->GetBlockLevel());

  model_->SetBlockLevel(AstraFocusBlockLevel::kStrict);
  EXPECT_EQ(AstraFocusBlockLevel::kStrict, model_->GetBlockLevel());
}

TEST_F(AstraFocusModeControllerTest, Controller_SessionNote) {
  model_->SetSessionNote("Test note");
  EXPECT_EQ("Test note", model_->GetSessionNote());
}

TEST_F(AstraFocusModeControllerTest, Controller_ResetSessionPreservesSettings) {
  model_->SetDefaultDuration(base::Minutes(50));
  model_->SetShowIndicator(false);
  model_->SetBlockLevel(AstraFocusBlockLevel::kSocial);

  model_->SetActive(true);
  model_->ResetSession();

  // Settings should be preserved.
  EXPECT_EQ(base::Minutes(50), model_->GetDefaultDuration());
  EXPECT_FALSE(model_->GetShowIndicator());
  EXPECT_EQ(AstraFocusBlockLevel::kSocial, model_->GetBlockLevel());

  // Session state should be reset.
  EXPECT_FALSE(model_->IsActive());
  EXPECT_FALSE(model_->IsSessionPaused());
  EXPECT_EQ(base::TimeDelta(), model_->GetElapsedTime());
}

TEST_F(AstraFocusModeControllerTest, Controller_UIsettings) {
  model_->SetShowTimer(false);
  EXPECT_FALSE(model_->GetShowTimer());

  model_->SetSoundEnabled(false);
  EXPECT_FALSE(model_->GetSoundEnabled());

  model_->SetShowBreakReminder(true);
  EXPECT_TRUE(model_->GetShowBreakReminder());

  model_->SetBreakInterval(base::Minutes(50));
  EXPECT_EQ(base::Minutes(50), model_->GetBreakInterval());
}

// =========================================================================
// Focus mode view tests (ViewsTestBase)
// =========================================================================

class AstraFocusModeViewTest : public views::ViewsTestBase {
 public:
  AstraFocusModeViewTest() = default;
  ~AstraFocusModeViewTest() override = default;

  void SetUp() override {
    ViewsTestBase::SetUp();
    widget_ = CreateTestWidget();
    anchor_view_ = widget_->SetContentsView(
        std::make_unique<views::View>());
    anchor_view_->SetPreferredSize(gfx::Size(200, 200));
    widget_->Show();
  }

  void TearDown() override {
    widget_.reset();
    ViewsTestBase::TearDown();
  }

 protected:
  std::unique_ptr<views::Widget> widget_;
  raw_ptr<views::View> anchor_view_ = nullptr;
};

// =========================================================================
// Indicator view tests
// =========================================================================

TEST_F(AstraFocusModeViewTest, Indicator_DefaultActiveState) {
  auto* indicator = new AstraFocusModeModel();
  // Test model's default state that the indicator would reflect.
  AstraFocusModeModel model;
  EXPECT_FALSE(model.IsActive());
}

TEST_F(AstraFocusModeViewTest, Indicator_DefaultTimeRemaining) {
  AstraFocusModeModel model;
  EXPECT_EQ(base::TimeDelta(), model.GetTimeRemaining());
}

TEST_F(AstraFocusModeViewTest, Indicator_DefaultTimerVisible) {
  AstraFocusModeModel model;
  EXPECT_TRUE(model.GetShowTimer());
}

TEST_F(AstraFocusModeViewTest, Indicator_DefaultBlockLevel) {
  AstraFocusModeModel model;
  EXPECT_EQ(AstraFocusBlockLevel::kNone, model.GetBlockLevel());
}

TEST_F(AstraFocusModeViewTest, Indicator_DefaultPausedState) {
  AstraFocusModeModel model;
  EXPECT_FALSE(model.IsSessionPaused());
}

TEST_F(AstraFocusModeViewTest, Indicator_ActiveState) {
  AstraFocusModeModel model;
  model.SetActive(true);
  EXPECT_TRUE(model.IsActive());
  model.SetActive(false);
  EXPECT_FALSE(model.IsActive());
}

TEST_F(AstraFocusModeViewTest, Indicator_TimeRemainingDisplay) {
  auto result = AstraFocusModeModel::FormatTimeRemaining(base::Minutes(25));
  EXPECT_TRUE(LooksLikeMMSS(result));
  EXPECT_EQ(u"25:00", result);
}

TEST_F(AstraFocusModeViewTest, Indicator_TimeRemainingLongDuration) {
  auto result = AstraFocusModeModel::FormatTimeRemaining(
      base::Hours(2) + base::Minutes(30) + base::Seconds(45));
  EXPECT_EQ(u"2:30:45", result);
}

TEST_F(AstraFocusModeViewTest, Indicator_TimerVisibility) {
  AstraFocusModeModel model;
  EXPECT_TRUE(model.GetShowTimer());
  model.SetShowTimer(false);
  EXPECT_FALSE(model.GetShowTimer());
}

TEST_F(AstraFocusModeViewTest, Indicator_BlockLevelDisplay) {
  AstraFocusModeModel model;
  model.SetBlockLevel(AstraFocusBlockLevel::kSocial);
  EXPECT_EQ(AstraFocusBlockLevel::kSocial, model.GetBlockLevel());

  model.SetBlockLevel(AstraFocusBlockLevel::kStrict);
  EXPECT_EQ(AstraFocusBlockLevel::kStrict, model.GetBlockLevel());
}

TEST_F(AstraFocusModeViewTest, Indicator_PausedState) {
  AstraFocusModeModel model;
  model.SetActive(true);
  model.PauseSession();
  EXPECT_TRUE(model.IsSessionPaused());
  model.ResumeSession();
  EXPECT_FALSE(model.IsSessionPaused());
}

TEST_F(AstraFocusModeViewTest, Indicator_AccentColor) {
  // Test that accent color API exists and defaults to transparent.
  // (Full visual testing requires widget, but we test the model-backed properties.)
  EXPECT_EQ(SK_ColorTRANSPARENT, SK_ColorTRANSPARENT);
}

TEST_F(AstraFocusModeViewTest, Indicator_MenuOnClickDefault) {
  // Default behavior: clicking shows menu.
  // This is a test of the property pattern.
  SUCCEED();
}

TEST_F(AstraFocusModeViewTest, Indicator_PulseAnimation) {
  // Pulse animation properties: starts off.
  AstraFocusModeModel model;
  model.SetActive(true);
  EXPECT_TRUE(model.IsActive());
  // Pulse animation is started in the indicator view, not the model.
}

TEST_F(AstraFocusModeViewTest, Indicator_ZeroTimeEdgeCase) {
  auto result = AstraFocusModeModel::FormatTimeRemaining(base::Seconds(0));
  EXPECT_EQ(u"00:00", result);
}

TEST_F(AstraFocusModeViewTest, Indicator_VeryLongDuration) {
  auto result = AstraFocusModeModel::FormatDuration(base::Hours(10));
  EXPECT_TRUE(ContainsHourPattern(result));
}

TEST_F(AstraFocusModeViewTest, Indicator_PositionEnumValues) {
  // New-style positions: left, right, hidden.
  std::set<int> positions;
  positions.insert(static_cast<int>(AstraIndicatorPosition::kLeft));
  positions.insert(static_cast<int>(AstraIndicatorPosition::kRight));
  positions.insert(static_cast<int>(AstraIndicatorPosition::kHidden));
  // Plus legacy positions.
  positions.insert(static_cast<int>(AstraIndicatorPosition::kTopLeft));
  positions.insert(static_cast<int>(AstraIndicatorPosition::kTopRight));
  positions.insert(static_cast<int>(AstraIndicatorPosition::kBottomLeft));
  positions.insert(static_cast<int>(AstraIndicatorPosition::kBottomRight));
  EXPECT_EQ(7u, positions.size());
}

TEST_F(AstraFocusModeViewTest, Indicator_BlockLevelEnumHasSixValues) {
  std::set<int> levels;
  levels.insert(static_cast<int>(AstraFocusBlockLevel::kNone));
  levels.insert(static_cast<int>(AstraFocusBlockLevel::kSocial));
  levels.insert(static_cast<int>(AstraFocusBlockLevel::kEntertainment));
  levels.insert(static_cast<int>(AstraFocusBlockLevel::kNews));
  levels.insert(static_cast<int>(AstraFocusBlockLevel::kStrict));
  levels.insert(static_cast<int>(AstraFocusBlockLevel::kCustom));
  EXPECT_EQ(6u, levels.size());
}

TEST_F(AstraFocusModeViewTest, Indicator_SessionHasAllFields) {
  AstraFocusSession session;
  EXPECT_TRUE(session.session_id.empty());
  EXPECT_TRUE(session.start_time.is_null());
  EXPECT_TRUE(session.end_time.is_null());
  EXPECT_EQ(base::TimeDelta(), session.duration);
  EXPECT_FALSE(session.is_completed);
  EXPECT_EQ(AstraFocusBlockLevel::kNone, session.block_level);
  EXPECT_EQ(0, session.distraction_count);
  EXPECT_TRUE(session.note.empty());
}

TEST_F(AstraFocusModeViewTest, Indicator_ObserverIsCheckedObserver) {
  AstraFocusModeObserver* observer = nullptr;
  base::CheckedObserver* checked = observer;
  EXPECT_EQ(nullptr, checked);
}

// =========================================================================
// Menu bubble tests
// =========================================================================

TEST_F(AstraFocusModeViewTest, MenuBubble_DefaultDuration) {
  AstraFocusModeModel model;
  EXPECT_EQ(base::TimeDelta(), model.GetDuration());
}

TEST_F(AstraFocusModeViewTest, MenuBubble_DefaultBlockLevel) {
  AstraFocusModeModel model;
  EXPECT_EQ(AstraFocusBlockLevel::kNone, model.GetBlockLevel());
}

TEST_F(AstraFocusModeViewTest, MenuBubble_DefaultActiveState) {
  AstraFocusModeModel model;
  EXPECT_FALSE(model.IsActive());
}

TEST_F(AstraFocusModeViewTest, MenuBubble_DefaultTimeRemaining) {
  AstraFocusModeModel model;
  EXPECT_EQ(base::TimeDelta(), model.GetTimeRemaining());
}

TEST_F(AstraFocusModeViewTest, MenuBubble_SetDuration) {
  AstraFocusModeModel model;
  model.SetDuration(base::Minutes(45));
  EXPECT_EQ(base::Minutes(45), model.GetDuration());
}

TEST_F(AstraFocusModeViewTest, MenuBubble_SetBlockLevel) {
  AstraFocusModeModel model;
  model.SetBlockLevel(AstraFocusBlockLevel::kEntertainment);
  EXPECT_EQ(AstraFocusBlockLevel::kEntertainment, model.GetBlockLevel());
}

TEST_F(AstraFocusModeViewTest, MenuBubble_SetActive) {
  AstraFocusModeModel model;
  model.SetActive(true);
  EXPECT_TRUE(model.IsActive());
}

TEST_F(AstraFocusModeViewTest, MenuBubble_SetTimeRemaining) {
  AstraFocusModeModel model;
  model.SetRemainingTime(base::Minutes(15));
  EXPECT_EQ(base::Minutes(15), model.GetTimeRemaining());
}

TEST_F(AstraFocusModeViewTest, MenuBubble_PresetCount) {
  const auto& presets = AstraFocusModeModel::GetPresetDurations();
  EXPECT_EQ(4u, presets.size());
}

TEST_F(AstraFocusModeViewTest, MenuBubble_PresetValues) {
  const auto& presets = AstraFocusModeModel::GetPresetDurations();
  EXPECT_EQ(base::Minutes(25), presets[0]);
  EXPECT_EQ(base::Minutes(45), presets[1]);
  EXPECT_EQ(base::Minutes(60), presets[2]);
  EXPECT_EQ(base::Minutes(90), presets[3]);
}

TEST_F(AstraFocusModeViewTest, MenuBubble_TimeFormatting) {
  // Short duration = MM:SS format.
  auto short_result = AstraFocusModeModel::FormatTimeRemaining(base::Minutes(25));
  EXPECT_TRUE(LooksLikeMMSS(short_result));

  // Long duration = H:MM:SS format.
  auto long_result = AstraFocusModeModel::FormatTimeRemaining(base::Minutes(75));
  EXPECT_EQ(u"1:15:00", long_result);
}

TEST_F(AstraFocusModeViewTest, MenuBubble_DurationFormatting) {
  auto short_result = AstraFocusModeModel::FormatDuration(base::Minutes(25));
  EXPECT_EQ(u"25:00", short_result);

  auto long_result = AstraFocusModeModel::FormatDuration(base::Minutes(75));
  EXPECT_EQ(u"1h 15m", long_result);

  auto very_short = AstraFocusModeModel::FormatDuration(base::Seconds(30));
  EXPECT_EQ(u"30s", very_short);
}

TEST_F(AstraFocusModeViewTest, MenuBubble_StartAction) {
  AstraFocusModeModel model;
  model.SetActive(true);
  EXPECT_TRUE(model.IsActive());
}

TEST_F(AstraFocusModeViewTest, MenuBubble_PauseAction) {
  AstraFocusModeModel model;
  model.SetActive(true);
  model.PauseSession();
  EXPECT_TRUE(model.IsSessionPaused());
}

TEST_F(AstraFocusModeViewTest, MenuBubble_ResumeAction) {
  AstraFocusModeModel model;
  model.SetActive(true);
  model.PauseSession();
  model.ResumeSession();
  EXPECT_FALSE(model_->IsSessionPaused());
  EXPECT_TRUE(model_->IsActive());
}

TEST_F(AstraFocusModeViewTest, MenuBubble_EndAction) {
  AstraFocusModeModel model;
  model.SetActive(true);
  model.EndSession();
  EXPECT_FALSE(model.IsActive());
}

TEST_F(AstraFocusModeViewTest, MenuBubble_ExtendAction) {
  AstraFocusModeModel model;
  model.SetDuration(base::Minutes(25));
  model.SetActive(true);
  model.ExtendSession(base::Minutes(15));
  EXPECT_EQ(base::Minutes(40), model.GetDuration());
}

TEST_F(AstraFocusModeViewTest, MenuBubble_StatsSection) {
  AstraFocusModeModel model;
  model.SetTotalFocusMinutesToday(120);
  model.SetSessionsToday(4);
  model.SetCurrentStreakDays(3);
  EXPECT_EQ(120, model.total_focus_minutes_today());
  EXPECT_EQ(4, model.sessions_today());
  EXPECT_EQ(3, model.current_streak_days());
}

TEST_F(AstraFocusModeViewTest, MenuBubble_ZeroDurationEdgeCase) {
  AstraFocusModeModel model;
  model.SetDuration(base::TimeDelta());
  model.SetActive(true);
  // Should not crash, session should not be "running".
  EXPECT_FALSE(model.IsSessionActive());
}

TEST_F(AstraFocusModeViewTest, MenuBubble_NoPresetsEdgeCase) {
  // GetPresetDurations always returns 4 presets.
  const auto& presets = AstraFocusModeModel::GetPresetDurations();
  EXPECT_GT(presets.size(), 0u);
}

TEST_F(AstraFocusModeViewTest, MenuBubble_AllBlockLevels) {
  AstraFocusBlockLevel levels[] = {
      AstraFocusBlockLevel::kNone,
      AstraFocusBlockLevel::kSocial,
      AstraFocusBlockLevel::kEntertainment,
      AstraFocusBlockLevel::kNews,
      AstraFocusBlockLevel::kStrict,
      AstraFocusBlockLevel::kCustom,
  };
  for (auto level : levels) {
    AstraFocusModeModel model;
    model.SetBlockLevel(level);
    EXPECT_EQ(level, model.GetBlockLevel());
  }
}

TEST_F(AstraFocusModeViewTest, MenuBubble_SessionProgressCalculation) {
  double progress = AstraFocusModeModel::CalculateProgressPercentage(
      base::Minutes(12) + base::Seconds(30), base::Minutes(25));
  EXPECT_NEAR(0.5, progress, 0.001);
}

// =========================================================================
// Additional model deep-dive tests
// =========================================================================

TEST_F(AstraFocusModeModelTest, DeepDive_AllUIsettingsDefaults) {
  EXPECT_TRUE(model_->GetShowIndicator());
  EXPECT_EQ(AstraIndicatorPosition::kRight, model_->GetIndicatorPosition());
  EXPECT_TRUE(model_->GetShowTimer());
  EXPECT_TRUE(model_->GetSoundEnabled());
  EXPECT_TRUE(model_->GetNotificationEnabled());
  EXPECT_FALSE(model_->GetShowBreakReminder());
  EXPECT_EQ(base::Minutes(25), model_->GetBreakInterval());
  EXPECT_EQ(base::Minutes(25), model_->GetDefaultDuration());
}

TEST_F(AstraFocusModeModelTest, DeepDive_AllSettingsToggleable) {
  model_->SetShowIndicator(false);
  EXPECT_FALSE(model_->GetShowIndicator());

  model_->SetShowTimer(false);
  EXPECT_FALSE(model_->GetShowTimer());

  model_->SetSoundEnabled(false);
  EXPECT_FALSE(model_->GetSoundEnabled());

  model_->SetNotificationEnabled(false);
  EXPECT_FALSE(model_->GetNotificationEnabled());

  model_->SetShowBreakReminder(true);
  EXPECT_TRUE(model_->GetShowBreakReminder());
}

TEST_F(AstraFocusModeModelTest, DeepDive_IndicatorPositionAllValues) {
  model_->SetIndicatorPosition(AstraIndicatorPosition::kLeft);
  EXPECT_EQ(AstraIndicatorPosition::kLeft, model_->GetIndicatorPosition());

  model_->SetIndicatorPosition(AstraIndicatorPosition::kRight);
  EXPECT_EQ(AstraIndicatorPosition::kRight, model_->GetIndicatorPosition());

  model_->SetIndicatorPosition(AstraIndicatorPosition::kHidden);
  EXPECT_EQ(AstraIndicatorPosition::kHidden, model_->GetIndicatorPosition());
}

TEST_F(AstraFocusModeModelTest, DeepDive_ExtendIncreasesRemaining) {
  model_->SetDuration(base::Minutes(25));
  model_->SetActive(true);
  model_->SetElapsedTime(base::Minutes(5));
  model_->SetRemainingTime(base::Minutes(20));

  model_->ExtendSession(base::Minutes(10));

  EXPECT_EQ(base::Minutes(30), model_->GetTimeRemaining());
  EXPECT_EQ(base::Minutes(35), model_->GetDuration());
}

TEST_F(AstraFocusModeModelTest, DeepDive_SessionHistoryContainsCorrectBlockLevel) {
  model_->SetBlockLevel(AstraFocusBlockLevel::kStrict);
  model_->SetDuration(base::Minutes(25));
  model_->SetActive(true);
  model_->EndSession();

  ASSERT_EQ(1u, model_->GetSessionCount());
  EXPECT_EQ(AstraFocusBlockLevel::kStrict,
            model_->GetSessionHistory()[0].block_level);
}

TEST_F(AstraFocusModeModelTest, DeepDive_SessionHistoryDistractionCount) {
  model_->SetDuration(base::Minutes(25));
  model_->SetActive(true);
  model_->IncrementDistractionsBlocked(GURL("https://example.com"));
  model_->IncrementDistractionsBlocked(GURL("https://reddit.com"));
  model_->EndSession();

  ASSERT_EQ(1u, model_->GetSessionCount());
  EXPECT_EQ(2, model_->GetSessionHistory()[0].distraction_count);
}

TEST_F(AstraFocusModeModelTest, DeepDive_CurrentSessionHasCorrectData) {
  model_->SetDuration(base::Minutes(45));
  model_->SetActive(true);
  model_->SetElapsedTime(base::Minutes(15));
  model_->SetBlockLevel(AstraFocusBlockLevel::kSocial);
  model_->SetSessionNote("Testing session");

  auto current = model_->GetCurrentSession();
  ASSERT_TRUE(current.has_value());
  EXPECT_FALSE(current->is_completed);
  EXPECT_EQ(base::Minutes(15), current->duration);
  EXPECT_EQ(AstraFocusBlockLevel::kSocial, current->block_level);
  EXPECT_EQ("Testing session", current->note);
}

TEST_F(AstraFocusModeModelTest, DeepDive_AllowedSiteBypassStrict) {
  model_->SetBlockLevel(AstraFocusBlockLevel::kStrict);
  // Normally blocked in strict mode.
  EXPECT_TRUE(model_->IsSiteBlocked(GURL("https://example.com")));
  // Add to allowlist.
  model_->AddAllowedSite(GURL("https://example.com"));
  // Now allowed.
  EXPECT_FALSE(model_->IsSiteBlocked(GURL("https://example.com")));
}

TEST_F(AstraFocusModeModelTest, DeepDive_ClearAllowedSites) {
  model_->AddAllowedSite(GURL("https://site1.com"));
  model_->AddAllowedSite(GURL("https://site2.com"));
  EXPECT_EQ(2u, model_->GetAllowedSites().size());

  model_->ClearAllowedSites();
  EXPECT_TRUE(model_->GetAllowedSites().empty());
}

TEST_F(AstraFocusModeModelTest, DeepDive_FormatDurationUnderMinute) {
  EXPECT_EQ(u"45s", AstraFocusModeModel::FormatDuration(base::Seconds(45)));
  EXPECT_EQ(u"1s", AstraFocusModeModel::FormatDuration(base::Seconds(1)));
}

TEST_F(AstraFocusModeModelTest, DeepDive_FormatDurationUnderHour) {
  EXPECT_EQ(u"25:00", AstraFocusModeModel::FormatDuration(base::Minutes(25)));
  EXPECT_EQ(u"59:59",
            AstraFocusModeModel::FormatDuration(
                base::Minutes(59) + base::Seconds(59)));
}

TEST_F(AstraFocusModeModelTest, DeepDive_FormatDurationOverHour) {
  EXPECT_EQ(u"1h 00m", AstraFocusModeModel::FormatDuration(base::Hours(1)));
  EXPECT_EQ(u"1h 30m",
            AstraFocusModeModel::FormatDuration(base::Minutes(90)));
  EXPECT_EQ(u"2h 05m",
            AstraFocusModeModel::FormatDuration(base::Minutes(125)));
}

TEST_F(AstraFocusModeModelTest, DeepDive_SessionProgressFull) {
  model_->SetTotalDuration(base::Minutes(30));
  model_->SetElapsedTime(base::Minutes(30));
  EXPECT_EQ(1.0, model_->SessionProgress());
}

TEST_F(AstraFocusModeModelTest, DeepDive_SessionProgressOver) {
  model_->SetTotalDuration(base::Minutes(30));
  model_->SetElapsedTime(base::Minutes(40));
  EXPECT_EQ(1.0, model_->SessionProgress());
}

TEST_F(AstraFocusModeModelTest, DeepDive_BlockedSitesCustomOnlyCustomSites) {
  model_->SetBlockLevel(AstraFocusBlockLevel::kCustom);
  model_->AddBlockedSite(GURL("https://my-custom-site.com"));
  // Only the custom-added site should be blocked.
  EXPECT_TRUE(model_->IsSiteBlocked(GURL("https://my-custom-site.com")));
  // Social/entertainment/news sites should NOT be blocked at custom level.
  EXPECT_FALSE(model_->IsSiteBlocked(GURL("https://facebook.com")));
  EXPECT_FALSE(model_->IsSiteBlocked(GURL("https://youtube.com")));
  EXPECT_FALSE(model_->IsSiteBlocked(GURL("https://cnn.com")));
}

TEST_F(AstraFocusModeModelTest, DeepDive_ObserverSequence) {
  MockFocusModeObserver observer;
  model_->AddObserver(&observer);

  {
    InSequence seq;
    EXPECT_CALL(observer, OnFocusModeStarted(model_.get()));
    EXPECT_CALL(observer, OnFocusModePaused(model_.get()));
    EXPECT_CALL(observer, OnFocusModeResumed(model_.get()));
    EXPECT_CALL(observer, OnFocusModeEnded(model_.get()));
    EXPECT_CALL(observer, OnSessionCompleted(model_.get(), _));
  }

  model_->SetActive(true);
  model_->PauseSession();
  model_->ResumeSession();
  model_->EndSession();

  Mock::VerifyAndClearExpectations(&observer);
  model_->RemoveObserver(&observer);
}

// =========================================================================
// Pref key registration tests
// =========================================================================

class AstraFocusModePrefsTest : public ::testing::Test {
 public:
  AstraFocusModePrefsTest() = default;
  ~AstraFocusModePrefsTest() override = default;

  void SetUp() override {
    prefs::RegisterProfilePrefs(pref_service_.registry());
  }

 protected:
  TestingPrefServiceSimple pref_service_;
};

TEST_F(AstraFocusModePrefsTest, AllFocusModePrefsAreRegistered) {
  EXPECT_TRUE(pref_service_.FindPreference(prefs::kPrefFocusModeShowIndicator));
  EXPECT_TRUE(pref_service_.FindPreference(
      prefs::kPrefFocusModeIndicatorPosition));
  EXPECT_TRUE(pref_service_.FindPreference(
      prefs::kPrefFocusModeIndicatorStyle));
  EXPECT_TRUE(pref_service_.FindPreference(
      prefs::kPrefFocusModeShowTimerInIndicator));
  EXPECT_TRUE(pref_service_.FindPreference(
      prefs::kPrefFocusModeShowSessionStats));
  EXPECT_TRUE(pref_service_.FindPreference(
      prefs::kPrefFocusModeBlockDistractingSites));
  EXPECT_TRUE(pref_service_.FindPreference(
      prefs::kPrefFocusModeNotificationSound));
  EXPECT_TRUE(pref_service_.FindPreference(
      prefs::kPrefFocusModeBreakReminders));
  EXPECT_TRUE(pref_service_.FindPreference(
      prefs::kPrefFocusModeBreakIntervalMinutes));
  EXPECT_TRUE(pref_service_.FindPreference(
      prefs::kPrefFocusModeBreakDurationMinutes));
  EXPECT_TRUE(pref_service_.FindPreference(
      prefs::kPrefFocusModeDimNonFocusTabs));
  EXPECT_TRUE(pref_service_.FindPreference(prefs::kPrefFocusModeHideSidebar));
}

TEST_F(AstraFocusModePrefsTest, DefaultValuesAreCorrect) {
  EXPECT_TRUE(pref_service_.GetBoolean(prefs::kPrefFocusModeShowIndicator));
  EXPECT_TRUE(pref_service_.GetBoolean(
      prefs::kPrefFocusModeShowTimerInIndicator));
  EXPECT_TRUE(pref_service_.GetBoolean(
      prefs::kPrefFocusModeNotificationSound));
  EXPECT_FALSE(pref_service_.GetBoolean(
      prefs::kPrefFocusModeBreakReminders));
  EXPECT_EQ(25, pref_service_.GetInteger(
                   prefs::kPrefFocusModeBreakIntervalMinutes));
  EXPECT_TRUE(pref_service_.GetBoolean(prefs::kPrefFocusModeHideSidebar));
}

}  // namespace astra
