// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_APP_ASTRA_MAIN_DELEGATE_H_
#define ASTRA_APP_ASTRA_MAIN_DELEGATE_H_

// Patch helper invoked from ChromeMainDelegate. This class does NOT replace
// ChromeMainDelegate — it is called from small patch points in
// chrome/app/chrome_main_delegate.cc to register Astra-specific hooks during
// the Chromium startup sequence.
//
// Chromium startup order (relevant phases):
//   1. PreSandboxStartup          — before the sandbox is engaged
//   2. BasicProcessPreInit        — very early, before field trials
//   3. BasicStartupComplete       — after basic feature list and locale init
//   4. PostEarlyInitialization    — after early initialization, before main
//   5. RegisterBrowserMainExtraParts — during ChromeBrowserMainParts construction
//
// All product logic lives in astra/browser; this layer only wires hooks.
//
// This class follows the singleton pattern: GetInstance() returns the single
// AstraMainDelegate instance.  The instance is created on first access and
// lives for the lifetime of the process.

#include <memory>

#include "base/memory/raw_ptr.h"

namespace content {
class ContentBrowserClient;
class ContentRendererClient;
class ContentUtilityClient;
}  // namespace content

class ChromeBrowserMainParts;

namespace astra {

class AstraMainDelegate {
 public:
  // Returns the singleton instance of AstraMainDelegate.
  // Creates the instance on first call.
  static AstraMainDelegate* GetInstance();

  // Returns the main delegate for testing, or null if none exists.
  // Unlike GetInstance(), this does not create the instance.
  static AstraMainDelegate* GetMainDelegateForTesting();

  AstraMainDelegate(const AstraMainDelegate&) = delete;
  AstraMainDelegate& operator=(const AstraMainDelegate&) = delete;
  ~AstraMainDelegate();

  // -- Process startup lifecycle -------------------------------------------

  // Called from ChromeMainDelegate::PreSandboxStartup() under an Astra
  // branding flag. Keep this minimal — most Astra setup must happen after
  // the sandbox is engaged, in BrowserMainExtraParts.
  //
  // Use this only for things that absolutely must run before the sandbox:
  //   - Custom sandbox policy (rare)
  //   - Early crash reporting setup
  //   - Low-level resource loading that can't happen post-sandbox
  //
  // Astra-specific initialization in PreSandboxStartup:
  //   - Initialize Astra feature list defaults
  //   - Register Astra command ID range
  //   - Set up Astra logging tags
  //
  // TODO(astra): Wire into chrome/app/chrome_main_delegate.cc PreSandboxStartup.
  void PreSandboxStartup();

  // Called from ChromeMainDelegate::BasicProcessPreInit() under an Astra
  // branding flag. Runs very early in process startup, before field trials
  // and feature list are initialized.
  //
  // Use this for:
  //   - Early logging configuration
  //   - Process-level setup that must happen before feature list
  //
  // WARNING: At this point, base::FeatureList is NOT yet initialized.
  // Do not call base::FeatureList::IsEnabled() here.
  //
  // TODO(astra): Wire into chrome/app/chrome_main_delegate.cc BasicProcessPreInit.
  void BasicProcessPreInit();

  // Called from ChromeMainDelegate::BasicStartupComplete() under an Astra
  // branding flag. Initializes Astra feature flags and resource registration
  // after Chromium's basic startup (feature list, locale, crash keys) is done.
  //
  // At this point:
  //   - base::FeatureList is initialized
  //   - Locale is loaded
  //   - Crash reporting is set up
  //   - Field trials are registered
  //
  // Use this for:
  //   - Logging Astra feature state
  //   - Registering Astra-specific field trials
  //   - Resource bundle setup
  //
  // TODO(astra): Wire into chrome/app/chrome_main_delegate.cc BasicStartupComplete.
  void BasicStartupComplete();

  // Called from ChromeMainDelegate::PostEarlyInitialization() under an Astra
  // branding flag. Runs after early initialization is complete but before
  // the browser main message loop starts.
  //
  // Use this phase for:
  //   - Late resource loading
  //   - Setting up process-wide state that depends on feature list
  //   - Validating branding / version info
  //
  // TODO(astra): Wire into chrome/app/chrome_main_delegate.cc
  //   PostEarlyInitialization if needed.
  //   Chromium owner: ChromeMainDelegate::PostEarlyInitialization()
  void PostEarlyInitialization();

  // Called from ChromeMainDelegate::EarlyInitialization() under an Astra
  // branding flag. Runs after basic startup but before browser main parts
  // are created.
  //
  // Use this for:
  //   - Early browser-process setup that doesn't need threads
  //   - Mojo broker initialization
  //   - Early service manager setup
  //
  // TODO(astra): Wire into chrome/app/chrome_main_delegate.cc EarlyInitialization.
  void EarlyInitialization();

  // -- Content client creation --------------------------------------------

  // Creates and returns Chrome's ContentBrowserClient with Astra hooks
  // installed.  Called from ChromeMainDelegate::CreateContentBrowserClient()
  // via a patch point.
  //
  // The returned client is a ChromeContentBrowserClient with Astra-specific
  // hooks installed (e.g., Web preference overrides, Mojo interface
  // registration, WebContents creation hooks).
  //
  // Chromium owns the ContentBrowserClient lifecycle; Astra only adds hooks.
  //
  // TODO(astra): Wire into chrome/app/chrome_main_delegate.cc
  //   CreateContentBrowserClient().
  //   Chromium owner: ChromeMainDelegate::CreateContentBrowserClient()
  std::unique_ptr<content::ContentBrowserClient> CreateContentBrowserClient();

  // Creates and returns Chrome's ContentRendererClient.  Astra doesn't need
  // a custom renderer client — this delegates entirely to Chrome's
  // implementation.  Provided for symmetry with the patch-point pattern.
  //
  // TODO(astra): Wire into chrome/app/chrome_main_delegate.cc
  //   CreateContentRendererClient() if Astra needs renderer-side hooks.
  //   Chromium owner: ChromeMainDelegate::CreateContentRendererClient()
  std::unique_ptr<content::ContentRendererClient> CreateContentRendererClient();

  // Creates and returns Chrome's ContentUtilityClient.  Astra doesn't need
  // a custom utility client — this delegates entirely to Chrome's
  // implementation.  Provided for symmetry with the patch-point pattern.
  //
  // TODO(astra): Wire into chrome/app/chrome_main_delegate.cc
  //   CreateContentUtilityClient() if Astra needs utility process hooks.
  //   Chromium owner: ChromeMainDelegate::CreateContentUtilityClient()
  std::unique_ptr<content::ContentUtilityClient> CreateContentUtilityClient();

  // -- Browser main parts registration ------------------------------------

  // Called from ChromeBrowserMainParts construction to register Astra's
  // browser-main extra parts. This is the primary hook for Astra product
  // initialization.
  void RegisterBrowserMainExtraParts(ChromeBrowserMainParts* main_parts);

  // -- Process shutdown ---------------------------------------------------

  // Called from ChromeMainDelegate::ProcessExiting() during shutdown.
  // Performs Astra-specific cleanup before the process exits.
  //
  // Use this for:
  //   - Final log flushing
  //   - Cleanup of global singletons
  //   - Unregistering from global systems
  //
  // TODO(astra): Wire into chrome/app/chrome_main_delegate.cc ProcessExiting.
  void ProcessExiting();

  // -- State query --------------------------------------------------------

  // Returns true if PreSandboxStartup has been called.
  bool pre_sandbox_startup_called() const { return pre_sandbox_startup_called_; }

  // Returns true if BasicProcessPreInit has been called.
  bool basic_process_pre_init_called() const {
    return basic_process_pre_init_called_;
  }

  // Returns true if BasicStartupComplete has been called.
  bool basic_startup_complete_called() const {
    return basic_startup_complete_called_;
  }

  // Returns true if EarlyInitialization has been called.
  bool early_initialization_called() const {
    return early_initialization_called_;
  }

  // Returns true if PostEarlyInitialization has been called.
  bool post_early_initialization_called() const {
    return post_early_initialization_called_;
  }

  // Returns true if ProcessExiting has been called.
  bool process_exiting_called() const { return process_exiting_called_; }

  // Returns true if the Astra resource bundle has been loaded.
  bool resource_bundle_loaded() const { return resource_bundle_loaded_; }

  // Returns true if browser main extra parts have been registered.
  bool browser_main_extra_parts_registered() const {
    return browser_main_extra_parts_registered_;
  }

  // Returns true if the content browser client has been created.
  bool content_browser_client_created() const {
    return content_browser_client_created_;
  }

  // Returns true if Astra feature list defaults have been initialized.
  bool feature_defaults_initialized() const {
    return feature_defaults_initialized_;
  }

  // Returns true if the Astra command ID range has been registered.
  bool command_id_range_registered() const {
    return command_id_range_registered_;
  }

  // Returns true if Astra logging tags have been set up.
  bool logging_tags_configured() const { return logging_tags_configured_; }

  // -- Test helpers -------------------------------------------------------

  // Resets the singleton instance for testing.
  // Only use in unit tests — never in production code.
  // This deletes the current instance; a new one will be created on
  // the next GetInstance() call.
  static void ResetInstanceForTesting();

 private:
  // Private constructor — singleton pattern.
  AstraMainDelegate();

  // Loads Astra resource bundle (astra_resources.pak).
  // Called during startup to add Astra-specific resources to the global
  // resource bundle.
  //
  // Chromium owns: ui::ResourceBundle singleton
  // Astra owns: astra_resources.pak (grit-generated resource file)
  //
  // TODO(astra): Implement once astra_resources.pak is generated by the
  //   resources build target.
  //   Patch point: chrome/app/chrome_main_delegate.cc
  //   Chromium owner: ui::ResourceBundle / AddDataPackFromPath
  void LoadAstraResourceBundle();

  // Initializes Astra feature list defaults.
  // Called from PreSandboxStartup to set up default feature state before
  // the full base::FeatureList is initialized.
  //
  // Note: This sets up Astra-specific default overrides that are applied
  // before field trials and command-line flags take effect.
  //
  // TODO(astra): Define Astra feature defaults that should be set before
  //   the full feature list is initialized.
  //   Chromium owner: base::FeatureList / base::FieldTrialList
  void InitializeAstraFeatureDefaults();

  // Registers the Astra command ID range with the command system.
  // Called from PreSandboxStartup to reserve the Astra command ID range.
  //
  // The command ID range is defined in astra/common/astra_command_constants.h
  // and starts at 60000 to avoid collisions with Chromium's built-in
  // command IDs.
  //
  // TODO(astra): Wire this into Chromium's command ID registration if
  //   needed.  Currently the range is just a compile-time constant.
  //   Chromium owner: chrome/app/chrome_command_ids.h
  void RegisterAstraCommandIdRange();

  // Sets up Astra-specific logging tags and VLOG module levels.
  // Called from PreSandboxStartup to configure logging early in the process
  // lifetime so that all subsequent log messages have proper tagging.
  //
  // TODO(astra): Configure Astra-specific VLOG levels and log tags.
  //   Chromium owner: logging / base::LogMessage
  void ConfigureAstraLoggingTags();

  // Singleton instance.
  static AstraMainDelegate* g_instance_;

  // -- State flags --------------------------------------------------------

  // Tracks whether PreSandboxStartup has been called (for DCHECK ordering).
  bool pre_sandbox_startup_called_ = false;

  // Tracks whether BasicProcessPreInit has been called.
  bool basic_process_pre_init_called_ = false;

  // Tracks whether BasicStartupComplete has been called.
  bool basic_startup_complete_called_ = false;

  // Tracks whether EarlyInitialization has been called.
  bool early_initialization_called_ = false;

  // Tracks whether PostEarlyInitialization has been called.
  bool post_early_initialization_called_ = false;

  // Tracks whether ProcessExiting has been called.
  bool process_exiting_called_ = false;

  // Tracks whether the Astra resource bundle has been loaded.
  bool resource_bundle_loaded_ = false;

  // Tracks whether browser main extra parts have been registered.
  bool browser_main_extra_parts_registered_ = false;

  // Tracks whether the content browser client has been created.
  bool content_browser_client_created_ = false;

  // Tracks whether Astra feature list defaults have been initialized.
  bool feature_defaults_initialized_ = false;

  // Tracks whether the Astra command ID range has been registered.
  bool command_id_range_registered_ = false;

  // Tracks whether Astra logging tags have been configured.
  bool logging_tags_configured_ = false;
};

}  // namespace astra

#endif  // ASTRA_APP_ASTRA_MAIN_DELEGATE_H_
