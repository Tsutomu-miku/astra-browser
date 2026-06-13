#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_ITEM_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_ITEM_VIEW_H_

#include <string>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/view.h"

namespace views {
class ImageButton;
class ImageView;
class Label;
}  // namespace views

namespace astra {

struct AstraSidebarDragData;

// Delegate interface for AstraSidebarItemView drag events.
// Implemented by the parent view (sidebar or section) that manages drag
// sessions. The item view itself does not own drag state — it detects
// drag gestures and delegates to the controller.
class AstraSidebarItemDragDelegate {
 public:
  virtual ~AstraSidebarItemDragDelegate() = default;

  // Called when a drag gesture is detected on |item|.
  // |mouse_location| is in the item's local coordinates.
  // The delegate should start a drag session with the appropriate data.
  virtual void OnItemDragStarted(AstraSidebarItemView* item,
                                 const gfx::Point& mouse_location) = 0;
};

// Delegate interface for AstraSidebarItemView hover events.
// Implemented by the parent sidebar view to trigger peek/preview popups
// when the user hovers over a sidebar item.
//
// TODO(astra): Consider adding proper hover timing configuration and
//   accessibility support (keyboard-triggered peek, screen reader support).
//   Chromium pattern: TabHoverCardController handles both mouse and
//   keyboard hover for accessibility.
//   Chromium owner: TabHoverCardController
//   (chrome/browser/ui/tabs/tab_hover_card_controller.h).
class AstraSidebarItemHoverDelegate {
 public:
  virtual ~AstraSidebarItemHoverDelegate() = default;

  // Called when the mouse enters |item|.
  // |mouse_location| is in the item's local coordinates.
  virtual void OnItemHoverStarted(AstraSidebarItemView* item,
                                  const gfx::Point& mouse_location) = 0;

  // Called when the mouse leaves |item|.
  virtual void OnItemHoverEnded(AstraSidebarItemView* item) = 0;

  // Called when the mouse moves within |item| after hover has started.
  virtual void OnItemHoverMoved(AstraSidebarItemView* item,
                                const gfx::Point& mouse_location) = 0;
};

// Delegate interface for AstraSidebarItemView context menu events.
// Implemented by the parent section view to show context menus.
class AstraSidebarItemContextMenuDelegate {
 public:
  virtual ~AstraSidebarItemContextMenuDelegate() = default;

  // Called when a context menu is requested on |item|.
  // |point| is in screen coordinates.
  virtual void OnItemContextMenuRequested(AstraSidebarItemView* item,
                                          const gfx::Point& point) = 0;
};

// Base class for all sidebar item views.
//
// Provides common state management and layout infrastructure shared by
// all sidebar item types (bookmarks, history, downloads, reading list,
// notes, passwords, extensions, recently closed, tabs, etc.).
//
// Visual structure:
//   [leading icon] [title text] [secondary text] [trailing icon/badge/chevron]
//   Hover background highlight
//   Selection background
//   Context menu on right-click
//
// This is a pure presentation view — it does not own product state.
// Data is projected from Chromium models by the parent sidebar section.
//
// TODO(astra): Replace icon placeholders with real vector icons from
//   Chromium's ui/resources/vector_icons/ once building against a full
//   Chromium checkout.
//   Chromium owner: ui/resources/vector_icons/
class AstraSidebarItemView : public views::View {
 public:
  // Item type — determines visual styling and behavior.
  // Each item type has slightly different presentation (icon, spacing,
  // hover behavior, etc.). The type is a presentation hint, not product state.
  enum class Type {
    kTab,        // Regular tab item
    kPinnedTab,  // Pinned tab item
    kFavorite,   // Favorite item
    kWorkspace,  // Workspace item
    kBookmark,   // Bookmark item
    kHistory,    // History item
    kDownload,   // Download item
    kExtension,  // Extension item
    kNote,       // Note item
    kPassword,   // Password item
    kCustom,     // Custom/generic item
  };

  // Audio indicator state for sidebar tab items.
  // Mirrors the audio states shown by Chromium's TabRenderer.
  // The sidebar projects WebContents audio state; it never stores audio truth.
  //
  // Chromium owner: TabRenderer::PaintTab (chrome/browser/ui/views/tabs/tab_renderer.h)
  // Chromium subsystem: content::WebContents audio state
  //   (content/public/browser/web_contents.h)
  enum class AudioState {
    kNone,    // No audio playing — indicator hidden.
    kPlaying, // Audio is playing — speaker icon shown.
    kMuted,   // Tab is muted — muted speaker icon shown.
  };

  // Suspended / sleeping state for sidebar tab items.
  // Mirrors the memory saver "sleeping tab" state.
  // The sidebar projects suspension state from AstraTabFeatures and
  // Chromium's WebContents discard state — it never stores suspension truth.
  //
  // Chromium owner: content::WebContents::IsDiscarded()
  //   (content/public/browser/web_contents.h)
  // Astra owner: AstraMemorySaverService
  //   (astra/browser/astra_memory_saver_service.h)
  enum class SuspendedState {
    kNone,       // Tab is active / not suspended.
    kSuspended,  // Tab is suspended (discarded / sleeping).
  };

  AstraSidebarItemView();
  explicit AstraSidebarItemView(const std::u16string& title,
                                Type type = Type::kCustom);
  AstraSidebarItemView(const AstraSidebarItemView&) = delete;
  AstraSidebarItemView& operator=(const AstraSidebarItemView&) = delete;
  ~AstraSidebarItemView() override;

  // -- Type ----------------------------------------------------------------

  // Get the item type (determines visual styling).
  Type type() const { return type_; }
  // Set the item type (updates visual styling).
  void SetType(Type type);

  // -- Title ---------------------------------------------------------------

  // Set the primary title text displayed in the item.
  virtual void SetTitle(const std::u16string& title);
  // Get the current title text.
  std::u16string GetTitle() const;
  // Alias for GetTitle (backward compatibility).
  std::u16string GetText() const { return GetTitle(); }

  // -- Tooltip -------------------------------------------------------------

  // Set the tooltip text shown when hovering over the item.
  void SetTooltip(const std::u16string& tooltip);
  // Get the current tooltip text.
  std::u16string GetTooltip() const;

  // -- Selection state -----------------------------------------------------

  // Set whether this item is selected (e.g. keyboard navigation highlight).
  virtual void SetSelected(bool selected);
  bool IsSelected() const { return is_selected_; }

  // -- Hover state ---------------------------------------------------------

  // Set whether this item is currently hovered.
  // Note: This is also set automatically by mouse enter/leave events.
  void SetHovered(bool hovered);
  bool IsHovered() const { return is_hovered_; }

  // -- Active state --------------------------------------------------------

  // Set whether this item is active (e.g. current tab, open workspace).
  virtual void SetActive(bool active);
  bool IsActive() const { return is_active_; }

  // -- Enabled state -------------------------------------------------------

  // Set whether this item is enabled (responds to user interaction).
  void SetEnabled(bool enabled);
  bool IsEnabled() const { return is_enabled_; }

  // -- Context menu --------------------------------------------------------

  // Set whether right-clicking shows a context menu for this item.
  void SetContextMenuEnabled(bool enabled);
  bool IsContextMenuEnabled() const { return context_menu_enabled_; }

  // -- Drag and drop -------------------------------------------------------

  // Set whether this item can be dragged. Defaults to false.
  void SetDragEnabled(bool enabled);
  bool IsDragEnabled() const { return drag_enabled_; }

  // Alias for SetDragEnabled/IsDragEnabled (backward compatibility).
  void SetDraggable(bool draggable) { SetDragEnabled(draggable); }
  bool IsDraggable() const { return IsDragEnabled(); }

  // Set whether this item is currently the drop target.
  void SetDropTarget(bool is_target);
  bool IsDropTarget() const { return is_drop_target_; }

  // Set whether a drag handle is shown on the item.
  void SetShowDragHandle(bool show);
  bool GetShowDragHandle() const { return show_drag_handle_; }

  // Set the drag delegate. Not owned by this view.
  void set_drag_delegate(AstraSidebarItemDragDelegate* delegate) {
    drag_delegate_ = delegate;
  }

  // -- Hover / peek --------------------------------------------------------

  // Set the hover delegate. Not owned by this view.
  // The hover delegate is notified when the mouse enters/leaves the item,
  // and can trigger peek/preview popups.
  void set_hover_delegate(AstraSidebarItemHoverDelegate* delegate) {
    hover_delegate_ = delegate;
  }
  AstraSidebarItemHoverDelegate* hover_delegate() const {
    return hover_delegate_;
  }

  // -- Context menu delegate -----------------------------------------------

  void set_context_menu_delegate(AstraSidebarItemContextMenuDelegate* delegate) {
    context_menu_delegate_ = delegate;
  }

  // -- Leading icon --------------------------------------------------------

  // Set the leading (left-side) icon.
  void SetIcon(const gfx::ImageSkia& icon);
  // Clear the leading icon (hides it).
  void ClearIcon();
  // Get the leading icon view for subclasses that need to customize it.
  views::ImageView* icon_view() { return icon_view_; }

  // -- Trailing icon -------------------------------------------------------

  // Set the trailing (right-side) icon.
  void SetTrailingIcon(const gfx::ImageSkia& icon);
  // Show or hide the trailing icon.
  void ShowTrailingIcon(bool show);
  // Get the trailing icon view.
  views::ImageView* trailing_icon_view() { return trailing_icon_view_; }

  // -- Secondary text ------------------------------------------------------

  // Set the secondary (subtitle) text displayed below the title.
  void SetSecondaryText(const std::u16string& text);
  // Show or hide the secondary text.
  void ShowSecondaryText(bool show);
  // Get the secondary text label.
  views::Label* secondary_label() { return secondary_label_; }

  // -- Badge ---------------------------------------------------------------

  // Set the badge text (e.g. count, notification).
  void SetBadgeText(const std::u16string& text);
  // Show or hide the badge.
  void ShowBadge(bool show);
  // Get the badge label.
  views::Label* badge_label() { return badge_label_; }

  // -- Chevron -------------------------------------------------------------

  // Set whether the expand/collapse chevron is visible.
  void SetChevronVisible(bool visible);
  bool IsChevronVisible() const { return chevron_visible_; }

  // Set whether the chevron is rotated (expanded state).
  void SetChevronRotated(bool rotated);
  bool IsChevronRotated() const { return chevron_rotated_; }
  // Get the chevron image view.
  views::ImageView* chevron_view() { return chevron_view_; }

  // -- Compact mode --------------------------------------------------------

  // Set whether the item is displayed in compact mode (icon-only).
  // In compact mode, labels and secondary text are hidden and only the
  // icon is shown.
  void SetCompactMode(bool compact);
  bool IsCompactMode() const { return is_compact_; }

  // -- Tooltip preview -----------------------------------------------------

  // Set a detailed tooltip with title and subtitle.
  // The detailed tooltip is shown on hover with a short delay.
  void SetDetailedTooltip(const std::u16string& title,
                          const std::u16string& subtitle = std::u16string());

  // Set whether a rich tooltip preview is shown on hover.
  void SetShowTooltipPreview(bool show);
  bool GetShowTooltipPreview() const { return show_tooltip_preview_; }

  // -- Audio indicator (tab items) -----------------------------------------

  // Set the audio state of the tab this item represents.
  // Updates the visibility and icon of the audio indicator button.
  // Audio state is always read from Chromium's WebContents — the sidebar
  // is a pure projection and never stores mute/audio state.
  //
  // Chromium owner: content::WebContents (content/public/browser/web_contents.h)
  void SetAudioState(AudioState state);
  AudioState audio_state() const { return audio_state_; }

  // Set the callback invoked when the user clicks the audio indicator button.
  // The callback should toggle the tab's mute state via WebContents::SetAudioMuted.
  using AudioToggleCallback = base::RepeatingClosure;
  void set_audio_toggle_callback(AudioToggleCallback callback) {
    audio_toggle_callback_ = std::move(callback);
  }

  // -- Primary click callback ----------------------------------------------

  // Set the callback invoked when the item is clicked (primary action).
  using ClickCallback = base::RepeatingClosure;
  void SetCallback(ClickCallback callback) {
    click_callback_ = std::move(callback);
  }

  // -- Suspended state (tab items) -----------------------------------------

  // Set the suspended state of the tab this item represents.
  //
  // Suspended state is always read from AstraMemorySaverService +
  // AstraTabFeatures — the sidebar is a pure projection and never
  // stores suspension truth.
  //
  // Chromium owner: content::WebContents::IsDiscarded()
  void SetSuspendedState(SuspendedState state);
  SuspendedState suspended_state() const { return suspended_state_; }

  // -- Tab metadata --------------------------------------------------------
  //
  // These are convenience setters/getters for attaching metadata to the
  // item view. The actual truth source is services and TabStripModel —
  // these are just cached on the view for drag-and-drop lookups.

  // TabStripModel index of the tab this item represents. -1 if not a tab.
  void set_tab_index(int index) { tab_index_ = index; }
  int tab_index() const { return tab_index_; }

  // Workspace ID associated with this item.
  void set_workspace_id(const std::string& id) { workspace_id_ = id; }
  const std::string& workspace_id() const { return workspace_id_; }

  // Favorite folder ID associated with this item.
  void set_favorite_folder_id(const std::string& id) {
    favorite_folder_id_ = id;
  }
  const std::string& favorite_folder_id() const {
    return favorite_folder_id_;
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
  bool OnMouseMoved(const ui::MouseEvent& event) override;
  bool OnKeyPressed(const ui::KeyEvent& event) override;

 protected:
  // Called when the item is clicked (primary action).
  // Subclasses should override to handle the click.
  virtual void OnItemClicked();

  // Called when the item is right-clicked (context menu).
  // Subclasses can override to show a context menu.
  virtual void ShowContextMenu(const gfx::Point& screen_point);

  // Update the visual appearance when state changes.
  // Called after state changes that affect visuals (selection, hover, active).
  virtual void UpdateVisuals();

  // Update the tooltip from current state.
  virtual void UpdateTooltip();

  // Build the common layout. Called once from constructor.
  // Subclasses can override to add additional child views.
  virtual void BuildLayout();

  // Get the text container (for subclasses to add to).
  views::View* text_container() { return text_container_; }

  // Get the trailing container (for subclasses to add to).
  views::View* trailing_container() { return trailing_container_; }

  // Get the title label (for subclasses to customize).
  views::Label* title_label() { return title_label_; }

 private:
  // Handler for audio button clicks — forwards to the audio toggle callback.
  void OnAudioButtonClicked();

  // Update the audio button's icon and tooltip based on the current audio state.
  void UpdateAudioButtonVisuals();

  // Child views (owned by the view hierarchy).
  raw_ptr<views::ImageView> icon_view_ = nullptr;       // Leading icon
  raw_ptr<views::ImageView> drag_handle_view_ = nullptr;  // Drag handle
  raw_ptr<views::View> text_container_ = nullptr;        // Title + secondary
  raw_ptr<views::Label> title_label_ = nullptr;          // Primary text
  raw_ptr<views::Label> secondary_label_ = nullptr;      // Subtitle text
  raw_ptr<views::View> trailing_container_ = nullptr;    // Right-side elements
  raw_ptr<views::ImageView> trailing_icon_view_ = nullptr; // Trailing icon
  raw_ptr<views::Label> badge_label_ = nullptr;          // Badge / count
  raw_ptr<views::ImageView> chevron_view_ = nullptr;     // Expand/collapse
  raw_ptr<views::ImageButton> audio_button_ = nullptr;   // Audio indicator

  // State flags.
  bool is_selected_ = false;
  bool is_hovered_ = false;
  bool is_active_ = false;
  bool is_enabled_ = true;
  bool context_menu_enabled_ = true;
  bool drag_enabled_ = false;
  bool is_drop_target_ = false;
  bool chevron_visible_ = false;
  bool chevron_rotated_ = false;
  bool show_drag_handle_ = false;
  bool is_compact_ = false;
  bool show_tooltip_preview_ = true;

  // Item type (for visual styling).
  Type type_ = Type::kCustom;

  // Audio / suspended state (tab items).
  AudioState audio_state_ = AudioState::kNone;
  AudioToggleCallback audio_toggle_callback_;
  SuspendedState suspended_state_ = SuspendedState::kNone;

  // Primary click callback.
  ClickCallback click_callback_;

  // Drag state.
  raw_ptr<AstraSidebarItemDragDelegate> drag_delegate_ = nullptr;
  gfx::Point drag_start_point_;
  bool is_dragging_ = false;

  // Hover delegate.
  raw_ptr<AstraSidebarItemHoverDelegate> hover_delegate_ = nullptr;

  // Context menu delegate.
  raw_ptr<AstraSidebarItemContextMenuDelegate> context_menu_delegate_ = nullptr;

  // Cached metadata.
  int tab_index_ = -1;
  std::string workspace_id_;
  std::string favorite_folder_id_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_ITEM_VIEW_H_
