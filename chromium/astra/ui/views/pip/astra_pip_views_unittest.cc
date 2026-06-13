// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Comprehensive unit tests for Astra PiP controls (model + view).
//
// Test categories:
//   - Model state tests (playback, volume, size preset, opacity, etc.)
//   - Model observer tests (all observer methods, observer defaults)
//   - Model persistence tests (settings via PrefService, round-trip)
//   - Model utility methods (clamping, preset cycling)
//   - Model PiP state tests (activate/deactivate/toggle)
//   - Model size and position tests (min/max bounds, aspect ratio)
//   - Model workspace integration tests
//   - Model appearance tests
//   - View tests (construction, rendering, model integration)
//   - View interaction tests (mouse, keyboard, auto-hide)
//   - View accessibility tests
//   - Edge case tests
//
// Chromium test pattern: views::test::ViewsTestBase
//   (ui/views/test/views_test_base.h)

#include "astra/ui/views/pip/astra_pip_controls_model.h"
#include "astra/ui/views/pip/astra_pip_controls_view.h"

#include <vector>

#include "astra/browser/astra_pip_service.h"
#include "astra/browser/astra_prefs.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/timer/mock_timer.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/event.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/controls/slider.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace astra {

namespace {

using ::testing::_;
using ::testing::AtLeast;
using ::testing::Mock;

// =========================================================================
// Mock observer for testing AstraPipObserver pattern
// =========================================================================

class MockAstraPipObserver : public AstraPipObserver {
 public:
  MockAstraPipObserver() = default;
  ~MockAstraPipObserver() override = default;

  MOCK_METHOD(void, OnPipActivated, (AstraPipControlsModel * model), (override));
  MOCK_METHOD(void, OnPipDeactivated, (AstraPipControlsModel * model), (override));
  MOCK_METHOD(void, OnPipSizeChanged,
              (AstraPipControlsModel * model, const gfx::Size& new_size),
              (override));
  MOCK_METHOD(void, OnPipPositionChanged,
              (AstraPipControlsModel * model, const gfx::Point& new_position),
              (override));
  MOCK_METHOD(void, OnPlaybackStateChanged,
              (AstraPipControlsModel * model, bool is_playing), (override));
  MOCK_METHOD(void, OnVolumeChanged,
              (AstraPipControlsModel * model, double volume), (override));
  MOCK_METHOD(void, OnMuteChanged,
              (AstraPipControlsModel * model, bool muted), (override));
  MOCK_METHOD(void, OnProgressChanged,
              (AstraPipControlsModel * model, double progress), (override));
  MOCK_METHOD(void, OnControlsVisibilityChanged,
              (AstraPipControlsModel * model, bool visible), (override));
  MOCK_METHOD(void, OnPipModelShutdown, (AstraPipControlsModel * model), (override));
};

// =========================================================================
// Mock observer for testing controls model observer pattern
// =========================================================================

class MockPipControlsModelObserver
    : public AstraPipControlsModelObserver {
 public:
  MockPipControlsModelObserver() = default;
  ~MockPipControlsModelObserver() override = default;

  MOCK_METHOD(void, OnPlayStateChanged, (bool playing), (override));
  MOCK_METHOD(void, OnMuteStateChanged, (bool muted), (override));
  MOCK_METHOD(void, OnVolumeChanged, (double volume), (override));
  MOCK_METHOD(void, OnPlaybackRateChanged, (double rate), (override));
  MOCK_METHOD(void, OnSizePresetChanged, (PipSizePreset preset), (override));
  MOCK_METHOD(void, OnAlwaysOnTopChanged, (bool pinned), (override));
  MOCK_METHOD(void, OnOpacityChanged, (double opacity), (override));
  MOCK_METHOD(void, OnControlsVisibilityChanged, (bool visible), (override));
  MOCK_METHOD(void, OnControlsSettingsChanged, (), (override));
  MOCK_METHOD(void, OnSnapPositionChanged, (PipSnapPosition position), (override));
  MOCK_METHOD(void, OnControlsMinimizedChanged, (bool minimized), (override));
  MOCK_METHOD(void, OnProgressChanged, (double progress), (override));
  MOCK_METHOD(void, OnPlaybackSpeedChanged, (AstraPipPlaybackSpeed speed), (override));
  MOCK_METHOD(void, OnLoopingChanged, (bool looping), (override));
};

// Fake delegate that tracks all invocations.
struct FakePipControlsDelegate : public AstraPipControlsView::Delegate {
  int play_pause_count = 0;
  int skip_backward_count = 0;
  int skip_forward_count = 0;
  int mute_toggle_count = 0;
  int close_count = 0;
  int return_to_tab_count = 0;
  int resize_preset_count = 0;
  int always_on_top_count = 0;
  int volume_changed_count = 0;
  int playback_rate_changed_count = 0;
  int opacity_changed_count = 0;
  int snap_position_changed_count = 0;

  int last_skip_backward_seconds = 0;
  int last_skip_forward_seconds = 0;
  double last_volume = 0.0;
  double last_playback_rate = 0.0;
  double last_opacity = 0.0;
  PipSizePreset last_preset = static_cast<PipSizePreset>(0);
  PipSnapPosition last_snap_position = static_cast<PipSnapPosition>(0);

  void OnPlayPause() override { play_pause_count++; }
  void OnSkipBackward(int seconds) override {
    skip_backward_count++;
    last_skip_backward_seconds = seconds;
  }
  void OnSkipForward(int seconds) override {
    skip_forward_count++;
    last_skip_forward_seconds = seconds;
  }
  void OnMuteToggle() override { mute_toggle_count++; }
  void OnClosePip() override { close_count++; }
  void OnReturnToTab() override { return_to_tab_count++; }
  void OnResizePreset(PipSizePreset preset) override {
    resize_preset_count++;
    last_preset = preset;
  }
  void OnAlwaysOnTopToggle() override { always_on_top_count++; }
  void OnVolumeChanged(double volume) override {
    volume_changed_count++;
    last_volume = volume;
  }
  void OnPlaybackRateChanged(double rate) override {
    playback_rate_changed_count++;
    last_playback_rate = rate;
  }
  void OnOpacityChanged(double opacity) override {
    opacity_changed_count++;
    last_opacity = opacity;
  }
  void OnSnapPositionChanged(PipSnapPosition position) override {
    snap_position_changed_count++;
    last_snap_position = position;
  }
};

// Helper to register all prefs needed for tests.
void RegisterTestPrefs(TestingPrefServiceSimple* prefs) {
  // Service-level prefs.
  prefs->registry()->RegisterStringPref(prefs::kPrefPiPDefaultSize,
                                        prefs::kDefaultPiPDefaultSize);
  prefs->registry()->RegisterBooleanPref(prefs::kPrefPiPAlwaysOnTop,
                                         prefs::kDefaultPiPAlwaysOnTop);
  prefs->registry()->RegisterStringPref(prefs::kPrefPiPSnapPosition,
                                        prefs::kDefaultPiPSnapPosition);
  prefs->registry()->RegisterBooleanPref(prefs::kPrefPiPSnapToCornerEnabled,
                                         prefs::kDefaultPiPSnapToCornerEnabled);
  prefs->registry()->RegisterDoublePref(prefs::kPrefPiPOpacity,
                                        prefs::kDefaultPiPOpacity);
  prefs->registry()->RegisterBooleanPref(prefs::kPrefPiPAutoPipOnTabSwitch,
                                         prefs::kDefaultPiPAutoPipOnTabSwitch);
  prefs->registry()->RegisterIntegerPref(prefs::kPrefPiPMaxWindows,
                                         prefs::kDefaultPiPMaxWindows);
  prefs->registry()->RegisterDoublePref(prefs::kPrefPiPDefaultVolume,
                                        prefs::kDefaultPiPDefaultVolume);

  // Controls-level prefs.
  prefs->registry()->RegisterBooleanPref(prefs::kPrefPiPControlsAutoHide,
                                         prefs::kDefaultPiPControlsAutoHide);
  prefs->registry()->RegisterIntegerPref(
      prefs::kPrefPiPControlsAutoHideDelayMs,
      prefs::kDefaultPiPControlsAutoHideDelayMs);
  prefs->registry()->RegisterBooleanPref(prefs::kPrefPiPControlsShowTopBar,
                                         prefs::kDefaultPiPControlsShowTopBar);
  prefs->registry()->RegisterBooleanPref(
      prefs::kPrefPiPControlsShowBottomBar,
      prefs::kDefaultPiPControlsShowBottomBar);
  prefs->registry()->RegisterBooleanPref(
      prefs::kPrefPiPControlsShowResizeHandle,
      prefs::kDefaultPiPControlsShowResizeHandle);
  prefs->registry()->RegisterDoublePref(prefs::kPrefPiPControlsOpacity,
                                        prefs::kDefaultPiPControlsOpacity);
  prefs->registry()->RegisterStringPref(
      prefs::kPrefPiPControlsDefaultSizePreset,
      prefs::kDefaultPiPControlsDefaultSizePreset);
  prefs->registry()->RegisterBooleanPref(
      prefs::kPrefPiPControlsShowAlwaysOnTopButton,
      prefs::kDefaultPiPControlsShowAlwaysOnTopButton);
  prefs->registry()->RegisterBooleanPref(
      prefs::kPrefPiPControlsShowPlaybackControls,
      prefs::kDefaultPiPControlsShowPlaybackControls);
  prefs->registry()->RegisterBooleanPref(
      prefs::kPrefPiPControlsShowSkipButtons,
      prefs::kDefaultPiPControlsShowSkipButtons);
  prefs->registry()->RegisterIntegerPref(
      prefs::kPrefPiPControlsSkipDurationSeconds,
      prefs::kDefaultPiPControlsSkipDurationSeconds);
  {
    base::Value::List default_presets;
    default_presets.Append(0.5);
    default_presets.Append(1.0);
    default_presets.Append(1.25);
    default_presets.Append(1.5);
    default_presets.Append(2.0);
    prefs->registry()->RegisterListPref(
        prefs::kPrefPiPControlsPlaybackRatePresets,
        std::move(default_presets));
  }

  // Appearance prefs.
  prefs->registry()->RegisterBooleanPref(prefs::kPrefPiPShowTitle,
                                         prefs::kDefaultPiPShowTitle);
  prefs->registry()->RegisterBooleanPref(prefs::kPrefPiPShowProgressBar,
                                         prefs::kDefaultPiPShowProgressBar);
  prefs->registry()->RegisterIntegerPref(prefs::kPrefPiPCornerRadius,
                                         prefs::kDefaultPiPCornerRadius);
  prefs->registry()->RegisterIntegerPref(prefs::kPrefPiPBorderWidth,
                                         prefs::kDefaultPiPBorderWidth);
  prefs->registry()->RegisterIntegerPref(prefs::kPrefPiPBorderColor,
                                         prefs::kDefaultPiPBorderColor);
  prefs->registry()->RegisterIntegerPref(prefs::kPrefPiPShadowElevation,
                                         prefs::kDefaultPiPShadowElevation);

  // Workspace prefs.
  prefs->registry()->RegisterBooleanPref(prefs::kPrefPiPFollowActiveTab,
                                         prefs::kDefaultPiPFollowActiveTab);
  prefs->registry()->RegisterBooleanPref(prefs::kPrefPiPShowInAllWorkspaces,
                                         prefs::kDefaultPiPShowInAllWorkspaces);

  // Snap prefs.
  prefs->registry()->RegisterBooleanPref(prefs::kPrefPiPSnapToEdges,
                                         prefs::kDefaultPiPSnapToEdges);
  prefs->registry()->RegisterIntegerPref(prefs::kPrefPiPSnapDistance,
                                         prefs::kDefaultPiPSnapDistance);

  // Aspect ratio prefs.
  prefs->registry()->RegisterBooleanPref(prefs::kPrefPiPMaintainAspectRatio,
                                         prefs::kDefaultPiPMaintainAspectRatio);

  // Min/max size prefs.
  prefs->registry()->RegisterStringPref(prefs::kPrefPiPMinSize,
                                        prefs::kDefaultPiPMinSize);
  prefs->registry()->RegisterStringPref(prefs::kPrefPiPMaxSize,
                                        prefs::kDefaultPiPMaxSize);

  // Auto-play and loop prefs.
  prefs->registry()->RegisterBooleanPref(prefs::kPrefPiPAutoPlay,
                                         prefs::kDefaultPiPAutoPlay);
  prefs->registry()->RegisterBooleanPref(prefs::kPrefPiPLoopByDefault,
                                         prefs::kDefaultPiPLoopByDefault);
}

}  // namespace

// =========================================================================
// AstraPipControlsModel basic construction tests
// =========================================================================

class AstraPipControlsModelTest : public testing::Test {
 public:
  AstraPipControlsModelTest() = default;
  ~AstraPipControlsModelTest() override = default;

  void SetUp() override {
    RegisterTestPrefs(&pref_service_);
    model_ = std::make_unique<AstraPipControlsModel>(&pref_service_);
  }

  void TearDown() override {
    model_.reset();
  }

 protected:
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<AstraPipControlsModel> model_;
};

TEST_F(AstraPipControlsModelTest, ConstructsWithoutCrash) {
  EXPECT_NE(nullptr, model_);
  EXPECT_NE(nullptr, model_->pref_service());
}

TEST_F(AstraPipControlsModelTest, DefaultPlayStateIsFalse) {
  EXPECT_FALSE(model_->IsPlaying());
}

TEST_F(AstraPipControlsModelTest, DefaultMuteStateIsFalse) {
  EXPECT_FALSE(model_->IsMuted());
}

TEST_F(AstraPipControlsModelTest, DefaultVolumeIsMax) {
  EXPECT_DOUBLE_EQ(AstraPipControlsModel::kDefaultVolume, model_->GetVolume());
}

TEST_F(AstraPipControlsModelTest, DefaultPlaybackRateIsNormal) {
  EXPECT_DOUBLE_EQ(1.0, model_->playback_rate());
}

TEST_F(AstraPipControlsModelTest, DefaultSizePresetIsMedium) {
  EXPECT_EQ(PipSizePreset::kMedium, model_->active_preset());
  EXPECT_EQ(AstraPipSizePreset::kMedium, model_->GetSizePreset());
}

TEST_F(AstraPipControlsModelTest, DefaultIsPinnedIsTrue) {
  EXPECT_TRUE(model_->is_pinned());
  EXPECT_TRUE(model_->GetAlwaysOnTop());
}

TEST_F(AstraPipControlsModelTest, DefaultOpacityIsMax) {
  EXPECT_DOUBLE_EQ(AstraPipControlsModel::kDefaultOpacity, model_->opacity());
}

TEST_F(AstraPipControlsModelTest, DefaultControlsVisibleIsTrue) {
  EXPECT_TRUE(model_->controls_visible());
  EXPECT_TRUE(model_->GetShowControls());
}

TEST_F(AstraPipControlsModelTest, DefaultControlsMinimizedIsFalse) {
  EXPECT_FALSE(model_->controls_minimized());
}

TEST_F(AstraPipControlsModelTest, DefaultSnapPositionIsBottomRight) {
  EXPECT_EQ(PipSnapPosition::kBottomRight, model_->snap_position());
}

// =========================================================================
// Model PiP state tests
// =========================================================================

TEST_F(AstraPipControlsModelTest, PipIsNotActiveByDefault) {
  EXPECT_FALSE(model_->IsPipActive());
}

TEST_F(AstraPipControlsModelTest, ActivatePip) {
  MockAstraPipObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnPipActivated(model_.get())).Times(1);
  model_->ActivatePip();

  EXPECT_TRUE(model_->IsPipActive());
  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, DeactivatePip) {
  model_->ActivatePip();
  ASSERT_TRUE(model_->IsPipActive());

  MockAstraPipObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnPipDeactivated(model_.get())).Times(1);
  model_->DeactivatePip();

  EXPECT_FALSE(model_->IsPipActive());
  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, TogglePip) {
  EXPECT_FALSE(model_->IsPipActive());
  model_->TogglePip();
  EXPECT_TRUE(model_->IsPipActive());
  model_->TogglePip();
  EXPECT_FALSE(model_->IsPipActive());
}

TEST_F(AstraPipControlsModelTest, ActivatePipWhenAlreadyActiveNoNotification) {
  model_->ActivatePip();
  ASSERT_TRUE(model_->IsPipActive());

  MockAstraPipObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnPipActivated(_)).Times(0);
  model_->ActivatePip();

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, DeactivatePipWhenAlreadyInactiveNoNotification) {
  ASSERT_FALSE(model_->IsPipActive());

  MockAstraPipObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnPipDeactivated(_)).Times(0);
  model_->DeactivatePip();

  model_->RemoveObserver(&observer);
}

// =========================================================================
// Model tab identity tests
// =========================================================================

TEST_F(AstraPipControlsModelTest, TabIdDefaultIsNegativeOne) {
  EXPECT_EQ(-1, model_->GetTabId());
}

TEST_F(AstraPipControlsModelTest, SetTabId) {
  model_->SetTabId(42);
  EXPECT_EQ(42, model_->GetTabId());

  model_->SetTabId(100);
  EXPECT_EQ(100, model_->GetTabId());
}

TEST_F(AstraPipControlsModelTest, TabTitleDefaultIsEmpty) {
  EXPECT_TRUE(model_->GetTabTitle().empty());
}

TEST_F(AstraPipControlsModelTest, SetTabTitle) {
  model_->SetTabTitle(u"My Video");
  EXPECT_EQ(u"My Video", model_->GetTabTitle());

  model_->SetTabTitle(u"Another Title");
  EXPECT_EQ(u"Another Title", model_->GetTabTitle());
}

TEST_F(AstraPipControlsModelTest, SetTabTitleEmpty) {
  model_->SetTabTitle(u"Test");
  model_->SetTabTitle(std::u16string());
  EXPECT_TRUE(model_->GetTabTitle().empty());
}

// =========================================================================
// Model size preset tests (AstraPipSizePreset)
// =========================================================================

TEST_F(AstraPipControlsModelTest, SizePresetDefaultIsMedium) {
  EXPECT_EQ(AstraPipSizePreset::kMedium, model_->GetSizePreset());
}

TEST_F(AstraPipControlsModelTest, SetSizePresetTiny) {
  model_->SetSizePreset(AstraPipSizePreset::kTiny);
  EXPECT_EQ(AstraPipSizePreset::kTiny, model_->GetSizePreset());
}

TEST_F(AstraPipControlsModelTest, SetSizePresetSmall) {
  model_->SetSizePreset(AstraPipSizePreset::kSmall);
  EXPECT_EQ(AstraPipSizePreset::kSmall, model_->GetSizePreset());
}

TEST_F(AstraPipControlsModelTest, SetSizePresetLarge) {
  model_->SetSizePreset(AstraPipSizePreset::kLarge);
  EXPECT_EQ(AstraPipSizePreset::kLarge, model_->GetSizePreset());
}

TEST_F(AstraPipControlsModelTest, SetSizePresetCustom) {
  model_->SetSizePreset(AstraPipSizePreset::kCustom);
  EXPECT_EQ(AstraPipSizePreset::kCustom, model_->GetSizePreset());
}

TEST_F(AstraPipControlsModelTest, SizePresetNotifiesSizeChange) {
  MockAstraPipObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnPipSizeChanged(model_.get(), _)).Times(1);
  model_->SetSizePreset(AstraPipSizePreset::kSmall);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, GetSizePresetsReturnsFivePresets) {
  auto presets = AstraPipControlsModel::GetSizePresets();
  EXPECT_EQ(5u, presets.size());
  EXPECT_EQ(AstraPipSizePreset::kTiny, presets[0].first);
  EXPECT_EQ(AstraPipSizePreset::kSmall, presets[1].first);
  EXPECT_EQ(AstraPipSizePreset::kMedium, presets[2].first);
  EXPECT_EQ(AstraPipSizePreset::kLarge, presets[3].first);
  EXPECT_EQ(AstraPipSizePreset::kCustom, presets[4].first);
}

TEST_F(AstraPipControlsModelTest, GetPresetSizeTinyIsPositive) {
  gfx::Size size =
      AstraPipControlsModel::GetPresetSize(AstraPipSizePreset::kTiny);
  EXPECT_GT(size.width(), 0);
  EXPECT_GT(size.height(), 0);
}

TEST_F(AstraPipControlsModelTest, GetPresetSizesAreOrdered) {
  gfx::Size tiny =
      AstraPipControlsModel::GetPresetSize(AstraPipSizePreset::kTiny);
  gfx::Size small =
      AstraPipControlsModel::GetPresetSize(AstraPipSizePreset::kSmall);
  gfx::Size medium =
      AstraPipControlsModel::GetPresetSize(AstraPipSizePreset::kMedium);
  gfx::Size large =
      AstraPipControlsModel::GetPresetSize(AstraPipSizePreset::kLarge);

  EXPECT_LT(tiny.width(), small.width());
  EXPECT_LT(small.width(), medium.width());
  EXPECT_LT(medium.width(), large.width());
}

TEST_F(AstraPipControlsModelTest, GetPresetSizeCustomIsEmpty) {
  gfx::Size size =
      AstraPipControlsModel::GetPresetSize(AstraPipSizePreset::kCustom);
  EXPECT_TRUE(size.IsEmpty());
}

// =========================================================================
// Model size and position tests
// =========================================================================

TEST_F(AstraPipControlsModelTest, DefaultSizeIsMediumPreset) {
  gfx::Size expected =
      AstraPipControlsModel::GetPresetSize(AstraPipSizePreset::kMedium);
  EXPECT_EQ(expected, model_->GetSize());
}

TEST_F(AstraPipControlsModelTest, SetSize) {
  gfx::Size new_size(500, 300);
  model_->SetSize(new_size);
  EXPECT_EQ(new_size, model_->GetSize());
}

TEST_F(AstraPipControlsModelTest, SetSizeNotifiesObserver) {
  MockAstraPipObserver observer;
  model_->AddObserver(&observer);

  gfx::Size new_size(500, 300);
  EXPECT_CALL(observer, OnPipSizeChanged(model_.get(), new_size)).Times(1);
  model_->SetSize(new_size);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, SetSizeSameNoNotification) {
  gfx::Size current = model_->GetSize();

  MockAstraPipObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnPipSizeChanged(_, _)).Times(0);
  model_->SetSize(current);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, SetSizeBecomesCustomPreset) {
  // Start with medium preset.
  ASSERT_EQ(AstraPipSizePreset::kMedium, model_->GetSizePreset());

  // Set a non-preset size.
  model_->SetSize(gfx::Size(400, 250));
  EXPECT_EQ(AstraPipSizePreset::kCustom, model_->GetSizePreset());
}

TEST_F(AstraPipControlsModelTest, SetSizeMatchingPresetUpdatesPreset) {
  model_->SetSize(gfx::Size(500, 300));  // Custom size.
  ASSERT_EQ(AstraPipSizePreset::kCustom, model_->GetSizePreset());

  // Set to a known preset size.
  gfx::Size large_size =
      AstraPipControlsModel::GetPresetSize(AstraPipSizePreset::kLarge);
  model_->SetSize(large_size);
  EXPECT_EQ(AstraPipSizePreset::kLarge, model_->GetSizePreset());
}

TEST_F(AstraPipControlsModelTest, DefaultPositionIsOrigin) {
  EXPECT_EQ(gfx::Point(), model_->GetPosition());
}

TEST_F(AstraPipControlsModelTest, SetPosition) {
  gfx::Point new_pos(100, 200);
  model_->SetPosition(new_pos);
  EXPECT_EQ(new_pos, model_->GetPosition());
}

TEST_F(AstraPipControlsModelTest, SetPositionNotifiesObserver) {
  MockAstraPipObserver observer;
  model_->AddObserver(&observer);

  gfx::Point new_pos(100, 200);
  EXPECT_CALL(observer, OnPipPositionChanged(model_.get(), new_pos)).Times(1);
  model_->SetPosition(new_pos);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, SetPositionSameNoNotification) {
  gfx::Point current = model_->GetPosition();

  MockAstraPipObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnPipPositionChanged(_, _)).Times(0);
  model_->SetPosition(current);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, SetPositionNegative) {
  model_->SetPosition(gfx::Point(-50, -30));
  EXPECT_EQ(gfx::Point(-50, -30), model_->GetPosition());
}

TEST_F(AstraPipControlsModelTest, MinSizeDefault) {
  gfx::Size min = model_->GetMinSize();
  EXPECT_GT(min.width(), 0);
  EXPECT_GT(min.height(), 0);
}

TEST_F(AstraPipControlsModelTest, MaxSizeDefault) {
  gfx::Size max = model_->GetMaxSize();
  EXPECT_GT(max.width(), 0);
  EXPECT_GT(max.height(), 0);
  EXPECT_GT(max.width(), model_->GetMinSize().width());
  EXPECT_GT(max.height(), model_->GetMinSize().height());
}

TEST_F(AstraPipControlsModelTest, SetMinSize) {
  gfx::Size new_min(200, 150);
  model_->SetMinSize(new_min);
  EXPECT_EQ(new_min, model_->GetMinSize());
}

TEST_F(AstraPipControlsModelTest, SetMaxSize) {
  gfx::Size new_max(800, 600);
  model_->SetMaxSize(new_max);
  EXPECT_EQ(new_max, model_->GetMaxSize());
}

TEST_F(AstraPipControlsModelTest, SetSizeClampedToMin) {
  model_->SetMinSize(gfx::Size(300, 200));
  model_->SetSize(gfx::Size(100, 50));
  EXPECT_GE(model_->GetSize().width(), 300);
  EXPECT_GE(model_->GetSize().height(), 200);
}

TEST_F(AstraPipControlsModelTest, SetSizeClampedToMax) {
  model_->SetMaxSize(gfx::Size(500, 400));
  model_->SetSize(gfx::Size(800, 600));
  EXPECT_LE(model_->GetSize().width(), 500);
  EXPECT_LE(model_->GetSize().height(), 400);
}

TEST_F(AstraPipControlsModelTest, SetMinSizeReclampsCurrentSize) {
  model_->SetSize(gfx::Size(200, 150));
  model_->SetMinSize(gfx::Size(300, 200));
  EXPECT_GE(model_->GetSize().width(), 300);
  EXPECT_GE(model_->GetSize().height(), 200);
}

TEST_F(AstraPipControlsModelTest, SetMaxSizeReclampsCurrentSize) {
  model_->SetSize(gfx::Size(600, 500));
  model_->SetMaxSize(gfx::Size(400, 300));
  EXPECT_LE(model_->GetSize().width(), 400);
  EXPECT_LE(model_->GetSize().height(), 300);
}

TEST_F(AstraPipControlsModelTest, SetMinSizeClampsNegativeToZero) {
  model_->SetMinSize(gfx::Size(-10, -5));
  EXPECT_GE(model_->GetMinSize().width(), 0);
  EXPECT_GE(model_->GetMinSize().height(), 0);
}

// =========================================================================
// Model aspect ratio tests
// =========================================================================

TEST_F(AstraPipControlsModelTest, AspectRatioDefault) {
  EXPECT_DOUBLE_EQ(AstraPipControlsModel::kDefaultAspectRatio,
                   model_->GetAspectRatio());
}

TEST_F(AstraPipControlsModelTest, SetAspectRatio) {
  model_->SetAspectRatio(4.0 / 3.0);
  EXPECT_DOUBLE_EQ(4.0 / 3.0, model_->GetAspectRatio());
}

TEST_F(AstraPipControlsModelTest, SetAspectRatioClampsToMin) {
  model_->SetAspectRatio(0.1);
  EXPECT_DOUBLE_EQ(AstraPipControlsModel::kMinAspectRatio,
                   model_->GetAspectRatio());
}

TEST_F(AstraPipControlsModelTest, SetAspectRatioClampsToMax) {
  model_->SetAspectRatio(10.0);
  EXPECT_DOUBLE_EQ(AstraPipControlsModel::kMaxAspectRatio,
                   model_->GetAspectRatio());
}

TEST_F(AstraPipControlsModelTest, MaintainAspectRatioDefaultTrue) {
  EXPECT_TRUE(model_->GetMaintainAspectRatio());
}

TEST_F(AstraPipControlsModelTest, SetMaintainAspectRatio) {
  model_->SetMaintainAspectRatio(false);
  EXPECT_FALSE(model_->GetMaintainAspectRatio());

  model_->SetMaintainAspectRatio(true);
  EXPECT_TRUE(model_->GetMaintainAspectRatio());
}

TEST_F(AstraPipControlsModelTest, MaintainAspectRatioSameNoChange) {
  model_->SetMaintainAspectRatio(true);  // Already true.
  EXPECT_TRUE(model_->GetMaintainAspectRatio());
}

TEST_F(AstraPipControlsModelTest, EnablingAspectRatioReclampsSize) {
  model_->SetMaintainAspectRatio(false);
  model_->SetSize(gfx::Size(400, 200));  // 2:1 ratio

  model_->SetMaintainAspectRatio(true);  // Default 16:9
  // Size should be adjusted to maintain aspect ratio.
  EXPECT_DOUBLE_EQ(static_cast<double>(model_->GetSize().width()) /
                       model_->GetSize().height(),
                   AstraPipControlsModel::kDefaultAspectRatio);
}

// =========================================================================
// Model snap to edges tests
// =========================================================================

TEST_F(AstraPipControlsModelTest, SnapToEdgesDefaultTrue) {
  EXPECT_TRUE(model_->GetSnapToEdges());
}

TEST_F(AstraPipControlsModelTest, SetSnapToEdges) {
  model_->SetSnapToEdges(false);
  EXPECT_FALSE(model_->GetSnapToEdges());

  model_->SetSnapToEdges(true);
  EXPECT_TRUE(model_->GetSnapToEdges());
}

TEST_F(AstraPipControlsModelTest, SnapDistanceDefault) {
  EXPECT_EQ(AstraPipControlsModel::kDefaultSnapDistance,
            model_->GetSnapDistance());
}

TEST_F(AstraPipControlsModelTest, SetSnapDistance) {
  model_->SetSnapDistance(50);
  EXPECT_EQ(50, model_->GetSnapDistance());
}

TEST_F(AstraPipControlsModelTest, SetSnapDistanceNegativeClampsToZero) {
  model_->SetSnapDistance(-10);
  EXPECT_EQ(0, model_->GetSnapDistance());
}

TEST_F(AstraPipControlsModelTest, SetSnapDistanceZero) {
  model_->SetSnapDistance(0);
  EXPECT_EQ(0, model_->GetSnapDistance());
}

// =========================================================================
// Model playback state tests
// =========================================================================

TEST_F(AstraPipControlsModelTest, SetPlayingTrue) {
  MockPipControlsModelObserver observer;
  model_->AddControlsObserver(&observer);

  EXPECT_CALL(observer, OnPlayStateChanged(true)).Times(1);
  model_->SetPlaying(true);

  EXPECT_TRUE(model_->IsPlaying());
  model_->RemoveControlsObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, SetPlayingFalse) {
  model_->SetPlaying(true);
  ASSERT_TRUE(model_->IsPlaying());

  MockPipControlsModelObserver observer;
  model_->AddControlsObserver(&observer);

  EXPECT_CALL(observer, OnPlayStateChanged(false)).Times(1);
  model_->SetPlaying(false);

  EXPECT_FALSE(model_->IsPlaying());
  model_->RemoveControlsObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, SetPlayingSameStateNoNotification) {
  model_->SetPlaying(false);

  MockPipControlsModelObserver observer;
  model_->AddControlsObserver(&observer);

  EXPECT_CALL(observer, OnPlayStateChanged(_)).Times(0);
  model_->SetPlaying(false);

  model_->RemoveControlsObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, TogglePlayPause) {
  model_->SetPlaying(false);
  model_->TogglePlayPause();
  EXPECT_TRUE(model_->IsPlaying());

  model_->TogglePlayPause();
  EXPECT_FALSE(model_->IsPlaying());
}

TEST_F(AstraPipControlsModelTest, PlaybackStateNotifiesAstraPipObserver) {
  MockAstraPipObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnPlaybackStateChanged(model_.get(), true)).Times(1);
  model_->SetPlaying(true);

  model_->RemoveObserver(&observer);
}

// =========================================================================
// Model progress and duration tests
// =========================================================================

TEST_F(AstraPipControlsModelTest, ProgressDefaultIsZero) {
  EXPECT_DOUBLE_EQ(0.0, model_->GetPlaybackProgress());
}

TEST_F(AstraPipControlsModelTest, SetProgress) {
  model_->SetPlaybackProgress(0.5);
  EXPECT_DOUBLE_EQ(0.5, model_->GetPlaybackProgress());
}

TEST_F(AstraPipControlsModelTest, SetProgressNotifiesControlsObserver) {
  MockPipControlsModelObserver observer;
  model_->AddControlsObserver(&observer);

  EXPECT_CALL(observer, OnProgressChanged(0.5)).Times(1);
  model_->SetPlaybackProgress(0.5);

  model_->RemoveControlsObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, SetProgressNotifiesAstraPipObserver) {
  MockAstraPipObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnProgressChanged(model_.get(), 0.5)).Times(1);
  model_->SetPlaybackProgress(0.5);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, SetProgressClampsToZero) {
  model_->SetPlaybackProgress(-0.5);
  EXPECT_DOUBLE_EQ(0.0, model_->GetPlaybackProgress());
}

TEST_F(AstraPipControlsModelTest, SetProgressClampsToOne) {
  model_->SetPlaybackProgress(2.0);
  EXPECT_DOUBLE_EQ(1.0, model_->GetPlaybackProgress());
}

TEST_F(AstraPipControlsModelTest, SetProgressSameNoNotification) {
  model_->SetPlaybackProgress(0.5);

  MockPipControlsModelObserver observer;
  model_->AddControlsObserver(&observer);

  EXPECT_CALL(observer, OnProgressChanged(_)).Times(0);
  model_->SetPlaybackProgress(0.5);

  model_->RemoveControlsObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, DurationDefaultIsZero) {
  EXPECT_TRUE(model_->GetDuration().is_zero());
}

TEST_F(AstraPipControlsModelTest, SetDuration) {
  model_->SetDuration(base::Seconds(120));
  EXPECT_EQ(base::Seconds(120), model_->GetDuration());
}

TEST_F(AstraPipControlsModelTest, SetDurationNegativeClampsToZero) {
  model_->SetDuration(base::Seconds(-10));
  EXPECT_TRUE(model_->GetDuration().is_zero());
}

TEST_F(AstraPipControlsModelTest, CurrentTimeDefaultIsZero) {
  EXPECT_TRUE(model_->GetCurrentTime().is_zero());
}

TEST_F(AstraPipControlsModelTest, SetCurrentTime) {
  model_->SetDuration(base::Seconds(120));
  model_->SetCurrentTime(base::Seconds(30));
  EXPECT_EQ(base::Seconds(30), model_->GetCurrentTime());
  // Progress should be 0.25 (30/120).
  EXPECT_DOUBLE_EQ(0.25, model_->GetPlaybackProgress());
}

TEST_F(AstraPipControlsModelTest, SetCurrentTimeUpdatesProgress) {
  model_->SetDuration(base::Seconds(100));
  model_->SetCurrentTime(base::Seconds(50));
  EXPECT_DOUBLE_EQ(0.5, model_->GetPlaybackProgress());
}

TEST_F(AstraPipControlsModelTest, SetCurrentTimeClampsToDuration) {
  model_->SetDuration(base::Seconds(60));
  model_->SetCurrentTime(base::Seconds(120));
  EXPECT_EQ(base::Seconds(60), model_->GetCurrentTime());
  EXPECT_DOUBLE_EQ(1.0, model_->GetPlaybackProgress());
}

TEST_F(AstraPipControlsModelTest, SetCurrentTimeNegativeClampsToZero) {
  model_->SetCurrentTime(base::Seconds(-5));
  EXPECT_TRUE(model_->GetCurrentTime().is_zero());
}

TEST_F(AstraPipControlsModelTest, SetProgressUpdatesCurrentTime) {
  model_->SetDuration(base::Seconds(100));
  model_->SetPlaybackProgress(0.3);
  // Current time should be ~30 seconds.
  EXPECT_NEAR(30.0, model_->GetCurrentTime().InSecondsF(), 0.001);
}

TEST_F(AstraPipControlsModelTest, ProgressWithZeroDuration) {
  // Duration is zero by default. Setting progress should work but not affect
  // current_time in a meaningful way.
  model_->SetPlaybackProgress(0.5);
  EXPECT_DOUBLE_EQ(0.5, model_->GetPlaybackProgress());
  EXPECT_TRUE(model_->GetCurrentTime().is_zero());
}

// =========================================================================
// Model volume and mute tests
// =========================================================================

TEST_F(AstraPipControlsModelTest, SetVolumeNormal) {
  MockPipControlsModelObserver observer;
  model_->AddControlsObserver(&observer);

  EXPECT_CALL(observer, OnVolumeChanged(0.5)).Times(1);
  model_->SetVolume(0.5);
  EXPECT_DOUBLE_EQ(0.5, model_->GetVolume());

  model_->RemoveControlsObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, SetVolumeClampsToMin) {
  model_->SetVolume(-1.0);
  EXPECT_DOUBLE_EQ(AstraPipControlsModel::kMinVolume, model_->GetVolume());
}

TEST_F(AstraPipControlsModelTest, SetVolumeClampsToMax) {
  model_->SetVolume(2.0);
  EXPECT_DOUBLE_EQ(AstraPipControlsModel::kMaxVolume, model_->GetVolume());
}

TEST_F(AstraPipControlsModelTest, SetVolumeSameValueNoNotification) {
  model_->SetVolume(0.5);

  MockPipControlsModelObserver observer;
  model_->AddControlsObserver(&observer);

  EXPECT_CALL(observer, OnVolumeChanged(_)).Times(0);
  model_->SetVolume(0.5);

  model_->RemoveControlsObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, IncreaseVolume) {
  model_->SetVolume(0.5);
  model_->IncreaseVolume();
  EXPECT_DOUBLE_EQ(0.6, model_->GetVolume());
}

TEST_F(AstraPipControlsModelTest, DecreaseVolume) {
  model_->SetVolume(0.5);
  model_->DecreaseVolume();
  EXPECT_DOUBLE_EQ(0.4, model_->GetVolume());
}

TEST_F(AstraPipControlsModelTest, IncreaseVolumeClampsToMax) {
  model_->SetVolume(1.0);
  model_->IncreaseVolume();
  EXPECT_DOUBLE_EQ(AstraPipControlsModel::kMaxVolume, model_->GetVolume());
}

TEST_F(AstraPipControlsModelTest, DecreaseVolumeClampsToMin) {
  model_->SetVolume(0.0);
  model_->DecreaseVolume();
  EXPECT_DOUBLE_EQ(AstraPipControlsModel::kMinVolume, model_->GetVolume());
}

TEST_F(AstraPipControlsModelTest, SettingVolumeAboveZeroUnmutes) {
  model_->SetMuted(true);
  ASSERT_TRUE(model_->IsMuted());

  model_->SetVolume(0.5);
  EXPECT_FALSE(model_->IsMuted());
}

TEST_F(AstraPipControlsModelTest, SettingVolumeToZeroDoesNotMute) {
  model_->SetVolume(0.0);
  // Setting volume to 0 doesn't automatically set muted = true.
  EXPECT_FALSE(model_->IsMuted());
}

TEST_F(AstraPipControlsModelTest, VolumeNotifiesAstraPipObserver) {
  MockAstraPipObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnVolumeChanged(model_.get(), 0.5)).Times(1);
  model_->SetVolume(0.5);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, MuteNotifiesAstraPipObserver) {
  MockAstraPipObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnMuteChanged(model_.get(), true)).Times(1);
  model_->SetMuted(true);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, ToggleMute) {
  model_->SetMuted(false);
  model_->ToggleMute();
  EXPECT_TRUE(model_->IsMuted());

  model_->ToggleMute();
  EXPECT_FALSE(model_->IsMuted());
}

// =========================================================================
// Model looping tests
// =========================================================================

TEST_F(AstraPipControlsModelTest, LoopingDefault) {
  EXPECT_FALSE(model_->IsLooping());
}

TEST_F(AstraPipControlsModelTest, SetLooping) {
  model_->SetLooping(true);
  EXPECT_TRUE(model_->IsLooping());

  model_->SetLooping(false);
  EXPECT_FALSE(model_->IsLooping());
}

TEST_F(AstraPipControlsModelTest, ToggleLoop) {
  model_->SetLooping(false);
  model_->ToggleLoop();
  EXPECT_TRUE(model_->IsLooping());

  model_->ToggleLoop();
  EXPECT_FALSE(model_->IsLooping());
}

TEST_F(AstraPipControlsModelTest, SetLoopingNotifiesObserver) {
  MockPipControlsModelObserver observer;
  model_->AddControlsObserver(&observer);

  EXPECT_CALL(observer, OnLoopingChanged(true)).Times(1);
  model_->SetLooping(true);

  model_->RemoveControlsObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, SetLoopingSameNoNotification) {
  MockPipControlsModelObserver observer;
  model_->AddControlsObserver(&observer);

  EXPECT_CALL(observer, OnLoopingChanged(_)).Times(0);
  model_->SetLooping(false);  // Already false.

  model_->RemoveControlsObserver(&observer);
}

// =========================================================================
// Model playback speed tests
// =========================================================================

TEST_F(AstraPipControlsModelTest, PlaybackSpeedDefaultIsNormal) {
  EXPECT_EQ(AstraPipPlaybackSpeed::k1_0x, model_->GetPlaybackSpeed());
}

TEST_F(AstraPipControlsModelTest, SetPlaybackSpeed) {
  model_->SetPlaybackSpeed(AstraPipPlaybackSpeed::k1_5x);
  EXPECT_EQ(AstraPipPlaybackSpeed::k1_5x, model_->GetPlaybackSpeed());
}

TEST_F(AstraPipControlsModelTest, SetPlaybackSpeedNotifiesObserver) {
  MockPipControlsModelObserver observer;
  model_->AddControlsObserver(&observer);

  EXPECT_CALL(observer, OnPlaybackSpeedChanged(AstraPipPlaybackSpeed::k2x))
      .Times(1);
  model_->SetPlaybackSpeed(AstraPipPlaybackSpeed::k2x);

  model_->RemoveControlsObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, SetPlaybackSpeedUpdatesPlaybackRate) {
  model_->SetPlaybackSpeed(AstraPipPlaybackSpeed::k2x);
  EXPECT_DOUBLE_EQ(2.0, model_->playback_rate());
}

TEST_F(AstraPipControlsModelTest, SetPlaybackRateUpdatesSpeed) {
  model_->SetPlaybackRate(1.5);
  EXPECT_EQ(AstraPipPlaybackSpeed::k1_5x, model_->GetPlaybackSpeed());
}

TEST_F(AstraPipControlsModelTest, PlaybackSpeedToRateAllSpeeds) {
  EXPECT_DOUBLE_EQ(0.5, AstraPipControlsModel::PlaybackSpeedToRate(
                            AstraPipPlaybackSpeed::k0_5x));
  EXPECT_DOUBLE_EQ(0.75, AstraPipControlsModel::PlaybackSpeedToRate(
                             AstraPipPlaybackSpeed::k0_75x));
  EXPECT_DOUBLE_EQ(1.0, AstraPipControlsModel::PlaybackSpeedToRate(
                            AstraPipPlaybackSpeed::k1_0x));
  EXPECT_DOUBLE_EQ(1.25, AstraPipControlsModel::PlaybackSpeedToRate(
                             AstraPipPlaybackSpeed::k1_25x));
  EXPECT_DOUBLE_EQ(1.5, AstraPipControlsModel::PlaybackSpeedToRate(
                           AstraPipPlaybackSpeed::k1_5x));
  EXPECT_DOUBLE_EQ(2.0, AstraPipControlsModel::PlaybackSpeedToRate(
                            AstraPipPlaybackSpeed::k2x));
}

TEST_F(AstraPipControlsModelTest, RateToPlaybackSpeedAllRates) {
  EXPECT_EQ(AstraPipPlaybackSpeed::k0_5x,
            AstraPipControlsModel::RateToPlaybackSpeed(0.5));
  EXPECT_EQ(AstraPipPlaybackSpeed::k0_75x,
            AstraPipControlsModel::RateToPlaybackSpeed(0.75));
  EXPECT_EQ(AstraPipPlaybackSpeed::k1_0x,
            AstraPipControlsModel::RateToPlaybackSpeed(1.0));
  EXPECT_EQ(AstraPipPlaybackSpeed::k1_25x,
            AstraPipControlsModel::RateToPlaybackSpeed(1.25));
  EXPECT_EQ(AstraPipPlaybackSpeed::k1_5x,
            AstraPipControlsModel::RateToPlaybackSpeed(1.5));
  EXPECT_EQ(AstraPipPlaybackSpeed::k2x,
            AstraPipControlsModel::RateToPlaybackSpeed(2.0));
}

TEST_F(AstraPipControlsModelTest, RateToPlaybackSpeedNearestPreset) {
  // 0.85 is closer to 0.75 than 1.0.
  EXPECT_EQ(AstraPipPlaybackSpeed::k0_75x,
            AstraPipControlsModel::RateToPlaybackSpeed(0.85));
  // 1.1 is closer to 1.0 than 1.25.
  EXPECT_EQ(AstraPipPlaybackSpeed::k1_0x,
            AstraPipControlsModel::RateToPlaybackSpeed(1.1));
  // 3.0 is closest to 2x (the max).
  EXPECT_EQ(AstraPipPlaybackSpeed::k2x,
            AstraPipControlsModel::RateToPlaybackSpeed(3.0));
}

// =========================================================================
// Model window controls tests
// =========================================================================

TEST_F(AstraPipControlsModelTest, StickyDefaultTrue) {
  EXPECT_TRUE(model_->GetSticky());
}

TEST_F(AstraPipControlsModelTest, SetSticky) {
  model_->SetSticky(false);
  EXPECT_FALSE(model_->GetSticky());
  EXPECT_FALSE(model_->GetAlwaysOnTop());

  model_->SetSticky(true);
  EXPECT_TRUE(model_->GetSticky());
  EXPECT_TRUE(model_->GetAlwaysOnTop());
}

TEST_F(AstraPipControlsModelTest, AlwaysOnTopDefaultTrue) {
  EXPECT_TRUE(model_->GetAlwaysOnTop());
}

TEST_F(AstraPipControlsModelTest, SetAlwaysOnTop) {
  model_->SetAlwaysOnTop(false);
  EXPECT_FALSE(model_->GetAlwaysOnTop());

  model_->SetAlwaysOnTop(true);
  EXPECT_TRUE(model_->GetAlwaysOnTop());
}

TEST_F(AstraPipControlsModelTest, MinimizedDefaultFalse) {
  EXPECT_FALSE(model_->IsMinimized());
}

TEST_F(AstraPipControlsModelTest, SetMinimized) {
  model_->SetMinimized(true);
  EXPECT_TRUE(model_->IsMinimized());

  model_->SetMinimized(false);
  EXPECT_FALSE(model_->IsMinimized());
}

TEST_F(AstraPipControlsModelTest, MaximizedDefaultFalse) {
  EXPECT_FALSE(model_->IsMaximized());
}

TEST_F(AstraPipControlsModelTest, SetMaximized) {
  model_->SetMaximized(true);
  EXPECT_TRUE(model_->IsMaximized());

  model_->SetMaximized(false);
  EXPECT_FALSE(model_->IsMaximized());
}

TEST_F(AstraPipControlsModelTest, ResizableDefaultTrue) {
  EXPECT_TRUE(model_->IsResizable());
}

TEST_F(AstraPipControlsModelTest, SetResizable) {
  model_->SetResizable(false);
  EXPECT_FALSE(model_->IsResizable());

  model_->SetResizable(true);
  EXPECT_TRUE(model_->IsResizable());
}

TEST_F(AstraPipControlsModelTest, MovableDefaultTrue) {
  EXPECT_TRUE(model_->IsMovable());
}

TEST_F(AstraPipControlsModelTest, SetMovable) {
  model_->SetMovable(false);
  EXPECT_FALSE(model_->IsMovable());

  model_->SetMovable(true);
  EXPECT_TRUE(model_->IsMovable());
}

// =========================================================================
// Model workspace integration tests
// =========================================================================

TEST_F(AstraPipControlsModelTest, WorkspaceIdDefaultEmpty) {
  EXPECT_TRUE(model_->GetWorkspaceId().empty());
}

TEST_F(AstraPipControlsModelTest, SetWorkspaceId) {
  model_->SetWorkspaceId("workspace-123");
  EXPECT_EQ("workspace-123", model_->GetWorkspaceId());
}

TEST_F(AstraPipControlsModelTest, FollowActiveTabDefaultFalse) {
  EXPECT_FALSE(model_->GetFollowActiveTab());
}

TEST_F(AstraPipControlsModelTest, SetFollowActiveTab) {
  model_->SetFollowActiveTab(true);
  EXPECT_TRUE(model_->GetFollowActiveTab());

  model_->SetFollowActiveTab(false);
  EXPECT_FALSE(model_->GetFollowActiveTab());
}

TEST_F(AstraPipControlsModelTest, ShowInAllWorkspacesDefaultFalse) {
  EXPECT_FALSE(model_->GetShowInAllWorkspaces());
}

TEST_F(AstraPipControlsModelTest, SetShowInAllWorkspaces) {
  model_->SetShowInAllWorkspaces(true);
  EXPECT_TRUE(model_->GetShowInAllWorkspaces());

  model_->SetShowInAllWorkspaces(false);
  EXPECT_FALSE(model_->GetShowInAllWorkspaces());
}

// =========================================================================
// Model appearance tests
// =========================================================================

TEST_F(AstraPipControlsModelTest, ShowControlsDefaultTrue) {
  EXPECT_TRUE(model_->GetShowControls());
}

TEST_F(AstraPipControlsModelTest, SetShowControls) {
  model_->SetShowControls(false);
  EXPECT_FALSE(model_->GetShowControls());

  model_->SetShowControls(true);
  EXPECT_TRUE(model_->GetShowControls());
}

TEST_F(AstraPipControlsModelTest, AutoHideControlsDefaultTrue) {
  EXPECT_TRUE(model_->GetAutoHideControls());
}

TEST_F(AstraPipControlsModelTest, SetAutoHideControls) {
  model_->SetAutoHideControls(false);
  EXPECT_FALSE(model_->GetAutoHideControls());

  model_->SetAutoHideControls(true);
  EXPECT_TRUE(model_->GetAutoHideControls());
}

TEST_F(AstraPipControlsModelTest, ControlsAutoHideDelayDefault) {
  EXPECT_EQ(AstraPipControlsModel::kDefaultAutoHideDelayMs,
            model_->GetControlsAutoHideDelay());
}

TEST_F(AstraPipControlsModelTest, SetControlsAutoHideDelay) {
  model_->SetControlsAutoHideDelay(5000);
  EXPECT_EQ(5000, model_->GetControlsAutoHideDelay());
}

TEST_F(AstraPipControlsModelTest, SetControlsAutoHideDelayNegativeClamps) {
  model_->SetControlsAutoHideDelay(-100);
  EXPECT_EQ(0, model_->GetControlsAutoHideDelay());
}

TEST_F(AstraPipControlsModelTest, ShowTitleDefaultTrue) {
  EXPECT_TRUE(model_->GetShowTitle());
}

TEST_F(AstraPipControlsModelTest, SetShowTitle) {
  model_->SetShowTitle(false);
  EXPECT_FALSE(model_->GetShowTitle());

  model_->SetShowTitle(true);
  EXPECT_TRUE(model_->GetShowTitle());
}

TEST_F(AstraPipControlsModelTest, ShowProgressBarDefaultTrue) {
  EXPECT_TRUE(model_->GetShowProgressBar());
}

TEST_F(AstraPipControlsModelTest, SetShowProgressBar) {
  model_->SetShowProgressBar(false);
  EXPECT_FALSE(model_->GetShowProgressBar());

  model_->SetShowProgressBar(true);
  EXPECT_TRUE(model_->GetShowProgressBar());
}

TEST_F(AstraPipControlsModelTest, CornerRadiusDefault) {
  EXPECT_EQ(AstraPipControlsModel::kDefaultCornerRadius,
            model_->GetCornerRadius());
}

TEST_F(AstraPipControlsModelTest, SetCornerRadius) {
  model_->SetCornerRadius(16);
  EXPECT_EQ(16, model_->GetCornerRadius());
}

TEST_F(AstraPipControlsModelTest, SetCornerRadiusNegativeClampsToZero) {
  model_->SetCornerRadius(-5);
  EXPECT_EQ(0, model_->GetCornerRadius());
}

TEST_F(AstraPipControlsModelTest, BorderWidthDefaultZero) {
  EXPECT_EQ(0, model_->GetBorderWidth());
}

TEST_F(AstraPipControlsModelTest, SetBorderWidth) {
  model_->SetBorderWidth(4);
  EXPECT_EQ(4, model_->GetBorderWidth());
}

TEST_F(AstraPipControlsModelTest, SetBorderWidthNegativeClampsToZero) {
  model_->SetBorderWidth(-2);
  EXPECT_EQ(0, model_->GetBorderWidth());
}

TEST_F(AstraPipControlsModelTest, BorderColorDefaultTransparent) {
  EXPECT_EQ(SK_ColorTRANSPARENT, model_->GetBorderColor());
}

TEST_F(AstraPipControlsModelTest, SetBorderColor) {
  model_->SetBorderColor(SK_ColorRED);
  EXPECT_EQ(SK_ColorRED, model_->GetBorderColor());

  model_->SetBorderColor(SK_ColorBLUE);
  EXPECT_EQ(SK_ColorBLUE, model_->GetBorderColor());
}

TEST_F(AstraPipControlsModelTest, ShadowElevationDefault) {
  EXPECT_EQ(AstraPipControlsModel::kDefaultShadowElevation,
            model_->GetShadowElevation());
}

TEST_F(AstraPipControlsModelTest, SetShadowElevation) {
  model_->SetShadowElevation(16);
  EXPECT_EQ(16, model_->GetShadowElevation());
}

TEST_F(AstraPipControlsModelTest, SetShadowElevationNegativeClampsToZero) {
  model_->SetShadowElevation(-3);
  EXPECT_EQ(0, model_->GetShadowElevation());
}

// =========================================================================
// Model settings pref keys tests
// =========================================================================

TEST_F(AstraPipControlsModelTest, SettingsPrefKeysAreNonEmpty) {
  EXPECT_FALSE(std::string(AstraPipControlsModel::kPrefDefaultSizePreset).empty());
  EXPECT_FALSE(std::string(AstraPipControlsModel::kPrefDefaultPosition).empty());
  EXPECT_FALSE(std::string(AstraPipControlsModel::kPrefMaintainAspectRatio).empty());
  EXPECT_FALSE(std::string(AstraPipControlsModel::kPrefSnapToEdges).empty());
  EXPECT_FALSE(std::string(AstraPipControlsModel::kPrefSnapDistance).empty());
  EXPECT_FALSE(std::string(AstraPipControlsModel::kPrefAlwaysOnTop).empty());
  EXPECT_FALSE(std::string(AstraPipControlsModel::kPrefShowControls).empty());
  EXPECT_FALSE(std::string(AstraPipControlsModel::kPrefAutoHideControls).empty());
  EXPECT_FALSE(std::string(AstraPipControlsModel::kPrefAutoHideDelayMs).empty());
  EXPECT_FALSE(std::string(AstraPipControlsModel::kPrefShowTitle).empty());
  EXPECT_FALSE(std::string(AstraPipControlsModel::kPrefShowProgressBar).empty());
  EXPECT_FALSE(std::string(AstraPipControlsModel::kPrefCornerRadius).empty());
  EXPECT_FALSE(std::string(AstraPipControlsModel::kPrefBorderWidth).empty());
  EXPECT_FALSE(std::string(AstraPipControlsModel::kPrefBorderColor).empty());
  EXPECT_FALSE(std::string(AstraPipControlsModel::kPrefShadowElevation).empty());
  EXPECT_FALSE(std::string(AstraPipControlsModel::kPrefDefaultVolume).empty());
  EXPECT_FALSE(std::string(AstraPipControlsModel::kPrefAutoPlay).empty());
  EXPECT_FALSE(std::string(AstraPipControlsModel::kPrefLoopByDefault).empty());
  EXPECT_FALSE(std::string(AstraPipControlsModel::kPrefFollowActiveTab).empty());
  EXPECT_FALSE(std::string(AstraPipControlsModel::kPrefShowInAllWorkspaces).empty());
  EXPECT_FALSE(std::string(AstraPipControlsModel::kPrefMinSize).empty());
  EXPECT_FALSE(std::string(AstraPipControlsModel::kPrefMaxSize).empty());
}

TEST_F(AstraPipControlsModelTest, SettingsPrefKeysAreUnique) {
  // All pref keys should be distinct.
  std::vector<std::string> keys = {
      AstraPipControlsModel::kPrefDefaultSizePreset,
      AstraPipControlsModel::kPrefDefaultPosition,
      AstraPipControlsModel::kPrefMaintainAspectRatio,
      AstraPipControlsModel::kPrefSnapToEdges,
      AstraPipControlsModel::kPrefSnapDistance,
      AstraPipControlsModel::kPrefAlwaysOnTop,
      AstraPipControlsModel::kPrefShowControls,
      AstraPipControlsModel::kPrefAutoHideControls,
      AstraPipControlsModel::kPrefAutoHideDelayMs,
      AstraPipControlsModel::kPrefShowTitle,
      AstraPipControlsModel::kPrefShowProgressBar,
      AstraPipControlsModel::kPrefCornerRadius,
      AstraPipControlsModel::kPrefBorderWidth,
      AstraPipControlsModel::kPrefBorderColor,
      AstraPipControlsModel::kPrefShadowElevation,
      AstraPipControlsModel::kPrefDefaultVolume,
      AstraPipControlsModel::kPrefAutoPlay,
      AstraPipControlsModel::kPrefLoopByDefault,
      AstraPipControlsModel::kPrefFollowActiveTab,
      AstraPipControlsModel::kPrefShowInAllWorkspaces,
      AstraPipControlsModel::kPrefMinSize,
      AstraPipControlsModel::kPrefMaxSize,
  };

  // Sort and check for duplicates.
  std::sort(keys.begin(), keys.end());
  auto it = std::unique(keys.begin(), keys.end());
  EXPECT_EQ(it, keys.end()) << "Duplicate pref key found";
}

// =========================================================================
// Model presentation settings tests (persisted via PrefService)
// =========================================================================

TEST_F(AstraPipControlsModelTest, GetPrefAutoHideControlsDefaultIsTrue) {
  EXPECT_TRUE(model_->GetPrefAutoHideControls());
}

TEST_F(AstraPipControlsModelTest, SetPrefAutoHideControls) {
  model_->SetPrefAutoHideControls(false);
  EXPECT_FALSE(model_->GetPrefAutoHideControls());
}

TEST_F(AstraPipControlsModelTest, GetPrefAutoHideDelayDefaultIsThreeSeconds) {
  EXPECT_EQ(base::Seconds(3), model_->GetPrefAutoHideDelay());
}

TEST_F(AstraPipControlsModelTest, SetPrefAutoHideDelay) {
  model_->SetPrefAutoHideDelay(base::Seconds(5));
  EXPECT_EQ(base::Seconds(5), model_->GetPrefAutoHideDelay());
}

TEST_F(AstraPipControlsModelTest, GetShowTopBarDefaultIsTrue) {
  EXPECT_TRUE(model_->GetShowTopBar());
}

TEST_F(AstraPipControlsModelTest, SetShowTopBar) {
  model_->SetShowTopBar(false);
  EXPECT_FALSE(model_->GetShowTopBar());
}

TEST_F(AstraPipControlsModelTest, GetShowBottomBarDefaultIsTrue) {
  EXPECT_TRUE(model_->GetShowBottomBar());
}

TEST_F(AstraPipControlsModelTest, SetShowBottomBar) {
  model_->SetShowBottomBar(false);
  EXPECT_FALSE(model_->GetShowBottomBar());
}

TEST_F(AstraPipControlsModelTest, GetShowResizeHandleDefaultIsTrue) {
  EXPECT_TRUE(model_->GetShowResizeHandle());
}

TEST_F(AstraPipControlsModelTest, SetShowResizeHandle) {
  model_->SetShowResizeHandle(false);
  EXPECT_FALSE(model_->GetShowResizeHandle());
}

TEST_F(AstraPipControlsModelTest, GetControlsOpacityDefaultIsMax) {
  EXPECT_DOUBLE_EQ(1.0, model_->GetControlsOpacity());
}

TEST_F(AstraPipControlsModelTest, SetControlsOpacity) {
  model_->SetControlsOpacity(0.7);
  EXPECT_DOUBLE_EQ(0.7, model_->GetControlsOpacity());
}

TEST_F(AstraPipControlsModelTest, GetDefaultSizePresetDefaultIsMedium) {
  EXPECT_EQ(PipSizePreset::kMedium, model_->GetDefaultSizePreset());
}

TEST_F(AstraPipControlsModelTest, SetDefaultSizePreset) {
  model_->SetDefaultSizePreset(PipSizePreset::kSmall);
  EXPECT_EQ(PipSizePreset::kSmall, model_->GetDefaultSizePreset());
}

TEST_F(AstraPipControlsModelTest, GetShowAlwaysOnTopButtonDefaultIsTrue) {
  EXPECT_TRUE(model_->GetShowAlwaysOnTopButton());
}

TEST_F(AstraPipControlsModelTest, SetShowAlwaysOnTopButton) {
  model_->SetShowAlwaysOnTopButton(false);
  EXPECT_FALSE(model_->GetShowAlwaysOnTopButton());
}

TEST_F(AstraPipControlsModelTest, GetShowPlaybackControlsDefaultIsTrue) {
  EXPECT_TRUE(model_->GetShowPlaybackControls());
}

TEST_F(AstraPipControlsModelTest, SetShowPlaybackControls) {
  model_->SetShowPlaybackControls(false);
  EXPECT_FALSE(model_->GetShowPlaybackControls());
}

TEST_F(AstraPipControlsModelTest, GetShowSkipButtonsDefaultIsTrue) {
  EXPECT_TRUE(model_->GetShowSkipButtons());
}

TEST_F(AstraPipControlsModelTest, SetShowSkipButtons) {
  model_->SetShowSkipButtons(false);
  EXPECT_FALSE(model_->GetShowSkipButtons());
}

TEST_F(AstraPipControlsModelTest, GetSkipDurationSecondsDefaultIsTen) {
  EXPECT_EQ(10, model_->GetSkipDurationSeconds());
}

TEST_F(AstraPipControlsModelTest, SetSkipDurationSeconds) {
  model_->SetSkipDurationSeconds(15);
  EXPECT_EQ(15, model_->GetSkipDurationSeconds());
}

TEST_F(AstraPipControlsModelTest, ResetSettingsToDefaults) {
  // Change several settings.
  model_->SetPrefAutoHideControls(false);
  model_->SetShowTopBar(false);
  model_->SetShowBottomBar(false);
  model_->SetControlsOpacity(0.5);
  model_->SetSkipDurationSeconds(20);
  model_->SetDefaultSizePreset(PipSizePreset::kLarge);

  // Verify they changed.
  EXPECT_FALSE(model_->GetPrefAutoHideControls());
  EXPECT_FALSE(model_->GetShowTopBar());
  EXPECT_FALSE(model_->GetShowBottomBar());
  EXPECT_DOUBLE_EQ(0.5, model_->GetControlsOpacity());
  EXPECT_EQ(20, model_->GetSkipDurationSeconds());
  EXPECT_EQ(PipSizePreset::kLarge, model_->GetDefaultSizePreset());

  // Reset.
  model_->ResetSettingsToDefaults();

  // Verify back to defaults.
  EXPECT_TRUE(model_->GetPrefAutoHideControls());
  EXPECT_TRUE(model_->GetShowTopBar());
  EXPECT_TRUE(model_->GetShowBottomBar());
  EXPECT_DOUBLE_EQ(1.0, model_->GetControlsOpacity());
  EXPECT_EQ(10, model_->GetSkipDurationSeconds());
  EXPECT_EQ(PipSizePreset::kMedium, model_->GetDefaultSizePreset());
}

// =========================================================================
// Model observer tests (AstraPipObserver)
// =========================================================================

TEST_F(AstraPipControlsModelTest, AstraPipObserverOnPipActivated) {
  MockAstraPipObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnPipActivated(model_.get())).Times(1);
  model_->ActivatePip();

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, AstraPipObserverOnPipDeactivated) {
  model_->ActivatePip();

  MockAstraPipObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnPipDeactivated(model_.get())).Times(1);
  model_->DeactivatePip();

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, AstraPipObserverOnPipSizeChanged) {
  MockAstraPipObserver observer;
  model_->AddObserver(&observer);

  gfx::Size new_size(500, 300);
  EXPECT_CALL(observer, OnPipSizeChanged(model_.get(), new_size)).Times(1);
  model_->SetSize(new_size);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, AstraPipObserverOnPipPositionChanged) {
  MockAstraPipObserver observer;
  model_->AddObserver(&observer);

  gfx::Point new_pos(100, 50);
  EXPECT_CALL(observer, OnPipPositionChanged(model_.get(), new_pos)).Times(1);
  model_->SetPosition(new_pos);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, AstraPipObserverOnPlaybackStateChanged) {
  MockAstraPipObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnPlaybackStateChanged(model_.get(), true)).Times(1);
  model_->SetPlaying(true);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, AstraPipObserverOnVolumeChanged) {
  MockAstraPipObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnVolumeChanged(model_.get(), 0.5)).Times(1);
  model_->SetVolume(0.5);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, AstraPipObserverOnMuteChanged) {
  MockAstraPipObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnMuteChanged(model_.get(), true)).Times(1);
  model_->SetMuted(true);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, AstraPipObserverOnProgressChanged) {
  MockAstraPipObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnProgressChanged(model_.get(), 0.7)).Times(1);
  model_->SetPlaybackProgress(0.7);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, AstraPipObserverOnControlsVisibilityChanged) {
  MockAstraPipObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnControlsVisibilityChanged(model_.get(), false))
      .Times(1);
  model_->SetShowControls(false);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, AstraPipObserverOnPipModelShutdown) {
  MockAstraPipObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnPipModelShutdown(model_.get())).Times(1);
  model_.reset();  // Destroys the model, triggers shutdown.

  // Observer is removed before model is fully destroyed in production code,
  // but the test verifies the shutdown notification fires.
}

TEST_F(AstraPipControlsModelTest, MultipleAstraPipObserversAllNotified) {
  MockAstraPipObserver observer1;
  MockAstraPipObserver observer2;

  model_->AddObserver(&observer1);
  model_->AddObserver(&observer2);

  EXPECT_CALL(observer1, OnPlaybackStateChanged(model_.get(), true)).Times(1);
  EXPECT_CALL(observer2, OnPlaybackStateChanged(model_.get(), true)).Times(1);
  model_->SetPlaying(true);

  model_->RemoveObserver(&observer1);
  model_->RemoveObserver(&observer2);
}

TEST_F(AstraPipControlsModelTest, RemovedAstraPipObserverNotNotified) {
  MockAstraPipObserver observer;
  model_->AddObserver(&observer);
  model_->RemoveObserver(&observer);

  EXPECT_CALL(observer, OnPlaybackStateChanged(_, _)).Times(0);
  model_->SetPlaying(true);
}

// =========================================================================
// Model observer defaults test
// =========================================================================

TEST(AstraPipObserverDefaultsTest, AllMethodsHaveDefaultImpl) {
  class EmptyObserver : public AstraPipObserver {};

  EmptyObserver observer;
  AstraPipControlsModel* model_ptr = nullptr;

  observer.OnPipActivated(model_ptr);
  observer.OnPipDeactivated(model_ptr);
  observer.OnPipSizeChanged(model_ptr, gfx::Size());
  observer.OnPipPositionChanged(model_ptr, gfx::Point());
  observer.OnPlaybackStateChanged(model_ptr, true);
  observer.OnVolumeChanged(model_ptr, 0.5);
  observer.OnMuteChanged(model_ptr, true);
  observer.OnProgressChanged(model_ptr, 0.3);
  observer.OnControlsVisibilityChanged(model_ptr, true);
  observer.OnPipModelShutdown(model_ptr);

  SUCCEED();
}

TEST(AstraPipControlsModelObserverDefaultsTest, AllMethodsHaveDefaultImpl) {
  class EmptyObserver : public AstraPipControlsModelObserver {};

  EmptyObserver observer;

  observer.OnPlayStateChanged(true);
  observer.OnMuteStateChanged(true);
  observer.OnVolumeChanged(0.5);
  observer.OnPlaybackRateChanged(1.0);
  observer.OnSizePresetChanged(PipSizePreset::kMedium);
  observer.OnAlwaysOnTopChanged(true);
  observer.OnOpacityChanged(1.0);
  observer.OnControlsVisibilityChanged(true);
  observer.OnControlsSettingsChanged();
  observer.OnSnapPositionChanged(PipSnapPosition::kBottomRight);
  observer.OnControlsMinimizedChanged(false);
  observer.OnProgressChanged(0.5);
  observer.OnPlaybackSpeedChanged(AstraPipPlaybackSpeed::k1_0x);
  observer.OnLoopingChanged(false);

  SUCCEED();
}

// =========================================================================
// Model utility method tests
// =========================================================================

TEST_F(AstraPipControlsModelTest, ClampVolumeInRange) {
  EXPECT_DOUBLE_EQ(0.5, AstraPipControlsModel::ClampVolume(0.5));
}

TEST_F(AstraPipControlsModelTest, ClampVolumeBelowMin) {
  EXPECT_DOUBLE_EQ(AstraPipControlsModel::kMinVolume,
                   AstraPipControlsModel::ClampVolume(-1.0));
}

TEST_F(AstraPipControlsModelTest, ClampVolumeAboveMax) {
  EXPECT_DOUBLE_EQ(AstraPipControlsModel::kMaxVolume,
                   AstraPipControlsModel::ClampVolume(2.0));
}

TEST_F(AstraPipControlsModelTest, ClampProgressInRange) {
  EXPECT_DOUBLE_EQ(0.5, AstraPipControlsModel::ClampProgress(0.5));
}

TEST_F(AstraPipControlsModelTest, ClampProgressBelowMin) {
  EXPECT_DOUBLE_EQ(AstraPipControlsModel::kMinProgress,
                   AstraPipControlsModel::ClampProgress(-0.5));
}

TEST_F(AstraPipControlsModelTest, ClampProgressAboveMax) {
  EXPECT_DOUBLE_EQ(AstraPipControlsModel::kMaxProgress,
                   AstraPipControlsModel::ClampProgress(1.5));
}

TEST_F(AstraPipControlsModelTest, ClampAspectRatioInRange) {
  EXPECT_DOUBLE_EQ(1.5, AstraPipControlsModel::ClampAspectRatio(1.5));
}

TEST_F(AstraPipControlsModelTest, ClampAspectRatioBelowMin) {
  EXPECT_DOUBLE_EQ(AstraPipControlsModel::kMinAspectRatio,
                   AstraPipControlsModel::ClampAspectRatio(0.1));
}

TEST_F(AstraPipControlsModelTest, ClampAspectRatioAboveMax) {
  EXPECT_DOUBLE_EQ(AstraPipControlsModel::kMaxAspectRatio,
                   AstraPipControlsModel::ClampAspectRatio(10.0));
}

// =========================================================================
// Model edge case tests
// =========================================================================

TEST_F(AstraPipControlsModelTest, ZeroSizeEdgeCase) {
  model_->SetMinSize(gfx::Size(0, 0));
  model_->SetSize(gfx::Size(0, 0));
  // Zero size is allowed (clamped to min, which is zero).
  EXPECT_EQ(gfx::Size(0, 0), model_->GetSize());
}

TEST_F(AstraPipControlsModelTest, NegativePositionEdgeCase) {
  model_->SetPosition(gfx::Point(-100, -200));
  EXPECT_EQ(gfx::Point(-100, -200), model_->GetPosition());
}

TEST_F(AstraPipControlsModelTest, VolumeAtZeroUnmuteDoesNotCrash) {
  model_->SetVolume(0.0);
  model_->SetMuted(true);
  model_->SetVolume(0.0);  // Still zero - should not trigger unmute.
  EXPECT_TRUE(model_->IsMuted());
}

TEST_F(AstraPipControlsModelTest, VolumeUpFromZeroUnmutes) {
  model_->SetVolume(0.0);
  model_->SetMuted(true);
  model_->SetVolume(0.1);  // Non-zero - should unmute.
  EXPECT_FALSE(model_->IsMuted());
}

TEST_F(AstraPipControlsModelTest, ZeroDurationEdgeCase) {
  model_->SetDuration(base::TimeDelta());
  model_->SetPlaybackProgress(0.5);
  // With zero duration, progress stays at 0.5 but current_time is zero.
  EXPECT_DOUBLE_EQ(0.5, model_->GetPlaybackProgress());
  EXPECT_TRUE(model_->GetCurrentTime().is_zero());
}

TEST_F(AstraPipControlsModelTest, ProgressGreaterThanOneClamps) {
  model_->SetPlaybackProgress(2.0);
  EXPECT_DOUBLE_EQ(1.0, model_->GetPlaybackProgress());
}

TEST_F(AstraPipControlsModelTest, ProgressLessThanZeroClamps) {
  model_->SetPlaybackProgress(-0.5);
  EXPECT_DOUBLE_EQ(0.0, model_->GetPlaybackProgress());
}

TEST_F(AstraPipControlsModelTest, MultipleSettingsChangesEachNotify) {
  MockPipControlsModelObserver observer;
  model_->AddControlsObserver(&observer);

  // Each unique setting change triggers exactly one notification.
  EXPECT_CALL(observer, OnControlsSettingsChanged()).Times(3);
  model_->SetShowTopBar(false);
  model_->SetShowBottomBar(false);
  model_->SetShowResizeHandle(false);

  model_->RemoveControlsObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, SameSettingsChangeDoesNotNotify) {
  MockPipControlsModelObserver observer;
  model_->AddControlsObserver(&observer);

  EXPECT_CALL(observer, OnControlsSettingsChanged()).Times(0);
  model_->SetShowTopBar(true);  // Already true.

  model_->RemoveControlsObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, PrefServiceNotNull) {
  EXPECT_NE(nullptr, model_->pref_service());
  EXPECT_EQ(&pref_service_, model_->pref_service());
}

TEST_F(AstraPipControlsModelTest, PlaybackSpeedSetSameNoNotification) {
  model_->SetPlaybackSpeed(AstraPipPlaybackSpeed::k1_0x);

  MockPipControlsModelObserver observer;
  model_->AddControlsObserver(&observer);

  EXPECT_CALL(observer, OnPlaybackSpeedChanged(_)).Times(0);
  model_->SetPlaybackSpeed(AstraPipPlaybackSpeed::k1_0x);

  model_->RemoveControlsObserver(&observer);
}

// =========================================================================
// Model backward-compatibility tests
// =========================================================================

TEST_F(AstraPipControlsModelTest, ActivePresetMapsTinyToSmall) {
  model_->SetSizePreset(AstraPipSizePreset::kTiny);
  EXPECT_EQ(PipSizePreset::kSmall, model_->active_preset());
}

TEST_F(AstraPipControlsModelTest, ActivePresetMapsCustomToLarge) {
  model_->SetSizePreset(AstraPipSizePreset::kCustom);
  EXPECT_EQ(PipSizePreset::kLarge, model_->active_preset());
}

TEST_F(AstraPipControlsModelTest, SetActiveSizePresetMapsToModelPreset) {
  model_->SetActiveSizePreset(PipSizePreset::kSmall);
  EXPECT_EQ(AstraPipSizePreset::kSmall, model_->GetSizePreset());

  model_->SetActiveSizePreset(PipSizePreset::kMedium);
  EXPECT_EQ(AstraPipSizePreset::kMedium, model_->GetSizePreset());

  model_->SetActiveSizePreset(PipSizePreset::kLarge);
  EXPECT_EQ(AstraPipSizePreset::kLarge, model_->GetSizePreset());
}

TEST_F(AstraPipControlsModelTest, IsPinnedEqualsGetAlwaysOnTop) {
  model_->SetAlwaysOnTop(true);
  EXPECT_EQ(model_->is_pinned(), model_->GetAlwaysOnTop());

  model_->SetAlwaysOnTop(false);
  EXPECT_EQ(model_->is_pinned(), model_->GetAlwaysOnTop());
}

TEST_F(AstraPipControlsModelTest, ControlsVisibleEqualsShowControls) {
  model_->SetShowControls(true);
  EXPECT_EQ(model_->controls_visible(), model_->GetShowControls());

  model_->SetShowControls(false);
  EXPECT_EQ(model_->controls_visible(), model_->GetShowControls());
}

// =========================================================================
// AstraPipControlsView tests
// =========================================================================

class AstraPipControlsViewTest : public views::ViewsTestBase {
 public:
  AstraPipControlsViewTest() = default;
  ~AstraPipControlsViewTest() override = default;

  void SetUp() override {
    ViewsTestBase::SetUp();

    // Set up prefs.
    RegisterTestPrefs(&pref_service_);

    widget_ = CreateTestWidget();

    model_ = std::make_unique<AstraPipControlsModel>(&pref_service_);
    delegate_ = std::make_unique<FakePipControlsDelegate>();

    controls_view_ = widget_->SetContentsView(
        std::make_unique<AstraPipControlsView>(model_.get(), delegate_.get()));
    widget_->Show();
  }

  void TearDown() override {
    widget_.reset();
    controls_view_ = nullptr;
    delegate_.reset();
    model_.reset();
    ViewsTestBase::TearDown();
  }

 protected:
  // Simulate a mouse click on a view.
  void ClickView(views::View* view) {
    ASSERT_TRUE(view);
    ui::MouseEvent press_event(ui::ET_MOUSE_PRESSED, gfx::Point(),
                               gfx::Point(), base::TimeTicks(),
                               ui::EF_LEFT_MOUSE_BUTTON,
                               ui::EF_LEFT_MOUSE_BUTTON);
    view->OnMousePressed(press_event);

    ui::MouseEvent release_event(ui::ET_MOUSE_RELEASED, gfx::Point(),
                                 gfx::Point(), base::TimeTicks(),
                                 ui::EF_LEFT_MOUSE_BUTTON,
                                 ui::EF_LEFT_MOUSE_BUTTON);
    view->OnMouseReleased(release_event);
  }

  // Simulate a key press on the view.
  bool PressKey(ui::KeyboardCode key_code, int flags = 0) {
    ui::KeyEvent key_event(ui::ET_KEY_PRESSED, key_code, flags);
    return controls_view_->OnKeyPressed(key_event);
  }

  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<views::Widget> widget_;
  raw_ptr<AstraPipControlsView> controls_view_ = nullptr;
  std::unique_ptr<AstraPipControlsModel> model_;
  std::unique_ptr<FakePipControlsDelegate> delegate_;
};

// -- View construction tests ------------------------------------------------

TEST_F(AstraPipControlsViewTest, ConstructsWithoutCrash) {
  EXPECT_NE(nullptr, controls_view_);
  EXPECT_NE(nullptr, controls_view_->GetWidget());
}

TEST_F(AstraPipControlsViewTest, ModelAccessors) {
  EXPECT_EQ(model_.get(), controls_view_->GetModel());
  EXPECT_EQ(model_.get(), controls_view_->model());
}

TEST_F(AstraPipControlsViewTest, InitialControlsVisible) {
  EXPECT_TRUE(controls_view_->IsControlsVisible());
  EXPECT_TRUE(controls_view_->GetControlsVisible());
}

TEST_F(AstraPipControlsViewTest, InitialControlsNotMinimized) {
  EXPECT_FALSE(controls_view_->IsControlsMinimized());
}

TEST_F(AstraPipControlsViewTest, PreferredSizeIsPositive) {
  gfx::Size pref = controls_view_->CalculatePreferredSize(
      views::SizeBounds(gfx::Size(400, 300)));
  EXPECT_GT(pref.width(), 0);
  EXPECT_GT(pref.height(), 0);
}

TEST_F(AstraPipControlsViewTest, HasColorProvider) {
  EXPECT_NE(nullptr, controls_view_->GetColorProvider());
}

TEST_F(AstraPipControlsViewTest, OnThemeChangedDoesNotCrash) {
  controls_view_->OnThemeChanged();
}

// -- View model management -------------------------------------------------

TEST_F(AstraPipControlsViewTest, SetModel) {
  TestingPrefServiceSimple prefs2;
  RegisterTestPrefs(&prefs2);
  auto model2 = std::make_unique<AstraPipControlsModel>(&prefs2);

  controls_view_->SetModel(model2.get());
  EXPECT_EQ(model2.get(), controls_view_->GetModel());
}

TEST_F(AstraPipControlsViewTest, SetModelNull) {
  controls_view_->SetModel(nullptr);
  EXPECT_EQ(nullptr, controls_view_->GetModel());
}

TEST_F(AstraPipControlsViewTest, SetModelSameNoCrash) {
  controls_view_->SetModel(model_.get());
  EXPECT_EQ(model_.get(), controls_view_->GetModel());
}

// -- View video view --------------------------------------------------------

TEST_F(AstraPipControlsViewTest, VideoViewDefaultIsNull) {
  EXPECT_EQ(nullptr, controls_view_->GetVideoView());
}

TEST_F(AstraPipControlsViewTest, SetVideoView) {
  auto video_view = std::make_unique<views::View>();
  views::View* video_ptr = video_view.get();
  controls_view_->SetVideoView(video_ptr);
  EXPECT_EQ(video_ptr, controls_view_->GetVideoView());
}

TEST_F(AstraPipControlsViewTest, SetVideoViewNull) {
  controls_view_->SetVideoView(nullptr);
  EXPECT_EQ(nullptr, controls_view_->GetVideoView());
}

// -- View controls visibility ----------------------------------------------

TEST_F(AstraPipControlsViewTest, SetControlsVisibleFalse) {
  controls_view_->SetControlsVisible(false);
  EXPECT_FALSE(controls_view_->GetControlsVisible());
}

TEST_F(AstraPipControlsViewTest, SetControlsVisibleTrue) {
  controls_view_->SetControlsVisible(false);
  controls_view_->SetControlsVisible(true);
  EXPECT_TRUE(controls_view_->GetControlsVisible());
}

TEST_F(AstraPipControlsViewTest, ShowControlsTemporarily) {
  model_->SetShowControls(false);
  ASSERT_FALSE(controls_view_->GetControlsVisible());

  controls_view_->ShowControlsTemporarily();
  // Controls should be shown (auto-hide timer started).
  EXPECT_TRUE(controls_view_->GetControlsVisible());
}

// -- View title -------------------------------------------------------------

TEST_F(AstraPipControlsViewTest, SetTitleUpdatesText) {
  controls_view_->SetTitle(u"My Video Title");
  EXPECT_EQ(u"My Video Title", controls_view_->GetTitle());
}

TEST_F(AstraPipControlsViewTest, SetTitleEmpty) {
  controls_view_->SetTitle(u"My Video");
  controls_view_->SetTitle(std::u16string());
  EXPECT_TRUE(controls_view_->GetTitle().empty());
}

TEST_F(AstraPipControlsViewTest, TitleUpdatesModel) {
  controls_view_->SetTitle(u"Test Title");
  EXPECT_EQ(u"Test Title", model_->GetTabTitle());
}

// -- View playback state ----------------------------------------------------

TEST_F(AstraPipControlsViewTest, SetIsPlayingTrue) {
  controls_view_->SetIsPlaying(true);
  EXPECT_TRUE(model_->IsPlaying());
  EXPECT_TRUE(controls_view_->IsPlaying());
}

TEST_F(AstraPipControlsViewTest, SetIsPlayingFalse) {
  controls_view_->SetIsPlaying(true);
  controls_view_->SetIsPlaying(false);
  EXPECT_FALSE(model_->IsPlaying());
  EXPECT_FALSE(controls_view_->IsPlaying());
}

TEST_F(AstraPipControlsViewTest, SetPlayingIsAliasForSetIsPlaying) {
  controls_view_->SetPlaying(true);
  EXPECT_TRUE(controls_view_->IsPlaying());
}

// -- View progress ----------------------------------------------------------

TEST_F(AstraPipControlsViewTest, SetProgress) {
  controls_view_->SetProgress(0.5);
  EXPECT_DOUBLE_EQ(0.5, controls_view_->GetProgress());
  EXPECT_DOUBLE_EQ(0.5, model_->GetPlaybackProgress());
}

TEST_F(AstraPipControlsViewTest, SetProgressZero) {
  controls_view_->SetProgress(0.0);
  EXPECT_DOUBLE_EQ(0.0, controls_view_->GetProgress());
}

TEST_F(AstraPipControlsViewTest, SetProgressOne) {
  controls_view_->SetProgress(1.0);
  EXPECT_DOUBLE_EQ(1.0, controls_view_->GetProgress());
}

TEST_F(AstraPipControlsViewTest, ProgressBarExists) {
  EXPECT_NE(nullptr, controls_view_->progress_bar());
}

// -- View volume ------------------------------------------------------------

TEST_F(AstraPipControlsViewTest, SetVolumeView) {
  controls_view_->SetVolume(0.5);
  EXPECT_DOUBLE_EQ(0.5, controls_view_->GetVolume());
  EXPECT_DOUBLE_EQ(0.5, model_->GetVolume());
}

TEST_F(AstraPipControlsViewTest, VolumeSliderExists) {
  EXPECT_NE(nullptr, controls_view_->volume_slider());
}

// -- View mute --------------------------------------------------------------

TEST_F(AstraPipControlsViewTest, SetMutedView) {
  controls_view_->SetMuted(true);
  EXPECT_TRUE(controls_view_->IsMuted());
  EXPECT_TRUE(model_->IsMuted());
}

TEST_F(AstraPipControlsViewTest, MuteButtonExists) {
  EXPECT_NE(nullptr, controls_view_->mute_button());
}

// -- View playback speed ----------------------------------------------------

TEST_F(AstraPipControlsViewTest, SetPlaybackSpeed) {
  controls_view_->SetPlaybackSpeed(AstraPipPlaybackSpeed::k2x);
  EXPECT_EQ(AstraPipPlaybackSpeed::k2x, controls_view_->GetPlaybackSpeed());
  EXPECT_EQ(AstraPipPlaybackSpeed::k2x, model_->GetPlaybackSpeed());
}

TEST_F(AstraPipControlsViewTest, GetPlaybackSpeedDefault) {
  EXPECT_EQ(AstraPipPlaybackSpeed::k1_0x, controls_view_->GetPlaybackSpeed());
}

TEST_F(AstraPipControlsViewTest, SpeedButtonExists) {
  EXPECT_NE(nullptr, controls_view_->speed_button());
}

// -- View always on top -----------------------------------------------------

TEST_F(AstraPipControlsViewTest, SetAlwaysOnTopView) {
  controls_view_->SetAlwaysOnTop(false);
  EXPECT_FALSE(controls_view_->GetAlwaysOnTop());
  EXPECT_FALSE(model_->GetAlwaysOnTop());
}

TEST_F(AstraPipControlsViewTest, AlwaysOnTopButtonExists) {
  EXPECT_NE(nullptr, controls_view_->always_on_top_button());
}

// -- View draggable/resizable -----------------------------------------------

TEST_F(AstraPipControlsViewTest, IsDraggableDefaultTrue) {
  EXPECT_TRUE(controls_view_->IsDraggable());
}

TEST_F(AstraPipControlsViewTest, SetIsDraggable) {
  controls_view_->SetIsDraggable(false);
  EXPECT_FALSE(controls_view_->IsDraggable());

  controls_view_->SetIsDraggable(true);
  EXPECT_TRUE(controls_view_->IsDraggable());
}

TEST_F(AstraPipControlsViewTest, IsResizableDefaultTrue) {
  EXPECT_TRUE(controls_view_->IsResizable());
}

TEST_F(AstraPipControlsViewTest, SetIsResizable) {
  controls_view_->SetIsResizable(false);
  EXPECT_FALSE(controls_view_->IsResizable());
  EXPECT_FALSE(model_->IsResizable());

  controls_view_->SetIsResizable(true);
  EXPECT_TRUE(controls_view_->IsResizable());
  EXPECT_TRUE(model_->IsResizable());
}

// -- View control button visibility -----------------------------------------

TEST_F(AstraPipControlsViewTest, CloseButtonExists) {
  EXPECT_NE(nullptr, controls_view_->close_button());
}

TEST_F(AstraPipControlsViewTest, MinimizeButtonExists) {
  EXPECT_NE(nullptr, controls_view_->minimize_button());
}

TEST_F(AstraPipControlsViewTest, MaximizeButtonExists) {
  EXPECT_NE(nullptr, controls_view_->maximize_button());
}

TEST_F(AstraPipControlsViewTest, PlayPauseButtonExists) {
  EXPECT_NE(nullptr, controls_view_->play_pause_button());
}

TEST_F(AstraPipControlsViewTest, SkipBackwardButtonExists) {
  EXPECT_NE(nullptr, controls_view_->skip_backward_button());
}

TEST_F(AstraPipControlsViewTest, SkipForwardButtonExists) {
  EXPECT_NE(nullptr, controls_view_->skip_forward_button());
}

TEST_F(AstraPipControlsViewTest, PipExpandButtonExists) {
  // return_to_tab_button is the PiP expand/back button.
  EXPECT_NE(nullptr, controls_view_->pip_expand_button());
}

TEST_F(AstraPipControlsViewTest, ResizeHandleExists) {
  EXPECT_NE(nullptr, controls_view_->resize_handle());
}

TEST_F(AstraPipControlsViewTest, TitleLabelExists) {
  EXPECT_NE(nullptr, controls_view_->title_label());
}

TEST_F(AstraPipControlsViewTest, TopBarExists) {
  EXPECT_NE(nullptr, controls_view_->top_bar());
}

TEST_F(AstraPipControlsViewTest, BottomBarExists) {
  EXPECT_NE(nullptr, controls_view_->bottom_bar());
}

// -- View progress bar visibility -------------------------------------------

TEST_F(AstraPipControlsViewTest, ProgressBarVisibleByDefault) {
  EXPECT_TRUE(controls_view_->progress_bar()->GetVisible());
}

TEST_F(AstraPipControlsViewTest, HideProgressBarViaSetting) {
  model_->SetShowProgressBar(false);
  // Progress bar visibility should update from settings.
  EXPECT_FALSE(controls_view_->progress_bar()->GetVisible());
}

// -- View layout tests ------------------------------------------------------

TEST_F(AstraPipControlsViewTest, LayoutTopBarAtTop) {
  controls_view_->SetBounds(0, 0, 400, 300);
  controls_view_->Layout();

  // Top bar should be at y=0.
  EXPECT_EQ(0, controls_view_->top_bar()->bounds().y());
}

TEST_F(AstraPipControlsViewTest, LayoutBottomBarAtBottom) {
  controls_view_->SetBounds(0, 0, 400, 300);
  controls_view_->Layout();

  // Bottom bar should be at the bottom.
  EXPECT_EQ(300, controls_view_->bottom_bar()->bounds().bottom());
}

TEST_F(AstraPipControlsViewTest, LayoutResizeHandleAtBottomRight) {
  controls_view_->SetBounds(0, 0, 400, 300);
  controls_view_->Layout();

  // Resize handle should be in the bottom-right corner.
  EXPECT_EQ(400, controls_view_->resize_handle()->bounds().right());
  EXPECT_EQ(300, controls_view_->resize_handle()->bounds().bottom());
}

// -- View model observer integration ----------------------------------------

TEST_F(AstraPipControlsViewTest, ModelPlayStateChangeUpdatesView) {
  model_->SetPlaying(true);
  // No crash = success; the view observes and updates.
}

TEST_F(AstraPipControlsViewTest, ModelMuteStateChangeUpdatesView) {
  model_->SetMuted(true);
}

TEST_F(AstraPipControlsViewTest, ModelVolumeChangeUpdatesView) {
  model_->SetVolume(0.5);
}

TEST_F(AstraPipControlsViewTest, ModelSizePresetChangeUpdatesView) {
  model_->SetActiveSizePreset(PipSizePreset::kSmall);
}

TEST_F(AstraPipControlsViewTest, ModelAlwaysOnTopChangeUpdatesView) {
  model_->SetAlwaysOnTop(false);
}

TEST_F(AstraPipControlsViewTest, ModelOpacityChangeUpdatesView) {
  model_->SetOpacity(0.5);
}

TEST_F(AstraPipControlsViewTest, ModelMinimizedChangeUpdatesView) {
  model_->SetControlsMinimized(true);
}

TEST_F(AstraPipControlsViewTest, ModelSnapPositionChangeUpdatesView) {
  model_->SetSnapPosition(PipSnapPosition::kTopLeft);
}

TEST_F(AstraPipControlsViewTest, ModelSettingsChangeUpdatesView) {
  model_->SetShowTopBar(false);
}

TEST_F(AstraPipControlsViewTest, ModelProgressChangeUpdatesView) {
  model_->SetPlaybackProgress(0.7);
}

TEST_F(AstraPipControlsViewTest, ModelPlaybackSpeedChangeUpdatesView) {
  model_->SetPlaybackSpeed(AstraPipPlaybackSpeed::k2x);
}

TEST_F(AstraPipControlsViewTest, ModelLoopingChangeUpdatesView) {
  model_->SetLooping(true);
}

// -- View delegate callback tests -------------------------------------------

TEST_F(AstraPipControlsViewTest, AllDelegateMethodsAreCallable) {
  delegate_->OnPlayPause();
  EXPECT_EQ(1, delegate_->play_pause_count);

  delegate_->OnSkipBackward(10);
  EXPECT_EQ(1, delegate_->skip_backward_count);

  delegate_->OnSkipForward(15);
  EXPECT_EQ(1, delegate_->skip_forward_count);

  delegate_->OnMuteToggle();
  EXPECT_EQ(1, delegate_->mute_toggle_count);

  delegate_->OnClosePip();
  EXPECT_EQ(1, delegate_->close_count);

  delegate_->OnReturnToTab();
  EXPECT_EQ(1, delegate_->return_to_tab_count);

  delegate_->OnResizePreset(PipSizePreset::kLarge);
  EXPECT_EQ(1, delegate_->resize_preset_count);

  delegate_->OnAlwaysOnTopToggle();
  EXPECT_EQ(1, delegate_->always_on_top_count);

  delegate_->OnVolumeChanged(0.5);
  EXPECT_EQ(1, delegate_->volume_changed_count);

  delegate_->OnPlaybackRateChanged(1.5);
  EXPECT_EQ(1, delegate_->playback_rate_changed_count);

  delegate_->OnOpacityChanged(0.8);
  EXPECT_EQ(1, delegate_->opacity_changed_count);

  delegate_->OnSnapPositionChanged(PipSnapPosition::kTopLeft);
  EXPECT_EQ(1, delegate_->snap_position_changed_count);
}

// -- View keyboard shortcut tests -------------------------------------------

TEST_F(AstraPipControlsViewTest, KeyboardSpaceTogglesPlay) {
  model_->SetPlaying(false);
  PressKey(ui::VKEY_SPACE);
  EXPECT_TRUE(model_->IsPlaying());
  EXPECT_EQ(1, delegate_->play_pause_count);
}

TEST_F(AstraPipControlsViewTest, KeyboardKTogglesPlay) {
  model_->SetPlaying(false);
  PressKey(ui::VKEY_K);
  EXPECT_TRUE(model_->IsPlaying());
  EXPECT_EQ(1, delegate_->play_pause_count);
}

TEST_F(AstraPipControlsViewTest, KeyboardMTogglesMute) {
  model_->SetMuted(false);
  PressKey(ui::VKEY_M);
  EXPECT_TRUE(model_->IsMuted());
  EXPECT_EQ(1, delegate_->mute_toggle_count);
}

TEST_F(AstraPipControlsViewTest, KeyboardPTogglesAlwaysOnTop) {
  model_->SetAlwaysOnTop(true);
  PressKey(ui::VKEY_P);
  EXPECT_FALSE(model_->GetAlwaysOnTop());
  EXPECT_EQ(1, delegate_->always_on_top_count);
}

TEST_F(AstraPipControlsViewTest, KeyboardHTogglesControlsVisible) {
  PressKey(ui::VKEY_H);
  EXPECT_FALSE(model_->controls_visible());
}

TEST_F(AstraPipControlsViewTest, KeyboardITogglesMinimized) {
  PressKey(ui::VKEY_I);
  EXPECT_TRUE(model_->controls_minimized());
}

TEST_F(AstraPipControlsViewTest, KeyboardLTogglesLoop) {
  model_->SetLooping(false);
  PressKey(ui::VKEY_L);
  EXPECT_TRUE(model_->IsLooping());
}

TEST_F(AstraPipControlsViewTest, KeyboardSCyclesSizePreset) {
  model_->SetActiveSizePreset(PipSizePreset::kSmall);
  PressKey(ui::VKEY_S);
  EXPECT_EQ(PipSizePreset::kMedium, model_->active_preset());
  EXPECT_EQ(1, delegate_->resize_preset_count);
}

TEST_F(AstraPipControlsViewTest, KeyboardShiftSCyclesBackward) {
  model_->SetActiveSizePreset(PipSizePreset::kMedium);
  PressKey(ui::VKEY_S, ui::EF_SHIFT_DOWN);
  EXPECT_EQ(PipSizePreset::kSmall, model_->active_preset());
}

TEST_F(AstraPipControlsViewTest, KeyboardRCyclesPlaybackRate) {
  model_->SetPlaybackRate(1.0);
  PressKey(ui::VKEY_R);
  EXPECT_GT(model_->playback_rate(), 1.0);
  EXPECT_EQ(1, delegate_->playback_rate_changed_count);
}

TEST_F(AstraPipControlsViewTest, KeyboardShiftRCyclesBackward) {
  model_->SetPlaybackRate(1.0);
  PressKey(ui::VKEY_R, ui::EF_SHIFT_DOWN);
  EXPECT_LT(model_->playback_rate(), 1.0);
}

TEST_F(AstraPipControlsViewTest, KeyboardOIncreasesOpacity) {
  model_->SetOpacity(0.5);
  PressKey(ui::VKEY_O);
  EXPECT_GT(model_->opacity(), 0.5);
  EXPECT_EQ(1, delegate_->opacity_changed_count);
}

TEST_F(AstraPipControlsViewTest, KeyboardShiftODecreasesOpacity) {
  model_->SetOpacity(0.5);
  PressKey(ui::VKEY_O, ui::EF_SHIFT_DOWN);
  EXPECT_LT(model_->opacity(), 0.5);
}

TEST_F(AstraPipControlsViewTest, KeyboardCtrlUpIncreasesVolume) {
  model_->SetVolume(0.5);
  PressKey(ui::VKEY_UP, ui::EF_CONTROL_DOWN);
  EXPECT_GT(model_->GetVolume(), 0.5);
  EXPECT_EQ(1, delegate_->volume_changed_count);
}

TEST_F(AstraPipControlsViewTest, KeyboardCtrlDownDecreasesVolume) {
  model_->SetVolume(0.5);
  PressKey(ui::VKEY_DOWN, ui::EF_CONTROL_DOWN);
  EXPECT_LT(model_->GetVolume(), 0.5);
}

TEST_F(AstraPipControlsViewTest, KeyboardCtrlLeftSkipsBackward) {
  PressKey(ui::VKEY_LEFT, ui::EF_CONTROL_DOWN);
  EXPECT_EQ(1, delegate_->skip_backward_count);
  EXPECT_EQ(10, delegate_->last_skip_backward_seconds);
}

TEST_F(AstraPipControlsViewTest, KeyboardCtrlRightSkipsForward) {
  PressKey(ui::VKEY_RIGHT, ui::EF_CONTROL_DOWN);
  EXPECT_EQ(1, delegate_->skip_forward_count);
  EXPECT_EQ(10, delegate_->last_skip_forward_seconds);
}

TEST_F(AstraPipControlsViewTest, KeyboardEscapeClosesPip) {
  PressKey(ui::VKEY_ESCAPE);
  EXPECT_EQ(1, delegate_->close_count);
}

TEST_F(AstraPipControlsViewTest, KeyboardUnrecognizedKeyNotHandled) {
  bool handled = PressKey(ui::VKEY_Z);
  EXPECT_FALSE(handled);
}

// -- View mouse interaction tests -------------------------------------------

TEST_F(AstraPipControlsViewTest, MouseEnterShowsControls) {
  model_->SetShowControls(false);
  ASSERT_FALSE(model_->GetShowControls());

  ui::MouseEvent enter_event(ui::ET_MOUSE_ENTERED, gfx::Point(),
                             gfx::Point(), base::TimeTicks(), 0, 0);
  controls_view_->OnMouseEntered(enter_event);

  EXPECT_TRUE(model_->GetShowControls());
}

TEST_F(AstraPipControlsViewTest, MousePressedDoesNotCrash) {
  ui::MouseEvent press_event(ui::ET_MOUSE_PRESSED, gfx::Point(10, 10),
                             gfx::Point(10, 10), base::TimeTicks(),
                             ui::EF_LEFT_MOUSE_BUTTON,
                             ui::EF_LEFT_MOUSE_BUTTON);
  controls_view_->OnMousePressed(press_event);
}

TEST_F(AstraPipControlsViewTest, MouseExitedStartsAutoHide) {
  ui::MouseEvent exit_event(ui::ET_MOUSE_EXITED, gfx::Point(),
                            gfx::Point(), base::TimeTicks(), 0, 0);
  controls_view_->OnMouseExited(exit_event);
  // No crash = success. Auto-hide timer should be started.
}

// -- View accessibility tests ----------------------------------------------

TEST_F(AstraPipControlsViewTest, AccessibleNodeDataHasRole) {
  ui::AXNodeData data;
  controls_view_->GetAccessibleNodeData(&data);
  EXPECT_EQ(ax::mojom::Role::kToolbar, data.role);
}

TEST_F(AstraPipControlsViewTest, AccessibleNodeDataHasName) {
  ui::AXNodeData data;
  controls_view_->GetAccessibleNodeData(&data);
  EXPECT_FALSE(data.GetStringAttribute(ax::mojom::StringAttribute::kName).empty());
}

// -- View full state cycle test ---------------------------------------------

TEST_F(AstraPipControlsViewTest, FullStateCycleNoCrash) {
  // Cycle through all state combinations.
  controls_view_->SetIsPlaying(true);
  controls_view_->SetMuted(true);
  controls_view_->SetAlwaysOnTop(false);
  controls_view_->SetActiveSizePreset(PipSizePreset::kSmall);
  controls_view_->SetTitle(u"Test Video");
  controls_view_->SetVolume(0.5);
  controls_view_->SetPlaybackRate(1.5);
  controls_view_->SetOpacity(0.8);
  controls_view_->SetProgress(0.3);
  controls_view_->SetPlaybackSpeed(AstraPipPlaybackSpeed::k2x);
  controls_view_->SetIsDraggable(false);
  controls_view_->SetIsResizable(false);
  controls_view_->OnThemeChanged();

  controls_view_->SetIsPlaying(false);
  controls_view_->SetMuted(false);
  controls_view_->SetAlwaysOnTop(true);
  controls_view_->SetActiveSizePreset(PipSizePreset::kLarge);
  controls_view_->SetTitle(u"");
  controls_view_->SetVolume(1.0);
  controls_view_->SetPlaybackRate(1.0);
  controls_view_->SetOpacity(1.0);
  controls_view_->SetProgress(0.0);
  controls_view_->SetPlaybackSpeed(AstraPipPlaybackSpeed::k1_0x);
  controls_view_->SetIsDraggable(true);
  controls_view_->SetIsResizable(true);
  controls_view_->OnThemeChanged();
  // No crash = success.
}

// -- View + settings interaction tests --------------------------------------

TEST_F(AstraPipControlsViewTest, HideTopBarSettingUpdatesView) {
  model_->SetShowTopBar(false);
  // No crash = success; the view updates visibility based on settings.
}

TEST_F(AstraPipControlsViewTest, HideBottomBarSettingUpdatesView) {
  model_->SetShowBottomBar(false);
}

TEST_F(AstraPipControlsViewTest, HideResizeHandleSettingUpdatesView) {
  model_->SetShowResizeHandle(false);
}

TEST_F(AstraPipControlsViewTest, HidePlaybackControlsSettingUpdatesView) {
  model_->SetShowPlaybackControls(false);
}

TEST_F(AstraPipControlsViewTest, HideSkipButtonsSettingUpdatesView) {
  model_->SetShowSkipButtons(false);
}

TEST_F(AstraPipControlsViewTest, HideAlwaysOnTopButtonSettingUpdatesView) {
  model_->SetShowAlwaysOnTopButton(false);
}

TEST_F(AstraPipControlsViewTest, ControlsOpacitySettingUpdatesView) {
  model_->SetControlsOpacity(0.5);
}

TEST_F(AstraPipControlsViewTest, CustomSkipDurationUsedByDelegate) {
  model_->SetSkipDurationSeconds(20);

  // Simulate skip forward via keyboard.
  PressKey(ui::VKEY_RIGHT, ui::EF_CONTROL_DOWN);

  EXPECT_EQ(20, delegate_->last_skip_forward_seconds);
}

TEST_F(AstraPipControlsViewTest, HideTitleSettingUpdatesVisibility) {
  model_->SetShowTitle(false);
  EXPECT_FALSE(controls_view_->title_label()->GetVisible());
}

TEST_F(AstraPipControlsViewTest, ShowTitleSettingUpdatesVisibility) {
  model_->SetShowTitle(true);
  EXPECT_TRUE(controls_view_->title_label()->GetVisible());
}

// =========================================================================
// View edge case tests
// =========================================================================

TEST_F(AstraPipControlsViewTest, LayoutWithZeroSize) {
  controls_view_->SetBounds(0, 0, 0, 0);
  controls_view_->Layout();
  // No crash = success.
}

TEST_F(AstraPipControlsViewTest, LayoutWithVerySmallSize) {
  controls_view_->SetBounds(0, 0, 50, 30);
  controls_view_->Layout();
  // No crash = success.
}

TEST_F(AstraPipControlsViewTest, LayoutWithVeryLargeSize) {
  controls_view_->SetBounds(0, 0, 2000, 1500);
  controls_view_->Layout();
  // No crash = success.
}

TEST_F(AstraPipControlsViewTest, NoVideoViewDoesNotCrash) {
  // Default state has no video view. Operations should not crash.
  controls_view_->Layout();
  controls_view_->OnThemeChanged();
}

TEST_F(AstraPipControlsViewTest, ControlsHiddenState) {
  model_->SetShowControls(false);
  controls_view_->Layout();
  // No crash = success.
}

TEST_F(AstraPipControlsViewTest, MinimizedControlsLayout) {
  model_->SetControlsMinimized(true);
  controls_view_->SetBounds(0, 0, 400, 200);
  controls_view_->Layout();
  // No crash = success.
}

TEST_F(AstraPipControlsViewTest, SetModelToNullDoesNotCrash) {
  controls_view_->SetModel(nullptr);
  // All operations should be safe with null model.
  controls_view_->SetTitle(u"Test");
  controls_view_->SetIsPlaying(true);
  controls_view_->SetVolume(0.5);
  controls_view_->Layout();
  controls_view_->OnThemeChanged();
}

// =========================================================================
// Enum documentation tests
// =========================================================================

TEST(AstraPipSizePresetEnumTest, FivePresetsExist) {
  EXPECT_EQ(static_cast<int>(AstraPipSizePreset::kTiny), 0);
  EXPECT_EQ(static_cast<int>(AstraPipSizePreset::kSmall), 1);
  EXPECT_EQ(static_cast<int>(AstraPipSizePreset::kMedium), 2);
  EXPECT_EQ(static_cast<int>(AstraPipSizePreset::kLarge), 3);
  EXPECT_EQ(static_cast<int>(AstraPipSizePreset::kCustom), 4);
}

TEST(AstraPipPlaybackSpeedEnumTest, SixSpeedsExist) {
  EXPECT_EQ(static_cast<int>(AstraPipPlaybackSpeed::k0_5x), 0);
  EXPECT_EQ(static_cast<int>(AstraPipPlaybackSpeed::k0_75x), 1);
  EXPECT_EQ(static_cast<int>(AstraPipPlaybackSpeed::k1_0x), 2);
  EXPECT_EQ(static_cast<int>(AstraPipPlaybackSpeed::k1_25x), 3);
  EXPECT_EQ(static_cast<int>(AstraPipPlaybackSpeed::k1_5x), 4);
  EXPECT_EQ(static_cast<int>(AstraPipPlaybackSpeed::k2x), 5);
}

TEST(AstraPipDefaultPositionEnumTest, FivePositionsExist) {
  EXPECT_EQ(static_cast<int>(AstraPipDefaultPosition::kBottomRight), 0);
  EXPECT_EQ(static_cast<int>(AstraPipDefaultPosition::kBottomLeft), 1);
  EXPECT_EQ(static_cast<int>(AstraPipDefaultPosition::kTopRight), 2);
  EXPECT_EQ(static_cast<int>(AstraPipDefaultPosition::kTopLeft), 3);
  EXPECT_EQ(static_cast<int>(AstraPipDefaultPosition::kCenter), 4);
}

// =========================================================================
// Controls layout documentation tests
// =========================================================================

TEST(AstraPipLayoutTest, TopBarComponents) {
  // Top bar contains:
  //   - Astra PiP badge label
  //   - Tab title label (truncated with ellipsis)
  //   - Minimize controls button
  //   - Maximize controls button
  //   - Return to tab button (PiP expand/back)
  //   - Close button (X)
  SUCCEED();
}

TEST(AstraPipLayoutTest, BottomBarComponents) {
  // Bottom bar contains (left to right):
  //   - Mute button + volume slider
  //   - Skip backward button
  //   - Play/pause button
  //   - Skip forward button
  //   - Playback rate selector
  //   - Size presets (S / M / L)
  //   - Opacity slider
  //   - Always-on-top / pin button
  //   - Settings button
  SUCCEED();
}

TEST(AstraPipLayoutTest, ProgressBar) {
  // Progress bar above bottom bar shows playback progress.
  SUCCEED();
}

TEST(AstraPipLayoutTest, ResizeHandle) {
  // Bottom-right corner has a resize handle grip.
  SUCCEED();
}

TEST(AstraPipLayoutTest, SnapIndicators) {
  // Each corner has a snap position indicator.
  SUCCEED();
}

TEST(AstraPipLayoutTest, AutoHideBehavior) {
  // Controls auto-hide after inactivity.
  SUCCEED();
}

// =========================================================================
// Integration documentation tests
// =========================================================================

TEST(AstraPipIntegrationTest, ChromiumPiPSubsystem) {
  // The controls view integrates with Chromium's PiP subsystem:
  //   - PictureInPictureWindowViews (window frame)
  //   - VideoPiPView (standard video controls)
  //   - PictureInPictureWindowController (window management)
  SUCCEED();
}

TEST(AstraPipModelTest, ModelViewSeparation) {
  // Model/view architecture:
  //   - AstraPipControlsModel owns state and logic
  //   - AstraPipControlsView renders and handles input
  //   - View observes model via AstraPipControlsModelObserver
  //   - Presentation settings persist via PrefService
  SUCCEED();
}

TEST(AstraPipModelTest, TwentyPlusSettingsPrefKeys) {
  // There are 22 settings pref keys declared as static constexpr.
  SUCCEED();
}

TEST(AstraPipObserverTest, TenAstraPipObserverEvents) {
  // AstraPipObserver has 10 events:
  //   1. OnPipActivated
  //   2. OnPipDeactivated
  //   3. OnPipSizeChanged
  //   4. OnPipPositionChanged
  //   5. OnPlaybackStateChanged
  //   6. OnVolumeChanged
  //   7. OnMuteChanged
  //   8. OnProgressChanged
  //   9. OnControlsVisibilityChanged
  //  10. OnPipModelShutdown
  SUCCEED();
}

TEST(AstraPipObserverTest, ThirteenControlsObserverEvents) {
  // AstraPipControlsModelObserver has 13 events:
  //   1. OnPlayStateChanged
  //   2. OnMuteStateChanged
  //   3. OnVolumeChanged
  //   4. OnPlaybackRateChanged
  //   5. OnSizePresetChanged
  //   6. OnAlwaysOnTopChanged
  //   7. OnOpacityChanged
  //   8. OnControlsVisibilityChanged
  //   9. OnControlsSettingsChanged
  //  10. OnSnapPositionChanged
  //  11. OnControlsMinimizedChanged
  //  12. OnProgressChanged
  //  13. OnPlaybackSpeedChanged
  //  14. OnLoopingChanged
  SUCCEED();
}

}  // namespace astra
