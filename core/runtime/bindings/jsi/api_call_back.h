// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_BINDINGS_JSI_API_CALL_BACK_H_
#define CORE_RUNTIME_BINDINGS_JSI_API_CALL_BACK_H_

#include <memory>
#include <unordered_map>

#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/runtime/common/utils.h"
#include "core/runtime/jsi/jsi.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace runtime {
class LynxRuntime;
}
namespace piper {
class Runtime;

class ApiCallBack {
 public:
  ApiCallBack(int id = -1) {
    id_ = id;
    trace_flow_id_ = TRACE_FLOW_ID();
  }
  int id() const { return id_; }
  bool IsValid() const { return id_ != -1; }
  uint64_t trace_flow_id() const { return trace_flow_id_; }

 private:
  int id_;
  uint64_t trace_flow_id_;
};

class CallBackHolder;

class ApiCallBackManager {
 public:
  ApiCallBackManager() : next_timer_index_(0) {}
  ApiCallBack createCallbackImpl(piper::Function func);

  template <typename... Args>
  void InvokeWithValue(piper::Runtime* rt, ApiCallBack callback,
                       Args&&... values);

  /**
   * Invoke Js ApiCallBack but does not erase ApiCallBack from callback_map_
   */
  template <typename... Args>
  void InvokeWithValuePersist(piper::Runtime* rt, ApiCallBack callback,
                              Args&&... values);

  void EraseWithCallback(ApiCallBack callback);
  void Destroy();

 private:
  std::unordered_map<int, std::shared_ptr<CallBackHolder>> callback_map_;
  int next_timer_index_;
};

class CallBackHolder : public std::enable_shared_from_this<CallBackHolder> {
 public:
  CallBackHolder(piper::Function func);

  ~CallBackHolder() = default;

  template <typename... Args>
  void InvokeWithValue(piper::Runtime* rt, Args&&... values);

 private:
  piper::Function function_;
};

}  // namespace piper
}  // namespace lynx

#include "core/runtime/bindings/jsi/api_call_back-inl.h"

#endif  // CORE_RUNTIME_BINDINGS_JSI_API_CALL_BACK_H_
