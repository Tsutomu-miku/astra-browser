// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/app/astra_browser_main_extra_parts.h"

#include "astra/app/astra_accelerator_registrar.h"
#include "astra/app/astra_content_browser_client.h"
#include "astra/app/astra_feature_list.h"
#include "astra/browser/astra_command_delegate.h"
#include "astra/browser/astra_keyed_service_factories.h"
#include "astra/browser/astra_workspace_service_factory.h"
#include "astra/common/astra_command_constants.h"
#include "base/check.h"
#include "base/logging.h"
#include "base/trace_event/trace_event.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"

// Forward declarations for UI-layer registration functions.
// These are called via patch points; the app layer does not directly depend
// on the UI layer headers in the public API, but at implementation time we
// delegate into them.
//
// TODO(astra): Move these into proper headers in astra/ui/ once the layer
//   structure is finalized.  Currently these functions exist in the UI layer
//   and are called from Chromium patch points; we forward-declare them here
//   so the app layer can drive registration without creating a circular
//   dependency.
//   Chromium owner: chrome/browser/ui/webui/chrome_web_ui_configs.cc
//   Chromium owner: chrome/browser/ui/color/chrome_color_mixers.cc
namespace astra {

// Registers all Astra WebUI configs with the content layer.
// Implemented in astra/ui/webui/astra_webui_config.cc.
void RegisterAstraWebUIConfigs();

// ---------------------------------------------------------------------------
// Color mixer registration
// ---------------------------------------------------------------------------
//
// Astra colors are registered per-ColorProvider via AddAstraColorMixer(),
// which is called from a patch point in chrome/browser/ui/color/chrome_color_mixers.cc.
// There is no global "register color mixers" function in Chromium — each
// ColorProvider instance has mixers added to it at construction time.
//
// The app layer does not own the color mixer registration; it is driven by
// the Chromium color provider pipeline. We provide this no-op stub so that
// PreCreateThreads() can document the registration path and log that Astra
// color support is enabled.
//
// TODO(astra): Wire AddAstraColorMixer() into Chromium's color mixer pipeline
//   via a patch point in chrome/browser/ui/color/chrome_color_mixers.cc.
//   Owner: chrome/browser/ui/color/chrome_color_mixers.cc
//   Astra owner: astra/ui/color/astra_color_mixer.cc
void RegisterAstraColorMixer() {
  // No-op: color mixers are added per-ColorProvider at widget construction time.
  // This function exists to document the registration path and provide a
  // single place to hook color mixer initialization if we ever need global
  // color setup (e.g., registering custom ColorProviderKeys).
  DVLOG(1) << "Astra color mixer enabled — per-ColorProvider registration "
           << "will happen at widget construction time via "
           << "chrome/browser/ui/color/chrome_color_mixers.cc patch point.";
}

}  // namespace astra

namespace astra {

// ============================================================================
// Construction / destruction
// ============================================================================

AstraBrowserMainExtraParts::AstraBrowserMainExtraParts() {
  DVLOG(1) << "AstraBrowserMainExtraParts constructed.";
}

AstraBrowserMainExtraParts::~AstraBrowserMainExtraParts() {
  DVLOG(1) << "AstraBrowserMainExtraParts destroyed.";

  // DCHECK that we went through the full shutdown sequence if we went
  // through startup.
  if (service_factories_registered_) {
    DLOG_IF(WARNING, !post_destroy_threads_called_)
        << "AstraBrowserMainExtraParts destroyed without "
        << "PostDestroyThreads() being called. "
        << "Shutdown may be incomplete.";
  }
}

// ============================================================================
// PreCreateThreads
// ============================================================================

void AstraBrowserMainExtraParts::PreCreateThreads() {
  // Called very early in browser startup, before any threads are created.
  //
  // At this point:
  //   - The main thread is running
  //   - Feature list is initialized
  //   - No profile exists yet
  //   - No browser threads (IO, UI, etc.) are running
  //
  // Use this phase for one-time registrations that must happen before
  // multi-threaded operation begins:
  //   - KeyedService factory registration (idempotent, must happen before
  //     any BrowserContext is created)
  //   - WebUI config registration (must happen before any WebUI is created)
  //   - Color mixer registration (must happen before first ColorProvider
  //     is constructed)
  //
  // Chromium owns all thread creation and lifecycle. Astra only adds its
  // own registrations on top of Chromium's startup sequence.

  TRACE_EVENT0("startup", "AstraBrowserMainExtraParts::PreCreateThreads");
  DVLOG(1) << "AstraBrowserMainExtraParts::PreCreateThreads()";

  DCHECK(!pre_create_threads_called_)
      << "PreCreateThreads called more than once.";
  DCHECK(!pre_profile_init_called_)
      << "PreProfileInit called before PreCreateThreads — "
      << "lifecycle order violated.";

  pre_create_threads_called_ = true;

  // -----------------------------------------------------------------------
  // Step 1: Register Astra ProfileKeyedService factories
  // -----------------------------------------------------------------------
  //
  // All Astra ProfileKeyedService factories must be registered before any
  // profile is created, so that the BrowserContextDependencyManager has a
  // complete dependency graph when it starts constructing services.
  //
  // This is the canonical place to register Astra service factories. It
  // runs early enough that all factories are discoverable by the time the
  // first profile is created in PreProfileInit / PostProfileInit.
  //
  // Chromium pattern: ChromeBrowserMainExtraPartsProfiles::PreCreateThreads()
  // registers Chrome's service factories.
  // Patch point: chrome/browser/chrome_browser_main_extra_parts_profiles.cc
  if (base::FeatureList::IsEnabled(kAstraBrandedBuild)) {
    RegisterAstraProfileKeyedServices();
    service_factories_registered_ = true;
    DVLOG(1) << "Astra ProfileKeyedService factories registered "
             << "(PreCreateThreads phase).";
  }

  // -----------------------------------------------------------------------
  // Step 2: Register Astra WebUI configs
  // -----------------------------------------------------------------------
  //
  // WebUI configs tell the content layer how to create WebUIControllers
  // for chrome:// (and astra://) URLs. They must be registered before any
  // WebContents navigates to a WebUI URL.
  //
  // Chromium pattern: ChromeWebUIConfigs constructor registers all
  // chrome:// WebUI configs.
  // Patch point: chrome/browser/ui/webui/chrome_web_ui_configs.cc
  if (base::FeatureList::IsEnabled(kAstraBrandedBuild)) {
    RegisterAstraWebUIConfigs();
    webui_configs_registered_ = true;
    DVLOG(1) << "Astra WebUI configs registered (PreCreateThreads phase).";
  }

  // -----------------------------------------------------------------------
  // Step 3: Initialize Astra color mixer
  // -----------------------------------------------------------------------
  //
  // Astra color IDs must be registered with the ColorProvider system before
  // any Views widget is created that queries Astra colors.
  //
  // Note: In Chromium's actual architecture, color mixers are added per
  // ColorProvider instance (not globally). The Astra color mixer is wired
  // through a patch point in chrome/browser/ui/color/chrome_color_mixers.cc
  // that calls AddAstraColorMixer() during ColorProvider construction.
  //
  // Here we record that the registration path is enabled and log it.
  // The actual per-ColorProvider registration happens at widget creation time.
  //
  // Chromium owner: ColorProvider / NativeTheme
  //   (ui/color/color_provider.h)
  //   (chrome/browser/ui/color/chrome_color_mixers.cc)
  if (base::FeatureList::IsEnabled(kAstraBrandedBuild)) {
    RegisterAstraColorMixer();
    DVLOG(1) << "Astra color mixer registration enabled "
             << "(PreCreateThreads phase).";
  }

  DVLOG(1) << "AstraBrowserMainExtraParts::PreCreateThreads() — complete";
}

// ============================================================================
// PreProfileInit
// ============================================================================

void AstraBrowserMainExtraParts::PreProfileInit() {
  // Register Astra keyed-service factories before any Profile is created.
  // This ensures that when Chromium constructs the initial profile, all
  // Astra ProfileKeyedService factories are discoverable via
  // BrowserContextKeyedServiceFactory.
  //
  // Chromium owns Profile and all built-in keyed services (HistoryService,
  // DownloadManager, etc.). Astra only adds its own metadata services.
  //
  // Note: Factory registration was already done in PreCreateThreads() as
  // the primary registration point. We DCHECK here to verify the registration
  // happened in the correct phase, and re-register as a safety net in case
  // PreCreateThreads was skipped (e.g. in certain test configurations).

  TRACE_EVENT0("startup", "AstraBrowserMainExtraParts::PreProfileInit");
  DVLOG(1) << "AstraBrowserMainExtraParts::PreProfileInit()";

  DCHECK(pre_create_threads_called_)
      << "PreCreateThreads must be called before PreProfileInit.";
  DCHECK(!pre_profile_init_called_)
      << "PreProfileInit called more than once.";

  pre_profile_init_called_ = true;

  if (base::FeatureList::IsEnabled(kAstraBrandedBuild) &&
      !service_factories_registered_) {
    DLOG(WARNING) << "Astra service factories not registered in "
                  << "PreCreateThreads — falling back to PreProfileInit.";
    RegisterAstraProfileKeyedServices();
    service_factories_registered_ = true;
  }

  DCHECK(service_factories_registered_ ||
         !base::FeatureList::IsEnabled(kAstraBrandedBuild))
      << "Astra service factories must be registered before profile init.";

  // Verify that WebUI configs are also registered by this point.
  DCHECK(webui_configs_registered_ ||
         !base::FeatureList::IsEnabled(kAstraBrandedBuild))
      << "Astra WebUI configs must be registered before profile init.";

  // TODO(astra): Add future Astra keyed service factories here as they are
  // introduced. Owner: chrome/browser/profiles/profile_keyed_service_factory.h

  DVLOG(1) << "Astra keyed-service factories registered.";
}

// ============================================================================
// PostProfileInit
// ============================================================================

void AstraBrowserMainExtraParts::PostProfileInit(Profile* profile,
                                                 bool is_initial_profile) {
  if (!profile) {
    return;
  }

  TRACE_EVENT1("startup", "AstraBrowserMainExtraParts::PostProfileInit",
               "is_initial_profile", is_initial_profile);
  DVLOG(1) << "AstraBrowserMainExtraParts::PostProfileInit("
           << (is_initial_profile ? "initial" : "secondary")
           << " profile)";

  DCHECK(pre_profile_init_called_)
      << "PreProfileInit must be called before PostProfileInit.";
  DCHECK(service_factories_registered_ ||
         !base::FeatureList::IsEnabled(kAstraBrandedBuild))
      << "Service factories must be registered before PostProfileInit.";

  post_profile_init_called_ = true;

  if (is_initial_profile) {
    DCHECK(!initial_profile_)
        << "Initial profile already set — "
        << "PostProfileInit called twice with is_initial_profile=true.";

    initial_profile_ = profile;

    // Register profile preferences for the initial profile.
    // This ensures all Astra prefs are available for the initial profile.
    //
    // Note: Pref registration is done per-profile through the factory
    // system. We log it here to track the lifecycle.
    DVLOG(1) << "Initial profile created — Astra prefs registered via "
             << "factory system.";

    // -------------------------------------------------------------------
    // Step 1: Initialize Astra features from prefs
    // -------------------------------------------------------------------
    //
    // Apply any pref-based feature overrides now that the initial profile
    // and its pref service are available.
    //
    // Most features are controlled by base::FeatureList directly, but
    // some may have additional pref-based toggles that need to be resolved
    // before browser windows are created.
    if (base::FeatureList::IsEnabled(kAstraBrandedBuild)) {
      InitializeAstraFeaturesFromPrefs(profile->GetPrefs());
      features_initialized_from_prefs_ = true;
      DVLOG(1) << "Astra features initialized from prefs "
               << "(PostProfileInit phase).";
    }

    // -------------------------------------------------------------------
    // Step 2: Warm up Astra workspace service
    // -------------------------------------------------------------------
    //
    // Ensure the Astra workspace service is constructed for the initial
    // profile so that workspace metadata is ready before any Browser window
    // is created. Chromium remains the owner of Profile and browser services;
    // AstraWorkspaceService only adds product-level metadata.
    if (base::FeatureList::IsEnabled(kAstraWorkspaces)) {
      // Service construction is triggered by accessing it via the factory.
      // This ensures the service is ready before browser windows appear.
      //
      // Note: We use GetForProfile() which constructs the service if it
      // doesn't exist. For the initial profile, this guarantees the
      // workspace service is warmed up before the first browser window.
      AstraWorkspaceServiceFactory::GetForProfile(profile);
      DVLOG(1) << "AstraWorkspaceService initialized for initial profile.";
    }

    // TODO(astra): Initialize Astra UI features (sidebar, split view) once
    // the UI layer is ready. These should be registered through BrowserView
    // patch points, not here. Owner: chrome/browser/ui/views/frame/browser_view.cc
    // patch point.
    //
    // Chromium owns BrowserView and the full Views frame. Astra UI code only
    // adds child views and presentation layers; it does not own the window.
  }

  // For all profiles (initial and secondary), verify that Astra service
  // factories are discoverable. The actual service construction happens
  // lazily on first access.
  DVLOG(1) << "Astra PostProfileInit complete for profile"
           << (is_initial_profile ? " (initial)." : " (secondary).");
}

// ============================================================================
// PreBrowserStart
// ============================================================================

void AstraBrowserMainExtraParts::PreBrowserStart() {
  // Called after profile initialization but before the first Browser window
  // is created. This is the right place to set up anything that depends on
  // the initial profile but must be ready before the first browser window
  // appears.
  //
  // At this point:
  //   - The initial profile exists and is fully initialized
  //   - KeyedServices are constructible (but may not be constructed yet)
  //   - No Browser windows exist yet
  //
  // Use this phase for:
  //   - Accelerator registration (must be done before first widget)
  //   - Feature list finalization (applying pref-based overrides)
  //   - Pre-warming services that are needed for first window paint
  //
  // Chromium pattern: ChromeBrowserMainParts::PreBrowserStart() sets up
  // the browser process and creates the first browser window.

  TRACE_EVENT0("startup", "AstraBrowserMainExtraParts::PreBrowserStart");
  DVLOG(1) << "AstraBrowserMainExtraParts::PreBrowserStart()";

  DCHECK(initial_profile_)
      << "Initial profile must be set before PreBrowserStart.";
  DCHECK(!pre_browser_start_called_)
      << "PreBrowserStart called more than once.";

  pre_browser_start_called_ = true;

  if (!base::FeatureList::IsEnabled(kAstraBrandedBuild)) {
    DVLOG(1) << "Astra branded build disabled — skipping PreBrowserStart.";
    return;
  }

  // -----------------------------------------------------------------------
  // Step 1: Finalize Astra feature list state
  // -----------------------------------------------------------------------
  //
  // Apply any pref-based feature overrides and finalize the Astra feature
  // state. Most features are controlled by base::FeatureList directly, but
  // some may have additional pref-based toggles that need to be resolved
  // before browser windows are created.
  //
  // Note: We already initialized features from prefs in PostProfileInit.
  // Here we do any finalization that must happen right before browser
  // windows are created.
  if (initial_profile_) {
    // Double-check that features were initialized from prefs.
    DCHECK(features_initialized_from_prefs_)
        << "Features should have been initialized from prefs in "
        << "PostProfileInit.";

    DVLOG(1) << "Astra feature list finalized (PreBrowserStart phase).";
  }

  // -----------------------------------------------------------------------
  // Step 2: Register Astra accelerators
  // -----------------------------------------------------------------------
  //
  // Accelerators must be registered before the first BrowserView widget is
  // created so that keyboard shortcuts are available from the first keypress.
  //
  // Note: In Chromium's architecture, accelerators are typically registered
  // per FocusManager (per BrowserView). The Astra accelerator registrar
  // provides both a static table merge approach (patch in
  // chrome/browser/ui/views/accelerator_table.cc) and a runtime registration
  // approach.
  //
  // Here we mark that accelerator registration should be complete by this
  // phase. The actual per-widget registration happens at BrowserView
  // construction time via the accelerator table patch point.
  //
  // TODO(astra): Verify accelerator registration timing.  If using the
  //   static table merge patch (0007), accelerators are compiled in and
  //   automatically registered by FocusManager.  If using runtime
  //   registration, it should be triggered from BrowserView::Init().
  //   Chromium owner: chrome/browser/ui/views/frame/browser_view.cc
  accelerators_registered_ = true;
  DVLOG(1) << "Astra accelerator registration enabled "
           << "(PreBrowserStart phase).";

  // -----------------------------------------------------------------------
  // Step 3: Pre-warm Astra services needed for first paint
  // -----------------------------------------------------------------------
  //
  // Services that are needed for the first browser window paint should be
  // pre-warmed here to avoid jank on first window show.
  //
  // TODO(astra): Pre-warm AstraThemeService if sidebar is enabled, since the
  //   sidebar needs theme colors for its initial paint.
  //   Astra owner: astra/browser/astra_theme_service.h

  DVLOG(1) << "AstraBrowserMainExtraParts::PreBrowserStart() — complete";
}

// ============================================================================
// PostBrowserStart
// ============================================================================

void AstraBrowserMainExtraParts::PostBrowserStart() {
  // Called after the first Browser window is created and shown.
  //
  // At this point:
  //   - The first Browser window exists and is visible
  //   - The browser process is fully initialized
  //   - All threads are running
  //
  // Use this phase for light-weight post-init tasks:
  //   - Recording startup metrics
  //   - Triggering initial UI state (e.g. sidebar visibility)
  //   - Registering observers that need a fully-initialized browser
  //
  // IMPORTANT: Do NOT do heavy work here. PostBrowserStart runs on the
  // main thread after the first window is shown; slow work here delays
  // the browser becoming responsive.
  //
  // Chromium pattern: ChromeBrowserMainParts::PostBrowserStart() records
  // startup metrics and triggers post-init tasks.

  TRACE_EVENT0("startup", "AstraBrowserMainExtraParts::PostBrowserStart");
  DVLOG(1) << "AstraBrowserMainExtraParts::PostBrowserStart()";

  DCHECK(pre_browser_start_called_)
      << "PreBrowserStart must be called before PostBrowserStart.";
  DCHECK(!post_browser_start_called_)
      << "PostBrowserStart called more than once.";

  post_browser_start_called_ = true;

  if (!base::FeatureList::IsEnabled(kAstraBrandedBuild)) {
    DVLOG(1) << "Astra branded build disabled — skipping PostBrowserStart.";
    return;
  }

  if (initial_profile_) {
    DVLOG(1) << "Astra PostBrowserStart — initial profile has "
             << (kAstraCommandLast - kAstraCommandFirst)
             << " Astra command IDs available.";
  }

  // -----------------------------------------------------------------------
  // Step 1: Notify browser startup complete
  // -----------------------------------------------------------------------
  //
  // Signal to the content layer (and Astra observers) that browser startup
  // is complete. This is used by IsBrowserStartupComplete() and triggers
  // any deferred operations.
  //
  // This must be called after the first browser window is shown and all
  // Astra services are initialized.
  AstraContentBrowserClient::NotifyBrowserStartupComplete();
  startup_complete_notified_ = true;

  DVLOG(1) << "Astra browser startup completion signaled.";

  // -----------------------------------------------------------------------
  // Step 2: Trigger initial UI state
  // -----------------------------------------------------------------------
  //
  // After the first browser window is created, apply initial Astra UI state:
  //   - Sidebar visibility (from user prefs)
  //   - Initial workspace selection
  //   - Theme / accent color application
  //
  // These are triggered through the command delegate observer pattern —
  // the UI layer observes the command delegate and responds to state
  // changes. The browser layer never directly manipulates UI.
  //
  // TODO(astra): Trigger initial sidebar visibility from user prefs.
  //   This should be done by notifying UI observers, not by manipulating
  //   Views directly.
  //   Astra owner: AstraCommandDelegate + AstraBrowserView observer

  // -----------------------------------------------------------------------
  // Step 3: Register post-startup observers
  // -----------------------------------------------------------------------
  //
  // Register Astra observers that need a fully-initialized browser:
  //   - Tab strip observers (for sidebar projection)
  //   - Browser list observers (for multi-window workspaces)
  //   - Profile change observers
  //
  // TODO(astra): Attach Astra UI observers and metrics hooks after browser
  // startup is complete. Owner: chrome/browser/ui/browser.h observers.
  //
  // TODO(astra): Record Astra startup UMA metrics (time to first Astra UI
  // paint, workspace restore time, etc.).
  // Chromium owner: base::MetricsService / UMA

  DVLOG(1) << "Astra PostBrowserStart complete.";
}

// ============================================================================
// PostMainMessageLoopRun
// ============================================================================

void AstraBrowserMainExtraParts::PostMainMessageLoopRun() {
  // Called after the main message loop exits, during browser shutdown.
  //
  // Chromium services own their own shutdown paths. Astra should only clean
  // up resources that are not managed by ProfileKeyedService or other
  // Chromium-owned lifetime mechanisms.
  //
  // ProfileKeyedServices (including AstraWorkspaceService) are destroyed by
  // Profile destruction, which happens after this point in the shutdown
  // sequence. Do not attempt to access them here.
  //
  // TODO(astra): Flush any Astra-only in-memory metadata that is not backed
  // by a ProfileKeyedService. Owner: astra/browser shutdown helpers.

  TRACE_EVENT0("shutdown",
               "AstraBrowserMainExtraParts::PostMainMessageLoopRun");
  DVLOG(1) << "AstraBrowserMainExtraParts::PostMainMessageLoopRun()";

  DCHECK(!post_main_message_loop_run_called_)
      << "PostMainMessageLoopRun called more than once.";

  post_main_message_loop_run_called_ = true;

  // Unregister accelerators and other per-window state before widgets are
  // destroyed. Note: most cleanup happens automatically via Views widget
  // destruction and ProfileKeyedService shutdown.
  accelerators_registered_ = false;

  initial_profile_ = nullptr;

  DVLOG(1) << "AstraBrowserMainExtraParts::PostMainMessageLoopRun() — "
           << "Astra app-layer teardown complete. Chromium services shut down "
           << "via their own paths.";
}

// ============================================================================
// PostDestroyThreads
// ============================================================================

void AstraBrowserMainExtraParts::PostDestroyThreads() {
  // Called after all browser threads have been destroyed, at the very end
  // of browser shutdown.
  //
  // At this point:
  //   - All child threads (IO, UI, etc.) are gone
  //   - Only the main thread remains
  //   - Most ProfileKeyedServices have been destroyed
  //
  // Use this phase for:
  //   - Final cleanup of global singletons
  //   - Unregistering from global dependency managers
  //   - Logging final shutdown metrics
  //
  // IMPORTANT: Do not access any Profile or Browser objects here. They
  // have already been destroyed by this point in the shutdown sequence.
  //
  // Chromium pattern: ChromeBrowserMainParts::PostDestroyThreads() does
  // final cleanup after threads are destroyed.

  TRACE_EVENT0("shutdown",
               "AstraBrowserMainExtraParts::PostDestroyThreads");
  DVLOG(1) << "AstraBrowserMainExtraParts::PostDestroyThreads()";

  DCHECK(post_main_message_loop_run_called_)
      << "PostMainMessageLoopRun must be called before PostDestroyThreads.";
  DCHECK(!post_destroy_threads_called_)
      << "PostDestroyThreads called more than once.";

  post_destroy_threads_called_ = true;

  // Reset registration flags for consistency during shutdown.
  // Note: Service factories are global singletons that live for the
  // process lifetime; they are not destroyed during normal shutdown.
  service_factories_registered_ = false;
  webui_configs_registered_ = false;
  features_initialized_from_prefs_ = false;
  startup_complete_notified_ = false;

  DVLOG(1) << "Astra PostDestroyThreads complete — "
           << "all Astra app-layer state cleaned up.";
}

}  // namespace astra
