#include "astra/ui/views/split_view/astra_split_view_controller.h"

#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include "astra/browser/astra_prefs.h"
#include "astra/browser/astra_tab_features.h"
#include "astra/ui/views/split_view/astra_split_view.h"
#include "base/strings/string_piece.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "ui/events/event.h"
#include "ui/views/view.h"

namespace astra {

namespace {

// Helper to convert a pointer to a string identifier.
// This is only valid within a single browser process session.
// TODO(astra): Replace with a proper stable tab identifier, such as
// base::Token or WebContents::GetController().GetWindowId().
template <typename T>
std::string PointerToId(T* ptr) {
  std::ostringstream oss;
  oss << static_cast<const void*>(ptr);
  return oss.str();
}

// Helper to parse an orientation string from prefs.
SplitViewOrientation OrientationFromString(const std::string& value) {
  if (value == "vertical") {
    return SplitViewOrientation::kVertical;
  }
  return SplitViewOrientation::kHorizontal;
}

// Helper to convert an orientation to a string for prefs.
std::string OrientationToString(SplitViewOrientation orientation) {
  switch (orientation) {
    case SplitViewOrientation::kHorizontal:
      return "horizontal";
    case SplitViewOrientation::kVertical:
      return "vertical";
  }
  return "horizontal";
}

}  // namespace

// =========================================================================
// AstraSplitViewController
// =========================================================================

AstraSplitViewController::AstraSplitViewController(BrowserView* browser_view)
    : browser_view_(browser_view) {
  // Load initial settings from prefs.
  LoadSettingsFromPrefs(&cached_settings_);

  // Set up pref change observation.
  Profile* prof = profile();
  if (prof) {
    pref_change_registrar_ = std::make_unique<PrefChangeRegistrar>();
    pref_change_registrar_->Init(prof->GetPrefs());

    // Register for all split view pref changes.
    pref_change_registrar_->Add(
        prefs::kPrefSplitViewDefaultOrientation,
        base::BindRepeating(
            &AstraSplitViewController::OnSplitViewPrefChanged,
            base::Unretained(this)));
    pref_change_registrar_->Add(
        prefs::kPrefSplitViewDefaultRatio,
        base::BindRepeating(
            &AstraSplitViewController::OnSplitViewPrefChanged,
            base::Unretained(this)));
    pref_change_registrar_->Add(
        prefs::kPrefSplitViewEnabled,
        base::BindRepeating(
            &AstraSplitViewController::OnSplitViewPrefChanged,
            base::Unretained(this)));
    pref_change_registrar_->Add(
        prefs::kPrefSplitViewSnapToPresets,
        base::BindRepeating(
            &AstraSplitViewController::OnSplitViewPrefChanged,
            base::Unretained(this)));
    pref_change_registrar_->Add(
        prefs::kPrefSplitViewDividerVisible,
        base::BindRepeating(
            &AstraSplitViewController::OnSplitViewPrefChanged,
            base::Unretained(this)));
    pref_change_registrar_->Add(
        prefs::kPrefSplitViewRememberRatio,
        base::BindRepeating(
            &AstraSplitViewController::OnSplitViewPrefChanged,
            base::Unretained(this)));
    pref_change_registrar_->Add(
        prefs::kPrefSplitViewMinimapEnabled,
        base::BindRepeating(
            &AstraSplitViewController::OnSplitViewPrefChanged,
            base::Unretained(this)));
  }
}

AstraSplitViewController::~AstraSplitViewController() {
  // Notify Astra observers of shutdown.
  NotifyAstraShutdown();

  // Notify legacy observers before tearing down.
  for (Observer& observer : observers_) {
    observer.OnSplitViewControllerDestroyed();
  }

  // Ensure clean teardown.
  if (is_active_) {
    HideSplitView();
  }
}

Profile* AstraSplitViewController::profile() {
  if (!browser_view_) {
    return nullptr;
  }
  return browser_view_->browser() ? browser_view_->browser()->profile() : nullptr;
}

const Profile* AstraSplitViewController::profile() const {
  if (!browser_view_) {
    return nullptr;
  }
  return browser_view_->browser() ? browser_view_->browser()->profile() : nullptr;
}

// =========================================================================
// Observer management
// =========================================================================

void AstraSplitViewController::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void AstraSplitViewController::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

// =========================================================================
// Split view lifecycle
// =========================================================================

void AstraSplitViewController::ShowSplitView(
    content::WebContents* primary,
    content::WebContents* secondary,
    SplitViewOrientation orientation) {
  if (!browser_view_ || !primary || !secondary || primary == secondary) {
    return;
  }

  // Check if split view is enabled in prefs.
  Profile* prof = profile();
  if (prof) {
    PrefService* prefs = prof->GetPrefs();
    if (!prefs->GetBoolean(prefs::kPrefSplitViewEnabled)) {
      return;
    }
  }

  // If already active, update the configuration.
  if (is_active_) {
    // If the same pair, just update orientation/ratio.
    if (primary == primary_web_contents_ && secondary == secondary_web_contents_) {
      if (orientation_ != orientation) {
        SetOrientation(orientation);
      }
      return;
    }
    // Different pair — hide first, then show with new pair.
    HideSplitView();
  }

  primary_web_contents_ = primary;
  secondary_web_contents_ = secondary;
  orientation_ = orientation;

  // Read the ratio from the primary tab's metadata, if available.
  AstraTabFeatures* primary_features =
      AstraTabFeatures::FromWebContents(primary);
  if (primary_features && cached_settings_.remember_ratio) {
    split_ratio_ = primary_features->split_view_ratio();
    // Clamp in case metadata has an out-of-range value.
    if (split_ratio_ < AstraSplitView::kMinPaneRatio) {
      split_ratio_ = AstraSplitView::kMinPaneRatio;
    } else if (split_ratio_ > 1.0f - AstraSplitView::kMinPaneRatio) {
      split_ratio_ = 1.0f - AstraSplitView::kMinPaneRatio;
    }
  } else {
    // Use default ratio from settings.
    split_ratio_ = cached_settings_.default_ratio;
  }

  // Create the split view and install it in the content area.
  split_view_ = new AstraSplitView();
  split_view_->SetOrientation(orientation_);
  split_view_->SetRatio(split_ratio_);

  // Apply settings to the view.
  ApplySettingsToView(cached_settings_);

  // Observe the split view for ratio changes from user interaction.
  split_view_->AddObserver(this);

  // Get the view representations of both WebContents.
  // TODO(astra): The correct way to get the views::View* for a WebContents
  // in Chrome's BrowserView is to use the contents container's child view.
  // For now, we use GetWebContentsView() as a placeholder.
  views::View* primary_view = GetWebContentsView(primary);
  views::View* secondary_view = GetWebContentsView(secondary);

  if (primary_view) {
    split_view_->SetPrimaryView(primary_view);
  }
  if (secondary_view) {
    split_view_->SetSecondaryView(secondary_view);
  }

  // Install the split view into the content area.
  InstallSplitViewInContentsArea();

  is_active_ = true;
  is_maximized_ = false;

  // Write metadata to both tabs.
  WriteMetadataToTabs();

  // Notify observers.
  NotifySplitViewShown();
}

void AstraSplitViewController::ShowSplitViewWithDefaults(
    content::WebContents* primary,
    content::WebContents* secondary) {
  ShowSplitView(primary, secondary, cached_settings_.default_orientation);
}

void AstraSplitViewController::HideSplitView() {
  if (!is_active_) {
    return;
  }

  // Stop observing the split view before it gets destroyed.
  if (split_view_) {
    split_view_->RemoveObserver(this);
  }

  // Clear metadata from both tabs before tearing down the UI.
  ClearMetadataFromTabs();

  // Remove the split view from the content area and restore normal layout.
  UninstallSplitViewFromContentsArea();

  // The split_view_ is owned by the parent view; it is deleted when removed.
  split_view_ = nullptr;

  primary_web_contents_ = nullptr;
  secondary_web_contents_ = nullptr;
  is_active_ = false;
  is_maximized_ = false;

  // Notify observers.
  NotifySplitViewHidden();
}

void AstraSplitViewController::ToggleSplitView(content::WebContents* primary,
                                               SplitViewOrientation orientation) {
  if (is_active_) {
    HideSplitView();
    return;
  }

  // Find a secondary tab.
  content::WebContents* secondary = FindSecondaryTab(primary);
  if (!secondary) {
    // Can't split with only one tab.
    return;
  }

  ShowSplitView(primary, secondary, orientation);
}

// =========================================================================
// Split view configuration
// =========================================================================

void AstraSplitViewController::SetSplitRatio(float ratio) {
  if (!is_active_ || !split_view_) {
    return;
  }

  float clamped_ratio = AstraSplitView::ClampRatio(ratio);
  if (split_ratio_ == clamped_ratio) {
    return;
  }

  split_ratio_ = clamped_ratio;
  split_view_->SetRatio(split_ratio_);

  // Persist to metadata.
  if (primary_web_contents_) {
    AstraTabFeatures* features =
        AstraTabFeatures::GetOrCreateForWebContents(primary_web_contents_);
    features->set_split_view_ratio(split_ratio_);
  }
  if (secondary_web_contents_) {
    AstraTabFeatures* features =
        AstraTabFeatures::GetOrCreateForWebContents(secondary_web_contents_);
    features->set_split_view_ratio(split_ratio_);
  }

  NotifySplitRatioChanged();
}

void AstraSplitViewController::SetPresetRatio(SplitViewPreset preset) {
  SetSplitRatio(SplitViewPresetToRatio(preset));
}

void AstraSplitViewController::SetOrientation(SplitViewOrientation orientation) {
  if (!is_active_ || !split_view_) {
    return;
  }

  if (orientation_ == orientation) {
    return;
  }

  orientation_ = orientation;
  split_view_->SetOrientation(orientation);

  // Persist to metadata.
  if (primary_web_contents_) {
    AstraTabFeatures* features =
        AstraTabFeatures::GetOrCreateForWebContents(primary_web_contents_);
    features->set_split_view_orientation(orientation);
  }
  if (secondary_web_contents_) {
    AstraTabFeatures* features =
        AstraTabFeatures::GetOrCreateForWebContents(secondary_web_contents_);
    features->set_split_view_orientation(orientation);
  }

  NotifySplitOrientationChanged();
}

void AstraSplitViewController::ToggleOrientation() {
  if (!is_active_) {
    return;
  }

  if (orientation_ == SplitViewOrientation::kHorizontal) {
    SetOrientation(SplitViewOrientation::kVertical);
  } else {
    SetOrientation(SplitViewOrientation::kHorizontal);
  }
}

void AstraSplitViewController::SwapViews() {
  if (!is_active_ || !split_view_) {
    return;
  }

  // Swap the WebContents pointers.
  std::swap(primary_web_contents_, secondary_web_contents_);

  // Tell the split view to swap its child views.
  split_view_->SwapViews();

  // Update metadata on both tabs (swap partner_ids and update primary/secondary).
  // The ratio stays the same value, but its "meaning" swaps (left vs right).
  WriteMetadataToTabs();

  NotifySplitViewsSwapped();
}

void AstraSplitViewController::ResizePrimaryPane(int delta) {
  if (!is_active_ || !split_view_) {
    return;
  }

  // Compute new ratio from current position + delta.
  gfx::Rect bounds = split_view_->GetLocalBounds();
  int total_size =
      (orientation_ == SplitViewOrientation::kHorizontal) ? bounds.width()
                                                          : bounds.height();
  if (total_size <= 0) {
    return;
  }

  int current_pos = static_cast<int>(total_size * split_ratio_);
  int new_pos = current_pos + delta;
  float new_ratio = static_cast<float>(new_pos) / total_size;

  SetSplitRatio(new_ratio);
}

void AstraSplitViewController::MaximizePrimaryPane() {
  if (!is_active_ || !split_view_) {
    return;
  }
  split_view_->MaximizePane(/*primary=*/true);
}

void AstraSplitViewController::MaximizeSecondaryPane() {
  if (!is_active_ || !split_view_) {
    return;
  }
  split_view_->MaximizePane(/*primary=*/false);
}

void AstraSplitViewController::Unmaximize() {
  if (!is_active_ || !split_view_) {
    return;
  }
  split_view_->Unmaximize();
}

bool AstraSplitViewController::IsMaximized() const {
  if (split_view_) {
    return split_view_->IsMaximized();
  }
  return is_maximized_;
}

void AstraSplitViewController::ReplacePrimaryTab(content::WebContents* new_contents) {
  if (!is_active_ || !split_view_ || !new_contents) {
    return;
  }
  if (new_contents == primary_web_contents_) {
    return;
  }

  // Clear metadata from the old primary tab.
  if (primary_web_contents_) {
    AstraTabFeatures* features =
        AstraTabFeatures::FromWebContents(primary_web_contents_);
    if (features) {
      features->set_is_in_split_view(false);
      features->set_split_view_partner_id(std::string());
    }
  }

  primary_web_contents_ = new_contents;

  // Update the view.
  views::View* new_view = GetWebContentsView(new_contents);
  if (new_view) {
    split_view_->ReplacePrimaryView(new_view);
  } else {
    split_view_->ReplacePrimaryView(nullptr);
  }

  // Write updated metadata.
  WriteMetadataToTabs();

  NotifySplitTabReplaced(/*is_primary=*/true, new_contents);
}

void AstraSplitViewController::ReplaceSecondaryTab(content::WebContents* new_contents) {
  if (!is_active_ || !split_view_ || !new_contents) {
    return;
  }
  if (new_contents == secondary_web_contents_) {
    return;
  }

  // Clear metadata from the old secondary tab.
  if (secondary_web_contents_) {
    AstraTabFeatures* features =
        AstraTabFeatures::FromWebContents(secondary_web_contents_);
    if (features) {
      features->set_is_in_split_view(false);
      features->set_split_view_partner_id(std::string());
    }
  }

  secondary_web_contents_ = new_contents;

  // Update the view.
  views::View* new_view = GetWebContentsView(new_contents);
  if (new_view) {
    split_view_->ReplaceSecondaryView(new_view);
  } else {
    split_view_->ReplaceSecondaryView(nullptr);
  }

  // Write updated metadata.
  WriteMetadataToTabs();

  NotifySplitTabReplaced(/*is_primary=*/false, new_contents);
}

void AstraSplitViewController::ApplyNamedPreset(const std::string& preset_name) {
  // TODO(astra): Implement named presets beyond ratio presets.
  // Named presets could be user-defined configurations that include
  // orientation, ratio, and even which tabs go in which pane.
  // For now, we handle a few built-in preset names.

  if (preset_name == "fifty_fifty" || preset_name == "50/50") {
    SetPresetRatio(SplitViewPreset::kFiftyFifty);
  } else if (preset_name == "seventy_thirty" || preset_name == "70/30") {
    SetPresetRatio(SplitViewPreset::kSeventyThirty);
  } else if (preset_name == "thirty_seventy" || preset_name == "30/70") {
    SetPresetRatio(SplitViewPreset::kThirtySeventy);
  } else if (preset_name == "sixty_forty" || preset_name == "60/40") {
    SetPresetRatio(SplitViewPreset::kSixtyForty);
  } else if (preset_name == "forty_sixty" || preset_name == "40/60") {
    SetPresetRatio(SplitViewPreset::kFortySixty);
  } else if (preset_name == "horizontal") {
    SetOrientation(SplitViewOrientation::kHorizontal);
  } else if (preset_name == "vertical") {
    SetOrientation(SplitViewOrientation::kVertical);
  }
}

bool AstraSplitViewController::IsSplitViewActive() const {
  return is_active_;
}

// =========================================================================
// Settings
// =========================================================================

AstraSplitViewSettings AstraSplitViewController::GetSettings() const {
  // Refresh from prefs to ensure we have the latest values.
  LoadSettingsFromPrefs(&cached_settings_);
  return cached_settings_;
}

void AstraSplitViewController::UpdateSettings(
    const AstraSplitViewSettings& settings) {
  cached_settings_ = settings;
  SaveSettingsToPrefs(settings);

  if (is_active_ && split_view_) {
    ApplySettingsToView(settings);
  }

  NotifySplitViewSettingsChanged();
}

void AstraSplitViewController::LoadSettingsFromPrefs(
    AstraSplitViewSettings* settings) const {
  if (!settings) {
    return;
  }

  Profile* prof = profile();
  if (!prof) {
    return;
  }

  PrefService* prefs = prof->GetPrefs();
  if (!prefs) {
    return;
  }

  settings->default_orientation = OrientationFromString(
      prefs->GetString(prefs::kPrefSplitViewDefaultOrientation));
  settings->default_ratio =
      static_cast<float>(prefs->GetDouble(prefs::kPrefSplitViewDefaultRatio));
  settings->snap_to_presets =
      prefs->GetBoolean(prefs::kPrefSplitViewSnapToPresets);
  settings->divider_visible =
      prefs->GetBoolean(prefs::kPrefSplitViewDividerVisible);
  settings->remember_ratio =
      prefs->GetBoolean(prefs::kPrefSplitViewRememberRatio);
  settings->minimap_enabled =
      prefs->GetBoolean(prefs::kPrefSplitViewMinimapEnabled);
  settings->keyboard_navigation_enabled = true;  // Default on.
  settings->show_menu_button = false;  // Not implemented yet.
}

void AstraSplitViewController::SaveSettingsToPrefs(
    const AstraSplitViewSettings& settings) {
  Profile* prof = profile();
  if (!prof) {
    return;
  }

  PrefService* prefs = prof->GetPrefs();
  if (!prefs) {
    return;
  }

  prefs->SetString(prefs::kPrefSplitViewDefaultOrientation,
                   OrientationToString(settings.default_orientation));
  prefs->SetDouble(prefs::kPrefSplitViewDefaultRatio,
                   static_cast<double>(settings.default_ratio));
  prefs->SetBoolean(prefs::kPrefSplitViewSnapToPresets,
                    settings.snap_to_presets);
  prefs->SetBoolean(prefs::kPrefSplitViewDividerVisible,
                    settings.divider_visible);
  prefs->SetBoolean(prefs::kPrefSplitViewRememberRatio,
                    settings.remember_ratio);
  prefs->SetBoolean(prefs::kPrefSplitViewMinimapEnabled,
                    settings.minimap_enabled);
}

void AstraSplitViewController::ApplySettingsToView(
    const AstraSplitViewSettings& settings) {
  if (!split_view_) {
    return;
  }
  split_view_->ApplySettings(settings);
}

void AstraSplitViewController::OnSplitViewPrefChanged(
    const std::string& pref_name) {
  // Reload settings from prefs.
  LoadSettingsFromPrefs(&cached_settings_);

  // Apply to the view if active.
  if (is_active_ && split_view_) {
    ApplySettingsToView(cached_settings_);
  }

  // Notify observers.
  NotifySplitViewSettingsChanged();
}

// =========================================================================
// AstraSplitView::Observer overrides
// =========================================================================

void AstraSplitViewController::OnSplitRatioChanged(float ratio) {
  // The user finished dragging the divider.  Persist the new ratio.
  if (ratio == split_ratio_) {
    return;
  }

  split_ratio_ = ratio;

  // Write metadata to both tabs.
  if (primary_web_contents_) {
    AstraTabFeatures* features =
        AstraTabFeatures::GetOrCreateForWebContents(primary_web_contents_);
    features->set_split_view_ratio(ratio);
  }
  if (secondary_web_contents_) {
    AstraTabFeatures* features =
        AstraTabFeatures::GetOrCreateForWebContents(secondary_web_contents_);
    features->set_split_view_ratio(ratio);
  }

  NotifySplitRatioChanged();
}

void AstraSplitViewController::OnSplitRatioChanging(float ratio) {
  // Continuous update during drag — don't persist to metadata yet,
  // but update our cached state for any live-updating UI.
  split_ratio_ = ratio;
  NotifySplitRatioChanging();
}

void AstraSplitViewController::OnSplitOrientationChanged(
    SplitViewOrientation orientation) {
  // Orientation changed from the view side (e.g. via menu action on divider).
  orientation_ = orientation;

  // Persist to metadata.
  if (primary_web_contents_) {
    AstraTabFeatures* features =
        AstraTabFeatures::GetOrCreateForWebContents(primary_web_contents_);
    features->set_split_view_orientation(orientation);
  }
  if (secondary_web_contents_) {
    AstraTabFeatures* features =
        AstraTabFeatures::GetOrCreateForWebContents(secondary_web_contents_);
    features->set_split_view_orientation(orientation);
  }

  NotifySplitOrientationChanged();
}

void AstraSplitViewController::OnSplitViewsSwapped() {
  // Views were swapped from the view side.
  // Swap our WebContents pointers to stay in sync.
  std::swap(primary_web_contents_, secondary_web_contents_);
  WriteMetadataToTabs();
  NotifySplitViewsSwapped();
}

void AstraSplitViewController::OnSplitViewReplaced(bool is_primary) {
  // A view was replaced from the view side.  We don't have the new
  // WebContents here, so just re-sync metadata.
  // TODO(astra): When the view replacement is driven from the controller,
  //   we already know the new WebContents.  This path is for when the
  //   view is replaced from somewhere else in the Views hierarchy.
  WriteMetadataToTabs();
}

void AstraSplitViewController::OnSplitViewSettingsChanged(
    const AstraSplitViewSettings& settings) {
  // Settings changed from the view side (e.g. divider menu action).
  cached_settings_ = settings;
  SaveSettingsToPrefs(settings);
  NotifySplitViewSettingsChanged();
}

void AstraSplitViewController::OnSplitViewMaximized(bool primary_maximized) {
  is_maximized_ = true;
  primary_maximized_ = primary_maximized;
  NotifySplitViewMaximized(primary_maximized);
}

void AstraSplitViewController::OnSplitViewUnmaximized() {
  is_maximized_ = false;
  NotifySplitViewUnmaximized();
}

void AstraSplitViewController::OnSplitViewDestroyed() {
  // The split view was destroyed externally (e.g., its parent was removed).
  // Clear our pointer and reset state.
  if (split_view_) {
    split_view_->RemoveObserver(this);
  }
  split_view_ = nullptr;

  if (is_active_) {
    is_active_ = false;
    is_maximized_ = false;
    ClearMetadataFromTabs();
    NotifySplitViewHidden();
  }
}

// =========================================================================
// Private helpers
// =========================================================================

views::View* AstraSplitViewController::GetWebContentsView(
    content::WebContents* web_contents) {
  // TODO(astra): This is a placeholder for the Chromium API that returns
  // the views::View* representation of a WebContents.
  //
  // The correct approach depends on the integration point:
  //
  // Option A (views::WebView):
  //   If the content area uses views::WebView, we can get the view via
  //   web_view->holder() or web_view->GetNativeView().
  //
  // Option B (WebContentsView directly):
  //   content::WebContentsView* wcv = web_contents->GetView();
  //   On desktop with views, this returns a views::WebContentsView which
  //   has a GetNativeView() or GetView() method.
  //
  // Option C (BrowserView contents container):
  //   browser_view_->contents_container() hosts the active WebContents view.
  //   For non-active tabs, the view is hidden or detached.
  //
  // The split view needs both WebContents views to be visible simultaneously.
  // This means we need to ensure both WebContents have their native views
  // attached to the views hierarchy, which is not the normal Chrome behavior
  // (only the active tab's WebContents view is attached).
  //
  // Chromium owner: content::WebContentsView (content/public/browser)
  // and chrome/browser/ui/views/frame/browser_view.cc contents_container().
  //
  // Patch point: May require a small patch to BrowserView to expose the
  // ability to attach a second WebContents view for split view mode.
  //
  // For now, return nullptr as a placeholder.  The split view will still
  // layout correctly with one or both children missing (degraded display).

  // Hint for developers: explore
  //   content::WebContentsView::GetNativeView()
  //   views::WebView::web_contents()
  //   browser_view_->contents_container()
  return nullptr;
}

void AstraSplitViewController::InstallSplitViewInContentsArea() {
  // TODO(astra): Insert the split view into the correct position in
  // BrowserView's layout hierarchy.
  //
  // The split view should replace or overlay the normal contents container.
  // In Chrome's BrowserView, the content area is managed by
  // BrowserViewLayoutManager (chrome/browser/ui/views/frame/browser_view_layout.h).
  //
  // Proper approach:
  //   1. Get the contents container from browser_view_.
  //   2. Hide or detach the normal single-tab contents view.
  //   3. Insert the AstraSplitView into the contents container or at the
  //      same level in the layout.
  //   4. Ensure the layout manager allocates the correct space.
  //
  // For the skeleton implementation, we add the split view as a child of
  // the BrowserView as a placeholder.  The real integration requires a
  // tiny Chromium patch to BrowserView that delegates content area layout
  // to Astra when split view is active.
  //
  // Chromium owner: BrowserView (chrome/browser/ui/views/frame/browser_view.h)
  // Patch file: chrome/browser/ui/views/frame/browser_view.cc.patch
  //
  // Patch point details:
  //   - File: chrome/browser/ui/views/frame/browser_view.cc
  //   - Hook: In BrowserView::InitViews() or BrowserViewLayout::Layout(),
  //     add a check for split view mode and delegate layout to Astra.
  //   - Alternative: Add an "overlay content view" to BrowserView that
  //     Astra can use to host the split view without modifying the main
  //     contents container layout.

  if (!browser_view_ || !split_view_) {
    return;
  }

  // For the skeleton: add the split view to the browser view.
  // In the real implementation, this goes into the contents container area.
  browser_view_->AddChildView(split_view_);

  // The split view should fill the content area.  For now, just set it to
  // fill the entire browser view for demonstration purposes.
  // TODO(astra): Position the split view correctly within the content area,
  // below the toolbar and tab strip.  This requires knowledge of
  // BrowserView's layout — the correct bounds depend on the frame view
  // and toolbar sizes.
}

void AstraSplitViewController::UninstallSplitViewFromContentsArea() {
  if (!browser_view_ || !split_view_) {
    return;
  }

  // Remove the split view from the hierarchy.
  // TODO(astra): Use the proper removal method matching the insertion point.
  // The parent owns the child, so RemoveChildViewT deletes it.
  if (browser_view_->GetIndexOf(split_view_).has_value()) {
    browser_view_->RemoveChildViewT(split_view_);
  }

  // TODO(astra): Restore the normal single-tab contents view.
  // When split view is deactivated, the active tab's WebContents view
  // should be re-attached to the normal contents container.
  //
  // Chromium subsystem: BrowserView + TabStripModel active tab handling.
  // When the active tab changes, Chrome normally swaps the WebContents
  // view in the contents container.  After split view, we need to ensure
  // the active tab's view is back in the standard position.
}

void AstraSplitViewController::WriteMetadataToTabs() {
  if (!primary_web_contents_ || !secondary_web_contents_) {
    return;
  }

  // Primary tab metadata.
  AstraTabFeatures* primary_features =
      AstraTabFeatures::GetOrCreateForWebContents(primary_web_contents_);
  primary_features->set_is_in_split_view(true);
  primary_features->set_split_view_partner_id(GetPartnerId(secondary_web_contents_));
  primary_features->set_split_view_ratio(split_ratio_);
  primary_features->set_split_view_orientation(orientation_);

  // Secondary tab metadata.
  AstraTabFeatures* secondary_features =
      AstraTabFeatures::GetOrCreateForWebContents(secondary_web_contents_);
  secondary_features->set_is_in_split_view(true);
  secondary_features->set_split_view_partner_id(GetPartnerId(primary_web_contents_));
  secondary_features->set_split_view_ratio(split_ratio_);
  secondary_features->set_split_view_orientation(orientation_);
}

void AstraSplitViewController::ClearMetadataFromTabs() {
  // Clear primary tab metadata.
  if (primary_web_contents_) {
    AstraTabFeatures* features =
        AstraTabFeatures::FromWebContents(primary_web_contents_);
    if (features) {
      features->set_is_in_split_view(false);
      features->set_split_view_partner_id(std::string());
    }
  }

  // Clear secondary tab metadata.
  if (secondary_web_contents_) {
    AstraTabFeatures* features =
        AstraTabFeatures::FromWebContents(secondary_web_contents_);
    if (features) {
      features->set_is_in_split_view(false);
      features->set_split_view_partner_id(std::string());
    }
  }
}

std::string AstraSplitViewController::GetPartnerId(
    content::WebContents* web_contents) {
  // TODO(astra): Use a proper stable identifier.  The pointer address is
  // only valid for the current process session and will not survive session
  // restore or tab discarding.
  //
  // Better options:
  //   - base::Token assigned by AstraTabFeatures at creation time.
  //   - WebContents::GetController().GetWindowId() — but this is per
  //     NavigationController, not per WebContents.
  //   - TabStripModel index — but indices change when tabs move.
  //
  // For now, use the pointer address as a session-scoped identifier.
  return PointerToId(web_contents);
}

content::WebContents* AstraSplitViewController::FindSecondaryTab(
    content::WebContents* primary) {
  // TODO(astra): Use TabStripModel to find the next tab after |primary|.
  //
  // Chromium owner: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)
  //
  // Implementation plan:
  //   1. Get TabStripModel from browser_view_->browser()->tab_strip_model().
  //   2. Find the index of |primary| in the tab strip.
  //   3. If there's a tab after primary, use that as secondary.
  //   4. If primary is the last tab, use the previous tab.
  //   5. If there's only one tab, return nullptr (can't split).
  //
  // For the skeleton, return nullptr — split view must be explicitly
  // shown with both tabs specified.

  // Hint for integration:
  //   TabStripModel* tab_strip =
  //       browser_view_->browser()->tab_strip_model();
  //   int primary_index = tab_strip->GetIndexOfWebContents(primary);
  //   int secondary_index = primary_index + 1;
  //   if (secondary_index >= tab_strip->count())
  //     secondary_index = primary_index - 1;
  //   if (secondary_index < 0) return nullptr;
  //   return tab_strip->GetWebContentsAt(secondary_index);

  return nullptr;
}

// =========================================================================
// Observer notification helpers
// =========================================================================

void AstraSplitViewController::NotifySplitViewShown() {
  for (Observer& observer : observers_) {
    observer.OnSplitViewShown(primary_web_contents_, secondary_web_contents_);
  }
}

void AstraSplitViewController::NotifySplitViewHidden() {
  for (Observer& observer : observers_) {
    observer.OnSplitViewHidden();
  }
}

void AstraSplitViewController::NotifySplitRatioChanged() {
  for (Observer& observer : observers_) {
    observer.OnSplitRatioChanged(split_ratio_);
  }
}

void AstraSplitViewController::NotifySplitRatioChanging() {
  for (Observer& observer : observers_) {
    observer.OnSplitRatioChanging(split_ratio_);
  }
}

void AstraSplitViewController::NotifySplitOrientationChanged() {
  for (Observer& observer : observers_) {
    observer.OnSplitOrientationChanged(orientation_);
  }
}

void AstraSplitViewController::NotifySplitViewsSwapped() {
  for (Observer& observer : observers_) {
    observer.OnSplitViewsSwapped();
  }
}

void AstraSplitViewController::NotifySplitTabReplaced(
    bool is_primary,
    content::WebContents* new_contents) {
  for (Observer& observer : observers_) {
    observer.OnSplitTabReplaced(is_primary, new_contents);
  }
}

void AstraSplitViewController::NotifySplitViewSettingsChanged() {
  for (Observer& observer : observers_) {
    observer.OnSplitViewSettingsChanged(cached_settings_);
  }
}

void AstraSplitViewController::NotifySplitViewMaximized(
    bool primary_maximized) {
  for (Observer& observer : observers_) {
    observer.OnSplitViewMaximized(primary_maximized);
  }
}

void AstraSplitViewController::NotifySplitViewUnmaximized() {
  for (Observer& observer : observers_) {
    observer.OnSplitViewUnmaximized();
  }
}

// =========================================================================
// Extended API implementations (Astra naming convention)
// =========================================================================

void AstraSplitViewController::ToggleSplitView() {
  if (is_active_) {
    DeactivateSplitView();
  } else {
    ActivateSplitView();
  }
}

void AstraSplitViewController::ActivateSplitView() {
  if (is_active_) {
    return;
  }

  // TODO(astra): Find primary and secondary tabs from TabStripModel.
  //   For now, we set the state and notify observers.
  //   Chromium owner: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)

  // Set default tab indices.
  primary_tab_index_ = 0;
  secondary_tab_index_ = 1;

  // Load default ratio and orientation from settings.
  if (cached_settings_.remember_split_state) {
    split_ratio_ = static_cast<float>(cached_settings_.default_ratio);
    orientation_ = ToLegacyOrientation(cached_settings_.default_orientation);
  }

  // Create the split view if needed.
  if (!split_view_) {
    split_view_ = new AstraSplitView();
    split_view_->SetOrientation(orientation_);
    split_view_->SetRatio(split_ratio_);
    split_view_->AddObserver(this);
    split_view_->SetFocusedPane(AstraSplitPane::kPrimary);

    // Apply settings to the view.
    split_view_->SetDividerWidth(divider_width_);
    split_view_->SetShowHandle(cached_settings_.show_divider_handle);
    split_view_->ShowPaneLabels(cached_settings_.show_pane_labels);
  }

  // Install into contents area.
  InstallSplitViewInContentsArea();

  is_active_ = true;
  focused_pane_ = AstraSplitPane::kPrimary;
  current_preset_ = cached_settings_.default_preset;

  // Notify observers.
  NotifyAstraSplitViewActivated();
  NotifySplitViewShown();
}

void AstraSplitViewController::DeactivateSplitView() {
  if (!is_active_) {
    return;
  }

  // Notify before tearing down.
  NotifyAstraSplitViewDeactivated();
  NotifySplitViewHidden();

  // Remove from contents area.
  UninstallSplitViewFromContentsArea();

  // Stop observing.
  if (split_view_) {
    split_view_->RemoveObserver(this);
  }

  split_view_ = nullptr;
  is_active_ = false;
  primary_tab_index_ = -1;
  secondary_tab_index_ = -1;
}

bool AstraSplitViewController::IsActive() const {
  return is_active_;
}

double AstraSplitViewController::GetSplitRatio() const {
  return static_cast<double>(split_ratio_);
}

void AstraSplitViewController::SetSplitRatio(double ratio) {
  if (!is_active_ || !split_view_) {
    return;
  }

  // Clamp to valid range.
  double clamped = ratio;
  if (clamped < 0.1) clamped = 0.1;
  if (clamped > 0.9) clamped = 0.9;

  if (static_cast<double>(split_ratio_) == clamped) {
    return;
  }

  split_ratio_ = static_cast<float>(clamped);
  split_view_->SetRatio(clamped);

  // Reset the current preset since the ratio no longer matches one exactly.
  current_preset_ = AstraSplitPreset::kEqual;

  NotifyAstraSplitRatioChanged();
  NotifySplitRatioChanged();
}

void AstraSplitViewController::SetOrientation(AstraSplitOrientation orientation) {
  if (!is_active_ || !split_view_) {
    return;
  }

  SplitViewOrientation legacy_orient = ToLegacyOrientation(orientation);
  if (orientation_ == legacy_orient) {
    return;
  }

  orientation_ = legacy_orient;
  split_view_->SetOrientation(legacy_orient);

  NotifyAstraSplitOrientationChanged();
  NotifySplitOrientationChanged();
}

AstraSplitOrientation AstraSplitViewController::GetOrientation() const {
  return FromLegacyOrientation(orientation_);
}

void AstraSplitViewController::ToggleOrientation() {
  if (!is_active_) {
    return;
  }

  if (orientation_ == SplitViewOrientation::kHorizontal) {
    SetOrientation(AstraSplitOrientation::kVertical);
  } else {
    SetOrientation(AstraSplitOrientation::kHorizontal);
  }
}

void AstraSplitViewController::SetPrimaryTab(int tab_index) {
  if (!is_active_) {
    return;
  }
  if (primary_tab_index_ == tab_index) {
    return;
  }

  primary_tab_index_ = tab_index;

  // TODO(astra): Update the actual WebContents view in the split view.
  //   Requires TabStripModel integration to get the WebContents at |tab_index|.
  //   Chromium owner: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)

  NotifyAstraPrimaryPaneChanged();
}

void AstraSplitViewController::SetSecondaryTab(int tab_index) {
  if (!is_active_) {
    return;
  }
  if (secondary_tab_index_ == tab_index) {
    return;
  }

  secondary_tab_index_ = tab_index;

  // TODO(astra): Update the actual WebContents view in the split view.
  NotifyAstraSecondaryPaneChanged();
}

int AstraSplitViewController::GetPrimaryTabIndex() const {
  if (!is_active_) {
    return -1;
  }
  return primary_tab_index_;
}

int AstraSplitViewController::GetSecondaryTabIndex() const {
  if (!is_active_) {
    return -1;
  }
  return secondary_tab_index_;
}

void AstraSplitViewController::SwapPanes() {
  if (!is_active_ || !split_view_) {
    return;
  }

  std::swap(primary_tab_index_, secondary_tab_index_);
  split_view_->SwapViews();

  // Focus stays with the same logical pane (primary), but since we swapped,
  // the focused content moves to the other side.
  // We keep the focused_pane_ value the same since it refers to the pane role,
  // not the content.

  NotifyAstraPaneSwapped();
  NotifySplitViewsSwapped();
}

void AstraSplitViewController::ClosePrimaryPane() {
  if (!is_active_) {
    return;
  }

  // TODO(astra): Close the primary tab via TabStripModel.
  //   Chromium owner: TabStripModel::CloseWebContentsAt
  //   (chrome/browser/ui/tabs/tab_strip_model.h)
  //   For now, we just deactivate split view.

  primary_tab_index_ = -1;
  DeactivateSplitView();
}

void AstraSplitViewController::CloseSecondaryPane() {
  if (!is_active_) {
    return;
  }

  // TODO(astra): Close the secondary tab via TabStripModel.
  secondary_tab_index_ = -1;
  DeactivateSplitView();
}

void AstraSplitViewController::FocusPrimaryPane() {
  if (!is_active_ || !split_view_) {
    return;
  }
  if (focused_pane_ == AstraSplitPane::kPrimary) {
    return;
  }

  focused_pane_ = AstraSplitPane::kPrimary;
  split_view_->SetFocusedPane(AstraSplitPane::kPrimary);

  // TODO(astra): Focus the actual WebContents in the primary pane.
  //   Chromium owner: content::WebContents::Focus()

  NotifyAstraFocusedPaneChanged();
}

void AstraSplitViewController::FocusSecondaryPane() {
  if (!is_active_ || !split_view_) {
    return;
  }
  if (focused_pane_ == AstraSplitPane::kSecondary) {
    return;
  }

  focused_pane_ = AstraSplitPane::kSecondary;
  split_view_->SetFocusedPane(AstraSplitPane::kSecondary);

  // TODO(astra): Focus the actual WebContents in the secondary pane.

  NotifyAstraFocusedPaneChanged();
}

void AstraSplitViewController::ToggleFocus() {
  if (!is_active_) {
    return;
  }

  if (focused_pane_ == AstraSplitPane::kPrimary) {
    FocusSecondaryPane();
  } else {
    FocusPrimaryPane();
  }
}

AstraSplitPane AstraSplitViewController::GetFocusedPane() const {
  return focused_pane_;
}

void AstraSplitViewController::SetLayoutPreset(AstraSplitPreset preset) {
  if (!is_active_) {
    return;
  }

  current_preset_ = preset;
  double ratio = AstraSplitPresetToRatio(preset);
  SetSplitRatio(ratio);

  // Note: SetSplitRatio resets current_preset_, so we set it again.
  current_preset_ = preset;
}

AstraSplitPreset AstraSplitViewController::GetLayoutPreset() const {
  return current_preset_;
}

void AstraSplitViewController::ApplyPreset(AstraSplitPreset preset) {
  SetLayoutPreset(preset);
}

void AstraSplitViewController::SetResizeMode(AstraResizeMode mode) {
  if (resize_mode_ == mode) {
    return;
  }
  resize_mode_ = mode;

  // TODO(astra): Apply resize mode to the view and persist to prefs.
  //   The resize mode affects how the split view adjusts during window resize.
}

AstraResizeMode AstraSplitViewController::GetResizeMode() const {
  return resize_mode_;
}

void AstraSplitViewController::SetMinPaneSize(int size_px) {
  if (size_px < 0) {
    size_px = 0;
  }
  if (min_pane_size_ == size_px) {
    return;
  }
  min_pane_size_ = size_px;

  // TODO(astra): Apply to the split view and re-clamp the current ratio.
}

int AstraSplitViewController::GetMinPaneSize() const {
  return min_pane_size_;
}

void AstraSplitViewController::SetDividerWidth(int width_px) {
  if (width_px < 0) {
    width_px = 0;
  }
  if (divider_width_ == width_px) {
    return;
  }
  divider_width_ = width_px;

  if (is_active_ && split_view_) {
    split_view_->SetDividerWidth(width_px);
  }
}

int AstraSplitViewController::GetDividerWidth() const {
  return divider_width_;
}

// =========================================================================
// Extended observer management
// =========================================================================

void AstraSplitViewController::AddAstraObserver(
    AstraSplitViewObserver* observer) {
  astra_observers_.AddObserver(observer);
}

void AstraSplitViewController::RemoveAstraObserver(
    AstraSplitViewObserver* observer) {
  astra_observers_.RemoveObserver(observer);
}

// =========================================================================
// Extended observer notification helpers
// =========================================================================

void AstraSplitViewController::NotifyAstraSplitViewActivated() {
  for (AstraSplitViewObserver& observer : astra_observers_) {
    observer.OnSplitViewActivated(this);
  }
}

void AstraSplitViewController::NotifyAstraSplitViewDeactivated() {
  for (AstraSplitViewObserver& observer : astra_observers_) {
    observer.OnSplitViewDeactivated(this);
  }
}

void AstraSplitViewController::NotifyAstraSplitRatioChanged() {
  for (AstraSplitViewObserver& observer : astra_observers_) {
    observer.OnSplitRatioChanged(this, static_cast<double>(split_ratio_));
  }
}

void AstraSplitViewController::NotifyAstraSplitOrientationChanged() {
  for (AstraSplitViewObserver& observer : astra_observers_) {
    observer.OnSplitOrientationChanged(this, GetOrientation());
  }
}

void AstraSplitViewController::NotifyAstraPrimaryPaneChanged() {
  for (AstraSplitViewObserver& observer : astra_observers_) {
    observer.OnPrimaryPaneChanged(this, primary_tab_index_);
  }
}

void AstraSplitViewController::NotifyAstraSecondaryPaneChanged() {
  for (AstraSplitViewObserver& observer : astra_observers_) {
    observer.OnSecondaryPaneChanged(this, secondary_tab_index_);
  }
}

void AstraSplitViewController::NotifyAstraPaneSwapped() {
  for (AstraSplitViewObserver& observer : astra_observers_) {
    observer.OnPaneSwapped(this);
  }
}

void AstraSplitViewController::NotifyAstraFocusedPaneChanged() {
  for (AstraSplitViewObserver& observer : astra_observers_) {
    observer.OnFocusedPaneChanged(this, focused_pane_);
  }
}

void AstraSplitViewController::NotifyAstraShutdown() {
  for (AstraSplitViewObserver& observer : astra_observers_) {
    observer.OnSplitViewControllerShutdown(this);
  }
}

// =========================================================================
// Layout mode management
// =========================================================================

AstraSplitLayoutMode AstraSplitViewController::GetLayoutMode() const {
  return model_.layout_mode();
}

void AstraSplitViewController::SetLayoutMode(AstraSplitLayoutMode mode) {
  if (!is_active_ || !split_view_) {
    // Update model even when inactive (for next activation).
    model_.SetLayoutMode(mode);
    return;
  }

  model_.SetLayoutMode(mode);

  // Update the view to reflect the new layout mode.
  split_view_->SetLayoutMode(mode);

  // Also update the orientation for 2-pane compatibility.
  bool horizontal = model_.IsHorizontal();
  orientation_ = horizontal ? SplitViewOrientation::kHorizontal
                            : SplitViewOrientation::kVertical;

  // Update metadata on tabs.
  WriteMetadataToTabs();
}

void AstraSplitViewController::CycleNextLayoutMode() {
  if (!is_active_) {
    model_.CycleNextLayoutMode();
    return;
  }
  model_.CycleNextLayoutMode();
  if (split_view_) {
    split_view_->SetLayoutMode(model_.layout_mode());
  }

  bool horizontal = model_.IsHorizontal();
  orientation_ = horizontal ? SplitViewOrientation::kHorizontal
                            : SplitViewOrientation::kVertical;
  WriteMetadataToTabs();
}

void AstraSplitViewController::CyclePreviousLayoutMode() {
  if (!is_active_) {
    model_.CyclePreviousLayoutMode();
    return;
  }
  model_.CyclePreviousLayoutMode();
  if (split_view_) {
    split_view_->SetLayoutMode(model_.layout_mode());
  }

  bool horizontal = model_.IsHorizontal();
  orientation_ = horizontal ? SplitViewOrientation::kHorizontal
                            : SplitViewOrientation::kVertical;
  WriteMetadataToTabs();
}

// =========================================================================
// Keyboard shortcut handling
// =========================================================================

bool AstraSplitViewController::HandleKeyboardShortcut(const ui::KeyEvent& event) {
  // TODO(astra): Integrate with Chromium's accelerator system properly.
  //   Chromium owner: chrome/browser/ui/views/accelerator_table.cc
  //   This is a skeleton implementation that demonstrates the concept.

  bool is_control = event.IsControlDown() || event.IsCommandDown();
  bool is_shift = event.IsShiftDown();
  bool is_alt = event.IsAltDown();

  // -- F-key shortcuts (no modifier required) -----------------------------

  if (event.key_code() == ui::VKEY_F6) {
    // F6: Cycle focus between panes (standard browser focus cycle).
    if (is_shift) {
      CycleFocusPreviousPane();
    } else {
      CycleFocusNextPane();
    }
    return true;
  }

  // -- Modifier-required shortcuts ----------------------------------------

  if (!is_control) {
    return false;
  }

  switch (event.key_code()) {
    case ui::VKEY_RETURN:
      // Ctrl+Enter: Toggle split view.
      if (is_shift) {
        // Ctrl+Shift+Enter: Toggle orientation.
        ToggleOrientation();
      } else {
        ToggleSplitView();
      }
      return true;

    case ui::VKEY_BACK:
    case ui::VKEY_OEM_5:  // Backslash '\' key (US keyboard)
      // Ctrl+\ or Ctrl+|: Toggle split view / swap panes.
      if (is_shift) {
        // Ctrl+Shift+\ (Ctrl+|): Swap panes.
        SwapPanes();
      } else {
        // Ctrl+\: Toggle split view.
        ToggleSplitView();
      }
      return true;

    case ui::VKEY_D:
      // Ctrl+D: Swap panes.
      SwapPanes();
      return true;

    case ui::VKEY_M:
      // Ctrl+M: Maximize/restore focused pane.
      if (model_.IsPaneMaximized()) {
        model_.RestoreFromMaximized();
        if (split_view_) {
          split_view_->Unmaximize();
        }
      } else {
        model_.MaximizePane(model_.focused_pane());
        if (split_view_) {
          bool is_primary =
              model_.focused_pane() == AstraSplitPaneId::kPane0;
          split_view_->MaximizePane(is_primary);
        }
      }
      return true;

    case ui::VKEY_G:
      // Ctrl+G: Toggle orientation (horizontal/vertical).
      ToggleOrientation();
      return true;

    case ui::VKEY_E:
      // Ctrl+E: Equalize panes (reset to 50/50).
      model_.SetEqualRatios();
      if (split_view_) {
        split_view_->SetRatio(0.5, /*animate=*/true);
      }
      return true;

    case ui::VKEY_1:
    case ui::VKEY_2:
    case ui::VKEY_3:
    case ui::VKEY_4:
    case ui::VKEY_5:
    case ui::VKEY_6: {
      // Ctrl+1..6: Focus pane N.
      int pane_index = event.key_code() - ui::VKEY_1;
      AstraSplitPaneId pane_id = static_cast<AstraSplitPaneId>(pane_index);
      if (model_.IsValidPaneId(pane_id)) {
        if (is_shift) {
          // Ctrl+Shift+N: Move current tab to pane N.
          // TODO(astra): Implement tab movement between panes.
          //   Requires TabStripModel integration.
          //   Chromium owner: TabStripModel::DetachAndInsertWebContentsAt
        } else {
          // Ctrl+N: Focus pane N.
          model_.SetFocusedPane(pane_id);
          if (pane_index == 0) {
            FocusPrimaryPane();
          } else if (pane_index == 1) {
            FocusSecondaryPane();
          }
        }
      }
      return true;
    }

    case ui::VKEY_OEM_4:  // '[' key
      // Ctrl+[: Focus previous pane.
      model_.FocusPreviousPane();
      if (model_.focused_pane() == AstraSplitPaneId::kPane0) {
        FocusPrimaryPane();
      } else {
        FocusSecondaryPane();
      }
      return true;

    case ui::VKEY_OEM_6:  // ']' key
      // Ctrl+]: Focus next pane.
      model_.FocusNextPane();
      if (model_.focused_pane() == AstraSplitPaneId::kPane0) {
        FocusPrimaryPane();
      } else {
        FocusSecondaryPane();
      }
      return true;

    case ui::VKEY_OEM_PLUS:
    case ui::VKEY_ADD:
      // Ctrl++: Grow focused pane (resize).
      // TODO(astra): Implement resize by keyboard step.
      //   The step size should be configurable via settings.
      ResizePrimaryPane(/*delta=*/20);
      return true;

    case ui::VKEY_OEM_MINUS:
    case ui::VKEY_SUBTRACT:
      // Ctrl+-: Shrink focused pane (resize).
      ResizePrimaryPane(/*delta=*/-20);
      return true;

    case ui::VKEY_W:
      // Ctrl+W: Close focused pane.
      if (model_.focused_pane() == AstraSplitPaneId::kPane0) {
        ClosePrimaryPane();
      } else {
        CloseSecondaryPane();
      }
      return true;

    case ui::VKEY_ZERO:
    case ui::VKEY_NUMPAD0:
      // Ctrl+0: Reset split ratio to default (equal).
      SetSplitRatio(0.5);
      return true;

    case ui::VKEY_H:
      // Ctrl+H: Toggle pane headers.
      model_.SetShowPaneHeaders(!model_.show_pane_headers());
      if (split_view_) {
        split_view_->SetShowPaneHeaders(model_.show_pane_headers());
      }
      return true;

    case ui::VKEY_L:
      // Ctrl+L: Cycle to next layout mode.
      CycleNextLayoutMode();
      return true;

    default:
      break;
  }

  // -- Alt+Ctrl shortcuts -------------------------------------------------

  if (is_alt && is_control) {
    switch (event.key_code()) {
      case ui::VKEY_LEFT:
        // Alt+Ctrl+Left: Shift divider left.
        ResizePrimaryPane(-20);
        return true;
      case ui::VKEY_RIGHT:
        // Alt+Ctrl+Right: Shift divider right.
        ResizePrimaryPane(20);
        return true;
      case ui::VKEY_UP:
        // Alt+Ctrl+Up: Shift divider up (vertical split).
        ResizePrimaryPane(-20);
        return true;
      case ui::VKEY_DOWN:
        // Alt+Ctrl+Down: Shift divider down (vertical split).
        ResizePrimaryPane(20);
        return true;
      default:
        break;
    }
  }

  return false;
}

// =========================================================================
// Tab drag and drop
// =========================================================================

bool AstraSplitViewController::CanDropTabOnPane(
    AstraSplitPaneId pane_id,
    content::WebContents* dragged_contents) const {
  if (!is_active_ || !dragged_contents) {
    return false;
  }

  // Can't drop on the same pane that contains the tab.
  // TODO(astra): Check if the dragged contents are already in this pane.
  //   Requires tracking which WebContents are in which pane.
  //   Chromium owner: TabStripModel + WebContents mapping.

  // Can't drop if the pane ID is invalid.
  if (!model_.IsValidPaneId(pane_id)) {
    return false;
  }

  return true;
}

void AstraSplitViewController::DropTabOnPane(AstraSplitPaneId pane_id,
                                              content::WebContents* dropped_contents) {
  if (!is_active_ || !dropped_contents) {
    return;
  }
  if (!model_.IsValidPaneId(pane_id)) {
    return;
  }

  // Replace the pane's contents with the dropped tab.
  // TODO(astra): Implement actual tab replacement with proper WebContents
  //   view management.  Requires TabStripModel integration for detaching
  //   and reattaching tabs.
  //   Chromium owner: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)
  //   Patch point: May need a small patch to expose tab detachment APIs.

  int pane_index = static_cast<int>(pane_id);
  if (pane_index == 0) {
    ReplacePrimaryTab(dropped_contents);
  } else if (pane_index == 1) {
    ReplaceSecondaryTab(dropped_contents);
  }

  // Update the model's focused pane to the drop target.
  model_.SetFocusedPane(pane_id);
}

bool AstraSplitViewController::StartSplitFromDrag(
    AstraSplitPaneId source_pane_id,
    content::WebContents* dragged_contents) {
  if (!is_active_ || !dragged_contents) {
    return false;
  }

  // TODO(astra): Implement split-from-drag.
  //   Dragging a tab out of a pane could create a new split, or open
  //   a new window, or reorder panes.
  //   Chromium owner: TabDragController (chrome/browser/ui/views/tabs)
  //   This is a placeholder for the feature.

  // For now, just indicate that the drag is accepted.
  return true;
}

// =========================================================================
// Workspace sync
// =========================================================================

void AstraSplitViewController::SaveSplitStateForWorkspace(
    const std::string& workspace_id) {
  if (workspace_id.empty()) {
    return;
  }

  // Save the current model state.
  std::string state = model_.SerializeToString();
  workspace_states_[workspace_id] = state;

  // Also update the model's workspace association.
  model_.SetWorkspaceId(workspace_id);

  // TODO(astra): Persist to PrefService for cross-session storage.
  //   Chromium owner: PrefService (components/prefs/pref_service.h)
  //   The state should be stored as a dictionary pref keyed by workspace ID.
}

bool AstraSplitViewController::RestoreSplitStateForWorkspace(
    const std::string& workspace_id) {
  if (workspace_id.empty()) {
    return false;
  }

  auto it = workspace_states_.find(workspace_id);
  if (it == workspace_states_.end()) {
    return false;
  }

  // Deserialize the saved state into the model.
  bool success = model_.DeserializeFromString(it->second);
  if (!success) {
    return false;
  }

  // Update the model's workspace association.
  model_.SetWorkspaceId(workspace_id);

  // If split view is active, update the view to reflect the restored state.
  if (is_active_ && split_view_) {
    split_view_->SetLayoutMode(model_.layout_mode());
    // TODO(astra): Restore divider positions and other state.
    //   For now, we just update the layout mode.
    //   Full restore should also restore ratios and focused pane, headers, etc.
  }

  return true;
}

void AstraSplitViewController::ClearWorkspaceState(
    const std::string& workspace_id) {
  workspace_states_.erase(workspace_id);
}

bool AstraSplitViewController::HasWorkspaceState(
    const std::string& workspace_id) const {
  return workspace_states_.find(workspace_id) != workspace_states_.end();
}

// =========================================================================
// Focus cycling
// =========================================================================

void AstraSplitViewController::CycleFocusNextPane() {
  if (!is_active_) {
    return;
  }

  model_.FocusNextPane();

  // Sync the focused pane in the view.
  if (split_view_) {
    AstraSplitPane pane = (model_.focused_pane() == AstraSplitPaneId::kPane0)
                             ? AstraSplitPane::kPrimary
                             : AstraSplitPane::kSecondary;
    split_view_->SetFocusedPane(pane);
  }

  // TODO(astra): Actually focus the WebContents in the newly focused pane.
  //   Chromium owner: content::WebContents::Focus()
  //   and views::View::RequestFocus()
}

void AstraSplitViewController::CycleFocusPreviousPane() {
  if (!is_active_) {
    return;
  }

  model_.FocusPreviousPane();

  if (split_view_) {
    AstraSplitPane pane = (model_.focused_pane() == AstraSplitPaneId::kPane0)
                             ? AstraSplitPane::kPrimary
                             : AstraSplitPane::kSecondary;
    split_view_->SetFocusedPane(pane);
  }
}

AstraSplitPaneId AstraSplitViewController::CycleFocus() {
  CycleFocusNextPane();
  return model_.focused_pane();
}

// =========================================================================
// History / back-forward per pane
// =========================================================================

void AstraSplitViewController::GoBackInFocusedPane() {
  if (!is_active_) {
    return;
  }
  GoBackInPane(model_.focused_pane());
}

void AstraSplitViewController::GoForwardInFocusedPane() {
  if (!is_active_) {
    return;
  }
  GoForwardInPane(model_.focused_pane());
}

void AstraSplitViewController::ReloadFocusedPane() {
  if (!is_active_) {
    return;
  }
  ReloadPane(model_.focused_pane());
}

void AstraSplitViewController::GoBackInPane(AstraSplitPaneId pane_id) {
  if (!is_active_ || !model_.IsValidPaneId(pane_id)) {
    return;
  }

  // TODO(astra): Navigate back in the specified pane's WebContents.
  //   Chromium owner: content::NavigationController::GoBack()
  //   (content/public/browser/navigation_controller.h)
  //
  //   We need to get the WebContents for the pane and call:
  //     web_contents->GetController().GoBack();
  //
  //   For now, this is a no-op placeholder.
  //   Chromium subsystem: NavigationController per WebContents.
  DLOG(INFO) << "GoBack in pane " << static_cast<int>(pane_id);
}

void AstraSplitViewController::GoForwardInPane(AstraSplitPaneId pane_id) {
  if (!is_active_ || !model_.IsValidPaneId(pane_id)) {
    return;
  }

  // TODO(astra): Navigate forward in the specified pane's WebContents.
  //   Chromium owner: content::NavigationController::GoForward()
  DLOG(INFO) << "GoForward in pane " << static_cast<int>(pane_id);
}

void AstraSplitViewController::ReloadPane(AstraSplitPaneId pane_id) {
  if (!is_active_ || !model_.IsValidPaneId(pane_id)) {
    return;
  }

  // TODO(astra): Reload the specified pane's WebContents.
  //   Chromium owner: content::WebContents::GetController().Reload()
  DLOG(INFO) << "Reload pane " << static_cast<int>(pane_id);
}

// =========================================================================
// Tab-to-split / split-to-tab conversion
// =========================================================================

bool AstraSplitViewController::ConvertTabToSplit(content::WebContents* web_contents,
                                               AstraSplitPaneId target_pane) {
  if (!web_contents || !model_.IsValidPaneId(target_pane)) {
    return false;
  }

  // If split view is not active, activate it first.
  if (!is_active_) {
    // TODO(astra): Find a suitable partner tab and activate split view.
    //   For now, we just activate with defaults.
    ActivateSplitView();
  }

  // Replace the target pane's contents with the new tab.
  int pane_index = static_cast<int>(target_pane);
  if (pane_index == 0) {
    ReplacePrimaryTab(web_contents);
  } else if (pane_index == 1) {
    ReplaceSecondaryTab(web_contents);
  }

  // Notify the model of the conversion.
  model_.NotifyTabToSplitConverted(target_pane);

  return true;
}

bool AstraSplitViewController::ConvertSplitToTab(AstraSplitPaneId pane_id) {
  if (!is_active_ || !model_.IsValidPaneId(pane_id)) {
    return false;
  }

  // Notify the model of the conversion.
  model_.NotifySplitToTabConverted(pane_id);

  // TODO(astra): For 2-pane mode, converting one pane to a tab means deactivating
  //   split view and keeping the remaining tab as the active tab.
  //   For multi-pane mode, we'd need to rebalance the layout.
  //
  //   Chromium owner: TabStripModel for tab management.
  //   Patch point: No patch needed — TabStripModel is public API.

  int pane_count = model_.GetPaneCount();
  if (pane_count <= 2) {
    // 2-pane mode — closing one pane deactivates split view.
    // The other pane becomes the single active tab.
    DeactivateSplitView();
  } else {
    // Multi-pane mode — close the pane and rebalance.
    // TODO(astra): Implement multi-pane close and rebalance.
    model_.ClosePane(pane_id);
  }

  return true;
}

}  // namespace astra
