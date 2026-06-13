#ifndef ASTRA_UI_VIEWS_COMMAND_PALETTE_ASTRA_COMMAND_PALETTE_BUBBLE_H_
#define ASTRA_UI_VIEWS_COMMAND_PALETTE_ASTRA_COMMAND_PALETTE_BUBBLE_H_

#include "base/memory/raw_ptr.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/widget/widget_observer.h"

#include "astra/ui/views/command_palette/astra_command_palette_view.h"

class Browser;

namespace astra {

// =========================================================================
// Command palette bubble — floating dialog wrapper
// =========================================================================
//
// AstraCommandPaletteBubble wraps the command palette content view in a
// Chromium bubble dialog.  Bubbles are the standard pattern for floating
// UI surfaces in Chrome (bookmark bubble, download bubble, etc.).
//
// The bubble is:
//   - Modeless (doesn't block the rest of the UI).
//   - Anchorable to a toolbar button or the omnibox.
//   - Auto-dismissed when focus is lost (click outside, switch windows).
//   - Styled with rounded corners and shadow via BubbleDialogDelegateView.
//   - Sized with fixed width and max height (content scrolls internally).
//
// Chromium subsystem reused: views::BubbleDialogDelegateView
// (ui/views/bubble/bubble_dialog_delegate_view.h).
//
// Implements AstraCommandPaletteView::Delegate to receive command execution
// and close events from the palette content view.
//
// Architecture:
//   - Bubble is the container / widget owner.
//   - View is the content / UI presentation.
//   - Model is the data / search logic (owned by the view).
//   - Delegate pattern bridges to browser-level command execution.
//
// TODO(astra): Anchor the bubble to the actual toolbar / omnibox position
// in the BrowserView.  Currently the anchor is passed in from the caller.
// The proper way to get the anchor is to find the location bar or toolbar
// button from BrowserView.
// Chromium owner: LocationBar / OmniboxView (chrome/browser/ui/views/location_bar)
//
// TODO(astra): Use a full-width top-of-window overlay style instead of a
// small anchored bubble for a more "command palette" feel (like VS Code's
// Ctrl+Shift+P or Sublime's Ctrl+P).  The bubble pattern works well for
// small popups but a command palette typically spans most of the window
// width and is centered near the top.
// =========================================================================

class AstraCommandPaletteBubble : public views::BubbleDialogDelegateView,
                                  public AstraCommandPaletteView::Delegate {
 public:
  // Delegate interface for command execution and close events at the
  // browser level.  The AstraBrowserView implements this to receive
  // notifications from the palette bubble.
  //
  // All methods have empty default implementations — subclasses only
  // override the methods they care about.
  class Delegate {
   public:
    virtual void OnCommandPaletteExecute(int command_id, bool is_astra) {}
    virtual void OnCommandPaletteClosed() {}

   protected:
    virtual ~Delegate() = default;
  };

  // Creates and shows the command palette bubble anchored to |anchor_view|.
  // Returns the bubble widget (owned by the widget system).
  // The caller can listen for closure via the Delegate.
  static views::Widget* ShowBubble(views::View* anchor_view,
                                   Browser* browser,
                                   Delegate* delegate);

  ~AstraCommandPaletteBubble() override;

  AstraCommandPaletteBubble(const AstraCommandPaletteBubble&) = delete;
  AstraCommandPaletteBubble& operator=(const AstraCommandPaletteBubble&) =
      delete;

  // -- Show / hide -------------------------------------------------------

  // Shows the bubble anchored to |anchor_view| with |anchor_rect| as the
  // anchor rectangle.  If the bubble is already visible, repositions it.
  void Show(gfx::NativeView anchor_view, const gfx::Rect& anchor_rect);

  // Hides the bubble (closes the widget).
  void Hide();

  // Returns whether the bubble is currently visible.
  bool IsVisible() const;

  // -- View access -------------------------------------------------------

  // Returns the inner command palette view.
  AstraCommandPaletteView* GetView() { return palette_view_; }
  const AstraCommandPaletteView* GetView() const { return palette_view_; }

  // Legacy accessor.
  AstraCommandPaletteView* palette_view() { return palette_view_; }
  const AstraCommandPaletteView* palette_view() const { return palette_view_; }

  // -- Search query ------------------------------------------------------

  // Sets the search query in the palette.
  void SetQuery(const std::u16string& query);

  // Returns the current search query.
  std::u16string GetQuery() const;

  // -- Selection ---------------------------------------------------------

  // Moves selection to the next item.
  void SelectNext();

  // Moves selection to the previous item.
  void SelectPrevious();

  // Selects the first item.
  void SelectFirst();

  // Selects the last item.
  void SelectLast();

  // Returns the index of the currently selected item.
  int GetSelectedIndex() const;

  // -- Execution ---------------------------------------------------------

  // Executes the currently selected command.
  void ExecuteSelected();

  // -- Sizing ------------------------------------------------------------

  // Sets the maximum height of the bubble.
  void SetMaxHeight(int height);

  // Returns the maximum height of the bubble.
  int GetMaxHeight() const { return max_height_; }

  // -- Behavior ----------------------------------------------------------

  // Sets whether the bubble closes when it loses activation (focus).
  void CloseOnDeactivate(bool close);

  // Returns whether the bubble closes on deactivation.
  bool close_on_deactivate() const { return close_on_deactivate_; }

  // -- views::BubbleDialogDelegateView -----------------------------------

  void OnWidgetDestroying(views::Widget* widget) override;
  void OnWidgetActivationChanged(views::Widget* widget, bool active) override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;

  // Gives focus to the search field inside the palette.
  void RequestSearchFocus();

  // -- AstraCommandPaletteView::Delegate ----------------------------------

  void OnCommandPaletteExecute(int command_id, bool is_astra) override;
  void OnCommandPaletteClose() override;

 private:
  AstraCommandPaletteBubble(views::View* anchor_view,
                            Browser* browser,
                            Delegate* delegate);

  // Build the bubble contents.
  void Init() override;

  // Apply Astra-specific theming to the bubble border and background.
  void UpdateBubbleTheme();

  // Forward command execution to the browser-level delegate.
  void ForwardExecuteCommand(int command_id, bool is_astra);

  raw_ptr<Browser> browser_;
  raw_ptr<Delegate> delegate_ = nullptr;

  // The content view (owned by the view hierarchy via AddChildView).
  raw_ptr<AstraCommandPaletteView> palette_view_ = nullptr;

  // Bubble sizing.
  int max_height_ = kBubbleMaxHeight;
  bool close_on_deactivate_ = true;

  // Bubble sizing constants.
  static constexpr int kBubbleWidth = 560;
  static constexpr int kBubbleMaxHeight = 600;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_COMMAND_PALETTE_ASTRA_COMMAND_PALETTE_BUBBLE_H_
