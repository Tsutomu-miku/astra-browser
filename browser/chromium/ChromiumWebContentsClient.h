#pragma once

#include "browser/core/Browser.h"

#include "include/cef_client.h"
#include "include/cef_life_span_handler.h"
#include "include/cef_load_handler.h"
#include "include/cef_display_handler.h"
#include "include/cef_find_handler.h"

namespace astra {

class ChromiumWebContents;

// Internal CEF client that forwards events to ChromiumWebContents.
// One client per WebContents (one per tab).
class ChromiumWebContentsClient
    : public CefClient,
      public CefLifeSpanHandler,
      public CefLoadHandler,
      public CefDisplayHandler,
      public CefFindHandler {
 public:
  explicit ChromiumWebContentsClient(ChromiumWebContents* web_contents);
  ~ChromiumWebContentsClient() override;

  // CefClient
  CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
  CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
  CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }
  CefRefPtr<CefFindHandler> GetFindHandler() override { return this; }

  // CefLifeSpanHandler
  void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
  void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

  // CefLoadHandler
  void OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                            bool isLoading,
                            bool canGoBack,
                            bool canGoForward) override;

  // CefDisplayHandler
  void OnTitleChange(CefRefPtr<CefBrowser> browser,
                     const CefString& title) override;
  void OnAddressChange(CefRefPtr<CefBrowser> browser,
                       CefRefPtr<CefFrame> frame,
                       const CefString& url) override;
  void OnFaviconURLChange(CefRefPtr<CefBrowser> browser,
                          const std::vector<CefString>& icon_urls) override;

  // CefFindHandler
  void OnFindResult(CefRefPtr<CefBrowser> browser,
                    int identifier,
                    int count,
                    const CefRect& selectionRect,
                    int activeMatchOrdinal,
                    bool finalUpdate) override;

  CefRefPtr<CefBrowser> browser() const { return browser_; }

 private:
  ChromiumWebContents* web_contents_;  // not owned
  CefRefPtr<CefBrowser> browser_;

  IMPLEMENT_REFCOUNTING(ChromiumWebContentsClient);
  DISALLOW_COPY_AND_ASSIGN(ChromiumWebContentsClient);
};

}  // namespace astra
