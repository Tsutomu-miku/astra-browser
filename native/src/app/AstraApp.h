#pragma once

#include "include/cef_app.h"
#include "include/cef_client.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

class AstraClient;

// ============================================================
// Data model — single source of truth for browser state
// ============================================================

struct TabGroup {
  std::string id;
  std::string name;
  std::string color;
  bool collapsed = false;
  std::vector<std::string> tabIds;  // ordered list of tab IDs in this group
};

struct HistoryEntry {
  std::string id;
  std::string url;
  std::string title;
  int64_t timestamp = 0;  // unix timestamp in ms
  std::string faviconUrl;
};

enum class DownloadState {
  InProgress,
  Complete,
  Cancelled,
  Interrupted,
};

struct DownloadItem {
  std::string id;
  std::string url;
  std::string filename;
  std::string filePath;  // full path once download starts
  std::string mimeType;
  int64_t totalBytes = 0;
  int64_t receivedBytes = 0;
  DownloadState state = DownloadState::InProgress;
  int64_t startTime = 0;  // unix timestamp in ms
  int64_t endTime = 0;    // unix timestamp in ms
};

struct BrowserTab {
  std::string id;
  std::string title;
  std::string url;
  std::string faviconUrl;
  bool isLoading = false;
  bool canGoBack = false;
  bool canGoForward = false;
  bool isMuted = false;
  bool isPinned = false;
  bool isFavorite = false;
  bool isIncognito = false;
  std::string groupId;  // empty if not in a group
  int64_t closedAt = 0;  // unix timestamp in ms, 0 if not closed
};

struct Workspace {
  std::string id;
  std::string name;
  std::string accentColor;
  std::vector<std::shared_ptr<BrowserTab>> tabs;
  std::string activeTabId;

  // Pinned tabs (ordered)
  std::vector<std::string> pinnedTabIds;

  // Favorites (ordered list of tab IDs)
  std::vector<std::string> favoriteTabIds;

  // Tab groups
  std::vector<std::shared_ptr<TabGroup>> groups;

  // Recently closed tabs (most recent first)
  std::vector<std::shared_ptr<BrowserTab>> recentlyClosed;
  static constexpr size_t kMaxRecentlyClosed = 25;

  // Browsing history (most recent first)
  std::vector<std::shared_ptr<HistoryEntry>> history;
  static constexpr size_t kMaxHistoryEntries = 500;

  // Downloads (most recent first)
  std::vector<std::shared_ptr<DownloadItem>> downloads;
  static constexpr size_t kMaxDownloads = 100;

  // Sidebar section collapse states
  bool pinnedCollapsed = false;
  bool favoritesCollapsed = false;
  bool tabsCollapsed = false;
  bool recentlyClosedCollapsed = false;
  bool historyCollapsed = false;
  bool downloadsCollapsed = false;
};

// Observer interface for UI to subscribe to state changes
class BrowserStateObserver {
 public:
  virtual ~BrowserStateObserver() = default;
  virtual void OnTabAdded(const std::shared_ptr<BrowserTab>& tab) {}
  virtual void OnTabRemoved(const std::string& tabId) {}
  virtual void OnActiveTabChanged(const std::shared_ptr<BrowserTab>& tab) {}
  virtual void OnTabUpdated(const std::shared_ptr<BrowserTab>& tab) {}
  virtual void OnTabPinnedChanged(const std::shared_ptr<BrowserTab>& tab) {}
  virtual void OnTabFavoriteChanged(const std::shared_ptr<BrowserTab>& tab) {}
  virtual void OnTabMoved(const std::string& tabId, size_t oldIndex, size_t newIndex) {}
  virtual void OnTabGroupCreated(const std::shared_ptr<TabGroup>& group) {}
  virtual void OnTabGroupRemoved(const std::string& groupId) {}
  virtual void OnTabGroupUpdated(const std::shared_ptr<TabGroup>& group) {}
  virtual void OnRecentlyClosedChanged() {}
  virtual void OnHistoryChanged() {}
  virtual void OnDownloadCreated(const std::shared_ptr<DownloadItem>& download) {}
  virtual void OnDownloadUpdated(const std::shared_ptr<DownloadItem>& download) {}
  virtual void OnTabLoadingStateChanged(const std::shared_ptr<BrowserTab>& tab,
                                   bool isLoading) {}
  virtual void OnFindResult(const std::string& tabId,
                            int matchCount,
                            int activeMatchOrdinal,
                            bool finalUpdate) {}
};

// ============================================================
// AstraApp — browser process app singleton
// ============================================================

class AstraApp : public CefApp,
                 public CefBrowserProcessHandler,
                 public CefRenderProcessHandler {
 public:
  AstraApp();
  ~AstraApp() override;

  // CefApp methods
  CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override {
    return this;
  }
  CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override {
    return this;
  }
  void OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar) override;

  // CefBrowserProcessHandler methods
  void OnContextInitialized() override;

  // CefRenderProcessHandler methods
  void OnContextCreated(CefRefPtr<CefBrowser> browser,
                        CefRefPtr<CefFrame> frame,
                        CefRefPtr<CefV8Context> context) override;

  // ==========================================================
  // Public API — tab / workspace management
  // ==========================================================

  std::shared_ptr<Workspace> GetActiveWorkspace();
  std::shared_ptr<Workspace> GetWorkspace(const std::string& workspaceId);

  // Tab operations
  std::shared_ptr<BrowserTab> CreateNewTab(
      const std::string& url,
      const std::string& title,
      bool activate = true);
  std::shared_ptr<BrowserTab> CreateNewIncognitoTab(
      const std::string& url,
      const std::string& title,
      bool activate = true);
  void CloseTab(const std::string& tabId);
  void SelectTab(const std::string& tabId);
  void NavigateTab(const std::string& tabId, const std::string& url);
  void ReloadTab(const std::string& tabId);
  void ReloadTabBypassingCache(const std::string& tabId);
  void GoBack(const std::string& tabId);
  void GoForward(const std::string& tabId);
  std::shared_ptr<BrowserTab> GetActiveTab();
  std::shared_ptr<BrowserTab> GetTab(const std::string& tabId);
  std::shared_ptr<BrowserTab> DuplicateTab(const std::string& tabId,
                                            bool activate = false);

  // Tab pinning
  void PinTab(const std::string& tabId);
  void UnpinTab(const std::string& tabId);
  void ToggleTabPinned(const std::string& tabId);

  // Tab favorites
  void AddToFavorites(const std::string& tabId);
  void RemoveFromFavorites(const std::string& tabId);
  void ToggleTabFavorite(const std::string& tabId);

  // Tab reordering
  void MoveTab(size_t fromIndex, size_t toIndex);

  // Tab mute
  void SetTabMuted(const std::string& tabId, bool muted);
  void ToggleTabMuted(const std::string& tabId);

  // Tab groups
  std::shared_ptr<TabGroup> CreateTabGroup(const std::string& name,
                                            const std::string& color);
  void RemoveTabGroup(const std::string& groupId);
  void MoveTabToGroup(const std::string& tabId, const std::string& groupId);
  void RemoveTabFromGroup(const std::string& tabId);
  void ToggleTabGroupCollapsed(const std::string& groupId);
  void RenameTabGroup(const std::string& groupId, const std::string& name);
  std::shared_ptr<TabGroup> GetTabGroup(const std::string& groupId);
  const std::vector<std::shared_ptr<TabGroup>>& GetTabGroups();

  // Recently closed tabs
  const std::vector<std::shared_ptr<BrowserTab>>& GetRecentlyClosed();
  std::shared_ptr<BrowserTab> RestoreRecentlyClosedTab(
      const std::string& tabId, bool activate = true);
  std::shared_ptr<BrowserTab> RestoreMostRecentlyClosed(bool activate = true);
  void ClearRecentlyClosed();

  // History
  const std::vector<std::shared_ptr<HistoryEntry>>& GetHistory();
  void AddHistoryEntry(const std::string& url, const std::string& title);
  void ClearHistory();

  // Downloads
  const std::vector<std::shared_ptr<DownloadItem>>& GetDownloads();
  std::shared_ptr<DownloadItem> GetDownload(const std::string& downloadId);
  void AddDownload(const std::shared_ptr<DownloadItem>& download);
  void UpdateDownload(const std::shared_ptr<DownloadItem>& download);
  void CancelDownload(const std::string& downloadId);
  void OpenDownload(const std::string& downloadId);
  void ShowDownloadInFinder(const std::string& downloadId);
  void ClearCompletedDownloads();
  void SetDownloadCallback(const std::string& downloadId,
                            CefRefPtr<CefDownloadItemCallback> callback);

  // CEF browser management
  CefRefPtr<CefBrowser> GetCefBrowserForTab(const std::string& tabId);
  void EnsureBrowserForTab(const std::string& tabId,
                          cef_window_handle_t parentHandle);

  // Internal pages
  void OpenSettingsPage();
  std::string GetSettingsPageHtml();

  // Observers
  void AddObserver(BrowserStateObserver* observer);
  void RemoveObserver(BrowserStateObserver* observer);

 private:
  void InitializeDefaultWorkspace();
  std::string GenerateId();

  void NotifyTabAdded(const std::shared_ptr<BrowserTab>& tab);
  void NotifyTabRemoved(const std::string& tabId);
  void NotifyActiveTabChanged(const std::shared_ptr<BrowserTab>& tab);
  void NotifyTabUpdated(const std::shared_ptr<BrowserTab>& tab);
  void NotifyTabPinnedChanged(const std::shared_ptr<BrowserTab>& tab);
  void NotifyTabFavoriteChanged(const std::shared_ptr<BrowserTab>& tab);
  void NotifyTabMoved(const std::string& tabId, size_t oldIndex, size_t newIndex);
  void NotifyTabGroupCreated(const std::shared_ptr<TabGroup>& group);
  void NotifyTabGroupRemoved(const std::string& groupId);
  void NotifyTabGroupUpdated(const std::shared_ptr<TabGroup>& group);
  void NotifyRecentlyClosedChanged();
  void NotifyHistoryChanged();
  void NotifyDownloadCreated(const std::shared_ptr<DownloadItem>& download);
  void NotifyDownloadUpdated(const std::shared_ptr<DownloadItem>& download);
  void NotifyFindResult(const std::string& tabId,
                        int matchCount,
                        int activeMatchOrdinal,
                        bool finalUpdate);

  // Called by AstraClient when browser events happen
  friend class AstraClient;
  void OnBrowserTitleChanged(const std::string& tabId, const std::string& title);
  void OnBrowserUrlChanged(const std::string& tabId, const std::string& url);
  void OnBrowserLoadingStateChanged(const std::string& tabId,
                                     bool isLoading,
                                     bool canGoBack,
                                     bool canGoForward);
  void OnClientBrowserCreated(const std::string& tabId, CefRefPtr<CefBrowser> browser);
  void OnBrowserClosed(const std::string& tabId);
  void OnFindResult(const std::string& tabId,
                    int matchCount,
                    int activeMatchOrdinal,
                    bool finalUpdate);

  std::vector<std::shared_ptr<Workspace>> workspaces_;
  std::string activeWorkspaceId_;

  // tabId → AstraClient mapping (CEF client per tab)
  std::map<std::string, CefRefPtr<AstraClient>> clients_;

  // downloadId → callback mapping (for cancellation)
  std::map<std::string, CefRefPtr<CefDownloadItemCallback>> download_callbacks_;

  // Incognito request context (in-memory, no persistence)
  CefRefPtr<CefRequestContext> incognito_request_context_;

  std::vector<BrowserStateObserver*> observers_;

  IMPLEMENT_REFCOUNTING(AstraApp);
  DISALLOW_COPY_AND_ASSIGN(AstraApp);
};

// Global app instance
extern CefRefPtr<AstraApp> g_astraApp;
