#ifndef ASTRA_BROWSER_ASTRA_EXTENSION_HELPER_H_
#define ASTRA_BROWSER_ASTRA_EXTENSION_HELPER_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "components/keyed_service/core/keyed_service.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/image/image_skia.h"

class PrefService;
class Profile;

namespace extensions {
class Extension;
class ExtensionRegistry;
}  // namespace extensions

namespace gfx {
class Image;
class ImageSkia;
}  // namespace gfx

namespace astra {

// =========================================================================
// Extension category
// =========================================================================
//
// High-level categories used by the Astra UI to group extensions.
// These are projected categories — the actual categorization logic is
// owned by Chromium's extension system (see ExtensionSystem and
// extensions::Manifest::Type).  Astra defines its own categories for
// presentation purposes that map to Chromium's extension types and
// permissions.
//
// Chromium owner: extensions::Manifest::Type
//   (extensions/common/manifest.h)
// Chromium owner: ChromeWebStoreStoreCategory or extension categories
//   from the web store metadata.
enum class AstraExtensionCategory {
  kProductivity,     // Productivity, tools, utilities
  kAccessibility,    // Accessibility extensions
  kDeveloperTools,   // Developer tools, debuggers, DevTools extensions
  kEntertainment,    // Games, entertainment, media
  kCommunication,    // Chat, email, social
  kNewsWeather,      // News, weather, RSS
  kPrivacySecurity,  // Privacy, security, ad blockers
  kPhotos,           // Photos, image editing
  kProductivityTab,  // Tab management, workspaces
  kThemes,           // Themes, appearance
  kOther,            // Uncategorized / other
};

// =========================================================================
// AstraExtensionState — extension presentation state
// =========================================================================
//
// Presentation-facing state of an extension. Mirrors a subset of
// extensions::Extension::State and extensions::UnloadedExtensionReason.
// Used by the UI layer to determine visual styling (opacity, badges, etc.).
//
// Chromium owner: extensions::Extension::State
//   (extensions/common/extension.h)
// Chromium owner: extensions::UnloadedExtensionReason
//   (extensions/browser/unloaded_extension_reason.h)
enum class AstraExtensionState {
  kEnabled,      // Extension is enabled and functioning normally
  kDisabled,     // Extension is disabled by user or policy
  kBlocked,      // Extension is blocked (e.g., by safe browsing)
  kError,        // Extension has encountered an error
  kUninstalled,  // Extension has been uninstalled (transient state)
};

// =========================================================================
// AstraExtensionInfo — projected extension data
// =========================================================================
//
// Struct representing an extension with a browser action that should appear
// in the Astra sidebar extensions panel.
//
// This is a projection-only data structure — it mirrors a subset of
// Chromium's Extension and ExtensionAction state. The truth source is
// always Chromium's ExtensionRegistry and ExtensionAction system.
struct AstraExtensionInfo {
  std::string extension_id;     // Extension ID (from extensions::Extension)
  std::u16string name;          // Extension display name
  std::u16string description;   // Short description (from manifest)
  std::string version;          // Extension version string
  gfx::ImageSkia icon;          // Extension icon image
  bool has_icon = false;        // Whether a custom icon is available
  AstraExtensionState state = AstraExtensionState::kEnabled;
  bool is_action = false;       // Whether this is a browser/page action
  bool is_pinned = false;       // Whether pinned to the sidebar
  bool has_popup = false;       // Whether the extension has a popup
  bool has_options_page = false;  // Whether the extension has options
  std::vector<std::string> permissions;  // Permission names for display
  base::Time install_time;      // When the extension was installed
  base::Time last_updated;      // When the extension was last updated

  // Legacy fields (kept for backward compatibility).
  // TODO(astra): Remove legacy bool enabled once all callers use state.
  bool enabled = false;         // Whether the extension is currently enabled
  bool has_browser_action = false;  // Whether the extension has an action
  bool has_page_action = false;     // Whether the extension has a page action
  std::string popup_url;        // Popup URL from the action manifest, if any
  std::string icon_url;         // Icon URL (for UI presentation)
  AstraExtensionCategory category = AstraExtensionCategory::kOther;
};

// =========================================================================
// AstraExtensionStats — aggregated extension statistics
// =========================================================================
//
// Aggregated extension statistics projected from Chromium's ExtensionRegistry.
// Used by the extension sidebar section and settings UI to show a summary
// of extension state (e.g. "12 installed, 8 enabled, 3 with browser actions").
//
// Chromium owner: ExtensionRegistry (extensions/browser/extension_registry.h)
struct AstraExtensionStats {
  // Total number of installed extensions (all states).
  size_t total_installed = 0;

  // Number of currently enabled extensions.
  size_t enabled_count = 0;

  // Number of disabled extensions.
  size_t disabled_count = 0;

  // Number of extensions with browser actions (action buttons).
  size_t with_browser_action_count = 0;

  // Number of extensions with page actions.
  size_t with_page_action_count = 0;

  // Number of themes installed.
  size_t theme_count = 0;

  // Number of extensions with permissions warnings (high-risk).
  size_t with_permission_warnings_count = 0;

  // Returns the number of "active" (enabled + user-visible) extensions.
  size_t active_count() const { return enabled_count; }

  // Returns true if there are any disabled extensions.
  bool has_disabled() const { return disabled_count > 0; }

  // Returns true if there are any themes installed.
  bool has_themes() const { return theme_count > 0; }
};

// =========================================================================
// AstraExtensionHelperObserver — observer interface
// =========================================================================
//
// Observer interface for AstraExtensionHelper. Notifies when the set of
// extensions changes (install, uninstall, enable, disable) or when
// extension presentation settings change.
//
// All observer methods have empty default implementations so observers
// only need to override the events they care about.
//
// The UI layer implements this observer to refresh the extensions sidebar
// section when extension state changes. The browser layer never depends
// on Views code.
//
// Chromium owner: ExtensionRegistryObserver
//   (extensions/browser/extension_registry_observer.h)
// These Astra notifications would be triggered by observing
// ExtensionRegistry and forwarding relevant events.
class AstraExtensionHelperObserver : public base::CheckedObserver {
 public:
  // Called when the full list of extensions has changed in any way
  // (added, removed, enabled, disabled, etc.).
  // The UI should re-read the list via GetExtensionsWithBrowserActions()
  // and rebuild its projection.
  virtual void OnExtensionsChanged() {}

  // Called when a specific extension's icon has been loaded or updated.
  // The UI can update just that extension's icon without a full rebuild.
  virtual void OnExtensionIconChanged(const std::string& extension_id) {}

  // Called when a new extension has been installed.
  // |extension| contains the projected info for the newly installed extension.
  virtual void OnExtensionInstalled(const AstraExtensionInfo& extension) {}

  // Called when an extension has been uninstalled.
  // |extension_id| is the ID of the uninstalled extension.
  // |extension_name| is the display name of the uninstalled extension.
  virtual void OnExtensionUninstalled(const std::string& extension_id,
                                      const std::u16string& extension_name) {}

  // Called when an extension is enabled.
  // |extension_id| is the ID of the enabled extension.
  virtual void OnExtensionEnabled(const std::string& extension_id) {}

  // Called when an extension is disabled.
  // |extension_id| is the ID of the disabled extension.
  virtual void OnExtensionDisabled(const std::string& extension_id) {}

  // Called when extension presentation settings have changed (e.g. show
  // toolbar, show in sidebar, shortcut visibility).
  // The UI should refresh its extension section presentation.
  virtual void OnExtensionSettingsChanged() {}

 protected:
  ~AstraExtensionHelperObserver() override = default;
};

// =========================================================================
// AstraExtensionHelper — extension projection helper
// =========================================================================
//
// Helper class for extension-related Astra functionality.
//
// This is a profile-scoped helper (KeyedService) that wraps Chromium's
// ExtensionRegistry and ExtensionAction APIs. It provides a clean
// interface for the Astra UI layer to query extension state without
// directly depending on the full extensions subsystem.
//
// Truth source: Chromium's ExtensionRegistry (extensions/browser/extension_registry.h)
// and ExtensionAction / ActionInfo (extensions/common/api/extension_action/action_info.h).
//
// IMPORTANT: Extensions are FULLY OWNED by Chromium's extensions system.
// This helper only projects state — it never modifies extension state.
// All mutations (enable/disable, uninstall, etc.) go through Chromium's
// extension system directly.
//
// Astra-specific presentation preferences (show toolbar, sidebar visibility,
// etc.) are persisted via the profile's PrefService.  These are purely
// presentation concerns and never affect the underlying extension data
// managed by Chromium.
//
// TODO(astra): Proper ExtensionRegistry observer integration.
//   Currently this helper does a one-time read of the registry. To get
//   live updates, we need to implement extensions::ExtensionRegistryObserver
//   and listen for OnExtensionLoaded, OnExtensionUnloaded,
//   OnExtensionInstalled, OnExtensionUninstalled, and OnExtensionStateChanged.
// Chromium owner: ExtensionRegistry (extensions/browser/extension_registry.h)
// Chromium patch point: None needed — ExtensionRegistryObserver is a
//   public observer interface that any KeyedService can implement.
class AstraExtensionHelper : public KeyedService {
 public:
  explicit AstraExtensionHelper(Profile* profile);
  AstraExtensionHelper(const AstraExtensionHelper&) = delete;
  AstraExtensionHelper& operator=(const AstraExtensionHelper&) = delete;
  ~AstraExtensionHelper() override;

  // -- Extension list queries --------------------------------------------

  // Returns the list of currently installed extensions that have a
  // browser action (action button). Only includes extensions that are
  // currently enabled.
  //
  // The returned list is a projection — callers should not cache it
  // across event loops since extension state can change at any time.
  std::vector<AstraExtensionInfo> GetExtensionsWithBrowserActions() const;

  // Returns true if the extension with |extension_id| has a browser
  // action and is enabled.
  bool HasBrowserAction(const std::string& extension_id) const;

  // Returns the total number of installed extensions (all states).
  //
  // TODO(astra): Query ExtensionRegistry for total count across all
  //   extension sets (enabled, disabled, etc.).
  // Chromium owner: ExtensionRegistry
  size_t GetInstalledExtensionsCount() const;

  // Returns the number of currently enabled extensions.
  size_t GetEnabledExtensionsCount() const;

  // Returns the number of disabled extensions.
  size_t GetDisabledExtensionsCount() const;

  // Returns projected info for the extension with the given |extension_id|.
  // Returns an empty AstraExtensionInfo (empty id) if not found.
  //
  // This searches across all installed extensions, not just enabled ones.
  AstraExtensionInfo GetExtensionById(const std::string& extension_id) const;

  // Returns a list of extensions whose name matches |query| (case-insensitive
  // substring match).
  //
  // |max_count| limits the number of results. Use 0 for no limit.
  std::vector<AstraExtensionInfo> GetExtensionByName(
      const std::u16string& query,
      size_t max_count = 0) const;

  // Returns a list of all installed extensions (projection only).
  //
  // |include_disabled| controls whether disabled extensions are included.
  // |max_count| limits the number of results. Use 0 for no limit.
  std::vector<AstraExtensionInfo> GetAllExtensions(
      bool include_disabled = true,
      size_t max_count = 0) const;

  // Returns a list of extensions in the given |category|.
  //
  // TODO(astra): Implement category classification based on extension
  //   manifest type, permissions, and web store category metadata.
  // Chromium owner: extensions::Manifest::Type
  std::vector<AstraExtensionInfo> GetExtensionsByCategory(
      AstraExtensionCategory category) const;

  // Returns the number of extensions in the given |category|.
  size_t GetExtensionCountByCategory(AstraExtensionCategory category) const;

  // -- Extension stats ---------------------------------------------------

  // Returns aggregated extension statistics.
  //
  // TODO(astra): Compute from real ExtensionRegistry data.
  //   For now, returns zeroed stats as a placeholder.
  AstraExtensionStats GetExtensionStats() const;

  // -- Recommended extensions --------------------------------------------

  // Returns a list of recommended extensions for Astra features.
  //
  // These are extensions that integrate well with Astra's workflow
  // (productivity, tab management, workspace tools, etc.).
  //
  // This is a static curated list that Astra surfaces — it does not
  // modify the user's actual installed extensions.
  //
  // Chromium owner: This is pure Astra metadata. The recommendation
  //   logic and list are owned by Astra.
  std::vector<AstraExtensionInfo> GetRecommendedExtensions() const;

  // Returns the number of recommended extensions.
  size_t GetRecommendedExtensionsCount() const;

  // Returns true if the extension with |extension_id| is in the
  // recommended list.
  bool IsRecommendedExtension(const std::string& extension_id) const;

  // -- Categories --------------------------------------------------------

  // Returns the list of extension categories that have at least one
  // installed extension.
  std::vector<AstraExtensionCategory> GetAvailableCategories() const;

  // Returns a human-readable label for a category.
  static std::u16string GetCategoryLabel(AstraExtensionCategory category);

  // Returns the category for an extension based on its manifest type
  // and permissions.
  //
  // TODO(astra): Implement real categorization based on Manifest::Type,
  //   permissions, and web store metadata.
  static AstraExtensionCategory ClassifyExtension(
      const extensions::Extension* extension);

  // -- Icon queries ------------------------------------------------------

  // Returns the icon for |extension_id| at the requested size.
  // If the icon hasn't been loaded yet, returns an empty image and
  // triggers an asynchronous load. Observers are notified via
  // OnExtensionIconChanged() when the icon becomes available.
  //
  // TODO(astra): Real icon loading via extensions::ExtensionIconManager
  //   or extensions::ExtensionImageLoader. For now, returns a placeholder.
  // Chromium owner: ExtensionIconManager
  //   (chrome/browser/extensions/extension_icon_manager.h)
  // Chromium owner: ExtensionImageLoader
  //   (extensions/browser/image_loader.h)
  gfx::Image GetExtensionIcon(const std::string& extension_id, int size) const;

  // Returns the default icon to show when an extension's icon is not
  // yet loaded or unavailable.
  gfx::ImageSkia GetDefaultExtensionIcon(int size) const;

  // -- Popup URL ---------------------------------------------------------

  // Returns the popup URL for |extension_id|'s browser action, or an
  // empty GURL if the extension has no popup.
  //
  // Chromium owner: ExtensionAction (extensions/browser/extension_action.h)
  // Chromium owner: ActionInfo (extensions/common/api/extension_action/action_info.h)
  GURL GetPopupURL(const std::string& extension_id) const;

  // -- Presentation settings --------------------------------------------

  // Returns whether the extension toolbar (browser action area) is shown
  // in Astra UI surfaces (e.g. the toolbar or sidebar).
  //
  // Persisted via PrefService.  Default: true.
  //
  // This is a presentation preference — it never affects whether
  // extensions are installed, enabled, or functional in Chromium.
  bool GetShowExtensionToolbar() const;

  // Sets whether the extension toolbar is shown in Astra UI.
  // Fires OnExtensionSettingsChanged observer notification.
  void SetShowExtensionToolbar(bool show);

  // Toggles the extension toolbar visibility.
  // Returns the new state.
  bool ToggleShowExtensionToolbar();

  // Returns whether extensions are shown in the Astra sidebar section.
  //
  // Persisted via PrefService.  Default: true.
  //
  // This controls whether the extensions section appears in the sidebar.
  // The actual extensions still work normally in Chromium.
  bool GetShowExtensionsInSidebar() const;

  // Sets whether extensions are shown in the sidebar.
  // Fires OnExtensionSettingsChanged observer notification.
  void SetShowExtensionsInSidebar(bool show);

  // Toggles sidebar extension visibility.  Returns the new state.
  bool ToggleShowExtensionsInSidebar();

  // Returns whether the extension manager shortcut is shown in the
  // Astra sidebar or toolbar.
  //
  // Persisted via PrefService.  Default: true.
  //
  // This controls whether the extension manager entry point appears in
  // the Astra UI.  The actual extension management is always available
  // through Chrome settings.
  bool GetExtensionManagerShortcut() const;

  // Sets whether the extension manager shortcut is shown.
  // Fires OnExtensionSettingsChanged observer notification.
  void SetExtensionManagerShortcut(bool show);

  // Toggles the extension manager shortcut visibility.
  // Returns the new state.
  bool ToggleExtensionManagerShortcut();

  // Returns whether recommended extensions are shown in the extension
  // sidebar section.
  //
  // Persisted via PrefService.  Default: true.
  bool GetShowRecommendedExtensions() const;

  // Sets whether recommended extensions are shown.
  // Fires OnExtensionSettingsChanged observer notification.
  void SetShowRecommendedExtensions(bool show);

  // Toggles recommended extensions visibility.  Returns the new state.
  bool ToggleShowRecommendedExtensions();

  // Returns the maximum number of extensions to show in the sidebar
  // extensions section.
  //
  // Persisted via PrefService.  Default: 10.
  int GetMaxSidebarExtensions() const;

  // Sets the maximum number of extensions to show in the sidebar.
  // Values are clamped between 1 and 50.
  // Fires OnExtensionSettingsChanged observer notification.
  void SetMaxSidebarExtensions(int max_count);

  // Returns the default sort order for the extension list.
  // Values: "name", "install_date", "category".
  // Default: "name".
  //
  // Persisted via PrefService.
  std::string GetExtensionSortOrder() const;

  // Sets the default sort order for extension lists.
  // Fires OnExtensionSettingsChanged observer notification.
  void SetExtensionSortOrder(const std::string& sort_order);

  // -- Extension management helpers --------------------------------------

  // Opens the extension management page (chrome://extensions).
  //
  // TODO(astra): Implement proper navigation to extensions settings.
  //   For now, this is a no-op stub.
  // Chromium WebUI: chrome://extensions
  //   (chrome/browser/ui/webui/extensions/extensions_ui.h)
  void OpenExtensionSettings(Profile* profile) const;

  // Opens the Chrome Web Store page for extensions.
  //
  // TODO(astra): Implement navigation to Chrome Web Store.
  void OpenChromeWebStore(Profile* profile) const;

  // Opens the details page for a specific extension.
  //
  // TODO(astra): Implement navigation to extension detail page.
  void OpenExtensionDetails(Profile* profile,
                            const std::string& extension_id) const;

  // Returns true if the extension management page can be opened.
  // In the overlay skeleton, this always returns true (the UI entry
  // point exists even if the actual navigation is not yet wired).
  bool IsExtensionManagementAvailable() const;

  // -- Bulk operations --------------------------------------------------

  // Returns true if bulk extension operations are available.
  // Bulk operations include enable-all, disable-all, etc.
  //
  // TODO(astra): Check if the current profile allows bulk operations.
  //   Some enterprise policies may restrict bulk changes.
  bool IsBulkOperationAvailable() const;

  // -- Observers ---------------------------------------------------------

  void AddObserver(AstraExtensionHelperObserver* observer);
  void RemoveObserver(AstraExtensionHelperObserver* observer);

  // -- Notification helpers (public for testing) ------------------------

  // Notify all observers that the extension list has changed.
  void NotifyExtensionsChanged();

  // Notify all observers that an extension's icon has changed.
  void NotifyExtensionIconChanged(const std::string& extension_id);

  // Notify all observers that an extension has been installed.
  void NotifyExtensionInstalled(const AstraExtensionInfo& extension);

  // Notify all observers that an extension has been uninstalled.
  void NotifyExtensionUninstalled(const std::string& extension_id,
                                  const std::u16string& extension_name);

  // Notify all observers that an extension has been enabled.
  void NotifyExtensionEnabled(const std::string& extension_id);

  // Notify all observers that an extension has been disabled.
  void NotifyExtensionDisabled(const std::string& extension_id);

  // Notify all observers that extension settings have changed.
  void NotifyExtensionSettingsChanged();

  // -- KeyedService ------------------------------------------------------

  void Shutdown() override;

 private:
  // Start observing ExtensionRegistry for changes.
  // TODO(astra): Implement once we wire up ExtensionRegistryObserver.
  void StartObservingRegistry();

  // Stop observing ExtensionRegistry. Called from Shutdown().
  void StopObservingRegistry();

  // Get the ExtensionRegistry for the associated profile.
  extensions::ExtensionRegistry* GetRegistry() const;

  // Get the PrefService for the associated profile.
  // Returns nullptr if profile_ is null or prefs are not available.
  PrefService* GetPrefs() const;

  // Get an Extension pointer from the registry by ID.
  // Returns nullptr if the extension is not found or not enabled.
  const extensions::Extension* GetExtension(const std::string& id) const;

  // Get an Extension pointer from any registry set (enabled, disabled, etc.).
  // Returns nullptr if the extension is not found anywhere.
  const extensions::Extension* GetExtensionAnyState(
      const std::string& id) const;

  // Creates a projected AstraExtensionInfo from a Chromium Extension pointer.
  // Returns an empty info if |extension| is null.
  AstraExtensionInfo CreateExtensionInfo(
      const extensions::Extension* extension) const;

  // The profile this helper is associated with. Not owned.
  raw_ptr<Profile> profile_ = nullptr;

  // Observers for extension state changes.
  base::ObserverList<AstraExtensionHelperObserver> observers_;

  // Tracks whether we're currently observing the extension registry.
  // TODO(astra): Flip this to true once ExtensionRegistryObserver
  // integration is implemented.
  bool is_observing_registry_ = false;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_EXTENSION_HELPER_H_
