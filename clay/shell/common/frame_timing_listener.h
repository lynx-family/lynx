// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_FRAME_TIMING_LISTENER_H_
#define CLAY_SHELL_COMMON_FRAME_TIMING_LISTENER_H_

#include <cstdint>

namespace clay {

class FrameTimingListener {
 public:
  virtual ~FrameTimingListener() = default;

  virtual void OnFrameTiming(int64_t frame_start_time_in_ns,
                             int64_t frame_finish_time_in_ns) = 0;
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_FRAME_TIMING_LISTENER_H_
