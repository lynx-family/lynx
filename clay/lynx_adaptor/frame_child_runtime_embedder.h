// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_LYNX_ADAPTOR_FRAME_CHILD_RUNTIME_EMBEDDER_H_
#define CLAY_LYNX_ADAPTOR_FRAME_CHILD_RUNTIME_EMBEDDER_H_

#include <memory>

#include "platform/embedder/core/lynx_template_renderer.h"

namespace lynx {
namespace tasm {

class FrameChildRuntimeFactory;

std::shared_ptr<FrameChildRuntimeFactory>
CreateFrameChildRuntimeFactoryEmbedder(
    embedder::LynxTemplateRenderer::Settings settings);

}  // namespace tasm
}  // namespace lynx

#endif  // CLAY_LYNX_ADAPTOR_FRAME_CHILD_RUNTIME_EMBEDDER_H_
