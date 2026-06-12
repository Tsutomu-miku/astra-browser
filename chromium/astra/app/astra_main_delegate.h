#ifndef ASTRA_APP_ASTRA_MAIN_DELEGATE_H_
#define ASTRA_APP_ASTRA_MAIN_DELEGATE_H_

class ChromeBrowserMainParts;

namespace astra {

// Helper invoked from the Chrome main delegate patch. It registers Astra
// extra parts without replacing ChromeMainDelegate.
class AstraMainDelegate {
 public:
  AstraMainDelegate() = delete;

  static void RegisterBrowserMainExtraParts(
      ChromeBrowserMainParts* main_parts);
};

}  // namespace astra

#endif  // ASTRA_APP_ASTRA_MAIN_DELEGATE_H_
