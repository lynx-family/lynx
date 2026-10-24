// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/frame_timing_listener_impl.h"

namespace lynx {
namespace embedder {

void FrameTimingListenerImpl::OnFrameTiming(int64_t frame_start_time_in_ns,
                                            int64_t frame_finish_time_in_ns) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  if (client_)
    client_->OnFrameTiming(frame_start_time_in_ns, frame_finish_time_in_ns);
}
void FrameTimingListenerImpl::SetClient(FrameTimingClient* client) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  client_ = client;
}

}  // namespace embedder
}  // namespace lynx
