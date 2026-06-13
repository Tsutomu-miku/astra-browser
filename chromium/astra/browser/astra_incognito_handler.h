#ifndef ASTRA_BROWSER_ASTRA_INCOGNITO_HANDLER_H_
#define ASTRA_BROWSER_ASTRA_INCOGNITO_HANDLER_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "components/keyed_service/core/keyed_service.h"

class PrefRegistrySimple;
class Profile;
class PrefChangeRegistrar;

namespace content {
class WebContents;
}  // namespace content

namespace astra {

// =========================================================================
// AstraIncognitoObserver
// =========================================================================
//
// Observer interface for incognito-related events and settings changes.
//
// Astra UI surfaces (sidebar, workspace switcher, window frames) implement
// this interface and register with AstraIncognitoHandler to react to
// incognito state changes.
//
// All methods have default empty implementations so subclasses only need
// to override the events they care about.
//
// Chromium subsystem reused:
//   - base::ObserverList (base/observer_list.h)
//   - base::CheckedObserver (base/observer_list_types.h)
// =========================================================================
class AstraIncognitoObserver : public base::CheckedObserver {
 public:
  // Called when an incognito window is created.
  // The service tracks this via BrowserList observation in full Chromium
  // integration, or via explicit NotifyIncognitoWindowCreated() calls.
  virtual void OnIncognitoWindowCreated() {}

  // Called when an incognito window is closed.
  virtual void OnIncognitoWindowClosed() {}

  // Called when the last incognito window is closed and the incognito
  // session ends.
  virtual void OnAllIncognitoWindowsClosed() {}

  // Called when the show-sidebar-badge setting changes.
  virtual void OnShowSidebarBadgeChanged(bool enabled) {}

  // Called when the confirm-close-all setting changes.
  virtual void OnConfirmCloseAllChanged(bool enabled) {}

  // Called when the warn-on-external-open setting changes.
  virtual void OnWarnOnExternalOpenChanged(bool enabled) {}

  // Called when the default workspace for incognito windows changes.
  virtual void OnDefaultWorkspaceChanged(const std::string& workspace_id) {}

  // Called when any incognito setting or state changes.
  // Use this for catch-all updates (e.g., full UI refresh).
  virtual void OnIncognitoSettingsChanged() {}

 protected:
  ~AstraIncognitoObserver() override = default;
};

// =========================================================================
// AstraIncognitoHandler
// =========================================================================
//
// Profile-scoped incognito management service for Astra.
//
// This service projects Chromium's incognito (Off-The-Record) state and
// adds Astra-specific incognito settings and session tracking.  Chromium
// owns all OTR profile logic, WebContents ownership, and privacy features —
// this service only adds metadata, UI settings, and session tracking on top.
//
// Incognito design principles for Astra:
//   - Workspace DEFINITIONS are shared with the original profile (the user
//     sees the same workspace list).  This is enforced by the
//     AstraWorkspaceServiceFactory using kRedirectedToOriginal.
//   - Workspace MUTATIONS (add, rename, delete, reorder) are DISABLED in
//     incognito because they would persist and affect the original profile.
//   - The ACTIVE workspace in incognito is local to the incognito session —
//     it does not affect the original profile's active workspace and is
//     not persisted.  The UI layer (sidebar) tracks it locally.
//   - Favorite FOLDER definitions are shared with the original profile.
//   - Favorite MUTATIONS (toggle favorite, move between folders, add/rename/
//     delete folders) are DISABLED in incognito.
//   - Per-tab favorite state (is_favorite) defaults to false for incognito
//     tabs and cannot be changed.
//   - Split view and glance work normally in incognito (they are per-tab
//     presentation features with no persistence).
//   - The sidebar works normally but shows an incognito indicator and
//     disables drag-and-drop into read-only sections.
//
// Chromium subsystems reused:
//   - Profile (chrome/browser/profiles/profile.h) — OTR profile management
//   - Profile::IsOffTheRecord() / Profile::IsIncognitoProfile()
//   - ProfileKeyedServiceFactory::kRedirectedToOriginal pattern
//   - PrefService — persistence for Astra-specific incognito settings
//   - BrowserList — window tracking (observed in full Chromium integration)
//   - TabStripModel — tab counting (observed in full integration)
//
// Chromium patch points:
//   - None needed for profile detection — uses public Profile APIs.
//   - Browser creation/destruction hooks for automatic window count tracking.
//   - UI-level incognito styling would patch into chrome/browser/ui/views/
//     frame/browser_frame_view.cc or similar.
//   - CloseAllIncognitoWindows delegates to BrowserList::CloseAllBrowsersWithProfile.
// =========================================================================

class AstraIncognitoHandler : public KeyedService {
 public:
  explicit AstraIncognitoHandler(Profile* profile);
  ~AstraIncognitoHandler() override;
  AstraIncognitoHandler(const AstraIncognitoHandler&) = delete;
  AstraIncognitoHandler& operator=(const AstraIncognitoHandler&) = delete;

  // KeyedService:
  void Shutdown() override;

  // -- Observers -----------------------------------------------------------

  void AddObserver(AstraIncognitoObserver* observer);
  void RemoveObserver(AstraIncognitoObserver* observer);

  // -- Profile detection (static utilities) --------------------------------
  //
  // These are static because they are pure query functions that do not
  // depend on any service state — they just delegate to Chromium APIs.

  // Returns true if |profile| is an off-the-record (incognito) profile.
  //
  // Chromium owner: Profile::IsOffTheRecord()
  // (chrome/browser/profiles/profile.h)
  static bool IsIncognitoProfile(Profile* profile);

  // Returns true if |web_contents| belongs to an incognito profile.
  // Convenience wrapper that extracts the profile from WebContents.
  static bool IsIncognitoWebContents(content::WebContents* web_contents);

  // -- Settings queries ----------------------------------------------------

  // Whether the sidebar should show an incognito badge/indicator.
  // This is an Astra UI preference — it does not affect Chromium's own
  // incognito visual treatment (title bar color, incognito icon, etc.).
  bool ShouldShowSidebarBadge() const;

  // Whether to show a confirmation dialog before closing all incognito
  // windows via the "Close All Incognito" command.
  bool ShouldConfirmCloseAll() const;

  // Whether to warn the user when an external link is about to open in
  // an incognito window.
  bool ShouldWarnOnExternalOpen() const;

  // Returns the default workspace ID for new incognito windows.
  //
  // Incognito windows start on this workspace.  The active workspace in
  // incognito is local to each window and does not affect the original
  // profile's active workspace.
  std::string GetDefaultWorkspaceId() const;

  // -- Settings mutation ---------------------------------------------------

  // Sets whether the sidebar shows an incognito badge.
  // Fires OnShowSidebarBadgeChanged and OnIncognitoSettingsChanged observers.
  void SetShowSidebarBadge(bool enabled);

  // Sets whether to confirm before closing all incognito windows.
  // Fires OnConfirmCloseAllChanged and OnIncognitoSettingsChanged observers.
  void SetConfirmCloseAll(bool enabled);

  // Sets whether to warn when external links open in incognito.
  // Fires OnWarnOnExternalOpenChanged and OnIncognitoSettingsChanged observers.
  void SetWarnOnExternalOpen(bool enabled);

  // Sets the default workspace ID for new incognito windows.
  // Fires OnDefaultWorkspaceChanged and OnIncognitoSettingsChanged observers.
  void SetDefaultWorkspaceId(const std::string& workspace_id);

  // -- Session tracking ----------------------------------------------------
  //
  // These methods project Chromium's incognito window and tab state.
  //
  // In a full Chromium integration, the window and tab counts are updated
  // automatically by observing BrowserList and TabStripModel.  For testing
  // and explicit use, this service provides NotifyWindowCreated /
  // NotifyWindowClosed methods.
  //
  // TODO(astra): Integrate with Chromium's BrowserListObserver for
  //   automatic window count tracking.
  // Chromium owner: BrowserList (chrome/browser/ui/browser_list.h)
  //
  // TODO(astra): Integrate with TabStripModelObserver for automatic
  //   tab count tracking across incognito windows.
  // Chromium owner: TabStripModel (chrome/browser/ui/tabs/tab_strip_model.h)

  // Returns the number of currently active incognito windows.
  int GetIncognitoWindowCount() const;

  // Returns the total number of tabs across all incognito windows.
  int GetIncognitoTabCount() const;

  // Returns true if there is at least one active incognito window.
  bool HasActiveIncognitoSession() const;

  // Notifies the service that an incognito window was created.
  // Increments the window count and notifies observers.
  // |tab_count| is the number of tabs in the new window (default: 1).
  //
  // In full Chromium integration, this is called automatically from
  // a BrowserListObserver or browser creation hook.
  void NotifyIncognitoWindowCreated(int tab_count = 1);

  // Notifies the service that an incognito window was closed.
  // Decrements the window count and notifies observers.
  // |tab_count| is the number of tabs in the closed window (default: 1).
  void NotifyIncognitoWindowClosed(int tab_count = 1);

  // -- Projected Chromium state --------------------------------------------
  //
  // These methods project Chromium-owned incognito-related settings.
  // Astra does NOT own these settings — it only reads them from Chromium.
  // They are provided as convenience accessors for Astra UI surfaces.
  //
  // TODO(astra): Integrate fully with Chromium's privacy settings.
  // Currently these return sensible defaults.  Full integration would read
  // from the corresponding Chromium prefs or service objects.

  // Returns true if browsing history should be automatically deleted
  // when the incognito session ends.
  //
  // This projects Chromium's "clear browsing data on exit" /
  // "auto-delete history" setting.  In Chromium's OTR profile, history
  // is inherently not persisted, but this setting controls whether
  // additional cleanup is performed.
  //
  // TODO(astra): Read from Chromium's browsing data / clear on exit pref.
  // Chromium owner: BrowsingDataRemover / clear_browsing_data settings
  //   (chrome/browser/browsing_data/browsing_data_remover.h)
  bool ShouldAutoDeleteHistoryOnExit() const;

  // Returns true if third-party cookies are blocked in incognito mode.
  //
  // This projects Chromium's third-party cookie blocking setting.
  //
  // TODO(astra): Read from Chromium's cookie settings.
  // Chromium owner: ContentSettings / cookie controls
  //   (components/content_settings/core/browser/host_content_settings_map.h)
  // Chromium owner: CookieSettings
  //   (components/content_settings/core/browser/cookie_settings.h)
  bool ShouldBlockThirdPartyCookies() const;

  // Returns true if incognito mode is available.
  // Incognito may be disabled by enterprise policy or parental controls.
  //
  // TODO(astra): Read from Chromium's policy / incognito availability.
  // Chromium owner: IncognitoModePrefs (chrome/browser/prefs/incognito_mode_prefs.h)
  bool IsIncognitoModeAvailable() const;

  // -- Workspace behavior --------------------------------------------------

  // Returns true if workspace mutations (add, rename, delete, reorder,
  // accent color change) are allowed for this service's profile.
  //
  // In incognito, workspace mutations are disabled because the workspace
  // service is redirected to the original profile (kRedirectedToOriginal),
  // meaning mutations would persist and affect the main profile.
  bool AreWorkspaceMutationsAllowed() const;

  // Returns true if activating (switching to) a different workspace should
  // affect the shared service state.
  //
  // In incognito, workspace activation is local to the incognito session —
  // it must NOT change the original profile's active workspace (which is
  // persisted via PrefService).
  bool DoesWorkspaceActivationAffectService() const;

  // -- Favorite behavior ---------------------------------------------------

  // Returns true if favorite state on individual tabs can be modified.
  //
  // In incognito, favorites are read-only: you can see existing favorites
  // (from the original profile) but cannot add, remove, or reorder them.
  // This matches Chromium's bookmark behavior in incognito.
  bool AreFavoritesMutable() const;

  // Returns true if favorite folder mutations (add, rename, delete, reorder,
  // toggle expanded) are allowed.
  //
  // Disabled in incognito because folder state is shared with the original
  // profile via kRedirectedToOriginal and mutations would persist.
  bool AreFavoriteFoldersMutable() const;

  // Returns the default favorite state for a new tab in this profile.
  // In incognito, tabs always start as non-favorite and cannot be favorited.
  bool DefaultFavoriteStateForProfile() const;

  // -- Sidebar behavior ----------------------------------------------------

  // Returns true if the sidebar should show an incognito mode indicator.
  // True when the profile is incognito AND the sidebar badge setting is on.
  bool ShouldShowIncognitoIndicator() const;

  // -- Split view / Glance -------------------------------------------------
  //
  // These remain static because they are pure policy queries that do not
  // depend on any service state.

  // Returns true if split view is allowed in incognito.
  // Always true — split view is a per-tab presentation feature.
  static bool IsSplitViewAllowed(Profile* profile);

  // Returns true if glance/peek is allowed in incognito.
  // Always true — glance is a per-tab presentation feature.
  static bool IsGlanceAllowed(Profile* profile);

  // -- Bulk operations -----------------------------------------------------

  // Closes all incognito windows for this profile's OTR profile.
  //
  // TODO(astra): Implement via BrowserList::CloseAllBrowsersWithProfile().
  // Chromium owner: BrowserList::CloseAllBrowsersWithProfile()
  //   (chrome/browser/ui/browser_list.h)
  //
  // For now, this method resets the local session tracking state and
  // notifies observers.  In full Chromium integration, the actual window
  // closing is handled by BrowserList and this service observes the result.
  void CloseAllIncognitoWindows();

  // -- Pref registration ---------------------------------------------------

  // Registers all incognito-related profile preferences.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  // Called when an incognito pref changes.  Notifies observers.
  void OnPrefChanged(const std::string& pref_name);

  // Notifies all observers via the catch-all OnIncognitoSettingsChanged.
  void NotifySettingsChanged();

  raw_ptr<Profile> profile_;

  // Pref change observer — listens for incognito pref changes.
  std::unique_ptr<PrefChangeRegistrar> pref_change_registrar_;

  // Observers (Astra UI surfaces).
  base::ObserverList<AstraIncognitoObserver> observers_;

  // Session tracking state.
  //
  // In a full Chromium integration, these would be derived from observing
  // BrowserList and TabStripModel.  For testing and explicit use, they
  // are managed via NotifyIncognitoWindowCreated/Closed.
  //
  // TODO(astra): Replace manual tracking with BrowserList observation
  //   in full Chromium integration.
  int window_count_ = 0;
  int tab_count_ = 0;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_INCOGNITO_HANDLER_H_
