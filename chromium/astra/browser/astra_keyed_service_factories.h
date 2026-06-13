#ifndef ASTRA_BROWSER_ASTRA_KEYED_SERVICE_FACTORIES_H_
#define ASTRA_BROWSER_ASTRA_KEYED_SERVICE_FACTORIES_H_

class PrefRegistrySimple;

namespace astra {

// =========================================================================
// Astra Keyed Service Factory Registration
// =========================================================================
//
// Top-level entry points for registering all Astra ProfileKeyedService
// factories with Chromium's dependency manager and pref registry.
//
// These functions are the integration point between Astra services and
// Chromium's profile keyed service system.  They should be called from
// Chromium patch points in the profile initialization pipeline.
//
// Chromium subsystems:
//   - BrowserContextDependencyManager
//       (components/keyed_service/content/browser_context_dependency_manager.h)
//   - ProfileKeyedServiceFactory
//       (chrome/browser/profiles/profile_keyed_service_factory.h)
//   - PrefService / PrefRegistry
//       (components/prefs/)
//
// Patch points:
//   - Factory registration: chrome/browser/profiles/profile_keyed_service_factory_*.cc
//   - Pref registration: chrome/browser/prefs/browser_prefs.cc
// =========================================================================

// Registers all Astra ProfileKeyedService factories with Chromium's
// BrowserContextDependencyManager.
//
// Call this during browser /profile initialization, before any profile is created,
// so that the dependency manager knows about all Astra factories
// and can build the dependency graph.
//
// Calling GetInstance() on each factory singleton triggers construction,
// which registers the factory with the dependency manager.
//
// Chromium pattern: RegisterProfileKeyedServices() in
//   chrome/browser/profiles/profile_keyed_service_factory_desktop.cc
//
// TODO(astra): Call this from a Chromium patch point in the profile
//   keyed service registration pipeline.
void RegisterAstraProfileKeyedServices();

// Registers all Astra profile-scoped preferences with the given registry.
//
// This delegates to each service factory's RegisterProfilePrefs method,
// which in turn registers the prefs owned by that service.
//
// Chromium pattern: RegisterUserPrefs() in chrome/browser/prefs/browser_prefs.cc
//
// TODO(astra): Call this from a Chromium patch point in the pref
//   registration pipeline, so Astra prefs are available at profile
//   profile creation time.
void RegisterAstraProfilePrefs(PrefRegistrySimple* registry);

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_KEYED_SERVICE_FACTORIES_H_
