// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxScreenCastHelper.h"
#import <CoreGraphics/CoreGraphics.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxUIKitAPIAdapter.h>
#import <Lynx/LynxView.h>
#include <memory>
#import "LynxDevToolFrameCapturer.h"
#include "base/screen_metadata.h"

static int const kCardPreviewQuality = 80;
static int const kCardPreviewMaxWidth = 150;
static int const kCardPreviewMaxHeight = 300;

NSString* const ScreenshotModeLynxView = @"lynxview";
NSString* const ScreenshotModeFullScreen = @"fullscreen";

// Delay for 1500ms to allow time for rendering remote resources
int const ScreenshotPreviewDelayTime = 1500;

#pragma mark - LynxScreenCapture
@interface LynxScreenCapture : LynxDevToolFrameCapturer <FrameCapturerDelegate>

+ (dispatch_queue_t)serialProcessQueue;

- (instancetype)initWithLynxView:(LynxView*)root
            withPlatformDelegate:(DevToolPlatformDarwinDelegate*)platformDelegate;

- (void)attachLynxView:(nonnull LynxView*)view;

- (void)startCapture:(int)quality
               width:(int)max_width
              height:(int)max_height
                mode:(NSString*)screenshot_mode;

- (void)stopCapture;
- (void)onAckReceived;

@end

@implementation LynxScreenCapture {
  int max_height_;
  int max_width_;
  int quality_;
  BOOL enabled_;
  BOOL ack_received_;
  ScreenshotMode screenshot_mode_;

  __weak DevToolPlatformDarwinDelegate* _platformDelegate;
  __weak LynxView* _lynxView;

  std::shared_ptr<lynx::devtool::ScreenMetadata> screen_metadata_;
}

+ (dispatch_queue_t)serialProcessQueue {
  static dispatch_queue_t screenCastQueue;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    screenCastQueue =
        dispatch_queue_create("com.lynx.devtool.screenCastQueue", DISPATCH_QUEUE_SERIAL);
  });
  return screenCastQueue;
}

- (instancetype)initWithLynxView:(LynxView*)root
            withPlatformDelegate:(DevToolPlatformDarwinDelegate*)platformDelegate {
  if (self = [super init]) {
    max_width_ = 0;
    max_height_ = 0;
    quality_ = 0;
    enabled_ = NO;
    ack_received_ = NO;
    screenshot_mode_ = ScreenshotModeFullScreen;

    _lynxView = root;
    _platformDelegate = platformDelegate;

    screen_metadata_ = std::make_shared<lynx::devtool::ScreenMetadata>();
    self.delegate = self;
  }
  return self;
}

- (void)attachLynxView:(nonnull LynxView*)view {
  [self attachView:view];
  _lynxView = view;
}

#if OS_IOS
- (UIColor*)getBackgroundColor {
  __strong typeof(_lynxView) lynxView = _lynxView;
  if (lynxView == nil) {
    return nil;
  }
  UIView* view = lynxView;
  auto parent_view = [view superview];
  UIColor* res = [UIColor whiteColor];
  while (parent_view &&
         (![view backgroundColor] || [[view backgroundColor] isEqual:[UIColor clearColor]])) {
    view = parent_view;
    parent_view = [view superview];
  }
  if ([view backgroundColor] && ![[view backgroundColor] isEqual:[UIColor clearColor]]) {
    res = [view backgroundColor];
  }
  return res;
}

// take snapshot for lynx view
- (UIImage*)createImageOfView:(UIView*)view forCardPreview:(bool)isPreview {
  void (^draw)(CGContextRef ctx) = ^(CGContextRef ctx) {
    auto rect = CGRectMake(0, 0, view.frame.size.width, view.frame.size.height);
    [[self getBackgroundColor] setFill];
    UIRectFill(rect);
    if (isPreview) {
      // 'drawViewHierarchyInRect: afterScreenUpdates:YES' may have undefined error when view is not
      // visible on screen
      if (view.window) {
        // pass NO may get blank image when view not on screen
        [view drawViewHierarchyInRect:view.bounds afterScreenUpdates:YES];
      } else {
        [view.layer renderInContext:ctx];
      }
    } else {
      [view drawViewHierarchyInRect:view.bounds afterScreenUpdates:NO];
    }
  };
  if (@available(iOS 17.0, *)) {
    UIGraphicsImageRenderer* render =
        [[UIGraphicsImageRenderer alloc] initWithSize:view.frame.size];
    UIImage* snapshotImage =
        [render imageWithActions:^(UIGraphicsImageRendererContext* _Nonnull rendererContext) {
          draw(rendererContext.CGContext);
        }];
    return snapshotImage;
  } else {
    UIGraphicsBeginImageContextWithOptions(view.frame.size, YES, 0.0);
    @try {
      draw(UIGraphicsGetCurrentContext());
    } @catch (NSException* exception) {
      LLogWarn(@"%@", exception.description);
      UIGraphicsEndImageContext();
      return nil;
    }
    UIImage* snapshotImage = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    return snapshotImage;
  }
}

// take snapshot for screen
- (UIImage*)createImageOfScreen:(UIView*)view {
  UIView* screenView = [LynxUIKitAPIAdapter getKeyWindow];
  if (@available(iOS 17.0, *)) {
    UIGraphicsImageRenderer* render =
        [[UIGraphicsImageRenderer alloc] initWithSize:screenView.frame.size];
    UIImage* snapshotImage =
        [render imageWithActions:^(UIGraphicsImageRendererContext* _Nonnull rendererContext) {
          [screenView drawViewHierarchyInRect:screenView.bounds afterScreenUpdates:NO];
        }];
    return snapshotImage;
  } else {
    UIGraphicsBeginImageContextWithOptions(screenView.frame.size, YES, 0.0);
    [screenView drawViewHierarchyInRect:screenView.bounds afterScreenUpdates:NO];
    UIImage* snapshotImage = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    return snapshotImage;
  }
}

- (NSString*)Get1xJPEGBytesFromUIImage:(UIImage*)uiimage withQuality:(int)quality {
  NSData* data = UIImageJPEGRepresentation(uiimage, quality / 100.0);
  NSString* str = [data base64EncodedStringWithOptions:NSDataBase64Encoding64CharacterLineLength];
  return str;
}

- (UIImage*)scaleImage:(UIImage*)image toScale:(float)scaleSize {
  int scaleWidth = image.size.width * scaleSize;
  int scaleHeight = image.size.height * scaleSize;
  if (@available(iOS 17.0, *)) {
    UIGraphicsImageRenderer* render =
        [[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(scaleWidth, scaleHeight)];
    UIImage* scaledImage =
        [render imageWithActions:^(UIGraphicsImageRendererContext* _Nonnull rendererContext) {
          [image drawInRect:CGRectMake(0, 0, scaleWidth, scaleHeight)];
        }];
    return scaledImage;
  } else {
    UIGraphicsBeginImageContextWithOptions(CGSizeMake(scaleWidth, scaleHeight), NO, 0);
    [image drawInRect:CGRectMake(0, 0, scaleWidth, scaleHeight)];
    UIImage* scaledImage = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    return scaledImage;
  }
}

#elif OS_OSX
- (NSColor*)getBackgroundColor {
  if (_lynxView == nil) {
    return nil;
  }
  NSColor* res = [NSColor whiteColor];
  return res;
}

- (NSImage*)createImageOfView:(NSView*)view {
  __strong typeof(_lynxView) lynxView = _lynxView;
  if (lynxView == nil) {
    return nil;
  }
  NSWindow* window = lynxView.window;
  CGRect rect = lynxView.bounds;
  rect = [lynxView convertRect:rect toView:nil];
  rect = [window convertRectToScreen:rect];
  CGRect frame = [NSScreen mainScreen].frame;
  rect.origin.y = frame.size.height - rect.size.height - rect.origin.y;

  CGImageRef cgImage = CGWindowListCreateImage(
      rect, kCGWindowListOptionIncludingWindow, CGWindowID(window.windowNumber),
      kCGWindowImageNominalResolution | kCGWindowImageBoundsIgnoreFraming);
  NSImage* image = [[NSImage alloc] initWithCGImage:cgImage size:rect.size];
  CGImageRelease(cgImage);
  return image;
}

- (NSImage*)createImageOfScreen:(NSView*)view {
  // We just take view snapshot on mac
  return [self createImageOfView:view];
}

- (NSString*)Get1xJPEGBytesFromUIImage:(NSImage*)uiimage withQuality:(int)quality {
  CGFloat width = uiimage.size.width;
  CGFloat height = uiimage.size.height;
  [uiimage lockFocus];
  NSBitmapImageRep* bits =
      [[NSBitmapImageRep alloc] initWithFocusedViewRect:NSMakeRect(0, 0, width, height)];
  [uiimage unlockFocus];
  NSDictionary* imageProps =
      [NSDictionary dictionaryWithObject:[NSNumber numberWithFloat:quality / 100.0]
                                  forKey:NSImageCompressionFactor];
  NSData* data = [bits representationUsingType:NSJPEGFileType properties:imageProps];
  NSString* str = [data base64EncodedStringWithOptions:NSDataBase64Encoding64CharacterLineLength];
  return str;
}

- (NSImage*)scaleImage:(NSImage*)image toScale:(float)scaleSize {
  int scaleWidth = image.size.width * scaleSize;
  int scaleHeight = image.size.height * scaleSize;
  NSSize size = NSMakeSize(scaleWidth, scaleHeight);
  NSImage* scaledImage = [[NSImage alloc] initWithSize:size];
  [scaledImage lockFocus];
  [image drawInRect:CGRectMake(0, 0, scaleWidth, scaleHeight)];
  [scaledImage unlockFocus];
  return scaledImage;
}
#endif

- (float)getScallingFromWidth:(int)original_width
                       height:(int)original_height
                     maxWidth:(int)max_width
                    maxHeight:(int)max_height {
  float scaling_width = 1;
  float scaling_height = 1;
#if OS_IOS
  float screen_factor = [UIScreen mainScreen].scale;
#elif OS_OSX
  float screen_factor = [NSScreen mainScreen].backingScaleFactor;
#endif
  if (max_height != 0 && max_width != 0 &&
      ((original_width * screen_factor) > max_width ||
       (original_height * screen_factor) > max_height)) {
    scaling_width = max_width / (float)(original_width * screen_factor);
    scaling_height = max_height / (float)(original_height * screen_factor);
  }
  return scaling_width < scaling_height ? scaling_width : scaling_height;
}

- (void)startCapture:(int)quality
               width:(int)max_width
              height:(int)max_height
                mode:(NSString*)screenshot_mode {
  quality_ = quality;
  max_width_ = max_width;
  max_height_ = max_height;
  enabled_ = YES;
  screenshot_mode_ = screenshot_mode;
  [self attachView:_lynxView];
  [self startFrameViewTrace];
  // manually trigger first snapshot
  [self triggerNextCapture];
}

- (void)stopCapture {
  [self stopFrameViewTrace];
  enabled_ = NO;
}

- (void)onAckReceived {
  ack_received_ = YES;
}

- (void)triggerNextCapture {
  if (!self.snapshotCache || ack_received_) {
    [self screenshot];
  }
}

- (BOOL)isEnabled {
  return enabled_;
}

- (void)takeSnapshot:(VIEW_CLASS*)view {
  IMAGE_CLASS* image = [self takeSnapshot:view forCardPreview:false];
  dispatch_async([[self class] serialProcessQueue], ^{
    NSString* screenData = [self getScreenDataFromImage:image forCardPreview:false];
    [self onTakeSnapShot:screenData];
  });
}

- (IMAGE_CLASS*)takeSnapshot:(VIEW_CLASS*)view forCardPreview:(bool)isPreview {
  __strong typeof(_lynxView) lynxView = _lynxView;
  if (!lynxView || lynxView.frame.size.width <= 0 || lynxView.frame.size.height <= 0) {
    LLogWarn(@"failed to take snapshot and the _lynxView is %p", _lynxView);
    return nil;
  }

  IMAGE_CLASS* image = nil;
#if OS_IOS
  if (isPreview) {
    image = [self createImageOfView:lynxView forCardPreview:true];
  } else if (lynxView.window) {
    // only view on screen can take screenshot repeatedly
    if ([screenshot_mode_ isEqualToString:ScreenshotModeLynxView]) {
      image = [self createImageOfView:lynxView forCardPreview:false];
    } else {  // fullscreen
      image = [self createImageOfScreen:lynxView];
    }
  }
#elif OS_OSX
  if (isPreview) {
    image = [self createImageOfView:lynxView];
    // only view on screen can take screenshot repeatedly
  } else if (lynxView.window) {
    image = [self createImageOfScreen:lynxView];
  }
#endif

  if (image == nil) {
    LLogWarn(@"failed to tack snapshot caused by nil image");
    return nil;
  }
  return image;
}

- (NSString*)getScreenDataFromImage:(IMAGE_CLASS*)image forCardPreview:(bool)isPreview {
  screen_metadata_->timestamp_ = [[NSDate date] timeIntervalSince1970];
  auto original_height = image.size.height;
  auto original_width = image.size.width;
  screen_metadata_->device_width_ = original_width;
  screen_metadata_->device_height_ = original_height;
  screen_metadata_->page_scale_factor_ = 1;
  float scalling = 1;
  int max_width = isPreview ? kCardPreviewMaxWidth : max_width_;
  int max_height = isPreview ? kCardPreviewMaxHeight : max_height_;
  int quality = isPreview ? kCardPreviewQuality : quality_;
  scalling = [self getScallingFromWidth:original_width
                                 height:original_height
                               maxWidth:max_width
                              maxHeight:max_height];
  image = [self scaleImage:image toScale:scalling];
  return [self Get1xJPEGBytesFromUIImage:image withQuality:quality];
}

- (void)onNewSnapshot:(NSString*)data {
  __strong typeof(_platformDelegate) platformDelegate = _platformDelegate;

  ack_received_ = NO;
  [platformDelegate sendScreenCast:data andMetadata:self->screen_metadata_];
}

- (void)onFrameChanged {
  [self triggerNextCapture];
}

@end

#pragma mark - LynxScreenCastHelper
@implementation LynxScreenCastHelper {
  __weak LynxView* _lynxView;
  __weak DevToolPlatformDarwinDelegate* _platformDelegate;
  BOOL _paused;
  BOOL _screencast_enabled;

  LynxScreenCapture* _screenCastHelper;
}

- (nonnull instancetype)initWithLynxView:(LynxView*)view
                    withPlatformDelegate:(DevToolPlatformDarwinDelegate*)platformDelegate {
  _lynxView = view;
  _platformDelegate = platformDelegate;
  _paused = NO;
  _screencast_enabled = NO;

  _screenCastHelper = [[LynxScreenCapture alloc] initWithLynxView:view
                                             withPlatformDelegate:platformDelegate];
  return self;
}

- (void)startCasting:(int)quality
               width:(int)max_width
              height:(int)max_height
                mode:(NSString*)screenshot_mode {
  __strong typeof(_platformDelegate) platformDelegate = _platformDelegate;
  _screencast_enabled = YES;
  [platformDelegate dispatchScreencastVisibilityChanged:YES];
  [_screenCastHelper startCapture:quality width:max_width height:max_height mode:screenshot_mode];
}

- (void)stopCasting {
  _screencast_enabled = NO;
  [_screenCastHelper stopCapture];
  __strong typeof(_platformDelegate) platformDelegate = _platformDelegate;
  [platformDelegate dispatchScreencastVisibilityChanged:NO];
}

- (void)continueCasting {
  if (_screencast_enabled) {
    if (_paused) {
      _screenCastHelper.snapshotCache = nil;
      _paused = NO;
      __strong typeof(_platformDelegate) platformDelegate = _platformDelegate;
      [platformDelegate dispatchScreencastVisibilityChanged:YES];
      [_screenCastHelper triggerNextCapture];
    }
  }
}

- (void)pauseCasting {
  if (_screencast_enabled) {
    if (!_paused) {
      _paused = YES;
      __strong typeof(_platformDelegate) platformDelegate = _platformDelegate;
      [platformDelegate dispatchScreencastVisibilityChanged:NO];
    }
  }
}

- (void)attachLynxView:(nonnull LynxView*)lynxView {
  _lynxView = lynxView;
  [_screenCastHelper attachLynxView:lynxView];
}

- (void)onAckReceived {
  [_screenCastHelper onAckReceived];
}

- (void)sendCardPreview {
  IMAGE_CLASS* image = [_screenCastHelper takeSnapshot:_lynxView forCardPreview:true];
  dispatch_async([[_screenCastHelper class] serialProcessQueue], ^{
    NSString* cardPreviewData = [self->_screenCastHelper getScreenDataFromImage:image
                                                                 forCardPreview:true];
    __strong typeof(_platformDelegate) platformDelegate = self->_platformDelegate;
    [platformDelegate sendCardPreviewData:cardPreviewData];
  });
}

@end
