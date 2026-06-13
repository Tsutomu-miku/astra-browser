#ifndef ASTRA_UI_VIEWS_SETTINGS_ASTRA_SETTINGS_PAGE_VIEW_H_
#define ASTRA_UI_VIEWS_SETTINGS_ASTRA_SETTINGS_PAGE_VIEW_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list_types.h"
#include "base/scoped_observation.h"
#include "components/prefs/pref_change_registrar.h"
#include "ui/views/view.h"

#include "astra/browser/astra_focus_mode_service.h"
#include "astra/browser/astra_workspace_service.h"
#include "astra/ui/views/settings/astra_settings_model.h"

class PrefService;
class Browser;

namespace views {
class ToggleButton;
class Slider;
class Combobox;
class Label;
class MdTextButton;
class Textfield;
class ScrollView;
class Separator;
}  // namespace views

namespace astra {

class AstraSettingsSectionView;
class AstraSearchSettingsView;
class AstraSettingsSearchBox;

// Navigation entry types.
enum class AstraSettingsPageType {
  kMainPage,       // All sections overview
  kSection,        // Single section detail
  kSearchResults,  // Search results
  kSubpage,        // Named subpage (e.g. search engine settings)
};

struct AstraSettingsNavigationEntry {
  AstraSettingsPageType type = AstraSettingsPageType::kMainPage;
  std::string section_id;  // For kSection type
  std::u16string query;    // For kSearchResults type
  std::string subpage_id;  // For kSubpage type
  std::u16string title;    // Display title for breadcrumbs
};

// =========================================================================
// Astra settings page view — main content of the settings bubble
// =========================================================================
//
// AstraSettingsPageView is a Views-based settings page for Astra-specific
// preferences. It is displayed inside AstraSettingsBubble and provides
// controls for all Astra features.
//
// Truth source: All settings read from and write directly to PrefService
// (via the profile), AstraWorkspaceService, and AstraFocusModeService.
// No state is stored in the view itself — the view is purely a projection
// of service and pref values.
//
// Navigation model:
//   - Main page: shows all sections in overview
//   - Section detail: shows one section's settings
//   - Search results: shows settings matching a query
//   - Subpages: special subpages (e.g. search engine settings)
//
// Layout (top to bottom):
//   1. Search box
//   2. Navigation breadcrumbs
//   3. Content area (sections list or settings list)
//   4. Footer with version info
//
// Chromium subsystems reused:
//   - PrefService (settings persistence)
//   - PrefChangeRegistrar (pref change observation)
//   - ProfileKeyedService (workspace, focus mode services)
//   - views::ToggleButton, views::Slider, views::Combobox (controls)
//   - views::BoxLayout (section layout)
//
// Chromium owner / pattern reference:
//   chrome/browser/ui/views/settings/ — Chrome's native settings Views
//   chrome/browser/resources/settings/ — Chrome's WebUI settings
// Since Astra is an overlay and cannot modify Chrome WebUI resources,
// we use a Views-based settings subpage shown from the Astra UI.
//
// TODO(astra): Consider adding a "Settings" entry to Chrome's WebUI
// settings page (chrome://settings) that opens this Astra subpage, or
// integrate Astra settings directly into the Chrome settings sidebar.
// Patch point: chrome/browser/resources/settings/sidebar/sidebar.ts
// or chrome/browser/ui/webui/settings/settings_page_ui_handler.cc.
// =========================================================================

class AstraSettingsPageView : public views::View,
                              public AstraWorkspaceServiceObserver,
                              public AstraFocusModeServiceObserver,
                              public AstraSettingsObserver {
 public:
  explicit AstraSettingsPageView(Browser* browser);
  ~AstraSettingsPageView() override;

  AstraSettingsPageView(const AstraSettingsPageView&) = delete;
  AstraSettingsPageView& operator=(const AstraSettingsPageView&) = delete;

  // -- Model management ---------------------------------------------------

  // Set the settings model to use.  The page view observes the model for
  // state changes.  The model is not owned by the page view.
  void SetModel(AstraSettingsModel* model);
  AstraSettingsModel* GetModel() { return settings_model_; }

  // Legacy alias (kept for backward compatibility).
  void SetSettingsModel(AstraSettingsModel* model) { SetModel(model); }
  AstraSettingsModel* settings_model() { return settings_model_; }

  // -- Navigation ---------------------------------------------------------

  // Show the main page with all sections overview.
  void ShowMainPage();

  // Show a specific section's settings.
  void ShowSection(const std::string& section_id);

  // Show search results for the given query.
  void ShowSearchResults(const std::u16string& query);

  // Navigate back in the navigation stack.
  void NavigateBack();

  // Returns true if we can navigate back (navigation stack > 1 entry).
  bool CanNavigateBack() const;

  // Returns the size of the navigation stack.
  size_t GetNavigationStackSize() const;

  // -- Search box access --------------------------------------------------

  // Returns the search box view.
  AstraSettingsSearchBox* GetSearchBox() { return search_box_; }

  // Set the search query programmatically.
  void SetSearchQuery(const std::u16string& query);

  // Handle search query changes (implements AstraSettingsObserver and
  // direct callback from search box).
  void OnSearchQueryChanged(const std::u16string& query);

  // -- Refresh ------------------------------------------------------------

  // Refresh all controls from the current pref values.
  void RefreshFromPrefs();

  // -- views::View --------------------------------------------------------

  void OnThemeChanged() override;

  // -- AstraWorkspaceServiceObserver --------------------------------------

  void OnWorkspaceAdded(const AstraWorkspace& workspace) override;
  void OnWorkspaceRemoved(const std::string& workspace_id) override;
  void OnWorkspaceRenamed(const std::string& workspace_id,
                          const std::string& new_name) override;
  void OnActiveWorkspaceChanged(const std::string& old_id,
                                const std::string& new_id) override;
  void OnWorkspacesReordered() override;

  // -- AstraFocusModeServiceObserver --------------------------------------

  void OnFocusModeEntered(base::TimeDelta duration) override;
  void OnFocusModeExited() override;
  void OnFocusTimeUpdated(base::TimeDelta remaining) override;
  void OnDistractionBlocklistChanged() override;

  // -- AstraSettingsObserver ----------------------------------------------

  void OnSettingChanged(AstraSettingsModel* model,
                        const std::string& key) override;
  void OnSettingsReset(AstraSettingsModel* model) override;
  void OnSettingsSearchResultsChanged(AstraSettingsModel* model) override;
  void OnSettingsModelShutdown(AstraSettingsModel* model) override;

  // Accessor for testing.
  const std::vector<AstraSettingsNavigationEntry>& navigation_stack() const {
    return navigation_stack_;
  }

  views::View* content_area() { return content_area_; }
  views::View* breadcrumbs_container() { return breadcrumbs_container_; }
  views::View* footer_view() { return footer_view_; }

 private:
  // Build the entire settings page with all sections.
  void BuildContents();

  // Build the search box at the top.
  void BuildSearchBox();

  // Build the breadcrumbs navigation bar.
  void BuildBreadcrumbs();

  // Build the content area (sections or settings list).
  void BuildContentArea();

  // Build the footer with version info.
  void BuildFooter();

  // Initialize service observers and pref change registrar.
  void InitObservers();

  // -- Content building helpers -------------------------------------------

  // Build all section views for the main page.
  void BuildAllSections();

  // Build a single section view with its settings.
  AstraSettingsSectionView* BuildSectionView(
      const std::string& section_id);

  // Rebuild content area based on current navigation state.
  void RebuildContentArea();

  // Update breadcrumbs to reflect current navigation state.
  void UpdateBreadcrumbs();

  // Push a new navigation entry onto the stack.
  void PushNavigationEntry(AstraSettingsNavigationEntry entry);

  // -- General section ----------------------------------------------------

  void BuildGeneralSection();
  void OnStartupBehaviorChanged();
  void OnDefaultWorkspaceChanged();
  void RefreshGeneralSection();

  // -- Sidebar section ----------------------------------------------------

  void BuildSidebarSection();
  void OnSidebarVisibleToggled();
  void OnSidebarPositionChanged();
  void OnSidebarAutoHideToggled();
  void OnSidebarWidthChanged(double value);
  void OnSidebarPinnedToggled();
  void RefreshSidebarSection();

  // -- Workspaces section -------------------------------------------------

  void BuildWorkspacesSection();
  void OnWorkspaceNewTabBehaviorChanged();
  void OnWorkspaceDisplayDensityChanged();
  void OnShowWorkspaceIndicatorToggled();
  void OnAddWorkspace();
  void OnManageWorkspaces();
  void RefreshWorkspacesSection();

  // -- Tab Management section ---------------------------------------------

  void BuildTabManagementSection();
  void OnTabStackingToggled();
  void OnSplitViewOrientationChanged();
  void OnRecentlyClosedCountChanged(double value);
  void OnTabHoverPeekToggled();
  void RefreshTabManagementSection();

  // -- Focus Mode section -------------------------------------------------

  void BuildFocusModeSection();
  void OnFocusDurationChanged(double value);
  void OnFocusAutoStartToggled();
  void OnFocusAutoHideSidebarToggled();
  void OnAddDistractionSite();
  void OnRemoveDistractionSite(const std::string& site);
  void RefreshFocusModeSection();
  void RefreshDistractionBlocklist();

  // -- Appearance section -------------------------------------------------

  void BuildAppearanceSection();
  void OnThemeSettingChanged();
  void OnAccentColorChanged();
  void OnDensityChanged();
  void RefreshAppearanceSection();

  // -- Search section -----------------------------------------------------

  void BuildSearchSection();
  void RefreshSearchSection();

  // -- Advanced section ---------------------------------------------------

  void BuildAdvancedSection();
  void OnExperimentalFeaturesToggled();
  void OnDevToolsPanelToggled();
  void OnOpenChromeSettings();
  void OnOpenChromeAbout();
  void RefreshAdvancedSection();

  // -- Privacy section ----------------------------------------------------

  void BuildPrivacySection();
  void OnTrackerBlockingToggled();
  void OnCookieControlChanged();
  void OnOpenPrivacySettings();
  void RefreshPrivacySection();

  // -- Accessibility section ----------------------------------------------

  void BuildAccessibilitySection();
  void OnHighContrastToggled();
  void OnReducedMotionToggled();
  void OnFontScaleChanged(double value);
  void OnOpenAccessibilitySettings();
  void RefreshAccessibilitySection();

  // -- Extensions section -------------------------------------------------

  void BuildExtensionsSection();
  void OnExtensionShowToolbarToggled();
  void OnExtensionShowInSidebarToggled();
  void OnManageExtensions();
  void RefreshExtensionsSection();

  // -- Performance section ------------------------------------------------

  void BuildPerformanceSection();
  void OnMemorySaverToggled();
  void OnMemorySaverTimeoutChanged();
  void OnHardwareAccelerationToggled();
  void RefreshPerformanceSection();

  // -- Notifications section ----------------------------------------------

  void BuildNotificationsSection();
  void OnNotificationsToggled();
  void OnDoNotDisturbToggled();
  void RefreshNotificationsSection();

  // -- Bulk operations ----------------------------------------------------

  void BuildBulkOperationsSection();
  void OnResetAllToDefaults();
  void OnExportSettings();
  void OnImportSettings();

  // -- Helpers ------------------------------------------------------------

  // Reads all sections and stores them for search filtering.
  void RegisterSection(AstraSettingsSectionView* section);

  // Updates section visibility based on current search query.
  void UpdateFilteredSections();

  // Opens the Chrome settings page at the given subpage.
  void OpenChromeSettingsPage(const std::string& subpage);

  // Accessor helpers.
  PrefService* GetPrefs();
  AstraWorkspaceService* GetWorkspaceService();
  AstraFocusModeService* GetFocusService();

  // Pref change handler — dispatches to section-specific refresh methods.
  void OnPrefChanged(const std::string& pref_name);

  // Handler for the back button in breadcrumbs.
  void OnBackButtonPressed();

  raw_ptr<Browser> browser_;

  // Settings model (not owned, may be null).
  raw_ptr<AstraSettingsModel> settings_model_ = nullptr;

  // Navigation stack — top of stack is current page.
  std::vector<AstraSettingsNavigationEntry> navigation_stack_;

  // Search box at the top (owned by view hierarchy).
  raw_ptr<AstraSettingsSearchBox> search_box_ = nullptr;

  // Breadcrumbs container (owned by view hierarchy).
  raw_ptr<views::View> breadcrumbs_container_ = nullptr;
  raw_ptr<views::MdTextButton> back_button_ = nullptr;
  raw_ptr<views::Label> breadcrumb_label_ = nullptr;

  // Content area scroll view (owned by view hierarchy).
  raw_ptr<views::ScrollView> content_scroll_view_ = nullptr;
  raw_ptr<views::View> content_area_ = nullptr;

  // Footer view (owned by view hierarchy).
  raw_ptr<views::View> footer_view_ = nullptr;
  raw_ptr<views::Label> version_label_ = nullptr;

  // Separator between content and footer.
  raw_ptr<views::Separator> footer_separator_ = nullptr;

  // All registered section views (for search filtering and navigation).
  // These views are owned by the view hierarchy (AddChildView).
  std::vector<AstraSettingsSectionView*> sections_;
  std::map<std::string, AstraSettingsSectionView*> section_map_;

  // Search settings view (owned by view hierarchy).
  raw_ptr<AstraSearchSettingsView> search_settings_view_ = nullptr;

  // General section controls.
  raw_ptr<AstraSettingsSectionView> general_section_ = nullptr;
  raw_ptr<views::Combobox> startup_behavior_combobox_ = nullptr;
  raw_ptr<views::Combobox> default_workspace_combobox_ = nullptr;

  // Sidebar section controls.
  raw_ptr<AstraSettingsSectionView> sidebar_section_ = nullptr;
  raw_ptr<views::ToggleButton> sidebar_visible_toggle_ = nullptr;
  raw_ptr<views::Combobox> sidebar_position_combobox_ = nullptr;
  raw_ptr<views::ToggleButton> sidebar_auto_hide_toggle_ = nullptr;
  raw_ptr<views::Slider> sidebar_width_slider_ = nullptr;
  raw_ptr<views::ToggleButton> sidebar_pinned_toggle_ = nullptr;

  // Workspaces section controls.
  raw_ptr<AstraSettingsSectionView> workspaces_section_ = nullptr;
  raw_ptr<views::Combobox> workspace_new_tab_behavior_combobox_ = nullptr;
  raw_ptr<views::Combobox> workspace_density_combobox_ = nullptr;
  raw_ptr<views::ToggleButton> show_workspace_indicator_toggle_ = nullptr;
  raw_ptr<views::MdTextButton> add_workspace_button_ = nullptr;
  raw_ptr<views::MdTextButton> manage_workspaces_button_ = nullptr;

  // Tab Management section controls.
  raw_ptr<AstraSettingsSectionView> tab_management_section_ = nullptr;
  raw_ptr<views::ToggleButton> tab_stacking_toggle_ = nullptr;
  raw_ptr<views::Combobox> split_view_orientation_combobox_ = nullptr;
  raw_ptr<views::Slider> recently_closed_count_slider_ = nullptr;
  raw_ptr<views::ToggleButton> tab_hover_peek_toggle_ = nullptr;

  // Focus Mode section controls.
  raw_ptr<AstraSettingsSectionView> focus_mode_section_ = nullptr;
  raw_ptr<views::Slider> focus_duration_slider_ = nullptr;
  raw_ptr<views::ToggleButton> focus_auto_start_toggle_ = nullptr;
  raw_ptr<views::ToggleButton> focus_auto_hide_sidebar_toggle_ = nullptr;
  raw_ptr<views::View> blocklist_container_ = nullptr;
  raw_ptr<views::Textfield> blocklist_textfield_ = nullptr;
  raw_ptr<views::MdTextButton> blocklist_add_button_ = nullptr;

  // Appearance section controls.
  raw_ptr<AstraSettingsSectionView> appearance_section_ = nullptr;
  raw_ptr<views::Combobox> theme_combobox_ = nullptr;
  raw_ptr<views::Combobox> accent_color_combobox_ = nullptr;
  raw_ptr<views::Combobox> density_combobox_ = nullptr;

  // Performance section controls.
  raw_ptr<AstraSettingsSectionView> performance_section_ = nullptr;
  raw_ptr<views::ToggleButton> memory_saver_toggle_ = nullptr;
  raw_ptr<views::Slider> memory_saver_timeout_slider_ = nullptr;
  raw_ptr<views::ToggleButton> hardware_acceleration_toggle_ = nullptr;

  // Notifications section controls.
  raw_ptr<AstraSettingsSectionView> notifications_section_ = nullptr;
  raw_ptr<views::ToggleButton> notifications_toggle_ = nullptr;
  raw_ptr<views::ToggleButton> do_not_disturb_toggle_ = nullptr;

  // Advanced section controls.
  raw_ptr<AstraSettingsSectionView> advanced_section_ = nullptr;
  raw_ptr<views::ToggleButton> experimental_features_toggle_ = nullptr;
  raw_ptr<views::ToggleButton> devtools_panel_toggle_ = nullptr;

  // Privacy section controls.
  raw_ptr<AstraSettingsSectionView> privacy_section_ = nullptr;
  raw_ptr<views::ToggleButton> tracker_blocking_toggle_ = nullptr;
  raw_ptr<views::Combobox> cookie_control_combobox_ = nullptr;

  // Accessibility section controls.
  raw_ptr<AstraSettingsSectionView> accessibility_section_ = nullptr;
  raw_ptr<views::ToggleButton> high_contrast_toggle_ = nullptr;
  raw_ptr<views::ToggleButton> reduced_motion_toggle_ = nullptr;
  raw_ptr<views::Slider> font_scale_slider_ = nullptr;

  // Extensions section controls.
  raw_ptr<AstraSettingsSectionView> extensions_section_ = nullptr;
  raw_ptr<views::ToggleButton> extension_show_toolbar_toggle_ = nullptr;
  raw_ptr<views::ToggleButton> extension_show_sidebar_toggle_ = nullptr;

  // Service observation.
  base::ScopedObservation<AstraWorkspaceService,
                          AstraWorkspaceServiceObserver>
      workspace_service_observation_{this};
  base::ScopedObservation<AstraFocusModeService,
                          AstraFocusModeServiceObserver>
      focus_service_observation_{this};

  // Pref change observation.
  std::unique_ptr<PrefChangeRegistrar> pref_change_registrar_;
};

}  // namespace astra

#endif  // ASTRA_UI_VIEWS_SETTINGS_ASTRA_SETTINGS_PAGE_VIEW_H_
