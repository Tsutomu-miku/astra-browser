#include "astra/browser/astra_incognito_handler.h"

#include "base/check.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"

#include "astra/browser/astra_prefs.h"

namespace astra {

namespace {

// Minimum valid tab count per window.
constexpr int kMinTabCount = 1;

}  // namespace

// =========================================================================
// Construction / destruction
// =========================================================================

AstraIncognitoHandler::AstraIncognitoHandler(Profile* profile)
    : profile_(profile) {
  DCHECK(profile_);

  // Set up pref change observer.
  pref_change_registrar_ = std::make_unique<PrefChangeRegistrar>();
  pref_change_registrar_->Init(profile_->GetPrefs());

  // Observe all incognito-related prefs.
  pref_change_registrar_->Add(
      prefs::kPrefIncognitoShowSidebarBadge,
      base::BindRepeating(&AstraIncognitoHandler::OnPrefChanged,
                          base::Unretained(this)));
  pref_change_registrar_->Add(
      prefs::kPrefIncognitoConfirmCloseAll,
      base::BindRepeating(&AstraIncognitoHandler::OnPrefChanged,
                          base::Unretained(this)));
  pref_change_registrar_->Add(
      prefs::kPrefIncognitoWarnOnExternalOpen,
      base::BindRepeating(&AstraIncognitoHandler::OnPrefChanged,
                          base::Unretained(this)));
  pref_change_registrar_->Add(
      prefs::kPrefIncognitoDefaultWorkspace,
      base::BindRepeating(&AstraIncognitoHandler::OnPrefChanged,
                          base::Unretained(this)));
}

AstraIncognitoHandler::~AstraIncognitoHandler() = default;

void AstraIncognitoHandler::Shutdown() {
  // Clean up pref observation.
  pref_change_registrar_.reset();
  profile_ = nullptr;
}

// =========================================================================
// Observers
// =========================================================================

void AstraIncognitoHandler::AddObserver(AstraIncognitoObserver* observer) {
  observers_.AddObserver(observer);
}

void AstraIncognitoHandler::RemoveObserver(AstraIncognitoObserver* observer) {
  observers_.RemoveObserver(observer);
}

// =========================================================================
// Profile detection (static utilities)
// =========================================================================

bool AstraIncognitoHandler::IsIncognitoProfile(Profile* profile) {
  if (!profile) {
    return false;
  }
  // Profile::IsOffTheRecord() is the canonical Chromium way to detect
  // incognito / OTR profiles.  IsIncognitoProfile() is an alias that also
  // returns true for guest profiles in some build configurations.
  // We use IsOffTheRecord() for precision — it specifically checks the
  // OTR (off-the-record) bit which is set for incognito profiles.
  //
  // Chromium owner: Profile (chrome/browser/profiles/profile.h)
  return profile->IsOffTheRecord();
}

bool AstraIncognitoHandler::IsIncognitoWebContents(
    content::WebContents* web_contents) {
  if (!web_contents) {
    return false;
  }
  // Extract the BrowserContext from WebContents and check if it's OTR.
  // This follows the standard Chromium pattern for checking incognito state
  // from a WebContents.
  //
  // Chromium owner: content::BrowserContext (content/public/browser/browser_context.h)
  content::BrowserContext* browser_context = web_contents->GetBrowserContext();
  if (!browser_context) {
    return false;
  }
  return browser_context->IsOffTheRecord();
}

// =========================================================================
// Settings queries
// =========================================================================

bool AstraIncognitoHandler::ShouldShowSidebarBadge() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return prefs::kDefaultIncognitoShowSidebarBadge;
  }
  return profile_->GetPrefs()->GetBoolean(
      prefs::kPrefIncognitoShowSidebarBadge);
}

bool AstraIncognitoHandler::ShouldConfirmCloseAll() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return prefs::kDefaultIncognitoConfirmCloseAll;
  }
  return profile_->GetPrefs()->GetBoolean(
      prefs::kPrefIncognitoConfirmCloseAll);
}

bool AstraIncognitoHandler::ShouldWarnOnExternalOpen() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return prefs::kDefaultIncognitoWarnOnExternalOpen;
  }
  return profile_->GetPrefs()->GetBoolean(
      prefs::kPrefIncognitoWarnOnExternalOpen);
}

std::string AstraIncognitoHandler::GetDefaultWorkspaceId() const {
  if (!profile_ || !profile_->GetPrefs()) {
    return prefs::kDefaultIncognitoDefaultWorkspace;
  }
  return profile_->GetPrefs()->GetString(
      prefs::kPrefIncognitoDefaultWorkspace);
}

// =========================================================================
// Settings mutation
// =========================================================================

void AstraIncognitoHandler::SetShowSidebarBadge(bool enabled) {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }

  PrefService* prefs = profile_->GetPrefs();
  if (prefs->GetBoolean(prefs::kPrefIncognitoShowSidebarBadge) == enabled) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefIncognitoShowSidebarBadge, enabled);
  // Pref change observer will fire and notify observers.
}

void AstraIncognitoHandler::SetConfirmCloseAll(bool enabled) {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }

  PrefService* prefs = profile_->GetPrefs();
  if (prefs->GetBoolean(prefs::kPrefIncognitoConfirmCloseAll) == enabled) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefIncognitoConfirmCloseAll, enabled);
  // Pref change observer will fire and notify observers.
}

void AstraIncognitoHandler::SetWarnOnExternalOpen(bool enabled) {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }

  PrefService* prefs = profile_->GetPrefs();
  if (prefs->GetBoolean(prefs::kPrefIncognitoWarnOnExternalOpen) == enabled) {
    return;
  }

  prefs->SetBoolean(prefs::kPrefIncognitoWarnOnExternalOpen, enabled);
  // Pref change observer will fire and notify observers.
}

void AstraIncognitoHandler::SetDefaultWorkspaceId(
    const std::string& workspace_id) {
  if (!profile_ || !profile_->GetPrefs()) {
    return;
  }

  PrefService* prefs = profile_->GetPrefs();
  if (prefs->GetString(prefs::kPrefIncognitoDefaultWorkspace) == workspace_id) {
    return;
  }

  prefs->SetString(prefs::kPrefIncognitoDefaultWorkspace, workspace_id);
  // Pref change observer will fire and notify observers.
}

// =========================================================================
// Session tracking
// =========================================================================

int AstraIncognitoHandler::GetIncognitoWindowCount() const {
  return window_count_;
}

int AstraIncognitoHandler::GetIncognitoTabCount() const {
  return tab_count_;
}

bool AstraIncognitoHandler::HasActiveIncognitoSession() const {
  return window_count_ > 0;
}

void AstraIncognitoHandler::NotifyIncognitoWindowCreated(int tab_count) {
  if (tab_count < kMinTabCount) {
    tab_count = kMinTabCount;
  }

  window_count_++;
  tab_count_ += tab_count;

  for (auto& observer : observers_) {
    observer.OnIncognitoWindowCreated();
  }
  NotifySettingsChanged();
}

void AstraIncognitoHandler::NotifyIncognitoWindowClosed(int tab_count) {
  if (window_count_ <= 0) {
    return;
  }
  if (tab_count < kMinTabCount) {
    tab_count = kMinTabCount;
  }

  window_count_--;
  tab_count_ -= tab_count;
  if (tab_count_ < 0) {
    tab_count_ = 0;
  }

  bool all_closed = (window_count_ == 0);
  if (all_closed) {
    tab_count_ = 0;
  }

  for (auto& observer : observers_) {
    observer.OnIncognitoWindowClosed();
    if (all_closed) {
      observer.OnAllIncognitoWindowsClosed();
    }
  }
  NotifySettingsChanged();
}

// =========================================================================
// Projected Chromium state
// =========================================================================

bool AstraIncognitoHandler::ShouldAutoDeleteHistoryOnExit() const {
  // TODO(astra): Read from Chromium's browsing data / clear on exit pref.
  // Chromium owner: BrowsingDataRemover (chrome/browser/browsing_data/)
  //
  // In incognito mode, history is inherently not persisted to disk — the
  // OTR profile keeps everything in memory and discards it on exit.
  // However, some privacy configurations perform additional cleanup on
  // exit (e.g., clearing cookies, site data, cache).
  //
  // For now, we return true as the default expectation for incognito:
  // all session data is discarded when the last incognito window closes.
  // For regular profiles, this would depend on the "clear on exit" setting.
  if (IsIncognitoProfile(profile_)) {
    return true;
  }
  return false;
}

bool AstraIncognitoHandler::ShouldBlockThirdPartyCookies() const {
  // TODO(astra): Read from Chromium's cookie settings.
  // Chromium owner: CookieSettings
  //   (components/content_settings/core/browser/cookie_settings.h)
  //
  // Chromium has multiple cookie blocking modes:
  //   - Allow all cookies
  //   - Block third-party cookies in incognito
  //   - Block third-party cookies
  //   - Block all cookies
  //
  // In Astra, we project this setting to UI surfaces so they can show
  // appropriate indicators (e.g., a "cookies blocked" badge in the
  // address bar area of the sidebar).
  //
  // For now, we return true for incognito profiles (matching Chromium's
  // default behavior of blocking third-party cookies in incognito) and
  // false for regular profiles.
  if (IsIncognitoProfile(profile_)) {
    return true;
  }
  return false;
}

bool AstraIncognitoHandler::IsIncognitoModeAvailable() const {
  // TODO(astra): Read from Chromium's incognito mode policy.
  // Chromium owner: IncognitoModePrefs
  //   (chrome/browser/prefs/incognito_mode_prefs.h)
  //
  // Incognito mode can be disabled by:
  //   - Enterprise policy (IncognitoModeAvailability policy)
  //   - Parental controls / supervised users
  //   - Guest mode restrictions
  //
  // For now, we return true — incognito is available by default.
  return true;
}

// =========================================================================
// Workspace behavior
// =========================================================================

bool AstraIncognitoHandler::AreWorkspaceMutationsAllowed() const {
  // Workspace mutations (add, rename, delete, reorder) modify data that is
  // persisted via PrefService on the original profile.  Since the workspace
  // service uses kRedirectedToOriginal for incognito, mutations from an
  // incognito window would affect the main profile's persisted state.
  //
  // Design decision: disable all workspace mutations in incognito.
  // Rationale: the user expects incognito to be a separate, non-persisting
  // session.  If they create/delete a workspace in incognito, it would
  // surprise them to see the change reflected in their main profile.
  return !IsIncognitoProfile(profile_);
}

bool AstraIncognitoHandler::DoesWorkspaceActivationAffectService() const {
  // In the regular profile, activating a workspace changes the service's
  // active_workspace_id_ which is persisted to prefs.  This is correct.
  //
  // In incognito, activation must NOT touch the shared service's active
  // workspace because:
  //   1. It would persist (via SaveToPrefs on the original profile),
  //      leaking the user's incognito activity into their main profile.
  //   2. It would affect other incognito windows too (since they share
  //      the service instance), which may or may not be desired but is
  //      definitely surprising if it also affects the main profile.
  //
  // Instead, the sidebar view tracks its own active_workspace_id_ locally
  // when the profile is incognito.  This gives each incognito window its
  // own independent active workspace that is lost when the window closes —
  // consistent with incognito's ephemeral nature.
  //
  // TODO(astra): Evaluate whether all incognito windows should share a
  // single active workspace (like Chromium's incognito profile is shared
  // across windows).  If so, we could store it on the OTR profile's
  // PrefService (which is in-memory only).  For now, per-window behavior
  // is simpler and matches the sidebar's per-window nature.
  // Chromium patch point: OTR profile PrefService is in-memory only, so
  // storing the active workspace id there would be safe and not persist.
  return !IsIncognitoProfile(profile_);
}

// =========================================================================
// Favorite behavior
// =========================================================================

bool AstraIncognitoHandler::AreFavoritesMutable() const {
  // Favorite state (is_favorite flag on AstraTabFeatures) is per-tab
  // metadata.  However, favoriting a tab is conceptually a persistent
  // action — users expect favorites to stick around.
  //
  // Design decision: favorites are read-only in incognito.
  // Rationale:
  //   1. Favorite folders are shared with the original profile (via
  //      kRedirectedToOriginal on AstraFavoriteService).  Moving a tab
  //      into a folder would require folder state to be consistent.
  //   2. Allowing favorites in incognito would mean:
  //      - The favorite state lives on the WebContents (per-tab).
  //      - It disappears when the incognito window closes.
  //      - This is confusing — "I favorited it but it's gone."
  //   3. This matches Chromium's approach with bookmarks in incognito:
  //      you can see existing bookmarks but modifications are disabled.
  return !IsIncognitoProfile(profile_);
}

bool AstraIncognitoHandler::AreFavoriteFoldersMutable() const {
  // Favorite folder mutations (add, rename, delete, reorder, toggle
  // expanded) modify state that is shared with the original profile
  // (kRedirectedToOriginal).  These are persistent structural changes
  // that must not happen from incognito.
  //
  // This is strictly stronger than AreFavoritesMutable — even if we
  // someday allow per-tab favorite toggling in incognito, folder
  // mutations should remain disabled because they affect the folder
  // tree structure shared with the main profile.
  return !IsIncognitoProfile(profile_);
}

bool AstraIncognitoHandler::DefaultFavoriteStateForProfile() const {
  // Incognito tabs always start as non-favorite.
  //
  // In the regular profile, tabs also default to non-favorite, so this
  // returns the same value.  But we make it a separate function because
  // the semantic reason is different:
  //   - Regular profile: default state is false, user can toggle it.
  //   - Incognito: default state is false AND it cannot be changed.
  if (IsIncognitoProfile(profile_)) {
    return false;
  }
  return false;
}

// =========================================================================
// Sidebar behavior
// =========================================================================

bool AstraIncognitoHandler::ShouldShowIncognitoIndicator() const {
  // Show an incognito indicator (e.g. a small badge or tint) in the
  // sidebar when the profile is incognito AND the setting is enabled.
  //
  // This is a visual reminder that the user is in incognito mode and
  // that certain features (workspace editing, favorite toggling) are
  // disabled.  It matches Chromium's own incognito visual treatment
  // (the incognito guy icon, the darker title bar, etc.).
  //
  // TODO(astra): Define the exact visual treatment of the incognito
  // indicator in the sidebar.  Options:
  //   1. A small incognito icon in the workspace switcher area.
  //   2. A subtle color tint on the sidebar background.
  //   3. An "Incognito" label in the section header area.
  // Chromium subsystem: ui/color/color_provider.h (for incognito colors)
  return IsIncognitoProfile(profile_) && ShouldShowSidebarBadge();
}

// =========================================================================
// Split view / Glance
// =========================================================================

bool AstraIncognitoHandler::IsSplitViewAllowed(Profile* /*profile*/) {
  // Split view is always allowed, even in incognito.
  //
  // Rationale:
  //   - Split view is a per-tab presentation feature — it only affects
  //     how tabs are displayed within a window.
  //   - It does not persist any state to disk (split view config lives
  //     on AstraTabFeatures, which is WebContentsUserData).
  //   - It does not share data with the original profile.
  //   - It is analogous to tab strip position or window size — purely
  //     a presentation concern with no privacy implications.
  //
  // This matches how Chromium treats window layout in incognito: you
  // can resize, arrange, and tile windows freely in incognito mode.
  return true;
}

bool AstraIncognitoHandler::IsGlanceAllowed(Profile* /*profile*/) {
  // Glance / peek mode is always allowed, even in incognito.
  //
  // Rationale (same as split view):
  //   - Glance is a per-tab presentation feature (preview overlay).
  //   - It does not persist state.
  //   - It does not share data with the original profile.
  //   - It is analogous to hover previews or tab tooltips — purely a
  //     presentation concern with no privacy implications.
  return true;
}

// =========================================================================
// Bulk operations
// =========================================================================

void AstraIncognitoHandler::CloseAllIncognitoWindows() {
  // TODO(astra): Implement via BrowserList::CloseAllBrowsersWithProfile().
  // Chromium owner: BrowserList (chrome/browser/ui/browser_list.h)
  //
  // For now, we reset our local tracking state and notify observers.
  // In full Chromium integration, the actual window closing is handled
  // by BrowserList and this service observes the result via
  // BrowserListObserver::OnBrowserRemoved().

  if (window_count_ == 0) {
    return;
  }

  window_count_ = 0;
  tab_count_ = 0;

  for (auto& observer : observers_) {
    observer.OnAllIncognitoWindowsClosed();
  }
  NotifySettingsChanged();
}

// =========================================================================
// Pref registration
// =========================================================================

// static
void AstraIncognitoHandler::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  // Whether the sidebar shows an incognito badge/indicator.
  // Default: true — the incognito indicator is shown by default as a
  // visual reminder that the user is in incognito mode.
  registry->RegisterBooleanPref(prefs::kPrefIncognitoShowSidebarBadge,
                                prefs::kDefaultIncognitoShowSidebarBadge);

  // Whether to confirm before closing all incognito windows.
  // Default: false — no confirmation by default, matching Chromium's
  // behavior of closing incognito windows immediately.
  registry->RegisterBooleanPref(prefs::kPrefIncognitoConfirmCloseAll,
                                prefs::kDefaultIncognitoConfirmCloseAll);

  // Whether to warn when external links open in incognito.
  // Default: false — no warning by default.
  registry->RegisterBooleanPref(prefs::kPrefIncognitoWarnOnExternalOpen,
                                prefs::kDefaultIncognitoWarnOnExternalOpen);

  // Default workspace ID for new incognito windows.
  // Default: "default" — incognito windows start on the default workspace.
  registry->RegisterStringPref(prefs::kPrefIncognitoDefaultWorkspace,
                               prefs::kDefaultIncognitoDefaultWorkspace);
}

// =========================================================================
// Pref change handling
// =========================================================================

void AstraIncognitoHandler::OnPrefChanged(const std::string& pref_name) {
  // Notify specific observers based on which pref changed,
  // then notify the catch-all.
  if (pref_name == prefs::kPrefIncognitoShowSidebarBadge) {
    bool enabled = ShouldShowSidebarBadge();
    for (auto& observer : observers_) {
      observer.OnShowSidebarBadgeChanged(enabled);
    }
  } else if (pref_name == prefs::kPrefIncognitoConfirmCloseAll) {
    bool enabled = ShouldConfirmCloseAll();
    for (auto& observer : observers_) {
      observer.OnConfirmCloseAllChanged(enabled);
    }
  } else if (pref_name == prefs::kPrefIncognitoWarnOnExternalOpen) {
    bool enabled = ShouldWarnOnExternalOpen();
    for (auto& observer : observers_) {
      observer.OnWarnOnExternalOpenChanged(enabled);
    }
  } else if (pref_name == prefs::kPrefIncognitoDefaultWorkspace) {
    std::string id = GetDefaultWorkspaceId();
    for (auto& observer : observers_) {
      observer.OnDefaultWorkspaceChanged(id);
    }
  }

  // Catch-all notification.
  NotifySettingsChanged();
}

void AstraIncognitoHandler::NotifySettingsChanged() {
  for (auto& observer : observers_) {
    observer.OnIncognitoSettingsChanged();
  }
}

}  // namespace astra
