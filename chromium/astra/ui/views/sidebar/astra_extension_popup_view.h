#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_EXTENSION_POPUP_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_EXTENSION_POPUP_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"

namespace content {
class WebContents;
}  // namespace content

namespace views {
class Label;
class Widget;
}  // namespace views

namespace astra {

// Delegate interface for AstraExtensionPopupView events.
// Implemented by the controller (sidebar extensions section) to handle
// popup lifecycle events.
class AstraExtensionPopupDelegate {
 public:
  virtual ~AstraExtensionPopupDelegate() = default;

  // Called when the popup widget is about to close.
  // |extension_id| identifies which extension's popup is closing.
  virtual void OnExtensionPopupClosed(const std::string& extension_id) = 0;
};

// A bubble that shows an extension's popup content.
//
// This is a presentation-only view — it hosts an extension's HTML popup
// via WebContents. The actual extension popup content is owned by the
// Chromium extension system. This view is just the container/bubble.
//
// Similar to Chromium's ExtensionPopup but simplified for the sidebar.
//
// The popup is anchored to the extension icon in the sidebar and auto-
// dismisses when the user clicks outside or switches tabs.
//
// Chromium owner: ExtensionPopup
//   (chrome/browser/ui/views/extensions/extension_popup.h)
// Chromium owner: ExtensionPopupViewViews
//   (chrome/browser/ui/views/extensions/extension_popup_view_views.h)
// Chromium owner: ExtensionActionPlatformView
//   (chrome/browser/ui/views/extensions/extension_action_platform_view.h)
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

  // Show the popup widget. Returns the Widget* for the popup.
  views::Widget* Show();

  // Close the popup widget. Safe to call multiple times.
  void ClosePopup();

  // -- WebContents hosting (placeholder for future implementation) -------

  // Set the WebContents that the popup should host.
  // TODO(astra): Implement real WebContents hosting. This is the main
  //   missing piece for real extension popup support. The WebContents
  //   should be created by ExtensionHost and rendered inside this view.
  // Chromium owner: ExtensionHost (extensions/browser/extension_host.h)
  // Chromium owner: ExtensionPopup
  //   (chrome/browser/ui/views/extensions/extension_popup.h)
  void SetPopupWebContents(content::WebContents* web_contents);

  // Get the extension ID this popup belongs to.
  const std::string& extension_id() const { return extension_id_; }

  // -- views::BubbleDialogDelegateView -----------------------------------

  void OnWidgetDestroying(views::Widget* widget) override;
  void OnWidgetActivationChanged(views::Widget* widget, bool active) override;

 private:
  // Build the popup content view hierarchy.
  void BuildLayout();

  // Resize the popup to fit the WebContents preferred size.
  // TODO(astra): Implement proper resize logic based on WebContents
  //   size requests, similar to ExtensionPopup::OnExtensionSizeChanged.
  void ResizeToContent();

  // The extension ID this popup belongs to.
  std::string extension_id_;

  // The extension display name (for placeholder content).
  std::u16string extension_name_;

  // Delegate for lifecycle events. Not owned.
  raw_ptr<AstraExtensionPopupDelegate> delegate_ = nullptr;

  // The WebContents being hosted in this popup. Not owned.
  // TODO(astra): Wire up real WebContents from ExtensionHost.
  raw_ptr<content::WebContents> popup_web_contents_ = nullptr;

  // Placeholder content views.
  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::View> content_area_ = nullptr;

  // Default popup size (used as fallback when WebContents has no size yet).
  static constexpr int kDefaultPopupWidth = 320;
  static constexpr int kDefaultPopupHeight = 400;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_EXTENSION_POPUP_VIEW_H_
