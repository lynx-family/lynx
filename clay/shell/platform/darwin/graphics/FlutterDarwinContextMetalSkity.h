// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_DARWIN_GRAPHICS_FLUTTER_DARWIN_CONTEXT_METAL_SKITY_H_
#define CLAY_SHELL_PLATFORM_DARWIN_GRAPHICS_FLUTTER_DARWIN_CONTEXT_METAL_SKITY_H_

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include <memory>
#include "skity/gpu/gpu_context_mtl.h"

NS_ASSUME_NONNULL_BEGIN

/**
 * Provides skity GPUContext that are shared between iOS and macOS embeddings.
 */
@interface FlutterDarwinContextMetalSkity : NSObject

/**
 * Initializes a FlutterDarwinContextMetalSkity with the system default MTLDevice and a new
 * MTLCommandQueue.
 */
- (instancetype)initWithDefaultMTLDevice;

/**
 * Initializes a FlutterDarwinContextMetalSkity with provided MTLDevice and MTLCommandQueue.
 */
- (instancetype)initWithMTLDevice:(id<MTLDevice>)device
                     commandQueue:(id<MTLCommandQueue>)commandQueue;

/**
 * Creates a skity::GPUContext with the provided `MTLDevice` and `MTLCommandQueue`.
 */
+ (std::shared_ptr<skity::GPUContext>)createGPUContext:(id<MTLDevice>)device
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
 * Skity GPUContext that is used for rendering.
 */
@property(nonatomic, readonly) std::shared_ptr<skity::GPUContext> skityContext;

@end

NS_ASSUME_NONNULL_END

#endif  // CLAY_SHELL_PLATFORM_DARWIN_GRAPHICS_FLUTTER_DARWIN_CONTEXT_METAL_SKITY_H_
