// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_theme_service_factory.h"

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/public/browser/browser_context.h"

#include "astra/browser/astra_prefs.h"
#include "astra/browser/astra_theme_service.h"

namespace astra {

// =========================================================================
// Construction / destruction
// =========================================================================

AstraThemeServiceFactory::AstraThemeServiceFactory()
    : ProfileKeyedServiceFactory(
          "AstraThemeService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kRedirectedToOriginal)
              // Incognito: redirect to original profile.
              // Theme state is user-level, not per-context.
              .WithGuest(ProfileSelection::kOwnInstance)
              // Guest: own instance (no backing profile to redirect to).
              .WithSystem(ProfileSelection::kNone)
              // System profile: no service needed.
              .Build()) {
  // Declare dependency on AstraWorkspaceService — the workspace service
  // must be created before the theme service because we observe it.
  //
  // TODO(astra): Add DependsOn() calls once both factories are properly
  //   registered with the BrowserContextDependencyManager.
  //   Chromium pattern: DependsOn(AstraWorkspaceServiceFactory::GetInstance())
  //   This ensures correct service construction/destruction order.
  // DependsOn(AstraWorkspaceServiceFactory::GetInstance());
  //
  // Also depend on ThemeServiceFactory:
  // DependsOn(ThemeServiceFactory::GetInstance());
}

AstraThemeServiceFactory::~AstraThemeServiceFactory() = default;

// =========================================================================
// Singleton access
// =========================================================================

// static
AstraThemeServiceFactory* AstraThemeServiceFactory::GetInstance() {
  static base::NoDestructor<AstraThemeServiceFactory> instance;
  return instance.get();
}

// static
AstraThemeService* AstraThemeServiceFactory::GetForProfile(Profile* profile) {
  if (!profile) {
    return nullptr;
  }
  return static_cast<AstraThemeService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// =========================================================================
// Pref registration
// =========================================================================

// static
void AstraThemeServiceFactory::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  // Register all theme-related prefs.
  //
  // These prefs control Astra's theme and accent color behavior.
  // Persistence is entirely through Chromium's PrefService — no custom
  // file I/O.
  //
  // Chromium pattern: each service's factory has a
  //   RegisterProfilePrefs() method called during profile initialization.

  // Theme preset (system/light/dark/auto).
  // Default: kSystem — follow the OS/system theme.
  registry->RegisterIntegerPref(
      "astra.theme.preset",
      static_cast<int>(AstraThemePreset::kSystem));

  // Accent color override (hex string, empty = no override).
  // Default: empty — use workspace accent color.
  registry->RegisterStringPref("astra.theme.accent_override", std::string());

  // Active theme scheme (string, empty = no named scheme).
  registry->RegisterStringPref("astra.theme.scheme", std::string());

  // Whether to use workspace accent for theming.
  // Default: true — accent follows active workspace.
  registry->RegisterBooleanPref("astra.theme.use_workspace_accent", true);

  // Custom accent color (stored as integer, ARGB format).
  registry->RegisterIntegerPref("astra.theme.custom_accent_color",
                                0xFF5B8FF9);  // Google Blue 500

  // High contrast mode.
  registry->RegisterBooleanPref("astra.theme.use_high_contrast", false);

  // Show accent on tab strips.
  registry->RegisterBooleanPref("astra.theme.show_accent_on_tabs", true);

  // Show accent on sidebar.
  registry->RegisterBooleanPref("astra.theme.show_accent_on_sidebar", true);

  // Accent intensity (0.5 - 2.0).
  registry->RegisterDoublePref("astra.theme.accent_intensity", 1.0);

  // Auto theme scheduling.
  registry->RegisterBooleanPref("astra.theme.use_auto_theme_schedule", false);

  // Auto theme light start time.
  registry->RegisterStringPref("astra.theme.auto_theme_light_start", "07:00");

  // Auto theme dark start time.
  registry->RegisterStringPref("astra.theme.auto_theme_dark_start", "19:00");
}

// =========================================================================
// Service construction
// =========================================================================

std::unique_ptr<KeyedService>
AstraThemeServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  if (!profile) {
    return nullptr;
  }
  return std::make_unique<AstraThemeService>(profile);
}

}  // namespace astra
