#ifndef ASTRA_UI_VIEWS_TAB_HOVER_ASTRA_TAB_HOVER_PREVIEW_VIEW_H_
#define ASTRA_UI_VIEWS_TAB_HOVER_ASTRA_TAB_HOVER_PREVIEW_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/widget/widget_observer.h"

#include "astra/ui/views/tab_hover/astra_tab_hover_model.h"

namespace content {
class WebContents;
}

namespace views {
class ImageView;
class Label;
class View;
}  // namespace views

namespace astra {

// =========================================================================
// AstraTabHoverPreviewView — tab hover preview card view
// =========================================================================
//
// A compact hover preview card that shows tab information:
//   - Tab index badge (optional)
//   - Favicon
//   - Page title
//   - URL / domain line
//   - Media indicator (playing / paused / muted)
//   - Mute toggle button (optional)
//   - Close button (top-right corner)
//   - Preview image / thumbnail (configurable size)
//   - Preview image loading indicator
//   - "Peek" / "Expand" action button
//   - Workspace indicator (small accent color dot)
//
// This view renders tab hover state.  It is driven by the controller
// (AstraTabHoverPeekController) which reads state from the model
// (AstraTabHoverModel) and updates the view accordingly.
//
// Architecture:
//   - Model owns truth state (AstraTabHoverModel).
//   - Controller drives view updates from model state.
//   - View renders state and forwards user actions to delegate.
//   - View never reads prefs directly.
//
// For a full live preview, use AstraGlanceView (via peek-to-glance).
//
// Layout:
//   +-----------------------------------------+
//   | [#] [fav] Title            [media] [X]  |  <- header row
//   |     domain.example.com                  |  <- URL/domain line
//   +-----------------------------------------+
//   |                                         |
//   |         [  preview image  ]             |  <- thumbnail area
//   |         [  loading...    ]             |
//   |                                         |
//   +-----------------------------------------+
//   | [workspace dot]            [Peek]      |  <- footer row
//   +-----------------------------------------+
//
// Chromium subsystem reused: views::BubbleDialogDelegateView
//   (ui/views/bubble/bubble_dialog_delegate_view.h).
//
// Chromium equivalent: TabHoverCardView
//   (chrome/browser/ui/views/tabs/tab_hover_card_bubble_view.h).
//
// TODO(astra): Add a real tab thumbnail image.  Chromium's tab hover cards
//   use TabRenderer::CaptureThumbnail or TabThumbnailTracker to get a
//   screenshot of the tab content.  Astra could reuse that system or
//   implement its own thumbnail capture.
//   Chromium owner: TabThumbnailTracker
//   (chrome/browser/ui/tabs/tab_thumbnail_tracker.h).
//   Patch point: could subscribe to existing tab thumbnail updates
//   via TabStripModelObserver or a dedicated thumbnail service.
// =========================================================================

class AstraTabHoverPreviewView : public views::BubbleDialogDelegateView {
 public:
  // Delegate interface for preview view actions.
  class Delegate {
   public:
    // Called when the user clicks the "Peek" button to expand to
    // a full glance view.
    virtual void OnPeekRequested() = 0;

    // Called when the user clicks the close button.
    virtual void OnCloseRequested() = 0;

    // Called when the user clicks the mute toggle button.
    virtual void OnMuteToggled() = 0;

    // Called when the preview widget is destroyed.
    virtual void OnPreviewViewDestroyed() = 0;

   protected:
    ~Delegate() = default;
  };

  // Creates and shows a preview bubble anchored to |anchor_view|.
  // |anchor_rect| is in |anchor_view|'s coordinate system (can be empty).
  // |web_contents| is the tab to show preview info for (may be null).
  // |delegate| receives action callbacks (must outlive the view).
  // The returned Widget is owned by the widget system.
  static views::Widget* ShowBubble(views::View* anchor_view,
                                   const gfx::Rect& anchor_rect,
                                   content::WebContents* web_contents,
                                   Delegate* delegate);

  ~AstraTabHoverPreviewView() override;

  AstraTabHoverPreviewView(const AstraTabHoverPreviewView&) = delete;
  AstraTabHoverPreviewView& operator=(const AstraTabHoverPreviewView&) = delete;

  // -- Content updates ----------------------------------------------------

  // Update the preview from a WebContents.
  void UpdateFromWebContents(content::WebContents* web_contents);

  // Update all content from model state.
  // Convenience method that applies all relevant model fields to the view.
  void UpdateFromModel(const AstraTabHoverModel& model);

  // Set the title text.
  void SetTitleText(const std::u16string& title);

  // Set the URL text.
  void SetUrlText(const std::u16string& url);

  // Set the domain text (shown below the title).
  void SetDomainText(const std::u16string& domain);

  // Set the favicon image.
  void SetFavicon(const gfx::ImageSkia& favicon);

  // Set a placeholder favicon (used when the real favicon isn't available).
  void SetFaviconPlaceholder();

  // Set the thumbnail image (for the large preview area).
  // TODO(astra): Connect to real tab thumbnail capture.
  void SetThumbnail(const gfx::ImageSkia& thumbnail);

  // Set placeholder state for the thumbnail area.
  void SetThumbnailPlaceholder();

  // Set the thumbnail loading state (shows loading indicator).
  void SetThumbnailLoadingState(AstraTabHoverImageLoadingState state);

  // -- Media indicator ----------------------------------------------------

  // Set the media playback state (none, playing, paused, muted).
  // Updates the media indicator icon and visibility.
  void SetMediaState(AstraTabHoverMediaState state);

  // Set whether the media indicator is visible.
  void SetMediaIndicatorVisible(bool visible);

  // Set whether the mute toggle button is visible.
  void SetMuteButtonVisible(bool visible);

  // Set whether the tab is currently muted.
  void SetMuted(bool muted);

  // -- Tab index badge ----------------------------------------------------

  // Set the tab index number (1-based display).
  // Pass -1 to hide the index badge.
  void SetTabIndex(int index);

  // Set whether the tab index badge is visible.
  void SetTabIndexVisible(bool visible);

  // -- Peek / expand ------------------------------------------------------

  // Show or hide the "Peek" button.
  void SetPeekButtonVisible(bool visible);

  // Set the text for the peek/expand action button.
  void SetPeekButtonText(const std::u16string& text);

  // Set peek mode (expanded preview).
  // When in peek mode, the preview shows a larger thumbnail.
  void SetPeekMode(bool is_peeking);

  // -- Workspace indicator ------------------------------------------------

  // Set whether the workspace indicator dot is visible.
  void SetWorkspaceIndicatorVisible(bool visible);

  // Set the workspace indicator color (accent color for the workspace).
  void SetWorkspaceIndicatorColor(SkColor color);

  // -- Close button -------------------------------------------------------

  // Set whether the close button is visible.
  void SetCloseButtonVisible(bool visible);

  // Set whether the title label is visible.
  void SetTitleVisible(bool visible);

  // Set whether the favicon is visible.
  void SetFaviconVisible(bool visible);

  // Set whether the domain/URL line is visible.
  void SetDomainVisible(bool visible);

  // -- Size configuration -------------------------------------------------

  // Show or hide the large thumbnail area.  When hidden, the preview is
  // compact (just title, URL, favicon).  When shown, it includes a large
  // thumbnail preview area.
  void SetThumbnailVisible(bool visible);
  bool thumbnail_visible() const { return thumbnail_visible_; }

  // Set the preview image size (small, medium, large).
  void SetPreviewSize(AstraTabHoverPreviewSize size);
  AstraTabHoverPreviewSize preview_size() const { return preview_size_; }

  // Set the card position preference.
  void SetCardPosition(AstraTabHoverCardPosition position);

  // -- Accessibility ------------------------------------------------------

  // Set the accessible name for the preview.
  void SetAccessibleName(const std::u16string& name);

  // -- Peek expansion animation (stub) ------------------------------------

  // Trigger the peek expansion animation (stub implementation).
  // TODO(astra): Implement real animation using views::Animation or
  //   gfx::Animation / ui::Compositor.
  void StartPeekExpandAnimation();

  // -- BubbleDialogDelegateView overrides ---------------------------------

  void OnWidgetDestroying(views::Widget* widget) override;
  bool AcceleratorPressed(const ui::Accelerator& accelerator) override;
  void OnThemeChanged() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

  // -- Size constants -----------------------------------------------------

  // Default size in compact mode (no thumbnail).
  static constexpr gfx::Size kCompactSize{340, 72};

  // Default size in expanded mode (with thumbnail, medium size).
  static constexpr gfx::Size kExpandedSize{340, 220};

  // Thumbnail area sizes by preset.
  static constexpr gfx::Size kSmallThumbnailSize{200, 100};
  static constexpr gfx::Size kMediumThumbnailSize{316, 128};
  static constexpr gfx::Size kLargeThumbnailSize{400, 220};

  // Favicon size.
  static constexpr int kFaviconSize = 16;

  // Tab index badge size.
  static constexpr int kTabIndexBadgeSize = 18;

  // Media indicator size.
  static constexpr int kMediaIndicatorSize = 16;

  // Mute button size.
  static constexpr int kMuteButtonSize = 20;

  // Workspace indicator dot size.
  static constexpr int kWorkspaceDotSize = 8;

  // Close button size.
  static constexpr int kCloseButtonSize = 20;

  // Peek/expand button size.
  static constexpr int kPeekButtonHeight = 28;
  static constexpr int kPeekButtonWidth = 72;

  // Padding around the content.
  static constexpr int kHorizontalPadding = 12;
  static constexpr int kVerticalPadding = 10;

  // Spacing between header and thumbnail.
  static constexpr int kHeaderToThumbnailSpacing = 8;

  // Spacing between thumbnail and footer.
  static constexpr int kThumbnailToFooterSpacing = 8;

  // Spacing between elements in the header row.
  static constexpr int kHeaderElementSpacing = 6;

 private:
  AstraTabHoverPreviewView(views::View* anchor_view,
                           const gfx::Rect& anchor_rect,
                           content::WebContents* web_contents,
                           Delegate* delegate);

  // Build the preview view contents.
  void Init() override;

  // Build the header row: tab index + favicon + text + media + mute + close.
  void BuildHeaderRow();

  // Build the text area (title + domain) inside the header.
  void BuildTextArea(views::View* text_container);

  // Build the tab index badge.
  void BuildTabIndexBadge();

  // Build the media indicator icon.
  void BuildMediaIndicator();

  // Build the mute toggle button.
  void BuildMuteButton();

  // Build the thumbnail area.
  void BuildThumbnailArea();

  // Build the footer row: workspace dot + peek button.
  void BuildFooterRow();

  // Build the workspace indicator dot.
  void BuildWorkspaceDot();

  // Build the peek/expand action button.
  void BuildPeekButton();

  // Build the close button.
  void BuildCloseButton();

  // Button handler: "Peek" button clicked.
  void OnPeekButtonPressed();

  // Button handler: close button clicked.
  void OnCloseButtonPressed();

  // Button handler: mute button clicked.
  void OnMuteButtonPressed();

  // Update the widget size based on thumbnail visibility and size.
  void UpdateWidgetSize();

  // Update the thumbnail container size based on preview_size_.
  void UpdateThumbnailSize();

  // Update arrow position based on card position setting.
  void UpdateArrowPosition();

  // Update colors from theme/color provider.
  void UpdateColors();

  // The WebContents this preview is for (not owned).
  raw_ptr<content::WebContents> web_contents_ = nullptr;

  // Delegate for action callbacks (not owned).
  raw_ptr<Delegate> delegate_ = nullptr;

  // Whether the large thumbnail area is visible.
  bool thumbnail_visible_ = false;

  // Current preview image size preset.
  AstraTabHoverPreviewSize preview_size_ = AstraTabHoverPreviewSize::kMedium;

  // Current card position preference.
  AstraTabHoverCardPosition card_position_ = AstraTabHoverCardPosition::kAuto;

  // Whether we're in peek mode (expanded preview).
  bool is_peek_mode_ = false;

  // Current media state.
  AstraTabHoverMediaState media_state_ = AstraTabHoverMediaState::kNone;

  // Whether the tab is muted.
  bool is_muted_ = false;

  // Current loading state for the thumbnail.
  AstraTabHoverImageLoadingState thumbnail_loading_state_ =
      AstraTabHoverImageLoadingState::kNotLoaded;

  // Accessible name for the preview bubble.
  std::u16string accessible_name_;

  // Child views (owned by the view hierarchy).
  raw_ptr<views::View> tab_index_badge_ = nullptr;
  raw_ptr<views::Label> tab_index_label_ = nullptr;
  raw_ptr<views::ImageView> favicon_view_ = nullptr;
  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> domain_label_ = nullptr;
  raw_ptr<views::ImageView> media_indicator_ = nullptr;
  raw_ptr<views::ImageButton> mute_button_ = nullptr;
  raw_ptr<views::ImageButton> close_button_ = nullptr;
  raw_ptr<views::LabelButton> peek_button_ = nullptr;
  raw_ptr<views::View> workspace_dot_ = nullptr;
  raw_ptr<views::ImageView> thumbnail_view_ = nullptr;
  raw_ptr<views::View> thumbnail_loading_indicator_ = nullptr;

  // Container views.
  raw_ptr<views::View> header_row_ = nullptr;
  raw_ptr<views::View> text_container_ = nullptr;
  raw_ptr<views::View> thumbnail_container_ = nullptr;
  raw_ptr<views::View> footer_row_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_TAB_HOVER_ASTRA_TAB_HOVER_PREVIEW_VIEW_H_
