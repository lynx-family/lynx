// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SHELL_PLATFORM_DARWIN_GRAPHICS_DARWIN_CONTEXT_METAL_H_
#define SHELL_PLATFORM_DARWIN_GRAPHICS_DARWIN_CONTEXT_METAL_H_

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include "third_party/skia/include/gpu/GrDirectContext.h"

NS_ASSUME_NONNULL_BEGIN

/**
 * Provides skia GrContexts that are shared between iOS and macOS embeddings.
 */
@interface FlutterDarwinContextMetalSkia : NSObject

/**
 * Initializes a FlutterDarwinContextMetalSkia with the system default MTLDevice and a new
 * MTLCommandQueue.
 */
- (instancetype)initWithDefaultMTLDevice;

/**
 * Initializes a FlutterDarwinContextMetalSkia with provided MTLDevice and MTLCommandQueue.
 */
- (instancetype)initWithMTLDevice:(id<MTLDevice>)device
                     commandQueue:(id<MTLCommandQueue>)commandQueue;

/**
 * Creates a GrDirectContext with the provided `MTLDevice` and `MTLCommandQueue`.
 */
+ (sk_sp<GrDirectContext>)createGrContext:(id<MTLDevice>)device
                             commandQueue:(id<MTLCommandQueue>)commandQueue;

/**
 * MTLDevice that is backing this context.s
 */
@property(nonatomic, readonly) id<MTLDevice> device;

/**
 * MTLCommandQueue that is acquired from the `device`. This queue is used both for rendering and
 * resource related commands.
 */
@property(nonatomic, readonly) id<MTLCommandQueue> commandQueue;

/**
 * Skia GrContext that is used for rendering.
 */
@property(nonatomic, readonly) sk_sp<GrDirectContext> mainContext;

/**
 * Skia GrContext that is used for resources (uploading textures etc).
 */
@property(nonatomic, readonly) sk_sp<GrDirectContext> resourceContext;

@end

NS_ASSUME_NONNULL_END

#endif  // SHELL_PLATFORM_DARWIN_GRAPHICS_DARWIN_CONTEXT_METAL_H_
