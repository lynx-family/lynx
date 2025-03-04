// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_WORKLET_LEPUS_RAF_HANDLER_H_
#define CORE_RENDERER_WORKLET_LEPUS_RAF_HANDLER_H_

#include <memory>
#include <unordered_map>

#include "core/runtime/vm/lepus/lepus_value.h"
#include "third_party/binding/napi/shim/shim_napi.h"

namespace lynx {
namespace tasm {
class TemplateAssembler;
}  // namespace tasm

namespace worklet {

class LepusLynx;
class NapiFrameCallback;
class NapiFuncCallback;

class LepusAnimationFrameTaskHandler {
 public:
  LepusAnimationFrameTaskHandler();
  ~LepusAnimationFrameTaskHandler();
  int64_t RequestAnimationFrame(std::unique_ptr<NapiFrameCallback> callback);
  void CancelAnimationFrame(int64_t id);
  void DoFrame(int64_t time_stamp,
               std::shared_ptr<tasm::TemplateAssembler> tasm);
  void Destroy();
  bool HasPendingRequest();

 private:
  class FrameTask {
   public:
    FrameTask(std::unique_ptr<NapiFrameCallback> callback);

    void Execute(int64_t time_stamp,
                 std::shared_ptr<tasm::TemplateAssembler> tasm);
    void Cancel();

   private:
    std::unique_ptr<NapiFrameCallback> callback_;
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

class LepusApiHandler {
 public:
  LepusApiHandler() = default;
  ~LepusApiHandler();
  int64_t StoreTask(std::unique_ptr<NapiFuncCallback> callback);
  int64_t StoreTimedTask(std::unique_ptr<NapiFuncCallback> callback);
  void InvokeWithTaskID(int64_t task_id, const lepus::Value& value,
                        tasm::TemplateAssembler* tasm);
  void InvokeWithTaskID(int64_t task_id, Napi::Value value,
                        tasm::TemplateAssembler* tasm);
  void InvokeWithTimedTaskID(int64_t task_id, Napi::Value value,
                             tasm::TemplateAssembler* tasm);
  void RemoveTimeTask(int64_t task_id);
  void RemoveTimeTask();

  void Destroy();
  bool HasPendingCalling();

 private:
  class FuncTask {
   public:
    FuncTask(std::unique_ptr<NapiFuncCallback> callback);

    void Execute(const lepus::Value& value, tasm::TemplateAssembler* tasm);
    void Execute(Napi::Value value, tasm::TemplateAssembler* tasm);
    void Cancel();

   private:
    std::unique_ptr<NapiFuncCallback> callback_;
    bool cancelled_;
  };
  using LepusTaskMap = std::unordered_map<int64_t, std::unique_ptr<FuncTask>>;

  int64_t current_task_id_{0};

  LepusTaskMap lepus_task_map_;
  LepusTaskMap lepus_timed_task_map_;
};

}  // namespace worklet
}  // namespace lynx

#endif  // CORE_RENDERER_WORKLET_LEPUS_RAF_HANDLER_H_
