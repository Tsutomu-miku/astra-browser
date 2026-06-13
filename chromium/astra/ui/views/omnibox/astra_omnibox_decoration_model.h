#ifndef ASTRA_UI_VIEWS_OMNIBOX_ASTRA_OMNIBOX_DECORATION_MODEL_H_
#define ASTRA_UI_VIEWS_OMNIBOX_ASTRA_OMNIBOX_DECORATION_MODEL_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "third_party/skia/include/core/SkColor.h"

class PrefService;

namespace astra {

// =========================================================================
// Astra omnibox decoration model
// =========================================================================
//
// AstraOmniboxDecorationModel owns the state and logic for the Astra
// location bar decoration.  It manages typed decoration items, presentation
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

// Type of omnibox decoration item.  Each decoration has its own semantics
// and visual representation in the location bar.
enum class AstraOmniboxDecorationType {
  kNone = 0,
  kWorkspaceIndicator,   // Shows current workspace name / color dot
  kFocusModeBadge,        // Shows focus mode active indicator
  kTabStackIndicator,     // Shows tab stack membership
  kReadingListBadge,      // "Add to reading list" button
  kNoteBadge,             // "Add note" button
  kFavoriteStar,          // "Favorite" star button
  kSidebarToggle,         // Sidebar toggle button
  kSplitViewToggle,       // Split view button
  kTranslateButton,       // Translate page button
  kAstraActionButton,   // Generic Astra action button
};

// Number of decoration types (excluding kNone).
constexpr size_t kNumDecorationTypes = 10;

// Decoration position relative to the omnibox.
enum class AstraDecorationPosition {
  kLeading,   // Left side (before the security icon / URL)
  kTrailing,  // Right side (after the star / other icons)
};

// A single decoration item in the omnibox decoration bar.
// Each decoration has a type, visibility, visual properties, and optional
// command association.
struct AstraOmniboxDecorationItem {
  // The type of this decoration.
  AstraOmniboxDecorationType type = AstraOmniboxDecorationType::kNone;

  // Whether this decoration is currently visible.
  bool is_visible = true;

  // Whether this decoration is in an "active" state (e.g. focus mode on,
  // sidebar open, page favorited).  Affects visual styling.
  bool is_active = false;

  // Icon identifier (placeholder for future vector icon system).
  // TODO(astra): Replace with gfx::VectorIcon or similar.
  std::string icon;

  // Tooltip text shown on hover.
  std::u16string tooltip;

  // Accessibility label for screen readers.
  std::u16string accessibility_label;

  // Order index — lower values appear first in the decoration row.
  int order_index = 0;

  // Whether clicking this decoration shows a bubble popup.
  bool has_bubble = false;

  // Small badge text shown on the decoration icon (e.g. unread count).
  std::u16string badge_text;

  // Background color of the badge.
  SkColor badge_color = SK_ColorTRANSPARENT;

  // Command ID to execute when the decoration is clicked.
  // Maps to an Astra command ID for execution.
  int command_id = 0;
};

// =========================================================================
// Observer interface
// =========================================================================
//
// All observer methods have empty default implementations.  Views only
// override the methods they care about.
//
// Observer pattern follows Chromium conventions:
//   - Derives from base::CheckedObserver for safe removal during iteration.
//   - Managed via base::ObserverList.
// =========================================================================

class AstraOmniboxDecorationObserver : public base::CheckedObserver {
 public:
  // Called when a decoration's visibility changes.
  virtual void OnDecorationVisibilityChanged(
      AstraOmniboxDecorationModel* model,
      AstraOmniboxDecorationType type,
      bool visible) {}

  // Called when a decoration's active state changes.
  virtual void OnDecorationActiveChanged(
      AstraOmniboxDecorationModel* model,
      AstraOmniboxDecorationType type,
      bool active) {}

  // Called when a decoration's badge changes (text or color).
  virtual void OnDecorationBadgeChanged(
      AstraOmniboxDecorationModel* model,
      AstraOmniboxDecorationType type) {}

  // Called when the order of decorations changes.
  virtual void OnDecorationsReordered(
      AstraOmniboxDecorationModel* model) {}

  // Called when a decoration action is executed.
  virtual void OnDecorationExecuted(
      AstraOmniboxDecorationModel* model,
      AstraOmniboxDecorationType type) {}

  // Called when a decoration's bubble is shown.
  virtual void OnBubbleShown(
      AstraOmniboxDecorationModel* model,
      AstraOmniboxDecorationType type) {}

  // Called when a decoration's bubble is hidden.
  virtual void OnBubbleHidden(
      AstraOmniboxDecorationModel* model,
      AstraOmniboxDecorationType type) {}

  // Called when the current workspace changes (name, color, etc.).
  virtual void OnWorkspaceChanged(
      AstraOmniboxDecorationModel* model,
      const std::u16string& name) {}

  // Called when focus mode state changes.
  virtual void OnFocusModeChanged(
      AstraOmniboxDecorationModel* model,
      bool active) {}

  // Called when the model is shutting down.  Observers should remove
  // themselves and clear their pointers.
  virtual void OnOmniboxDecorationModelShutdown(
      AstraOmniboxDecorationModel* model) {}

 protected:
  ~AstraOmniboxDecorationObserver() override = default;
};

// =========================================================================
// Model class
// =========================================================================
//
// Owns all omnibox decoration state.
// Decorations are typed (AstraOmniboxDecorationType) and have individual
// state (visible/active/badge).  The model is the single source of truth
// for decoration state.
//
// The model also tracks workspace, focus mode, tab stack, reading list, note,
// favorite, sidebar, and split view presentation state — these are projected
// onto specific decoration items.
// =========================================================================

class AstraOmniboxDecorationModel {
 public:
  // -----------------------------------------------------------------------
  // Settings / Pref keys (public static constexpr for registration
  // These match the pref key strings used with PrefService.
  // TODO(astra): Register these in AstraPrefs.
  // -----------------------------------------------------------------------

  static constexpr const char* kSettingShowWorkspaceIndicator =
      "astra.omnibox.decoration.show_workspace_indicator";
  static constexpr const char* kSettingWorkspaceIndicatorPosition =
      "astra.omnibox.decoration.workspace_indicator_position";
  static constexpr const char* kSettingShowFocusModeBadge =
      "astra.omnibox.decoration.show_focus_mode_badge";
  static constexpr const char* kSettingShowTabStackIndicator =
      "astra.omnibox.decoration.show_tab_stack_indicator";
  static constexpr const char* kSettingShowReadingListButton =
      "astra.omnibox.decoration.show_reading_list_button";
  static constexpr const char* kSettingShowNoteButton =
      "astra.omnibox.decoration.show_note_button";
  static constexpr const char* kSettingShowFavoriteStar =
      "astra.omnibox.decoration.show_favorite_star";
  static constexpr const char* kSettingShowSidebarToggle =
      "astra.omnibox.decoration.show_sidebar_toggle";
  static constexpr const char* kSettingShowSplitViewButton =
      "astra.omnibox.decoration.show_split_view_button";
  static constexpr const char* kSettingShowTranslateButton =
      "astra.omnibox.decoration.show_translate_button";
  static constexpr const char* kSettingDecorationOrder =
      "astra.omnibox.decoration.decoration_order";
  static constexpr const char* kSettingShowBadgesOnHoverOnly =
      "astra.omnibox.decoration.show_badges_on_hover_only";
  static constexpr const char* kSettingCompactMode =
      "astra.omnibox.decoration.compact_mode";
  static constexpr const char* kSettingAnimationEnabled =
      "astra.omnibox.decoration.animation_enabled";
  static constexpr const char* kSettingDecorationIconSize =
      "astra.omnibox.decoration.icon_size";

  // Default values for settings.
  static constexpr bool kDefaultShowWorkspaceIndicator = true;
  static constexpr const char* kDefaultWorkspaceIndicatorPosition = "left";
  static constexpr bool kDefaultShowFocusModeBadge = true;
  static constexpr bool kDefaultShowTabStackIndicator = true;
  static constexpr bool kDefaultShowReadingListButton = true;
  static constexpr bool kDefaultShowNoteButton = true;
  static constexpr bool kDefaultShowFavoriteStar = true;
  static constexpr bool kDefaultShowSidebarToggle = true;
  static constexpr bool kDefaultShowSplitViewButton = true;
  static constexpr bool kDefaultShowTranslateButton = true;
  static constexpr bool kDefaultShowBadgesOnHoverOnly = false;
  static constexpr bool kDefaultCompactMode = false;
  static constexpr bool kDefaultAnimationEnabled = true;
  static constexpr int kDefaultDecorationIconSize = 20;

  // Minimum and maximum icon size in pixels.
  static constexpr int kMinIconSize = 16;
  static constexpr int kMaxIconSize = 32;

  AstraOmniboxDecorationModel();
  ~AstraOmniboxDecorationModel();

  AstraOmniboxDecorationModel(const AstraOmniboxDecorationModel&) = delete;
  AstraOmniboxDecorationModel& operator=(
      const AstraOmniboxDecorationModel&) = delete;

  // -- Decoration item access ----------------------------------------------

  // Returns all decoration items in their current order.
  const std::vector<AstraOmniboxDecorationItem>& GetDecorations() const;

  // Returns the total number of decoration items.
  int GetDecorationCount() const;

  // Returns the decoration at |index| in the current order.
  // Returns nullptr if index is out of range.
  const AstraOmniboxDecorationItem* GetDecorationAt(int index) const;

  // Returns the decoration of the given type, or nullptr if not found.
  const AstraOmniboxDecorationItem* GetDecorationByType(
      AstraOmniboxDecorationType type) const;

  // -- Decoration visibility ---------------------------------------------

  // Sets whether a decoration is visible.  Notifies
  // OnDecorationVisibilityChanged.
  void SetDecorationVisible(AstraOmniboxDecorationType type, bool visible);

  // Returns whether a decoration is visible.
  bool IsDecorationVisible(AstraOmniboxDecorationType type) const;

  // -- Decoration active state --------------------------------------------

  // Sets whether a decoration is active.  Notifies
  // OnDecorationActiveChanged.
  void SetDecorationActive(AstraOmniboxDecorationType type, bool active);

  // Returns whether a decoration is active.
  bool IsDecorationActive(AstraOmniboxDecorationType type) const;

  // -- Decoration tooltip --------------------------------------------------

  // Sets the tooltip text for a decoration.
  void SetDecorationTooltip(AstraOmniboxDecorationType type,
                           const std::u16string& tooltip);

  // -- Decoration badges ---------------------------------------------------

  // Sets the badge text and color for a decoration.
  // Notifies OnDecorationBadgeChanged.
  void SetDecorationBadge(AstraOmniboxDecorationType type,
                         const std::u16string& badge_text,
                         SkColor color);

  // Clears the badge for a decoration (sets badge to empty / transparent).
  // Notifies OnDecorationBadgeChanged.
  void ClearDecorationBadge(AstraOmniboxDecorationType type);

  // -- Decoration ordering ------------------------------------------------

  // Reorders decorations according to the given order of types.
  // Types not in the list keep their relative order at the end.
  // Notifies OnDecorationsReordered.
  void ReorderDecorations(
      const std::vector<AstraOmniboxDecorationType>& order);

  // Returns the current decoration order as a list of types.
  std::vector<AstraOmniboxDecorationType> GetDecorationOrder() const;

  // Resets decoration order to the default order.
  // Notifies OnDecorationsReordered.
  void ResetDecorationOrder();

  // -- Decoration execution -----------------------------------------------

  // Executes the action associated with a decoration.
  // Notifies OnDecorationExecuted.
  void ExecuteDecoration(AstraOmniboxDecorationType type);

  // -- Bubble management --------------------------------------------------

  // Shows the bubble for a decoration.
  // Notifies OnBubbleShown.
  void ShowDecorationBubble(AstraOmniboxDecorationType type);

  // Hides the bubble for a decoration.
  // Notifies OnBubbleHidden.
  void HideDecorationBubble(AstraOmniboxDecorationType type);

  // Returns the type of the currently open bubble, or kNone if no bubble.
  AstraOmniboxDecorationType GetOpenBubbleType() const;

  // Hides all open bubbles.
  // Notifies OnBubbleHidden for each open bubble.
  void HideAllBubbles();

  // -- Workspace decoration state ------------------------------------------

  // Sets the current workspace name.
  // Updates the workspace indicator decoration.
  // Notifies OnWorkspaceChanged.
  void SetCurrentWorkspaceName(const std::u16string& name);

  // Returns the current workspace name.
  const std::u16string& GetCurrentWorkspaceName() const;

  // Sets the workspace accent color.
  void SetWorkspaceColor(SkColor color);

  // Returns the workspace accent color.
  SkColor GetWorkspaceColor() const;

  // Sets the workspace badge count (number of workspaces / unread count).
  void SetWorkspaceBadgeCount(int count);

  // Returns the workspace badge count.
  int GetWorkspaceBadgeCount() const;

  // Sets whether the workspace indicator is shown.
  void SetShowWorkspaceIndicator(bool show);

  // Returns whether the workspace indicator is shown.
  bool GetShowWorkspaceIndicator() const;

  // -- Focus mode decoration state ---------------------------------------

  // Sets whether focus mode is active.
  // Updates the focus mode badge decoration.
  // Notifies OnFocusModeChanged and OnDecorationActiveChanged.
  void SetFocusModeActive(bool active);

  // Returns whether focus mode is active.
  bool IsFocusModeActive() const;

  // Sets the remaining time in focus mode.
  void SetFocusModeTimeRemaining(base::TimeDelta remaining);

  // Returns the remaining time in focus mode.
  base::TimeDelta GetFocusModeTimeRemaining() const;

  // Sets whether the focus mode badge is shown.
  void SetShowFocusModeBadge(bool show);

  // Returns whether the focus mode badge is shown.
  bool GetShowFocusModeBadge() const;

  // Sets the focus mode indicator color.
  void SetFocusModeColor(SkColor color);

  // Returns the focus mode indicator color.
  SkColor GetFocusModeColor() const;

  // -- Tab stack decoration state ----------------------------------------

  // Sets the current tab stack name.
  void SetTabStackName(const std::u16string& name);

  // Returns the current tab stack name.
  const std::u16string& GetTabStackName() const;

  // Sets the tab stack color.
  void SetTabStackColor(SkColor color);

  // Returns the tab stack color.
  SkColor GetTabStackColor() const;

  // Sets the number of tabs in the current stack.
  void SetTabStackTabCount(int count);

  // Returns the number of tabs in the current stack.
  int GetTabStackTabCount() const;

  // Sets whether the tab stack indicator is shown.
  void SetShowTabStackIndicator(bool show);

  // Returns whether the tab stack indicator is shown.
  bool GetShowTabStackIndicator() const;

  // -- Reading list decoration state -------------------------------------

  // Sets whether the current page is in the reading list.
  // Updates the reading list badge decoration active state.
  void SetIsInReadingList(bool in_list);

  // Returns whether the current page is in the reading list.
  bool IsInReadingList() const;

  // Sets whether the reading list button is shown.
  void SetShowReadingListButton(bool show);

  // Returns whether the reading list button is shown.
  bool GetShowReadingListButton() const;

  // -- Note decoration state ---------------------------------------------

  // Sets whether the current page has a note.
  // Updates the note badge decoration active state.
  void SetHasNote(bool has_note);

  // Returns whether the current page has a note.
  bool HasNote() const;

  // Sets the note preview text (shown in tooltip / bubble).
  void SetNotePreview(const std::u16string& preview);

  // Returns the note preview text.
  const std::u16string& GetNotePreview() const;

  // Sets whether the note button is shown.
  void SetShowNoteButton(bool show);

  // Returns whether the note button is shown.
  bool GetShowNoteButton() const;

  // -- Favorite decoration state -----------------------------------------

  // Sets whether the current page is favorited.
  // Updates the favorite star decoration active state.
  void SetIsFavorited(bool favorited);

  // Returns whether the current page is favorited.
  bool IsFavorited() const;

  // Sets whether the favorite star is shown.
  void SetShowFavoriteStar(bool show);

  // Returns whether the favorite star is shown.
  bool GetShowFavoriteStar() const;

  // -- Sidebar decoration state ------------------------------------------

  // Sets whether the sidebar is open.
  // Updates the sidebar toggle decoration active state.
  void SetSidebarOpen(bool open);

  // Returns whether the sidebar is open.
  bool IsSidebarOpen() const;

  // Sets whether the sidebar toggle is shown.
  void SetShowSidebarToggle(bool show);

  // Returns whether the sidebar toggle is shown.
  bool GetShowSidebarToggle() const;

  // -- Split view decoration state ---------------------------------------

  // Sets whether split view is active.
  // Updates the split view toggle decoration active state.
  void SetSplitViewActive(bool active);

  // Returns whether split view is active.
  bool IsSplitViewActive() const;

  // Sets whether the split view button is shown.
  void SetShowSplitViewButton(bool show);

  // Returns whether the split view button is shown.
  bool GetShowSplitViewButton() const;

  // -- Presentation settings ----------------------------------------------

  // Sets whether badges are only shown on hover.
  void SetShowBadgesOnHoverOnly(bool show);
  bool GetShowBadgesOnHoverOnly() const;

  // Sets whether compact mode is enabled (smaller decorations).
  void SetCompactMode(bool compact);
  bool GetCompactMode() const;

  // Sets whether animations are enabled.
  void SetAnimationEnabled(bool enabled);
  bool GetAnimationEnabled() const;

  // Sets the decoration icon size in pixels.
  // Clamped to [kMinIconSize, kMaxIconSize].
  void SetDecorationIconSize(int size_px);
  int GetDecorationIconSize() const;

  // -- Observers -----------------------------------------------------------

  void AddObserver(AstraOmniboxDecorationObserver* observer);
  void RemoveObserver(AstraOmniboxDecorationObserver* observer);

  // -- Persistence ---------------------------------------------------------

  // Loads all settings from PrefService.
  void LoadFromPrefs(PrefService* prefs);

  // Saves all settings to PrefService.
  void SaveToPrefs(PrefService* prefs) const;

  // -- Utility ---------------------------------------------------------------

  // Returns the default decoration order as a list of types.
  static std::vector<AstraOmniboxDecorationType> GetDefaultDecorationOrder();

  // Clamps an icon size value to the valid range.
  static int ClampIconSize(int size);

 private:
  // Populates decorations_ with the default set of Astra decorations.
  void InitializeDefaultDecorations();

  // Finds the index of a decoration by type.  Returns -1 if not found.
  int FindDecorationIndex(AstraOmniboxDecorationType type) const;

  // Returns a mutable pointer to a decoration by type, or nullptr.
  AstraOmniboxDecorationItem* GetMutableDecoration(
      AstraOmniboxDecorationType type);

  // Notifies observers that visibility changed.
  void NotifyVisibilityChanged(AstraOmniboxDecorationType type, bool visible);

  // Notifies observers that active state changed.
  void NotifyActiveChanged(AstraOmniboxDecorationType type, bool active);

  // Notifies observers that badge changed.
  void NotifyBadgeChanged(AstraOmniboxDecorationType type);

  // Notifies observers that decorations were reordered.
  void NotifyReordered();

  // Notifies observers that a decoration was executed.
  void NotifyExecuted(AstraOmniboxDecorationType type);

  // Notifies observers that a bubble was shown.
  void NotifyBubbleShown(AstraOmniboxDecorationType type);

  // Notifies observers that a bubble was hidden.
  void NotifyBubbleHidden(AstraOmniboxDecorationType type);

  // Notifies observers that workspace changed.
  void NotifyWorkspaceChanged(const std::u16string& name);

  // Notifies observers that focus mode changed.
  void NotifyFocusModeChanged(bool active);

  // Notifies observers of shutdown.
  void NotifyShutdown();

  // -- Data ----------------------------------------------------------------

  // All decoration items in their current order.
  std::vector<AstraOmniboxDecorationItem> decorations_;

  // The currently open bubble type, or kNone.
  AstraOmniboxDecorationType open_bubble_type_ =
      AstraOmniboxDecorationType::kNone;

  // -- Workspace state ---------------------------------------------------

  std::u16string workspace_name_;
  SkColor workspace_color_ = SK_ColorGRAY;
  int workspace_badge_count_ = 0;
  bool show_workspace_indicator_ = kDefaultShowWorkspaceIndicator;

  // -- Focus mode state --------------------------------------------------

  bool focus_mode_active_ = false;
  base::TimeDelta focus_mode_time_remaining_;
  bool show_focus_mode_badge_ = kDefaultShowFocusModeBadge;
  SkColor focus_mode_color_ = SkColorSetRGB(0x5A, 0xD8, 0xA6);

  // -- Tab stack state ---------------------------------------------------

  std::u16string tab_stack_name_;
  SkColor tab_stack_color_ = SK_ColorGRAY;
  int tab_stack_tab_count_ = 0;
  bool show_tab_stack_indicator_ = kDefaultShowTabStackIndicator;

  // -- Reading list state ------------------------------------------------

  bool is_in_reading_list_ = false;
  bool show_reading_list_button_ = kDefaultShowReadingListButton;

  // -- Note state --------------------------------------------------------

  bool has_note_ = false;
  std::u16string note_preview_;
  bool show_note_button_ = kDefaultShowNoteButton;

  // -- Favorite state ----------------------------------------------------

  bool is_favorited_ = false;
  bool show_favorite_star_ = kDefaultShowFavoriteStar;

  // -- Sidebar state -----------------------------------------------------

  bool sidebar_open_ = false;
  bool show_sidebar_toggle_ = kDefaultShowSidebarToggle;

  // -- Split view state --------------------------------------------------

  bool split_view_active_ = false;
  bool show_split_view_button_ = kDefaultShowSplitViewButton;

  // -- Presentation settings ---------------------------------------------

  bool show_translate_button_ = kDefaultShowTranslateButton;
  bool show_badges_on_hover_only_ = kDefaultShowBadgesOnHoverOnly;
  bool compact_mode_ = kDefaultCompactMode;
  bool animation_enabled_ = kDefaultAnimationEnabled;
  int decoration_icon_size_ = kDefaultDecorationIconSize;

  // Observers.
  base::ObserverList<AstraOmniboxDecorationObserver> observers_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_OMNIBOX_ASTRA_OMNIBOX_DECORATION_MODEL_H_
