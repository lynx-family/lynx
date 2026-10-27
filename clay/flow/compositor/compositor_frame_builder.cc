// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/compositor/compositor_frame_builder.h"

#include <algorithm>
#include <memory>
#include <unordered_set>
#include <utility>
namespace clay::display {
void CompositorFrameBuilder::Reset() {
  scene_.reset();
  latest_.reset();
  submitted_layout_ = expected_layout_ = last_render_ = 0;
}
void CompositorFrameBuilder::SetBinding(RasterBindingKey binding) {
  if (binding != binding_) {
    Reset();
    binding_ = binding;
  }
}
void CompositorFrameBuilder::SetOutputConfig(OutputConfig config) {
  if (config != config_) {
    scene_.reset();
    latest_.reset();
    config_ = config;
  }
}
void CompositorFrameBuilder::NotifyLayoutSubmitted(uint64_t version) {
  // Keep the latest complete snapshot eligible while newer trees rasterize.
  // Advancing the display watermark here can starve every frame when UI
  // submissions run ahead of Raster. SetScene advances it with complete output.
  submitted_layout_ = std::max(submitted_layout_, version);
}
bool CompositorFrameBuilder::SetScene(
    std::shared_ptr<const PageSceneSnapshot> scene) {
  if (!scene || scene->binding != binding_ || scene->config != config_ ||
      !scene->layout_version || scene->layout_version > submitted_layout_ ||
      scene->layout_version < expected_layout_ ||
      !scene->segment_plan_version || scene->ordered_segments.empty() ||
      scene->ordered_segments.size() > 8)
    return false;
  std::unordered_set<SegmentId> ids;
  for (auto& segment : scene->ordered_segments)
    if (!segment.segment_id || !ids.insert(segment.segment_id).second)
      return false;
  if (scene_ && scene->layout_version == scene_->layout_version &&
      scene->segment_plan_version < scene_->segment_plan_version)
    return false;
  if (latest_ && (latest_->layout_version != scene->layout_version ||
                  latest_->segment_plan_version != scene->segment_plan_version))
    latest_.reset();
  expected_layout_ = scene->layout_version;
  scene_ = std::move(scene);
  return true;
}
bool CompositorFrameBuilder::Accept(
    std::shared_ptr<const RasterResult> result) {
  if (!scene_ || !result || !config_.IsValid() || result->binding != binding_ ||
      result->config != config_ || result->layout_version != expected_layout_ ||
      result->segment_plan_version != scene_->segment_plan_version ||
      result->render_version <= last_render_ ||
      result->outputs.size() != scene_->ordered_segments.size())
    return false;
  for (size_t i = 0; i < result->outputs.size(); ++i) {
    const auto& entry = result->outputs[i];
    const auto& placement = scene_->ordered_segments[i];
    if (entry.segment_id != placement.segment_id || !entry.texture_ref ||
        !entry.texture_ref->ready() ||
        entry.content_version != entry.texture_ref->content_version() ||
        entry.properties.x != placement.properties.x ||
        entry.properties.y != placement.properties.y)
      return false;
    const auto& key = entry.texture_ref->resource()->key();
    const auto& image = entry.texture_ref->image();
    if (key.device_id != config_.device_id ||
        key.device_generation != config_.device_generation ||
        image.format != config_.format || image.alpha != config_.alpha ||
        image.color != config_.color || !image.width || !image.height ||
        !key.resource_id || !key.allocation_generation ||
        image.valid_rect.x < 0 || image.valid_rect.y < 0 ||
        !image.valid_rect.width || !image.valid_rect.height ||
        static_cast<uint64_t>(image.valid_rect.x) + image.valid_rect.width >
            image.width ||
        static_cast<uint64_t>(image.valid_rect.y) + image.valid_rect.height >
            image.height)
      return false;
  }
  last_render_ = result->render_version;
  latest_ = std::move(result);
  return true;
}
std::shared_ptr<const CompositorFrame> CompositorFrameBuilder::Build(
    WindowKey window) {
  if (!latest_ || !window.id || !window.generation) return nullptr;
  return std::make_shared<const CompositorFrame>(
      CompositorFrame{++next_frame_, window, std::exchange(latest_, {})});
}
}  // namespace clay::display
