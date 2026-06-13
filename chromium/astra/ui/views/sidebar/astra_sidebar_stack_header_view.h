#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_STACK_HEADER_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_STACK_HEADER_VIEW_H_

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
//   - A menu button for stack actions (rename, delete, change color)
//   - Hover state highlighting
//
// This view is for named stacks (AstraTabStack).  For hierarchical
// parent/child stacks, see the original AstraSidebarStackHeaderView
// (TODO(astra): rename or consolidate hierarchical stack header view).
//
// Truth source:
//   - Stack name/color/order: AstraTabStackService
//   - Tab count: derived from tabs with this stack_id in AstraTabFeatures
//   - Collapsed/expanded state: AstraTabStackService (persisted)
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

  // Update the displayed stack name.
  void SetTitle(const std::u16string& title);

  // Set whether this stack is expanded (children visible) or collapsed.
  void SetExpanded(bool expanded);
  bool IsExpanded() const { return is_expanded_; }

  // Set this stack header as active (one of its tabs is the active tab).
  void SetActive(bool active);
  bool IsActive() const { return is_active_; }

  // Set the number of tabs in this stack.
  // Shown as a badge on the trailing edge.
  void SetChildCount(size_t count);
  size_t child_count() const { return child_count_; }

  // Set the accent color for this stack (hex string, e.g. "#5B8FF9").
  // Used for the left-edge color bar.
  void SetAccentColor(const std::string& color_hex);
  const std::string& accent_color() const { return accent_color_; }

  // Set the delegate for stack header actions. Not owned.
  void set_delegate(AstraSidebarStackHeaderDelegate* delegate) {
    delegate_ = delegate;
  }

  // -- Stack metadata -----------------------------------------------------

  // ID of the named stack this header represents.
  void set_stack_id(const std::string& stack_id) { stack_id_ = stack_id; }
  const std::string& stack_id() const { return stack_id_; }

  // -- TabStripModel index (for hierarchical stacks) ----------------------
  //
  // For hierarchical (parent-child) stacks, this is the TabStripModel index
  // of the parent tab.  For named stacks, this is unused.
  //
  // TODO(astra): Remove tab_index_ when hierarchical stacks are fully
  //   migrated to named stacks or have their own dedicated header view.
  void set_tab_index(int index) { tab_index_ = index; }
  int tab_index() const { return tab_index_; }

  // -- views::View --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void Layout() override;
  void OnThemeChanged() override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

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

  // Updates the child count label text and visibility.
  void UpdateChildCountBadge();

  // Updates the color accent bar on the left edge.
  void UpdateAccentColorBar();

  // Parses a hex color string (e.g. "#RRGGBB") to an SkColor.
  // Returns SK_ColorGRAY on failure.
  static SkColor ParseHexColor(const std::string& hex);

  // Layout constants.
  static constexpr int kStackHeaderHeight = 32;
  static constexpr int kStackHeaderHorizontalPadding = 12;
  static constexpr int kStackHeaderIconSpacing = 8;
  static constexpr int kExpandArrowSize = 12;
  static constexpr int kStackHeaderCornerRadius = 6;

  // Width of the color accent bar on the left edge.
  static constexpr int kAccentColorBarWidth = 3;

  // Spacing between the title and the child count badge.
  static constexpr int kChildCountBadgeSpacing = 8;

  // Child count badge sizing.
  static constexpr int kChildCountBadgeMinWidth = 20;
  static constexpr int kChildCountBadgeHeight = 18;
  static constexpr int kChildCountBadgeCornerRadius = 9;

  // Menu button size.
  static constexpr int kMenuButtonSize = 16;

  raw_ptr<views::ImageButton> expand_button_ = nullptr;
  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> child_count_label_ = nullptr;
  raw_ptr<views::ImageButton> menu_button_ = nullptr;

  // Color accent bar on the left edge (painted as a separate view layer).
  raw_ptr<views::View> accent_color_bar_ = nullptr;

  bool is_expanded_ = true;
  bool is_active_ = false;
  bool is_hovered_ = false;
  size_t child_count_ = 0;

  std::string accent_color_;  // hex string, e.g. "#5B8FF9"
  std::string stack_id_;
  int tab_index_ = -1;  // for hierarchical stacks

  raw_ptr<AstraSidebarStackHeaderDelegate> delegate_ = nullptr;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_STACK_HEADER_VIEW_H_
