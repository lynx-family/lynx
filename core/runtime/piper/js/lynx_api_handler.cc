// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/piper/js/lynx_api_handler.h"

#include <string>
#include <utility>

#include "core/runtime/jsi/jsi.h"
#include "core/runtime/piper/js/lynx_runtime.h"

namespace lynx {
namespace runtime {

AnimationFrameTaskHandler::AnimationFrameTaskHandler()
    : current_index_(0), first_map_is_the_current_(true), doing_frame_(false) {}

int64_t AnimationFrameTaskHandler::RequestAnimationFrame(piper::Function func) {
  const int64_t task_id = current_index_++;
  std::unique_ptr<FrameTask> task =
      std::make_unique<FrameTask>(std::move(func), task_id);

  if (doing_frame_) {
    // avoid recursive function call of "RequestAnimationFrame"
    NextFrameTaskMap().insert_or_assign(task_id, std::move(task));
  } else {
    CurrentFrameTaskMap().insert_or_assign(task_id, std::move(task));
  }
  return task_id;
}

void AnimationFrameTaskHandler::CancelAnimationFrame(int64_t id) {
  auto itr = task_map_first_.find(id);
  if (itr != task_map_first_.end()) {
    itr->second->Cancel();
    return;
  }

  itr = task_map_second_.find(id);
  if (itr != task_map_second_.end()) {
    itr->second->Cancel();
  }
}

void AnimationFrameTaskHandler::DoFrame(int64_t time_stamp,
                                        piper::Runtime* rt) {
  doing_frame_ = true;
  TaskMap& task_map = CurrentFrameTaskMap();
  for (auto& itr : task_map) {
    itr.second->Execute(rt, time_stamp);
  }
  task_map.clear();

  // swap current task map and pending task map.
  first_map_is_the_current_ = !(first_map_is_the_current_);

  doing_frame_ = false;
}

void AnimationFrameTaskHandler::Destroy() {
  task_map_first_.clear();
  task_map_second_.clear();
}

bool AnimationFrameTaskHandler::HasPendingRequest() {
  return !task_map_first_.empty() || !task_map_second_.empty();
}

AnimationFrameTaskHandler::TaskMap&
AnimationFrameTaskHandler::CurrentFrameTaskMap() {
  if (first_map_is_the_current_) {
    return task_map_first_;
  }
  return task_map_second_;
}

AnimationFrameTaskHandler::TaskMap&
AnimationFrameTaskHandler::NextFrameTaskMap() {
  if (!first_map_is_the_current_) {
    return task_map_first_;
  }
  return task_map_second_;
}

AnimationFrameTaskHandler::FrameTask::FrameTask(piper::Function func,
                                                int64_t id)
    : func_(std::move(func)), cancelled_(false) {}

void AnimationFrameTaskHandler::FrameTask::Execute(piper::Runtime* rt,
                                                   int64_t time_stamp) {
  if (cancelled_) {
    return;
  }

  assert(rt != nullptr);
  if (rt) {
    piper::Scope scope(*rt);

    piper::Value time(static_cast<double>(time_stamp));
    const piper::Value args[1] = {std::move(time)};
    size_t count = 1;
    func_.call(*rt, args, count);
  }
}

void AnimationFrameTaskHandler::FrameTask::Cancel() { cancelled_ = true; }

LynxApiHandler::LynxApiHandler(LynxRuntime* rt) : rt_(rt) {}

}  // namespace runtime
}  // namespace lynx
