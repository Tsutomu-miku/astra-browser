#ifndef ASTRA_UI_VIEWS_GLANCE_ASTRA_GLANCE_VIEW_CONTROLLER_H_
#define ASTRA_UI_VIEWS_GLANCE_ASTRA_GLANCE_VIEW_CONTROLLER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/memory/raw_ptr.h"
#include "base/timer/timer.h"
#include "content/public/browser/web_contents_observer.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "url/gurl.h"

#include "astra/ui/views/glance/astra_glance_view.h"

class PrefService;

namespace content {
class WebContents;
class BrowserContext;
}  // namespace content

namespace views {
class Widget;
class View;
}  // namespace views

class BrowserView;

namespace astra {

// =========================================================================
// AstraGlanceSize — size presets for the glance bubble
// =========================================================================
//
// Quick size presets that control the overall dimensions of the glance bubble.
// The actual pixel sizes come from PrefService or hardcoded defaults.
//
//  - kSmall: compact thumbnail-sized preview (quick info)
//  - kMedium: default size
//  - kLarge: large preview
//  - kExtraLarge: almost full-tab preview
enum class AstraGlanceSize {
  kSmall,
  kMedium,
  kLarge,
  kExtraLarge,
};

// =========================================================================
// AstraGlancePlacement — placement of the glance relative to its anchor
// =========================================================================
//
// Controls which side of the anchor point the glance bubble appears on.
// kAuto lets the system choose the best placement based on screen bounds.
enum class AstraGlancePlacement {
  kAuto,
  kBelow,
  kAbove,
  kLeft,
  kRight,
};

// =========================================================================
// AstraGlanceTriggerMode — what triggers a glance to appear
// =========================================================================
//
// Different ways a glance can be triggered:
//  - kHover: show immediately on mouse hover (short delay)
//  - kHoverLong: show after a longer hover delay
//  - kClick: show on click
//  - kKeyboard: show via keyboard shortcut
//  - kDisabled: glance is disabled
enum class AstraGlanceTriggerMode {
  kHover,
  kHoverLong,
  kClick,
  kKeyboard,
  kDisabled,
};

class AstraGlanceViewController;

// =========================================================================
// AstraGlanceObserver — observer interface for glance state changes
// =========================================================================
//
// Observers are notified of glance state transitions.  All methods have
// empty default implementations so observers can override only what they
// need.  Extends base::CheckedObserver for safe observer list management.
//
// Use AstraGlanceViewController::AddObserver() / RemoveObserver() to
// subscribe.
class AstraGlanceObserver : public base::CheckedObserver {
 public:
  // Called when the glance becomes visible for a tab.
  virtual void OnGlanceShown(AstraGlanceViewController* controller,
                             int tab_index) {}

  // Called when the glance is hidden.
  virtual void OnGlanceHidden(AstraGlanceViewController* controller) {}

  // Called when the glance pinned state changes.
  virtual void OnGlancePinned(AstraGlanceViewController* controller,
                              bool pinned) {}

  // Called when the glance expanded state changes.
  virtual void OnGlanceExpanded(AstraGlanceViewController* controller,
                                bool expanded) {}

  // Called when the glance size changes.
  virtual void OnGlanceSizeChanged(AstraGlanceViewController* controller,
                                   const gfx::Size& new_size) {}

  // Called when the glance content type changes.
  virtual void OnGlanceContentTypeChanged(
      AstraGlanceViewController* controller,
      AstraGlanceContentType type) {}

  // Called when the controller is shutting down.
  virtual void OnGlanceViewControllerShutdown(
      AstraGlanceViewController* controller) {}

 protected:
  ~AstraGlanceObserver() override = default;
};

// =========================================================================
// AstraGlanceViewController — controller for the glance / peek overlay
// =========================================================================
//
// Manages the lifecycle, state, and behavior of a glance preview overlay.
// The controller is NOT a View subclass — it is a coordinator object that
// owns the glance view widget and the optional temporary WebContents (for
// URL glance mode).
//
// Two glance modes:
//
//   **Tab glance** — preview an existing tab from the sidebar.
//     - Uses an existing WebContents (owned by TabStripModel).
//     - The original tab stays in TabStripModel.
//     - The glance is a different view of the same WebContents.
//     - When dismissed, the WebContents returns to its normal tab position.
//
//   **URL glance** — preview a URL (link hover preview).
//     - Creates a new temporary WebContents.
//     - Loads the URL into it.
//     - Shows it in the glance view.
//     - When dismissed, the WebContents is either discarded or promoted to
//       a full tab in TabStripModel.
//
// Three trigger sources:
//   - Sidebar hover: user hovers over a sidebar tab item.
//   - Link hover: user hovers over a link in a page.
//   - Keyboard: user presses a keyboard shortcut to peek at the current tab.
//
// Hover-triggered glances have a show delay (to avoid accidental triggers)
// and auto-hide on mouse leave (with hysteresis to prevent flicker).
//
// Observer pattern:
//   Observers can listen for glance state changes (shown, hidden, expanded,
//   pinned as tab).  Use AddObserver() / RemoveObserver().
//
// Delegate pattern:
//   The controller delegates browser-level actions (open tab, add favorite,
//   close tab, etc.) to its Delegate interface.  The browser view or a
//   higher-level controller implements the delegate.
//
// Ownership boundary (CRITICAL):
//   - For tab glance: Chromium (TabStripModel) owns the WebContents.
//     The controller only temporarily reparents its view into the glance bubble.
//   - For URL glance: The controller owns the temporary WebContents via
//     unique_ptr.  On close, it either destroys it or transfers ownership
//     to TabStripModel (promote to tab).
//   - The glance view widget is owned by the Views widget system.
//   - The controller holds a raw_ptr to the view and is notified of destruction.
//
// State model:
//   - Glance state is stored as AstraTabFeatures metadata on the WebContents
//     being shown (is_glance_tab, glance_source_tab_id).
//   - The controller sets these fields when showing glance and clears them
//     when hiding.
//
// TODO(astra): Investigate whether a single WebContents can have its view
//   shown in two places simultaneously (tab strip + glance bubble).
//   If not, tab glance will need to either:
//     a) Temporarily detach the WebContents view from the tab strip and
//        reattach it to the glance bubble, restoring it on close.
//     b) Create a "preview" WebContents that mirrors the original (like
//        Chrome's tab preview / hover card feature).
//
//   Reference: Chrome tab hover cards (chrome/browser/ui/tabs/tab_hover_card_*)
//   show tab previews — but they use thumbnails/screenshots, not live
//   WebContents.  A live preview (like Edge's "peek" feature) would need
//   a second WebContents rendering the same page.
//
//   Chromium owner: content::WebContentsView (content/public/browser/),
//   chrome/browser/ui/tabs/tab_strip_model.h.
//
//   Patch point: May need a patch to WebContentsView to support showing the
//   same RenderWidgetHostView in multiple views::View containers, or a
//   lightweight "mirror" view.
// =========================================================================

class AstraGlanceViewController : public AstraGlanceView::Delegate,
                                  public content::WebContentsObserver {
 public:
  // -----------------------------------------------------------------------
  // Observer interface
  // -----------------------------------------------------------------------
  //
  // Observers are notified of glance state changes.  Observers do not
  // control the glance — they just react to state transitions.
  //
  // All observer methods have empty default implementations so observers
  // can override only the methods they care about.
  //
  // Use AddObserver() / RemoveObserver() to subscribe.
  class Observer : public base::CheckedObserver {
   public:
    // Called when the glance becomes visible.
    virtual void OnGlanceShown() {}

    // Called when the glance is hidden.
    virtual void OnGlanceHidden() {}

    // Called when the glance switches to expanded mode.
    virtual void OnGlanceExpanded() {}

    // Called when the glance is pinned as a full tab (promoted).
    virtual void OnGlancePinnedAsTab() {}

    // Called when the glance mode (tab/url) changes.
    virtual void OnGlanceModeChanged(Mode mode) {}

    // Called when the glance is resized (via drag handle or size preset).
    virtual void OnGlanceResized(const gfx::Size& new_size) {}

    // Called when the glance source (sidebar/link/keyboard) changes.
    virtual void OnGlanceSourceChanged(Source source) {}

    // Called when the glance pinned state changes (pinned open).
    virtual void OnGlancePinnedChanged(bool pinned) {}

    // Called when any glance presentation setting changes.
    virtual void OnGlanceSettingsChanged() {}

   protected:
    ~Observer() override = default;
  };

  // -----------------------------------------------------------------------
  // Delegate interface (browser integration)
  // -----------------------------------------------------------------------
  //
  // The delegate handles browser-level actions that the glance controller
  // cannot perform directly (it doesn't own the browser).  Typically
  // implemented by BrowserView or a browser-level coordinator.
  //
  // Controller -> Browser communication.
  class Delegate {
   public:
    // Open the given URL in a new tab in the browser window.
    // Returns the WebContents of the new tab, or nullptr on failure.
    // TODO(astra): Figure out return type — do we need the WebContents back?
    virtual void OpenURLInNewTab(const GURL& url) = 0;

    // Toggle the favorite state of the given WebContents.
    virtual void ToggleFavorite(content::WebContents* web_contents) = 0;

    // Close the tab corresponding to the given WebContents.
    // After this call, the WebContents pointer is invalid.
    virtual void CloseTab(content::WebContents* web_contents) = 0;

    // Toggle the pinned state of the given WebContents.
    virtual void TogglePinned(content::WebContents* web_contents) = 0;

    // Copy the given URL to the system clipboard.
    virtual void CopyURLToClipboard(const GURL& url) = 0;

    // Returns the currently active WebContents in the browser, or nullptr.
    // Used to determine the source tab for link-hover glance.
    virtual content::WebContents* GetActiveWebContents() = 0;

   protected:
    ~Delegate() = default;
  };

  // The trigger source of the current glance request.
  using Source = AstraGlanceView::Source;

  // The mode of the current glance.
  enum class Mode {
    kNone,     // No active glance.
    kTab,      // Previewing an existing tab WebContents.
    kUrl,      // Previewing a URL with a temporary WebContents.
  };

  explicit AstraGlanceViewController(BrowserView* browser_view);
  ~AstraGlanceViewController() override;

  AstraGlanceViewController(const AstraGlanceViewController&) = delete;
  AstraGlanceViewController& operator=(const AstraGlanceViewController&) = delete;

  // -- Show / hide --------------------------------------------------------

  // Show glance for an existing tab's WebContents, anchored to |anchor|.
  // |for_contents| must be a valid WebContents owned by TabStripModel.
  // Sets is_glance_tab=true on |for_contents|'s AstraTabFeatures.
  void ShowGlance(content::WebContents* for_contents,
                  const gfx::Rect& anchor,
                  Source source = Source::kUnknown);

  // Show glance for a URL, creating a temporary WebContents.
  // The URL is loaded into a new WebContents owned by this controller.
  // Sets is_glance_tab=true on the temporary WebContents' AstraTabFeatures.
  void ShowGlanceForURL(const GURL& url,
                        const gfx::Rect& anchor,
                        Source source = Source::kUnknown);

  // Show glance for a URL after a delay (for hover-triggered glances).
  // If HideGlance() is called before the delay expires, the glance
  // never appears (prevents accidental glances from quick mouse moves).
  void ShowGlanceForURLDelayed(const GURL& url,
                               const gfx::Rect& anchor,
                               base::TimeDelta delay,
                               Source source = Source::kUnknown);

  // Show glance for a tab after a delay.
  void ShowGlanceDelayed(content::WebContents* for_contents,
                         const gfx::Rect& anchor,
                         base::TimeDelta delay,
                         Source source = Source::kUnknown);

  // Hide the glance and clean up.
  // For tab glance: restores the WebContents to its normal position.
  // For URL glance: destroys the temporary WebContents unless promoted.
  // If a delayed show is pending, it is cancelled.
  void HideGlance();

  // Returns true if a glance is currently visible.
  bool IsGlanceVisible() const;

  // Returns true if a delayed show is pending.
  bool IsShowPending() const { return show_delay_timer_.IsRunning(); }

  // -- Show / hide (tab index API) ----------------------------------------
  //
  // Tab-index-based API for showing glance for a tab by its index in the
  // tab strip.  These are the primary public API methods for controlling
  // glance visibility from higher-level code.

  // Show glance for the tab at |tab_index|, anchored at |anchor_point|
  // (in screen coordinates).
  void ShowGlance(int tab_index, const gfx::Point& anchor_point);

  // Hide the glance immediately.
  void HideGlance();

  // Hide the glance after the configured dismiss delay.
  // The hide can be cancelled with CancelHide().
  void HideGlanceAfterDelay();

  // Cancel a pending delayed hide.
  void CancelHide();

  // Returns true if the glance is currently visible.
  bool IsVisible() const;

  // Returns true if a show or hide animation is in progress.
  // TODO(astra): Implement real animation tracking when animations are
  //   properly implemented.  Chromium pattern: views::Animation.
  bool IsAnimating() const;

  // Returns the index of the tab currently being glanced, or -1 if none.
  int GetTabIndex() const;

  // -- Accessors ----------------------------------------------------------

  // The WebContents being shown in the glance view, or nullptr.
  content::WebContents* glance_contents() { return glance_contents_; }
  const content::WebContents* glance_contents() const { return glance_contents_; }

  // The glance view widget, or nullptr if not visible.
  views::Widget* glance_widget() { return glance_widget_; }

  // The glance view itself, or nullptr if not visible.
  AstraGlanceView* glance_view() { return glance_view_; }

  // Current glance mode.
  Mode mode() const { return mode_; }

  // Current glance source.
  Source source() const { return source_; }

  // -- Display mode -------------------------------------------------------

  // Switch between compact and expanded display modes.
  void SetDisplayMode(AstraGlanceView::DisplayMode mode);
  void ToggleDisplayMode();

  // -- Promotion ----------------------------------------------------------

  // Promote the current glance to a full tab in the tab strip.
  // For tab glance: no-op (already a tab).
  // For URL glance: adds the temporary WebContents to TabStripModel and
  // transfers ownership.
  // Returns true if promotion happened, false if not applicable.
  bool PromoteToTab();

  // -- Pinning (glance pinned open) ----------------------------------------
  //
  // When the glance is pinned, it stays open until explicitly closed.
  // Auto-hide on mouse leave is disabled while pinned.
  // This is separate from "pin tab" — it pins the *glance window* open.

  // Pin the glance open (prevents auto-hide).
  void SetPinned(bool pinned);
  bool IsPinned() const { return is_pinned_; }
  void TogglePinned();

  // -- Size presets --------------------------------------------------------
  //
  // Quick resize presets: small, medium, large.
  // The actual sizes are configurable via prefs.

  enum class SizePreset {
    kSmall,
    kMedium,
    kLarge,
  };

  // Apply a size preset to the current glance.
  // Returns true if the size was changed.
  bool ApplySizePreset(SizePreset preset);

  // Get the size for a given preset (reads from prefs).
  gfx::Size GetPresetSize(SizePreset preset) const;

  // Get the current size preset (based on current widget size).
  SizePreset GetCurrentSizePreset() const;

  // -- Content type -------------------------------------------------------

  // Set the type of content currently shown in the glance.
  void SetContentType(AstraGlanceContentType type);
  AstraGlanceContentType GetContentType() const;

  // -- Sizing -------------------------------------------------------------
  //
  // Methods for controlling the glance size.  Sizes are in DIPs.

  // Set the exact size of the glance bubble.
  void SetSize(const gfx::Size& size);
  gfx::Size GetSize() const;

  // Set the size preset (convenience API using AstraGlanceSize enum).
  void SetSizePreset(AstraGlanceSize preset);
  AstraGlanceSize GetSizePreset() const;

  // Set the minimum allowed size.
  void SetMinSize(const gfx::Size& size);
  gfx::Size GetMinSize() const;

  // Set the maximum allowed size.
  void SetMaxSize(const gfx::Size& size);
  gfx::Size GetMaxSize() const;

  // Set whether the aspect ratio is maintained during resize.
  void SetMaintainAspectRatio(bool maintain);
  bool GetMaintainAspectRatio() const;

  // -- Positioning --------------------------------------------------------
  //
  // Methods for controlling where the glance appears relative to its anchor.

  // Set the anchor point (screen coordinates).
  void SetAnchorPosition(const gfx::Point& point);
  gfx::Point GetAnchorPosition() const;

  // Set the placement of the glance relative to its anchor.
  void SetPlacement(AstraGlancePlacement placement);
  AstraGlancePlacement GetPlacement() const;

  // Set the distance in pixels between the anchor and the glance edge.
  void SetOffset(int offset_px);
  int GetOffset() const;

  // -- Position presets ----------------------------------------------------
  //
  // Controls which side of the anchor the glance appears on.

  enum class Position {
    kLeft,
    kRight,
    kTop,
    kBottom,
  };

  // Set the glance position (which side of the anchor).
  void SetPosition(Position position);
  Position position() const { return position_; }

  // Cycle to the next position preset.
  void CyclePosition();

  // Convert Position to/from string (for prefs).
  static std::string PositionToString(Position position);
  static Position StringToPosition(const std::string& str);

  // -- Recent glance history ----------------------------------------------
  //
  // Tracks recently-previewed URLs for quick re-preview.
  // Persisted via PrefService.

  // Add a URL to recent glance history.
  void AddToRecentGlances(const GURL& url);

  // Get the list of recent glance URLs (most recent first).
  std::vector<GURL> GetRecentGlances() const;

  // Clear all recent glance history.
  void ClearRecentGlances();

  // -- Trigger settings ---------------------------------------------------
  //
  // Controls how and when glances are triggered.

  // Set the trigger mode for showing glances.
  void SetTriggerMode(AstraGlanceTriggerMode mode);
  AstraGlanceTriggerMode GetTriggerMode() const;

  // Set the delay in milliseconds before a hover-triggered glance appears.
  void SetHoverDelay(int delay_ms);
  int GetHoverDelay() const;

  // Set the delay in milliseconds before an auto-dismiss happens.
  void SetDismissDelay(int delay_ms);
  int GetDismissDelay() const;

  // -- Hover trigger toggles ----------------------------------------------
  //
  // Controls which UI elements trigger a glance on hover.

  void SetShowOnTabHover(bool show);
  bool GetShowOnTabHover() const;

  void SetShowOnBookmarkHover(bool show);
  bool GetShowOnBookmarkHover() const;

  void SetShowOnHistoryHover(bool show);
  bool GetShowOnHistoryHover() const;

  // -- Interaction actions ------------------------------------------------
  //
  // User actions that can be performed on the glanced content.

  // Pin the glance open (prevents auto-dismiss).
  void PinGlance();
  void UnpinGlance();
  bool IsPinned() const;

  // Expand the glance to a larger size.
  void ExpandGlance();
  void CollapseGlance();
  bool IsExpanded() const;

  // Navigate to the glanced tab (switch the active tab to it).
  void NavigateToTab();

  // Open the glanced URL in a new tab.
  void OpenInNewTab();

  // Close the glanced tab.
  void CloseTab();

  // -- Presentation settings ----------------------------------------------
  //
  // These are persisted via PrefService.
  // The controller reads them and applies them to the view.

  // Get/set default display mode from prefs.
  AstraGlanceView::DisplayMode GetDefaultDisplayMode() const;
  void SetDefaultDisplayMode(AstraGlanceView::DisplayMode mode);

  // Get/set show delay from prefs.
  base::TimeDelta GetShowDelay() const;
  void SetShowDelay(base::TimeDelta delay);

  // Get/set auto-hide delay from prefs.
  base::TimeDelta GetAutoHideDelay() const;
  void SetAutoHideDelay(base::TimeDelta delay);

  // Get/set compact size from prefs.
  gfx::Size GetCompactSize() const;
  void SetCompactSize(const gfx::Size& size);

  // Get/set expanded size from prefs.
  gfx::Size GetExpandedSize() const;
  void SetExpandedSize(const gfx::Size& size);

  // Get/set show action bar from prefs.
  bool GetShowActionBar() const;
  void SetShowActionBar(bool show);

  // Get/set show status bar from prefs.
  bool GetShowStatusBar() const;
  void SetShowStatusBar(bool show);

  // Get/set show resize handle from prefs.
  bool GetShowResizeHandle() const;
  void SetShowResizeHandle(bool show);

  // Get/set hover peek enabled from prefs.
  bool GetHoverPeekEnabled() const;
  void SetHoverPeekEnabled(bool enabled);

  // Get/set default position from prefs.
  Position GetDefaultPosition() const;
  void SetDefaultPosition(Position position);

  // Get/set pinned by default from prefs.
  bool GetPinnedByDefault() const;
  void SetPinnedByDefault(bool pinned);

  // Get/set remember size from prefs.
  bool GetRememberSize() const;
  void SetRememberSize(bool remember);

  // Get/set animations enabled from prefs.
  bool GetAnimationsEnabled() const;
  void SetAnimationsEnabled(bool enabled);

  // Get/set show settings button from prefs.
  bool GetShowSettingsButton() const;
  void SetShowSettingsButton(bool show);

  // -- Static pref keys ---------------------------------------------------
  //
  // Pref key constants for glance-related settings.  These are the pref
  // paths used with PrefService to read and write glance settings.
  //
  // All keys are public static constexpr so they can be referenced from
  // registration code (astra_prefs.cc) and test code.

  // Default size preset (AstraGlanceSize as string: "small", "medium", etc.)
  static constexpr char kPrefDefaultSizePreset[] =
      "astra.glance.default_size_preset";

  // Default placement (AstraGlancePlacement as string)
  static constexpr char kPrefDefaultPlacement[] =
      "astra.glance.default_placement";

  // Trigger mode (AstraGlanceTriggerMode as string)
  static constexpr char kPrefTriggerMode[] =
      "astra.glance.trigger_mode";

  // Hover delay in milliseconds
  static constexpr char kPrefHoverDelayMs[] =
      "astra.glance.hover_delay_ms";

  // Dismiss delay in milliseconds (auto-hide)
  static constexpr char kPrefDismissDelayMs[] =
      "astra.glance.dismiss_delay_ms";

  // Maintain aspect ratio during resize
  static constexpr char kPrefMaintainAspectRatio[] =
      "astra.glance.maintain_aspect_ratio";

  // Show glance on tab hover
  static constexpr char kPrefShowOnTabHover[] =
      "astra.glance.show_on_tab_hover";

  // Show glance on bookmark hover
  static constexpr char kPrefShowOnBookmarkHover[] =
      "astra.glance.show_on_bookmark_hover";

  // Show glance on history hover
  static constexpr char kPrefShowOnHistoryHover[] =
      "astra.glance.show_on_history_hover";

  // Show glance on sidebar item hover
  static constexpr char kPrefShowOnSidebarItemHover[] =
      "astra.glance.show_on_sidebar_item_hover";

  // Pin glance on click
  static constexpr char kPrefPinOnClick[] =
      "astra.glance.pin_on_click";

  // Expand glance on double-click
  static constexpr char kPrefExpandOnDoubleClick[] =
      "astra.glance.expand_on_double_click";

  // Animation enabled
  static constexpr char kPrefAnimationEnabled[] =
      "astra.glance.animation_enabled";

  // Animation duration in milliseconds
  static constexpr char kPrefAnimationDurationMs[] =
      "astra.glance.animation_duration_ms";

  // Show close button
  static constexpr char kPrefShowCloseButton[] =
      "astra.glance.show_close_button";

  // Show pin button
  static constexpr char kPrefShowPinButton[] =
      "astra.glance.show_pin_button";

  // Show open button
  static constexpr char kPrefShowOpenButton[] =
      "astra.glance.show_open_button";

  // Offset from anchor in pixels
  static constexpr char kPrefOffsetPx[] =
      "astra.glance.offset_px";

  // -- Utility methods ----------------------------------------------------

  // Get the PrefService used by this controller.
  PrefService* GetPrefService() const;

  // Reset all glance settings to defaults.
  void ResetSettingsToDefaults();

  // -- Observer management ------------------------------------------------

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // -- AstraGlanceObserver management --------------------------------------
  //
  // New observer interface (AstraGlanceObserver) for glance state changes.
  // Observers are notified of all significant state transitions.

  void AddObserver(AstraGlanceObserver* observer);
  void RemoveObserver(AstraGlanceObserver* observer);

  // -- Delegate -----------------------------------------------------------

  void set_delegate(Delegate* delegate) { delegate_ = delegate; }
  Delegate* delegate() { return delegate_; }

  // -- Auto-hide control --------------------------------------------------

  // Enable or disable auto-hide on mouse leave.
  void set_auto_hide_on_mouse_leave(bool auto_hide) {
    auto_hide_on_mouse_leave_ = auto_hide;
  }
  bool auto_hide_on_mouse_leave() const {
    return auto_hide_on_mouse_leave_;
  }

  // Notify the controller that the mouse entered the glance area.
  // Cancels any pending auto-hide timer.
  void OnMouseEnteredGlance();

  // Notify the controller that the mouse left the glance area.
  // Starts the auto-hide timer if auto-hide is enabled.
  void OnMouseLeftGlance();

  // Set the auto-hide delay (hysteresis).
  void set_auto_hide_delay(base::TimeDelta delay) {
    auto_hide_delay_ = delay;
  }
  base::TimeDelta auto_hide_delay() const { return auto_hide_delay_; }

  // -- AstraGlanceView::Delegate -----------------------------------------

  void OnGlanceCloseRequested() override;
  void OnGlancePromoteToTab() override;
  void OnGlanceViewDestroyed() override;
  void OnGlanceAddToFavorites() override;
  void OnGlanceCopyURL() override;
  void OnGlancePinTab() override;
  void OnGlanceCloseTab() override;
  void OnGlanceToggleExpanded() override;

  // -- content::WebContentsObserver ---------------------------------------

  void DidStartLoading() override;
  void DidStopLoading() override;
  void PrimaryPageChanged(content::Page& page) override;
  void DidFailLoad(content::RenderFrameHost* render_frame_host,
                   const GURL& validated_url,
                   int error_code) override;
  void TitleWasSet(content::NavigationEntry* entry) override;

 private:
  // Get the BrowserContext (profile) from the browser view.
  content::BrowserContext* GetBrowserContext();

  // Get the anchor view for the glance bubble.
  // TODO(astra): Determine the right anchor view.  For sidebar-triggered
  // glance, this is the sidebar item view.  For link hover, we may need
  // to create an anchor or use the WebContents view.
  views::View* GetAnchorView();

  // Create a temporary WebContents for URL glance mode.
  // The WebContents is created with Chromium's WebContents::Create() pattern.
  // TODO(astra): Use the correct WebContents creation parameters including
  // browser context, site instance, and initial dimensions.
  std::unique_ptr<content::WebContents> CreateTemporaryWebContents(
      const GURL& url);

  // Write glance metadata to the WebContents' AstraTabFeatures.
  void WriteGlanceMetadata(content::WebContents* web_contents,
                           const std::string& source_tab_id);

  // Clear glance metadata from the WebContents' AstraTabFeatures.
  void ClearGlanceMetadata(content::WebContents* web_contents);

  // Restore a tab-mode WebContents to its normal position in the tab strip.
  // After the glance bubble is closed, the WebContents view must be
  // reattached to the normal content area.
  // TODO(astra): Implement proper WebContents view restoration.
  // This is the reverse of whatever mechanism we use to show it in the
  // glance bubble.  See the class-level TODO(astra) about dual view placement.
  void RestoreTabWebContents(content::WebContents* web_contents);

  // Generate a source tab identifier from a WebContents.
  // TODO(astra): Use a proper stable identifier, not pointer address.
  std::string GetSourceTabId(content::WebContents* web_contents);

  // -- Internal helpers ---------------------------------------------------

  // Internal show implementation used by both ShowGlance and the delayed
  // show timer callback.
  void ShowGlanceInternal(content::WebContents* for_contents,
                          const gfx::Rect& anchor,
                          Source source);

  // Internal URL show implementation.
  void ShowGlanceForURLInternal(const GURL& url,
                                const gfx::Rect& anchor,
                                Source source);

  // Start observing a WebContents for loading/title changes.
  void StartObservingContents(content::WebContents* web_contents);

  // Stop observing the current WebContents.
  void StopObservingContents();

  // Update the view's state from the current WebContents.
  void UpdateViewFromContents();

  // Notify observers of glance shown.
  void NotifyGlanceShown();

  // Notify observers of glance hidden.
  void NotifyGlanceHidden();

  // Notify observers of glance expanded.
  void NotifyGlanceExpanded();

  // Notify observers of glance pinned as tab.
  void NotifyGlancePinnedAsTab();

  // Notify observers of glance mode change.
  void NotifyGlanceModeChanged(Mode mode);

  // Notify observers of glance resize.
  void NotifyGlanceResized(const gfx::Size& new_size);

  // Notify observers of glance source change.
  void NotifyGlanceSourceChanged(Source source);

  // Notify observers of glance pinned state change.
  void NotifyGlancePinnedChanged(bool pinned);

  // Notify observers of glance settings change.
  void NotifyGlanceSettingsChanged();

  // Callback for the show delay timer.
  void OnShowDelayTimerFired();

  // Callback for the auto-hide timer.
  void OnAutoHideTimerFired();

  // Callback for the hide delay timer (HideGlanceAfterDelay).
  void OnHideDelayTimerFired();

  // -- New observer notification helpers ----------------------------------
  //
  // Notifications for the AstraGlanceObserver interface.

  void NotifyNewObserversGlanceShown(int tab_index);
  void NotifyNewObserversGlanceHidden();
  void NotifyNewObserversGlancePinned(bool pinned);
  void NotifyNewObserversGlanceExpanded(bool expanded);
  void NotifyNewObserversGlanceSizeChanged(const gfx::Size& new_size);
  void NotifyNewObserversGlanceContentTypeChanged(AstraGlanceContentType type);
  void NotifyNewObserversShutdown();

  // -- Members ------------------------------------------------------------

  raw_ptr<BrowserView> browser_view_;

  // Delegate for browser-level actions.
  raw_ptr<Delegate> delegate_ = nullptr;

  // Observers.
  base::ObserverList<Observer> observers_;

  // Current glance mode and source.
  Mode mode_ = Mode::kNone;
  Source source_ = Source::kUnknown;

  // The glance widget and view.
  // The widget is owned by the widget system; we hold raw_ptrs and are
  // notified of destruction via OnGlanceViewDestroyed().
  raw_ptr<views::Widget> glance_widget_ = nullptr;
  raw_ptr<AstraGlanceView> glance_view_ = nullptr;

  // The WebContents being shown in the glance.
  // For tab glance: owned by TabStripModel, we hold a raw_ptr.
  // For URL glance: owned by this controller (via glance_contents_owned_).
  raw_ptr<content::WebContents> glance_contents_ = nullptr;

  // Owned WebContents for URL glance mode.
  // null when in tab glance mode or no active glance.
  std::unique_ptr<content::WebContents> owned_glance_contents_;

  // Source tab ID for the current glance (from AstraTabFeatures).
  std::string source_tab_id_;

  // Show delay timer (for hover-triggered glances).
  base::OneShotTimer show_delay_timer_;

  // Pending show data (used by the delayed show timer).
  GURL pending_url_;
  raw_ptr<content::WebContents> pending_contents_ = nullptr;
  gfx::Rect pending_anchor_;
  Source pending_source_ = Source::kUnknown;
  bool pending_is_url_mode_ = false;

  // Auto-hide.
  bool auto_hide_on_mouse_leave_ = true;
  base::OneShotTimer auto_hide_timer_;
  base::TimeDelta auto_hide_delay_ = base::Milliseconds(300);

  // -- Presentation state --------------------------------------------------

  // Whether the glance is pinned open (prevents auto-hide).
  bool is_pinned_ = false;

  // Current glance position (which side of the anchor).
  Position position_ = Position::kRight;

  // Cached last-used size (for remember-size feature).
  gfx::Size last_size_;

  // -- New unified API state ---------------------------------------------

  // Index of the tab currently being glanced (-1 if none).
  int tab_index_ = -1;

  // Current content type.
  AstraGlanceContentType content_type_ = AstraGlanceContentType::kTabInfo;

  // Current size preset (AstraGlanceSize enum).
  AstraGlanceSize size_preset_ = AstraGlanceSize::kMedium;

  // Minimum allowed size.
  gfx::Size min_size_{200, 150};

  // Maximum allowed size.
  gfx::Size max_size_{1200, 900};

  // Whether aspect ratio is maintained during resize.
  bool maintain_aspect_ratio_ = false;

  // Anchor point (screen coordinates).
  gfx::Point anchor_position_;

  // Placement relative to anchor.
  AstraGlancePlacement placement_ = AstraGlancePlacement::kAuto;

  // Offset from anchor in pixels.
  int offset_px_ = 8;

  // Trigger mode.
  AstraGlanceTriggerMode trigger_mode_ = AstraGlanceTriggerMode::kHover;

  // Hover delay in milliseconds.
  int hover_delay_ms_ = 500;

  // Dismiss delay in milliseconds.
  int dismiss_delay_ms_ = 300;

  // Whether to show glance on tab hover.
  bool show_on_tab_hover_ = true;

  // Whether to show glance on bookmark hover.
  bool show_on_bookmark_hover_ = true;

  // Whether to show glance on history hover.
  bool show_on_history_hover_ = true;

  // Whether an animation is currently in progress.
  bool is_animating_ = false;

  // Timer for delayed hide.
  base::OneShotTimer hide_delay_timer_;

  // Observers for the new observer pattern (AstraGlanceObserver).
  base::ObserverList<AstraGlanceObserver> glance_observers_;

  // Default show delay for hover-triggered glance.
  static constexpr base::TimeDelta kDefaultShowDelay = base::Milliseconds(500);

  // -- Size constants ------------------------------------------------------
  //
  // These are fallback sizes used when prefs are not available.
  // In production, sizes come from PrefService.

  static constexpr gfx::Size kFallbackCompactSize{420, 280};
  static constexpr gfx::Size kFallbackExpandedSize{560, 420};
  static constexpr gfx::Size kFallbackSmallSize{320, 240};
  static constexpr gfx::Size kFallbackLargeSize{720, 540};
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_GLANCE_ASTRA_GLANCE_VIEW_CONTROLLER_H_
