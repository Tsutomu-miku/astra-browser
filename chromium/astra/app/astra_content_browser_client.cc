#include "astra/app/astra_content_browser_client.h"

namespace astra {

void AstraContentBrowserClient::InstallChromeContentBrowserClientHooks() {
  // TODO(astra): Add only product policy hooks that cannot be expressed through
  // existing Chrome prefs, policy, or keyed services.
}

}  // namespace astra
