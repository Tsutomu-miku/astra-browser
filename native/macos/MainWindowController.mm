//
//  MainWindowController.mm
//  Astra Browser
//
//  Manages the main browser window: sidebar + content area.
//  Uses astra::Browser for tab management (Chromium content module pattern).

#import "native/macos/MainWindowController.h"
#import "native/macos/SidebarViewController.h"
#import "native/macos/BrowserContainerView.h"
#import "native/macos/AddressBar.h"
#import "native/macos/FindBarView.h"

#include "browser/core/Browser.h"

// DCHECK macro for debug assertions
#ifndef DCHECK
#define DCHECK(condition) NSAssert(condition, @"DCHECK failed: %s", #condition)
#endif

// C++ → ObjC bridge for BrowserObserver
namespace {
class ObserverBridge : public astra::BrowserObserver {
 public:
  __weak MainWindowController* owner = nil;

  void OnTabAdded(astra::WebContents* tab) override;
  void OnTabRemoved(const std::string& tab_id) override;
  void OnActiveTabChanged(astra::WebContents* tab) override;
  void OnTitleChanged(astra::WebContents* tab, const std::string& title) override;
  void OnURLChanged(astra::WebContents* tab, const std::string& url) override;
  void OnFaviconChanged(astra::WebContents* tab, const std::string& favicon_url) override;
  void OnLoadingStateChanged(astra::WebContents* tab,
                             const astra::NavigationState& state) override;
  void OnTabPinnedChanged(astra::WebContents* tab, bool pinned) override;
  void OnTabMutedChanged(astra::WebContents* tab, bool muted) override;
};
}  // namespace

@interface MainWindowController () <SidebarViewControllerDelegate, AddressBarDelegate, FindBarViewDelegate> {
 @public
  std::unique_ptr<ObserverBridge> _observerBridge;
  std::unique_ptr<astra::Browser> _browser;
}

@property (nonatomic, strong) NSSplitView *splitView;
@property (nonatomic, strong) SidebarViewController *sidebarVC;
@property (nonatomic, strong) BrowserContainerView *browserContainer;
@property (nonatomic, strong) AddressBar *addressBar;
@property (nonatomic, strong) NSVisualEffectView *topBarView;
@property (nonatomic, strong) NSLayoutConstraint *splitViewTopConstraint;

@end

@implementation MainWindowController

- (instancetype)init {
  return [self initWithIncognito:NO];
}

- (instancetype)initWithIncognito:(BOOL)incognito {
  // Legacy initializer — creates a default browser from the global app.
  DCHECK(astra::g_browser_app);
  auto browser = astra::g_browser_app->CreateBrowser(incognito);
  return [self initWithBrowser:std::move(browser) incognito:incognito];
}

- (instancetype)initWithBrowser:(std::unique_ptr<astra::Browser>)browser
                      incognito:(BOOL)incognito {
  self = [super init];
  if (self) {
    _isIncognito = incognito;
    _browser = std::move(browser);

    // Create window programmatically
    NSRect frame = NSMakeRect(0, 0, 1200, 800);
    NSWindow *window = [[NSWindow alloc] initWithContentRect:frame
                                                   styleMask:NSWindowStyleMaskTitled |
                                                             NSWindowStyleMaskClosable |
                                                             NSWindowStyleMaskMiniaturizable |
                                                             NSWindowStyleMaskResizable |
                                                             NSWindowStyleMaskFullSizeContentView
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];
    [window setTitle:incognito ? @"Astra — Incognito" : @"Astra"];
    [window setTitlebarAppearsTransparent:YES];
    [window setBackgroundColor:[NSColor windowBackgroundColor]];
    [window setFrameAutosaveName:incognito ? @"AstraIncognitoWindow" : @"AstraMainWindow"];
    [window center];

    // Incognito window: dark purple appearance
    if (incognito) {
      if (@available(macOS 10.14, *)) {
        [window setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameDarkAqua]];
      }
    }

    self.window = window;
    self.window.delegate = self;

    // Set up C++ observer bridge
    _observerBridge = std::make_unique<ObserverBridge>();
    _observerBridge->owner = self;
    _browser->AddObserver(_observerBridge.get());

    // Add initial tab
    _browser->AddTab("https://www.google.com",
                     incognito ? "New Incognito Tab" : "New Tab",
                     /*activate=*/true);

    // Set up UI (windowDidLoad is not called for programmatic windows)
    [self setupTopBar];
    [self setupSplitView];

    // Create native view for the active tab (after view hierarchy is ready)
    dispatch_async(dispatch_get_main_queue(), ^{
      [self createActiveTabNativeView];
    });
  }
  return self;
}

- (void)dealloc {
  if (_browser && _observerBridge) {
    _browser->RemoveObserver(_observerBridge.get());
  }
}

- (astra::Browser *)browser {
  return _browser.get();
}

- (void)windowDidLoad {
  [super windowDidLoad];
}

#pragma mark - UI Setup

- (void)setupTopBar {
  NSView *contentView = self.window.contentView;
  contentView.wantsLayer = YES;

  self.topBarView = [[NSVisualEffectView alloc] init];
  self.topBarView.material = NSVisualEffectMaterialTitlebar;
  self.topBarView.blendingMode = NSVisualEffectBlendingModeBehindWindow;
  self.topBarView.state = NSVisualEffectStateActive;
  self.topBarView.translatesAutoresizingMaskIntoConstraints = NO;
  [contentView addSubview:self.topBarView];

  // Incognito icon (eye slash = private browsing) in the top bar
  NSImageView *incognitoIcon = nil;
  if (self.isIncognito) {
    incognitoIcon = [[NSImageView alloc] init];
    incognitoIcon.image = [NSImage imageWithSystemSymbolName:@"eye.slash.fill" accessibilityDescription:nil];
    incognitoIcon.imageScaling = NSImageScaleProportionallyUpOrDown;
    incognitoIcon.translatesAutoresizingMaskIntoConstraints = NO;
    incognitoIcon.toolTip = @"Incognito Mode";
    incognitoIcon.contentTintColor = [NSColor colorWithRed:0.58 green:0.20 blue:0.92 alpha:1.0];
    [self.topBarView addSubview:incognitoIcon];
  }

  self.addressBar = [[AddressBar alloc] init];
  self.addressBar.delegate = self;
  self.addressBar.translatesAutoresizingMaskIntoConstraints = NO;
  [self.topBarView addSubview:self.addressBar];

  if (self.isIncognito) {
    self.addressBar.incognitoMode = YES;
  }

  [NSLayoutConstraint activateConstraints:@[
    [self.topBarView.topAnchor constraintEqualToAnchor:contentView.topAnchor],
    [self.topBarView.leadingAnchor constraintEqualToAnchor:contentView.leadingAnchor],
    [self.topBarView.trailingAnchor constraintEqualToAnchor:contentView.trailingAnchor],
    [self.topBarView.heightAnchor constraintEqualToConstant:52.0],

    [self.addressBar.centerYAnchor constraintEqualToAnchor:self.topBarView.centerYAnchor constant:2.0],
    [self.addressBar.trailingAnchor constraintEqualToAnchor:self.topBarView.trailingAnchor constant:-16.0],
    [self.addressBar.heightAnchor constraintEqualToConstant:30.0],
  ]];

  if (incognitoIcon) {
    [NSLayoutConstraint activateConstraints:@[
      [incognitoIcon.leadingAnchor constraintEqualToAnchor:self.topBarView.leadingAnchor constant:240.0],
      [incognitoIcon.centerYAnchor constraintEqualToAnchor:self.topBarView.centerYAnchor constant:2.0],
      [incognitoIcon.widthAnchor constraintEqualToConstant:20.0],
      [incognitoIcon.heightAnchor constraintEqualToConstant:20.0],
      [self.addressBar.leadingAnchor constraintEqualToAnchor:incognitoIcon.trailingAnchor constant:8.0],
    ]];
  } else {
    [NSLayoutConstraint activateConstraints:@[
      [self.addressBar.leadingAnchor constraintEqualToAnchor:self.topBarView.leadingAnchor constant:240.0],
    ]];
  }
}

- (void)setupSplitView {
  NSView *contentView = self.window.contentView;

  // Find bar (hidden by default)
  self.findBar = [[FindBarView alloc] init];
  self.findBar.delegate = self;
  self.findBar.hidden = YES;
  self.findBar.alphaValue = 0.0;
  [contentView addSubview:self.findBar];

  self.splitView = [[NSSplitView alloc] init];
  self.splitView.translatesAutoresizingMaskIntoConstraints = NO;
  self.splitView.vertical = YES;
  self.splitView.dividerStyle = NSSplitViewDividerStyleThin;
  [contentView addSubview:self.splitView];

  // Sidebar
  self.sidebarVC = [[SidebarViewController alloc] init];
  self.sidebarVC.delegate = self;
  self.sidebarVC.incognitoMode = self.isIncognito;
  self.sidebarVC.browser = _browser.get();
  auto* activeTab = _browser->GetActiveTab();
  if (activeTab) {
    self.sidebarVC.activeTabId = [NSString stringWithUTF8String:activeTab->id().c_str()];
  }
  NSView *sidebarView = self.sidebarVC.view;
  sidebarView.translatesAutoresizingMaskIntoConstraints = NO;
  [self.splitView addArrangedSubview:sidebarView];

  // Browser content
  self.browserContainer = [[BrowserContainerView alloc] init];
  self.browserContainer.translatesAutoresizingMaskIntoConstraints = NO;
  [self.splitView addArrangedSubview:self.browserContainer];

  self.splitViewTopConstraint =
      [self.splitView.topAnchor constraintEqualToAnchor:self.topBarView.bottomAnchor];

  [NSLayoutConstraint activateConstraints:@[
    // Find bar
    [self.findBar.topAnchor constraintEqualToAnchor:self.topBarView.bottomAnchor constant:4],
    [self.findBar.leadingAnchor constraintEqualToAnchor:contentView.leadingAnchor constant:240],
    [self.findBar.widthAnchor constraintEqualToConstant:320],
    [self.findBar.heightAnchor constraintEqualToConstant:32],

    // Split view
    self.splitViewTopConstraint,
    [self.splitView.leadingAnchor constraintEqualToAnchor:contentView.leadingAnchor],
    [self.splitView.trailingAnchor constraintEqualToAnchor:contentView.trailingAnchor],
    [self.splitView.bottomAnchor constraintEqualToAnchor:contentView.bottomAnchor],

    [sidebarView.widthAnchor constraintGreaterThanOrEqualToConstant:220.0],
    [sidebarView.widthAnchor constraintLessThanOrEqualToConstant:400.0],
  ]];

  [self.splitView setPosition:260.0 ofDividerAtIndex:0];
}

- (void)createActiveTabNativeView {
  auto* tab = _browser->GetActiveTab();
  if (!tab) return;

  if (!tab->GetNativeView()) {
    NSView *hostView = self.browserContainer.browserHostView;
    tab->CreateNativeView((__bridge void*)hostView);
  }
  // Native view will be shown when OnTitleChanged / OnLoadingStateChanged fires
}

#pragma mark - Show Active Tab

- (void)showActiveTabBrowser {
  auto* tab = _browser->GetActiveTab();
  if (!tab) {
    [self.browserContainer hideAllBrowsers];
    return;
  }

  void* nativeView = tab->GetNativeView();
  if (nativeView) {
    NSView *view = (__bridge NSView *)nativeView;
    [self.browserContainer showBrowserWithTabId:tab->id() view:view];
  } else {
    // Native view not yet created — create it now
    NSView *hostView = self.browserContainer.browserHostView;
    tab->CreateNativeView((__bridge void*)hostView);
    // It will be shown when the browser is ready
  }

  // Update address bar
  self.addressBar.urlString = [NSString stringWithUTF8String:tab->url().c_str()];
  self.addressBar.isLoading = tab->navigation_state().is_loading;
}

#pragma mark - Public API

- (void)createNewTab:(NSString *)url title:(NSString *)title {
  std::string urlStr = [url UTF8String];
  std::string titleStr = [title UTF8String];
  auto* tab = _browser->AddTab(urlStr, titleStr, /*activate=*/true);
  if (tab) {
    self.sidebarVC.activeTabId = [NSString stringWithUTF8String:tab->id().c_str()];
    [self.sidebarVC reloadTabData];
  }
}

- (void)closeActiveTab {
  auto* tab = _browser->GetActiveTab();
  if (!tab) return;
  std::string tabId = tab->id();
  _browser->CloseTab(tabId);
}

- (void)selectTabWithIndex:(NSInteger)index {
  const auto& tabs = _browser->tabs();
  if (index < 0 || (size_t)index >= tabs.size()) return;
  _browser->ActivateTab(tabs[index]->id());
}

- (void)navigateActiveTabTo:(NSString *)url {
  auto* tab = _browser->GetActiveTab();
  if (!tab) return;
  tab->Navigate(std::string([url UTF8String]));
}

#pragma mark - Navigation

- (void)reloadActiveTab {
  auto* tab = _browser->GetActiveTab();
  if (tab) tab->Reload();
}

- (void)forceReloadActiveTab {
  auto* tab = _browser->GetActiveTab();
  if (tab) tab->ReloadBypassingCache();
}

- (void)goBack {
  auto* tab = _browser->GetActiveTab();
  if (tab && tab->navigation_state().can_go_back) {
    tab->GoBack();
  }
}

- (void)goForward {
  auto* tab = _browser->GetActiveTab();
  if (tab && tab->navigation_state().can_go_forward) {
    tab->GoForward();
  }
}

#pragma mark - Zoom

- (void)zoomIn {
  auto* tab = _browser->GetActiveTab();
  if (tab) {
    tab->SetZoomLevel(tab->GetZoomLevel() + 0.5);
  }
}

- (void)zoomOut {
  auto* tab = _browser->GetActiveTab();
  if (tab) {
    tab->SetZoomLevel(tab->GetZoomLevel() - 0.5);
  }
}

- (void)zoomActualSize {
  auto* tab = _browser->GetActiveTab();
  if (tab) {
    tab->SetZoomLevel(0.0);
  }
}

#pragma mark - Find

- (void)showFind {
  [self showFindBar];
}

- (void)showFindBar {
  self.findBar.hidden = NO;
  [NSAnimationContext runAnimationGroup:^(NSAnimationContext *context) {
    context.duration = 0.15;
    self.findBar.alphaValue = 1.0;
  } completionHandler:^{
    [self.findBar focusSearchField];
  }];
}

- (void)hideFindBar {
  [NSAnimationContext runAnimationGroup:^(NSAnimationContext *context) {
    context.duration = 0.15;
    self.findBar.alphaValue = 0.0;
  } completionHandler:^{
    self.findBar.hidden = YES;
    auto* tab = self->_browser->GetActiveTab();
    if (tab) {
      tab->StopFinding(true);
    }
  }];
}

#pragma mark - Other

- (void)savePageAs {
  // TODO: implement save as
}

- (void)printPage {
  auto* tab = _browser->GetActiveTab();
  if (tab) {
    tab->Print();
  }
}

- (void)showPreferences {
  // TODO: implement settings page
  auto* tab = _browser->GetActiveTab();
  if (tab) {
    tab->Navigate("astra://settings");
  }
}

- (void)toggleDevTools {
  auto* tab = _browser->GetActiveTab();
  if (!tab) return;

  if (tab->HasDevTools()) {
    tab->CloseDevTools();
  } else {
    tab->ShowDevTools();
  }
}

#pragma mark - SidebarViewControllerDelegate

- (void)sidebarDidSelectTab:(const std::string&)tabId {
  _browser->ActivateTab(tabId);
}

- (void)sidebarDidRequestNewTab {
  [self createNewTab:@"https://www.google.com"
                title:self.isIncognito ? @"New Incognito Tab" : @"New Tab"];
}

- (void)sidebarDidRequestCloseTab:(const std::string&)tabId {
  _browser->CloseTab(tabId);
}

- (void)sidebarDidRequestPinTab:(const std::string&)tabId {
  auto* tab = _browser->GetTab(tabId);
  if (tab) tab->SetPinned(true);
}

- (void)sidebarDidRequestUnpinTab:(const std::string&)tabId {
  auto* tab = _browser->GetTab(tabId);
  if (tab) tab->SetPinned(false);
}

- (void)sidebarDidRequestFavoriteTab:(const std::string&)tabId {
  // TODO: implement favorites
  (void)tabId;
}

- (void)sidebarDidRequestUnfavoriteTab:(const std::string&)tabId {
  // TODO: implement favorites
  (void)tabId;
}

- (void)sidebarDidRequestDuplicateTab:(const std::string&)tabId {
  auto* tab = _browser->GetTab(tabId);
  if (!tab) return;
  _browser->AddTab(tab->url(), tab->title(), /*activate=*/true);
}

- (void)sidebarDidRequestReloadTab:(const std::string&)tabId {
  auto* tab = _browser->GetTab(tabId);
  if (tab) tab->Reload();
}

- (void)sidebarDidRequestMuteTab:(const std::string&)tabId muted:(BOOL)muted {
  auto* tab = _browser->GetTab(tabId);
  if (tab) tab->SetMuted(muted);
}

- (void)sidebarDidRequestRestoreRecentlyClosed:(const std::string&)tabId {
  // TODO: implement recently closed tabs
  (void)tabId;
}

- (void)sidebarDidToggleSection:(SidebarSection)section {
  // TODO: handle section toggle
  (void)section;
}

#pragma mark - AddressBarDelegate

- (void)addressBarDidCommitNavigation:(NSString *)url {
  [self navigateActiveTabTo:url];
}

#pragma mark - FindBarViewDelegate

- (void)findBarDidChangeText:(NSString *)text {
  auto* tab = _browser->GetActiveTab();
  if (!tab) return;

  if (text.length == 0) {
    tab->StopFinding(true);
    self.findBar.countLabel.stringValue = @"";
    self.findBar.prevButton.enabled = NO;
    self.findBar.nextButton.enabled = NO;
    return;
  }

  // Find first occurrence (findNext = false means first search)
  tab->Find(std::string([text UTF8String]),
            /*forward=*/true,
            /*match_case=*/false,
            /*find_next=*/false);
}

- (void)findBarDidFindNext {
  auto* tab = _browser->GetActiveTab();
  if (!tab) return;

  NSString *text = self.findBar.searchField.stringValue;
  if (text.length == 0) return;

  tab->Find(std::string([text UTF8String]),
            /*forward=*/true,
            /*match_case=*/false,
            /*find_next=*/true);
}

- (void)findBarDidFindPrevious {
  auto* tab = _browser->GetActiveTab();
  if (!tab) return;

  NSString *text = self.findBar.searchField.stringValue;
  if (text.length == 0) return;

  tab->Find(std::string([text UTF8String]),
            /*forward=*/false,
            /*match_case=*/false,
            /*find_next=*/true);
}

- (void)findBarDidClose {
  [self hideFindBar];
}

#pragma mark - State observation (called from C++ bridge)

- (void)handleTabAdded:(astra::WebContents*)tab {
  dispatch_async(dispatch_get_main_queue(), ^{
    [self.sidebarVC reloadTabData];
  });
}

- (void)handleTabRemoved:(const std::string&)tabId {
  NSString *tabIdStr = [NSString stringWithUTF8String:tabId.c_str()];
  BOOL isActiveTab = [self.sidebarVC.activeTabId isEqualToString:tabIdStr];

  dispatch_async(dispatch_get_main_queue(), ^{
    [self.browserContainer removeBrowserForTabId:tabId];
    [self.sidebarVC reloadTabData];

    // If the closed tab was the active one, update UI
    if (isActiveTab) {
      auto* newActive = self->_browser->GetActiveTab();
      if (newActive) {
        self.sidebarVC.activeTabId = [NSString stringWithUTF8String:newActive->id().c_str()];
        [self showActiveTabBrowser];
      } else {
        // No tabs left — close window
        [self.window close];
      }
    }
  });
}

- (void)handleActiveTabChanged:(astra::WebContents*)tab {
  dispatch_async(dispatch_get_main_queue(), ^{
    if (tab) {
      self.sidebarVC.activeTabId = [NSString stringWithUTF8String:tab->id().c_str()];
    } else {
      self.sidebarVC.activeTabId = nil;
    }
    [self showActiveTabBrowser];
  });
}

- (void)handleTitleChanged:(astra::WebContents*)tab title:(const std::string&)title {
  dispatch_async(dispatch_get_main_queue(), ^{
    // Update sidebar
    [self.sidebarVC reloadTabData];

    // Update window title if this is the active tab
    auto* active = self->_browser->GetActiveTab();
    if (active && active == tab) {
      NSString *titleStr = [NSString stringWithUTF8String:title.c_str()];
      if (titleStr.length > 0) {
        self.window.title = [NSString stringWithFormat:@"%@ — Astra", titleStr];
      }
    }
  });
}

- (void)handleURLChanged:(astra::WebContents*)tab url:(const std::string&)url {
  dispatch_async(dispatch_get_main_queue(), ^{
    // Update address bar if this is the active tab
    auto* active = self->_browser->GetActiveTab();
    if (active && active == tab) {
      self.addressBar.urlString = [NSString stringWithUTF8String:url.c_str()];
    }
  });
}

- (void)handleFaviconChanged:(astra::WebContents*)tab faviconUrl:(const std::string&)faviconUrl {
  dispatch_async(dispatch_get_main_queue(), ^{
    [self.sidebarVC reloadTabData];
  });
}

- (void)handleLoadingStateChanged:(astra::WebContents*)tab state:(const astra::NavigationState&)state {
  dispatch_async(dispatch_get_main_queue(), ^{
    // Update address bar loading state if this is the active tab
    auto* active = self->_browser->GetActiveTab();
    if (active && active == tab) {
      self.addressBar.isLoading = state.is_loading;
    }

    // If browser just became ready and we haven't shown it yet, show it
    if (![self.browserContainer hasBrowserForTabId:tab->id()]) {
      void* nativeView = tab->GetNativeView();
      if (nativeView) {
        NSView *view = (__bridge NSView *)nativeView;
        [self.browserContainer showBrowserWithTabId:tab->id() view:view];
      }
    }
  });
}

- (void)handleTabPinnedChanged:(astra::WebContents*)tab pinned:(bool)pinned {
  dispatch_async(dispatch_get_main_queue(), ^{
    [self.sidebarVC reloadTabData];
  });
}

- (void)handleTabMutedChanged:(astra::WebContents*)tab muted:(bool)muted {
  dispatch_async(dispatch_get_main_queue(), ^{
    [self.sidebarVC reloadTabData];
  });
}

#pragma mark - NSWindowDelegate

- (void)windowWillClose:(NSNotification *)notification {
  // Browser will be destroyed when _browser unique_ptr goes out of scope
  // All tabs are owned by the browser and will be cleaned up automatically
}

@end

// ============================================================
// C++ → ObjC bridge implementations
// ============================================================

namespace {

void ObserverBridge::OnTabAdded(astra::WebContents* tab) {
  if (owner) { [owner handleTabAdded:tab]; }
}

void ObserverBridge::OnTabRemoved(const std::string& tab_id) {
  if (owner) { [owner handleTabRemoved:tab_id]; }
}

void ObserverBridge::OnActiveTabChanged(astra::WebContents* tab) {
  if (owner) { [owner handleActiveTabChanged:tab]; }
}

void ObserverBridge::OnTitleChanged(astra::WebContents* tab, const std::string& title) {
  if (owner) { [owner handleTitleChanged:tab title:title]; }
}

void ObserverBridge::OnURLChanged(astra::WebContents* tab, const std::string& url) {
  if (owner) { [owner handleURLChanged:tab url:url]; }
}

void ObserverBridge::OnFaviconChanged(astra::WebContents* tab, const std::string& favicon_url) {
  if (owner) { [owner handleFaviconChanged:tab faviconUrl:favicon_url]; }
}

void ObserverBridge::OnLoadingStateChanged(astra::WebContents* tab,
                                          const astra::NavigationState& state) {
  if (owner) { [owner handleLoadingStateChanged:tab state:state]; }
}

void ObserverBridge::OnTabPinnedChanged(astra::WebContents* tab, bool pinned) {
  if (owner) { [owner handleTabPinnedChanged:tab pinned:pinned]; }
}

void ObserverBridge::OnTabMutedChanged(astra::WebContents* tab, bool muted) {
  if (owner) { [owner handleTabMutedChanged:tab muted:muted]; }
}

}  // namespace
