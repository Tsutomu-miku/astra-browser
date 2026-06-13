#ifndef ASTRA_UI_VIEWS_NEWTAB_ASTRA_NTP_SHORTCUT_VIEW_H_
#define ASTRA_UI_VIEWS_NEWTAB_ASTRA_NTP_SHORTCUT_VIEW_H_

#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/view.h"

namespace astra {

// Size presets for the shortcut tile.
enum class AstraNtpShortcutSize {
  kSmall,   // Small icon + no title
  kMedium,  // Standard icon + title (default)
  kLarge,   // Large icon + title
};

// =========================================================================
// AstraNtpShortcutView — individual shortcut tile on the new tab page
// =========================================================================
//
// A single shortcut tile on the Astra new tab page. Shows a site icon +
// site name label below it.  Clicking the tile navigates to the URL.
// Hovering reveals a remove button, drag handle, and triggers a subtle
// scale + shadow animation.  The tile supports keyboard focus, activation,
// and drag-and-drop reordering.
//
// This is a presentation-only view.  All data (title, URL, icon) is
// pushed in by the parent view / controller.  The tile never reads
// from services directly — it receives data from its parent.
//
// Visual layout (medium size, default):
//   +-------------+   ← tile bounds
//   |⋮ [remove]   |   ← drag handle (left) + remove button (right)
//   |             |
//   |   (icon)    |   ← circular/rounded square icon (favicon)
//   |             |
//   +-------------+
//   |  Site Name |   ← label below icon
//   +-------------+
//
// States:
//   - Default: white background, subtle border.
//   - Hover: lighter background, drop shadow, slight scale-up.
//   - Focus: focus ring around the tile.
//   - Pressed: darker background, slight scale-down.
//   - Dragging: semi-transparent, elevated shadow.
//
// Chromium owner: NTPTile, MostVisitedTile
//   (chrome/browser/ui/webui/new_tab_page/most_visited_handler.h)
//
// TODO(astra): Add context menu (remove, edit, etc.) similar to
// Chrome's NTP tiles.
// TODO(astra): Replace placeholder icon with real favicons from Chromium's
// favicon service.
// =========================================================================

class AstraNtpShortcutView : public views::View {
 public:
  // Callback type for when the shortcut is clicked.
  // The callback receives the URL of the shortcut.
  using ClickCallback = base::RepeatingCallback<void(const GURL&)>;

  // Callback type for when the remove button is pressed.
  using RemoveCallback = base::RepeatingCallback<void(const GURL&)>;

  // Callback type for when the context menu is requested.
  // Parameters: URL, screen point of the context menu anchor.
  using ContextMenuCallback =
      base::RepeatingCallback<void(const GURL&, const gfx::Point&)>;

  // Callback type for when the edit action (rename.
  using EditCallback = base::RepeatingCallback<void(const GURL&)>;

  // Callback type for drag start (initiates drag-and-drop reordering).
  using DragStartCallback =
      base::RepeatingCallback<void(AstraNtpShortcutView*)>;

  AstraNtpShortcutView();
  explicit AstraNtpShortcutView(AstraNtpShortcutSize size);
  AstraNtpShortcutView(const AstraNtpShortcutView&) = delete;
  AstraNtpShortcutView& operator=(const AstraNtpShortcutView&) = delete;
  ~AstraNtpShortcutView() override;

  // -- Data setters (called by parent view / controller)

  // Sets the display title shown below the icon.
  void SetTitle(const std::u16string& title);

  // Gets the current title.
  const std::u16string& title() const { return title_; }

  // Sets the URL the shortcut navigates to.
  void SetURL(const GURL& url);

  // Gets the current URL.
  const GURL& url() const { return url_; }

  // Sets the icon / favicon URL.
  void SetIconURL(const GURL& icon_url);

  // Sets the size preset (small/medium/large).
  void SetSize(AstraNtpShortcutSize size);

  // Gets the current size.
  AstraNtpShortcutSize size() const { return size_; }

  // Sets whether the drag handle is visible.
  void SetShowDragHandle(bool show);

  // Sets whether the shortcut is in edit mode (show title becomes editable textfield.
  void SetEditMode(bool edit_mode);

  // Returns whether the shortcut is in edit mode.
  bool is_edit_mode() const { return is_edit_mode_; }

  // -- Special tile modes --

  // Sets this as an "add shortcut" special tile (+ icon).
  void SetIsAddShortcutTile(bool is_add_tile);
  bool is_add_shortcut_tile() const { return is_add_shortcut_tile_; }

  // -- Badge --

  // Sets a badge count (0 hides the badge).
  void SetBadgeCount(int count);
  int badge_count() const { return badge_count_; }

  // -- States --

  // Sets loading state (shows skeleton/placeholder).
  void SetLoading(bool loading);
  bool is_loading() const { return is_loading_; }

  // Sets error state (shows broken icon indicator).
  void SetErrorState(bool error);
  bool has_error() const { return has_error_; }

  // -- Callbacks

  void SetClickCallback(ClickCallback callback);
  void SetRemoveCallback(RemoveCallback callback);
  void SetContextMenuCallback(ContextMenuCallback callback);
  void SetEditCallback(EditCallback callback);
  void SetDragStartCallback(DragStartCallback callback);

  // -- views::View

  bool OnMousePressed(const ui::MouseEvent& event) override;
  bool OnMouseDragged(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  bool OnKeyPressed(const ui::KeyEvent& event) override;
  bool OnKeyReleased(const ui::KeyEvent& event) override;
  void OnFocus() override;
  void OnBlur() override;
  void OnPaintBackground(gfx::Canvas* canvas) override;
  void OnPaint(gfx::Canvas* canvas) override;
  void OnThemeChanged() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  void Layout() override;
  gfx::Size CalculatePreferredSize() const override;
  bool OnGestureEvent(ui::GestureEvent* event) override;

 private:
  // Build the child views and layout.  Called once from constructor.
  void BuildLayout();

  // Updates the visual state based on hover, focus, pressed flags.
  void UpdateVisualState();

  // Updates the visibility of the remove button and drag handle.
  void UpdateControlVisibility();

  // Paints the focus ring around the tile.
  void PaintFocusRing(gfx::Canvas* canvas);

  // Paints the icon placeholder (colored circle with first letter).
  void PaintIcon(gfx::Canvas* canvas);

  // Paints the drag handle (three vertical dots / grabber).
  void PaintDragHandle(gfx::Canvas* canvas);

  // Handler for the remove button.
  void OnRemoveButtonPressed();

  // Handler for the edit commit (when user presses Enter in edit mode).
  void OnEditCommitted();

  // Shows the context menu at the current mouse position.
  void ShowContextMenu(const gfx::Point& screen_point);

  // Starts a drag operation.
  void StartDrag(const ui::LocatedEvent& event);

  // Builds the "add shortcut" tile variant.
  void BuildAddShortcutLayout();

  // Paints the badge.
  void PaintBadge(gfx::Canvas* canvas);

  // Paints loading skeleton.
  void PaintLoadingSkeleton(gfx::Canvas* canvas);

  // Paints error state indicator.
  void PaintErrorIndicator(gfx::Canvas* canvas);

  // Calculates icon size based on current size preset.
  int GetIconSize() const;

  // Calculates tile size based on current size preset.
  gfx::Size GetTileSize() const;

  // Data state (pushed in by parent).
  std::u16string title_;
  GURL url_;
  GURL icon_url_;
  AstraNtpShortcutSize size_ = AstraNtpShortcutSize::kMedium;
  bool is_hovered_ = false;
  bool is_pressed_ = false;
  bool is_focused_ = false;
  bool is_dragging_ = false;
  bool is_edit_mode_ = false;
  bool show_drag_handle_ = false;

  // Special tile modes.
  bool is_add_shortcut_tile_ = false;

  // Badge state.
  int badge_count_ = 0;

  // Loading / error states.
  bool is_loading_ = false;
  bool has_error_ = false;

  ClickCallback click_callback_;
  RemoveCallback remove_callback_;
  ContextMenuCallback context_menu_callback_;
  EditCallback edit_callback_;
  DragStartCallback drag_start_callback_;

  // Point where drag started (for drag detection.
  gfx::Point drag_start_point_;

  // Child views (owned by view hierarchy).
  raw_ptr<views::View> icon_view_ = nullptr;
  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Textfield> title_textfield_ = nullptr;
  raw_ptr<views::ImageButton> remove_button_ = nullptr;
  raw_ptr<views::View> drag_handle_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_NEWTAB_ASTRA_NTP_SHORTCUT_VIEW_H_
