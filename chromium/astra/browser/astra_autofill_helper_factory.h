// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASTRA_BROWSER_ASTRA_AUTOFILL_HELPER_FACTORY_H_
#define ASTRA_BROWSER_ASTRA_AUTOFILL_HELPER_FACTORY_H_

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class PrefRegistrySimple;
class Profile;

namespace content {
class BrowserContext;
}

namespace astra {

class AstraAutofillHelper;

// =========================================================================
// Factory for AstraAutofillHelper
// =========================================================================
//
// ProfileKeyedServiceFactory for AstraAutofillHelper.
//
// Incognito behavior: kRedirectedToOriginal — autofill data is shared
// across regular and incognito windows in the same profile. Autofill
// suggestions from saved profiles work in incognito mode.
//
// Guest: kOwnInstance — guest sessions get their own autofill state.
// System: kNone — system profile has no user autofill data.
//
// Chromium pattern: ProfileKeyedServiceFactory
//   (chrome/browser/profiles/profile_keyed_service_factory.h)
// =========================================================================

class AstraAutofillHelperFactory : public ProfileKeyedServiceFactory {
 public:
  // Returns the AstraAutofillHelper instance for |profile|.
  // Returns nullptr for system profiles.
  static AstraAutofillHelper* GetForProfile(Profile* profile);

  // Returns the singleton factory instance.
  static AstraAutofillHelperFactory* GetInstance();

  // Registers autofill-related profile prefs on the given registry.
  // Autofill prefs are part of the shared Astra pref set.
  //
  // Chromium component: PrefRegistry / PrefService.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  friend class base::NoDestructor<AstraAutofillHelperFactory>;

  AstraAutofillHelperFactory();
  ~AstraAutofillHelperFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_AUTOFILL_HELPER_FACTORY_H_
