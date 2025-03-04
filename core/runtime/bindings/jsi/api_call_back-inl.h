// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_API_CALL_BACK_INL_H_
#define CORE_RUNTIME_BINDINGS_JSI_API_CALL_BACK_INL_H_

#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"

namespace lynx {
namespace piper {

template <typename... Args>
inline void ApiCallBackManager::InvokeWithValue(piper::Runtime *rt,
                                                ApiCallBack callback,
                                                Args &&...values) {
  InvokeWithValuePersist(rt, callback, std::forward<Args>(values)...);
  callback_map_.erase(callback.id());
}

template <typename... Args>
inline void ApiCallBackManager::InvokeWithValuePersist(piper::Runtime *rt,
                                                       ApiCallBack callback,
                                                       Args &&...values) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ApiCallBackManager::InvokeWithValue",
              [&](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_terminating_flow_ids(callback.trace_flow_id());
              });

  DCHECK(rt);
  auto iter = callback_map_.find(callback.id());
  if (iter == callback_map_.end()) {
    LOGE("ApiCallBackManager::InvokeWithValue with illegal id:"
         << callback.id());
    return;
  }

  auto &holder = iter->second;
  DCHECK(holder);
  holder->InvokeWithValue(rt, std::forward<Args>(values)...);
}

template <typename... Args>
inline void CallBackHolder::InvokeWithValue(piper::Runtime *rt,
                                            Args &&...values) {
  DCHECK(rt);
  piper::Scope scope(*rt);
  function_.call(*rt, std::forward<Args>(values)...);
}

template <>
inline void CallBackHolder::InvokeWithValue<const lepus::Value &>(
    piper::Runtime *rt, const lepus::Value &value) {
  DCHECK(rt != nullptr);
  if (rt) {
    piper::Scope scope(*rt);
    if (value.IsNil()) {
      function_.call(*rt, nullptr, 0);
    } else {
      auto jsArgs = piper::valueFromLepus(*rt, value);
      if (jsArgs) {
        function_.call(*rt, *jsArgs);
      }
    }
  }
}
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_JSI_API_CALL_BACK_INL_H_
