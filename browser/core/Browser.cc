#include "browser/core/Browser.h"

namespace astra {

BrowserApp* g_browser_app = nullptr;

BrowserApp* BrowserApp::Get() {
  return g_browser_app;
}

}  // namespace astra
