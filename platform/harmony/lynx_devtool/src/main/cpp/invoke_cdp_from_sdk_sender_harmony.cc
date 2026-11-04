// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_devtool/src/main/cpp/invoke_cdp_from_sdk_sender_harmony.h"

#include <uv.h>

#include "base/include/fml/message_loop.h"
#include "base/include/log/logging.h"
#include "base/include/platform/harmony/napi_util.h"

namespace lynx {
namespace devtool {

InvokeCDPFromSDKSenderHarmony::InvokeCDPFromSDKSenderHarmony(
    napi_env env, napi_value callback)
    : env_(env), callback_ref_(nullptr) {
  napi_create_reference(env_, callback, 1, &callback_ref_);
  uv_loop_t* loop = nullptr;
  if (napi_get_uv_event_loop(env_, &loop) == napi_ok && loop != nullptr) {
    ui_task_runner_ = fml::MessageLoop::EnsureInitializedForCurrentThread(loop)
                          .GetTaskRunner();
  }
}

InvokeCDPFromSDKSenderHarmony::~InvokeCDPFromSDKSenderHarmony() {
  if (env_ != nullptr && callback_ref_ != nullptr) {
    napi_delete_reference(env_, callback_ref_);
    callback_ref_ = nullptr;
  }
}

void InvokeCDPFromSDKSenderHarmony::SendMessage(const std::string& type,
                                                const Json::Value& msg) {
  DispatchResult(msg.toStyledString());
}

void InvokeCDPFromSDKSenderHarmony::SendMessage(const std::string& type,
                                                const std::string& msg) {
  DispatchResult(msg);
}

void InvokeCDPFromSDKSenderHarmony::DispatchResult(const std::string& result) {
  if (ui_task_runner_ == nullptr) {
    return;
  }
  // Keep the sender alive until the posted task runs, otherwise a CDP response
  // arriving right before the dispatch finishes could be dropped.
  ui_task_runner_->PostTask([self = shared_from_this(), result]() {
    if (self->env_ == nullptr || self->callback_ref_ == nullptr) {
      return;
    }
    base::NapiHandleScope scope(self->env_);
    napi_value callback = nullptr;
    napi_status status =
        napi_get_reference_value(self->env_, self->callback_ref_, &callback);
    if (status != napi_ok || callback == nullptr) {
      return;
    }
    napi_value undefined = nullptr;
    napi_get_undefined(self->env_, &undefined);
    napi_value param[1];
    napi_create_string_utf8(self->env_, result.c_str(), result.length(),
                            &param[0]);
    status =
        napi_call_function(self->env_, undefined, callback, 1, param, nullptr);
    if (status != napi_ok) {
      LOGE("InvokeJsMethod CDP result callback failed");
    }
    // A CDP method only expects one response, release the callback right away.
    napi_delete_reference(self->env_, self->callback_ref_);
    self->callback_ref_ = nullptr;
  });
}

}  // namespace devtool
}  // namespace lynx
