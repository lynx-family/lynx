// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_PIPER_JS_LYNX_API_HANDLER_H_
#define CORE_RUNTIME_PIPER_JS_LYNX_API_HANDLER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/include/linked_hash_map.h"
#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace runtime {
class LynxRuntime;

// run on js thread
class AnimationFrameTaskHandler {
 public:
  AnimationFrameTaskHandler();
  int64_t RequestAnimationFrame(piper::Function func);
  void CancelAnimationFrame(int64_t id);
  void DoFrame(int64_t time_stamp, piper::Runtime* rt);
  void Destroy();
  bool HasPendingRequest();

 private:
  class FrameTask {
   public:
    FrameTask(piper::Function func, int64_t id);
    void Execute(piper::Runtime* rt, int64_t time_stamp);
    void Cancel();

   private:
    piper::Function func_;
    bool cancelled_;
  };
  using TaskMap = base::LinkedHashMap<int64_t, std::unique_ptr<FrameTask>>;
  TaskMap& CurrentFrameTaskMap();
  TaskMap& NextFrameTaskMap();
  int64_t current_index_;
  bool first_map_is_the_current_;
  bool doing_frame_;
  TaskMap task_map_first_;
  TaskMap task_map_second_;
};

// run on js thread
class LynxApiHandler {
 public:
  LynxApiHandler(LynxRuntime* rt);
  ~LynxApiHandler() = default;

 private:
  [[maybe_unused]] LynxRuntime* const rt_;
};

}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_PIPER_JS_LYNX_API_HANDLER_H_
