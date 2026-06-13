#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_EXTENSION_ICON_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_EXTENSION_ICON_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/metadata/view_factory.h"

namespace astra {

// States an extension can be in.
// Mirrors extensions::Extension::State but is presentation-facing.
enum class AstraExtensionState {
  kEnabled,
  kDisabled,
  kBlocked,
};

// Delegate interface for AstraExtensionIconView events.
// Implemented by the parent extensions sidebar section.
class AstraExtensionIconDelegate {
 public:
  virtual ~AstraExtensionIconDelegate() = default;

  // Called when the extension icon is clicked.
  virtual void OnExtensionIconClicked(const std::string& extension_id,
                                      views::View* anchor_view) = 0;

  // Called when the context menu is requested on the extension icon.
  virtual void OnExtensionIconContextMenu(const std::string& extension_id,
                                          const gfx::Point& point) = 0;
};

// Individual extension icon button shown in the sidebar extensions section.
//
// This is a pure presentation view — it shows an extension icon and
// tooltip, and delegates click/context-menu actions to its delegate.
// It does not own extension state and does not manage popups.
//
// Similar to Chromium's ToolbarActionView but simplified for sidebar
// presentation (smaller icon, no label, horizontal grid layout).
//
// Chromium owner: ToolbarActionView
//   (chrome/browser/ui/views/toolbar/toolbar_action_view.h)
class AstraExtensionIconView : public views::ImageButton {
 public:
  METADATA_HEADER(AstraExtensionIconView);

  // |extension_id| is the stable ID of the extension.
  // |delegate| receives click and context-menu events. Not owned.
  explicit AstraExtensionIconView(const std::string& extension_id,
                                  AstraExtensionIconDelegate* delegate);
  AstraExtensionIconView(const AstraExtensionIconView&) = delete;
  AstraExtensionIconView& operator=(const AstraExtensionIconView&) = delete;
  ~AstraExtensionIconView() override;

  // -- Extension identity -------------------------------------------------

  const std::string& GetExtensionId() const { return extension_id_; }

  // -- Extension info -----------------------------------------------------

  // Set all extension info at once.
  void SetExtensionInfo(const std::string& extension_id,
                        const std::u16string& name,
                        const gfx::ImageSkia& icon);

  // -- Icon ---------------------------------------------------------------

  // Set the extension icon image.
  void SetExtensionIcon(const gfx::ImageSkia& icon);

  // -- Badge --------------------------------------------------------------

  // Set the badge text (e.g. notification count).
  void SetBadgeText(const std::u16string& text);

  // -- Action vs extension ------------------------------------------------

  // Set whether this is a browser action (toolbar action) vs. a sidebar
  // extension. Affects styling.
  void SetIsAction(bool is_action);
  bool IsAction() const { return is_action_; }

  // -- Extension state ----------------------------------------------------

  // Set the extension state (enabled, disabled, blocked).
  void SetExtensionState(AstraExtensionState state);
  AstraExtensionState GetExtensionState() const { return extension_state_; }

  // -- Enabled shortcut ---------------------------------------------------

  // Set whether the extension is enabled.
  void SetExtensionEnabled(bool enabled);
  bool IsExtensionEnabled() const {
    return extension_state_ == AstraExtensionState::kEnabled;
  }

  // -- Popup state --------------------------------------------------------

  // Called to indicate whether the popup for this extension is currently
  // showing. Used to update the icon's active/selected state.
  void SetPopupShowing(bool showing);
  bool popup_showing() const { return popup_showing_; }

  // -- views::View --------------------------------------------------------

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnGestureEvent(ui::GestureEvent* event) override;

 private:
  // Handle the primary action (left click / tap).
  void HandlePrimaryAction(const ui::Event& event);

  // Show the context menu at the given screen position.
  void ShowContextMenu(const gfx::Point& screen_point);

  // Update the visual appearance based on extension state.
  void UpdateVisuals();

  // Update the tooltip from name and badge text.
  void UpdateTooltip();

  // The extension ID this icon represents.
  std::string extension_id_;

  // Extension display name.
  std::u16string extension_name_;

  // Delegate for action handling. Not owned.
  raw_ptr<AstraExtensionIconDelegate> delegate_ = nullptr;

  // Whether the extension popup is currently showing for this icon.
  bool popup_showing_ = false;

  // Extension state (enabled/disabled/blocked).
  AstraExtensionState extension_state_ = AstraExtensionState::kEnabled;

  // Whether this is a browser action.
  bool is_action_ = false;

  // Badge text (notification count).
  std::u16string badge_text_;

  // Cached icon image.
  gfx::ImageSkia icon_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_EXTENSION_ICON_VIEW_H_
