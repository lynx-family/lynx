// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_DISPLAY_TEXTURE_COMPOSITOR_H_
#define CLAY_SHELL_COMMON_DISPLAY_TEXTURE_COMPOSITOR_H_

#include <functional>
#include <memory>

#include "clay/flow/compositor/compositor_frame.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {
class OutputSurface;
namespace display {
struct RasterBuffer;

struct TextureComposition {
  struct Result {
    bool success = false;
    std::shared_ptr<const GPUCompletion> completion;
  };
  // The batch owns its imports until the caller retires the encoded frame.
  // A failed draw still returns its completion if GPU work was submitted.
  std::function<Result()> encode;
};

// Display owns composition policy and GPU encoding. Preparation never commits
// GPU work; an empty encoder leaves the caller free to use Canvas instead.
class TextureCompositor {
 public:
  virtual ~TextureCompositor() = default;
  virtual TextureComposition Prepare(const RasterBuffer& target,
                                     const RasterResult& result,
                                     OutputSurface& output) = 0;
};

std::unique_ptr<TextureCompositor> CreateTextureCompositor(
    GrContextPtr context);

}  // namespace display
}  // namespace clay

#endif  // CLAY_SHELL_COMMON_DISPLAY_TEXTURE_COMPOSITOR_H_
