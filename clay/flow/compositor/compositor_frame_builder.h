// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_COMPOSITOR_COMPOSITOR_FRAME_BUILDER_H_
#define CLAY_FLOW_COMPOSITOR_COMPOSITOR_FRAME_BUILDER_H_
#include <memory>

#include "clay/flow/compositor/compositor_frame.h"
namespace clay::display {
// Platform-owned. A window generation is deliberately absent from RasterResult.
class CompositorFrameBuilder {
 public:
  void SetBinding(RasterBindingKey binding);
  void SetOutputConfig(OutputConfig config);
  // A submitted tree is pending work, not a replacement display snapshot.
  void NotifyLayoutSubmitted(uint64_t version);
  // Consumes a complete result validated by FrameMailbox. Only the active
  // Platform binding, configuration and submission watermarks remain to check.
  std::shared_ptr<const CompositorFrame> Build(
      std::shared_ptr<const RasterResult> result, WindowKey window);
  void Reset();

 private:
  RasterBindingKey binding_;
  OutputConfig config_;
  uint64_t submitted_layout_ = 0, last_layout_ = 0, last_plan_ = 0;
  uint64_t last_render_ = 0, next_frame_ = 0;
};
}  // namespace clay::display
#endif  // CLAY_FLOW_COMPOSITOR_COMPOSITOR_FRAME_BUILDER_H_
