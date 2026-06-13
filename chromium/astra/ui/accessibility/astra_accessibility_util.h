// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_ACCESSIBILITY_ASTRA_ACCESSIBILITY_UTIL_H_
#define ASTRA_UI_ACCESSIBILITY_ASTRA_ACCESSIBILITY_UTIL_H_

#include <string>

#include "ui/accessibility/ax_enums.mojom-forward.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/view.h"

namespace astra {
namespace accessibility {

// =========================================================================
// Astra Accessibility Utilities
// =========================================================================
//
// General accessibility utility functions for Astra views.
// These helpers provide higher-level accessibility operations built on
// Chromium's accessibility infrastructure.
//
// Chromium subsystems reused:
//   - ui/accessibility/platform/ — AXPlatformNode, AXPlatformNodeDelegate
//   - ui/views/ — View accessibility, FocusManager, FocusRing
//   - ui/accessibility/ — AXNodeData, AXEventGenerator
//
// TODO(astra): Integrate with Chromium's accessibility notification system.
//   Screen reader announcements should go through
//   ui::AXPlatformNode::NotifyAccessibilityEvent for platform-level AT
//   notifications.  Live regions are the cross-platform approach.
//   Chromium owner: ui/accessibility/platform/ax_platform_node.h
//   Patch point: Views override GetAccessibleNodeData(); no Chromium patch needed.
// =========================================================================

// -- Accessible name helpers -------------------------------------------------

// Retrieves the accessible name for a view.
//
// This helper checks the view's accessible name property and falls back to
// tooltip text or label text if no explicit accessible name is set.
//
// For views that override GetAccessibleNodeData(), this returns the name
// that would be computed by that method.
//
// Chromium component: views::View::GetAccessibleName()
// Chromium component: ui/accessibility/ax_node_data.h (AXNodeData::GetName())
std::u16string GetAccessibleNameForView(views::View* view);

// -- Screen reader announcements --------------------------------------------

// Announces a message to the screen reader using platform accessibility APIs.
//
// This function posts an accessibility announcement that screen readers
// will read without requiring focus to move.  It works by creating a
// hidden live region or by using the platform's native announcement API.
//
// |announcement| is the text to announce.
// |politeness| controls whether the announcement interrupts the user.
//   - kPolite: waits for the user to be idle (recommended for most updates)
//   - kAssertive: interrupts immediately (use sparingly, e.g., errors)
// |context_view| is a view in the widget where the announcement originates.
//   The announcement is associated with this widget's accessibility tree.
//
// TODO(astra): Implement using ui::AXPlatformNode::NotifyAccessibilityEvent
//   with ax::mojom::Event::kLiveRegionChanged or via a dedicated live region
//   view.  The exact implementation depends on platform support.
//   On Windows: use UIA LiveRegionChanged events.
//   On macOS: use NSAccessibilityAnnouncement.
//   On Linux/ATK: use object:state-changed:showing on a live region.
// Chromium owner: ui/accessibility/platform/ax_platform_node.h
// Chromium owner: content/browser/accessibility/browser_accessibility_manager.h
void AnnounceToScreenReader(const std::u16string& announcement,
                            ax::mojom::LiveSetting politeness,
                            views::View* context_view);

// Convenience overload that uses polite politeness by default.
void AnnounceToScreenReader(const std::u16string& announcement,
                            views::View* context_view);

// -- Focus ring / focus indicator helpers -----------------------------------

// Shows a visible focus ring around the specified view.
//
// The focus ring is an accessibility indicator that highlights the
// currently focused element.  Astra views may use custom focus rings
// that match the Astra design language while remaining visible in
// high contrast mode.
//
// TODO(astra): Implement using views::FocusRing or a custom focus ring
//   view.  Chromium has views::FocusRing for tab focus indicators.
//   For Astra widgets, we may want a custom focus ring with specific
//   color and thickness.
// Chromium component: views/controls/focus_ring.h (views::FocusRing)
// Chromium component: views/focus/focus_manager.h (FocusChangeListener)
void ShowFocusRing(views::View* view);

// Hides the focus ring on the specified view.
void HideFocusRing(views::View* view);

// Returns true if the view has a visible focus ring.
bool HasFocusRing(views::View* view);

// Sets whether the focus ring should use a high-contrast style.
// High contrast focus rings are thicker and use higher contrast colors
// to ensure visibility for users with low vision.
//
// TODO(astra): Integrate with NativeTheme high contrast detection.
//   The focus ring style should automatically switch to high contrast
//   when the system is in high contrast mode.
// Chromium component: ui/native_theme/native_theme.h (UsesHighContrastColors)
void SetFocusRingHighContrast(views::View* view, bool high_contrast);

// -- Keyboard navigation helpers --------------------------------------------

// Returns true if the key event is a standard focus navigation key.
// Focus navigation keys include Tab, Shift+Tab, arrow keys (in list
// contexts), and Home/End (in list contexts).
//
// This is useful for determining whether a key event should be handled
// by the focus system or by the view's own key handling.
bool IsFocusNavigationKey(const ui::KeyEvent& event);

// Returns true if the key event moves focus forward (Tab).
bool IsForwardFocusKey(const ui::KeyEvent& event);

// Returns true if the key event moves focus backward (Shift+Tab).
bool IsBackwardFocusKey(const ui::KeyEvent& event);

// Returns true if the key event activates the focused item
// (Return, Space, Enter).
bool IsActivationKey(const ui::KeyEvent& event);

// Traps focus within a container view (e.g., a modal dialog or bubble).
// When focus would leave the container, it wraps around to the other end.
//
// Call this from a view's OnKeyPressed handler to implement focus trapping.
// Returns true if the key event was handled (focus was moved within
// the container).
//
// TODO(astra): Implement using views::FocusManager::AddFocusChangeListener
//   or by custom key handling.  Modal dialogs in Chromium use
//   views::Widget::InitParams::TYPE_WINDOW_FRAMELESS with modal type.
//   For bubble dialogs, focus trapping may need custom handling.
// Chromium component: views/focus/focus_manager.h
// Chromium component: views/bubble/bubble_dialog_delegate_view.h
bool TrapFocusInContainer(views::View* container, const ui::KeyEvent& event);

}  // namespace accessibility
}  // namespace astra

#endif  // ASTRA_UI_ACCESSIBILITY_ASTRA_ACCESSIBILITY_UTIL_H_
