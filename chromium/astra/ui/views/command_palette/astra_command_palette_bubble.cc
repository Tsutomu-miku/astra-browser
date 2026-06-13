#include "astra/ui/views/command_palette/astra_command_palette_bubble.h"

#include <memory>
#include <utility>

#include "chrome/browser/ui/browser.h"
#include "ui/color/color_provider.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

#include "astra/ui/color/astra_color_ids.h"
#include "astra/ui/views/command_palette/astra_command_palette_view.h"

namespace astra {

// =========================================================================
// Static factory
// =========================================================================

views::Widget* AstraCommandPaletteBubble::ShowBubble(views::View* anchor_view,
                                                     Browser* browser,
                                                     Delegate* delegate) {
  DCHECK(anchor_view);
  DCHECK(browser);

  // Create the bubble delegate.  BubbleDialogDelegateView creates its own
  // widget when shown.  The widget owns the delegate and will delete it
  // when the widget is destroyed.
  auto* bubble = new AstraCommandPaletteBubble(anchor_view, browser, delegate);

  // Show the bubble widget.  CreateBubble returns the widget, which is
  // owned by the native widget system.
  views::Widget* widget = views::BubbleDialogDelegateView::CreateBubble(bubble);
  bubble->SetArrow(views::BubbleBorder::TOP_LEFT);
  // TODO(astra): Adjust the arrow position based on where the bubble is
  // anchored.  For an omnibox anchor, TOP_LEFT or TOP_CENTER makes sense.
  // For a toolbar button, the arrow should point to the button.

  widget->Show();
  bubble->RequestSearchFocus();

  return widget;
}

// =========================================================================
// Construction / destruction
// =========================================================================

AstraCommandPaletteBubble::AstraCommandPaletteBubble(
    views::View* anchor_view,
    Browser* browser,
    Delegate* delegate)
    : views::BubbleDialogDelegateView(anchor_view,
                                       views::BubbleBorder::TOP_LEFT,
                                       views::BubbleBorder::STANDARD_SHADOW),
      browser_(browser),
      delegate_(delegate) {
  // Set bubble properties.
  SetAcceptCallback(base::DoNothing());  // Accept is handled by Enter key.
  SetCancelCallback(base::DoNothing());  // Cancel is handled by Escape key.
  SetButtons(ui::DIALOG_BUTTON_NONE);    // No OK/Cancel buttons.
  SetShowTitle(false);
  SetShowCloseButton(false);
  set_fixed_width(kBubbleWidth);

  // Auto-dismiss when the widget loses activation (user clicks outside).
  // This is the default behavior for BubbleDialogDelegateView, but we
  // explicitly note it here as a design decision.
  set_close_on_deactivate(true);

  // The bubble is modeless — it doesn't block the rest of the UI.
  // This is the default for BubbleDialogDelegateView.
  set_modal_type(ui::MODAL_TYPE_NONE);
}

AstraCommandPaletteBubble::~AstraCommandPaletteBubble() {
  // Notify the model that the palette is closing.
  if (palette_view_ && palette_view_->GetModel()) {
    palette_view_->GetModel()->NotifyPaletteClosed();
  }

  // Notify the delegate that the palette is closing.
  if (delegate_) {
    delegate_->OnCommandPaletteClosed();
  }
}

// =========================================================================
// Show / hide
// =========================================================================

void AstraCommandPaletteBubble::Show(gfx::NativeView anchor_view,
                                     const gfx::Rect& anchor_rect) {
  // TODO(astra): Implement re-anchoring and showing.  The current
  // BubbleDialogDelegateView pattern creates the widget at construction
  // time.  For a Show() method that can be called multiple times, we
  // would need to either:
  //   a) Recreate the widget each time, or
  //   b) Update the anchor and show an existing widget.
  // For now, ShowBubble() is the primary entry point.
  //
  // Chromium pattern: Some bubbles use a lazy-create pattern where the
  // widget is created on first Show() and reused thereafter.
  //   (e.g. chrome/browser/ui/views/bookmarks/bookmark_bubble_view.cc)
  if (GetWidget()) {
    GetWidget()->Show();
    RequestSearchFocus();
  }
}

void AstraCommandPaletteBubble::Hide() {
  if (GetWidget()) {
    GetWidget()->Hide();
  }
}

bool AstraCommandPaletteBubble::IsVisible() const {
  if (GetWidget()) {
    return GetWidget()->IsVisible();
  }
  return false;
}

// =========================================================================
// Search query
// =========================================================================

void AstraCommandPaletteBubble::SetQuery(const std::u16string& query) {
  if (palette_view_) {
    palette_view_->SetQuery(query);
  }
}

std::u16string AstraCommandPaletteBubble::GetQuery() const {
  if (palette_view_) {
    return palette_view_->GetQuery();
  }
  return std::u16string();
}

// =========================================================================
// Selection
// =========================================================================

void AstraCommandPaletteBubble::SelectNext() {
  if (palette_view_) {
    palette_view_->SelectNext();
  }
}

void AstraCommandPaletteBubble::SelectPrevious() {
  if (palette_view_) {
    palette_view_->SelectPrevious();
  }
}

void AstraCommandPaletteBubble::SelectFirst() {
  if (palette_view_) {
    palette_view_->SelectFirst();
  }
}

void AstraCommandPaletteBubble::SelectLast() {
  if (palette_view_) {
    palette_view_->SelectLast();
  }
}

int AstraCommandPaletteBubble::GetSelectedIndex() const {
  if (palette_view_) {
    return palette_view_->GetSelectedIndex();
  }
  return -1;
}

// =========================================================================
// Execution
// =========================================================================

void AstraCommandPaletteBubble::ExecuteSelected() {
  if (palette_view_) {
    palette_view_->ExecuteSelected();
  }
}

// =========================================================================
// Sizing
// =========================================================================

void AstraCommandPaletteBubble::SetMaxHeight(int height) {
  if (max_height_ == height) {
    return;
  }
  max_height_ = height;
  if (GetWidget()) {
    GetWidget()->SetSize(CalculatePreferredSize(
        views::SizeBounds(gfx::Size(kBubbleWidth, max_height_))));
  }
  SizeToContents();
}

// =========================================================================
// Behavior
// =========================================================================

void AstraCommandPaletteBubble::CloseOnDeactivate(bool close) {
  close_on_deactivate_ = close;
  set_close_on_deactivate(close);
}

// =========================================================================
// BubbleDialogDelegateView overrides
// =========================================================================

void AstraCommandPaletteBubble::Init() {
  // Use fill layout so the palette takes up the entire bubble content area.
  SetLayoutManager(std::make_unique<views::FillLayout>());

  // Create the palette content view.
  auto palette_view = std::make_unique<AstraCommandPaletteView>(browser_);
  palette_view_ = palette_view.get();

  // The bubble itself acts as the palette's delegate.
  palette_view->SetDelegate(this);

  AddChildView(std::move(palette_view));

  // Notify the model that the palette has been opened.
  if (palette_view_->GetModel()) {
    palette_view_->GetModel()->NotifyPaletteOpened();
  }
}

void AstraCommandPaletteBubble::OnWidgetDestroying(views::Widget* widget) {
  views::BubbleDialogDelegateView::OnWidgetDestroying(widget);
  // The widget is being destroyed — the delegate will be notified by the
  // destructor (which runs after this call since the bubble is owned by
  // the widget).
}

void AstraCommandPaletteBubble::OnWidgetActivationChanged(
    views::Widget* widget,
    bool active) {
  views::BubbleDialogDelegateView::OnWidgetActivationChanged(widget, active);
  // When the widget loses activation (user clicks outside), the bubble
  // auto-closes due to set_close_on_deactivate(true).  No extra work needed.
}

gfx::Size AstraCommandPaletteBubble::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  gfx::Size size =
      views::BubbleDialogDelegateView::CalculatePreferredSize(available_size);

  // Clamp height to max.
  if (size.height() > max_height_) {
    size.set_height(max_height_);
  }

  return size;
}

// =========================================================================
// Focus management
// =========================================================================

void AstraCommandPaletteBubble::RequestSearchFocus() {
  if (palette_view_) {
    palette_view_->RequestSearchFocus();
  }
}

// =========================================================================
// AstraCommandPaletteView::Delegate implementation
// =========================================================================

void AstraCommandPaletteBubble::OnCommandPaletteExecute(int command_id,
                                                        bool is_astra) {
  // Close the palette first, then execute the command.
  // This way the command (e.g. "New Tab") won't try to focus the
  // palette while it's closing.
  if (GetWidget()) {
    GetWidget()->Close();
  }

  ForwardExecuteCommand(command_id, is_astra);
}

void AstraCommandPaletteBubble::OnCommandPaletteClose() {
  if (GetWidget()) {
    GetWidget()->Close();
  }
}

// =========================================================================
// Theme
// =========================================================================
//
// TODO(astra): Apply Astra color scheme to the bubble frame (border,
// background, shadow).  The bubble frame is drawn by BubbleFrameView and
// BubbleBorder.  To customize colors, we would need to either:
//   a) Override OnThemeChanged and call SetColor() on the BubbleBorder.
//   b) Use a custom BubbleBorder subclass that uses Astra color IDs.
//   c) Add Astra colors to the NativeTheme / ColorProvider so the
//      standard bubble border picks them up automatically.
//
// Chromium owner: BubbleBorder / BubbleFrameView
//   (ui/views/bubble/bubble_border.h)
// Patch point: ui/views/bubble/bubble_border.cc — OnThemeChanged method
// where border colors are resolved from ColorProvider.
//
// For now, the bubble uses the standard Chromium bubble colors, which
// adapt to light/dark theme automatically.
// =========================================================================

void AstraCommandPaletteBubble::UpdateBubbleTheme() {
  // TODO(astra): Implement theme updates for the bubble frame.
  // See the comment block above for details.
}

// =========================================================================
// Private helpers
// =========================================================================

void AstraCommandPaletteBubble::ForwardExecuteCommand(int command_id,
                                                      bool is_astra) {
  if (delegate_) {
    delegate_->OnCommandPaletteExecute(command_id, is_astra);
  }
}

}  // namespace astra
