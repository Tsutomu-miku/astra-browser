#include "browser/chromium/ChromiumWebContents.h"

#include "browser/chromium/ChromiumWebContentsClient.h"

#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"

namespace astra {

ChromiumWebContents::ChromiumWebContents(const std::string& id,
                                         const std::string& url,
                                         const std::string& title,
                                         bool incognito)
    : id_(id),
      title_(title),
      url_(url),
      incognito_(incognito) {}

ChromiumWebContents::~ChromiumWebContents() {
  if (client_ && client_->browser()) {
    // Trigger graceful close
    client_->browser()->GetHost()->CloseBrowser(false);
  }
}

void ChromiumWebContents::CreateNativeView(void* parent_nsview) {
  DCHECK(!client_);
  client_ = new ChromiumWebContentsClient(this);

  CefWindowInfo window_info;
  // On macOS, we embed the CEF browser as a child NSView
  CefRect initial_frame(0, 0, 800, 600);
  window_info.SetAsChild(
      reinterpret_cast<cef_window_handle_t>(parent_nsview), initial_frame);

  CefBrowserSettings settings;
  // TODO: configure browser settings (fonts, JS, etc.)

  // TODO: use incognito request context
  CefBrowserHost::CreateBrowser(window_info, client_, url_, settings,
                                nullptr, nullptr);
}

// ===== Navigation =====

void ChromiumWebContents::Navigate(const std::string& url) {
  if (client_ && client_->browser()) {
    client_->browser()->GetMainFrame()->LoadURL(url);
  } else {
    url_ = url;  // Will be used when browser is created
  }
}

void ChromiumWebContents::GoBack() {
  if (client_ && client_->browser()) {
    client_->browser()->GoBack();
  }
}

void ChromiumWebContents::GoForward() {
  if (client_ && client_->browser()) {
    client_->browser()->GoForward();
  }
}

void ChromiumWebContents::Reload() {
  if (client_ && client_->browser()) {
    client_->browser()->Reload();
  }
}

void ChromiumWebContents::ReloadBypassingCache() {
  if (client_ && client_->browser()) {
    client_->browser()->ReloadIgnoreCache();
  }
}

void ChromiumWebContents::Stop() {
  if (client_ && client_->browser()) {
    client_->browser()->StopLoad();
  }
}

// ===== Properties =====

void ChromiumWebContents::SetPinned(bool pinned) {
  if (pinned_ == pinned) return;
  pinned_ = pinned;
  if (observer_delegate_) {
    observer_delegate_->OnTabPinnedChanged(this, pinned_);
  }
}

void ChromiumWebContents::SetMuted(bool muted) {
  if (muted_ == muted) return;
  muted_ = muted;
  if (client_ && client_->browser()) {
    client_->browser()->GetHost()->SetAudioMuted(muted);
  }
  if (observer_delegate_) {
    observer_delegate_->OnTabMutedChanged(this, muted_);
  }
}

void* ChromiumWebContents::GetNativeView() const {
  if (client_ && client_->browser()) {
    return reinterpret_cast<void*>(
        client_->browser()->GetHost()->GetWindowHandle());
  }
  return nullptr;
}

// ===== Find in page =====

void ChromiumWebContents::Find(const std::string& search_text,
                               bool forward,
                               bool match_case,
                               bool find_next) {
  if (!client_ || !client_->browser()) return;

  client_->browser()->GetHost()->Find(search_text, forward, match_case,
                                       find_next);
}

void ChromiumWebContents::StopFinding(bool clear_selection) {
  if (client_ && client_->browser()) {
    client_->browser()->GetHost()->StopFinding(clear_selection);
  }
}

// ===== DevTools =====

void ChromiumWebContents::ShowDevTools() {
  if (!client_ || !client_->browser()) return;

  CefWindowInfo window_info;
  CefBrowserSettings settings;
  client_->browser()->GetHost()->ShowDevTools(
      window_info, client_, settings, CefPoint());
}

void ChromiumWebContents::CloseDevTools() {
  if (client_ && client_->browser()) {
    client_->browser()->GetHost()->CloseDevTools();
  }
}

bool ChromiumWebContents::HasDevTools() const {
  if (client_ && client_->browser()) {
    return client_->browser()->GetHost()->HasDevTools();
  }
  return false;
}

// ===== Zoom =====

double ChromiumWebContents::GetZoomLevel() const {
  if (client_ && client_->browser()) {
    return client_->browser()->GetHost()->GetZoomLevel();
  }
  return 0.0;
}

void ChromiumWebContents::SetZoomLevel(double level) {
  if (client_ && client_->browser()) {
    client_->browser()->GetHost()->SetZoomLevel(level);
  }
}

// ===== Printing =====

void ChromiumWebContents::Print() {
  if (client_ && client_->browser()) {
    client_->browser()->GetHost()->Print();
  }
}

// ===== Internal event handlers (called by ChromiumWebContentsClient) =====

void ChromiumWebContents::OnBrowserCreated() {
  // Browser is ready — update loading state
  nav_state_.is_loading = true;
  if (observer_delegate_) {
    observer_delegate_->OnLoadingStateChanged(this, nav_state_);
  }
}

void ChromiumWebContents::OnBrowserClosing() {
  // Browser is about to close
}

void ChromiumWebContents::OnTitleChanged(const std::string& title) {
  title_ = title;
  if (observer_delegate_) {
    observer_delegate_->OnTitleChanged(this, title_);
  }
}

void ChromiumWebContents::OnURLChanged(const std::string& url) {
  url_ = url;
  if (observer_delegate_) {
    observer_delegate_->OnURLChanged(this, url_);
  }
}

void ChromiumWebContents::OnFaviconChanged(const std::string& favicon_url) {
  favicon_url_ = favicon_url;
  if (observer_delegate_) {
    observer_delegate_->OnFaviconChanged(this, favicon_url_);
  }
}

void ChromiumWebContents::OnLoadingStateChanged(bool is_loading,
                                                bool can_go_back,
                                                bool can_go_forward) {
  nav_state_.is_loading = is_loading;
  nav_state_.can_go_back = can_go_back;
  nav_state_.can_go_forward = can_go_forward;
  if (observer_delegate_) {
    observer_delegate_->OnLoadingStateChanged(this, nav_state_);
  }
}

void ChromiumWebContents::OnLoadProgressChanged(double progress) {
  nav_state_.load_progress = progress;
  if (observer_delegate_) {
    observer_delegate_->OnLoadingStateChanged(this, nav_state_);
  }
}

void ChromiumWebContents::OnFindResult(int match_count,
                                       int active_match_ordinal,
                                       bool final_update) {
  // TODO: add find result to observer interface
  // For now, stored internally — can be queried later
}

}  // namespace astra
