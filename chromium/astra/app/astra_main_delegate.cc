#include "astra/app/astra_main_delegate.h"

#include "astra/app/astra_browser_main_extra_parts.h"
#include "chrome/browser/chrome_browser_main.h"

#include <memory>

namespace astra {

void AstraMainDelegate::RegisterBrowserMainExtraParts(
    ChromeBrowserMainParts* main_parts) {
  if (!main_parts) {
    return;
  }

  // TODO(astra): Wire this into ChromeBrowserMainParts construction under an
  // Astra branding/build flag.
  main_parts->AddParts(std::make_unique<AstraBrowserMainExtraParts>());
}

}  // namespace astra
