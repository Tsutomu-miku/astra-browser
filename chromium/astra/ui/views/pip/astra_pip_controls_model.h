#ifndef ASTRA_UI_VIEWS_PIP_ASTRA_PIP_CONTROLS_MODEL_H_
#define ASTRA_UI_VIEWS_PIP_ASTRA_PIP_CONTROLS_MODEL_H_

#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"

class PrefService;

namespace astra {

// Forward-declared from astra/browser/astra_pip_service.h.
enum class PipSizePreset;
enum class PipSnapPosition;

// =========================================================================
// AstraPipControlsModel — state and logic model for PiP controls
// =========================================================================
//
// AstraPipControlsModel is the model layer for the PiP controls view.  It
// owns control state (playback, volume, size preset, etc.) and presentation
// settings (auto-hide, bar visibility, opacity, etc.) that are persisted
// via PrefService.
//
// The view observes the model and re-renders when state changes.  User
// interactions in the view call model methods, which update state and
// notify observers.
//
// Truth model:
//   - Runtime control state (playing, muted, volume, etc.) is owned by
//     the model and reflected in the view.
//   - Presentation settings are persisted via PrefService and loaded/saved
//     through the model.
//   - Actual PiP window state is still owned by Chromium's
//     PictureInPictureWindowController — the model tracks Astra-projected
//     control state, not window state.
//
// Chromium subsystems reused:
//   - PrefService (persistence of presentation settings)
//   - base::ObserverList (observer pattern)
//   - base::CheckedObserver (safe observer base class)
//
// Chromium owner / pattern reference:
//   - chrome/browser/ui/views/picture_in_picture/video_pip_view.h
//     (standard PiP controls pattern)
// =========================================================================

// Observer interface for AstraPipControlsModel.
//
// All methods have empty default implementations so observers can override
// only the methods they care about.
class AstraPipControlsModelObserver : public base::CheckedObserver {
 public:
  // Called when the play/pause state changes.
  virtual void OnPlayStateChanged(bool playing) {}

  // Called when the mute state changes.
  virtual void OnMuteStateChanged(bool muted) {}

  // Called when the volume level changes.
  // |volume| is in the range [0.0, 1.0].
  virtual void OnVolumeChanged(double volume) {}

  // Called when the playback rate changes.
  virtual void OnPlaybackRateChanged(double rate) {}

  // Called when the active size preset changes.
  virtual void OnSizePresetChanged(PipSizePreset preset) {}

  // Called when the always-on-top (pin) state changes.
  virtual void OnAlwaysOnTopChanged(bool pinned) {}

  // Called when the PiP window opacity changes.
  // |opacity| is in the range [0.2, 1.0].
  virtual void OnOpacityChanged(double opacity) {}

  // Called when the controls visibility changes (shown or hidden).
  virtual void OnControlsVisibilityChanged(bool visible) {}

  // Called when any controls presentation setting changes.
  // Observers should re-read all settings they care about.
  virtual void OnControlsSettingsChanged() {}

  // Called when the snap position changes.
  virtual void OnSnapPositionChanged(PipSnapPosition position) {}

  // Called when the controls are minimized or expanded.
  virtual void OnControlsMinimizedChanged(bool minimized) {}

 protected:
  ~AstraPipControlsModelObserver() override = default;
};

class AstraPipControlsModel {
 public:
  // Volume range constants.
  static constexpr double kMinVolume = 0.0;
  static constexpr double kMaxVolume = 1.0;
  static constexpr double kDefaultVolume = 1.0;
  static constexpr double kVolumeStep = 0.1;

  // Opacity range constants.
  static constexpr double kMinOpacity = 0.2;
  static constexpr double kMaxOpacity = 1.0;
  static constexpr double kDefaultOpacity = 1.0;
  static constexpr double kOpacityStep = 0.1;

  // Playback rate constants.
  static constexpr double kMinPlaybackRate = 0.25;
  static constexpr double kMaxPlaybackRate = 4.0;

  // Controls opacity range (for the controls overlay itself).
  static constexpr double kMinControlsOpacity = 0.3;
  static constexpr double kMaxControlsOpacity = 1.0;
  static constexpr double kDefaultControlsOpacity = 1.0;

  // Default auto-hide delay.
  static constexpr base::TimeDelta kDefaultAutoHideDelay =
      base::Seconds(3);

  // Default skip duration in seconds.
  static constexpr int kDefaultSkipDurationSeconds = 10;
  static constexpr int kMinSkipDurationSeconds = 1;
  static constexpr int kMaxSkipDurationSeconds = 60;

  explicit AstraPipControlsModel(PrefService* pref_service);
  ~AstraPipControlsModel();

  AstraPipControlsModel(const AstraPipControlsModel&) = delete;
  AstraPipControlsModel& operator=(const AstraPipControlsModel&) = delete;

  // -- Observer management -------------------------------------------------

  void AddObserver(AstraPipControlsModelObserver* observer);
  void RemoveObserver(AstraPipControlsModelObserver* observer);

  // -- Playback state -----------------------------------------------------

  bool is_playing() const { return is_playing_; }
  void SetPlaying(bool playing);
  void TogglePlay();

  bool is_muted() const { return is_muted_; }
  void SetMuted(bool muted);
  void ToggleMute();

  double volume() const { return volume_; }
  void SetVolume(double volume);
  void IncreaseVolume();
  void DecreaseVolume();

  double playback_rate() const { return playback_rate_; }
  void SetPlaybackRate(double rate);
  void CyclePlaybackRateForward();
  void CyclePlaybackRateBackward();

  // -- Size preset --------------------------------------------------------

  PipSizePreset active_preset() const;
  void SetActiveSizePreset(PipSizePreset preset);
  void CycleSizePresetForward();
  void CycleSizePresetBackward();

  // -- Always-on-top ------------------------------------------------------

  bool is_pinned() const { return is_pinned_; }
  void SetAlwaysOnTop(bool pinned);
  void ToggleAlwaysOnTop();

  // -- Window opacity -----------------------------------------------------

  double opacity() const { return opacity_; }
  void SetOpacity(double opacity);
  void IncreaseOpacity();
  void DecreaseOpacity();

  // -- Controls visibility ------------------------------------------------

  bool controls_visible() const { return controls_visible_; }
  void SetControlsVisible(bool visible);
  void ToggleControlsVisible();

  bool controls_minimized() const { return controls_minimized_; }
  void SetControlsMinimized(bool minimized);
  void ToggleControlsMinimized();

  // -- Snap position ------------------------------------------------------

  PipSnapPosition snap_position() const;
  void SetSnapPosition(PipSnapPosition position);
  void CycleSnapPosition();

  // -- Presentation settings (persisted via PrefService) -----------------
  //
  // These settings control how the controls are presented.  They are
  // persisted to PrefService and apply across PiP sessions.

  // Whether controls auto-hide after inactivity.
  bool GetAutoHideControls() const;
  void SetAutoHideControls(bool auto_hide);

  // Delay before controls auto-hide after the mouse leaves.
  base::TimeDelta GetAutoHideDelay() const;
  void SetAutoHideDelay(base::TimeDelta delay);

  // Whether the top bar (title, close button) is shown.
  bool GetShowTopBar() const;
  void SetShowTopBar(bool show);

  // Whether the bottom bar (playback controls) is shown.
  bool GetShowBottomBar() const;
  void SetShowBottomBar(bool show);

  // Whether the resize handle is shown in the bottom-right corner.
  bool GetShowResizeHandle() const;
  void SetShowResizeHandle(bool show);

  // Opacity of the controls overlay (not the PiP window itself).
  double GetControlsOpacity() const;
  void SetControlsOpacity(double opacity);

  // Default size preset applied to new PiP windows.
  PipSizePreset GetDefaultSizePreset() const;
  void SetDefaultSizePreset(PipSizePreset preset);

  // Whether the always-on-top button is visible in the controls.
  bool GetShowAlwaysOnTopButton() const;
  void SetShowAlwaysOnTopButton(bool show);

  // Whether playback controls (play/pause, skip) are visible.
  bool GetShowPlaybackControls() const;
  void SetShowPlaybackControls(bool show);

  // Whether skip forward/backward buttons are visible.
  bool GetShowSkipButtons() const;
  void SetShowSkipButtons(bool show);

  // Duration in seconds for skip forward/backward actions.
  int GetSkipDurationSeconds() const;
  void SetSkipDurationSeconds(int seconds);

  // List of playback rate presets shown in the rate selector.
  std::vector<double> GetPlaybackRatePresets() const;
  void SetPlaybackRatePresets(const std::vector<double>& presets);

  // Reset all presentation settings to their default values.
  void ResetSettingsToDefaults();

  // -- Utility methods ----------------------------------------------------

  // Returns the number of size presets available.
  static size_t GetSizePresetCount();

  // Returns the next size preset in the cycle.
  static PipSizePreset GetNextSizePreset(PipSizePreset current);

  // Returns the previous size preset in the cycle.
  static PipSizePreset GetPreviousSizePreset(PipSizePreset current);

  // Clamps a volume value to the valid range.
  static double ClampVolume(double volume);

  // Clamps an opacity value to the valid range.
  static double ClampOpacity(double opacity);

  // Clamps a playback rate to the valid range.
  static double ClampPlaybackRate(double rate);

  // Clamps controls opacity to the valid range.
  static double ClampControlsOpacity(double opacity);

  PrefService* pref_service() { return pref_service_; }
  const PrefService* pref_service() const { return pref_service_; }

 private:
  // Notify observers about state changes.
  void NotifyPlayStateChanged(bool playing);
  void NotifyMuteStateChanged(bool muted);
  void NotifyVolumeChanged(double volume);
  void NotifyPlaybackRateChanged(double rate);
  void NotifySizePresetChanged(PipSizePreset preset);
  void NotifyAlwaysOnTopChanged(bool pinned);
  void NotifyOpacityChanged(double opacity);
  void NotifyControlsVisibilityChanged(bool visible);
  void NotifyControlsSettingsChanged();
  void NotifySnapPositionChanged(PipSnapPosition position);
  void NotifyControlsMinimizedChanged(bool minimized);

  // Loads default values from prefs into runtime state.
  void LoadDefaultsFromPrefs();

  raw_ptr<PrefService> pref_service_ = nullptr;

  // -- Runtime control state (not persisted) -----------------------------

  // Playback state.
  bool is_playing_ = false;
  bool is_muted_ = false;
  double volume_ = kDefaultVolume;
  double playback_rate_ = 1.0;

  // Window state.
  // TODO(astra): active_preset_ initial value depends on PipSizePreset enum.
  //   We default to kMedium via LoadDefaultsFromPrefs() in the constructor.
  int active_preset_index_ = 1;  // PipSizePreset::kMedium
  bool is_pinned_ = true;
  double opacity_ = kDefaultOpacity;

  // Controls display state.
  bool controls_visible_ = true;
  bool controls_minimized_ = false;

  // Snap position (runtime value, default loaded from prefs).
  // Using int to avoid including the enum header.
  int snap_position_index_ = 3;  // PipSnapPosition::kBottomRight

  // -- Observers ----------------------------------------------------------

  base::ObserverList<AstraPipControlsModelObserver> observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_PIP_ASTRA_PIP_CONTROLS_MODEL_H_
