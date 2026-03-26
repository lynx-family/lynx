// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_FRAME_TIMING_LISTENER_IMPL_H_
#define PLATFORM_EMBEDDER_FRAME_TIMING_LISTENER_IMPL_H_

#include <list>

#include "clay/shell/common/frame_timing_listener.h"

namespace lynx {
namespace embedder {

class LynxViewClients;

class FrameTimingListenerImpl : public clay::FrameTimingListener {
 public:
  ~FrameTimingListenerImpl() override = default;

  void OnFrameTiming(int64_t frame_start_time_in_ns,
                     int64_t frame_finish_time_in_ns) override;

  void AddClient(LynxViewClients* client);
  void RemoveAllClients();

 private:
  std::list<LynxViewClients*> clients_;
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_FRAME_TIMING_LISTENER_IMPL_H_
