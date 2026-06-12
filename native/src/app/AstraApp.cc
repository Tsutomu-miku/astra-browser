#include "native/src/app/AstraApp.h"
#include "native/src/app/AstraClient.h"
#include "native/src/common/AstraIds.h"

#include "include/cef_scheme.h"
#include "include/base/cef_logging.h"
#include "include/wrapper/cef_helpers.h"

#include <algorithm>
#include <random>
#include <chrono>
#include <fstream>
#include <sys/stat.h>

namespace {

std::string GenerateUUID() {
  static std::mt19937 rng(static_cast<unsigned int>(
      std::chrono::steady_clock::now().time_since_epoch().count()));
  static const char* hex = "0123456789abcdef";
  std::string id(32, '0');
  for (int i = 0; i < 32; i++) {
    id[i] = hex[rng() % 16];
  }
  return id.substr(0, 8) + "-" + id.substr(8, 4) + "-" + id.substr(12, 4) +
         "-" + id.substr(16, 4) + "-" + id.substr(20, 12);
}

int64_t CurrentTimeMs() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
}

}  // namespace

CefRefPtr<AstraApp> g_astraApp;

AstraApp::AstraApp() = default;
AstraApp::~AstraApp() = default;

// ============================================================
// CefApp
// ============================================================

void AstraApp::OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar) {
  // Register astra:// scheme (for internal pages)
  // registrar->AddCustomScheme("astra", ...);
}

// ============================================================
// CefBrowserProcessHandler
// ============================================================

void AstraApp::OnContextInitialized() {
  CEF_REQUIRE_UI_THREAD();
  LOG(INFO) << "AstraApp: CEF context initialized";
  InitializeDefaultWorkspace();
}

// ============================================================
// CefRenderProcessHandler
// ============================================================

void AstraApp::OnContextCreated(CefRefPtr<CefBrowser> browser,
                                 CefRefPtr<CefFrame> frame,
                                 CefRefPtr<CefV8Context> context) {
  // Could inject JS APIs here for content pages
}

// ============================================================
// Workspace / Tab management
// ============================================================

void AstraApp::InitializeDefaultWorkspace() {
  auto workspace = std::make_shared<Workspace>();
  workspace->id = GenerateId();
  workspace->name = "Personal";
  workspace->accentColor = "#5B8FF9";
  workspaces_.push_back(workspace);
  activeWorkspaceId_ = workspace->id;

  // Create default "New Tab"
  auto tab = CreateNewTab(astra::kDefaultHomepage, "New Tab", /*activate=*/true);
  workspace->activeTabId = tab->id;
}

std::shared_ptr<Workspace> AstraApp::GetActiveWorkspace() {
  for (auto& ws : workspaces_) {
    if (ws->id == activeWorkspaceId_) return ws;
  }
  return nullptr;
}

std::shared_ptr<Workspace> AstraApp::GetWorkspace(const std::string& workspaceId) {
  for (auto& ws : workspaces_) {
    if (ws->id == workspaceId) return ws;
  }
  return nullptr;
}

std::shared_ptr<BrowserTab> AstraApp::CreateNewTab(
    const std::string& url,
    const std::string& title,
    bool activate) {
  auto ws = GetActiveWorkspace();
  if (!ws) return nullptr;

  auto tab = std::make_shared<BrowserTab>();
  tab->id = GenerateId();
  tab->title = title;
  tab->url = url;
  tab->isLoading = false;

  ws->tabs.push_back(tab);

  if (activate) {
    SelectTab(tab->id);
  }

  NotifyTabAdded(tab);
  return tab;
}

std::shared_ptr<BrowserTab> AstraApp::CreateNewIncognitoTab(
    const std::string& url,
    const std::string& title,
    bool activate) {
  auto ws = GetActiveWorkspace();
  if (!ws) return nullptr;

  auto tab = std::make_shared<BrowserTab>();
  tab->id = GenerateId();
  tab->title = title;
  tab->url = url;
  tab->isLoading = false;
  tab->isIncognito = true;

  ws->tabs.push_back(tab);

  if (activate) {
    SelectTab(tab->id);
  }

  NotifyTabAdded(tab);
  return tab;
}

void AstraApp::CloseTab(const std::string& tabId) {
  auto ws = GetActiveWorkspace();
  if (!ws) return;

  auto it = std::find_if(ws->tabs.begin(), ws->tabs.end(),
    [&](const std::shared_ptr<BrowserTab>& t) { return t->id == tabId; });
  if (it == ws->tabs.end()) return;

  // If closing the active tab, select another one
  if (ws->activeTabId == tabId) {
    if (ws->tabs.size() > 1) {
      size_t idx = it - ws->tabs.begin();
      if (idx > 0) {
        SelectTab(ws->tabs[idx - 1]->id);
      } else {
        SelectTab(ws->tabs[idx + 1]->id);
      }
    } else {
      ws->activeTabId.clear();
    }
  }

  // Close the CEF browser if it exists
  auto clientIt = clients_.find(tabId);
  if (clientIt != clients_.end()) {
    auto browser = clientIt->second->GetBrowser();
    if (browser) {
      browser->GetHost()->CloseBrowser(true);
    }
    // client will be removed in OnBrowserClosed callback
  }

  std::shared_ptr<BrowserTab> removedTab = *it;
  ws->tabs.erase(it);

  // Unpin and unfavorite (clean up ordered lists)
  std::erase(ws->pinnedTabIds, tabId);
  std::erase(ws->favoriteTabIds, tabId);

  // Remove from group if any
  if (!removedTab->groupId.empty()) {
    auto group = GetTabGroup(removedTab->groupId);
    if (group) {
      std::erase(group->tabIds, tabId);
      NotifyTabGroupUpdated(group);
    }
  }

  // Add to recently closed (most recent first)
  removedTab->closedAt = CurrentTimeMs();
  ws->recentlyClosed.insert(ws->recentlyClosed.begin(), removedTab);
  if (ws->recentlyClosed.size() > Workspace::kMaxRecentlyClosed) {
    ws->recentlyClosed.pop_back();
  }
  NotifyRecentlyClosedChanged();

  NotifyTabRemoved(tabId);
}

void AstraApp::SelectTab(const std::string& tabId) {
  auto ws = GetActiveWorkspace();
  if (!ws) return;

  if (ws->activeTabId == tabId) return;

  ws->activeTabId = tabId;

  auto tab = GetTab(tabId);
  if (tab) {
    NotifyActiveTabChanged(tab);
  }
}

void AstraApp::NavigateTab(const std::string& tabId, const std::string& url) {
  auto tab = GetTab(tabId);
  if (!tab) return;

  tab->url = url;

  auto clientIt = clients_.find(tabId);
  if (clientIt != clients_.end()) {
    auto browser = clientIt->second->GetBrowser();
    if (browser) {
      browser->GetMainFrame()->LoadURL(url);
    }
  }

  NotifyTabUpdated(tab);
}

void AstraApp::ReloadTab(const std::string& tabId) {
  auto clientIt = clients_.find(tabId);
  if (clientIt != clients_.end()) {
    auto browser = clientIt->second->GetBrowser();
    if (browser) {
      browser->Reload();
    }
  }
}

void AstraApp::GoBack(const std::string& tabId) {
  auto clientIt = clients_.find(tabId);
  if (clientIt != clients_.end()) {
    auto browser = clientIt->second->GetBrowser();
    if (browser && browser->CanGoBack()) {
      browser->GoBack();
    }
  }
}

void AstraApp::GoForward(const std::string& tabId) {
  auto clientIt = clients_.find(tabId);
  if (clientIt != clients_.end()) {
    auto browser = clientIt->second->GetBrowser();
    if (browser && browser->CanGoForward()) {
      browser->GoForward();
    }
  }
}

std::shared_ptr<BrowserTab> AstraApp::GetActiveTab() {
  auto ws = GetActiveWorkspace();
  if (!ws) return nullptr;
  return GetTab(ws->activeTabId);
}

std::shared_ptr<BrowserTab> AstraApp::GetTab(const std::string& tabId) {
  auto ws = GetActiveWorkspace();
  if (!ws) return nullptr;

  for (auto& tab : ws->tabs) {
    if (tab->id == tabId) return tab;
  }
  return nullptr;
}

CefRefPtr<CefBrowser> AstraApp::GetCefBrowserForTab(const std::string& tabId) {
  auto it = clients_.find(tabId);
  if (it != clients_.end()) {
    return it->second->GetBrowser();
  }
  return nullptr;
}

void AstraApp::EnsureBrowserForTab(const std::string& tabId,
                                    cef_window_handle_t parentHandle) {
  auto tab = GetTab(tabId);
  if (!tab) return;

  // Already have a browser for this tab
  if (clients_.find(tabId) != clients_.end()) return;

  // Create client
  CefRefPtr<AstraClient> client = new AstraClient(tabId);
  clients_[tabId] = client;

  // Browser settings
  CefBrowserSettings browser_settings;

  // Window info — embed in parent view
  CefWindowInfo window_info;
  CefRect initialFrame(0, 0, 800, 600);
  window_info.SetAsChild(parentHandle, initialFrame);

  // Get request context (incognito tabs use in-memory context)
  CefRefPtr<CefRequestContext> request_context;
  if (tab->isIncognito) {
    if (!incognito_request_context_) {
      CefRequestContextSettings settings;
      // Empty cache path = in-memory only
      incognito_request_context_ = CefRequestContext::CreateContext(
          settings, nullptr);
    }
    request_context = incognito_request_context_;
  }

  // Create browser (async — OnAfterCreated will be called)
  CefBrowserHost::CreateBrowser(window_info, client.get(),
                                  tab->url, browser_settings,
                                  nullptr, request_context);
}

// ============================================================
// Observers
// ============================================================

void AstraApp::AddObserver(BrowserStateObserver* observer) {
  observers_.push_back(observer);
}

void AstraApp::RemoveObserver(BrowserStateObserver* observer) {
  auto it = std::find(observers_.begin(), observers_.end(), observer);
  if (it != observers_.end()) {
    observers_.erase(it);
  }
}

void AstraApp::NotifyTabAdded(const std::shared_ptr<BrowserTab>& tab) {
  for (auto* obs : observers_) {
    obs->OnTabAdded(tab);
  }
}

void AstraApp::NotifyTabRemoved(const std::string& tabId) {
  for (auto* obs : observers_) {
    obs->OnTabRemoved(tabId);
  }
}

void AstraApp::NotifyActiveTabChanged(const std::shared_ptr<BrowserTab>& tab) {
  for (auto* obs : observers_) {
    obs->OnActiveTabChanged(tab);
  }
}

void AstraApp::NotifyTabUpdated(const std::shared_ptr<BrowserTab>& tab) {
  for (auto* obs : observers_) {
    obs->OnTabUpdated(tab);
  }
}

// ============================================================
// Called by AstraClient (browser event callbacks)
// ============================================================

void AstraApp::OnBrowserTitleChanged(const std::string& tabId,
                                      const std::string& title) {
  auto tab = GetTab(tabId);
  if (!tab) return;
  tab->title = title;
  NotifyTabUpdated(tab);
}

void AstraApp::OnBrowserUrlChanged(const std::string& tabId,
                                    const std::string& url) {
  auto tab = GetTab(tabId);
  if (!tab) return;
  tab->url = url;
  NotifyTabUpdated(tab);
}

void AstraApp::OnBrowserLoadingStateChanged(const std::string& tabId,
                                             bool isLoading,
                                             bool canGoBack,
                                             bool canGoForward) {
  auto tab = GetTab(tabId);
  if (!tab) return;

  bool wasLoading = tab->isLoading;
  tab->isLoading = isLoading;
  tab->canGoBack = canGoBack;
  tab->canGoForward = canGoForward;
  NotifyTabUpdated(tab);

  // Record history when page finishes loading (skip incognito tabs)
  if (wasLoading && !isLoading && !tab->isIncognito) {
    std::string title = tab->title.empty() ? tab->url : tab->title;
    AddHistoryEntry(tab->url, title);
  }
}

void AstraApp::OnClientBrowserCreated(const std::string& tabId,
                                       CefRefPtr<CefBrowser> browser) {
  auto tab = GetTab(tabId);
  if (!tab) return;

  // If this is the active tab, trigger active tab change so UI shows the browser.
  // Browser creation is async, so UI needs a nudge when it's ready.
  auto ws = GetActiveWorkspace();
  if (ws && ws->activeTabId == tabId) {
    NotifyActiveTabChanged(tab);
  }
}

void AstraApp::OnBrowserClosed(const std::string& tabId) {
  DLOG(INFO) << "Browser closed for tab " << tabId;
  clients_.erase(tabId);
}

void AstraApp::OnFindResult(const std::string& tabId,
                             int matchCount,
                             int activeMatchOrdinal,
                             bool finalUpdate) {
  NotifyFindResult(tabId, matchCount, activeMatchOrdinal, finalUpdate);
}

std::string AstraApp::GenerateId() {
  return GenerateUUID();
}

// ============================================================
// Tab pinning
// ============================================================

void AstraApp::PinTab(const std::string& tabId) {
  auto ws = GetActiveWorkspace();
  auto tab = GetTab(tabId);
  if (!ws || !tab || tab->isPinned) return;

  tab->isPinned = true;
  // Add to the end of pinned list
  ws->pinnedTabIds.push_back(tabId);

  NotifyTabPinnedChanged(tab);
  NotifyTabUpdated(tab);
}

void AstraApp::UnpinTab(const std::string& tabId) {
  auto ws = GetActiveWorkspace();
  auto tab = GetTab(tabId);
  if (!ws || !tab || !tab->isPinned) return;

  tab->isPinned = false;
  std::erase(ws->pinnedTabIds, tabId);

  NotifyTabPinnedChanged(tab);
  NotifyTabUpdated(tab);
}

void AstraApp::ToggleTabPinned(const std::string& tabId) {
  auto tab = GetTab(tabId);
  if (!tab) return;
  if (tab->isPinned) {
    UnpinTab(tabId);
  } else {
    PinTab(tabId);
  }
}

// ============================================================
// Tab favorites
// ============================================================

void AstraApp::AddToFavorites(const std::string& tabId) {
  auto ws = GetActiveWorkspace();
  auto tab = GetTab(tabId);
  if (!ws || !tab || tab->isFavorite) return;

  tab->isFavorite = true;
  ws->favoriteTabIds.push_back(tabId);

  NotifyTabFavoriteChanged(tab);
  NotifyTabUpdated(tab);
}

void AstraApp::RemoveFromFavorites(const std::string& tabId) {
  auto ws = GetActiveWorkspace();
  auto tab = GetTab(tabId);
  if (!ws || !tab || !tab->isFavorite) return;

  tab->isFavorite = false;
  std::erase(ws->favoriteTabIds, tabId);

  NotifyTabFavoriteChanged(tab);
  NotifyTabUpdated(tab);
}

void AstraApp::ToggleTabFavorite(const std::string& tabId) {
  auto tab = GetTab(tabId);
  if (!tab) return;
  if (tab->isFavorite) {
    RemoveFromFavorites(tabId);
  } else {
    AddToFavorites(tabId);
  }
}

// ============================================================
// Tab reordering
// ============================================================

void AstraApp::MoveTab(size_t fromIndex, size_t toIndex) {
  auto ws = GetActiveWorkspace();
  if (!ws) return;
  if (fromIndex >= ws->tabs.size() || toIndex >= ws->tabs.size()) return;
  if (fromIndex == toIndex) return;

  auto tab = ws->tabs[fromIndex];
  ws->tabs.erase(ws->tabs.begin() + fromIndex);
  ws->tabs.insert(ws->tabs.begin() + toIndex, tab);

  NotifyTabMoved(tab->id, fromIndex, toIndex);
  NotifyTabUpdated(tab);
}

// ============================================================
// Tab mute
// ============================================================

void AstraApp::SetTabMuted(const std::string& tabId, bool muted) {
  auto tab = GetTab(tabId);
  if (!tab) return;

  tab->isMuted = muted;
  auto browser = GetCefBrowserForTab(tabId);
  if (browser) {
    browser->GetHost()->SetAudioMuted(muted);
  }

  NotifyTabUpdated(tab);
}

void AstraApp::ToggleTabMuted(const std::string& tabId) {
  auto tab = GetTab(tabId);
  if (!tab) return;
  SetTabMuted(tabId, !tab->isMuted);
}

// ============================================================
// Duplicate tab
// ============================================================

std::shared_ptr<BrowserTab> AstraApp::DuplicateTab(const std::string& tabId,
                                                    bool activate) {
  auto tab = GetTab(tabId);
  if (!tab) return nullptr;

  auto newTab = CreateNewTab(tab->url, tab->title, activate);
  if (newTab && tab->isPinned) {
    PinTab(newTab->id);
  }
  if (newTab && tab->isFavorite) {
    AddToFavorites(newTab->id);
  }
  return newTab;
}

// ============================================================
// Force reload
// ============================================================

void AstraApp::ReloadTabBypassingCache(const std::string& tabId) {
  auto clientIt = clients_.find(tabId);
  if (clientIt != clients_.end()) {
    auto browser = clientIt->second->GetBrowser();
    if (browser) {
      browser->ReloadIgnoreCache();
    }
  }
}

// ============================================================
// Tab groups
// ============================================================

std::shared_ptr<TabGroup> AstraApp::CreateTabGroup(const std::string& name,
                                                    const std::string& color) {
  auto ws = GetActiveWorkspace();
  if (!ws) return nullptr;

  auto group = std::make_shared<TabGroup>();
  group->id = GenerateId();
  group->name = name;
  group->color = color;
  ws->groups.push_back(group);

  NotifyTabGroupCreated(group);
  return group;
}

void AstraApp::RemoveTabGroup(const std::string& groupId) {
  auto ws = GetActiveWorkspace();
  if (!ws) return;

  auto it = std::find_if(ws->groups.begin(), ws->groups.end(),
    [&](const std::shared_ptr<TabGroup>& g) { return g->id == groupId; });
  if (it == ws->groups.end()) return;

  // Ungroup all tabs in this group
  for (auto& tabId : (*it)->tabIds) {
    auto tab = GetTab(tabId);
    if (tab) {
      tab->groupId.clear();
      NotifyTabUpdated(tab);
    }
  }

  ws->groups.erase(it);
  NotifyTabGroupRemoved(groupId);
}

void AstraApp::MoveTabToGroup(const std::string& tabId, const std::string& groupId) {
  auto ws = GetActiveWorkspace();
  auto tab = GetTab(tabId);
  auto group = GetTabGroup(groupId);
  if (!ws || !tab || !group) return;

  // Remove from previous group if any
  if (!tab->groupId.empty()) {
    auto prevGroup = GetTabGroup(tab->groupId);
    if (prevGroup) {
      std::erase(prevGroup->tabIds, tabId);
      NotifyTabGroupUpdated(prevGroup);
    }
  }

  tab->groupId = groupId;
  group->tabIds.push_back(tabId);

  NotifyTabUpdated(tab);
  NotifyTabGroupUpdated(group);
}

void AstraApp::RemoveTabFromGroup(const std::string& tabId) {
  auto tab = GetTab(tabId);
  if (!tab || tab->groupId.empty()) return;

  auto group = GetTabGroup(tab->groupId);
  if (group) {
    std::erase(group->tabIds, tabId);
    NotifyTabGroupUpdated(group);
  }

  std::string oldGroupId = tab->groupId;
  tab->groupId.clear();
  NotifyTabUpdated(tab);
}

void AstraApp::ToggleTabGroupCollapsed(const std::string& groupId) {
  auto group = GetTabGroup(groupId);
  if (!group) return;

  group->collapsed = !group->collapsed;
  NotifyTabGroupUpdated(group);
}

void AstraApp::RenameTabGroup(const std::string& groupId, const std::string& name) {
  auto group = GetTabGroup(groupId);
  if (!group) return;

  group->name = name;
  NotifyTabGroupUpdated(group);
}

std::shared_ptr<TabGroup> AstraApp::GetTabGroup(const std::string& groupId) {
  auto ws = GetActiveWorkspace();
  if (!ws) return nullptr;
  for (auto& g : ws->groups) {
    if (g->id == groupId) return g;
  }
  return nullptr;
}

const std::vector<std::shared_ptr<TabGroup>>& AstraApp::GetTabGroups() {
  static std::vector<std::shared_ptr<TabGroup>> empty;
  auto ws = GetActiveWorkspace();
  if (!ws) return empty;
  return ws->groups;
}

// ============================================================
// Recently closed tabs
// ============================================================

const std::vector<std::shared_ptr<BrowserTab>>& AstraApp::GetRecentlyClosed() {
  static std::vector<std::shared_ptr<BrowserTab>> empty;
  auto ws = GetActiveWorkspace();
  if (!ws) return empty;
  return ws->recentlyClosed;
}

std::shared_ptr<BrowserTab> AstraApp::RestoreRecentlyClosedTab(
    const std::string& tabId, bool activate) {
  auto ws = GetActiveWorkspace();
  if (!ws) return nullptr;

  auto it = std::find_if(ws->recentlyClosed.begin(), ws->recentlyClosed.end(),
    [&](const std::shared_ptr<BrowserTab>& t) { return t->id == tabId; });
  if (it == ws->recentlyClosed.end()) return nullptr;

  auto closedTab = *it;
  ws->recentlyClosed.erase(it);

  // Create a new tab with the same URL and title
  auto newTab = CreateNewTab(closedTab->url, closedTab->title, activate);

  // Restore pinned/favorite state
  if (newTab) {
    if (closedTab->isPinned) PinTab(newTab->id);
    if (closedTab->isFavorite) AddToFavorites(newTab->id);
  }

  NotifyRecentlyClosedChanged();
  return newTab;
}

std::shared_ptr<BrowserTab> AstraApp::RestoreMostRecentlyClosed(bool activate) {
  auto ws = GetActiveWorkspace();
  if (!ws || ws->recentlyClosed.empty()) return nullptr;
  return RestoreRecentlyClosedTab(ws->recentlyClosed.front()->id, activate);
}

void AstraApp::ClearRecentlyClosed() {
  auto ws = GetActiveWorkspace();
  if (!ws) return;
  ws->recentlyClosed.clear();
  NotifyRecentlyClosedChanged();
}

// ============================================================
// History
// ============================================================

const std::vector<std::shared_ptr<HistoryEntry>>& AstraApp::GetHistory() {
  static std::vector<std::shared_ptr<HistoryEntry>> empty;
  auto ws = GetActiveWorkspace();
  if (!ws) return empty;
  return ws->history;
}

void AstraApp::AddHistoryEntry(const std::string& url, const std::string& title) {
  auto ws = GetActiveWorkspace();
  if (!ws) return;

  // Skip empty URLs or internal pages? For now, record everything.
  if (url.empty()) return;

  // Skip if same as most recent entry (consecutive duplicate)
  if (!ws->history.empty() && ws->history.front()->url == url) {
    // Update title if it changed
    if (ws->history.front()->title != title) {
      ws->history.front()->title = title;
      NotifyHistoryChanged();
    }
    return;
  }

  auto entry = std::make_shared<HistoryEntry>();
  entry->id = GenerateId();
  entry->url = url;
  entry->title = title.empty() ? url : title;
  entry->timestamp = CurrentTimeMs();

  ws->history.insert(ws->history.begin(), entry);

  // Trim to max size
  while (ws->history.size() > Workspace::kMaxHistoryEntries) {
    ws->history.pop_back();
  }

  NotifyHistoryChanged();
}

void AstraApp::ClearHistory() {
  auto ws = GetActiveWorkspace();
  if (!ws) return;
  ws->history.clear();
  NotifyHistoryChanged();
}

// ============================================================
// Downloads
// ============================================================

const std::vector<std::shared_ptr<DownloadItem>>& AstraApp::GetDownloads() {
  static std::vector<std::shared_ptr<DownloadItem>> empty;
  auto ws = GetActiveWorkspace();
  if (!ws) return empty;
  return ws->downloads;
}

std::shared_ptr<DownloadItem> AstraApp::GetDownload(const std::string& downloadId) {
  auto ws = GetActiveWorkspace();
  if (!ws) return nullptr;
  for (auto& d : ws->downloads) {
    if (d->id == downloadId) return d;
  }
  return nullptr;
}

void AstraApp::AddDownload(const std::shared_ptr<DownloadItem>& download) {
  auto ws = GetActiveWorkspace();
  if (!ws) return;

  ws->downloads.insert(ws->downloads.begin(), download);

  // Trim to max size
  while (ws->downloads.size() > Workspace::kMaxDownloads) {
    ws->downloads.pop_back();
  }

  NotifyDownloadCreated(download);
}

void AstraApp::UpdateDownload(const std::shared_ptr<DownloadItem>& download) {
  NotifyDownloadUpdated(download);
}

void AstraApp::CancelDownload(const std::string& downloadId) {
  // Use CEF callback to cancel if available
  auto it = download_callbacks_.find(downloadId);
  if (it != download_callbacks_.end() && it->second) {
    it->second->Cancel();
  }

  // Also update our model state
  auto download = GetDownload(downloadId);
  if (download && download->state == DownloadState::InProgress) {
    download->state = DownloadState::Cancelled;
    download->endTime = CurrentTimeMs();
    NotifyDownloadUpdated(download);
  }
}

void AstraApp::SetDownloadCallback(const std::string& downloadId,
                                    CefRefPtr<CefDownloadItemCallback> callback) {
  download_callbacks_[downloadId] = callback;
}

void AstraApp::OpenDownload(const std::string& downloadId) {
  auto download = GetDownload(downloadId);
  if (!download || download->state != DownloadState::Complete) return;
  if (download->filePath.empty()) return;

  std::string cmd = "open \"" + download->filePath + "\"";
  std::system(cmd.c_str());
}

void AstraApp::ShowDownloadInFinder(const std::string& downloadId) {
  auto download = GetDownload(downloadId);
  if (!download || download->filePath.empty()) return;

  std::string cmd = "open -R \"" + download->filePath + "\"";
  std::system(cmd.c_str());
}

void AstraApp::ClearCompletedDownloads() {
  auto ws = GetActiveWorkspace();
  if (!ws) return;

  std::vector<std::shared_ptr<DownloadItem>> remaining;
  for (auto& d : ws->downloads) {
    if (d->state == DownloadState::InProgress) {
      remaining.push_back(d);
    }
  }
  ws->downloads = std::move(remaining);

  if (!ws->downloads.empty()) {
    NotifyDownloadUpdated(ws->downloads.front());
  }
}

// ============================================================
// Internal pages
// ============================================================

void AstraApp::OpenSettingsPage() {
  auto tab = GetActiveTab();
  if (!tab) return;

  // Write settings HTML to a file and load it
  // In a future version we'll use a custom astra:// scheme handler
  std::string html = GetSettingsPageHtml();

  const char* home = std::getenv("HOME");
  if (!home) return;

  std::string appSupportDir = std::string(home) + "/Library/Application Support/Astra";
  mkdir(appSupportDir.c_str(), 0755);

  std::string settingsPath = appSupportDir + "/settings.html";
  std::ofstream file(settingsPath);
  if (file.is_open()) {
    file << html;
    file.close();
  }

  std::string fileUrl = "file://" + settingsPath;
  NavigateTab(tab->id, fileUrl);

  // Update tab title
  tab->title = "Settings";
  NotifyTabUpdated(tab);
}

std::string AstraApp::GetSettingsPageHtml() {
  return R"(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Astra Settings</title>
<style>
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body {
    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
    font-size: 14px;
    color: var(--text-color, #1f2328);
    background: var(--bg-color, #f6f8fa);
    display: flex;
    height: 100vh;
    overflow: hidden;
  }
  @media (prefers-color-scheme: dark) {
    body { --text-color: #e6edf3; --bg-color: #0d1117; }
    .sidebar { background: #161b22 !important; border-right-color: #30363d !important; }
    .sidebar-item { color: #e6edf3 !important; }
    .sidebar-item:hover { background: #21262d !important; }
    .sidebar-item.active { background: #1f6feb33 !important; color: #58a6ff !important; }
    .content { background: #0d1117 !important; }
    .card { background: #161b22 !important; border-color: #30363d !important; }
    .card h3 { color: #e6edf3 !important; }
    .setting-row { border-color: #21262d !important; color: #e6edf3 !important; }
    .setting-desc { color: #8b949e !important; }
    input[type="text"], select {
      background: #0d1117 !important;
      color: #e6edf3 !important;
      border-color: #30363d !important;
    }
    .section-page h2 { color: #e6edf3 !important; }
    .section-page p { color: #8b949e !important; }
  }
  .sidebar {
    width: 220px;
    background: #ffffff;
    border-right: 1px solid #d0d7de;
    padding: 16px 0;
    overflow-y: auto;
  }
  .sidebar-header {
    padding: 0 16px 12px;
    font-size: 18px;
    font-weight: 600;
    color: var(--text-color, #1f2328);
    border-bottom: 1px solid #d0d7de;
    margin-bottom: 8px;
    padding-bottom: 16px;
  }
  .sidebar-item {
    padding: 8px 16px;
    cursor: pointer;
    color: #656d76;
    font-size: 13px;
    border-left: 3px solid transparent;
  }
  .sidebar-item:hover {
    background: #f3f4f6;
    color: #1f2328;
  }
  .sidebar-item.active {
    background: #ddf4ff;
    color: #0969da;
    border-left-color: #0969da;
    font-weight: 500;
  }
  .content {
    flex: 1;
    padding: 24px;
    overflow-y: auto;
    background: #f6f8fa;
  }
  .section-page { display: none; }
  .section-page.active { display: block; }
  .section-page h2 {
    font-size: 22px;
    font-weight: 600;
    margin-bottom: 8px;
    color: #1f2328;
  }
  .section-page p {
    color: #656d76;
    margin-bottom: 24px;
    font-size: 13px;
  }
  .card {
    background: #ffffff;
    border: 1px solid #d0d7de;
    border-radius: 8px;
    padding: 16px;
    margin-bottom: 16px;
  }
  .card h3 {
    font-size: 15px;
    font-weight: 600;
    margin-bottom: 12px;
    color: #1f2328;
  }
  .setting-row {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 10px 0;
    border-bottom: 1px solid #eaeef2;
  }
  .setting-row:last-child { border-bottom: none; }
  .setting-label {
    font-size: 13px;
    color: #1f2328;
  }
  .setting-desc {
    font-size: 12px;
    color: #656d76;
    margin-top: 2px;
  }
  .setting-control { flex-shrink: 0; }
  input[type="checkbox"] { width: 16px; height: 16px; cursor: pointer; }
  input[type="text"] {
    padding: 6px 10px;
    border: 1px solid #d0d7de;
    border-radius: 6px;
    font-size: 13px;
    width: 200px;
  }
  select {
    padding: 6px 10px;
    border: 1px solid #d0d7de;
    border-radius: 6px;
    font-size: 13px;
    background: #ffffff;
    cursor: pointer;
    min-width: 150px;
  }
</style>
</head>
<body>
<div class="sidebar">
  <div class="sidebar-header">Settings</div>
  <div class="sidebar-item active" data-section="general">General</div>
  <div class="sidebar-item" data-section="appearance">Appearance</div>
  <div class="sidebar-item" data-section="privacy">Privacy &amp; Security</div>
  <div class="sidebar-item" data-section="search">Search Engine</div>
  <div class="sidebar-item" data-section="downloads">Downloads</div>
  <div class="sidebar-item" data-section="tabs">Tabs</div>
  <div class="sidebar-item" data-section="bookmarks">Bookmarks</div>
  <div class="sidebar-item" data-section="history">History</div>
  <div class="sidebar-item" data-section="passwords">Passwords</div>
  <div class="sidebar-item" data-section="autofill">Autofill</div>
  <div class="sidebar-item" data-section="languages">Languages</div>
  <div class="sidebar-item" data-section="accessibility">Accessibility</div>
  <div class="sidebar-item" data-section="system">System</div>
  <div class="sidebar-item" data-section="extensions">Extensions</div>
  <div class="sidebar-item" data-section="about">About Astra</div>
  <div class="sidebar-item" data-section="advanced">Advanced</div>
</div>

<div class="content">
  <div class="section-page active" id="general">
    <h2>General</h2>
    <p>Basic browser settings</p>
    <div class="card">
      <h3>Startup</h3>
      <div class="setting-row">
        <div>
          <div class="setting-label">On startup</div>
          <div class="setting-desc">What to do when Astra starts</div>
        </div>
        <div class="setting-control">
          <select>
            <option>Open the New Tab page</option>
            <option>Continue where you left off</option>
            <option>Open a specific page</option>
          </select>
        </div>
      </div>
      <div class="setting-row">
        <div>
          <div class="setting-label">Default browser</div>
          <div class="setting-desc">Make Astra your default browser</div>
        </div>
        <div class="setting-control">
          <button style="padding: 6px 14px; border: 1px solid #d0d7de; border-radius: 6px; background: #fff; cursor: pointer; font-size: 13px;">Make Default</button>
        </div>
      </div>
    </div>
    <div class="card">
      <h3>Performance</h3>
      <div class="setting-row">
        <div>
          <div class="setting-label">Memory saver</div>
          <div class="setting-desc">Reduces memory usage by deactivating inactive tabs</div>
        </div>
        <div class="setting-control"><input type="checkbox" checked></div>
      </div>
      <div class="setting-row">
        <div>
          <div class="setting-label">Hardware acceleration</div>
          <div class="setting-desc">Use GPU for faster rendering</div>
        </div>
        <div class="setting-control"><input type="checkbox" checked></div>
      </div>
    </div>
  </div>

  <div class="section-page" id="appearance">
    <h2>Appearance</h2>
    <p>Customize how Astra looks</p>
    <div class="card">
      <h3>Theme</h3>
      <div class="setting-row">
        <div>
          <div class="setting-label">Theme</div>
          <div class="setting-desc">Choose a color scheme</div>
        </div>
        <div class="setting-control">
          <select>
            <option>System default</option>
            <option>Light</option>
            <option>Dark</option>
          </select>
        </div>
      </div>
    </div>
    <div class="card">
      <h3>Fonts</h3>
      <div class="setting-row">
        <div>
          <div class="setting-label">Font size</div>
          <div class="setting-desc">Adjust text size</div>
        </div>
        <div class="setting-control">
          <select>
            <option>Very small</option>
            <option>Small</option>
            <option selected>Medium (Recommended)</option>
            <option>Large</option>
            <option>Very large</option>
          </select>
        </div>
      </div>
    </div>
  </div>

  <div class="section-page" id="privacy">
    <h2>Privacy &amp; Security</h2>
    <p>Control your privacy and security settings</p>
    <div class="card">
      <h3>Security</h3>
      <div class="setting-row">
        <div>
          <div class="setting-label">Safe browsing</div>
          <div class="setting-desc">Warn about dangerous sites and downloads</div>
        </div>
        <div class="setting-control"><input type="checkbox" checked></div>
      </div>
    </div>
    <div class="card">
      <h3>Privacy</h3>
      <div class="setting-row">
        <div>
          <div class="setting-label">Do Not Track</div>
          <div class="setting-desc">Send Do Not Track request with your browsing traffic</div>
        </div>
        <div class="setting-control"><input type="checkbox"></div>
      </div>
      <div class="setting-row">
        <div>
          <div class="setting-label">Block third-party cookies</div>
          <div class="setting-desc">Prevent sites from tracking you across the web</div>
        </div>
        <div class="setting-control"><input type="checkbox"></div>
      </div>
      <div class="setting-row">
        <div>
          <div class="setting-label">Clear browsing data</div>
          <div class="setting-desc">Remove history, cookies, and cached data</div>
        </div>
        <div class="setting-control">
          <button style="padding: 6px 14px; border: 1px solid #d0d7de; border-radius: 6px; background: #fff; cursor: pointer; font-size: 13px;">Clear Data</button>
        </div>
      </div>
    </div>
  </div>

  <div class="section-page" id="search">
    <h2>Search Engine</h2>
    <p>Choose your default search engine</p>
    <div class="card">
      <h3>Default search engine</h3>
      <div class="setting-row">
        <div>
          <div class="setting-label">Search engine used in the address bar</div>
        </div>
        <div class="setting-control">
          <select>
            <option selected>Google</option>
            <option>Bing</option>
            <option>DuckDuckGo</option>
            <option>Brave Search</option>
            <option>Wikipedia</option>
          </select>
        </div>
      </div>
    </div>
  </div>

  <div class="section-page" id="downloads">
    <h2>Downloads</h2>
    <p>Manage download settings</p>
    <div class="card">
      <h3>Download location</h3>
      <div class="setting-row">
        <div>
          <div class="setting-label">Download folder</div>
          <div class="setting-desc">Where downloaded files are saved</div>
        </div>
        <div class="setting-control">
          <button style="padding: 6px 14px; border: 1px solid #d0d7de; border-radius: 6px; background: #fff; cursor: pointer; font-size: 13px;">Change...</button>
        </div>
      </div>
      <div class="setting-row">
        <div>
          <div class="setting-label">Ask where to save each file</div>
          <div class="setting-desc">Show save dialog for every download</div>
        </div>
        <div class="setting-control"><input type="checkbox"></div>
      </div>
      <div class="setting-row">
        <div>
          <div class="setting-label">Open files after downloading</div>
          <div class="setting-desc">Automatically open downloaded files</div>
        </div>
        <div class="setting-control"><input type="checkbox"></div>
      </div>
    </div>
  </div>

  <div class="section-page" id="tabs">
    <h2>Tabs</h2>
    <p>Configure tab behavior</p>
    <div class="card">
      <h3>Tab behavior</h3>
      <div class="setting-row">
        <div>
          <div class="setting-label">Open new tab page in</div>
          <div class="setting-desc">Where new tabs appear</div>
        </div>
        <div class="setting-control">
          <select>
            <option selected>End of tab strip</option>
            <option>Next to current tab</option>
          </select>
        </div>
      </div>
      <div class="setting-row">
        <div>
          <div class="setting-label">Warn before closing multiple tabs</div>
          <div class="setting-desc">Show confirmation when closing window with multiple tabs</div>
        </div>
        <div class="setting-control"><input type="checkbox" checked></div>
      </div>
      <div class="setting-row">
        <div>
          <div class="setting-label">Pin tab on right-click</div>
          <div class="setting-desc">Show pin option in tab context menu</div>
        </div>
        <div class="setting-control"><input type="checkbox" checked></div>
      </div>
    </div>
  </div>

  <div class="section-page" id="bookmarks">
    <h2>Bookmarks</h2>
    <p>Manage your bookmarks</p>
    <div class="card">
      <h3>Bookmarks bar</h3>
      <div class="setting-row">
        <div>
          <div class="setting-label">Show bookmarks bar</div>
          <div class="setting-desc">Display bookmarks under the address bar</div>
        </div>
        <div class="setting-control"><input type="checkbox"></div>
      </div>
    </div>
  </div>

  <div class="section-page" id="history">
    <h2>History</h2>
    <p>Browsing history settings</p>
    <div class="card">
      <h3>History</h3>
      <div class="setting-row">
        <div>
          <div class="setting-label">Save browsing history</div>
          <div class="setting-desc">Record websites you visit</div>
        </div>
        <div class="setting-control"><input type="checkbox" checked></div>
      </div>
      <div class="setting-row">
        <div>
          <div class="setting-label">Auto-complete from history</div>
          <div class="setting-desc">Suggest URLs from history in the address bar</div>
        </div>
        <div class="setting-control"><input type="checkbox" checked></div>
      </div>
    </div>
  </div>

  <div class="section-page" id="passwords">
    <h2>Passwords</h2>
    <p>Manage saved passwords</p>
    <div class="card">
      <h3>Password settings</h3>
      <div class="setting-row">
        <div>
          <div class="setting-label">Offer to save passwords</div>
          <div class="setting-desc">Prompt to save passwords when you log in</div>
        </div>
        <div class="setting-control"><input type="checkbox" checked></div>
      </div>
      <div class="setting-row">
        <div>
          <div class="setting-label">Auto-sign in</div>
          <div class="setting-desc">Automatically sign in to sites with saved passwords</div>
        </div>
        <div class="setting-control"><input type="checkbox"></div>
      </div>
    </div>
  </div>

  <div class="section-page" id="autofill">
    <h2>Autofill</h2>
    <p>Configure autofill for forms</p>
    <div class="card">
      <h3>Autofill settings</h3>
      <div class="setting-row">
        <div>
          <div class="setting-label">Save and fill addresses</div>
          <div class="setting-desc">Auto-fill address forms</div>
        </div>
        <div class="setting-control"><input type="checkbox" checked></div>
      </div>
      <div class="setting-row">
        <div>
          <div class="setting-label">Save and fill payment methods</div>
          <div class="setting-desc">Auto-fill credit card information</div>
        </div>
        <div class="setting-control"><input type="checkbox" checked></div>
      </div>
    </div>
  </div>

  <div class="section-page" id="languages">
    <h2>Languages</h2>
    <p>Language and spell check settings</p>
    <div class="card">
      <h3>Languages</h3>
      <div class="setting-row">
        <div>
          <div class="setting-label">Display language</div>
          <div class="setting-desc">Language used for the Astra interface</div>
        </div>
        <div class="setting-control">
          <select>
            <option selected>English (United States)</option>
            <option>中文 (简体)</option>
            <option>日本語</option>
            <option>Français</option>
            <option>Deutsch</option>
          </select>
        </div>
      </div>
      <div class="setting-row">
        <div>
          <div class="setting-label">Spell check</div>
          <div class="setting-desc">Check spelling as you type</div>
        </div>
        <div class="setting-control"><input type="checkbox" checked></div>
      </div>
    </div>
  </div>

  <div class="section-page" id="accessibility">
    <h2>Accessibility</h2>
    <p>Make Astra easier to use</p>
    <div class="card">
      <h3>Accessibility features</h3>
      <div class="setting-row">
        <div>
          <div class="setting-label">Page zoom</div>
          <div class="setting-desc">Default zoom level for all pages</div>
        </div>
        <div class="setting-control">
          <select>
            <option>50%</option>
            <option>75%</option>
            <option>90%</option>
            <option selected>100%</option>
            <option>110%</option>
            <option>125%</option>
            <option>150%</option>
          </select>
        </div>
      </div>
      <div class="setting-row">
        <div>
          <div class="setting-label">Force page zoom</div>
          <div class="setting-desc">Make all text larger regardless of page settings</div>
        </div>
        <div class="setting-control"><input type="checkbox"></div>
      </div>
      <div class="setting-row">
        <div>
          <div class="setting-label">Caret browsing</div>
          <div class="setting-desc">Navigate pages with a text cursor</div>
        </div>
        <div class="setting-control"><input type="checkbox"></div>
      </div>
    </div>
  </div>

  <div class="section-page" id="system">
    <h2>System</h2>
    <p>System-level settings</p>
    <div class="card">
      <h3>System integration</h3>
      <div class="setting-row">
        <div>
          <div class="setting-label">Continue running background apps</div>
          <div class="setting-desc">Keep Astra running when window is closed</div>
        </div>
        <div class="setting-control"><input type="checkbox"></div>
      </div>
      <div class="setting-row">
        <div>
          <div class="setting-label">Use hardware acceleration</div>
          <div class="setting-desc">Use GPU for rendering</div>
        </div>
        <div class="setting-control"><input type="checkbox" checked></div>
      </div>
    </div>
  </div>

  <div class="section-page" id="extensions">
    <h2>Extensions</h2>
    <p>Manage browser extensions</p>
    <div class="card">
      <h3>Extensions</h3>
      <div class="setting-row">
        <div>
          <div class="setting-label">Developer mode</div>
          <div class="setting-desc">Enable developer tools for extensions</div>
        </div>
        <div class="setting-control"><input type="checkbox"></div>
      </div>
      <div class="setting-row">
        <div>
          <div class="setting-label">Allow extensions from other stores</div>
          <div class="setting-desc">Install extensions from outside the official store</div>
        </div>
        <div class="setting-control"><input type="checkbox"></div>
      </div>
    </div>
  </div>

  <div class="section-page" id="about">
    <h2>About Astra</h2>
    <p>Information about Astra Browser</p>
    <div class="card">
      <h3>Version</h3>
      <div class="setting-row">
        <div>
          <div class="setting-label">Astra Browser</div>
          <div class="setting-desc">Version 0.1.0 (Milestone 1)</div>
        </div>
        <div class="setting-control" style="font-size: 12px; color: #1a7f37;">Up to date</div>
      </div>
      <div class="setting-row">
        <div>
          <div class="setting-label">Chromium engine</div>
          <div class="setting-desc">Based on CEF (Chromium Embedded Framework)</div>
        </div>
        <div class="setting-control"></div>
      </div>
    </div>
    <div class="card">
      <h3>Legal</h3>
      <div class="setting-row">
        <div class="setting-label">Terms of Service</div>
        <div class="setting-control" style="color: #0969da; cursor: pointer;">View</div>
      </div>
      <div class="setting-row">
        <div class="setting-label">Privacy Policy</div>
        <div class="setting-control" style="color: #0969da; cursor: pointer;">View</div>
      </div>
      <div class="setting-row">
        <div class="setting-label">Open Source Licenses</div>
        <div class="setting-control" style="color: #0969da; cursor: pointer;">View</div>
      </div>
    </div>
  </div>

  <div class="section-page" id="advanced">
    <h2>Advanced</h2>
    <p>Advanced settings</p>
    <div class="card">
      <h3>Network</h3>
      <div class="setting-row">
        <div>
          <div class="setting-label">Use secure DNS</div>
          <div class="setting-desc">Use DNS-over-HTTPS for more secure lookups</div>
        </div>
        <div class="setting-control"><input type="checkbox" checked></div>
      </div>
      <div class="setting-row">
        <div>
          <div class="setting-label">Proxy</div>
          <div class="setting-desc">Configure network proxy settings</div>
        </div>
        <div class="setting-control">
          <button style="padding: 6px 14px; border: 1px solid #d0d7de; border-radius: 6px; background: #fff; cursor: pointer; font-size: 13px;">Configure...</button>
        </div>
      </div>
    </div>
    <div class="card">
      <h3>Developer</h3>
      <div class="setting-row">
        <div>
          <div class="setting-label">Remote debugging port</div>
          <div class="setting-desc">Port for Chrome DevTools remote debugging</div>
        </div>
        <div class="setting-control">
          <input type="text" placeholder="Not set" style="width: 120px;">
        </div>
      </div>
    </div>
  </div>
</div>

<script>
  // Section switching
  const sidebarItems = document.querySelectorAll('.sidebar-item');
  const sections = document.querySelectorAll('.section-page');

  sidebarItems.forEach(item => {
    item.addEventListener('click', () => {
      const section = item.dataset.section;

      // Update sidebar
      sidebarItems.forEach(i => i.classList.remove('active'));
      item.classList.add('active');

      // Update content
      sections.forEach(s => s.classList.remove('active'));
      const target = document.getElementById(section);
      if (target) target.classList.add('active');
    });
  });
</script>
</body>
</html>
)";
}

void AstraApp::NotifyTabPinnedChanged(const std::shared_ptr<BrowserTab>& tab) {
  for (auto* obs : observers_) {
    obs->OnTabPinnedChanged(tab);
  }
}

void AstraApp::NotifyTabFavoriteChanged(const std::shared_ptr<BrowserTab>& tab) {
  for (auto* obs : observers_) {
    obs->OnTabFavoriteChanged(tab);
  }
}

void AstraApp::NotifyTabMoved(const std::string& tabId, size_t oldIndex, size_t newIndex) {
  for (auto* obs : observers_) {
    obs->OnTabMoved(tabId, oldIndex, newIndex);
  }
}

void AstraApp::NotifyTabGroupCreated(const std::shared_ptr<TabGroup>& group) {
  for (auto* obs : observers_) {
    obs->OnTabGroupCreated(group);
  }
}

void AstraApp::NotifyTabGroupRemoved(const std::string& groupId) {
  for (auto* obs : observers_) {
    obs->OnTabGroupRemoved(groupId);
  }
}

void AstraApp::NotifyTabGroupUpdated(const std::shared_ptr<TabGroup>& group) {
  for (auto* obs : observers_) {
    obs->OnTabGroupUpdated(group);
  }
}

void AstraApp::NotifyRecentlyClosedChanged() {
  for (auto* obs : observers_) {
    obs->OnRecentlyClosedChanged();
  }
}

void AstraApp::NotifyHistoryChanged() {
  for (auto* obs : observers_) {
    obs->OnHistoryChanged();
  }
}

void AstraApp::NotifyDownloadCreated(const std::shared_ptr<DownloadItem>& download) {
  for (auto* obs : observers_) {
    obs->OnDownloadCreated(download);
  }
}

void AstraApp::NotifyDownloadUpdated(const std::shared_ptr<DownloadItem>& download) {
  for (auto* obs : observers_) {
    obs->OnDownloadUpdated(download);
  }
}

void AstraApp::NotifyFindResult(const std::string& tabId,
                                int matchCount,
                                int activeMatchOrdinal,
                                bool finalUpdate) {
  for (auto* obs : observers_) {
    obs->OnFindResult(tabId, matchCount, activeMatchOrdinal, finalUpdate);
  }
}
