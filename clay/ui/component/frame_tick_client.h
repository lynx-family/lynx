// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_FRAME_TICK_CLIENT_H_
#define CLAY_UI_COMPONENT_FRAME_TICK_CLIENT_H_

#include "base/include/fml/time/time_point.h"

namespace clay {

struct FrameTickInfo {
  fml::TimePoint vsync_start;
  fml::TimePoint vsync_target;
  int vsync_sequence_id = -1;
  bool parent_forced = false;
};

class FrameTickClient {
 public:
  virtual ~FrameTickClient() = default;

  virtual bool BeginScheduledFrame(const FrameTickInfo& tick, bool forced) = 0;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_FRAME_TICK_CLIENT_H_
