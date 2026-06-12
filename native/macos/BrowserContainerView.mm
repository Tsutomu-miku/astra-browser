//
//  BrowserContainerView.mm
//  Astra Browser
//

#import "native/macos/BrowserContainerView.h"

@interface BrowserContainerView ()

@property (nonatomic, strong) NSMutableDictionary<NSString *, NSView *> *browserViews;  // tabId -> NSView
@property (nonatomic, copy) NSString *activeTabId;
@property (nonatomic, strong) NSVisualEffectView *backgroundView;

@end

@implementation BrowserContainerView

- (instancetype)init {
  return [self initWithFrame:NSZeroRect];
}

- (instancetype)initWithFrame:(NSRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    self.wantsLayer = YES;
    self.translatesAutoresizingMaskIntoConstraints = NO;
    self.browserViews = [NSMutableDictionary dictionary];

    // Background
    self.backgroundView = [[NSVisualEffectView alloc] initWithFrame:self.bounds];
    self.backgroundView.material = NSVisualEffectMaterialWindowBackground;
    self.backgroundView.blendingMode = NSVisualEffectBlendingModeBehindWindow;
    self.backgroundView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [self addSubview:self.backgroundView];

    // Host view for all browser views
    self.browserHostView = [[NSView alloc] initWithFrame:self.bounds];
    self.browserHostView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    self.browserHostView.wantsLayer = YES;
    [self addSubview:self.browserHostView];
  }
  return self;
}

- (void)showBrowserWithTabId:(const std::string &)tabId
                        view:(NSView *)browserView {
  NSString *key = [NSString stringWithUTF8String:tabId.c_str()];

  // Hide current browser view
  if (self.activeTabId && ![self.activeTabId isEqualToString:key]) {
    NSView *oldView = self.browserViews[self.activeTabId];
    oldView.hidden = YES;
  }

  self.activeTabId = key;

  // Check if we already have a view for this tab
  NSView *existingView = self.browserViews[key];
  if (existingView) {
    existingView.hidden = NO;
    existingView.frame = self.browserHostView.bounds;
    [self.browserHostView addSubview:existingView];
    return;
  }

  // Add the new browser view
  if (browserView) {
    browserView.frame = self.browserHostView.bounds;
    browserView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    browserView.hidden = NO;
    [self.browserHostView addSubview:browserView];
    self.browserViews[key] = browserView;
  }
}

- (void)hideAllBrowsers {
  for (NSView *view in self.browserViews.allValues) {
    view.hidden = YES;
  }
  self.activeTabId = nil;
}

- (void)removeBrowserForTabId:(const std::string &)tabId {
  NSString *key = [NSString stringWithUTF8String:tabId.c_str()];
  NSView *view = self.browserViews[key];
  if (view) {
    [view removeFromSuperview];
    [self.browserViews removeObjectForKey:key];
  }
  if ([self.activeTabId isEqualToString:key]) {
    self.activeTabId = nil;
  }
}

- (BOOL)hasBrowserForTabId:(const std::string &)tabId {
  NSString *key = [NSString stringWithUTF8String:tabId.c_str()];
  return self.browserViews[key] != nil;
}

- (void)resizeWithOldSuperviewSize:(NSSize)oldSize {
  [super resizeWithOldSuperviewSize:oldSize];
  // Browser views auto-resize via autoresizingMask
}

@end
