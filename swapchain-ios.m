#import <CoreFoundation/CoreFoundation.h>
#import <MetalKit/MetalKit.h>
#import <UIKit/UIKit.h>

void vlk_init();
void vlk_frame();
void vlk_deinit();

CAMetalLayer * g_layer;

@interface POCViewDelegate : NSObject<MTKViewDelegate>
@property (nonatomic) BOOL ready;
@end
@implementation POCViewDelegate
- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
}
- (void)drawInMTKView:(MTKView *)view {
  if (!self.ready) {
    g_layer = (CAMetalLayer *)view.layer;

    vlk_init();
    self.ready = YES;
  }
  vlk_frame();
}
@end

@interface POCAppDelegate : NSObject<UIApplicationDelegate>
@property(nonatomic, strong) UIWindow * window;
@end
@implementation POCAppDelegate
- (BOOL)application:(UIApplication *)app didFinishLaunchingWithOptions:(id)options {
  MTKView * view = [MTKView new];
  view.delegate = [POCViewDelegate new];

  UIViewController * vc = [UIViewController new];
  vc.view = view;

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

CAMetalLayer * vlk_metal_layer() { return g_layer; }

void vlk_log(int r, const char * msg) {
  NSLog(@"Vulkan call failed (code=%d): %s\n", r, msg);
  exit(1);
}

int main(int argc, char ** argv) {
  @autoreleasepool {
    return UIApplicationMain(argc, argv, nil, @"POCAppDelegate");
  }
}
