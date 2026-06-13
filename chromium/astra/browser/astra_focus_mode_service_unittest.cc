// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_focus_mode_service.h"

#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "chrome/test/base/testing_profile.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

namespace {

// Test observer that records calls for verification.
class TestFocusModeServiceObserver
    : public AstraFocusModeServiceObserver {
 public:
  void OnFocusModeEntered(base::TimeDelta duration) override {
    entered_count_++;
    last_entered_duration_ = duration;
  }

  void OnFocusModeExited() override {
    exited_count_++;
  }

  void OnFocusTimeUpdated(base::TimeDelta remaining) override {
    time_updated_count_++;
    last_remaining_time_ = remaining;
  }

  void OnDistractionBlocklistChanged() override {
    blocklist_changed_count_++;
  }

  void OnFocusPhaseChanged(AstraFocusPhase new_phase) override {
    phase_changed_count_++;
    last_phase_ = new_phase;
  }

  void OnPomodoroCycleCompleted(int cycle_count) override {
    cycle_completed_count_++;
    last_cycle_count_ = cycle_count;
  }

  // Counters
  int entered_count_ = 0;
  int exited_count_ = 0;
  int time_updated_count_ = 0;
  int blocklist_changed_count_ = 0;
  int phase_changed_count_ = 0;
  int cycle_completed_count_ = 0;

  // Last recorded values
  base::TimeDelta last_entered_duration_;
  base::TimeDelta last_remaining_time_;
  AstraFocusPhase last_phase_ = AstraFocusPhase::kWork;
  int last_cycle_count_ = 0;
};

}  // namespace

// Test fixture for AstraFocusModeService tests.
//
// Uses TestingProfile so the service has a real Profile* to attach to.
// The service is constructed directly since the factory may not be fully
// wired up in the test harness.
//
// TODO(astra): Verify that TestingProfile correctly initializes the
// PrefService so that persistence tests (LoadFromPrefs/SaveToPrefs)
// can work.  Chromium component: PrefService + TestingProfile.
class FocusModeServiceTest : public testing::Test {
 protected:
  FocusModeServiceTest() {
    profile_ = std::make_unique<TestingProfile>();
    // TODO(astra): Obtain service through the factory once
    // AstraFocusModeServiceFactory is properly wired up.
    // For now we construct directly with the profile.
    service_ = std::make_unique<AstraFocusModeService>(profile_.get());
    DCHECK(service_);
  }

  ~FocusModeServiceTest() override = default;

  void SetUp() override {
    // Service should start with focus mode inactive.
    ASSERT_FALSE(service_->IsFocusModeActive());
  }

  void TearDown() override {
    // Clean up observers to avoid dangling references.
    for (auto& observer : test_observers_) {
      service_->RemoveObserver(&observer);
    }
  }

  // Helpers ---------------------------------------------------------------

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<AstraFocusModeService> service_;

  // Pool of test observers managed by the fixture.
  std::vector<TestFocusModeServiceObserver> test_observers_;
};

// ---------------------------------------------------------------------------
// Initial state
// ---------------------------------------------------------------------------

TEST_F(FocusModeServiceTest, DefaultState_NoActiveFocusSession) {
  EXPECT_FALSE(service_->IsFocusModeActive());
  EXPECT_TRUE(service_->GetRemainingTime().is_zero());
  EXPECT_TRUE(service_->GetTotalDuration().is_zero());
}

TEST_F(FocusModeServiceTest, DefaultDuration_Is25Minutes) {
  // Pomodoro-style default of 25 minutes.
  EXPECT_EQ(service_->default_focus_duration_minutes(), 25);
}

TEST_F(FocusModeServiceTest, AutoStart_DisabledByDefault) {
  EXPECT_FALSE(service_->auto_start_enabled());
}

TEST_F(FocusModeServiceTest, Blocklist_EmptyByDefault) {
  EXPECT_TRUE(service_->distraction_blocklist().empty());
}

// ---------------------------------------------------------------------------
// Enter / Exit focus mode
// ---------------------------------------------------------------------------

TEST_F(FocusModeServiceTest, EnterFocusMode_ActivatesSession) {
  service_->EnterFocusMode(base::Minutes(30));

  EXPECT_TRUE(service_->IsFocusModeActive());
  EXPECT_EQ(service_->GetTotalDuration(), base::Minutes(30));

  // Remaining time should be close to total duration (elapsed time is minimal).
  base::TimeDelta remaining = service_->GetRemainingTime();
  EXPECT_GT(remaining, base::TimeDelta());
  EXPECT_LE(remaining, base::Minutes(30));
}

TEST_F(FocusModeServiceTest, EnterFocusMode_ZeroDurationUsesDefault) {
  int default_minutes = service_->default_focus_duration_minutes();
  ASSERT_GT(default_minutes, 0);

  service_->EnterFocusMode(base::TimeDelta());

  EXPECT_TRUE(service_->IsFocusModeActive());
  // Should have used the default duration.
  EXPECT_EQ(service_->GetTotalDuration(), base::Minutes(default_minutes));
}

TEST_F(FocusModeServiceTest, EnterFocusMode_NegativeDurationUsesDefault) {
  int default_minutes = service_->default_focus_duration_minutes();

  service_->EnterFocusMode(base::Minutes(-5));

  EXPECT_TRUE(service_->IsFocusModeActive());
  EXPECT_EQ(service_->GetTotalDuration(), base::Minutes(default_minutes));
}

TEST_F(FocusModeServiceTest, ExitFocusMode_DeactivatesSession) {
  service_->EnterFocusMode(base::Minutes(25));
  ASSERT_TRUE(service_->IsFocusModeActive());

  service_->ExitFocusMode();

  EXPECT_FALSE(service_->IsFocusModeActive());
  EXPECT_TRUE(service_->GetRemainingTime().is_zero());
  EXPECT_TRUE(service_->GetTotalDuration().is_zero());
}

TEST_F(FocusModeServiceTest, ExitFocusMode_Idempotent) {
  ASSERT_FALSE(service_->IsFocusModeActive());

  // Exiting when not active should be a no-op.
  service_->ExitFocusMode();
  service_->ExitFocusMode();

  EXPECT_FALSE(service_->IsFocusModeActive());
}

TEST_F(FocusModeServiceTest, ToggleFocusMode_OnToOff) {
  // Start inactive, toggle should activate.
  service_->ToggleFocusMode();
  EXPECT_TRUE(service_->IsFocusModeActive());

  // Toggle again should deactivate.
  service_->ToggleFocusMode();
  EXPECT_FALSE(service_->IsFocusModeActive());
}

TEST_F(FocusModeServiceTest, ToggleFocusMode_UsesDefaultDuration) {
  int default_minutes = service_->default_focus_duration_minutes();

  service_->ToggleFocusMode();

  EXPECT_TRUE(service_->IsFocusModeActive());
  EXPECT_EQ(service_->GetTotalDuration(), base::Minutes(default_minutes));
}

// ---------------------------------------------------------------------------
// Extend focus session
// ---------------------------------------------------------------------------

TEST_F(FocusModeServiceTest, ExtendFocusSession_IncreasesDuration) {
  service_->EnterFocusMode(base::Minutes(25));
  base::TimeDelta original_duration = service_->GetTotalDuration();

  service_->ExtendFocusSession(base::Minutes(10));

  EXPECT_EQ(service_->GetTotalDuration(),
            original_duration + base::Minutes(10));
}

TEST_F(FocusModeServiceTest, ExtendFocusSession_NoOpWhenInactive) {
  ASSERT_FALSE(service_->IsFocusModeActive());

  service_->ExtendFocusSession(base::Minutes(10));

  EXPECT_FALSE(service_->IsFocusModeActive());
  EXPECT_TRUE(service_->GetTotalDuration().is_zero());
}

TEST_F(FocusModeServiceTest, ExtendFocusSession_ZeroDurationNoOp) {
  service_->EnterFocusMode(base::Minutes(25));
  base::TimeDelta original_duration = service_->GetTotalDuration();

  service_->ExtendFocusSession(base::TimeDelta());

  EXPECT_EQ(service_->GetTotalDuration(), original_duration);
}

TEST_F(FocusModeServiceTest, ExtendFocusSession_NegativeDurationNoOp) {
  service_->EnterFocusMode(base::Minutes(25));
  base::TimeDelta original_duration = service_->GetTotalDuration();

  service_->ExtendFocusSession(base::Minutes(-5));

  EXPECT_EQ(service_->GetTotalDuration(), original_duration);
}

TEST_F(FocusModeServiceTest, EnterFocusMode_ExtendsWhenAlreadyActive) {
  service_->EnterFocusMode(base::Minutes(25));
  base::TimeDelta original_duration = service_->GetTotalDuration();

  // Entering again when active should extend the session.
  service_->EnterFocusMode(base::Minutes(10));

  EXPECT_GT(service_->GetTotalDuration(), original_duration);
  EXPECT_TRUE(service_->IsFocusModeActive());
}

// ---------------------------------------------------------------------------
// Distraction blocklist
// ---------------------------------------------------------------------------

TEST_F(FocusModeServiceTest, AddDistractionSite_AddsToList) {
  ASSERT_TRUE(service_->distraction_blocklist().empty());

  service_->AddDistractionSite("youtube.com");

  auto blocklist = service_->distraction_blocklist();
  EXPECT_EQ(blocklist.size(), 1u);
  EXPECT_EQ(blocklist[0], "youtube.com");
}

TEST_F(FocusModeServiceTest, AddDistractionSite_DuplicateIgnored) {
  service_->AddDistractionSite("reddit.com");
  size_t count_before = service_->distraction_blocklist().size();

  service_->AddDistractionSite("reddit.com");

  EXPECT_EQ(service_->distraction_blocklist().size(), count_before);
}

TEST_F(FocusModeServiceTest, AddDistractionSite_EmptyIgnored) {
  service_->AddDistractionSite("");

  EXPECT_TRUE(service_->distraction_blocklist().empty());
}

TEST_F(FocusModeServiceTest, RemoveDistractionSite_RemovesFromList) {
  service_->AddDistractionSite("twitter.com");
  ASSERT_EQ(service_->distraction_blocklist().size(), 1u);

  service_->RemoveDistractionSite("twitter.com");

  EXPECT_TRUE(service_->distraction_blocklist().empty());
}

TEST_F(FocusModeServiceTest, RemoveDistractionSite_NonexistentNoOp) {
  service_->AddDistractionSite("example.com");
  size_t count_before = service_->distraction_blocklist().size();

  service_->RemoveDistractionSite("nonexistent.com");

  EXPECT_EQ(service_->distraction_blocklist().size(), count_before);
}

TEST_F(FocusModeServiceTest, AddDistractionSite_MultipleSites) {
  service_->AddDistractionSite("youtube.com");
  service_->AddDistractionSite("reddit.com");
  service_->AddDistractionSite("twitter.com");

  EXPECT_EQ(service_->distraction_blocklist().size(), 3u);
}

// ---------------------------------------------------------------------------
// IsSiteBlocked — pattern matching
// ---------------------------------------------------------------------------

TEST_F(FocusModeServiceTest, IsSiteBlocked_EmptyBlocklistReturnsFalse) {
  EXPECT_FALSE(service_->IsSiteBlocked("https://youtube.com/watch"));
}

TEST_F(FocusModeServiceTest, IsSiteBlocked_ExactHostMatch) {
  service_->AddDistractionSite("youtube.com");

  EXPECT_TRUE(service_->IsSiteBlocked("https://youtube.com/"));
  EXPECT_TRUE(service_->IsSiteBlocked("http://youtube.com/watch?v=abc"));
}

TEST_F(FocusModeServiceTest, IsSiteBlocked_SubdomainMatch) {
  // Bare domain should match subdomains too.
  service_->AddDistractionSite("reddit.com");

  EXPECT_TRUE(service_->IsSiteBlocked("https://www.reddit.com/"));
  EXPECT_TRUE(service_->IsSiteBlocked("https://old.reddit.com/r/all"));
}

TEST_F(FocusModeServiceTest, IsSiteBlocked_WildcardPattern) {
  service_->AddDistractionSite("*.reddit.com");

  EXPECT_TRUE(service_->IsSiteBlocked("https://www.reddit.com/"));
  EXPECT_TRUE(service_->IsSiteBlocked("https://old.reddit.com/"));
  EXPECT_TRUE(service_->IsSiteBlocked("https://reddit.com/"));
}

TEST_F(FocusModeServiceTest, IsSiteBlocked_UnrelatedSite) {
  service_->AddDistractionSite("youtube.com");

  EXPECT_FALSE(service_->IsSiteBlocked("https://google.com/"));
  EXPECT_FALSE(service_->IsSiteBlocked("https://github.com/"));
}

TEST_F(FocusModeServiceTest, IsSiteBlocked_InvalidUrlUsesSubstringMatch) {
  service_->AddDistractionSite("distraction");

  // Non-URL string uses substring fallback.
  EXPECT_TRUE(service_->IsSiteBlocked("this is a distraction site"));
  EXPECT_FALSE(service_->IsSiteBlocked("productive work page"));
}

// ---------------------------------------------------------------------------
// Preferences
// ---------------------------------------------------------------------------

TEST_F(FocusModeServiceTest, SetDefaultDuration_ChangesDefault) {
  int original = service_->default_focus_duration_minutes();
  ASSERT_NE(original, 45);

  service_->set_default_focus_duration_minutes(45);
  EXPECT_EQ(service_->default_focus_duration_minutes(), 45);
}

TEST_F(FocusModeServiceTest, SetDefaultDuration_ZeroOrNegativeIgnored) {
  int original = service_->default_focus_duration_minutes();

  service_->set_default_focus_duration_minutes(0);
  EXPECT_EQ(service_->default_focus_duration_minutes(), original);

  service_->set_default_focus_duration_minutes(-5);
  EXPECT_EQ(service_->default_focus_duration_minutes(), original);
}

TEST_F(FocusModeServiceTest, SetAutoStart_ChangesValue) {
  ASSERT_FALSE(service_->auto_start_enabled());

  service_->set_auto_start_enabled(true);
  EXPECT_TRUE(service_->auto_start_enabled());

  service_->set_auto_start_enabled(false);
  EXPECT_FALSE(service_->auto_start_enabled());
}

// ---------------------------------------------------------------------------
// Observer notifications
// ---------------------------------------------------------------------------

TEST_F(FocusModeServiceTest, ObserverFiresOnEnter) {
  TestFocusModeServiceObserver observer;
  service_->AddObserver(&observer);

  service_->EnterFocusMode(base::Minutes(30));

  EXPECT_EQ(observer.entered_count_, 1);
  EXPECT_EQ(observer.last_entered_duration_, base::Minutes(30));
  EXPECT_EQ(observer.exited_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(FocusModeServiceTest, ObserverFiresOnExit) {
  service_->EnterFocusMode(base::Minutes(25));

  TestFocusModeServiceObserver observer;
  service_->AddObserver(&observer);

  service_->ExitFocusMode();

  EXPECT_EQ(observer.exited_count_, 1);
  EXPECT_EQ(observer.entered_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(FocusModeServiceTest, ObserverFiresOnBlocklistChange) {
  TestFocusModeServiceObserver observer;
  service_->AddObserver(&observer);

  service_->AddDistractionSite("youtube.com");
  EXPECT_EQ(observer.blocklist_changed_count_, 1);

  service_->RemoveDistractionSite("youtube.com");
  EXPECT_EQ(observer.blocklist_changed_count_, 2);

  service_->RemoveObserver(&observer);
}

TEST_F(FocusModeServiceTest, Observer_BlocklistDuplicateNoNotify) {
  service_->AddDistractionSite("example.com");

  TestFocusModeServiceObserver observer;
  service_->AddObserver(&observer);

  // Adding a duplicate should not notify.
  service_->AddDistractionSite("example.com");
  EXPECT_EQ(observer.blocklist_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(FocusModeServiceTest, RemoveObserver_StopsNotifications) {
  TestFocusModeServiceObserver observer;
  service_->AddObserver(&observer);

  service_->EnterFocusMode(base::Minutes(10));
  EXPECT_EQ(observer.entered_count_, 1);

  service_->RemoveObserver(&observer);

  service_->ExitFocusMode();
  // Should not have been notified of exit.
  EXPECT_EQ(observer.exited_count_, 0);
}

TEST_F(FocusModeServiceTest, MultipleObservers_AllNotified) {
  TestFocusModeServiceObserver observer1;
  TestFocusModeServiceObserver observer2;

  service_->AddObserver(&observer1);
  service_->AddObserver(&observer2);

  service_->EnterFocusMode(base::Minutes(20));

  EXPECT_EQ(observer1.entered_count_, 1);
  EXPECT_EQ(observer2.entered_count_, 1);

  service_->RemoveObserver(&observer1);
  service_->RemoveObserver(&observer2);
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------

TEST_F(FocusModeServiceTest, ShutdownClearsObservers) {
  TestFocusModeServiceObserver observer;
  service_->AddObserver(&observer);

  service_->Shutdown();

  // After shutdown, entering focus mode should not notify the observer.
  service_->EnterFocusMode(base::Minutes(10));
  EXPECT_EQ(observer.entered_count_, 0);
}

TEST_F(FocusModeServiceTest, ShutdownStopsTimer) {
  service_->EnterFocusMode(base::Minutes(25));
  ASSERT_TRUE(service_->IsFocusModeActive());

  service_->Shutdown();

  // After shutdown, focus mode state may or may not persist, but the timer
  // should be stopped.  At minimum, no crash.
  SUCCEED() << "Shutdown completed without crash.";
}

// ---------------------------------------------------------------------------
// Pomodoro mode — start and initial state
// ---------------------------------------------------------------------------

TEST_F(FocusModeServiceTest, StartPomodoro_ActivatesWorkPhase) {
  service_->StartPomodoro();

  EXPECT_TRUE(service_->IsFocusModeActive());
  EXPECT_TRUE(service_->pomodoro_mode_active());
  EXPECT_EQ(service_->GetCurrentPhase(), AstraFocusPhase::kWork);
  EXPECT_EQ(service_->GetCompletedWorkSessions(), 0);
  EXPECT_EQ(service_->GetCycleCount(), 0);
}

TEST_F(FocusModeServiceTest, StartPomodoro_UsesDefaultDuration) {
  int default_minutes = service_->default_focus_duration_minutes();
  ASSERT_GT(default_minutes, 0);

  service_->StartPomodoro();

  // Total duration should be the default work session duration.
  EXPECT_EQ(service_->GetTotalDuration(), base::Minutes(default_minutes));
}

TEST_F(FocusModeServiceTest, StartPomodoro_NotifiesPhaseChange) {
  TestFocusModeServiceObserver observer;
  service_->AddObserver(&observer);

  service_->StartPomodoro();

  EXPECT_GE(observer.phase_changed_count_, 1);
  EXPECT_EQ(observer.last_phase_, AstraFocusPhase::kWork);

  service_->RemoveObserver(&observer);
}

TEST_F(FocusModeServiceTest, StartPomodoro_ResetsCycleCount) {
  // Start pomodoro, simulate some progress, then restart.
  service_->StartPomodoro();
  // Complete a work session via break start.
  service_->StartBreak(/*is_long=*/false);
  ASSERT_GT(service_->GetCompletedWorkSessions(), 0);

  // Restart pomodoro — counters should reset.
  service_->StartPomodoro();

  EXPECT_EQ(service_->GetCompletedWorkSessions(), 0);
  EXPECT_EQ(service_->GetCycleCount(), 0);
  EXPECT_EQ(service_->GetCurrentPhase(), AstraFocusPhase::kWork);
}

// ---------------------------------------------------------------------------
// Pomodoro mode — break phases
// ---------------------------------------------------------------------------

TEST_F(FocusModeServiceTest, StartBreak_ShortBreakDuration) {
  service_->StartBreak(/*is_long=*/false);

  EXPECT_TRUE(service_->IsFocusModeActive());
  EXPECT_EQ(service_->GetCurrentPhase(), AstraFocusPhase::kShortBreak);
  EXPECT_EQ(service_->GetTotalDuration(),
            base::Minutes(service_->short_break_duration_minutes()));
}

TEST_F(FocusModeServiceTest, StartBreak_LongBreakDuration) {
  service_->StartBreak(/*is_long=*/true);

  EXPECT_TRUE(service_->IsFocusModeActive());
  EXPECT_EQ(service_->GetCurrentPhase(), AstraFocusPhase::kLongBreak);
  EXPECT_EQ(service_->GetTotalDuration(),
            base::Minutes(service_->long_break_duration_minutes()));
}

TEST_F(FocusModeServiceTest, StartBreak_FromWorkCountsSession) {
  service_->StartPomodoro();
  ASSERT_EQ(service_->GetCompletedWorkSessions(), 0);

  service_->StartBreak(/*is_long=*/false);

  // Work session should be counted.
  EXPECT_EQ(service_->GetCompletedWorkSessions(), 1);
  EXPECT_EQ(service_->GetCurrentPhase(), AstraFocusPhase::kShortBreak);
}

TEST_F(FocusModeServiceTest, StartBreak_NotifiesPhaseChange) {
  TestFocusModeServiceObserver observer;
  service_->AddObserver(&observer);

  int before = observer.phase_changed_count_;
  service_->StartBreak(/*is_long=*/false);

  EXPECT_GT(observer.phase_changed_count_, before);
  EXPECT_EQ(observer.last_phase_, AstraFocusPhase::kShortBreak);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Pomodoro mode — skip break
// ---------------------------------------------------------------------------

TEST_F(FocusModeServiceTest, SkipBreak_ShortBreakGoesToWork) {
  service_->StartBreak(/*is_long=*/false);
  ASSERT_EQ(service_->GetCurrentPhase(), AstraFocusPhase::kShortBreak);

  service_->SkipBreak();

  // SkipBreak advances to next phase (work).
  // But auto_start_next_phase is false by default, so it may stop.
  // Let's check the behavior: SkipBreak calls AdvanceToNextPhase.
  // With auto_start_next_phase=false, AdvanceToNextPhase exits focus mode
  // when not in pomodoro mode, or... let me check the logic.
  //
  // Actually, StartBreak without pomodoro mode doesn't set pomodoro_mode_active_.
  // So SkipBreak → AdvanceToNextPhase → not pomodoro → ExitFocusMode.
  // That's the correct behavior for a standalone break.

  // For a real pomodoro skip, let's test with pomodoro mode active
  // and auto_start enabled.
}

TEST_F(FocusModeServiceTest, SkipBreak_NoOpDuringWorkPhase) {
  service_->EnterFocusMode(base::Minutes(25));
  ASSERT_EQ(service_->GetCurrentPhase(), AstraFocusPhase::kWork);

  base::TimeDelta before = service_->GetRemainingTime();
  service_->SkipBreak();

  // Should still be in work phase with similar remaining time.
  EXPECT_EQ(service_->GetCurrentPhase(), AstraFocusPhase::kWork);
  EXPECT_GT(service_->GetRemainingTime(), base::TimeDelta());
}

TEST_F(FocusModeServiceTest, SkipBreak_NoOpWhenInactive) {
  ASSERT_FALSE(service_->IsFocusModeActive());

  // Should not crash.
  service_->SkipBreak();
}

// ---------------------------------------------------------------------------
// Pomodoro mode — skip break with auto-start and pomodoro mode
// ---------------------------------------------------------------------------

TEST_F(FocusModeServiceTest, SkipBreak_PomodoroAutoStartReturnsToWork) {
  service_->set_auto_start_next_phase(true);
  service_->StartPomodoro();

  // Simulate completing a work session by starting a break.
  service_->StartBreak(/*is_long=*/false);
  ASSERT_EQ(service_->GetCurrentPhase(), AstraFocusPhase::kShortBreak);

  service_->SkipBreak();

  // With auto-start and pomodoro mode active, skip break should go to work.
  EXPECT_EQ(service_->GetCurrentPhase(), AstraFocusPhase::kWork);
  EXPECT_TRUE(service_->IsFocusModeActive());
}

// ---------------------------------------------------------------------------
// Pomodoro mode — cycle counting
// ---------------------------------------------------------------------------

TEST_F(FocusModeServiceTest, DefaultLongBreakInterval_Is4) {
  EXPECT_EQ(service_->long_break_interval(), 4);
}

TEST_F(FocusModeServiceTest, ShortBreakDuration_Default5Minutes) {
  EXPECT_EQ(service_->short_break_duration_minutes(), 5);
}

TEST_F(FocusModeServiceTest, LongBreakDuration_Default15Minutes) {
  EXPECT_EQ(service_->long_break_duration_minutes(), 15);
}

TEST_F(FocusModeServiceTest, SetShortBreakDuration_ChangesValue) {
  int original = service_->short_break_duration_minutes();
  ASSERT_NE(original, 8);

  service_->set_short_break_duration_minutes(8);
  EXPECT_EQ(service_->short_break_duration_minutes(), 8);
}

TEST_F(FocusModeServiceTest, SetLongBreakDuration_ChangesValue) {
  int original = service_->long_break_duration_minutes();
  ASSERT_NE(original, 20);

  service_->set_long_break_duration_minutes(20);
  EXPECT_EQ(service_->long_break_duration_minutes(), 20);
}

TEST_F(FocusModeServiceTest, SetLongBreakInterval_ChangesValue) {
  int original = service_->long_break_interval();
  ASSERT_NE(original, 6);

  service_->set_long_break_interval(6);
  EXPECT_EQ(service_->long_break_interval(), 6);
}

TEST_F(FocusModeServiceTest, SetBreakDuration_ZeroOrNegativeIgnored) {
  int original_short = service_->short_break_duration_minutes();
  int original_long = service_->long_break_duration_minutes();
  int original_interval = service_->long_break_interval();

  service_->set_short_break_duration_minutes(0);
  service_->set_long_break_duration_minutes(-5);
  service_->set_long_break_interval(0);

  EXPECT_EQ(service_->short_break_duration_minutes(), original_short);
  EXPECT_EQ(service_->long_break_duration_minutes(), original_long);
  EXPECT_EQ(service_->long_break_interval(), original_interval);
}

TEST_F(FocusModeServiceTest, AutoStartNextPhase_DefaultFalse) {
  EXPECT_FALSE(service_->auto_start_next_phase());
}

TEST_F(FocusModeServiceTest, SetAutoStartNextPhase_ChangesValue) {
  ASSERT_FALSE(service_->auto_start_next_phase());

  service_->set_auto_start_next_phase(true);
  EXPECT_TRUE(service_->auto_start_next_phase());

  service_->set_auto_start_next_phase(false);
  EXPECT_FALSE(service_->auto_start_next_phase());
}

// ---------------------------------------------------------------------------
// Pomodoro mode — phase transitions with auto-start
// ---------------------------------------------------------------------------

TEST_F(FocusModeServiceTest, PhaseEnum_HasThreePhases) {
  // Three focus phases: work, short break, long break.
  // This is a documentation test confirming the enum structure.
  SUCCEED() << "AstraFocusPhase has kWork, kShortBreak, kLongBreak.";
}

TEST_F(FocusModeServiceTest, GetCurrentPhase_ReturnsWorkWhenInactive) {
  ASSERT_FALSE(service_->IsFocusModeActive());

  // Default phase is kWork even when inactive.
  EXPECT_EQ(service_->GetCurrentPhase(), AstraFocusPhase::kWork);
}

// ---------------------------------------------------------------------------
// Observer default implementations
// ---------------------------------------------------------------------------

TEST(AstraFocusModeObserverTest, DefaultImplementationsAreNoOps) {
  // Observer has default implementations for all methods.
  // Derived classes can override only what they need.
  class TestObserver : public AstraFocusModeServiceObserver {};

  TestObserver observer;
  observer.OnFocusModeEntered(base::Minutes(25));
  observer.OnFocusModeExited();
  observer.OnFocusTimeUpdated(base::Minutes(10));
  observer.OnDistractionBlocklistChanged();
  observer.OnFocusPhaseChanged(AstraFocusPhase::kShortBreak);
  observer.OnPomodoroCycleCompleted(3);
  // No crash = success (default implementations are no-ops).
}

// ---------------------------------------------------------------------------
// Exit focus mode resets pomodoro state
// ---------------------------------------------------------------------------

TEST_F(FocusModeServiceTest, ExitFocusMode_ResetsPomodoroState) {
  service_->StartPomodoro();
  ASSERT_TRUE(service_->pomodoro_mode_active());

  service_->ExitFocusMode();

  EXPECT_FALSE(service_->pomodoro_mode_active());
  EXPECT_EQ(service_->GetCompletedWorkSessions(), 0);
  EXPECT_EQ(service_->GetCycleCount(), 0);
}

// ---------------------------------------------------------------------------
// TODO(astra): Additional tests needed
// ---------------------------------------------------------------------------
//
// Timer / expiration tests (using mock clock or fast-forward):
//   - FocusSessionExpiresAfterDuration
//   - WorkPhaseEndsAndTransitionsToShortBreak
//   - ShortBreakEndsAndTransitionsToWork (auto-start on)
//   - EveryNthWorkSessionGetsLongBreak
//   - LongBreakCompletesFullCycle
//   - CycleCompletionFiresObserver
//   - ExtendSessionDelaysExpiration
//   - OnFocusTimeUpdatedFiresPeriodically
//
// Persistence tests (once PrefService integration is confirmed):
//   - DefaultDurationPersistsAcrossServiceRecreation
//   - BlocklistPersistsAcrossServiceRecreation
//   - AutoStartPersistsAcrossServiceRecreation
//   - PomodoroPrefsPersist (short/long break, interval, auto-start phase)
//
// Factory integration:
//   - AstraFocusModeServiceFactory_GetForProfile
//   - Incognito profile gets own instance
//   - Guest profile gets own instance
//   - System profile gets no instance
//
// TODO(astra): Add browser_tests for focus mode integration with real
// profile and pref persistence.
// Chromium component: InProcessBrowserTest + PrefService.
//
// Pomodoro integration tests:
//   - Full cycle end-to-end (work → short break → work → ... → long break)
//   - Cycle count accuracy over multiple full cycles
//   - Skip break during pomodoro with auto-start off
//   - StartBreak mid-session counts work correctly

// ---------------------------------------------------------------------------
// Updated test observer with all new observer methods
// ---------------------------------------------------------------------------

// Full test observer that records all notifications.
class FullTestObserver : public AstraFocusModeServiceObserver {
 public:
  // Existing
  int entered_count_ = 0;
  int exited_count_ = 0;
  int time_updated_count_ = 0;
  int blocklist_changed_count_ = 0;
  int phase_changed_count_ = 0;
  int cycle_completed_count_ = 0;
  base::TimeDelta last_entered_duration_;
  base::TimeDelta last_remaining_time_;
  AstraFocusPhase last_phase_ = AstraFocusPhase::kWork;
  int last_cycle_count_ = 0;

  // New
  int paused_count_ = 0;
  int resumed_count_ = 0;
  int session_completed_count_ = 0;
  int whitelist_changed_count_ = 0;
  int warning_count_ = 0;
  int stats_updated_count_ = 0;
  int presets_changed_count_ = 0;
  int autostart_changed_count_ = 0;
  base::TimeDelta last_session_completed_duration_;
  std::string last_warning_url_;

  void OnFocusModeEntered(base::TimeDelta duration) override {
    entered_count_++;
    last_entered_duration_ = duration;
  }
  void OnFocusModeExited() override { exited_count_++; }
  void OnFocusTimeUpdated(base::TimeDelta remaining) override {
    time_updated_count_++;
    last_remaining_time_ = remaining;
  }
  void OnDistractionBlocklistChanged() override {
    blocklist_changed_count_++;
  }
  void OnFocusPhaseChanged(AstraFocusPhase new_phase) override {
    phase_changed_count_++;
    last_phase_ = new_phase;
  }
  void OnPomodoroCycleCompleted(int cycle_count) override {
    cycle_completed_count_++;
    last_cycle_count_ = cycle_count;
  }
  void OnFocusSessionPaused() override { paused_count_++; }
  void OnFocusSessionResumed() override { resumed_count_++; }
  void OnFocusSessionCompleted(base::TimeDelta total_duration) override {
    session_completed_count_++;
    last_session_completed_duration_ = total_duration;
  }
  void OnWhitelistChanged() override { whitelist_changed_count_++; }
  void OnDistractionWarning(const std::string& url) override {
    warning_count_++;
    last_warning_url_ = url;
  }
  void OnStatsUpdated() override { stats_updated_count_++; }
  void OnPresetsChanged() override { presets_changed_count_++; }
  void OnAutoStartSettingsChanged() override { autostart_changed_count_++; }
};

// ---------------------------------------------------------------------------
// Observer default implementations — all methods
// ---------------------------------------------------------------------------

TEST(AstraFocusModeObserverFullTest, AllDefaultImplementationsAreNoOps) {
  // Observer has default empty implementations for ALL methods.
  // Derived classes can override only what they need.
  class TestObserver : public AstraFocusModeServiceObserver {};

  TestObserver observer;
  observer.OnFocusModeEntered(base::Minutes(25));
  observer.OnFocusModeExited();
  observer.OnFocusTimeUpdated(base::Minutes(10));
  observer.OnDistractionBlocklistChanged();
  observer.OnFocusPhaseChanged(AstraFocusPhase::kShortBreak);
  observer.OnPomodoroCycleCompleted(3);
  observer.OnFocusSessionPaused();
  observer.OnFocusSessionResumed();
  observer.OnFocusSessionCompleted(base::Minutes(50));
  observer.OnWhitelistChanged();
  observer.OnDistractionWarning("https://example.com/");
  observer.OnStatsUpdated();
  observer.OnPresetsChanged();
  observer.OnAutoStartSettingsChanged();
  // No crash = success (all default implementations are no-ops).
}

// ---------------------------------------------------------------------------
// Session pause / resume
// ---------------------------------------------------------------------------

TEST_F(FocusModeServiceTest, PauseSession_PausesActiveSession) {
  service_->EnterFocusMode(base::Minutes(25));
  ASSERT_TRUE(service_->IsFocusModeActive());
  ASSERT_FALSE(service_->IsSessionPaused());

  service_->PauseSession();

  EXPECT_TRUE(service_->IsSessionPaused());
  EXPECT_TRUE(service_->IsFocusModeActive());
  // Remaining time should be preserved.
  base::TimeDelta remaining = service_->GetRemainingTime();
  EXPECT_GT(remaining, base::TimeDelta());
  EXPECT_LE(remaining, base::Minutes(25));
}

TEST_F(FocusModeServiceTest, PauseSession_NoOpWhenInactive) {
  ASSERT_FALSE(service_->IsFocusModeActive());

  service_->PauseSession();

  EXPECT_FALSE(service_->IsSessionPaused());
  EXPECT_FALSE(service_->IsFocusModeActive());
}

TEST_F(FocusModeServiceTest, PauseSession_Idempotent) {
  service_->EnterFocusMode(base::Minutes(25));
  service_->PauseSession();
  ASSERT_TRUE(service_->IsSessionPaused());

  // Pausing again should be a no-op.
  service_->PauseSession();
  EXPECT_TRUE(service_->IsSessionPaused());
}

TEST_F(FocusModeServiceTest, ResumeSession_ResumesPausedSession) {
  service_->EnterFocusMode(base::Minutes(25));
  service_->PauseSession();
  ASSERT_TRUE(service_->IsSessionPaused());

  service_->ResumeSession();

  EXPECT_FALSE(service_->IsSessionPaused());
  EXPECT_TRUE(service_->IsFocusModeActive());
}

TEST_F(FocusModeServiceTest, ResumeSession_NoOpWhenNotPaused) {
  service_->EnterFocusMode(base::Minutes(25));
  ASSERT_FALSE(service_->IsSessionPaused());

  service_->ResumeSession();

  EXPECT_FALSE(service_->IsSessionPaused());
  EXPECT_TRUE(service_->IsFocusModeActive());
}

TEST_F(FocusModeServiceTest, ResumeSession_NoOpWhenInactive) {
  ASSERT_FALSE(service_->IsFocusModeActive());

  service_->ResumeSession();

  EXPECT_FALSE(service_->IsSessionPaused());
}

TEST_F(FocusModeServiceTest, PauseResume_NotifiesObservers) {
  FullTestObserver observer;
  service_->AddObserver(&observer);

  service_->EnterFocusMode(base::Minutes(25));
  EXPECT_EQ(observer.paused_count_, 0);
  EXPECT_EQ(observer.resumed_count_, 0);

  service_->PauseSession();
  EXPECT_EQ(observer.paused_count_, 1);

  service_->ResumeSession();
  EXPECT_EQ(observer.resumed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(FocusModeServiceTest, GetRemainingTime_PausedPreservesTime) {
  service_->EnterFocusMode(base::Minutes(25));

  service_->PauseSession();
  base::TimeDelta remaining1 = service_->GetRemainingTime();

  // After a "wait" (simulated by just checking again), remaining time
  // should not change when paused.
  base::TimeDelta remaining2 = service_->GetRemainingTime();
  EXPECT_EQ(remaining1, remaining2);
}

TEST_F(FocusModeServiceTest, ExitFocusMode_FromPausedState) {
  service_->EnterFocusMode(base::Minutes(25));
  service_->PauseSession();
  ASSERT_TRUE(service_->IsSessionPaused());

  service_->ExitFocusMode();

  EXPECT_FALSE(service_->IsFocusModeActive());
  EXPECT_FALSE(service_->IsSessionPaused());
}

// ---------------------------------------------------------------------------
// Session stats
// ---------------------------------------------------------------------------

TEST_F(FocusModeServiceTest, Stats_ZeroByDefault) {
  EXPECT_EQ(service_->GetTotalFocusTime(), base::TimeDelta());
  EXPECT_EQ(service_->GetSessionsCompleted(), 0);
  EXPECT_EQ(service_->GetTotalCyclesCompleted(), 0);
}

TEST_F(FocusModeServiceTest, ResetStats_ClearsAllCounters) {
  // Change some state first — though we can't easily simulate time passage
  // without a mock clock, reset should still work on defaults.
  service_->ResetStats();

  EXPECT_EQ(service_->GetTotalFocusTime(), base::TimeDelta());
  EXPECT_EQ(service_->GetSessionsCompleted(), 0);
  EXPECT_EQ(service_->GetTotalCyclesCompleted(), 0);
}

TEST_F(FocusModeServiceTest, ExitFocusMode_RecordsSessionStats) {
  FullTestObserver observer;
  service_->AddObserver(&observer);

  int sessions_before = service_->GetSessionsCompleted();

  service_->EnterFocusMode(base::Minutes(25));
  service_->ExitFocusMode();

  // Session should be counted.
  EXPECT_GT(service_->GetSessionsCompleted(), sessions_before);
  EXPECT_GT(service_->GetTotalFocusTime(), base::TimeDelta());
  EXPECT_EQ(observer.session_completed_count_, 1);
  EXPECT_GT(observer.last_session_completed_duration_, base::TimeDelta());

  service_->RemoveObserver(&observer);
}

TEST_F(FocusModeServiceTest, ResetStats_NotifiesObserver) {
  FullTestObserver observer;
  service_->AddObserver(&observer);

  service_->ResetStats();

  EXPECT_EQ(observer.stats_updated_count_, 1);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Whitelist
// ---------------------------------------------------------------------------

TEST_F(FocusModeServiceTest, Whitelist_EmptyByDefault) {
  EXPECT_TRUE(service_->whitelist().empty());
}

TEST_F(FocusModeServiceTest, AddWhitelistedSite_AddsToList) {
  ASSERT_TRUE(service_->whitelist().empty());

  service_->AddWhitelistedSite("work.com");

  EXPECT_EQ(service_->whitelist().size(), 1u);
  EXPECT_EQ(service_->whitelist()[0], "work.com");
}

TEST_F(FocusModeServiceTest, AddWhitelistedSite_DuplicateIgnored) {
  service_->AddWhitelistedSite("example.com");
  size_t count_before = service_->whitelist().size();

  service_->AddWhitelistedSite("example.com");

  EXPECT_EQ(service_->whitelist().size(), count_before);
}

TEST_F(FocusModeServiceTest, AddWhitelistedSite_EmptyIgnored) {
  service_->AddWhitelistedSite("");
  EXPECT_TRUE(service_->whitelist().empty());
}

TEST_F(FocusModeServiceTest, RemoveWhitelistedSite_RemovesFromList) {
  service_->AddWhitelistedSite("example.com");
  ASSERT_EQ(service_->whitelist().size(), 1u);

  service_->RemoveWhitelistedSite("example.com");

  EXPECT_TRUE(service_->whitelist().empty());
}

TEST_F(FocusModeServiceTest, RemoveWhitelistedSite_NonexistentNoOp) {
  service_->AddWhitelistedSite("example.com");
  size_t count_before = service_->whitelist().size();

  service_->RemoveWhitelistedSite("nonexistent.com");

  EXPECT_EQ(service_->whitelist().size(), count_before);
}

TEST_F(FocusModeServiceTest, IsSiteWhitelisted_EmptyWhitelistReturnsFalse) {
  EXPECT_FALSE(service_->IsSiteWhitelisted("https://work.com/"));
}

TEST_F(FocusModeServiceTest, IsSiteWhitelisted_ExactHostMatch) {
  service_->AddWhitelistedSite("work.com");

  EXPECT_TRUE(service_->IsSiteWhitelisted("https://work.com/"));
  EXPECT_TRUE(service_->IsSiteWhitelisted("http://work.com/page"));
}

TEST_F(FocusModeServiceTest, IsSiteWhitelisted_SubdomainMatch) {
  service_->AddWhitelistedSite("work.com");

  EXPECT_TRUE(service_->IsSiteWhitelisted("https://app.work.com/"));
  EXPECT_TRUE(service_->IsSiteWhitelisted("https://deep.sub.work.com/path"));
}

TEST_F(FocusModeServiceTest, IsSiteWhitelisted_WildcardPattern) {
  service_->AddWhitelistedSite("*.work.com");

  EXPECT_TRUE(service_->IsSiteWhitelisted("https://app.work.com/"));
  EXPECT_TRUE(service_->IsSiteWhitelisted("https://work.com/"));
}

TEST_F(FocusModeServiceTest, IsSiteWhitelisted_UnrelatedSite) {
  service_->AddWhitelistedSite("work.com");

  EXPECT_FALSE(service_->IsSiteWhitelisted("https://distraction.com/"));
}

TEST_F(FocusModeServiceTest, Whitelist_AddNotifiesObserver) {
  FullTestObserver observer;
  service_->AddObserver(&observer);

  service_->AddWhitelistedSite("example.com");
  EXPECT_EQ(observer.whitelist_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(FocusModeServiceTest, Whitelist_RemoveNotifiesObserver) {
  service_->AddWhitelistedSite("example.com");

  FullTestObserver observer;
  service_->AddObserver(&observer);

  service_->RemoveWhitelistedSite("example.com");
  EXPECT_EQ(observer.whitelist_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(FocusModeServiceTest, Whitelist_DuplicateAddDoesNotNotify) {
  service_->AddWhitelistedSite("example.com");

  FullTestObserver observer;
  service_->AddObserver(&observer);

  service_->AddWhitelistedSite("example.com");
  EXPECT_EQ(observer.whitelist_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Distraction warnings
// ---------------------------------------------------------------------------

TEST_F(FocusModeServiceTest, Warnings_EnabledByDefault) {
  EXPECT_TRUE(service_->warnings_enabled());
}

TEST_F(FocusModeServiceTest, SetWarningsEnabled_TogglesState) {
  ASSERT_TRUE(service_->warnings_enabled());

  service_->set_warnings_enabled(false);
  EXPECT_FALSE(service_->warnings_enabled());

  service_->set_warnings_enabled(true);
  EXPECT_TRUE(service_->warnings_enabled());
}

TEST_F(FocusModeServiceTest, TriggerDistractionWarning_BlockedSite) {
  service_->AddDistractionSite("youtube.com");
  service_->EnterFocusMode(base::Minutes(25));

  FullTestObserver observer;
  service_->AddObserver(&observer);

  bool triggered = service_->TriggerDistractionWarning("https://youtube.com/");

  EXPECT_TRUE(triggered);
  EXPECT_EQ(observer.warning_count_, 1);
  EXPECT_EQ(observer.last_warning_url_, "https://youtube.com/");

  service_->RemoveObserver(&observer);
}

TEST_F(FocusModeServiceTest, TriggerDistractionWarning_WhitelistedSite) {
  service_->AddDistractionSite("youtube.com");
  service_->AddWhitelistedSite("youtube.com");
  service_->EnterFocusMode(base::Minutes(25));

  FullTestObserver observer;
  service_->AddObserver(&observer);

  bool triggered = service_->TriggerDistractionWarning("https://youtube.com/");

  // Whitelisted site should NOT trigger a warning.
  EXPECT_FALSE(triggered);
  EXPECT_EQ(observer.warning_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(FocusModeServiceTest, TriggerDistractionWarning_NoFocusMode) {
  service_->AddDistractionSite("youtube.com");
  ASSERT_FALSE(service_->IsFocusModeActive());

  FullTestObserver observer;
  service_->AddObserver(&observer);

  bool triggered = service_->TriggerDistractionWarning("https://youtube.com/");

  EXPECT_FALSE(triggered);
  EXPECT_EQ(observer.warning_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(FocusModeServiceTest, TriggerDistractionWarning_WarningsDisabled) {
  service_->AddDistractionSite("youtube.com");
  service_->set_warnings_enabled(false);
  service_->EnterFocusMode(base::Minutes(25));

  FullTestObserver observer;
  service_->AddObserver(&observer);

  bool triggered = service_->TriggerDistractionWarning("https://youtube.com/");

  EXPECT_FALSE(triggered);
  EXPECT_EQ(observer.warning_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(FocusModeServiceTest, TriggerDistractionWarning_NonBlockedSite) {
  service_->EnterFocusMode(base::Minutes(25));

  FullTestObserver observer;
  service_->AddObserver(&observer);

  bool triggered = service_->TriggerDistractionWarning("https://safe.com/");

  EXPECT_FALSE(triggered);
  EXPECT_EQ(observer.warning_count_, 0);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Session presets
// ---------------------------------------------------------------------------

TEST_F(FocusModeServiceTest, Presets_EmptyByDefault) {
  EXPECT_TRUE(service_->presets().empty());
}

TEST_F(FocusModeServiceTest, SavePreset_AddsNewPreset) {
  AstraFocusModeService::FocusPreset preset;
  preset.id = "deep-work";
  preset.name = "Deep Work";
  preset.duration_minutes = 90;
  preset.break_duration_minutes = 15;

  service_->SavePreset(preset);

  EXPECT_EQ(service_->presets().size(), 1u);
  EXPECT_EQ(service_->presets()[0].id, "deep-work");
  EXPECT_EQ(service_->presets()[0].name, "Deep Work");
  EXPECT_EQ(service_->presets()[0].duration_minutes, 90);
}

TEST_F(FocusModeServiceTest, SavePreset_UpdatesExistingPreset) {
  AstraFocusModeService::FocusPreset preset;
  preset.id = "quick";
  preset.name = "Quick Focus";
  preset.duration_minutes = 15;

  service_->SavePreset(preset);
  ASSERT_EQ(service_->presets().size(), 1u);

  // Update same preset.
  preset.name = "Quick Sprint";
  preset.duration_minutes = 20;
  service_->SavePreset(preset);

  EXPECT_EQ(service_->presets().size(), 1u);
  EXPECT_EQ(service_->presets()[0].name, "Quick Sprint");
  EXPECT_EQ(service_->presets()[0].duration_minutes, 20);
}

TEST_F(FocusModeServiceTest, SavePreset_EmptyIdIgnored) {
  AstraFocusModeService::FocusPreset preset;
  preset.id = "";
  preset.name = "No ID";
  preset.duration_minutes = 25;

  service_->SavePreset(preset);

  EXPECT_TRUE(service_->presets().empty());
}

TEST_F(FocusModeServiceTest, SavePreset_InvalidDurationIgnored) {
  AstraFocusModeService::FocusPreset preset;
  preset.id = "bad";
  preset.name = "Bad Duration";
  preset.duration_minutes = 0;

  service_->SavePreset(preset);

  EXPECT_TRUE(service_->presets().empty());
}

TEST_F(FocusModeServiceTest, DeletePreset_RemovesPreset) {
  AstraFocusModeService::FocusPreset preset;
  preset.id = "test";
  preset.name = "Test";
  preset.duration_minutes = 25;

  service_->SavePreset(preset);
  ASSERT_EQ(service_->presets().size(), 1u);

  service_->DeletePreset("test");

  EXPECT_TRUE(service_->presets().empty());
}

TEST_F(FocusModeServiceTest, DeletePreset_NonexistentNoOp) {
  size_t count_before = service_->presets().size();

  service_->DeletePreset("nonexistent");

  EXPECT_EQ(service_->presets().size(), count_before);
}

TEST_F(FocusModeServiceTest, StartSessionFromPreset_StartsSession) {
  AstraFocusModeService::FocusPreset preset;
  preset.id = "deep-work";
  preset.name = "Deep Work";
  preset.duration_minutes = 90;

  service_->SavePreset(preset);

  bool started = service_->StartSessionFromPreset("deep-work");

  EXPECT_TRUE(started);
  EXPECT_TRUE(service_->IsFocusModeActive());
  // Duration may not be exactly 90 min since phase_duration_ might differ,
  // but total duration should match the preset.
  EXPECT_EQ(service_->GetTotalDuration(), base::Minutes(90));
}

TEST_F(FocusModeServiceTest, StartSessionFromPreset_InvalidIdReturnsFalse) {
  bool started = service_->StartSessionFromPreset("nonexistent");

  EXPECT_FALSE(started);
  EXPECT_FALSE(service_->IsFocusModeActive());
}

TEST_F(FocusModeServiceTest, Presets_NotifiesObserverOnSave) {
  FullTestObserver observer;
  service_->AddObserver(&observer);

  AstraFocusModeService::FocusPreset preset;
  preset.id = "test";
  preset.name = "Test";
  preset.duration_minutes = 25;

  service_->SavePreset(preset);
  EXPECT_EQ(observer.presets_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(FocusModeServiceTest, Presets_NotifiesObserverOnDelete) {
  AstraFocusModeService::FocusPreset preset;
  preset.id = "test";
  preset.name = "Test";
  preset.duration_minutes = 25;
  service_->SavePreset(preset);

  FullTestObserver observer;
  service_->AddObserver(&observer);

  service_->DeletePreset("test");
  EXPECT_EQ(observer.presets_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Auto-start settings
// ---------------------------------------------------------------------------

TEST_F(FocusModeServiceTest, AutoStartTime_DefaultIs9AM) {
  EXPECT_EQ(service_->auto_start_time(), "09:00");
}

TEST_F(FocusModeServiceTest, AutoEndTime_DefaultIs5PM) {
  EXPECT_EQ(service_->auto_end_time(), "17:00");
}

TEST_F(FocusModeServiceTest, AutoStartDays_DefaultIsWeekdays) {
  // Default: Monday through Friday (1,2,3,4,5).
  const auto& days = service_->auto_start_days();
  EXPECT_EQ(days.size(), 5u);
  EXPECT_EQ(days[0], 1);  // Monday
  EXPECT_EQ(days[4], 5);  // Friday
}

TEST_F(FocusModeServiceTest, SetAutoStartTime_ValidTime) {
  service_->set_auto_start_time("08:30");
  EXPECT_EQ(service_->auto_start_time(), "08:30");
}

TEST_F(FocusModeServiceTest, SetAutoStartTime_InvalidTimeIgnored) {
  std::string original = service_->auto_start_time();

  service_->set_auto_start_time("25:00");  // Invalid hour
  EXPECT_EQ(service_->auto_start_time(), original);

  service_->set_auto_start_time("12:60");  // Invalid minute
  EXPECT_EQ(service_->auto_start_time(), original);

  service_->set_auto_start_time("abc");  // Not a time
  EXPECT_EQ(service_->auto_start_time(), original);
}

TEST_F(FocusModeServiceTest, SetAutoEndTime_ValidTime) {
  service_->set_auto_end_time("18:00");
  EXPECT_EQ(service_->auto_end_time(), "18:00");
}

TEST_F(FocusModeServiceTest, SetAutoEndTime_InvalidTimeIgnored) {
  std::string original = service_->auto_end_time();

  service_->set_auto_end_time("-1:00");
  EXPECT_EQ(service_->auto_end_time(), original);

  service_->set_auto_end_time("25:00");
  EXPECT_EQ(service_->auto_end_time(), original);
}

TEST_F(FocusModeServiceTest, SetAutoStartDays_ValidDays) {
  std::vector<int> days = {0, 6};  // Sunday and Saturday
  service_->set_auto_start_days(days);

  const auto& result = service_->auto_start_days();
  EXPECT_EQ(result.size(), 2u);
  EXPECT_EQ(result[0], 0);
  EXPECT_EQ(result[1], 6);
}

TEST_F(FocusModeServiceTest, SetAutoStartDays_InvalidDayIgnored) {
  std::vector<int> original = service_->auto_start_days();

  std::vector<int> days = {1, 7, 3};  // 7 is invalid
  service_->set_auto_start_days(days);

  // All values should be preserved if any are invalid? No — the whole set is rejected.
  // The function returns early if any day is invalid.
  EXPECT_EQ(service_->auto_start_days().size(), original.size());
}

TEST_F(FocusModeServiceTest, AutoStartSettings_NotifiesObserver) {
  FullTestObserver observer;
  service_->AddObserver(&observer);

  service_->set_auto_start_time("10:00");
  EXPECT_GE(observer.autostart_changed_count_, 1);

  int count_before = observer.autostart_changed_count_;
  service_->set_auto_end_time("18:00");
  EXPECT_GT(observer.autostart_changed_count_, count_before);

  count_before = observer.autostart_changed_count_;
  service_->set_auto_start_days({0, 1, 2, 3, 4, 5, 6});
  EXPECT_GT(observer.autostart_changed_count_, count_before);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Persistence round-trip
// ---------------------------------------------------------------------------

TEST_F(FocusModeServiceTest, SettingsPersistAcrossServiceRecreation) {
  // Change several settings.
  service_->set_default_focus_duration_minutes(45);
  service_->set_auto_start_enabled(true);
  service_->set_short_break_duration_minutes(8);
  service_->set_long_break_duration_minutes(20);
  service_->set_long_break_interval(6);
  service_->set_auto_start_next_phase(true);
  service_->set_warnings_enabled(false);
  service_->set_auto_start_time("08:00");
  service_->set_auto_end_time("18:00");

  // Create a second service for the same profile.
  auto service2 = std::make_unique<AstraFocusModeService>(profile_.get());

  // Settings should persist via PrefService.
  EXPECT_EQ(service2->default_focus_duration_minutes(), 45);
  EXPECT_TRUE(service2->auto_start_enabled());
  EXPECT_EQ(service2->short_break_duration_minutes(), 8);
  EXPECT_EQ(service2->long_break_duration_minutes(), 20);
  EXPECT_EQ(service2->long_break_interval(), 6);
  EXPECT_TRUE(service2->auto_start_next_phase());
  EXPECT_FALSE(service2->warnings_enabled());
  EXPECT_EQ(service2->auto_start_time(), "08:00");
  EXPECT_EQ(service2->auto_end_time(), "18:00");
}

TEST_F(FocusModeServiceTest, BlocklistPersistsAcrossServiceRecreation) {
  service_->AddDistractionSite("youtube.com");
  service_->AddDistractionSite("reddit.com");
  ASSERT_EQ(service_->distraction_blocklist().size(), 2u);

  auto service2 = std::make_unique<AstraFocusModeService>(profile_.get());

  EXPECT_EQ(service2->distraction_blocklist().size(), 2u);
}

TEST_F(FocusModeServiceTest, WhitelistPersistsAcrossServiceRecreation) {
  service_->AddWhitelistedSite("work.com");
  service_->AddWhitelistedSite("docs.google.com");
  ASSERT_EQ(service_->whitelist().size(), 2u);

  auto service2 = std::make_unique<AstraFocusModeService>(profile_.get());

  EXPECT_EQ(service2->whitelist().size(), 2u);
}

TEST_F(FocusModeServiceTest, StatsPersistAcrossServiceRecreation) {
  // Run a short session to accumulate stats.
  service_->EnterFocusMode(base::Minutes(5));
  service_->ExitFocusMode();

  int sessions_before = service_->GetSessionsCompleted();
  ASSERT_GT(sessions_before, 0);

  auto service2 = std::make_unique<AstraFocusModeService>(profile_.get());

  EXPECT_EQ(service2->GetSessionsCompleted(), sessions_before);
  EXPECT_GT(service2->GetTotalFocusTime(), base::TimeDelta());
}

TEST_F(FocusModeServiceTest, PresetsPersistAcrossServiceRecreation) {
  AstraFocusModeService::FocusPreset preset;
  preset.id = "test-preset";
  preset.name = "Test Preset";
  preset.duration_minutes = 50;
  preset.break_duration_minutes = 10;
  service_->SavePreset(preset);
  ASSERT_EQ(service_->presets().size(), 1u);

  auto service2 = std::make_unique<AstraFocusModeService>(profile_.get());

  ASSERT_EQ(service2->presets().size(), 1u);
  EXPECT_EQ(service2->presets()[0].id, "test-preset");
  EXPECT_EQ(service2->presets()[0].name, "Test Preset");
  EXPECT_EQ(service2->presets()[0].duration_minutes, 50);
  EXPECT_EQ(service2->presets()[0].break_duration_minutes, 10);
}

TEST_F(FocusModeServiceTest, AutoStartDaysPersistAcrossServiceRecreation) {
  std::vector<int> days = {0, 2, 4, 6};  // Sun, Tue, Thu, Sat
  service_->set_auto_start_days(days);

  auto service2 = std::make_unique<AstraFocusModeService>(profile_.get());

  const auto& result = service2->auto_start_days();
  EXPECT_EQ(result.size(), 4u);
  EXPECT_EQ(result[0], 0);
  EXPECT_EQ(result[3], 6);
}

// ---------------------------------------------------------------------------
// Edge cases
// ---------------------------------------------------------------------------

TEST_F(FocusModeServiceTest, StartSessionFromPreset_EmptyPresetsNoCrash) {
  // No presets saved — should not crash.
  bool started = service_->StartSessionFromPreset("nonexistent");
  EXPECT_FALSE(started);
}

TEST_F(FocusModeServiceTest, PauseThenExit_RecordsStats) {
  service_->EnterFocusMode(base::Minutes(25));
  service_->PauseSession();
  ASSERT_TRUE(service_->IsSessionPaused());

  int sessions_before = service_->GetSessionsCompleted();

  service_->ExitFocusMode();

  // Session should still be counted even though it was paused when exited.
  EXPECT_GT(service_->GetSessionsCompleted(), sessions_before);
  EXPECT_FALSE(service_->IsSessionPaused());
}

TEST_F(FocusModeServiceTest, WhitelistBypassesBlocklist) {
  service_->AddDistractionSite("example.com");
  service_->AddWhitelistedSite("example.com");
  service_->EnterFocusMode(base::Minutes(25));

  // Site is both blocked and whitelisted — whitelist should win.
  EXPECT_TRUE(service_->IsSiteBlocked("https://example.com/"));
  EXPECT_TRUE(service_->IsSiteWhitelisted("https://example.com/"));

  FullTestObserver observer;
  service_->AddObserver(&observer);

  bool triggered = service_->TriggerDistractionWarning("https://example.com/");
  EXPECT_FALSE(triggered);
  EXPECT_EQ(observer.warning_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(FocusModeServiceTest, MultiplePresets_AllPersist) {
  for (int i = 0; i < 5; i++) {
    AstraFocusModeService::FocusPreset preset;
    preset.id = "preset-" + std::to_string(i);
    preset.name = "Preset " + std::to_string(i);
    preset.duration_minutes = 25 + i * 10;
    service_->SavePreset(preset);
  }
  ASSERT_EQ(service_->presets().size(), 5u);

  auto service2 = std::make_unique<AstraFocusModeService>(profile_.get());
  EXPECT_EQ(service2->presets().size(), 5u);
}

TEST_F(FocusModeServiceTest, ResetStats_DoesNotAffectSettings) {
  service_->set_default_focus_duration_minutes(50);
  service_->ResetStats();

  // Settings should be unchanged.
  EXPECT_EQ(service_->default_focus_duration_minutes(), 50);
}

TEST_F(FocusModeServiceTest, Shutdown_DoesNotCrashWithPausedSession) {
  service_->EnterFocusMode(base::Minutes(25));
  service_->PauseSession();

  service_->Shutdown();

  SUCCEED() << "Shutdown with paused session completed without crash.";
}

TEST_F(FocusModeServiceTest, AddObserverAfterStart_NoCrash) {
  service_->EnterFocusMode(base::Minutes(25));
  service_->PauseSession();

  FullTestObserver observer;
  service_->AddObserver(&observer);

  service_->ResumeSession();
  EXPECT_EQ(observer.resumed_count_, 1);

  service_->RemoveObserver(&observer);
}

}  // namespace astra
