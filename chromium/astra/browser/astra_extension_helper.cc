#include "astra/browser/astra_extension_helper.h"

#include <algorithm>
#include <vector>

#include "base/observer_list.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_action.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest_handlers/icons_handler.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "url/gurl.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Default extension icon size (favicon-sized for sidebar display).
constexpr int kDefaultIconSize = 16;

// Minimum and maximum for max sidebar extensions setting.
constexpr int kMinSidebarExtensions = 1;
constexpr int kMaxSidebarExtensionsLimit = 50;

// TODO(astra): Load real default extension icon from Chromium resources.
//   Chromium has IDR_EXTENSIONS_FAVICON in chrome/app/theme/chrome/ or
//   extensions/browser/icons. For now, return an empty ImageSkia.
// Chromium resource: IDR_EXTENSIONS_FAVICON
// Chromium owner: extension_icon_manager.h (chrome/browser/extensions/)
gfx::ImageSkia CreateDefaultIconSkia(int size) {
  // Return an empty image skia as a placeholder.
  // In the real implementation, this would load the default extension
  // puzzle piece icon from Chromium's resource bundle.
  return gfx::ImageSkia();
}

// Valid sort order values.
constexpr const char* kValidSortOrders[] = {
  "name",
  "install_date",
  "category",
};

bool IsValidSortOrder(const std::string& sort_order) {
  for (const char* valid : kValidSortOrders) {
    if (sort_order == valid) {
      return true;
    }
  }
  return false;
}

}  // namespace

// =========================================================================
// Constructor / destructor
// =========================================================================

AstraExtensionHelper::AstraExtensionHelper(Profile* profile)
    : profile_(profile) {
  // TODO(astra): Start observing ExtensionRegistry for live updates.
  //   extensions::ExtensionRegistry::Get(profile_)->AddObserver(this);
  // Requires implementing extensions::ExtensionRegistryObserver.
  StartObservingRegistry();
}

AstraExtensionHelper::~AstraExtensionHelper() {
  // Observers should already be cleaned up by Shutdown().
  DCHECK(!is_observing_registry_);
}

void AstraExtensionHelper::Shutdown() {
  StopObservingRegistry();
  profile_ = nullptr;
}

// =========================================================================
// Extension list queries
// =========================================================================

std::vector<AstraExtensionInfo>
AstraExtensionHelper::GetExtensionsWithBrowserActions() const {
  std::vector<AstraExtensionInfo> result;

  extensions::ExtensionRegistry* registry = GetRegistry();
  if (!registry) {
    return result;
  }

  // Iterate over enabled extensions and collect those with browser actions.
  //
  // Chromium's ExtensionRegistry has separate sets for different states:
  //   - enabled_extensions()
  //   - disabled_extensions()
  //   - terminated_extensions()
  //   - blacklisted_extensions()
  //
  // We only show enabled extensions in the sidebar.
  //
  // TODO(astra): Also consider page actions and the "action" manifest key
  //   (Manifest V3 unified action). The modern API uses "action" instead of
  //   "browser_action" / "page_action". We should handle all three.
  //   Chromium owner: ActionInfo (extensions/common/api/extension_action/action_info.h)
  //   Chromium owner: ExtensionActionManager
  //     (chrome/browser/extensions/extension_action_manager.h)
  const auto& extensions = registry->enabled_extensions();
  for (const auto& kv : extensions) {
    const extensions::Extension* extension = kv.second.get();
    if (!extension) {
      continue;
    }

    // Check if the extension has a browser action or action (Manifest V3).
    // TODO(astra): Use ExtensionActionManager to properly detect action
    //   presence for both Manifest V2 (browser_action) and V3 (action).
    //   ExtensionActionManager::GetExtensionAction() returns the action
    //   regardless of manifest version.
    bool has_browser_action = extension->manifest()->HasKey("browser_action") ||
                              extension->manifest()->HasKey("action");
    bool has_page_action = extension->manifest()->HasKey("page_action");

    if (!has_browser_action && !has_page_action) {
      continue;
    }

    AstraExtensionInfo info = CreateExtensionInfo(extension);
    info.enabled = true;
    info.has_browser_action = has_browser_action;
    info.has_page_action = has_page_action;

    result.push_back(info);
  }

  // TODO(astra): Sort extensions in a consistent order (e.g. by name or
  //   by installation order). Chromium's toolbar action order is managed
  //   by ToolbarActionsModel / ExtensionActionToolbarController.
  // Chromium owner: ToolbarActionsModel
  //   (chrome/browser/ui/toolbar/toolbar_actions_model.h)
  std::sort(result.begin(), result.end(),
            [](const AstraExtensionInfo& a, const AstraExtensionInfo& b) {
              return a.name < b.name;
            });

  return result;
}

bool AstraExtensionHelper::HasBrowserAction(
    const std::string& extension_id) const {
  const extensions::Extension* extension = GetExtension(extension_id);
  if (!extension) {
    return false;
  }
  return extension->manifest()->HasKey("browser_action") ||
         extension->manifest()->HasKey("action");
}

size_t AstraExtensionHelper::GetInstalledExtensionsCount() const {
  extensions::ExtensionRegistry* registry = GetRegistry();
  if (!registry) {
    return 0;
  }
  // Total installed = enabled + disabled + terminated + blacklisted.
  return registry->enabled_extensions().size() +
         registry->disabled_extensions().size() +
         registry->terminated_extensions().size() +
         registry->blacklisted_extensions().size();
}

size_t AstraExtensionHelper::GetEnabledExtensionsCount() const {
  extensions::ExtensionRegistry* registry = GetRegistry();
  if (!registry) {
    return 0;
  }
  return registry->enabled_extensions().size();
}

size_t AstraExtensionHelper::GetDisabledExtensionsCount() const {
  extensions::ExtensionRegistry* registry = GetRegistry();
  if (!registry) {
    return 0;
  }
  return registry->disabled_extensions().size();
}

AstraExtensionInfo AstraExtensionHelper::GetExtensionById(
    const std::string& extension_id) const {
  const extensions::Extension* extension = GetExtensionAnyState(extension_id);
  if (!extension) {
    return AstraExtensionInfo();
  }
  return CreateExtensionInfo(extension);
}

std::vector<AstraExtensionInfo> AstraExtensionHelper::GetExtensionByName(
    const std::u16string& query,
    size_t max_count) const {
  std::vector<AstraExtensionInfo> results;

  extensions::ExtensionRegistry* registry = GetRegistry();
  if (!registry || query.empty()) {
    return results;
  }

  std::u16 query_lower = base::ToLowerASCII(query);

  // Search across all installed extensions.
  auto search_set = [&](const extensions::ExtensionSet& set) {
    for (const auto& kv : set) {
      const extensions::Extension* extension = kv.second.get();
      if (!extension) {
        continue;
      }

      std::u16string name = base::UTF8ToUTF16(extension->name());
      std::u16string name_lower = base::ToLowerASCII(name);

      if (name_lower.find(query_lower) != std::u16string::npos) {
        results.push_back(CreateExtensionInfo(extension));
        if (max_count > 0 && results.size() >= max_count) {
          return true;  // Stop searching.
        }
      }
    }
    return false;
  };

  // Search enabled first (more relevant), then disabled.
  if (search_set(registry->enabled_extensions())) {
    return results;
  }
  search_set(registry->disabled_extensions());

  // Sort by name.
  std::sort(results.begin(), results.end(),
            [](const AstraExtensionInfo& a, const AstraExtensionInfo& b) {
              return a.name < b.name;
            });

  return results;
}

std::vector<AstraExtensionInfo> AstraExtensionHelper::GetAllExtensions(
    bool include_disabled,
    size_t max_count) const {
  std::vector<AstraExtensionInfo> results;

  extensions::ExtensionRegistry* registry = GetRegistry();
  if (!registry) {
    return results;
  }

  auto add_from_set = [&](const extensions::ExtensionSet& set, bool enabled) {
    for (const auto& kv : set) {
      const extensions::Extension* extension = kv.second.get();
      if (!extension) {
        continue;
      }
      AstraExtensionInfo info = CreateExtensionInfo(extension);
      info.enabled = enabled;
      results.push_back(info);
      if (max_count > 0 && results.size() >= max_count) {
        return true;
      }
    }
    return false;
  };

  if (add_from_set(registry->enabled_extensions(), true)) {
    return results;
  }
  if (include_disabled) {
    add_from_set(registry->disabled_extensions(), false);
  }

  // Sort by name.
  std::sort(results.begin(), results.end(),
            [](const AstraExtensionInfo& a, const AstraExtensionInfo& b) {
              return a.name < b.name;
            });

  return results;
}

std::vector<AstraExtensionInfo> AstraExtensionHelper::GetExtensionsByCategory(
    AstraExtensionCategory category) const {
  std::vector<AstraExtensionInfo> results;

  extensions::ExtensionRegistry* registry = GetRegistry();
  if (!registry) {
    return results;
  }

  const auto& extensions = registry->enabled_extensions();
  for (const auto& kv : extensions) {
    const extensions::Extension* extension = kv.second.get();
    if (!extension) {
      continue;
    }
    AstraExtensionCategory ext_category = ClassifyExtension(extension);
    if (ext_category == category) {
      results.push_back(CreateExtensionInfo(extension));
    }
  }

  std::sort(results.begin(), results.end(),
            [](const AstraExtensionInfo& a, const AstraExtensionInfo& b) {
              return a.name < b.name;
            });

  return results;
}

size_t AstraExtensionHelper::GetExtensionCountByCategory(
    AstraExtensionCategory category) const {
  size_t count = 0;

  extensions::ExtensionRegistry* registry = GetRegistry();
  if (!registry) {
    return 0;
  }

  const auto& extensions = registry->enabled_extensions();
  for (const auto& kv : extensions) {
    const extensions::Extension* extension = kv.second.get();
    if (!extension) {
      continue;
    }
    if (ClassifyExtension(extension) == category) {
      count++;
    }
  }

  return count;
}

// =========================================================================
// Extension stats
// =========================================================================

AstraExtensionStats AstraExtensionHelper::GetExtensionStats() const {
  AstraExtensionStats stats;

  extensions::ExtensionRegistry* registry = GetRegistry();
  if (!registry) {
    return stats;
  }

  stats.total_installed = GetInstalledExtensionsCount();
  stats.enabled_count = GetEnabledExtensionsCount();
  stats.disabled_count = GetDisabledExtensionsCount();

  // Count extensions with browser actions and themes.
  const auto& enabled = registry->enabled_extensions();
  for (const auto& kv : enabled) {
    const extensions::Extension* extension = kv.second.get();
    if (!extension) {
      continue;
    }

    if (extension->manifest()->HasKey("browser_action") ||
        extension->manifest()->HasKey("action")) {
      stats.with_browser_action_count++;
    }
    if (extension->manifest()->HasKey("page_action")) {
      stats.with_page_action_count++;
    }

    // Check if it's a theme.
    // TODO(astra): Use Manifest::IsTheme() or Manifest::TYPE_THEME.
    //   For now, check for "theme" key in manifest.
    if (extension->manifest()->HasKey("theme")) {
      stats.theme_count++;
    }
  }

  return stats;
}

// =========================================================================
// Recommended extensions
// =========================================================================

std::vector<AstraExtensionInfo>
AstraExtensionHelper::GetRecommendedExtensions() const {
  // Curated list of recommended extensions for Astra users.
  // These are productivity and workflow-enhancing extensions that
  // complement Astra's workspace and focus features.
  //
  // TODO(astra): In production, this list would come from a server-side
  //   configuration or be tied to specific Astra features. For the overlay
  //   skeleton, we return a static list of extension IDs.
  //
  // Note: These are Astra-level recommendations. The actual extensions
  // may or may not be installed — the UI should show install buttons for
  // non-installed ones.

  // Since we can't reliably check if recommended extensions are installed
  // without web store integration, we return a static list of placeholder
  // recommendations that the UI can project.
  std::vector<AstraExtensionInfo> recommended;

  // Productivity / workspace recommendations.
  {
    AstraExtensionInfo info;
    info.extension_id = "astra.recommended.tab-suspender";
    info.name = u"Tab Suspender";
    info.description = u"Automatically suspend inactive tabs to save memory";
    info.category = AstraExtensionCategory::kProductivityTab;
    info.enabled = false;  // Not marked as installed/enabled.
    info.has_browser_action = true;
    recommended.push_back(info);
  }

  {
    AstraExtensionInfo info;
    info.extension_id = "astra.recommended.ad-blocker";
    info.name = u"Ad Blocker";
    info.description = u"Block ads and trackers for faster browsing";
    info.category = AstraExtensionCategory::kPrivacySecurity;
    info.enabled = false;
    info.has_browser_action = true;
    recommended.push_back(info);
  }

  {
    AstraExtensionInfo info;
    info.extension_id = "astra.recommended.password-manager";
    info.name = u"Password Manager";
    info.description = u"Secure password management with auto-fill";
    info.category = AstraExtensionCategory::kPrivacySecurity;
    info.enabled = false;
    info.has_browser_action = true;
    recommended.push_back(info);
  }

  {
    AstraExtensionInfo info;
    info.extension_id = "astra.recommended.reader-mode";
    info.name = u"Reader Mode";
    info.description = u"Distraction-free reading view";
    info.category = AstraExtensionCategory::kProductivity;
    info.enabled = false;
    info.has_browser_action = true;
    recommended.push_back(info);
  }

  {
    AstraExtensionInfo info;
    info.extension_id = "astra.recommended.devtools-theme";
    info.name = u"DevTools Theme";
    info.description = u"Custom themes for Chrome DevTools";
    info.category = AstraExtensionCategory::kDeveloperTools;
    info.enabled = false;
    info.has_browser_action = true;
    recommended.push_back(info);
  }

  return recommended;
}

size_t AstraExtensionHelper::GetRecommendedExtensionsCount() const {
  return GetRecommendedExtensions().size();
}

bool AstraExtensionHelper::IsRecommendedExtension(
    const std::string& extension_id) const {
  auto recommended = GetRecommendedExtensions();
  for (const auto& ext : recommended) {
    if (ext.extension_id == extension_id) {
      return true;
    }
  }
  return false;
}

// =========================================================================
// Categories
// =========================================================================

std::vector<AstraExtensionCategory>
AstraExtensionHelper::GetAvailableCategories() const {
  std::vector<AstraExtensionCategory> available;

  extensions::ExtensionRegistry* registry = GetRegistry();
  if (!registry) {
    return available;
  }

  // Check which categories have at least one extension.
  auto all_categories = {
    AstraExtensionCategory::kProductivity,
    AstraExtensionCategory::kAccessibility,
    AstraExtensionCategory::kDeveloperTools,
    AstraExtensionCategory::kEntertainment,
    AstraExtensionCategory::kCommunication,
    AstraExtensionCategory::kNewsWeather,
    AstraExtensionCategory::kPrivacySecurity,
    AstraExtensionCategory::kPhotos,
    AstraExtensionCategory::kProductivityTab,
    AstraExtensionCategory::kThemes,
    AstraExtensionCategory::kOther,
  };

  for (auto category : all_categories) {
    if (GetExtensionCountByCategory(category) > 0) {
      available.push_back(category);
    }
  }

  return available;
}

std::u16string AstraExtensionHelper::GetCategoryLabel(
    AstraExtensionCategory category) {
  switch (category) {
    case AstraExtensionCategory::kProductivity:
      return u"Productivity";
    case AstraExtensionCategory::kAccessibility:
      return u"Accessibility";
    case AstraExtensionCategory::kDeveloperTools:
      return u"Developer Tools";
    case AstraExtensionCategory::kEntertainment:
      return u"Entertainment";
    case AstraExtensionCategory::kCommunication:
      return u"Communication";
    case AstraExtensionCategory::kNewsWeather:
      return u"News & Weather";
    case AstraExtensionCategory::kPrivacySecurity:
      return u"Privacy & Security";
    case AstraExtensionCategory::kPhotos:
      return u"Photos";
    case AstraExtensionCategory::kProductivityTab:
      return u"Tab Management";
    case AstraExtensionCategory::kThemes:
      return u"Themes";
    case AstraExtensionCategory::kOther:
      return u"Other";
  }
  return u"Other";
}

AstraExtensionCategory AstraExtensionHelper::ClassifyExtension(
    const extensions::Extension* extension) {
  if (!extension) {
    return AstraExtensionCategory::kOther;
  }

  // Check for themes first.
  if (extension->manifest()->HasKey("theme")) {
    return AstraExtensionCategory::kThemes;
  }

  // Check for DevTools extensions.
  if (extension->manifest()->HasKey("devtools_page")) {
    return AstraExtensionCategory::kDeveloperTools;
  }

  // Check for accessibility extensions.
  if (extension->manifest()->HasKey("accessibility")) {
    return AstraExtensionCategory::kAccessibility;
  }

  // Check for tab management extensions (has tabs permission and browser action).
  // TODO(astra): Use proper permission checking. For now, heuristic based on
  //   manifest keys.
  // Chromium owner: extensions::PermissionSet (extensions/common/permissions/)
  if (extension->manifest()->HasKey("tabs") &&
      extension->manifest()->HasKey("browser_action")) {
    return AstraExtensionCategory::kProductivityTab;
  }

  // TODO(astra): Implement more sophisticated classification using:
  //   - Extension type (Manifest::TYPE_EXTENSION, TYPE_THEME, etc.)
  //   - Permissions (tabs, bookmarks, etc.)
  //   - Web store category metadata (from web store installation)
  //
  // For the overlay skeleton, default to productivity if it has a browser
  // action, otherwise other.
  if (extension->manifest()->HasKey("browser_action") ||
      extension->manifest()->HasKey("action")) {
    return AstraExtensionCategory::kProductivity;
  }

  return AstraExtensionCategory::kOther;
}

// =========================================================================
// Icon queries
// =========================================================================

gfx::Image AstraExtensionHelper::GetExtensionIcon(
    const std::string& extension_id, int size) const {
  const extensions::Extension* extension = GetExtension(extension_id);
  if (!extension) {
    return gfx::Image(GetDefaultExtensionIcon(size));
  }

  // TODO(astra): Load the real extension icon asynchronously.
  //   Use extensions::ImageLoader or ExtensionIconManager to load icons
  //   at the requested size. For now, return the default icon.
  //
  //   The proper flow:
  //     1. Check if we have a cached icon for this extension + size.
  //     2. If not, trigger an async load via ImageLoader.
  //     3. When load completes, notify observers via OnExtensionIconChanged.
  //     4. Return the default icon synchronously as a placeholder.
  //
  // Chromium owner: ImageLoader (extensions/browser/image_loader.h)
  // Chromium owner: ExtensionIconManager
  //   (chrome/browser/extensions/extension_icon_manager.h)
  return gfx::Image(GetDefaultExtensionIcon(size));
}

gfx::ImageSkia AstraExtensionHelper::GetDefaultExtensionIcon(int size) const {
  // TODO(astra): Load from Chromium's resource bundle (IDR_EXTENSIONS_FAVICON).
  //   For now, return an empty placeholder image.
  return CreateDefaultIconSkia(size);
}

// =========================================================================
// Popup URL
// =========================================================================

GURL AstraExtensionHelper::GetPopupURL(const std::string& extension_id) const {
  const extensions::Extension* extension = GetExtension(extension_id);
  if (!extension) {
    return GURL();
  }

  // TODO(astra): Properly extract the popup URL from the extension action.
  //   Use ExtensionActionManager::GetExtensionAction() then
  //   ExtensionAction::GetPopupUrl(web_contents) to get the popup URL.
  //   For now, return an empty URL.
  //
  // Chromium owner: ExtensionActionManager
  //   (chrome/browser/extensions/extension_action_manager.h)
  // Chromium owner: ExtensionAction (extensions/browser/extension_action.h)
  return GURL();
}

// =========================================================================
// Presentation settings — show extension toolbar
// =========================================================================

bool AstraExtensionHelper::GetShowExtensionToolbar() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultExtensionShowToolbar;
  }
  return prefs->GetBoolean(prefs::kPrefExtensionShowToolbar);
}

void AstraExtensionHelper::SetShowExtensionToolbar(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefExtensionShowToolbar) == show) {
    return;  // No change.
  }

  prefs->SetBoolean(prefs::kPrefExtensionShowToolbar, show);
  NotifyExtensionSettingsChanged();
}

bool AstraExtensionHelper::ToggleShowExtensionToolbar() {
  bool new_value = !GetShowExtensionToolbar();
  SetShowExtensionToolbar(new_value);
  return new_value;
}

// =========================================================================
// Presentation settings — show extensions in sidebar
// =========================================================================

bool AstraExtensionHelper::GetShowExtensionsInSidebar() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultExtensionShowInSidebar;
  }
  return prefs->GetBoolean(prefs::kPrefExtensionShowInSidebar);
}

void AstraExtensionHelper::SetShowExtensionsInSidebar(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefExtensionShowInSidebar) == show) {
    return;  // No change.
  }

  prefs->SetBoolean(prefs::kPrefExtensionShowInSidebar, show);
  NotifyExtensionSettingsChanged();
}

bool AstraExtensionHelper::ToggleShowExtensionsInSidebar() {
  bool new_value = !GetShowExtensionsInSidebar();
  SetShowExtensionsInSidebar(new_value);
  return new_value;
}

// =========================================================================
// Presentation settings — extension manager shortcut
// =========================================================================

bool AstraExtensionHelper::GetExtensionManagerShortcut() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultExtensionManagerShortcut;
  }
  return prefs->GetBoolean(prefs::kPrefExtensionManagerShortcut);
}

void AstraExtensionHelper::SetExtensionManagerShortcut(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefExtensionManagerShortcut) == show) {
    return;  // No change.
  }

  prefs->SetBoolean(prefs::kPrefExtensionManagerShortcut, show);
  NotifyExtensionSettingsChanged();
}

bool AstraExtensionHelper::ToggleExtensionManagerShortcut() {
  bool new_value = !GetExtensionManagerShortcut();
  SetExtensionManagerShortcut(new_value);
  return new_value;
}

// =========================================================================
// Presentation settings — show recommended extensions
// =========================================================================

bool AstraExtensionHelper::GetShowRecommendedExtensions() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultExtensionShowRecommended;
  }
  return prefs->GetBoolean(prefs::kPrefExtensionShowRecommended);
}

void AstraExtensionHelper::SetShowRecommendedExtensions(bool show) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (prefs->GetBoolean(prefs::kPrefExtensionShowRecommended) == show) {
    return;  // No change.
  }

  prefs->SetBoolean(prefs::kPrefExtensionShowRecommended, show);
  NotifyExtensionSettingsChanged();
}

bool AstraExtensionHelper::ToggleShowRecommendedExtensions() {
  bool new_value = !GetShowRecommendedExtensions();
  SetShowRecommendedExtensions(new_value);
  return new_value;
}

// =========================================================================
// Presentation settings — max sidebar extensions
// =========================================================================

int AstraExtensionHelper::GetMaxSidebarExtensions() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultExtensionMaxSidebarCount;
  }
  return prefs->GetInteger(prefs::kPrefExtensionMaxSidebarCount);
}

void AstraExtensionHelper::SetMaxSidebarExtensions(int max_count) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  // Clamp to valid range.
  int clamped = std::max(kMinSidebarExtensions,
                         std::min(max_count, kMaxSidebarExtensionsLimit));

  if (prefs->GetInteger(prefs::kPrefExtensionMaxSidebarCount) == clamped) {
    return;  // No change.
  }

  prefs->SetInteger(prefs::kPrefExtensionMaxSidebarCount, clamped);
  NotifyExtensionSettingsChanged();
}

// =========================================================================
// Presentation settings — extension sort order
// =========================================================================

std::string AstraExtensionHelper::GetExtensionSortOrder() const {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return prefs::kDefaultExtensionSortOrder;
  }
  return prefs->GetString(prefs::kPrefExtensionSortOrder);
}

void AstraExtensionHelper::SetExtensionSortOrder(const std::string& sort_order) {
  PrefService* prefs = GetPrefs();
  if (!prefs) {
    return;
  }

  if (!IsValidSortOrder(sort_order)) {
    return;  // Invalid value, ignore.
  }

  if (prefs->GetString(prefs::kPrefExtensionSortOrder) == sort_order) {
    return;  // No change.
  }

  prefs->SetString(prefs::kPrefExtensionSortOrder, sort_order);
  NotifyExtensionSettingsChanged();
}

// =========================================================================
// Extension management helpers
// =========================================================================

void AstraExtensionHelper::OpenExtensionSettings(Profile* profile) const {
  // TODO(astra): Implement proper navigation to chrome://extensions.
  //   Use chrome::NavigateParams or similar mechanism to open the
  //   extensions management page.
  //
  // Chromium owner: chrome::OpenSettings (chrome/browser/ui/settings/...)
  //   or chrome/browser/ui/browser_navigator.h
  if (!profile) {
    return;
  }
  // Stub: In the overlay, this is a no-op.
}

void AstraExtensionHelper::OpenChromeWebStore(Profile* profile) const {
  // TODO(astra): Implement navigation to Chrome Web Store.
  //   This would open a new tab with the web store URL.
  if (!profile) {
    return;
  }
  // Stub: In the overlay, this is a no-op.
}

void AstraExtensionHelper::OpenExtensionDetails(
    Profile* profile,
    const std::string& extension_id) const {
  // TODO(astra): Implement navigation to extension detail page.
  //   The extensions page supports deep-linking to specific extensions.
  if (!profile || extension_id.empty()) {
    return;
  }
  // Stub: In the overlay, this is a no-op.
}

bool AstraExtensionHelper::IsExtensionManagementAvailable() const {
  // Extension management is always available in Chromium.
  // The management page (chrome://extensions) is a built-in WebUI.
  return true;
}

// =========================================================================
// Bulk operations
// =========================================================================

bool AstraExtensionHelper::IsBulkOperationAvailable() const {
  // TODO(astra): Check if bulk operations are allowed by policy.
  //   Some enterprise policies may restrict enable-all/disable-all.
  //   For the overlay skeleton, bulk operations are always available.
  //
  // Chromium owner: ExtensionService (chrome/browser/extensions/extension_service.h)
  return true;
}

// =========================================================================
// Observers
// =========================================================================

void AstraExtensionHelper::AddObserver(
    AstraExtensionHelperObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraExtensionHelper::RemoveObserver(
    AstraExtensionHelperObserver* observer) {
  observers_.RemoveObserver(observer);
}

// =========================================================================
// Notification helpers
// =========================================================================

void AstraExtensionHelper::NotifyExtensionsChanged() {
  for (auto& observer : observers_) {
    observer.OnExtensionsChanged();
  }
}

void AstraExtensionHelper::NotifyExtensionIconChanged(
    const std::string& extension_id) {
  for (auto& observer : observers_) {
    observer.OnExtensionIconChanged(extension_id);
  }
}

void AstraExtensionHelper::NotifyExtensionInstalled(
    const AstraExtensionInfo& extension) {
  for (auto& observer : observers_) {
    observer.OnExtensionInstalled(extension);
  }
  // Also fire the general changed notification.
  NotifyExtensionsChanged();
}

void AstraExtensionHelper::NotifyExtensionUninstalled(
    const std::string& extension_id,
    const std::u16string& extension_name) {
  for (auto& observer : observers_) {
    observer.OnExtensionUninstalled(extension_id, extension_name);
  }
  // Also fire the general changed notification.
  NotifyExtensionsChanged();
}

void AstraExtensionHelper::NotifyExtensionEnabled(
    const std::string& extension_id) {
  for (auto& observer : observers_) {
    observer.OnExtensionEnabled(extension_id);
  }
  // Also fire the general changed notification.
  NotifyExtensionsChanged();
}

void AstraExtensionHelper::NotifyExtensionDisabled(
    const std::string& extension_id) {
  for (auto& observer : observers_) {
    observer.OnExtensionDisabled(extension_id);
  }
  // Also fire the general changed notification.
  NotifyExtensionsChanged();
}

void AstraExtensionHelper::NotifyExtensionSettingsChanged() {
  for (auto& observer : observers_) {
    observer.OnExtensionSettingsChanged();
  }
}

// =========================================================================
// Registry observation
// =========================================================================

void AstraExtensionHelper::StartObservingRegistry() {
  // TODO(astra): Implement proper ExtensionRegistry observation.
  //   extensions::ExtensionRegistry* registry = GetRegistry();
  //   if (registry) {
  //     registry->AddObserver(this);
  //     is_observing_registry_ = true;
  //   }
  //
  // Required observer methods:
  //   - OnExtensionLoaded(content::BrowserContext*, const Extension*)
  //   - OnExtensionUnloaded(content::BrowserContext*, const Extension*,
  //                         UnloadedExtensionReason reason)
  //   - OnExtensionInstalled(content::BrowserContext*, const Extension*,
  //                          bool is_update, const base::FilePath& old_path)
  //   - OnExtensionUninstalled(content::BrowserContext*, const Extension*,
  //                            UninstallReason reason)
  //   - OnShutdown(extensions::ExtensionRegistry*)
  //
  // Each of these should trigger the appropriate notification to
  // our observers so the sidebar can update its projection.
  is_observing_registry_ = false;
}

void AstraExtensionHelper::StopObservingRegistry() {
  if (!is_observing_registry_) {
    return;
  }

  // TODO(astra): Remove observer from ExtensionRegistry.
  //   extensions::ExtensionRegistry* registry = GetRegistry();
  //   if (registry) {
  //     registry->RemoveObserver(this);
  //   }
  is_observing_registry_ = false;
}

extensions::ExtensionRegistry* AstraExtensionHelper::GetRegistry() const {
  if (!profile_) {
    return nullptr;
  }
  return extensions::ExtensionRegistry::Get(profile_);
}

PrefService* AstraExtensionHelper::GetPrefs() const {
  if (!profile_) {
    return nullptr;
  }
  return profile_->GetPrefs();
}

const extensions::Extension* AstraExtensionHelper::GetExtension(
    const std::string& id) const {
  extensions::ExtensionRegistry* registry = GetRegistry();
  if (!registry) {
    return nullptr;
  }
  return registry->enabled_extensions().GetByID(id);
}

const extensions::Extension* AstraExtensionHelper::GetExtensionAnyState(
    const std::string& id) const {
  extensions::ExtensionRegistry* registry = GetRegistry();
  if (!registry) {
    return nullptr;
  }

  // Check enabled first.
  const extensions::Extension* ext = registry->enabled_extensions().GetByID(id);
  if (ext) return ext;

  // Check disabled.
  ext = registry->disabled_extensions().GetByID(id);
  if (ext) return ext;

  // Check terminated.
  ext = registry->terminated_extensions().GetByID(id);
  if (ext) return ext;

  // Check blacklisted.
  ext = registry->blacklisted_extensions().GetByID(id);
  return ext;
}

AstraExtensionInfo AstraExtensionHelper::CreateExtensionInfo(
    const extensions::Extension* extension) const {
  AstraExtensionInfo info;
  if (!extension) {
    return info;
  }

  info.extension_id = extension->id();
  info.name = base::UTF8ToUTF16(extension->name());
  info.description = base::UTF8ToUTF16(extension->description());
  info.version = extension->VersionString();
  info.enabled = true;  // Assume enabled unless caller overrides.
  info.has_browser_action = extension->manifest()->HasKey("browser_action") ||
                            extension->manifest()->HasKey("action");
  info.has_page_action = extension->manifest()->HasKey("page_action");
  info.has_options_page = extension->manifest()->HasKey("options_page") ||
                          extension->manifest()->HasKey("options_ui");
  info.category = ClassifyExtension(extension);

  // TODO(astra): Get the popup URL from the extension's action manifest.
  //   Use ExtensionAction::default_popup_url() or parse it from manifest.
  //   For now, leave it empty — the popup view will show a placeholder.
  // Chromium owner: ExtensionAction (extensions/browser/extension_action.h)
  //   -> GetPopupUrl(content::WebContents* web_contents)
  info.popup_url = std::string();

  // TODO(astra): Get the icon URL from the extension manifest.
  //   Use IconsInfo::GetIcons() or similar.
  // Chromium owner: extensions::IconsInfo (extensions/common/manifest_handlers/icons_handler.h)
  info.icon_url = std::string();

  return info;
}

}  // namespace astra
