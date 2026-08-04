// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Metal/Metal.h>

#include "clay/shell/gpu/gpu_surface_metal_skity.h"
#include "skity/gpu/gpu_context_mtl.h"

namespace clay {
namespace testing {

using TestedMetalSurface = GPUSurfaceMetalSkity;
using TestedMetalContext = std::shared_ptr<skity::GPUContext>;

TestedMetalContext CreateTestedMetalContext(id<MTLDevice> device,
                                            id<MTLCommandQueue> command_queue) {
  return skity::MTLContextCreate(device, command_queue);
}

}  // namespace testing
}  // namespace clay

#include "clay/testing/gpu_surface_metal_unittests.h"
