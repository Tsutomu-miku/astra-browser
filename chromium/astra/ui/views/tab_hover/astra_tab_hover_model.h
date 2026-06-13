#ifndef ASTRA_UI_VIEWS_TAB_HOVER_ASTRA_TAB_HOVER_MODEL_H_
#define ASTRA_UI_VIEWS_TAB_HOVER_ASTRA_TAB_HOVER_MODEL_H_

#include <string>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "url/gurl.h"

class PrefService;

namespace astra {

// =========================================================================
// AstraTabHoverModel — state and settings for tab hover previews
// =========================================================================
//
// Central model for tab hover preview state and presentation settings.
// Owns the truth for:
//   - Hover visibility state (shown/hidden)
//   - Tab data being previewed (title, URL, favicon, media state, index)
//   - Preview image state (has_image, dimensions, loading state)
//   - Peek mode state (is_peeking, peek_size, peek_quality)
//   - Presentation settings (16+ configurable options, persisted via PrefService)
//
// Architecture:
//   - Model owns state and logic.
//   - View (AstraTabHoverPreviewView) renders model state and handles input.
//   - Controller (AstraTabHoverPeekController) drives model state changes.
//   - UI never owns truth state — model owns it.
//
// Persistence:
//   - All presentation settings are persisted via PrefService.
//   - No custom file I/O.
//   - LoadFromPrefs / SaveToPrefs handle round-trip.
//
// Observer pattern:
//   - Observers derive from AstraTabHoverModelObserver (base::CheckedObserver).
//   - All observer methods have empty default implementations.
//   - Subclasses override only the methods they care about.
//
// Chromium subsystem reused: PrefService (persistence), base::ObserverList.
// =========================================================================

// Preview image size enum.
enum class AstraTabHoverPreviewSize {
  kSmall,
  kMedium,
  kLarge,
};

// Card position enum.
enum class AstraTabHoverCardPosition {
  kAbove,
  kBelow,
  kAuto,
};

// Preview image loading state.
enum class AstraTabHoverImageLoadingState {
  kNotLoaded,   // No image has been requested.
  kLoading,     // Image is currently loading.
  kLoaded,      // Image has loaded successfully.
  kFailed,      // Image loading failed.
};

// Media playback state for the tab.
enum class AstraTabHoverMediaState {
  kNone,        // No media playing.
  kPlaying,     // Media is playing.
  kPaused,      // Media is paused.
  kMuted,       // Media is muted (may be playing or paused).
};

// =========================================================================
// Tab data struct
// =========================================================================

// Data about the tab being previewed.
struct AstraTabHoverTabData {
  std::u16string title;
  GURL url;
  gfx::ImageSkia favicon;
  AstraTabHoverMediaState media_state = AstraTabHoverMediaState::kNone;
  bool is_muted = false;
  int tab_index = -1;

  bool has_valid_index() const { return tab_index >= 0; }
  bool has_media() const { return media_state != AstraTabHoverMediaState::kNone; }
};

// =========================================================================
// Preview image state
// =========================================================================

// State of the preview thumbnail image.
struct AstraTabHoverPreviewImageState {
  bool has_image = false;
  gfx::Size dimensions;
  AstraTabHoverImageLoadingState loading_state =
      AstraTabHoverImageLoadingState::kNotLoaded;

  bool is_loading() const {
    return loading_state == AstraTabHoverImageLoadingState::kLoading;
  }
  bool is_loaded() const {
    return loading_state == AstraTabHoverImageLoadingState::kLoaded;
  }
  bool has_failed() const {
    return loading_state == AstraTabHoverImageLoadingState::kFailed;
  }
};

// =========================================================================
// Peek mode state
// =========================================================================

// State of peek mode (expanded hover preview).
struct AstraTabHoverPeekState {
  bool is_peeking = false;
  AstraTabHoverPreviewSize peek_size = AstraTabHoverPreviewSize::kMedium;
  // Quality as a 0-100 percentage for the peek preview.
  int peek_quality = 70;

  bool is_expanded() const {
    return is_peeking && peek_size != AstraTabHoverPreviewSize::kSmall;
  }
};

// =========================================================================
// Presentation settings
// =========================================================================

// All configurable presentation settings for tab hover previews.
// These are persisted via PrefService.
struct AstraTabHoverSettings {
  // -- Visibility ---------------------------------------------------------

  bool show_tab_hover_cards = true;
  bool show_preview_image = true;
  bool show_tab_title = true;
  bool show_tab_url = true;
  bool show_favicon = true;
  bool show_close_button = true;

  // -- Timing -------------------------------------------------------------

  base::TimeDelta hover_show_delay = base::Milliseconds(500);
  base::TimeDelta hover_hide_delay = base::Milliseconds(300);

  // -- Sizing / position --------------------------------------------------

  AstraTabHoverPreviewSize preview_image_size =
      AstraTabHoverPreviewSize::kMedium;
  AstraTabHoverCardPosition card_position = AstraTabHoverCardPosition::kAuto;

  // -- Peek mode ----------------------------------------------------------

  bool enable_peek_mode = true;
  base::TimeDelta peek_activation_delay = base::Milliseconds(1500);

  // -- Additional info ----------------------------------------------------

  bool show_tab_index = false;
  bool show_media_indicator = true;
  bool show_mute_button = true;

  // -- Animation ----------------------------------------------------------

  bool animation_enabled = true;
};

// =========================================================================
// Observer interface
// =========================================================================

class AstraTabHoverModelObserver : public base::CheckedObserver {
 public:
  // Called when the hover preview is shown.
  virtual void OnHoverShown() {}

  // Called when the hover preview is hidden.
  virtual void OnHoverHidden() {}

  // Called when the preview image changes (new image, loading state change).
  virtual void OnPreviewImageChanged() {}

  // Called when the tab data changes (title, URL, favicon, media state).
  virtual void OnTabDataChanged() {}

  // Called when peek mode state changes (started, ended, size changed).
  virtual void OnPeekModeChanged() {}

  // Called when any hover presentation setting changes.
  virtual void OnHoverSettingsChanged() {}

 protected:
  ~AstraTabHoverModelObserver() override = default;
};

// =========================================================================
// Model class
// =========================================================================

class AstraTabHoverModel {
 public:
  AstraTabHoverModel();
  ~AstraTabHoverModel();

  AstraTabHoverModel(const AstraTabHoverModel&) = delete;
  AstraTabHoverModel& operator=(const AstraTabHoverModel&) = delete;

  // -- Hover visibility ---------------------------------------------------

  // Show the hover preview. Notifies observers with OnHoverShown.
  void ShowHover();

  // Hide the hover preview. Notifies observers with OnHoverHidden.
  void HideHover();

  // Returns true if the hover is currently shown.
  bool is_hover_shown() const { return is_hover_shown_; }

  // -- Tab data -----------------------------------------------------------

  // Set the full tab data. Notifies observers with OnTabDataChanged.
  void SetTabData(const AstraTabHoverTabData& data);

  // Get the current tab data.
  const AstraTabHoverTabData& tab_data() const { return tab_data_; }

  // Individual setters (each notifies OnTabDataChanged).
  void SetTabTitle(const std::u16string& title);
  void SetTabUrl(const GURL& url);
  void SetFavicon(const gfx::ImageSkia& favicon);
  void SetMediaState(AstraTabHoverMediaState state);
  void SetMuted(bool muted);
  void SetTabIndex(int index);

  // -- Preview image ------------------------------------------------------

  // Set the full preview image state.
  // Notifies observers with OnPreviewImageChanged.
  void SetPreviewImageState(const AstraTabHoverPreviewImageState& state);

  // Get the current preview image state.
  const AstraTabHoverPreviewImageState& preview_image_state() const {
    return preview_image_state_;
  }

  // Set the preview image and mark as loaded.
  void SetPreviewImage(const gfx::ImageSkia& image, const gfx::Size& dimensions);

  // Set the loading state.
  void SetPreviewImageLoadingState(AstraTabHoverImageLoadingState state);

  // Clear the preview image (reset to not loaded).
  void ClearPreviewImage();

  // -- Peek mode ----------------------------------------------------------

  // Set the full peek state.
  // Notifies observers with OnPeekModeChanged.
  void SetPeekState(const AstraTabHoverPeekState& state);

  // Get the current peek state.
  const AstraTabHoverPeekState& peek_state() const { return peek_state_; }

  // Start peek mode.
  void StartPeek();

  // End peek mode.
  void EndPeek();

  // Set the peek preview size.
  void SetPeekSize(AstraTabHoverPreviewSize size);

  // Set the peek quality (0-100).
  void SetPeekQuality(int quality);

  // -- Presentation settings ----------------------------------------------

  // Get the current settings.
  const AstraTabHoverSettings& settings() const { return settings_; }

  // Set all settings at once. Notifies observers with OnHoverSettingsChanged.
  void SetSettings(const AstraTabHoverSettings& settings);

  // Individual setting setters (each notifies OnHoverSettingsChanged).
  void SetShowTabHoverCards(bool show);
  void SetHoverShowDelay(base::TimeDelta delay);
  void SetHoverHideDelay(base::TimeDelta delay);
  void SetShowPreviewImage(bool show);
  void SetShowTabTitle(bool show);
  void SetShowTabUrl(bool show);
  void SetShowFavicon(bool show);
  void SetShowCloseButton(bool show);
  void SetPreviewImageSize(AstraTabHoverPreviewSize size);
  void SetCardPosition(AstraTabHoverCardPosition position);
  void SetEnablePeekMode(bool enable);
  void SetPeekActivationDelay(base::TimeDelta delay);
  void SetShowTabIndex(bool show);
  void SetShowMediaIndicator(bool show);
  void SetShowMuteButton(bool show);
  void SetAnimationEnabled(bool enabled);

  // Bulk: reset all settings to defaults.
  void ResetSettingsToDefaults();

  // Bulk: apply a group of settings (helper for bulk operations).
  // Takes a settings object and applies only fields where |apply_mask| is true.
  // Notifies observers once after all changes.
  struct SettingsApplyMask {
    bool apply_show_tab_hover_cards = false;
    bool apply_hover_show_delay = false;
    bool apply_hover_hide_delay = false;
    bool apply_show_preview_image = false;
    bool apply_show_tab_title = false;
    bool apply_show_tab_url = false;
    bool apply_show_favicon = false;
    bool apply_show_close_button = false;
    bool apply_preview_image_size = false;
    bool apply_card_position = false;
    bool apply_enable_peek_mode = false;
    bool apply_peek_activation_delay = false;
    bool apply_show_tab_index = false;
    bool apply_show_media_indicator = false;
    bool apply_show_mute_button = false;
    bool apply_animation_enabled = false;
  };
  void ApplySettings(const AstraTabHoverSettings& settings,
                     const SettingsApplyMask& mask);

  // -- Persistence --------------------------------------------------------

  // Load presentation settings from PrefService.
  // Notifies observers with OnHoverSettingsChanged if values changed.
  void LoadFromPrefs(PrefService* prefs);

  // Save presentation settings to PrefService.
  void SaveToPrefs(PrefService* prefs) const;

  // -- Utility methods ----------------------------------------------------

  // Format a tab title for display, truncating if needed.
  // |max_length| is the maximum number of characters before truncation.
  // Returns the title truncated with ellipsis if too long.
  static std::u16string FormatTabTitle(const std::u16string& title,
                                       size_t max_length = 60);

  // Format a domain from a URL for display.
  // Returns the host portion of the URL, or an empty string if invalid.
  static std::u16string FormatDomainFromUrl(const GURL& url);

  // Clamp a delay value to valid range.
  // Min: 0ms, Max: 10000ms (10 seconds).
  static base::TimeDelta ClampDelay(base::TimeDelta delay);

  // Clamp a preview image size enum to valid values.
  static AstraTabHoverPreviewSize ClampPreviewSize(int value);

  // Clamp a card position enum to valid values.
  static AstraTabHoverCardPosition ClampCardPosition(int value);

  // Clamp peek quality to valid range (0-100).
  static int ClampPeekQuality(int quality);

  // Get the pixel dimensions for a preview size enum.
  static gfx::Size GetPreviewSizePixels(AstraTabHoverPreviewSize size);

  // -- Observers ----------------------------------------------------------

  void AddObserver(AstraTabHoverModelObserver* observer);
  void RemoveObserver(AstraTabHoverModelObserver* observer);

  // -- Constants ----------------------------------------------------------

  // Minimum delay for hover show/hide (0ms).
  static constexpr base::TimeDelta kMinDelay = base::Milliseconds(0);

  // Maximum delay for hover show/hide (10 seconds).
  static constexpr base::TimeDelta kMaxDelay = base::Milliseconds(10000);

  // Maximum tab title display length.
  static constexpr size_t kMaxTitleLength = 60;

  // Default peek quality percentage.
  static constexpr int kDefaultPeekQuality = 70;

  // Minimum peek quality.
  static constexpr int kMinPeekQuality = 10;

  // Maximum peek quality.
  static constexpr int kMaxPeekQuality = 100;

 private:
  // Notify observers of hover shown.
  void NotifyHoverShown();

  // Notify observers of hover hidden.
  void NotifyHoverHidden();

  // Notify observers of preview image change.
  void NotifyPreviewImageChanged();

  // Notify observers of tab data change.
  void NotifyTabDataChanged();

  // Notify observers of peek mode change.
  void NotifyPeekModeChanged();

  // Notify observers of settings change.
  void NotifyHoverSettingsChanged();

  // -- State --------------------------------------------------------------

  // Whether the hover preview is currently shown.
  bool is_hover_shown_ = false;

  // Tab data for the current preview.
  AstraTabHoverTabData tab_data_;

  // Preview image state.
  AstraTabHoverPreviewImageState preview_image_state_;

  // Peek mode state.
  AstraTabHoverPeekState peek_state_;

  // Presentation settings.
  AstraTabHoverSettings settings_;

  // Observers.
  base::ObserverList<AstraTabHoverModelObserver> observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_TAB_HOVER_ASTRA_TAB_HOVER_MODEL_H_
