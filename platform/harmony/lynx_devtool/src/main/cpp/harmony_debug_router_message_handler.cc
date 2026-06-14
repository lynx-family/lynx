// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_devtool/src/main/cpp/harmony_debug_router_message_handler.h"

#include <utility>

#include "base/include/fml/message_loop.h"
#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/fml/time/time_delta.h"
#include "base/include/log/logging.h"
#include "base/include/platform/harmony/napi_util.h"

namespace lynx {
namespace devtool {

HarmonyDebugRouterMessageHandler::HarmonyDebugRouterMessageHandler(
    napi_env env, napi_value js_object, std::string handler_name)
    : env_(env),
      js_this_ref_(nullptr),
      loop_(nullptr),
      handler_name_(std::move(handler_name)) {
  napi_create_reference(env_, js_object, 1, &js_this_ref_);
  napi_get_uv_event_loop(env_, &loop_);
}

std::string HarmonyDebugRouterMessageHandler::Handle(std::string params) {
  return InvokeHandleMethod(params);
}

std::string HarmonyDebugRouterMessageHandler::GetName() const {
  return handler_name_;
}

std::string HarmonyDebugRouterMessageHandler::InvokeHandleMethod(
    const std::string& params) {
  auto ui_task_runner =
      fml::MessageLoop::EnsureInitializedForCurrentThread(loop_)
          .GetTaskRunner();
  if (ui_task_runner == nullptr) {
    LOGE("HarmonyDebugRouterMessageHandler missing task runner: handle");
    return "";
  }

  struct InvokeResult {
    fml::AutoResetWaitableEvent event;
    std::string value;
  };

  auto invoke_result = std::make_shared<InvokeResult>();
  ui_task_runner->PostTask([weak_ptr = weak_from_this(), invoke_result,
                            params]() {
    auto handler = weak_ptr.lock();
    if (handler == nullptr) {
      invoke_result->event.Signal();
      return;
    }

    invoke_result->value = handler->InvokeHandleMethodOnCurrentThread(params);
    invoke_result->event.Signal();
  });

  if (invoke_result->event.WaitWithTimeout(fml::TimeDelta::FromSeconds(5))) {
    LOGE("HarmonyDebugRouterMessageHandler invoke timeout: handle");
    return "";
  }
  return invoke_result->value;
}

std::string HarmonyDebugRouterMessageHandler::InvokeHandleMethodOnCurrentThread(
    const std::string& params) {
  napi_value args[1];
  napi_value result = nullptr;
  napi_create_string_utf8(env_, params.c_str(), NAPI_AUTO_LENGTH, &args[0]);
  napi_status status = base::NapiUtil::InvokeJsMethod(
      env_, js_this_ref_, "handle", 1, args, &result);
  if (status != napi_ok || result == nullptr) {
    LOGE("HarmonyDebugRouterMessageHandler invoke failed: handle");
    return "";
  }
  return base::NapiUtil::ConvertToString(env_, result);
}

}  // namespace devtool
}  // namespace lynx
