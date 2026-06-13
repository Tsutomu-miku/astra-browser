// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/app/astra_main_delegate.h"

#include "astra/app/astra_browser_main_extra_parts.h"
#include "astra/app/astra_brand.h"
#include "astra/app/astra_content_browser_client.h"
#include "astra/app/astra_feature_list.h"
#include "astra/app/astra_version.h"
#include "astra/common/astra_command_constants.h"
#include "base/check.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "base/trace_event/trace_event.h"
#include "chrome/browser/chrome_browser_main.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/renderer/content_renderer_client.h"
#include "content/public/utility/content_utility_client.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

#include <memory>

namespace astra {

namespace {

// Path key for Astra resources directory relative to the main binary.
// In Chromium, resources are loaded from chrome::DIR_RESOURCES or
// ui::GetPakPathForLocale(). For Astra, we use a similar path under
// the resources directory.
//
// TODO(astra): Add Astra-specific PathService key or use the standard
//   Chrome resources path with a subdirectory.
//   Chromium owner: base::PathService / chrome/common/chrome_paths.cc
constexpr char kAstraResourcesPak[] = "astra_resources.pak";

// The module name used for Astra log messages.
constexpr char kAstraLogTag[] = "astra";

}  // namespace

// Static member definition.
AstraMainDelegate* AstraMainDelegate::g_instance_ = nullptr;

// ============================================================================
// Singleton pattern
// ============================================================================

AstraMainDelegate* AstraMainDelegate::GetInstance() {
  if (!g_instance_) {
    g_instance_ = new AstraMainDelegate();
  }
  return g_instance_;
}

AstraMainDelegate* AstraMainDelegate::GetMainDelegateForTesting() {
  return g_instance_;
}

void AstraMainDelegate::ResetInstanceForTesting() {
  if (g_instance_) {
    delete g_instance_;
    g_instance_ = nullptr;
  }
}

// ============================================================================
// Construction / destruction
// ============================================================================

AstraMainDelegate::AstraMainDelegate() {
  DVLOG(1) << "AstraMainDelegate constructed.";
}

AstraMainDelegate::~AstraMainDelegate() {
  DVLOG(1) << "AstraMainDelegate destroyed.";

  // If we're being destroyed and ProcessExiting wasn't called, log a warning.
  // In normal shutdown, ProcessExiting should have been called before
  // destruction.
  if (!process_exiting_called_) {
    DLOG(WARNING)
        << "AstraMainDelegate destroyed without ProcessExiting() being "
        << "called first. This may indicate an abnormal shutdown.";
  }
}

// ============================================================================
// PreSandboxStartup
// ============================================================================

void AstraMainDelegate::PreSandboxStartup() {
  // Pre-sandbox Astra setup. Keep this minimal — most initialization
  // should happen after the sandbox is engaged to maintain Chromium's security
  // model. Only place pre-sandbox work here if it absolutely cannot happen
  // later (e.g., custom sandbox policy, low-level resource loading).
  //
  // At this point:
  //   - The process has just started
  //   - base::FeatureList is NOT initialized yet
  //   - Sandbox is not yet engaged
  //   - No threads have been created
  //
  // WARNING: Do not call base::FeatureList::IsEnabled() here. Feature list
  // is initialized later in BasicStartupComplete().
  //
  // TODO(astra): Add pre-sandbox hooks only when a specific product requirement
  // cannot be satisfied post-sandbox. Owner: chrome/app/chrome_main_delegate.cc
  // patch point.

  TRACE_EVENT0("startup", "AstraMainDelegate::PreSandboxStartup");
  DVLOG(1) << "AstraMainDelegate::PreSandboxStartup()";

  DCHECK(!pre_sandbox_startup_called_)
      << "PreSandboxStartup called more than once.";

  pre_sandbox_startup_called_ = true;

  // -----------------------------------------------------------------------
  // Step 1: Initialize Astra feature list defaults
  // -----------------------------------------------------------------------
  //
  // Set up Astra-specific feature defaults before the full base::FeatureList
  // is initialized.  This allows Astra to override default feature states
  // early in the startup sequence.
  InitializeAstraFeatureDefaults();

  // -----------------------------------------------------------------------
  // Step 2: Register Astra command ID range
  // -----------------------------------------------------------------------
  //
  // Reserve the Astra command ID range (60000-60499) so that Chromium's
  // command system knows about Astra commands.  The actual command enum
  // is defined in astra/browser/astra_command_delegate.h.
  RegisterAstraCommandIdRange();

  // -----------------------------------------------------------------------
  // Step 3: Set up Astra logging tags
  // -----------------------------------------------------------------------
  //
  // Configure Astra-specific VLOG levels and log tags early so that all
  // subsequent log messages from Astra code are properly tagged.
  ConfigureAstraLoggingTags();

  // -----------------------------------------------------------------------
  // Step 4: Early branding / identity setup
  // -----------------------------------------------------------------------
  //
  // At this point we can do early branding setup that doesn't depend on
  // feature list or sandbox. This includes things like setting up the
  // process name for crash reporting.
  //
  // TODO(astra): Set up Astra-specific crash reporting keys here if needed.
  //   Chromium owner: crashpad / breakpad integration
  //   Patch point: chrome/app/chrome_main_delegate.cc PreSandboxStartup

  DVLOG(1) << "AstraMainDelegate::PreSandboxStartup() — complete";
}

// ============================================================================
// BasicProcessPreInit
// ============================================================================

void AstraMainDelegate::BasicProcessPreInit() {
  // Very early process initialization, before field trials and feature list.
  //
  // At this point:
  //   - Sandbox may or may not be engaged (varies by platform)
  //   - base::FeatureList is NOT initialized
  //   - Field trials are NOT set up
  //   - Locale is NOT loaded
  //
  // Use this for:
  //   - Setting up command-line switches for field trials
  //   - Early logging configuration
  //   - Process-wide state that must be set up before feature list
  //
  // WARNING: Do not use base::FeatureList here. It is not yet initialized.
  //
  // This corresponds to ChromeMainDelegate::BasicProcessPreInit() in the
  // Chromium startup sequence.
  //
  // TODO(astra): Wire into chrome/app/chrome_main_delegate.cc BasicProcessPreInit.
  //   Chromium owner: ChromeMainDelegate::BasicProcessPreInit()

  TRACE_EVENT0("startup", "AstraMainDelegate::BasicProcessPreInit");
  DVLOG(1) << "AstraMainDelegate::BasicProcessPreInit()";

  DCHECK(pre_sandbox_startup_called_)
      << "PreSandboxStartup must be called before BasicProcessPreInit.";
  DCHECK(!basic_process_pre_init_called_)
      << "BasicProcessPreInit called more than once.";

  basic_process_pre_init_called_ = true;

  // -----------------------------------------------------------------------
  // Step 1: Early logging configuration
  // -----------------------------------------------------------------------
  //
  // Configure any Astra-specific logging at this early stage. VLOG levels
  // and log file locations are typically set from command line, but if we
  // need programmatic setup, this is the place.
  //
  // TODO(astra): Add Astra-specific VLOG module levels or log file setup
  //   if needed for development builds.

  DVLOG(1) << "AstraMainDelegate::BasicProcessPreInit() — complete";
}

// ============================================================================
// BasicStartupComplete
// ============================================================================

void AstraMainDelegate::BasicStartupComplete() {
  // Astra feature flags are declared in astra_feature_list.h and controlled
  // through Chromium's base::FeatureList mechanism (about:flags, field trials,
  // command-line switches). We do not reimplement flag parsing.
  //
  // At this point:
  //   - base::FeatureList is fully initialized
  //   - Field trials are registered and randomized
  //   - Locale is loaded
  //   - Crash reporting is set up (if enabled)
  //   - Sandbox is engaged (on platforms where it applies at this stage)
  //
  // Use this phase for:
  //   - Logging Astra feature state
  //   - Registering Astra-specific field trial observers
  //   - Setting up resource bundle paths
  //   - Loading Astra resources
  //
  // TODO(astra): Register Astra-specific field trials and variation configs
  // here if needed. Owner: chrome/variations/service + chrome_main_delegate
  // patch point.

  TRACE_EVENT0("startup", "AstraMainDelegate::BasicStartupComplete");
  DVLOG(1) << "AstraMainDelegate::BasicStartupComplete()";

  // DCHECK that startup phases are called in the correct order.
  DCHECK(pre_sandbox_startup_called_)
      << "PreSandboxStartup must be called before BasicStartupComplete.";
  DCHECK(basic_process_pre_init_called_)
      << "BasicProcessPreInit must be called before BasicStartupComplete.";
  DCHECK(!basic_startup_complete_called_)
      << "BasicStartupComplete called more than once.";

  basic_startup_complete_called_ = true;

  // -----------------------------------------------------------------------
  // Step 1: Log Astra feature state
  // -----------------------------------------------------------------------
  //
  // Log the state of all Astra features for debugging. This is useful for
  // bug reports and crash analysis — we can see which features were enabled
  // at startup.
  DVLOG(1) << "Astra features at startup:"
           << " branded="
           << (base::FeatureList::IsEnabled(kAstraBrandedBuild) ? "on" : "off")
           << " sidebar="
           << (base::FeatureList::IsEnabled(kAstraSidebar) ? "on" : "off")
           << " workspaces="
           << (base::FeatureList::IsEnabled(kAstraWorkspaces) ? "on" : "off")
           << " split_view="
           << (base::FeatureList::IsEnabled(kAstraSplitView) ? "on" : "off")
           << " command_palette="
           << (base::FeatureList::IsEnabled(kAstraCommandPalette) ? "on" : "off")
           << " favorites="
           << (base::FeatureList::IsEnabled(kAstraFavorites) ? "on" : "off")
           << " tab_search="
           << (base::FeatureList::IsEnabled(kAstraTabSearch) ? "on" : "off");

  // -----------------------------------------------------------------------
  // Step 2: Load Astra resource bundle
  // -----------------------------------------------------------------------
  //
  // Load astra_resources.pak which contains Astra-specific resources:
  //   - String resources (Astra UI strings)
  //   - Image resources (icons, logos)
  //   - HTML/CSS/JS for Astra WebUI pages
  //
  // The resource bundle is loaded into the global ui::ResourceBundle
  // singleton and is available for the lifetime of the process.
  //
  // Chromium pattern: ChromeMainDelegate::BasicStartupComplete() loads
  // Chrome's resource bundles (chrome.pak, theme_resources.pak, etc.).
  LoadAstraResourceBundle();

  // -----------------------------------------------------------------------
  // Step 3: Initialize Astra field trials
  // -----------------------------------------------------------------------
  //
  // Register any Astra-specific field trials (A/B experiments). Field
  // trials control gradual rollout of features and are managed by the
  // Chromium variations system.
  //
  // TODO(astra): Set up Astra-specific field trials for gradual feature
  //   rollouts. Use the standard Chromium variations / field trial system.
  //   Chromium owner: chrome/variations/service

  DVLOG(1) << "AstraMainDelegate::BasicStartupComplete() — complete";
}

// ============================================================================
// PostEarlyInitialization
// ============================================================================

void AstraMainDelegate::PostEarlyInitialization() {
  // Called after early initialization is complete but before browser main.
  //
  // At this point:
  //   - Feature list is initialized
  //   - Resources are loaded
  //   - Locale is set up
  //   - Field trials are active
  //   - Sandbox is engaged
  //
  // Use this phase for:
  //   - Late resource loading
  //   - Validating branding and version info
  //   - Setting up process-wide state that depends on feature list
  //
  // This corresponds to ChromeMainDelegate::PostEarlyInitialization().
  //
  // TODO(astra): Wire into chrome/app/chrome_main_delegate.cc
  //   PostEarlyInitialization if needed.
  //   Chromium owner: ChromeMainDelegate::PostEarlyInitialization()

  TRACE_EVENT0("startup", "AstraMainDelegate::PostEarlyInitialization");
  DVLOG(1) << "AstraMainDelegate::PostEarlyInitialization()";

  DCHECK(basic_startup_complete_called_)
      << "BasicStartupComplete must be called before PostEarlyInitialization.";
  DCHECK(!post_early_initialization_called_)
      << "PostEarlyInitialization called more than once.";

  post_early_initialization_called_ = true;

  // -----------------------------------------------------------------------
  // Step 1: Validate branding and version
  // -----------------------------------------------------------------------
  //
  // Log the full Astra version string for debugging and crash reporting.
  // This is useful for correlating crash reports with specific Astra versions.
  DVLOG(1) << "Astra version: " << GetAstraFullVersionString();
  DVLOG(1) << "Astra product: " << GetAstraProductName();
  DVLOG(1) << "Astra update channel: " << GetAstraUpdateChannel();

  // -----------------------------------------------------------------------
  // Step 2: Feature-dependent early setup
  // -----------------------------------------------------------------------
  //
  // Any setup that depends on feature flags and needs to happen before
  // browser main. This is a good place for pre-warming caches or setting
  // up observers that are feature-flag gated.
  //
  // TODO(astra): Add feature-dependent early setup here as needed.

  DVLOG(1) << "AstraMainDelegate::PostEarlyInitialization() — complete";
}

// ============================================================================
// EarlyInitialization
// ============================================================================

void AstraMainDelegate::EarlyInitialization() {
  // Called from ChromeMainDelegate::EarlyInitialization() during browser
  // process startup.  Runs after basic startup but before browser main
  // parts are created.
  //
  // At this point:
  //   - Feature list is fully initialized
  //   - Resources are loaded
  //   - Main thread is running
  //   - No browser threads yet
  //   - No profiles yet
  //
  // Use this for:
  //   - Early browser-process setup that doesn't need threads
  //   - Mojo broker initialization
  //   - Early service manager setup
  //
  // TODO(astra): Wire into chrome/app/chrome_main_delegate.cc
  //   EarlyInitialization if needed.
  //   Chromium owner: ChromeMainDelegate::EarlyInitialization()

  TRACE_EVENT0("startup", "AstraMainDelegate::EarlyInitialization");
  DVLOG(1) << "AstraMainDelegate::EarlyInitialization()";

  DCHECK(post_early_initialization_called_ || basic_startup_complete_called_)
      << "BasicStartupComplete must be called before EarlyInitialization.";
  DCHECK(!early_initialization_called_)
      << "EarlyInitialization called more than once.";

  early_initialization_called_ = true;

  // -----------------------------------------------------------------------
  // Step 1: Install content browser client hooks
  // -----------------------------------------------------------------------
  //
  // Install Astra hooks into the content browser client.  These hooks
  // allow Astra to intercept and augment various content-layer operations
  // such as Web preference overrides and Mojo interface registration.
  //
  // Note: In the current architecture, hooks are installed lazily when
  // the content browser client is created.  We log the intent here for
  // lifecycle tracking.
  DVLOG(1) << "Astra content browser client hooks will be installed "
           << "during CreateContentBrowserClient().";

  // -----------------------------------------------------------------------
  // Step 2: Early Mojo setup
  // -----------------------------------------------------------------------
  //
  // Set up any early Mojo broker or service manager configuration needed
  // by Astra services.
  //
  // TODO(astra): Set up Astra-specific Mojo services if needed.
  //   Chromium owner: content::ServiceManagerConnection / Mojo

  DVLOG(1) << "AstraMainDelegate::EarlyInitialization() — complete";
}

// ============================================================================
// CreateContentBrowserClient
// ============================================================================

std::unique_ptr<content::ContentBrowserClient>
AstraMainDelegate::CreateContentBrowserClient() {
  // Creates Chrome's ContentBrowserClient and installs Astra hooks into it.
  //
  // The ContentBrowserClient is the main interface between the content layer
  // and the embedder (Chrome / Astra).  It controls:
  //   - Web preferences
  //   - Mojo interface binding
  //   - WebContents creation
  //   - URL policy (incognito, external)
  //   - And many other embedder-specific behaviors
  //
  // Astra doesn't replace ChromeContentBrowserClient — it augments it via
  // patch points.  The returned client is a standard ChromeContentBrowserClient
  // with Astra hooks installed through the AstraContentBrowserClient helper.
  //
  // Chromium pattern: ChromeMainDelegate::CreateContentBrowserClient()
  // returns a ChromeContentBrowserClient instance.
  //
  // TODO(astra): Wire into chrome/app/chrome_main_delegate.cc
  //   CreateContentBrowserClient().
  //   Chromium owner: ChromeMainDelegate::CreateContentBrowserClient()

  TRACE_EVENT0("startup", "AstraMainDelegate::CreateContentBrowserClient");
  DVLOG(1) << "AstraMainDelegate::CreateContentBrowserClient()";

  DCHECK(!content_browser_client_created_)
      << "CreateContentBrowserClient called more than once.";

  content_browser_client_created_ = true;

  // -----------------------------------------------------------------------
  // Step 1: Install Astra content browser client hooks
  // -----------------------------------------------------------------------
  //
  // Install Astra hooks into the content browser client.  This sets up
  // the observer lists and state tracking for Astra's content-layer
  // integrations.
  AstraContentBrowserClient::InstallChromeContentBrowserClientHooks();

  DVLOG(1) << "Astra content browser client hooks installed.";

  // -----------------------------------------------------------------------
  // Step 2: Return nullptr to let Chrome create its own client
  // -----------------------------------------------------------------------
  //
  // In the patch-point pattern, Astra doesn't create the ContentBrowserClient
  // — Chrome does.  Astra only installs hooks into Chrome's client.
  //
  // Returning nullptr here indicates that Chrome should proceed with its
  // normal client creation.  The actual hook installation happens via
  // patch points in ChromeContentBrowserClient's constructor and methods.
  //
  // Note: In a full Chromium build with the patch applied, this function
  // would be called from ChromeMainDelegate::CreateContentBrowserClient()
  // and the return value would be used or ignored depending on the patch
  // implementation.
  //
  // TODO(astra): Determine exact patch-point semantics.  Options:
  //   1. Astra creates the ChromeContentBrowserClient and returns it
  //   2. Chrome creates it, Astra just hooks into it
  //   Currently using option 2 (hooks only).

  DVLOG(1) << "AstraMainDelegate::CreateContentBrowserClient() — "
           << "returning nullptr (Chrome creates its own client, "
           << "Astra installs hooks via patch points).";

  return nullptr;
}

// ============================================================================
// CreateContentRendererClient
// ============================================================================

std::unique_ptr<content::ContentRendererClient>
AstraMainDelegate::CreateContentRendererClient() {
  // Creates Chrome's ContentRendererClient.
  //
  // Astra currently doesn't need a custom renderer client.  Renderer-side
  // code is generally better avoided — most product logic should live in
  // the browser process for security and stability.
  //
  // This method exists for completeness of the patch-point pattern.  If
  // Astra ever needs renderer-side extensions (e.g., custom WebUI bindings,
  // render-frame observers), they would be added here.
  //
  // Returning nullptr lets Chrome create its standard renderer client.
  //
  // TODO(astra): Add renderer-side hooks if needed.
  //   Chromium owner: ChromeMainDelegate::CreateContentRendererClient()

  TRACE_EVENT0("startup", "AstraMainDelegate::CreateContentRendererClient");
  DVLOG(1) << "AstraMainDelegate::CreateContentRendererClient() — "
           << "returning nullptr (Chrome creates its own renderer client).";

  // Astra doesn't need a custom renderer client — delegate to Chrome.
  return nullptr;
}

// ============================================================================
// CreateContentUtilityClient
// ============================================================================

std::unique_ptr<content::ContentUtilityClient>
AstraMainDelegate::CreateContentUtilityClient() {
  // Creates Chrome's ContentUtilityClient.
  //
  // Utility processes handle out-of-process operations like image decoding,
  // network service, and other isolated work.  Astra currently doesn't need
  // custom utility process behavior.
  //
  // This method exists for completeness of the patch-point pattern.  If
  // Astra ever needs utility process extensions (e.g., custom utility
  // handlers), they would be added here.
  //
  // Returning nullptr lets Chrome create its standard utility client.
  //
  // TODO(astra): Add utility process hooks if needed.
  //   Chromium owner: ChromeMainDelegate::CreateContentUtilityClient()

  TRACE_EVENT0("startup", "AstraMainDelegate::CreateContentUtilityClient");
  DVLOG(1) << "AstraMainDelegate::CreateContentUtilityClient() — "
           << "returning nullptr (Chrome creates its own utility client).";

  // Astra doesn't need a custom utility client — delegate to Chrome.
  return nullptr;
}

// ============================================================================
// RegisterBrowserMainExtraParts
// ============================================================================

void AstraMainDelegate::RegisterBrowserMainExtraParts(
    ChromeBrowserMainParts* main_parts) {
  if (!main_parts) {
    return;
  }

  // Registers Astra's browser main extra parts with Chromium's browser main.
  //
  // AstraBrowserMainExtraParts is the primary hook for Astra product
  // initialization. It adds Astra-specific code to each phase of the
  // browser main startup sequence.
  //
  // The extra parts are added to ChromeBrowserMainParts, which calls each
  // part's lifecycle methods at the appropriate point in the startup
  // sequence (PreCreateThreads, PreProfileInit, PostProfileInit, etc.).
  //
  // Chromium pattern: ChromeBrowserMainParts::ChromeBrowserMainParts()
  // adds various ChromeBrowserMainExtraParts subclasses via AddParts().
  //
  // TODO(astra): Wire this into ChromeBrowserMainParts construction under an
  // Astra branding/build flag. Owner: chrome/browser/chrome_browser_main.cc
  // patch point.

  TRACE_EVENT0("startup", "AstraMainDelegate::RegisterBrowserMainExtraParts");
  DVLOG(1) << "AstraMainDelegate::RegisterBrowserMainExtraParts()";

  DCHECK(!browser_main_extra_parts_registered_)
      << "RegisterBrowserMainExtraParts called more than once.";

  browser_main_extra_parts_registered_ = true;

  main_parts->AddParts(std::make_unique<AstraBrowserMainExtraParts>());

  DVLOG(1) << "AstraBrowserMainExtraParts registered with ChromeBrowserMainParts.";
}

// ============================================================================
// ProcessExiting
// ============================================================================

void AstraMainDelegate::ProcessExiting() {
  // Called during process shutdown to perform Astra-specific cleanup.
  //
  // At this point:
  //   - The main message loop has exited
  //   - Browser threads may still be running (depending on shutdown phase)
  //   - Profiles may already be destroyed
  //   - The process is about to exit
  //
  // Use this for:
  //   - Final log flushing
  //   - Cleanup of global singletons
  //   - Unregistering from global systems
  //   - Final metrics / telemetry upload
  //
  // IMPORTANT: Be careful what you access here.  Many subsystems may have
  // already been torn down by this point in the shutdown sequence.
  //
  // TODO(astra): Wire into chrome/app/chrome_main_delegate.cc ProcessExiting.
  //   Chromium owner: ChromeMainDelegate::ProcessExiting()

  TRACE_EVENT0("shutdown", "AstraMainDelegate::ProcessExiting");
  DVLOG(1) << "AstraMainDelegate::ProcessExiting()";

  DCHECK(!process_exiting_called_)
      << "ProcessExiting called more than once.";

  process_exiting_called_ = true;

  // -----------------------------------------------------------------------
  // Step 1: Clean up Astra global state
  // -----------------------------------------------------------------------
  //
  // Clean up any Astra-specific global state that was set up during
  // startup.  This includes:
  //   - Feature override state
  //   - Logging configuration
  //   - Observer registrations
  //
  // Note: Most cleanup happens automatically through RAII and Profile /
  // KeyedService destruction.  This is for truly global / process-level
  // state.

  // Reset feature override state.
  ResetAstraFeatureOverridesForTesting();

  DVLOG(1) << "Astra global state cleaned up.";

  // -----------------------------------------------------------------------
  // Step 2: Final log flush
  // -----------------------------------------------------------------------
  //
  // Log final shutdown messages before the log system is torn down.
  DVLOG(1) << "Astra is shutting down. Total features: "
           << GetEnabledAstraFeatureCount();

  DVLOG(1) << "AstraMainDelegate::ProcessExiting() — complete";
}

// ============================================================================
// Internal helpers
// ============================================================================

void AstraMainDelegate::LoadAstraResourceBundle() {
  // Loads the Astra resource bundle (astra_resources.pak).
  //
  // The resource bundle contains all of Astra's compiled resources:
  //   - String tables (for different locales)
  //   - Images (icons, logos, UI graphics)
  //   - HTML/CSS/JS for WebUI pages
  //
  // Resources are loaded into the global ui::ResourceBundle singleton
  // and can be accessed anywhere in the codebase via:
  //   ui::ResourceBundle::GetSharedInstance()
  //   ui::ResourceBundle::GetSharedInstance().GetLocalizedString(IDR_*)
  //
  // Chromium pattern: ChromeContentBrowserClient packs resources via
  // grit and loads them in the resource bundle at startup.
  //
  // TODO(astra): Implement actual resource loading once astra_resources.pak
  //   is generated by the grit build target. For now, this function is a
  //   placeholder that logs the intent.
  //   Astra owner: astra/app/resources/ (grit build)
  //   Chromium owner: ui/ResourceBundle
  //   Patch point: chrome/app/chrome_main_delegate.cc resource loading

  if (resource_bundle_loaded_) {
    DVLOG(1) << "Astra resource bundle already loaded — skipping.";
    return;
  }

  DVLOG(1) << "AstraMainDelegate::LoadAstraResourceBundle() — "
           << "loading " << kAstraResourcesPak;

  // TODO(astra): Uncomment and implement actual resource loading.
  //
  // Example Chromium pattern:
  //   base::FilePath pak_path;
  //   PathService::Get(chrome::DIR_RESOURCES, &pak_path);
  //   pak_path = pak_path.AppendASCII(kAstraResourcesPak);
  //   ui::ResourceBundle::GetSharedInstance().AddDataPackFromPath(
  //       pak_path, ui::kScaleFactorNone);
  //
  // For locale-specific resources:
  //   std::string locale = l10n_util::GetApplicationLocale(std::string());
  //   ui::ResourceBundle::GetSharedInstance().OverrideLocalePak({locale_pak});

  // Mark as loaded even though it's a no-op for now, so callers can
  // DCHECK that loading was attempted.
  resource_bundle_loaded_ = true;

  DVLOG(1) << "Astra resource bundle load complete "
           << "(placeholder — actual pak file not yet generated).";
}

void AstraMainDelegate::InitializeAstraFeatureDefaults() {
  // Initializes Astra feature list defaults.
  //
  // This is called from PreSandboxStartup(), before the full base::FeatureList
  // is initialized.  It sets up any Astra-specific default overrides that
  // need to be in place before field trials and command-line flags take
  // effect.
  //
  // Note: Feature defaults are typically set by the feature declarations
  // themselves (BASE_FEATURE macros).  This function is for any additional
  // setup that needs to happen before feature list initialization, such as:
  //   - Registering custom feature overrides
  //   - Setting up field trial list attributes
  //   - Configuring feature list observers
  //
  // TODO(astra): Implement actual feature default initialization if needed.
  //   For now, features use their declared default states (enabled/disabled).
  //   Chromium owner: base::FeatureList / base::FieldTrialList

  DVLOG(1) << "InitializeAstraFeatureDefaults()";

  DCHECK(!feature_defaults_initialized_)
      << "InitializeAstraFeatureDefaults called more than once.";

  feature_defaults_initialized_ = true;

  // -----------------------------------------------------------------------
  // Astra feature defaults summary:
  //   - kAstraBrandedBuild: enabled by default in Astra-branded builds
  //   - kAstraSidebar: enabled by default (core product surface)
  //   - kAstraWorkspaces: enabled by default (core product metadata)
  //   - kAstraSplitView: disabled by default (experimental)
  //   - kAstraCommandPalette: disabled by default (experimental)
  //   - kAstraFavorites: disabled by default (experimental)
  //   - kAstraTabSearch: disabled by default (planned)
  //   - ... others: disabled by default (experimental / planned)
  //
  // These defaults are set in astra_feature_list.cc via BASE_FEATURE macros.

  DVLOG(1) << "Astra feature defaults initialized.";
}

void AstraMainDelegate::RegisterAstraCommandIdRange() {
  // Registers the Astra command ID range with the command system.
  //
  // The Astra command ID range is [60000, 60500), reserved for
  // Astra-specific commands.  This avoids collisions with Chromium's
  // built-in command IDs (0-59999).
  //
  // The actual command enum is defined in astra/browser/astra_command_delegate.h.
  // This function validates the range and registers it with any global
  // command tracking systems.
  //
  // Note: In the current architecture, command IDs are compile-time
  // constants and don't need explicit registration.  This function
  // provides a hook point for any future registration needs and
  // validates the range bounds.
  //
  // TODO(astra): Wire this into Chromium's command ID registration if
  //   needed.  Currently the range is just a compile-time constant.
  //   Chromium owner: chrome/app/chrome_command_ids.h

  DVLOG(1) << "RegisterAstraCommandIdRange() — range: ["
           << kAstraCommandFirst << ", " << kAstraCommandLast << ") "
           << "(" << (kAstraCommandLast - kAstraCommandFirst)
           << " command IDs available)";

  DCHECK(!command_id_range_registered_)
      << "RegisterAstraCommandIdRange called more than once.";

  command_id_range_registered_ = true;

  // Validate the range bounds.
  DCHECK_GT(kAstraCommandLast, kAstraCommandFirst)
      << "Astra command ID range must have at least one entry.";
  DCHECK_GE(kAstraCommandFirst, 60000)
      << "Astra command IDs must start at 60000 or higher to avoid "
      << "collisions with Chromium's built-in command IDs.";

  DVLOG(1) << "Astra command ID range registered: "
           << kAstraCommandFirst << " to " << kAstraCommandLast - 1
           << " (" << (kAstraCommandLast - kAstraCommandFirst)
           << " total IDs).";
}

void AstraMainDelegate::ConfigureAstraLoggingTags() {
  // Sets up Astra-specific logging tags and VLOG module levels.
  //
  // This is called from PreSandboxStartup() to configure logging early in
  // the process lifetime so that all subsequent log messages have proper
  // tagging.
  //
  // Astra log messages use the "astra" tag for easy filtering.  VLOG
  // levels for Astra modules can be configured via command line:
  //   --vmodule=astra*=1
  //
  // TODO(astra): Configure Astra-specific VLOG levels and log tags.
  //   Chromium owner: logging / base::LogMessage

  DVLOG(1) << "ConfigureAstraLoggingTags()";

  DCHECK(!logging_tags_configured_)
      << "ConfigureAstraLoggingTags called more than once.";

  logging_tags_configured_ = true;

  // Log tag information for debugging.
  DVLOG(1) << "Astra log tag: " << kAstraLogTag;
  DVLOG(1) << "Astra VLOG module prefix: astra/*";

  // TODO(astra): Set up programmatic VLOG levels for Astra modules if
  //   needed.  For now, VLOG levels are controlled by command-line flags
  //   (--vmodule=astra*=N), which is the standard Chromium pattern.

  DVLOG(1) << "Astra logging tags configured.";
}

}  // namespace astra
