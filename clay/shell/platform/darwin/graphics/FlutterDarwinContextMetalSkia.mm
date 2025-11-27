// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "clay/shell/platform/darwin/graphics/FlutterDarwinContextMetalSkia.h"

#import "FlutterMacros.h"
#include "clay/common/graphics/persistent_cache.h"
#include "clay/fml/logging.h"
#include "clay/shell/common/context_options.h"

FLUTTER_ASSERT_ARC

@implementation FlutterDarwinContextMetalSkia

- (instancetype)initWithDefaultMTLDevice {
  id<MTLDevice> device = MTLCreateSystemDefaultDevice();
  return [self initWithMTLDevice:device commandQueue:[device newCommandQueue]];
}

- (instancetype)initWithMTLDevice:(id<MTLDevice>)device
                     commandQueue:(id<MTLCommandQueue>)commandQueue {
  self = [super init];
  if (self != nil) {
    _device = device;

    if (!_device) {
      FML_DLOG(ERROR) << "Could not acquire Metal device.";
      return nil;
    }

    _commandQueue = commandQueue;

    if (!_commandQueue) {
      FML_DLOG(ERROR) << "Could not create Metal command queue.";
      return nil;
    }

    [_commandQueue setLabel:@"Flutter Main Queue"];

    // The devices are in the same "sharegroup" because they share the same device and command
    // queues for now. When the resource context gets its own transfer queue, this will have to be
    // refactored.
    _mainContext = [self createGrContext];
    _resourceContext = [self createGrContext];

    if (!_mainContext || !_resourceContext) {
      FML_DLOG(ERROR) << "Could not create Skia Metal contexts.";
      return nil;
    }

    _resourceContext->setResourceCacheLimit(0u);
  }
  return self;
}

- (sk_sp<GrDirectContext>)createGrContext {
  const auto contextOptions =
      clay::MakeDefaultContextOptions(clay::ContextType::kRender, GrBackendApi::kMetal);
  id<MTLDevice> device = _device;
  id<MTLCommandQueue> commandQueue = _commandQueue;
  return [FlutterDarwinContextMetalSkia createGrContext:device commandQueue:commandQueue];
}

+ (sk_sp<GrDirectContext>)createGrContext:(id<MTLDevice>)device
                             commandQueue:(id<MTLCommandQueue>)commandQueue {
  const auto contextOptions =
      clay::MakeDefaultContextOptions(clay::ContextType::kRender, GrBackendApi::kMetal);
  // Skia expect arguments to `MakeMetal` transfer ownership of the reference in for release later
  // when the GrDirectContext is collected.
  return GrDirectContext::MakeMetal((__bridge_retained void*)device,
                                    (__bridge_retained void*)commandQueue, contextOptions);
}

@end
