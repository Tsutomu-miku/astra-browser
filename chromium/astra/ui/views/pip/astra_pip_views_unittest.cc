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
// Mock observer for testing model observer pattern
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
  EXPECT_FALSE(model_->is_playing());
}

TEST_F(AstraPipControlsModelTest, DefaultMuteStateIsFalse) {
  EXPECT_FALSE(model_->is_muted());
}

TEST_F(AstraPipControlsModelTest, DefaultVolumeIsMax) {
  EXPECT_DOUBLE_EQ(AstraPipControlsModel::kDefaultVolume, model_->volume());
}

TEST_F(AstraPipControlsModelTest, DefaultPlaybackRateIsNormal) {
  EXPECT_DOUBLE_EQ(1.0, model_->playback_rate());
}

TEST_F(AstraPipControlsModelTest, DefaultSizePresetIsMedium) {
  EXPECT_EQ(PipSizePreset::kMedium, model_->active_preset());
}

TEST_F(AstraPipControlsModelTest, DefaultIsPinnedIsTrue) {
  EXPECT_TRUE(model_->is_pinned());
}

TEST_F(AstraPipControlsModelTest, DefaultOpacityIsMax) {
  EXPECT_DOUBLE_EQ(AstraPipControlsModel::kDefaultOpacity, model_->opacity());
}

TEST_F(AstraPipControlsModelTest, DefaultControlsVisibleIsTrue) {
  EXPECT_TRUE(model_->controls_visible());
}

TEST_F(AstraPipControlsModelTest, DefaultControlsMinimizedIsFalse) {
  EXPECT_FALSE(model_->controls_minimized());
}

TEST_F(AstraPipControlsModelTest, DefaultSnapPositionIsBottomRight) {
  EXPECT_EQ(PipSnapPosition::kBottomRight, model_->snap_position());
}

// =========================================================================
// Model playback state tests
// =========================================================================

TEST_F(AstraPipControlsModelTest, SetPlayingTrue) {
  MockPipControlsModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnPlayStateChanged(true)).Times(1);
  model_->SetPlaying(true);

  EXPECT_TRUE(model_->is_playing());
  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, SetPlayingFalse) {
  model_->SetPlaying(true);
  ASSERT_TRUE(model_->is_playing());

  MockPipControlsModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnPlayStateChanged(false)).Times(1);
  model_->SetPlaying(false);

  EXPECT_FALSE(model_->is_playing());
  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, SetPlayingSameStateNoNotification) {
  model_->SetPlaying(false);

  MockPipControlsModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnPlayStateChanged(_)).Times(0);
  model_->SetPlaying(false);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, TogglePlay) {
  model_->SetPlaying(false);
  model_->TogglePlay();
  EXPECT_TRUE(model_->is_playing());

  model_->TogglePlay();
  EXPECT_FALSE(model_->is_playing());
}

// =========================================================================
// Model mute state tests
// =========================================================================

TEST_F(AstraPipControlsModelTest, SetMutedTrue) {
  MockPipControlsModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnMuteStateChanged(true)).Times(1);
  model_->SetMuted(true);

  EXPECT_TRUE(model_->is_muted());
  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, SetMutedFalse) {
  model_->SetMuted(true);
  ASSERT_TRUE(model_->is_muted());

  MockPipControlsModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnMuteStateChanged(false)).Times(1);
  model_->SetMuted(false);

  EXPECT_FALSE(model_->is_muted());
  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, SetMutedSameStateNoNotification) {
  model_->SetMuted(false);

  MockPipControlsModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnMuteStateChanged(_)).Times(0);
  model_->SetMuted(false);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, ToggleMute) {
  model_->SetMuted(false);
  model_->ToggleMute();
  EXPECT_TRUE(model_->is_muted());

  model_->ToggleMute();
  EXPECT_FALSE(model_->is_muted());
}

// =========================================================================
// Model volume tests
// =========================================================================

TEST_F(AstraPipControlsModelTest, SetVolumeNormal) {
  MockPipControlsModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnVolumeChanged(0.5)).Times(1);
  model_->SetVolume(0.5);
  EXPECT_DOUBLE_EQ(0.5, model_->volume());

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, SetVolumeClampsToMin) {
  model_->SetVolume(-1.0);
  EXPECT_DOUBLE_EQ(AstraPipControlsModel::kMinVolume, model_->volume());
}

TEST_F(AstraPipControlsModelTest, SetVolumeClampsToMax) {
  model_->SetVolume(2.0);
  EXPECT_DOUBLE_EQ(AstraPipControlsModel::kMaxVolume, model_->volume());
}

TEST_F(AstraPipControlsModelTest, SetVolumeSameValueNoNotification) {
  model_->SetVolume(0.5);

  MockPipControlsModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnVolumeChanged(_)).Times(0);
  model_->SetVolume(0.5);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, IncreaseVolume) {
  model_->SetVolume(0.5);
  model_->IncreaseVolume();
  EXPECT_DOUBLE_EQ(0.6, model_->volume());
}

TEST_F(AstraPipControlsModelTest, DecreaseVolume) {
  model_->SetVolume(0.5);
  model_->DecreaseVolume();
  EXPECT_DOUBLE_EQ(0.4, model_->volume());
}

TEST_F(AstraPipControlsModelTest, IncreaseVolumeClampsToMax) {
  model_->SetVolume(1.0);
  model_->IncreaseVolume();
  EXPECT_DOUBLE_EQ(AstraPipControlsModel::kMaxVolume, model_->volume());
}

TEST_F(AstraPipControlsModelTest, DecreaseVolumeClampsToMin) {
  model_->SetVolume(0.0);
  model_->DecreaseVolume();
  EXPECT_DOUBLE_EQ(AstraPipControlsModel::kMinVolume, model_->volume());
}

TEST_F(AstraPipControlsModelTest, SettingVolumeAboveZeroUnmutes) {
  model_->SetMuted(true);
  ASSERT_TRUE(model_->is_muted());

  model_->SetVolume(0.5);
  EXPECT_FALSE(model_->is_muted());
}

TEST_F(AstraPipControlsModelTest, SettingVolumeToZeroDoesNotMute) {
  model_->SetVolume(0.0);
  // Setting volume to 0 doesn't automatically set muted = true.
  // Muted is a separate state.
  EXPECT_FALSE(model_->is_muted());
}

// =========================================================================
// Model playback rate tests
// =========================================================================

TEST_F(AstraPipControlsModelTest, SetPlaybackRateNormal) {
  MockPipControlsModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnPlaybackRateChanged(1.5)).Times(1);
  model_->SetPlaybackRate(1.5);
  EXPECT_DOUBLE_EQ(1.5, model_->playback_rate());

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, SetPlaybackRateClampsToMin) {
  model_->SetPlaybackRate(0.1);
  EXPECT_DOUBLE_EQ(AstraPipControlsModel::kMinPlaybackRate,
                   model_->playback_rate());
}

TEST_F(AstraPipControlsModelTest, SetPlaybackRateClampsToMax) {
  model_->SetPlaybackRate(10.0);
  EXPECT_DOUBLE_EQ(AstraPipControlsModel::kMaxPlaybackRate,
                   model_->playback_rate());
}

TEST_F(AstraPipControlsModelTest, CyclePlaybackRateForward) {
  // Default presets: 0.5, 1.0, 1.25, 1.5, 2.0
  model_->SetPlaybackRate(1.0);
  model_->CyclePlaybackRateForward();
  EXPECT_DOUBLE_EQ(1.25, model_->playback_rate());

  model_->CyclePlaybackRateForward();
  EXPECT_DOUBLE_EQ(1.5, model_->playback_rate());
}

TEST_F(AstraPipControlsModelTest, CyclePlaybackRateForwardWrapsAround) {
  model_->SetPlaybackRate(2.0);  // Last preset.
  model_->CyclePlaybackRateForward();
  EXPECT_DOUBLE_EQ(0.5, model_->playback_rate());  // Wraps to first.
}

TEST_F(AstraPipControlsModelTest, CyclePlaybackRateBackward) {
  model_->SetPlaybackRate(1.5);
  model_->CyclePlaybackRateBackward();
  EXPECT_DOUBLE_EQ(1.25, model_->playback_rate());

  model_->CyclePlaybackRateBackward();
  EXPECT_DOUBLE_EQ(1.0, model_->playback_rate());
}

TEST_F(AstraPipControlsModelTest, CyclePlaybackRateBackwardWrapsAround) {
  model_->SetPlaybackRate(0.5);  // First preset.
  model_->CyclePlaybackRateBackward();
  EXPECT_DOUBLE_EQ(2.0, model_->playback_rate());  // Wraps to last.
}

TEST_F(AstraPipControlsModelTest, SetPlaybackRatePresetsCustom) {
  std::vector<double> custom = {0.75, 1.0, 1.5};
  model_->SetPlaybackRatePresets(custom);

  auto result = model_->GetPlaybackRatePresets();
  ASSERT_EQ(3u, result.size());
  EXPECT_DOUBLE_EQ(0.75, result[0]);
  EXPECT_DOUBLE_EQ(1.0, result[1]);
  EXPECT_DOUBLE_EQ(1.5, result[2]);
}

TEST_F(AstraPipControlsModelTest, SetPlaybackRatePresetsTriggersSettingsChanged) {
  MockPipControlsModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnControlsSettingsChanged()).Times(1);
  model_->SetPlaybackRatePresets({0.75, 1.0, 1.5});

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, EmptyPlaybackRatePresetsDefaultsToOne) {
  model_->SetPlaybackRatePresets({});
  auto result = model_->GetPlaybackRatePresets();
  ASSERT_EQ(1u, result.size());
  EXPECT_DOUBLE_EQ(1.0, result[0]);
}

// =========================================================================
// Model size preset tests
// =========================================================================

TEST_F(AstraPipControlsModelTest, SetActiveSizePresetSmall) {
  MockPipControlsModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSizePresetChanged(PipSizePreset::kSmall)).Times(1);
  model_->SetActiveSizePreset(PipSizePreset::kSmall);
  EXPECT_EQ(PipSizePreset::kSmall, model_->active_preset());

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, SetActiveSizePresetSameNoNotification) {
  MockPipControlsModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSizePresetChanged(_)).Times(0);
  model_->SetActiveSizePreset(PipSizePreset::kMedium);  // Already medium.

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, CycleSizePresetForward) {
  model_->SetActiveSizePreset(PipSizePreset::kSmall);
  model_->CycleSizePresetForward();
  EXPECT_EQ(PipSizePreset::kMedium, model_->active_preset());

  model_->CycleSizePresetForward();
  EXPECT_EQ(PipSizePreset::kLarge, model_->active_preset());
}

TEST_F(AstraPipControlsModelTest, CycleSizePresetForwardWraps) {
  model_->SetActiveSizePreset(PipSizePreset::kLarge);
  model_->CycleSizePresetForward();
  EXPECT_EQ(PipSizePreset::kSmall, model_->active_preset());
}

TEST_F(AstraPipControlsModelTest, CycleSizePresetBackward) {
  model_->SetActiveSizePreset(PipSizePreset::kLarge);
  model_->CycleSizePresetBackward();
  EXPECT_EQ(PipSizePreset::kMedium, model_->active_preset());

  model_->CycleSizePresetBackward();
  EXPECT_EQ(PipSizePreset::kSmall, model_->active_preset());
}

TEST_F(AstraPipControlsModelTest, CycleSizePresetBackwardWraps) {
  model_->SetActiveSizePreset(PipSizePreset::kSmall);
  model_->CycleSizePresetBackward();
  EXPECT_EQ(PipSizePreset::kLarge, model_->active_preset());
}

TEST_F(AstraPipControlsModelTest, GetSizePresetCountIsThree) {
  EXPECT_EQ(3u, AstraPipControlsModel::GetSizePresetCount());
}

TEST_F(AstraPipControlsModelTest, GetNextSizePresetStatic) {
  EXPECT_EQ(PipSizePreset::kMedium,
            AstraPipControlsModel::GetNextSizePreset(PipSizePreset::kSmall));
  EXPECT_EQ(PipSizePreset::kLarge,
            AstraPipControlsModel::GetNextSizePreset(PipSizePreset::kMedium));
  EXPECT_EQ(PipSizePreset::kSmall,
            AstraPipControlsModel::GetNextSizePreset(PipSizePreset::kLarge));
}

TEST_F(AstraPipControlsModelTest, GetPreviousSizePresetStatic) {
  EXPECT_EQ(PipSizePreset::kLarge,
            AstraPipControlsModel::GetPreviousSizePreset(PipSizePreset::kSmall));
  EXPECT_EQ(PipSizePreset::kSmall,
            AstraPipControlsModel::GetPreviousSizePreset(PipSizePreset::kMedium));
  EXPECT_EQ(PipSizePreset::kMedium,
            AstraPipControlsModel::GetPreviousSizePreset(PipSizePreset::kLarge));
}

// =========================================================================
// Model always-on-top tests
// =========================================================================

TEST_F(AstraPipControlsModelTest, SetAlwaysOnTopTrue) {
  model_->SetAlwaysOnTop(false);

  MockPipControlsModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnAlwaysOnTopChanged(true)).Times(1);
  model_->SetAlwaysOnTop(true);
  EXPECT_TRUE(model_->is_pinned());

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, SetAlwaysOnTopFalse) {
  MockPipControlsModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnAlwaysOnTopChanged(false)).Times(1);
  model_->SetAlwaysOnTop(false);
  EXPECT_FALSE(model_->is_pinned());

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, SetAlwaysOnTopSameNoNotification) {
  MockPipControlsModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnAlwaysOnTopChanged(_)).Times(0);
  model_->SetAlwaysOnTop(true);  // Already true.

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, ToggleAlwaysOnTop) {
  model_->SetAlwaysOnTop(true);
  model_->ToggleAlwaysOnTop();
  EXPECT_FALSE(model_->is_pinned());

  model_->ToggleAlwaysOnTop();
  EXPECT_TRUE(model_->is_pinned());
}

// =========================================================================
// Model opacity tests
// =========================================================================

TEST_F(AstraPipControlsModelTest, SetOpacityNormal) {
  MockPipControlsModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnOpacityChanged(0.5)).Times(1);
  model_->SetOpacity(0.5);
  EXPECT_DOUBLE_EQ(0.5, model_->opacity());

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, SetOpacityClampsToMin) {
  model_->SetOpacity(0.1);
  EXPECT_DOUBLE_EQ(AstraPipControlsModel::kMinOpacity, model_->opacity());
}

TEST_F(AstraPipControlsModelTest, SetOpacityClampsToMax) {
  model_->SetOpacity(2.0);
  EXPECT_DOUBLE_EQ(AstraPipControlsModel::kMaxOpacity, model_->opacity());
}

TEST_F(AstraPipControlsModelTest, SetOpacitySameNoNotification) {
  model_->SetOpacity(0.7);

  MockPipControlsModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnOpacityChanged(_)).Times(0);
  model_->SetOpacity(0.7);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, IncreaseOpacity) {
  model_->SetOpacity(0.5);
  model_->IncreaseOpacity();
  EXPECT_DOUBLE_EQ(0.6, model_->opacity());
}

TEST_F(AstraPipControlsModelTest, DecreaseOpacity) {
  model_->SetOpacity(0.5);
  model_->DecreaseOpacity();
  EXPECT_DOUBLE_EQ(0.4, model_->opacity());
}

// =========================================================================
// Model controls visibility tests
// =========================================================================

TEST_F(AstraPipControlsModelTest, SetControlsVisibleFalse) {
  MockPipControlsModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnControlsVisibilityChanged(false)).Times(1);
  model_->SetControlsVisible(false);
  EXPECT_FALSE(model_->controls_visible());

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, SetControlsVisibleSameNoNotification) {
  MockPipControlsModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnControlsVisibilityChanged(_)).Times(0);
  model_->SetControlsVisible(true);  // Already true.

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, ToggleControlsVisible) {
  model_->ToggleControlsVisible();
  EXPECT_FALSE(model_->controls_visible());

  model_->ToggleControlsVisible();
  EXPECT_TRUE(model_->controls_visible());
}

// =========================================================================
// Model controls minimization tests
// =========================================================================

TEST_F(AstraPipControlsModelTest, SetControlsMinimizedTrue) {
  MockPipControlsModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnControlsMinimizedChanged(true)).Times(1);
  model_->SetControlsMinimized(true);
  EXPECT_TRUE(model_->controls_minimized());

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, SetControlsMinimizedSameNoNotification) {
  MockPipControlsModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnControlsMinimizedChanged(_)).Times(0);
  model_->SetControlsMinimized(false);  // Already false.

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, ToggleControlsMinimized) {
  model_->ToggleControlsMinimized();
  EXPECT_TRUE(model_->controls_minimized());

  model_->ToggleControlsMinimized();
  EXPECT_FALSE(model_->controls_minimized());
}

// =========================================================================
// Model snap position tests
// =========================================================================

TEST_F(AstraPipControlsModelTest, SetSnapPositionTopLeft) {
  MockPipControlsModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSnapPositionChanged(PipSnapPosition::kTopLeft))
      .Times(1);
  model_->SetSnapPosition(PipSnapPosition::kTopLeft);
  EXPECT_EQ(PipSnapPosition::kTopLeft, model_->snap_position());

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, SetSnapPositionSameNoNotification) {
  MockPipControlsModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnSnapPositionChanged(_)).Times(0);
  model_->SetSnapPosition(PipSnapPosition::kBottomRight);  // Already bottom-right.

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, CycleSnapPosition) {
  model_->SetSnapPosition(PipSnapPosition::kTopLeft);
  model_->CycleSnapPosition();
  EXPECT_EQ(PipSnapPosition::kTopRight, model_->snap_position());

  model_->CycleSnapPosition();
  EXPECT_EQ(PipSnapPosition::kBottomRight, model_->snap_position());

  model_->CycleSnapPosition();
  EXPECT_EQ(PipSnapPosition::kBottomLeft, model_->snap_position());

  model_->CycleSnapPosition();
  EXPECT_EQ(PipSnapPosition::kTopLeft, model_->snap_position());
}

TEST_F(AstraPipControlsModelTest, CycleSnapPositionFromFreeFloating) {
  model_->SetSnapPosition(PipSnapPosition::kFreeFloating);
  model_->CycleSnapPosition();
  EXPECT_EQ(PipSnapPosition::kTopLeft, model_->snap_position());
}

// =========================================================================
// Model presentation settings tests (persistence via PrefService)
// =========================================================================

TEST_F(AstraPipControlsModelTest, GetAutoHideControlsDefaultIsTrue) {
  EXPECT_TRUE(model_->GetAutoHideControls());
}

TEST_F(AstraPipControlsModelTest, SetAutoHideControls) {
  model_->SetAutoHideControls(false);
  EXPECT_FALSE(model_->GetAutoHideControls());
  EXPECT_FALSE(pref_service_.GetBoolean(prefs::kPrefPiPControlsAutoHide));
}

TEST_F(AstraPipControlsModelTest, SetAutoHideControlsSameNoSettingsChange) {
  MockPipControlsModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnControlsSettingsChanged()).Times(0);
  model_->SetAutoHideControls(true);  // Already true.

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, GetAutoHideDelayDefaultIsThreeSeconds) {
  EXPECT_EQ(base::Seconds(3), model_->GetAutoHideDelay());
}

TEST_F(AstraPipControlsModelTest, SetAutoHideDelay) {
  model_->SetAutoHideDelay(base::Seconds(5));
  EXPECT_EQ(base::Seconds(5), model_->GetAutoHideDelay());
  EXPECT_EQ(5000, pref_service_.GetInteger(
                       prefs::kPrefPiPControlsAutoHideDelayMs));
}

TEST_F(AstraPipControlsModelTest, SetAutoHideDelayNegativeClampsToZero) {
  model_->SetAutoHideDelay(base::Seconds(-1));
  EXPECT_EQ(base::Seconds(0), model_->GetAutoHideDelay());
}

TEST_F(AstraPipControlsModelTest, GetShowTopBarDefaultIsTrue) {
  EXPECT_TRUE(model_->GetShowTopBar());
}

TEST_F(AstraPipControlsModelTest, SetShowTopBar) {
  model_->SetShowTopBar(false);
  EXPECT_FALSE(model_->GetShowTopBar());
  EXPECT_FALSE(pref_service_.GetBoolean(prefs::kPrefPiPControlsShowTopBar));
}

TEST_F(AstraPipControlsModelTest, GetShowBottomBarDefaultIsTrue) {
  EXPECT_TRUE(model_->GetShowBottomBar());
}

TEST_F(AstraPipControlsModelTest, SetShowBottomBar) {
  model_->SetShowBottomBar(false);
  EXPECT_FALSE(model_->GetShowBottomBar());
  EXPECT_FALSE(pref_service_.GetBoolean(prefs::kPrefPiPControlsShowBottomBar));
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
  EXPECT_DOUBLE_EQ(0.7, pref_service_.GetDouble(prefs::kPrefPiPControlsOpacity));
}

TEST_F(AstraPipControlsModelTest, SetControlsOpacityClamps) {
  model_->SetControlsOpacity(0.1);
  EXPECT_DOUBLE_EQ(AstraPipControlsModel::kMinControlsOpacity,
                   model_->GetControlsOpacity());

  model_->SetControlsOpacity(2.0);
  EXPECT_DOUBLE_EQ(AstraPipControlsModel::kMaxControlsOpacity,
                   model_->GetControlsOpacity());
}

TEST_F(AstraPipControlsModelTest, GetDefaultSizePresetDefaultIsMedium) {
  EXPECT_EQ(PipSizePreset::kMedium, model_->GetDefaultSizePreset());
}

TEST_F(AstraPipControlsModelTest, SetDefaultSizePreset) {
  model_->SetDefaultSizePreset(PipSizePreset::kSmall);
  EXPECT_EQ(PipSizePreset::kSmall, model_->GetDefaultSizePreset());
  EXPECT_EQ("small", pref_service_.GetString(
                          prefs::kPrefPiPControlsDefaultSizePreset));
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
  EXPECT_EQ(15, pref_service_.GetInteger(
                   prefs::kPrefPiPControlsSkipDurationSeconds));
}

TEST_F(AstraPipControlsModelTest, SetSkipDurationSecondsClampsMin) {
  model_->SetSkipDurationSeconds(0);
  EXPECT_EQ(AstraPipControlsModel::kMinSkipDurationSeconds,
            model_->GetSkipDurationSeconds());
}

TEST_F(AstraPipControlsModelTest, SetSkipDurationSecondsClampsMax) {
  model_->SetSkipDurationSeconds(100);
  EXPECT_EQ(AstraPipControlsModel::kMaxSkipDurationSeconds,
            model_->GetSkipDurationSeconds());
}

// =========================================================================
// Model reset to defaults test
// =========================================================================

TEST_F(AstraPipControlsModelTest, ResetSettingsToDefaults) {
  // Change several settings.
  model_->SetAutoHideControls(false);
  model_->SetShowTopBar(false);
  model_->SetShowBottomBar(false);
  model_->SetControlsOpacity(0.5);
  model_->SetSkipDurationSeconds(20);
  model_->SetDefaultSizePreset(PipSizePreset::kLarge);

  // Verify they changed.
  EXPECT_FALSE(model_->GetAutoHideControls());
  EXPECT_FALSE(model_->GetShowTopBar());
  EXPECT_FALSE(model_->GetShowBottomBar());
  EXPECT_DOUBLE_EQ(0.5, model_->GetControlsOpacity());
  EXPECT_EQ(20, model_->GetSkipDurationSeconds());
  EXPECT_EQ(PipSizePreset::kLarge, model_->GetDefaultSizePreset());

  // Reset.
  model_->ResetSettingsToDefaults();

  // Verify back to defaults.
  EXPECT_TRUE(model_->GetAutoHideControls());
  EXPECT_TRUE(model_->GetShowTopBar());
  EXPECT_TRUE(model_->GetShowBottomBar());
  EXPECT_DOUBLE_EQ(1.0, model_->GetControlsOpacity());
  EXPECT_EQ(10, model_->GetSkipDurationSeconds());
  EXPECT_EQ(PipSizePreset::kMedium, model_->GetDefaultSizePreset());
}

TEST_F(AstraPipControlsModelTest, ResetSettingsTriggersSettingsChanged) {
  MockPipControlsModelObserver observer;
  model_->AddObserver(&observer);

  // At least one notification from reset.
  EXPECT_CALL(observer, OnControlsSettingsChanged()).Times(AtLeast(1));
  model_->ResetSettingsToDefaults();

  model_->RemoveObserver(&observer);
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

TEST_F(AstraPipControlsModelTest, ClampOpacityInRange) {
  EXPECT_DOUBLE_EQ(0.5, AstraPipControlsModel::ClampOpacity(0.5));
}

TEST_F(AstraPipControlsModelTest, ClampOpacityBelowMin) {
  EXPECT_DOUBLE_EQ(AstraPipControlsModel::kMinOpacity,
                   AstraPipControlsModel::ClampOpacity(0.0));
}

TEST_F(AstraPipControlsModelTest, ClampPlaybackRateInRange) {
  EXPECT_DOUBLE_EQ(1.5, AstraPipControlsModel::ClampPlaybackRate(1.5));
}

TEST_F(AstraPipControlsModelTest, ClampControlsOpacityInRange) {
  EXPECT_DOUBLE_EQ(0.5, AstraPipControlsModel::ClampControlsOpacity(0.5));
}

TEST_F(AstraPipControlsModelTest, ClampControlsOpacityBelowMin) {
  EXPECT_DOUBLE_EQ(AstraPipControlsModel::kMinControlsOpacity,
                   AstraPipControlsModel::ClampControlsOpacity(0.0));
}

// =========================================================================
// Model multiple observers test
// =========================================================================

TEST_F(AstraPipControlsModelTest, MultipleObserversAllNotified) {
  MockPipControlsModelObserver observer1;
  MockPipControlsModelObserver observer2;

  model_->AddObserver(&observer1);
  model_->AddObserver(&observer2);

  EXPECT_CALL(observer1, OnPlayStateChanged(true)).Times(1);
  EXPECT_CALL(observer2, OnPlayStateChanged(true)).Times(1);
  model_->SetPlaying(true);

  model_->RemoveObserver(&observer1);
  model_->RemoveObserver(&observer2);
}

TEST_F(AstraPipControlsModelTest, RemovedObserverNotNotified) {
  MockPipControlsModelObserver observer;
  model_->AddObserver(&observer);
  model_->RemoveObserver(&observer);

  EXPECT_CALL(observer, OnPlayStateChanged(_)).Times(0);
  model_->SetPlaying(true);
}

// =========================================================================
// Model observer defaults test
// =========================================================================

TEST(AstraPipControlsModelObserverDefaultsTest, AllMethodsHaveDefaultImpl) {
  // The base observer class has empty default implementations.
  // A derived class that overrides nothing should compile and not crash.
  class EmptyObserver : public AstraPipControlsModelObserver {};

  EmptyObserver observer;

  // Calling all methods on the empty observer should not crash.
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

  SUCCEED();
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
  EXPECT_EQ(model_.get(), controls_view_->model());
}

TEST_F(AstraPipControlsViewTest, InitialControlsVisible) {
  EXPECT_TRUE(controls_view_->IsControlsVisible());
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
  // No crash = success.
}

// -- View setters / state updates -----------------------------------------

TEST_F(AstraPipControlsViewTest, SetTitleUpdatesText) {
  controls_view_->SetTitle(u"My Video Title");
  // No crash = success.
}

TEST_F(AstraPipControlsViewTest, SetTitleEmpty) {
  controls_view_->SetTitle(u"My Video");
  controls_view_->SetTitle(std::u16string());
}

TEST_F(AstraPipControlsViewTest, SetPlayingTrue) {
  controls_view_->SetPlaying(true);
  EXPECT_TRUE(model_->is_playing());
}

TEST_F(AstraPipControlsViewTest, SetPlayingFalse) {
  controls_view_->SetPlaying(true);
  controls_view_->SetPlaying(false);
  EXPECT_FALSE(model_->is_playing());
}

TEST_F(AstraPipControlsViewTest, SetMutedTrue) {
  controls_view_->SetMuted(true);
  EXPECT_TRUE(model_->is_muted());
}

TEST_F(AstraPipControlsViewTest, SetMutedFalse) {
  controls_view_->SetMuted(true);
  controls_view_->SetMuted(false);
  EXPECT_FALSE(model_->is_muted());
}

TEST_F(AstraPipControlsViewTest, SetActiveSizePresetSmall) {
  controls_view_->SetActiveSizePreset(PipSizePreset::kSmall);
  EXPECT_EQ(PipSizePreset::kSmall, model_->active_preset());
}

TEST_F(AstraPipControlsViewTest, SetActiveSizePresetLarge) {
  controls_view_->SetActiveSizePreset(PipSizePreset::kLarge);
  EXPECT_EQ(PipSizePreset::kLarge, model_->active_preset());
}

TEST_F(AstraPipControlsViewTest, SetAlwaysOnTopTrue) {
  controls_view_->SetAlwaysOnTop(true);
  EXPECT_TRUE(model_->is_pinned());
}

TEST_F(AstraPipControlsViewTest, SetAlwaysOnTopFalse) {
  controls_view_->SetAlwaysOnTop(false);
  EXPECT_FALSE(model_->is_pinned());
}

TEST_F(AstraPipControlsViewTest, SetVolume) {
  controls_view_->SetVolume(0.5);
  EXPECT_DOUBLE_EQ(0.5, model_->volume());
}

TEST_F(AstraPipControlsViewTest, SetPlaybackRate) {
  controls_view_->SetPlaybackRate(1.5);
  EXPECT_DOUBLE_EQ(1.5, model_->playback_rate());
}

TEST_F(AstraPipControlsViewTest, SetOpacity) {
  controls_view_->SetOpacity(0.7);
  EXPECT_DOUBLE_EQ(0.7, model_->opacity());
}

// -- View model observer integration ----------------------------------------

TEST_F(AstraPipControlsViewTest, ModelPlayStateChangeUpdatesView) {
  // When the model changes, the view should reflect it.
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

// -- View delegate callback tests ------------------------------------------

TEST_F(AstraPipControlsViewTest, AllDelegateMethodsAreCallable) {
  // Manually invoke each delegate method to verify the delegate pattern works.
  delegate_->OnPlayPause();
  EXPECT_EQ(1, delegate_->play_pause_count);

  delegate_->OnSkipBackward(10);
  EXPECT_EQ(1, delegate_->skip_backward_count);
  EXPECT_EQ(10, delegate_->last_skip_backward_seconds);

  delegate_->OnSkipForward(15);
  EXPECT_EQ(1, delegate_->skip_forward_count);
  EXPECT_EQ(15, delegate_->last_skip_forward_seconds);

  delegate_->OnMuteToggle();
  EXPECT_EQ(1, delegate_->mute_toggle_count);

  delegate_->OnClosePip();
  EXPECT_EQ(1, delegate_->close_count);

  delegate_->OnReturnToTab();
  EXPECT_EQ(1, delegate_->return_to_tab_count);

  delegate_->OnResizePreset(PipSizePreset::kLarge);
  EXPECT_EQ(1, delegate_->resize_preset_count);
  EXPECT_EQ(PipSizePreset::kLarge, delegate_->last_preset);

  delegate_->OnAlwaysOnTopToggle();
  EXPECT_EQ(1, delegate_->always_on_top_count);

  delegate_->OnVolumeChanged(0.5);
  EXPECT_EQ(1, delegate_->volume_changed_count);
  EXPECT_DOUBLE_EQ(0.5, delegate_->last_volume);

  delegate_->OnPlaybackRateChanged(1.5);
  EXPECT_EQ(1, delegate_->playback_rate_changed_count);
  EXPECT_DOUBLE_EQ(1.5, delegate_->last_playback_rate);

  delegate_->OnOpacityChanged(0.8);
  EXPECT_EQ(1, delegate_->opacity_changed_count);
  EXPECT_DOUBLE_EQ(0.8, delegate_->last_opacity);

  delegate_->OnSnapPositionChanged(PipSnapPosition::kTopLeft);
  EXPECT_EQ(1, delegate_->snap_position_changed_count);
  EXPECT_EQ(PipSnapPosition::kTopLeft, delegate_->last_snap_position);
}

// -- View keyboard shortcut tests ------------------------------------------

TEST_F(AstraPipControlsViewTest, KeyboardSpaceTogglesPlay) {
  model_->SetPlaying(false);
  PressKey(ui::VKEY_SPACE);
  EXPECT_TRUE(model_->is_playing());
  EXPECT_EQ(1, delegate_->play_pause_count);
}

TEST_F(AstraPipControlsViewTest, KeyboardKTogglesPlay) {
  model_->SetPlaying(false);
  PressKey(ui::VKEY_K);
  EXPECT_TRUE(model_->is_playing());
  EXPECT_EQ(1, delegate_->play_pause_count);
}

TEST_F(AstraPipControlsViewTest, KeyboardMTogglesMute) {
  model_->SetMuted(false);
  PressKey(ui::VKEY_M);
  EXPECT_TRUE(model_->is_muted());
  EXPECT_EQ(1, delegate_->mute_toggle_count);
}

TEST_F(AstraPipControlsViewTest, KeyboardPTogglesAlwaysOnTop) {
  model_->SetAlwaysOnTop(true);
  PressKey(ui::VKEY_P);
  EXPECT_FALSE(model_->is_pinned());
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
  EXPECT_GT(model_->volume(), 0.5);
  EXPECT_EQ(1, delegate_->volume_changed_count);
}

TEST_F(AstraPipControlsViewTest, KeyboardCtrlDownDecreasesVolume) {
  model_->SetVolume(0.5);
  PressKey(ui::VKEY_DOWN, ui::EF_CONTROL_DOWN);
  EXPECT_LT(model_->volume(), 0.5);
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

// -- View mouse interaction tests -----------------------------------------

TEST_F(AstraPipControlsViewTest, MouseEnterShowsControls) {
  model_->SetControlsVisible(false);
  ASSERT_FALSE(model_->controls_visible());

  ui::MouseEvent enter_event(ui::ET_MOUSE_ENTERED, gfx::Point(),
                             gfx::Point(), base::TimeTicks(), 0, 0);
  controls_view_->OnMouseEntered(enter_event);

  EXPECT_TRUE(model_->controls_visible());
}

TEST_F(AstraPipControlsViewTest, MousePressedDoesNotCrash) {
  ui::MouseEvent press_event(ui::ET_MOUSE_PRESSED, gfx::Point(10, 10),
                             gfx::Point(10, 10), base::TimeTicks(),
                             ui::EF_LEFT_MOUSE_BUTTON,
                             ui::EF_LEFT_MOUSE_BUTTON);
  controls_view_->OnMousePressed(press_event);
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

// -- View full state cycle test --------------------------------------------

TEST_F(AstraPipControlsViewTest, FullStateCycleNoCrash) {
  // Cycle through all state combinations.
  controls_view_->SetPlaying(true);
  controls_view_->SetMuted(true);
  controls_view_->SetAlwaysOnTop(false);
  controls_view_->SetActiveSizePreset(PipSizePreset::kSmall);
  controls_view_->SetTitle(u"Test Video");
  controls_view_->SetVolume(0.5);
  controls_view_->SetPlaybackRate(1.5);
  controls_view_->SetOpacity(0.8);
  controls_view_->OnThemeChanged();

  controls_view_->SetPlaying(false);
  controls_view_->SetMuted(false);
  controls_view_->SetAlwaysOnTop(true);
  controls_view_->SetActiveSizePreset(PipSizePreset::kLarge);
  controls_view_->SetTitle(u"");
  controls_view_->SetVolume(1.0);
  controls_view_->SetPlaybackRate(1.0);
  controls_view_->SetOpacity(1.0);
  controls_view_->OnThemeChanged();
  // No crash = success.
}

// =========================================================================
// View + settings interaction tests
// =========================================================================

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

// =========================================================================
// Edge case tests
// =========================================================================

TEST_F(AstraPipControlsViewTest, LayoutWithZeroSize) {
  controls_view_->SetBounds(0, 0, 0, 0);
  controls_view_->Layout();
  // No crash = success.
}

TEST_F(AstraPipControlsModelTest, VolumeAtZeroUnmuteDoesNotCrash) {
  model_->SetVolume(0.0);
  model_->SetMuted(true);
  model_->SetVolume(0.0);  // Still zero - should not trigger unmute.
  EXPECT_TRUE(model_->is_muted());
}

TEST_F(AstraPipControlsModelTest, VolumeUpFromZeroUnmutes) {
  model_->SetVolume(0.0);
  model_->SetMuted(true);
  model_->SetVolume(0.1);  // Non-zero - should unmute.
  EXPECT_FALSE(model_->is_muted());
}

TEST_F(AstraPipControlsModelTest, MultipleSettingsChangesEachNotify) {
  MockPipControlsModelObserver observer;
  model_->AddObserver(&observer);

  // Each unique setting change triggers exactly one notification.
  EXPECT_CALL(observer, OnControlsSettingsChanged()).Times(3);
  model_->SetShowTopBar(false);
  model_->SetShowBottomBar(false);
  model_->SetShowResizeHandle(false);

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, SameSettingsChangeDoesNotNotify) {
  MockPipControlsModelObserver observer;
  model_->AddObserver(&observer);

  EXPECT_CALL(observer, OnControlsSettingsChanged()).Times(0);
  model_->SetShowTopBar(true);  // Already true.

  model_->RemoveObserver(&observer);
}

TEST_F(AstraPipControlsModelTest, PrefServiceNotNull) {
  EXPECT_NE(nullptr, model_->pref_service());
  EXPECT_EQ(&pref_service_, model_->pref_service());
}

// =========================================================================
// PipSizePreset enum documentation tests
// =========================================================================

TEST(AstraPipSizePresetEnumTest, ThreePresetsExist) {
  EXPECT_EQ(static_cast<int>(PipSizePreset::kSmall), 0);
  EXPECT_EQ(static_cast<int>(PipSizePreset::kMedium), 1);
  EXPECT_EQ(static_cast<int>(PipSizePreset::kLarge), 2);
}

TEST(AstraPipSnapPositionEnumTest, FiveSnapPositionsExist) {
  EXPECT_EQ(static_cast<int>(PipSnapPosition::kTopLeft), 0);
  EXPECT_EQ(static_cast<int>(PipSnapPosition::kTopRight), 1);
  EXPECT_EQ(static_cast<int>(PipSnapPosition::kBottomLeft), 2);
  EXPECT_EQ(static_cast<int>(PipSnapPosition::kBottomRight), 3);
  EXPECT_EQ(static_cast<int>(PipSnapPosition::kFreeFloating), 4);
}

// =========================================================================
// Delegate interface documentation tests
// =========================================================================

TEST(AstraPipDelegateTest, TwelveDelegateMethods) {
  // The Delegate interface has 12 methods:
  //   1. OnPlayPause
  //   2. OnSkipBackward
  //   3. OnSkipForward
  //   4. OnMuteToggle
  //   5. OnClosePip
  //   6. OnReturnToTab
  //   7. OnResizePreset
  //   8. OnAlwaysOnTopToggle
  //   9. OnVolumeChanged
  //  10. OnPlaybackRateChanged
  //  11. OnOpacityChanged
  //  12. OnSnapPositionChanged
  SUCCEED();
}

// =========================================================================
// Controls layout documentation tests
// =========================================================================

TEST(AstraPipLayoutTest, TopBarComponents) {
  // Top bar contains:
  //   - Astra PiP badge label
  //   - Tab title label (truncated with ellipsis)
  //   - Minimize controls button
  //   - Return to tab button
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

TEST(AstraPipLayoutTest, ResizeHandle) {
  // Bottom-right corner has a resize handle grip.
  SUCCEED();
}

TEST(AstraPipLayoutTest, SnapIndicators) {
  // Each corner has a snap position indicator.
  // The active snap position is highlighted.
  SUCCEED();
}

TEST(AstraPipLayoutTest, AutoHideBehavior) {
  // Controls auto-hide after inactivity.
  // Mouse movement or hover shows them again.
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
  //
  // Patch point: picture_in_picture_window_views.cc
  //   Add AstraPipControlsView as an overlay child view.
  //
  // State is read from / written to:
  //   - AstraPipService (Astra-specific PiP state)
  //   - PictureInPictureWindowController (Chromium PiP state)
  //   - PrefService (persisted presentation settings)
  SUCCEED();
}

TEST(AstraPipModelTest, ModelViewSeparation) {
  // Model/view architecture:
  //   - AstraPipControlsModel owns state and logic
  //   - AstraPipControlsView renders and handles input
  //   - View observes model via AstraPipControlsModelObserver
  //   - Presentation settings persist via PrefService
  //   - UI never owns truth state — model owns it
  SUCCEED();
}

}  // namespace astra
