#include "astra/ui/views/pip/astra_pip_controls_model.h"

#include <algorithm>

#include "astra/browser/astra_pip_service.h"
#include "astra/browser/astra_prefs.h"
#include "base/check.h"
#include "base/values.h"
#include "components/prefs/pref_service.h"

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

}  // namespace

// ---------------------------------------------------------------------------
// AstraPipControlsModel
// ---------------------------------------------------------------------------

AstraPipControlsModel::AstraPipControlsModel(PrefService* pref_service)
    : pref_service_(pref_service) {
  DCHECK(pref_service_);
  LoadDefaultsFromPrefs();
}

AstraPipControlsModel::~AstraPipControlsModel() = default;

// -- Observer management -----------------------------------------------------

void AstraPipControlsModel::AddObserver(
    AstraPipControlsModelObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraPipControlsModel::RemoveObserver(
    AstraPipControlsModelObserver* observer) {
  observers_.RemoveObserver(observer);
}

// -- Playback state ---------------------------------------------------------

void AstraPipControlsModel::SetPlaying(bool playing) {
  if (is_playing_ == playing) {
    return;
  }
  is_playing_ = playing;
  NotifyPlayStateChanged(playing);
}

void AstraPipControlsModel::TogglePlay() {
  SetPlaying(!is_playing_);
}

void AstraPipControlsModel::SetMuted(bool muted) {
  if (is_muted_ == muted) {
    return;
  }
  is_muted_ = muted;
  NotifyMuteStateChanged(muted);
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

  // If volume is set above 0 and we're muted, unmute.
  if (volume > 0.0 && is_muted_) {
    SetMuted(false);
  }
}

void AstraPipControlsModel::IncreaseVolume() {
  SetVolume(volume_ + kVolumeStep);
}

void AstraPipControlsModel::DecreaseVolume() {
  SetVolume(volume_ - kVolumeStep);
}

void AstraPipControlsModel::SetPlaybackRate(double rate) {
  rate = ClampPlaybackRate(rate);
  if (playback_rate_ == rate) {
    return;
  }
  playback_rate_ = rate;
  NotifyPlaybackRateChanged(rate);
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

  // Find the current rate in the presets, or the closest one.
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

// -- Size preset ------------------------------------------------------------

PipSizePreset AstraPipControlsModel::active_preset() const {
  return static_cast<PipSizePreset>(active_preset_index_);
}

void AstraPipControlsModel::SetActiveSizePreset(PipSizePreset preset) {
  int index = static_cast<int>(preset);
  if (active_preset_index_ == index) {
    return;
  }
  active_preset_index_ = index;
  NotifySizePresetChanged(preset);
}

void AstraPipControlsModel::CycleSizePresetForward() {
  SetActiveSizePreset(
      GetNextSizePreset(static_cast<PipSizePreset>(active_preset_index_)));
}

void AstraPipControlsModel::CycleSizePresetBackward() {
  SetActiveSizePreset(
      GetPreviousSizePreset(static_cast<PipSizePreset>(active_preset_index_)));
}

// -- Always-on-top ----------------------------------------------------------

void AstraPipControlsModel::SetAlwaysOnTop(bool pinned) {
  if (is_pinned_ == pinned) {
    return;
  }
  is_pinned_ = pinned;
  NotifyAlwaysOnTopChanged(pinned);
}

void AstraPipControlsModel::ToggleAlwaysOnTop() {
  SetAlwaysOnTop(!is_pinned_);
}

// -- Window opacity ---------------------------------------------------------

void AstraPipControlsModel::SetOpacity(double opacity) {
  opacity = ClampOpacity(opacity);
  if (opacity_ == opacity) {
    return;
  }
  opacity_ = opacity;
  NotifyOpacityChanged(opacity);
}

void AstraPipControlsModel::IncreaseOpacity() {
  SetOpacity(opacity_ + kOpacityStep);
}

void AstraPipControlsModel::DecreaseOpacity() {
  SetOpacity(opacity_ - kOpacityStep);
}

// -- Controls visibility ----------------------------------------------------

void AstraPipControlsModel::SetControlsVisible(bool visible) {
  if (controls_visible_ == visible) {
    return;
  }
  controls_visible_ = visible;
  NotifyControlsVisibilityChanged(visible);
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

// -- Snap position ----------------------------------------------------------

PipSnapPosition AstraPipControlsModel::snap_position() const {
  return static_cast<PipSnapPosition>(snap_position_index_);
}

void AstraPipControlsModel::SetSnapPosition(PipSnapPosition position) {
  int index = static_cast<int>(position);
  if (snap_position_index_ == index) {
    return;
  }
  snap_position_index_ = index;
  NotifySnapPositionChanged(position);
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

// -- Presentation settings --------------------------------------------------

bool AstraPipControlsModel::GetAutoHideControls() const {
  return pref_service_->GetBoolean(prefs::kPrefPiPControlsAutoHide);
}

void AstraPipControlsModel::SetAutoHideControls(bool auto_hide) {
  if (GetAutoHideControls() == auto_hide) {
    return;
  }
  pref_service_->SetBoolean(prefs::kPrefPiPControlsAutoHide, auto_hide);
  NotifyControlsSettingsChanged();
}

base::TimeDelta AstraPipControlsModel::GetAutoHideDelay() const {
  return base::Milliseconds(
      pref_service_->GetInteger(prefs::kPrefPiPControlsAutoHideDelayMs));
}

void AstraPipControlsModel::SetAutoHideDelay(base::TimeDelta delay) {
  int ms = static_cast<int>(delay.InMilliseconds());
  if (ms < 0) {
    ms = 0;
  }
  if (pref_service_->GetInteger(prefs::kPrefPiPControlsAutoHideDelayMs) == ms) {
    return;
  }
  pref_service_->SetInteger(prefs::kPrefPiPControlsAutoHideDelayMs, ms);
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

PipSizePreset AstraPipControlsModel::GetNextSizePreset(PipSizePreset current) {
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

// -- Private helpers --------------------------------------------------------

void AstraPipControlsModel::NotifyPlayStateChanged(bool playing) {
  for (auto& observer : observers_) {
    observer.OnPlayStateChanged(playing);
  }
}

void AstraPipControlsModel::NotifyMuteStateChanged(bool muted) {
  for (auto& observer : observers_) {
    observer.OnMuteStateChanged(muted);
  }
}

void AstraPipControlsModel::NotifyVolumeChanged(double volume) {
  for (auto& observer : observers_) {
    observer.OnVolumeChanged(volume);
  }
}

void AstraPipControlsModel::NotifyPlaybackRateChanged(double rate) {
  for (auto& observer : observers_) {
    observer.OnPlaybackRateChanged(rate);
  }
}

void AstraPipControlsModel::NotifySizePresetChanged(PipSizePreset preset) {
  for (auto& observer : observers_) {
    observer.OnSizePresetChanged(preset);
  }
}

void AstraPipControlsModel::NotifyAlwaysOnTopChanged(bool pinned) {
  for (auto& observer : observers_) {
    observer.OnAlwaysOnTopChanged(pinned);
  }
}

void AstraPipControlsModel::NotifyOpacityChanged(double opacity) {
  for (auto& observer : observers_) {
    observer.OnOpacityChanged(opacity);
  }
}

void AstraPipControlsModel::NotifyControlsVisibilityChanged(bool visible) {
  for (auto& observer : observers_) {
    observer.OnControlsVisibilityChanged(visible);
  }
}

void AstraPipControlsModel::NotifyControlsSettingsChanged() {
  for (auto& observer : observers_) {
    observer.OnControlsSettingsChanged();
  }
}

void AstraPipControlsModel::NotifySnapPositionChanged(PipSnapPosition position) {
  for (auto& observer : observers_) {
    observer.OnSnapPositionChanged(position);
  }
}

void AstraPipControlsModel::NotifyControlsMinimizedChanged(bool minimized) {
  for (auto& observer : observers_) {
    observer.OnControlsMinimizedChanged(minimized);
  }
}

void AstraPipControlsModel::LoadDefaultsFromPrefs() {
  // Load default size preset as the initial active preset.
  active_preset_index_ = static_cast<int>(GetDefaultSizePreset());

  // Load default snap position.
  std::string snap_str =
      pref_service_->GetString(prefs::kPrefPiPSnapPosition);
  PipSnapPosition snap = AstraPipService::SnapPositionFromString(snap_str);
  snap_position_index_ = static_cast<int>(snap);

  // Load default always-on-top.
  is_pinned_ = pref_service_->GetBoolean(prefs::kPrefPiPAlwaysOnTop);

  // Load default opacity.
  opacity_ = pref_service_->GetDouble(prefs::kPrefPiPOpacity);
}

}  // namespace astra
