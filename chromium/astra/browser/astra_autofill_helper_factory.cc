// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_autofill_helper_factory.h"

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/public/browser/browser_context.h"

#include "astra/browser/astra_autofill_helper.h"
#include "astra/browser/astra_prefs.h"

namespace astra {

// =========================================================================
// Construction / destruction
// =========================================================================

AstraAutofillHelperFactory::AstraAutofillHelperFactory()
    : ProfileKeyedServiceFactory(
          "AstraAutofillHelper",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kRedirectedToOriginal)
              // Incognito: redirect to original profile.
              // Autofill data is shared between regular and incognito.
              .WithGuest(ProfileSelection::kOwnInstance)
              // Guest: own instance — guest sessions get their own
              // ephemeral autofill data.
              .WithSystem(ProfileSelection::kNone)
              // System profile: no user autofill data.
              .Build()) {}

AstraAutofillHelperFactory::~AstraAutofillHelperFactory() = default;

// =========================================================================
// Singleton access
// =========================================================================

// static
AstraAutofillHelperFactory* AstraAutofillHelperFactory::GetInstance() {
  static base::NoDestructor<AstraAutofillHelperFactory> instance;
  return instance.get();
}

// static
AstraAutofillHelper* AstraAutofillHelperFactory::GetForProfile(
    Profile* profile) {
  if (!profile) {
    return nullptr;
  }
  return static_cast<AstraAutofillHelper*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// =========================================================================
// Pref registration
// =========================================================================

// static
void AstraAutofillHelperFactory::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  // Autofill presentation preferences.
  //
  // Autofill data is fully owned by Chromium's autofill subsystem.
  // Astra only projects the state and adds presentation preferences.
  //
  // Chromium component: PersonalDataManager
  //   (components/autofill/core/browser/personal_data_manager.h)
  // Chromium component: AutofillProfile
  //   (components/autofill/core/browser/data_model/autofill_profile.h)
  // Chromium component: CreditCard
  //   (components/autofill/core/browser/data_model/credit_card.h)
  //
  // These Astra-specific prefs control only presentation — they never
  // store autofill data. See AstraAutofillHelper for the projection
  // layer that reads from PersonalDataManager.

  // Whether autofill is globally enabled in Astra UI.
  // Default: true — autofill is on by default.
  registry->RegisterBooleanPref(prefs::kPrefAutofillEnabled,
                                prefs::kDefaultAutofillEnabled);

  // Whether address autofill is enabled in Astra UI.
  // Default: true — address autofill is on by default.
  registry->RegisterBooleanPref(prefs::kPrefAddressAutofillEnabled,
                                prefs::kDefaultAddressAutofillEnabled);

  // Whether credit card autofill is enabled in Astra UI.
  // Default: true — credit card autofill is on by default.
  registry->RegisterBooleanPref(prefs::kPrefCreditCardAutofillEnabled,
                                prefs::kDefaultCreditCardAutofillEnabled);

  // Whether auto sign-in is enabled in Astra UI.
  // Auto sign-in automatically fills and submits login forms.
  // Default: true — auto sign-in is on by default.
  registry->RegisterBooleanPref(prefs::kPrefAutosignInEnabled,
                                prefs::kDefaultAutosignInEnabled);

  // Whether the autofill popup is shown.
  // Default: true — popup is shown by default.
  registry->RegisterBooleanPref(prefs::kPrefShowAutofillPopup,
                                prefs::kDefaultShowAutofillPopup);

  // Autofill popup position.
  // Values: "below_field", "above_field", "auto".
  // Default: "auto".
  registry->RegisterStringPref(prefs::kPrefAutofillPopupPosition,
                               prefs::kDefaultAutofillPopupPosition);

  // Maximum number of suggestions shown in the autofill popup.
  // Default: 6.
  // Clamped: 1 to 20.
  registry->RegisterIntegerPref(prefs::kPrefAutofillMaxSuggestions,
                                prefs::kDefaultAutofillMaxSuggestions);

  // Whether suggestion icons are shown.
  // Default: true — icons are shown by default.
  registry->RegisterBooleanPref(prefs::kPrefShowSuggestionIcons,
                                prefs::kDefaultShowSuggestionIcons);

  // Whether suggestion labels are shown.
  // Default: true — labels are shown by default.
  registry->RegisterBooleanPref(prefs::kPrefShowSuggestionLabels,
                                prefs::kDefaultShowSuggestionLabels);

  // Whether suggestion subtext is shown.
  // Default: true — subtext is shown by default.
  registry->RegisterBooleanPref(prefs::kPrefShowSuggestionSubtext,
                                prefs::kDefaultShowSuggestionSubtext);

  // Whether autofill triggers on tap/click.
  // Default: true — autofill on tap is on by default.
  registry->RegisterBooleanPref(prefs::kPrefAutofillOnTap,
                                prefs::kDefaultAutofillOnTap);

  // Suggestions sort order.
  // Values: "most_recent", "most_used", "alphabetical".
  // Default: "most_recent".
  registry->RegisterStringPref(prefs::kPrefAutofillSuggestionsSortOrder,
                               prefs::kDefaultAutofillSuggestionsSortOrder);

  // Whether credit card network icons are shown.
  // Default: true — card icons are shown by default.
  registry->RegisterBooleanPref(prefs::kPrefShowCreditCardIcons,
                                prefs::kDefaultShowCreditCardIcons);

  // Whether quick checkout flow is enabled.
  // Quick checkout provides a streamlined payment flow.
  // Default: false — off by default (experimental feature).
  registry->RegisterBooleanPref(prefs::kPrefAutofillQuickCheckout,
                                prefs::kDefaultAutofillQuickCheckout);
}

// =========================================================================
// Service construction
// =========================================================================

std::unique_ptr<KeyedService>
AstraAutofillHelperFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  if (!profile) {
    return nullptr;
  }
  return std::make_unique<AstraAutofillHelper>(profile);
}

}  // namespace astra
