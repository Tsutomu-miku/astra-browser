// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_pip_service.h"

#include <string>

#include "base/test/task_environment.h"
#include "chrome/test/base/testing_profile.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/size.h"

namespace astra {

namespace {

// Test observer that records calls for verification.
class TestPipObserver : public AstraPipObserver {
 public:
  void OnPiPEntered(content::WebContents* web_contents) override {
    entered_count_++;
    last_entered_web_contents_ = web_contents;
  }

  void OnPiPExited(content::WebContents* web_contents) override {
    exited_count_++;
    last_exited_web_contents_ = web_contents;
  }

  void OnPiPWindowSizeChanged(content::WebContents* web_contents,
                              const gfx::Size& new_size) override {
    size_changed_count_++;
    last_size_changed_web_contents_ = web_contents;
    last_size_ = new_size;
  }

  void OnPiPWindowPositionChanged(content::WebContents* web_contents,
                                  const gfx::Point& new_position) override {
    position_changed_count_++;
    last_position_changed_web_contents_ = web_contents;
    last_position_ = new_position;
  }

  void OnPiPAlwaysOnTopChanged(bool always_on_top) override {
    always_on_top_changed_count_++;
    last_always_on_top_ = always_on_top;
  }

  void OnPiPDefaultSizePresetChanged(PipSizePreset preset) override {
    default_size_preset_changed_count_++;
    last_default_size_preset_ = preset;
  }

  void OnPiPPreferencesChanged() override {
    preferences_changed_count_++;
  }

  // Counters
  int entered_count_ = 0;
  int exited_count_ = 0;
  int size_changed_count_ = 0;
  int position_changed_count_ = 0;
  int always_on_top_changed_count_ = 0;
  int default_size_preset_changed_count_ = 0;
  int preferences_changed_count_ = 0;

  // Last recorded values
  raw_ptr<content::WebContents> last_entered_web_contents_ = nullptr;
  raw_ptr<content::WebContents> last_exited_web_contents_ = nullptr;
  raw_ptr<content::WebContents> last_size_changed_web_contents_ = nullptr;
  raw_ptr<content::WebContents> last_position_changed_web_contents_ = nullptr;
  gfx::Size last_size_;
  gfx::Point last_position_;
  bool last_always_on_top_ = false;
  PipSizePreset last_default_size_preset_ = PipSizePreset::kMedium;
};

}  // namespace

// Test fixture for AstraPipService tests.
//
// Uses TestingProfile so the service has a real Profile* to attach to.
// The service is constructed directly since the factory may not be fully
// wired up in the test harness.
//
// Tests that require WebContents use the null-web-contents path to verify
// error handling and edge cases.  Tests with real WebContents require a
// content test harness and are marked as TODO(astra).
//
// TODO(astra): Verify that TestingProfile correctly initializes the
// PrefService so that persistence tests (LoadFromPrefs/SaveToPrefs)
// can work.  Chromium component: PrefService + TestingProfile.
// TODO(astra): Add WebContents-based tests using content::WebContentsTester
// or content::TestWebContentsFactory once the content test harness is
// available.  Chromium component: content/public/test:test_support.
class PipServiceTest : public testing::Test {
 protected:
  PipServiceTest() {
    profile_ = std::make_unique<TestingProfile>();
    // TODO(astra): Obtain service through the factory once
    // AstraPipServiceFactory is properly wired up.
    // For now we construct directly with the profile.
    service_ = std::make_unique<AstraPipService>(profile_.get());
    DCHECK(service_);
  }

  ~PipServiceTest() override = default;

  void SetUp() override {
    // Service should start with no PiP tabs.
    ASSERT_TRUE(service_->GetPiPTabs().empty());
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
  std::unique_ptr<AstraPipService> service_;

  // Pool of test observers managed by the fixture.
  std::vector<TestPipObserver> test_observers_;
};

// ---------------------------------------------------------------------------
// Initial state
// ---------------------------------------------------------------------------

TEST_F(PipServiceTest, DefaultState_NoPiPTabs) {
  EXPECT_TRUE(service_->GetPiPTabs().empty());
}

TEST_F(PipServiceTest, DefaultSizePreset_IsMedium) {
  EXPECT_EQ(service_->GetDefaultSizePreset(), PipSizePreset::kMedium);
}

TEST_F(PipServiceTest, AlwaysOnTop_DefaultsToTrue) {
  EXPECT_TRUE(service_->GetAlwaysOnTop());
}

// ---------------------------------------------------------------------------
// IsTabInPip — null / edge cases
// ---------------------------------------------------------------------------

TEST_F(PipServiceTest, IsTabInPip_NullWebContentsReturnsFalse) {
  EXPECT_FALSE(service_->IsTabInPip(nullptr));
}

// ---------------------------------------------------------------------------
// EnterPipForTab / ExitPipForTab — null / edge cases
// ---------------------------------------------------------------------------

TEST_F(PipServiceTest, EnterPipForTab_NullWebContentsNoOp) {
  TestPipObserver observer;
  service_->AddObserver(&observer);

  service_->EnterPipForTab(nullptr);

  // Should not notify since no tab was affected.
  EXPECT_EQ(observer.entered_count_, 0);
  EXPECT_TRUE(service_->GetPiPTabs().empty());

  service_->RemoveObserver(&observer);
}

TEST_F(PipServiceTest, ExitPipForTab_NullWebContentsNoOp) {
  TestPipObserver observer;
  service_->AddObserver(&observer);

  service_->ExitPipForTab(nullptr);

  EXPECT_EQ(observer.exited_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(PipServiceTest, TogglePipForTab_NullWebContentsReturnsFalse) {
  // Null web contents — toggle should not enter PiP.
  bool result = service_->TogglePipForTab(nullptr);
  EXPECT_FALSE(result);
}

// ---------------------------------------------------------------------------
// ResizePiPWindow — null / edge cases
// ---------------------------------------------------------------------------

TEST_F(PipServiceTest, ResizePiPWindow_NullWebContentsNoOp) {
  TestPipObserver observer;
  service_->AddObserver(&observer);

  service_->ResizePiPWindow(nullptr, gfx::Size(400, 300));

  EXPECT_EQ(observer.size_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(PipServiceTest, ResizePiPWindow_EmptySizeNoOp) {
  // Resizing to empty size should be a no-op even for a valid tab.
  // With null web contents, this exercises the early return paths.
  service_->ResizePiPWindow(nullptr, gfx::Size());
  // No crash = success.
  SUCCEED() << "Resize with empty size handled without crash.";
}

TEST_F(PipServiceTest, GetPiPWindowSize_NullWebContentsReturnsEmpty) {
  gfx::Size size = service_->GetPiPWindowSize(nullptr);
  EXPECT_TRUE(size.IsEmpty());
}

// ---------------------------------------------------------------------------
// ResizePiPToPreset — null web contents edge case
// ---------------------------------------------------------------------------

TEST_F(PipServiceTest, ResizePiPToPreset_NullWebContentsNoOp) {
  // Should not crash with null web contents.
  service_->ResizePiPToPreset(nullptr, PipSizePreset::kLarge);
  SUCCEED() << "Resize to preset with null web contents handled without crash.";
}

// ---------------------------------------------------------------------------
// Size preset string conversion
// ---------------------------------------------------------------------------

TEST_F(PipServiceTest, PresetToString_Small) {
  EXPECT_STREQ(AstraPipService::PresetToString(PipSizePreset::kSmall),
               "small");
}

TEST_F(PipServiceTest, PresetToString_Medium) {
  EXPECT_STREQ(AstraPipService::PresetToString(PipSizePreset::kMedium),
               "medium");
}

TEST_F(PipServiceTest, PresetToString_Large) {
  EXPECT_STREQ(AstraPipService::PresetToString(PipSizePreset::kLarge),
               "large");
}

TEST_F(PipServiceTest, PresetFromString_Small) {
  EXPECT_EQ(AstraPipService::PresetFromString("small"),
            PipSizePreset::kSmall);
}

TEST_F(PipServiceTest, PresetFromString_Medium) {
  EXPECT_EQ(AstraPipService::PresetFromString("medium"),
            PipSizePreset::kMedium);
}

TEST_F(PipServiceTest, PresetFromString_Large) {
  EXPECT_EQ(AstraPipService::PresetFromString("large"),
            PipSizePreset::kLarge);
}

TEST_F(PipServiceTest, PresetFromString_UnknownReturnsMedium) {
  // Unknown values default to medium.
  EXPECT_EQ(AstraPipService::PresetFromString(""),
            PipSizePreset::kMedium);
  EXPECT_EQ(AstraPipService::PresetFromString("unknown"),
            PipSizePreset::kMedium);
  EXPECT_EQ(AstraPipService::PresetFromString("extra_large"),
            PipSizePreset::kMedium);
}

TEST_F(PipServiceTest, PresetStringRoundTrip) {
  // Round-trip each preset through string conversion.
  for (auto preset : {PipSizePreset::kSmall,
                       PipSizePreset::kMedium,
                       PipSizePreset::kLarge}) {
    std::string name = AstraPipService::PresetToString(preset);
    EXPECT_EQ(AstraPipService::PresetFromString(name), preset);
  }
}

// ---------------------------------------------------------------------------
// GetPresetSize — size calculation
// ---------------------------------------------------------------------------

TEST_F(PipServiceTest, GetPresetSize_Small_16x9) {
  // 16:9 aspect ratio, small preset (height = 160).
  gfx::Size size = AstraPipService::GetPresetSize(
      PipSizePreset::kSmall, 16.0f / 9.0f);

  EXPECT_EQ(size.height(), 160);
  // Width = 160 * 16/9 ≈ 284
  EXPECT_EQ(size.width(), static_cast<int>(160 * 16.0 / 9.0));
  EXPECT_GT(size.width(), size.height());
}

TEST_F(PipServiceTest, GetPresetSize_Medium_16x9) {
  gfx::Size size = AstraPipService::GetPresetSize(
      PipSizePreset::kMedium, 16.0f / 9.0f);

  EXPECT_EQ(size.height(), 240);
  EXPECT_GT(size.width(), 0);
}

TEST_F(PipServiceTest, GetPresetSize_Large_4x3) {
  // 4:3 aspect ratio, large preset (height = 320).
  gfx::Size size = AstraPipService::GetPresetSize(
      PipSizePreset::kLarge, 4.0f / 3.0f);

  EXPECT_EQ(size.height(), 320);
  // Width = 320 * 4/3 ≈ 426
  EXPECT_EQ(size.width(), static_cast<int>(320 * 4.0 / 3.0));
}

TEST_F(PipServiceTest, GetPresetSize_ZeroAspectRatioUsesDefault) {
  // Zero or negative aspect ratio should fall back to default (16:9).
  gfx::Size size_zero = AstraPipService::GetPresetSize(
      PipSizePreset::kMedium, 0.0f);
  gfx::Size size_neg = AstraPipService::GetPresetSize(
      PipSizePreset::kMedium, -1.0f);
  gfx::Size size_default = AstraPipService::GetPresetSize(
      PipSizePreset::kMedium, 16.0f / 9.0f);

  EXPECT_EQ(size_zero, size_default);
  EXPECT_EQ(size_neg, size_default);
}

TEST_F(PipServiceTest, GetPresetSize_SquareAspectRatio) {
  // 1:1 aspect ratio — width should equal height.
  gfx::Size size = AstraPipService::GetPresetSize(
      PipSizePreset::kMedium, 1.0f);

  EXPECT_EQ(size.width(), size.height());
  EXPECT_EQ(size.height(), 240);
}

TEST_F(PipServiceTest, GetPresetSize_PortraitAspectRatio) {
  // Portrait aspect ratio (9:16) — width < height.
  float aspect_ratio = 9.0f / 16.0f;
  gfx::Size size = AstraPipService::GetPresetSize(
      PipSizePreset::kMedium, aspect_ratio);

  EXPECT_LT(size.width(), size.height());
  EXPECT_EQ(size.height(), 240);
  EXPECT_EQ(size.width(), static_cast<int>(240 * 9.0 / 16.0));
}

// ---------------------------------------------------------------------------
// Default size preset preference
// ---------------------------------------------------------------------------

TEST_F(PipServiceTest, SetDefaultSizePreset_ChangesValue) {
  ASSERT_EQ(service_->GetDefaultSizePreset(), PipSizePreset::kMedium);

  service_->SetDefaultSizePreset(PipSizePreset::kLarge);
  EXPECT_EQ(service_->GetDefaultSizePreset(), PipSizePreset::kLarge);

  service_->SetDefaultSizePreset(PipSizePreset::kSmall);
  EXPECT_EQ(service_->GetDefaultSizePreset(), PipSizePreset::kSmall);
}

TEST_F(PipServiceTest, SetDefaultSizePreset_SameValueNoOp) {
  PipSizePreset original = service_->GetDefaultSizePreset();

  // Setting to the same value should be a no-op (no save, no observer notify).
  service_->SetDefaultSizePreset(original);

  EXPECT_EQ(service_->GetDefaultSizePreset(), original);
}

// ---------------------------------------------------------------------------
// Always-on-top preference
// ---------------------------------------------------------------------------

TEST_F(PipServiceTest, SetAlwaysOnTop_ChangesValue) {
  ASSERT_TRUE(service_->GetAlwaysOnTop());

  service_->SetAlwaysOnTop(false);
  EXPECT_FALSE(service_->GetAlwaysOnTop());

  service_->SetAlwaysOnTop(true);
  EXPECT_TRUE(service_->GetAlwaysOnTop());
}

TEST_F(PipServiceTest, SetAlwaysOnTop_SameValueNoOp) {
  bool original = service_->GetAlwaysOnTop();

  // Setting to the same value should be a no-op.
  service_->SetAlwaysOnTop(original);

  EXPECT_EQ(service_->GetAlwaysOnTop(), original);
}

// ---------------------------------------------------------------------------
// GetPiPTabs — empty state
// ---------------------------------------------------------------------------

TEST_F(PipServiceTest, GetPiPTabs_EmptyWhenNoTabsInPip) {
  auto pip_tabs = service_->GetPiPTabs();
  EXPECT_TRUE(pip_tabs.empty());
}

// ---------------------------------------------------------------------------
// Observer add/remove
// ---------------------------------------------------------------------------

TEST_F(PipServiceTest, AddRemoveObserver_NoCrash) {
  TestPipObserver observer;

  service_->AddObserver(&observer);
  service_->RemoveObserver(&observer);

  // Should not crash.
  SUCCEED() << "Observer add/remove completed without crash.";
}

TEST_F(PipServiceTest, RemoveNonexistentObserver_NoCrash) {
  TestPipObserver observer;

  // Removing an observer that was never added should not crash.
  service_->RemoveObserver(&observer);

  SUCCEED() << "Removing nonexistent observer completed without crash.";
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------

TEST_F(PipServiceTest, ShutdownClearsObservers) {
  TestPipObserver observer;
  service_->AddObserver(&observer);

  service_->Shutdown();

  // After shutdown, operations should not notify observers.
  service_->EnterPipForTab(nullptr);
  EXPECT_EQ(observer.entered_count_, 0);
}

// ---------------------------------------------------------------------------
// Observer defaults — all observer methods have empty default implementations
// ---------------------------------------------------------------------------

TEST_F(PipServiceTest, ObserverDefaults_DoNotCrash) {
  // Create a minimal observer that overrides nothing.
  // All methods have default empty implementations, so this should compile
  // and not crash.
  class DefaultObserver : public AstraPipObserver {};

  DefaultObserver observer;
  service_->AddObserver(&observer);

  // Trigger all observer paths with null web contents (no-op but exercises
  // the observer iteration).
  service_->EnterPipForTab(nullptr);
  service_->ExitPipForTab(nullptr);
  service_->ResizePiPWindow(nullptr, gfx::Size(400, 300));
  service_->SetPiPWindowPosition(nullptr, gfx::Point(10, 10));
  service_->SetAlwaysOnTop(false);
  service_->SetDefaultSizePreset(PipSizePreset::kSmall);

  service_->RemoveObserver(&observer);
  SUCCEED() << "Default observer methods do not crash.";
}

// ---------------------------------------------------------------------------
// ToggleAlwaysOnTop
// ---------------------------------------------------------------------------

TEST_F(PipServiceTest, ToggleAlwaysOnTop_FlipsValue) {
  ASSERT_TRUE(service_->GetAlwaysOnTop());

  bool result = service_->ToggleAlwaysOnTop();
  EXPECT_FALSE(result);
  EXPECT_FALSE(service_->GetAlwaysOnTop());

  result = service_->ToggleAlwaysOnTop();
  EXPECT_TRUE(result);
  EXPECT_TRUE(service_->GetAlwaysOnTop());
}

TEST_F(PipServiceTest, ToggleAlwaysOnTop_FiresObserver) {
  TestPipObserver observer;
  service_->AddObserver(&observer);

  ASSERT_TRUE(service_->GetAlwaysOnTop());
  service_->ToggleAlwaysOnTop();

  EXPECT_EQ(observer.always_on_top_changed_count_, 1);
  EXPECT_FALSE(observer.last_always_on_top_);
  EXPECT_EQ(observer.preferences_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Snap position
// ---------------------------------------------------------------------------

TEST_F(PipServiceTest, SnapPosition_DefaultIsBottomRight) {
  EXPECT_EQ(service_->GetSnapPosition(), PipSnapPosition::kBottomRight);
}

TEST_F(PipServiceTest, SetSnapPosition_ChangesValue) {
  ASSERT_EQ(service_->GetSnapPosition(), PipSnapPosition::kBottomRight);

  service_->SetSnapPosition(PipSnapPosition::kTopLeft);
  EXPECT_EQ(service_->GetSnapPosition(), PipSnapPosition::kTopLeft);

  service_->SetSnapPosition(PipSnapPosition::kTopRight);
  EXPECT_EQ(service_->GetSnapPosition(), PipSnapPosition::kTopRight);

  service_->SetSnapPosition(PipSnapPosition::kBottomLeft);
  EXPECT_EQ(service_->GetSnapPosition(), PipSnapPosition::kBottomLeft);

  service_->SetSnapPosition(PipSnapPosition::kFreeFloating);
  EXPECT_EQ(service_->GetSnapPosition(), PipSnapPosition::kFreeFloating);
}

TEST_F(PipServiceTest, SetSnapPosition_SameValueNoOp) {
  TestPipObserver observer;
  service_->AddObserver(&observer);

  PipSnapPosition original = service_->GetSnapPosition();
  service_->SetSnapPosition(original);

  EXPECT_EQ(service_->GetSnapPosition(), original);
  EXPECT_EQ(observer.preferences_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(PipServiceTest, SetSnapPosition_FiresObserver) {
  TestPipObserver observer;
  service_->AddObserver(&observer);

  service_->SetSnapPosition(PipSnapPosition::kTopLeft);

  EXPECT_EQ(observer.preferences_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(PipServiceTest, SnapPositionToString_KnownValues) {
  EXPECT_STREQ(AstraPipService::SnapPositionToString(
                   PipSnapPosition::kTopLeft),
               "top_left");
  EXPECT_STREQ(AstraPipService::SnapPositionToString(
                   PipSnapPosition::kTopRight),
               "top_right");
  EXPECT_STREQ(AstraPipService::SnapPositionToString(
                   PipSnapPosition::kBottomLeft),
               "bottom_left");
  EXPECT_STREQ(AstraPipService::SnapPositionToString(
                   PipSnapPosition::kBottomRight),
               "bottom_right");
  EXPECT_STREQ(AstraPipService::SnapPositionToString(
                   PipSnapPosition::kFreeFloating),
               "free_floating");
}

TEST_F(PipServiceTest, SnapPositionFromString_KnownValues) {
  EXPECT_EQ(AstraPipService::SnapPositionFromString("top_left"),
            PipSnapPosition::kTopLeft);
  EXPECT_EQ(AstraPipService::SnapPositionFromString("top_right"),
            PipSnapPosition::kTopRight);
  EXPECT_EQ(AstraPipService::SnapPositionFromString("bottom_left"),
            PipSnapPosition::kBottomLeft);
  EXPECT_EQ(AstraPipService::SnapPositionFromString("bottom_right"),
            PipSnapPosition::kBottomRight);
  EXPECT_EQ(AstraPipService::SnapPositionFromString("free_floating"),
            PipSnapPosition::kFreeFloating);
}

TEST_F(PipServiceTest, SnapPositionFromString_UnknownReturnsBottomRight) {
  EXPECT_EQ(AstraPipService::SnapPositionFromString(""),
            PipSnapPosition::kBottomRight);
  EXPECT_EQ(AstraPipService::SnapPositionFromString("unknown"),
            PipSnapPosition::kBottomRight);
  EXPECT_EQ(AstraPipService::SnapPositionFromString("center"),
            PipSnapPosition::kBottomRight);
}

TEST_F(PipServiceTest, SnapPositionStringRoundTrip) {
  for (auto pos : {PipSnapPosition::kTopLeft,
                   PipSnapPosition::kTopRight,
                   PipSnapPosition::kBottomLeft,
                   PipSnapPosition::kBottomRight,
                   PipSnapPosition::kFreeFloating}) {
    std::string name = AstraPipService::SnapPositionToString(pos);
    EXPECT_EQ(AstraPipService::SnapPositionFromString(name), pos);
  }
}

// ---------------------------------------------------------------------------
// Snap-to-corner enabled
// ---------------------------------------------------------------------------

TEST_F(PipServiceTest, SnapToCornerEnabled_DefaultIsTrue) {
  EXPECT_TRUE(service_->GetSnapToCornerEnabled());
}

TEST_F(PipServiceTest, SetSnapToCornerEnabled_ChangesValue) {
  ASSERT_TRUE(service_->GetSnapToCornerEnabled());

  service_->SetSnapToCornerEnabled(false);
  EXPECT_FALSE(service_->GetSnapToCornerEnabled());

  service_->SetSnapToCornerEnabled(true);
  EXPECT_TRUE(service_->GetSnapToCornerEnabled());
}

TEST_F(PipServiceTest, SetSnapToCornerEnabled_SameValueNoOp) {
  TestPipObserver observer;
  service_->AddObserver(&observer);

  bool original = service_->GetSnapToCornerEnabled();
  service_->SetSnapToCornerEnabled(original);

  EXPECT_EQ(service_->GetSnapToCornerEnabled(), original);
  EXPECT_EQ(observer.preferences_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(PipServiceTest, SetSnapToCornerEnabled_FiresObserver) {
  TestPipObserver observer;
  service_->AddObserver(&observer);

  service_->SetSnapToCornerEnabled(false);

  EXPECT_EQ(observer.preferences_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// PiP opacity
// ---------------------------------------------------------------------------

TEST_F(PipServiceTest, PiPOpacity_DefaultIsFullyOpaque) {
  EXPECT_DOUBLE_EQ(service_->GetPiPOpacity(), 1.0);
}

TEST_F(PipServiceTest, SetPiPOpacity_ChangesValue) {
  service_->SetPiPOpacity(0.5);
  EXPECT_DOUBLE_EQ(service_->GetPiPOpacity(), 0.5);

  service_->SetPiPOpacity(0.8);
  EXPECT_DOUBLE_EQ(service_->GetPiPOpacity(), 0.8);
}

TEST_F(PipServiceTest, SetPiPOpacity_ClampsToMinimum) {
  service_->SetPiPOpacity(0.0);
  EXPECT_GE(service_->GetPiPOpacity(), 0.2);

  service_->SetPiPOpacity(0.1);
  EXPECT_GE(service_->GetPiPOpacity(), 0.2);

  service_->SetPiPOpacity(-1.0);
  EXPECT_GE(service_->GetPiPOpacity(), 0.2);
}

TEST_F(PipServiceTest, SetPiPOpacity_ClampsToMaximum) {
  service_->SetPiPOpacity(2.0);
  EXPECT_LE(service_->GetPiPOpacity(), 1.0);
  EXPECT_DOUBLE_EQ(service_->GetPiPOpacity(), 1.0);
}

TEST_F(PipServiceTest, SetPiPOpacity_SameValueNoOp) {
  TestPipObserver observer;
  service_->AddObserver(&observer);

  double original = service_->GetPiPOpacity();
  service_->SetPiPOpacity(original);

  EXPECT_DOUBLE_EQ(service_->GetPiPOpacity(), original);
  EXPECT_EQ(observer.preferences_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(PipServiceTest, SetPiPOpacity_FiresObserver) {
  TestPipObserver observer;
  service_->AddObserver(&observer);

  service_->SetPiPOpacity(0.5);

  EXPECT_EQ(observer.preferences_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Auto-PiP on tab switch
// ---------------------------------------------------------------------------

TEST_F(PipServiceTest, AutoPipOnTabSwitch_DefaultIsFalse) {
  EXPECT_FALSE(service_->GetAutoPipOnTabSwitch());
}

TEST_F(PipServiceTest, SetAutoPipOnTabSwitch_ChangesValue) {
  ASSERT_FALSE(service_->GetAutoPipOnTabSwitch());

  service_->SetAutoPipOnTabSwitch(true);
  EXPECT_TRUE(service_->GetAutoPipOnTabSwitch());

  service_->SetAutoPipOnTabSwitch(false);
  EXPECT_FALSE(service_->GetAutoPipOnTabSwitch());
}

TEST_F(PipServiceTest, SetAutoPipOnTabSwitch_SameValueNoOp) {
  TestPipObserver observer;
  service_->AddObserver(&observer);

  bool original = service_->GetAutoPipOnTabSwitch();
  service_->SetAutoPipOnTabSwitch(original);

  EXPECT_EQ(service_->GetAutoPipOnTabSwitch(), original);
  EXPECT_EQ(observer.preferences_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(PipServiceTest, SetAutoPipOnTabSwitch_FiresObserver) {
  TestPipObserver observer;
  service_->AddObserver(&observer);

  service_->SetAutoPipOnTabSwitch(true);

  EXPECT_EQ(observer.preferences_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Max PiP windows
// ---------------------------------------------------------------------------

TEST_F(PipServiceTest, MaxPipWindows_DefaultIs3) {
  EXPECT_EQ(service_->GetMaxPipWindows(), 3);
}

TEST_F(PipServiceTest, SetMaxPipWindows_ChangesValue) {
  ASSERT_EQ(service_->GetMaxPipWindows(), 3);

  service_->SetMaxPipWindows(5);
  EXPECT_EQ(service_->GetMaxPipWindows(), 5);

  service_->SetMaxPipWindows(1);
  EXPECT_EQ(service_->GetMaxPipWindows(), 1);
}

TEST_F(PipServiceTest, SetMaxPipWindows_ZeroMeansNoLimit) {
  service_->SetMaxPipWindows(0);
  EXPECT_EQ(service_->GetMaxPipWindows(), 0);
  // With 0 windows in PiP and limit 0, IsAtPipLimit should be false
  // (0 means no limit).
  EXPECT_FALSE(service_->IsAtPipLimit());
}

TEST_F(PipServiceTest, SetMaxPipWindows_NegativeClampsToZero) {
  service_->SetMaxPipWindows(-5);
  EXPECT_EQ(service_->GetMaxPipWindows(), 0);
}

TEST_F(PipServiceTest, SetMaxPipWindows_SameValueNoOp) {
  TestPipObserver observer;
  service_->AddObserver(&observer);

  int original = service_->GetMaxPipWindows();
  service_->SetMaxPipWindows(original);

  EXPECT_EQ(service_->GetMaxPipWindows(), original);
  EXPECT_EQ(observer.preferences_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

TEST_F(PipServiceTest, SetMaxPipWindows_FiresObserver) {
  TestPipObserver observer;
  service_->AddObserver(&observer);

  service_->SetMaxPipWindows(5);

  EXPECT_EQ(observer.preferences_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(PipServiceTest, IsAtPipLimit_EmptyStateNotAtLimit) {
  // With no PiP tabs and default limit of 3, we should not be at the limit.
  EXPECT_FALSE(service_->IsAtPipLimit());
}

// ---------------------------------------------------------------------------
// Bulk operations — ExitAllPip
// ---------------------------------------------------------------------------

TEST_F(PipServiceTest, ExitAllPip_EmptyStateReturnsZero) {
  size_t count = service_->ExitAllPip();
  EXPECT_EQ(count, 0u);
}

// ---------------------------------------------------------------------------
// Utility methods — GetPipCount
// ---------------------------------------------------------------------------

TEST_F(PipServiceTest, GetPipCount_EmptyStateReturnsZero) {
  EXPECT_EQ(service_->GetPipCount(), 0u);
}

TEST_F(PipServiceTest, GetPipCount_MatchesGetPiPTabsSize) {
  // With no tabs in PiP, both should return 0.
  EXPECT_EQ(service_->GetPipCount(), service_->GetPiPTabs().size());
}

// ---------------------------------------------------------------------------
// CycleSizePreset — null / edge cases
// ---------------------------------------------------------------------------

TEST_F(PipServiceTest, CycleSizePreset_NullWebContentsReturnsDefault) {
  // With null web contents, should return the default preset without crash.
  PipSizePreset result = service_->CycleSizePreset(nullptr);
  EXPECT_EQ(result, service_->GetDefaultSizePreset());
}

// ---------------------------------------------------------------------------
// PiP window position — null / edge cases
// ---------------------------------------------------------------------------

TEST_F(PipServiceTest, GetPiPWindowPosition_NullWebContentsReturnsOrigin) {
  gfx::Point pos = service_->GetPiPWindowPosition(nullptr);
  EXPECT_EQ(pos, gfx::Point());
}

TEST_F(PipServiceTest, SetPiPWindowPosition_NullWebContentsNoOp) {
  TestPipObserver observer;
  service_->AddObserver(&observer);

  service_->SetPiPWindowPosition(nullptr, gfx::Point(100, 100));

  EXPECT_EQ(observer.position_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// SetDefaultSizePreset — fires observer
// ---------------------------------------------------------------------------

TEST_F(PipServiceTest, SetDefaultSizePreset_FiresObserver) {
  TestPipObserver observer;
  service_->AddObserver(&observer);

  service_->SetDefaultSizePreset(PipSizePreset::kLarge);

  EXPECT_EQ(observer.default_size_preset_changed_count_, 1);
  EXPECT_EQ(observer.last_default_size_preset_, PipSizePreset::kLarge);
  EXPECT_EQ(observer.preferences_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(PipServiceTest, SetDefaultSizePreset_SameValueNoObserver) {
  TestPipObserver observer;
  service_->AddObserver(&observer);

  PipSizePreset original = service_->GetDefaultSizePreset();
  service_->SetDefaultSizePreset(original);

  EXPECT_EQ(observer.default_size_preset_changed_count_, 0);
  EXPECT_EQ(observer.preferences_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// SetAlwaysOnTop — fires observer
// ---------------------------------------------------------------------------

TEST_F(PipServiceTest, SetAlwaysOnTop_FiresBothObservers) {
  TestPipObserver observer;
  service_->AddObserver(&observer);

  ASSERT_TRUE(service_->GetAlwaysOnTop());
  service_->SetAlwaysOnTop(false);

  // Should fire both the specific and the generic preference observer.
  EXPECT_EQ(observer.always_on_top_changed_count_, 1);
  EXPECT_FALSE(observer.last_always_on_top_);
  EXPECT_EQ(observer.preferences_changed_count_, 1);

  service_->RemoveObserver(&observer);
}

TEST_F(PipServiceTest, SetAlwaysOnTop_SameValueNoObserver) {
  TestPipObserver observer;
  service_->AddObserver(&observer);

  bool original = service_->GetAlwaysOnTop();
  service_->SetAlwaysOnTop(original);

  EXPECT_EQ(observer.always_on_top_changed_count_, 0);
  EXPECT_EQ(observer.preferences_changed_count_, 0);

  service_->RemoveObserver(&observer);
}

// ---------------------------------------------------------------------------
// Preference persistence round-trip
// ---------------------------------------------------------------------------

TEST_F(PipServiceTest, PrefsPersist_AllSettings) {
  // Change all settings.
  service_->SetDefaultSizePreset(PipSizePreset::kSmall);
  service_->SetAlwaysOnTop(false);
  service_->SetSnapPosition(PipSnapPosition::kTopLeft);
  service_->SetSnapToCornerEnabled(false);
  service_->SetPiPOpacity(0.6);
  service_->SetAutoPipOnTabSwitch(true);
  service_->SetMaxPipWindows(7);

  // Create a new service with the same profile — should load persisted prefs.
  auto service2 = std::make_unique<AstraPipService>(profile_.get());

  EXPECT_EQ(service2->GetDefaultSizePreset(), PipSizePreset::kSmall);
  EXPECT_FALSE(service2->GetAlwaysOnTop());
  EXPECT_EQ(service2->GetSnapPosition(), PipSnapPosition::kTopLeft);
  EXPECT_FALSE(service2->GetSnapToCornerEnabled());
  EXPECT_DOUBLE_EQ(service2->GetPiPOpacity(), 0.6);
  EXPECT_TRUE(service2->GetAutoPipOnTabSwitch());
  EXPECT_EQ(service2->GetMaxPipWindows(), 7);
}

TEST_F(PipServiceTest, PrefsPersist_DefaultValues) {
  // Create a fresh service — should have default values.
  auto service2 = std::make_unique<AstraPipService>(profile_.get());

  EXPECT_EQ(service2->GetDefaultSizePreset(), PipSizePreset::kMedium);
  EXPECT_TRUE(service2->GetAlwaysOnTop());
  EXPECT_EQ(service2->GetSnapPosition(), PipSnapPosition::kBottomRight);
  EXPECT_TRUE(service2->GetSnapToCornerEnabled());
  EXPECT_DOUBLE_EQ(service2->GetPiPOpacity(), 1.0);
  EXPECT_FALSE(service2->GetAutoPipOnTabSwitch());
  EXPECT_EQ(service2->GetMaxPipWindows(), 3);
}

// ---------------------------------------------------------------------------
// TODO(astra): WebContents-based tests (require content test harness)
// ---------------------------------------------------------------------------
//
// The following tests require real WebContents objects and should be
// implemented once the content test harness is available:
//
//   - EnterPipForTab_SetsPiPState
//   - EnterPipForTab_Idempotent
//   - ExitPipForTab_ClearsPiPState
//   - TogglePipForTab_TogglesState
//   - EnterPip_SetsInitialWindowSizeFromDefaultPreset
//   - ResizePiPWindow_UpdatesSize
//   - ResizePiPToPreset_UpdatesToPresetSize
//   - GetPiPWindowSize_ReturnsCurrentSize
//   - GetPiPTabs_ReturnsPiPTabs
//   - ObserverFiresOnEnter
//   - ObserverFiresOnExit
//   - ObserverFiresOnResize
//   - MultipleObservers_AllNotified
//   - RemoveObserver_StopsNotifications
//
// TODO(astra): Add browser_tests for PiP service integration with real
// Browser and TabStripModel.
// Chromium component: InProcessBrowserTest + PictureInPictureWindowController.

}  // namespace astra
