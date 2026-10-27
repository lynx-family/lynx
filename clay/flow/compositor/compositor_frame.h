// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_COMPOSITOR_COMPOSITOR_FRAME_H_
#define CLAY_FLOW_COMPOSITOR_COMPOSITOR_FRAME_H_
#include <memory>
#include <optional>
#include <vector>

#include "clay/gfx/shared_image/output_texture.h"
namespace clay::display {
using SegmentId = uint64_t;
struct SegmentPlacement {
  SegmentId segment_id = 0;
  CompositionProperties properties;
};
struct PageSceneSnapshot {
  RasterBindingKey binding;
  uint64_t layout_version = 0, segment_plan_version = 0;
  OutputConfig config;
  std::vector<SegmentPlacement> ordered_segments;
};
struct TextureEntry {
  SegmentId segment_id = 0;
  uint64_t content_version = 0;
  std::shared_ptr<const OutputTexture> texture_ref;
  CompositionProperties properties;
  std::optional<MediaTiming> media_timing;
};
struct RasterResult {
  RasterBindingKey binding;
  uint64_t render_version = 0, layout_version = 0, segment_plan_version = 0;
  OutputConfig config;
  std::vector<TextureEntry> outputs;
  TimingCorrelation timing;
};
struct CompositorFrame {
  uint64_t frame_id = 0;
  WindowKey window;
  std::shared_ptr<const RasterResult> result;
};
enum class PipelineError {
  kNone,
  kProducerFailure,
  kDisplayFailure,
  kGpuTimeout,
  kUnsupportedContent
};
struct PresentationFeedback {
  enum class Status { kUnavailable, kPresented, kFailed };
  Status status = Status::kUnavailable;
  int64_t actual_present_time_ns = 0;
};
}  // namespace clay::display
#endif  // CLAY_FLOW_COMPOSITOR_COMPOSITOR_FRAME_H_
