// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_LYNX_UI_RENDERER_FACTORY_H_
#define PLATFORM_EMBEDDER_LYNX_UI_RENDERER_FACTORY_H_

#include <memory>

#include "platform/embedder/public/capi/lynx_view_builder_capi.h"

namespace lynx::embedder {

class LynxUIRenderer;

// Engine-private entry point for converting Builder state and selecting a
// backend.
std::unique_ptr<LynxUIRenderer> CreateUIRenderer(lynx_view_builder_t* builder);

}  // namespace lynx::embedder

#endif  // PLATFORM_EMBEDDER_LYNX_UI_RENDERER_FACTORY_H_
