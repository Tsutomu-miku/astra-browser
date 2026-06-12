#pragma once

// ============================================================
// Astra Browser — Chromium-Native Core
// ============================================================
//
// Architecture:
//   BrowserApp      — singleton, owns Chromium runtime
//   Browser         — one window, owns multiple WebContents
//   WebContents     — one tab, Chromium renderer + navigation
//   NavigationCtrl  — per-tab navigation history
//
// Rendering: Chromium (via CEF distribution, architecture mirrors
//            Chromium content module directly)
// UI:        100% native AppKit (macOS)
//
// No Electron, no Node.js, no JS main process.

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace astra {

// Forward declarations
class WebContents;
class Browser;
class BrowserApp;

// ============================================================
// Navigation state
// ============================================================

struct NavigationState {
  bool can_go_back = false;
  bool can_go_forward = false;
  bool is_loading = false;
  double load_progress = 0.0;
};

// ============================================================
// BrowserObserver — UI subscribes to browser events
// ============================================================

class BrowserObserver {
 public:
  virtual ~BrowserObserver() = default;

  // Tab lifecycle
  virtual void OnTabAdded(WebContents* tab) {}
  virtual void OnTabRemoved(const std::string& tab_id) {}
  virtual void OnActiveTabChanged(WebContents* tab) {}

  // Tab state
  virtual void OnTitleChanged(WebContents* tab, const std::string& title) {}
  virtual void OnURLChanged(WebContents* tab, const std::string& url) {}
  virtual void OnFaviconChanged(WebContents* tab, const std::string& favicon_url) {}
  virtual void OnLoadingStateChanged(WebContents* tab, const NavigationState& state) {}

  // Tab properties
  virtual void OnTabPinnedChanged(WebContents* tab, bool pinned) {}
  virtual void OnTabMutedChanged(WebContents* tab, bool muted) {}
};

// ============================================================
// WebContents — one tab's web content
// ============================================================
// Mirrors Chromium's content::WebContents pattern.

class WebContents {
 public:
  virtual ~WebContents() = default;

  // Identity
  virtual const std::string& id() const = 0;
  virtual const std::string& title() const = 0;
  virtual const std::string& url() const = 0;
  virtual const std::string& favicon_url() const = 0;

  // Navigation
  virtual const NavigationState& navigation_state() const = 0;
  virtual void Navigate(const std::string& url) = 0;
  virtual void GoBack() = 0;
  virtual void GoForward() = 0;
  virtual void Reload() = 0;
  virtual void ReloadBypassingCache() = 0;
  virtual void Stop() = 0;

  // Properties
  virtual bool is_incognito() const = 0;
  virtual bool is_pinned() const = 0;
  virtual bool is_muted() const = 0;
  virtual void SetPinned(bool pinned) = 0;
  virtual void SetMuted(bool muted) = 0;

  // Native view (NSView* on macOS)
  virtual void* GetNativeView() const = 0;

  // Create the native browser view parented to the given native view.
  // On platforms where browser creation is asynchronous, the native view
  // becomes available after OnTitleChanged / OnLoadingStateChanged fires.
  virtual void CreateNativeView(void* parent_native_view) = 0;

  // Find in page
  virtual void Find(const std::string& search_text,
                     bool forward,
                     bool match_case,
                     bool find_next) = 0;
  virtual void StopFinding(bool clear_selection) = 0;

  // DevTools
  virtual void ShowDevTools() = 0;
  virtual void CloseDevTools() = 0;
  virtual bool HasDevTools() const = 0;

  // Zoom
  virtual double GetZoomLevel() const = 0;
  virtual void SetZoomLevel(double level) = 0;

  // Printing
  virtual void Print() = 0;
};

// ============================================================
// Browser — window-level tab container
// ============================================================

class Browser {
 public:
  virtual ~Browser() = default;

  // Tab management
  virtual WebContents* AddTab(const std::string& url,
                               const std::string& title,
                               bool activate = true) = 0;
  virtual WebContents* AddIncognitoTab(const std::string& url,
                                        const std::string& title,
                                        bool activate = true) = 0;
  virtual void CloseTab(const std::string& tab_id) = 0;
  virtual void ActivateTab(const std::string& tab_id) = 0;
  virtual void MoveTab(size_t from_index, size_t to_index) = 0;

  // Query
  virtual WebContents* GetActiveTab() const = 0;
  virtual WebContents* GetTab(const std::string& tab_id) const = 0;
  virtual const std::vector<WebContents*>& tabs() const = 0;
  virtual size_t tab_count() const = 0;
  virtual bool is_incognito() const = 0;

  // Observers
  virtual void AddObserver(BrowserObserver* observer) = 0;
  virtual void RemoveObserver(BrowserObserver* observer) = 0;
};

// ============================================================
// BrowserApp — app-level singleton
// ============================================================

class BrowserApp {
 public:
  virtual ~BrowserApp() = default;

  // Lifecycle
  virtual bool Initialize(int argc, char* argv[]) = 0;
  virtual void Shutdown() = 0;

  // Browser creation
  virtual std::unique_ptr<Browser> CreateBrowser(bool incognito = false) = 0;

  // Message loop (for platforms that need explicit pumping)
  virtual void RunMessageLoopIteration() = 0;

  // Singleton access
  static BrowserApp* Get();
};

// Global app instance (non-owning pointer — lifecycle managed internally)
extern BrowserApp* g_browser_app;

}  // namespace astra
