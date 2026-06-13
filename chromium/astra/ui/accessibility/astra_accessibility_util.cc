// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/accessibility/astra_accessibility_util.h"

#include "astra/ui/accessibility/astra_accessibility_ids.h"

#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/accessibility/platform/ax_platform_node.h"
#include "ui/base/class_property.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace astra {
namespace accessibility {

// =========================================================================
// View property keys for focus ring state
// =========================================================================
//
// The focus ring state is stored as a view property so that it can be
// queried and modified without requiring a custom view subclass.
//
// TODO(astra): Consider using views::FocusRing instead of a custom
//   property if it provides the styling flexibility Astra needs.
// Chromium component: views/controls/focus_ring.h

DEFINE_UI_CLASS_PROPERTY_KEY(bool, kAstraFocusRingVisibleKey, false)

DEFINE_UI_CLASS_PROPERTY_KEY(bool, kAstraFocusRingHighContrastKey, false)

// =========================================================================
// Accessible name helpers
// =========================================================================

std::u16string GetAccessibleNameForView(views::View* view) {
  if (!view) {
    return std::u16string();
  }

  // First, try the view's direct accessible name property.
  std::u16string name = view->GetAccessibleName();
  if (!name.empty()) {
    return name;
  }

  // Fall back to tooltip text (Chromium uses tooltip as description,
  // but some views use it as the accessible name when no label is set).
  std::u16string tooltip = view->GetTooltipText();
  if (!tooltip.empty()) {
    return tooltip;
  }

  // If the view has accessible node data, get the name from there.
  // This handles views that override GetAccessibleNodeData().
  ui::AXNodeData node_data;
  view->GetAccessibleNodeData(&node_data);
  if (!node_data.GetName().empty()) {
    return base::UTF8ToUTF16(node_data.GetName());
  }

  return std::u16string();
}

// =========================================================================
// Screen reader announcements
// =========================================================================

void AnnounceToScreenReader(const std::u16string& announcement,
                            ax::mojom::LiveSetting politeness,
                            views::View* context_view) {
  if (!context_view || announcement.empty()) {
    return;
  }

  // Get the widget from the context view.
  views::Widget* widget = context_view->GetWidget();
  if (!widget) {
    return;
  }

  // TODO(astra): Implement proper screen reader announcement.
  //   There are several approaches in Chromium:
  //
  //   1. Live region approach: Update a hidden live region view's text
  //      content.  This is the most cross-platform approach and works
  //      with all screen readers.
  //      Chromium example: Omnibox result count announcements.
  //
  //   2. Platform notification approach: Use platform-specific APIs like
  //      NSAccessibilityAnnouncement (macOS) or UIA LiveRegionChanged.
  //      Chromium owner: ui/accessibility/platform/ax_platform_node.h
  //
  //   3. AX event approach: Call NotifyAccessibilityEvent on a view's
  //      AXPlatformNode with an appropriate event type.
  //
  //   For now, we fire an accessibility event on the context view's
  //   platform node with a status change.  The actual announcement text
  //   should be conveyed via a live region.
  //
  // TODO(astra): Create a shared live region view on the widget's root
  //   view that can be used for all Astra announcements.  This would
  //   be similar to Chrome's status bubble live region.
  // Chromium pattern: chrome/browser/ui/views/status_icons/status_icon_linux.cc
  // Chromium pattern: ui/message_center/views/notification_view.cc

  // For now, use the context view's accessible name as a live region.
  // This is a simplified approach; production code should use a dedicated
  // live region element.
  //
  // TODO(astra): Replace with a proper live region implementation.
  //   We need to ensure the announcement text reaches the platform AT.
  // Chromium owner: ui/accessibility/ax_event_generator.h
  ui::AXNodeData node_data;
  context_view->GetAccessibleNodeData(&node_data);

  // Mark the view as a live region if it isn't already.
  // This is a temporary approach; the real implementation should use
  // a dedicated live region element.
  if (politeness != ax::mojom::LiveSetting::kOff) {
    // TODO(astra): Use a proper live region view.
    //   Setting the name and triggering an event may cause the AT to
    //   announce the change if the view is in a live region context.
    // Chromium owner: ui/accessibility/platform/ax_platform_node_base.cc
  }

  // Fire a live region changed event on the view's platform node.
  // Note: In Chromium, accessibility events are typically generated by
  // the AX tree system, not fired directly on platform nodes.
  //
  // TODO(astra): Investigate the correct way to fire accessibility events
  //   from Views code.  In Chrome, this is done via WebContents or via
  //   views::View::NotifyAccessibilityEvent.
  // Chromium pattern: views/accessibility/view_ax_platform_node_delegate.cc
  // Chromium pattern: views/view.cc (NotifyAccessibilityEvent)

  // For now, we rely on NotifyAccessibilityEvent on the view.
  // Views have a NotifyAccessibilityEvent method that fires an event
  // on the platform accessibility object.
  context_view->NotifyAccessibilityEvent(
      ax::mojom::Event::kLiveRegionCreated, true);
}

void AnnounceToScreenReader(const std::u16string& announcement,
                            views::View* context_view) {
  AnnounceToScreenReader(announcement,
                         ax::mojom::LiveSetting::kPolite,
                         context_view);
}

// =========================================================================
// Focus ring helpers
// =========================================================================

void ShowFocusRing(views::View* view) {
  if (!view) {
    return;
  }
  view->SetProperty(kAstraFocusRingVisibleKey, true);

  // TODO(astra): Actually render a focus ring around the view.
  //   Options:
  //   1. Use views::FocusRing::Get(view)->SetColor() and SchedulePaint()
  //   2. Create a custom focus ring layer / view
  //   3. Use the view's border or paint override
  //
  //   Chromium's views::FocusRing is a non-owning helper that paints
  //   a focus ring around a view.  It requires the view to have a layer.
  //
  // TODO(astra): Check if views::FocusRing is the right approach or
  //   if Astra needs custom focus ring styling.
  // Chromium component: views/controls/focus_ring.h
  view->SchedulePaint();
}

void HideFocusRing(views::View* view) {
  if (!view) {
    return;
  }
  view->SetProperty(kAstraFocusRingVisibleKey, false);
  view->SchedulePaint();
}

bool HasFocusRing(views::View* view) {
  if (!view) {
    return false;
  }
  return view->GetProperty(kAstraFocusRingVisibleKey);
}

void SetFocusRingHighContrast(views::View* view, bool high_contrast) {
  if (!view) {
    return;
  }
  view->SetProperty(kAstraFocusRingHighContrastKey, high_contrast);
  if (view->GetProperty(kAstraFocusRingVisibleKey)) {
    view->SchedulePaint();
  }
}

// =========================================================================
// Keyboard navigation helpers
// =========================================================================

bool IsFocusNavigationKey(const ui::KeyEvent& event) {
  if (event.type() != ui::ET_KEY_PRESSED) {
    return false;
  }

  switch (event.key_code()) {
    case ui::VKEY_TAB:
    case ui::VKEY_LEFT:
    case ui::VKEY_RIGHT:
    case ui::VKEY_UP:
    case ui::VKEY_DOWN:
    case ui::VKEY_HOME:
    case ui::VKEY_END:
    case ui::VKEY_PRIOR:  // Page Up
    case ui::VKEY_NEXT:   // Page Down
      return true;
    default:
      return false;
  }
}

bool IsForwardFocusKey(const ui::KeyEvent& event) {
  if (event.type() != ui::ET_KEY_PRESSED) {
    return false;
  }
  // Tab without Shift = forward focus.
  return event.key_code() == ui::VKEY_TAB &&
         !(event.flags() & ui::EF_SHIFT_DOWN);
}

bool IsBackwardFocusKey(const ui::KeyEvent& event) {
  if (event.type() != ui::ET_KEY_PRESSED) {
    return false;
  }
  // Tab with Shift = backward focus.
  return event.key_code() == ui::VKEY_TAB &&
         (event.flags() & ui::EF_SHIFT_DOWN);
}

bool IsActivationKey(const ui::KeyEvent& event) {
  if (event.type() != ui::ET_KEY_PRESSED) {
    return false;
  }
  switch (event.key_code()) {
    case ui::VKEY_RETURN:
    case ui::VKEY_SPACE:
    case ui::VKEY_EXECUTE:
      return true;
    default:
      return false;
  }
}

bool TrapFocusInContainer(views::View* container, const ui::KeyEvent& event) {
  if (!container || event.type() != ui::ET_KEY_PRESSED) {
    return false;
  }

  // Only handle Tab (forward or backward) focus trapping.
  if (event.key_code() != ui::VKEY_TAB) {
    return false;
  }

  views::FocusManager* focus_manager = container->GetFocusManager();
  if (!focus_manager) {
    return false;
  }

  views::View* focused = focus_manager->GetFocusedView();
  if (!focused || !container->Contains(focused)) {
    // If focus isn't inside the container, don't trap it.
    return false;
  }

  bool forward = !(event.flags() & ui::EF_SHIFT_DOWN);

  // Try to advance focus normally within the container.
  if (forward) {
    // Try to find the next focusable view inside the container.
    // If there is none, wrap to the first.
    //
    // TODO(astra): Implement proper focus traversal with wrapping.
    //   This requires checking if the next focusable view after the
    //   current one is still inside the container.  If not, wrap to
    //   the first focusable view in the container.
    //
    //   Chromium's views::FocusManager::AdvanceFocus moves focus according
    //   to the focus traversal order, but it doesn't support containment
    //   wrapping by default.
    //
    //   For modal dialogs, Chromium uses Widget::IsModal() and the focus
    //   system prevents focus from leaving the widget.
    //
    // TODO(astra): Use views::FocusTraversable for custom focus order.
    //   Views supports custom focus traversal via FocusTraversable, which
    //   can be used to implement custom focus order and containment.
    // Chromium component: views/focus/focus_traversable.h

    // For now, use the existing FocusNextChild helper from the utils
    // library.  If focus is on the last item, wrap to the first.
    views::View* next = nullptr;

    // TODO(astra): This implementation is simplified.
    //   Proper focus trapping should use the Views focus system.
    //   One approach: override GetFocusManager() on the container's
    //   widget to use a custom focus manager with trapping behavior.
    //   Another approach: install a focus change listener that redirects
    //   focus back into the container if it tries to leave.
    // Chromium pattern: views/widget/widget.cc (OnKeyEvent, focus handling)

    // Return true to indicate we handled the key (suppress default Tab behavior).
    // The actual focus movement is delegated to the focus manager.
    //
    // TODO(astra): Implement full focus trapping.
    //   Currently this just suppresses Tab navigation when focus should
    //   be trapped.  A real implementation would wrap focus around.
    if (forward) {
      // Check if there's a next focusable inside the container.
      // If not, wrap to first.
      //
      // This is a stub; real implementation needed.
      // For now, we let the default focus behavior happen.
      //
      // TODO(astra): Integrate with astra::accessibility::FocusNextChild
      //   from the utils library, but be careful about the dependency.
      //   The util library should not depend on the utils library.
      return false;  // Not fully implemented yet.
    }
  }

  return false;
}

// =========================================================================
// Accessibility ID string lookup
// =========================================================================
//
// Implemented here (in the util .cc file) because the IDs header is
// a lightweight header that should not include implementation details.

const char* AstraAccessibilityIdToString(AstraAccessibilityId id) {
  switch (id) {
    case AstraAccessibilityId::kSidebarContainer:
      return "AstraSidebarContainer";
    case AstraAccessibilityId::kSidebarItem:
      return "AstraSidebarItem";
    case AstraAccessibilityId::kSidebarSection:
      return "AstraSidebarSection";
    case AstraAccessibilityId::kSidebarFavoritesSection:
      return "AstraSidebarFavoritesSection";
    case AstraAccessibilityId::kSidebarHistorySection:
      return "AstraSidebarHistorySection";
    case AstraAccessibilityId::kSidebarDownloadsSection:
      return "AstraSidebarDownloadsSection";
    case AstraAccessibilityId::kSidebarExtensionsSection:
      return "AstraSidebarExtensionsSection";

    case AstraAccessibilityId::kSpaceSelector:
      return "AstraSpaceSelector";
    case AstraAccessibilityId::kSpaceItem:
      return "AstraSpaceItem";
    case AstraAccessibilityId::kSpaceSwitcher:
      return "AstraSpaceSwitcher";

    case AstraAccessibilityId::kSplitViewContainer:
      return "AstraSplitViewContainer";
    case AstraAccessibilityId::kSplitViewDivider:
      return "AstraSplitViewDivider";
    case AstraAccessibilityId::kTabGroup:
      return "AstraTabGroup";
    case AstraAccessibilityId::kFavoriteTab:
      return "AstraFavoriteTab";

    case AstraAccessibilityId::kGlancePreview:
      return "AstraGlancePreview";
    case AstraAccessibilityId::kGlanceTooltip:
      return "AstraGlanceTooltip";

    case AstraAccessibilityId::kCommandPalette:
      return "AstraCommandPalette";
    case AstraAccessibilityId::kCommandPaletteItem:
      return "AstraCommandPaletteItem";

    case AstraAccessibilityId::kStatusAnnouncement:
      return "AstraStatusAnnouncement";
    case AstraAccessibilityId::kAlertAnnouncement:
      return "AstraAlertAnnouncement";
    case AstraAccessibilityId::kLiveRegionStatus:
      return "AstraLiveRegionStatus";

    case AstraAccessibilityId::kActionPinTab:
      return "AstraActionPinTab";
    case AstraAccessibilityId::kActionUnpinTab:
      return "AstraActionUnpinTab";
    case AstraAccessibilityId::kActionMoveToSpace:
      return "AstraActionMoveToSpace";
    case AstraAccessibilityId::kActionAddToFavorites:
      return "AstraActionAddToFavorites";
    case AstraAccessibilityId::kActionRemoveFromFavorites:
      return "AstraActionRemoveFromFavorites";
    case AstraAccessibilityId::kActionSplitView:
      return "AstraActionSplitView";
    case AstraAccessibilityId::kActionCloseSplit:
      return "AstraActionCloseSplit";

    case AstraAccessibilityId::kFocusRingSidebar:
      return "AstraFocusRingSidebar";
    case AstraAccessibilityId::kFocusRingWidget:
      return "AstraFocusRingWidget";
    case AstraAccessibilityId::kFocusRingBubble:
      return "AstraFocusRingBubble";
  }

  // Fallback for unknown IDs.
  return "AstraUnknownAccessibilityId";
}

}  // namespace accessibility
}  // namespace astra
