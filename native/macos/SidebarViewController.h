//
//  SidebarViewController.h
//  Astra Browser
//
//  Sidebar with workspace selector + tab list.
//

#import <Cocoa/Cocoa.h>

#include <string>

#include "browser/core/Browser.h"

typedef NS_ENUM(NSInteger, SidebarSection) {
  SidebarSectionPinned = 0,
  SidebarSectionFavorites,
  SidebarSectionGroups,  // Tab groups
  SidebarSectionTabs,    // Regular tabs (not in groups)
  SidebarSectionRecentlyClosed,
  SidebarSectionHistory,
  SidebarSectionDownloads,
  SidebarSectionCount
};

@protocol SidebarViewControllerDelegate <NSObject>
- (void)sidebarDidSelectTab:(const std::string&)tabId;
- (void)sidebarDidRequestNewTab;
- (void)sidebarDidRequestCloseTab:(const std::string&)tabId;
- (void)sidebarDidRequestPinTab:(const std::string&)tabId;
- (void)sidebarDidRequestUnpinTab:(const std::string&)tabId;
- (void)sidebarDidRequestFavoriteTab:(const std::string&)tabId;
- (void)sidebarDidRequestUnfavoriteTab:(const std::string&)tabId;
- (void)sidebarDidRequestDuplicateTab:(const std::string&)tabId;
- (void)sidebarDidRequestReloadTab:(const std::string&)tabId;
- (void)sidebarDidRequestMuteTab:(const std::string&)tabId muted:(BOOL)muted;
- (void)sidebarDidRequestRestoreRecentlyClosed:(const std::string&)tabId;
- (void)sidebarDidToggleSection:(SidebarSection)section;
@end

@interface SidebarViewController : NSViewController <NSTableViewDelegate, NSTableViewDataSource>

@property (nonatomic, weak) id<SidebarViewControllerDelegate> delegate;
@property (nonatomic, assign) BOOL incognitoMode;
@property (nonatomic, copy) NSString *activeTabId;

// Browser data source (weak reference)
@property (nonatomic, assign) astra::Browser* browser;

- (void)reloadTabData;
- (void)reloadRecentlyClosed;
- (void)reloadHistory;
- (void)reloadDownloads;

@end
