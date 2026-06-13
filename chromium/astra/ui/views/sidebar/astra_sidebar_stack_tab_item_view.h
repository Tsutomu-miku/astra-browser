#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_STACK_TAB_ITEM_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_STACK_TAB_ITEM_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/view.h"

namespace views {
class ImageButton;
class ImageView;
class Label;
}  // namespace views

namespace content {
class WebContents;
}  // namespace content

namespace astra {

// Delegate interface for AstraSidebarStackTabItemView actions.
// Implemented by the parent stack view to handle tab activation,
// close actions, and drag for tab items within a named stack.
//
// The tab item view is presentation-only — it never mutates tab or
// stack state directly.  All user actions are forwarded to the delegate.
//
// Chromium owner: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)
class AstraSidebarStackTabItemDelegate {
 public:
  virtual ~AstraSidebarStackTabItemDelegate() = default;

  // Called when the user clicks a tab item in a stack.
  // |web_contents| is the tab represented by this item.
  virtual void OnStackTabClicked(content::WebContents* web_contents) = 0;

  // Called when the user clicks the close button on a tab item.
  // |web_contents| is the tab represented by this item.
  virtual void OnStackTabClosed(content::WebContents* web_contents) = 0;

  // Called when the user starts dragging a tab item out of a stack.
  // |web_contents| is the tab being dragged.
  // |mouse_location| is in the item view's local coordinates.
  virtual void OnStackTabDragStarted(
      content::WebContents* web_contents,
      const gfx::Point& mouse_location) = 0;
};

// A sidebar item that represents a tab within a named stack.
//
// Stack tab items are indented from the left edge to visually show they
// belong to a stack.  They show:
//   - Favicon (or placeholder icon)
//   - Tab title
//   - Audio indicator if the tab is playing audio
//   - Close button on hover
//
// The text may be slightly smaller or lighter than top-level tabs to
// visually de-emphasize stacked tabs.
//
// Truth source:
//   - Tab title/favicon/audio: Chromium WebContents / TabStripModel
//   - Stack membership: AstraTabFeatures (stack_id)
//   - Stack metadata: AstraTabStackService
//
// This extends views::View rather than AstraSidebarItemView because
// the indentation and layout differ from regular sidebar items.
// However, it follows the same presentation-only pattern.
//
// Chromium owner: TabRenderer (chrome/browser/ui/views/tabs/tab_renderer.h)
// Chromium pattern: views::View with mouse event handling for drag.
class AstraSidebarStackTabItemView : public views::View {
 public:
  // Audio indicator state, mirroring AstraSidebarItemView::AudioState.
  enum class AudioState {
    kNone,    // No audio playing — indicator hidden.
    kPlaying, // Audio is playing — speaker icon shown.
    kMuted,   // Tab is muted — muted speaker icon shown.
  };

  // TODO(astra): Accept WebContents* or a tab ID as the primary
  //   identifier instead of just a title string.  For the skeleton
  //   implementation we use a title string for layout testing.
  explicit AstraSidebarStackTabItemView(const std::u16string& title);
  AstraSidebarStackTabItemView(const AstraSidebarStackTabItemView&) = delete;
  AstraSidebarStackTabItemView& operator=(
      const AstraSidebarStackTabItemView&) = delete;
  ~AstraSidebarStackTabItemView() override;

  // Update the displayed title.
  void SetTitle(const std::u16string& title);

  // Set this tab item as active (highlighted).
  void SetActive(bool active);
  bool IsActive() const { return is_active_; }

  // Set the audio state of the tab this item represents.
  void SetAudioState(AudioState state);
  AudioState audio_state() const { return audio_state_; }

  // Set whether the close button is visible (typically shown on hover).
  void SetCloseButtonVisible(bool visible);

  // Set the delegate for tab item actions. Not owned.
  void set_delegate(AstraSidebarStackTabItemDelegate* delegate) {
    delegate_ = delegate;
  }

  // -- Tab metadata -------------------------------------------------------

  // WebContents associated with this tab item.  Used for delegate
  // callbacks so the parent view can identify which tab was acted upon.
  // TODO(astra): Use WebContents* as the primary identifier once the
  //   sidebar is wired to the actual TabStripModel.
  void set_web_contents(content::WebContents* web_contents) {
    web_contents_ = web_contents;
  }
  content::WebContents* web_contents() const { return web_contents_; }

  // Stack ID this tab belongs to.  Cached on the view for convenience;
  // truth source is AstraTabFeatures on the WebContents.
  void set_stack_id(const std::string& stack_id) { stack_id_ = stack_id; }
  const std::string& stack_id() const { return stack_id_; }

  // -- Drag and drop ------------------------------------------------------

  // Set whether this tab item can be dragged. Defaults to false.
  void SetDraggable(bool draggable);
  bool draggable() const { return draggable_; }

  // -- views::View --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;
  void OnThemeChanged() override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  bool OnMouseDragged(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnMouseCaptureLost() override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

 private:
  // Handler for close button clicks.
  void OnCloseButtonClicked();

  // Handler for audio button clicks — toggles mute state.
  void OnAudioButtonClicked();

  // Update the audio button's icon and tooltip.
  void UpdateAudioButtonVisuals();

  // Indentation from the left edge for stacked tab items (in DIPs).
  // This is extra indent beyond normal sidebar item padding, to visually
  // show the hierarchical relationship with the stack header.
  static constexpr int kTabItemIndent = 24;

  // Height of a stack tab item.
  static constexpr int kTabItemHeight = 28;

  // Horizontal padding within the tab item.
  static constexpr int kHorizontalPadding = 12;

  // Icon size and spacing.
  static constexpr int kIconSize = 14;
  static constexpr int kIconSpacing = 8;

  // Close button size.
  static constexpr int kCloseButtonSize = 16;

  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::ImageButton> close_button_ = nullptr;
  raw_ptr<views::ImageButton> audio_button_ = nullptr;
  raw_ptr<views::ImageView> favicon_view_ = nullptr;

  bool is_active_ = false;
  bool is_hovered_ = false;
  AudioState audio_state_ = AudioState::kNone;

  bool draggable_ = false;
  gfx::Point drag_start_point_;
  bool is_dragging_ = false;

  raw_ptr<content::WebContents> web_contents_ = nullptr;
  std::string stack_id_;

  raw_ptr<AstraSidebarStackTabItemDelegate> delegate_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_STACK_TAB_ITEM_VIEW_H_
