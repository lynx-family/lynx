// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUI+Internal.h"

#import <libkern/OSAtomic.h>
#import "LynxDefines.h"
#import "LynxUIText.h"
#import "LynxUIUnitUtils.h"
#import "LynxUnitUtils.h"
#import "LynxView+Internal.h"

@implementation LynxUI (AsyncDisplay)

+ (void)drawRect:(CGRect)bounds withParameters:(id)drawParameters {
}

+ (UIImage *)imageWithActionBlock:(LynxCGContextImageDrawingActions)action
                           opaque:(BOOL)opaque
                            scale:(CGFloat)scale
                             size:(CGSize)size {
  if (size.width == 0 || size.height == 0) {
    return nil;
  }

  if (@available(iOS 17.0, *)) {
    UIGraphicsImageRendererFormat *format = [UIGraphicsImageRendererFormat new];
    format.scale = scale;
    format.opaque = opaque;
    UIGraphicsImageRenderer *renderer = [[UIGraphicsImageRenderer alloc] initWithSize:size
                                                                               format:format];
    UIImage *image =
        [renderer imageWithActions:^(UIGraphicsImageRendererContext *_Nonnull rendererContext) {
          CGContextRef ctx = rendererContext.CGContext;
          action(ctx);
        }];
    return image;
  }

  // TODO(renzhongyue): remove the following code after upgrading target deployment iOS version
  // above 10.0.
  UIGraphicsBeginImageContextWithOptions(size, opaque, scale);
  CGContextRef ctx = UIGraphicsGetCurrentContext();
  action(ctx);
  UIImage *image = UIGraphicsGetImageFromCurrentImageContext();
  UIGraphicsEndImageContext();
  return image;
}

- (id)drawParameter {
  return nil;
}

+ (dispatch_queue_t)displayQueue {
  static dispatch_queue_t displayQueue = NULL;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    displayQueue =
        dispatch_queue_create("com.lynx.asyncDisplay.displayQueue", DISPATCH_QUEUE_SERIAL);
    // we use the highpri queue to prioritize UI rendering over other async operations
    dispatch_set_target_queue(displayQueue,
                              dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0));
  });

  return displayQueue;
}

#ifndef LYNX_CHECK_CANCELLED_AND_RETURN_NIL
#define LYNX_CHECK_CANCELLED_AND_RETURN_NIL(expr) \
  if (isCancelledBlock()) {                       \
    expr;                                         \
    return nil;                                   \
  }

- (BOOL)enableAsyncDisplay {
  BOOL config = self.context.rootView.enableAsyncDisplay;
  return config && _asyncDisplayFromTTML;
}

- (void)displayAsynchronously {
  __weak LynxUI *weakSelf = self;
  [self displayAsyncWithCompletionBlock:^(UIImage *_Nonnull image) {
    CALayer *layer = weakSelf.view.layer;
    layer.contents = (id)image.CGImage;
    layer.contentsScale = [LynxUIUnitUtils screenScale];
  }];
}

- (void)displayComplexBackgroundAsynchronouslyWithDisplay:
            (lynx_async_get_background_image_block_t)displayBlock
                                               completion:(lynx_async_display_completion_block_t)
                                                              completionBlock {
  if (self.enableAsyncDisplay) {
    dispatch_async([self.class displayQueue], ^{
      UIImage *value = displayBlock();
      dispatch_async(dispatch_get_main_queue(), ^{
        completionBlock(value);
      });
    });
  } else {
    id value;
    @try {
      value = displayBlock();
    } @catch (NSException *exception) {
    }
    completionBlock(value);
  }
}

- (void)displayAsyncWithCompletionBlock:(lynx_async_display_completion_block_t)block {
  CGRect bounds = {.origin = CGPointZero, .size = self.frame.size};
  if (CGSizeEqualToSize(bounds.size, CGSizeZero)) {
    return;
  }
  NSAssert(self.view.layer != nil, @"LynxUI+AsyncDispaly should has layer.");
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  int32_t displaySentinelValue = OSAtomicIncrement32(&_displaySentinel);
#pragma GCC diagnostic pop
  __weak LynxUI *weakSelf = self;
  id drawParameter = [weakSelf drawParameter];
  lynx_iscancelled_block_t isCancelledBlock = ^BOOL {
    __strong LynxUI *strongSelf = weakSelf;
    return strongSelf == nil || (displaySentinelValue != strongSelf->_displaySentinel);
  };
  lynx_async_operation_block_t displayBlock = ^id {
    LYNX_CHECK_CANCELLED_AND_RETURN_NIL();

    UIImage *image = [LynxUI
        imageWithActionBlock:^(CGContextRef _Nonnull context) {
          UIGraphicsPushContext(context);
          [weakSelf.class drawRect:bounds withParameters:drawParameter];
          UIGraphicsPopContext();
        }
                      opaque:NO
                       scale:[LynxUIUnitUtils screenScale]
                        size:weakSelf.frameSize];
    return image;
  };
  lynx_async_operation_completion_block_t completionBlock = ^(id value, BOOL canceled) {
    LynxMainThreadChecker();
    if (!canceled && !isCancelledBlock()) {
      UIImage *image = (UIImage *)value;
      block(image);
    }
  };
  if (self.enableAsyncDisplay) {
    dispatch_async([self.class displayQueue], ^{
      id value;
      @try {
        value = displayBlock();
      } @catch (NSException *exception) {
      }
      dispatch_async(dispatch_get_main_queue(), ^{
        completionBlock(value, value == nil);
      });
    });
  } else {
    id value = displayBlock();
    completionBlock(value, value == nil);
  }
}
@end

#endif
