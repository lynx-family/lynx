// Copyright 2026 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_MEMORY_JS_MEMORY_TRACK_SCOPE_H_
#define BASE_INCLUDE_FML_MEMORY_JS_MEMORY_TRACK_SCOPE_H_

#include <cassert>

#include "base/include/fml/memory/js_memory_track_define.h"
#include "base/include/fml/message_loop.h"
#include "base/include/fml/task_runner.h"

namespace lynx {
namespace fml {

struct JSMemoryTrackAsCommon {
  JSMemoryTrackAsCommon()
      : task_runner_(MessageLoop::GetCurrent().GetTaskRunner()) {
    auto& current_slot = task_runner_->GetCurrentAllocSlot();
    previous_slot_ = current_slot;
    current_slot = JSMemoryTrackSlotType::Common;
  }

  ~JSMemoryTrackAsCommon() {
    task_runner_->GetCurrentAllocSlot() = previous_slot_;
  }

  JSMemoryTrackAsCommon(const JSMemoryTrackAsCommon&) = delete;
  JSMemoryTrackAsCommon& operator=(const JSMemoryTrackAsCommon&) = delete;

 private:
  RefPtr<TaskRunner> task_runner_;
  int32_t previous_slot_;
};

struct JSMemoryTrackSlot {
  JSMemoryTrackSlot(TaskRunner* task_runner, int32_t slot)
      : task_runner_(task_runner) {
    assert(task_runner_ != nullptr);
    task_runner_->PushCurrentAllocSlot(slot);
  }

  ~JSMemoryTrackSlot() { task_runner_->PopCurrentAllocSlot(); }

  JSMemoryTrackSlot(const JSMemoryTrackSlot&) = delete;
  JSMemoryTrackSlot& operator=(const JSMemoryTrackSlot&) = delete;

 private:
  TaskRunner* task_runner_;
};

}  // namespace fml
}  // namespace lynx

#endif  // BASE_INCLUDE_FML_MEMORY_JS_MEMORY_TRACK_SCOPE_H_
