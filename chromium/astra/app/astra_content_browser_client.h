// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_APP_ASTRA_CONTENT_BROWSER_CLIENT_H_
#define ASTRA_APP_ASTRA_CONTENT_BROWSER_CLIENT_H_

// Thin patch-point helper for ChromeContentBrowserClient integration.
//
// IMPORTANT: This class does NOT replace ChromeContentBrowserClient. It is
// called from small patch points in chrome/browser/chrome_content_browser_client.cc
// to add Astra-specific product policy overrides.
//
// Keep this class tiny: ChromeContentBrowserClient owns 99% of browser
// behavior — network, permissions, downloads, file access, security defaults,
// WebUI registration, and renderer process management.
//
// Astra only adds:
//   - Product-specific policy overrides that cannot be expressed via prefs
//     or enterprise policy configuration.
//   - Astra-specific web preference defaults (sidebar web view tweaks, etc.)
//   - Product URL policy for incognito / guest modes.
//   - Mojo interface exposure for Astra WebUI pages.
//   - WebContents creation hooks for Astra tab metadata.
//
// All methods are static — this is a patch helper, not a class instance.
//
// Chromium subsystem reused:
//   - content::ContentBrowserClient — the full browser client interface
//   - ChromeContentBrowserClient — Chrome's implementation of it
//   - Mojo interface binder system — for WebUI <-> browser communication
//
// Patch points:
//   - chrome/browser/chrome_content_browser_client.cc (constructor, methods)
//   - content/public/browser/content_browser_client.h (interface definition)

#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"

namespace blink {
namespace web_pref {
struct WebPreferences;
}  // namespace web_pref
}  // namespace blink

namespace content {
class BrowserContext;
class RenderFrameHost;
class WebContents;
}  // namespace content

class GURL;

namespace astra {

// ============================================================================
// AstraStartupObserver
// ============================================================================
//
// Observer interface for browser startup completion events.
//
// Observers are notified when the browser has fully completed startup,
// including Astra-specific initialization (workspace metadata loaded,
// UI surfaces ready).
//
// This follows the standard Chromium observer pattern using
// base::CheckedObserver for safe iteration during removal.
//
// Typical use cases:
//   - Deferring operations that need a fully-initialized browser
//   - Recording startup metrics
//   - Starting background services after the UI is ready
// ============================================================================

class AstraStartupObserver : public base::CheckedObserver {
 public:
  // Called when the browser has completed startup.
  //
  // At this point:
  //   - The initial browser window is visible
  //   - All Astra services are initialized
  //   - The main message loop is running
  //   - Profile and keyed services are ready
  //
  // The observer is called on the main (UI) thread.
  virtual void OnBrowserStartupComplete() = 0;

 protected:
  ~AstraStartupObserver() override = default;
};

// ============================================================================
// AstraContentBrowserClient
// ============================================================================

class AstraContentBrowserClient {
 public:
  AstraContentBrowserClient() = delete;

  // -- Installation -------------------------------------------------------

  // Installs Astra hooks into ChromeContentBrowserClient. Called once during
  // browser process initialization from the ChromeContentBrowserClient
  // constructor patch point.
  // TODO(astra): Wire into chrome/browser/chrome_content_browser_client.cc
  // constructor under an Astra branding flag.
  static void InstallChromeContentBrowserClientHooks();

  // -- Startup state ------------------------------------------------------

  // Returns true if the browser has completed startup.
  //
  // Called from ChromeContentBrowserClient::IsBrowserStartupComplete()
  // patch point. The content layer uses this to delay certain operations
  // (e.g. prerendering, service workers) until the browser UI is fully
  // initialized.
  //
  // Chromium owns the overall startup signal; Astra only adds its own
  // startup completion state (e.g. workspace metadata loaded).
  //
  // TODO(astra): Integrate with Chromium's startup completion signal.
  // Patch point: chrome/browser/chrome_content_browser_client.cc
  // Chromium owner: content::ContentBrowserClient::IsBrowserStartupComplete()
  static bool IsBrowserStartupComplete();

  // Notifies the Astra layer that browser startup is complete.
  //
  // Called from ChromeBrowserMainExtraParts::PostBrowserStart() to signal
  // that the browser UI is fully initialized. This is used by
  // IsBrowserStartupComplete() to return true after startup.
  //
  // Notifies all registered AstraStartupObservers.
  //
  // TODO(astra): Wire this into the browser startup completion path.
  static void NotifyBrowserStartupComplete();

  // -- Startup observers --------------------------------------------------

  // Registers a startup observer.
  //
  // If startup has already completed, the observer's OnBrowserStartupComplete()
  // will be called synchronously before AddStartupObserver returns.
  //
  // The observer must outlive the registration (or be removed before
  // destruction).
  static void AddStartupObserver(AstraStartupObserver* observer);

  // Unregisters a startup observer.
  static void RemoveStartupObserver(AstraStartupObserver* observer);

  // Returns the number of registered startup observers.
  // For testing and debugging only.
  static int GetStartupObserverCountForTesting();

  // -- Web preferences ----------------------------------------------------

  // Overrides web preferences for Astra-specific surfaces. Called from
  // ChromeContentBrowserClient::OverrideWebPreferences() patch point.
  //
  // Use this sparingly. Most web prefs should be controlled through existing
  // Chrome prefs or enterprise policy. Only override here for Astra-specific
  // surfaces that have no Chrome equivalent (e.g., sidebar web view defaults).
  //
  // TODO(astra): Implement sidebar web preference defaults once the sidebar
  // view has WebContents. Owner: chrome_content_browser_client patch point.
  static void OverrideWebPreferences(content::WebContents* web_contents,
                                     blink::web_pref::WebPreferences* prefs);

  // -- Mojo interface binders ---------------------------------------------

  // Registers Astra-specific Mojo interface binders for a frame.
  //
  // Called from ChromeContentBrowserClient::RegisterBrowserInterfaceBindersForFrame()
  // patch point. This allows Astra WebUI pages to call browser-side Mojo
  // interfaces (e.g., workspace management, sidebar control).
  //
  // Each Astra WebUI page that needs browser communication should register
  // its Mojo interfaces here. The binders are scoped to a RenderFrameHost,
  // following Chromium's per-frame Mojo binder pattern.
  //
  // Chromium pattern: ChromeContentBrowserClient::RegisterBrowserInterfaceBindersForFrame
  // uses mojo::BinderMap to register interfaces per frame.
  //
  // TODO(astra): Add Mojo interfaces for Astra WebUI pages
  //   (astra://newtab, astra://settings, etc.).
  //   Patch point: chrome/browser/chrome_content_browser_client.cc
  //   Chromium owner: mojo::BinderMap / content::BrowserInterfaceBroker
  static void RegisterBrowserInterfaceBindersForFrame(
      content::RenderFrameHost* render_frame_host);

  // -- WebContents lifecycle ----------------------------------------------

  // Called when a new WebContents is created.
  //
  // Called from ChromeContentBrowserClient::WebContentsCreated() patch point.
  // This is the hook where Astra attaches tab-scoped metadata (via
  // content::WebContentsUserData) to every new WebContents.
  //
  // Chromium owns WebContents creation and lifecycle. Astra only adds
  // its own metadata user data objects to the WebContents.
  //
  // TODO(astra): Attach AstraTabFeatures (WebContentsUserData) here when
  //   a WebContents is created.
  //   Astra owner: astra/browser/astra_tab_features.h
  //   Patch point: chrome/browser/chrome_content_browser_client.cc
  //   Chromium owner: content::WebContents / content::WebContentsUserData
  static void WebContentsCreated(content::WebContents* web_contents);

  // -- URL policy ---------------------------------------------------------

  // Returns true if the given URL is allowed to load in an incognito window.
  // Called from ChromeContentBrowserClient::IsURLAllowedInIncognito() patch.
  //
  // Most URL restrictions should go through Safe Browsing or enterprise
  // policy. Only use this for Astra product-specific restrictions that
  // cannot be expressed through existing Chrome mechanisms.
  //
  // Returns false for Astra internal URLs that don't make sense in
  // incognito mode (e.g., workspace management pages).
  //
  // TODO(astra): Implement if Astra has product-specific incognito URL rules.
  // Owner: chrome_content_browser_client patch point.
  static bool IsURLAllowedInIncognito(const GURL& url);

  // Returns true if |url| should be allowed to be opened externally from
  // the browser. Called from ChromeContentBrowserClient::ShouldAllowOpenExternal.
  //
  // Returns false for Astra internal URLs (they shouldn't be opened in
  // external apps).
  //
  // TODO(astra): Implement if Astra has product-specific external URL rules.
  // Owner: chrome_content_browser_client patch point.
  static bool ShouldAllowOpenExternalURL(const GURL& url);

  // -- Astra WebUI detection ----------------------------------------------

  // Returns true if the given WebUI URL scheme/host is an Astra WebUI.
  //
  // This is used by the patch point in ChromeContentBrowserClient to
  // identify Astra WebUI pages and apply Astra-specific policies (e.g.,
  // CSP, bindings).
  //
  // A URL is considered an Astra WebUI if it uses the "astra://" scheme
  // and has a known valid host.
  //
  // TODO(astra): Use this in WebUI-related patches to identify Astra pages.
  static bool IsAstraWebUI(const GURL& url);

  // Returns a vector of valid Astra WebUI host names.
  //
  // These are the known hosts for astra:// URLs (e.g., "newtab", "settings").
  // Used for URL validation and policy decisions.
  //
  // Note: This list should be kept in sync with the actual Astra WebUI
  // pages defined in astra/ui/webui/.
  static std::vector<std::string> GetAstraWebUIHosts();

  // Returns true if the URL is internal to Astra (not user-navigable).
  //
  // Internal URLs are Astra-specific URLs that users should not be able
  // to navigate to directly from the omnibox. They are used for internal
  // UI surfaces and privileged operations.
  //
  // Examples of internal URLs:
  //   - astra://workspaces-internal/
  //   - astra://settings-internal/
  //
  // Most Astra WebUI pages are NOT internal — they are user-navigable
  // (e.g., astra://settings, astra://newtab).
  static bool IsAstraInternalURL(const GURL& url);

  // Returns true if a frame should receive Astra Mojo bindings.
  //
  // Frames get Astra Mojo bindings if:
  //   - They are Astra WebUI pages (astra:// scheme with valid host)
  //   - They are in a normal browser context (not incognito-only restrictions)
  //   - The feature is enabled
  //
  // This is used to determine whether to register Astra Mojo interface
  // binders for a given frame.
  static bool ShouldExposeAstraBindings(
      content::RenderFrameHost* render_frame_host);

  // -- Test helpers -------------------------------------------------------

  // Resets all state for testing.
  // Only use in unit tests — never in production code.
  static void ResetStateForTesting();

 private:
  // Tracks whether browser startup has completed.
  // Set to true by NotifyBrowserStartupComplete().
  static bool browser_startup_complete_;

  // Tracks whether hooks have been installed.
  static bool hooks_installed_;

  // Returns the static observer list.
  static base::ObserverList<AstraStartupObserver>& GetObservers();
};

}  // namespace astra

#endif  // ASTRA_APP_ASTRA_CONTENT_BROWSER_CLIENT_H_
