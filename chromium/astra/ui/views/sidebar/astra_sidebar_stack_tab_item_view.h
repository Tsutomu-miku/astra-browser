#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_STACK_TAB_ITEM_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_STACK_TAB_ITEM_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/view.h"
#include "url/gurl.h"

namespace views {
class ImageButton;
class ImageView;
class Label;
}  // namespace views

namespace content {
class WebContents;
}  // namespace content

namespace astra {

// =========================================================================
// AstraStackTabInfo — metadata for a tab within a stack
// =========================================================================
//
// Value type that carries all display-relevant metadata for a single tab
// within a named stack.  Copies are cheap — views always receive copies so
// they cannot mutate model state directly.
//
// Truth source:
//   - Tab title/favicon/audio: Chromium WebContents / TabStripModel
//   - Stack membership: AstraTabFeatures (stack_id)
//   - Active state: TabStripModel active tab
//
// Chromium owner: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)
struct AstraStackTabInfo {
  // Unique identifier for the tab.
  std::string tab_id;

  // Tab title (UTF-16 for i18n).
  std::u16string title;

  // Tab URL.
  GURL url;

  // Whether this tab is the active tab in the browser.
  bool is_active = false;

  // Whether this tab is pinned.
  bool is_pinned = false;

  // Whether audio is currently playing in this tab.
  bool is_audible = false;

  // Whether the tab is muted.
  bool is_muted = false;

  // Whether the tab is currently loading.
  bool is_loading = false;

  // Whether the tab has crashed.
  bool is_crashed = false;

  // Favicon image.
  gfx::ImageSkia favicon;

  // Whether a favicon is available.
  bool has_favicon = false;

  // Index of this tab within its parent stack.
  int index_in_stack = 0;

  // Timestamp of last access.
  base::Time last_accessed;
};

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
//   - Loading indicator if the tab is loading
//   - Crash indicator if the tab has crashed
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
  // Audio indicator state.
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

  // -- Tab info -----------------------------------------------------------

  // Set all tab info at once.  Updates all visual elements.
  void SetTabInfo(const AstraStackTabInfo& info);

  // Get the tab ID.
  std::string GetTabId() const { return tab_id_; }

  // -- Title --------------------------------------------------------------

  // Update the displayed title.
  void SetTitle(const std::u16string& title);
  std::u16string GetTitle() const;

  // -- URL ----------------------------------------------------------------

  // Set the tab URL (shown as tooltip).
  void SetUrl(const GURL& url);
  GURL GetUrl() const { return url_; }

  // -- Favicon ------------------------------------------------------------

  // Set the favicon image.
  void SetFavicon(const gfx::ImageSkia& favicon);

  // Set whether a favicon is available (controls visibility).
  void SetHasFavicon(bool has_favicon);

  // -- Active state -------------------------------------------------------

  // Set this tab item as active (highlighted).
  void SetActive(bool active);
  bool IsActive() const { return is_active_; }

  // -- Close button -------------------------------------------------------

  // Set whether the close button is visible (typically shown on hover).
  void SetCloseButtonVisible(bool visible);
  bool IsCloseButtonVisible() const;

  // Set whether the close button should be shown (enabled feature).
  void SetShowCloseButton(bool show);
  bool GetShowCloseButton() const { return show_close_button_; }

  // -- Favicon visibility -------------------------------------------------

  // Set whether favicons are shown.
  void SetShowFavicon(bool show);
  bool GetShowFavicon() const { return show_favicon_; }

  // -- Drag state ---------------------------------------------------------

  // Set whether this tab item is currently being dragged.
  void SetIsDragging(bool dragging);
  bool IsDragging() const { return is_dragging_; }

  // Set whether this item is a drop target (drag hovered).
  void SetDragHovered(bool hovered);
  bool IsDragHovered() const { return is_drag_hovered_; }

  // -- Index --------------------------------------------------------------

  // Set the position index within the parent stack.
  void SetIndex(int index);
  int GetIndex() const { return index_in_stack_; }

  // -- Stack ID -----------------------------------------------------------

  // Set the ID of the stack this tab belongs to.
  void SetStackId(const std::string& stack_id);
  std::string GetStackId() const { return stack_id_; }

  // -- First/last in stack ------------------------------------------------

  // Set whether this is the first item in the stack (affects corner radius).
  void SetIsFirst(bool first);
  bool IsFirst() const { return is_first_; }

  // Set whether this is the last item in the stack (affects corner radius).
  void SetIsLast(bool last);
  bool IsLast() const { return is_last_; }

  // -- Pinned -------------------------------------------------------------

  // Set whether this tab is pinned.
  void SetPinned(bool pinned);
  bool IsPinned() const { return is_pinned_; }

  // -- Audio state --------------------------------------------------------

  // Set whether audio is playing in this tab.
  void SetIsAudible(bool audible);
  bool IsAudible() const { return is_audible_; }

  // Set whether the tab is muted.
  void SetIsMuted(bool muted);
  bool IsMuted() const { return is_muted_; }

  // Set the audio state directly (legacy / combined method).
  void SetAudioState(AudioState state);
  AudioState audio_state() const { return audio_state_; }

  // -- Loading ------------------------------------------------------------

  // Set whether the tab is currently loading.
  void SetIsLoading(bool loading);
  bool IsLoading() const { return is_loading_; }

  // -- Crashed ------------------------------------------------------------

  // Set whether the tab has crashed.
  void SetIsCrashed(bool crashed);
  bool IsCrashed() const { return is_crashed_; }

  // -- Drag and drop ------------------------------------------------------

  // Set whether this tab item can be dragged. Defaults to false.
  void SetDraggable(bool draggable);
  bool draggable() const { return draggable_; }

  // -- Legacy / compatibility ---------------------------------------------
  //
  // These methods are kept for backward compatibility.
  // TODO(astra): Remove legacy methods once all callers are updated.

  void set_web_contents(content::WebContents* web_contents) {
    web_contents_ = web_contents;
  }
  content::WebContents* web_contents() const { return web_contents_; }

  void set_stack_id(const std::string& stack_id) { SetStackId(stack_id); }
  const std::string& stack_id() const { return stack_id_; }

  void set_delegate(AstraSidebarStackTabItemDelegate* delegate) {
    delegate_ = delegate;
  }

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
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

 private:
  // Handler for close button clicks.
  void OnCloseButtonClicked();

  // Handler for audio button clicks — toggles mute state.
  void OnAudioButtonClicked();

  // Update the audio button's icon and tooltip.
  void UpdateAudioButtonVisuals();

  // Update the favicon view based on show_favicon_ and has_favicon_.
  void UpdateFaviconVisibility();

  // Update background color based on current state.
  void UpdateBackgroundColor();

  // Update corner radius based on is_first_ and is_last_.
  void UpdateCornerRadius();

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

  // Corner radius for the tab item background.
  static constexpr int kTabItemCornerRadius = 4;

  // -- Child views --------------------------------------------------------

  raw_ptr<views::ImageView> favicon_view_ = nullptr;
  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::ImageButton> close_button_ = nullptr;
  raw_ptr<views::ImageButton> audio_button_ = nullptr;

  // -- State --------------------------------------------------------------

  std::string tab_id_;
  std::u16string title_;
  GURL url_;

  bool is_active_ = false;
  bool is_hovered_ = false;
  bool is_dragging_ = false;
  bool is_drag_hovered_ = false;
  bool is_first_ = false;
  bool is_last_ = false;
  bool is_pinned_ = false;
  bool is_audible_ = false;
  bool is_muted_ = false;
  bool is_loading_ = false;
  bool is_crashed_ = false;
  bool has_favicon_ = false;

  bool show_favicon_ = true;
  bool show_close_button_ = true;

  AudioState audio_state_ = AudioState::kNone;

  int index_in_stack_ = -1;

  bool draggable_ = false;
  gfx::Point drag_start_point_;

  raw_ptr<content::WebContents> web_contents_ = nullptr;
  std::string stack_id_;

  raw_ptr<AstraSidebarStackTabItemDelegate> delegate_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_STACK_TAB_ITEM_VIEW_H_
