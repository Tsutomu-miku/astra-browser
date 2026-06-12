//
//  BrowserContainerView.h
//  Astra Browser
//
//  Container view that hosts native browser views.
//  Manages showing/hiding browser views as tabs are switched.
//  Pure AppKit — no CEF dependencies.

#import <Cocoa/Cocoa.h>

#include <string>

@interface BrowserContainerView : NSView

// The host view that all browser views are parented to
@property (nonatomic, strong) NSView *browserHostView;

// Shows the native browser view for the given tab ID
- (void)showBrowserWithTabId:(const std::string &)tabId
                        view:(NSView *)browserView;

// Hides all browser views
- (void)hideAllBrowsers;

// Removes a browser view (when tab is closed)
- (void)removeBrowserForTabId:(const std::string &)tabId;

// Returns YES if we have a view for this tab
- (BOOL)hasBrowserForTabId:(const std::string &)tabId;

@end
