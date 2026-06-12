#ifndef ASTRA_APP_ASTRA_BROWSER_MAIN_EXTRA_PARTS_H_
#define ASTRA_APP_ASTRA_BROWSER_MAIN_EXTRA_PARTS_H_

#include "chrome/browser/chrome_browser_main_extra_parts.h"

class Profile;

namespace astra {

// Registered from ChromeBrowserMainParts. This is the top-level Astra hook for
// direct Chromium builds; it must not initialize a parallel browser runtime.
class AstraBrowserMainExtraParts final : public ChromeBrowserMainExtraParts {
 public:
  AstraBrowserMainExtraParts();
  AstraBrowserMainExtraParts(const AstraBrowserMainExtraParts&) = delete;
  AstraBrowserMainExtraParts& operator=(const AstraBrowserMainExtraParts&) =
      delete;
  ~AstraBrowserMainExtraParts() override;

  void PreProfileInit() override;
  void PostProfileInit(Profile* profile, bool is_initial_profile) override;
  void PostBrowserStart() override;
  void PostMainMessageLoopRun() override;
};

}  // namespace astra

#endif  // ASTRA_APP_ASTRA_BROWSER_MAIN_EXTRA_PARTS_H_
