// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/js/bindings/api_call_back.h"

#include <utility>

#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/runtime/js/jsi/jsi.h"
#include "core/runtime/trace/runtime_trace_event_def.h"

namespace lynx {
namespace piper {

ApiCallBack ApiCallBackManager::createCallbackImpl(piper::Function func) {
  int id = next_timer_index_++;
  const auto &callback = ApiCallBack(id);
  std::shared_ptr<CallBackHolder> holder =
      std::make_shared<CallBackHolder>(std::move(func));
  callback_map_.insert(std::make_pair(id, holder));
  return callback;
}

void ApiCallBackManager::EraseWithCallback(ApiCallBack callback) {
  callback_map_.erase(callback.id());
}

void ApiCallBackManager::Destroy() { callback_map_.clear(); }

CallBackHolder::CallBackHolder(piper::Function func)
    : function_(std::move(func)) {}
}  // namespace piper
}  // namespace lynx
