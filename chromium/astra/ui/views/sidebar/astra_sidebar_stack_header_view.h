#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_STACK_HEADER_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_STACK_HEADER_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/view.h"

namespace views {
class ImageButton;
class ImageView;
class Label;
}  // namespace views

namespace astra {

struct AstraStackInfo;

// Delegate interface for AstraSidebarStackHeaderView actions.
// Implemented by the parent sidebar stack view to handle expand/collapse,
// rename, delete, and color change actions for named tab stacks.
//
// The stack header view is presentation-only — it never mutates stack state
// directly.  All user actions are forwarded to the delegate, which routes
// through AstraTabStackService.
//
// Chromium owner: TabGroupHeader (chrome/browser/ui/views/tabs/tab_group_header.h)
class AstraSidebarStackHeaderDelegate {
 public:
  virtual ~AstraSidebarStackHeaderDelegate() = default;

  // Called when the user clicks the expand/collapse arrow on a stack header.
  // |stack_id| identifies the stack.
  virtual void OnStackToggleExpanded(const std::string& stack_id) = 0;

  // Called when the user clicks the stack header title/body (not the arrow
  // or menu button).  This should activate the first tab in the stack or
  // toggle expand state depending on the interaction model.
  // |stack_id| identifies the stack.
  virtual void OnStackHeaderClicked(const std::string& stack_id) = 0;

  // Called when the user clicks the menu button on a stack header.
  // |stack_id| identifies the stack.
  // |anchor_point| is the anchor point for the menu, in screen coordinates.
  virtual void OnStackMenuClicked(const std::string& stack_id,
                                  const gfx::Point& anchor_point) = 0;
};

// A sidebar item that represents a named tab stack header.
//
// The stack header shows:
//   - A color accent bar on the left edge (stack color)
//   - An expand/collapse arrow on the leading edge
//   - The stack name
//   - A child count badge (e.g., "3" for 3 tabs)
//   - An unread indicator dot
//   - A pin indicator (for pinned stacks)
//   - A menu button for stack actions (rename, delete, change color)
//   - Hover background highlight
//   - Selection background
//   - Drag hover highlight
//
// This view is for named stacks (AstraTabStack).  For hierarchical
// parent/child stacks, see the hierarchical stack header view.
//
// Truth source:
//   - Stack name/color/order: AstraTabStackService
//   - Tab count: derived from tabs with this stack_id in AstraTabFeatures
//   - Collapsed/expanded state: AstraTabStackService (persisted)
//   - Unread state: derived from tab unread state
//
// Chromium owner: TabGroupHeader (chrome/browser/ui/views/tabs/tab_group_header.h)
// Chromium pattern: views::View with mouse event handling.
class AstraSidebarStackHeaderView : public views::View {
 public:
  explicit AstraSidebarStackHeaderView(const std::u16string& title);
  AstraSidebarStackHeaderView(const AstraSidebarStackHeaderView&) = delete;
  AstraSidebarStackHeaderView& operator=(
      const AstraSidebarStackHeaderView&) = delete;
  ~AstraSidebarStackHeaderView() override;

  // -- Stack info ---------------------------------------------------------

  // Set all stack info at once.  Updates all visual elements.
  void SetStackInfo(const AstraStackInfo& info);

  // Get the stack ID.
  std::string GetStackId() const { return stack_id_; }

  // -- Name ---------------------------------------------------------------

  // Update the displayed stack name.
  void SetName(const std::u16string& name);
  std::u16string GetName() const;

  // -- Color --------------------------------------------------------------

  // Set the accent color for this stack.
  // Used for the left-edge color bar.
  void SetColor(SkColor color);
  SkColor GetColor() const { return color_; }

  // -- Tab count ----------------------------------------------------------

  // Set the number of tabs in this stack.
  // Shown as a badge on the trailing edge.
  void SetTabCount(int count);
  int GetTabCount() const { return tab_count_; }

  // -- Expansion ----------------------------------------------------------

  // Set whether this stack is expanded (children visible) or collapsed.
  void SetExpanded(bool expanded);
  bool IsExpanded() const { return is_expanded_; }

  // -- Selection ----------------------------------------------------------

  // Set whether this stack is selected.
  void SetSelected(bool selected);
  bool IsSelected() const { return is_selected_; }

  // -- Pinned -------------------------------------------------------------

  // Set whether this stack is pinned.
  void SetPinned(bool pinned);
  bool IsPinned() const { return is_pinned_; }

  // -- Chevron ------------------------------------------------------------

  // Set whether the expand/collapse chevron is shown.
  void SetShowChevron(bool show);
  bool GetShowChevron() const { return show_chevron_; }

  // -- Tab count display --------------------------------------------------

  // Set whether the tab count badge is shown.
  void SetShowTabCount(bool show);
  bool GetShowTabCount() const { return show_tab_count_; }

  // -- Color indicator ----------------------------------------------------

  // Set whether the color indicator bar is shown.
  void SetShowColorIndicator(bool show);
  bool GetShowColorIndicator() const { return show_color_indicator_; }

  // -- Menu button --------------------------------------------------------

  // Set whether the menu button is shown.
  void SetShowMenuButton(bool show);
  bool GetShowMenuButton() const { return show_menu_button_; }

  // -- Unread -------------------------------------------------------------

  // Set whether the stack has unread tabs (shows a dot indicator).
  void SetHasUnread(bool has_unread);
  bool GetHasUnread() const { return has_unread_; }

  // -- Compact mode -------------------------------------------------------

  // Set compact mode (reduced height and padding).
  void SetCompact(bool compact);
  bool IsCompact() const { return is_compact_; }

  // -- Drag hover ---------------------------------------------------------

  // Set whether this header is currently a drag-drop target.
  void SetDragHovered(bool hovered);
  bool IsDragHovered() const { return is_drag_hovered_; }

  // -- Legacy / compatibility ---------------------------------------------
  //
  // These methods are kept for backward compatibility with existing code.
  // New code should use the methods above.
  // TODO(astra): Remove legacy methods once all callers are updated.

  void SetTitle(const std::u16string& title) { SetName(title); }
  void SetAccentColor(const std::string& color_hex);
  const std::string& accent_color() const { return accent_color_hex_; }
  void SetChildCount(size_t count) { SetTabCount(static_cast<int>(count)); }
  size_t child_count() const { return static_cast<size_t>(tab_count_); }
  void SetActive(bool active) { SetSelected(active); }
  bool IsActive() const { return IsSelected(); }

  void set_stack_id(const std::string& stack_id) { stack_id_ = stack_id; }
  const std::string& stack_id() const { return stack_id_; }

  // TabStripModel index (for hierarchical stacks).
  // TODO(astra): Remove tab_index_ when hierarchical stacks are fully
  //   migrated to named stacks or have their own dedicated header view.
  void set_tab_index(int index) { tab_index_ = index; }
  int tab_index() const { return tab_index_; }

  // Set the delegate for stack header actions. Not owned.
  void set_delegate(AstraSidebarStackHeaderDelegate* delegate) {
    delegate_ = delegate;
  }

  // Parse a hex color string (e.g. "#RRGGBB") to an SkColor.
  // Returns SK_ColorGRAY on failure.
  static SkColor ParseHexColor(const std::string& hex);

  // -- views::View --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;
  void OnThemeChanged() override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

 private:
  // Handler for expand/collapse arrow button clicks.
  void OnExpandButtonClicked();

  // Handler for menu button clicks.
  void OnMenuButtonClicked();

  // Updates the expand arrow icon based on current expanded state.
  //
  // TODO(astra): Use real Chromium vector icons for expand/collapse arrows.
  //   Chromium resources: arrow_down / arrow_right in ui/resources/vector_icons/
  //   or views::kTreeViewExpandedIcon / kTreeViewCollapsedIcon.
  void UpdateExpandArrowVisuals();

  // Updates the tab count label text and visibility.
  void UpdateTabCountBadge();

  // Updates the color accent bar on the left edge.
  void UpdateAccentColorBar();

  // Updates the unread indicator dot visibility and color.
  void UpdateUnreadIndicator();

  // Updates the pin indicator visibility.
  void UpdatePinIndicator();

  // Updates background color based on current state.
  void UpdateBackgroundColor();

  // Returns the current header height based on compact mode.
  int GetHeaderHeight() const;

  // Layout constants (normal mode).
  static constexpr int kStackHeaderHeight = 32;
  static constexpr int kStackHeaderCompactHeight = 24;
  static constexpr int kStackHeaderHorizontalPadding = 12;
  static constexpr int kStackHeaderIconSpacing = 8;
  static constexpr int kExpandArrowSize = 12;
  static constexpr int kStackHeaderCornerRadius = 6;

  // Width of the color accent bar on the left edge.
  static constexpr int kAccentColorBarWidth = 3;

  // Spacing between the title and the tab count badge.
  static constexpr int kTabCountBadgeSpacing = 8;

  // Tab count badge sizing.
  static constexpr int kTabCountBadgeMinWidth = 20;
  static constexpr int kTabCountBadgeHeight = 18;
  static constexpr int kTabCountBadgeCornerRadius = 9;

  // Unread indicator size.
  static constexpr int kUnreadIndicatorSize = 8;

  // Pin indicator size.
  static constexpr int kPinIndicatorSize = 12;

  // Menu button size.
  static constexpr int kMenuButtonSize = 16;

  // -- Child views --------------------------------------------------------

  // Color accent bar on the left edge (painted as a separate view layer).
  raw_ptr<views::View> accent_color_bar_ = nullptr;

  // Expand/collapse arrow button on the leading edge.
  raw_ptr<views::ImageButton> expand_button_ = nullptr;

  // Stack name label.
  raw_ptr<views::Label> title_label_ = nullptr;

  // Unread indicator dot.
  raw_ptr<views::View> unread_indicator_ = nullptr;

  // Pin indicator icon.
  raw_ptr<views::ImageView> pin_indicator_ = nullptr;

  // Tab count badge on the trailing edge.
  raw_ptr<views::Label> tab_count_label_ = nullptr;

  // Menu button for stack actions (rename, delete, change color).
  raw_ptr<views::ImageButton> menu_button_ = nullptr;

  // -- State --------------------------------------------------------------

  SkColor color_ = SK_ColorGRAY;
  int tab_count_ = 0;
  bool is_expanded_ = true;
  bool is_selected_ = false;
  bool is_pinned_ = false;
  bool is_hovered_ = false;
  bool is_drag_hovered_ = false;
  bool is_compact_ = false;
  bool has_unread_ = false;

  bool show_chevron_ = true;
  bool show_tab_count_ = true;
  bool show_color_indicator_ = true;
  bool show_menu_button_ = false;  // Default: shown on hover only.

  std::string stack_id_;
  std::string accent_color_hex_;  // Legacy: kept for accent_color() getter.
  int tab_index_ = -1;  // for hierarchical stacks

  raw_ptr<AstraSidebarStackHeaderDelegate> delegate_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_STACK_HEADER_VIEW_H_
