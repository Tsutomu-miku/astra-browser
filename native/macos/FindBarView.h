//
//  FindBarView.h
//  Astra Browser
//
//  Find-in-page toolbar with next/prev/count/close controls.
//

#import <Cocoa/Cocoa.h>

@protocol FindBarViewDelegate <NSObject>
- (void)findBarDidChangeText:(NSString *)text;
- (void)findBarDidFindNext;
- (void)findBarDidFindPrevious;
- (void)findBarDidClose;
@end

@interface FindBarView : NSView <NSTextFieldDelegate>

@property (nonatomic, weak) id<FindBarViewDelegate> delegate;
@property (nonatomic, strong) NSTextField *searchField;
@property (nonatomic, strong) NSTextField *countLabel;
@property (nonatomic, strong) NSButton *prevButton;
@property (nonatomic, strong) NSButton *nextButton;
@property (nonatomic, strong) NSButton *closeButton;

- (void)updateMatchCount:(NSInteger)current total:(NSInteger)total;
- (void)showNoMatch;
- (void)focusSearchField;

@end
