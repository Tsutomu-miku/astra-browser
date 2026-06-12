#pragma once

#include "browser/core/Browser.h"

#include <memory>

#include "include/cef_app.h"
#include "include/cef_browser_process_handler.h"

namespace astra {

// Chromium-backed implementation of BrowserApp.
// Singleton that initializes the Chromium runtime and creates Browser windows.
//
// Currently uses CEF as the Chromium distribution, but the public API follows
// the Chromium content module pattern. The CEF dependency is an implementation
// detail hidden behind the BrowserApp interface.
class ChromiumBrowserApp
    : public BrowserApp,
      public CefApp,
      public CefBrowserProcessHandler {
 public:
  ChromiumBrowserApp();
  ~ChromiumBrowserApp() override;

  // BrowserApp interface
  bool Initialize(int argc, char* argv[]) override;
  void Shutdown() override;
  std::unique_ptr<Browser> CreateBrowser(bool incognito = false) override;
  void RunMessageLoopIteration() override;

  // CefApp methods
  CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override {
    return this;
  }

  // CefBrowserProcessHandler methods
  void OnContextInitialized() override;

  // Singleton
  static ChromiumBrowserApp* GetInstance();

 private:
  bool initialized_ = false;
  CefRefPtr<CefApp> app_ref_;  // holds self-reference for CEF refcounting

  // Incognito request context (in-memory, no persistence)
  CefRefPtr<CefRequestContext> incognito_request_context_;

  IMPLEMENT_REFCOUNTING(ChromiumBrowserApp);
  DISALLOW_COPY_AND_ASSIGN(ChromiumBrowserApp);
};

}  // namespace astra
