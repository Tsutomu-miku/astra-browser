#include "browser/chromium/ChromiumWebContentsClient.h"

#include "browser/chromium/ChromiumWebContents.h"

namespace astra {

ChromiumWebContentsClient::ChromiumWebContentsClient(
    ChromiumWebContents* web_contents)
    : web_contents_(web_contents) {}

ChromiumWebContentsClient::~ChromiumWebContentsClient() = default;

// ===== CefLifeSpanHandler =====

void ChromiumWebContentsClient::OnAfterCreated(
    CefRefPtr<CefBrowser> browser) {
  browser_ = browser;
  if (web_contents_) {
    web_contents_->OnBrowserCreated();
  }
}

void ChromiumWebContentsClient::OnBeforeClose(
    CefRefPtr<CefBrowser> browser) {
  if (web_contents_) {
    web_contents_->OnBrowserClosing();
  }
  browser_ = nullptr;
}

// ===== CefLoadHandler =====

void ChromiumWebContentsClient::OnLoadingStateChange(
    CefRefPtr<CefBrowser> browser,
    bool isLoading,
    bool canGoBack,
    bool canGoForward) {
  if (web_contents_) {
    web_contents_->OnLoadingStateChanged(isLoading, canGoBack, canGoForward);
  }
}

// ===== CefDisplayHandler =====

void ChromiumWebContentsClient::OnTitleChange(CefRefPtr<CefBrowser> browser,
                                              const CefString& title) {
  if (web_contents_) {
    web_contents_->OnTitleChanged(title);
  }
}

void ChromiumWebContentsClient::OnAddressChange(CefRefPtr<CefBrowser> browser,
                                                CefRefPtr<CefFrame> frame,
                                                const CefString& url) {
  if (web_contents_ && frame->IsMain()) {
    web_contents_->OnURLChanged(url);
  }
}

void ChromiumWebContentsClient::OnFaviconURLChange(
    CefRefPtr<CefBrowser> browser,
    const std::vector<CefString>& icon_urls) {
  if (web_contents_ && !icon_urls.empty()) {
    web_contents_->OnFaviconChanged(icon_urls.front());
  }
}

// ===== CefFindHandler =====

void ChromiumWebContentsClient::OnFindResult(CefRefPtr<CefBrowser> browser,
                                             int identifier,
                                             int count,
                                             const CefRect& /*selectionRect*/,
                                             int activeMatchOrdinal,
                                             bool finalUpdate) {
  if (web_contents_) {
    web_contents_->OnFindResult(count, activeMatchOrdinal, finalUpdate);
  }
}

}  // namespace astra
