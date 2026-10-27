// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_COMPOSITOR_COMPOSITOR_FRAME_H_
#define CLAY_FLOW_COMPOSITOR_COMPOSITOR_FRAME_H_
#include <memory>
#include <vector>

#include "clay/common/graphics/output_texture.h"
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
};
struct RasterResult {
  RasterBindingKey binding;
  uint64_t render_version = 0, layout_version = 0, segment_plan_version = 0;
  OutputConfig config;
  std::vector<TextureEntry> outputs;
};
struct CompositorFrame {
  uint64_t frame_id = 0;
  WindowKey window;
  std::shared_ptr<const RasterResult> result;
};
struct DisplaySubmission {
  enum class OutputMode { kComposed, kForwarded, kNativeWindow };
  std::shared_ptr<const GPUCompletion> completion;
  std::shared_ptr<void> resources;
  bool presented = false;
  bool retry = false;
  // Window surfaces present directly. Other targets export an immutable output
  // or forward the original whole-page snapshot without changing its fence.
  std::shared_ptr<const OutputTexture> output;
  OutputMode output_mode = OutputMode::kComposed;
};
enum class PipelineError {
  kNone,
  kProducerFailure,
  kDisplayFailure,
  kGpuTimeout,
  kUnsupportedContent
};
}  // namespace clay::display
#endif  // CLAY_FLOW_COMPOSITOR_COMPOSITOR_FRAME_H_
