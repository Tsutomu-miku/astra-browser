#include "astra/ui/views/sidebar/astra_extension_popup_view.h"

#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/web_contents.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kPopupHeaderHeight = 32;
constexpr int kPopupContentPadding = 8;

}  // namespace

AstraExtensionPopupView::AstraExtensionPopupView(
    const std::string& extension_id,
    const std::u16string& extension_name,
    views::View* anchor_view,
    AstraExtensionPopupDelegate* delegate)
    : views::BubbleDialogDelegateView(anchor_view,
                                      views::BubbleBorder::TOP_RIGHT),
      extension_id_(extension_id),
      extension_name_(extension_name),
      delegate_(delegate) {
  // Bubble configuration.
  set_close_on_deactivate(true);      // Auto-dismiss when focus is lost.
  set_close_on_esc(true);             // Dismiss on Escape key.
  set_margins(gfx::Insets(0));        // No internal margins — content fills.
  set_shadow(views::BubbleBorder::STANDARD_SHADOW);

  // Set the bubble arrow to point at the extension icon.
  // TOP_RIGHT places the bubble to the right of the icon with the arrow
  // on the left side — this is the standard placement for sidebar actions.
  // TODO(astra): Adjust arrow position based on sidebar position (left vs
  //   right side of window). The Astra sidebar is on the left, so the
  //   popup should appear to the right of the sidebar.
  SetArrow(views::BubbleBorder::LEFT_CENTER);

  BuildLayout();
}

AstraExtensionPopupView::~AstraExtensionPopupView() {
  // Widget is already closing at this point. Just clear pointers.
  delegate_ = nullptr;
  popup_web_contents_ = nullptr;
}

// =========================================================================
// Show / close
// =========================================================================

views::Widget* AstraExtensionPopupView::Show() {
  // Create the widget if it doesn't exist yet.
  views::Widget* widget = GetWidget();
  if (!widget) {
    widget = CreateBubble();
  }

  widget->Show();
  return widget;
}

void AstraExtensionPopupView::ClosePopup() {
  views::Widget* widget = GetWidget();
  if (widget && !widget->IsClosed()) {
    widget->Close();
  }
}

// =========================================================================
// WebContents hosting
// =========================================================================

void AstraExtensionPopupView::SetPopupWebContents(
    content::WebContents* web_contents) {
  // TODO(astra): Implement real WebContents hosting in the popup.
  //   This is the key integration point with Chromium's extension popup
  //   system. The proper implementation would:
  //
  //   1. Create an ExtensionHost for the popup URL:
  //      extensions::ExtensionHost* host =
  //          extensions::ExtensionHostRegistry::Get(profile)
  //              ->CreateHost(...);
  //
  //   2. Attach the host's WebContents to this view:
  //      content_area_->AddChildView(host->view());
  //      host->view()->SetBounds(content_area_->GetLocalBounds());
  //
  //   3. Handle resize messages from the popup:
  //      Override OnExtensionSizeChanged() or similar to resize the bubble.
  //
  //   4. Handle popup closing when the extension navigates or calls
  //      window.close().
  //
  // Chromium owner: ExtensionPopup
  //   (chrome/browser/ui/views/extensions/extension_popup.h)
  // Chromium owner: ExtensionHost (extensions/browser/extension_host.h)
  // Chromium patch point: We could either reuse ExtensionPopup by
  //   creating it with our anchor view, or reimplement the hosting logic.
  //   Reusing ExtensionPopup is preferred to stay on Chromium rails.

  popup_web_contents_ = web_contents;
}

// =========================================================================
// Layout
// =========================================================================

void AstraExtensionPopupView::BuildLayout() {
  // Vertical layout: header + content area.
  // TODO(astra): Replace placeholder content with actual WebContents
  //   once extension popup hosting is implemented.

  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // Header: extension name.
  // In a real implementation, the header might not be visible (many
  // extension popups are chromeless), but we include it for the
  // placeholder to make it clear which extension is shown.
  title_label_ = AddChildView(std::make_unique<views::Label>(extension_name_));
  title_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(8, kPopupContentPadding)));
  title_label_->SetAutoColorReadabilityEnabled(false);

  // Content area placeholder.
  // In the real implementation, this hosts the extension popup WebContents.
  content_area_ = AddChildView(std::make_unique<views::View>());
  content_area_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets::VH(0, kPopupContentPadding)));

  // Set preferred size for the placeholder.
  // TODO(astra): Dynamic sizing based on WebContents preferred size.
  //   Extension popups can be any size up to a max (typically ~800x600).
  SetPreferredSize(gfx::Size(kDefaultPopupWidth, kDefaultPopupHeight));
}

void AstraExtensionPopupView::ResizeToContent() {
  // TODO(astra): Resize the popup to fit the WebContents content.
  //   Extension popups can request size changes via JavaScript or CSS.
  //   We should listen for size changes and update the bubble bounds.
  //
  //   Chromium pattern: ExtensionPopup::OnExtensionSizeChanged() handles
  //   this for toolbar action popups.
  //
  //   For now, the popup uses a fixed default size.
}

// =========================================================================
// Widget observer
// =========================================================================

void AstraExtensionPopupView::OnWidgetDestroying(views::Widget* widget) {
  views::BubbleDialogDelegateView::OnWidgetDestroying(widget);

  // Notify the delegate that the popup is closing.
  if (delegate_) {
    delegate_->OnExtensionPopupClosed(extension_id_);
  }
}

void AstraExtensionPopupView::OnWidgetActivationChanged(
    views::Widget* widget,
    bool active) {
  views::BubbleDialogDelegateView::OnWidgetActivationChanged(widget, active);

  // Auto-dismiss on deactivation is handled by set_close_on_deactivate(true).
  // We don't need to do anything extra here.
}

}  // namespace astra
