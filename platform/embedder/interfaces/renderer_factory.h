// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_INTERFACES_RENDERER_FACTORY_H_
#define PLATFORM_EMBEDDER_INTERFACES_RENDERER_FACTORY_H_

#include <memory>

#include "platform/embedder/interfaces/renderer_config.h"

namespace lynx::embedder {

// Selected at link time: an Engine-owned mock on mobile, a platform renderer
// in the desktop SDK, or the test renderer in unit tests.
std::unique_ptr<LynxUIRenderer> CreateDefaultUIRenderer(RendererConfig config);

}  // namespace lynx::embedder

#endif  // PLATFORM_EMBEDDER_INTERFACES_RENDERER_FACTORY_H_
