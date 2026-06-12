#ifndef ASTRA_BROWSER_ASTRA_WORKSPACE_SERVICE_H_
#define ASTRA_BROWSER_ASTRA_WORKSPACE_SERVICE_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"

class Profile;

namespace content {
class BrowserContext;
}

namespace astra {

struct AstraWorkspace {
  std::string id;
  std::string name;
  std::string accent_color;
};

// Product-only service. Chromium owns Profile, Browser, TabStripModel,
// WebContents, session, history, downloads, passwords, and permissions.
class AstraWorkspaceService final : public KeyedService {
 public:
  explicit AstraWorkspaceService(Profile* profile);
  AstraWorkspaceService(const AstraWorkspaceService&) = delete;
  AstraWorkspaceService& operator=(const AstraWorkspaceService&) = delete;
  ~AstraWorkspaceService() override;

  const std::vector<AstraWorkspace>& workspaces() const { return workspaces_; }
  const std::string& active_workspace_id() const { return active_workspace_id_; }

  void EnsureDefaultWorkspace();
  void ActivateWorkspace(const std::string& workspace_id);
  void AddWorkspace(AstraWorkspace workspace);

 private:
  raw_ptr<Profile> profile_;
  std::vector<AstraWorkspace> workspaces_;
  std::string active_workspace_id_;
};

class AstraWorkspaceServiceFactory final
    : public ProfileKeyedServiceFactory {
 public:
  static AstraWorkspaceService* GetForProfile(Profile* profile);
  static AstraWorkspaceServiceFactory* GetInstance();

 private:
  AstraWorkspaceServiceFactory();
  ~AstraWorkspaceServiceFactory() override;

  KeyedService* BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace astra

#endif  // ASTRA_BROWSER_ASTRA_WORKSPACE_SERVICE_H_
