// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// Portions adapted from SuperCoreGraphicLayer, Copyright (c) 2026 zhongyr.
// The original MIT copyright and permission notice is in this directory's LICENSE.

#import <QuartzCore/QuartzCore.h>

NS_ASSUME_NONNULL_BEGIN

FOUNDATION_EXPORT NSErrorDomain const LynxSCGLayerErrorDomain;
typedef NS_ERROR_ENUM(LynxSCGLayerErrorDomain, LynxSCGLayerError){
    LynxSCGLayerErrorUnavailable = 1,
    LynxSCGLayerErrorInvalidGeometry,
    LynxSCGLayerErrorRenderingFailed,
    LynxSCGLayerErrorWrongThread,
};

/// The context uses points with a top-left origin. It is valid only during this call.
typedef void (^LynxSCGDrawingBlock)(CGContextRef context, CGRect bounds);

/// Experimental CoreGraphics drawing into a bounded IOSurface pool.
/// Uses private APIs. Not suitable for an App Store submission using public APIs only.
/// Configure, invalidate, and render on the main thread. Set contentsScale explicitly.
@interface LynxSCGLayer : CALayer

/// Symbol availability only: neither successful context creation nor GPU acceleration is
/// guaranteed.
@property(class, nonatomic, readonly) BOOL contextAPIAvailable;

/// Setting the block invalidates the layer. Avoid strongly capturing the layer or its owner.
/// If nil, rendering calls -drawInContext: so subclasses can supply their drawing there.
@property(nonatomic, copy, nullable) LynxSCGDrawingBlock drawingBlock;
@property(nonatomic, strong, readonly, nullable) NSError *lastRenderError;
@property(nonatomic, readonly) NSUInteger renderedFrameCount;
@property(nonatomic, readonly) NSUInteger droppedFrameCount;
@property(nonatomic, readonly) NSUInteger surfaceCount;
@property(nonatomic, readonly) size_t ownedSurfaceBytes;
@property(nonatomic, readonly) BOOL usingBitmapFallback;

/// Draw immediately instead of waiting for Core Animation's display pass.
/// A successful return submits contents; it is not a GPU completion/presentation fence.
/// Failures keep the previous contents except when a resize retires the old backing.
/// Context-creation failure falls back to a public bitmap context. Busy buffers retry.
/// Each dimension is limited to 4096 physical pixels.
- (BOOL)renderWithError:(NSError *_Nullable *_Nullable)error;

@end

NS_ASSUME_NONNULL_END
