//
//  MainWindowController.h
//  Astra Browser
//
//  Manages the main browser window: sidebar + content area.
//  Uses astra::Browser for tab management (Chromium content module pattern).

#import <Cocoa/Cocoa.h>

#include <memory>

#include "browser/core/Browser.h"

@class FindBarView;
@class SidebarViewController;
@class BrowserContainerView;
@class AddressBar;

@interface MainWindowController : NSWindowController <NSWindowDelegate>

@property (nonatomic, assign) BOOL isIncognito;
@property (nonatomic, strong) FindBarView *findBar;

// Initialize with a Browser instance (takes ownership).
- (instancetype)initWithBrowser:(std::unique_ptr<astra::Browser>)browser
                      incognito:(BOOL)incognito;

// Convenience initializer (legacy — creates a default browser)
- (instancetype)initWithIncognito:(BOOL)incognito;

// Tab operations
- (void)createNewTab:(NSString *)url title:(NSString *)title;
- (void)closeActiveTab;
- (void)selectTabWithIndex:(NSInteger)index;
- (void)navigateActiveTabTo:(NSString *)url;

// Navigation
- (void)reloadActiveTab;
- (void)forceReloadActiveTab;
- (void)goBack;
- (void)goForward;

// Zoom
- (void)zoomIn;
- (void)zoomOut;
- (void)zoomActualSize;

// Find
- (void)showFind;

// Other
- (void)savePageAs;
- (void)printPage;
- (void)showPreferences;
- (void)toggleDevTools;

// Access the underlying browser (for internal use)
- (astra::Browser *)browser;

@end
