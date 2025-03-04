// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_VM_LEPUS_TASKS_LEPUS_RAF_MANAGER_H_
#define CORE_RUNTIME_VM_LEPUS_TASKS_LEPUS_RAF_MANAGER_H_

#include <memory>
#include <unordered_map>

#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {
class TemplateAssembler;

class AnimationFrameManager {
 public:
  AnimationFrameManager();
  ~AnimationFrameManager();
  int64_t RequestAnimationFrame(lepus::Context* context,
                                std::unique_ptr<lepus::Value> callback_closure);
  void CancelAnimationFrame(int64_t id);
  void DoFrame(int64_t time_stamp);
  void Destroy();
  bool HasPendingRequest();

 private:
  class FrameTask {
   public:
    FrameTask(lepus::Context* context, std::unique_ptr<lepus::Value> closure);

    void Execute(int64_t time_stamp);
    void Cancel();

   private:
    std::unique_ptr<lepus::Value> callback_closure_;
    lepus::Context* context_;
    bool cancelled_;
  };
  using TaskMap = std::unordered_map<int64_t, std::unique_ptr<FrameTask>>;

  TaskMap& CurrentFrameTaskMap();
  TaskMap& NextFrameTaskMap();

  int64_t current_index_;

  // When first_map_is_the_current_ is true, it indicates that the new task
  // needs to be stored in task_map_first_. Otherwisr, it needs to be stored in
  // task_map_second_.
  bool first_map_is_the_current_;

  // When raf tasks are executed, doing_frame_ is true. Otherwisr, it is flase.
  bool doing_frame_;

  // Store two task maps, one of the maps will be used during the current raf
  // execution, and the new tasks will be stored in the other map.
  TaskMap task_map_first_;
  TaskMap task_map_second_;
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_TASKS_LEPUS_RAF_MANAGER_H_
