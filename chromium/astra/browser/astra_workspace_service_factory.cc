#include "astra/browser/astra_workspace_service_factory.h"

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/public/browser/browser_context.h"

#include "astra/browser/astra_prefs.h"
#include "astra/browser/astra_workspace_service.h"

namespace astra {

// =========================================================================
// Construction / destruction
// =========================================================================

AstraWorkspaceServiceFactory::AstraWorkspaceServiceFactory()
    : ProfileKeyedServiceFactory(
          "AstraWorkspaceService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kRedirectedToOriginal)
              // Incognito: redirect to original profile.
              // Workspace definitions are product-level state that should
              // reflect the user's main profile.  An incognito window is
              // still the same user with the same workspace set — only the
              // browsing session is isolated.
              .WithGuest(ProfileSelection::kOwnInstance)
              // Guest: own instance (no backing profile to redirect to).
              .WithSystem(ProfileSelection::kNone)
              // System profile: no user workspaces.
              .Build()) {}

AstraWorkspaceServiceFactory::~AstraWorkspaceServiceFactory() = default;

// =========================================================================
// Singleton access
// =========================================================================

// static
AstraWorkspaceServiceFactory* AstraWorkspaceServiceFactory::GetInstance() {
  static base::NoDestructor<AstraWorkspaceServiceFactory> instance;
  return instance.get();
}

// static
AstraWorkspaceService* AstraWorkspaceServiceFactory::GetForProfile(
    Profile* profile) {
  if (!profile) {
    return nullptr;
  }
  return static_cast<AstraWorkspaceService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// =========================================================================
// Pref registration
// =========================================================================

// static
void AstraWorkspaceServiceFactory::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  // Workspace prefs are registered as part of the shared Astra pref set.
  // See astra::prefs::RegisterProfilePrefs in astra_prefs.h.
  //
  // TODO(astra): Register all workspace prefs here instead of relying
  //   on the shared pref registration.  Each service factory should own
  //   the registration of its own prefs, following Chromium's pattern
  //   where each ProfileKeyedServiceFactory registers its own prefs.
  // Chromium pattern: each service's factory has a
  //   RegisterProfilePrefs() method called during profile initialization.
  astra::prefs::RegisterProfilePrefs(registry);
}

// =========================================================================
// Service construction
// =========================================================================

std::unique_ptr<KeyedService>
AstraWorkspaceServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  if (!profile) {
    return nullptr;
  }
  return std::make_unique<AstraWorkspaceService>(profile);
}

}  // namespace astra
