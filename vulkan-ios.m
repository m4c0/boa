#import <CoreFoundation/CoreFoundation.h>
#import <MetalKit/MetalKit.h>
#import <UIKit/UIKit.h>

#include "gme.h"
#include "vlk.h"

CAMetalLayer * vlk_metal_layer;

@interface POCViewDelegate : NSObject<MTKViewDelegate>
@property (nonatomic) BOOL ready;
@end
@implementation POCViewDelegate
- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
}
- (void)drawInMTKView:(MTKView *)view {
  if (!self.ready) {
    vlk_metal_layer = (CAMetalLayer *)view.layer;

    vlk_init(1);
    self.ready = YES;
  }
  vlk_frame();
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
  vlk_deinit();
}
@end

__strong static NSData * last_resource;
unsigned vlk_open(const char * name, const char * ext, const void ** ptr) {
  NSString * n = [NSString stringWithFormat:@"%s", name];
  NSString * e = [NSString stringWithFormat:@"%s", ext];
  NSString * path = [[NSBundle mainBundle] pathForResource:n ofType:e];
  last_resource = [NSData dataWithContentsOfFile:path];
  *ptr = [last_resource bytes];
  return [last_resource length];
}

void vlk_log(int r, const char * msg) {
  NSLog(@"Vulkan call failed (code=%d): %s\n", r, msg);
  exit(1);
}

int main(int argc, char ** argv) {
  @autoreleasepool {
    return UIApplicationMain(argc, argv, nil, @"POCAppDelegate");
  }
}
