// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_APP_ASTRA_BROWSER_MAIN_EXTRA_PARTS_H_
#define ASTRA_APP_ASTRA_BROWSER_MAIN_EXTRA_PARTS_H_

#include "base/memory/raw_ptr.h"
#include "chrome/browser/chrome_browser_main_extra_parts.h"

class Profile;

namespace astra {

// Registered from ChromeBrowserMainParts via AstraMainDelegate. This is the
// top-level Astra hook for direct Chromium builds.
//
// Ownership boundary:
//   Chromium owns Profile, Browser, TabStripModel, WebContents, and all
//   standard browser services (history, downloads, passwords, extensions, etc.).
//
//   Astra adds only product metadata and UI projections:
//     - Workspace service (ProfileKeyedService for Spaces metadata)
//     - Sidebar UI projection (Views layer, reads TabStripModel state)
//     - Astra-only command IDs (delegates to Chrome command controller)
//     - Split/Glance presentation metadata
//
// This class must NOT initialize a parallel browser runtime or duplicate
// Chromium-owned services.
//
// Lifecycle order (relevant Astra hooks):
//   1. PreCreateThreads()          — service factory registration, WebUI configs
//   2. PreProfileInit()            — keyed-service factory readiness
//   3. PostProfileInit()           — per-profile service construction
//   4. PreBrowserStart()           — accelerators, feature finalization
//   5. PostBrowserStart()          — UI state, light-weight post-init
//   6. PostMainMessageLoopRun()    — shutdown after message loop exits
//   7. PostDestroyThreads()        — final cleanup after threads are gone
class AstraBrowserMainExtraParts final : public ChromeBrowserMainExtraParts {
 public:
  AstraBrowserMainExtraParts();
  AstraBrowserMainExtraParts(const AstraBrowserMainExtraParts&) = delete;
  AstraBrowserMainExtraParts& operator=(const AstraBrowserMainExtraParts&) =
      delete;
  ~AstraBrowserMainExtraParts() override;

  // ChromeBrowserMainExtraParts:
  void PreCreateThreads() override;
  void PreProfileInit() override;
  void PostProfileInit(Profile* profile, bool is_initial_profile) override;
  void PreBrowserStart() override;
  void PostBrowserStart() override;
  void PostMainMessageLoopRun() override;
  void PostDestroyThreads() override;

  // -- State accessors (for testing and DCHECKs) --------------------------

  // Returns true if PreCreateThreads has been called.
  bool pre_create_threads_called() const { return pre_create_threads_called_; }

  // Returns true if PreProfileInit has been called.
  bool pre_profile_init_called() const { return pre_profile_init_called_; }

  // Returns true if PostProfileInit has been called.
  bool post_profile_init_called() const { return post_profile_init_called_; }

  // Returns true if PreBrowserStart has been called.
  bool pre_browser_start_called() const { return pre_browser_start_called_; }

  // Returns true if PostBrowserStart has been called.
  bool post_browser_start_called() const {
    return post_browser_start_called_;
  }

  // Returns true if PostMainMessageLoopRun has been called.
  bool post_main_message_loop_run_called() const {
    return post_main_message_loop_run_called_;
  }

  // Returns true if PostDestroyThreads has been called.
  bool post_destroy_threads_called() const {
    return post_destroy_threads_called_;
  }

  // Returns true if service factories have been registered.
  bool service_factories_registered() const {
    return service_factories_registered_;
  }

  // Returns true if WebUI configs have been registered.
  bool webui_configs_registered() const { return webui_configs_registered_; }

  // Returns true if accelerators have been registered.
  bool accelerators_registered() const { return accelerators_registered_; }

  // Returns true if features have been initialized from prefs.
  bool features_initialized_from_prefs() const {
    return features_initialized_from_prefs_;
  }

  // Returns true if startup completion has been notified.
  bool startup_complete_notified() const {
    return startup_complete_notified_;
  }

  // Returns the initial (primary) profile, or null if not set.
  Profile* initial_profile() const { return initial_profile_; }

 private:
  // The initial (primary) profile for this browser process. Not owned —
  // Chromium's ProfileManager owns all Profile lifetimes.
  raw_ptr<Profile> initial_profile_ = nullptr;

  // Tracks whether Astra keyed-service factories have been registered.
  // Registration is idempotent but we track it for DCHECKs and logging.
  bool service_factories_registered_ = false;

  // Tracks whether Astra WebUI configs have been registered.
  bool webui_configs_registered_ = false;

  // Tracks whether Astra accelerators have been registered.
  bool accelerators_registered_ = false;

  // Tracks whether Astra features have been initialized from prefs.
  bool features_initialized_from_prefs_ = false;

  // Tracks whether startup completion has been notified.
  bool startup_complete_notified_ = false;

  // -- Lifecycle phase tracking -------------------------------------------
  //
  // These boolean flags track which lifecycle phases have been entered.
  // They are used for DCHECKing correct ordering and for state queries
  // in tests.

  bool pre_create_threads_called_ = false;
  bool pre_profile_init_called_ = false;
  bool post_profile_init_called_ = false;
  bool pre_browser_start_called_ = false;
  bool post_browser_start_called_ = false;
  bool post_main_message_loop_run_called_ = false;
  bool post_destroy_threads_called_ = false;
};

}  // namespace astra

#endif  // ASTRA_APP_ASTRA_BROWSER_MAIN_EXTRA_PARTS_H_
