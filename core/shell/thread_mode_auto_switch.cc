// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/thread_mode_auto_switch.h"

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/base/threading/task_runner_manufactor.h"

namespace lynx {
namespace shell {

ThreadModeAutoSwitch::ThreadModeAutoSwitch(ThreadModeManager& manager) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "ThreadModeAutoSwitch::ThreadModeAutoSwitch");
  if (!manager) {
    return;
  }

  manager_ = &manager;

  // mark manager to be held.
  manager_->is_held = true;

  merger_ = std::make_optional<base::ThreadMerger>(manager.ui_runner,
                                                   manager.engine_runner);

  // transfer the queue after the threads have merged.
  manager.queue->Transfer(base::ThreadStrategyForRendering::PART_ON_LAYOUT);
}

ThreadModeAutoSwitch::~ThreadModeAutoSwitch() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "ThreadModeAutoSwitch::~ThreadModeAutoSwitch");

  if (manager_ == nullptr) {
    return;
  }

  // transfer the queue before the threads have unmerged.
  // The threads will unmerge when merger_ release.
  manager_->queue->Transfer(base::ThreadStrategyForRendering::MULTI_THREADS);

  // mark manager to be unheld.
  manager_->is_held = false;
}

}  // namespace shell
}  // namespace lynx
