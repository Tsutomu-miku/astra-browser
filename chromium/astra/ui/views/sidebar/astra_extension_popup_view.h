#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_EXTENSION_POPUP_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_EXTENSION_POPUP_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"

namespace content {
class WebContents;
}  // namespace content

namespace views {
class ImageView;
class Label;
class LabelButton;
class Widget;
}  // namespace views

namespace astra {

// Position of the popup relative to its anchor.
// Mirrors views::BubbleBorder::Arrow but is presentation-facing.
enum class AstraPopupPosition {
  kAuto,   // Automatically choose the best position
  kAbove,  // Popup appears above the anchor
  kBelow,  // Popup appears below the anchor
  kLeft,   // Popup appears to the left of the anchor
  kRight,  // Popup appears to the right of the anchor
};

// Delegate interface for AstraExtensionPopupView events.
// Implemented by the controller (sidebar extensions section) to handle
// popup lifecycle events.
//
// Chromium owner: ExtensionPopup delegate / observer
//   (chrome/browser/ui/views/extensions/extension_popup.h)
class AstraExtensionPopupDelegate {
 public:
  virtual ~AstraExtensionPopupDelegate() = default;

  // Called when the popup widget is about to close.
  // |extension_id| identifies which extension's popup is closing.
  virtual void OnExtensionPopupClosed(const std::string& extension_id) = 0;

  // Called when the popup widget is shown.
  virtual void OnExtensionPopupShown(const std::string& extension_id) = 0;
};

// A bubble that shows an extension's popup content.
//
// This is a presentation-only view — it hosts an extension's HTML popup
// via WebContents. The actual extension popup content is owned by the
// Chromium extension system. This view is just the container/bubble.
//
// Layout:
//   +-----------------------------------+
//   | [icon] Extension Name   [opts][x] |  <- Header (optional)
//   +-----------------------------------+
//   |                                   |
//   |     Extension HTML content        |  <- Content area
//   |                                   |
//   +-----------------------------------+
//
// Similar to Chromium's ExtensionPopup but simplified for the sidebar.
//
// Chromium owner: ExtensionPopup
//   (chrome/browser/ui/views/extensions/extension_popup.h)
// Chromium owner: ExtensionPopupViewViews
//   (chrome/browser/ui/views/extensions/extension_popup_view_views.h)
//
// TODO(astra): Real WebContents-based extension popup view.
//   Currently this is a placeholder bubble with the extension name.
//   The full implementation would:
//     1. Create a WebContents for the popup URL
//     2. Attach it to ExtensionHost / ExtensionPopup
//     3. Host the WebContents in this bubble view
//     4. Handle resize requests from the extension popup
//     5. Handle action badge updates
//   Chromium patch point: ExtensionPopup could be subclassed or wrapped
//   to work in the sidebar context instead of the toolbar context.
class AstraExtensionPopupView : public views::BubbleDialogDelegateView {
 public:
  // Create a popup bubble for |extension_id| anchored to |anchor_view|.
  // |delegate| receives popup lifecycle events. Not owned.
  //
  // The popup is created but not shown — call Show() to display it.
  AstraExtensionPopupView(const std::string& extension_id,
                          const std::u16string& extension_name,
                          views::View* anchor_view,
                          AstraExtensionPopupDelegate* delegate);
  AstraExtensionPopupView(const AstraExtensionPopupView&) = delete;
  AstraExtensionPopupView& operator=(const AstraExtensionPopupView&) = delete;
  ~AstraExtensionPopupView() override;

  // -- Extension identity -------------------------------------------------

  void SetExtensionId(const std::string& extension_id);
  const std::string& GetExtensionId() const;

  // -- Title / header ----------------------------------------------------

  void SetTitle(const std::u16string& title);
  const std::u16string& GetTitle() const;

  void SetIcon(const gfx::ImageSkia& icon);

  void SetShowTitle(bool show);
  bool GetShowTitle() const;

  void SetShowCloseButton(bool show);
  bool GetShowCloseButton() const;

  void SetShowOptionsButton(bool show);
  bool GetShowOptionsButton() const;

  // -- Sizing -------------------------------------------------------------

  void SetContentSize(const gfx::Size& size);
  gfx::Size GetContentSize() const;

  void SetMinSize(const gfx::Size& size);
  gfx::Size GetMinSize() const;

  void SetMaxSize(const gfx::Size& size);
  gfx::Size GetMaxSize() const;

  // -- Behavior -----------------------------------------------------------

  void SetIsResizable(bool resizable);
  bool IsResizable() const;

  void SetDismissOnDeactivate(bool dismiss);
  bool GetDismissOnDeactivate() const;

  // -- Visibility ---------------------------------------------------------

  // Show the popup widget. Returns the Widget* for the popup.
  views::Widget* Show();

  // Hide (close) the popup widget. Safe to call multiple times.
  void Hide();

  // Returns true if the popup widget is currently visible.
  bool IsVisible() const;

  // -- Anchor & position --------------------------------------------------

  void SetAnchorView(views::View* anchor);
  views::View* GetAnchorView() const;

  void SetPopupPosition(AstraPopupPosition position);
  AstraPopupPosition GetPopupPosition() const;

  // -- Header access ------------------------------------------------------

  void SetHeaderVisible(bool visible);
  bool IsHeaderVisible() const;

  views::View* GetHeaderView();

  // -- Content access -----------------------------------------------------

  views::View* GetContentView();

  // -- Extension view hosting --------------------------------------------

  // Set the view that contains the actual extension content (e.g., a
  // WebContents view). The view is reparented into the content area.
  void SetExtensionView(views::View* extension_view);
  views::View* GetExtensionView() const;

  // -- WebContents hosting (placeholder for future implementation) -------

  // Set the WebContents that the popup should host.
  // TODO(astra): Implement real WebContents hosting. This is the main
  //   missing piece for real extension popup support. The WebContents
  //   should be created by ExtensionHost and rendered inside this view.
  // Chromium owner: ExtensionHost (extensions/browser/extension_host.h)
  void SetPopupWebContents(content::WebContents* web_contents);

  // -- Loading state ------------------------------------------------------

  void SetLoading(bool loading);
  bool IsLoading() const;

  // -- Error state --------------------------------------------------------

  void SetErrorState(bool error, const std::u16string& error_message);
  bool HasError() const;
  const std::u16string& GetErrorMessage() const;

  // -- Actions ------------------------------------------------------------

  // Reload the extension popup content.
  void ReloadExtension();

  // Open DevTools for the popup (for debugging).
  void InspectPopup();

  // -- views::BubbleDialogDelegateView -----------------------------------

  void OnWidgetDestroying(views::Widget* widget) override;
  void OnWidgetActivationChanged(views::Widget* widget, bool active) override;

 private:
  // Build the popup content view hierarchy. Called once from constructor.
  void BuildLayout();

  // Update the header visibility based on show_title, show_close_button,
  // and show_options_button settings.
  void UpdateHeaderVisibility();

  // Resize the popup to fit the content size.
  void ResizeToContent();

  // Convert AstraPopupPosition to views::BubbleBorder::Arrow.
  static views::BubbleBorder::Arrow PositionToArrow(AstraPopupPosition pos);

  // Handle close button click.
  void OnCloseButtonPressed();

  // Handle options button click.
  void OnOptionsButtonPressed();

  // The extension ID this popup belongs to.
  std::string extension_id_;

  // The extension display name (shown in the header).
  std::u16string extension_name_;

  // Delegate for lifecycle events. Not owned.
  raw_ptr<AstraExtensionPopupDelegate> delegate_ = nullptr;

  // The WebContents being hosted in this popup. Not owned.
  // TODO(astra): Wire up real WebContents from ExtensionHost.
  raw_ptr<content::WebContents> popup_web_contents_ = nullptr;

  // The extension content view (hosts WebContents or placeholder).
  raw_ptr<views::View> extension_view_ = nullptr;

  // Header child views.
  raw_ptr<views::View> header_view_ = nullptr;
  raw_ptr<views::ImageView> header_icon_ = nullptr;
  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::LabelButton> options_button_ = nullptr;
  raw_ptr<views::LabelButton> close_button_ = nullptr;

  // Content area (hosts the extension view).
  raw_ptr<views::View> content_area_ = nullptr;

  // Current popup position relative to anchor.
  AstraPopupPosition popup_position_ = AstraPopupPosition::kRight;

  // Whether the popup is resizable by the user.
  bool is_resizable_ = false;

  // Whether to show the title in the header.
  bool show_title_ = true;

  // Whether to show the close button in the header.
  bool show_close_button_ = true;

  // Whether to show the options button in the header.
  bool show_options_button_ = false;

  // Whether the popup is currently in a loading state.
  bool is_loading_ = false;

  // Whether the popup is in an error state.
  bool has_error_ = false;
  std::u16string error_message_;

  // Minimum and maximum popup sizes.
  gfx::Size min_size_;
  gfx::Size max_size_;

  // Default popup size (used as fallback when content has no size yet).
  static constexpr int kDefaultPopupWidth = 320;
  static constexpr int kDefaultPopupHeight = 400;

  // Minimum popup size constraints.
  static constexpr int kMinPopupWidth = 25;
  static constexpr int kMinPopupHeight = 25;

  // Maximum popup size constraints.
  static constexpr int kMaxPopupWidth = 800;
  static constexpr int kMaxPopupHeight = 600;

  // Header height in dp.
  static constexpr int kPopupHeaderHeight = 32;

  // Header padding.
  static constexpr int kPopupHeaderPadding = 8;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_EXTENSION_POPUP_VIEW_H_
