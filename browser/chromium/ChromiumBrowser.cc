#include "browser/chromium/ChromiumBrowser.h"

#include "browser/chromium/ChromiumWebContents.h"

namespace astra {

int ChromiumBrowser::next_tab_id_ = 0;

ChromiumBrowser::ChromiumBrowser(bool incognito) : incognito_(incognito) {}

ChromiumBrowser::~ChromiumBrowser() {
  // Tabs are owned by unique_ptr — they will be cleaned up automatically
}

// ===== Tab management =====

WebContents* ChromiumBrowser::AddTab(const std::string& url,
                                      const std::string& title,
                                      bool activate) {
  auto tab = std::make_unique<ChromiumWebContents>(
      GenerateTabId(), url, title, /*incognito=*/incognito_);
  ChromiumWebContents* raw = tab.get();
  tab->set_observer_delegate(this);
  owned_tabs_.push_back(std::move(tab));
  tab_ptrs_dirty_ = true;

  NotifyTabAdded(raw);

  if (activate || active_tab_id_.empty()) {
    ActivateTab(raw->id());
  }

  return raw;
}

WebContents* ChromiumBrowser::AddIncognitoTab(const std::string& url,
                                               const std::string& title,
                                               bool activate) {
  // Incognito tabs can only be added to an incognito browser
  if (!incognito_) {
    return nullptr;
  }
  return AddTab(url, title, activate);
}

void ChromiumBrowser::CloseTab(const std::string& tab_id) {
  auto it = std::find_if(owned_tabs_.begin(), owned_tabs_.end(),
                           [&tab_id](const std::unique_ptr<ChromiumWebContents>& t) {
                             return t->id() == tab_id;
                           });
  if (it == owned_tabs_.end()) return;

  size_t index = it - owned_tabs_.begin();
  std::string removed_id = tab_id;
  bool was_active = (active_tab_id_ == tab_id);

  owned_tabs_.erase(it);
  tab_ptrs_dirty_ = true;

  NotifyTabRemoved(removed_id);

  // If we closed the active tab, activate a neighbor
  if (was_active && !owned_tabs_.empty()) {
    size_t new_index = index > 0 ? index - 1 : 0;
    if (new_index < owned_tabs_.size()) {
      ActivateTab(owned_tabs_[new_index]->id());
    }
  }
}

void ChromiumBrowser::ActivateTab(const std::string& tab_id) {
  if (active_tab_id_ == tab_id) return;

  auto* tab = GetTab(tab_id);
  if (!tab) return;

  active_tab_id_ = tab_id;
  NotifyActiveTabChanged(tab);
}

void ChromiumBrowser::MoveTab(size_t from_index, size_t to_index) {
  if (from_index >= owned_tabs_.size() ||
      to_index >= owned_tabs_.size() ||
      from_index == to_index) {
    return;
  }

  auto tab = std::move(owned_tabs_[from_index]);
  owned_tabs_.erase(owned_tabs_.begin() + from_index);
  if (to_index > from_index) to_index--;
  owned_tabs_.insert(owned_tabs_.begin() + to_index, std::move(tab));
  tab_ptrs_dirty_ = true;

  // TODO: notify tab moved
}

// ===== Query =====

WebContents* ChromiumBrowser::GetActiveTab() const {
  if (active_tab_id_.empty()) return nullptr;
  return GetTab(active_tab_id_);
}

WebContents* ChromiumBrowser::GetTab(const std::string& tab_id) const {
  for (const auto& tab : owned_tabs_) {
    if (tab->id() == tab_id) {
      return tab.get();
    }
  }
  return nullptr;
}

const std::vector<WebContents*>& ChromiumBrowser::tabs() const {
  if (tab_ptrs_dirty_) {
    tab_ptrs_.clear();
    for (const auto& tab : owned_tabs_) {
      tab_ptrs_.push_back(tab.get());
    }
    tab_ptrs_dirty_ = false;
  }
  return tab_ptrs_;
}

size_t ChromiumBrowser::tab_count() const {
  return owned_tabs_.size();
}

// ===== Observers =====

void ChromiumBrowser::AddObserver(BrowserObserver* observer) {
  observers_.push_back(observer);
}

void ChromiumBrowser::RemoveObserver(BrowserObserver* observer) {
  auto it = std::find(observers_.begin(), observers_.end(), observer);
  if (it != observers_.end()) {
    observers_.erase(it);
  }
}

// ===== Native view =====

void ChromiumBrowser::CreateActiveTabView(void* parent_nsview) {
  auto* tab = GetActiveTab();
  if (!tab) return;
  // Create the underlying native browser view for the active tab.
  // Note: this triggers async browser creation; the native view will be
  // available after OnTitleChanged / OnLoadingStateChanged fires.
  static_cast<ChromiumWebContents*>(tab)->CreateNativeView(parent_nsview);
}

// ===== BrowserObserver — forward events =====

void ChromiumBrowser::OnTitleChanged(WebContents* tab,
                                      const std::string& title) {
  for (auto* obs : observers_) {
    obs->OnTitleChanged(tab, title);
  }
}

void ChromiumBrowser::OnURLChanged(WebContents* tab,
                                   const std::string& url) {
  for (auto* obs : observers_) {
    obs->OnURLChanged(tab, url);
  }
}

void ChromiumBrowser::OnFaviconChanged(WebContents* tab,
                                        const std::string& favicon_url) {
  for (auto* obs : observers_) {
    obs->OnFaviconChanged(tab, favicon_url);
  }
}

void ChromiumBrowser::OnLoadingStateChanged(WebContents* tab,
                                            const NavigationState& state) {
  for (auto* obs : observers_) {
    obs->OnLoadingStateChanged(tab, state);
  }
}

void ChromiumBrowser::OnTabPinnedChanged(WebContents* tab, bool pinned) {
  for (auto* obs : observers_) {
    obs->OnTabPinnedChanged(tab, pinned);
  }
}

void ChromiumBrowser::OnTabMutedChanged(WebContents* tab, bool muted) {
  for (auto* obs : observers_) {
    obs->OnTabMutedChanged(tab, muted);
  }
}

// ===== Internal notifications =====

void ChromiumBrowser::NotifyTabAdded(WebContents* tab) {
  for (auto* obs : observers_) {
    obs->OnTabAdded(tab);
  }
}

void ChromiumBrowser::NotifyTabRemoved(const std::string& tab_id) {
  for (auto* obs : observers_) {
    obs->OnTabRemoved(tab_id);
  }
}

void ChromiumBrowser::NotifyActiveTabChanged(WebContents* tab) {
  for (auto* obs : observers_) {
    obs->OnActiveTabChanged(tab);
  }
}

std::string ChromiumBrowser::GenerateTabId() {
  next_tab_id_++;
  return "tab-" + std::to_string(next_tab_id_);
}

}  // namespace astra
