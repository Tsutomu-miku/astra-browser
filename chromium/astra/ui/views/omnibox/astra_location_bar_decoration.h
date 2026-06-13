#ifndef ASTRA_UI_VIEWS_OMNIBOX_ASTRA_LOCATION_BAR_DECORATION_H_
#define ASTRA_UI_VIEWS_OMNIBOX_ASTRA_LOCATION_BAR_DECORATION_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/view.h"

#include "astra/ui/views/omnibox/astra_omnibox_decoration_model.h"

namespace views {
class ImageButton;
class Label;
class MenuRunner;
}  // namespace views

namespace astra {

// =========================================================================
// Astra location bar decoration
// =========================================================================
//
// A decorative view added to Chrome's location bar (omnibox) that provides
// Astra-specific action buttons and status indicators.  The view observes
// AstraOmniboxDecorationModel for state changes and renders accordingly.
//
// Elements:
//   - Workspace indicator: colored dot/badge showing the current workspace
//     accent color.  Clickable — opens the workspace switcher menu.
//   - Focus mode indicator: subtle badge shown only when focus mode is
//     active.  Indicates that the browser is in focus mode.
//   - Action buttons: configurable set of Astra feature buttons
//     (screenshot, note, split view, reading list, translate, share, etc.)
//   - Overflow menu: "..." button that shows hidden actions in a dropdown.
//
// Model/View separation:
//   - AstraOmniboxDecorationModel owns all state and logic.
//   - This view purely renders state and forwards user input.
//   - The view never stores truth — all state comes from the model.
//
// Delegate pattern:
//   AstraLocationBarDecorationView::Delegate is an interface that browser-
//   level code implements to receive action events.  This keeps the views
//   layer decoupled from browser command infrastructure.
//
// Chromium owner: LocationBarView (chrome/browser/ui/views/location_bar/)
// and IconLabelBubbleView / location bar decoration patterns.
//
// TODO(astra): Integrate with LocationBarView via a patch to
// chrome/browser/ui/views/location_bar/location_bar_view.cc — add the
// Astra decoration to the leading or trailing decoration row.
//
// TODO(astra): Consider using views::MdTextButton or IconLabelBubbleView
// as the base class instead of plain views::View.
// =========================================================================

class AstraLocationBarDecorationView
    : public views::View,
      public AstraOmniboxDecorationModelObserver {
 public:
  // Which edge of the location bar the decoration appears on.
  enum class Edge {
    kLeading,   // Left side (before the security icon / URL).
    kTrailing,  // Right side (after the star / other icons).
  };

  // Delegate interface for events from the location bar decoration.
  // Implemented by browser-level code (e.g. AstraBrowserView) to handle
  // user interactions without the views layer depending on browser
  // command infrastructure.
  class Delegate {
   public:
    // Called when the workspace indicator is clicked.
    virtual void OnWorkspaceIndicatorClicked() = 0;

    // Called when an action button is clicked.
    // |action_id| identifies which action was triggered.
    virtual void OnActionClicked(const std::string& action_id) = 0;

    // Called when the overflow menu button is clicked.
    virtual void OnOverflowMenuClicked() = 0;

   protected:
    ~Delegate() = default;
  };

  explicit AstraLocationBarDecorationView(Edge edge = Edge::kLeading);
  ~AstraLocationBarDecorationView() override;

  AstraLocationBarDecorationView(const AstraLocationBarDecorationView&) =
      delete;
  AstraLocationBarDecorationView& operator=(
      const AstraLocationBarDecorationView&) = delete;

  // -- Model binding -------------------------------------------------------

  // Sets the model that this view observes.  Pass nullptr to disconnect.
  // The view does not take ownership of the model.
  void SetModel(AstraOmniboxDecorationModel* model);

  // Returns the model currently bound to this view.
  AstraOmniboxDecorationModel* model() const { return model_; }

  // -- State update (convenience setters that forward to the model) -------

  // Updates the workspace indicator.  Wraps model state projection.
  void UpdateWorkspace(const std::string& workspace_name,
                       const std::string& accent_color);

  // Sets whether focus mode is active.
  void SetFocusModeActive(bool active);

  // Sets whether the decoration is visible.
  void SetDecorationVisible(bool visible);

  // Sets the delegate for action callbacks.
  void SetDelegate(Delegate* delegate) { delegate_ = delegate; }

  // -- views::View: ------------------------------------------------------

  gfx::Size CalculatePreferredSize() const override;
  void Layout() override;
  void OnThemeChanged() override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

  // -- AstraOmniboxDecorationModelObserver: -------------------------------

  void OnActionAdded(const std::string& action_id) override;
  void OnActionRemoved(const std::string& action_id) override;
  void OnActionVisibilityChanged(const std::string& action_id,
                                 bool visible) override;
  void OnActionOrderChanged() override;
  void OnDecorationSettingsChanged() override;
  void OnOmniboxFocusChanged(bool focused) override;
  void OnSecurityStateChanged(AstraSecurityLevel level) override;

  // Accessors for testing / inspection.
  bool focus_mode_active() const { return focus_mode_active_; }
  Edge edge() const { return edge_; }
  bool is_hovered() const { return is_hovered_; }
  size_t action_button_count() const { return action_buttons_.size(); }
  bool has_overflow_button() const { return overflow_button_ != nullptr; }

  // Returns the action button for the given ID, or nullptr if not found.
  views::LabelButton* GetActionButtonForTest(const std::string& action_id);

 private:
  // -- Inner classes -------------------------------------------------------

  // A clickable colored dot that serves as the workspace indicator.
  // Supports hover and active states with proper feedback.
  class WorkspaceIndicatorButton : public views::View {
   public:
    explicit WorkspaceIndicatorButton(SkColor color);
    ~WorkspaceIndicatorButton() override = default;

    void SetColor(SkColor color);
    void SetWorkspaceName(const std::u16string& name);
    void SetBadgeVisible(bool visible);
    bool badge_visible() const { return badge_visible_; }

    void SetCallback(base::RepeatingClosure callback) {
      pressed_callback_ = std::move(callback);
    }

    // views::View:
    void OnPaint(gfx::Canvas* canvas) override;
    bool OnMousePressed(const ui::MouseEvent& event) override;
    void OnMouseReleased(const ui::MouseEvent& event) override;
    void OnMouseEntered(const ui::MouseEvent& event) override;
    void OnMouseExited(const ui::MouseEvent& event) override;
    void OnGestureEvent(ui::GestureEvent* event) override;
    void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
    std::u16string GetTooltipText(const gfx::Point& p) const override;
    void OnThemeChanged() override;

   private:
    void UpdateStateVisuals();

    SkColor color_;
    std::u16string workspace_name_;
    bool hovered_ = false;
    bool pressed_ = false;
    bool badge_visible_ = true;
    base::RepeatingClosure pressed_callback_;
  };

  // A small badge view indicating focus mode is active.
  class FocusModeBadge : public views::View {
   public:
    FocusModeBadge();
    ~FocusModeBadge() override = default;

    // views::View:
    void OnPaint(gfx::Canvas* canvas) override;
    void OnThemeChanged() override;
    void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
    std::u16string GetTooltipText(const gfx::Point& p) const override;
  };

  // -- Private methods -----------------------------------------------------

  // Rebuilds all action buttons from the current model state.
  void RebuildActionButtons();

  // Updates the visual state of all child views.
  void UpdateVisuals();

  // Updates visibility of all child views based on model state.
  void UpdateVisibility();

  // Updates the layout for the current button style.
  void UpdateButtonStyles();

  // Called when the workspace indicator is clicked.
  void OnWorkspaceIndicatorPressed();

  // Called when an action button is clicked.
  void OnActionButtonPressed(const std::string& action_id);

  // Called when the overflow menu button is clicked.
  void OnOverflowButtonPressed();

  // Creates a single action button view for the given action.
  std::unique_ptr<views::LabelButton> CreateActionButton(
      const AstraDecorationAction& action);

  // Returns true if the decoration should be visible given the current state.
  bool ShouldBeVisible() const;

  // Returns the number of actions that should be shown directly
  // (not in the overflow menu).
  int GetDirectActionCount() const;

  // -- Data ----------------------------------------------------------------

  // Edge of the location bar this decoration appears on.
  Edge edge_;

  // Delegate for action callbacks (not owned).
  raw_ptr<Delegate> delegate_ = nullptr;

  // Model for state and logic (not owned).
  raw_ptr<AstraOmniboxDecorationModel> model_ = nullptr;

  // Current workspace display name (for tooltip / accessible name).
  std::u16string workspace_name_;

  // Current workspace accent color (hex string, e.g. "#5AD8A6").
  std::string accent_color_;

  // Whether focus mode is currently active.
  bool focus_mode_active_ = false;

  // Whether the mouse is currently hovering over the decoration.
  bool is_hovered_ = false;

  // Child views (owned by the view hierarchy).
  raw_ptr<WorkspaceIndicatorButton> workspace_indicator_ = nullptr;
  raw_ptr<FocusModeBadge> focus_mode_badge_ = nullptr;
  raw_ptr<views::LabelButton> overflow_button_ = nullptr;

  // Action button views in order, mapped by action ID.
  // Stored as a vector to preserve ordering.
  std::vector<std::pair<std::string, raw_ptr<views::LabelButton>>>
      action_buttons_;

  // -- Constants -----------------------------------------------------------

  // The diameter of the workspace indicator dot in dp.
  static constexpr int kDotSize = 10;

  // Size of the workspace indicator button (click target).
  static constexpr int kWorkspaceButtonSize = 24;

  // Size of the focus mode badge.
  static constexpr int kFocusBadgeSize = 16;

  // Default size of action buttons.
  static constexpr int kDefaultActionButtonSize = 28;

  // Spacing between decoration elements.
  static constexpr int kElementSpacing = 4;

  // Horizontal padding around the entire decoration.
  static constexpr int kHorizontalPadding = 4;

  // Height of the decoration.
  static constexpr int kDecorationHeight = 32;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_OMNIBOX_ASTRA_LOCATION_BAR_DECORATION_H_
