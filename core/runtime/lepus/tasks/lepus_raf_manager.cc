// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/tasks/lepus_raf_manager.h"

#include <utility>

#include "base/include/value/base_value.h"
#include "base/trace/native/trace_event.h"
#include "core/runtime/lepus/context.h"
#include "core/runtime/trace/runtime_trace_event_def.h"

namespace lynx {
namespace tasm {

AnimationFrameManager::FrameTask::FrameTask(
    lepus::Context* context, std::unique_ptr<lepus::Value> callback_closure)
    : callback_closure_(std::move(callback_closure)),
      context_(context),
      cancelled_(false) {}

void AnimationFrameManager::FrameTask::Execute(int64_t time_stamp) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ANIMATION_FRAME_TASK_EXECUTE);
  if (cancelled_) {
    return;
  }

  context_->CallClosure(*callback_closure_, lepus::Value(time_stamp));
}

void AnimationFrameManager::FrameTask::Cancel() { cancelled_ = true; }

AnimationFrameManager::AnimationFrameManager()
    : current_index_(0), first_map_is_the_current_(true), doing_frame_(false) {}

AnimationFrameManager::~AnimationFrameManager() { Destroy(); }

int64_t AnimationFrameManager::RequestAnimationFrame(
    lepus::Context* context, std::unique_ptr<lepus::Value> callback_closure) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              ANIMATION_FRAME_TASK_REQUEST_ANIMATION_FRAME);
  const int64_t task_id = current_index_++;
  std::unique_ptr<FrameTask> task =
      std::make_unique<FrameTask>(context, std::move(callback_closure));

  if (doing_frame_) {
    // avoid recursive function call of "RequestAnimationFrame"
    NextFrameTaskMap().insert(std::make_pair(task_id, std::move(task)));
  } else {
    CurrentFrameTaskMap().insert(std::make_pair(task_id, std::move(task)));
  }
  return task_id;
}

void AnimationFrameManager::CancelAnimationFrame(int64_t id) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ANIMATION_FRAME_TASK_CANCEL_ANIMATION_FRAME);
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

void AnimationFrameManager::DoFrame(int64_t time_stamp) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ANIMATION_FRAME_TASK_DO_FRAME);
  doing_frame_ = true;
  TaskMap& task_map = CurrentFrameTaskMap();
  for (auto& itr : task_map) {
    itr.second->Execute(time_stamp);
  }
  task_map.clear();

  // swap current task map and pending task map.
  first_map_is_the_current_ = !(first_map_is_the_current_);
  doing_frame_ = false;
}

void AnimationFrameManager::Destroy() {
  task_map_first_.clear();
  task_map_second_.clear();
}

bool AnimationFrameManager::HasPendingRequest() {
  return !task_map_first_.empty() || !task_map_second_.empty();
}

AnimationFrameManager::TaskMap& AnimationFrameManager::CurrentFrameTaskMap() {
  if (first_map_is_the_current_) {
    return task_map_first_;
  }
  return task_map_second_;
}

AnimationFrameManager::TaskMap& AnimationFrameManager::NextFrameTaskMap() {
  if (!first_map_is_the_current_) {
    return task_map_first_;
  }
  return task_map_second_;
}

}  // namespace tasm
}  // namespace lynx
