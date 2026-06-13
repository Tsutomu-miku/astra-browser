#include "astra/ui/views/sidebar/astra_extension_popup_view.h"

#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/web_contents.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace astra {

namespace {

// Content area padding inside the popup bubble.
constexpr int kPopupContentPadding = 0;

}  // namespace

AstraExtensionPopupView::AstraExtensionPopupView(
    const std::string& extension_id,
    const std::u16string& extension_name,
    views::View* anchor_view,
    AstraExtensionPopupDelegate* delegate)
    : views::BubbleDialogDelegateView(anchor_view,
                                      views::BubbleBorder::LEFT_CENTER),
      extension_id_(extension_id),
      extension_name_(extension_name),
      delegate_(delegate) {
  // Bubble configuration.
  set_close_on_deactivate(true);      // Auto-dismiss when focus is lost.
  set_close_on_esc(true);             // Dismiss on Escape key.
  set_margins(gfx::Insets(0));        // No internal margins — content fills.
  set_shadow(views::BubbleBorder::STANDARD_SHADOW);

  // Default size constraints.
  min_size_ = gfx::Size(kMinPopupWidth, kMinPopupHeight);
  max_size_ = gfx::Size(kMaxPopupWidth, kMaxPopupHeight);

  BuildLayout();

  // Set preferred size to default.
  SetPreferredSize(gfx::Size(kDefaultPopupWidth, kDefaultPopupHeight));
}

AstraExtensionPopupView::~AstraExtensionPopupView() {
  // Widget is already closing at this point. Just clear pointers.
  delegate_ = nullptr;
  popup_web_contents_ = nullptr;
  extension_view_ = nullptr;
}

// =========================================================================
// Extension identity
// =========================================================================

void AstraExtensionPopupView::SetExtensionId(const std::string& extension_id) {
  extension_id_ = extension_id;
}

const std::string& AstraExtensionPopupView::GetExtensionId() const {
  return extension_id_;
}

// =========================================================================
// Title / header
// =========================================================================

void AstraExtensionPopupView::SetTitle(const std::u16string& title) {
  extension_name_ = title;
  if (title_label_) {
    title_label_->SetText(title);
  }
}

const std::u16string& AstraExtensionPopupView::GetTitle() const {
  return extension_name_;
}

void AstraExtensionPopupView::SetIcon(const gfx::ImageSkia& icon) {
  if (header_icon_) {
    header_icon_->SetImage(icon);
  }
}

void AstraExtensionPopupView::SetShowTitle(bool show) {
  if (show_title_ == show) {
    return;
  }
  show_title_ = show;
  UpdateHeaderVisibility();
}

bool AstraExtensionPopupView::GetShowTitle() const {
  return show_title_;
}

void AstraExtensionPopupView::SetShowCloseButton(bool show) {
  if (show_close_button_ == show) {
    return;
  }
  show_close_button_ = show;
  if (close_button_) {
    close_button_->SetVisible(show);
  }
  UpdateHeaderVisibility();
}

bool AstraExtensionPopupView::GetShowCloseButton() const {
  return show_close_button_;
}

void AstraExtensionPopupView::SetShowOptionsButton(bool show) {
  if (show_options_button_ == show) {
    return;
  }
  show_options_button_ = show;
  if (options_button_) {
    options_button_->SetVisible(show);
  }
  UpdateHeaderVisibility();
}

bool AstraExtensionPopupView::GetShowOptionsButton() const {
  return show_options_button_;
}

// =========================================================================
// Sizing
// =========================================================================

void AstraExtensionPopupView::SetContentSize(const gfx::Size& size) {
  if (!content_area_) {
    return;
  }

  // Clamp to min/max constraints.
  int width = std::clamp(size.width(), min_size_.width(), max_size_.width());
  int height =
      std::clamp(size.height(), min_size_.height(), max_size_.height());

  content_area_->SetPreferredSize(gfx::Size(width, height));
  ResizeToContent();
}

gfx::Size AstraExtensionPopupView::GetContentSize() const {
  if (!content_area_) {
    return gfx::Size();
  }
  return content_area_->GetPreferredSize();
}

void AstraExtensionPopupView::SetMinSize(const gfx::Size& size) {
  min_size_ = size;
  // Ensure current size respects new minimum.
  if (content_area_) {
    gfx::Size current = content_area_->GetPreferredSize();
    if (current.width() < min_size_.width() ||
        current.height() < min_size_.height()) {
      SetContentSize(current);  // Will be clamped by SetContentSize.
    }
  }
}

gfx::Size AstraExtensionPopupView::GetMinSize() const {
  return min_size_;
}

void AstraExtensionPopupView::SetMaxSize(const gfx::Size& size) {
  max_size_ = size;
  // Ensure current size respects new maximum.
  if (content_area_) {
    gfx::Size current = content_area_->GetPreferredSize();
    if (current.width() > max_size_.width() ||
        current.height() > max_size_.height()) {
      SetContentSize(current);  // Will be clamped by SetContentSize.
    }
  }
}

gfx::Size AstraExtensionPopupView::GetMaxSize() const {
  return max_size_;
}

// =========================================================================
// Behavior
// =========================================================================

void AstraExtensionPopupView::SetIsResizable(bool resizable) {
  is_resizable_ = resizable;
  // TODO(astra): Implement resizable popup borders.
  //   Chromium pattern: Widget::SetCanResize or custom resize handles.
}

bool AstraExtensionPopupView::IsResizable() const {
  return is_resizable_;
}

void AstraExtensionPopupView::SetDismissOnDeactivate(bool dismiss) {
  set_close_on_deactivate(dismiss);
}

bool AstraExtensionPopupView::GetDismissOnDeactivate() const {
  return close_on_deactivate();
}

// =========================================================================
// Visibility
// =========================================================================

views::Widget* AstraExtensionPopupView::Show() {
  // Create the widget if it doesn't exist yet.
  views::Widget* widget = GetWidget();
  if (!widget) {
    widget = CreateBubble();
  }

  widget->Show();

  // Notify delegate that the popup was shown.
  if (delegate_) {
    delegate_->OnExtensionPopupShown(extension_id_);
  }

  return widget;
}

void AstraExtensionPopupView::Hide() {
  views::Widget* widget = GetWidget();
  if (widget && !widget->IsClosed()) {
    widget->Close();
  }
}

bool AstraExtensionPopupView::IsVisible() const {
  const views::Widget* widget = GetWidget();
  return widget && widget->IsVisible();
}

// =========================================================================
// Anchor & position
// =========================================================================

void AstraExtensionPopupView::SetAnchorView(views::View* anchor) {
  if (views::BubbleDialogDelegateView::GetAnchorView() == anchor) {
    return;
  }
  views::BubbleDialogDelegateView::SetAnchorView(anchor);
  // If the widget is already showing, reposition it.
  views::Widget* widget = GetWidget();
  if (widget) {
    SizeToContents();
  }
}

views::View* AstraExtensionPopupView::GetAnchorView() const {
  return views::BubbleDialogDelegateView::GetAnchorView();
}

void AstraExtensionPopupView::SetPopupPosition(AstraPopupPosition position) {
  if (popup_position_ == position) {
    return;
  }
  popup_position_ = position;
  SetArrow(PositionToArrow(position));

  views::Widget* widget = GetWidget();
  if (widget) {
    SizeToContents();
  }
}

AstraPopupPosition AstraExtensionPopupView::GetPopupPosition() const {
  return popup_position_;
}

// static
views::BubbleBorder::Arrow AstraExtensionPopupView::PositionToArrow(
    AstraPopupPosition pos) {
  switch (pos) {
    case AstraPopupPosition::kAuto:
      return views::BubbleBorder::TOP_LEFT;  // Will be auto-adjusted
    case AstraPopupPosition::kAbove:
      return views::BubbleBorder::BOTTOM_CENTER;
    case AstraPopupPosition::kBelow:
      return views::BubbleBorder::TOP_CENTER;
    case AstraPopupPosition::kLeft:
      return views::BubbleBorder::RIGHT_CENTER;
    case AstraPopupPosition::kRight:
      return views::BubbleBorder::LEFT_CENTER;
  }
  return views::BubbleBorder::LEFT_CENTER;
}

// =========================================================================
// Header access
// =========================================================================

void AstraExtensionPopupView::SetHeaderVisible(bool visible) {
  if (header_view_) {
    header_view_->SetVisible(visible);
  }
}

bool AstraExtensionPopupView::IsHeaderVisible() const {
  return header_view_ && header_view_->GetVisible();
}

views::View* AstraExtensionPopupView::GetHeaderView() {
  return header_view_;
}

// =========================================================================
// Content access
// =========================================================================

views::View* AstraExtensionPopupView::GetContentView() {
  return content_area_;
}

// =========================================================================
// Extension view hosting
// =========================================================================

void AstraExtensionPopupView::SetExtensionView(views::View* extension_view) {
  if (!content_area_) {
    return;
  }

  // Remove old extension view if any.
  if (extension_view_) {
    content_area_->RemoveChildViewT(extension_view_);
  }

  extension_view_ = extension_view;

  if (extension_view_) {
    content_area_->AddChildView(extension_view_);
    extension_view_->SetBounds(content_area_->GetLocalBounds());
  }
}

views::View* AstraExtensionPopupView::GetExtensionView() const {
  return extension_view_;
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
// Loading state
// =========================================================================

void AstraExtensionPopupView::SetLoading(bool loading) {
  if (is_loading_ == loading) {
    return;
  }
  is_loading_ = loading;
  // TODO(astra): Show/hide loading spinner in the content area.
  //   Chromium pattern: Throbber view for loading states.
  SchedulePaint();
}

bool AstraExtensionPopupView::IsLoading() const {
  return is_loading_;
}

// =========================================================================
// Error state
// =========================================================================

void AstraExtensionPopupView::SetErrorState(bool error,
                                             const std::u16string& error_message) {
  has_error_ = error;
  error_message_ = error_message;
  // TODO(astra): Show error message in the content area when error is true.
  //   Replace extension content with an error state view.
  SchedulePaint();
}

bool AstraExtensionPopupView::HasError() const {
  return has_error_;
}

const std::u16string& AstraExtensionPopupView::GetErrorMessage() const {
  return error_message_;
}

// =========================================================================
// Actions
// =========================================================================

void AstraExtensionPopupView::ReloadExtension() {
  // TODO(astra): Reload the extension popup content.
  //   Chromium pattern: ExtensionPopup::Reload() or navigate the WebContents.
  if (popup_web_contents_) {
    popup_web_contents_->GetController().Reload(content::ReloadType::NORMAL,
                                                 true);
  }
}

void AstraExtensionPopupView::InspectPopup() {
  // TODO(astra): Open DevTools for the popup WebContents.
  //   Chromium pattern: DevToolsWindow::OpenDevToolsWindow() or
  //   DevToolsController::ShowDevTools().
  //   Chromium owner: chrome/browser/devtools/devtools_window.h
}

// =========================================================================
// Layout
// =========================================================================

void AstraExtensionPopupView::BuildLayout() {
  // Vertical layout: header + content area.
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  // --------------------------------------------------------------------
  // Header row
  // --------------------------------------------------------------------
  header_view_ = AddChildView(std::make_unique<views::View>());
  header_view_->SetPreferredSize(gfx::Size(0, kPopupHeaderHeight));
  auto* header_layout =
      header_view_->SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(0, kPopupHeaderPadding), 0));
  header_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  header_layout->set_between_child_spacing(8);

  // Extension icon.
  header_icon_ =
      header_view_->AddChildView(std::make_unique<views::ImageView>());
  header_icon_->SetPreferredSize(gfx::Size(16, 16));
  header_icon_->SetVisible(false);  // Hidden until icon is set.

  // Extension name / title.
  title_label_ =
      header_view_->AddChildView(std::make_unique<views::Label>(extension_name_));
  title_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_label_->SetAutoColorReadabilityEnabled(false);
  header_layout->SetFlexForView(title_label_, 1);

  // Options button (opens extension options page).
  options_button_ =
      header_view_->AddChildView(std::make_unique<views::LabelButton>(
          base::BindRepeating(&AstraExtensionPopupView::OnOptionsButtonPressed,
                              base::Unretained(this)),
          u"Options"));
  options_button_->SetVisible(show_options_button_);
  // TODO(astra): Replace text button with icon button.

  // Close button.
  close_button_ =
      header_view_->AddChildView(std::make_unique<views::LabelButton>(
          base::BindRepeating(&AstraExtensionPopupView::OnCloseButtonPressed,
                              base::Unretained(this)),
          u"x"));
  close_button_->SetVisible(show_close_button_);
  // TODO(astra): Replace with a proper close icon button.

  // --------------------------------------------------------------------
  // Content area
  // --------------------------------------------------------------------
  content_area_ = AddChildView(std::make_unique<views::View>());
  content_area_->SetLayoutManager(std::make_unique<views::FillLayout>());
  content_area_->SetBorder(
      views::CreateEmptyBorder(gfx::Insets(kPopupContentPadding)));

  // Set flex so content area fills remaining space.
  layout->SetFlexForView(content_area_, 1);

  UpdateHeaderVisibility();
}

void AstraExtensionPopupView::UpdateHeaderVisibility() {
  if (!header_view_) {
    return;
  }
  // Header is visible if any of its elements are shown.
  bool visible = show_title_ || show_close_button_ || show_options_button_;
  header_view_->SetVisible(visible);

  if (title_label_) {
    title_label_->SetVisible(show_title_);
  }
}

void AstraExtensionPopupView::ResizeToContent() {
  // TODO(astra): Resize the popup widget to fit the content.
  //   Extension popups can request size changes via JavaScript or CSS.
  //   We should listen for size changes and update the bubble bounds.
  //
  //   Chromium pattern: ExtensionPopup::OnExtensionSizeChanged() handles
  //   this for toolbar action popups.
  SizeToContents();
}

// =========================================================================
// Button handlers
// =========================================================================

void AstraExtensionPopupView::OnCloseButtonPressed() {
  Hide();
}

void AstraExtensionPopupView::OnOptionsButtonPressed() {
  // TODO(astra): Open extension options page.
  //   Forward to delegate or open chrome://extensions?options=...
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

void AstraExtensionPopupView::OnWidgetActivationChanged(views::Widget* widget,
                                                        bool active) {
  views::BubbleDialogDelegateView::OnWidgetActivationChanged(widget, active);

  // Auto-dismiss on deactivation is handled by set_close_on_deactivate(true).
  // We don't need to do anything extra here.
}

}  // namespace astra
