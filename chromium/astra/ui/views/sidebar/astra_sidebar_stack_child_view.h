#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_STACK_CHILD_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_STACK_CHILD_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/view.h"

namespace views {
class ImageButton;
class ImageView;
class Label;
}  // namespace views

namespace astra {

// Delegate interface for AstraSidebarStackChildView actions.
// Implemented by the parent sidebar view to handle tab activation and
// close actions for child tabs in a stack.
//
// The child view is presentation-only — it never mutates tab or stack state
// directly.  All user actions are forwarded to the delegate.
class AstraSidebarStackChildDelegate {
 public:
  virtual ~AstraSidebarStackChildDelegate() = default;

  // Called when the user clicks a child tab in a stack.
  // |tab_index| is the TabStripModel index of the child tab.
  virtual void OnStackChildClicked(int tab_index) = 0;

  // Called when the user clicks the close button on a child tab.
  // |tab_index| is the TabStripModel index of the child tab.
  virtual void OnStackChildClosed(int tab_index) = 0;

  // Called when the user starts dragging a child tab.
  // |tab_index| is the TabStripModel index of the child tab.
  // |mouse_location| is in the child view's local coordinates.
  virtual void OnStackChildDragStarted(int tab_index,
                                       const gfx::Point& mouse_location) = 0;
};

// A sidebar item that represents a child tab within a stack.
//
// Stack child items are indented from the left edge to visually show they
// belong to a parent stack.  They show:
//   - Favicon (or placeholder icon)
//   - Tab title
//   - Audio indicator if the tab is playing audio
//   - Close button on hover
//
// The text may be slightly smaller or lighter than top-level tabs to
// visually de-emphasize child tabs.
//
// Truth source:
//   - Tab title/favicon/audio: Chromium WebContents / TabStripModel
//   - Stack membership: AstraTabFeatures (stack_parent_id)
//
// This extends views::View rather than AstraSidebarItemView because
// the indentation and close button layout differ from regular sidebar
// items.  However, it follows the same presentation-only pattern.
//
// Chromium owner: TreeView/TreeViewController (ui/views/controls/tree/)
// TODO(astra): Consider migrating to views::TreeView for proper tree UI.
class AstraSidebarStackChildView : public views::View {
 public:
  // Audio indicator state, mirroring AstraSidebarItemView::AudioState.
  enum class AudioState {
    kNone,    // No audio playing — indicator hidden.
    kPlaying, // Audio is playing — speaker icon shown.
    kMuted,   // Tab is muted — muted speaker icon shown.
  };

  explicit AstraSidebarStackChildView(const std::u16string& title);
  AstraSidebarStackChildView(const AstraSidebarStackChildView&) = delete;
  AstraSidebarStackChildView& operator=(
      const AstraSidebarStackChildView&) = delete;
  ~AstraSidebarStackChildView() override;

  // Update the displayed title.
  void SetTitle(const std::u16string& title);

  // Set this child tab as active (highlighted).
  void SetActive(bool active);
  bool IsActive() const { return is_active_; }

  // Set the audio state of the tab this child represents.
  void SetAudioState(AudioState state);
  AudioState audio_state() const { return audio_state_; }

  // Set whether the close button is visible (typically shown on hover).
  void SetCloseButtonVisible(bool visible);

  // Set the delegate for child tab actions. Not owned.
  void set_delegate(AstraSidebarStackChildDelegate* delegate) {
    delegate_ = delegate;
  }

  // -- Tab metadata -------------------------------------------------------

  // TabStripModel index of the tab this child represents.
  void set_tab_index(int index) { tab_index_ = index; }
  int tab_index() const { return tab_index_; }

  // -- Drag and drop ------------------------------------------------------

  // Set whether this child can be dragged. Defaults to false.
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

  // Indentation from the left edge for child tabs (in DIPs).
  // This is extra indent beyond the normal sidebar item padding, to
  // visually show the hierarchical relationship.
  static constexpr int kChildIndent = 20;

  // Height of a stack child item.
  static constexpr int kChildItemHeight = 28;

  // Horizontal padding within the child item.
  static constexpr int kChildHorizontalPadding = 12;

  // Icon size and spacing.
  static constexpr int kChildIconSize = 14;
  static constexpr int kChildIconSpacing = 8;

  // Close button size.
  static constexpr int kCloseButtonSize = 16;

  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::ImageButton> close_button_ = nullptr;
  raw_ptr<views::ImageButton> audio_button_ = nullptr;

  bool is_active_ = false;
  bool is_hovered_ = false;
  AudioState audio_state_ = AudioState::kNone;

  bool draggable_ = false;
  gfx::Point drag_start_point_;
  bool is_dragging_ = false;

  int tab_index_ = -1;

  raw_ptr<AstraSidebarStackChildDelegate> delegate_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_STACK_CHILD_VIEW_H_
