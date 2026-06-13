#ifndef ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_MODEL_H_
#define ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_MODEL_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class PrefService;

namespace astra {

// Sidebar position: left or right side of the browser window.
enum class AstraSidebarPosition {
  kLeft,
  kRight,
};

// =========================================================================
// AstraSidebarSection — metadata for a sidebar section
// =========================================================================
//
// Each sidebar section has an ID, display name, icon identifier, and
// visibility state.  Sections are the top-level organizational units of
// the sidebar (workspaces, favorites, tabs, history, downloads, etc.).
//
// The section struct is a value type — copies are cheap and the model
// returns copies so views cannot mutate model state directly.
struct AstraSidebarSection {
  // Unique identifier for the section (e.g. "favorites", "open_tabs").
  std::string id;

  // Human-readable display name (UTF-16 for i18n compatibility).
  std::u16string name;

  // Icon identifier.  The actual icon resource lookup is done by the view.
  std::string icon_id;

  // Whether the section is currently visible in the sidebar.
  bool is_visible = true;

  // Display position index (0-based).  Lower numbers appear higher in
  // the sidebar.
  int position = 0;

  // Whether this section can be collapsed (header-only mode).
  bool is_collapsible = true;

  // Whether this section is currently collapsed.
  bool is_collapsed = false;
};

// =========================================================================
// AstraSidebarModelObserver — observer interface for sidebar model changes
// =========================================================================
//
// All observer methods have empty default implementations so observers can
// override only the methods they care about.  This follows Chromium's
// CheckedObserver pattern for safe observer lifetime management.
//
// Chromium pattern: base::CheckedObserver (base/observer_list_types.h)
class AstraSidebarModelObserver : public base::CheckedObserver {
 public:
  // Called when the sidebar is shown (becomes visible).
  virtual void OnSidebarShown() {}

  // Called when the sidebar is hidden.
  virtual void OnSidebarHidden() {}

  // Called when the sidebar pinned state changes.
  virtual void OnSidebarPinnedChanged(bool pinned) {}

  // Called when the active section changes.
  // |section_id| is the ID of the newly active section.
  virtual void OnActiveSectionChanged(const std::string& section_id) {}

  // Called when a single section's visibility changes.
  virtual void OnSectionVisibilityChanged(const std::string& section_id,
                                          bool visible) {}

  // Called when the order of sections changes.
  virtual void OnSectionOrderChanged() {}

  // Called when a section's collapsed state changes.
  virtual void OnSectionCollapsedChanged(const std::string& section_id,
                                         bool collapsed) {}

  // Called when the sidebar width changes.
  virtual void OnSidebarWidthChanged(int width) {}

  // Called when the sidebar position (left/right) changes.
  virtual void OnSidebarPositionChanged(AstraSidebarPosition position) {}

  // Called when any sidebar presentation setting changes.
  // This is a catch-all for settings that don't have a specific observer
  // method (compact mode, show icons, show labels, etc.).
  virtual void OnSidebarSettingsChanged() {}

 protected:
  ~AstraSidebarModelObserver() override = default;
};

// =========================================================================
// AstraSidebarModel — state and settings model for the sidebar
// =========================================================================
//
// The model owns all sidebar UI state and presentation settings.
// It is the single source of truth for the sidebar views layer.
//
// Truth hierarchy:
//   - Chromium services (TabStripModel, HistoryService, etc.) — own
//     browser data truth.
//   - AstraSidebarModel (views-layer) — owns sidebar presentation state,
//     settings, section metadata, and visibility configuration.
//   - Views (sidebar, sections, items) — pure presentation, no state.
//
// The model persists presentation settings via PrefService.
//
// Chromium subsystems reused:
//   - PrefService (persistence for presentation settings).
//   - Observer pattern (base::CheckedObserver / base::ObserverList).
//
// Chromium patch point: none — this is pure Astra presentation-layer code.
class AstraSidebarModel {
 public:
  // Constructs a model backed by |pref_service| for settings persistence.
  // |pref_service| may be null for unit tests; in that case, settings use
  // in-memory defaults only.
  explicit AstraSidebarModel(PrefService* pref_service = nullptr);
  ~AstraSidebarModel();

  AstraSidebarModel(const AstraSidebarModel&) = delete;
  AstraSidebarModel& operator=(const AstraSidebarModel&) = delete;

  // -- Observers -----------------------------------------------------------

  void AddObserver(AstraSidebarModelObserver* observer);
  void RemoveObserver(AstraSidebarModelObserver* observer);

  // -- Sidebar state -------------------------------------------------------

  // Whether the sidebar is currently visible.
  bool is_visible() const;
  void SetVisible(bool visible);
  void ToggleVisible();

  // Whether the sidebar is pinned open.
  bool is_pinned() const;
  void SetPinned(bool pinned);
  void TogglePinned();

  // Sidebar width in pixels.
  int width() const;
  void SetWidth(int width);

  // Sidebar position (left or right).
  AstraSidebarPosition position() const;
  void SetPosition(AstraSidebarPosition position);

  // -- Active section ------------------------------------------------------

  // The ID of the currently active section.
  const std::string& active_section_id() const { return active_section_id_; }
  void SetActiveSection(const std::string& section_id);

  // -- Section management --------------------------------------------------

  // Returns all sections in their current order.
  std::vector<AstraSidebarSection> GetAllSections() const;

  // Returns only visible sections in their current order.
  std::vector<AstraSidebarSection> GetVisibleSections() const;

  // Returns only collapsed sections' IDs.
  std::vector<std::string> GetCollapsedSectionIds() const;

  // Gets a section by ID.  Returns nullopt if not found.
  absl::optional<AstraSidebarSection> GetSectionById(
      const std::string& section_id) const;

  // Sets the visibility of a specific section.
  // Returns true if the section exists and was updated.
  bool SetSectionVisible(const std::string& section_id, bool visible);

  // Toggles the visibility of a specific section.
  // Returns true if the section exists and was toggled.
  bool ToggleSectionVisible(const std::string& section_id);

  // Toggles the collapsed state of a specific section.
  // Returns true if the section exists and was toggled.
  bool ToggleSectionCollapsed(const std::string& section_id);

  // Sets the collapsed state of a specific section.
  // Returns true if the section exists and was updated.
  bool SetSectionCollapsed(const std::string& section_id, bool collapsed);

  // Reorders sections by moving a section to a new position.
  // Returns true if the section exists and was moved.
  bool MoveSection(const std::string& section_id, int new_position);

  // Reorders sections by providing a full ordered list of section IDs.
  // All IDs must correspond to existing sections.
  // Returns true if the reorder was successful.
  bool ReorderSections(const std::vector<std::string>& section_ids_in_order);

  // Returns the number of sections.
  size_t GetSectionCount() const;

  // Returns the number of visible sections.
  size_t GetVisibleSectionCount() const;

  // -- Bulk operations -----------------------------------------------------

  // Sets all sections' visibility to |visible|.
  void SetAllSectionsVisible(bool visible);

  // Toggles visibility for multiple sections at once.
  void ToggleMultipleSections(const std::vector<std::string>& section_ids);

  // Collapses all collapsible sections.
  void CollapseAllSections();

  // Expands (uncollapses) all sections.
  void ExpandAllSections();

  // Resets all sidebar settings to their default values.
  // Does not reset runtime state like active section or width.
  void ResetAllSettings();

  // Resets sections to their default configuration (order, visibility,
  // collapsed state).  Preserves runtime state like width and pinned.
  void ResetSectionsToDefaults();

  // -- Presentation settings (persisted via PrefService) ------------------

  // Whether section icons are shown.
  bool show_section_icons() const;
  void SetShowSectionIcons(bool show);

  // Whether section labels are shown.
  bool show_section_labels() const;
  void SetShowSectionLabels(bool show);

  // Whether compact mode is enabled.
  bool compact_mode() const;
  void SetCompactMode(bool enabled);

  // Whether the sidebar auto-hides when a tab is clicked.
  bool auto_hide_on_tab_click() const;
  void SetAutoHideOnTabClick(bool enabled);

  // Whether tab count badges are shown on section headers.
  bool show_tab_count_badges() const;
  void SetShowTabCountBadges(bool show);

  // Whether the workspace badge is shown in the sidebar header.
  bool show_workspace_badge() const;
  void SetShowWorkspaceBadge(bool show);

  // Whether animations are enabled for sidebar transitions.
  bool animation_enabled() const;
  void SetAnimationEnabled(bool enabled);

  // Whether to remember the last active section between sessions.
  bool remember_last_section() const;
  void SetRememberLastSection(bool remember);

  // The default active section (used when remember_last_section is false).
  std::string default_active_section() const;
  void SetDefaultActiveSection(const std::string& section_id);

  // The last active section (persisted for recall).
  std::string last_active_section() const;
  void SetLastActiveSection(const std::string& section_id);

  // -- Section visibility settings (persisted) ----------------------------
  //
  // These are convenience getters/setters for per-section visibility prefs.
  // They mirror the section management API but operate through prefs.

  bool show_tab_groups_section() const;
  void SetShowTabGroupsSection(bool show);

  bool show_history_section() const;
  void SetShowHistorySection(bool show);

  bool show_recently_closed_section() const;
  void SetShowRecentlyClosedSection(bool show);

  bool show_reading_list_section() const;
  void SetShowReadingListSection(bool show);

  bool show_notes_section() const;
  void SetShowNotesSection(bool show);

  bool show_downloads_section() const;
  void SetShowDownloadsSection(bool show);

  bool show_passwords_section() const;
  void SetShowPasswordsSection(bool show);

  bool show_extensions_section() const;
  void SetShowExtensionsSection(bool show);

  // -- Utility methods -----------------------------------------------------

  // Returns the default section list (all sections, default order,
  // all visible).
  static std::vector<AstraSidebarSection> GetDefaultSections();

  // Clamps a width value to the valid sidebar width range.
  // Returns the clamped value.
  static int ClampWidth(int width);

  // Minimum and maximum sidebar width constants.
  static constexpr int kMinWidth = 200;
  static constexpr int kMaxWidth = 500;
  static constexpr int kDefaultWidth = 280;

  // Known section ID constants.
  static const char kSectionWorkspaces[];
  static const char kSectionFavorites[];
  static const char kSectionPinnedTabs[];
  static const char kSectionOpenTabs[];
  static const char kSectionTabGroups[];
  static const char kSectionBookmarks[];
  static const char kSectionHistory[];
  static const char kSectionRecentlyClosed[];
  static const char kSectionReadingList[];
  static const char kSectionNotes[];
  static const char kSectionDownloads[];
  static const char kSectionPasswords[];
  static const char kSectionExtensions[];
  static const char kSectionDevTools[];

  PrefService* pref_service() { return pref_service_; }
  const PrefService* pref_service() const { return pref_service_; }

 private:
  // Loads state and settings from PrefService.
  void LoadFromPrefs();

  // Saves the current section order to PrefService.
  void SaveSectionOrderToPrefs();

  // Saves the current collapsed sections to PrefService.
  void SaveCollapsedSectionsToPrefs();

  // Loads section order from PrefService.
  void LoadSectionOrderFromPrefs();

  // Loads collapsed sections from PrefService.
  void LoadCollapsedSectionsFromPrefs();

  // Synchronizes section visibility states with per-section prefs.
  void SyncSectionVisibilityWithPrefs();

  // -- Observer notification helpers ---------------------------------------

  void NotifySidebarShown();
  void NotifySidebarHidden();
  void NotifySidebarPinnedChanged(bool pinned);
  void NotifyActiveSectionChanged(const std::string& section_id);
  void NotifySectionVisibilityChanged(const std::string& section_id,
                                      bool visible);
  void NotifySectionOrderChanged();
  void NotifySectionCollapsedChanged(const std::string& section_id,
                                     bool collapsed);
  void NotifySidebarWidthChanged(int width);
  void NotifySidebarPositionChanged(AstraSidebarPosition position);
  void NotifySidebarSettingsChanged();

  raw_ptr<PrefService> pref_service_ = nullptr;
  base::ObserverList<AstraSidebarModelObserver> observers_;

  // -- Runtime state -------------------------------------------------------

  // Currently active section ID.
  std::string active_section_id_;

  // -- Section data --------------------------------------------------------
  //
  // Sections are stored in a vector for ordered access.  Position is
  // implicit (index in the vector).  The vector is always kept sorted by
  // position.

  std::vector<AstraSidebarSection> sections_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SIDEBAR_ASTRA_SIDEBAR_MODEL_H_
