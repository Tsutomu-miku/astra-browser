// Copyright 2026 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/browser/astra_reading_list_service_factory.h"

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/public/browser/browser_context.h"

#include "astra/browser/astra_reading_list_service.h"

namespace astra {

// =========================================================================
// Construction / destruction
// =========================================================================

AstraReadingListServiceFactory::AstraReadingListServiceFactory()
    : ProfileKeyedServiceFactory(
          "AstraReadingListService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kRedirectedToOriginal)
              // Incognito: redirect to original profile.
              // Reading list entries are profile-level data.
              .WithGuest(ProfileSelection::kOwnInstance)
              // Guest: own instance (ephemeral reading list).
              .WithSystem(ProfileSelection::kNone)
              // System profile: no reading list.
              .Build()) {}

AstraReadingListServiceFactory::~AstraReadingListServiceFactory() = default;

// =========================================================================
// Singleton access
// =========================================================================

// static
AstraReadingListServiceFactory*
AstraReadingListServiceFactory::GetInstance() {
  static base::NoDestructor<AstraReadingListServiceFactory> instance;
  return instance.get();
}

// static
AstraReadingListService* AstraReadingListServiceFactory::GetForProfile(
    Profile* profile) {
  if (!profile) {
    return nullptr;
  }
  return static_cast<AstraReadingListService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// =========================================================================
// Pref registration
// =========================================================================

// static
void AstraReadingListServiceFactory::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  // Delegate to the service's static registration method so that pref
  // key constants and their default values live alongside the service
  // implementation.
  //
  // The actual reading list data is owned by Chromium's ReadingListModel.
  // These are Astra-specific presentation prefs, folder metadata, and
  // entry-level extra metadata (favorites, tags, notes).
  //
  // Chromium owner: ReadingListModel (components/reading_list/core/)
  // Chromium owner: reading_list_model_factory.cc registers the real prefs.
  AstraReadingListService::RegisterProfilePrefs(registry);
}

// =========================================================================
// Service construction
// =========================================================================

std::unique_ptr<KeyedService>
AstraReadingListServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  if (!profile) {
    return nullptr;
  }
  return std::make_unique<AstraReadingListService>(profile);
}

}  // namespace astra
