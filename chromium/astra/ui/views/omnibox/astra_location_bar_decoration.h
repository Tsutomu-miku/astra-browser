#ifndef ASTRA_UI_VIEWS_OMNIBOX_ASTRA_LOCATION_BAR_DECORATION_H_
#define ASTRA_UI_VIEWS_OMNIBOX_ASTRA_LOCATION_BAR_DECORATION_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/view.h"

#include "astra/ui/views/omnibox/astra_omnibox_decoration_model.h"

namespace views {
class ImageButton;
class Label;
class BubbleDialogModelHost;
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
//     accent color.  Clickable — opens the workspace switcher bubble.
//   - Focus mode indicator: subtle badge shown only when focus mode is
//     active.  Indicates that the browser is in focus mode.
//   - Tab stack indicator: shows tab stack membership.
//   - Action buttons: configurable set of Astra feature buttons
//     (reading list, note, favorite, sidebar, split view, translate, etc.)
//
// Model/View separation:
//   - AstraOmniboxDecorationModel owns all state and logic.
//   - This view purely renders state and forwards user input.
//   - The view never stores truth — all state comes from the model.
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
      public AstraOmniboxDecorationObserver {
 public:
  // Delegate interface for events from the location bar decoration.
  // Implemented by browser-level code (e.g. AstraBrowserView) to handle
  // user interactions without the views layer depending on browser
  // command infrastructure.
  class Delegate {
   public:
    // Called when a decoration is clicked.
    // |type| identifies which decoration was triggered.
    virtual void OnDecorationClicked(AstraOmniboxDecorationType type) = 0;

    // Called when a decoration bubble should be shown.
    virtual void ShowBubbleForDecoration(AstraOmniboxDecorationType type) = 0;

   protected:
    ~Delegate() = default;
  };

  explicit AstraLocationBarDecorationView(
      AstraDecorationPosition position = AstraDecorationPosition::kLeading);
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
  AstraOmniboxDecorationModel* GetModel() const { return model_; }

  // -- Decoration view access ----------------------------------------------

  // Returns the button view for a specific decoration type, or nullptr
  // if the decoration doesn't have a button or is not found.
  views::View* GetDecorationView(AstraOmniboxDecorationType type);

  // Returns the number of decoration buttons currently in the view.
  int GetDecorationCount() const;

  // -- Position ----------------------------------------------------------

  // Sets which side of the omnibox the decoration appears on.
  void SetPosition(AstraDecorationPosition position);

  // Returns the current position.
  AstraDecorationPosition GetPosition() const { return position_; }

  // -- Compact mode --------------------------------------------------------

  // Sets whether compact mode is enabled (smaller decorations, tighter
  // spacing).
  void SetCompactMode(bool compact);

  // Returns whether compact mode is enabled.
  bool GetCompactMode() const { return compact_mode_; }

  // -- Icon size -----------------------------------------------------------

  // Sets the decoration icon size in pixels.
  void SetIconSize(int size_px);

  // Returns the decoration icon size in pixels.
  int GetIconSize() const { return icon_size_px_; }

  // -- Animation -----------------------------------------------------------

  // Sets whether animations are enabled for decoration state changes.
  void SetAnimationEnabled(bool enabled);

  // Returns whether animations are enabled.
  bool GetAnimationEnabled() const { return animation_enabled_; }

  // -- Spacing -------------------------------------------------------------

  // Sets the spacing between decoration elements in pixels.
  void SetSpacing(int spacing_px);

  // Returns the spacing between decoration elements in pixels.
  int GetSpacing() const { return spacing_px_; }

  // -- Bulk update ---------------------------------------------------------

  // Refreshes all decoration views from the current model state.
  void UpdateAllDecorations();

  // -- Bubble management ---------------------------------------------------

  // Shows the bubble for a specific decoration.
  // Delegates to the Delegate if set.
  void ShowBubbleForDecoration(AstraOmniboxDecorationType type);

  // Hides all open bubbles.
  void HideAllBubbles();

  // Returns the type of the currently open bubble, or kNone.
  AstraOmniboxDecorationType GetOpenBubbleType() const;

  // -- Delegate ------------------------------------------------------------

  // Sets the delegate for action callbacks.
  void SetDelegate(Delegate* delegate) { delegate_ = delegate; }

  // -- views::View: --------------------------------------------------------

  gfx::Size CalculatePreferredSize() const override;
  void Layout() override;
  void OnThemeChanged() override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

  // -- AstraOmniboxDecorationObserver: -------------------------------------

  void OnDecorationVisibilityChanged(
      AstraOmniboxDecorationModel* model,
      AstraOmniboxDecorationType type,
      bool visible) override;
  void OnDecorationActiveChanged(
      AstraOmniboxDecorationModel* model,
      AstraOmniboxDecorationType type,
      bool active) override;
  void OnDecorationBadgeChanged(
      AstraOmniboxDecorationModel* model,
      AstraOmniboxDecorationType type) override;
  void OnDecorationsReordered(
      AstraOmniboxDecorationModel* model) override;
  void OnDecorationExecuted(
      AstraOmniboxDecorationModel* model,
      AstraOmniboxDecorationType type) override;
  void OnBubbleShown(
      AstraOmniboxDecorationModel* model,
      AstraOmniboxDecorationType type) override;
  void OnBubbleHidden(
      AstraOmniboxDecorationModel* model,
      AstraOmniboxDecorationType type) override;
  void OnWorkspaceChanged(
      AstraOmniboxDecorationModel* model,
      const std::u16string& name) override;
  void OnFocusModeChanged(
      AstraOmniboxDecorationModel* model,
      bool active) override;
  void OnOmniboxDecorationModelShutdown(
      AstraOmniboxDecorationModel* model) override;

  // -- Test accessors ------------------------------------------------------

  // Returns whether the mouse is currently hovering over the decoration.
  bool is_hovered() const { return is_hovered_; }

  // Returns the workspace indicator button for testing.
  views::View* workspace_indicator_for_test() { return workspace_indicator_; }

  // Returns the focus mode badge for testing.
  views::View* focus_mode_badge_for_test() { return focus_mode_badge_; }

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
    void SetBadgeText(const std::u16string& text, SkColor color);
    void ClearBadge();

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
    bool has_badge_ = false;
    std::u16string badge_text_;
    SkColor badge_color_ = SK_ColorRED;
    base::RepeatingClosure pressed_callback_;
  };

  // A small badge view indicating focus mode is active.
  class FocusModeBadge : public views::View {
   public:
    FocusModeBadge();
    ~FocusModeBadge() override = default;

    void SetActive(bool active);
    bool active() const { return active_; }

    // views::View:
    void OnPaint(gfx::Canvas* canvas) override;
    void OnThemeChanged() override;
    void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
    std::u16string GetTooltipText(const gfx::Point& p) const override;

   private:
    bool active_ = false;
  };

  // A decoration button view that shows an icon with optional badge.
  class DecorationButton : public views::LabelButton {
   public:
    DecorationButton(base::RepeatingClosure callback,
                    AstraOmniboxDecorationType type);
    ~DecorationButton() override = default;

    void SetType(AstraOmniboxDecorationType type) { type_ = type; }
    AstraOmniboxDecorationType type() const { return type_; }

    void SetBadge(const std::u16string& text, SkColor color);
    void ClearBadge();
    bool has_badge() const { return has_badge_; }

    void SetActive(bool active);
    bool active() const { return active_; }

    // views::LabelButton:
    void OnPaint(gfx::Canvas* canvas) override;
    void OnThemeChanged() override;

   private:
    AstraOmniboxDecorationType type_;
    bool has_badge_ = false;
    std::u16string badge_text_;
    SkColor badge_color_ = SK_ColorRED;
    bool active_ = false;
  };

  // -- Private methods -----------------------------------------------------

  // Rebuilds all decoration buttons from the current model state.
  void RebuildDecorationButtons();

  // Updates the visual state of all child views from the model.
  void UpdateVisuals();

  // Updates visibility of all child views based on model state.
  void UpdateVisibility();

  // Updates the layout for current settings.
  void UpdateLayout();

  // Called when a decoration button is pressed.
  void OnDecorationButtonPressed(AstraOmniboxDecorationType type);

  // Called when the workspace indicator is pressed.
  void OnWorkspaceIndicatorPressed();

  // Creates a decoration button for the given item.
  std::unique_ptr<DecorationButton> CreateDecorationButton(
      const AstraOmniboxDecorationItem& item);

  // Returns the effective icon size (considering compact mode).
  int GetEffectiveIconSize() const;

  // Returns the effective spacing (considering compact mode).
  int GetEffectiveSpacing() const;

  // -- Data ----------------------------------------------------------------

  // Which side of the location bar this decoration appears on.
  AstraDecorationPosition position_;

  // Delegate for action callbacks (not owned).
  raw_ptr<Delegate> delegate_ = nullptr;

  // Model for state and logic (not owned).
  raw_ptr<AstraOmniboxDecorationModel> model_ = nullptr;

  // Whether the mouse is currently hovering over the decoration.
  bool is_hovered_ = false;

  // Whether compact mode is enabled.
  bool compact_mode_ = false;

  // Icon size in pixels (before compact mode adjustment).
  int icon_size_px_ = 20;

  // Whether animations are enabled.
  bool animation_enabled_ = true;

  // Spacing between elements in pixels (before compact mode adjustment).
  int spacing_px_ = 4;

  // The currently "open" bubble type (tracked locally for the view).
  AstraOmniboxDecorationType open_bubble_type_ =
      AstraOmniboxDecorationType::kNone;

  // Child views (owned by the view hierarchy).
  raw_ptr<WorkspaceIndicatorButton> workspace_indicator_ = nullptr;
  raw_ptr<FocusModeBadge> focus_mode_badge_ = nullptr;

  // Decoration button views in order, mapped by type.
  // Stored as a vector to preserve ordering.
  std::vector<std::pair<AstraOmniboxDecorationType, raw_ptr<DecorationButton>>>
      decoration_buttons_;

  // -- Constants -----------------------------------------------------------

  // The diameter of the workspace indicator dot in dp.
  static constexpr int kDotSize = 10;

  // Default size of the workspace indicator button (click target).
  static constexpr int kWorkspaceButtonSize = 24;

  // Default size of the focus mode badge.
  static constexpr int kFocusBadgeSize = 16;

  // Default size of decoration buttons.
  static constexpr int kDefaultButtonSize = 28;

  // Default height of the decoration bar.
  static constexpr int kDecorationHeight = 32;

  // Horizontal padding around the entire decoration.
  static constexpr int kHorizontalPadding = 4;

  // Compact mode scaling factor.
  static constexpr float kCompactScaleFactor = 0.8f;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_OMNIBOX_ASTRA_LOCATION_BAR_DECORATION_H_
