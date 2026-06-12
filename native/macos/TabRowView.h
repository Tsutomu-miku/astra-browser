//
//  TabRowView.h
//  Astra Browser
//
//  Custom table cell view for sidebar tab rows.
//

#import <Cocoa/Cocoa.h>

@interface TabRowView : NSTableCellView

@property (nonatomic, strong) NSImageView *faviconView;
@property (nonatomic, strong) NSTextField *titleLabel;
@property (nonatomic, strong) NSButton *closeButton;
@property (nonatomic, strong) NSProgressIndicator *loadingIndicator;
@property (nonatomic, strong) NSImageView *statusIcon;  // muted, pinned, etc.

@property (nonatomic, copy) NSString *title;
@property (nonatomic, copy) NSString *url;
@property (nonatomic, assign) BOOL isLoading;
@property (nonatomic, assign) BOOL isActive;
@property (nonatomic, assign) BOOL isPinned;
@property (nonatomic, assign) BOOL isFavorite;
@property (nonatomic, assign) BOOL isMuted;

@property (nonatomic, assign) BOOL showsCloseButton;

@property (nonatomic, copy) void (^closeAction)(void);

@end
