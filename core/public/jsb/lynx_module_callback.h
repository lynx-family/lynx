// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_JSB_LYNX_MODULE_CALLBACK_H_
#define CORE_PUBLIC_JSB_LYNX_MODULE_CALLBACK_H_

#include <memory>
#include <unordered_map>

#include "core/public/pub_value.h"

namespace lynx {
namespace piper {

class LynxModuleCallback {
 public:
  explicit LynxModuleCallback(int64_t callback_id)
      : callback_id_(callback_id) {}

  virtual ~LynxModuleCallback() = default;

  // Set callback args before invoking callback
  virtual void SetArgs(std::unique_ptr<pub::Value> args) = 0;
  void SetCallbackFlowId(uint64_t flow_id) { callback_flow_id_ = flow_id; }
  uint64_t CallbackFlowId() const { return callback_flow_id_; }
  uint64_t CallbackId() const { return callback_id_; }

 protected:
  const int64_t callback_id_;
  // We need callback flow id to bind CallJSB and InvokeCallback in tracing.
  uint64_t callback_flow_id_ = 0;
};

// The key of CallbackMap represents the index at which the callback is located
// among all parameters.
using CallbackMap =
    std::unordered_map<int64_t, std::shared_ptr<LynxModuleCallback>>;

}  // namespace piper
}  // namespace lynx

#endif  // CORE_PUBLIC_JSB_LYNX_MODULE_CALLBACK_H_
