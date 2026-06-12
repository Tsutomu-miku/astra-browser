//
//  AddressBar.mm
//  Astra Browser
//

#import "native/macos/AddressBar.h"

@interface AddressBar ()

@property (nonatomic, strong) NSVisualEffectView *backgroundView;
@property (nonatomic, strong) NSImageView *lockIcon;
@property (nonatomic, strong) NSProgressIndicator *loadingIndicator;

@end

@implementation AddressBar

@synthesize delegate = _delegate;
@synthesize urlString = _urlString;
@synthesize urlField = _urlField;
@synthesize backgroundView = _backgroundView;
@synthesize lockIcon = _lockIcon;
@synthesize loadingIndicator = _loadingIndicator;
@synthesize isLoading = _isLoading;
@synthesize incognitoMode = _incognitoMode;

- (instancetype)init {
  return [self initWithFrame:NSZeroRect];
}

- (instancetype)initWithFrame:(NSRect)frameRect {
  self = [super initWithFrame:frameRect];
  if (self) {
    self.wantsLayer = YES;
    self.translatesAutoresizingMaskIntoConstraints = NO;
    self.layer.cornerRadius = 8.0;
    self.layer.borderWidth = 1.0;
    self.layer.borderColor = [[NSColor separatorColor] CGColor];

    [self setupSubviews];
  }
  return self;
}

- (void)setupSubviews {
  // Loading indicator (shown while page is loading)
  self.loadingIndicator = [[NSProgressIndicator alloc] initWithFrame:NSMakeRect(10, 7, 16, 16)];
  self.loadingIndicator.style = NSProgressIndicatorStyleSpinning;
  self.loadingIndicator.controlSize = NSControlSizeSmall;
  self.loadingIndicator.hidden = YES;
  [self addSubview:self.loadingIndicator];

  // Lock icon (secure indicator — shown when not loading)
  self.lockIcon = [[NSImageView alloc] initWithFrame:NSMakeRect(10, 7, 16, 16)];
  self.lockIcon.image = [NSImage imageWithSystemSymbolName:@"lock" accessibilityDescription:nil];
  self.lockIcon.symbolConfiguration = [NSImageSymbolConfiguration configurationWithScale:NSImageSymbolScaleSmall];
  self.lockIcon.imageScaling = NSImageScaleProportionallyUpOrDown;
  self.lockIcon.hidden = YES;
  [self addSubview:self.lockIcon];

  // URL text field
  self.urlField = [[NSTextField alloc] initWithFrame:NSMakeRect(34, 3, 0, 24)];
  self.urlField.font = [NSFont systemFontOfSize:13];
  self.urlField.textColor = [NSColor labelColor];
  self.urlField.backgroundColor = [NSColor clearColor];
  self.urlField.bezeled = NO;
  self.urlField.drawsBackground = NO;
  self.urlField.delegate = self;
  self.urlField.lineBreakMode = NSLineBreakByTruncatingMiddle;
  self.urlField.usesSingleLineMode = YES;
  self.urlField.translatesAutoresizingMaskIntoConstraints = NO;
  self.urlField.maximumNumberOfLines = 1;
  self.urlField.placeholderString = @"Search or enter website";
  [self addSubview:self.urlField];

  // Layout
  [NSLayoutConstraint activateConstraints:@[
    [self.urlField.leadingAnchor constraintEqualToAnchor:self.lockIcon.trailingAnchor constant:8],
    [self.urlField.trailingAnchor constraintEqualToAnchor:self.trailingAnchor constant:-12],
    [self.urlField.centerYAnchor constraintEqualToAnchor:self.centerYAnchor],
    [self.urlField.heightAnchor constraintEqualToConstant:24],
  ]];
}

- (void)setUrlString:(NSString *)urlString {
  _urlString = [urlString copy];
  self.urlField.stringValue = urlString;

  // Update security indicator
  [self updateSecurityIndicatorForURL:urlString];
}

- (void)startLoading {
  self.lockIcon.hidden = YES;
  self.loadingIndicator.hidden = NO;
  [self.loadingIndicator startAnimation:nil];
}

- (void)stopLoading {
  [self.loadingIndicator stopAnimation:nil];
  self.loadingIndicator.hidden = YES;
  [self updateSecurityIndicatorForURL:_urlString];
}

- (void)setIsLoading:(BOOL)isLoading {
  _isLoading = isLoading;
  if (isLoading) {
    [self startLoading];
  } else {
    [self stopLoading];
  }
}

- (void)setIncognitoMode:(BOOL)incognitoMode {
  _incognitoMode = incognitoMode;
  if (incognitoMode) {
    self.layer.borderColor = [[NSColor colorWithRed:0.58 green:0.20 blue:0.92 alpha:0.6] CGColor];
    self.urlField.placeholderString = @"Search or enter website (Incognito)";
  } else {
    self.layer.borderColor = [[NSColor separatorColor] CGColor];
    self.urlField.placeholderString = @"Search or enter website";
  }
}

- (void)updateSecurityState:(BOOL)isSecure {
  self.lockIcon.hidden = NO;
  if (isSecure) {
    self.lockIcon.image = [NSImage imageWithSystemSymbolName:@"lock" accessibilityDescription:nil];
    self.lockIcon.contentTintColor = [NSColor systemGreenColor];
  } else {
    self.lockIcon.image = [NSImage imageWithSystemSymbolName:@"lock.open" accessibilityDescription:nil];
    self.lockIcon.contentTintColor = [NSColor secondaryLabelColor];
  }
}

- (void)updateSecurityIndicatorForURL:(NSString *)url {
  if (self.loadingIndicator.hidden == NO) return;  // Loading takes precedence

  NSString *lowered = url.lowercaseString;
  if ([lowered hasPrefix:@"https://"]) {
    [self updateSecurityState:YES];
  } else if ([lowered hasPrefix:@"http://"]) {
    [self updateSecurityState:NO];
  } else {
    self.lockIcon.hidden = YES;
  }
}

#pragma mark - NSTextFieldDelegate

- (void)controlTextDidEndEditing:(NSNotification *)obj {
  NSString *text = self.urlField.stringValue;
  if (text.length == 0) return;

  // Determine if it's a URL or a search query
  NSString *navUrl = text;
  if (![text containsString:@"."] && ![text containsString:@"/"]) {
    // Treat as search query
    navUrl = [NSString stringWithFormat:@"https://www.google.com/search?q=%@",
              [text stringByAddingPercentEncodingWithAllowedCharacters:
                      [NSCharacterSet URLQueryAllowedCharacterSet]]];
  } else if (![text.lowercaseString hasPrefix:@"http://"] &&
             ![text.lowercaseString hasPrefix:@"https://"]) {
    navUrl = [@"https://" stringByAppendingString:text];
  }

  if ([self.delegate respondsToSelector:@selector(addressBarDidCommitNavigation:)]) {
    [self.delegate addressBarDidCommitNavigation:navUrl];
  }
}

- (BOOL)control:(NSControl *)control textView:(NSTextView *)textView doCommandBySelector:(SEL)commandSelector {
  if (commandSelector == @selector(insertNewline:)) {
    // Enter key — commit navigation
    [self.urlField.window makeFirstResponder:nil];
    return YES;
  }
  return NO;
}

@end
