// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_devtool/src/main/cpp/cdp_event_listener_sender_harmony.h"

#include <uv.h>

#include "base/include/fml/message_loop.h"
#include "base/include/log/logging.h"
#include "base/include/platform/harmony/napi_util.h"

namespace lynx {
namespace devtool {

CDPEventListenerSenderHarmony::CDPEventListenerSenderHarmony(
    napi_env env, napi_value listener)
    : env_(env), listener_ref_(nullptr) {
  napi_create_reference(env_, listener, 0, &listener_ref_);
  uv_loop_t* loop = nullptr;
  if (napi_get_uv_event_loop(env_, &loop) == napi_ok && loop != nullptr) {
    ui_task_runner_ = fml::MessageLoop::EnsureInitializedForCurrentThread(loop)
                          .GetTaskRunner();
  }
}

CDPEventListenerSenderHarmony::~CDPEventListenerSenderHarmony() {
  if (env_ != nullptr && listener_ref_ != nullptr) {
    napi_delete_reference(env_, listener_ref_);
    listener_ref_ = nullptr;
  }
}

void CDPEventListenerSenderHarmony::SendMessage(const std::string& type,
                                                const Json::Value& msg) {
  DispatchEvent(msg.toStyledString());
}

void CDPEventListenerSenderHarmony::SendMessage(const std::string& type,
                                                const std::string& msg) {
  DispatchEvent(msg);
}

void CDPEventListenerSenderHarmony::DispatchEvent(const std::string& event) {
  if (ui_task_runner_ == nullptr) {
    return;
  }
  ui_task_runner_->PostTask([weak_self = weak_from_this(), event]() {
    auto self = weak_self.lock();
    if (self == nullptr || self->env_ == nullptr ||
        self->listener_ref_ == nullptr) {
      return;
    }
    base::NapiHandleScope scope(self->env_);
    napi_value listener = nullptr;
    napi_status status =
        napi_get_reference_value(self->env_, self->listener_ref_, &listener);
    // The weak reference becomes invalid once the listener is collected.
    if (status != napi_ok || listener == nullptr) {
      return;
    }
    napi_value param[1];
    napi_create_string_utf8(self->env_, event.c_str(), event.length(),
                            &param[0]);
    status = base::NapiUtil::InvokeJsMethod(self->env_, self->listener_ref_,
                                            "onEvent", 1, param);
    if (status != napi_ok) {
      LOGE("InvokeJsMethod onEvent failed");
    }
  });
}

}  // namespace devtool
}  // namespace lynx
