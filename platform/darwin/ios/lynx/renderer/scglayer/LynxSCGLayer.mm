// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// Portions adapted from SuperCoreGraphicLayer, Copyright (c) 2026 zhongyr.
// The original MIT copyright and permission notice is in this directory's LICENSE.

#import "LynxSCGLayer.h"
#include <cmath>
#import "LynxSCGSurfaceRenderer.h"

NSErrorDomain const LynxSCGLayerErrorDomain = @"LynxSCGLayerErrorDomain";

@interface LynxSCGLayer ()
- (BOOL)renderFrameWithError:(NSError **)error;
- (BOOL)renderBitmap;
@end

@implementation LynxSCGLayer {
  LynxSCGSurfaceRenderer *_renderer;
  NSError *_lastRenderError;
  NSUInteger _renderedFrameCount;
  BOOL _retryScheduled;
  BOOL _usingBitmapFallback;
}
@synthesize drawingBlock = _drawingBlock;

+ (BOOL)contextAPIAvailable {
  return [[LynxSCGSurfaceRenderer alloc] initWithPrivateAPIEnabled:YES].contextAPIAvailable;
}

- (instancetype)init {
  if ((self = [super init])) self.needsDisplayOnBoundsChange = YES;
  return self;
}

- (instancetype)initWithCoder:(NSCoder *)coder {
  if ((self = [super initWithCoder:coder])) self.needsDisplayOnBoundsChange = YES;
  return self;
}

- (instancetype)initWithLayer:(id)layer {
  if ((self = [super initWithLayer:layer])) {
    if ([layer isKindOfClass:LynxSCGLayer.class])
      _drawingBlock = [((LynxSCGLayer *)layer).drawingBlock copy];
    // Never share writable backing pools with Core Animation's layer copies.
  }
  return self;
}

- (void)setDrawingBlock:(LynxSCGDrawingBlock)block {
  _drawingBlock = [block copy];
  [self setNeedsDisplay];
}

- (void)setContentsScale:(CGFloat)scale {
  CGFloat old = self.contentsScale;
  [super setContentsScale:scale];
  if (old != scale) [self setNeedsDisplay];
}

- (id<CAAction>)actionForKey:(NSString *)event {
  return (id<CAAction>)[NSNull null];
}

- (void)display {
  [self renderFrameWithError:nullptr];
}

- (BOOL)renderWithError:(NSError **)error {
  if (!NSThread.isMainThread) {
    if (error)
      *error =
          [NSError errorWithDomain:LynxSCGLayerErrorDomain
                              code:LynxSCGLayerErrorWrongThread
                          userInfo:@{NSLocalizedDescriptionKey : @"Render on the main thread"}];
    return NO;
  }
  // Let Core Animation clear the dirty flag, avoiding a second automatic draw
  // after an explicit render in the same transaction.
  [self setNeedsDisplay];
  [self displayIfNeeded];
  if (error) *error = _lastRenderError;
  return _lastRenderError == nil;
}

- (BOOL)renderFrameWithError:(NSError **)error {
  if (error) *error = nil;
  if (!NSThread.isMainThread) {
    if (error)
      *error =
          [NSError errorWithDomain:LynxSCGLayerErrorDomain
                              code:LynxSCGLayerErrorWrongThread
                          userInfo:@{NSLocalizedDescriptionKey : @"Render on the main thread"}];
    return NO;
  }
  CGRect bounds = self.bounds;
  CGFloat scale = self.contentsScale;
  NSError *failure = nil;
  if (!std::isfinite(bounds.origin.x) || !std::isfinite(bounds.origin.y) ||
      !std::isfinite(bounds.size.width) || !std::isfinite(bounds.size.height) ||
      !std::isfinite(scale) || scale <= 0 || bounds.size.width <= 0 || bounds.size.height <= 0 ||
      ceil(bounds.size.width * scale) > 4096 || ceil(bounds.size.height * scale) > 4096) {
    failure =
        [NSError errorWithDomain:LynxSCGLayerErrorDomain
                            code:LynxSCGLayerErrorInvalidGeometry
                        userInfo:@{
                          NSLocalizedDescriptionKey : @"Use finite, nonempty bounds and a positive "
                                                      @"scale; maximum 4096 pixels per dimension"
                        }];
  } else if (_usingBitmapFallback) {
    if (![self renderBitmap]) {
      failure = [NSError errorWithDomain:LynxSCGLayerErrorDomain
                                    code:LynxSCGLayerErrorRenderingFailed
                                userInfo:nil];
    }
  } else {
    if (!_renderer) _renderer = [[LynxSCGSurfaceRenderer alloc] initWithPrivateAPIEnabled:YES];
    if (!_renderer.contextAPIAvailable) {
      failure = [NSError errorWithDomain:LynxSCGLayerErrorDomain
                                    code:LynxSCGLayerErrorUnavailable
                                userInfo:@{NSLocalizedDescriptionKey : _renderer.status}];
    } else {
      NSUInteger droppedBefore = _renderer.droppedFrames;
      BOOL rendered =
          [_renderer renderSize:bounds.size
                          scale:scale
                        toLayer:self
                           draw:^(CGContextRef context) {
                             CGContextTranslateCTM(context, -bounds.origin.x, -bounds.origin.y);
                             if (self->_drawingBlock)
                               self->_drawingBlock(context, bounds);
                             else
                               [self drawInContext:context];
                           }];
      if (rendered)
        ++_renderedFrameCount;
      else {
        failure = [NSError errorWithDomain:LynxSCGLayerErrorDomain
                                      code:LynxSCGLayerErrorRenderingFailed
                                  userInfo:@{NSLocalizedDescriptionKey : _renderer.status}];
        if (_renderer.droppedFrames > droppedBefore) {
          // A one-off image completion must not disappear because CA still owns
          // the back buffers. Retry without blocking or overwriting an in-use surface.
          if (!_retryScheduled) {
            _retryScheduled = YES;
            __weak LynxSCGLayer *weakSelf = self;
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC / 60),
                           dispatch_get_main_queue(), ^{
                             LynxSCGLayer *layer = weakSelf;
                             if (layer == nil) return;
                             layer->_retryScheduled = NO;
                             [layer setNeedsDisplay];
                           });
          }
        } else if ([self renderBitmap]) {
          // Symbol availability does not guarantee that a constructor accepts
          // this process/device. Keep content visible using public CoreGraphics.
          _usingBitmapFallback = YES;
          _renderer = nil;
          failure = nil;
        }
      }
    }
  }
  _lastRenderError = failure;
  if (error) *error = failure;
  return failure == nil;
}

- (BOOL)renderBitmap {
  CGRect bounds = self.bounds;
  CGFloat scale = self.contentsScale;
  size_t width = ceil(bounds.size.width * scale), height = ceil(bounds.size.height * scale);
  CGColorSpaceRef colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
  CGContextRef context = CGBitmapContextCreate(
      nullptr, width, height, 8, width * 4, colorSpace,
      kCGBitmapByteOrder32Little | (CGBitmapInfo)kCGImageAlphaPremultipliedFirst);
  CGColorSpaceRelease(colorSpace);
  if (context == nullptr) return NO;
  CGContextTranslateCTM(context, 0, height);
  CGContextScaleCTM(context, scale, -scale);
  CGContextTranslateCTM(context, -bounds.origin.x, -bounds.origin.y);
  if (_drawingBlock)
    _drawingBlock(context, bounds);
  else
    [self drawInContext:context];
  CGImageRef image = CGBitmapContextCreateImage(context);
  CGContextRelease(context);
  if (image == nullptr) return NO;
  self.contents = (__bridge id)image;
  CGImageRelease(image);
  ++_renderedFrameCount;
  return YES;
}

- (BOOL)usingBitmapFallback {
  return _usingBitmapFallback;
}
- (NSError *)lastRenderError {
  return _lastRenderError;
}
- (NSUInteger)renderedFrameCount {
  return _renderedFrameCount;
}
- (NSUInteger)droppedFrameCount {
  return _renderer.droppedFrames;
}
- (NSUInteger)surfaceCount {
  return _renderer.bufferCount;
}
- (size_t)ownedSurfaceBytes {
  return _renderer.allocatedBytes;
}
@end
