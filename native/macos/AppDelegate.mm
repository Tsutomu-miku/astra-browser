//
//  AppDelegate.mm
//  Astra Browser
//
//  App delegate for the Chromium-native architecture.
//  Each window gets its own Browser instance from BrowserApp.

#import "native/macos/AppDelegate.h"
#import "native/macos/MainWindowController.h"

#include "browser/core/Browser.h"

// DCHECK macro for debug assertions
#ifndef DCHECK
#define DCHECK(condition) NSCAssert(condition, @"DCHECK failed: %s", #condition)
#endif

@interface AppDelegate ()

@property (nonatomic, strong) NSTimer *messageLoopTimer;

@end

@implementation AppDelegate

+ (instancetype)sharedDelegate {
  return (AppDelegate *)[NSApp delegate];
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _windowControllers = [NSMutableArray array];
  }
  return self;
}

- (void)applicationWillFinishLaunching:(NSNotification *)notification {
  // Chromium runtime is already initialized in main.mm.
  // Nothing to do here — NSApplication is being set up.
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
  // Create main menu
  [self setupMainMenu];

  // Create main window with a normal Browser
  [self createMainWindow:NO];

  // Start Chromium message loop pump timer.
  // On macOS, we manually pump the Chromium message loop via a timer
  // to ensure callbacks fire reliably on the main thread.
  self.messageLoopTimer = [NSTimer scheduledTimerWithTimeInterval:1.0/60.0
                                                           target:self
                                                         selector:@selector(pumpMessageLoop:)
                                                         userInfo:nil
                                                          repeats:YES];
  [[NSRunLoop mainRunLoop] addTimer:self.messageLoopTimer
                            forMode:NSRunLoopCommonModes];

  // Activate the app
  [NSApp activateIgnoringOtherApps:YES];
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
  // Stop message loop timer
  if (self.messageLoopTimer) {
    [self.messageLoopTimer invalidate];
    self.messageLoopTimer = nil;
  }

  // Shut down Chromium runtime
  if (astra::g_browser_app) {
    astra::g_browser_app->Shutdown();
    astra::g_browser_app = nullptr;
  }
}

- (void)pumpMessageLoop:(NSTimer *)timer {
  // Pump Chromium message loop — process any pending tasks
  if (astra::g_browser_app) {
    astra::g_browser_app->RunMessageLoopIteration();
  }
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
  return YES;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender {
  // Give Chromium time to shut down cleanly
  // For now, just terminate immediately
  return NSTerminateNow;
}

#pragma mark - Window creation

- (MainWindowController *)createMainWindow:(BOOL)incognito {
  DCHECK(astra::g_browser_app);

  // Create a new Browser instance from the app singleton
  std::unique_ptr<astra::Browser> browser =
      astra::g_browser_app->CreateBrowser(incognito);

  // Create window controller and pass ownership of the browser
  MainWindowController *wc = [[MainWindowController alloc]
      initWithBrowser:std::move(browser) incognito:incognito];

  if (!_mainWindowController && !incognito) {
    self.mainWindowController = wc;
  }
  [self.windowControllers addObject:wc];
  [wc showWindow:self];

  return wc;
}

#pragma mark - Main Menu

- (void)setupMainMenu {
  NSMenu *mainMenu = [[NSMenu alloc] initWithTitle:@"Astra"];
  NSApp.mainMenu = mainMenu;

  // App menu
  NSMenuItem *appMenuItem = [mainMenu addItemWithTitle:@"" action:NULL keyEquivalent:@""];
  NSMenu *appMenu = [[NSMenu alloc] initWithTitle:@"Astra"];
  [appMenu addItemWithTitle:@"About Astra"
                     action:@selector(orderFrontStandardAboutPanel:)
              keyEquivalent:@""];
  [appMenu addItem:[NSMenuItem separatorItem]];
  [appMenu addItemWithTitle:@"Settings…"
                     action:@selector(showPreferences:)
              keyEquivalent:@","];
  [appMenu addItem:[NSMenuItem separatorItem]];
  [appMenu addItemWithTitle:@"Services"
                     action:NULL
              keyEquivalent:@""];
  [appMenu addItem:[NSMenuItem separatorItem]];
  [appMenu addItemWithTitle:@"Hide Astra"
                     action:@selector(hide:)
              keyEquivalent:@"h"];
  [appMenu addItemWithTitle:@"Hide Others"
                     action:@selector(hideOtherApplications:)
              keyEquivalent:@"H"];
  [appMenu addItemWithTitle:@"Show All"
                     action:@selector(unhideAllApplications:)
              keyEquivalent:@""];
  [appMenu addItem:[NSMenuItem separatorItem]];
  [appMenu addItemWithTitle:@"Quit Astra"
                     action:@selector(terminate:)
              keyEquivalent:@"q"];
  appMenuItem.submenu = appMenu;

  // File menu
  NSMenuItem *fileMenuItem = [mainMenu addItemWithTitle:@"File" action:NULL keyEquivalent:@""];
  NSMenu *fileMenu = [[NSMenu alloc] initWithTitle:@"File"];
  [fileMenu addItemWithTitle:@"New Tab"
                      action:@selector(newTab:)
               keyEquivalent:@"t"];
  [fileMenu addItemWithTitle:@"New Window"
                      action:@selector(newWindow:)
               keyEquivalent:@"n"];
  [fileMenu addItemWithTitle:@"New Incognito Window"
                      action:@selector(newIncognitoWindow:)
               keyEquivalent:@"N"];
  [fileMenu addItem:[NSMenuItem separatorItem]];
  [fileMenu addItemWithTitle:@"Close Tab"
                      action:@selector(closeTab:)
               keyEquivalent:@"w"];
  [fileMenu addItemWithTitle:@"Close Window"
                      action:@selector(performClose:)
               keyEquivalent:@"W"];
  [fileMenu addItem:[NSMenuItem separatorItem]];
  [fileMenu addItemWithTitle:@"Save As…"
                      action:@selector(savePageAs:)
               keyEquivalent:@"s"];
  [fileMenu addItemWithTitle:@"Print…"
                      action:@selector(printPage:)
               keyEquivalent:@"p"];
  fileMenuItem.submenu = fileMenu;

  // Edit menu
  NSMenuItem *editMenuItem = [mainMenu addItemWithTitle:@"Edit" action:NULL keyEquivalent:@""];
  NSMenu *editMenu = [[NSMenu alloc] initWithTitle:@"Edit"];
  [editMenu addItemWithTitle:@"Undo" action:@selector(undo:) keyEquivalent:@"z"];
  [editMenu addItemWithTitle:@"Redo" action:@selector(redo:) keyEquivalent:@"Z"];
  [editMenu addItem:[NSMenuItem separatorItem]];
  [editMenu addItemWithTitle:@"Cut" action:@selector(cut:) keyEquivalent:@"x"];
  [editMenu addItemWithTitle:@"Copy" action:@selector(copy:) keyEquivalent:@"c"];
  [editMenu addItemWithTitle:@"Paste" action:@selector(paste:) keyEquivalent:@"v"];
  [editMenu addItemWithTitle:@"Select All" action:@selector(selectAll:) keyEquivalent:@"a"];
  [editMenu addItem:[NSMenuItem separatorItem]];
  [editMenu addItemWithTitle:@"Find…" action:@selector(showFind:) keyEquivalent:@"f"];
  editMenuItem.submenu = editMenu;

  // View menu
  NSMenuItem *viewMenuItem = [mainMenu addItemWithTitle:@"View" action:NULL keyEquivalent:@""];
  NSMenu *viewMenu = [[NSMenu alloc] initWithTitle:@"View"];
  [viewMenu addItemWithTitle:@"Reload"
                      action:@selector(reloadTab:)
               keyEquivalent:@"r"];
  [viewMenu addItemWithTitle:@"Force Reload"
                      action:@selector(forceReloadTab:)
               keyEquivalent:@"R"];
  [viewMenu addItem:[NSMenuItem separatorItem]];
  [viewMenu addItemWithTitle:@"Actual Size"
                      action:@selector(zoomActualSize:)
               keyEquivalent:@"0"];
  [viewMenu addItemWithTitle:@"Zoom In"
                      action:@selector(zoomIn:)
               keyEquivalent:@"+"];
  [viewMenu addItemWithTitle:@"Zoom Out"
                      action:@selector(zoomOut:)
               keyEquivalent:@"-"];
  [viewMenu addItem:[NSMenuItem separatorItem]];
  [viewMenu addItemWithTitle:@"Toggle Developer Tools"
                      action:@selector(toggleDevTools:)
               keyEquivalent:@"I"];
  viewMenuItem.submenu = viewMenu;

  // History menu
  NSMenuItem *historyMenuItem = [mainMenu addItemWithTitle:@"History" action:NULL keyEquivalent:@""];
  NSMenu *historyMenu = [[NSMenu alloc] initWithTitle:@"History"];
  [historyMenu addItemWithTitle:@"Back" action:@selector(goBack:) keyEquivalent:@"["];
  [historyMenu addItemWithTitle:@"Forward" action:@selector(goForward:) keyEquivalent:@"]"];
  historyMenuItem.submenu = historyMenu;

  // Window menu
  NSMenuItem *windowMenuItem = [mainMenu addItemWithTitle:@"Window" action:NULL keyEquivalent:@""];
  NSMenu *windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];
  [windowMenu addItemWithTitle:@"Minimize"
                        action:@selector(performMiniaturize:)
                 keyEquivalent:@"m"];
  [windowMenu addItemWithTitle:@"Zoom"
                        action:@selector(zoom:)
                 keyEquivalent:@""];
  windowMenuItem.submenu = windowMenu;
  NSApp.windowsMenu = windowMenu;

  // Help menu
  NSMenuItem *helpMenuItem = [mainMenu addItemWithTitle:@"Help" action:NULL keyEquivalent:@""];
  NSMenu *helpMenu = [[NSMenu alloc] initWithTitle:@"Help"];
  helpMenuItem.submenu = helpMenu;
  NSApp.helpMenu = helpMenu;
}

#pragma mark - Menu Actions

- (MainWindowController *)frontmostWindowController {
  // Prefer key window, then main window, then first in list
  for (MainWindowController *wc in self.windowControllers) {
    if (wc.window.isKeyWindow) return wc;
  }
  for (MainWindowController *wc in self.windowControllers) {
    if (wc.window.isMainWindow) return wc;
  }
  return self.mainWindowController;
}

- (IBAction)newTab:(id)sender {
  MainWindowController *wc = [self frontmostWindowController];
  if (wc) {
    [wc createNewTab:@"https://www.google.com" title:wc.isIncognito ? @"New Incognito Tab" : @"New Tab"];
  }
}

- (IBAction)closeTab:(id)sender {
  MainWindowController *wc = [self frontmostWindowController];
  if (wc) {
    [wc closeActiveTab];
  }
}

- (IBAction)newWindow:(id)sender {
  [self createMainWindow:NO];
  [NSApp activateIgnoringOtherApps:YES];
}

- (IBAction)newIncognitoWindow:(id)sender {
  [self createMainWindow:YES];
  [NSApp activateIgnoringOtherApps:YES];
}

- (IBAction)reloadTab:(id)sender {
  MainWindowController *wc = [self frontmostWindowController];
  if (wc) {
    [wc reloadActiveTab];
  }
}

- (IBAction)forceReloadTab:(id)sender {
  MainWindowController *wc = [self frontmostWindowController];
  if (wc) {
    [wc forceReloadActiveTab];
  }
}

- (IBAction)toggleDevTools:(id)sender {
  MainWindowController *wc = [self frontmostWindowController];
  if (wc) {
    [wc toggleDevTools];
  }
}

- (IBAction)goBack:(id)sender {
  MainWindowController *wc = [self frontmostWindowController];
  if (wc) {
    [wc goBack];
  }
}

- (IBAction)goForward:(id)sender {
  MainWindowController *wc = [self frontmostWindowController];
  if (wc) {
    [wc goForward];
  }
}

- (IBAction)showPreferences:(id)sender {
  MainWindowController *wc = [self frontmostWindowController];
  if (wc) {
    [wc showPreferences];
  }
}

- (IBAction)showFind:(id)sender {
  MainWindowController *wc = [self frontmostWindowController];
  if (wc) {
    [wc showFind];
  }
}

- (IBAction)savePageAs:(id)sender {
  MainWindowController *wc = [self frontmostWindowController];
  if (wc) {
    [wc savePageAs];
  }
}

- (IBAction)printPage:(id)sender {
  MainWindowController *wc = [self frontmostWindowController];
  if (wc) {
    [wc printPage];
  }
}

- (IBAction)zoomIn:(id)sender {
  MainWindowController *wc = [self frontmostWindowController];
  if (wc) {
    [wc zoomIn];
  }
}

- (IBAction)zoomOut:(id)sender {
  MainWindowController *wc = [self frontmostWindowController];
  if (wc) {
    [wc zoomOut];
  }
}

- (IBAction)zoomActualSize:(id)sender {
  MainWindowController *wc = [self frontmostWindowController];
  if (wc) {
    [wc zoomActualSize];
  }
}

@end
