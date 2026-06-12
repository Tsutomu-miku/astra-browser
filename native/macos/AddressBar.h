//
//  AddressBar.h
//  Astra Browser
//
//  URL / search address bar in the top bar.
//

#import <Cocoa/Cocoa.h>

@protocol AddressBarDelegate <NSObject>
- (void)addressBarDidCommitNavigation:(NSString *)url;
@end

@interface AddressBar : NSView <NSTextFieldDelegate>

@property (nonatomic, weak) id<AddressBarDelegate> delegate;
@property (nonatomic, copy) NSString *urlString;
@property (nonatomic, strong) NSTextField *urlField;
@property (nonatomic, assign) BOOL isLoading;
@property (nonatomic, assign) BOOL incognitoMode;

- (void)startLoading;
- (void)stopLoading;
- (void)updateSecurityState:(BOOL)isSecure;

@end
