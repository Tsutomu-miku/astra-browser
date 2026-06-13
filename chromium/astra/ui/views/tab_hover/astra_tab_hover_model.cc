#include "astra/ui/views/tab_hover/astra_tab_hover_model.h"

#include <algorithm>
#include <string>

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "components/prefs/pref_service.h"
#include "ui/gfx/geometry/size.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Preview size pixel dimensions.
constexpr gfx::Size kSmallPreviewSize{200, 100};
constexpr gfx::Size kMediumPreviewSize{316, 128};
constexpr gfx::Size kLargePreviewSize{400, 220};

// Clamp an integer delay in ms to valid range.
int ClampDelayMs(int delay_ms) {
  return std::clamp(delay_ms, 0, 10000);
}

}  // namespace

// =========================================================================
// Construction / destruction
// =========================================================================

AstraTabHoverModel::AstraTabHoverModel() = default;

AstraTabHoverModel::~AstraTabHoverModel() = default;

// =========================================================================
// Hover visibility
// =========================================================================

void AstraTabHoverModel::ShowHover() {
  if (is_hover_shown_) {
    return;
  }
  is_hover_shown_ = true;
  NotifyHoverShown();
}

void AstraTabHoverModel::HideHover() {
  if (!is_hover_shown_) {
    return;
  }
  is_hover_shown_ = false;
  NotifyHoverHidden();
}

// =========================================================================
// Tab data
// =========================================================================

void AstraTabHoverModel::SetTabData(const AstraTabHoverTabData& data) {
  tab_data_ = data;
  NotifyTabDataChanged();
}

void AstraTabHoverModel::SetTabTitle(const std::u16string& title) {
  if (tab_data_.title == title) {
    return;
  }
  tab_data_.title = title;
  NotifyTabDataChanged();
}

void AstraTabHoverModel::SetTabUrl(const GURL& url) {
  if (tab_data_.url == url) {
    return;
  }
  tab_data_.url = url;
  NotifyTabDataChanged();
}

void AstraTabHoverModel::SetFavicon(const gfx::ImageSkia& favicon) {
  tab_data_.favicon = favicon;
  NotifyTabDataChanged();
}

void AstraTabHoverModel::SetMediaState(AstraTabHoverMediaState state) {
  if (tab_data_.media_state == state) {
    return;
  }
  tab_data_.media_state = state;
  NotifyTabDataChanged();
}

void AstraTabHoverModel::SetMuted(bool muted) {
  if (tab_data_.is_muted == muted) {
    return;
  }
  tab_data_.is_muted = muted;
  NotifyTabDataChanged();
}

void AstraTabHoverModel::SetTabIndex(int index) {
  if (tab_data_.tab_index == index) {
    return;
  }
  tab_data_.tab_index = index;
  NotifyTabDataChanged();
}

// =========================================================================
// Preview image
// =========================================================================

void AstraTabHoverModel::SetPreviewImageState(
    const AstraTabHoverPreviewImageState& state) {
  preview_image_state_ = state;
  NotifyPreviewImageChanged();
}

void AstraTabHoverModel::SetPreviewImage(const gfx::ImageSkia& image,
                                         const gfx::Size& dimensions) {
  preview_image_state_.has_image = !image.isNull();
  preview_image_state_.dimensions = dimensions;
  preview_image_state_.loading_state =
      image.isNull() ? AstraTabHoverImageLoadingState::kFailed
                     : AstraTabHoverImageLoadingState::kLoaded;
  NotifyPreviewImageChanged();
}

void AstraTabHoverModel::SetPreviewImageLoadingState(
    AstraTabHoverImageLoadingState state) {
  if (preview_image_state_.loading_state == state) {
    return;
  }
  preview_image_state_.loading_state = state;
  NotifyPreviewImageChanged();
}

void AstraTabHoverModel::ClearPreviewImage() {
  if (!preview_image_state_.has_image &&
      preview_image_state_.loading_state ==
          AstraTabHoverImageLoadingState::kNotLoaded) {
    return;
  }
  preview_image_state_.has_image = false;
  preview_image_state_.dimensions = gfx::Size();
  preview_image_state_.loading_state = AstraTabHoverImageLoadingState::kNotLoaded;
  NotifyPreviewImageChanged();
}

// =========================================================================
// Peek mode
// =========================================================================

void AstraTabHoverModel::SetPeekState(const AstraTabHoverPeekState& state) {
  peek_state_ = state;
  NotifyPeekModeChanged();
}

void AstraTabHoverModel::StartPeek() {
  if (peek_state_.is_peeking) {
    return;
  }
  peek_state_.is_peeking = true;
  NotifyPeekModeChanged();
}

void AstraTabHoverModel::EndPeek() {
  if (!peek_state_.is_peeking) {
    return;
  }
  peek_state_.is_peeking = false;
  NotifyPeekModeChanged();
}

void AstraTabHoverModel::SetPeekSize(AstraTabHoverPreviewSize size) {
  if (peek_state_.peek_size == size) {
    return;
  }
  peek_state_.peek_size = size;
  NotifyPeekModeChanged();
}

void AstraTabHoverModel::SetPeekQuality(int quality) {
  int clamped = ClampPeekQuality(quality);
  if (peek_state_.peek_quality == clamped) {
    return;
  }
  peek_state_.peek_quality = clamped;
  NotifyPeekModeChanged();
}

// =========================================================================
// Presentation settings — individual setters
// =========================================================================

void AstraTabHoverModel::SetSettings(const AstraTabHoverSettings& settings) {
  settings_ = settings;
  NotifyHoverSettingsChanged();
}

void AstraTabHoverModel::SetShowTabHoverCards(bool show) {
  if (settings_.show_tab_hover_cards == show) {
    return;
  }
  settings_.show_tab_hover_cards = show;
  NotifyHoverSettingsChanged();
}

void AstraTabHoverModel::SetHoverShowDelay(base::TimeDelta delay) {
  base::TimeDelta clamped = ClampDelay(delay);
  if (settings_.hover_show_delay == clamped) {
    return;
  }
  settings_.hover_show_delay = clamped;
  NotifyHoverSettingsChanged();
}

void AstraTabHoverModel::SetHoverHideDelay(base::TimeDelta delay) {
  base::TimeDelta clamped = ClampDelay(delay);
  if (settings_.hover_hide_delay == clamped) {
    return;
  }
  settings_.hover_hide_delay = clamped;
  NotifyHoverSettingsChanged();
}

void AstraTabHoverModel::SetShowPreviewImage(bool show) {
  if (settings_.show_preview_image == show) {
    return;
  }
  settings_.show_preview_image = show;
  NotifyHoverSettingsChanged();
}

void AstraTabHoverModel::SetShowTabTitle(bool show) {
  if (settings_.show_tab_title == show) {
    return;
  }
  settings_.show_tab_title = show;
  NotifyHoverSettingsChanged();
}

void AstraTabHoverModel::SetShowTabUrl(bool show) {
  if (settings_.show_tab_url == show) {
    return;
  }
  settings_.show_tab_url = show;
  NotifyHoverSettingsChanged();
}

void AstraTabHoverModel::SetShowFavicon(bool show) {
  if (settings_.show_favicon == show) {
    return;
  }
  settings_.show_favicon = show;
  NotifyHoverSettingsChanged();
}

void AstraTabHoverModel::SetShowCloseButton(bool show) {
  if (settings_.show_close_button == show) {
    return;
  }
  settings_.show_close_button = show;
  NotifyHoverSettingsChanged();
}

void AstraTabHoverModel::SetPreviewImageSize(AstraTabHoverPreviewSize size) {
  if (settings_.preview_image_size == size) {
    return;
  }
  settings_.preview_image_size = size;
  NotifyHoverSettingsChanged();
}

void AstraTabHoverModel::SetCardPosition(AstraTabHoverCardPosition position) {
  if (settings_.card_position == position) {
    return;
  }
  settings_.card_position = position;
  NotifyHoverSettingsChanged();
}

void AstraTabHoverModel::SetEnablePeekMode(bool enable) {
  if (settings_.enable_peek_mode == enable) {
    return;
  }
  settings_.enable_peek_mode = enable;
  NotifyHoverSettingsChanged();
}

void AstraTabHoverModel::SetPeekActivationDelay(base::TimeDelta delay) {
  base::TimeDelta clamped = ClampDelay(delay);
  if (settings_.peek_activation_delay == clamped) {
    return;
  }
  settings_.peek_activation_delay = clamped;
  NotifyHoverSettingsChanged();
}

void AstraTabHoverModel::SetShowTabIndex(bool show) {
  if (settings_.show_tab_index == show) {
    return;
  }
  settings_.show_tab_index = show;
  NotifyHoverSettingsChanged();
}

void AstraTabHoverModel::SetShowMediaIndicator(bool show) {
  if (settings_.show_media_indicator == show) {
    return;
  }
  settings_.show_media_indicator = show;
  NotifyHoverSettingsChanged();
}

void AstraTabHoverModel::SetShowMuteButton(bool show) {
  if (settings_.show_mute_button == show) {
    return;
  }
  settings_.show_mute_button = show;
  NotifyHoverSettingsChanged();
}

void AstraTabHoverModel::SetAnimationEnabled(bool enabled) {
  if (settings_.animation_enabled == enabled) {
    return;
  }
  settings_.animation_enabled = enabled;
  NotifyHoverSettingsChanged();
}

// =========================================================================
// Settings — bulk operations
// =========================================================================

void AstraTabHoverModel::ResetSettingsToDefaults() {
  AstraTabHoverSettings defaults;
  settings_ = defaults;
  NotifyHoverSettingsChanged();
}

void AstraTabHoverModel::ApplySettings(const AstraTabHoverSettings& settings,
                                       const SettingsApplyMask& mask) {
  bool changed = false;

  if (mask.apply_show_tab_hover_cards &&
      settings_.show_tab_hover_cards != settings.show_tab_hover_cards) {
    settings_.show_tab_hover_cards = settings.show_tab_hover_cards;
    changed = true;
  }
  if (mask.apply_hover_show_delay) {
    base::TimeDelta clamped = ClampDelay(settings.hover_show_delay);
    if (settings_.hover_show_delay != clamped) {
      settings_.hover_show_delay = clamped;
      changed = true;
    }
  }
  if (mask.apply_hover_hide_delay) {
    base::TimeDelta clamped = ClampDelay(settings.hover_hide_delay);
    if (settings_.hover_hide_delay != clamped) {
      settings_.hover_hide_delay = clamped;
      changed = true;
    }
  }
  if (mask.apply_show_preview_image &&
      settings_.show_preview_image != settings.show_preview_image) {
    settings_.show_preview_image = settings.show_preview_image;
    changed = true;
  }
  if (mask.apply_show_tab_title &&
      settings_.show_tab_title != settings.show_tab_title) {
    settings_.show_tab_title = settings.show_tab_title;
    changed = true;
  }
  if (mask.apply_show_tab_url &&
      settings_.show_tab_url != settings.show_tab_url) {
    settings_.show_tab_url = settings.show_tab_url;
    changed = true;
  }
  if (mask.apply_show_favicon &&
      settings_.show_favicon != settings.show_favicon) {
    settings_.show_favicon = settings.show_favicon;
    changed = true;
  }
  if (mask.apply_show_close_button &&
      settings_.show_close_button != settings.show_close_button) {
    settings_.show_close_button = settings.show_close_button;
    changed = true;
  }
  if (mask.apply_preview_image_size &&
      settings_.preview_image_size != settings.preview_image_size) {
    settings_.preview_image_size = settings.preview_image_size;
    changed = true;
  }
  if (mask.apply_card_position &&
      settings_.card_position != settings.card_position) {
    settings_.card_position = settings.card_position;
    changed = true;
  }
  if (mask.apply_enable_peek_mode &&
      settings_.enable_peek_mode != settings.enable_peek_mode) {
    settings_.enable_peek_mode = settings.enable_peek_mode;
    changed = true;
  }
  if (mask.apply_peek_activation_delay) {
    base::TimeDelta clamped = ClampDelay(settings.peek_activation_delay);
    if (settings_.peek_activation_delay != clamped) {
      settings_.peek_activation_delay = clamped;
      changed = true;
    }
  }
  if (mask.apply_show_tab_index &&
      settings_.show_tab_index != settings.show_tab_index) {
    settings_.show_tab_index = settings.show_tab_index;
    changed = true;
  }
  if (mask.apply_show_media_indicator &&
      settings_.show_media_indicator != settings.show_media_indicator) {
    settings_.show_media_indicator = settings.show_media_indicator;
    changed = true;
  }
  if (mask.apply_show_mute_button &&
      settings_.show_mute_button != settings.show_mute_button) {
    settings_.show_mute_button = settings.show_mute_button;
    changed = true;
  }
  if (mask.apply_animation_enabled &&
      settings_.animation_enabled != settings.animation_enabled) {
    settings_.animation_enabled = settings.animation_enabled;
    changed = true;
  }

  if (changed) {
    NotifyHoverSettingsChanged();
  }
}

// =========================================================================
// Persistence
// =========================================================================

void AstraTabHoverModel::LoadFromPrefs(PrefService* prefs) {
  if (!prefs) {
    return;
  }

  AstraTabHoverSettings loaded;

  loaded.show_tab_hover_cards =
      prefs->GetBoolean(prefs::kPrefTabHoverShowCards);

  loaded.hover_show_delay = base::Milliseconds(
      ClampDelayMs(prefs->GetInteger(prefs::kPrefTabHoverShowDelayMs)));

  loaded.hover_hide_delay = base::Milliseconds(
      ClampDelayMs(prefs->GetInteger(prefs::kPrefTabHoverHideDelayMs)));

  loaded.show_preview_image =
      prefs->GetBoolean(prefs::kPrefTabHoverShowPreviewImage);

  loaded.show_tab_title = prefs->GetBoolean(prefs::kPrefTabHoverShowTitle);

  loaded.show_tab_url = prefs->GetBoolean(prefs::kPrefTabHoverShowUrl);

  loaded.show_favicon = prefs->GetBoolean(prefs::kPrefTabHoverShowFavicon);

  loaded.show_close_button =
      prefs->GetBoolean(prefs::kPrefTabHoverShowCloseButton);

  loaded.preview_image_size = ClampPreviewSize(
      prefs->GetInteger(prefs::kPrefTabHoverPreviewImageSize));

  loaded.card_position = ClampCardPosition(
      prefs->GetInteger(prefs::kPrefTabHoverCardPosition));

  loaded.enable_peek_mode =
      prefs->GetBoolean(prefs::kPrefTabHoverEnablePeekMode);

  loaded.peek_activation_delay = base::Milliseconds(
      ClampDelayMs(prefs->GetInteger(prefs::kPrefTabHoverPeekDelayMs)));

  loaded.show_tab_index = prefs->GetBoolean(prefs::kPrefTabHoverShowTabIndex);

  loaded.show_media_indicator =
      prefs->GetBoolean(prefs::kPrefTabHoverShowMediaIndicator);

  loaded.show_mute_button =
      prefs->GetBoolean(prefs::kPrefTabHoverShowMuteButton);

  loaded.animation_enabled =
      prefs->GetBoolean(prefs::kPrefTabHoverAnimationEnabled);

  // Only notify if something actually changed.
  bool changed =
      loaded.show_tab_hover_cards != settings_.show_tab_hover_cards ||
      loaded.hover_show_delay != settings_.hover_show_delay ||
      loaded.hover_hide_delay != settings_.hover_hide_delay ||
      loaded.show_preview_image != settings_.show_preview_image ||
      loaded.show_tab_title != settings_.show_tab_title ||
      loaded.show_tab_url != settings_.show_tab_url ||
      loaded.show_favicon != settings_.show_favicon ||
      loaded.show_close_button != settings_.show_close_button ||
      loaded.preview_image_size != settings_.preview_image_size ||
      loaded.card_position != settings_.card_position ||
      loaded.enable_peek_mode != settings_.enable_peek_mode ||
      loaded.peek_activation_delay != settings_.peek_activation_delay ||
      loaded.show_tab_index != settings_.show_tab_index ||
      loaded.show_media_indicator != settings_.show_media_indicator ||
      loaded.show_mute_button != settings_.show_mute_button ||
      loaded.animation_enabled != settings_.animation_enabled;

  if (changed) {
    settings_ = loaded;
    NotifyHoverSettingsChanged();
  }
}

void AstraTabHoverModel::SaveToPrefs(PrefService* prefs) const {
  if (!prefs) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefTabHoverShowCards,
                    settings_.show_tab_hover_cards);

  prefs->SetInteger(prefs::kPrefTabHoverShowDelayMs,
                    settings_.hover_show_delay.InMilliseconds());

  prefs->SetInteger(prefs::kPrefTabHoverHideDelayMs,
                    settings_.hover_hide_delay.InMilliseconds());

  prefs->SetBoolean(prefs::kPrefTabHoverShowPreviewImage,
                    settings_.show_preview_image);

  prefs->SetBoolean(prefs::kPrefTabHoverShowTitle,
                    settings_.show_tab_title);

  prefs->SetBoolean(prefs::kPrefTabHoverShowUrl,
                    settings_.show_tab_url);

  prefs->SetBoolean(prefs::kPrefTabHoverShowFavicon,
                    settings_.show_favicon);

  prefs->SetBoolean(prefs::kPrefTabHoverShowCloseButton,
                    settings_.show_close_button);

  prefs->SetInteger(prefs::kPrefTabHoverPreviewImageSize,
                    static_cast<int>(settings_.preview_image_size));

  prefs->SetInteger(prefs::kPrefTabHoverCardPosition,
                    static_cast<int>(settings_.card_position));

  prefs->SetBoolean(prefs::kPrefTabHoverEnablePeekMode,
                    settings_.enable_peek_mode);

  prefs->SetInteger(prefs::kPrefTabHoverPeekDelayMs,
                    settings_.peek_activation_delay.InMilliseconds());

  prefs->SetBoolean(prefs::kPrefTabHoverShowTabIndex,
                    settings_.show_tab_index);

  prefs->SetBoolean(prefs::kPrefTabHoverShowMediaIndicator,
                    settings_.show_media_indicator);

  prefs->SetBoolean(prefs::kPrefTabHoverShowMuteButton,
                    settings_.show_mute_button);

  prefs->SetBoolean(prefs::kPrefTabHoverAnimationEnabled,
                    settings_.animation_enabled);
}

// =========================================================================
// Utility methods
// =========================================================================

std::u16string AstraTabHoverModel::FormatTabTitle(const std::u16string& title,
                                                  size_t max_length) {
  if (title.length() <= max_length) {
    return title;
  }
  // Truncate and add ellipsis.
  // TODO(astra): Use proper Unicode-aware truncation with ellipsis.
  // For now, simple truncation + "..." is sufficient.
  std::u16string truncated = title.substr(0, max_length);
  truncated += u"\u2026";  // Ellipsis character
  return truncated;
}

std::u16string AstraTabHoverModel::FormatDomainFromUrl(const GURL& url) {
  if (!url.is_valid() || url.host().empty()) {
    return std::u16string();
  }
  return base::UTF8ToUTF16(url.host());
}

base::TimeDelta AstraTabHoverModel::ClampDelay(base::TimeDelta delay) {
  if (delay < kMinDelay) {
    return kMinDelay;
  }
  if (delay > kMaxDelay) {
    return kMaxDelay;
  }
  return delay;
}

AstraTabHoverPreviewSize AstraTabHoverModel::ClampPreviewSize(int value) {
  if (value < 0 || value > static_cast<int>(AstraTabHoverPreviewSize::kLarge)) {
    return AstraTabHoverPreviewSize::kMedium;
  }
  return static_cast<AstraTabHoverPreviewSize>(value);
}

AstraTabHoverCardPosition AstraTabHoverModel::ClampCardPosition(int value) {
  if (value < 0 || value > static_cast<int>(AstraTabHoverCardPosition::kAuto)) {
    return AstraTabHoverCardPosition::kAuto;
  }
  return static_cast<AstraTabHoverCardPosition>(value);
}

int AstraTabHoverModel::ClampPeekQuality(int quality) {
  return std::clamp(quality, kMinPeekQuality, kMaxPeekQuality);
}

gfx::Size AstraTabHoverModel::GetPreviewSizePixels(
    AstraTabHoverPreviewSize size) {
  switch (size) {
    case AstraTabHoverPreviewSize::kSmall:
      return kSmallPreviewSize;
    case AstraTabHoverPreviewSize::kMedium:
      return kMediumPreviewSize;
    case AstraTabHoverPreviewSize::kLarge:
      return kLargePreviewSize;
  }
  return kMediumPreviewSize;
}

// =========================================================================
// Observer management
// =========================================================================

void AstraTabHoverModel::AddObserver(AstraTabHoverModelObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraTabHoverModel::RemoveObserver(AstraTabHoverModelObserver* observer) {
  observers_.RemoveObserver(observer);
}

// =========================================================================
// Notification helpers
// =========================================================================

void AstraTabHoverModel::NotifyHoverShown() {
  for (AstraTabHoverModelObserver& observer : observers_) {
    observer.OnHoverShown();
  }
}

void AstraTabHoverModel::NotifyHoverHidden() {
  for (AstraTabHoverModelObserver& observer : observers_) {
    observer.OnHoverHidden();
  }
}

void AstraTabHoverModel::NotifyPreviewImageChanged() {
  for (AstraTabHoverModelObserver& observer : observers_) {
    observer.OnPreviewImageChanged();
  }
}

void AstraTabHoverModel::NotifyTabDataChanged() {
  for (AstraTabHoverModelObserver& observer : observers_) {
    observer.OnTabDataChanged();
  }
}

void AstraTabHoverModel::NotifyPeekModeChanged() {
  for (AstraTabHoverModelObserver& observer : observers_) {
    observer.OnPeekModeChanged();
  }
}

void AstraTabHoverModel::NotifyHoverSettingsChanged() {
  for (AstraTabHoverModelObserver& observer : observers_) {
    observer.OnHoverSettingsChanged();
  }
}

}  // namespace astra
