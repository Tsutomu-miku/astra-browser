//
//  SidebarViewController.mm
//  Astra Browser
//
//  Multi-section sidebar: Pinned, Favorites, Tab Groups, Tabs, Recently Closed.
//

#import "native/macos/SidebarViewController.h"
#import "native/macos/TabRowView.h"

#include "browser/core/Browser.h"

typedef NS_ENUM(NSInteger, SidebarRowType) {
  SidebarRowTypeSectionHeader,
  SidebarRowTypeGroupHeader,
  SidebarRowTypeTab,
  SidebarRowTypeRecentlyClosed,
  SidebarRowTypeHistoryDayHeader,
  SidebarRowTypeHistoryEntry,
  SidebarRowTypeDownload,
};

@interface SidebarItem : NSObject
@property (nonatomic, assign) SidebarRowType type;
@property (nonatomic, assign) SidebarSection section;
@property (nonatomic, copy) NSString *title;
@property (nonatomic, copy) NSString *tabId;
@property (nonatomic, copy) NSString *groupId;
@property (nonatomic, copy) NSString *url;  // for history entries
@property (nonatomic, assign) NSInteger itemCount;
@property (nonatomic, assign) BOOL collapsed;
@property (nonatomic, copy) NSString *groupColor;
@end

@implementation SidebarItem
@end

@interface SidebarViewController ()

@property (nonatomic, strong) NSScrollView *scrollView;
@property (nonatomic, strong) NSTableView *tableView;
@property (nonatomic, strong) NSVisualEffectView *backgroundView;
@property (nonatomic, strong) NSTextField *workspaceLabel;
@property (nonatomic, strong) NSButton *addTabButton;
@property (nonatomic, strong) NSColor *workspaceAccentColor;
@property (nonatomic, strong) NSMutableArray<SidebarItem *> *sidebarItems;
@property (nonatomic, copy) NSString *contextMenuTabId;
@property (nonatomic, assign) BOOL contextMenuIsRecentlyClosed;
@property (nonatomic, strong) NSMutableDictionary<NSNumber *, NSNumber *> *sectionCollapsedState;

@end

@implementation SidebarViewController

- (void)loadView {
  self.view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 260, 600)];
  self.view.wantsLayer = YES;
  self.sidebarItems = [NSMutableArray array];
  self.sectionCollapsedState = [NSMutableDictionary dictionary];
}

- (void)viewDidLoad {
  [super viewDidLoad];
  [self setupBackground];
  [self setupWorkspaceHeader];
  [self setupTableView];
  [self reloadTabData];
}

- (void)setupBackground {
  self.backgroundView = [[NSVisualEffectView alloc] initWithFrame:self.view.bounds];
  self.backgroundView.material = NSVisualEffectMaterialSidebar;
  self.backgroundView.blendingMode = NSVisualEffectBlendingModeBehindWindow;
  self.backgroundView.state = NSVisualEffectStateActive;
  self.backgroundView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
  [self.view addSubview:self.backgroundView];
}

- (void)setupWorkspaceHeader {
  NSView *headerView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 260, 50)];
  headerView.wantsLayer = YES;
  headerView.autoresizingMask = NSViewWidthSizable;

  NSString *workspaceName = self.incognitoMode ? @"Incognito" : @"Personal";
  NSString *accentHex = self.incognitoMode ? @"#9333EA" : @"#5B8FF9";

  // Accent color indicator (left bar)
  NSView *accentBar = [[NSView alloc] initWithFrame:NSMakeRect(0, 14, 3, 22)];
  accentBar.wantsLayer = YES;
  accentBar.layer.cornerRadius = 1.5;
  accentBar.layer.backgroundColor = [NSColor colorWithRed:0.36 green:0.56 blue:0.98 alpha:1.0].CGColor;
  [headerView addSubview:accentBar];

  self.workspaceAccentColor = [NSColor colorWithRed:0.36 green:0.56 blue:0.98 alpha:1.0];
  if (accentHex.length == 7) {
    unsigned int rgb = 0;
    NSScanner *scanner = [NSScanner scannerWithString:[accentHex substringFromIndex:1]];
    [scanner scanHexInt:&rgb];
    CGFloat r = ((rgb >> 16) & 0xFF) / 255.0;
    CGFloat g = ((rgb >> 8) & 0xFF) / 255.0;
    CGFloat b = (rgb & 0xFF) / 255.0;
    self.workspaceAccentColor = [NSColor colorWithRed:r green:g blue:b alpha:1.0];
    accentBar.layer.backgroundColor = self.workspaceAccentColor.CGColor;
  }

  self.workspaceLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(16, 14, 180, 22)];
  self.workspaceLabel.stringValue = workspaceName;
  self.workspaceLabel.font = [NSFont systemFontOfSize:13 weight:NSFontWeightSemibold];
  self.workspaceLabel.textColor = [NSColor labelColor];
  self.workspaceLabel.bezeled = NO;
  self.workspaceLabel.drawsBackground = NO;
  self.workspaceLabel.editable = NO;
  self.workspaceLabel.selectable = NO;
  [headerView addSubview:self.workspaceLabel];

  self.addTabButton = [NSButton buttonWithImage:[NSImage imageWithSystemSymbolName:@"plus" accessibilityDescription:nil]
                                         target:self
                                         action:@selector(newTabClicked:)];
  self.addTabButton.frame = NSMakeRect(228, 12, 28, 28);
  self.addTabButton.bezelStyle = NSBezelStyleRoundRect;
  self.addTabButton.toolTip = self.incognitoMode ? @"New Incognito Tab (⌘T)" : @"New Tab (⌘T)";
  [headerView addSubview:self.addTabButton];

  [self.view addSubview:headerView];
}

- (void)setupTableView {
  CGFloat headerHeight = 50.0;
  NSRect scrollFrame = NSMakeRect(0, headerHeight,
                                   self.view.bounds.size.width,
                                   self.view.bounds.size.height - headerHeight);

  self.scrollView = [[NSScrollView alloc] initWithFrame:scrollFrame];
  self.scrollView.hasVerticalScroller = YES;
  self.scrollView.drawsBackground = NO;
  self.scrollView.backgroundColor = [NSColor clearColor];
  self.scrollView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
  self.scrollView.verticalScrollElasticity = NSScrollElasticityAllowed;

  self.tableView = [[NSTableView alloc] initWithFrame:scrollFrame];
  self.tableView.delegate = self;
  self.tableView.dataSource = self;
  self.tableView.rowHeight = 32;
  self.tableView.intercellSpacing = NSMakeSize(0, 2);
  self.tableView.backgroundColor = [NSColor clearColor];
  self.tableView.headerView = nil;
  self.tableView.selectionHighlightStyle = NSTableViewSelectionHighlightStyleRegular;
  self.tableView.usesAlternatingRowBackgroundColors = NO;
  self.tableView.allowsEmptySelection = YES;
  self.tableView.target = self;
  self.tableView.doubleAction = @selector(doubleClickedRow:);
  self.tableView.focusRingType = NSFocusRingTypeNone;
  self.tableView.allowsMultipleSelection = NO;

  // Drag and drop support
  [self.tableView registerForDraggedTypes:@[@"AstraTabID"]];

  NSTableColumn *column = [[NSTableColumn alloc] initWithIdentifier:@"tab"];
  column.width = 240;
  [self.tableView addTableColumn:column];

  self.scrollView.documentView = self.tableView;
  [self.view addSubview:self.scrollView];
}

#pragma mark - Data building

- (void)rebuildSidebarItems {
  [self.sidebarItems removeAllObjects];
  if (!_browser) return;

  const auto& tabs = _browser->tabs();

  // Count pinned vs regular tabs
  NSInteger pinnedCount = 0;
  NSInteger regularCount = 0;
  for (auto* tab : tabs) {
    if (tab->is_pinned()) {
      pinnedCount++;
    } else {
      regularCount++;
    }
  }

  // Pinned section
  {
    BOOL collapsed = [self.sectionCollapsedState[@(SidebarSectionPinned)] boolValue];
    SidebarItem *header = [SidebarItem new];
    header.type = SidebarRowTypeSectionHeader;
    header.section = SidebarSectionPinned;
    header.title = @"Pinned";
    header.itemCount = pinnedCount;
    header.collapsed = collapsed;
    [self.sidebarItems addObject:header];

    if (!collapsed) {
      for (auto* tab : tabs) {
        if (!tab->is_pinned()) continue;
        SidebarItem *item = [SidebarItem new];
        item.type = SidebarRowTypeTab;
        item.section = SidebarSectionPinned;
        item.tabId = [NSString stringWithUTF8String:tab->id().c_str()];
        [self.sidebarItems addObject:item];
      }
    }
  }

  // Favorites section (placeholder — not implemented in core browser yet)
  {
    BOOL collapsed = [self.sectionCollapsedState[@(SidebarSectionFavorites)] boolValue] ?: YES;
    SidebarItem *header = [SidebarItem new];
    header.type = SidebarRowTypeSectionHeader;
    header.section = SidebarSectionFavorites;
    header.title = @"Favorites";
    header.itemCount = 0;
    header.collapsed = collapsed;
    [self.sidebarItems addObject:header];
  }

  // Tab groups section (placeholder — not implemented in core browser yet)
  {
    BOOL collapsed = [self.sectionCollapsedState[@(SidebarSectionGroups)] boolValue] ?: YES;
    SidebarItem *header = [SidebarItem new];
    header.type = SidebarRowTypeSectionHeader;
    header.section = SidebarSectionGroups;
    header.title = @"Tab Groups";
    header.itemCount = 0;
    header.collapsed = collapsed;
    [self.sidebarItems addObject:header];
  }

  // Tabs section (regular tabs, not pinned)
  {
    BOOL collapsed = [self.sectionCollapsedState[@(SidebarSectionTabs)] boolValue];
    SidebarItem *header = [SidebarItem new];
    header.type = SidebarRowTypeSectionHeader;
    header.section = SidebarSectionTabs;
    header.title = @"Tabs";
    header.itemCount = regularCount;
    header.collapsed = collapsed;
    [self.sidebarItems addObject:header];

    if (!collapsed) {
      for (auto* tab : tabs) {
        if (tab->is_pinned()) continue;
        SidebarItem *item = [SidebarItem new];
        item.type = SidebarRowTypeTab;
        item.section = SidebarSectionTabs;
        item.tabId = [NSString stringWithUTF8String:tab->id().c_str()];
        [self.sidebarItems addObject:item];
      }
    }
  }

  // Downloads section (placeholder — not implemented in core browser yet)
  if (!self.incognitoMode) {
    BOOL collapsed = [self.sectionCollapsedState[@(SidebarSectionDownloads)] boolValue] ?: YES;
    SidebarItem *header = [SidebarItem new];
    header.type = SidebarRowTypeSectionHeader;
    header.section = SidebarSectionDownloads;
    header.title = @"Downloads";
    header.itemCount = 0;
    header.collapsed = collapsed;
    [self.sidebarItems addObject:header];
  }

  // Recently closed section (placeholder — not implemented in core browser yet)
  {
    BOOL collapsed = [self.sectionCollapsedState[@(SidebarSectionRecentlyClosed)] boolValue] ?: YES;
    SidebarItem *header = [SidebarItem new];
    header.type = SidebarRowTypeSectionHeader;
    header.section = SidebarSectionRecentlyClosed;
    header.title = @"Recently Closed";
    header.itemCount = 0;
    header.collapsed = collapsed;
    [self.sidebarItems addObject:header];
  }

  // History section (placeholder — not implemented in core browser yet)
  if (!self.incognitoMode) {
    BOOL collapsed = [self.sectionCollapsedState[@(SidebarSectionHistory)] boolValue] ?: YES;
    SidebarItem *header = [SidebarItem new];
    header.type = SidebarRowTypeSectionHeader;
    header.section = SidebarSectionHistory;
    header.title = @"History";
    header.itemCount = 0;
    header.collapsed = collapsed;
    [self.sidebarItems addObject:header];
  }
}

- (void)reloadTabData {
  [self rebuildSidebarItems];
  [self.tableView reloadData];

  // Select the active tab row (use window's activeTabId, not global)
  NSString *activeTabId = self.activeTabId;
  if (activeTabId.length > 0) {
    for (NSInteger row = 0; row < (NSInteger)self.sidebarItems.count; row++) {
      SidebarItem *item = self.sidebarItems[row];
      if (item.type == SidebarRowTypeTab && [item.tabId isEqualToString:activeTabId]) {
        [self.tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:row]
                byExtendingSelection:NO];
        break;
      }
    }
  }
}

- (void)reloadRecentlyClosed {
  [self reloadTabData];
}

- (void)reloadHistory {
  [self reloadTabData];
}

- (void)reloadDownloads {
  [self reloadTabData];
}

#pragma mark - Actions

- (void)newTabClicked:(id)sender {
  if ([self.delegate respondsToSelector:@selector(sidebarDidRequestNewTab)]) {
    [self.delegate sidebarDidRequestNewTab];
  }
}

- (void)doubleClickedRow:(id)sender {
  NSInteger clickedRow = [self.tableView clickedRow];
  if (clickedRow == -1) {
    if ([self.delegate respondsToSelector:@selector(sidebarDidRequestNewTab)]) {
      [self.delegate sidebarDidRequestNewTab];
    }
  }
}

- (void)sectionHeaderClicked:(NSButton *)sender {
  NSInteger row = sender.tag;
  if (row < 0 || row >= (NSInteger)self.sidebarItems.count) return;

  SidebarItem *item = self.sidebarItems[row];
  if (item.type != SidebarRowTypeSectionHeader && item.type != SidebarRowTypeGroupHeader) return;

  if (item.type == SidebarRowTypeSectionHeader) {
    // Toggle collapse state
    BOOL current = [self.sectionCollapsedState[@(item.section)] boolValue];
    self.sectionCollapsedState[@(item.section)] = @(!current);

    if ([self.delegate respondsToSelector:@selector(sidebarDidToggleSection:)]) {
      [self.delegate sidebarDidToggleSection:item.section];
    }
  }
  // Group headers are not implemented in core browser yet

  [self reloadTabData];
}

- (void)closeTabClicked:(NSButton *)sender {
  NSInteger row = sender.tag;
  if (row < 0 || row >= (NSInteger)self.sidebarItems.count) return;

  SidebarItem *item = self.sidebarItems[row];
  if (item.type != SidebarRowTypeTab) return;

  std::string tabId = [item.tabId UTF8String];
  if ([self.delegate respondsToSelector:@selector(sidebarDidRequestCloseTab:)]) {
    [self.delegate sidebarDidRequestCloseTab:tabId];
  }
}

- (void)cancelDownloadClicked:(NSButton *)sender {
  NSInteger row = sender.tag;
  if (row < 0 || row >= (NSInteger)self.sidebarItems.count) return;

  SidebarItem *item = self.sidebarItems[row];
  if (item.type != SidebarRowTypeDownload) return;

  // Download cancellation not yet implemented in core browser layer
}

#pragma mark - NSTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
  return self.sidebarItems.count;
}

- (nullable NSView *)tableView:(NSTableView *)tableView
            viewForTableColumn:(nullable NSTableColumn *)tableColumn
                           row:(NSInteger)row {
  SidebarItem *item = self.sidebarItems[row];

  if (item.type == SidebarRowTypeSectionHeader || item.type == SidebarRowTypeGroupHeader) {
    return [self makeSectionHeaderView:item row:row];
  } else if (item.type == SidebarRowTypeHistoryDayHeader) {
    return [self makeHistoryDayHeaderView:item row:row];
  } else if (item.type == SidebarRowTypeTab) {
    return [self makeTabRowView:item row:row];
  } else if (item.type == SidebarRowTypeRecentlyClosed) {
    return [self makeRecentlyClosedRowView:item row:row];
  } else if (item.type == SidebarRowTypeHistoryEntry) {
    return [self makeHistoryEntryView:item row:row];
  } else if (item.type == SidebarRowTypeDownload) {
    return [self makeDownloadRowView:item row:row];
  }

  return nil;
}

- (NSTableCellView *)makeSectionHeaderView:(SidebarItem *)item row:(NSInteger)row {
  static NSString *headerIdentifier = @"SectionHeader";

  NSTableCellView *cellView = [self.tableView makeViewWithIdentifier:headerIdentifier owner:self];
  if (!cellView) {
    cellView = [[NSTableCellView alloc] initWithFrame:NSMakeRect(0, 0, 240, 28)];
    cellView.identifier = headerIdentifier;

    // Disclosure button
    NSButton *disclosureButton = [[NSButton alloc] initWithFrame:NSMakeRect(8, 4, 20, 20)];
    disclosureButton.bezelStyle = NSBezelStyleRoundRect;
    disclosureButton.showsBorderOnlyWhileMouseInside = YES;
    disclosureButton.bordered = YES;
    disclosureButton.image = [NSImage imageWithSystemSymbolName:@"chevron.right" accessibilityDescription:nil];
    disclosureButton.target = self;
    disclosureButton.action = @selector(sectionHeaderClicked:);
    [cellView addSubview:disclosureButton];

    // Title label
    NSTextField *titleLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(32, 3, 140, 22)];
    titleLabel.font = [NSFont systemFontOfSize:11 weight:NSFontWeightSemibold];
    titleLabel.textColor = [NSColor secondaryLabelColor];
    titleLabel.bezeled = NO;
    titleLabel.drawsBackground = NO;
    titleLabel.editable = NO;
    titleLabel.selectable = NO;
    titleLabel.tag = 100;
    [cellView addSubview:titleLabel];

    // Count label
    NSTextField *countLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(200, 3, 40, 22)];
    countLabel.font = [NSFont systemFontOfSize:11];
    countLabel.textColor = [NSColor tertiaryLabelColor];
    countLabel.alignment = NSTextAlignmentRight;
    countLabel.bezeled = NO;
    countLabel.drawsBackground = NO;
    countLabel.editable = NO;
    countLabel.selectable = NO;
    countLabel.tag = 101;
    [cellView addSubview:countLabel];
  }

  // Update content
  NSTextField *titleLabel = [cellView viewWithTag:100];
  titleLabel.stringValue = item.title;

  NSTextField *countLabel = [cellView viewWithTag:101];
  countLabel.stringValue = [NSString stringWithFormat:@"%ld", (long)item.itemCount];

  // Find disclosure button (first subview)
  NSButton *disclosureButton = nil;
  for (NSView *subview in cellView.subviews) {
    if ([subview isKindOfClass:[NSButton class]]) {
      disclosureButton = (NSButton *)subview;
      break;
    }
  }
  disclosureButton.tag = row;
  if (item.collapsed) {
    disclosureButton.image = [NSImage imageWithSystemSymbolName:@"chevron.right" accessibilityDescription:nil];
  } else {
    disclosureButton.image = [NSImage imageWithSystemSymbolName:@"chevron.down" accessibilityDescription:nil];
  }

  // Group header: adjust styling + add color dot
  NSView *colorDot = nil;
  for (NSView *subview in cellView.subviews) {
    if ([subview.identifier isEqualToString:@"ColorDot"]) {
      colorDot = subview;
      break;
    }
  }
  if (item.type == SidebarRowTypeGroupHeader) {
    titleLabel.textColor = [NSColor labelColor];
    if (!colorDot) {
      colorDot = [[NSView alloc] initWithFrame:NSMakeRect(34, 8, 12, 12)];
      colorDot.wantsLayer = YES;
      colorDot.layer.cornerRadius = 6.0;
      colorDot.identifier = @"ColorDot";
      [cellView addSubview:colorDot];
      // Adjust title position for group headers
      titleLabel.frame = NSMakeRect(52, 3, 120, 22);
    }
    if (item.groupColor.length == 7) {
      unsigned int rgb = 0;
      NSScanner *scanner = [NSScanner scannerWithString:[item.groupColor substringFromIndex:1]];
      [scanner scanHexInt:&rgb];
      CGFloat r = ((rgb >> 16) & 0xFF) / 255.0;
      CGFloat g = ((rgb >> 8) & 0xFF) / 255.0;
      CGFloat b = (rgb & 0xFF) / 255.0;
      colorDot.layer.backgroundColor = [NSColor colorWithRed:r green:g blue:b alpha:1.0].CGColor;
    }
    colorDot.hidden = NO;
  } else {
    colorDot.hidden = YES;
    titleLabel.textColor = [NSColor secondaryLabelColor];
    titleLabel.frame = NSMakeRect(32, 3, 140, 22);
  }

  return cellView;
}

- (TabRowView *)makeTabRowView:(SidebarItem *)item row:(NSInteger)row {
  static NSString *tabIdentifier = @"TabRow";
  TabRowView *cellView = (TabRowView *)[self.tableView makeViewWithIdentifier:tabIdentifier owner:self];

  if (!cellView) {
    cellView = [[TabRowView alloc] initWithFrame:NSMakeRect(0, 0, 240, 32)];
    cellView.identifier = tabIdentifier;
  }

  std::string tabId = [item.tabId UTF8String];
  auto* tab = _browser ? _browser->GetTab(tabId) : nullptr;
  if (tab) {
    cellView.title = [NSString stringWithUTF8String:tab->title().c_str()];
    cellView.toolTip = [NSString stringWithUTF8String:tab->url().c_str()];
    cellView.isLoading = tab->navigation_state().is_loading;
    cellView.isPinned = tab->is_pinned();
    cellView.isFavorite = NO;  // Not implemented in core browser yet
    cellView.isMuted = tab->is_muted();
  }

  cellView.showsCloseButton = YES;

  __weak typeof(self) weakSelf = self;
  cellView.closeAction = ^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    if (strongSelf && strongSelf.delegate &&
        [strongSelf.delegate respondsToSelector:@selector(sidebarDidRequestCloseTab:)]) {
      [strongSelf.delegate sidebarDidRequestCloseTab:tabId];
    }
  };

  return cellView;
}

- (TabRowView *)makeRecentlyClosedRowView:(SidebarItem *)item row:(NSInteger)row {
  static NSString *rcIdentifier = @"RecentlyClosedRow";
  TabRowView *cellView = (TabRowView *)[self.tableView makeViewWithIdentifier:rcIdentifier owner:self];

  if (!cellView) {
    cellView = [[TabRowView alloc] initWithFrame:NSMakeRect(0, 0, 240, 32)];
    cellView.identifier = rcIdentifier;
    cellView.showsCloseButton = NO;
  }

  cellView.title = item.title;
  cellView.toolTip = item.url;
  cellView.isLoading = NO;
  cellView.isPinned = NO;
  cellView.isFavorite = NO;
  cellView.isMuted = NO;

  // Clock icon for recently closed items
  cellView.faviconView.image = [NSImage imageWithSystemSymbolName:@"clock.arrow.circlepath" accessibilityDescription:nil];

  return cellView;
}

- (NSTableCellView *)makeHistoryDayHeaderView:(SidebarItem *)item row:(NSInteger)row {
  static NSString *dayHeaderIdentifier = @"HistoryDayHeader";

  NSTableCellView *cellView = [self.tableView makeViewWithIdentifier:dayHeaderIdentifier owner:self];
  if (!cellView) {
    cellView = [[NSTableCellView alloc] initWithFrame:NSMakeRect(0, 0, 240, 22)];
    cellView.identifier = dayHeaderIdentifier;

    NSTextField *titleLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(28, 0, 200, 22)];
    titleLabel.font = [NSFont systemFontOfSize:10 weight:NSFontWeightMedium];
    titleLabel.textColor = [NSColor tertiaryLabelColor];
    titleLabel.bezeled = NO;
    titleLabel.drawsBackground = NO;
    titleLabel.editable = NO;
    titleLabel.selectable = NO;
    titleLabel.tag = 100;
    [cellView addSubview:titleLabel];
  }

  NSTextField *titleLabel = [cellView viewWithTag:100];
  titleLabel.stringValue = item.title;

  return cellView;
}

- (TabRowView *)makeHistoryEntryView:(SidebarItem *)item row:(NSInteger)row {
  static NSString *historyIdentifier = @"HistoryEntryRow";
  TabRowView *cellView = (TabRowView *)[self.tableView makeViewWithIdentifier:historyIdentifier owner:self];

  if (!cellView) {
    cellView = [[TabRowView alloc] initWithFrame:NSMakeRect(0, 0, 240, 32)];
    cellView.identifier = historyIdentifier;
    cellView.showsCloseButton = NO;
  }

  cellView.title = item.title;
  cellView.toolTip = item.url;
  cellView.isLoading = NO;
  cellView.isPinned = NO;
  cellView.isFavorite = NO;
  cellView.isMuted = NO;

  // History icon
  cellView.faviconView.image = [NSImage imageWithSystemSymbolName:@"clock" accessibilityDescription:nil];

  return cellView;
}

- (NSTableCellView *)makeDownloadRowView:(SidebarItem *)item row:(NSInteger)row {
  static NSString *downloadIdentifier = @"DownloadRow";

  NSTableCellView *cellView = [self.tableView makeViewWithIdentifier:downloadIdentifier owner:self];
  if (!cellView) {
    cellView = [[NSTableCellView alloc] initWithFrame:NSMakeRect(0, 0, 240, 36)];
    cellView.identifier = downloadIdentifier;

    // Icon
    NSImageView *iconView = [[NSImageView alloc] initWithFrame:NSMakeRect(12, 10, 16, 16)];
    iconView.image = [NSImage imageWithSystemSymbolName:@"arrow.down.circle" accessibilityDescription:nil];
    iconView.symbolConfiguration = [NSImageSymbolConfiguration configurationWithScale:NSImageSymbolScaleSmall];
    iconView.imageScaling = NSImageScaleProportionallyUpOrDown;
    iconView.tag = 100;
    [cellView addSubview:iconView];

    // Filename label
    NSTextField *nameLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(36, 10, 160, 18)];
    nameLabel.font = [NSFont systemFontOfSize:12];
    nameLabel.textColor = [NSColor labelColor];
    nameLabel.backgroundColor = [NSColor clearColor];
    nameLabel.bezeled = NO;
    nameLabel.drawsBackground = NO;
    nameLabel.editable = NO;
    nameLabel.selectable = NO;
    nameLabel.lineBreakMode = NSLineBreakByTruncatingTail;
    nameLabel.tag = 101;
    [cellView addSubview:nameLabel];

    // Status label
    NSTextField *statusLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(36, 26, 160, 12)];
    statusLabel.font = [NSFont systemFontOfSize:10];
    statusLabel.textColor = [NSColor secondaryLabelColor];
    statusLabel.backgroundColor = [NSColor clearColor];
    statusLabel.bezeled = NO;
    statusLabel.drawsBackground = NO;
    statusLabel.editable = NO;
    statusLabel.selectable = NO;
    statusLabel.lineBreakMode = NSLineBreakByTruncatingTail;
    statusLabel.tag = 102;
    [cellView addSubview:statusLabel];

    // Cancel button
    NSButton *cancelButton = [NSButton buttonWithImage:[NSImage imageWithSystemSymbolName:@"xmark" accessibilityDescription:nil]
                                                target:self
                                                action:@selector(cancelDownloadClicked:)];
    cancelButton.frame = NSMakeRect(208, 6, 24, 24);
    cancelButton.bezelStyle = NSBezelStyleRoundRect;
    cancelButton.showsBorderOnlyWhileMouseInside = YES;
    cancelButton.toolTip = @"Cancel";
    cancelButton.tag = 103;
    [cellView addSubview:cancelButton];
  }

  // Update content — placeholder, downloads not yet in core browser
  NSTextField *nameLabel = [cellView viewWithTag:101];
  nameLabel.stringValue = item.title;

  NSTextField *statusLabel = [cellView viewWithTag:102];
  statusLabel.stringValue = @"";

  NSButton *cancelButton = [cellView viewWithTag:103];
  cancelButton.hidden = YES;
  cancelButton.tag = row;

  return cellView;
}

#pragma mark - NSTableViewDelegate

- (CGFloat)tableView:(NSTableView *)tableView heightOfRow:(NSInteger)row {
  SidebarItem *item = self.sidebarItems[row];
  if (item.type == SidebarRowTypeSectionHeader || item.type == SidebarRowTypeGroupHeader) {
    return 28.0;
  }
  if (item.type == SidebarRowTypeHistoryDayHeader) {
    return 22.0;
  }
  if (item.type == SidebarRowTypeDownload) {
    return 36.0;
  }
  return 32.0;
}

- (void)tableViewSelectionDidChange:(NSNotification *)notification {
  NSInteger selectedRow = self.tableView.selectedRow;
  if (selectedRow < 0 || selectedRow >= (NSInteger)self.sidebarItems.count) return;

  SidebarItem *item = self.sidebarItems[selectedRow];

  if (item.type == SidebarRowTypeTab) {
    std::string tabId = [item.tabId UTF8String];
    if (self.delegate && [self.delegate respondsToSelector:@selector(sidebarDidSelectTab:)]) {
      [self.delegate sidebarDidSelectTab:tabId];
    }
  } else if (item.type == SidebarRowTypeRecentlyClosed) {
    std::string tabId = [item.tabId UTF8String];
    if (self.delegate && [self.delegate respondsToSelector:@selector(sidebarDidRequestRestoreRecentlyClosed:)]) {
      [self.delegate sidebarDidRequestRestoreRecentlyClosed:tabId];
    }
  } else if (item.type == SidebarRowTypeHistoryEntry) {
    // Navigate active tab to history URL
    if (_browser && item.url.length > 0) {
      auto* activeTab = _browser->GetActiveTab();
      if (activeTab) {
        std::string url = [item.url UTF8String];
        activeTab->Navigate(url);
      }
    }
  }
}

- (BOOL)tableView:(NSTableView *)tableView shouldSelectRow:(NSInteger)row {
  SidebarItem *item = self.sidebarItems[row];
  return item.type == SidebarRowTypeTab ||
         item.type == SidebarRowTypeRecentlyClosed ||
         item.type == SidebarRowTypeHistoryEntry ||
         item.type == SidebarRowTypeDownload;
}

- (nullable NSTableRowView *)tableView:(NSTableView *)tableView rowViewForRow:(NSInteger)row {
  static NSString *rowIdentifier = @"SidebarRowView";
  NSTableRowView *rowView = [tableView makeViewWithIdentifier:rowIdentifier owner:nil];
  if (!rowView) {
    rowView = [[NSTableRowView alloc] initWithFrame:NSZeroRect];
    rowView.identifier = rowIdentifier;
  }
  return rowView;
}

#pragma mark - Context Menu

- (nullable NSMenu *)tableView:(NSTableView *)tableView
         menuForTableColumn:(nullable NSTableColumn *)tableColumn
                       row:(NSInteger)row {
  if (row < 0 || row >= (NSInteger)self.sidebarItems.count) return nil;

  SidebarItem *item = self.sidebarItems[row];

  // Section header context menus
  if (item.type == SidebarRowTypeSectionHeader) {
    if (item.section == SidebarSectionHistory) {
      NSMenu *menu = [[NSMenu alloc] init];
      NSMenuItem *clearItem = [menu addItemWithTitle:@"Clear History"
                                                action:@selector(clearHistoryFromContextMenu:)
                                         keyEquivalent:@""];
      clearItem.target = self;
      return menu;
    }
    if (item.section == SidebarSectionDownloads) {
      NSMenu *menu = [[NSMenu alloc] init];
      NSMenuItem *clearItem = [menu addItemWithTitle:@"Clear Completed"
                                                action:@selector(clearCompletedDownloadsFromContextMenu:)
                                         keyEquivalent:@""];
      clearItem.target = self;
      return menu;
    }
    return nil;
  }

  if (item.type == SidebarRowTypeDownload) {
    _contextMenuTabId = item.tabId;
    _contextMenuIsRecentlyClosed = NO;

    // Download context menu — placeholder, not yet in core browser
    NSMenu *menu = [[NSMenu alloc] init];
    return menu;
  }

  if (item.type != SidebarRowTypeTab && item.type != SidebarRowTypeRecentlyClosed) return nil;

  // Store the context menu tab ID for action handlers
  _contextMenuTabId = item.tabId;
  _contextMenuIsRecentlyClosed = (item.type == SidebarRowTypeRecentlyClosed);

  NSMenu *menu = [[NSMenu alloc] init];

  if (item.type == SidebarRowTypeRecentlyClosed) {
    // Recently closed items have a simpler menu
    NSMenuItem *restoreItem = [menu addItemWithTitle:@"Restore"
                                               action:@selector(restoreFromContextMenu:)
                                        keyEquivalent:@""];
    restoreItem.target = self;

    [menu addItem:[NSMenuItem separatorItem]];

    NSMenuItem *deleteItem = [menu addItemWithTitle:@"Delete"
                                              action:@selector(deleteFromRecentlyClosed:)
                                       keyEquivalent:@""];
    deleteItem.target = self;
    deleteItem.enabled = NO; // Not yet implemented

    return menu;
  }

  // Regular tab context menu
  std::string tabId = [item.tabId UTF8String];
  auto* tab = _browser ? _browser->GetTab(tabId) : nullptr;
  if (!tab) return nil;

  // New Tab
  NSMenuItem *newTabItem = [menu addItemWithTitle:@"New Tab"
                                            action:@selector(newTabFromContextMenu:)
                                     keyEquivalent:@"t"];
  newTabItem.target = self;

  // Duplicate Tab
  NSMenuItem *duplicateItem = [menu addItemWithTitle:@"Duplicate Tab"
                                              action:@selector(duplicateFromContextMenu:)
                                       keyEquivalent:@""];
  duplicateItem.target = self;

  [menu addItem:[NSMenuItem separatorItem]];

  // Close Tab
  NSMenuItem *closeItem = [menu addItemWithTitle:@"Close Tab"
                                          action:@selector(closeFromContextMenu:)
                                   keyEquivalent:@"w"];
  closeItem.target = self;

  [menu addItem:[NSMenuItem separatorItem]];

  // Pin / Unpin
  NSString *pinTitle = tab->is_pinned() ? @"Unpin Tab" : @"Pin Tab";
  NSMenuItem *pinItem = [menu addItemWithTitle:pinTitle
                                        action:@selector(togglePinFromContextMenu:)
                                 keyEquivalent:@""];
  pinItem.target = self;

  // Add to Favorites / Remove from Favorites
  // Favorites not yet implemented in core browser
  NSString *favTitle = @"Add to Favorites";
  NSMenuItem *favItem = [menu addItemWithTitle:favTitle
                                        action:@selector(toggleFavoriteFromContextMenu:)
                                 keyEquivalent:@""];
  favItem.target = self;

  [menu addItem:[NSMenuItem separatorItem]];

  // Reload
  NSMenuItem *reloadItem = [menu addItemWithTitle:@"Reload"
                                           action:@selector(reloadFromContextMenu:)
                                    keyEquivalent:@"r"];
  reloadItem.target = self;

  // Force Reload
  NSMenuItem *forceReloadItem = [menu addItemWithTitle:@"Force Reload"
                                                 action:@selector(forceReloadFromContextMenu:)
                                          keyEquivalent:@"r"];
  forceReloadItem.keyEquivalentModifierMask = NSEventModifierFlagShift | NSEventModifierFlagCommand;
  forceReloadItem.target = self;

  [menu addItem:[NSMenuItem separatorItem]];

  // Mute / Unmute
  NSString *muteTitle = tab->is_muted() ? @"Unmute Tab" : @"Mute Tab";
  NSMenuItem *muteItem = [menu addItemWithTitle:muteTitle
                                         action:@selector(toggleMuteFromContextMenu:)
                                  keyEquivalent:@""];
  muteItem.target = self;

  [menu addItem:[NSMenuItem separatorItem]];

  // Copy URL
  NSMenuItem *copyUrlItem = [menu addItemWithTitle:@"Copy URL"
                                            action:@selector(copyUrlFromContextMenu:)
                                     keyEquivalent:@""];
  copyUrlItem.target = self;

  return menu;
}

#pragma mark - Context Menu Actions

- (void)newTabFromContextMenu:(id)sender {
  if ([self.delegate respondsToSelector:@selector(sidebarDidRequestNewTab)]) {
    [self.delegate sidebarDidRequestNewTab];
  }
}

- (void)duplicateFromContextMenu:(id)sender {
  if (!_contextMenuTabId) return;
  std::string tabId = [_contextMenuTabId UTF8String];
  if ([self.delegate respondsToSelector:@selector(sidebarDidRequestDuplicateTab:)]) {
    [self.delegate sidebarDidRequestDuplicateTab:tabId];
  }
}

- (void)closeFromContextMenu:(id)sender {
  if (!_contextMenuTabId) return;
  std::string tabId = [_contextMenuTabId UTF8String];
  if ([self.delegate respondsToSelector:@selector(sidebarDidRequestCloseTab:)]) {
    [self.delegate sidebarDidRequestCloseTab:tabId];
  }
}

- (void)togglePinFromContextMenu:(id)sender {
  if (!_contextMenuTabId) return;
  std::string tabId = [_contextMenuTabId UTF8String];
  auto* tab = _browser ? _browser->GetTab(tabId) : nullptr;
  if (!tab) return;
  if (tab->is_pinned()) {
    if ([self.delegate respondsToSelector:@selector(sidebarDidRequestUnpinTab:)]) {
      [self.delegate sidebarDidRequestUnpinTab:tabId];
    }
  } else {
    if ([self.delegate respondsToSelector:@selector(sidebarDidRequestPinTab:)]) {
      [self.delegate sidebarDidRequestPinTab:tabId];
    }
  }
}

- (void)toggleFavoriteFromContextMenu:(id)sender {
  if (!_contextMenuTabId) return;
  std::string tabId = [_contextMenuTabId UTF8String];
  // Favorites not yet implemented in core browser — always treat as not favorited
  if ([self.delegate respondsToSelector:@selector(sidebarDidRequestFavoriteTab:)]) {
    [self.delegate sidebarDidRequestFavoriteTab:tabId];
  }
}

- (void)reloadFromContextMenu:(id)sender {
  if (!_contextMenuTabId) return;
  std::string tabId = [_contextMenuTabId UTF8String];
  if ([self.delegate respondsToSelector:@selector(sidebarDidRequestReloadTab:)]) {
    [self.delegate sidebarDidRequestReloadTab:tabId];
  }
}

- (void)forceReloadFromContextMenu:(id)sender {
  if (!_contextMenuTabId) return;
  std::string tabId = [_contextMenuTabId UTF8String];
  // Force reload not yet in WebContents API — fall back to normal reload via delegate
  if ([self.delegate respondsToSelector:@selector(sidebarDidRequestReloadTab:)]) {
    [self.delegate sidebarDidRequestReloadTab:tabId];
  }
}

- (void)toggleMuteFromContextMenu:(id)sender {
  if (!_contextMenuTabId) return;
  std::string tabId = [_contextMenuTabId UTF8String];
  auto* tab = _browser ? _browser->GetTab(tabId) : nullptr;
  if (!tab) return;
  if ([self.delegate respondsToSelector:@selector(sidebarDidRequestMuteTab:muted:)]) {
    [self.delegate sidebarDidRequestMuteTab:tabId muted:!tab->is_muted()];
  }
}

- (void)copyUrlFromContextMenu:(id)sender {
  if (!_contextMenuTabId) return;
  std::string tabId = [_contextMenuTabId UTF8String];
  auto* tab = _browser ? _browser->GetTab(tabId) : nullptr;
  if (!tab) return;
  NSString *url = [NSString stringWithUTF8String:tab->url().c_str()];
  NSPasteboard *pb = [NSPasteboard generalPasteboard];
  [pb clearContents];
  [pb setString:url forType:NSPasteboardTypeString];
}

- (void)restoreFromContextMenu:(id)sender {
  if (!_contextMenuTabId) return;
  std::string tabId = [_contextMenuTabId UTF8String];
  if ([self.delegate respondsToSelector:@selector(sidebarDidRequestRestoreRecentlyClosed:)]) {
    [self.delegate sidebarDidRequestRestoreRecentlyClosed:tabId];
  }
}

- (void)deleteFromRecentlyClosed:(id)sender {
  // Not yet implemented
}

- (void)clearHistoryFromContextMenu:(id)sender {
  // Clear history not yet implemented in core browser
}

- (void)cancelDownloadFromContextMenu:(id)sender {
  if (!_contextMenuTabId) return;
  // Cancel download not yet implemented in core browser
}

- (void)openDownloadFromContextMenu:(id)sender {
  if (!_contextMenuTabId) return;
  // Open download not yet implemented in core browser
}

- (void)showDownloadInFinderFromContextMenu:(id)sender {
  if (!_contextMenuTabId) return;
  // Show download in Finder not yet implemented in core browser
}

- (void)clearCompletedDownloadsFromContextMenu:(id)sender {
  // Clear completed downloads not yet implemented in core browser
}

#pragma mark - Drag and Drop

- (BOOL)tableView:(NSTableView *)tableView writeRowsWithIndexes:(NSIndexSet *)rowIndexes
     toPasteboard:(NSPasteboard *)pasteboard {
  if (rowIndexes.count != 1) return NO;

  NSInteger row = rowIndexes.firstIndex;
  if (row < 0 || row >= (NSInteger)self.sidebarItems.count) return NO;

  SidebarItem *item = self.sidebarItems[row];
  if (item.type != SidebarRowTypeTab) return NO;
  if (item.section == SidebarSectionRecentlyClosed) return NO;

  [pasteboard setString:item.tabId forType:@"AstraTabID"];
  return YES;
}

- (NSDragOperation)tableView:(NSTableView *)tableView validateDrop:(id <NSDraggingInfo>)info
                 proposedRow:(NSInteger)row
       proposedDropOperation:(NSTableViewDropOperation)dropOperation {
  NSString *tabIdStr = [info.draggingPasteboard stringForType:@"AstraTabID"];
  if (!tabIdStr || tabIdStr.length == 0) return NSDragOperationNone;

  std::string tabId = [tabIdStr UTF8String];
  auto* tab = _browser ? _browser->GetTab(tabId) : nullptr;
  if (!tab) return NSDragOperationNone;

  // Row -1 means drop below all rows
  if (row < 0 || row >= (NSInteger)self.sidebarItems.count) {
    return NSDragOperationNone;
  }

  SidebarItem *targetItem = self.sidebarItems[row];

  // Drop ON a section header
  if (dropOperation == NSTableViewDropOn && targetItem.type == SidebarRowTypeSectionHeader) {
    switch (targetItem.section) {
      case SidebarSectionPinned:
        return tab->is_pinned() ? NSDragOperationNone : NSDragOperationMove;
      case SidebarSectionTabs:
        // Can "make regular" if tab is pinned
        return tab->is_pinned() ? NSDragOperationMove : NSDragOperationNone;
      case SidebarSectionFavorites:
      case SidebarSectionGroups:
      case SidebarSectionRecentlyClosed:
      case SidebarSectionHistory:
      case SidebarSectionDownloads:
      default:
        return NSDragOperationNone;
    }
  }

  // Drop ABOVE a tab row in Tabs section → reorder
  if (dropOperation == NSTableViewDropAbove &&
      targetItem.type == SidebarRowTypeTab &&
      targetItem.section == SidebarSectionTabs) {
    // Only allow reorder if source tab is also a regular (non-pinned) tab
    if (!tab->is_pinned()) {
      return NSDragOperationMove;
    }
    return NSDragOperationNone;
  }

  return NSDragOperationNone;
}

- (BOOL)tableView:(NSTableView *)tableView acceptDrop:(id <NSDraggingInfo>)info
               row:(NSInteger)row
     dropOperation:(NSTableViewDropOperation)dropOperation {
  NSString *tabIdStr = [info.draggingPasteboard stringForType:@"AstraTabID"];
  if (!tabIdStr || tabIdStr.length == 0) return NO;

  std::string tabId = [tabIdStr UTF8String];
  auto* tab = _browser ? _browser->GetTab(tabId) : nullptr;
  if (!tab) return NO;

  if (row < 0 || row >= (NSInteger)self.sidebarItems.count) return NO;

  SidebarItem *targetItem = self.sidebarItems[row];

  // Drop ON section header
  if (dropOperation == NSTableViewDropOn && targetItem.type == SidebarRowTypeSectionHeader) {
    switch (targetItem.section) {
      case SidebarSectionPinned:
        if ([self.delegate respondsToSelector:@selector(sidebarDidRequestPinTab:)]) {
          [self.delegate sidebarDidRequestPinTab:tabId];
        }
        return YES;
      case SidebarSectionTabs:
        // Make regular: unpin
        if (tab->is_pinned()) {
          if ([self.delegate respondsToSelector:@selector(sidebarDidRequestUnpinTab:)]) {
            [self.delegate sidebarDidRequestUnpinTab:tabId];
          }
        }
        return YES;
      default:
        return NO;
    }
  }

  // Drop ABOVE a tab row in Tabs section → reorder
  if (dropOperation == NSTableViewDropAbove &&
      targetItem.type == SidebarRowTypeTab &&
      targetItem.section == SidebarSectionTabs) {
    std::string targetTabId = [targetItem.tabId UTF8String];

    if (!_browser) return NO;
    const auto& tabs = _browser->tabs();

    // Find indices in full tabs array (only consider non-pinned tabs)
    size_t fromIndex = 0, toIndex = 0;
    BOOL foundFrom = NO, foundTo = NO;
    size_t regularIndex = 0;
    for (size_t i = 0; i < tabs.size(); i++) {
      if (tabs[i]->is_pinned()) continue;
      if (tabs[i]->id() == tabId) {
        fromIndex = regularIndex;
        foundFrom = YES;
      }
      if (tabs[i]->id() == targetTabId) {
        toIndex = regularIndex;
        foundTo = YES;
      }
      regularIndex++;
    }

    if (!foundFrom || !foundTo) return NO;
    if (fromIndex == toIndex) return NO;

    // Use MoveTab on the browser (operates on full tab list)
    // We need to map back to full indices
    size_t fullFromIndex = 0, fullToIndex = 0;
    BOOL foundFullFrom = NO, foundFullTo = NO;
    for (size_t i = 0; i < tabs.size(); i++) {
      if (tabs[i]->id() == tabId) {
        fullFromIndex = i;
        foundFullFrom = YES;
      }
      if (tabs[i]->id() == targetTabId) {
        fullToIndex = i;
        foundFullTo = YES;
      }
    }
    if (!foundFullFrom || !foundFullTo) return NO;

    _browser->MoveTab(fullFromIndex, fullToIndex);
    return YES;
  }

  return NO;
}

@end
