// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_UI_VIEWS_PAGE_ACTIONS_ASTRA_PAGE_ACTIONS_MODEL_H_
#define ASTRA_UI_VIEWS_PAGE_ACTIONS_ASTRA_PAGE_ACTIONS_MODEL_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"

namespace astra {

// Types of page actions that can appear in the page actions container.
//
// These include both Astra-specific actions and projections of Chromium
// actions (e.g. bookmarks, zoom, translate).
//
// Chromium owner: PageActionIconController / PageActionIconView
//   (chrome/browser/ui/page_action/)
enum class AstraPageActionType {
  kNone = 0,

  // Standard Chromium page actions (projected).
  kBookmarkStar,     // Add/remove bookmark
  kZoom,             // Zoom level / zoom bubble
  kTranslate,        // Google Translate
  kSaveCard,         // Save credit card
  kSavePassword,     // Save password
  kLocalCardMigration,  // Migrate local cards
  kManagePasswords,  // Manage passwords
  kSendTabToSelf,    // Send tab to device
  kSharingHub,       // Sharing hub
  kDownloadsPage,    // Downloads page action
  kFind,             // Find in page
  kPrint,            // Print

  // Astra-specific page actions.
  kFocusMode,        // Focus mode toggle
  kCommandPalette,   // Open command palette
  kSplitView,        // Split view toggle
  kSidebar,          // Sidebar toggle
  kScreenshot,       // Take screenshot
  kReadingList,      // Add to reading list
  kNote,             // Add note
  kFavorite,         // Add to favorites

  // Extension page actions.
  kExtensionAction,  // Generic extension action (has extension_id)
};

// Visual state of a page action icon.
enum class AstraPageActionState {
  kDefault,     // Normal state
  kActive,      // Active/selected (e.g. bookmarked)
  kDisabled,    // Disabled/unavailable
  kAttention,   // Needs attention (animated/colored)
  kError,       // Error state
};

// A single page action item.
struct AstraPageActionItem {
  AstraPageActionType type = AstraPageActionType::kNone;
  std::u16string label;            // Display name (tooltip)
  std::string icon_name;           // Name of the icon to use
  std::u16string badge_text;       // Optional badge text (e.g. "42")
  SkColor badge_color = SK_ColorRED;  // Badge background color
  AstraPageActionState state = AstraPageActionState::kDefault;
  bool visible = true;             // Whether the action is currently visible
  bool pinned = true;              // Whether shown in main row vs overflow
  int order = 0;                   // Display order (lower = leftmost)

  // For extension actions only.
  std::string extension_id;
  std::u16string extension_name;

  bool operator==(const AstraPageActionItem& other) const {
    return type == other.type && label == other.label &&
           icon_name == other.icon_name && state == other.state &&
           visible == other.visible && pinned == other.pinned &&
           order == other.order && extension_id == other.extension_id;
  }
};

// Observer interface for AstraPageActionsModel.
class AstraPageActionsObserver : public base::CheckedObserver {
 public:
  // Called when the set of actions changes (add/remove/reorder).
  virtual void OnActionsChanged(AstraPageActionsModel* model) {}

  // Called when a specific action's state changes (icon, badge, visibility).
  virtual void OnActionChanged(AstraPageActionsModel* model,
                               AstraPageActionType type) {}

  // Called when the model is about to be destroyed.
  virtual void OnPageActionsModelShutdown(AstraPageActionsModel* model) {}

 protected:
  ~AstraPageActionsObserver() override = default;
};

// Model for the page actions container.
//
// Owns the list of page action items and their state.  The view observes
// this model and updates its UI when the model changes.
//
// State ownership:
//   - Chromium-owned actions (bookmark, zoom, translate, etc.) are
//     projections — state comes from Chromium subsystems, this model
//     mirrors it for the Astra UI.
//   - Astra-specific actions (focus mode, command palette, etc.) are
//     owned by this model and reflect Astra product state.
//   - Extension actions come from the extensions system and are projected.
//
// Chromium owner: PageActionIconController
//   (chrome/browser/ui/page_action/page_action_icon_controller.h)
//
// TODO(astra): Wire up to real Chromium page action controllers via
// a patch to chrome/browser/ui/page_action/page_action_icon_controller.cc.
class AstraPageActionsModel {
 public:
  AstraPageActionsModel();
  ~AstraPageActionsModel();

  AstraPageActionsModel(const AstraPageActionsModel&) = delete;
  AstraPageActionsModel& operator=(const AstraPageActionsModel&) = delete;

  // -- Observer management --------------------------------------------------

  void AddObserver(AstraPageActionsObserver* observer);
  void RemoveObserver(AstraPageActionsObserver* observer);

  // -- Action list ----------------------------------------------------------

  // Returns the list of all actions (visible and hidden, pinned and not).
  const std::vector<AstraPageActionItem>& GetAllActions() const;

  // Returns actions that are both visible and pinned (shown in the main
  // row, not in the overflow menu).
  std::vector<AstraPageActionItem> GetPinnedActions() const;

  // Returns actions that are visible but not pinned (shown in overflow).
  std::vector<AstraPageActionItem> GetOverflowActions() const;

  // Returns the number of visible actions.
  size_t GetVisibleActionCount() const;

  // Returns the number of pinned (always-visible) actions.
  size_t GetPinnedActionCount() const;

  // Get a specific action by type. Returns nullptr if not found.
  const AstraPageActionItem* GetAction(AstraPageActionType type) const;

  // Get an extension action by extension ID. Returns nullptr if not found.
  const AstraPageActionItem* GetExtensionAction(
      const std::string& extension_id) const;

  // -- Action manipulation --------------------------------------------------

  // Add or update an action. If an action of the same type already exists,
  // it is replaced.
  void SetAction(const AstraPageActionItem& item);

  // Remove an action by type.
  void RemoveAction(AstraPageActionType type);

  // Remove an extension action by extension ID.
  void RemoveExtensionAction(const std::string& extension_id);

  // Update the state of an existing action.
  void SetActionState(AstraPageActionType type, AstraPageActionState state);

  // Update the badge of an existing action.
  void SetActionBadge(AstraPageActionType type,
                      const std::u16string& badge_text,
                      SkColor badge_color);

  // Update the visibility of an existing action.
  void SetActionVisible(AstraPageActionType type, bool visible);

  // Update the pinned state of an existing action.
  void SetActionPinned(AstraPageActionType type, bool pinned);

  // Update the order of an existing action.
  void SetActionOrder(AstraPageActionType type, int order);

  // -- Bulk operations ------------------------------------------------------

  // Populate the model with the default set of Astra and Chromium actions.
  void PopulateDefaultActions();

  // Clear all actions.
  void ClearAllActions();

  // -- Extension actions ----------------------------------------------------

  // Add or update an extension action.
  void SetExtensionAction(const std::string& extension_id,
                          const std::u16string& name,
                          const std::string& icon_name,
                          bool pinned = false);

  // Returns all extension actions.
  std::vector<AstraPageActionItem> GetExtensionActions() const;

  // -- Visibility budget ----------------------------------------------------

  // Set the maximum number of actions to show in the pinned row before
  // overflowing. 0 = no limit (all pinned actions shown).
  void SetMaxVisibleActions(int max);
  int GetMaxVisibleActions() const;

  // Returns true if the action at the given index would overflow given
  // the current max visible count.
  bool WouldOverflow(size_t index) const;

  // -- Compact mode ---------------------------------------------------------

  // Set compact mode (smaller icons, tighter spacing hint for view).
  void SetCompactMode(bool compact);
  bool GetCompactMode() const;

 private:
  // Notify observers that the full action list has changed.
  void NotifyActionsChanged();

  // Notify observers that a specific action has changed.
  void NotifyActionChanged(AstraPageActionType type);

  // Sort actions by order.
  void SortActions();

  // Find the index of an action by type. Returns -1 if not found.
  int FindActionIndex(AstraPageActionType type) const;

  // Find the index of an extension action by ID. Returns -1 if not found.
  int FindExtensionActionIndex(const std::string& extension_id) const;

  // The list of action items, sorted by order.
  std::vector<AstraPageActionItem> actions_;

  // Maximum visible actions before overflow (0 = no limit).
  int max_visible_actions_ = 0;

  // Compact mode flag.
  bool compact_mode_ = false;

  // Observers.
  base::ObserverList<AstraPageActionsObserver> observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_PAGE_ACTIONS_ASTRA_PAGE_ACTIONS_MODEL_H_
