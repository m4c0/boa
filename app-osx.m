#import <AppKit/AppKit.h>
#import <CoreFoundation/CoreFoundation.h>
#import <MetalKit/MetalKit.h>

#include "gme.h"

@interface POCViewDelegate : NSObject<MTKViewDelegate>
@property (nonatomic,strong) id<MTLCommandQueue> queue;
@property (nonatomic,strong) id<MTLRenderPipelineState> pipeline;
@property (nonatomic,strong) id<MTLBuffer> grid;
@end
@implementation POCViewDelegate
- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
  gme_resize(size.width, size.height);
}
- (void)drawInMTKView:(MTKView *)view {
  gme_load(self.grid.contents);
  gme_frame();

  MTLRenderPassDescriptor * rpd = view.currentRenderPassDescriptor;
  if (rpd == nil) return;

  id<MTLCommandBuffer> cb = [self.queue commandBuffer];

  id<MTLRenderCommandEncoder> enc = [cb renderCommandEncoderWithDescriptor:rpd];
  [enc setRenderPipelineState:self.pipeline];
  [enc setVertexBytes:&gme_pc length:sizeof(gme_upc_t) atIndex:0];
  [enc setFragmentBytes:&gme_pc length:sizeof(gme_upc_t) atIndex:0];
  [enc setFragmentBuffer:self.grid offset:0 atIndex:1];
  [enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
  [enc endEncoding];

  [cb presentDrawable:view.currentDrawable];
  [cb commit];
}
@end

@interface POCView : MTKView
@end
@implementation POCView
- (BOOL)acceptsFirstResponder {
  return YES;
}
- (void)keyDown:(NSEvent *)event {
  NSString * chrs = event.charactersIgnoringModifiers;
  if (chrs.length != 1) return;

  unichar c = [chrs characterAtIndex:0];
  switch (c) {
    case NSLeftArrowFunctionKey:  return gme_left();
    case NSRightArrowFunctionKey: return gme_right();
    case NSUpArrowFunctionKey:    return gme_up();
    case NSDownArrowFunctionKey:  return gme_down();

    case ' ': return gme_new_game();
  }
}
@end

@interface POCAppDelegate : NSObject<NSApplicationDelegate>
@end
@implementation POCAppDelegate
- (void)applicationWillTerminate:(NSApplication *)app {
  gme_deinit();
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app {
  return YES;
}
@end

static id<MTLLibrary> load_library(id<MTLDevice> device, NSString * name) {
  NSString * path = [[NSBundle mainBundle] pathForResource:name ofType:@"metal"];
  NSString * src = [NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:nil];
  MTLCompileOptions * opts = [MTLCompileOptions new];
  NSError * err;
  id<MTLLibrary> lib = [device newLibraryWithSource:src options:opts error:&err];
  if (err) {
    NSLog(@"Error compiling shader: %@", err);
    return nil;
  }
  return lib;
}
static void run() {
  NSDictionary * info = [[NSBundle mainBundle] infoDictionary];
  NSString * name = info[@"CFBundleDisplayName"];
  if (!name) name = info[@"CFBundleName"];
  if (!name) name = @"App";

  MTKView * v = [POCView new];
  v.device = MTLCreateSystemDefaultDevice();
  v.clearColor = MTLClearColorMake(0.01, 0.02, 0.03, 1.0);

  POCViewDelegate * vc = [POCViewDelegate new];
  v.delegate = vc;
  vc.queue = [v.device newCommandQueue];

  id<MTLLibrary> vert = load_library(v.device, @"shader.vert");
  id<MTLLibrary> frag = load_library(v.device, @"shader.frag");
  if (!vert || !frag) return;

  MTLRenderPipelineDescriptor * pd = [MTLRenderPipelineDescriptor new];
  pd.vertexFunction   = [vert newFunctionWithName:@"main0"];
  pd.fragmentFunction = [frag newFunctionWithName:@"main0"];
  pd.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA8Unorm;
  NSError * err;
  vc.pipeline = [v.device newRenderPipelineStateWithDescriptor:pd error:&err];
  if (err) {
    NSLog(@"Error creating pipeline: %@", err);
    return;
  }

  vc.grid = [v.device newBufferWithLength:GME_BUF_SIZE options:MTLResourceStorageModeShared];

  NSWindow * w = [NSWindow new];
  w.contentView = v;
  w.styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;

  NSRect crect = NSMakeRect(0, 0, 600, 800);
  NSRect frect = [w frameRectForContentRect:crect];
  [w setFrame:frect display:YES];
  [w center];
  [w makeKeyAndOrderFront:w];

  gme_init();

  // Apple menu
  NSMenu * menu = [NSMenu new];
  [menu       addItem:[[NSMenuItem alloc]
        initWithTitle:[@"Quit " stringByAppendingString:name]
               action:@selector(terminate:)
        keyEquivalent:@"q"]];

  NSMenuItem * item = [NSMenuItem new];
  item.submenu = menu;

  NSMenu * bar = [NSMenu new];
  [bar addItem:item];

  NSApplication * a = [NSApplication sharedApplication];
  a.delegate = [POCAppDelegate new];
  a.mainMenu = bar;
  [a activateIgnoringOtherApps:YES];
  [a run];
}

int main() {
  @autoreleasepool {
    run();
  }
}
