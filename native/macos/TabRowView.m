//
//  TabRowView.m
//  Astra Browser
//

#import "native/macos/TabRowView.h"

@implementation TabRowView

- (instancetype)initWithFrame:(NSRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    [self setupSubviews];
  }
  return self;
}

- (void)setupSubviews {
  // Favicon
  self.faviconView = [[NSImageView alloc] initWithFrame:NSMakeRect(12, 8, 16, 16)];
  self.faviconView.image = [NSImage imageWithSystemSymbolName:@"globe" accessibilityDescription:nil];
  self.faviconView.symbolConfiguration = [NSImageSymbolConfiguration configurationWithScale:NSImageSymbolScaleSmall];
  self.faviconView.imageScaling = NSImageScaleProportionallyUpOrDown;
  [self addSubview:self.faviconView];

  // Loading indicator
  self.loadingIndicator = [[NSProgressIndicator alloc] initWithFrame:NSMakeRect(12, 8, 16, 16)];
  self.loadingIndicator.style = NSProgressIndicatorStyleSpinning;
  self.loadingIndicator.controlSize = NSControlSizeSmall;
  self.loadingIndicator.hidden = YES;
  [self addSubview:self.loadingIndicator];

  // Title
  self.titleLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(36, 5, 160, 22)];
  self.titleLabel.font = [NSFont systemFontOfSize:13];
  self.titleLabel.textColor = [NSColor labelColor];
  self.titleLabel.backgroundColor = [NSColor clearColor];
  self.titleLabel.bezeled = NO;
  self.titleLabel.drawsBackground = NO;
  self.titleLabel.editable = NO;
  self.titleLabel.selectable = NO;
  self.titleLabel.lineBreakMode = NSLineBreakByTruncatingTail;
  [self addSubview:self.titleLabel];

  // Status icon (right side, for muted/pinned/favorite indicators)
  self.statusIcon = [[NSImageView alloc] initWithFrame:NSMakeRect(208, 8, 16, 16)];
  self.statusIcon.symbolConfiguration = [NSImageSymbolConfiguration configurationWithScale:NSImageSymbolScaleSmall];
  self.statusIcon.imageScaling = NSImageScaleProportionallyUpOrDown;
  self.statusIcon.hidden = YES;
  [self addSubview:self.statusIcon];

  // Close button
  self.closeButton = [NSButton buttonWithImage:[NSImage imageWithSystemSymbolName:@"xmark" accessibilityDescription:nil]
                                         target:self
                                         action:@selector(closeClicked:)];
  self.closeButton.frame = NSMakeRect(230, 4, 24, 24);
  self.closeButton.bezelStyle = NSBezelStyleRoundRect;
  self.closeButton.showsBorderOnlyWhileMouseInside = YES;
  self.closeButton.toolTip = @"Close Tab (⌘W)";
  [self addSubview:self.closeButton];

  _showsCloseButton = YES;
}

- (void)setTitle:(NSString *)title {
  _title = [title copy];
  self.titleLabel.stringValue = title;
}

- (void)setIsLoading:(BOOL)isLoading {
  _isLoading = isLoading;
  self.faviconView.hidden = isLoading;
  self.loadingIndicator.hidden = !isLoading;
  if (isLoading) {
    [self.loadingIndicator startAnimation:nil];
  } else {
    [self.loadingIndicator stopAnimation:nil];
  }
}

- (void)setIsActive:(BOOL)isActive {
  _isActive = isActive;
}

- (void)setIsPinned:(BOOL)isPinned {
  _isPinned = isPinned;
  [self updateStatusIcon];
}

- (void)setIsFavorite:(BOOL)isFavorite {
  _isFavorite = isFavorite;
  [self updateStatusIcon];
}

- (void)setIsMuted:(BOOL)isMuted {
  _isMuted = isMuted;
  [self updateStatusIcon];
}

- (void)setShowsCloseButton:(BOOL)showsCloseButton {
  _showsCloseButton = showsCloseButton;
  self.closeButton.hidden = !showsCloseButton;
}

- (void)updateStatusIcon {
  // Priority: muted > favorite > pinned
  if (self.isMuted) {
    self.statusIcon.hidden = NO;
    self.statusIcon.image = [NSImage imageWithSystemSymbolName:@"speaker.slash.fill" accessibilityDescription:nil];
    self.statusIcon.contentTintColor = [NSColor secondaryLabelColor];
  } else if (self.isFavorite) {
    self.statusIcon.hidden = NO;
    self.statusIcon.image = [NSImage imageWithSystemSymbolName:@"star.fill" accessibilityDescription:nil];
    self.statusIcon.contentTintColor = [NSColor systemYellowColor];
  } else if (self.isPinned) {
    self.statusIcon.hidden = NO;
    self.statusIcon.image = [NSImage imageWithSystemSymbolName:@"pin.fill" accessibilityDescription:nil];
    self.statusIcon.contentTintColor = [NSColor systemBlueColor];
  } else {
    self.statusIcon.hidden = YES;
  }
}

- (void)closeClicked:(id)sender {
  if (self.closeAction) {
    self.closeAction();
  }
}

- (void)setBackgroundStyle:(NSBackgroundStyle)backgroundStyle {
  [super setBackgroundStyle:backgroundStyle];
  if (backgroundStyle == NSBackgroundStyleEmphasized) {
    self.titleLabel.textColor = [NSColor whiteColor];
    self.faviconView.contentTintColor = [NSColor whiteColor];
  } else {
    self.titleLabel.textColor = [NSColor labelColor];
    self.faviconView.contentTintColor = [NSColor secondaryLabelColor];
  }
}

@end
