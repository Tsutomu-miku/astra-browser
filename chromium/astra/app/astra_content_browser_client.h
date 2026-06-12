#ifndef ASTRA_APP_ASTRA_CONTENT_BROWSER_CLIENT_H_
#define ASTRA_APP_ASTRA_CONTENT_BROWSER_CLIENT_H_

namespace astra {

// Thin patch-point helper for ChromeContentBrowserClient integration.
// Keep this class small: ChromeContentBrowserClient should continue to own
// network, permissions, downloads, file access, and security defaults.
class AstraContentBrowserClient {
 public:
  AstraContentBrowserClient() = delete;

  static void InstallChromeContentBrowserClientHooks();
};

}  // namespace astra

#endif  // ASTRA_APP_ASTRA_CONTENT_BROWSER_CLIENT_H_
