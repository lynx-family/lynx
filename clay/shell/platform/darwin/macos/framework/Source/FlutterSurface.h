// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Cocoa/Cocoa.h>

#import "clay/shell/platform/embedder/embedder_surface_metal.h"

/**
 * Opaque surface type.
 * Can be represented as FlutterMetalTexture to cross the embedder API boundary.
 */
@interface FlutterSurface : NSObject

- (clay::GPUMTLTextureInfo)asGPUMTLTextureInfo;

+ (nullable FlutterSurface*)fromGPUMTLTextureInfo:
    (nonnull const clay::GPUMTLTextureInfo*)textureInfo;

@end

/**
 * Internal FlutterSurface interface used by FlutterSurfaceManager.
 * Wraps an IOSurface framebuffer and metadata related to the surface.
 */
@interface FlutterSurface (Private)

- (nonnull instancetype)initWithSize:(CGSize)size device:(nonnull id<MTLDevice>)device;

@property(readonly, nonatomic, nonnull) IOSurfaceRef ioSurface;
@property(readonly, nonatomic) CGSize size;
@property(readonly, nonatomic) int64_t textureId;

@end
