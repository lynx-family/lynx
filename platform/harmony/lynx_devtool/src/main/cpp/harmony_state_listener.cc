// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_devtool/src/main/cpp/harmony_state_listener.h"

#include "base/include/fml/message_loop.h"
#include "base/include/platform/harmony/napi_util.h"
#include "napi/native_api.h"

namespace lynx {
namespace devtool {

HarmonyStateListener::HarmonyStateListener(napi_env env, napi_value js_this)
    : env_(env), js_this_ref_(nullptr) {
  napi_create_reference(env, js_this, 1, &js_this_ref_);
  napi_get_uv_event_loop(env, &loop_);
}

void HarmonyStateListener::OnOpen(debugrouter::common::ConnectionType type) {
  auto ui_task_runner =
      fml::MessageLoop::EnsureInitializedForCurrentThread(loop_)
          .GetTaskRunner();
  ui_task_runner->PostTask([weak_ptr = weak_from_this(), type]() {
    napi_value js_this;
    auto handler = weak_ptr.lock();
    if (!handler) {
      return;
    }
    napi_get_reference_value(handler->env_, handler->js_this_ref_, &js_this);
    napi_value onOpen;
    auto status =
        napi_get_named_property(handler->env_, js_this, "onOpen", &onOpen);

    napi_value args[1];
    std::string type_str = "";
    if (type == debugrouter::common::ConnectionType::WebSocket) {
      type_str = "websocket";
    } else if (type == debugrouter::common::ConnectionType::USB) {
      type_str = "usb";
    } else {
      type_str = "unknown";
    }
    napi_create_string_utf8(handler->env_, type_str.c_str(), NAPI_AUTO_LENGTH,
                            &args[0]);

    napi_value result;
    status =
        napi_call_function(handler->env_, js_this, onOpen, 1, args, &result);
  });
}

void HarmonyStateListener::OnClose(int32_t code, const std::string &reason) {
  auto ui_task_runner =
      fml::MessageLoop::EnsureInitializedForCurrentThread(loop_)
          .GetTaskRunner();
  ui_task_runner->PostTask([weak_ptr = weak_from_this(), code, reason]() {
    napi_value js_this;
    auto handler = weak_ptr.lock();
    if (!handler) {
      return;
    }
    napi_get_reference_value(handler->env_, handler->js_this_ref_, &js_this);
    napi_value onClose;
    auto status =
        napi_get_named_property(handler->env_, js_this, "onClose", &onClose);

    napi_value args[2];
    napi_create_int32(handler->env_, code, &args[0]);
    napi_create_string_utf8(handler->env_, reason.c_str(), NAPI_AUTO_LENGTH,
                            &args[1]);

    napi_value result;
    status =
        napi_call_function(handler->env_, js_this, onClose, 2, args, &result);
  });
}

void HarmonyStateListener::OnMessage(const std::string &message) {
  auto ui_task_runner =
      fml::MessageLoop::EnsureInitializedForCurrentThread(loop_)
          .GetTaskRunner();
  ui_task_runner->PostTask([weak_ptr = weak_from_this(), message]() {
    napi_value js_this;
    auto handler = weak_ptr.lock();
    if (!handler) {
      return;
    }
    napi_get_reference_value(handler->env_, handler->js_this_ref_, &js_this);
    napi_value onMessage;
    auto status = napi_get_named_property(handler->env_, js_this, "onMessage",
                                          &onMessage);

    napi_value args[1];
    napi_create_string_utf8(handler->env_, message.c_str(), NAPI_AUTO_LENGTH,
                            &args[0]);

    napi_value result;
    status =
        napi_call_function(handler->env_, js_this, onMessage, 1, args, &result);
  });
}

void HarmonyStateListener::OnError(const std::string &error) {
  auto ui_task_runner =
      fml::MessageLoop::EnsureInitializedForCurrentThread(loop_)
          .GetTaskRunner();
  ui_task_runner->PostTask([weak_ptr = weak_from_this(), error]() {
    napi_value js_this;
    auto handler = weak_ptr.lock();
    if (!handler) {
      return;
    }
    napi_get_reference_value(handler->env_, handler->js_this_ref_, &js_this);
    napi_value onError;
    auto status =
        napi_get_named_property(handler->env_, js_this, "onError", &onError);

    napi_value args[1];
    napi_create_string_utf8(handler->env_, error.c_str(), NAPI_AUTO_LENGTH,
                            &args[0]);

    napi_value result;
    status =
        napi_call_function(handler->env_, js_this, onError, 1, args, &result);
  });
}

}  // namespace devtool
}  // namespace lynx
