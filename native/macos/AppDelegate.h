//
//  AppDelegate.h
//  Astra Browser
//

#import <Cocoa/Cocoa.h>

@class MainWindowController;

@interface AppDelegate : NSObject <NSApplicationDelegate>

@property (nonatomic, strong) MainWindowController *mainWindowController;
@property (nonatomic, strong) NSMutableArray<MainWindowController *> *windowControllers;

+ (instancetype)sharedDelegate;

@end
