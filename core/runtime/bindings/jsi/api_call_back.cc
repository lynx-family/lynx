// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/bindings/jsi/api_call_back.h"

#include <utility>

#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/runtime/jsi/jsi.h"
#include "core/runtime/piper/js/lynx_runtime.h"

namespace lynx {
namespace piper {

ApiCallBack ApiCallBackManager::createCallbackImpl(piper::Function func) {
  int id = next_timer_index_++;
  const auto &callback = ApiCallBack(id);

  // Now ApiCallBack supports tracing with flow.
  // TRACE_EVENT_FLOW_BEGIN0 and TRACE_EVENT_FLOW_END0 is already
  // implemented in ApiCallBackManager.
  // If you want to trace ApiCallBack, you can use
  // `ctx.event()->add_flow_ids(callback.trace_flow_id());` to
  // add you trace in the lifecycle of ApiCallBack.
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ApiCallBackManager::createCallbackImpl",
              [=](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_flow_ids(callback.trace_flow_id());
              });

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
