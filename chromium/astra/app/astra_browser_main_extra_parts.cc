#include "astra/app/astra_browser_main_extra_parts.h"

#include "astra/browser/astra_workspace_service.h"
#include "chrome/browser/profiles/profile.h"

namespace astra {

AstraBrowserMainExtraParts::AstraBrowserMainExtraParts() = default;
AstraBrowserMainExtraParts::~AstraBrowserMainExtraParts() = default;

void AstraBrowserMainExtraParts::PreProfileInit() {
  // TODO(astra): Register Astra feature flags, resources, and keyed-service
  // factories here. Do not create Profile, Browser, or WebContents manually.
}

void AstraBrowserMainExtraParts::PostProfileInit(Profile* profile,
                                                 bool is_initial_profile) {
  if (!profile || !is_initial_profile) {
    return;
  }

  // Ensure the product-only workspace service is constructed for the initial
  // profile. Chromium remains the owner of Profile and browser services.
  AstraWorkspaceServiceFactory::GetForProfile(profile);
}

void AstraBrowserMainExtraParts::PostBrowserStart() {
  // TODO(astra): Attach metrics, command hooks, and UI experiments after Chrome
  // browser startup is complete.
}

void AstraBrowserMainExtraParts::PostMainMessageLoopRun() {
  // TODO(astra): Flush Astra-only metadata. Chromium services own their own
  // shutdown paths.
}

}  // namespace astra
