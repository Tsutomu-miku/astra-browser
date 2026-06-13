#ifndef ASTRA_UI_VIEWS_OMNIBOX_ASTRA_OMNIBOX_DECORATION_MODEL_H_
#define ASTRA_UI_VIEWS_OMNIBOX_ASTRA_OMNIBOX_DECORATION_MODEL_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"

class PrefService;

namespace astra {

// =========================================================================
// Astra omnibox decoration model
// =========================================================================
//
// AstraOmniboxDecorationModel owns the state and logic for the Astra
// location bar decoration.  It manages action buttons, presentation
// settings, and persistence via PrefService.  The view layer observes
// the model and renders state — it never owns truth.
//
// Architecture:
//   - Model owns state, logic, and persistence.
//   - View renders state and forwards user input to the model/delegate.
//   - Observers get notified when state changes so views can update.
//
// Chromium component: PrefService (persistence),
//   IconLabelBubbleView (location bar decoration pattern).
// Patch point: chrome/browser/ui/views/location_bar/location_bar_view.cc
// =========================================================================

// Security level — mirrors Chromium's security_state::SecurityLevel
// so the model can track security state without depending on
// //components/security_state directly.
enum class AstraSecurityLevel {
  kNone,
  kNeutral,
  kSecure,
  kInsecure,
  kDangerous,
};

// Icon size for decoration action buttons.
enum class AstraDecorationIconSize {
  kSmall,   // 16 dp
  kMedium,  // 20 dp
  kLarge,   // 24 dp
};

// Button style for decoration action buttons.
enum class AstraDecorationButtonStyle {
  kIconOnly,       // Just an icon
  kIconWithLabel,  // Icon + text label
  kChip,           // Rounded pill shape with icon + label
};

// Decoration position relative to the omnibox.
enum class AstraDecorationPosition {
  kLeading,   // Left side (before the security icon / URL)
  kTrailing,  // Right side (after the star / other icons)
};

// A single action button definition for the omnibox decoration.
struct AstraDecorationAction {
  // Unique identifier for the action (e.g. "workspace", "screenshot").
  std::string id;

  // Human-readable label shown when button style includes text.
  std::u16string label;

  // Icon identifier (placeholder for future vector icon system).
  // TODO(astra): Replace with gfx::VectorIcon or similar.
  std::string icon;

  // Tooltip text shown on hover.
  std::u16string tooltip;

  // Whether the action is currently visible.
  bool is_visible = true;

  // Position / priority in the action list.  Lower values appear first.
  int position = 0;

  // Keyboard shortcut hint (e.g. "⌘Shift+S").
  std::u16string shortcut;
};

// =========================================================================
// Observer interface
// =========================================================================
//
// All observer methods have empty default implementations.  Views only
// override the methods they care about.
// =========================================================================

class AstraOmniboxDecorationModelObserver : public base::CheckedObserver {
 public:
  // Called when a new action is added to the decoration.
  virtual void OnActionAdded(const std::string& action_id) {}

  // Called when an action is removed from the decoration.
  virtual void OnActionRemoved(const std::string& action_id) {}

  // Called when an action's visibility changes.
  virtual void OnActionVisibilityChanged(const std::string& action_id,
                                         bool visible) {}

  // Called when the order of actions changes.
  virtual void OnActionOrderChanged() {}

  // Called when decoration presentation settings change.
  virtual void OnDecorationSettingsChanged() {}

  // Called when the omnibox focus state changes.
  virtual void OnOmniboxFocusChanged(bool focused) {}

  // Called when the page security state changes.
  virtual void OnSecurityStateChanged(AstraSecurityLevel level) {}

 protected:
  ~AstraOmniboxDecorationModelObserver() override = default;
};

// =========================================================================
// Model class
// =========================================================================

class AstraOmniboxDecorationModel {
 public:
  // Default action IDs for the 8 Astra feature buttons.
  static constexpr const char* kActionWorkspace = "workspace";
  static constexpr const char* kActionFocusMode = "focus_mode";
  static constexpr const char* kActionScreenshot = "screenshot";
  static constexpr const char* kActionNote = "note";
  static constexpr const char* kActionSplitView = "split_view";
  static constexpr const char* kActionReadingList = "reading_list";
  static constexpr const char* kActionTranslate = "translate";
  static constexpr const char* kActionShare = "share";

  // Minimum and maximum values for max_visible_actions.
  static constexpr int kMinVisibleActions = 2;
  static constexpr int kMaxVisibleActions = 8;

  AstraOmniboxDecorationModel();
  ~AstraOmniboxDecorationModel();

  AstraOmniboxDecorationModel(const AstraOmniboxDecorationModel&) = delete;
  AstraOmniboxDecorationModel& operator=(
      const AstraOmniboxDecorationModel&) = delete;

  // -- Action management ---------------------------------------------------

  // Adds a new action.  Returns false if an action with the same ID
  // already exists.  Notifies OnActionAdded.
  bool AddAction(const AstraDecorationAction& action);

  // Removes an action by ID.  Returns false if not found.
  // Notifies OnActionRemoved.
  bool RemoveAction(const std::string& action_id);

  // Sets whether an action is visible.  Returns false if not found.
  // Notifies OnActionVisibilityChanged.
  bool SetActionVisible(const std::string& action_id, bool visible);

  // Returns true if an action with the given ID exists.
  bool HasAction(const std::string& action_id) const;

  // Gets an action by ID.  Returns nullptr if not found.
  const AstraDecorationAction* GetAction(const std::string& action_id) const;

  // Returns all actions in their current order.
  const std::vector<AstraDecorationAction>& GetAllActions() const {
    return actions_;
  }

  // Returns only the visible actions in order.
  std::vector<AstraDecorationAction> GetVisibleActions() const;

  // Returns the number of visible actions.
  size_t GetVisibleActionCount() const;

  // Returns the total number of actions (visible + hidden).
  size_t GetTotalActionCount() const { return actions_.size(); }

  // -- Action ordering -----------------------------------------------------

  // Reorders actions so the action at |from_index| moves to |to_index|.
  // Returns false if either index is out of range.
  // Notifies OnActionOrderChanged.
  bool ReorderAction(size_t from_index, size_t to_index);

  // Moves an action by ID to a specific position index.
  // Returns false if the action is not found.
  bool MoveActionTo(const std::string& action_id, size_t index);

  // Resets the action order to the default order.
  // Notifies OnActionOrderChanged.
  void ResetActionOrder();

  // Returns the default action order as a list of action IDs.
  static std::vector<std::string> GetDefaultActionOrder();

  // -- Omnibox state -------------------------------------------------------

  // Sets whether the omnibox is focused.
  // Notifies OnOmniboxFocusChanged.
  void SetOmniboxFocused(bool focused);

  // Returns whether the omnibox is currently focused.
  bool omnibox_focused() const { return omnibox_focused_; }

  // Sets the current security level.
  // Notifies OnSecurityStateChanged.
  void SetSecurityLevel(AstraSecurityLevel level);

  // Returns the current security level.
  AstraSecurityLevel security_level() const { return security_level_; }

  // -- Presentation settings -----------------------------------------------

  // Whether the Astra decoration is shown in the omnibox at all.
  bool show_decoration() const { return show_decoration_; }
  void SetShowDecoration(bool show);

  // Which side of the omnibox the decoration appears on.
  AstraDecorationPosition position() const { return position_; }
  void SetPosition(AstraDecorationPosition pos);

  // Maximum number of action buttons visible before overflow.
  // Clamped to [kMinVisibleActions, kMaxVisibleActions].
  int max_visible_actions() const { return max_visible_actions_; }
  void SetMaxVisibleActions(int max);

  // Whether to show text labels on action buttons.
  bool show_labels() const { return show_labels_; }
  void SetShowLabels(bool show);

  // Icon size for action buttons.
  AstraDecorationIconSize icon_size() const { return icon_size_; }
  void SetIconSize(AstraDecorationIconSize size);

  // Button style for action buttons.
  AstraDecorationButtonStyle button_style() const { return button_style_; }
  void SetButtonStyle(AstraDecorationButtonStyle style);

  // Whether to only show the decoration when the omnibox is focused.
  bool show_on_focus_only() const { return show_on_focus_only_; }
  void SetShowOnFocusOnly(bool show);

  // Individual action visibility toggles.
  bool show_workspace() const { return show_workspace_; }
  void SetShowWorkspace(bool show);

  bool show_focus_mode() const { return show_focus_mode_; }
  void SetShowFocusMode(bool show);

  bool show_screenshot() const { return show_screenshot_; }
  void SetShowScreenshot(bool show);

  bool show_note() const { return show_note_; }
  void SetShowNote(bool show);

  bool show_split_view() const { return show_split_view_; }
  void SetShowSplitView(bool show);

  bool show_reading_list() const { return show_reading_list_; }
  void SetShowReadingList(bool show);

  bool show_translate() const { return show_translate_; }
  void SetShowTranslate(bool show);

  bool show_share() const { return show_share_; }
  void SetShowShare(bool show);

  // Whether to show an overflow menu for hidden actions.
  bool show_overflow_menu() const { return show_overflow_menu_; }
  void SetShowOverflowMenu(bool show);

  // Whether the decoration expands on hover.
  bool hover_expansion() const { return hover_expansion_; }
  void SetHoverExpansion(bool enabled);

  // -- Utility methods -----------------------------------------------------

  // Formats an action label for display, truncating if too long.
  static std::u16string FormatActionLabel(const std::u16string& label,
                                          size_t max_length = 12);

  // Clamps a max_visible_actions value to the valid range.
  static int ClampMaxVisibleActions(int value);

  // Returns the pixel size for the current icon size setting.
  static int GetIconSizeDp(AstraDecorationIconSize size);

  // -- Persistence ---------------------------------------------------------

  // Loads all presentation settings from PrefService.
  // Notifies OnDecorationSettingsChanged.
  void LoadFromPrefs(PrefService* prefs);

  // Saves all presentation settings to PrefService.
  void SaveToPrefs(PrefService* prefs) const;

  // -- Observers -----------------------------------------------------------

  void AddObserver(AstraOmniboxDecorationModelObserver* observer);
  void RemoveObserver(AstraOmniboxDecorationModelObserver* observer);

  // -- Bulk operations -----------------------------------------------------

  // Sets visibility for multiple actions at once.
  // Notifies OnActionVisibilityChanged for each action whose visibility
  // actually changes.
  void SetBulkVisibility(const std::vector<std::string>& action_ids,
                         bool visible);

  // Shows all default actions.  Notifies relevant observers.
  void ShowAllDefaultActions();

  // Hides all actions.  Notifies relevant observers.
  void HideAllActions();

 private:
  // Populates actions_ with the default set of Astra actions.
  void InitializeDefaultActions();

  // Finds the index of an action by ID.  Returns -1 if not found.
  int FindActionIndex(const std::string& action_id) const;

  // Syncs individual action visibility flags with the actual action list.
  // Called when individual show_* settings change.
  void SyncActionVisibilityFromSettings();

  // Notifies all observers that settings changed.
  void NotifySettingsChanged();

  // -- Data ----------------------------------------------------------------

  // All action buttons in their current order.
  std::vector<AstraDecorationAction> actions_;

  // Omnibox focus state.
  bool omnibox_focused_ = false;

  // Current page security level.
  AstraSecurityLevel security_level_ = AstraSecurityLevel::kNone;

  // -- Presentation settings ----------------------------------------------

  bool show_decoration_ = true;
  AstraDecorationPosition position_ = AstraDecorationPosition::kLeading;
  int max_visible_actions_ = 4;
  bool show_labels_ = false;
  AstraDecorationIconSize icon_size_ = AstraDecorationIconSize::kMedium;
  AstraDecorationButtonStyle button_style_ =
      AstraDecorationButtonStyle::kIconOnly;
  bool show_on_focus_only_ = false;
  bool show_workspace_ = true;
  bool show_focus_mode_ = true;
  bool show_screenshot_ = true;
  bool show_note_ = true;
  bool show_split_view_ = true;
  bool show_reading_list_ = true;
  bool show_translate_ = true;
  bool show_share_ = true;
  bool show_overflow_menu_ = true;
  bool hover_expansion_ = false;

  // Observers.
  base::ObserverList<AstraOmniboxDecorationModelObserver> observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_OMNIBOX_ASTRA_OMNIBOX_DECORATION_MODEL_H_
