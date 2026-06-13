#ifndef ASTRA_UI_VIEWS_GLANCE_ASTRA_GLANCE_VIEW_H_
#define ASTRA_UI_VIEWS_GLANCE_ASTRA_GLANCE_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/widget/widget_observer.h"
#include "url/gurl.h"

namespace content {
class WebContents;
}

namespace views {
class ImageButton;
class ImageView;
class Label;
class Throbber;
class View;
}  // namespace views

namespace astra {

// =========================================================================
// AstraGlanceContentType — types of content a glance can display
// =========================================================================
//
// The glance view can show different kinds of previews depending on what
// the user is hovering over.  The content type controls which visual
// elements are visible and how the preview area is populated.
//
// TODO(astra): Consider reusing Chromium's tab hover card as the base for
//   tab preview content.  Chromium owner: TabHoverCardBubbleView
//   (chrome/browser/ui/views/tabs/tab_hover_card_bubble_view.h).
enum class AstraGlanceContentType {
  kTabPreview,    // Mini WebContents preview (live tab content).
  kTabInfo,       // Title, URL, domain, favicon (no live preview).
  kScreenshot,    // Cached thumbnail/screenshot of a tab.
  kReadingList,   // Reading list entry preview with title/URL/snippet.
  kNote,          // Note attached to a tab.
  kBookmark,      // Bookmark info: title, URL, folder, date added.
};

// =========================================================================
// AstraGlanceView — the glance / peek preview bubble
// =========================================================================
//
// A rich preview bubble that shows a Chromium WebContents in a small overlay.
// Used for "peeking" at a tab (from the sidebar) or a URL (from a link hover)
// without navigating away from the current tab.
//
// The glance view is implemented as a views::BubbleDialogDelegateView, which
// gives us rounded corners, shadow, auto-dismiss on deactivation, and anchor
// positioning — all standard Chromium bubble patterns.
//
// Layout (top to bottom):
//   1. Header bar — favicon + site title + URL + close button.
//   2. Preview area — WebContents view or loading/error placeholder.
//   3. Status bar — security indicator + loading state (thin bar).
//   4. Action bar — action buttons (open in tab, favorite, copy, pin, close).
//   5. Resize handle — bottom-right corner drag handle.
//
// Display modes:
//   - Compact: smaller size, header + preview only, no action bar.
//   - Expanded: full size, all sections visible.
//
// States:
//   - Loading: throbber + "Loading..." text shown in preview area.
//   - Loaded: WebContents visible.
//   - Error: error message shown in preview area.
//
// Chromium subsystem reused: views::BubbleDialogDelegateView
//   (ui/views/bubble/bubble_dialog_delegate_view.h).
//
// TODO(astra): The WebContents view embedding is currently schematic.
//   The actual embedding depends on how Chromium exposes WebContentsView:
//
//   Option A: views::WebView — if Chrome has a views wrapper for WebContents.
//     chrome/browser/ui/views/tab_contents/web_contents_view_views.h
//
//   Option B: Native view host — wrap WebContents::GetNativeView() in a
//     views::NativeViewHost and add it to the view hierarchy.
//
//   Option C: Direct WebContentsView access — web_contents->GetView() returns
//     a content::WebContentsView*, which on desktop Views has a GetView()
//     method returning a views::View*.
//
//   Reference pattern: Chrome's side panel (chrome/browser/ui/views/side_panel)
//   hosts WebContents for side panel pages — we should follow that pattern.
//   Chromium owner: SidePanelContentView (chrome/browser/ui/views/side_panel/).
//
//   Patch point: May need a small patch to expose WebContentsView's views
//   representation, or to use views::WebView as the embedding layer.
// =========================================================================

class AstraGlanceView : public views::BubbleDialogDelegateView {
 public:
  // Delegate interface for glance view events.  The controller implements
  // this to receive user actions from the view.
  //
  // View -> Controller communication.
  class Delegate {
   public:
    // Called when the user clicks the close button or presses Escape.
    virtual void OnGlanceCloseRequested() = 0;

    // Called when the user clicks "Open in new tab" — promote the glance
    // WebContents to a full tab in TabStripModel.
    virtual void OnGlancePromoteToTab() = 0;

    // Called when the glance widget is destroyed.  The delegate should
    // clean up its raw pointers to the view.
    virtual void OnGlanceViewDestroyed() = 0;

    // Called when the user clicks "Add to favorites".
    virtual void OnGlanceAddToFavorites() = 0;

    // Called when the user clicks "Copy URL".
    virtual void OnGlanceCopyURL() = 0;

    // Called when the user clicks "Pin tab".
    virtual void OnGlancePinTab() = 0;

    // Called when the user clicks "Close tab" (closes the underlying tab).
    virtual void OnGlanceCloseTab() = 0;

    // Called when the user toggles expanded mode (e.g. via Ctrl+Enter or
    // the expand button).
    virtual void OnGlanceToggleExpanded() = 0;

    // Called when the user toggles the glance pinned state (pin window open).
    virtual void OnGlanceTogglePinned() = 0;

    // Called when the user changes the size preset.
    virtual void OnGlanceSizePresetChanged(
        AstraGlanceView::SizePreset preset) = 0;

    // Called when the user clicks the settings button.
    virtual void OnGlanceSettingsRequested() = 0;

    // Called when the glance is resized (via drag handle).
    virtual void OnGlanceResized(const gfx::Size& new_size) = 0;

    // Called when the user cycles the glance position.
    virtual void OnGlanceCyclePosition() = 0;

   protected:
    ~Delegate() = default;
  };

  // Loading state for the preview content.
  enum class LoadingState {
    kLoading,  // Content is loading; show throbber + placeholder.
    kLoaded,   // Content is loaded; show WebContents.
    kError,    // Content failed to load; show error message.
  };

  // Security state for the current URL.
  enum class SecurityState {
    kSecure,    // HTTPS with valid certificate.
    kNonSecure, // HTTP or other non-secure scheme.
    kUnknown,   // Security state not yet determined.
  };

  // Display mode controls which sections of the glance are visible and
  // the overall size.
  enum class DisplayMode {
    kCompact,   // Small size: header + preview only.
    kExpanded,  // Full size: all sections visible.
  };

  // Source of the glance request (used for analytics and positioning).
  enum class Source {
    kSidebarHover,  // Triggered by hovering over a sidebar item.
    kLinkHover,     // Triggered by hovering over a link.
    kKeyboard,      // Triggered by keyboard shortcut.
    kUnknown,       // Source not specified.
  };

  // Size preset for quick resizing.
  enum class SizePreset {
    kSmall,
    kMedium,
    kLarge,
  };

  // Creates and shows a glance bubble anchored to |anchor_view|.
  // |anchor_rect| is in |anchor_view|'s coordinate system; pass an empty
  // rect to anchor to the view's bounds.
  // The returned Widget is owned by the widget system.
  static views::Widget* ShowBubble(views::View* anchor_view,
                                   const gfx::Rect& anchor_rect,
                                   Delegate* delegate);

  ~AstraGlanceView() override;

  AstraGlanceView(const AstraGlanceView&) = delete;
  AstraGlanceView& operator=(const AstraGlanceView&) = delete;

  // -- WebContents embedding ----------------------------------------------

  // Set the WebContents to display in the glance preview area.
  // Pass nullptr to clear and show a placeholder.
  void SetWebContents(content::WebContents* web_contents);

  content::WebContents* web_contents() { return web_contents_; }
  const content::WebContents* web_contents() const { return web_contents_; }

  // -- Content metadata ---------------------------------------------------

  // Set the page title displayed in the header.
  void SetTitleText(const std::u16string& title);

  // Set the URL displayed in the header (below the title).
  void SetURLText(const std::u16string& url);

  // Set the favicon displayed in the header.
  // TODO(astra): Use gfx::Image or gfx::ImageSkia; figure out the right
  //   Chromium favicon API.  Chromium owner: FaviconService
  //   (chrome/browser/favicon/favicon_service.h).
  void SetFavicon(const gfx::ImageSkia& icon);

  // -- Unified content API -------------------------------------------------
  //
  // Higher-level API for setting glance content.  These methods provide a
  // simpler interface for the controller to update the view's content.
  // The view handles layout and rendering details internally.

  // Set all glance content at once.  Updates title, URL, content type,
  // and preview image.
  void SetContent(AstraGlanceContentType type,
                  const std::u16string& title,
                  const GURL& url,
                  const gfx::ImageSkia& preview_image);

  // Set the glance title.
  void SetTitle(const std::u16string& title);
  std::u16string GetTitle() const;

  // Set the glance URL.  The URL is displayed in the header and status bar.
  void SetUrl(const GURL& url);
  GURL GetUrl() const;

  // Set a preview image (screenshot/thumbnail) for the preview area.
  void SetPreviewImage(const gfx::ImageSkia& image);
  void ClearPreviewImage();

  // Set the content type, which controls which UI elements are visible.
  void SetContentType(AstraGlanceContentType type);
  AstraGlanceContentType GetContentType() const;

  // -- Pin state (glance pinned open) -------------------------------------

  // Set whether the glance is pinned open (stays visible, no auto-dismiss).
  void SetPinned(bool pinned);
  bool IsPinned() const;

  // -- Expanded state -----------------------------------------------------

  // Set whether the glance is in expanded (full-size) mode.
  void SetExpanded(bool expanded);
  bool IsExpanded() const;

  // -- Action button visibility -------------------------------------------

  // Show or hide individual action buttons in the header or action bar.
  void SetShowCloseButton(bool show);
  void SetShowPinButton(bool show);
  void SetShowOpenButton(bool show);

  // -- Domain text --------------------------------------------------------

  // Set the domain text displayed below the title (e.g., "example.com").
  void SetDomainText(const std::u16string& domain);

  // -- Loading state (boolean API) ----------------------------------------

  // Simplified boolean loading API.  SetLoading(true) shows the throbber;
  // SetLoading(false) shows the loaded content.
  void SetLoading(bool is_loading);
  bool IsLoading() const;

  // -- Loading state ------------------------------------------------------

  // Update the loading state.  This controls whether the throbber,
  // WebContents, or error message is shown in the preview area.
  void SetLoadingState(LoadingState state);
  LoadingState loading_state() const { return loading_state_; }

  // Set the error message shown in the error state.
  void SetErrorMessage(const std::u16string& message);

  // -- Security state -----------------------------------------------------

  // Update the security indicator shown in the status bar.
  void SetSecurityState(SecurityState state);
  SecurityState security_state() const { return security_state_; }

  // -- Display mode -------------------------------------------------------

  // Switch between compact and expanded display modes.
  void SetDisplayMode(DisplayMode mode);
  DisplayMode display_mode() const { return display_mode_; }
  void ToggleDisplayMode();

  // -- Action button states -----------------------------------------------

  // Update the "Add to favorites" button toggle state.
  void SetIsFavorite(bool is_favorite);

  // Update the "Pin tab" button toggle state.
  void SetIsPinned(bool is_pinned);

  // Enable or disable individual action buttons.
  void SetActionButtonEnabled(/* action type */ bool open_in_tab_enabled,
                              bool favorite_enabled,
                              bool copy_url_enabled,
                              bool pin_tab_enabled,
                              bool close_tab_enabled);

  // -- Glance pinning (window pin) -----------------------------------------

  // Set whether the glance is pinned open (window stays visible).
  void SetGlancePinned(bool pinned);
  bool IsGlancePinned() const { return glance_pinned_; }

  // -- Size preset ----------------------------------------------------------

  // Apply a size preset.  Updates the widget size.
  void ApplySizePreset(SizePreset preset);

  // Get the current size preset (closest match to current size).
  SizePreset GetCurrentSizePreset() const;

  // -- Settings button -----------------------------------------------------

  // Show or hide the settings button in the header.
  void SetSettingsButtonVisible(bool visible);
  bool IsSettingsButtonVisible() const { return settings_button_visible_; }

  // -- Status bar visibility ------------------------------------------------

  void SetStatusBarVisible(bool visible);
  bool IsStatusBarVisible() const;

  // -- Action bar visibility ------------------------------------------------

  void SetActionBarVisible(bool visible);
  bool IsActionBarVisible() const;

  // -- Resize handle visibility ---------------------------------------------

  void SetResizeHandleVisible(bool visible);
  bool IsResizeHandleVisible() const;

  // -- Animation stubs ----------------------------------------------------

  // Play the entrance animation (fade + scale in).
  // TODO(astra): Implement real animation using views::Animation or
  //   gfx::Animation.  Chromium pattern: views::SlideAnimation.
  void PlayEntranceAnimation();

  // Play the exit animation (fade + scale out).  Calls |on_done| when
  // the animation completes.
  // TODO(astra): Implement real animation.  See PlayEntranceAnimation().
  void PlayExitAnimation(base::OnceClosure on_done);

  // -- Size constants -----------------------------------------------------

  // Default sizes in DIPs for each display mode.
  static constexpr gfx::Size kCompactSize{420, 280};
  static constexpr gfx::Size kExpandedSize{560, 420};

  // Size presets.
  static constexpr gfx::Size kSmallSize{320, 240};
  static constexpr gfx::Size kMediumSize{560, 420};
  static constexpr gfx::Size kLargeSize{720, 540};

  // Heights of each section in DIPs.
  static constexpr int kHeaderHeight = 48;
  static constexpr int kStatusBarHeight = 24;
  static constexpr int kActionBarHeight = 44;
  static constexpr int kResizeHandleSize = 16;

  // -- BubbleDialogDelegateView overrides ---------------------------------

  void OnWidgetDestroying(views::Widget* widget) override;
  void OnWidgetActivationChanged(views::Widget* widget, bool active) override;
  bool AcceleratorPressed(const ui::Accelerator& accelerator) override;
  gfx::Size CalculatePreferredSize() const override;
  void OnThemeChanged() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

 private:
  AstraGlanceView(views::View* anchor_view,
                  const gfx::Rect& anchor_rect,
                  Delegate* delegate);

  // Build the glance view contents (all child views).
  void Init() override;

  // -- Build methods ------------------------------------------------------

  // Build the header bar: favicon, title + URL (stacked), close button.
  void BuildHeader();

  // Build the main preview area that hosts the WebContents or state views.
  void BuildPreviewArea();

  // Build the status bar: security indicator + status text.
  void BuildStatusBar();

  // Build the action bar with action buttons.
  void BuildActionBar();

  // Build the bottom-right resize handle.
  void BuildResizeHandle();

  // Build the loading state overlay (throbber + label).
  void BuildLoadingView();

  // Build the error state overlay (icon + message).
  void BuildErrorView();

  // Build the preview image view (for screenshot/thumbnail content).
  void BuildPreviewImageView();

  // -- WebContents embedding ----------------------------------------------

  // Attach the WebContents view to the preview area.
  // TODO(astra): Implement actual WebContents view embedding.
  //   See class-level TODO(astra) for the embedding strategy.
  void AttachWebContentsView();

  // Detach the WebContents view from the preview area.
  void DetachWebContentsView();

  // -- Button handlers ----------------------------------------------------

  void OnCloseButtonPressed();
  void OnPromoteButtonPressed();
  void OnFavoriteButtonPressed();
  void OnCopyURLButtonPressed();
  void OnPinTabButtonPressed();
  void OnCloseTabButtonPressed();
  void OnExpandButtonPressed();
  void OnPinButtonPressed();
  void OnSettingsButtonPressed();
  void OnPositionButtonPressed();
  void OnSizeSmallButtonPressed();
  void OnSizeMediumButtonPressed();
  void OnSizeLargeButtonPressed();

  // -- Resize handling ----------------------------------------------------

  // Resize handle mouse events.
  bool OnResizeHandleMousePressed(const ui::MouseEvent& event);
  bool OnResizeHandleMouseDragged(const ui::MouseEvent& event);
  void OnResizeHandleMouseReleased(const ui::MouseEvent& event);

  // -- State update helpers ----------------------------------------------

  // Update the visibility of state subviews (loading, error, web content)
  // based on the current loading state.
  void UpdateStateViews();

  // Update colors from the color provider.
  void UpdateColors();

  // Update layout visibility for the current display mode.
  void UpdateLayoutForDisplayMode();

  // Update action button tooltips and accessible names.
  void UpdateActionButtonLabels();

  raw_ptr<Delegate> delegate_ = nullptr;

  // The WebContents being previewed (not owned by this view).
  // Ownership depends on glance mode:
  //   - Tab glance: owned by TabStripModel.
  //   - URL glance: owned by the glance controller (temporary WebContents).
  raw_ptr<content::WebContents> web_contents_ = nullptr;

  // -- Child views (owned by the view hierarchy) -------------------------

  // Header bar.
  raw_ptr<views::View> header_ = nullptr;
  raw_ptr<views::ImageView> favicon_view_ = nullptr;
  raw_ptr<views::View> header_text_container_ = nullptr;
  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> url_label_ = nullptr;
  raw_ptr<views::ImageButton> close_button_ = nullptr;

  // Preview area (hosts WebContents or state overlays).
  raw_ptr<views::View> preview_area_ = nullptr;

  // Loading state overlay.
  raw_ptr<views::View> loading_view_ = nullptr;
  raw_ptr<views::Throbber> loading_throbber_ = nullptr;
  raw_ptr<views::Label> loading_label_ = nullptr;

  // Error state overlay.
  raw_ptr<views::View> error_view_ = nullptr;
  raw_ptr<views::Label> error_label_ = nullptr;

  // Status bar.
  raw_ptr<views::View> status_bar_ = nullptr;
  raw_ptr<views::ImageView> security_icon_ = nullptr;
  raw_ptr<views::Label> status_label_ = nullptr;

  // Action bar.
  raw_ptr<views::View> action_bar_ = nullptr;
  raw_ptr<views::ImageButton> promote_action_button_ = nullptr;
  raw_ptr<views::ImageButton> favorite_button_ = nullptr;
  raw_ptr<views::ImageButton> copy_url_button_ = nullptr;
  raw_ptr<views::ImageButton> pin_tab_button_ = nullptr;
  raw_ptr<views::ImageButton> close_tab_button_ = nullptr;

  // Resize handle.
  raw_ptr<views::View> resize_handle_ = nullptr;

  // Header: size preset buttons (small, medium, large).
  raw_ptr<views::View> size_preset_container_ = nullptr;
  raw_ptr<views::ImageButton> size_small_button_ = nullptr;
  raw_ptr<views::ImageButton> size_medium_button_ = nullptr;
  raw_ptr<views::ImageButton> size_large_button_ = nullptr;

  // Header: pin button (pin glance open).
  raw_ptr<views::ImageButton> pin_button_ = nullptr;

  // Header: settings menu button.
  raw_ptr<views::ImageButton> settings_button_ = nullptr;

  // Header: position cycle button.
  raw_ptr<views::ImageButton> position_button_ = nullptr;

  // Header: expand/collapse button.
  raw_ptr<views::ImageButton> expand_button_ = nullptr;

  // -- State --------------------------------------------------------------

  LoadingState loading_state_ = LoadingState::kLoaded;
  SecurityState security_state_ = SecurityState::kUnknown;
  DisplayMode display_mode_ = DisplayMode::kExpanded;

  // Action button toggle states.
  bool is_favorite_ = false;
  bool is_pinned_ = false;

  // Glance window pin state (pinned open).
  bool glance_pinned_ = false;

  // Settings button visibility.
  bool settings_button_visible_ = true;

  // Resize interaction state.
  bool is_resizing_ = false;
  gfx::Point resize_start_mouse_;
  gfx::Size resize_start_size_;

  // Error message text.
  std::u16string error_message_;

  // -- New unified API state ---------------------------------------------

  // Current content type.
  AstraGlanceContentType content_type_ = AstraGlanceContentType::kTabInfo;

  // Preview image view (for screenshot/thumbnail content type).
  raw_ptr<views::ImageView> preview_image_view_ = nullptr;

  // Stored URL (for GetUrl()).
  GURL url_;

  // Domain text (displayed below title).
  std::u16string domain_text_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_GLANCE_ASTRA_GLANCE_VIEW_H_
