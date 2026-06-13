#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_EXTENSION_ICON_VIEW_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_EXTENSION_ICON_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "astra/browser/astra_extension_helper.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/metadata/view_factory.h"

namespace astra {

// Delegate interface for AstraExtensionIconView events.
// Implemented by the parent extensions sidebar section.
//
// Chromium owner: ToolbarActionView delegate
//   (chrome/browser/ui/views/toolbar/toolbar_action_view.h)
class AstraExtensionIconDelegate {
 public:
  virtual ~AstraExtensionIconDelegate() = default;

  // Called when the extension icon is left-clicked.
  virtual void OnExtensionClicked(const std::string& extension_id) = 0;

  // Called when the extension icon is middle-clicked.
  virtual void OnExtensionMiddleClicked(const std::string& extension_id) = 0;

  // Called when the extension icon is right-clicked (context menu).
  virtual void OnExtensionRightClicked(const std::string& extension_id,
                                       const gfx::Point& point) = 0;

  // Called when the extension popup is shown for this icon.
  virtual void OnExtensionPopupShown(const std::string& extension_id) = 0;

  // Called when the extension popup is closed for this icon.
  virtual void OnExtensionPopupClosed(const std::string& extension_id) = 0;

  // Called when the pin state is toggled via context menu.
  virtual void OnPinExtension(const std::string& extension_id, bool pinned) = 0;

  // Called when "Manage extension" is requested from the context menu.
  virtual void OnManageExtensionRequested(
      const std::string& extension_id) = 0;

  // Called when "Remove extension" is requested from the context menu.
  virtual void OnRemoveExtensionRequested(
      const std::string& extension_id) = 0;

  // Called when "Disable extension" is requested from the context menu.
  virtual void OnDisableExtensionRequested(
      const std::string& extension_id) = 0;

  // Called when "Options" is requested from the context menu.
  virtual void OnExtensionOptionsRequested(
      const std::string& extension_id) = 0;

  // Legacy delegate method — kept for backward compatibility.
  // TODO(astra): Remove once all callers use the new delegate methods.
  virtual void OnExtensionIconClicked(const std::string& extension_id,
                                      views::View* anchor_view) {}
  virtual void OnExtensionIconContextMenu(const std::string& extension_id,
                                          const gfx::Point& point) {}
};

// Individual extension icon button shown in the sidebar extensions section.
//
// This is a pure presentation view — it shows an extension icon with
// optional badges and indicators, and delegates all user actions to its
// delegate. It does not own extension state and does not manage popups.
//
// Visual elements (top to bottom, front to back):
//   - Hover/active background
//   - Extension icon (center)
//   - State indicator (enabled/disabled/blocked tint)
//   - Badge (top-right, text + background)
//   - Notification badge
//   - Pinned indicator
//   - Loading spinner
//   - Error indicator
//
// Similar to Chromium's ToolbarActionView but simplified for sidebar
// presentation (smaller icon, grid layout, no label).
//
// Chromium owner: ToolbarActionView
//   (chrome/browser/ui/views/toolbar/toolbar_action_view.h)
// Chromium owner: ExtensionActionPlatformView
//   (chrome/browser/ui/views/extensions/extension_action_platform_view.h)
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

  const std::string& GetExtensionId() const;

  // -- Extension info -----------------------------------------------------

  // Set all extension info at once. Convenience method that updates
  // all properties from a single AstraExtensionInfo struct.
  void SetExtensionInfo(const AstraExtensionInfo& info);

  // -- Name / tooltip ------------------------------------------------------

  // Set the extension display name (used for tooltip and accessibility).
  void SetName(const std::u16string& name);
  const std::u16string& GetName() const;

  // -- Icon ---------------------------------------------------------------

  // Set the extension icon image.
  void SetIcon(const gfx::ImageSkia& icon);
  const gfx::ImageSkia& GetIcon() const;

  // Set whether the extension has a custom icon (vs. default placeholder).
  void SetHasIcon(bool has_icon);
  bool HasIcon() const;

  // -- Badge --------------------------------------------------------------

  // Set the badge text (e.g. notification count like "42" or "!").
  void SetBadgeText(const std::u16string& text);
  const std::u16string& GetBadgeText() const;

  // Set the badge text color.
  void SetBadgeColor(SkColor color);
  SkColor GetBadgeColor() const;

  // Set the badge background color.
  void SetBadgeBackgroundColor(SkColor color);
  SkColor GetBadgeBackgroundColor() const;

  // -- Extension state ----------------------------------------------------

  // Set the extension state (enabled, disabled, blocked, error, uninstalled).
  void SetExtensionState(AstraExtensionState state);
  AstraExtensionState GetExtensionState() const;

  // Shortcut: set state to enabled or disabled.
  void SetExtensionEnabled(bool enabled);
  bool IsExtensionEnabled() const;

  // -- Action vs extension ------------------------------------------------

  // Set whether this is a browser action (toolbar action) vs. a regular
  // sidebar extension. Affects styling and context menu items.
  void SetIsAction(bool is_action);
  bool IsAction() const;

  // -- Pinned state -------------------------------------------------------

  // Set whether the extension is pinned to the sidebar.
  // Pinned extensions appear in a dedicated section at the top.
  void SetPinned(bool pinned);
  bool IsPinned() const;

  // -- Tooltip ------------------------------------------------------------

  // Set whether a tooltip should be shown on hover.
  void SetShowTooltip(bool show);
  bool GetShowTooltip() const;

  // -- Context menu -------------------------------------------------------

  // Set whether right-clicking shows a context menu.
  void SetShowContextMenu(bool show);
  bool GetShowContextMenu() const;

  // -- Icon size ----------------------------------------------------------

  // Set the icon size in pixels (the actual image area, not including
  // padding). Defaults to 24dp.
  void SetIconSize(int size_px);
  int GetIconSize() const;

  // -- Popup --------------------------------------------------------------

  // Set whether this extension has a popup. Extensions without popups
  // execute their action directly on click instead of opening a popup.
  void SetHasPopup(bool has_popup);
  bool HasPopup() const;

  // Called to indicate whether the popup for this extension is currently
  // showing. Used to update the icon's active/selected state.
  void SetPopupShowing(bool showing);
  bool popup_showing() const;

  // -- Current / selected -------------------------------------------------

  // Set whether this icon represents the currently active extension
  // (e.g., its popup is showing or it's the selected item).
  void SetIsCurrent(bool current);
  bool IsCurrent() const;

  // -- Notification badge -------------------------------------------------

  // Set the notification count shown in the badge.
  // A value of 0 hides the notification badge.
  void SetNotificationCount(int count);
  int GetNotificationCount() const;

  // Set whether to show the notification badge.
  void SetShowNotificationBadge(bool show);
  bool GetShowNotificationBadge() const;

  // -- Loading state ------------------------------------------------------

  // Set whether the extension is currently loading.
  // When loading, a spinner is shown over the icon.
  void SetLoading(bool loading);
  bool IsLoading() const;

  // -- Error state --------------------------------------------------------

  // Set whether the extension is in an error state.
  // Error state shows a visual indicator over the icon.
  void SetInErrorState(bool error);
  bool IsInErrorState() const;

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

  // Handle the middle click action.
  void HandleMiddleClickAction();

  // Show the context menu at the given screen position.
  void ShowContextMenu(const gfx::Point& screen_point);

  // Update the visual appearance based on extension state.
  void UpdateVisuals();

  // Update the tooltip from name and badge text.
  void UpdateTooltip();

  // Compute the effective opacity based on state and popup status.
  float GetEffectiveOpacity() const;

  // The extension ID this icon represents.
  std::string extension_id_;

  // Extension display name.
  std::u16string extension_name_;

  // Delegate for action handling. Not owned.
  raw_ptr<AstraExtensionIconDelegate> delegate_ = nullptr;

  // Cached icon image.
  gfx::ImageSkia icon_;

  // Whether a custom icon has been set.
  bool has_icon_ = false;

  // Whether the extension popup is currently showing for this icon.
  bool popup_showing_ = false;

  // Whether this extension has a popup (vs. action-only).
  bool has_popup_ = true;

  // Extension state (enabled/disabled/blocked/error/uninstalled).
  AstraExtensionState extension_state_ = AstraExtensionState::kEnabled;

  // Whether this is a browser action.
  bool is_action_ = false;

  // Whether this extension is pinned to the sidebar.
  bool is_pinned_ = false;

  // Badge text (notification count or status).
  std::u16string badge_text_;

  // Badge text color.
  SkColor badge_text_color_ = SK_ColorWHITE;

  // Badge background color.
  SkColor badge_background_color_ = SK_ColorRED;

  // Whether to show the tooltip on hover.
  bool show_tooltip_ = true;

  // Whether to show the context menu on right-click.
  bool show_context_menu_ = true;

  // Icon size in pixels (the image area, not including padding).
  int icon_size_ = 24;

  // Icon padding (surrounding the icon image).
  static constexpr int kIconPadding = 4;

  // Whether this is the currently active/selected extension.
  bool is_current_ = false;

  // Notification count (0 = no notification).
  int notification_count_ = 0;

  // Whether to show the notification badge.
  bool show_notification_badge_ = true;

  // Whether the extension is loading.
  bool is_loading_ = false;

  // Whether the extension is in an error state.
  bool is_in_error_state_ = false;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_EXTENSION_ICON_VIEW_H_
