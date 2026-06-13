#include "astra/ui/views/pip/astra_pip_controls_model.h"

#include <algorithm>

#include "astra/browser/astra_pip_service.h"
#include "astra/browser/astra_prefs.h"
#include "base/check.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "components/prefs/pref_service.h"
#include "ui/gfx/geometry/size.h"

namespace astra {

namespace {

// Helper to convert a list of doubles in PrefService to a vector.
std::vector<double> ListPrefToDoubleVector(const PrefService* prefs,
                                           const std::string& pref_key) {
  std::vector<double> result;
  const base::Value::List& list = prefs->GetList(pref_key);
  for (const auto& item : list) {
    if (item.is_double()) {
      result.push_back(item.GetDouble());
    } else if (item.is_int()) {
      result.push_back(static_cast<double>(item.GetInt()));
    }
  }
  return result;
}

// Helper to convert a vector of doubles to a base::Value::List.
base::Value::List DoubleVectorToList(const std::vector<double>& values) {
  base::Value::List list;
  for (double v : values) {
    list.Append(v);
  }
  return list;
}

// Helper to parse a size from a "width;height" string.
gfx::Size SizeFromString(const std::string& str,
                         const gfx::Size& default_size) {
  std::vector<std::string> parts = base::SplitString(
      str, ";", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  if (parts.size() != 2) {
    return default_size;
  }
  int width = 0, height = 0;
  if (!base::StringToInt(parts[0], &width) ||
      !base::StringToInt(parts[1], &height)) {
    return default_size;
  }
  if (width <= 0 || height <= 0) {
    return default_size;
  }
  return gfx::Size(width, height);
}

// Helper to convert a size to a "width;height" string.
std::string SizeToString(const gfx::Size& size) {
  return base::NumberToString(size.width()) + ";" +
         base::NumberToString(size.height());
}

}  // namespace

// ---------------------------------------------------------------------------
// AstraPipControlsModel
// ---------------------------------------------------------------------------

AstraPipControlsModel::AstraPipControlsModel(PrefService* pref_service)
    : pref_service_(pref_service) {
  DCHECK(pref_service_);
  LoadDefaultsFromPrefs();
}

AstraPipControlsModel::~AstraPipControlsModel() {
  // Notify observers that the model is shutting down.
  for (auto& observer : observers_) {
    observer.OnPipModelShutdown(this);
  }
}

// -- PiP state --------------------------------------------------------------

void AstraPipControlsModel::ActivatePip() {
  if (is_pip_active_) {
    return;
  }
  is_pip_active_ = true;
  NotifyPipActivated();
}

void AstraPipControlsModel::DeactivatePip() {
  if (!is_pip_active_) {
    return;
  }
  is_pip_active_ = false;
  NotifyPipDeactivated();
}

void AstraPipControlsModel::TogglePip() {
  if (is_pip_active_) {
    DeactivatePip();
  } else {
    ActivatePip();
  }
}

// -- Tab identity -----------------------------------------------------------

void AstraPipControlsModel::SetTabId(int tab_id) {
  tab_id_ = tab_id;
}

void AstraPipControlsModel::SetTabTitle(const std::u16string& title) {
  tab_title_ = title;
}

// -- Size preset (AstraPipSizePreset) --------------------------------------

void AstraPipControlsModel::SetSizePreset(AstraPipSizePreset preset) {
  if (size_preset_ == preset) {
    return;
  }
  size_preset_ = preset;

  // Update the actual size for non-custom presets.
  if (preset != AstraPipSizePreset::kCustom) {
    gfx::Size preset_size = GetPresetSize(preset);
    if (size_ != preset_size) {
      size_ = preset_size;
      NotifyPipSizeChanged(size_);
    }
  }

  // Also update the legacy PipSizePreset mapping.
  // Map 5-level preset down to 3-level service-level preset.
  PipSizePreset legacy_preset = PipSizePreset::kMedium;
  switch (preset) {
    case AstraPipSizePreset::kTiny:
    case AstraPipSizePreset::kSmall:
      legacy_preset = PipSizePreset::kSmall;
      break;
    case AstraPipSizePreset::kMedium:
      legacy_preset = PipSizePreset::kMedium;
      break;
    case AstraPipSizePreset::kLarge:
    case AstraPipSizePreset::kCustom:
      legacy_preset = PipSizePreset::kLarge;
      break;
  }
  NotifyControlsSizePresetChanged(legacy_preset);
}

std::vector<std::pair<AstraPipSizePreset, gfx::Size>>
AstraPipControlsModel::GetSizePresets() {
  return {
      {AstraPipSizePreset::kTiny, GetPresetSize(AstraPipSizePreset::kTiny)},
      {AstraPipSizePreset::kSmall, GetPresetSize(AstraPipSizePreset::kSmall)},
      {AstraPipSizePreset::kMedium, GetPresetSize(AstraPipSizePreset::kMedium)},
      {AstraPipSizePreset::kLarge, GetPresetSize(AstraPipSizePreset::kLarge)},
      {AstraPipSizePreset::kCustom, gfx::Size()},
  };
}

gfx::Size AstraPipControlsModel::GetPresetSize(AstraPipSizePreset preset) {
  // Sizes are based on a 16:9 aspect ratio reference.
  // These are approximate — actual size depends on video aspect ratio.
  switch (preset) {
    case AstraPipSizePreset::kTiny:
      return gfx::Size(180, 101);   // ~16:9, very compact
    case AstraPipSizePreset::kSmall:
      return gfx::Size(284, 160);   // ~16:9, small
    case AstraPipSizePreset::kMedium:
      return gfx::Size(426, 240);   // ~16:9, default
    case AstraPipSizePreset::kLarge:
      return gfx::Size(568, 320);   // ~16:9, large
    case AstraPipSizePreset::kCustom:
      return gfx::Size();           // Custom has no fixed size
  }
  return gfx::Size(426, 240);  // Fallback
}

// -- Size and position ------------------------------------------------------

void AstraPipControlsModel::SetSize(const gfx::Size& size) {
  // Clamp to min/max bounds.
  int width = std::clamp(size.width(), min_size_.width(), max_size_.width());
  int height =
      std::clamp(size.height(), min_size_.height(), max_size_.height());

  // Apply aspect ratio constraint if enabled.
  if (maintain_aspect_ratio_ && aspect_ratio_ > 0.0) {
    // Compute height from width maintaining aspect ratio.
    int new_height = static_cast<int>(width / aspect_ratio_ + 0.5);
    new_height = std::clamp(new_height, min_size_.height(), max_size_.height());
    // Recompute width from the clamped height.
    width = static_cast<int>(new_height * aspect_ratio_ + 0.5);
    width = std::clamp(width, min_size_.width(), max_size_.width());
    height = new_height;
  }

  gfx::Size clamped_size(width, height);
  if (size_ == clamped_size) {
    return;
  }
  size_ = clamped_size;

  // If the new size doesn't match any preset, mark as custom.
  bool matches_preset = false;
  auto presets = GetSizePresets();
  for (const auto& [preset, preset_size] : presets) {
    if (preset == AstraPipSizePreset::kCustom) {
      continue;
    }
    if (size_ == preset_size) {
      size_preset_ = preset;
      matches_preset = true;
      break;
    }
  }
  if (!matches_preset) {
    size_preset_ = AstraPipSizePreset::kCustom;
  }

  NotifyPipSizeChanged(size_);
}

void AstraPipControlsModel::SetPosition(const gfx::Point& position) {
  if (position_ == position) {
    return;
  }
  position_ = position;
  NotifyPipPositionChanged(position_);
}

void AstraPipControlsModel::SetMinSize(const gfx::Size& size) {
  min_size_.SetSize(std::max(0, size.width()), std::max(0, size.height()));

  // Ensure current size is not below the new minimum.
  if (size_.width() < min_size_.width() ||
      size_.height() < min_size_.height()) {
    SetSize(size_);  // Re-clamp to new bounds.
  }
}

void AstraPipControlsModel::SetMaxSize(const gfx::Size& size) {
  max_size_.SetSize(std::max(1, size.width()), std::max(1, size.height()));

  // Ensure min_size is not larger than max_size.
  if (min_size_.width() > max_size_.width()) {
    min_size_.set_width(max_size_.width());
  }
  if (min_size_.height() > max_size_.height()) {
    min_size_.set_height(max_size_.height());
  }

  // Ensure current size is not above the new maximum.
  if (size_.width() > max_size_.width() ||
      size_.height() > max_size_.height()) {
    SetSize(size_);  // Re-clamp to new bounds.
  }
}

// -- Aspect ratio -----------------------------------------------------------

void AstraPipControlsModel::SetAspectRatio(double ratio) {
  aspect_ratio_ = ClampAspectRatio(ratio);

  // Re-apply size constraint if maintaining aspect ratio.
  if (maintain_aspect_ratio_) {
    SetSize(size_);  // Re-clamp with new aspect ratio.
  }
}

void AstraPipControlsModel::SetMaintainAspectRatio(bool maintain) {
  if (maintain_aspect_ratio_ == maintain) {
    return;
  }
  maintain_aspect_ratio_ = maintain;

  // Re-apply size constraint when enabling.
  if (maintain) {
    SetSize(size_);  // Re-clamp with aspect ratio.
  }
}

// -- Snap to edges ----------------------------------------------------------

void AstraPipControlsModel::SetSnapToEdges(bool snap) {
  snap_to_edges_ = snap;
}

void AstraPipControlsModel::SetSnapDistance(int distance_px) {
  snap_distance_ = std::max(0, distance_px);
}

// -- Playback state ---------------------------------------------------------

void AstraPipControlsModel::SetPlaying(bool playing) {
  if (is_playing_ == playing) {
    return;
  }
  is_playing_ = playing;
  NotifyPlaybackStateChanged(playing);
  NotifyControlsPlayStateChanged(playing);
}

void AstraPipControlsModel::TogglePlayPause() {
  SetPlaying(!is_playing_);
}

void AstraPipControlsModel::SetPlaybackProgress(double progress) {
  progress = ClampProgress(progress);
  if (playback_progress_ == progress) {
    return;
  }
  playback_progress_ = progress;
  NotifyProgressChanged(progress);
  NotifyControlsProgressChanged(progress);

  // Update current_time_ based on progress and duration.
  if (duration_.is_positive()) {
    current_time_ = base::Seconds(duration_.InSecondsF() * progress);
  }
}

void AstraPipControlsModel::SetDuration(base::TimeDelta duration) {
  if (duration.is_negative()) {
    duration = base::TimeDelta();
  }
  duration_ = duration;

  // Recompute progress based on current time.
  if (duration_.is_positive() && current_time_ <= duration_) {
    double progress = current_time_ / duration_;
    if (playback_progress_ != progress) {
      playback_progress_ = progress;
      NotifyProgressChanged(playback_progress_);
      NotifyControlsProgressChanged(playback_progress_);
    }
  }
}

void AstraPipControlsModel::SetCurrentTime(base::TimeDelta time) {
  if (time.is_negative()) {
    time = base::TimeDelta();
  }
  if (duration_.is_positive() && time > duration_) {
    time = duration_;
  }
  if (current_time_ == time) {
    return;
  }
  current_time_ = time;

  // Update progress based on current time and duration.
  if (duration_.is_positive()) {
    double progress = current_time_ / duration_;
    if (playback_progress_ != progress) {
      playback_progress_ = ClampProgress(progress);
      NotifyProgressChanged(playback_progress_);
      NotifyControlsProgressChanged(playback_progress_);
    }
  }
}

// -- Volume and mute --------------------------------------------------------

void AstraPipControlsModel::SetMuted(bool muted) {
  if (is_muted_ == muted) {
    return;
  }
  is_muted_ = muted;
  NotifyMuteChanged(muted);
  NotifyControlsMuteStateChanged(muted);
}

void AstraPipControlsModel::ToggleMute() {
  SetMuted(!is_muted_);
}

void AstraPipControlsModel::SetVolume(double volume) {
  volume = ClampVolume(volume);
  if (volume_ == volume) {
    return;
  }
  volume_ = volume;
  NotifyVolumeChanged(volume);
  NotifyControlsVolumeChanged(volume);

  // If volume is set above 0 and we're muted, unmute.
  if (volume > 0.0 && is_muted_) {
    SetMuted(false);
  }
}

// -- Looping ----------------------------------------------------------------

void AstraPipControlsModel::SetLooping(bool loop) {
  if (is_looping_ == loop) {
    return;
  }
  is_looping_ = loop;
  NotifyControlsLoopingChanged(loop);
}

void AstraPipControlsModel::ToggleLoop() {
  SetLooping(!is_looping_);
}

// -- Playback speed ---------------------------------------------------------

void AstraPipControlsModel::SetPlaybackSpeed(AstraPipPlaybackSpeed speed) {
  if (playback_speed_ == speed) {
    return;
  }
  playback_speed_ = speed;

  // Also update the playback rate to match.
  double rate = PlaybackSpeedToRate(speed);
  if (playback_rate_ != rate) {
    playback_rate_ = rate;
    NotifyControlsPlaybackRateChanged(rate);
  }

  NotifyControlsPlaybackSpeedChanged(speed);
}

double AstraPipControlsModel::PlaybackSpeedToRate(AstraPipPlaybackSpeed speed) {
  switch (speed) {
    case AstraPipPlaybackSpeed::k0_5x:
      return 0.5;
    case AstraPipPlaybackSpeed::k0_75x:
      return 0.75;
    case AstraPipPlaybackSpeed::k1_0x:
      return 1.0;
    case AstraPipPlaybackSpeed::k1_25x:
      return 1.25;
    case AstraPipPlaybackSpeed::k1_5x:
      return 1.5;
    case AstraPipPlaybackSpeed::k2x:
      return 2.0;
  }
  return 1.0;  // Fallback
}

AstraPipPlaybackSpeed AstraPipControlsModel::RateToPlaybackSpeed(double rate) {
  // Find the nearest preset.
  double min_diff = 100.0;
  AstraPipPlaybackSpeed best = AstraPipPlaybackSpeed::k1_0x;

  auto check = [&](AstraPipPlaybackSpeed speed) {
    double r = PlaybackSpeedToRate(speed);
    double diff = std::abs(r - rate);
    if (diff < min_diff) {
      min_diff = diff;
      best = speed;
    }
  };

  check(AstraPipPlaybackSpeed::k0_5x);
  check(AstraPipPlaybackSpeed::k0_75x);
  check(AstraPipPlaybackSpeed::k1_0x);
  check(AstraPipPlaybackSpeed::k1_25x);
  check(AstraPipPlaybackSpeed::k1_5x);
  check(AstraPipPlaybackSpeed::k2x);

  return best;
}

// -- Window controls --------------------------------------------------------

void AstraPipControlsModel::SetSticky(bool sticky) {
  SetAlwaysOnTop(sticky);
}

void AstraPipControlsModel::SetAlwaysOnTop(bool on_top) {
  if (is_pinned_ == on_top) {
    return;
  }
  is_pinned_ = on_top;
  NotifyControlsAlwaysOnTopChanged(on_top);
}

void AstraPipControlsModel::SetMinimized(bool minimized) {
  if (is_minimized_ == minimized) {
    return;
  }
  is_minimized_ = minimized;

  // When minimizing, also deactivate PiP state conceptually.
  // When un-minimizing, re-activate.
  if (minimized) {
    // PiP window still exists but is minimized.
  }
}

void AstraPipControlsModel::SetMaximized(bool maximized) {
  if (is_maximized_ == maximized) {
    return;
  }
  is_maximized_ = maximized;
}

void AstraPipControlsModel::SetResizable(bool resizable) {
  is_resizable_ = resizable;
}

void AstraPipControlsModel::SetMovable(bool movable) {
  is_movable_ = movable;
}

// -- Workspace integration --------------------------------------------------

void AstraPipControlsModel::SetWorkspaceId(const std::string& workspace_id) {
  workspace_id_ = workspace_id;
}

void AstraPipControlsModel::SetFollowActiveTab(bool follow) {
  follow_active_tab_ = follow;
}

void AstraPipControlsModel::SetShowInAllWorkspaces(bool show_all) {
  show_in_all_workspaces_ = show_all;
}

// -- Appearance -------------------------------------------------------------

void AstraPipControlsModel::SetShowControls(bool show) {
  if (show_controls_ == show) {
    return;
  }
  show_controls_ = show;
  // Also update the legacy controls_visible_ for backward compatibility.
  if (controls_visible_ != show) {
    controls_visible_ = show;
    NotifyControlsVisibilityChanged(show);
  }
  NotifyControlsVisibilityChanged(show);
}

void AstraPipControlsModel::SetAutoHideControls(bool auto_hide) {
  if (auto_hide_controls_ == auto_hide) {
    return;
  }
  auto_hide_controls_ = auto_hide;
  NotifyControlsSettingsChanged();
}

void AstraPipControlsModel::SetControlsAutoHideDelay(int delay_ms) {
  if (delay_ms < 0) {
    delay_ms = 0;
  }
  if (controls_auto_hide_delay_ms_ == delay_ms) {
    return;
  }
  controls_auto_hide_delay_ms_ = delay_ms;
  NotifyControlsSettingsChanged();
}

void AstraPipControlsModel::SetShowTitle(bool show) {
  if (show_title_ == show) {
    return;
  }
  show_title_ = show;
  NotifyControlsSettingsChanged();
}

void AstraPipControlsModel::SetShowProgressBar(bool show) {
  if (show_progress_bar_ == show) {
    return;
  }
  show_progress_bar_ = show;
  NotifyControlsSettingsChanged();
}

void AstraPipControlsModel::SetCornerRadius(int radius_px) {
  if (radius_px < 0) {
    radius_px = 0;
  }
  corner_radius_ = radius_px;
}

void AstraPipControlsModel::SetBorderWidth(int width_px) {
  if (width_px < 0) {
    width_px = 0;
  }
  border_width_ = width_px;
}

void AstraPipControlsModel::SetBorderColor(SkColor color) {
  border_color_ = color;
}

void AstraPipControlsModel::SetShadowElevation(int elevation) {
  if (elevation < 0) {
    elevation = 0;
  }
  shadow_elevation_ = elevation;
}

// -- Observer management ---------------------------------------------------

void AstraPipControlsModel::AddObserver(AstraPipObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraPipControlsModel::RemoveObserver(AstraPipObserver* observer) {
  observers_.RemoveObserver(observer);
}

void AstraPipControlsModel::AddControlsObserver(
    AstraPipControlsModelObserver* observer) {
  controls_observers_.AddObserver(observer);
}

void AstraPipControlsModel::RemoveControlsObserver(
    AstraPipControlsModelObserver* observer) {
  controls_observers_.RemoveObserver(observer);
}

// -- Backward-compatible size preset (PipSizePreset) -----------------------

PipSizePreset AstraPipControlsModel::active_preset() const {
  // Map 5-level AstraPipSizePreset to 3-level PipSizePreset.
  switch (size_preset_) {
    case AstraPipSizePreset::kTiny:
    case AstraPipSizePreset::kSmall:
      return PipSizePreset::kSmall;
    case AstraPipSizePreset::kMedium:
      return PipSizePreset::kMedium;
    case AstraPipSizePreset::kLarge:
    case AstraPipSizePreset::kCustom:
      return PipSizePreset::kLarge;
  }
  return PipSizePreset::kMedium;
}

void AstraPipControlsModel::SetActiveSizePreset(PipSizePreset preset) {
  // Map 3-level service preset to 5-level model preset.
  AstraPipSizePreset model_preset = AstraPipSizePreset::kMedium;
  switch (preset) {
    case PipSizePreset::kSmall:
      model_preset = AstraPipSizePreset::kSmall;
      break;
    case PipSizePreset::kMedium:
      model_preset = AstraPipSizePreset::kMedium;
      break;
    case PipSizePreset::kLarge:
      model_preset = AstraPipSizePreset::kLarge;
      break;
  }
  SetSizePreset(model_preset);
}

void AstraPipControlsModel::CycleSizePresetForward() {
  SetActiveSizePreset(GetNextSizePreset(active_preset()));
}

void AstraPipControlsModel::CycleSizePresetBackward() {
  SetActiveSizePreset(GetPreviousSizePreset(active_preset()));
}

// -- Always-on-top (backward-compatible) -----------------------------------

void AstraPipControlsModel::ToggleAlwaysOnTop() {
  SetAlwaysOnTop(!is_pinned_);
}

// -- Window opacity (backward-compatible) ----------------------------------

void AstraPipControlsModel::SetOpacity(double opacity) {
  opacity = ClampOpacity(opacity);
  if (opacity_ == opacity) {
    return;
  }
  opacity_ = opacity;
  NotifyControlsOpacityChanged(opacity);
}

void AstraPipControlsModel::IncreaseOpacity() {
  SetOpacity(opacity_ + kOpacityStep);
}

void AstraPipControlsModel::DecreaseOpacity() {
  SetOpacity(opacity_ - kOpacityStep);
}

// -- Controls visibility (backward-compatible) -----------------------------

void AstraPipControlsModel::SetControlsVisible(bool visible) {
  if (controls_visible_ == visible) {
    return;
  }
  controls_visible_ = visible;
  show_controls_ = visible;
  NotifyControlsVisibilityChanged(visible);
  NotifyControlsSettingsChanged();
}

void AstraPipControlsModel::ToggleControlsVisible() {
  SetControlsVisible(!controls_visible_);
}

void AstraPipControlsModel::SetControlsMinimized(bool minimized) {
  if (controls_minimized_ == minimized) {
    return;
  }
  controls_minimized_ = minimized;
  NotifyControlsMinimizedChanged(minimized);
}

void AstraPipControlsModel::ToggleControlsMinimized() {
  SetControlsMinimized(!controls_minimized_);
}

// -- Snap position (backward-compatible) -----------------------------------

PipSnapPosition AstraPipControlsModel::snap_position() const {
  return static_cast<PipSnapPosition>(snap_position_index_);
}

void AstraPipControlsModel::SetSnapPosition(PipSnapPosition position) {
  int index = static_cast<int>(position);
  if (snap_position_index_ == index) {
    return;
  }
  snap_position_index_ = index;
  NotifyControlsSnapPositionChanged(position);
}

void AstraPipControlsModel::CycleSnapPosition() {
  // Cycle through: TopLeft -> TopRight -> BottomRight -> BottomLeft -> TopLeft
  // Skip kFreeFloating in the quick cycle.
  PipSnapPosition current = static_cast<PipSnapPosition>(snap_position_index_);
  PipSnapPosition next;

  switch (current) {
    case PipSnapPosition::kTopLeft:
      next = PipSnapPosition::kTopRight;
      break;
    case PipSnapPosition::kTopRight:
      next = PipSnapPosition::kBottomRight;
      break;
    case PipSnapPosition::kBottomRight:
      next = PipSnapPosition::kBottomLeft;
      break;
    case PipSnapPosition::kBottomLeft:
      next = PipSnapPosition::kTopLeft;
      break;
    case PipSnapPosition::kFreeFloating:
    default:
      next = PipSnapPosition::kTopLeft;
      break;
  }

  SetSnapPosition(next);
}

// -- Playback rate (backward-compatible) -----------------------------------

void AstraPipControlsModel::SetPlaybackRate(double rate) {
  rate = ClampPlaybackRate(rate);
  if (playback_rate_ == rate) {
    return;
  }
  playback_rate_ = rate;

  // Also update playback_speed_ to the nearest preset.
  AstraPipPlaybackSpeed speed = RateToPlaybackSpeed(rate);
  if (playback_speed_ != speed) {
    playback_speed_ = speed;
    NotifyControlsPlaybackSpeedChanged(speed);
  }

  NotifyControlsPlaybackRateChanged(rate);
}

void AstraPipControlsModel::CyclePlaybackRateForward() {
  std::vector<double> presets = GetPlaybackRatePresets();
  if (presets.empty()) {
    return;
  }

  // Find the current rate in the presets, or the closest one.
  auto it = std::find_if(
      presets.begin(), presets.end(),
      [this](double r) { return std::abs(r - playback_rate_) < 0.01; });

  if (it == presets.end() || it + 1 == presets.end()) {
    // Not found or at the end — go to first.
    SetPlaybackRate(presets.front());
  } else {
    SetPlaybackRate(*(it + 1));
  }
}

void AstraPipControlsModel::CyclePlaybackRateBackward() {
  std::vector<double> presets = GetPlaybackRatePresets();
  if (presets.empty()) {
    return;
  }

  auto it = std::find_if(
      presets.begin(), presets.end(),
      [this](double r) { return std::abs(r - playback_rate_) < 0.01; });

  if (it == presets.end() || it == presets.begin()) {
    // Not found or at the beginning — go to last.
    SetPlaybackRate(presets.back());
  } else {
    SetPlaybackRate(*(it - 1));
  }
}

// -- Volume helpers (backward-compatible) ----------------------------------

void AstraPipControlsModel::IncreaseVolume() {
  SetVolume(volume_ + kVolumeStep);
}

void AstraPipControlsModel::DecreaseVolume() {
  SetVolume(volume_ - kVolumeStep);
}

// -- Presentation settings (persisted via PrefService) ---------------------

bool AstraPipControlsModel::GetPrefAutoHideControls() const {
  return pref_service_->GetBoolean(prefs::kPrefPiPControlsAutoHide);
}

void AstraPipControlsModel::SetPrefAutoHideControls(bool auto_hide) {
  if (GetPrefAutoHideControls() == auto_hide) {
    return;
  }
  pref_service_->SetBoolean(prefs::kPrefPiPControlsAutoHide, auto_hide);
  auto_hide_controls_ = auto_hide;
  NotifyControlsSettingsChanged();
}

base::TimeDelta AstraPipControlsModel::GetPrefAutoHideDelay() const {
  return base::Milliseconds(
      pref_service_->GetInteger(prefs::kPrefPiPControlsAutoHideDelayMs));
}

void AstraPipControlsModel::SetPrefAutoHideDelay(base::TimeDelta delay) {
  int ms = static_cast<int>(delay.InMilliseconds());
  if (ms < 0) {
    ms = 0;
  }
  if (pref_service_->GetInteger(prefs::kPrefPiPControlsAutoHideDelayMs) == ms) {
    return;
  }
  pref_service_->SetInteger(prefs::kPrefPiPControlsAutoHideDelayMs, ms);
  controls_auto_hide_delay_ms_ = ms;
  NotifyControlsSettingsChanged();
}

bool AstraPipControlsModel::GetShowTopBar() const {
  return pref_service_->GetBoolean(prefs::kPrefPiPControlsShowTopBar);
}

void AstraPipControlsModel::SetShowTopBar(bool show) {
  if (GetShowTopBar() == show) {
    return;
  }
  pref_service_->SetBoolean(prefs::kPrefPiPControlsShowTopBar, show);
  NotifyControlsSettingsChanged();
}

bool AstraPipControlsModel::GetShowBottomBar() const {
  return pref_service_->GetBoolean(prefs::kPrefPiPControlsShowBottomBar);
}

void AstraPipControlsModel::SetShowBottomBar(bool show) {
  if (GetShowBottomBar() == show) {
    return;
  }
  pref_service_->SetBoolean(prefs::kPrefPiPControlsShowBottomBar, show);
  NotifyControlsSettingsChanged();
}

bool AstraPipControlsModel::GetShowResizeHandle() const {
  return pref_service_->GetBoolean(prefs::kPrefPiPControlsShowResizeHandle);
}

void AstraPipControlsModel::SetShowResizeHandle(bool show) {
  if (GetShowResizeHandle() == show) {
    return;
  }
  pref_service_->SetBoolean(prefs::kPrefPiPControlsShowResizeHandle, show);
  NotifyControlsSettingsChanged();
}

double AstraPipControlsModel::GetControlsOpacity() const {
  return pref_service_->GetDouble(prefs::kPrefPiPControlsOpacity);
}

void AstraPipControlsModel::SetControlsOpacity(double opacity) {
  opacity = ClampControlsOpacity(opacity);
  if (GetControlsOpacity() == opacity) {
    return;
  }
  pref_service_->SetDouble(prefs::kPrefPiPControlsOpacity, opacity);
  NotifyControlsSettingsChanged();
}

PipSizePreset AstraPipControlsModel::GetDefaultSizePreset() const {
  std::string preset_str =
      pref_service_->GetString(prefs::kPrefPiPControlsDefaultSizePreset);
  return AstraPipService::PresetFromString(preset_str);
}

void AstraPipControlsModel::SetDefaultSizePreset(PipSizePreset preset) {
  std::string preset_str = AstraPipService::PresetToString(preset);
  if (pref_service_->GetString(prefs::kPrefPiPControlsDefaultSizePreset) ==
      preset_str) {
    return;
  }
  pref_service_->SetString(prefs::kPrefPiPControlsDefaultSizePreset,
                           preset_str);
  NotifyControlsSettingsChanged();
}

bool AstraPipControlsModel::GetShowAlwaysOnTopButton() const {
  return pref_service_->GetBoolean(prefs::kPrefPiPControlsShowAlwaysOnTopButton);
}

void AstraPipControlsModel::SetShowAlwaysOnTopButton(bool show) {
  if (GetShowAlwaysOnTopButton() == show) {
    return;
  }
  pref_service_->SetBoolean(prefs::kPrefPiPControlsShowAlwaysOnTopButton, show);
  NotifyControlsSettingsChanged();
}

bool AstraPipControlsModel::GetShowPlaybackControls() const {
  return pref_service_->GetBoolean(prefs::kPrefPiPControlsShowPlaybackControls);
}

void AstraPipControlsModel::SetShowPlaybackControls(bool show) {
  if (GetShowPlaybackControls() == show) {
    return;
  }
  pref_service_->SetBoolean(prefs::kPrefPiPControlsShowPlaybackControls, show);
  NotifyControlsSettingsChanged();
}

bool AstraPipControlsModel::GetShowSkipButtons() const {
  return pref_service_->GetBoolean(prefs::kPrefPiPControlsShowSkipButtons);
}

void AstraPipControlsModel::SetShowSkipButtons(bool show) {
  if (GetShowSkipButtons() == show) {
    return;
  }
  pref_service_->SetBoolean(prefs::kPrefPiPControlsShowSkipButtons, show);
  NotifyControlsSettingsChanged();
}

int AstraPipControlsModel::GetSkipDurationSeconds() const {
  return pref_service_->GetInteger(prefs::kPrefPiPControlsSkipDurationSeconds);
}

void AstraPipControlsModel::SetSkipDurationSeconds(int seconds) {
  if (seconds < kMinSkipDurationSeconds) {
    seconds = kMinSkipDurationSeconds;
  }
  if (seconds > kMaxSkipDurationSeconds) {
    seconds = kMaxSkipDurationSeconds;
  }
  if (GetSkipDurationSeconds() == seconds) {
    return;
  }
  pref_service_->SetInteger(prefs::kPrefPiPControlsSkipDurationSeconds, seconds);
  NotifyControlsSettingsChanged();
}

std::vector<double> AstraPipControlsModel::GetPlaybackRatePresets() const {
  return ListPrefToDoubleVector(pref_service_,
                                prefs::kPrefPiPControlsPlaybackRatePresets);
}

void AstraPipControlsModel::SetPlaybackRatePresets(
    const std::vector<double>& presets) {
  // Validate: all values within range, at least one preset.
  std::vector<double> validated;
  for (double rate : presets) {
    validated.push_back(ClampPlaybackRate(rate));
  }
  if (validated.empty()) {
    validated.push_back(1.0);  // Ensure at least one preset.
  }

  base::Value::List list = DoubleVectorToList(validated);
  pref_service_->SetList(prefs::kPrefPiPControlsPlaybackRatePresets,
                         std::move(list));
  NotifyControlsSettingsChanged();
}

void AstraPipControlsModel::ResetSettingsToDefaults() {
  pref_service_->ClearPref(prefs::kPrefPiPControlsAutoHide);
  pref_service_->ClearPref(prefs::kPrefPiPControlsAutoHideDelayMs);
  pref_service_->ClearPref(prefs::kPrefPiPControlsShowTopBar);
  pref_service_->ClearPref(prefs::kPrefPiPControlsShowBottomBar);
  pref_service_->ClearPref(prefs::kPrefPiPControlsShowResizeHandle);
  pref_service_->ClearPref(prefs::kPrefPiPControlsOpacity);
  pref_service_->ClearPref(prefs::kPrefPiPControlsDefaultSizePreset);
  pref_service_->ClearPref(prefs::kPrefPiPControlsShowAlwaysOnTopButton);
  pref_service_->ClearPref(prefs::kPrefPiPControlsShowPlaybackControls);
  pref_service_->ClearPref(prefs::kPrefPiPControlsShowSkipButtons);
  pref_service_->ClearPref(prefs::kPrefPiPControlsSkipDurationSeconds);
  pref_service_->ClearPref(prefs::kPrefPiPControlsPlaybackRatePresets);

  LoadDefaultsFromPrefs();
  NotifyControlsSettingsChanged();
}

// -- Utility methods --------------------------------------------------------

size_t AstraPipControlsModel::GetSizePresetCount() {
  // PipSizePreset has: kSmall, kMedium, kLarge
  return 3;
}

PipSizePreset AstraPipControlsModel::GetNextSizePreset(
    PipSizePreset current) {
  switch (current) {
    case PipSizePreset::kSmall:
      return PipSizePreset::kMedium;
    case PipSizePreset::kMedium:
      return PipSizePreset::kLarge;
    case PipSizePreset::kLarge:
      return PipSizePreset::kSmall;
  }
  return PipSizePreset::kMedium;  // Fallback.
}

PipSizePreset AstraPipControlsModel::GetPreviousSizePreset(
    PipSizePreset current) {
  switch (current) {
    case PipSizePreset::kSmall:
      return PipSizePreset::kLarge;
    case PipSizePreset::kMedium:
      return PipSizePreset::kSmall;
    case PipSizePreset::kLarge:
      return PipSizePreset::kMedium;
  }
  return PipSizePreset::kMedium;  // Fallback.
}

double AstraPipControlsModel::ClampVolume(double volume) {
  if (volume < kMinVolume) return kMinVolume;
  if (volume > kMaxVolume) return kMaxVolume;
  return volume;
}

double AstraPipControlsModel::ClampOpacity(double opacity) {
  if (opacity < kMinOpacity) return kMinOpacity;
  if (opacity > kMaxOpacity) return kMaxOpacity;
  return opacity;
}

double AstraPipControlsModel::ClampPlaybackRate(double rate) {
  if (rate < kMinPlaybackRate) return kMinPlaybackRate;
  if (rate > kMaxPlaybackRate) return kMaxPlaybackRate;
  return rate;
}

double AstraPipControlsModel::ClampControlsOpacity(double opacity) {
  if (opacity < kMinControlsOpacity) return kMinControlsOpacity;
  if (opacity > kMaxControlsOpacity) return kMaxControlsOpacity;
  return opacity;
}

double AstraPipControlsModel::ClampProgress(double progress) {
  if (progress < kMinProgress) return kMinProgress;
  if (progress > kMaxProgress) return kMaxProgress;
  return progress;
}

double AstraPipControlsModel::ClampAspectRatio(double ratio) {
  if (ratio < kMinAspectRatio) return kMinAspectRatio;
  if (ratio > kMaxAspectRatio) return kMaxAspectRatio;
  return ratio;
}

// -- Private helpers: AstraPipObserver notifications -----------------------

void AstraPipControlsModel::NotifyPipActivated() {
  for (auto& observer : observers_) {
    observer.OnPipActivated(this);
  }
}

void AstraPipControlsModel::NotifyPipDeactivated() {
  for (auto& observer : observers_) {
    observer.OnPipDeactivated(this);
  }
}

void AstraPipControlsModel::NotifyPipSizeChanged(const gfx::Size& new_size) {
  for (auto& observer : observers_) {
    observer.OnPipSizeChanged(this, new_size);
  }
}

void AstraPipControlsModel::NotifyPipPositionChanged(
    const gfx::Point& new_position) {
  for (auto& observer : observers_) {
    observer.OnPipPositionChanged(this, new_position);
  }
}

void AstraPipControlsModel::NotifyPlaybackStateChanged(bool is_playing) {
  for (auto& observer : observers_) {
    observer.OnPlaybackStateChanged(this, is_playing);
  }
}

void AstraPipControlsModel::NotifyVolumeChanged(double volume) {
  for (auto& observer : observers_) {
    observer.OnVolumeChanged(this, volume);
  }
}

void AstraPipControlsModel::NotifyMuteChanged(bool muted) {
  for (auto& observer : observers_) {
    observer.OnMuteChanged(this, muted);
  }
}

void AstraPipControlsModel::NotifyProgressChanged(double progress) {
  for (auto& observer : observers_) {
    observer.OnProgressChanged(this, progress);
  }
}

void AstraPipControlsModel::NotifyControlsVisibilityChanged(bool visible) {
  for (auto& observer : observers_) {
    observer.OnControlsVisibilityChanged(this, visible);
  }
}

// -- Private helpers: Controls observer notifications ----------------------

void AstraPipControlsModel::NotifyControlsPlayStateChanged(bool playing) {
  for (auto& observer : controls_observers_) {
    observer.OnPlayStateChanged(playing);
  }
}

void AstraPipControlsModel::NotifyControlsMuteStateChanged(bool muted) {
  for (auto& observer : controls_observers_) {
    observer.OnMuteStateChanged(muted);
  }
}

void AstraPipControlsModel::NotifyControlsVolumeChanged(double volume) {
  for (auto& observer : controls_observers_) {
    observer.OnVolumeChanged(volume);
  }
}

void AstraPipControlsModel::NotifyControlsPlaybackRateChanged(double rate) {
  for (auto& observer : controls_observers_) {
    observer.OnPlaybackRateChanged(rate);
  }
}

void AstraPipControlsModel::NotifyControlsSizePresetChanged(
    PipSizePreset preset) {
  for (auto& observer : controls_observers_) {
    observer.OnSizePresetChanged(preset);
  }
}

void AstraPipControlsModel::NotifyControlsAlwaysOnTopChanged(bool pinned) {
  for (auto& observer : controls_observers_) {
    observer.OnAlwaysOnTopChanged(pinned);
  }
}

void AstraPipControlsModel::NotifyControlsOpacityChanged(double opacity) {
  for (auto& observer : controls_observers_) {
    observer.OnOpacityChanged(opacity);
  }
}

void AstraPipControlsModel::NotifyControlsVisibilityChanged(bool visible) {
  for (auto& observer : controls_observers_) {
    observer.OnControlsVisibilityChanged(visible);
  }
}

void AstraPipControlsModel::NotifyControlsSettingsChanged() {
  for (auto& observer : controls_observers_) {
    observer.OnControlsSettingsChanged();
  }
}

void AstraPipControlsModel::NotifyControlsSnapPositionChanged(
    PipSnapPosition position) {
  for (auto& observer : controls_observers_) {
    observer.OnSnapPositionChanged(position);
  }
}

void AstraPipControlsModel::NotifyControlsMinimizedChanged(bool minimized) {
  for (auto& observer : controls_observers_) {
    observer.OnControlsMinimizedChanged(minimized);
  }
}

void AstraPipControlsModel::NotifyControlsProgressChanged(double progress) {
  for (auto& observer : controls_observers_) {
    observer.OnProgressChanged(progress);
  }
}

void AstraPipControlsModel::NotifyControlsPlaybackSpeedChanged(
    AstraPipPlaybackSpeed speed) {
  for (auto& observer : controls_observers_) {
    observer.OnPlaybackSpeedChanged(speed);
  }
}

void AstraPipControlsModel::NotifyControlsLoopingChanged(bool looping) {
  for (auto& observer : controls_observers_) {
    observer.OnLoopingChanged(looping);
  }
}

// -- Private helpers --------------------------------------------------------

void AstraPipControlsModel::LoadDefaultsFromPrefs() {
  // Load default size preset as the initial active preset.
  active_preset_index_ = static_cast<int>(GetDefaultSizePreset());
  // Also set the 5-level preset.
  switch (GetDefaultSizePreset()) {
    case PipSizePreset::kSmall:
      size_preset_ = AstraPipSizePreset::kSmall;
      break;
    case PipSizePreset::kMedium:
      size_preset_ = AstraPipSizePreset::kMedium;
      break;
    case PipSizePreset::kLarge:
      size_preset_ = AstraPipSizePreset::kLarge;
      break;
  }
  size_ = GetPresetSize(size_preset_);

  // Load default snap position.
  std::string snap_str =
      pref_service_->GetString(prefs::kPrefPiPSnapPosition);
  PipSnapPosition snap = AstraPipService::SnapPositionFromString(snap_str);
  snap_position_index_ = static_cast<int>(snap);

  // Load default always-on-top.
  is_pinned_ = pref_service_->GetBoolean(prefs::kPrefPiPAlwaysOnTop);

  // Load default opacity.
  opacity_ = pref_service_->GetDouble(prefs::kPrefPiPOpacity);

  // Load default volume.
  volume_ = pref_service_->GetDouble(prefs::kPrefPiPDefaultVolume);

  // Load auto-hide settings.
  auto_hide_controls_ =
      pref_service_->GetBoolean(prefs::kPrefPiPControlsAutoHide);
  controls_auto_hide_delay_ms_ =
      pref_service_->GetInteger(prefs::kPrefPiPControlsAutoHideDelayMs);

  // Load appearance settings.
  show_title_ = pref_service_->GetBoolean(prefs::kPrefPiPShowTitle);
  show_progress_bar_ = pref_service_->GetBoolean(prefs::kPrefPiPShowProgressBar);
  corner_radius_ = pref_service_->GetInteger(prefs::kPrefPiPCornerRadius);
  border_width_ = pref_service_->GetInteger(prefs::kPrefPiPBorderWidth);
  border_color_ = static_cast<SkColor>(
      pref_service_->GetInteger(prefs::kPrefPiPBorderColor));
  shadow_elevation_ = pref_service_->GetInteger(prefs::kPrefPiPShadowElevation);

  // Load workspace settings.
  follow_active_tab_ =
      pref_service_->GetBoolean(prefs::kPrefPiPFollowActiveTab);
  show_in_all_workspaces_ =
      pref_service_->GetBoolean(prefs::kPrefPiPShowInAllWorkspaces);

  // Load snap settings.
  snap_to_edges_ = pref_service_->GetBoolean(prefs::kPrefPiPSnapToEdges);
  snap_distance_ = pref_service_->GetInteger(prefs::kPrefPiPSnapDistance);

  // Load aspect ratio setting.
  maintain_aspect_ratio_ =
      pref_service_->GetBoolean(prefs::kPrefPiPMaintainAspectRatio);

  // Load min/max sizes.
  min_size_ = SizeFromString(
      pref_service_->GetString(prefs::kPrefPiPMinSize),
      gfx::Size(kDefaultMinSizeWidth, kDefaultMinSizeHeight));
  max_size_ = SizeFromString(
      pref_service_->GetString(prefs::kPrefPiPMaxSize),
      gfx::Size(kDefaultMaxSizeWidth, kDefaultMaxSizeHeight));

  // Load auto-play and loop settings.
  is_looping_ = pref_service_->GetBoolean(prefs::kPrefPiPLoopByDefault);
}

}  // namespace astra
