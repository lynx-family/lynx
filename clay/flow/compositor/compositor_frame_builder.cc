// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/compositor/compositor_frame_builder.h"

#include <algorithm>
#include <memory>
#include <utility>
namespace clay::display {
void CompositorFrameBuilder::Reset() {
  submitted_layout_ = last_layout_ = last_plan_ = last_render_ = 0;
}
void CompositorFrameBuilder::SetBinding(RasterBindingKey binding) {
  if (binding != binding_) {
    Reset();
    binding_ = binding;
  }
}
void CompositorFrameBuilder::SetOutputConfig(OutputConfig config) {
  if (config != config_) last_plan_ = 0;
  config_ = config;
}
void CompositorFrameBuilder::NotifyLayoutSubmitted(uint64_t version) {
  // Keep the latest complete snapshot eligible while newer trees rasterize.
  // Advancing the display watermark here can starve every frame when UI
  // submissions run ahead of Raster. Build advances it with complete output.
  submitted_layout_ = std::max(submitted_layout_, version);
}
std::shared_ptr<const CompositorFrame> CompositorFrameBuilder::Build(
    std::shared_ptr<const RasterResult> result, WindowKey window) {
  if (!result || !window.id || !window.generation || !config_.IsValid() ||
      result->binding != binding_ || result->config != config_ ||
      result->layout_version > submitted_layout_ ||
      result->layout_version < last_layout_ ||
      (result->layout_version == last_layout_ &&
       result->segment_plan_version < last_plan_) ||
      result->render_version <= last_render_)
    return nullptr;
  last_layout_ = result->layout_version;
  last_plan_ = result->segment_plan_version;
  last_render_ = result->render_version;
  return std::make_shared<const CompositorFrame>(
      CompositorFrame{++next_frame_, window, std::move(result)});
}
}  // namespace clay::display
