// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/js/bindings/api_call_back.h"

#include <utility>

#include "base/include/fml/message_loop.h"
#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/runtime/js/jsi/jsi.h"
#include "core/runtime/trace/runtime_trace_event_def.h"

namespace lynx {
namespace runtime {
namespace js {
ApiCallBack ApiCallBackManager::createCallbackImpl(Function func) {
  int id = next_timer_index_++;
  const auto &callback = ApiCallBack(id);
  const auto alloc_slot =
      fml::MessageLoop::GetCurrent().GetTaskRunner()->GetCurrentAllocSlot();
  callback_map_.emplace(
      id, std::make_unique<CallBackHolder>(std::move(func), alloc_slot));
  return callback;
}

void ApiCallBackManager::EraseWithCallback(ApiCallBack callback) {
  callback_map_.erase(callback.id());
}

void ApiCallBackManager::Destroy() { callback_map_.clear(); }

CallBackHolder::CallBackHolder(Function func, int32_t alloc_slot)
    : function_(std::move(func)), alloc_slot_(alloc_slot) {}
}  // namespace js
}  // namespace runtime
}  // namespace lynx
