// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/windows/egl/graphics_preparation.h"
#include "platform/embedder/lynx_env_platform.h"

namespace lynx {
namespace embedder {

bool PrewarmPlatformAsync() { return clay::egl::PrepareGraphicsAsync(); }

}  // namespace embedder
}  // namespace lynx
