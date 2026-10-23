// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_FRAME_TIMING_LISTENER_IMPL_H_
#define PLATFORM_EMBEDDER_FRAME_TIMING_LISTENER_IMPL_H_

#include <mutex>

#include "clay/shell/common/frame_timing_listener.h"
#include "platform/embedder/interfaces/renderer_config.h"

namespace lynx {
namespace embedder {

class FrameTimingListenerImpl : public clay::FrameTimingListener {
 public:
  ~FrameTimingListenerImpl() override = default;

  void OnFrameTiming(int64_t frame_start_time_in_ns,
                     int64_t frame_finish_time_in_ns) override;

  void SetClient(FrameTimingClient* client);

 private:
  std::recursive_mutex mutex_;
  FrameTimingClient* client_ = nullptr;
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_FRAME_TIMING_LISTENER_IMPL_H_
