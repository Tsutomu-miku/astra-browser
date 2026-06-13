// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Astra Theme Service Factory — ProfileKeyedServiceFactory for AstraThemeService.
//
// Creates and manages AstraThemeService instances, one per profile.
//
// Incognito behavior: kRedirectedToOriginal — theme state is shared
// between regular and incognito profiles because theme is a user-level
// preference, not a browsing-context preference.  Dark mode, accent
// color, and theme selection apply to all windows of the same profile.
//
// System profile: kNone — theme service is not needed for the
// system profile (which is used for background tasks and sign-in).
//
// Dependencies:
//   - AstraWorkspaceService — for per-workspace accent colors.
//   - ThemeService — for system theme state.
//
// Chromium pattern: ProfileKeyedServiceFactory
//   (chrome/browser/profiles/profile_keyed_service_factory.h)
//
// TODO(astra): Wire this into Chromium's profile keyed service
//   registration pipeline so the service is created at profile
//   initialization time.  See AstraWorkspaceServiceFactory for the
//   registration pattern and TODOs.

#ifndef ASTRA_BROWSER_ASTRA_THEME_SERVICE_FACTORY_H_
#define ASTRA_BROWSER_ASTRA_THEME_SERVICE_FACTORY_H_

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class PrefRegistrySimple;
class Profile;

namespace content {
class BrowserContext;
}

namespace astra {

class AstraThemeService;

// =========================================================================
// Factory for AstraThemeService
// =========================================================================

class AstraThemeServiceFactory : public ProfileKeyedServiceFactory {
 public:
  // Returns the AstraThemeService instance for |profile|.
  // Returns nullptr for system profiles or other contexts where the
  // service is not available.
  static AstraThemeService* GetForProfile(Profile* profile);

  // Returns the singleton factory instance.
  static AstraThemeServiceFactory* GetInstance();

  // Registers theme-related profile prefs on the given registry.
  //
  // Currently delegates to astra::prefs::RegisterProfilePrefs which
  // covers all Astra prefs.  Theme-specific prefs (e.g. accent
  // overrides, follow-system-theme setting) can be registered here
  // when they are added.
  //
  // Chromium component: PrefRegistry / PrefService.
  // Patch point: called from factory registration or from a browser prefs
  // registration hook (e.g. chrome/browser/prefs/browser_prefs.cc).
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  friend class base::NoDestructor<AstraThemeServiceFactory>;

  AstraThemeServiceFactory();
  ~AstraThemeServiceFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_THEME_SERVICE_FACTORY_H_
