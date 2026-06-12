#pragma once

#include "browser/core/Browser.h"

#include <string>
#include <vector>
#include <memory>
#include <algorithm>

namespace astra {

class ChromiumWebContents;

// Chromium-backed implementation of Browser (window-level tab container).
// Owns a list of WebContents and manages the active tab.
class ChromiumBrowser : public Browser, public BrowserObserver {
 public:
  explicit ChromiumBrowser(bool incognito = false);
  ~ChromiumBrowser() override;

  // Tab management
  WebContents* AddTab(const std::string& url,
                      const std::string& title,
                      bool activate = true) override;
  WebContents* AddIncognitoTab(const std::string& url,
                               const std::string& title,
                               bool activate = true) override;
  void CloseTab(const std::string& tab_id) override;
  void ActivateTab(const std::string& tab_id) override;
  void MoveTab(size_t from_index, size_t to_index) override;

  // Query
  WebContents* GetActiveTab() const override;
  WebContents* GetTab(const std::string& tab_id) const override;
  const std::vector<WebContents*>& tabs() const override;
  size_t tab_count() const override;
  bool is_incognito() const override { return incognito_; }

  // Observers
  void AddObserver(BrowserObserver* observer) override;
  void RemoveObserver(BrowserObserver* observer) override;

  // Create the native browser view (NSView*) for the active tab.
  // Must be called after the native window is set up.
  void CreateActiveTabView(void* parent_nsview);

 private:
  // BrowserObserver — forward events from WebContents to our observers
  void OnTitleChanged(WebContents* tab, const std::string& title) override;
  void OnURLChanged(WebContents* tab, const std::string& url) override;
  void OnFaviconChanged(WebContents* tab, const std::string& favicon_url) override;
  void OnLoadingStateChanged(WebContents* tab,
                             const NavigationState& state) override;
  void OnTabPinnedChanged(WebContents* tab, bool pinned) override;
  void OnTabMutedChanged(WebContents* tab, bool muted) override;

  // Notify all observers
  void NotifyTabAdded(WebContents* tab);
  void NotifyTabRemoved(const std::string& tab_id);
  void NotifyActiveTabChanged(WebContents* tab);

  std::string GenerateTabId();

  bool incognito_ = false;
  std::string active_tab_id_;
  std::vector<std::unique_ptr<ChromiumWebContents>> owned_tabs_;
  // Cached raw-pointer vector for the tabs() accessor
  mutable std::vector<WebContents*> tab_ptrs_;
  mutable bool tab_ptrs_dirty_ = true;

  std::vector<BrowserObserver*> observers_;

  static int next_tab_id_;
};

}  // namespace astra
