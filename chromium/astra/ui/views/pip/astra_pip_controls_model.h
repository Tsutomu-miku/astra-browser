#ifndef ASTRA_UI_VIEWS_PIP_ASTRA_PIP_CONTROLS_MODEL_H_
#define ASTRA_UI_VIEWS_PIP_ASTRA_PIP_CONTROLS_MODEL_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/size.h"

class PrefService;

namespace astra {

// Forward-declared from astra/browser/astra_pip_service.h.
enum class PipSizePreset;
enum class PipSnapPosition;

// =========================================================================
// Enums for PiP controls model
// =========================================================================

// Size presets for the PiP window.
//
// These map to concrete pixel dimensions used when resizing via quick presets.
// kCustom means the user has manually resized the window to a non-preset size.
//
// Chromium reference: video_pip_window_resizer.h
// TODO(astra): Map these presets to actual PiP window sizes via the Chromium
//   PiP window controller.  Chromium owner: PictureInPictureWindowController.
enum class AstraPipSizePreset {
  kTiny,    // Very small — roughly 160x120 (4:3) or 180x100 (16:9)
  kSmall,   // Compact — roughly 240x160 (4:3) or 284x160 (16:9)
  kMedium,  // Default — roughly 360x240 (4:3) or 426x240 (16:9)
  kLarge,   // Large — roughly 480x320 (4:3) or 568x320 (16:9)
  kCustom,  // User-defined custom size
};

// Playback speed presets.
enum class AstraPipPlaybackSpeed {
  k0_5x,   // 0.5x
  k0_75x,  // 0.75x
  k1_0x,   // 1.0x (normal)
  k1_25x,  // 1.25x
  k1_5x,   // 1.5x
  k2x,     // 2.0x
};

// Default position preset for new PiP windows.
enum class AstraPipDefaultPosition {
  kBottomRight,
  kBottomLeft,
  kTopRight,
  kTopLeft,
  kCenter,
};

// =========================================================================
// AstraPipObserver — observer interface for PiP model state changes
// =========================================================================
//
// Observer interface for AstraPipControlsModel.  All methods have empty
// default implementations so observers can override only the methods they
// care about.
//
// Each method receives the model that generated the notification as the
// first parameter, following the Chromium observer pattern where multiple
// models may be observed by a single observer.
//
// Chromium pattern reference: base::CheckedObserver, base::ObserverList
class AstraPipObserver : public base::CheckedObserver {
 public:
  // Called when PiP mode is activated (window shown).
  virtual void OnPipActivated(AstraPipControlsModel* model) {}

  // Called when PiP mode is deactivated (window hidden / closed).
  virtual void OnPipDeactivated(AstraPipControlsModel* model) {}

  // Called when the PiP window size changes.
  // |new_size| is the new window size in DIP.
  virtual void OnPipSizeChanged(AstraPipControlsModel* model,
                                const gfx::Size& new_size) {}

  // Called when the PiP window position changes.
  // |new_position| is the new screen position in DIP.
  virtual void OnPipPositionChanged(AstraPipControlsModel* model,
                                    const gfx::Point& new_position) {}

  // Called when the playback state (playing / paused) changes.
  virtual void OnPlaybackStateChanged(AstraPipControlsModel* model,
                                      bool is_playing) {}

  // Called when the volume level changes.
  // |volume| is in the range [0.0, 1.0].
  virtual void OnVolumeChanged(AstraPipControlsModel* model, double volume) {}

  // Called when the mute state changes.
  virtual void OnMuteChanged(AstraPipControlsModel* model, bool muted) {}

  // Called when the playback progress changes.
  // |progress| is in the range [0.0, 1.0].
  virtual void OnProgressChanged(AstraPipControlsModel* model,
                                 double progress) {}

  // Called when the controls visibility changes (shown or hidden).
  virtual void OnControlsVisibilityChanged(AstraPipControlsModel* model,
                                           bool visible) {}

  // Called when the model is about to be destroyed.
  // Observers should remove themselves from the model in this callback.
  virtual void OnPipModelShutdown(AstraPipControlsModel* model) {}

 protected:
  ~AstraPipObserver() override = default;
};

// =========================================================================
// AstraPipControlsModelObserver — extended observer for controls-specific
// state changes
// =========================================================================
//
// Extended observer interface with controls-specific state change events.
// This is a separate observer for fine-grained control state changes
// (playback rate, size preset, opacity, settings, etc.).
//
// All methods have empty default implementations.
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

  // Called when the playback progress changes.
  virtual void OnProgressChanged(double progress) {}

  // Called when the playback speed preset changes.
  virtual void OnPlaybackSpeedChanged(AstraPipPlaybackSpeed speed) {}

  // Called when the looping state changes.
  virtual void OnLoopingChanged(bool looping) {}

 protected:
  ~AstraPipControlsModelObserver() override = default;
};

// =========================================================================
// AstraPipControlsModel — state and logic model for PiP controls
// =========================================================================
//
// AstraPipControlsModel is the model layer for the PiP controls view.  It
// owns control state (playback, volume, size preset, etc.), window state
// (position, size, always-on-top), workspace integration metadata, and
// presentation settings (auto-hide, bar visibility, opacity, etc.) that
// are persisted via PrefService.
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
//   - Workspace integration metadata is Astra-projected PiP metadata.
//
// Chromium subsystems reused:
//   - PrefService (persistence of presentation settings)
//   - base::ObserverList (observer pattern)
//   - base::CheckedObserver (safe observer base class)
//   - gfx::Size / gfx::Point (geometry types)
//   - base::TimeDelta (time duration types)
//
// Chromium owner / pattern reference:
//   - chrome/browser/ui/views/picture_in_picture/video_pip_view.h
//     (standard PiP controls pattern)
//   - content/browser/picture_in_picture/picture_in_picture_window_controller.h
//     (core PiP window controller)
//
// TODO(astra): Wire to Chromium PiP controller for actual video PiP.
//   Chromium owner: PictureInPictureWindowController
// TODO(astra): Evaluate reusing Chromium's PiP window with custom controls overlay.
//   Chromium pattern: content/browser/picture_in_picture/picture_in_picture_window_controller.h
// =========================================================================

class AstraPipControlsModel {
 public:
  // =========================================================================
  // Settings pref keys (persisted via PrefService)
  // =========================================================================

  // Default size preset applied to new PiP windows.
  static constexpr const char* kPrefDefaultSizePreset =
      "astra.pip.default_size_preset";

  // Default screen position for new PiP windows.
  static constexpr const char* kPrefDefaultPosition =
      "astra.pip.default_position";

  // Whether to maintain the video's aspect ratio when resizing.
  static constexpr const char* kPrefMaintainAspectRatio =
      "astra.pip.maintain_aspect_ratio";

  // Whether PiP windows snap to screen edges when dragged near them.
  static constexpr const char* kPrefSnapToEdges = "astra.pip.snap_to_edges";

  // Distance in pixels from the edge where snapping triggers.
  static constexpr const char* kPrefSnapDistance = "astra.pip.snap_distance";

  // Whether PiP windows are always-on-top by default.
  static constexpr const char* kPrefAlwaysOnTop = "astra.pip.always_on_top";

  // Whether control overlays are shown by default.
  static constexpr const char* kPrefShowControls = "astra.pip.show_controls";

  // Whether controls auto-hide after inactivity.
  static constexpr const char* kPrefAutoHideControls =
      "astra.pip.auto_hide_controls";

  // Delay in milliseconds before controls auto-hide.
  static constexpr const char* kPrefAutoHideDelayMs =
      "astra.pip.auto_hide_delay_ms";

  // Whether to show the tab title in the PiP window.
  static constexpr const char* kPrefShowTitle = "astra.pip.show_title";

  // Whether to show the progress bar in the PiP window.
  static constexpr const char* kPrefShowProgressBar =
      "astra.pip.show_progress_bar";

  // Corner radius of the PiP window in pixels.
  static constexpr const char* kPrefCornerRadius = "astra.pip.corner_radius";

  // Border width of the PiP window in pixels.
  static constexpr const char* kPrefBorderWidth = "astra.pip.border_width";

  // Border color of the PiP window (stored as integer ARGB).
  static constexpr const char* kPrefBorderColor = "astra.pip.border_color";

  // Shadow elevation of the PiP window.
  static constexpr const char* kPrefShadowElevation =
      "astra.pip.shadow_elevation";

  // Default volume level for PiP playback [0.0, 1.0].
  static constexpr const char* kPrefDefaultVolume = "astra.pip.default_volume";

  // Whether to auto-play videos when entering PiP.
  static constexpr const char* kPrefAutoPlay = "astra.pip.auto_play";

  // Whether to loop video playback by default.
  static constexpr const char* kPrefLoopByDefault = "astra.pip.loop_by_default";

  // Whether PiP follows the active tab when switching workspaces.
  static constexpr const char* kPrefFollowActiveTab =
      "astra.pip.follow_active_tab";

  // Whether PiP windows appear in all workspaces.
  static constexpr const char* kPrefShowInAllWorkspaces =
      "astra.pip.show_in_all_workspaces";

  // Minimum PiP window size (stored as width;height string).
  static constexpr const char* kPrefMinSize = "astra.pip.min_size";

  // Maximum PiP window size (stored as width;height string).
  static constexpr const char* kPrefMaxSize = "astra.pip.max_size";

  // =========================================================================
  // Default values for settings
  // =========================================================================

  static constexpr AstraPipSizePreset kDefaultSizePreset =
      AstraPipSizePreset::kMedium;
  static constexpr AstraPipDefaultPosition kDefaultPosition =
      AstraPipDefaultPosition::kBottomRight;
  static constexpr bool kDefaultMaintainAspectRatio = true;
  static constexpr bool kDefaultSnapToEdges = true;
  static constexpr int kDefaultSnapDistance = 20;
  static constexpr bool kDefaultAlwaysOnTop = true;
  static constexpr bool kDefaultShowControls = true;
  static constexpr bool kDefaultAutoHideControls = true;
  static constexpr int kDefaultAutoHideDelayMs = 3000;
  static constexpr bool kDefaultShowTitle = true;
  static constexpr bool kDefaultShowProgressBar = true;
  static constexpr int kDefaultCornerRadius = 8;
  static constexpr int kDefaultBorderWidth = 0;
  static constexpr SkColor kDefaultBorderColor = SK_ColorTRANSPARENT;
  static constexpr int kDefaultShadowElevation = 8;
  static constexpr double kDefaultVolume = 1.0;
  static constexpr bool kDefaultAutoPlay = true;
  static constexpr bool kDefaultLoopByDefault = false;
  static constexpr bool kDefaultFollowActiveTab = false;
  static constexpr bool kDefaultShowInAllWorkspaces = false;
  static constexpr int kDefaultMinSizeWidth = 120;
  static constexpr int kDefaultMinSizeHeight = 80;
  static constexpr int kDefaultMaxSizeWidth = 960;
  static constexpr int kDefaultMaxSizeHeight = 720;

  // Volume range constants.
  static constexpr double kMinVolume = 0.0;
  static constexpr double kMaxVolume = 1.0;
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
  static constexpr base::TimeDelta kDefaultAutoHideDelay = base::Seconds(3);

  // Default skip duration in seconds.
  static constexpr int kDefaultSkipDurationSeconds = 10;
  static constexpr int kMinSkipDurationSeconds = 1;
  static constexpr int kMaxSkipDurationSeconds = 60;

  // Progress range constants.
  static constexpr double kMinProgress = 0.0;
  static constexpr double kMaxProgress = 1.0;

  // Aspect ratio bounds.
  static constexpr double kMinAspectRatio = 0.25;
  static constexpr double kMaxAspectRatio = 4.0;
  static constexpr double kDefaultAspectRatio = 16.0 / 9.0;

  // Default PiP window size.
  static constexpr int kDefaultWidth = 426;
  static constexpr int kDefaultHeight = 240;

  explicit AstraPipControlsModel(PrefService* pref_service);
  ~AstraPipControlsModel();

  AstraPipControlsModel(const AstraPipControlsModel&) = delete;
  AstraPipControlsModel& operator=(const AstraPipControlsModel&) = delete;

  // -- PiP state ------------------------------------------------------------

  bool IsPipActive() const { return is_pip_active_; }
  void ActivatePip();
  void DeactivatePip();
  void TogglePip();

  // -- Tab identity ---------------------------------------------------------

  int GetTabId() const { return tab_id_; }
  void SetTabId(int tab_id);

  const std::u16string& GetTabTitle() const { return tab_title_; }
  void SetTabTitle(const std::u16string& title);

  // -- Size preset (AstraPipSizePreset) -----------------------------------

  AstraPipSizePreset GetSizePreset() const { return size_preset_; }
  void SetSizePreset(AstraPipSizePreset preset);

  // Returns the list of all size presets with their default sizes.
  // Each pair contains the preset enum and its default pixel size.
  static std::vector<std::pair<AstraPipSizePreset, gfx::Size>>
  GetSizePresets();

  // Returns the default size for a given preset.
  static gfx::Size GetPresetSize(AstraPipSizePreset preset);

  // -- Size and position ----------------------------------------------------

  const gfx::Size& GetSize() const { return size_; }
  void SetSize(const gfx::Size& size);

  const gfx::Point& GetPosition() const { return position_; }
  void SetPosition(const gfx::Point& position);

  const gfx::Size& GetMinSize() const { return min_size_; }
  void SetMinSize(const gfx::Size& size);

  const gfx::Size& GetMaxSize() const { return max_size_; }
  void SetMaxSize(const gfx::Size& size);

  // -- Aspect ratio --------------------------------------------------------

  double GetAspectRatio() const { return aspect_ratio_; }
  void SetAspectRatio(double ratio);

  bool GetMaintainAspectRatio() const { return maintain_aspect_ratio_; }
  void SetMaintainAspectRatio(bool maintain);

  // -- Snap to edges -------------------------------------------------------

  bool GetSnapToEdges() const { return snap_to_edges_; }
  void SetSnapToEdges(bool snap);

  int GetSnapDistance() const { return snap_distance_; }
  void SetSnapDistance(int distance_px);

  // -- Playback state -------------------------------------------------------

  bool IsPlaying() const { return is_playing_; }
  void SetPlaying(bool playing);
  void TogglePlayPause();

  double GetPlaybackProgress() const { return playback_progress_; }
  void SetPlaybackProgress(double progress);

  base::TimeDelta GetDuration() const { return duration_; }
  void SetDuration(base::TimeDelta duration);

  base::TimeDelta GetCurrentTime() const { return current_time_; }
  void SetCurrentTime(base::TimeDelta time);

  // -- Volume and mute ------------------------------------------------------

  bool IsMuted() const { return is_muted_; }
  void SetMuted(bool muted);
  void ToggleMute();

  double GetVolume() const { return volume_; }
  void SetVolume(double volume);

  // -- Looping --------------------------------------------------------------

  bool IsLooping() const { return is_looping_; }
  void SetLooping(bool loop);
  void ToggleLoop();

  // -- Playback speed -------------------------------------------------------

  AstraPipPlaybackSpeed GetPlaybackSpeed() const { return playback_speed_; }
  void SetPlaybackSpeed(AstraPipPlaybackSpeed speed);

  // Converts a speed preset to a rate multiplier (e.g. k1_5x -> 1.5).
  static double PlaybackSpeedToRate(AstraPipPlaybackSpeed speed);

  // Converts a rate multiplier to the nearest speed preset.
  static AstraPipPlaybackSpeed RateToPlaybackSpeed(double rate);

  // -- Window controls ------------------------------------------------------

  bool GetSticky() const { return is_pinned_; }
  void SetSticky(bool sticky);

  bool GetAlwaysOnTop() const { return is_pinned_; }
  void SetAlwaysOnTop(bool on_top);

  bool IsMinimized() const { return is_minimized_; }
  void SetMinimized(bool minimized);

  bool IsMaximized() const { return is_maximized_; }
  void SetMaximized(bool maximized);

  bool IsResizable() const { return is_resizable_; }
  void SetResizable(bool resizable);

  bool IsMovable() const { return is_movable_; }
  void SetMovable(bool movable);

  // -- Workspace integration ------------------------------------------------

  const std::string& GetWorkspaceId() const { return workspace_id_; }
  void SetWorkspaceId(const std::string& workspace_id);

  bool GetFollowActiveTab() const { return follow_active_tab_; }
  void SetFollowActiveTab(bool follow);

  bool GetShowInAllWorkspaces() const { return show_in_all_workspaces_; }
  void SetShowInAllWorkspaces(bool show_all);

  // -- Appearance -----------------------------------------------------------

  bool GetShowControls() const { return show_controls_; }
  void SetShowControls(bool show);

  bool GetAutoHideControls() const { return auto_hide_controls_; }
  void SetAutoHideControls(bool auto_hide);

  int GetControlsAutoHideDelay() const { return controls_auto_hide_delay_ms_; }
  void SetControlsAutoHideDelay(int delay_ms);

  bool GetShowTitle() const { return show_title_; }
  void SetShowTitle(bool show);

  bool GetShowProgressBar() const { return show_progress_bar_; }
  void SetShowProgressBar(bool show);

  int GetCornerRadius() const { return corner_radius_; }
  void SetCornerRadius(int radius_px);

  int GetBorderWidth() const { return border_width_; }
  void SetBorderWidth(int width_px);

  SkColor GetBorderColor() const { return border_color_; }
  void SetBorderColor(SkColor color);

  int GetShadowElevation() const { return shadow_elevation_; }
  void SetShadowElevation(int elevation);

  // -- Observer management -------------------------------------------------

  void AddObserver(AstraPipObserver* observer);
  void RemoveObserver(AstraPipObserver* observer);

  void AddControlsObserver(AstraPipControlsModelObserver* observer);
  void RemoveControlsObserver(AstraPipControlsModelObserver* observer);

  // -- Backward-compatible size preset (PipSizePreset) ---------------------
  //
  // These use the service-level PipSizePreset enum (3 values).
  // New code should use AstraPipSizePreset (5 values + kCustom).

  PipSizePreset active_preset() const;
  void SetActiveSizePreset(PipSizePreset preset);
  void CycleSizePresetForward();
  void CycleSizePresetBackward();

  // -- Always-on-top (backward-compatible) ---------------------------------

  bool is_pinned() const { return is_pinned_; }
  void ToggleAlwaysOnTop();

  // -- Window opacity (backward-compatible) --------------------------------

  double opacity() const { return opacity_; }
  void SetOpacity(double opacity);
  void IncreaseOpacity();
  void DecreaseOpacity();

  // -- Controls visibility (backward-compatible) ---------------------------

  bool controls_visible() const { return controls_visible_; }
  void SetControlsVisible(bool visible);
  void ToggleControlsVisible();

  bool controls_minimized() const { return controls_minimized_; }
  void SetControlsMinimized(bool minimized);
  void ToggleControlsMinimized();

  // -- Snap position (backward-compatible) ----------------------------------

  PipSnapPosition snap_position() const;
  void SetSnapPosition(PipSnapPosition position);
  void CycleSnapPosition();

  // -- Playback rate (backward-compatible) ---------------------------------

  double playback_rate() const { return playback_rate_; }
  void SetPlaybackRate(double rate);
  void CyclePlaybackRateForward();
  void CyclePlaybackRateBackward();

  // -- Volume helpers (backward-compatible) --------------------------------

  void IncreaseVolume();
  void DecreaseVolume();

  // -- Presentation settings (persisted via PrefService) -------------------
  //
  // These settings control how the controls are presented.  They are
  // persisted to PrefService and apply across PiP sessions.

  // Whether controls auto-hide after inactivity.
  bool GetPrefAutoHideControls() const;
  void SetPrefAutoHideControls(bool auto_hide);

  // Delay before controls auto-hide after the mouse leaves.
  base::TimeDelta GetPrefAutoHideDelay() const;
  void SetPrefAutoHideDelay(base::TimeDelta delay);

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

  // Default size preset applied to new PiP windows (PipSizePreset version).
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

  // -- Utility methods ------------------------------------------------------

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

  // Clamps progress to [0.0, 1.0].
  static double ClampProgress(double progress);

  // Clamps aspect ratio to valid range.
  static double ClampAspectRatio(double ratio);

  PrefService* pref_service() { return pref_service_; }
  const PrefService* pref_service() const { return pref_service_; }

 private:
  // Notify AstraPipObserver about state changes.
  void NotifyPipActivated();
  void NotifyPipDeactivated();
  void NotifyPipSizeChanged(const gfx::Size& new_size);
  void NotifyPipPositionChanged(const gfx::Point& new_position);
  void NotifyPlaybackStateChanged(bool is_playing);
  void NotifyVolumeChanged(double volume);
  void NotifyMuteChanged(bool muted);
  void NotifyProgressChanged(double progress);
  void NotifyControlsVisibilityChanged(bool visible);

  // Notify AstraPipControlsModelObserver about state changes.
  void NotifyControlsPlayStateChanged(bool playing);
  void NotifyControlsMuteStateChanged(bool muted);
  void NotifyControlsVolumeChanged(double volume);
  void NotifyControlsPlaybackRateChanged(double rate);
  void NotifyControlsSizePresetChanged(PipSizePreset preset);
  void NotifyControlsAlwaysOnTopChanged(bool pinned);
  void NotifyControlsOpacityChanged(double opacity);
  void NotifyControlsVisibilityChanged(bool visible);
  void NotifyControlsSettingsChanged();
  void NotifyControlsSnapPositionChanged(PipSnapPosition position);
  void NotifyControlsMinimizedChanged(bool minimized);
  void NotifyControlsProgressChanged(double progress);
  void NotifyControlsPlaybackSpeedChanged(AstraPipPlaybackSpeed speed);
  void NotifyControlsLoopingChanged(bool looping);

  // Loads default values from prefs into runtime state.
  void LoadDefaultsFromPrefs();

  raw_ptr<PrefService> pref_service_ = nullptr;

  // -- PiP state ------------------------------------------------------------

  // Whether PiP is currently active (window is shown).
  bool is_pip_active_ = false;

  // The tab that is currently in PiP mode.
  int tab_id_ = -1;
  std::u16string tab_title_;

  // -- Size and position ----------------------------------------------------

  // Current size preset (5-level + custom).
  AstraPipSizePreset size_preset_ = AstraPipSizePreset::kMedium;

  // Current window size in DIP.
  gfx::Size size_ = gfx::Size(kDefaultWidth, kDefaultHeight);

  // Current window position (screen coordinates) in DIP.
  gfx::Point position_;

  // Minimum allowed window size.
  gfx::Size min_size_ = gfx::Size(kDefaultMinSizeWidth, kDefaultMinSizeHeight);

  // Maximum allowed window size.
  gfx::Size max_size_ = gfx::Size(kDefaultMaxSizeWidth, kDefaultMaxSizeHeight);

  // Aspect ratio (width / height).
  double aspect_ratio_ = kDefaultAspectRatio;
  bool maintain_aspect_ratio_ = kDefaultMaintainAspectRatio;

  // Snap-to-edges settings.
  bool snap_to_edges_ = kDefaultSnapToEdges;
  int snap_distance_ = kDefaultSnapDistance;

  // -- Playback state -------------------------------------------------------

  bool is_playing_ = false;
  bool is_muted_ = false;
  double volume_ = kDefaultVolume;
  double playback_rate_ = 1.0;

  // Playback progress in [0.0, 1.0].
  double playback_progress_ = 0.0;

  // Total duration of the media.
  base::TimeDelta duration_;

  // Current playback position.
  base::TimeDelta current_time_;

  // Whether playback loops.
  bool is_looping_ = kDefaultLoopByDefault;

  // Playback speed preset.
  AstraPipPlaybackSpeed playback_speed_ = AstraPipPlaybackSpeed::k1_0x;

  // -- Window state ---------------------------------------------------------

  bool is_pinned_ = true;
  double opacity_ = kDefaultOpacity;

  // Whether the window is minimized.
  bool is_minimized_ = false;

  // Whether the window is maximized.
  bool is_maximized_ = false;

  // Whether the window is resizable by the user.
  bool is_resizable_ = true;

  // Whether the window is movable by the user.
  bool is_movable_ = true;

  // -- Controls display state -----------------------------------------------

  bool controls_visible_ = true;
  bool controls_minimized_ = false;

  // Snap position (runtime value, default loaded from prefs).
  // Using int to avoid including the enum header.
  int snap_position_index_ = 3;  // PipSnapPosition::kBottomRight

  // -- Workspace integration ------------------------------------------------

  // Current workspace ID (empty if not workspace-scoped).
  std::string workspace_id_;

  // Whether PiP follows the active tab across workspaces.
  bool follow_active_tab_ = kDefaultFollowActiveTab;

  // Whether PiP is visible in all workspaces.
  bool show_in_all_workspaces_ = kDefaultShowInAllWorkspaces;

  // -- Appearance -----------------------------------------------------------

  bool show_controls_ = kDefaultShowControls;
  bool auto_hide_controls_ = kDefaultAutoHideControls;
  int controls_auto_hide_delay_ms_ = kDefaultAutoHideDelayMs;
  bool show_title_ = kDefaultShowTitle;
  bool show_progress_bar_ = kDefaultShowProgressBar;
  int corner_radius_ = kDefaultCornerRadius;
  int border_width_ = kDefaultBorderWidth;
  SkColor border_color_ = kDefaultBorderColor;
  int shadow_elevation_ = kDefaultShadowElevation;

  // -- Observers ------------------------------------------------------------

  base::ObserverList<AstraPipObserver> observers_;
  base::ObserverList<AstraPipControlsModelObserver> controls_observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_PIP_ASTRA_PIP_CONTROLS_MODEL_H_
