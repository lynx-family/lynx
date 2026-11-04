// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// Portions adapted from SuperCoreGraphicLayer, Copyright (c) 2026 zhongyr.
// The original MIT copyright and permission notice is in this directory's LICENSE.

#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>

// Private-SPI experiment. Owns bounded IOSurfaces, not a UIView backing store.
@interface LynxSCGSurfaceRenderer : NSObject
@property(nonatomic, readonly) NSString *status;
@property(nonatomic, readonly) BOOL contextAPIAvailable;
@property(nonatomic, readonly) size_t allocatedBytes;
@property(nonatomic, readonly) NSUInteger bufferCount;
@property(nonatomic, readonly) NSUInteger droppedFrames;
- (instancetype)initWithPrivateAPIEnabled:(BOOL)enabled;
- (BOOL)renderSize:(CGSize)size
             scale:(CGFloat)scale
           toLayer:(CALayer *)layer
              draw:(void (^)(CGContextRef))draw;
// Flushes and reads back the last frame for validation only, outside timing.
- (NSData *)copyPixels;
- (CGSize)pixelSize;
@end
