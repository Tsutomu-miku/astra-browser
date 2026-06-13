#include "astra/ui/accessibility/astra_accessibility_utils.h"

#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/class_property.h"
#include "ui/base/ime/input_method.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/view.h"
#include "ui/views/controls/scroll_view.h"

namespace astra {
namespace accessibility {

// =========================================================================
// View property keys
// =========================================================================
//
// Accessibility attributes that cannot be set directly via views::View APIs
// are stored as view properties.  View subclasses that override
// GetAccessibleNodeData() can call ApplyAstraAccessibleProperties() to
// include these attributes in the AX tree.
//
// Property ownership: views owns the key values when set via SetProperty.
// For string properties, we use owned pointers (the view property system
// takes ownership).

DEFINE_UI_CLASS_PROPERTY_KEY(std::u16string*,
                             kAstraAccessibleDescriptionKey,
                             nullptr)

DEFINE_UI_CLASS_PROPERTY_KEY(std::u16string*,
                             kAstraRoleDescriptionKey,
                             nullptr)

DEFINE_UI_CLASS_PROPERTY_KEY(bool, kAstraPressedStateKey, false)

DEFINE_UI_CLASS_PROPERTY_KEY(bool, kAstraSelectedStateKey, false)

DEFINE_UI_CLASS_PROPERTY_KEY(bool, kAstraExpandedStateKey, false)

DEFINE_UI_CLASS_PROPERTY_KEY(ax::mojom::LiveSetting,
                             kAstraLiveRegionKey,
                             ax::mojom::LiveSetting::kOff)

// =========================================================================
// Accessible name helpers
// =========================================================================

void SetAccessibleName(views::View* view, const std::u16string& name) {
  if (!view) {
    return;
  }
  SetAccessibleName(view, name, ax::mojom::NameFrom::kAttribute);
}

void SetAccessibleName(views::View* view,
                       const std::u16string& name,
                       ax::mojom::NameFrom name_from) {
  if (!view) {
    return;
  }
  // Override GetAccessibleNodeData to set the name.
  // We set a property on the view that will be read by our custom
  // accessibility node data handler.
  //
  // TODO(astra): For proper accessible name support, each Astra view should
  //   override GetAccessibleNodeData() directly.  This helper provides a
  //   convenience for views that use the default View implementation.
  //   A more complete approach would use a View subclass or a helper mixin.
  // Chromium owner: views::View::GetAccessibleNodeData
  // Chromium owner: ui/accessibility/ax_node_data.h
  view->SetAccessibleName(name);
}

void SetAccessibleDescription(views::View* view,
                              const std::u16string& description) {
  if (!view) {
    return;
  }
  // Store the description as a view property.  View subclasses that
  // override GetAccessibleNodeData() should call
  // ApplyAstraAccessibleProperties() to include this in the AX tree.
  //
  // We also set the tooltip text as a fallback, since Chromium's
  // default accessibility plumbing uses tooltip text as the description
  // when no explicit description is set.
  //
  // TODO(astra): Consider also exposing this via a dedicated
  //   kDescription AX attribute when the view subclass uses
  //   ApplyAstraAccessibleProperties.
  // Chromium owner: ui/accessibility/ax_node_data.h
  //   ax::mojom::StringAttribute::kDescription
  view->SetTooltipText(description);
  view->SetProperty(kAstraAccessibleDescriptionKey,
                    std::make_unique<std::u16string>(description));
}

// =========================================================================
// Role helpers
// =========================================================================

void SetAccessibleRole(views::View* view, ax::mojom::Role role) {
  if (!view) {
    return;
  }
  view->SetAccessibleRole(role);
}

void SetRoleDescription(views::View* view,
                        const std::u16string& description) {
  if (!view) {
    return;
  }
  // Store the role description as a view property.  View subclasses
  // that override GetAccessibleNodeData() should call
  // ApplyAstraAccessibleProperties() to apply this to the AX node data.
  //
  // views::View does not have a direct SetRoleDescription() method,
  // so we use a property key + helper pattern.
  //
  // TODO(astra): Consider adding a dedicated role description API to
  //   views::View upstream, or use a mixin base class for Astra views.
  // Chromium owner: ui/accessibility/ax_node_data.h
  //   ax::mojom::StringAttribute::kRoleDescription
  view->SetProperty(kAstraRoleDescriptionKey,
                    std::make_unique<std::u16string>(description));
}

// =========================================================================
// State helpers
// =========================================================================

void SetFocusable(views::View* view, bool focusable) {
  if (!view) {
    return;
  }
  view->SetFocusBehavior(
      focusable ? views::View::FocusBehavior::ALWAYS
                : views::View::FocusBehavior::NEVER);
}

void SetFocused(views::View* view, bool focused) {
  if (!view) {
    return;
  }
  // Focus state is managed by FocusManager; we just request it.
  if (focused && view->GetFocusBehavior() != views::View::FocusBehavior::NEVER) {
    view->RequestFocus();
  }
}

void SetPressedState(views::View* view, bool pressed) {
  if (!view) {
    return;
  }
  // Store pressed state as a view property.  View subclasses that
  // override GetAccessibleNodeData() should call
  // ApplyAstraAccessibleProperties() to reflect this in the AX tree
  // via ax::mojom::State::kPressed.
  //
  // views::View has SetPressed() for image buttons but not as a general
  // accessibility state attribute, so we use a property key pattern.
  //
  // TODO(astra): Verify whether views::Button or similar base classes
  //   already provide pressed state accessibility.  For custom views,
  //   the property key approach is appropriate.
  // Chromium owner: ui/accessibility/ax_enums.mojom.h (State::kPressed)
  view->SetProperty(kAstraPressedStateKey, pressed);
}

void SetSelectedState(views::View* view, bool selected) {
  if (!view) {
    return;
  }
  // Store selected state as a view property.  Applied to AX tree via
  // ApplyAstraAccessibleProperties() (ax::mojom::State::kSelected).
  //
  // TODO(astra): For tab-like views, consider using
  //   views::Tab::SetSelected() or similar if available.  For generic
  //   list items, the property key pattern works.
  // Chromium owner: ui/accessibility/ax_enums.mojom.h (State::kSelected)
  view->SetProperty(kAstraSelectedStateKey, selected);
}

void SetExpandedState(views::View* view, bool expanded) {
  if (!view) {
    return;
  }
  // Store expanded state as a view property.  Applied to AX tree via
  // ApplyAstraAccessibleProperties()
  // (ax::mojom::State::kExpanded / kCollapsed).
  //
  // TODO(astra): For tree/expandable views, consider reusing Chromium's
  //   TreeViewController or similar patterns.  For custom collapsible
  //   sections, the property key pattern works.
  // Chromium owner: ui/accessibility/ax_enums.mojom.h
  //   (State::kExpanded / State::kCollapsed)
  view->SetProperty(kAstraExpandedStateKey, expanded);
}

void SetDisabledState(views::View* view, bool disabled) {
  if (!view) {
    return;
  }
  view->SetEnabled(!disabled);
}

// =========================================================================
// Live region helpers
// =========================================================================

void SetLiveRegion(views::View* view, ax::mojom::LiveSetting politeness) {
  if (!view) {
    return;
  }
  // Store live region setting as a view property.  View subclasses that
  // override GetAccessibleNodeData() should call
  // ApplyAstraAccessibleProperties() to apply this to the AX node data.
  //
  // Live regions cause screen readers to announce content changes without
  // requiring user focus.  Use kPolite for most updates, kAssertive for
  // important status changes (use sparingly).
  //
  // TODO(astra): Ensure live region attributes are properly reflected in
  //   the platform accessibility tree.  Some platforms may require
  //   additional handling beyond AXNodeData attributes.
  // Chromium owner: ui/accessibility/ax_enums.mojom.h (LiveSetting)
  // Chromium owner: ui/accessibility/ax_node_data.h (live_region_attributes)
  view->SetProperty(kAstraLiveRegionKey, politeness);
}

void AnnounceLiveMessage(views::View* live_region_view,
                         const std::u16string& message) {
  if (!live_region_view) {
    return;
  }
  // Setting the accessible name triggers a live region announcement
  // if the view is configured as a live region.
  SetAccessibleName(live_region_view, message);
  // TODO(astra): Use a proper live region announcement pattern.
  //   In Chromium, the standard approach is to update a hidden live region
  //   element's text content, which triggers AT announcements.
  // Chromium pattern: similar to Omnibox result count announcements or
  //   status message announcements in the browser frame.
}

// =========================================================================
// Focus management helpers
// =========================================================================

views::View* FocusNextChild(views::View* parent, bool wrap) {
  if (!parent || parent->children().empty()) {
    return nullptr;
  }

  views::View* focused = parent->GetFocusManager()->GetFocusedView();
  const auto& children = parent->children();

  // Find the index of the currently focused child (if any).
  size_t current_index = 0;
  bool found_focused = false;
  for (size_t i = 0; i < children.size(); ++i) {
    if (children[i] == focused || children[i]->Contains(focused)) {
      current_index = i;
      found_focused = true;
      break;
    }
  }

  // Search forward from the current position (or from the start).
  size_t start = found_focused ? current_index + 1 : 0;
  for (size_t i = start; i < children.size(); ++i) {
    if (IsFocusable(children[i])) {
      children[i]->RequestFocus();
      return children[i];
    }
    // Also check descendants.
    views::View* focusable = GetFirstFocusableChild(children[i]);
    if (focusable) {
      focusable->RequestFocus();
      return focusable;
    }
  }

  // If wrapping, start from the beginning.
  if (wrap) {
    for (size_t i = 0; i < start; ++i) {
      if (IsFocusable(children[i])) {
        children[i]->RequestFocus();
        return children[i];
      }
      views::View* focusable = GetFirstFocusableChild(children[i]);
      if (focusable) {
        focusable->RequestFocus();
        return focusable;
      }
    }
  }

  return nullptr;
}

views::View* FocusPreviousChild(views::View* parent, bool wrap) {
  if (!parent || parent->children().empty()) {
    return nullptr;
  }

  views::View* focused = parent->GetFocusManager()->GetFocusedView();
  const auto& children = parent->children();

  // Find the index of the currently focused child.
  size_t current_index = children.size();
  bool found_focused = false;
  for (size_t i = 0; i < children.size(); ++i) {
    if (children[i] == focused || children[i]->Contains(focused)) {
      current_index = i;
      found_focused = true;
      break;
    }
  }

  // Search backward from the current position (or from the end).
  size_t start = found_focused ? current_index : children.size();
  for (size_t i = start; i > 0; --i) {
    size_t idx = i - 1;
    if (IsFocusable(children[idx])) {
      children[idx]->RequestFocus();
      return children[idx];
    }
    views::View* focusable = GetLastFocusableChild(children[idx]);
    if (focusable) {
      focusable->RequestFocus();
      return focusable;
    }
  }

  // If wrapping, start from the end.
  if (wrap) {
    for (size_t i = children.size(); i > start; --i) {
      size_t idx = i - 1;
      if (IsFocusable(children[idx])) {
        children[idx]->RequestFocus();
        return children[idx];
      }
      views::View* focusable = GetLastFocusableChild(children[idx]);
      if (focusable) {
        focusable->RequestFocus();
        return focusable;
      }
    }
  }

  return nullptr;
}

bool IsFocusable(const views::View* view) {
  if (!view) {
    return false;
  }
  if (!view->GetVisible()) {
    return false;
  }
  if (!view->GetEnabled()) {
    return false;
  }
  return view->GetFocusBehavior() != views::View::FocusBehavior::NEVER;
}

views::View* GetFirstFocusableChild(views::View* parent) {
  if (!parent) {
    return nullptr;
  }
  for (auto* child : parent->children()) {
    if (IsFocusable(child)) {
      return child;
    }
    views::View* focusable = GetFirstFocusableChild(child);
    if (focusable) {
      return focusable;
    }
  }
  return nullptr;
}

views::View* GetLastFocusableChild(views::View* parent) {
  if (!parent) {
    return nullptr;
  }
  const auto& children = parent->children();
  for (auto it = children.rbegin(); it != children.rend(); ++it) {
    views::View* child = *it;
    if (IsFocusable(child)) {
      return child;
    }
    views::View* focusable = GetLastFocusableChild(child);
    if (focusable) {
      return focusable;
    }
  }
  return nullptr;
}

void ScrollChildIntoView(views::View* scroll_container, views::View* child) {
  if (!scroll_container || !child) {
    return;
  }
  // TODO(astra): Implement proper scroll-into-view for arbitrary scroll
  //   containers.  For ScrollView-based containers, use ScrollView::ScrollRectToVisible.
  //   For custom scrollable views, this needs custom handling.
  // Chromium component: views::ScrollView::ScrollRectToVisible
  // Chromium component: views::ScrollBar / views::ScrollBarController
  auto* scroll_view =
      views::ScrollView::GetScrollViewForView(child);
  if (scroll_view) {
    gfx::Rect child_bounds = child->bounds();
    // Convert to ScrollView contents coordinates.
    views::View* contents = scroll_view->contents();
    if (contents) {
      gfx::Point origin = child->origin();
      views::View::ConvertPointToTarget(child->parent(), contents, &origin);
      child_bounds.set_origin(origin);
    }
    scroll_view->ScrollRectToVisible(child_bounds);
  }
}

// =========================================================================
// High contrast and theme helpers
// =========================================================================

bool IsHighContrastMode() {
  // TODO(astra): Read high contrast state from NativeTheme.
  //   The actual API is NativeTheme::ShouldUseHighContrast() or we check
  //   the system's high contrast flag via ui::GetDisplayColorSpaces() or
  //   system metrics.
  // Chromium owner: ui/native_theme/native_theme.h
  //   NativeTheme::UsesHighContrastColors()
  return ui::NativeTheme::GetInstanceForNativeUi()
      ->UsesHighContrastColors();
}

bool IsReducedMotionPreferred() {
  // TODO(astra): Read prefers-reduced-motion from NativeTheme.
  //   The actual API is NativeTheme::prefers_reduced_transitions().
  // Chromium owner: ui/native_theme/native_theme.h
  //   NativeTheme::prefers_reduced_transitions()
  return ui::NativeTheme::GetInstanceForNativeUi()
      ->prefers_reduced_transitions();
}

// =========================================================================
// Keyboard navigation helpers
// =========================================================================

bool HandleListKeyboardNavigation(
    const ui::KeyEvent& event,
    const MoveSelectionCallback& move_selection,
    const ActivateCallback& activate) {
  if (event.type() != ui::ET_KEY_PRESSED) {
    return false;
  }

  switch (event.key_code()) {
    case ui::VKEY_UP:
      if (move_selection) {
        move_selection.Run(-1);
      }
      return true;

    case ui::VKEY_DOWN:
      if (move_selection) {
        move_selection.Run(1);
      }
      return true;

    case ui::VKEY_HOME:
      // Home = jump to first (delta = very large negative).
      if (move_selection) {
        move_selection.Run(-1000000);
      }
      return true;

    case ui::VKEY_END:
      // End = jump to last.
      if (move_selection) {
        move_selection.Run(1000000);
      }
      return true;

    case ui::VKEY_RETURN:
    case ui::VKEY_SPACE:
      if (activate) {
        activate.Run();
      }
      return true;

    default:
      return false;
  }
}

// =========================================================================
// ApplyAstraAccessibleProperties
// =========================================================================
//
// Applies all Astra accessibility properties stored on a view to the
// AX node data.  View subclasses that override GetAccessibleNodeData()
// should call this function to include Astra accessibility attributes.
//
// This function reads view properties set by the various setter helpers
// (SetRoleDescription, SetPressedState, etc.) and applies them to the
// AXNodeData structure.
//
// TODO(astra): Consider adding more properties here as needed.
//   Chromium owner: ui/accessibility/ax_node_data.h
//   Chromium owner: ui/accessibility/ax_enums.mojom.h

void ApplyAstraAccessibleProperties(views::View* view,
                                    ui::AXNodeData* node_data) {
  if (!view || !node_data) {
    return;
  }

  // Accessible description (help text).
  std::u16string* desc =
      view->GetProperty(kAstraAccessibleDescriptionKey);
  if (desc && !desc->empty()) {
    node_data->AddStringAttribute(
        ax::mojom::StringAttribute::kDescription, *desc);
  }

  // Role description (human-readable role name).
  std::u16string* role_desc =
      view->GetProperty(kAstraRoleDescriptionKey);
  if (role_desc && !role_desc->empty()) {
    node_data->AddStringAttribute(
        ax::mojom::StringAttribute::kRoleDescription, *role_desc);
  }

  // Pressed state (toggle buttons, etc.).
  if (view->GetProperty(kAstraPressedStateKey)) {
    node_data->AddState(ax::mojom::State::kPressed);
  }

  // Selected state (list items, tabs, etc.).
  if (view->GetProperty(kAstraSelectedStateKey)) {
    node_data->AddState(ax::mojom::State::kSelected);
  }

  // Expanded / collapsed state (expandable sections, tree nodes).
  if (view->GetProperty(kAstraExpandedStateKey)) {
    node_data->AddState(ax::mojom::State::kExpanded);
  } else {
    // If the view is explicitly set to not expanded (collapsed),
    // add the collapsed state.  We only add collapsed if there is
    // some indication this is an expandable element (e.g., it has
    // a role description that implies expandability).  For now we
    // only add expanded state when true to avoid false positives.
    // TODO(astra): Determine whether we should always add kCollapsed
    //   for views that are expandable but not expanded.  This requires
    //   knowing whether the element should have expand/collapse semantics.
  }

  // Live region setting.
  ax::mojom::LiveSetting live_setting =
      view->GetProperty(kAstraLiveRegionKey);
  if (live_setting != ax::mojom::LiveSetting::kOff) {
    node_data->AddLiveRegionAttributes(
        live_setting, ax::mojom::DefaultActionVerb::kClick,
        ax::mojom::LiveStatus::kOn, ax::mojom::LiveRelevant::kAdditionsText);
  }
}

}  // namespace accessibility
}  // namespace astra
