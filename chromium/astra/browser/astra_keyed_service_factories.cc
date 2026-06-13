// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_keyed_service_factories.h"

#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_registry_simple.h"

#include "astra/browser/astra_accessibility_service_factory.h"
#include "astra/browser/astra_autofill_helper_factory.h"
#include "astra/browser/astra_favorite_service_factory.h"
#include "astra/browser/astra_focus_mode_service_factory.h"
#include "astra/browser/astra_history_helper_factory.h"
#include "astra/browser/astra_downloads_helper_factory.h"
#include "astra/browser/astra_memory_saver_service_factory.h"
#include "astra/browser/astra_new_tab_page_service_factory.h"
#include "astra/browser/astra_note_service_factory.h"
#include "astra/browser/astra_notification_service_factory.h"
#include "astra/browser/astra_pip_service_factory.h"
#include "astra/browser/astra_reading_list_service_factory.h"
#include "astra/browser/astra_safety_helper_factory.h"
#include "astra/browser/astra_screenshot_service_factory.h"
#include "astra/browser/astra_sync_helper_factory.h"
#include "astra/browser/astra_tab_stack_service_factory.h"
#include "astra/browser/astra_theme_service_factory.h"
#include "astra/browser/astra_workspace_service_factory.h"

namespace astra {

// =========================================================================
// RegisterAstraProfileKeyedServices
// =========================================================================
//
// Registers all Astra ProfileKeyedService factories with Chromium's
// BrowserContextDependencyManager.  This ensures that Astra services are
// created alongside other profile-keyed services during profile initialization,
// and that service dependencies are tracked correctly by the dependency graph.
//
// In Chromium, each ProfileKeyedServiceFactory registers itself with the
// BrowserContextDependencyManager in its constructor.  Calling GetInstance()
// on each factory singleton triggers construction, which registers the
// factory in the dependency graph.
//
// This function should be called from a Chromium patch point in the profile
// keyed service registration pipeline.
//
// Chromium pattern: RegisterProfileKeyedServices() in
//   chrome/browser/profiles/profile_keyed_service_factory*.cc
//
// Patch point: chrome/browser/profiles/profile_keyed_service_factory_desktop.cc
//   (or the platform equivalent) — add a call to this function alongside
//   the registration of other Chromium service factories.
//
// TODO(astra): Wire this into Chromium's profile keyed service registration
//   pipeline.  The specific patch point depends on the platform:
//     - Desktop: chrome/browser/profiles/profile_keyed_service_factory_desktop.cc
//     - Android: chrome/browser/profiles/profile_keyed_service_factory_android.cc
//     - Ash:     chrome/browser/profiles/profile_keyed_service_factory_ash.cc
//   Each file has a RegisterProfileKeyedServices() function that we can
//   patch to call astra::RegisterAstraProfileKeyedServices().
//
// TODO(astra): Add DependsOn() calls to factories that depend on other
//   services.  For example, AstraThemeService depends on
//   AstraWorkspaceService (for per-workspace accent colors).  Using
//   DependsOn() ensures correct construction and destruction ordering.
//   Chromium pattern: factory constructor calls DependsOn(other_factory).
// =========================================================================

void RegisterAstraProfileKeyedServices() {
  // Accessing each factory singleton triggers its constructor, which
  // registers the factory with BrowserContextDependencyManager.
  //
  // Order follows logical dependency order:
  //   - Core metadata services first (workspace, favorite, notes)
  //   - Projection services next (reading list, NTP)
  //   - Feature services (focus mode, memory saver, PiP, screenshot)
  //   - UI support services (accessibility, tab stack, theme)

  AstraWorkspaceServiceFactory::GetInstance();
  AstraFavoriteServiceFactory::GetInstance();
  AstraNoteServiceFactory::GetInstance();
  AstraReadingListServiceFactory::GetInstance();
  AstraHistoryHelperFactory::GetInstance();
  AstraDownloadsHelperFactory::GetInstance();
  AstraAutofillHelperFactory::GetInstance();
  AstraFocusModeServiceFactory::GetInstance();
  AstraMemorySaverServiceFactory::GetInstance();
  AstraScreenshotServiceFactory::GetInstance();
  AstraSafetyHelperFactory::GetInstance();
  AstraSyncHelperFactory::GetInstance();
  AstraNotificationServiceFactory::GetInstance();
  AstraTabStackServiceFactory::GetInstance();
  AstraNewTabPageServiceFactory::GetInstance();
  AstraPipServiceFactory::GetInstance();
  AstraAccessibilityServiceFactory::GetInstance();
  AstraThemeServiceFactory::GetInstance();
}

// =========================================================================
// RegisterAstraProfilePrefs
// =========================================================================
//
// Registers all Astra profile-scoped preferences with the given registry.
//
// This is a convenience function that registers prefs for all Astra services.
// In production, each service factory's RegisterProfilePrefs method should
// be called individually as part of the Chromium pref registration pipeline.
// This collective function is useful for tests and for the patch point
// where Astra prefs need to be registered all at once.
//
// Chromium pattern: chrome/browser/prefs/browser_prefs.cc RegisterUserPrefs()
//
// TODO(astra): Wire pref registration into Chromium's browser_prefs.cc
//   or the profile pref initialization path.  Two options:
//     1. Call RegisterAstraProfilePrefs from chrome/browser/prefs/browser_prefs.cc
//     2. Let each factory register its own prefs via the factory system
//
// Option 2 is more Chromium-idiomatic but requires factory registration to
// happen before profile initialization.  Option 1 is simpler for the
// overlay patch.
// =========================================================================

void RegisterAstraProfilePrefs(PrefRegistrySimple* registry) {
  AstraWorkspaceServiceFactory::RegisterProfilePrefs(registry);
  AstraFavoriteServiceFactory::RegisterProfilePrefs(registry);
  AstraNoteServiceFactory::RegisterProfilePrefs(registry);
  AstraReadingListServiceFactory::RegisterProfilePrefs(registry);
  AstraHistoryHelperFactory::RegisterProfilePrefs(registry);
  AstraDownloadsHelperFactory::RegisterProfilePrefs(registry);
  AstraAutofillHelperFactory::RegisterProfilePrefs(registry);
  AstraFocusModeServiceFactory::RegisterProfilePrefs(registry);
  AstraMemorySaverServiceFactory::RegisterProfilePrefs(registry);
  AstraScreenshotServiceFactory::RegisterProfilePrefs(registry);
  AstraSafetyHelperFactory::RegisterProfilePrefs(registry);
  AstraSyncHelperFactory::RegisterProfilePrefs(registry);
  AstraNotificationServiceFactory::RegisterProfilePrefs(registry);
  AstraTabStackServiceFactory::RegisterProfilePrefs(registry);
  AstraNewTabPageServiceFactory::RegisterProfilePrefs(registry);
  AstraPipServiceFactory::RegisterProfilePrefs(registry);
  AstraAccessibilityServiceFactory::RegisterProfilePrefs(registry);
  AstraThemeServiceFactory::RegisterProfilePrefs(registry);
}

}  // namespace astra
