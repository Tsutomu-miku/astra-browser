#include "astra/browser/astra_pip_service.h"

#include <algorithm>
#include <cmath>

#include "base/no_destructor.h"
#include "base/ranges/algorithm.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/size.h"

#include "astra/browser/astra_prefs.h"
#include "astra/browser/astra_tab_features.h"

namespace astra {

namespace {

// Reference heights for each PiP size preset.
// Width is computed from the content's aspect ratio.
constexpr int kSmallPipHeight = 160;
constexpr int kMediumPipHeight = 240;
constexpr int kLargePipHeight = 320;

// Default aspect ratio used when the actual content aspect ratio is unknown.
// 16:9 is the most common video aspect ratio.
constexpr float kDefaultAspectRatio = 16.0f / 9.0f;

// Minimum allowed PiP window opacity.
constexpr double kMinPiPOpacity = 0.2;

const char kPresetSmall[] = "small";
const char kPresetMedium[] = "medium";
const char kPresetLarge[] = "large";

const char kSnapTopLeft[] = "top_left";
const char kSnapTopRight[] = "top_right";
const char kSnapBottomLeft[] = "bottom_left";
const char kSnapBottomRight[] = "bottom_right";
const char kSnapFreeFloating[] = "free_floating";

}  // namespace

// ---------------------------------------------------------------------------
// AstraPipService
// ---------------------------------------------------------------------------

AstraPipService::AstraPipService(Profile* profile) : profile_(profile) {
  // Load persisted preferences from the profile's PrefService.
  LoadFromPrefs();
}

AstraPipService::~AstraPipService() = default;

void AstraPipService::Shutdown() {
  // KeyedService shutdown: clear all observer references before the
  // profile goes away.
  observers_.Clear();
  profile_ = nullptr;
}

// -- Observers ---------------------------------------------------------------

void AstraPipService::AddObserver(AstraPipObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraPipService::RemoveObserver(AstraPipObserver* observer) {
  observers_.RemoveObserver(observer);
}

// -- PiP state queries -------------------------------------------------------

bool AstraPipService::IsTabInPip(content::WebContents* web_contents) const {
  if (!web_contents) {
    return false;
  }

  // TODO(astra): Query Chromium's PictureInPictureWindowController for the
  // actual PiP state instead of relying on Astra metadata.  The metadata
  // is a projection that should be kept in sync with the real state via
  // observer hooks into the PiP controller.
  //
  // Chromium owner: PictureInPictureWindowController
  //   (chrome/browser/picture_in_picture/picture_in_picture_window_controller.h)
  // Patch point: Add an Astra observer to PiP window controller lifecycle.
  AstraTabFeatures* features = AstraTabFeatures::FromWebContents(web_contents);
  return features && features->is_pip_tab();
}

std::vector<content::WebContents*> AstraPipService::GetPiPTabs() const {
  std::vector<content::WebContents*> pip_tabs;

  if (!profile_) {
    return pip_tabs;
  }

  // TODO(astra): Iterate over all browser windows for this profile using
  // BrowserList::GetInstance() and collect PiP tabs from each window's
  // TabStripModel.  Currently this is a placeholder that assumes a single
  // active browser window.
  //
  // Chromium owner: BrowserList (chrome/browser/ui/browser_list.h)
  // Chromium owner: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)
  //
  // For now, we check the last active browser for this profile.
  Browser* browser = nullptr;
  for (auto* b : *BrowserList::GetInstance()) {
    if (b->profile() == profile_) {
      browser = b;
      break;
    }
  }

  if (!browser || !browser->tab_strip_model()) {
    return pip_tabs;
  }

  TabStripModel* tab_strip = browser->tab_strip_model();
  int count = tab_strip->GetTabCount();
  for (int i = 0; i < count; ++i) {
    content::WebContents* web_contents = tab_strip->GetWebContentsAt(i);
    if (web_contents && IsTabInPip(web_contents)) {
      pip_tabs.push_back(web_contents);
    }
  }

  return pip_tabs;
}

// -- PiP control -------------------------------------------------------------

void AstraPipService::EnterPipForTab(content::WebContents* web_contents) {
  if (!web_contents || IsTabInPip(web_contents)) {
    return;
  }

  // TODO(astra): Proper integration with Chromium's PictureInPictureWindowController.
  //
  // For video PiP: the controller is already triggered by the media session
  // API or the PiP button in the video controls.  We should hook into the
  // existing PiP entry path and add Astra metadata.
  //
  // For tab PiP (showing an entire tab as PiP): Chromium may have a tab-level
  // PiP feature, or we may need to implement it as a mini widget that shows
  // the tab's WebContents.
  //
  // Chromium owner: PictureInPictureWindowController
  //   (chrome/browser/picture_in_picture/picture_in_picture_window_controller.h)
  // Chromium owner: DocumentPictureInPicture
  //   (third_party/blink/renderer/modules/document_picture_in_picture/)
  //
  // Patch point: picture_in_picture_window_controller.cc — add Astra hook
  // in OnWindowCreated or similar entry point.
  //
  // For now, set Astra metadata as a stand-in for the real PiP window.
  AstraTabFeatures* features =
      AstraTabFeatures::GetOrCreateForWebContents(web_contents);
  features->set_is_pip_tab(true);

  // Set initial window size based on the default preset.
  // TODO(astra): Read the actual video/tab aspect ratio from WebContents.
  gfx::Size initial_size =
      GetPresetSize(default_size_preset_, kDefaultAspectRatio);
  features->set_pip_window_size(initial_size);

  // Notify observers.
  for (auto& observer : observers_) {
    observer.OnPiPEntered(web_contents);
  }
}

void AstraPipService::ExitPipForTab(content::WebContents* web_contents) {
  if (!web_contents || !IsTabInPip(web_contents)) {
    return;
  }

  // TODO(astra): Integrate with PictureInPictureWindowController::Close()
  // or the equivalent exit path to actually close the PiP window.
  //
  // Chromium owner: PictureInPictureWindowController::Close()
  //   (chrome/browser/picture_in_picture/picture_in_picture_window_controller.h)
  //
  // For now, clear Astra metadata.
  AstraTabFeatures* features =
      AstraTabFeatures::GetOrCreateForWebContents(web_contents);
  features->set_is_pip_tab(false);
  features->set_pip_window_size(gfx::Size());

  // Notify observers.
  for (auto& observer : observers_) {
    observer.OnPiPExited(web_contents);
  }
}

bool AstraPipService::TogglePipForTab(content::WebContents* web_contents) {
  if (IsTabInPip(web_contents)) {
    ExitPipForTab(web_contents);
    return false;
  } else {
    EnterPipForTab(web_contents);
    return true;
  }
}

// -- Window sizing -----------------------------------------------------------

void AstraPipService::ResizePiPWindow(content::WebContents* web_contents,
                                      const gfx::Size& size) {
  if (!web_contents || !IsTabInPip(web_contents)) {
    return;
  }

  if (size.IsEmpty()) {
    return;
  }

  // TODO(astra): Wire the actual window resize to Chromium's PiP window
  // widget.  The PiP window is a views::Widget owned by
  // PictureInPictureWindowController.  We need to call SetBounds on that
  // widget or use UpdateLayerBounds on the controller.
  //
  // Chromium owner: views::Widget::SetBounds()
  //   (ui/views/widget/widget.h)
  // Chromium owner: PictureInPictureWindowController::UpdateLayerBounds()
  //   (chrome/browser/picture_in_picture/picture_in_picture_window_controller.h)
  //
  // For now, update the Astra metadata.
  AstraTabFeatures* features =
      AstraTabFeatures::GetOrCreateForWebContents(web_contents);
  features->set_pip_window_size(size);

  // Notify observers.
  for (auto& observer : observers_) {
    observer.OnPiPWindowSizeChanged(web_contents, size);
  }
}

gfx::Size AstraPipService::GetPiPWindowSize(
    content::WebContents* web_contents) const {
  if (!web_contents || !IsTabInPip(web_contents)) {
    return gfx::Size();
  }

  AstraTabFeatures* features = AstraTabFeatures::FromWebContents(web_contents);
  if (!features) {
    return gfx::Size();
  }

  return features->pip_window_size();
}

void AstraPipService::ResizePiPToPreset(content::WebContents* web_contents,
                                        PipSizePreset preset) {
  // TODO(astra): Determine the actual aspect ratio from the WebContents
  // (video natural size, or tab viewport aspect ratio).  For now, use
  // the default 16:9 ratio.
  //
  // Chromium owner: media::VideoRenderer (for video natural size)
  //   or content::WebContents::GetContainerBounds() for tab viewport.
  gfx::Size size = GetPresetSize(preset, kDefaultAspectRatio);
  ResizePiPWindow(web_contents, size);
}

PipSizePreset AstraPipService::GetDefaultSizePreset() const {
  return default_size_preset_;
}

void AstraPipService::SetDefaultSizePreset(PipSizePreset preset) {
  if (default_size_preset_ == preset) {
    return;
  }
  default_size_preset_ = preset;
  SaveToPrefs();

  for (auto& observer : observers_) {
    observer.OnPiPDefaultSizePresetChanged(default_size_preset_);
  }
  NotifyPreferencesChanged();
}

bool AstraPipService::GetAlwaysOnTop() const {
  return always_on_top_;
}

void AstraPipService::SetAlwaysOnTop(bool always_on_top) {
  if (always_on_top_ == always_on_top) {
    return;
  }
  always_on_top_ = always_on_top;
  SaveToPrefs();

  // TODO(astra): Apply the always-on-top setting to all active PiP windows
  // via views::Widget::SetZOrderLevel() or the PiP window controller.
  //
  // Chromium owner: views::Widget::SetZOrderLevel()
  //   (ui/views/widget/widget.h)
  // Chromium owner: PictureInPictureWindowController
  //   (chrome/browser/picture_in_picture/picture_in_picture_window_controller.h)

  for (auto& observer : observers_) {
    observer.OnPiPAlwaysOnTopChanged(always_on_top_);
  }
  NotifyPreferencesChanged();
}

bool AstraPipService::ToggleAlwaysOnTop() {
  SetAlwaysOnTop(!always_on_top_);
  return always_on_top_;
}

// -- Window position ---------------------------------------------------------

gfx::Point AstraPipService::GetPiPWindowPosition(
    content::WebContents* web_contents) const {
  if (!web_contents || !IsTabInPip(web_contents)) {
    return gfx::Point();
  }

  AstraTabFeatures* features = AstraTabFeatures::FromWebContents(web_contents);
  if (!features) {
    return gfx::Point();
  }

  // TODO(astra): Add pip_window_position_ field to AstraTabFeatures.
  // For now, return (0, 0) as the default position.
  // Chromium owner: views::Widget::GetBounds() for the actual position.
  return gfx::Point();
}

void AstraPipService::SetPiPWindowPosition(content::WebContents* web_contents,
                                           const gfx::Point& position) {
  if (!web_contents || !IsTabInPip(web_contents)) {
    return;
  }

  // TODO(astra): Store and apply the position via AstraTabFeatures and
  // views::Widget::SetPosition() on the PiP window.
  //
  // Chromium owner: views::Widget::SetPosition()
  //   (ui/views/widget/widget.h)

  for (auto& observer : observers_) {
    observer.OnPiPWindowPositionChanged(web_contents, position);
  }
}

// -- Snap position -----------------------------------------------------------

PipSnapPosition AstraPipService::GetSnapPosition() const {
  return snap_position_;
}

void AstraPipService::SetSnapPosition(PipSnapPosition position) {
  if (snap_position_ == position) {
    return;
  }
  snap_position_ = position;
  SaveToPrefs();
  NotifyPreferencesChanged();
}

bool AstraPipService::GetSnapToCornerEnabled() const {
  return snap_to_corner_enabled_;
}

void AstraPipService::SetSnapToCornerEnabled(bool enabled) {
  if (snap_to_corner_enabled_ == enabled) {
    return;
  }
  snap_to_corner_enabled_ = enabled;
  SaveToPrefs();
  NotifyPreferencesChanged();
}

// static
const char* AstraPipService::SnapPositionToString(PipSnapPosition position) {
  switch (position) {
    case PipSnapPosition::kTopLeft:
      return kSnapTopLeft;
    case PipSnapPosition::kTopRight:
      return kSnapTopRight;
    case PipSnapPosition::kBottomLeft:
      return kSnapBottomLeft;
    case PipSnapPosition::kBottomRight:
      return kSnapBottomRight;
    case PipSnapPosition::kFreeFloating:
      return kSnapFreeFloating;
  }
  return kSnapBottomRight;
}

// static
PipSnapPosition AstraPipService::SnapPositionFromString(
    const std::string& name) {
  if (name == kSnapTopLeft) {
    return PipSnapPosition::kTopLeft;
  }
  if (name == kSnapTopRight) {
    return PipSnapPosition::kTopRight;
  }
  if (name == kSnapBottomLeft) {
    return PipSnapPosition::kBottomLeft;
  }
  if (name == kSnapBottomRight) {
    return PipSnapPosition::kBottomRight;
  }
  if (name == kSnapFreeFloating) {
    return PipSnapPosition::kFreeFloating;
  }
  // Default to bottom-right for unknown values.
  return PipSnapPosition::kBottomRight;
}

// -- Opacity -----------------------------------------------------------------

double AstraPipService::GetPiPOpacity() const {
  return pip_opacity_;
}

void AstraPipService::SetPiPOpacity(double opacity) {
  // Clamp to valid range.
  if (opacity < kMinPiPOpacity) {
    opacity = kMinPiPOpacity;
  }
  if (opacity > 1.0) {
    opacity = 1.0;
  }

  if (std::fabs(pip_opacity_ - opacity) < 0.001) {
    return;
  }
  pip_opacity_ = opacity;
  SaveToPrefs();
  NotifyPreferencesChanged();
}

// -- Auto-PiP ----------------------------------------------------------------

bool AstraPipService::GetAutoPipOnTabSwitch() const {
  return auto_pip_on_tab_switch_;
}

void AstraPipService::SetAutoPipOnTabSwitch(bool enabled) {
  if (auto_pip_on_tab_switch_ == enabled) {
    return;
  }
  auto_pip_on_tab_switch_ = enabled;
  SaveToPrefs();
  NotifyPreferencesChanged();
}

// -- Multi-PiP limit ---------------------------------------------------------

int AstraPipService::GetMaxPipWindows() const {
  return max_pip_windows_;
}

void AstraPipService::SetMaxPipWindows(int max) {
  if (max < 0) {
    max = 0;
  }
  if (max_pip_windows_ == max) {
    return;
  }
  max_pip_windows_ = max;
  SaveToPrefs();
  NotifyPreferencesChanged();
}

bool AstraPipService::IsAtPipLimit() const {
  if (max_pip_windows_ <= 0) {
    return false;  // 0 means no limit
  }
  return GetPipCount() >= static_cast<size_t>(max_pip_windows_);
}

// -- Bulk operations ---------------------------------------------------------

size_t AstraPipService::ExitAllPip() {
  // Collect all PiP tabs first (to avoid mutating while iterating).
  std::vector<content::WebContents*> pip_tabs = GetPiPTabs();

  for (auto* tab : pip_tabs) {
    ExitPipForTab(tab);
  }

  return pip_tabs.size();
}

// -- Utility methods ---------------------------------------------------------

size_t AstraPipService::GetPipCount() const {
  return GetPiPTabs().size();
}

PipSizePreset AstraPipService::CycleSizePreset(
    content::WebContents* web_contents) {
  if (!web_contents || !IsTabInPip(web_contents)) {
    return default_size_preset_;
  }

  // Determine current preset from window size.
  gfx::Size current_size = GetPiPWindowSize(web_contents);
  PipSizePreset current_preset = PipSizePreset::kMedium;

  // Approximate current preset based on height.
  int height = current_size.height();
  if (height <= kSmallPipHeight + (kMediumPipHeight - kSmallPipHeight) / 2) {
    current_preset = PipSizePreset::kSmall;
  } else if (height >=
             kMediumPipHeight + (kLargePipHeight - kMediumPipHeight) / 2) {
    current_preset = PipSizePreset::kLarge;
  }

  // Cycle to next preset.
  PipSizePreset next_preset;
  switch (current_preset) {
    case PipSizePreset::kSmall:
      next_preset = PipSizePreset::kMedium;
      break;
    case PipSizePreset::kMedium:
      next_preset = PipSizePreset::kLarge;
      break;
    case PipSizePreset::kLarge:
      next_preset = PipSizePreset::kSmall;
      break;
  }

  ResizePiPToPreset(web_contents, next_preset);
  return next_preset;
}

// -- Size preset helpers -----------------------------------------------------

// static
const char* AstraPipService::PresetToString(PipSizePreset preset) {
  switch (preset) {
    case PipSizePreset::kSmall:
      return kPresetSmall;
    case PipSizePreset::kMedium:
      return kPresetMedium;
    case PipSizePreset::kLarge:
      return kPresetLarge;
  }
  return kPresetMedium;
}

// static
PipSizePreset AstraPipService::PresetFromString(const std::string& name) {
  if (name == kPresetSmall) {
    return PipSizePreset::kSmall;
  }
  if (name == kPresetLarge) {
    return PipSizePreset::kLarge;
  }
  // Default to medium for unknown values.
  return PipSizePreset::kMedium;
}

// static
gfx::Size AstraPipService::GetPresetSize(PipSizePreset preset,
                                         float aspect_ratio) {
  if (aspect_ratio <= 0.0f) {
    aspect_ratio = kDefaultAspectRatio;
  }

  int height;
  switch (preset) {
    case PipSizePreset::kSmall:
      height = kSmallPipHeight;
      break;
    case PipSizePreset::kMedium:
      height = kMediumPipHeight;
      break;
    case PipSizePreset::kLarge:
      height = kLargePipHeight;
      break;
  }

  int width = static_cast<int>(height * aspect_ratio);
  return gfx::Size(width, height);
}

// -- Private helpers ---------------------------------------------------------

void AstraPipService::LoadFromPrefs() {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }

  PrefService* prefs = profile_->GetPrefs();

  // Default PiP size preset.
  std::string preset_name = prefs->GetString(prefs::kPrefPiPDefaultSize);
  default_size_preset_ = PresetFromString(preset_name);

  // Always-on-top preference.
  always_on_top_ = prefs->GetBoolean(prefs::kPrefPiPAlwaysOnTop);

  // Snap position.
  std::string snap_name = prefs->GetString(prefs::kPrefPiPSnapPosition);
  snap_position_ = SnapPositionFromString(snap_name);

  // Snap-to-corner enabled.
  snap_to_corner_enabled_ = prefs->GetBoolean(prefs::kPrefPiPSnapToCornerEnabled);

  // PiP window opacity.
  pip_opacity_ = prefs->GetDouble(prefs::kPrefPiPOpacity);
  if (pip_opacity_ < kMinPiPOpacity) {
    pip_opacity_ = kMinPiPOpacity;
  }
  if (pip_opacity_ > 1.0) {
    pip_opacity_ = 1.0;
  }

  // Auto-PiP on tab switch.
  auto_pip_on_tab_switch_ = prefs->GetBoolean(prefs::kPrefPiPAutoPipOnTabSwitch);

  // Max PiP windows.
  max_pip_windows_ = prefs->GetInteger(prefs::kPrefPiPMaxWindows);
  if (max_pip_windows_ < 0) {
    max_pip_windows_ = 0;
  }
}

void AstraPipService::SaveToPrefs() {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }

  PrefService* prefs = profile_->GetPrefs();

  prefs->SetString(prefs::kPrefPiPDefaultSize,
                   PresetToString(default_size_preset_));
  prefs->SetBoolean(prefs::kPrefPiPAlwaysOnTop, always_on_top_);
  prefs->SetString(prefs::kPrefPiPSnapPosition,
                   SnapPositionToString(snap_position_));
  prefs->SetBoolean(prefs::kPrefPiPSnapToCornerEnabled,
                    snap_to_corner_enabled_);
  prefs->SetDouble(prefs::kPrefPiPOpacity, pip_opacity_);
  prefs->SetBoolean(prefs::kPrefPiPAutoPipOnTabSwitch,
                    auto_pip_on_tab_switch_);
  prefs->SetInteger(prefs::kPrefPiPMaxWindows, max_pip_windows_);
}

void AstraPipService::NotifyPreferencesChanged() {
  for (auto& observer : observers_) {
    observer.OnPiPPreferencesChanged();
  }
}

}  // namespace astra
