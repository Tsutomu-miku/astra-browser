#include "astra/ui/views/settings/astra_settings_model.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "components/prefs/pref_service.h"

namespace astra {

namespace {

// Section ID constants.
constexpr char kSectionAppearance[] = "appearance";
constexpr char kSectionWorkspaces[] = "workspaces";
constexpr char kSectionSidebar[] = "sidebar";
constexpr char kSectionTabs[] = "tabs";
constexpr char kSectionPrivacySecurity[] = "privacy_security";
constexpr char kSectionSearch[] = "search";
constexpr char kSectionAccessibility[] = "accessibility";
constexpr char kSectionPerformance[] = "performance";
constexpr char kSectionNotifications[] = "notifications";
constexpr char kSectionAdvanced[] = "advanced";

// Helper to check if a query matches text (case-insensitive substring).
bool MatchesText(const std::u16string& text, const std::u16string& query) {
  if (query.empty()) {
    return true;
  }
  return base::ToLowerASCII(text).find(base::ToLowerASCII(query)) !=
         std::u16string::npos;
}

}  // namespace

// =========================================================================
// AstraSettingItem
// =========================================================================

AstraSettingItem::AstraSettingItem()
    : type(AstraSettingType::kBoolean),
      is_managed(false),
      is_recommended(false) {}

AstraSettingItem::AstraSettingItem(const AstraSettingItem& other) = default;

AstraSettingItem& AstraSettingItem::operator=(const AstraSettingItem& other) =
    default;

AstraSettingItem::~AstraSettingItem() = default;

// =========================================================================
// Construction / destruction
// =========================================================================

AstraSettingsModel::AstraSettingsModel(PrefService* pref_service)
    : pref_service_(pref_service) {
  InitializeDefaults();
}

AstraSettingsModel::~AstraSettingsModel() {
  NotifyShutdown();
}

// =========================================================================
// Observer management
// =========================================================================

void AstraSettingsModel::AddObserver(AstraSettingsObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraSettingsModel::RemoveObserver(AstraSettingsObserver* observer) {
  observers_.RemoveObserver(observer);
}

// =========================================================================
// Defaults initialization
// =========================================================================

void AstraSettingsModel::InitializeDefaults() {
  // -- Define sections ---------------------------------------------------

  // Appearance section.
  AstraSettingsSectionInfo appearance;
  appearance.id = kSectionAppearance;
  appearance.name = u"Appearance";
  appearance.description = u"Theme, color, and visual style settings";
  appearance.icon_name = "appearance";
  sections_[appearance.id] = appearance;

  // Workspaces section.
  AstraSettingsSectionInfo workspaces;
  workspaces.id = kSectionWorkspaces;
  workspaces.name = u"Workspaces";
  workspaces.description = u"Workspace organization and behavior";
  workspaces.icon_name = "workspaces";
  sections_[workspaces.id] = workspaces;

  // Sidebar section.
  AstraSettingsSectionInfo sidebar;
  sidebar.id = kSectionSidebar;
  sidebar.name = u"Sidebar";
  sidebar.description = u"Sidebar visibility, position, and behavior";
  sidebar.icon_name = "sidebar";
  sections_[sidebar.id] = sidebar;

  // Tabs section.
  AstraSettingsSectionInfo tabs;
  tabs.id = kSectionTabs;
  tabs.name = u"Tabs";
  tabs.description = u"Tab behavior, split view, and tab management";
  tabs.icon_name = "tabs";
  sections_[tabs.id] = tabs;

  // Privacy & Security section.
  AstraSettingsSectionInfo privacy;
  privacy.id = kSectionPrivacySecurity;
  privacy.name = u"Privacy & Security";
  privacy.description = u"Tracking, cookies, and security controls";
  privacy.icon_name = "privacy";
  sections_[privacy.id] = privacy;

  // Search section.
  AstraSettingsSectionInfo search;
  search.id = kSectionSearch;
  search.name = u"Search";
  search.description = u"Search engine and search suggestions";
  search.icon_name = "search";
  sections_[search.id] = search;

  // Accessibility section.
  AstraSettingsSectionInfo accessibility;
  accessibility.id = kSectionAccessibility;
  accessibility.name = u"Accessibility";
  accessibility.description = u"Display and accessibility options";
  accessibility.icon_name = "accessibility";
  sections_[accessibility.id] = accessibility;

  // Performance section.
  AstraSettingsSectionInfo performance;
  performance.id = kSectionPerformance;
  performance.name = u"Performance";
  performance.description = u"Memory saver and performance settings";
  performance.icon_name = "performance";
  sections_[performance.id] = performance;

  // Notifications section.
  AstraSettingsSectionInfo notifications;
  notifications.id = kSectionNotifications;
  notifications.name = u"Notifications";
  notifications.description = u"Notification preferences and alerts";
  notifications.icon_name = "notifications";
  sections_[notifications.id] = notifications;

  // Advanced section.
  AstraSettingsSectionInfo advanced;
  advanced.id = kSectionAdvanced;
  advanced.name = u"Advanced";
  advanced.description = u"Experimental features and developer settings";
  advanced.icon_name = "advanced";
  sections_[advanced.id] = advanced;

  // -- Appearance settings ----------------------------------------------

  // Theme mode.
  {
    AstraSettingItem setting;
    setting.key = "theme_mode";
    setting.title = u"Theme";
    setting.description = u"Choose the browser color theme";
    setting.type = AstraSettingType::kEnum;
    setting.section = kSectionAppearance;
    setting.icon_name = "theme";
    setting.search_tags = {u"theme", u"color", u"dark", u"light",
                           u"appearance", u"mode"};
    setting.default_value = base::Value("system");
    setting.current_value = base::Value("system");
    setting.options = {u"System default", u"Light", u"Dark"};
    settings_[setting.key] = setting;
    sections_[kSectionAppearance].setting_keys.push_back(setting.key);
  }

  // Accent color.
  {
    AstraSettingItem setting;
    setting.key = "accent_color";
    setting.title = u"Accent color";
    setting.description = u"Choose your accent color";
    setting.type = AstraSettingType::kEnum;
    setting.section = kSectionAppearance;
    setting.icon_name = "color";
    setting.search_tags = {u"accent", u"color", u"highlight", u"theme"};
    setting.default_value = base::Value("blue");
    setting.current_value = base::Value("blue");
    setting.options = {u"Blue", u"Purple", u"Green", u"Orange", u"Pink"};
    settings_[setting.key] = setting;
    sections_[kSectionAppearance].setting_keys.push_back(setting.key);
  }

  // UI density.
  {
    AstraSettingItem setting;
    setting.key = "ui_density";
    setting.title = u"UI density";
    setting.description = u"Adjust the spacing of UI elements";
    setting.type = AstraSettingType::kEnum;
    setting.section = kSectionAppearance;
    setting.icon_name = "density";
    setting.search_tags = {u"density", u"spacing", u"compact", u"comfortable",
                           u"spacious", u"size"};
    setting.default_value = base::Value("comfortable");
    setting.current_value = base::Value("comfortable");
    setting.options = {u"Compact", u"Comfortable", u"Spacious"};
    settings_[setting.key] = setting;
    sections_[kSectionAppearance].setting_keys.push_back(setting.key);
  }

  // Font scale.
  {
    AstraSettingItem setting;
    setting.key = "font_scale";
    setting.title = u"Text size";
    setting.description = u"Adjust the font scale";
    setting.type = AstraSettingType::kDouble;
    setting.section = kSectionAppearance;
    setting.icon_name = "font_size";
    setting.search_tags = {u"font", u"text", u"size", u"scale", u"zoom",
                           u"accessibility", u"large"};
    setting.default_value = base::Value(1.0);
    setting.current_value = base::Value(1.0);
    setting.min_value = base::Value(0.8);
    setting.max_value = base::Value(1.5);
    settings_[setting.key] = setting;
    sections_[kSectionAppearance].setting_keys.push_back(setting.key);
  }

  // -- Workspaces settings -----------------------------------------------

  // Show workspace indicator.
  {
    AstraSettingItem setting;
    setting.key = "show_workspace_indicator";
    setting.title = u"Show workspace indicator";
    setting.description = u"Display the active workspace in the tab strip";
    setting.type = AstraSettingType::kBoolean;
    setting.section = kSectionWorkspaces;
    setting.icon_name = "indicator";
    setting.search_tags = {u"workspace", u"indicator", u"tab strip",
                           u"display", u"visible"};
    setting.default_value = base::Value(true);
    setting.current_value = base::Value(true);
    settings_[setting.key] = setting;
    sections_[kSectionWorkspaces].setting_keys.push_back(setting.key);
  }

  // New tab behavior.
  {
    AstraSettingItem setting;
    setting.key = "new_tab_behavior";
    setting.title = u"New tabs open in";
    setting.description = u"Choose where new tabs are placed";
    setting.type = AstraSettingType::kEnum;
    setting.section = kSectionWorkspaces;
    setting.icon_name = "new_tab";
    setting.search_tags = {u"new tab", u"workspace", u"behavior",
                           u"create", u"open"};
    setting.default_value = base::Value("current");
    setting.current_value = base::Value("current");
    setting.options = {u"Current workspace", u"New workspace", u"No workspace"};
    settings_[setting.key] = setting;
    sections_[kSectionWorkspaces].setting_keys.push_back(setting.key);
  }

  // Workspace display density.
  {
    AstraSettingItem setting;
    setting.key = "workspace_display_density";
    setting.title = u"Workspace display density";
    setting.description = u"Adjust how workspace items appear in the sidebar";
    setting.type = AstraSettingType::kEnum;
    setting.section = kSectionWorkspaces;
    setting.icon_name = "density";
    setting.search_tags = {u"workspace", u"density", u"size", u"sidebar",
                           u"display"};
    setting.default_value = base::Value("comfortable");
    setting.current_value = base::Value("comfortable");
    setting.options = {u"Compact", u"Comfortable", u"Spacious"};
    settings_[setting.key] = setting;
    sections_[kSectionWorkspaces].setting_keys.push_back(setting.key);
  }

  // -- Sidebar settings --------------------------------------------------

  // Sidebar visible.
  {
    AstraSettingItem setting;
    setting.key = "sidebar_visible";
    setting.title = u"Show sidebar";
    setting.description = u"Toggle the sidebar visibility";
    setting.type = AstraSettingType::kBoolean;
    setting.section = kSectionSidebar;
    setting.icon_name = "sidebar";
    setting.search_tags = {u"sidebar", u"visible", u"show", u"hide",
                           u"panel", u"display"};
    setting.default_value = base::Value(true);
    setting.current_value = base::Value(true);
    settings_[setting.key] = setting;
    sections_[kSectionSidebar].setting_keys.push_back(setting.key);
  }

  // Sidebar position.
  {
    AstraSettingItem setting;
    setting.key = "sidebar_position";
    setting.title = u"Sidebar position";
    setting.description = u"Choose which side the sidebar appears on";
    setting.type = AstraSettingType::kEnum;
    setting.section = kSectionSidebar;
    setting.icon_name = "position";
    setting.search_tags = {u"sidebar", u"position", u"left", u"right",
                           u"side", u"dock"};
    setting.default_value = base::Value("left");
    setting.current_value = base::Value("left");
    setting.options = {u"Left", u"Right"};
    settings_[setting.key] = setting;
    sections_[kSectionSidebar].setting_keys.push_back(setting.key);
  }

  // Sidebar auto-hide.
  {
    AstraSettingItem setting;
    setting.key = "sidebar_auto_hide";
    setting.title = u"Auto-hide when not in use";
    setting.description = u"Automatically hide the sidebar when not needed";
    setting.type = AstraSettingType::kBoolean;
    setting.section = kSectionSidebar;
    setting.icon_name = "auto_hide";
    setting.search_tags = {u"sidebar", u"auto-hide", u"hide", u"automatic",
                           u"minimize"};
    setting.default_value = base::Value(false);
    setting.current_value = base::Value(false);
    settings_[setting.key] = setting;
    sections_[kSectionSidebar].setting_keys.push_back(setting.key);
  }

  // Sidebar width.
  {
    AstraSettingItem setting;
    setting.key = "sidebar_width";
    setting.title = u"Sidebar width";
    setting.description = u"Adjust the sidebar width in pixels";
    setting.type = AstraSettingType::kInteger;
    setting.section = kSectionSidebar;
    setting.icon_name = "width";
    setting.search_tags = {u"sidebar", u"width", u"size", u"resize",
                           u"pixel"};
    setting.default_value = base::Value(320);
    setting.current_value = base::Value(320);
    setting.min_value = base::Value(200);
    setting.max_value = base::Value(500);
    settings_[setting.key] = setting;
    sections_[kSectionSidebar].setting_keys.push_back(setting.key);
  }

  // Sidebar pinned.
  {
    AstraSettingItem setting;
    setting.key = "sidebar_pinned";
    setting.title = u"Pin sidebar open";
    setting.description = u"Keep the sidebar pinned open at all times";
    setting.type = AstraSettingType::kBoolean;
    setting.section = kSectionSidebar;
    setting.icon_name = "pin";
    setting.search_tags = {u"sidebar", u"pin", u"pinned", u"fixed",
                           u"always"};
    setting.default_value = base::Value(true);
    setting.current_value = base::Value(true);
    settings_[setting.key] = setting;
    sections_[kSectionSidebar].setting_keys.push_back(setting.key);
  }

  // -- Tabs settings -----------------------------------------------------

  // Tab stacking.
  {
    AstraSettingItem setting;
    setting.key = "tab_stacking";
    setting.title = u"Enable tab stacking";
    setting.description = u"Group related tabs into stacks";
    setting.type = AstraSettingType::kBoolean;
    setting.section = kSectionTabs;
    setting.icon_name = "stack";
    setting.search_tags = {u"tabs", u"stacking", u"stack", u"groups",
                           u"organize", u"tab groups"};
    setting.default_value = base::Value(true);
    setting.current_value = base::Value(true);
    settings_[setting.key] = setting;
    sections_[kSectionTabs].setting_keys.push_back(setting.key);
  }

  // Split view orientation.
  {
    AstraSettingItem setting;
    setting.key = "split_view_orientation";
    setting.title = u"Default split view orientation";
    setting.description = u"Choose the default split view layout direction";
    setting.type = AstraSettingType::kEnum;
    setting.section = kSectionTabs;
    setting.icon_name = "split";
    setting.search_tags = {u"split", u"split view", u"orientation",
                           u"horizontal", u"vertical", u"layout"};
    setting.default_value = base::Value("horizontal");
    setting.current_value = base::Value("horizontal");
    setting.options = {u"Horizontal", u"Vertical"};
    settings_[setting.key] = setting;
    sections_[kSectionTabs].setting_keys.push_back(setting.key);
  }

  // Split view enabled.
  {
    AstraSettingItem setting;
    setting.key = "split_view_enabled";
    setting.title = u"Enable split view";
    setting.description = u"Allow tabs to be arranged in split view";
    setting.type = AstraSettingType::kBoolean;
    setting.section = kSectionTabs;
    setting.icon_name = "split";
    setting.search_tags = {u"split", u"split view", u"tiling", u"layout",
                           u"arrange"};
    setting.default_value = base::Value(true);
    setting.current_value = base::Value(true);
    settings_[setting.key] = setting;
    sections_[kSectionTabs].setting_keys.push_back(setting.key);
  }

  // Recently closed count.
  {
    AstraSettingItem setting;
    setting.key = "recently_closed_count";
    setting.title = u"Recently closed tabs";
    setting.description = u"Number of recently closed tabs to show";
    setting.type = AstraSettingType::kInteger;
    setting.section = kSectionTabs;
    setting.icon_name = "history";
    setting.search_tags = {u"recently closed", u"recent", u"history",
                           u"count", u"tabs"};
    setting.default_value = base::Value(10);
    setting.current_value = base::Value(10);
    setting.min_value = base::Value(0);
    setting.max_value = base::Value(50);
    settings_[setting.key] = setting;
    sections_[kSectionTabs].setting_keys.push_back(setting.key);
  }

  // Tab hover peek.
  {
    AstraSettingItem setting;
    setting.key = "tab_hover_peek";
    setting.title = u"Tab hover preview (peek)";
    setting.description = u"Show a thumbnail preview when hovering over tabs";
    setting.type = AstraSettingType::kBoolean;
    setting.section = kSectionTabs;
    setting.icon_name = "hover";
    setting.search_tags = {u"tabs", u"hover", u"peek", u"preview",
                           u"thumbnail", u"card"};
    setting.default_value = base::Value(true);
    setting.current_value = base::Value(true);
    settings_[setting.key] = setting;
    sections_[kSectionTabs].setting_keys.push_back(setting.key);
  }

  // -- Privacy & Security settings ---------------------------------------

  // Tracker blocking.
  {
    AstraSettingItem setting;
    setting.key = "tracker_blocking";
    setting.title = u"Block trackers";
    setting.description = u"Block cross-site tracking";
    setting.type = AstraSettingType::kBoolean;
    setting.section = kSectionPrivacySecurity;
    setting.icon_name = "shield";
    setting.search_tags = {u"privacy", u"tracking", u"trackers",
                           u"block", u"protection", u"security"};
    setting.default_value = base::Value(true);
    setting.current_value = base::Value(true);
    settings_[setting.key] = setting;
    sections_[kSectionPrivacySecurity].setting_keys.push_back(setting.key);
  }

  // Cookie control.
  {
    AstraSettingItem setting;
    setting.key = "cookie_control";
    setting.title = u"Cookies";
    setting.description = u"Control how cookies are handled";
    setting.type = AstraSettingType::kEnum;
    setting.section = kSectionPrivacySecurity;
    setting.icon_name = "cookie";
    setting.search_tags = {u"cookies", u"privacy", u"block", u"allow",
                           u"third-party", u"tracking"};
    setting.default_value = base::Value("block_third_party");
    setting.current_value = base::Value("block_third_party");
    setting.options = {u"Allow all cookies", u"Block third-party cookies",
                       u"Block all cookies"};
    settings_[setting.key] = setting;
    sections_[kSectionPrivacySecurity].setting_keys.push_back(setting.key);
  }

  // Safe browsing.
  {
    AstraSettingItem setting;
    setting.key = "safe_browsing";
    setting.title = u"Safe Browsing";
    setting.description = u"Protect against dangerous sites and downloads";
    setting.type = AstraSettingType::kBoolean;
    setting.section = kSectionPrivacySecurity;
    setting.icon_name = "shield";
    setting.search_tags = {u"safe browsing", u"security", u"protection",
                           u"dangerous", u"malware", u"phishing"};
    setting.default_value = base::Value(true);
    setting.current_value = base::Value(true);
    setting.is_recommended = true;
    settings_[setting.key] = setting;
    sections_[kSectionPrivacySecurity].setting_keys.push_back(setting.key);
  }

  // -- Search settings ---------------------------------------------------

  // Default search engine.
  {
    AstraSettingItem setting;
    setting.key = "default_search_engine";
    setting.title = u"Default search engine";
    setting.description = u"Choose your default search engine";
    setting.type = AstraSettingType::kString;
    setting.section = kSectionSearch;
    setting.icon_name = "search";
    setting.search_tags = {u"search", u"engine", u"default", u"google",
                           u"bing", u"duckduckgo", u"query"};
    setting.default_value = base::Value("google");
    setting.current_value = base::Value("google");
    settings_[setting.key] = setting;
    sections_[kSectionSearch].setting_keys.push_back(setting.key);
  }

  // Search suggestions.
  {
    AstraSettingItem setting;
    setting.key = "search_suggestions";
    setting.title = u"Search suggestions";
    setting.description = u"Show search suggestions as you type";
    setting.type = AstraSettingType::kBoolean;
    setting.section = kSectionSearch;
    setting.icon_name = "suggestions";
    setting.search_tags = {u"search", u"suggestions", u"autocomplete",
                           u"predictions", u"typing"};
    setting.default_value = base::Value(true);
    setting.current_value = base::Value(true);
    settings_[setting.key] = setting;
    sections_[kSectionSearch].setting_keys.push_back(setting.key);
  }

  // -- Accessibility settings --------------------------------------------

  // High contrast mode.
  {
    AstraSettingItem setting;
    setting.key = "high_contrast_mode";
    setting.title = u"High contrast mode";
    setting.description = u"Increase contrast for better readability";
    setting.type = AstraSettingType::kBoolean;
    setting.section = kSectionAccessibility;
    setting.icon_name = "contrast";
    setting.search_tags = {u"accessibility", u"high contrast", u"contrast",
                           u"readability", u"visibility", u"a11y"};
    setting.default_value = base::Value(false);
    setting.current_value = base::Value(false);
    settings_[setting.key] = setting;
    sections_[kSectionAccessibility].setting_keys.push_back(setting.key);
  }

  // Reduced motion.
  {
    AstraSettingItem setting;
    setting.key = "reduced_motion";
    setting.title = u"Reduced motion";
    setting.description = u"Reduce animations and motion effects";
    setting.type = AstraSettingType::kBoolean;
    setting.section = kSectionAccessibility;
    setting.icon_name = "motion";
    setting.search_tags = {u"accessibility", u"reduced motion", u"animation",
                           u"motion", u"effects", u"a11y", u"reduce"};
    setting.default_value = base::Value(false);
    setting.current_value = base::Value(false);
    settings_[setting.key] = setting;
    sections_[kSectionAccessibility].setting_keys.push_back(setting.key);
  }

  // Screen reader support.
  {
    AstraSettingItem setting;
    setting.key = "screen_reader_support";
    setting.title = u"Screen reader support";
    setting.description = u"Optimize UI for screen readers";
    setting.type = AstraSettingType::kBoolean;
    setting.section = kSectionAccessibility;
    setting.icon_name = "screen_reader";
    setting.search_tags = {u"accessibility", u"screen reader", u"voiceover",
                           u"a11y", u"access", u"narrator"};
    setting.default_value = base::Value(false);
    setting.current_value = base::Value(false);
    settings_[setting.key] = setting;
    sections_[kSectionAccessibility].setting_keys.push_back(setting.key);
  }

  // -- Performance settings ----------------------------------------------

  // Memory saver.
  {
    AstraSettingItem setting;
    setting.key = "memory_saver";
    setting.title = u"Memory saver";
    setting.description = u"Automatically free up memory from inactive tabs";
    setting.type = AstraSettingType::kBoolean;
    setting.section = kSectionPerformance;
    setting.icon_name = "memory";
    setting.search_tags = {u"performance", u"memory", u"ram", u"saver",
                           u"optimize", u"speed", u"inactive", u"tabs"};
    setting.default_value = base::Value(false);
    setting.current_value = base::Value(false);
    settings_[setting.key] = setting;
    sections_[kSectionPerformance].setting_keys.push_back(setting.key);
  }

  // Memory saver timeout.
  {
    AstraSettingItem setting;
    setting.key = "memory_saver_timeout";
    setting.title = u"Memory saver timeout";
    setting.description = u"Time before inactive tabs are suspended";
    setting.type = AstraSettingType::kInteger;
    setting.section = kSectionPerformance;
    setting.icon_name = "timer";
    setting.search_tags = {u"performance", u"memory", u"timeout",
                           u"inactive", u"suspend", u"minutes"};
    setting.default_value = base::Value(30);
    setting.current_value = base::Value(30);
    setting.min_value = base::Value(1);
    setting.max_value = base::Value(60);
    settings_[setting.key] = setting;
    sections_[kSectionPerformance].setting_keys.push_back(setting.key);
  }

  // Hardware acceleration.
  {
    AstraSettingItem setting;
    setting.key = "hardware_acceleration";
    setting.title = u"Hardware acceleration";
    setting.description = u"Use GPU for faster rendering when available";
    setting.type = AstraSettingType::kBoolean;
    setting.section = kSectionPerformance;
    setting.icon_name = "gpu";
    setting.search_tags = {u"performance", u"hardware", u"acceleration",
                           u"gpu", u"graphics", u"rendering"};
    setting.default_value = base::Value(true);
    setting.current_value = base::Value(true);
    setting.is_recommended = true;
    settings_[setting.key] = setting;
    sections_[kSectionPerformance].setting_keys.push_back(setting.key);
  }

  // -- Notifications settings --------------------------------------------

  // Notifications enabled.
  {
    AstraSettingItem setting;
    setting.key = "notifications_enabled";
    setting.title = u"Enable notifications";
    setting.description = u"Allow websites to show notifications";
    setting.type = AstraSettingType::kBoolean;
    setting.section = kSectionNotifications;
    setting.icon_name = "bell";
    setting.search_tags = {u"notifications", u"alerts", u"popups",
                           u"permission", u"push"};
    setting.default_value = base::Value(true);
    setting.current_value = base::Value(true);
    settings_[setting.key] = setting;
    sections_[kSectionNotifications].setting_keys.push_back(setting.key);
  }

  // Do not disturb mode.
  {
    AstraSettingItem setting;
    setting.key = "do_not_disturb";
    setting.title = u"Do not disturb";
    setting.description = u"Silence all notifications";
    setting.type = AstraSettingType::kBoolean;
    setting.section = kSectionNotifications;
    setting.icon_name = "dnd";
    setting.search_tags = {u"notifications", u"do not disturb", u"dnd",
                           u"silent", u"quiet", u"focus"};
    setting.default_value = base::Value(false);
    setting.current_value = base::Value(false);
    settings_[setting.key] = setting;
    sections_[kSectionNotifications].setting_keys.push_back(setting.key);
  }

  // -- Advanced settings -------------------------------------------------

  // Experimental features.
  {
    AstraSettingItem setting;
    setting.key = "experimental_features";
    setting.title = u"Enable experimental features";
    setting.description = u"Try out new features that are still in development";
    setting.type = AstraSettingType::kBoolean;
    setting.section = kSectionAdvanced;
    setting.icon_name = "flask";
    setting.search_tags = {u"advanced", u"experimental", u"features",
                           u"beta", u"flags", u"developer"};
    setting.default_value = base::Value(false);
    setting.current_value = base::Value(false);
    settings_[setting.key] = setting;
    sections_[kSectionAdvanced].setting_keys.push_back(setting.key);
  }

  // DevTools Astra panel.
  {
    AstraSettingItem setting;
    setting.key = "devtools_astra_panel";
    setting.title = u"Show Astra panel in DevTools";
    setting.description = u"Display the Astra workspace panel in DevTools";
    setting.type = AstraSettingType::kBoolean;
    setting.section = kSectionAdvanced;
    setting.icon_name = "devtools";
    setting.search_tags = {u"devtools", u"developer", u"panel",
                           u"debug", u"inspector", u"tools"};
    setting.default_value = base::Value(false);
    setting.current_value = base::Value(false);
    settings_[setting.key] = setting;
    sections_[kSectionAdvanced].setting_keys.push_back(setting.key);
  }

  // -- Initialize section expansion state -------------------------------

  for (const auto& [id, section] : sections_) {
    section_expanded_[id] = true;
  }
}

// =========================================================================
// Setting accessors
// =========================================================================

const AstraSettingItem* AstraSettingsModel::GetSetting(
    const std::string& key) const {
  auto it = settings_.find(key);
  if (it == settings_.end()) {
    return nullptr;
  }
  return &it->second;
}

std::vector<const AstraSettingItem*> AstraSettingsModel::GetAllSettings()
    const {
  std::vector<const AstraSettingItem*> result;
  result.reserve(settings_.size());
  for (const auto& [key, setting] : settings_) {
    result.push_back(&setting);
  }
  return result;
}

std::vector<const AstraSettingItem*> AstraSettingsModel::GetSettingsBySection(
    const std::string& section_id) const {
  std::vector<const AstraSettingItem*> result;
  auto section_it = sections_.find(section_id);
  if (section_it == sections_.end()) {
    return result;
  }
  for (const auto& key : section_it->second.setting_keys) {
    auto setting_it = settings_.find(key);
    if (setting_it != settings_.end()) {
      result.push_back(&setting_it->second);
    }
  }
  return result;
}

size_t AstraSettingsModel::GetSettingCount() const {
  return settings_.size();
}

// =========================================================================
// Section accessors
// =========================================================================

const AstraSettingsSectionInfo* AstraSettingsModel::GetSection(
    const std::string& section_id) const {
  auto it = sections_.find(section_id);
  if (it == sections_.end()) {
    return nullptr;
  }
  return &it->second;
}

std::vector<const AstraSettingsSectionInfo*>
AstraSettingsModel::GetAllSections() const {
  std::vector<const AstraSettingsSectionInfo*> result;
  result.reserve(sections_.size());
  for (const auto& [id, section] : sections_) {
    result.push_back(&section);
  }
  return result;
}

size_t AstraSettingsModel::GetSectionCount() const {
  return sections_.size();
}

// =========================================================================
// Search
// =========================================================================

std::vector<std::string> AstraSettingsModel::SearchSettings(
    const std::u16string& query) const {
  std::vector<std::string> result;
  if (query.empty()) {
    // Empty query returns all setting keys.
    for (const auto& [key, setting] : settings_) {
      result.push_back(key);
    }
    return result;
  }

  for (const auto& [key, setting] : settings_) {
    // Match title.
    if (MatchesText(setting.title, query)) {
      result.push_back(key);
      continue;
    }
    // Match description.
    if (MatchesText(setting.description, query)) {
      result.push_back(key);
      continue;
    }
    // Match search tags.
    bool tag_match = false;
    for (const auto& tag : setting.search_tags) {
      if (MatchesText(tag, query)) {
        tag_match = true;
        break;
      }
    }
    if (tag_match) {
      result.push_back(key);
    }
  }

  return result;
}

// =========================================================================
// Setting value mutation
// =========================================================================

bool AstraSettingsModel::SetSettingValue(const std::string& key,
                                          const base::Value& value) {
  AstraSettingItem* setting = FindSetting(key);
  if (!setting) {
    return false;
  }

  // Managed settings cannot be changed.
  if (setting->is_managed) {
    return false;
  }

  // Check if value is actually changing.
  if (setting->current_value == value) {
    return false;
  }

  setting->current_value = value.Clone();

  // TODO(astra): Wire to PrefService for persistence.
  // Chromium owner: PrefService (components/prefs/pref_service.h)
  // Patch point: When integrating with PrefService, write the value here
  //   via pref_service_->Set* methods.

  NotifySettingChanged(key);
  return true;
}

bool AstraSettingsModel::ResetSetting(const std::string& key) {
  AstraSettingItem* setting = FindSetting(key);
  if (!setting) {
    return false;
  }

  // Managed settings cannot be reset.
  if (setting->is_managed) {
    return false;
  }

  if (setting->current_value == setting->default_value) {
    return false;
  }

  setting->current_value = setting->default_value.Clone();
  NotifySettingChanged(key);
  return true;
}

void AstraSettingsModel::ResetAllSettings() {
  bool any_changed = false;
  for (auto& [key, setting] : settings_) {
    if (setting.is_managed) {
      continue;
    }
    if (setting.current_value != setting.default_value) {
      setting.current_value = setting.default_value.Clone();
      any_changed = true;
    }
  }

  if (any_changed) {
    NotifySettingsReset();
  }
}

// =========================================================================
// Managed / default helpers
// =========================================================================

bool AstraSettingsModel::IsSettingManaged(const std::string& key) const {
  const AstraSettingItem* setting = GetSetting(key);
  if (!setting) {
    return false;
  }
  return setting->is_managed;
}

base::Value AstraSettingsModel::GetDefaultValue(
    const std::string& key) const {
  const AstraSettingItem* setting = GetSetting(key);
  if (!setting) {
    return base::Value();
  }
  return setting->default_value.Clone();
}

// =========================================================================
// Presentation state
// =========================================================================

bool AstraSettingsModel::IsSectionExpanded(
    const std::string& section_id) const {
  auto it = section_expanded_.find(section_id);
  if (it == section_expanded_.end()) {
    return false;
  }
  return it->second;
}

void AstraSettingsModel::SetSectionExpanded(const std::string& section_id,
                                             bool expanded) {
  auto it = section_expanded_.find(section_id);
  if (it == section_expanded_.end()) {
    return;
  }
  if (it->second == expanded) {
    return;
  }
  it->second = expanded;
  // Section expansion is presentation state — no model notification needed.
  // Views observe this through the model directly or via section-specific
  // observer events.
}

void AstraSettingsModel::ToggleSectionExpanded(
    const std::string& section_id) {
  SetSectionExpanded(section_id, !IsSectionExpanded(section_id));
}

void AstraSettingsModel::ExpandAllSections() {
  for (auto& [id, expanded] : section_expanded_) {
    expanded = true;
  }
}

void AstraSettingsModel::CollapseAllSections() {
  for (auto& [id, expanded] : section_expanded_) {
    expanded = false;
  }
}

void AstraSettingsModel::SetSearchQuery(const std::u16string& query) {
  if (search_query_ == query) {
    return;
  }
  search_query_ = query;
  NotifySearchResultsChanged();
}

// =========================================================================
// Static section metadata helpers
// =========================================================================

std::u16string AstraSettingsModel::GetSectionTitle(
    const std::string& section_id) {
  if (section_id == kSectionAppearance) return u"Appearance";
  if (section_id == kSectionWorkspaces) return u"Workspaces";
  if (section_id == kSectionSidebar) return u"Sidebar";
  if (section_id == kSectionTabs) return u"Tabs";
  if (section_id == kSectionPrivacySecurity) return u"Privacy & Security";
  if (section_id == kSectionSearch) return u"Search";
  if (section_id == kSectionAccessibility) return u"Accessibility";
  if (section_id == kSectionPerformance) return u"Performance";
  if (section_id == kSectionNotifications) return u"Notifications";
  if (section_id == kSectionAdvanced) return u"Advanced";
  return u"";
}

std::u16string AstraSettingsModel::GetSectionDescription(
    const std::string& section_id) {
  if (section_id == kSectionAppearance)
    return u"Theme, color, and visual style settings";
  if (section_id == kSectionWorkspaces)
    return u"Workspace organization and behavior";
  if (section_id == kSectionSidebar)
    return u"Sidebar visibility, position, and behavior";
  if (section_id == kSectionTabs)
    return u"Tab behavior, split view, and tab management";
  if (section_id == kSectionPrivacySecurity)
    return u"Tracking, cookies, and security controls";
  if (section_id == kSectionSearch)
    return u"Search engine and search suggestions";
  if (section_id == kSectionAccessibility)
    return u"Display and accessibility options";
  if (section_id == kSectionPerformance)
    return u"Memory saver and performance settings";
  if (section_id == kSectionNotifications)
    return u"Notification preferences and alerts";
  if (section_id == kSectionAdvanced)
    return u"Experimental features and developer settings";
  return u"";
}

std::vector<std::u16string> AstraSettingsModel::GetSectionKeywords(
    const std::string& section_id) {
  if (section_id == kSectionAppearance) {
    return {u"theme", u"color", u"accent", u"dark", u"light",
            u"density", u"font", u"size", u"appearance", u"visual", u"style"};
  }
  if (section_id == kSectionWorkspaces) {
    return {u"workspaces", u"spaces", u"tab groups", u"organize",
            u"new tab", u"indicator", u"workspace"};
  }
  if (section_id == kSectionSidebar) {
    return {u"sidebar", u"side", u"panel", u"dock", u"pin",
            u"width", u"hide", u"auto-hide", u"position", u"left", u"right"};
  }
  if (section_id == kSectionTabs) {
    return {u"tabs", u"tab", u"stacking", u"stack", u"split", u"split view",
            u"recently closed", u"recent tabs", u"hover", u"peek", u"preview"};
  }
  if (section_id == kSectionPrivacySecurity) {
    return {u"privacy", u"security", u"tracking", u"tracker", u"cookies",
            u"data", u"block", u"protection", u"safe browsing", u"safe"};
  }
  if (section_id == kSectionSearch) {
    return {u"search", u"engine", u"google", u"bing", u"duckduckgo",
            u"default search", u"url", u"keyword", u"suggestions"};
  }
  if (section_id == kSectionAccessibility) {
    return {u"accessibility", u"high contrast", u"reduced motion",
            u"font scale", u"font size", u"zoom", u"a11y",
            u"screen reader", u"visibility", u"large text"};
  }
  if (section_id == kSectionPerformance) {
    return {u"performance", u"memory", u"ram", u"speed", u"optimize",
            u"memory saver", u"hardware", u"acceleration", u"gpu"};
  }
  if (section_id == kSectionNotifications) {
    return {u"notifications", u"alerts", u"do not disturb", u"dnd",
            u"silent", u"push", u"permission"};
  }
  if (section_id == kSectionAdvanced) {
    return {u"advanced", u"experimental", u"developer", u"devtools",
            u"flags", u"about", u"chrome://"};
  }
  return {};
}

// =========================================================================
// Internal helpers
// =========================================================================

AstraSettingItem* AstraSettingsModel::FindSetting(const std::string& key) {
  auto it = settings_.find(key);
  if (it == settings_.end()) {
    return nullptr;
  }
  return &it->second;
}

// =========================================================================
// Notification helpers
// =========================================================================

void AstraSettingsModel::NotifySettingChanged(const std::string& key) {
  for (auto& observer : observers_) {
    observer.OnSettingChanged(this, key);
  }
}

void AstraSettingsModel::NotifySettingsReset() {
  for (auto& observer : observers_) {
    observer.OnSettingsReset(this);
  }
}

void AstraSettingsModel::NotifySearchResultsChanged() {
  for (auto& observer : observers_) {
    observer.OnSettingsSearchResultsChanged(this);
  }
}

void AstraSettingsModel::NotifyShutdown() {
  for (auto& observer : observers_) {
    observer.OnSettingsModelShutdown(this);
  }
}

}  // namespace astra
