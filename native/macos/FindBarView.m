//
//  FindBarView.m
//  Astra Browser
//

#import "native/macos/FindBarView.h"

@implementation FindBarView

- (instancetype)init {
  return [self initWithFrame:NSZeroRect];
}

- (instancetype)initWithFrame:(NSRect)frameRect {
  self = [super initWithFrame:frameRect];
  if (self) {
    self.wantsLayer = YES;
    self.translatesAutoresizingMaskIntoConstraints = NO;
    self.layer.backgroundColor = [[NSColor windowBackgroundColor] CGColor];
    self.layer.borderWidth = 1.0;
    self.layer.borderColor = [[NSColor separatorColor] CGColor];
    self.layer.cornerRadius = 8.0;

    [self setupSubviews];
  }
  return self;
}

- (void)setupSubviews {
  // Close button (left side)
  self.closeButton = [NSButton buttonWithImage:[NSImage imageWithSystemSymbolName:@"xmark" accessibilityDescription:nil]
                                        target:self
                                        action:@selector(closeClicked:)];
  self.closeButton.bezelStyle = NSBezelStyleRoundRect;
  self.closeButton.translatesAutoresizingMaskIntoConstraints = NO;
  self.closeButton.toolTip = @"Close Find (Esc)";
  [self addSubview:self.closeButton];

  // Search text field
  self.searchField = [[NSTextField alloc] init];
  self.searchField.translatesAutoresizingMaskIntoConstraints = NO;
  self.searchField.delegate = self;
  self.searchField.font = [NSFont systemFontOfSize:13];
  self.searchField.placeholderString = @"Find in page";
  self.searchField.bezeled = NO;
  self.searchField.drawsBackground = NO;
  self.searchField.focusRingType = NSFocusRingTypeNone;
  [self addSubview:self.searchField];

  // Count label
  self.countLabel = [[NSTextField alloc] init];
  self.countLabel.translatesAutoresizingMaskIntoConstraints = NO;
  self.countLabel.font = [NSFont systemFontOfSize:12];
  self.countLabel.textColor = [NSColor secondaryLabelColor];
  self.countLabel.alignment = NSTextAlignmentRight;
  self.countLabel.bezeled = NO;
  self.countLabel.drawsBackground = NO;
  self.countLabel.editable = NO;
  self.countLabel.selectable = NO;
  self.countLabel.stringValue = @"";
  [self addSubview:self.countLabel];

  // Previous button
  self.prevButton = [NSButton buttonWithImage:[NSImage imageWithSystemSymbolName:@"chevron.up" accessibilityDescription:nil]
                                       target:self
                                       action:@selector(prevClicked:)];
  self.prevButton.bezelStyle = NSBezelStyleRoundRect;
  self.prevButton.translatesAutoresizingMaskIntoConstraints = NO;
  self.prevButton.toolTip = @"Previous (Shift+Enter)";
  self.prevButton.enabled = NO;
  [self addSubview:self.prevButton];

  // Next button
  self.nextButton = [NSButton buttonWithImage:[NSImage imageWithSystemSymbolName:@"chevron.down" accessibilityDescription:nil]
                                       target:self
                                       action:@selector(nextClicked:)];
  self.nextButton.bezelStyle = NSBezelStyleRoundRect;
  self.nextButton.translatesAutoresizingMaskIntoConstraints = NO;
  self.nextButton.toolTip = @"Next (Enter)";
  self.nextButton.enabled = NO;
  [self addSubview:self.nextButton];

  [NSLayoutConstraint activateConstraints:@[
    // Close button
    [self.closeButton.leadingAnchor constraintEqualToAnchor:self.leadingAnchor constant:8],
    [self.closeButton.centerYAnchor constraintEqualToAnchor:self.centerYAnchor],
    [self.closeButton.widthAnchor constraintEqualToConstant:24],
    [self.closeButton.heightAnchor constraintEqualToConstant:24],

    // Search field
    [self.searchField.leadingAnchor constraintEqualToAnchor:self.closeButton.trailingAnchor constant:4],
    [self.searchField.centerYAnchor constraintEqualToAnchor:self.centerYAnchor],
    [self.searchField.widthAnchor constraintGreaterThanOrEqualToConstant:150],

    // Count label
    [self.countLabel.leadingAnchor constraintGreaterThanOrEqualToAnchor:self.searchField.trailingAnchor constant:8],
    [self.countLabel.centerYAnchor constraintEqualToAnchor:self.centerYAnchor],
    [self.countLabel.widthAnchor constraintEqualToConstant:80],

    // Prev button
    [self.prevButton.leadingAnchor constraintEqualToAnchor:self.countLabel.trailingAnchor constant:4],
    [self.prevButton.centerYAnchor constraintEqualToAnchor:self.centerYAnchor],
    [self.prevButton.widthAnchor constraintEqualToConstant:24],
    [self.prevButton.heightAnchor constraintEqualToConstant:24],

    // Next button
    [self.nextButton.leadingAnchor constraintEqualToAnchor:self.prevButton.trailingAnchor constant:2],
    [self.nextButton.trailingAnchor constraintEqualToAnchor:self.trailingAnchor constant:-8],
    [self.nextButton.centerYAnchor constraintEqualToAnchor:self.centerYAnchor],
    [self.nextButton.widthAnchor constraintEqualToConstant:24],
    [self.nextButton.heightAnchor constraintEqualToConstant:24],
  ]];
}

- (void)updateMatchCount:(NSInteger)current total:(NSInteger)total {
  if (total == 0) {
    [self showNoMatch];
    return;
  }
  self.countLabel.textColor = [NSColor secondaryLabelColor];
  self.countLabel.stringValue = [NSString stringWithFormat:@"%ld of %ld", (long)current, (long)total];
  self.prevButton.enabled = YES;
  self.nextButton.enabled = YES;
}

- (void)showNoMatch {
  self.countLabel.textColor = [NSColor systemRedColor];
  self.countLabel.stringValue = @"No match";
  self.prevButton.enabled = NO;
  self.nextButton.enabled = NO;
}

- (void)focusSearchField {
  [self.window makeFirstResponder:self.searchField];
  [self.searchField selectText:nil];
}

- (void)closeClicked:(id)sender {
  if ([self.delegate respondsToSelector:@selector(findBarDidClose)]) {
    [self.delegate findBarDidClose];
  }
}

- (void)prevClicked:(id)sender {
  if ([self.delegate respondsToSelector:@selector(findBarDidFindPrevious)]) {
    [self.delegate findBarDidFindPrevious];
  }
}

- (void)nextClicked:(id)sender {
  if ([self.delegate respondsToSelector:@selector(findBarDidFindNext)]) {
    [self.delegate findBarDidFindNext];
  }
}

#pragma mark - NSTextFieldDelegate

- (void)controlTextDidChange:(NSNotification *)obj {
  NSString *text = self.searchField.stringValue;
  if ([self.delegate respondsToSelector:@selector(findBarDidChangeText:)]) {
    [self.delegate findBarDidChangeText:text];
  }
}

- (BOOL)control:(NSControl *)control textView:(NSTextView *)textView doCommandBySelector:(SEL)commandSelector {
  if (commandSelector == @selector(insertNewline:)) {
    // Enter = find next, Shift+Enter = find previous
    NSEvent *event = [NSApp currentEvent];
    if (event.modifierFlags & NSEventModifierFlagShift) {
      if ([self.delegate respondsToSelector:@selector(findBarDidFindPrevious)]) {
        [self.delegate findBarDidFindPrevious];
      }
    } else {
      if ([self.delegate respondsToSelector:@selector(findBarDidFindNext)]) {
        [self.delegate findBarDidFindNext];
      }
    }
    return YES;
  }
  if (commandSelector == @selector(cancelOperation:)) {
    // Escape = close find bar
    if ([self.delegate respondsToSelector:@selector(findBarDidClose)]) {
      [self.delegate findBarDidClose];
    }
    return YES;
  }
  return NO;
}

@end
