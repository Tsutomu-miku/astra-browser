#ifndef ASTRA_UI_ACCESSIBILITY_ASTRA_ACCESSIBILITY_UTILS_H_
#define ASTRA_UI_ACCESSIBILITY_ASTRA_ACCESSIBILITY_UTILS_H_

#include <string>

#include "base/strings/string_piece.h"
#include "ui/accessibility/ax_enums.mojom-forward.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/class_property.h"
#include "ui/views/view.h"

namespace astra {
namespace accessibility {

// =========================================================================
// Astra accessibility utilities
// =========================================================================
//
// Utility functions for accessibility in Astra views.  These helpers wrap
// Chromium's AXNodeData and views accessibility patterns to provide
// consistent, reusable accessibility primitives for Astra UI surfaces.
//
// Chromium subsystems reused:
//   - ui/accessibility/ — AXNodeData, AXRole, AXState
//   - ui/views/accessibility/ — View accessibility infrastructure
//   - ui/native_theme/ — high contrast theme detection
//
// TODO(astra): Full AX tree integration for custom Astra widgets.
//   Currently, Astra views rely on Chromium's default View accessibility
//   plumbing.  For custom widgets (e.g., sidebar sections with complex
//   layout), we may need to provide a richer AX tree with parent/child
//   relationships and group boundaries.
//   Chromium owner: ui/accessibility/platform/ and views::View::GetAccessibleNodeData
//   Patch point: Override GetAccessibleNodeData on each custom Astra widget.
// =========================================================================

// -- Accessible name helpers -------------------------------------------------

// Sets the accessible name on a view via its AX node data.
// The accessible name is what screen readers announce when the view
// receives focus.  Use this instead of relying on visible text alone,
// especially for icon-only buttons and decorative elements.
//
// Chromium component: ui/accessibility/ax_node_data.h
//   AXNodeData::SetName() and AXNodeData::SetNameFrom()
void SetAccessibleName(views::View* view, const std::u16string& name);

// Sets the accessible name with an explicit name source.
// Use ax::mojom::NameFrom::kAttribute for programmatic names, or
// ax::mojom::NameFrom::kContents for names derived from visible text.
void SetAccessibleName(views::View* view,
                       const std::u16string& name,
                       ax::mojom::NameFrom name_from);

// Sets an accessible description (help text) on a view.
// Descriptions provide additional context beyond the accessible name,
// e.g., "Press Enter to activate" or "Unread: 5 messages".
//
// Chromium component: ui/accessibility/ax_node_data.h
//   AXNodeData::AddStringAttribute(ax::mojom::StringAttribute::kDescription)
void SetAccessibleDescription(views::View* view,
                              const std::u16string& description);

// -- Role helpers -----------------------------------------------------------

// Sets the accessibility role on a view.
// Use ax::mojom::Role::kButton for clickable items,
// ax::mojom::Role::kListBox for list containers, etc.
//
// Chromium component: ui/accessibility/ax_enums.mojom.h (AXRole)
void SetAccessibleRole(views::View* view, ax::mojom::Role role);

// Sets a role description (human-readable role name) on a view.
// Use this when the standard ARIA role is not descriptive enough,
// e.g., "Sidebar" instead of "region", or "Tab" instead of "button".
//
// Chromium component: ui/accessibility/ax_node_data.h
//   AXNodeData::AddStringAttribute(ax::mojom::StringAttribute::kRoleDescription)
void SetRoleDescription(views::View* view, const std::u16string& description);

// -- State helpers ----------------------------------------------------------

// Sets a view as keyboard-focusable in the accessibility tree.
// Also sets the view's focus behavior so it actually receives focus
// from Tab traversal and programmatic focus requests.
void SetFocusable(views::View* view, bool focusable = true);

// Marks a view as currently focused in the accessibility tree.
// Call this from OnFocus() / OnBlur() on custom focusable views.
void SetFocused(views::View* view, bool focused);

// Sets the "pressed" state on a toggle-style button or similar control.
void SetPressedState(views::View* view, bool pressed);

// Sets the "selected" state on a list item or tab.
void SetSelectedState(views::View* view, bool selected);

// Sets the "expanded" state on a collapsible section.
void SetExpandedState(views::View* view, bool expanded);

// Sets the disabled/inert state on a view.
void SetDisabledState(views::View* view, bool disabled);

// -- Live region helpers ----------------------------------------------------

// Marks a view as a live region for accessibility announcements.
// When the content of a live region changes, screen readers announce
// the change without the user having to move focus.
//
// |politeness| controls how interruptive the announcement is:
//   - kPolite: announces when the user is idle (default for most updates)
//   - kAssertive: announces immediately (use sparingly, e.g. errors)
//
// Chromium component: ui/accessibility/ax_node_data.h
//   AXNodeData::AddLiveRegionAttributes()
void SetLiveRegion(views::View* view,
                   ax::mojom::LiveSetting politeness =
                       ax::mojom::LiveSetting::kPolite);

// Announces a status message via the given live region view.
// Updates the view's accessible name to trigger a live region announcement.
// The view should have been configured as a live region via SetLiveRegion().
void AnnounceLiveMessage(views::View* live_region_view,
                         const std::u16string& message);

// -- Focus management helpers -----------------------------------------------

// Moves focus to the next focusable child of |parent|, wrapping around
// if |wrap| is true.  Returns the view that received focus, or nullptr
// if no focusable child was found.
//
// This is useful for arrow-key navigation within a container (e.g.,
// a sidebar section or a list of workspace cards) where standard Tab
// traversal is not the desired pattern.
//
// Chromium component: views::FocusManager and views::FocusTraversable
views::View* FocusNextChild(views::View* parent, bool wrap = true);

// Moves focus to the previous focusable child of |parent|.
// See FocusNextChild for details.
views::View* FocusPreviousChild(views::View* parent, bool wrap = true);

// Returns true if the view is both visible and enabled (can receive focus).
bool IsFocusable(const views::View* view);

// Returns the first focusable descendant of |parent|, or nullptr if none.
views::View* GetFirstFocusableChild(views::View* parent);

// Returns the last focusable descendant of |parent|, or nullptr if none.
views::View* GetLastFocusableChild(views::View* parent);

// Scrolls the given child view into view within its scroll container.
// Use this when changing keyboard selection so the focused item is always
// visible on screen.
//
// Chromium component: views::ScrollView::ScrollRectToVisible
void ScrollChildIntoView(views::View* scroll_container, views::View* child);

// -- High contrast and theme helpers ---------------------------------------

// Returns true if the system is in high contrast mode.
// Reads from the native theme's high contrast state.
//
// Chromium component: ui/native_theme/native_theme.h
//   NativeTheme::ShouldUseHighContrast()
bool IsHighContrastMode();

// Returns true if reduced motion is preferred.
// Checks the system's prefers-reduced-motion setting.
//
// Chromium component: ui/native_theme/native_theme.h
//   NativeTheme::prefers_reduced_transitions()
bool IsReducedMotionPreferred();

// -- View property keys ----------------------------------------------------
//
// Accessibility properties that cannot be set directly via views::View APIs
// are stored as view properties.  View subclasses that override
// GetAccessibleNodeData() should call ApplyAstraAccessibleProperties()
// to apply these stored properties to the AX node data.
//
// TODO(astra): These property keys use the Views property system.
//   Chromium component: ui/base/class_property.h
//   Chromium pattern: DEFINE_LOCAL_UI_CLASS_PROPERTY_KEY or
//   DEFINE_OWNED_UI_CLASS_PROPERTY_KEY for owned types.

// Property key for accessible description (help text).
// Set via SetAccessibleDescription(), read via ApplyAstraAccessibleProperties.
extern const ui::ClassProperty<std::u16string*>* const
    kAstraAccessibleDescriptionKey;

// Property key for role description (human-readable role name).
// Set via SetRoleDescription(), read via ApplyAstraAccessibleProperties.
extern const ui::ClassProperty<std::u16string*>* const
    kAstraRoleDescriptionKey;

// Property key for pressed state (toggle buttons).
// Set via SetPressedState(), read via ApplyAstraAccessibleProperties.
extern const ui::ClassProperty<bool>* const kAstraPressedStateKey;

// Property key for selected state (list items, tabs).
// Set via SetSelectedState(), read via ApplyAstraAccessibleProperties.
extern const ui::ClassProperty<bool>* const kAstraSelectedStateKey;

// Property key for expanded state (collapsible sections).
// Set via SetExpandedState(), read via ApplyAstraAccessibleProperties.
extern const ui::ClassProperty<bool>* const kAstraExpandedStateKey;

// Property key for live region politeness setting.
// Set via SetLiveRegion(), read via ApplyAstraAccessibleProperties.
extern const ui::ClassProperty<ax::mojom::LiveSetting>* const
    kAstraLiveRegionKey;

// -- Accessible node data helper -------------------------------------------

// Applies all Astra accessibility properties stored on |view| to |node_data|.
// View subclasses that override GetAccessibleNodeData() should call this
// function from within their override to ensure all accessibility attributes
// set via the Astra accessibility utils are reflected in the AX tree.
//
// Usage in a view subclass:
//   void MyView::GetAccessibleNodeData(ui::AXNodeData* node_data) {
//     View::GetAccessibleNodeData(node_data);
//     astra::accessibility::ApplyAstraAccessibleProperties(this, node_data);
//   }
//
// Chromium component: views::View::GetAccessibleNodeData
//   + ui/accessibility/ax_node_data.h
void ApplyAstraAccessibleProperties(views::View* view,
                                    ui::AXNodeData* node_data);

// -- Keyboard navigation helpers -------------------------------------------

// Handles common keyboard navigation for a list container.
// Call this from OnKeyPressed or AcceleratorPressed on list views.
//
// Supports:
//   - Arrow Up / Down: move selection (calls MoveSelection callback)
//   - Home / End: jump to first/last
//   - Enter / Space: activate selected item
//
// Returns true if the key event was handled.
//
// Chromium component: ui/events/keycodes/keyboard_codes.h
using MoveSelectionCallback = base::RepeatingCallback<void(int delta)>;
using ActivateCallback = base::RepeatingClosure;

bool HandleListKeyboardNavigation(
    const ui::KeyEvent& event,
    const MoveSelectionCallback& move_selection,
    const ActivateCallback& activate);

}  // namespace accessibility
}  // namespace astra

#endif  // ASTRA_UI_ACCESSIBILITY_ASTRA_ACCESSIBILITY_UTILS_H_
