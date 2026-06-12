#pragma once

#include "browser/core/Browser.h"

#include <string>
#include <memory>

#include "include/base/cef_ref_counted.h"
#include "include/internal/cef_ptr.h"

namespace astra {

class ChromiumWebContentsClient;

// Chromium-backed implementation of WebContents.
// Wraps a CefBrowser and forwards events to BrowserObservers.
class ChromiumWebContents : public WebContents {
 public:
  ChromiumWebContents(const std::string& id,
                      const std::string& url,
                      const std::string& title,
                      bool incognito = false);
  ~ChromiumWebContents() override;

  // Create the underlying native browser view parented to the given NSView.
  // Must be called on the UI thread after the native view hierarchy is ready.
  void CreateNativeView(void* parent_nsview) override;

  // Identity
  const std::string& id() const override { return id_; }
  const std::string& title() const override { return title_; }
  const std::string& url() const override { return url_; }
  const std::string& favicon_url() const override { return favicon_url_; }

  // Navigation
  const NavigationState& navigation_state() const override { return nav_state_; }
  void Navigate(const std::string& url) override;
  void GoBack() override;
  void GoForward() override;
  void Reload() override;
  void ReloadBypassingCache() override;
  void Stop() override;

  // Properties
  bool is_incognito() const override { return incognito_; }
  bool is_pinned() const override { return pinned_; }
  bool is_muted() const override { return muted_; }
  void SetPinned(bool pinned) override;
  void SetMuted(bool muted) override;

  // Native view (NSView* on macOS)
  void* GetNativeView() const override;

  // Find in page
  void Find(const std::string& search_text,
            bool forward,
            bool match_case,
            bool find_next) override;
  void StopFinding(bool clear_selection) override;

  // DevTools
  void ShowDevTools() override;
  void CloseDevTools() override;
  bool HasDevTools() const override;

  // Zoom
  double GetZoomLevel() const override;
  void SetZoomLevel(double level) override;

  // Printing
  void Print() override;

  // Internal — called by ChromiumWebContentsClient when events arrive
  void OnBrowserCreated();
  void OnBrowserClosing();
  void OnTitleChanged(const std::string& title);
  void OnURLChanged(const std::string& url);
  void OnFaviconChanged(const std::string& favicon_url);
  void OnLoadingStateChanged(bool is_loading,
                             bool can_go_back,
                             bool can_go_forward);
  void OnLoadProgressChanged(double progress);
  void OnFindResult(int match_count,
                    int active_match_ordinal,
                    bool final_update);

  // Set the observer delegate (the Browser that owns this WebContents).
  // Called by ChromiumBrowser when adding the tab.
  void set_observer_delegate(BrowserObserver* delegate) {
    observer_delegate_ = delegate;
  }

 private:
  std::string id_;
  std::string title_;
  std::string url_;
  std::string favicon_url_;
  NavigationState nav_state_;
  bool incognito_ = false;
  bool pinned_ = false;
  bool muted_ = false;
  int find_identifier_ = 0;

  CefRefPtr<ChromiumWebContentsClient> client_;
  BrowserObserver* observer_delegate_ = nullptr;  // not owned

  friend class ChromiumWebContentsClient;
};

}  // namespace astra
