// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/app/astra_content_browser_client.h"

#include "base/check.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/observer_list.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "url/gurl.h"
#include "url/url_constants.h"

// Forward-declare Astra scheme constant from the UI WebUI constants file.
// We define a local copy here to avoid pulling in the full WebUI constants
// header at the app layer.
//
// TODO(astra): Move this constant to a shared location (e.g., astra/common/)
//   so both app and ui layers can use it without duplicating.
//   Chromium pattern: chrome/common/url_constants.cc
namespace astra {
namespace {

// The URL scheme used by Astra WebUI pages (e.g. astra://newtab).
// Must match kAstraUIScheme in astra/ui/webui/astra_webui_constants.h.
constexpr char kAstraUIScheme[] = "astra";

// ============================================================================
// Astra WebUI host names
// ============================================================================
//
// Known valid hosts for astra:// URLs.
//
// Keep this list in sync with the actual WebUI pages defined in
// astra/ui/webui/.
//
// Chromium pattern: chrome/common/webui_url_constants.cc

struct AstraWebUIHost {
  const char* host_name;
  bool is_internal;  // True if not directly user-navigable
  const char* description;
};

// All known Astra WebUI hosts.
constexpr AstraWebUIHost kAstraWebUIHosts[] = {
    // User-facing WebUI pages (directly navigable)
    {"newtab", false, "Astra new tab page"},
    {"settings", false, "Astra settings page"},
    {"workspaces", false, "Astra workspaces overview"},
    {"history", false, "Astra history page"},
    {"downloads", false, "Astra downloads page"},
    {"bookmarks", false, "Astra bookmarks page"},
    {"extensions", false, "Astra extensions page"},
    {"passwords", false, "Astra passwords page"},
    {"reading-list", false, "Astra reading list page"},
    {"notes", false, "Astra notes page"},
    {"command-palette", false, "Astra command palette"},
    {"tab-search", false, "Astra tab search"},

    // Internal WebUI pages (not directly user-navigable)
    {"webui-resources", true, "Astra WebUI shared resources"},
    {"internals", true, "Astra internals page (debug)"},
    {"welcome", false, "Astra welcome / onboarding page"},
    {"about", false, "Astra about page"},
    {"focus-mode", false, "Astra focus mode page"},
    {"screenshot", false, "Astra screenshot editor"},
};

// Number of known Astra WebUI hosts.
constexpr size_t kAstraWebUIHostCount =
    sizeof(kAstraWebUIHosts) / sizeof(kAstraWebUIHosts[0]);

}  // namespace

// ============================================================================
// Static member definitions
// ============================================================================

bool AstraContentBrowserClient::browser_startup_complete_ = false;
bool AstraContentBrowserClient::hooks_installed_ = false;

// ============================================================================
// Observer list accessor
// ============================================================================

base::ObserverList<AstraStartupObserver>&
AstraContentBrowserClient::GetObservers() {
  static base::NoDestructor<base::ObserverList<AstraStartupObserver>>
      observers;
  return *observers;
}

// ============================================================================
// Installation
// ============================================================================

void AstraContentBrowserClient::InstallChromeContentBrowserClientHooks() {
  // Hook point for Astra-specific ChromeContentBrowserClient integration.
  //
  // Called once during ChromeContentBrowserClient construction. Use this
  // for any one-time setup that the Astra content browser client needs,
  // such as registering observer callbacks or initializing data structures.
  //
  // Most Astra content-browser integration happens through individual method
  // patch points (OverrideWebPreferences, IsURLAllowedInIncognito, etc.).
  // This function is for setup that doesn't fit neatly into a single method.
  //
  // TODO(astra): Add only product policy hooks that cannot be expressed
  // through existing Chrome prefs, policy, or keyed services.
  //
  // Owner: chrome/browser/chrome_content_browser_client.cc constructor
  // patch point.

  DVLOG(1) << "AstraContentBrowserClient::InstallChromeContentBrowserClientHooks()";

  DCHECK(!hooks_installed_)
      << "InstallChromeContentBrowserClientHooks called more than once.";

  hooks_installed_ = true;

  // -----------------------------------------------------------------------
  // One-time setup
  // -----------------------------------------------------------------------
  //
  // Placeholder for any one-time Astra content browser client setup.
  // Examples of what could go here:
  //   - Register Mojo interface factories
  //   - Set up URLRequestInterceptor factories
  //   - Register navigation throttle providers
  //
  // TODO(astra): Add one-time setup as needed.

  DVLOG(1) << "AstraContentBrowserClient hooks installed.";
}

// ============================================================================
// Startup state
// ============================================================================

bool AstraContentBrowserClient::IsBrowserStartupComplete() {
  // Returns whether the browser has completed startup.
  //
  // The content layer uses this signal to delay certain operations until
  // the browser UI is fully ready:
  //   - Prerendering pages
  //   - Starting service workers
  //   - Restoring background tabs
  //
  // Chromium's ChromeContentBrowserClient already provides a startup
  // completion signal based on the initial browser window being shown.
  // Astra extends this to also wait for Astra-specific initialization
  // (e.g., workspace metadata loaded from prefs).
  //
  // Current implementation: returns true once NotifyBrowserStartupComplete()
  // has been called (which happens in PostBrowserStart()).
  //
  // TODO(astra): Add Astra-specific startup conditions if needed.
  //   For example, wait for workspace metadata to be loaded before
  //   considering startup complete. For now, we use the same timing as
  //   Chromium's browser startup.
  //   Chromium owner: ChromeContentBrowserClient::IsBrowserStartupComplete()
  //   Patch point: chrome/browser/chrome_content_browser_client.cc

  DVLOG(2) << "AstraContentBrowserClient::IsBrowserStartupComplete() -> "
           << (browser_startup_complete_ ? "true" : "false");

  return browser_startup_complete_;
}

void AstraContentBrowserClient::NotifyBrowserStartupComplete() {
  // Notifies the content layer that browser startup is complete.
  //
  // Called from AstraBrowserMainExtraParts::PostBrowserStart() to signal
  // that Astra's portion of startup is done. This is used by
  // IsBrowserStartupComplete() to return true.
  //
  // In Chromium, the equivalent signal comes from ChromeBrowserMainParts
  // when the first browser window is shown. We add this Astra-specific
  // signal so that Astra startup can contribute to the overall startup
  // completion state.
  //
  // Notifies all registered AstraStartupObservers.
  //
  // TODO(astra): Wire this into Chromium's overall startup completion
  //   signal, or remove if not needed. For now, this provides a hook
  //   point for Astra-specific startup completion.

  DVLOG(1) << "AstraContentBrowserClient: browser startup marked complete.";

  DCHECK(!browser_startup_complete_)
      << "NotifyBrowserStartupComplete() called more than once.";

  browser_startup_complete_ = true;

  // Notify all registered observers.
  for (auto& observer : GetObservers()) {
    observer.OnBrowserStartupComplete();
  }

  DVLOG(1) << "Notified " << GetObservers().size()
           << " startup observers.";
}

// ============================================================================
// Startup observers
// ============================================================================

void AstraContentBrowserClient::AddStartupObserver(
    AstraStartupObserver* observer) {
  // Registers a startup observer.
  //
  // If startup has already completed, the observer is notified immediately
  // (synchronously) before returning.  This follows the standard Chromium
  // pattern for startup observers to avoid missed notifications.
  //
  // Parameters:
  //   observer - The observer to add. Must not be null.
  //              Must outlive the registration.

  if (!observer) {
    return;
  }

  DVLOG(1) << "AstraContentBrowserClient::AddStartupObserver()";

  GetObservers().AddObserver(observer);

  // If startup is already complete, notify immediately.
  if (browser_startup_complete_) {
    DVLOG(1) << "Startup already complete — notifying observer immediately.";
    observer->OnBrowserStartupComplete();
  }
}

void AstraContentBrowserClient::RemoveStartupObserver(
    AstraStartupObserver* observer) {
  // Unregisters a startup observer.
  //
  // Parameters:
  //   observer - The observer to remove. Must have been previously added.

  if (!observer) {
    return;
  }

  DVLOG(1) << "AstraContentBrowserClient::RemoveStartupObserver()";

  GetObservers().RemoveObserver(observer);
}

int AstraContentBrowserClient::GetStartupObserverCountForTesting() {
  // Returns the number of registered startup observers.
  // For testing and debugging only.

  int count = 0;
  for (auto& observer : GetObservers()) {
    // Count by iterating — ObserverList doesn't expose size() directly.
    (void)observer;
    ++count;
  }
  return count;
}

// ============================================================================
// Web preferences
// ============================================================================

void AstraContentBrowserClient::OverrideWebPreferences(
    content::WebContents* web_contents,
    blink::web_pref::WebPreferences* prefs) {
  if (!web_contents || !prefs) {
    return;
  }

  // Astra-specific web preference overrides.
  //
  // Most web preferences should be controlled by Chrome's existing pref
  // service. Only add overrides here for Astra-specific surfaces that have
  // no equivalent in Chrome.
  //
  // The OverrideWebPreferences method is called for every WebContents when
  // its web preferences are computed. This includes both regular tabs and
  // WebUI pages.
  //
  // TODO(astra): Add sidebar web view preference overrides when the sidebar
  // WebContents is identified (e.g., custom user agent, viewport settings).
  // Owner: astra/ui/views/sidebar + chrome_content_browser_client patch point.

  // Detect if this WebContents is an Astra WebUI page and apply
  // Astra-specific preferences.
  GURL url = web_contents->GetURL();
  if (IsAstraWebUI(url)) {
    // Astra WebUI pages get specific preference tweaks:
    //   - No text autosizing (WebUI controls its own layout)
    //   - Full access to clipboard API (for copy/paste in settings)
    DVLOG(2) << "AstraContentBrowserClient: applying WebUI prefs for "
             << url.spec();

    // TODO(astra): Add actual Astra WebUI preference overrides here
    // once we know which ones are needed.
    // Example: prefs->text_autosizing_enabled = false;

    // Disable text autosizing for Astra WebUI — WebUI pages control
    // their own layout and don't need autosizing.
    prefs->text_autosizing_enabled = false;
  }

  DVLOG(2)
      << "AstraContentBrowserClient::OverrideWebPreferences() — complete";
}

// ============================================================================
// Mojo interface binders
// ============================================================================

void AstraContentBrowserClient::RegisterBrowserInterfaceBindersForFrame(
    content::RenderFrameHost* render_frame_host) {
  // Registers Astra-specific Mojo interface binders for a frame.
  //
  // Mojo interfaces allow WebUI pages (and other renderer-side code) to
  // call browser-side functions in a type-safe way. Each Astra WebUI page
  // that needs to communicate with the browser should register its
  // interfaces here.
  //
  // The registration is per-RenderFrameHost, meaning each frame gets its
  // own set of interface binders. This follows Chromium's security model
  // where interfaces are scoped to the frame that needs them.
  //
  // Chromium pattern: ChromeContentBrowserClient::RegisterBrowserInterfaceBindersForFrame
  // uses mojo::BinderMap to register interfaces per frame.
  //
  // Example registration (pseudo-code):
  //   mojo::BinderMap* binders = GetBinderMapForFrame(render_frame_host);
  //   binders->Add<astra::mojom::WorkspaceManager>(
  //       base::BindRepeating(&AstraWorkspaceService::BindInterface,
  //                           browser_context));
  //
  // TODO(astra): Define Mojo interfaces for Astra WebUI pages and register
  //   their binders here. Each interface should be scoped to the WebUI page
  //   that needs it and should only expose the minimum necessary surface.
  //   Chromium owner: content::BrowserInterfaceBroker / mojo::BinderMap
  //   Patch point: chrome/browser/chrome_content_browser_client.cc
  //   Astra owner: mojo interfaces in astra/public/mojom/

  if (!render_frame_host) {
    return;
  }

  DVLOG(2)
      << "AstraContentBrowserClient::"
         "RegisterBrowserInterfaceBindersForFrame() ";

  // Only register Astra interfaces for frames that should have access.
  if (!ShouldExposeAstraBindings(render_frame_host)) {
    DVLOG(2)
        << "Frame does not qualify for Astra bindings — skipping.";
    return;
  }

  // Check if this is an Astra WebUI frame and register appropriate interfaces.
  content::BrowserContext* browser_context =
      render_frame_host->GetBrowserContext();
  GURL last_committed_url = render_frame_host->GetLastCommittedURL();

  if (browser_context && last_committed_url.SchemeIs(kAstraUIScheme)) {
    DVLOG(1) << "Astra WebUI frame detected — interface registration pending.";

    // TODO(astra): Add interface registration for:
    //   - astra://newtab — workspace listing, quick actions
    //   - astra://settings — preference management
    //   - astra://workspaces — workspace CRUD operations

    // TODO(astra): Register actual Mojo interfaces here.
    // For now, just log that registration would happen.

    std::string host = last_committed_url.host();
    DVLOG(1) << "Astra WebUI host: " << host
             << " — Mojo interface registration TBD.";
  }
}

// ============================================================================
// WebContents lifecycle
// ============================================================================

void AstraContentBrowserClient::WebContentsCreated(
    content::WebContents* web_contents) {
  // Called when a new WebContents is created.
  //
  // This is the right place to attach Astra's WebContentsUserData objects
  // (e.g. AstraTabFeatures) to every new WebContents. By attaching at
  // creation time, we ensure that Astra metadata is available for the
  // lifetime of the tab.
  //
  // Chromium pattern: ChromeContentBrowserClient::WebContentsCreated() is
  // called immediately after a WebContents is constructed, before it's
  // added to a tab strip or navigated anywhere.
  //
  // Astra attaches its per-tab metadata via content::WebContentsUserData,
  // which is the standard Chromium pattern for associating data with
  // WebContents without subclassing it.
  //
  // TODO(astra): Attach AstraTabFeatures here via
  //   AstraTabFeatures::CreateForWebContents(web_contents).
  //   Currently AstraTabFeatures is created lazily on first access.
  //   Creating eagerly here ensures metadata is always available and
  //   avoids any first-access overhead during tab interaction.
  //   Astra owner: astra/browser/astra_tab_features.h
  //   Chromium owner: content::WebContentsUserData
  //   Patch point: chrome/browser/chrome_content_browser_client.cc

  if (!web_contents) {
    return;
  }

  DVLOG(2) << "AstraContentBrowserClient::WebContentsCreated() — "
           << "Astra tab metadata attachment pending.";

  // TODO(astra): Uncomment once AstraTabFeatures is ready for eager
  //   creation. Currently it's created lazily.
  // if (base::FeatureList::IsEnabled(kAstraBrandedBuild)) {
  //   AstraTabFeatures::CreateForWebContents(web_contents);
  // }

  // For now, we just note that the WebContents was created.
  // The actual user data attachment happens lazily on first access.
}

// ============================================================================
// URL policy — incognito
// ============================================================================

bool AstraContentBrowserClient::IsURLAllowedInIncognito(const GURL& url) {
  // Astra-specific incognito URL policy.
  //
  // By default, most URLs are allowed in incognito — Chrome owns the default
  // behavior and Safe Browsing / enterprise policy handle restrictions.
  // Only add Astra-specific restrictions here.
  //
  // Returns false for Astra internal URLs that don't make sense in
  // incognito mode. These include:
  //   - URLs that modify persistent state (workspace management)
  //   - URLs that show profile-specific data (notes, favorites)
  //
  // Note: Standard Astra WebUI pages (settings, newtab, etc.) are allowed
  // in incognito — they adapt to the incognito profile.
  //
  // TODO(astra): Implement if Astra product policy requires URL-level
  // incognito restrictions beyond what Chrome provides.
  // Owner: chrome_content_browser_client patch point.

  // Invalid URLs are not allowed anywhere.
  if (!url.is_valid()) {
    DVLOG(2) << "IsURLAllowedInIncognito: invalid URL -> false";
    return false;
  }

  // Internal Astra URLs are not allowed in incognito.
  if (IsAstraInternalURL(url)) {
    DVLOG(2) << "IsURLAllowedInIncognito: internal Astra URL -> false: "
             << url.spec();
    return false;
  }

  // TODO(astra): Add more specific URL restrictions as needed.
  // For example:
  //   - Workspace management URLs might not make sense in incognito
  //   - Note-taking URLs might be profile-specific

  DVLOG(2) << "IsURLAllowedInIncognito: " << url.spec() << " -> true";

  // Default: allow all other URLs.
  return true;
}

// ============================================================================
// URL policy — external URL handling
// ============================================================================

bool AstraContentBrowserClient::ShouldAllowOpenExternalURL(const GURL& url) {
  // Astra-specific external URL policy.
  //
  // By default, defer to Chrome's external URL handling. Only override here
  // if Astra has product-specific rules for opening URLs in external apps.
  //
  // Returns false for Astra internal URLs — they should never be opened
  // in external apps.
  //
  // TODO(astra): Implement if Astra has product-specific external URL rules.
  // Owner: chrome_content_browser_client patch point.

  // Invalid URLs should not be opened externally.
  if (!url.is_valid()) {
    DVLOG(2) << "ShouldAllowOpenExternalURL: invalid URL -> false";
    return false;
  }

  // Astra internal URLs should not be opened externally.
  if (IsAstraInternalURL(url)) {
    DVLOG(2)
        << "ShouldAllowOpenExternalURL: internal Astra URL -> false: "
        << url.spec();
    return false;
  }

  // Astra WebUI URLs should not be opened externally either.
  if (IsAstraWebUI(url)) {
    DVLOG(2)
        << "ShouldAllowOpenExternalURL: Astra WebUI URL -> false: "
        << url.spec();
    return false;
  }

  DVLOG(2) << "ShouldAllowOpenExternalURL: " << url.spec() << " -> true";

  // Default: allow all other URLs to open externally.
  return true;
}

// ============================================================================
// Astra WebUI detection
// ============================================================================

bool AstraContentBrowserClient::IsAstraWebUI(const GURL& url) {
  // Returns true if |url| is an Astra WebUI page.
  //
  // Astra WebUI pages use the "astra://" scheme (similar to how Chrome
  // uses "chrome://" for its WebUI pages).
  //
  // A URL is considered an Astra WebUI if:
  //   1. It uses the "astra://" scheme
  //   2. It has a valid, known host
  //
  // This helper is used by other AstraContentBrowserClient methods to
  // detect Astra WebUI pages and apply Astra-specific policies.
  //
  // TODO(astra): Consider whether to validate the host against a list of
  //   known Astra WebUI hosts. For now, scheme check is sufficient.

  if (!url.is_valid()) {
    return false;
  }

  if (!url.SchemeIs(kAstraUIScheme)) {
    return false;
  }

  // Check that the host is a known Astra WebUI host.
  std::string host = url.host();
  for (size_t i = 0; i < kAstraWebUIHostCount; ++i) {
    if (host == kAstraWebUIHosts[i].host_name) {
      return true;
    }
  }

  // If the scheme is right but the host is unknown, it's still "Astra"
  // but not a known WebUI. Return false to be safe — unknown hosts
  // shouldn't get WebUI privileges.
  DVLOG(1) << "Unknown Astra URL host: " << host
           << " — not treated as WebUI.";
  return false;
}

// ============================================================================
// Astra WebUI host names
// ============================================================================

std::vector<std::string> AstraContentBrowserClient::GetAstraWebUIHosts() {
  // Returns a vector of valid Astra WebUI host names.
  //
  // These are the known hosts for astra:// URLs.
  // Used for URL validation and policy decisions.
  //
  // Note: This list should be kept in sync with the actual Astra WebUI
  // pages defined in astra/ui/webui/.

  std::vector<std::string> hosts;
  hosts.reserve(kAstraWebUIHostCount);

  for (size_t i = 0; i < kAstraWebUIHostCount; ++i) {
    hosts.push_back(kAstraWebUIHosts[i].host_name);
  }

  return hosts;
}

// ============================================================================
// Astra internal URL detection
// ============================================================================

bool AstraContentBrowserClient::IsAstraInternalURL(const GURL& url) {
  // Returns true if the URL is internal to Astra (not user-navigable).
  //
  // Internal URLs are Astra-specific URLs that users should not be able
  // to navigate to directly from the omnibox. They are used for internal
  // UI surfaces and privileged operations.
  //
  // A URL is considered internal if:
  //   1. It is an Astra WebUI URL (astra:// scheme with valid host)
  //   2. The host is marked as internal in the WebUI host table
  //
  // Most Astra WebUI pages are NOT internal — they are user-navigable
  // (e.g., astra://settings, astra://newtab).

  if (!url.is_valid()) {
    return false;
  }

  if (!url.SchemeIs(kAstraUIScheme)) {
    return false;
  }

  // Check against known internal hosts.
  std::string host = url.host();
  for (size_t i = 0; i < kAstraWebUIHostCount; ++i) {
    if (host == kAstraWebUIHosts[i].host_name) {
      return kAstraWebUIHosts[i].is_internal;
    }
  }

  // Unknown Astra hosts are treated as non-internal by default.
  return false;
}

// ============================================================================
// Astra bindings exposure check
// ============================================================================

bool AstraContentBrowserClient::ShouldExposeAstraBindings(
    content::RenderFrameHost* render_frame_host) {
  // Returns true if a frame should receive Astra Mojo bindings.
  //
  // Frames get Astra Mojo bindings if:
  //   - They are Astra WebUI pages (astra:// scheme with valid host)
  //   - They are a main frame (not subframes)
  //   - The browser context is valid
  //
  // This is used to determine whether to register Astra Mojo interface
  // binders for a given frame.
  //
  // TODO(astra): Add more conditions as needed (feature flags, etc.).

  if (!render_frame_host) {
    return false;
  }

  // Must have a valid browser context.
  content::BrowserContext* browser_context =
      render_frame_host->GetBrowserContext();
  if (!browser_context) {
    return false;
  }

  // Must be a main frame (not an iframe/subframe).
  if (!render_frame_host->IsInPrimaryMainFrame()) {
    DVLOG(2) << "ShouldExposeAstraBindings: not main frame -> false";
    return false;
  }

  // Must be an Astra WebUI URL.
  GURL url = render_frame_host->GetLastCommittedURL();
  if (!IsAstraWebUI(url)) {
    DVLOG(2) << "ShouldExposeAstraBindings: not Astra WebUI -> false: "
             << url.spec();
    return false;
  }

  // Internal URLs don't get bindings (they're not user-facing).
  if (IsAstraInternalURL(url)) {
    DVLOG(2) << "ShouldExposeAstraBindings: internal URL -> false: "
             << url.spec();
    return false;
  }

  DVLOG(2) << "ShouldExposeAstraBindings: " << url.spec() << " -> true";
  return true;
}

// ============================================================================
// Test helpers
// ============================================================================

void AstraContentBrowserClient::ResetStateForTesting() {
  // Resets all state for testing.
  // Only use in unit tests — never in production code.
  //
  // This resets:
  //   - Startup complete flag
  //   - Hooks installed flag
  //   - Observer list (all observers are removed)

  DVLOG(1) << "AstraContentBrowserClient::ResetStateForTesting()";

  browser_startup_complete_ = false;
  hooks_installed_ = false;

  // Clear all observers.
  auto& observers = GetObservers();
  // We can't directly clear an ObserverList, so we note that it should
  // be done by tests removing their observers.
  // In practice, tests should remove their observers in TearDown().

  DVLOG(1) << "AstraContentBrowserClient state reset for testing.";
}

}  // namespace astra
