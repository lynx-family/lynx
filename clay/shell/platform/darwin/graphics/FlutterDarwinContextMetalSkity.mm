// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "clay/shell/platform/darwin/graphics/FlutterDarwinContextMetalSkity.h"

#import "FlutterMacros.h"
#include "clay/common/graphics/persistent_cache.h"
#include "clay/fml/logging.h"
#include "clay/shell/common/context_options.h"

FLUTTER_ASSERT_ARC

@implementation FlutterDarwinContextMetalSkity

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
    _skityContext = [self createGPUContext];

    if (!_skityContext) {
      FML_DLOG(ERROR) << "Could not create Skity Metal contexts.";
      return nil;
    }
  }
  return self;
}

- (std::shared_ptr<skity::GPUContext>)createGPUContext {
  id<MTLDevice> device = _device;
  id<MTLCommandQueue> commandQueue = _commandQueue;
  return [FlutterDarwinContextMetalSkity createGPUContext:device commandQueue:commandQueue];
}

+ (std::shared_ptr<skity::GPUContext>)createGPUContext:(id<MTLDevice>)device
                                          commandQueue:(id<MTLCommandQueue>)commandQueue {
  return skity::MTLContextCreate(device, commandQueue);
}

@end
