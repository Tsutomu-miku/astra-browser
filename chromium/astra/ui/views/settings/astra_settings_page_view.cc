#include "astra/ui/views/settings/astra_settings_page_view.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/button/toggle_button.h"
#include "ui/views/controls/combobox/combobox.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/separator.h"
#include "ui/views/controls/slider.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"

#include "astra/browser/astra_extension_helper.h"
#include "astra/browser/astra_focus_mode_service.h"
#include "astra/browser/astra_prefs.h"
#include "astra/browser/astra_workspace_service.h"
#include "astra/ui/views/settings/astra_search_settings_view.h"
#include "astra/ui/views/settings/astra_search_settings_view.h"
#include "astra/ui/views/settings/astra_settings_search_box.h"
#include "astra/ui/views/settings/astra_settings_section_view.h"

namespace astra {

namespace {

// Layout constants.
constexpr int kPageHorizontalPadding = 16;
constexpr int kPageTopPadding = 8;
constexpr int kPageBottomPadding = 8;
constexpr int kSectionSpacing = 16;
constexpr int kBreadcrumbsHeight = 32;
constexpr int kFooterHeight = 36;
constexpr int kSearchBoxBottomPadding = 8;
constexpr int kBreadcrumbsBottomPadding = 8;
constexpr int kContentFooterSpacing = 8;
constexpr int kBackButtonSize = 28;

// Sidebar width slider range.
constexpr int kMinSidebarWidth = 200;
constexpr int kMaxSidebarWidth = 500;

// Focus duration slider: 5 to 120 minutes.
constexpr int kMinFocusDuration = 5;
constexpr int kMaxFocusDuration = 120;

// Font scale slider: 0.8 to 1.5.
constexpr double kMinFontScale = 0.8;
constexpr double kMaxFontScale = 1.5;

// Recently closed count slider: 0 to 50 tabs.
constexpr int kMinRecentlyClosed = 0;
constexpr int kMaxRecentlyClosed = 50;

// Memory saver timeout: 1 to 60 minutes.
constexpr int kMinMemorySaverTimeout = 1;
constexpr int kMaxMemorySaverTimeout = 60;

// Orientation combobox items.
constexpr const char* kOrientationHorizontal = "Horizontal";
constexpr const char* kOrientationVertical = "Vertical";

// Sidebar position items.
constexpr const char* kSidebarPositionLeft = "Left";
constexpr const char* kSidebarPositionRight = "Right";

// Startup behavior items.
constexpr const char* kStartupRestore = "Restore last session";
constexpr const char* kStartupNewTab = "Open new tab page";
constexpr const char* kStartupDefaultWorkspace = "Open default workspace";

// New tab behavior items.
constexpr const char* kNewTabCurrentWorkspace = "Current workspace";
constexpr const char* kNewTabNewWorkspace = "New workspace";
constexpr const char* kNewTabNoWorkspace = "No workspace";

// Theme items.
constexpr const char* kThemeSystem = "System default";
constexpr const char* kThemeLight = "Light";
constexpr const char* kThemeDark = "Dark";

// Density items.
constexpr const char* kDensityCompact = "Compact";
constexpr const char* kDensityComfortable = "Comfortable";
constexpr const char* kDensitySpacious = "Spacious";

// Accent color options.
constexpr const char* kAccentBlue = "Blue";
constexpr const char* kAccentPurple = "Purple";
constexpr const char* kAccentGreen = "Green";
constexpr const char* kAccentOrange = "Orange";
constexpr const char* kAccentPink = "Pink";

// Helper: format a slider value as "X px".
std::u16string FormatPixelValue(double normalized_value,
                                int min_val,
                                int max_val) {
  int value = min_val + static_cast<int>(normalized_value * (max_val - min_val));
  return base::NumberToString16(value) + u" px";
}

// Helper: format focus duration as "X min".
std::u16string FormatFocusDuration(double normalized_value) {
  int minutes = kMinFocusDuration +
      static_cast<int>(normalized_value * (kMaxFocusDuration - kMinFocusDuration));
  return base::NumberToString16(minutes) + u" min";
}

// Helper: format font scale as "X%".
std::u16string FormatFontScale(double normalized_value) {
  double scale = kMinFontScale +
      normalized_value * (kMaxFontScale - kMinFontScale);
  return base::NumberToString16(static_cast<int>(scale * 100)) + u"%";
}

// Helper: format recently closed count.
std::u16string FormatRecentlyClosedCount(double normalized_value) {
  int count = kMinRecentlyClosed +
      static_cast<int>(normalized_value * (kMaxRecentlyClosed - kMinRecentlyClosed));
  if (count == 0) {
    return u"Off";
  }
  return base::NumberToString16(count);
}

// Helper: format memory saver timeout as "X min".
std::u16string FormatMemorySaverTimeout(double normalized_value) {
  int minutes = kMinMemorySaverTimeout +
      static_cast<int>(normalized_value *
                       (kMaxMemorySaverTimeout - kMinMemorySaverTimeout));
  return base::NumberToString16(minutes) + u" min";
}

// Create a simple combobox model from a list of strings.
std::unique_ptr<ui::SimpleComboboxModel> CreateStringComboboxModel(
    const std::vector<const char*>& items) {
  auto model = std::make_unique<ui::SimpleComboboxModel>();
  for (const auto* item : items) {
    model->AddItem(base::UTF8ToUTF16(item));
  }
  return model;
}

}  // namespace

// =========================================================================
// Construction / destruction
// =========================================================================

AstraSettingsPageView::AstraSettingsPageView(Browser* browser)
    : browser_(browser) {
  DCHECK(browser_);

  // Initialize navigation stack with the main page.
  navigation_stack_.push_back({
      AstraSettingsPageType::kMainPage,
      std::string(),      // section_id
      std::u16string(),   // query
      std::string(),      // subpage_id
      u"Settings"         // title
  });

  BuildContents();
  InitObservers();
  RefreshFromPrefs();
}

AstraSettingsPageView::~AstraSettingsPageView() {
  // Remove ourselves as a model observer if we're observing one.
  if (settings_model_) {
    settings_model_->RemoveObserver(this);
  }
}

// =========================================================================
// Model management
// =========================================================================

void AstraSettingsPageView::SetModel(AstraSettingsModel* model) {
  // Remove old observer.
  if (settings_model_) {
    settings_model_->RemoveObserver(this);
  }

  settings_model_ = model;

  // Add new observer.
  if (settings_model_) {
    settings_model_->AddObserver(this);
  }

  // Update search settings view model.
  if (search_settings_view_) {
    search_settings_view_->SetModel(model);
  }
}

// =========================================================================
// Navigation
// =========================================================================

void AstraSettingsPageView::ShowMainPage() {
  // Clear stack and push main page.
  navigation_stack_.clear();
  navigation_stack_.push_back({
      AstraSettingsPageType::kMainPage,
      std::string(),
      std::u16string(),
      std::string(),
      u"Settings"
  });
  RebuildContentArea();
  UpdateBreadcrumbs();
}

void AstraSettingsPageView::ShowSection(const std::string& section_id) {
  if (section_id.empty()) {
    return;
  }

  const auto* section_info = settings_model_
      ? settings_model_->GetSection(section_id)
      : nullptr;

  std::u16string title = section_info
      ? section_info->name
      : base::UTF8ToUTF16(section_id);

  PushNavigationEntry({
      AstraSettingsPageType::kSection,
      section_id,
      std::u16string(),
      std::string(),
      title
  });
  RebuildContentArea();
  UpdateBreadcrumbs();
}

void AstraSettingsPageView::ShowSearchResults(const std::u16string& query) {
  if (query.empty()) {
    // Empty query goes to main page.
    ShowMainPage();
    return;
  }

  PushNavigationEntry({
      AstraSettingsPageType::kSearchResults,
      std::string(),
      query,
      std::string(),
      u"Search results"
  });
  RebuildContentArea();
  UpdateBreadcrumbs();
}

void AstraSettingsPageView::NavigateBack() {
  if (navigation_stack_.size() <= 1) {
    return;
  }

  navigation_stack_.pop_back();
  RebuildContentArea();
  UpdateBreadcrumbs();
}

bool AstraSettingsPageView::CanNavigateBack() const {
  return navigation_stack_.size() > 1;
}

size_t AstraSettingsPageView::GetNavigationStackSize() const {
  return navigation_stack_.size();
}

// =========================================================================
// Search box access
// =========================================================================

void AstraSettingsPageView::SetSearchQuery(const std::u16string& query) {
  if (search_box_) {
    search_box_->SetQuery(query);
  }
}

void AstraSettingsPageView::OnSearchQueryChanged(const std::u16string& query) {
  if (settings_model_) {
    settings_model_->SetSearchQuery(query);
    return;
  }

  // Fallback: directly filter sections if no model.
  // For empty query, go to main page.
  if (query.empty()) {
    if (navigation_stack_.size() > 1 &&
        navigation_stack_.back().type ==
            AstraSettingsPageType::kSearchResults) {
      NavigateBack();
    }
    UpdateFilteredSections();
    return;
  }

  // Show search results.
  ShowSearchResults(query);
}

// =========================================================================
// Contents building
// =========================================================================

void AstraSettingsPageView::BuildContents() {
  // Vertical box layout for the whole page.
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::TLBR(kPageTopPadding, kPageHorizontalPadding,
                        kPageBottomPadding, kPageHorizontalPadding),
      0));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  BuildSearchBox();
  BuildBreadcrumbs();
  BuildContentArea();
  BuildFooter();
}

void AstraSettingsPageView::BuildSearchBox() {
  auto search_box = std::make_unique<AstraSettingsSearchBox>(
      base::BindRepeating(&AstraSettingsPageView::OnSearchQueryChanged,
                          base::Unretained(this)));
  search_box_ = search_box.get();
  AddChildView(std::move(search_box));

  // Add spacing after search box.
  auto spacer = std::make_unique<views::View>();
  spacer->SetPreferredSize(gfx::Size(0, kSearchBoxBottomPadding));
  AddChildView(std::move(spacer));
}

void AstraSettingsPageView::BuildBreadcrumbs() {
  auto container = std::make_unique<views::View>();
  auto* layout = container->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets(), 8));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kStart);
  container->SetPreferredSize(gfx::Size(0, kBreadcrumbsHeight));

  // Back button (initially hidden).
  auto back_button = views::MdTextButton::Create(
      base::BindRepeating(&AstraSettingsPageView::OnBackButtonPressed,
                          base::Unretained(this)),
      u"\u2039");  // ‹ (single left-pointing angle quotation mark)
  back_button->SetPreferredSize(
      gfx::Size(kBackButtonSize, kBackButtonSize));
  back_button->SetVisible(false);
  back_button_ = back_button.get();
  container->AddChildView(std::move(back_button));

  // Breadcrumb label.
  auto label = std::make_unique<views::Label>(u"Settings");
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SetAutoColorReadabilityEnabled(false);
  label->SetEnabledColorId(ui::kColorLabelForegroundPrimary);
  label->SetFontList(
      label->font_list().DeriveWithWeight(gfx::Font::Weight::MEDIUM));
  breadcrumb_label_ = label.get();
  layout->SetFlexForView(label.get(), 1);
  container->AddChildView(std::move(label));

  breadcrumbs_container_ = container.get();
  AddChildView(std::move(container));

  // Add spacing after breadcrumbs.
  auto spacer = std::make_unique<views::View>();
  spacer->SetPreferredSize(gfx::Size(0, kBreadcrumbsBottomPadding));
  AddChildView(std::move(spacer));
}

void AstraSettingsPageView::BuildContentArea() {
  auto scroll_view = std::make_unique<views::ScrollView>();
  scroll_view->SetClipHeight(true);
  scroll_view->SetDrawOverflowIndicator(true);
  scroll_view->SetHorizontalScrollBarMode(
      views::ScrollView::ScrollBarMode::kDisabled);
  content_scroll_view_ = scroll_view.get();

  // Content container inside the scroll view.
  auto content = std::make_unique<views::View>();
  content->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(),
      kSectionSpacing));
  content->layout()->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);
  content_area_ = content.get();

  // Build all sections (they'll be shown/hidden based on navigation state).
  BuildAllSections();

  scroll_view->SetContents(std::move(content));

  // The scroll view expands to fill remaining space.
  auto* layout = static_cast<views::BoxLayout*>(GetLayoutManager());
  if (layout) {
    layout->SetFlexForView(scroll_view.get(), 1);
  }

  AddChildView(std::move(scroll_view));
}

void AstraSettingsPageView::BuildFooter() {
  // Spacer between content and footer.
  auto spacer = std::make_unique<views::View>();
  spacer->SetPreferredSize(gfx::Size(0, kContentFooterSpacing));
  AddChildView(std::move(spacer));

  // Separator line.
  auto separator = std::make_unique<views::Separator>();
  separator->SetColorId(ui::kColorSeparator);
  footer_separator_ = separator.get();
  AddChildView(std::move(separator));

  // Footer container.
  auto footer = std::make_unique<views::View>();
  footer->SetPreferredSize(gfx::Size(0, kFooterHeight));
  auto* layout = footer->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets::VH(8, 0), 0));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kCenter);

  auto version_label = std::make_unique<views::Label>(u"Astra Browser 1.0");
  version_label->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  version_label->SetAutoColorReadabilityEnabled(false);
  version_label->SetEnabledColorId(ui::kColorLabelForegroundSecondary);
  version_label->SetFontList(
      version_label->font_list().DeriveWithSizeDelta(-1));
  version_label_ = version_label.get();
  footer->AddChildView(std::move(version_label));

  footer_view_ = footer.get();
  AddChildView(std::move(footer));
}

void AstraSettingsPageView::InitObservers() {
  // Observe workspace service.
  AstraWorkspaceService* workspace_service = GetWorkspaceService();
  if (workspace_service) {
    workspace_service_observation_.Observe(workspace_service);
  }

  // Observe focus mode service.
  AstraFocusModeService* focus_service = GetFocusService();
  if (focus_service) {
    focus_service_observation_.Observe(focus_service);
  }

  // Observe pref changes.
  PrefService* prefs = GetPrefs();
  if (prefs) {
    pref_change_registrar_ = std::make_unique<PrefChangeRegistrar>();
    pref_change_registrar_->Init(prefs);

    // Register all relevant pref changes to trigger refresh.
    const char* pref_names[] = {
        prefs::kPrefSidebarVisible,
        prefs::kPrefSidebarWidth,
        prefs::kPrefSidebarPinned,
        prefs::kPrefSplitViewDefaultOrientation,
        prefs::kPrefFocusModeDefaultDuration,
        prefs::kPrefFocusModeAutoStart,
        prefs::kPrefFocusModeBlocklist,
        prefs::kPrefMemorySaverEnabled,
        prefs::kPrefMemorySaverTimeoutMinutes,
        prefs::kPrefMemorySaverSuspendActiveWorkspace,
        prefs::kPrefPiPDefaultSize,
        prefs::kPrefPiPAlwaysOnTop,
        prefs::kPrefSearchShowDefaultEngine,
        prefs::kPrefSearchShowOtherEngines,
        prefs::kPrefHighContrastMode,
        prefs::kPrefReducedMotion,
        prefs::kPrefAccessibilityFontScale,
    };

    for (const auto* pref : pref_names) {
      pref_change_registrar_->Add(
          pref, base::BindRepeating(&AstraSettingsPageView::OnPrefChanged,
                                    base::Unretained(this)));
    }
  }
}

// =========================================================================
// Content building helpers
// =========================================================================

void AstraSettingsPageView::BuildAllSections() {
  // Build all sections but they may not all be visible depending on
  // navigation state.
  BuildAppearanceSection();
  BuildWorkspacesSection();
  BuildSidebarSection();
  BuildTabManagementSection();
  BuildPrivacySection();
  BuildSearchSection();
  BuildAccessibilitySection();
  BuildPerformanceSection();
  BuildNotificationsSection();
  BuildAdvancedSection();
  BuildExtensionsSection();
  BuildGeneralSection();
  BuildFocusModeSection();
  BuildBulkOperationsSection();

  // Initial state: show all sections (main page view).
  for (auto* section : sections_) {
    section->SetVisible(true);
  }
}

AstraSettingsSectionView* AstraSettingsPageView::BuildSectionView(
    const std::string& section_id) {
  // This is a helper for programmatic section building.
  // Most sections are built by the dedicated Build*Section methods.
  auto section = std::make_unique<AstraSettingsSectionView>(
      base::UTF8ToUTF16(section_id));
  AstraSettingsSectionView* section_ptr = section.get();
  RegisterSection(section_ptr);
  content_area_->AddChildView(std::move(section));
  return section_ptr;
}

void AstraSettingsPageView::RebuildContentArea() {
  if (navigation_stack_.empty() || !content_area_) {
    return;
  }

  const auto& current = navigation_stack_.back();

  switch (current.type) {
    case AstraSettingsPageType::kMainPage:
      // Show all sections.
      for (auto* section : sections_) {
        section->SetVisible(true);
        section->SetExpanded(false);  // Collapsed in overview
      }
      if (search_settings_view_) {
        search_settings_view_->SetVisible(true);
      }
      break;

    case AstraSettingsPageType::kSection: {
      // Show only the selected section, expanded.
      auto it = section_map_.find(current.section_id);
      for (auto* section : sections_) {
        section->SetVisible(section->GetSectionId() == current.section_id);
        if (section->GetSectionId() == current.section_id) {
          section->SetExpanded(true);
        }
      }
      if (search_settings_view_) {
        search_settings_view_->SetVisible(current.section_id == "search");
      }
      break;
    }

    case AstraSettingsPageType::kSearchResults: {
      // Filter sections by search query.
      const auto& query = current.query;
      for (auto* section : sections_) {
        section->SetVisible(section->MatchesSearch(query));
        section->SetExpanded(true);  // Expand matching sections
      }
      if (search_settings_view_) {
        search_settings_view_->SetVisible(
            search_settings_view_->MatchesSearch(query));
      }
      break;
    }

    case AstraSettingsPageType::kSubpage:
      // Subpages are handled by their specific views.
      break;
  }

  content_area_->InvalidateLayout();
}

void AstraSettingsPageView::UpdateBreadcrumbs() {
  if (navigation_stack_.empty()) {
    return;
  }

  const auto& current = navigation_stack_.back();
  bool can_go_back = CanNavigateBack();

  if (back_button_) {
    back_button_->SetVisible(can_go_back);
  }

  if (breadcrumb_label_) {
    breadcrumb_label_->SetText(current.title);
  }
}

void AstraSettingsPageView::PushNavigationEntry(
    AstraSettingsNavigationEntry entry) {
  navigation_stack_.push_back(std::move(entry));
}

void AstraSettingsPageView::OnBackButtonPressed() {
  NavigateBack();
}

// =========================================================================
// Search filtering
// =========================================================================

void AstraSettingsPageView::RegisterSection(
    AstraSettingsSectionView* section) {
  sections_.push_back(section);
  if (!section->GetSectionId().empty()) {
    section_map_[section->GetSectionId()] = section;
  }
}

void AstraSettingsPageView::UpdateFilteredSections() {
  if (!search_box_) {
    return;
  }
  std::u16string query = search_box_->GetQuery();

  if (query.empty()) {
    // Show all sections.
    for (auto* section : sections_) {
      section->SetVisible(true);
    }
    if (search_settings_view_) {
      search_settings_view_->SetVisible(true);
    }
    InvalidateLayout();
    return;
  }

  for (auto* section : sections_) {
    section->SetVisible(section->MatchesSearch(query));
  }

  if (search_settings_view_) {
    search_settings_view_->SetVisible(
        search_settings_view_->MatchesSearch(query));
  }

  InvalidateLayout();
}


// =========================================================================
// General section
// =========================================================================

void AstraSettingsPageView::BuildGeneralSection() {
  auto section = std::make_unique<AstraSettingsSectionView>(u"General");
  section->SetSection("general", u"General",
                      u"Startup and default behavior settings");
  section->SetIconName("general");
  section->AddSearchKeywords(
      {u"general", u"startup", u"default", u"on launch", u"workspace",
       u"home", u"new tab", u"behavior", u"start page"});

  // Startup behavior combobox.
  auto startup_model = CreateStringComboboxModel(
      {"Open new tab", "Continue where you left off", "Open specific pages"});
  startup_behavior_combobox_ = section->AddComboboxRow(
      u"On startup", std::move(startup_model),
      base::BindRepeating(
          &AstraSettingsPageView::OnStartupBehaviorChanged,
          base::Unretained(this)));

  // Default workspace combobox.
  auto ws_model = CreateStringComboboxModel(
      {"Last used", "Personal", "Work", "School"});
  default_workspace_combobox_ = section->AddComboboxRow(
      u"Default workspace", std::move(ws_model),
      base::BindRepeating(
          &AstraSettingsPageView::OnDefaultWorkspaceChanged,
          base::Unretained(this)));

  section->SetSettingCount(2);

  general_section_ = section.get();
  RegisterSection(general_section_);
  content_area_->AddChildView(std::move(section));
}

void AstraSettingsPageView::OnStartupBehaviorChanged() {
  PrefService* prefs = GetPrefs();
  if (!prefs || !startup_behavior_combobox_) {
    return;
  }
  // TODO(astra): Wire startup behavior to Chrome's session restore prefs.
  //
  // Chromium owner: StartupBrowserCreator / SessionRestore
  //   (chrome/browser/sessions/)
  // Patch point: chrome/browser/prefs/session_startup_pref.cc
}

void AstraSettingsPageView::OnDefaultWorkspaceChanged() {
  PrefService* prefs = GetPrefs();
  if (!prefs || !default_workspace_combobox_) {
    return;
  }
  // TODO(astra): Persist default workspace setting.
  //
  // Chromium owner: AstraWorkspaceService (Astra-specific)
}

void AstraSettingsPageView::RefreshGeneralSection() {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  // TODO(astra): Read actual prefs for general settings.
  // For now, these are presentation placeholders.
  if (startup_behavior_combobox_) {
    startup_behavior_combobox_->SetSelectedIndex(0);
  }
  if (default_workspace_combobox_) {
    default_workspace_combobox_->SetSelectedIndex(0);
  }
}

// =========================================================================
// Sidebar section
// =========================================================================

void AstraSettingsPageView::BuildSidebarSection() {
  auto section = std::make_unique<AstraSettingsSectionView>(u"Sidebar");
  section->SetSection("sidebar", u"Sidebar",
                      u"Sidebar visibility, position, and behavior settings");
  section->SetIconName("sidebar");
  section->AddSearchKeywords(
      {u"sidebar", u"side panel", u"position", u"left", u"right",
       u"auto-hide", u"pin", u"width", u"size", u"visible",
       u"toggle", u"bookmarks", u"history"});

  // Visibility toggle.
  sidebar_visible_toggle_ = section->AddToggleRow(
      u"Show sidebar", true,
      base::BindRepeating(
          &AstraSettingsPageView::OnSidebarVisibleToggled,
          base::Unretained(this)));

  // Position combobox.
  auto pos_model = CreateStringComboboxModel({"Left", "Right"});
  sidebar_position_combobox_ = section->AddComboboxRow(
      u"Sidebar position", std::move(pos_model),
      base::BindRepeating(
          &AstraSettingsPageView::OnSidebarPositionChanged,
          base::Unretained(this)));

  // Auto-hide toggle.
  sidebar_auto_hide_toggle_ = section->AddToggleRow(
      u"Auto-hide sidebar", false,
      base::BindRepeating(
          &AstraSettingsPageView::OnSidebarAutoHideToggled,
          base::Unretained(this)));

  // Width slider.
  sidebar_width_slider_ = section->AddSliderRow(
      u"Sidebar width", 0.3,
      base::BindRepeating(&FormatPixelValue, kMinSidebarWidth,
                          kMaxSidebarWidth),
      base::BindRepeating(&AstraSettingsPageView::OnSidebarWidthChanged,
                          base::Unretained(this)));

  // Pinned toggle.
  sidebar_pinned_toggle_ = section->AddToggleRow(
      u"Pin sidebar", true,
      base::BindRepeating(
          &AstraSettingsPageView::OnSidebarPinnedToggled,
          base::Unretained(this)));

  section->SetSettingCount(5);

  sidebar_section_ = section.get();
  RegisterSection(sidebar_section_);
  content_area_->AddChildView(std::move(section));
}

void AstraSettingsPageView::OnSidebarVisibleToggled() {
  PrefService* prefs = GetPrefs();
  if (!prefs || !sidebar_visible_toggle_) {
    return;
  }
  bool visible = sidebar_visible_toggle_->GetIsOn();
  // TODO(astra): Persist sidebar visibility pref.
  //   prefs->SetBoolean(astra::prefs::kPrefSidebarVisible, visible);
}

void AstraSettingsPageView::OnSidebarPositionChanged() {
  PrefService* prefs = GetPrefs();
  if (!prefs || !sidebar_position_combobox_) {
    return;
  }
  // TODO(astra): Persist sidebar position pref.
  //
  // Chromium owner: SidePanelUI (chrome/browser/ui/side_panel/)
}

void AstraSettingsPageView::OnSidebarAutoHideToggled() {
  PrefService* prefs = GetPrefs();
  if (!prefs || !sidebar_auto_hide_toggle_) {
    return;
  }
  // TODO(astra): Persist sidebar auto-hide pref.
}

void AstraSettingsPageView::OnSidebarWidthChanged(double value) {
  PrefService* prefs = GetPrefs();
  if (!prefs || !sidebar_width_slider_) {
    return;
  }
  int width = kMinSidebarWidth +
      static_cast<int>(value * (kMaxSidebarWidth - kMinSidebarWidth));
  // TODO(astra): Persist sidebar width pref.
  //   prefs->SetInteger(astra::prefs::kPrefSidebarWidth, width);
}

void AstraSettingsPageView::OnSidebarPinnedToggled() {
  PrefService* prefs = GetPrefs();
  if (!prefs || !sidebar_pinned_toggle_) {
    return;
  }
  // TODO(astra): Persist sidebar pinned pref.
}

void AstraSettingsPageView::RefreshSidebarSection() {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  // TODO(astra): Read actual prefs for sidebar settings.
  // For now, these are presentation placeholders.
  if (sidebar_visible_toggle_) {
    sidebar_visible_toggle_->SetIsOn(true);
  }
  if (sidebar_position_combobox_) {
    sidebar_position_combobox_->SetSelectedIndex(0);
  }
  if (sidebar_auto_hide_toggle_) {
    sidebar_auto_hide_toggle_->SetIsOn(false);
  }
  if (sidebar_width_slider_) {
    sidebar_width_slider_->SetValue(0.3);  // ~290px default
  }
  if (sidebar_pinned_toggle_) {
    sidebar_pinned_toggle_->SetIsOn(true);
  }
}

// =========================================================================
// Workspaces section
// =========================================================================

void AstraSettingsPageView::BuildWorkspacesSection() {
  auto section =
      std::make_unique<AstraSettingsSectionView>(u"Workspaces");
  section->SetSection("workspaces", u"Workspaces",
                      u"Workspace management and display settings");
  section->SetIconName("workspaces");
  section->AddSearchKeywords(
      {u"workspaces", u"spaces", u"profiles", u"groups",
       u"organize", u"tabs", u"new tab", u"indicator",
       u"density", u"compact", u"comfortable", u"spacious"});

  // New tab behavior combobox.
  auto new_tab_model = CreateStringComboboxModel(
      {"Open in current workspace", "Open in new workspace", "Ask every time"});
  workspace_new_tab_behavior_combobox_ = section->AddComboboxRow(
      u"New tab behavior", std::move(new_tab_model),
      base::BindRepeating(
          &AstraSettingsPageView::OnWorkspaceNewTabBehaviorChanged,
          base::Unretained(this)));

  // Display density combobox.
  auto density_model = CreateStringComboboxModel(
      {kDensityCompact, kDensityComfortable, kDensitySpacious});
  workspace_density_combobox_ = section->AddComboboxRow(
      u"Workspace display density", std::move(density_model),
      base::BindRepeating(
          &AstraSettingsPageView::OnWorkspaceDisplayDensityChanged,
          base::Unretained(this)));

  // Workspace indicator toggle.
  show_workspace_indicator_toggle_ = section->AddToggleRow(
      u"Show workspace indicator", true,
      base::BindRepeating(
          &AstraSettingsPageView::OnShowWorkspaceIndicatorToggled,
          base::Unretained(this)));

  section->AddDivider();

  // Add workspace button.
  add_workspace_button_ = section->AddButtonRow(
      u"Add workspace", u"Add",
      base::BindRepeating(&AstraSettingsPageView::OnAddWorkspace,
                          base::Unretained(this)));

  // Manage workspaces button.
  manage_workspaces_button_ = section->AddButtonRow(
      u"Manage workspaces", u"Manage",
      base::BindRepeating(&AstraSettingsPageView::OnManageWorkspaces,
                          base::Unretained(this)));

  section->SetSettingCount(3);

  workspaces_section_ = section.get();
  RegisterSection(workspaces_section_);
  content_area_->AddChildView(std::move(section));
}

void AstraSettingsPageView::OnWorkspaceNewTabBehaviorChanged() {
  PrefService* prefs = GetPrefs();
  if (!prefs || !workspace_new_tab_behavior_combobox_) {
    return;
  }
  // TODO(astra): Persist new tab behavior pref.
}

void AstraSettingsPageView::OnWorkspaceDisplayDensityChanged() {
  PrefService* prefs = GetPrefs();
  if (!prefs || !workspace_density_combobox_) {
    return;
  }
  // TODO(astra): Persist workspace display density pref.
}

void AstraSettingsPageView::OnShowWorkspaceIndicatorToggled() {
  PrefService* prefs = GetPrefs();
  if (!prefs || !show_workspace_indicator_toggle_) {
    return;
  }
  // TODO(astra): Persist workspace indicator pref.
}

void AstraSettingsPageView::OnAddWorkspace() {
  // TODO(astra): Open a dialog to create a new workspace.
  //
  // This would call AstraWorkspaceService::CreateWorkspace() and show
  // a text input dialog for the workspace name.
}

void AstraSettingsPageView::OnManageWorkspaces() {
  // TODO(astra): Navigate to a workspaces management subpage.
  //
  // For now, we could open a dedicated workspaces bubble or dialog.
}

void AstraSettingsPageView::RefreshWorkspacesSection() {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  // TODO(astra): Read actual prefs for workspace settings.
  // For now, these are presentation placeholders.
  if (workspace_new_tab_behavior_combobox_) {
    workspace_new_tab_behavior_combobox_->SetSelectedIndex(0);
  }
  if (workspace_density_combobox_) {
    workspace_density_combobox_->SetSelectedIndex(1);  // Comfortable
  }
  if (show_workspace_indicator_toggle_) {
    show_workspace_indicator_toggle_->SetIsOn(true);
  }
}

// =========================================================================
// Tab Management section
// =========================================================================

void AstraSettingsPageView::BuildTabManagementSection() {
  auto section =
      std::make_unique<AstraSettingsSectionView>(u"Tab Management");
  section->SetSection("tabs", u"Tab Management",
                      u"Tab stacking, split view, and hover settings");
  section->SetIconName("tabs");
  section->AddSearchKeywords(
      {u"tabs", u"tab management", u"stacking", u"groups", u"split view",
       u"split", u"multitasking", u"recently closed", u"hover",
       u"peek", u"preview", u"tab stack", u"tab groups"});

  // Tab stacking toggle.
  tab_stacking_toggle_ = section->AddToggleRow(
      u"Tab stacking", true,
      base::BindRepeating(&AstraSettingsPageView::OnTabStackingToggled,
                          base::Unretained(this)));

  // Split view orientation combobox.
  auto orientation_model = CreateStringComboboxModel(
      {kOrientationHorizontal, kOrientationVertical});
  split_view_orientation_combobox_ = section->AddComboboxRow(
      u"Default split view orientation", std::move(orientation_model),
      base::BindRepeating(
          &AstraSettingsPageView::OnSplitViewOrientationChanged,
          base::Unretained(this)));

  // Recently closed count slider.
  recently_closed_count_slider_ = section->AddSliderRow(
      u"Recently closed count", 0.2,
      base::BindRepeating(&FormatRecentlyClosedCount),
      base::BindRepeating(
          &AstraSettingsPageView::OnRecentlyClosedCountChanged,
          base::Unretained(this)));

  // Tab hover peek toggle.
  tab_hover_peek_toggle_ = section->AddToggleRow(
      u"Tab hover peek", true,
      base::BindRepeating(&AstraSettingsPageView::OnTabHoverPeekToggled,
                          base::Unretained(this)));

  section->SetSettingCount(4);

  tab_management_section_ = section.get();
  RegisterSection(tab_management_section_);
  content_area_->AddChildView(std::move(section));
}

void AstraSettingsPageView::OnTabStackingToggled() {
  PrefService* prefs = GetPrefs();
  if (!prefs || !tab_stacking_toggle_) {
    return;
  }
  // TODO(astra): Persist tab stacking pref.
  //
  // Chromium owner: TabStripModel / TabGroupController
  //   (chrome/browser/ui/tabs/)
}

void AstraSettingsPageView::OnSplitViewOrientationChanged() {
  PrefService* prefs = GetPrefs();
  if (!prefs || !split_view_orientation_combobox_) {
    return;
  }
  // TODO(astra): Persist split view default orientation pref.
  //
  // This is an Astra-specific feature.
}

void AstraSettingsPageView::OnRecentlyClosedCountChanged(double value) {
  PrefService* prefs = GetPrefs();
  if (!prefs || !recently_closed_count_slider_) {
    return;
  }
  int count = kMinRecentlyClosed + static_cast<int>(
      value * (kMaxRecentlyClosed - kMinRecentlyClosed));
  // TODO(astra): Persist recently closed count pref.
}

void AstraSettingsPageView::OnTabHoverPeekToggled() {
  PrefService* prefs = GetPrefs();
  if (!prefs || !tab_hover_peek_toggle_) {
    return;
  }
  // TODO(astra): Persist tab hover peek pref.
  //
  // Chromium owner: TabHoverCardController
  //   (chrome/browser/ui/views/tabs/tab_hover_card/)
}

void AstraSettingsPageView::RefreshTabManagementSection() {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  // TODO(astra): Read actual prefs for tab management settings.
  // For now, these are presentation placeholders.
  if (tab_stacking_toggle_) {
    tab_stacking_toggle_->SetIsOn(true);
  }
  if (split_view_orientation_combobox_) {
    split_view_orientation_combobox_->SetSelectedIndex(0);  // Horizontal
  }
  if (recently_closed_count_slider_) {
    recently_closed_count_slider_->SetValue(0.2);  // ~10 tabs
  }
  if (tab_hover_peek_toggle_) {
    tab_hover_peek_toggle_->SetIsOn(true);
  }
}

// =========================================================================
// Refresh all sections
// =========================================================================

void AstraSettingsPageView::RefreshFromPrefs() {
  RefreshGeneralSection();
  RefreshSidebarSection();
  RefreshWorkspacesSection();
  RefreshTabManagementSection();
  RefreshFocusModeSection();
  RefreshAppearanceSection();
  RefreshSearchSection();
  RefreshPrivacySection();
  RefreshAccessibilitySection();
  RefreshExtensionsSection();
  RefreshPerformanceSection();
  RefreshNotificationsSection();
  RefreshAdvancedSection();
}

// =========================================================================
// Focus Mode section
// =========================================================================

void AstraSettingsPageView::BuildFocusModeSection() {
  auto section = std::make_unique<AstraSettingsSectionView>(u"Focus Mode");
  section->SetSection("focus_mode", u"Focus Mode",
                       u"Distraction-free browsing sessions");
  section->SetIconName("focus");
  section->AddSearchKeywords(
      {u"focus", u"pomodoro", u"timer", u"distraction", u"block",
       u"productivity", u"concentration", u"do not disturb", u"dnd"});

  // Default duration slider.
  focus_duration_slider_ = section->AddSliderRow(
      u"Default focus duration", 0.2,
      base::BindRepeating(&FormatFocusDuration),
      base::BindRepeating(&AstraSettingsPageView::OnFocusDurationChanged,
                          base::Unretained(this)));

  // Auto-start toggle.
  focus_auto_start_toggle_ = section->AddToggleRow(
      u"Auto-start focus mode", false,
      base::BindRepeating(&AstraSettingsPageView::OnFocusAutoStartToggled,
                          base::Unretained(this)));

  // Auto-hide sidebar in focus mode.
  focus_auto_hide_sidebar_toggle_ = section->AddToggleRow(
      u"Auto-hide sidebar during focus", true,
      base::BindRepeating(
          &AstraSettingsPageView::OnFocusAutoHideSidebarToggled,
          base::Unretained(this)));

  section->AddDivider();

  // Distraction blocklist.
  auto blocklist_row = std::make_unique<views::View>();
  auto* row_layout = blocklist_row->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical, gfx::Insets(), 6));
  row_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  auto blocklist_label = std::make_unique<views::Label>(u"Distraction sites");
  blocklist_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  blocklist_label->SetAutoColorReadabilityEnabled(false);
  blocklist_label->SetEnabledColorId(ui::kColorLabelForegroundPrimary);
  blocklist_row->AddChildView(std::move(blocklist_label));

  // Input row: textfield + add button.
  auto input_row = std::make_unique<views::View>();
  auto* input_layout = input_row->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal, gfx::Insets(), 8));
  input_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  auto textfield = std::make_unique<views::Textfield>();
  textfield->SetPlaceholderText(u"e.g. youtube.com");
  textfield->SetBackgroundColor(SK_ColorTRANSPARENT);
  blocklist_textfield_ = textfield.get();
  input_layout->SetFlexForView(textfield.get(), 1);
  input_row->AddChildView(std::move(textfield));

  auto add_button = views::MdTextButton::Create(
      base::BindRepeating(&AstraSettingsPageView::OnAddDistractionSite,
                          base::Unretained(this)),
      u"Add");
  blocklist_add_button_ = add_button.get();
  input_row->AddChildView(std::move(add_button));

  blocklist_row->AddChildView(std::move(input_row));

  // Blocklist items container.
  auto blocklist_container = std::make_unique<views::View>();
  blocklist_container->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets::VH(4, 0), 4));
  blocklist_container_ = blocklist_container.get();
  blocklist_row->AddChildView(std::move(blocklist_container));

  section->rows_container()->AddChildView(std::move(blocklist_row));

  focus_mode_section_ = section.get();
  RegisterSection(focus_mode_section_);
  content_area_->AddChildView(std::move(section));

  // Initial blocklist refresh.
  RefreshDistractionBlocklist();
}

void AstraSettingsPageView::OnFocusDurationChanged(double value) {
  AstraFocusModeService* service = GetFocusService();
  if (!service) {
    return;
  }
  int minutes = kMinFocusDuration +
      static_cast<int>(value * (kMaxFocusDuration - kMinFocusDuration));
  service->set_default_focus_duration_minutes(minutes);

  if (settings_model_) {
    settings_model_->SetSettingValue("focus_mode_default_duration",
                                      base::Value(minutes));
  }
}

void AstraSettingsPageView::OnFocusAutoStartToggled() {
  AstraFocusModeService* service = GetFocusService();
  if (!service || !focus_auto_start_toggle_) {
    return;
  }
  service->set_auto_start_enabled(focus_auto_start_toggle_->GetIsOn());
}

void AstraSettingsPageView::OnFocusAutoHideSidebarToggled() {
  // TODO(astra): Implement focus mode auto-hide sidebar pref.
  // Controls whether the sidebar is automatically hidden when focus mode
  // is activated.
  //
  // TODO(astra): Add kPrefFocusModeAutoHideSidebar to astra_prefs.h.
}

void AstraSettingsPageView::OnAddDistractionSite() {
  AstraFocusModeService* service = GetFocusService();
  if (!service || !blocklist_textfield_) {
    return;
  }
  std::u16string text = blocklist_textfield_->GetText();
  if (text.empty()) {
    return;
  }
  std::string site = base::UTF16ToUTF8(text);
  service->AddDistractionSite(site);
  blocklist_textfield_->SetText(std::u16string());
  // Refresh handled by OnDistractionBlocklistChanged observer.
}

void AstraSettingsPageView::OnRemoveDistractionSite(const std::string& site) {
  AstraFocusModeService* service = GetFocusService();
  if (!service) {
    return;
  }
  service->RemoveDistractionSite(site);
  // Refresh handled by OnDistractionBlocklistChanged observer.
}

void AstraSettingsPageView::RefreshFocusModeSection() {
  AstraFocusModeService* service = GetFocusService();
  if (!service) {
    return;
  }

  // Default duration.
  if (focus_duration_slider_) {
    int minutes = service->default_focus_duration_minutes();
    minutes = std::clamp(minutes, kMinFocusDuration, kMaxFocusDuration);
    double value = static_cast<double>(minutes - kMinFocusDuration) /
                   (kMaxFocusDuration - kMinFocusDuration);
    focus_duration_slider_->SetValue(value);
  }

  // Auto-start.
  if (focus_auto_start_toggle_) {
    focus_auto_start_toggle_->SetIsOn(service->auto_start_enabled());
  }

  // Blocklist.
  RefreshDistractionBlocklist();
}

void AstraSettingsPageView::RefreshDistractionBlocklist() {
  if (!blocklist_container_) {
    return;
  }

  blocklist_container_->RemoveAllChildViews();

  AstraFocusModeService* service = GetFocusService();
  if (!service) {
    return;
  }

  const auto& blocklist = service->distraction_blocklist();
  if (blocklist.empty()) {
    auto placeholder = std::make_unique<views::Label>(
        u"No distraction sites added");
    placeholder->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    placeholder->SetAutoColorReadabilityEnabled(false);
    placeholder->SetEnabledColorId(ui::kColorLabelForegroundSecondary);
    placeholder->SetFontList(
        placeholder->font_list().DeriveWithSizeDelta(-1));
    blocklist_container_->AddChildView(std::move(placeholder));
  } else {
    for (const auto& site : blocklist) {
      auto row = std::make_unique<views::View>();
      auto* layout = row->SetLayoutManager(
          std::make_unique<views::BoxLayout>(
              views::BoxLayout::Orientation::kHorizontal,
              gfx::Insets::VH(4, 8), 8));
      layout->set_main_axis_alignment(
          views::BoxLayout::MainAxisAlignment::kSpaceBetween);
      layout->set_cross_axis_alignment(
          views::BoxLayout::CrossAxisAlignment::kCenter);
      row->SetPreferredSize(gfx::Size(0, 28));

      auto site_label = std::make_unique<views::Label>(
          base::UTF8ToUTF16(site));
      site_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
      site_label->SetAutoColorReadabilityEnabled(false);
      site_label->SetEnabledColorId(ui::kColorLabelForegroundPrimary);
      layout->SetFlexForView(site_label.get(), 1);
      row->AddChildView(std::move(site_label));

      auto remove_button = views::MdTextButton::Create(
          base::BindRepeating(
              &AstraSettingsPageView::OnRemoveDistractionSite,
              base::Unretained(this), site),
          u"\u2715");  // ✕
      remove_button->SetFontList(
          remove_button->font_list().DeriveWithSizeDelta(-1));
      row->AddChildView(std::move(remove_button));

      blocklist_container_->AddChildView(std::move(row));
    }
  }

  blocklist_container_->InvalidateLayout();
}

// =========================================================================
// Appearance section
// =========================================================================

void AstraSettingsPageView::BuildAppearanceSection() {
  auto section = std::make_unique<AstraSettingsSectionView>(u"Appearance");
  section->SetSection("appearance", u"Appearance",
                       u"Theme, color, and visual style settings");
  section->SetIconName("appearance");
  section->AddSearchKeywords(
      {u"theme", u"color", u"accent", u"dark", u"light", u"density",
       u"font", u"size", u"accessibility", u"high contrast",
       u"reduced motion", u"animation"});

  // Theme combobox.
  auto theme_model = CreateStringComboboxModel(
      {kThemeSystem, kThemeLight, kThemeDark});
  theme_combobox_ = section->AddComboboxRow(
      u"Theme", std::move(theme_model),
      base::BindRepeating(&AstraSettingsPageView::OnThemeSettingChanged,
                          base::Unretained(this)));

  // Accent color combobox.
  auto accent_model = CreateStringComboboxModel(
      {kAccentBlue, kAccentPurple, kAccentGreen, kAccentOrange, kAccentPink});
  accent_color_combobox_ = section->AddComboboxRow(
      u"Accent color", std::move(accent_model),
      base::BindRepeating(&AstraSettingsPageView::OnAccentColorChanged,
                          base::Unretained(this)));

  // Density combobox.
  auto density_model = CreateStringComboboxModel(
      {kDensityCompact, kDensityComfortable, kDensitySpacious});
  density_combobox_ = section->AddComboboxRow(
      u"UI density", std::move(density_model),
      base::BindRepeating(&AstraSettingsPageView::OnDensityChanged,
                          base::Unretained(this)));

  section->SetSettingCount(3);

  appearance_section_ = section.get();
  RegisterSection(appearance_section_);
  content_area_->AddChildView(std::move(section));
}

void AstraSettingsPageView::OnThemeSettingChanged() {
  // TODO(astra): Implement theme selection for Astra UI.
  // Currently Astra UI follows the system / Chrome theme.
  // A dedicated theme pref could override it.
  //
  // TODO(astra): Add kPrefTheme to astra_prefs.h (system/light/dark).
  // Chromium owner: ThemeService / NativeTheme.
  // Patch point: ui/native_theme/native_theme.h
}

void AstraSettingsPageView::OnAccentColorChanged() {
  // TODO(astra): Implement accent color pref.
  // Accent color is used for workspace indicators, highlights, etc.
  //
  // TODO(astra): Add kPrefAccentColor to astra_prefs.h.
  // Implementation: AstraColorProvider or a theme mixin.
}

void AstraSettingsPageView::OnDensityChanged() {
  // TODO(astra): Implement UI density pref.
  // Controls the spacing and size of Astra UI elements.
  //
  // TODO(astra): Add kPrefUIDensity to astra_prefs.h (compact/comfortable/spacious).
}

void AstraSettingsPageView::RefreshAppearanceSection() {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  // TODO(astra): Refresh theme, accent color, density when those prefs
  // are implemented.
}

// =========================================================================
// Search section
// =========================================================================

void AstraSettingsPageView::BuildSearchSection() {
  // Search engine settings section — projects Chromium's TemplateURLService
  // state and allows changing the default search engine.
  //
  // This uses the dedicated AstraSearchSettingsView component which handles
  // its own layout and refresh logic.  The settings page just embeds it
  // as a section.
  auto search_view = std::make_unique<AstraSearchSettingsView>(browser_);
  search_settings_view_ = search_view.get();

  // Set model if available.
  if (settings_model_) {
    search_settings_view_->SetModel(settings_model_);
  }

  content_area_->AddChildView(std::move(search_view));
}

void AstraSettingsPageView::RefreshSearchSection() {
  if (search_settings_view_) {
    search_settings_view_->RefreshFromService();
  }
}

// =========================================================================
// Privacy & Security section
// =========================================================================

void AstraSettingsPageView::BuildPrivacySection() {
  auto section = std::make_unique<AstraSettingsSectionView>(u"Privacy & Security");
  section->SetSection("privacy_security", u"Privacy & Security",
                       u"Tracking, cookies, and security controls");
  section->SetIconName("privacy");
  section->AddSearchKeywords(
      {u"privacy", u"tracking", u"tracker", u"cookies", u"security",
       u"data", u"block", u"protection", u"safe browsing"});

  // Tracker blocking toggle.
  tracker_blocking_toggle_ = section->AddToggleRow(
      u"Block trackers", true,
      base::BindRepeating(&AstraSettingsPageView::OnTrackerBlockingToggled,
                          base::Unretained(this)));

  // Cookie control combobox.
  auto cookie_model = CreateStringComboboxModel(
      {"Block third-party cookies", "Allow all cookies", "Block all cookies"});
  cookie_control_combobox_ = section->AddComboboxRow(
      u"Cookies", std::move(cookie_model),
      base::BindRepeating(&AstraSettingsPageView::OnCookieControlChanged,
                          base::Unretained(this)));

  section->AddDivider();

  // Open Chrome privacy settings button.
  section->AddButtonRow(
      u"Full privacy settings", u"Open",
      base::BindRepeating(&AstraSettingsPageView::OnOpenPrivacySettings,
                          base::Unretained(this)));

  section->SetSettingCount(2);

  privacy_section_ = section.get();
  RegisterSection(privacy_section_);
  content_area_->AddChildView(std::move(section));
}

void AstraSettingsPageView::OnTrackerBlockingToggled() {
  PrefService* prefs = GetPrefs();
  if (!prefs || !tracker_blocking_toggle_) {
    return;
  }
  // TODO(astra): Wire this to Chrome's Safe Browsing / tracking protection.
  // For now, this is a presentation-only toggle in the Astra settings UI.
  //
  // Chromium owner: SafeBrowsingService / Tracking Protection
  //   (chrome/browser/safe_browsing/)
  // Patch point: chrome/browser/prefs/safe_browsing_prefs.cc
}

void AstraSettingsPageView::OnCookieControlChanged() {
  PrefService* prefs = GetPrefs();
  if (!prefs || !cookie_control_combobox_) {
    return;
  }
  // TODO(astra): Wire this to Chrome's content settings / cookie controls.
  //
  // Chromium owner: CookieSettings (chrome/browser/content_settings/)
  // Patch point: components/content_settings/core/browser/cookie_settings.cc
}

void AstraSettingsPageView::OnOpenPrivacySettings() {
  if (!browser_) {
    return;
  }
  // Open Chrome's privacy settings page.
  chrome::ShowSettings(browser_);
  // TODO(astra): Navigate directly to the privacy subpage.
}

void AstraSettingsPageView::RefreshPrivacySection() {
  // TODO(astra): Refresh privacy settings from Chrome's prefs.
  // For now, these are presentation placeholders.
}

// =========================================================================
// Accessibility section
// =========================================================================

void AstraSettingsPageView::BuildAccessibilitySection() {
  auto section = std::make_unique<AstraSettingsSectionView>(u"Accessibility");
  section->SetSection("accessibility", u"Accessibility",
                      u"Visual and interaction accessibility settings");
  section->SetIconName("accessibility");
  section->AddSearchKeywords(
      {u"accessibility", u"high contrast", u"contrast", u"reduced motion",
       u"animation", u"font", u"text size", u"zoom", u"screen reader",
       u"large text", u"visual aids", u"dyslexia", u"colorblind"});

  // High contrast toggle.
  high_contrast_toggle_ = section->AddToggleRow(
      u"High contrast", false,
      base::BindRepeating(&AstraSettingsPageView::OnHighContrastToggled,
                          base::Unretained(this)));

  // Reduced motion toggle.
  reduced_motion_toggle_ = section->AddToggleRow(
      u"Reduced motion", false,
      base::BindRepeating(&AstraSettingsPageView::OnReducedMotionToggled,
                          base::Unretained(this)));

  // Font scale slider.
  font_scale_slider_ = section->AddSliderRow(
      u"Font scale", 0.4,  // 1.0 = 40% of range from 0.8 to 1.5
      base::BindRepeating(&FormatFontScale),
      base::BindRepeating(&AstraSettingsPageView::OnFontScaleChanged,
                          base::Unretained(this)));

  section->AddDivider();

  // Open Chrome accessibility settings button.
  section->AddButtonRow(
      u"Chrome accessibility", u"Open",
      base::BindRepeating(&AstraSettingsPageView::OnOpenAccessibilitySettings,
                          base::Unretained(this)));

  section->SetSettingCount(3);

  accessibility_section_ = section.get();
  RegisterSection(accessibility_section_);
  content_area_->AddChildView(std::move(section));
}

void AstraSettingsPageView::OnHighContrastToggled() {
  PrefService* prefs = GetPrefs();
  if (!prefs || !high_contrast_toggle_) {
    return;
  }
  // TODO(astra): Wire to Chrome's high contrast / accessibility prefs.
  //
  // Chromium owner: AccessibilityController
  //   (chrome/browser/accessibility/)
  // Patch point: components/prefs/pref_names.h —
  //   prefs::kAccessibilityHighContrastEnabled
}

void AstraSettingsPageView::OnReducedMotionToggled() {
  PrefService* prefs = GetPrefs();
  if (!prefs || !reduced_motion_toggle_) {
    return;
  }
  // TODO(astra): Wire to reduced motion pref (Astra-specific).
  //
  // This is an Astra-level setting that would suppress sidebar animations,
  // workspace transition animations, etc.
}

void AstraSettingsPageView::OnFontScaleChanged(double value) {
  PrefService* prefs = GetPrefs();
  if (!prefs || !font_scale_slider_) {
    return;
  }
  // TODO(astra): Apply font scale to Astra UI surfaces.
  //
  // For Chrome's own font size, see:
  //   chrome/browser/prefs/font_prefs.cc
  //   prefs::kWebKitDefaultFontSize
}

void AstraSettingsPageView::OnOpenAccessibilitySettings() {
  if (!browser_) {
    return;
  }
  OpenChromeSettingsPage("accessibility");
}

void AstraSettingsPageView::RefreshAccessibilitySection() {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  // TODO(astra): Read actual prefs for accessibility settings.
  // For now, these are presentation placeholders.
  if (high_contrast_toggle_) {
    high_contrast_toggle_->SetIsOn(false);
  }
  if (reduced_motion_toggle_) {
    reduced_motion_toggle_->SetIsOn(false);
  }
  if (font_scale_slider_) {
    font_scale_slider_->SetValue(0.4);  // 1.0x default
  }
}

// =========================================================================
// Extensions section
// =========================================================================

void AstraSettingsPageView::BuildExtensionsSection() {
  auto section = std::make_unique<AstraSettingsSectionView>(u"Extensions");
  section->SetSection("extensions", u"Extensions",
                      u"Extension management and integration settings");
  section->SetIconName("extensions");
  section->AddSearchKeywords(
      {u"extensions", u"add-ons", u"plugins", u"toolbar", u"sidebar",
       u"chrome web store", u"manage", u"disable", u"enable"});

  // Show on toolbar toggle.
  extension_show_toolbar_toggle_ = section->AddToggleRow(
      u"Show extensions on toolbar", true,
      base::BindRepeating(
          &AstraSettingsPageView::OnExtensionShowToolbarToggled,
          base::Unretained(this)));

  // Show in sidebar toggle.
  extension_show_sidebar_toggle_ = section->AddToggleRow(
      u"Show extensions in sidebar", true,
      base::BindRepeating(
          &AstraSettingsPageView::OnExtensionShowInSidebarToggled,
          base::Unretained(this)));

  section->AddDivider();

  // Manage extensions button.
  section->AddButtonRow(
      u"Manage extensions", u"Open",
      base::BindRepeating(&AstraSettingsPageView::OnManageExtensions,
                          base::Unretained(this)));

  section->SetSettingCount(2);

  extensions_section_ = section.get();
  RegisterSection(extensions_section_);
  content_area_->AddChildView(std::move(section));
}

void AstraSettingsPageView::OnExtensionShowToolbarToggled() {
  PrefService* prefs = GetPrefs();
  if (!prefs || !extension_show_toolbar_toggle_) {
    return;
  }
  // TODO(astra): Toggle extension toolbar visibility via AstraExtensionHelper.
  //
  // Chromium owner: ExtensionToolbarModel / ToolbarActionsModel
  //   (chrome/browser/ui/toolbar/)
}

void AstraSettingsPageView::OnExtensionShowInSidebarToggled() {
  PrefService* prefs = GetPrefs();
  if (!prefs || !extension_show_sidebar_toggle_) {
    return;
  }
  // TODO(astra): Toggle whether extensions appear in Astra sidebar.
  //
  // This is an Astra-level setting controlled by a pref.
}

void AstraSettingsPageView::OnManageExtensions() {
  if (!browser_) {
    return;
  }
  // Open Chrome's extensions page.
  chrome::ShowExtensions(browser_);
}

void AstraSettingsPageView::RefreshExtensionsSection() {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  // TODO(astra): Read actual prefs for extension settings.
  // For now, these are presentation placeholders.
  if (extension_show_toolbar_toggle_) {
    extension_show_toolbar_toggle_->SetIsOn(true);
  }
  if (extension_show_sidebar_toggle_) {
    extension_show_sidebar_toggle_->SetIsOn(true);
  }
}

// =========================================================================
// Performance section
// =========================================================================

void AstraSettingsPageView::BuildPerformanceSection() {
  auto section = std::make_unique<AstraSettingsSectionView>(u"Performance");
  section->SetSection("performance", u"Performance",
                      u"Memory and performance optimization settings");
  section->SetIconName("performance");
  section->AddSearchKeywords(
      {u"performance", u"memory", u"ram", u"speed", u"battery",
       u"power", u"energy", u"hardware acceleration", u"gpu",
       u"memory saver", u"sleep", u"inactive tabs"});

  // Memory saver toggle.
  memory_saver_toggle_ = section->AddToggleRow(
      u"Memory saver", false,
      base::BindRepeating(&AstraSettingsPageView::OnMemorySaverToggled,
                          base::Unretained(this)));

  // Memory saver timeout slider.
  memory_saver_timeout_slider_ = section->AddSliderRow(
      u"Memory saver timeout", 0.15,
      base::BindRepeating(&FormatMemorySaverTimeout),
      base::BindRepeating(
          &AstraSettingsPageView::OnMemorySaverTimeoutChanged,
          base::Unretained(this)));

  // Hardware acceleration toggle.
  hardware_acceleration_toggle_ = section->AddToggleRow(
      u"Hardware acceleration", true,
      base::BindRepeating(
          &AstraSettingsPageView::OnHardwareAccelerationToggled,
          base::Unretained(this)));

  section->SetSettingCount(3);

  performance_section_ = section.get();
  RegisterSection(performance_section_);
  content_area_->AddChildView(std::move(section));
}

void AstraSettingsPageView::OnMemorySaverToggled() {
  PrefService* prefs = GetPrefs();
  if (!prefs || !memory_saver_toggle_) {
    return;
  }
  // TODO(astra): Wire to Chrome's memory saver prefs.
  //
  // Chromium owner: PerformanceManager
  //   (chrome/browser/performance_manager/)
  // Patch point: chrome/browser/performance_manager/public/user_tuning/
  //   user_performance_tuning_manager.h
}

void AstraSettingsPageView::OnMemorySaverTimeoutChanged(double value) {
  PrefService* prefs = GetPrefs();
  if (!prefs || !memory_saver_timeout_slider_) {
    return;
  }
  // TODO(astra): Wire memory saver timeout to Chrome's prefs.
  //
  // See chrome/browser/performance_manager/.
}

void AstraSettingsPageView::OnHardwareAccelerationToggled() {
  PrefService* prefs = GetPrefs();
  if (!prefs || !hardware_acceleration_toggle_) {
    return;
  }
  // TODO(astra): Wire to Chrome's hardware acceleration setting.
  //
  // Chromium owner: GpuFeatureChecker / gpu::GpuFeatureInfo
  // Patch point: chrome/browser/gpu/gpu_mode_manager.cc
}

void AstraSettingsPageView::RefreshPerformanceSection() {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  // TODO(astra): Read actual prefs for performance settings.
  // For now, these are presentation placeholders.
  if (memory_saver_toggle_) {
    memory_saver_toggle_->SetIsOn(false);
  }
  if (memory_saver_timeout_slider_) {
    memory_saver_timeout_slider_->SetValue(0.15);  // ~10 min default
  }
  if (hardware_acceleration_toggle_) {
    hardware_acceleration_toggle_->SetIsOn(true);
  }
}

// =========================================================================
// Notifications section
// =========================================================================

void AstraSettingsPageView::BuildNotificationsSection() {
  auto section = std::make_unique<AstraSettingsSectionView>(u"Notifications");
  section->SetSection("notifications", u"Notifications",
                      u"Notification and do-not-disturb settings");
  section->SetIconName("notifications");
  section->AddSearchKeywords(
      {u"notifications", u"alerts", u"do not disturb", u"dnd",
       u"silent", u"quiet hours", u"focus mode", u"push",
       u"web notifications", u"site notifications"});

  // Notifications toggle.
  notifications_toggle_ = section->AddToggleRow(
      u"Notifications", true,
      base::BindRepeating(&AstraSettingsPageView::OnNotificationsToggled,
                          base::Unretained(this)));

  // Do not disturb toggle.
  do_not_disturb_toggle_ = section->AddToggleRow(
      u"Do not disturb", false,
      base::BindRepeating(&AstraSettingsPageView::OnDoNotDisturbToggled,
                          base::Unretained(this)));

  section->SetSettingCount(2);

  notifications_section_ = section.get();
  RegisterSection(notifications_section_);
  content_area_->AddChildView(std::move(section));
}

void AstraSettingsPageView::OnNotificationsToggled() {
  PrefService* prefs = GetPrefs();
  if (!prefs || !notifications_toggle_) {
    return;
  }
  // TODO(astra): Wire to Chrome's notification content settings.
  //
  // Chromium owner: PermissionManager / NotificationPermissionContext
  //   (chrome/browser/notifications/)
  // Patch point: components/content_settings/core/browser/
  //   host_content_settings_map.cc
}

void AstraSettingsPageView::OnDoNotDisturbToggled() {
  PrefService* prefs = GetPrefs();
  if (!prefs || !do_not_disturb_toggle_) {
    return;
  }
  // TODO(astra): Toggle do-not-disturb mode.
  //
  // This is an Astra-level feature that integrates with focus mode.
}

void AstraSettingsPageView::RefreshNotificationsSection() {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  // TODO(astra): Read actual prefs for notification settings.
  // For now, these are presentation placeholders.
  if (notifications_toggle_) {
    notifications_toggle_->SetIsOn(true);
  }
  if (do_not_disturb_toggle_) {
    do_not_disturb_toggle_->SetIsOn(false);
  }
}

// =========================================================================
// Advanced section
// =========================================================================

void AstraSettingsPageView::BuildAdvancedSection() {
  auto section = std::make_unique<AstraSettingsSectionView>(u"Advanced");
  section->SetSection("advanced", u"Advanced",
                      u"Experimental features and developer settings");
  section->SetIconName("advanced");
  section->AddSearchKeywords(
      {u"advanced", u"experimental", u"flags", u"beta", u"dev",
       u"developer", u"devtools", u"debug", u"about", u"version",
       u"chrome settings"});

  // Experimental features toggle.
  experimental_features_toggle_ = section->AddToggleRow(
      u"Experimental features", false,
      base::BindRepeating(
          &AstraSettingsPageView::OnExperimentalFeaturesToggled,
          base::Unretained(this)));

  // DevTools panel toggle.
  devtools_panel_toggle_ = section->AddToggleRow(
      u"DevTools panel", false,
      base::BindRepeating(&AstraSettingsPageView::OnDevToolsPanelToggled,
                          base::Unretained(this)));

  section->AddDivider();

  // Open Chrome settings button.
  section->AddButtonRow(
      u"Chrome settings", u"Open",
      base::BindRepeating(&AstraSettingsPageView::OnOpenChromeSettings,
                          base::Unretained(this)));

  // About button.
  section->AddButtonRow(
      u"About Astra", u"Open",
      base::BindRepeating(&AstraSettingsPageView::OnOpenChromeAbout,
                          base::Unretained(this)));

  section->SetSettingCount(2);

  advanced_section_ = section.get();
  RegisterSection(advanced_section_);
  content_area_->AddChildView(std::move(section));
}

void AstraSettingsPageView::OnExperimentalFeaturesToggled() {
  PrefService* prefs = GetPrefs();
  if (!prefs || !experimental_features_toggle_) {
    return;
  }

  bool enabled = experimental_features_toggle_->GetIsOn();
  // TODO(astra): Persist experimental features enabled state.
  //   prefs->SetBoolean(astra::prefs::kExperimentalFeaturesEnabled, enabled);
}

void AstraSettingsPageView::OnDevToolsPanelToggled() {
  PrefService* prefs = GetPrefs();
  if (!prefs || !devtools_panel_toggle_) {
    return;
  }
  // TODO(astra): Toggle Astra DevTools panel visibility.
  //
  // This would be controlled by an extension or a content script.
}

void AstraSettingsPageView::OnOpenChromeSettings() {
  if (!browser_) {
    return;
  }
  chrome::ShowSettings(browser_);
}

void AstraSettingsPageView::OnOpenChromeAbout() {
  if (!browser_) {
    return;
  }
  // Open Chrome's about page.
  chrome::ShowAboutChrome(browser_);
}

void AstraSettingsPageView::RefreshAdvancedSection() {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  // TODO(astra): Read actual prefs for advanced settings.
  // For now, these are presentation placeholders.
  if (experimental_features_toggle_) {
    experimental_features_toggle_->SetIsOn(false);
  }
  if (devtools_panel_toggle_) {
    devtools_panel_toggle_->SetIsOn(false);
  }
}

// =========================================================================
// Bulk operations section
// =========================================================================

void AstraSettingsPageView::BuildBulkOperationsSection() {
  auto section = std::make_unique<AstraSettingsSectionView>(u"System");
  section->SetSection("system", u"System",
                      u"Settings management and system operations");
  section->SetIconName("system");
  section->AddSearchKeywords(
      {u"system", u"reset", u"export", u"import", u"backup",
       u"restore", u"defaults", u"settings", u"data", u"sync"});

  // Reset to defaults button.
  section->AddButtonRow(
      u"Reset all settings", u"Reset",
      base::BindRepeating(&AstraSettingsPageView::OnResetAllToDefaults,
                          base::Unretained(this)));

  section->AddDivider();

  // Export settings button.
  section->AddButtonRow(
      u"Export settings", u"Export",
      base::BindRepeating(&AstraSettingsPageView::OnExportSettings,
                          base::Unretained(this)));

  // Import settings button.
  section->AddButtonRow(
      u"Import settings", u"Import",
      base::BindRepeating(&AstraSettingsPageView::OnImportSettings,
                          base::Unretained(this)));

  section->SetSettingCount(0);

  // Store the section for search filtering — it has no specific member.
  RegisterSection(section.get());
  content_area_->AddChildView(std::move(section));
}

void AstraSettingsPageView::OnResetAllToDefaults() {
  if (!settings_model_) {
    return;
  }
  settings_model_->ResetAllSettings();
  RefreshFromPrefs();
}

void AstraSettingsPageView::OnExportSettings() {
  // TODO(astra): Export settings to JSON file.
  //
  // Chromium owner: Profile (file access)
  // Use base::FileUtilProxy or chrome::SelectFileDialog for file I/O.
}

void AstraSettingsPageView::OnImportSettings() {
  // TODO(astra): Import settings from JSON file.
  //
  // Use a file picker dialog and parse JSON into PrefService.
}

// =========================================================================
// Workspace service observer overrides
// =========================================================================

void AstraSettingsPageView::OnWorkspaceAdded(const AstraWorkspace& workspace) {
  RefreshWorkspacesSection();
}

void AstraSettingsPageView::OnWorkspaceRemoved(const std::string& workspace_id) {
  RefreshWorkspacesSection();
}

void AstraSettingsPageView::OnWorkspaceRenamed(const std::string& workspace_id,
                                               const std::string& new_name) {
  RefreshWorkspacesSection();
}

void AstraSettingsPageView::OnActiveWorkspaceChanged(
    const std::string& old_id,
    const std::string& new_id) {
  RefreshWorkspacesSection();
}

void AstraSettingsPageView::OnWorkspacesReordered() {
  RefreshWorkspacesSection();
}

// =========================================================================
// Focus mode service observer overrides
// =========================================================================

void AstraSettingsPageView::OnFocusModeEntered(base::TimeDelta duration) {
  RefreshFocusModeSection();
}

void AstraSettingsPageView::OnFocusModeExited() {
  RefreshFocusModeSection();
}

void AstraSettingsPageView::OnFocusTimeUpdated(base::TimeDelta remaining) {
  // Could update a live display, but for settings page refresh is fine.
  RefreshFocusModeSection();
}

void AstraSettingsPageView::OnDistractionBlocklistChanged() {
  RefreshDistractionBlocklist();
}

// =========================================================================
// AstraSettingsObserver overrides
// =========================================================================

void AstraSettingsPageView::OnSettingChanged(AstraSettingsModel* model,
                                             const std::string& key) {
  // TODO(astra): Dispatch to the relevant section refresh based on key.
  // For now, refresh everything.
  RefreshFromPrefs();
}

void AstraSettingsPageView::OnSettingsReset(AstraSettingsModel* model) {
  RefreshFromPrefs();
}

void AstraSettingsPageView::OnSettingsSearchResultsChanged(
    AstraSettingsModel* model) {
  // TODO(astra): If we have a dedicated search results view, update it here.
  // Currently search results are shown by filtering sections in place.
}

void AstraSettingsPageView::OnSettingsModelShutdown(AstraSettingsModel* model) {
  if (settings_model_ == model) {
    settings_model_ = nullptr;
  }
}

// =========================================================================
// Pref change handler
// =========================================================================

void AstraSettingsPageView::OnPrefChanged(const std::string& pref_name) {
  // Dispatch pref changes to the relevant section refresh methods.
  // TODO(astra): Map pref names to sections for targeted refreshes.
  // For now, just refresh everything — acceptable for a settings page.
  RefreshFromPrefs();
}

// =========================================================================
// Helper methods
// =========================================================================

PrefService* AstraSettingsPageView::GetPrefs() {
  if (!browser_) {
    return nullptr;
  }
  Profile* profile = browser_->profile();
  if (!profile) {
    return nullptr;
  }
  return profile->GetPrefs();
}

AstraWorkspaceService* AstraSettingsPageView::GetWorkspaceService() {
  if (!browser_) {
    return nullptr;
  }
  Profile* profile = browser_->profile();
  if (!profile) {
    return nullptr;
  }
  return AstraWorkspaceService::GetForProfile(profile);
}

AstraFocusModeService* AstraSettingsPageView::GetFocusService() {
  if (!browser_) {
    return nullptr;
  }
  Profile* profile = browser_->profile();
  if (!profile) {
    return nullptr;
  }
  return AstraFocusModeService::GetForProfile(profile);
}

void AstraSettingsPageView::OpenChromeSettingsPage(
    const std::string& subpage) {
  if (!browser_) {
    return;
  }
  // For subpage navigation, we'd use chrome::ShowSettingsSubPage() or
  // navigate to chrome://settings/<subpage>.
  // For now, just open the main settings page.
  // TODO(astra): Navigate to specific subpage when needed.
  chrome::ShowSettings(browser_);
}

void AstraSettingsPageView::OnThemeChanged() {
  views::View::OnThemeChanged();

  // Colors are set via color IDs throughout the view hierarchy, so they
  // update automatically on theme change.  We just need to ensure any
  // custom-drawn elements are invalidated.
  if (content_scroll_view_) {
    content_scroll_view_->InvalidateLayout();
  }
}

}  // namespace astra
