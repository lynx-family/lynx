// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_FRAME_TICK_FENCE_H_
#define PLATFORM_EMBEDDER_FRAME_TICK_FENCE_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>

#include "base/include/vsync_frame_tick_work.h"

namespace lynx {
namespace embedder {

class FrameTickFence {
 private:
  struct State;

 public:
  using CompleteCallback = std::function<void(uint64_t, int64_t)>;

  class Work final : public base::VSyncFrameTickWork {
   public:
    ~Work() override;
    void Complete() override;

   private:
    friend class FrameTickFence;
    explicit Work(std::shared_ptr<State> state) : state_(std::move(state)) {}
    std::shared_ptr<State> state_;
  };

  void SetCompleteCallback(CompleteCallback callback);
  void Begin(uint64_t frame_id, int64_t frame_start_time_ns);
  std::shared_ptr<base::VSyncFrameTickWork> AcquireWork(
      int64_t frame_start_time_ns, const char* source = nullptr);
  void Seal(int64_t frame_start_time_ns);

 private:
  std::mutex mutex_;
  std::shared_ptr<State> current_;
  CompleteCallback complete_callback_;
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_FRAME_TICK_FENCE_H_
