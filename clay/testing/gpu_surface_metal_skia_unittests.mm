// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Metal/Metal.h>

#include "clay/shell/gpu/gpu_surface_metal_skia.h"
#include "third_party/skia/include/gpu/GrDirectContext.h"

namespace clay {
namespace testing {

using TestedMetalSurface = GPUSurfaceMetalSkia;
using TestedMetalContext = sk_sp<GrDirectContext>;

TestedMetalContext CreateTestedMetalContext(id<MTLDevice> device,
                                            id<MTLCommandQueue> command_queue) {
  return GrDirectContext::MakeMetal((__bridge_retained void*)device,
                                    (__bridge_retained void*)command_queue);
}

}  // namespace testing
}  // namespace clay

#include "clay/testing/gpu_surface_metal_unittests.h"
