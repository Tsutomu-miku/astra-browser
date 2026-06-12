#include "astra/browser/astra_workspace_service.h"

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile.h"

#include <utility>

namespace astra {

AstraWorkspaceService::AstraWorkspaceService(Profile* profile)
    : profile_(profile) {
  EnsureDefaultWorkspace();
}

AstraWorkspaceService::~AstraWorkspaceService() = default;

void AstraWorkspaceService::EnsureDefaultWorkspace() {
  if (!workspaces_.empty()) {
    return;
  }

  workspaces_.push_back({
      .id = "default",
      .name = "Personal",
      .accent_color = "#5B8FF9",
  });
  active_workspace_id_ = workspaces_.front().id;
}

void AstraWorkspaceService::ActivateWorkspace(
    const std::string& workspace_id) {
  // TODO(astra): Project Chrome TabStripModel contents by workspace metadata
  // instead of moving WebContents ownership outside Chromium.
  active_workspace_id_ = workspace_id;
}

void AstraWorkspaceService::AddWorkspace(AstraWorkspace workspace) {
  workspaces_.push_back(std::move(workspace));
}

AstraWorkspaceService* AstraWorkspaceServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<AstraWorkspaceService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

AstraWorkspaceServiceFactory* AstraWorkspaceServiceFactory::GetInstance() {
  static base::NoDestructor<AstraWorkspaceServiceFactory> instance;
  return instance.get();
}

AstraWorkspaceServiceFactory::AstraWorkspaceServiceFactory()
    : ProfileKeyedServiceFactory(
          "AstraWorkspaceService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOriginalOnly)
              .WithGuest(ProfileSelection::kOwnInstance)
              .Build()) {}

AstraWorkspaceServiceFactory::~AstraWorkspaceServiceFactory() = default;

KeyedService* AstraWorkspaceServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return new AstraWorkspaceService(Profile::FromBrowserContext(context));
}

}  // namespace astra
