#import <CoreFoundation/CoreFoundation.h>
#import <MetalKit/MetalKit.h>
#import <UIKit/UIKit.h>

#include "gme.h"

@interface POCViewDelegate : NSObject<MTKViewDelegate>
@property (nonatomic) BOOL ready;
@end
@implementation POCViewDelegate
- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
}
- (void)drawInMTKView:(MTKView *)view {
}
@end

@interface POCAppDelegate : NSObject<UIApplicationDelegate>
@property(nonatomic, strong) UIWindow * window;
@end
@implementation POCAppDelegate
- (void)swipeLeft   { gme_left  (); }
- (void)swipeRight  { gme_right (); }
- (void)swipeTop    { gme_up    (); }
- (void)swipeBottom { gme_down  (); }

- (void)tap { gme_new_game(); }

- (BOOL)application:(UIApplication *)app didFinishLaunchingWithOptions:(id)options {
  MTKView * view = [MTKView new];
  view.delegate = [POCViewDelegate new];

  UIViewController * vc = [UIViewController new];
  vc.view = view;

  UISwipeGestureRecognizer * left = [UISwipeGestureRecognizer new];
  left.direction = UISwipeGestureRecognizerDirectionLeft;
  left.cancelsTouchesInView = NO;
  [left addTarget:self action:@selector(swipeLeft)];
  [vc.view addGestureRecognizer:left];

  UISwipeGestureRecognizer * right = [UISwipeGestureRecognizer new];
  right.direction = UISwipeGestureRecognizerDirectionRight;
  right.cancelsTouchesInView = NO;
  [right addTarget:self action:@selector(swipeRight)];
  [vc.view addGestureRecognizer:right];

  UISwipeGestureRecognizer * top = [UISwipeGestureRecognizer new];
  top.direction = UISwipeGestureRecognizerDirectionUp;
  top.cancelsTouchesInView = NO;
  [top addTarget:self action:@selector(swipeTop)];
  [vc.view addGestureRecognizer:top];

  UISwipeGestureRecognizer * bottom = [UISwipeGestureRecognizer new];
  bottom.direction = UISwipeGestureRecognizerDirectionDown;
  bottom.cancelsTouchesInView = NO;
  [bottom addTarget:self action:@selector(swipeBottom)];
  [vc.view addGestureRecognizer:bottom];

  UITapGestureRecognizer * tap = [UITapGestureRecognizer new];
  tap.numberOfTapsRequired = 1;
  tap.numberOfTouchesRequired = 1;
  tap.cancelsTouchesInView = NO;
  [tap addTarget:self action:@selector(tap)];
  [vc.view addGestureRecognizer:tap];

  self.window = [UIWindow new];
  self.window.frame = [UIScreen mainScreen].bounds;
  self.window.rootViewController = vc;
  [self.window makeKeyAndVisible];
  return YES;
}
- (void)applicationWillTerminate:(UIApplication *)app {
}
@end

int main(int argc, char ** argv) {
  @autoreleasepool {
    return UIApplicationMain(argc, argv, nil, @"POCAppDelegate");
  }
}
