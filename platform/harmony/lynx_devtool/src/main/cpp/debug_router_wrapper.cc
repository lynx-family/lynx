// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_devtool/src/main/cpp/debug_router_wrapper.h"

#include <utility>

#include "base/include/fml/message_loop.h"
#include "base/include/log/logging.h"
#include "base/include/platform/harmony/napi_util.h"
#include "napi/native_api.h"
#include "platform/harmony/lynx_devtool/src/main/cpp/harmony_state_listener.h"
#include "third_party/debug_router/src/debug_router/native/core/debug_router_core.h"

namespace lynx {
namespace devtool {

std::map<napi_value, std::shared_ptr<HarmonyGlobalHandler>, NapiValueCompare>
    DebugRouterWrapper::global_handlers_;
std::map<napi_value, std::shared_ptr<HarmonySessionHandler>, NapiValueCompare>
    DebugRouterWrapper::session_handlers_;
std::map<std::string, std::shared_ptr<HarmonyDebugRouterMessageHandler>>
    DebugRouterWrapper::message_handlers_;

napi_value DebugRouterWrapper::Init(napi_env env, napi_value exports) {
  napi_property_descriptor properties[] = {
      {"addGlobalHandler", 0, DebugRouterWrapper::AddGlobalHandler, 0, 0, 0,
       napi_static, 0},
      {"removeGlobalHandler", 0, DebugRouterWrapper::RemoveGlobalHandler, 0, 0,
       0, napi_static, 0},
      {"addMessageHandler", 0, DebugRouterWrapper::AddMessageHandler, 0, 0, 0,
       napi_static, 0},
      {"removeMessageHandler", 0, DebugRouterWrapper::RemoveMessageHandler, 0,
       0, 0, napi_static, 0},
      {"sendDataAsync", 0, DebugRouterWrapper::SendDataAsync, 0, 0, 0,
       napi_static, 0},
      {"addSessionHandler", 0, DebugRouterWrapper::AddSessionHandler, 0, 0, 0,
       napi_static, 0},
      {"handleSchema", 0, DebugRouterWrapper::HandleSchema, 0, 0, 0,
       napi_static, 0},
      {"getAppInfoByKey", 0, DebugRouterWrapper::GetAppInfoByKey, 0, 0, 0,
       napi_static, 0},
      {"addStateListener", 0, DebugRouterWrapper::AddStateListener, 0, 0, 0,
       napi_static, 0},
  };
  constexpr size_t size = std::size(properties);

  napi_value cons;
  napi_status status =
      napi_define_class(env, "DebugRouterWrapper", NAPI_AUTO_LENGTH,
                        Constructor, nullptr, size, properties, &cons);

  status = napi_set_named_property(env, exports, "DebugRouterWrapper", cons);
  return exports;
}

napi_value DebugRouterWrapper::AddGlobalHandler(napi_env env,
                                                napi_callback_info info) {
  napi_value js_this;
  size_t argc = 1;
  napi_value argv[1];
  napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);
  if (global_handlers_.find(argv[0]) == global_handlers_.end()) {
    auto globalHandler = std::make_shared<HarmonyGlobalHandler>(env, argv[0]);
    global_handlers_[argv[0]] = globalHandler;
    debugrouter::common::DebugRouter::GetInstance().AddGlobalHandler(
        globalHandler.get());
  }
  return nullptr;
}

napi_value DebugRouterWrapper::RemoveGlobalHandler(napi_env env,
                                                   napi_callback_info info) {
  napi_value js_this;
  size_t argc = 1;
  napi_value argv[1];
  napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);
  if (global_handlers_.find(argv[0]) != global_handlers_.end()) {
    auto globalHandler = global_handlers_[argv[0]];
    debugrouter::common::DebugRouter::GetInstance().RemoveGlobalHandler(
        globalHandler.get());
    global_handlers_.erase(argv[0]);
  }
  return nullptr;
}

napi_value DebugRouterWrapper::AddMessageHandler(napi_env env,
                                                 napi_callback_info info) {
  napi_value js_this;
  size_t argc = 2;
  napi_value argv[2];
  napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);
  std::string handler_name = base::NapiUtil::ConvertToString(env, argv[0]);
  if (handler_name.empty()) {
    LOGE("DebugRouterWrapper::AddMessageHandler empty handler name");
    return nullptr;
  }
  auto existing = message_handlers_.find(handler_name);
  if (existing != message_handlers_.end()) {
    debugrouter::core::DebugRouterCore::GetInstance().RemoveMessageHandler(
        handler_name);
    message_handlers_.erase(existing);
  }

  auto message_handler = std::make_shared<HarmonyDebugRouterMessageHandler>(
      env, argv[1], handler_name);
  message_handlers_[handler_name] = message_handler;
  debugrouter::core::DebugRouterCore::GetInstance().AddMessageHandler(
      message_handler.get());
  return nullptr;
}

napi_value DebugRouterWrapper::RemoveMessageHandler(napi_env env,
                                                    napi_callback_info info) {
  napi_value js_this;
  size_t argc = 1;
  napi_value argv[1];
  napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);
  std::string handler_name = base::NapiUtil::ConvertToString(env, argv[0]);
  if (handler_name.empty()) {
    return nullptr;
  }
  auto it = message_handlers_.find(handler_name);
  if (it == message_handlers_.end()) {
    return nullptr;
  }
  debugrouter::core::DebugRouterCore::GetInstance().RemoveMessageHandler(
      handler_name);
  message_handlers_.erase(it);
  return nullptr;
}

napi_value DebugRouterWrapper::SendDataAsync(napi_env env,
                                             napi_callback_info info) {
  napi_value js_this;
  size_t argc = 3;
  napi_value argv[3];
  napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);
  std::string type = base::NapiUtil::ConvertToString(env, argv[0]);
  int32_t session;
  napi_get_value_int32(env, argv[1], &session);
  std::string data = base::NapiUtil::ConvertToString(env, argv[2]);
  debugrouter::common::DebugRouter::GetInstance().SendDataAsync(data, type,
                                                                session);
  return nullptr;
}

napi_value DebugRouterWrapper::AddSessionHandler(napi_env env,
                                                 napi_callback_info info) {
  napi_value js_this;
  size_t argc = 1;
  napi_value argv[1];
  napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);
  if (session_handlers_.find(argv[0]) == session_handlers_.end()) {
    auto sessionHandler = std::make_shared<HarmonySessionHandler>(env, argv[0]);
    session_handlers_[argv[0]] = sessionHandler;
    debugrouter::common::DebugRouter::GetInstance().AddSessionHandler(
        sessionHandler.get());
  }
  return nullptr;
}

napi_value DebugRouterWrapper::HandleSchema(napi_env env,
                                            napi_callback_info info) {
  napi_value js_this;
  size_t argc = 1;
  napi_value argv[1];
  napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);
  std::string url = base::NapiUtil::ConvertToString(env, argv[0]);
  bool handle_result =
      debugrouter::common::DebugRouter::GetInstance().HandleSchema(url);
  napi_value result = nullptr;
  napi_status ret = napi_get_boolean(env, handle_result, &result);
  if (ret != napi_ok) {
    LOGE("DebugRouterWrapper::HandleSchema failed to convert bool result");
    return nullptr;
  }
  return result;
}

napi_value DebugRouterWrapper::GetAppInfoByKey(napi_env env,
                                               napi_callback_info info) {
  napi_value js_this;
  size_t argc = 1;
  napi_value argv[1];
  napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);
  std::string key = base::NapiUtil::ConvertToString(env, argv[0]);
  std::string value =
      debugrouter::common::DebugRouter::GetInstance().GetAppInfoByKey(key);
  napi_value result;
  napi_create_string_utf8(env, value.c_str(), value.size(), &result);
  return result;
}

napi_value DebugRouterWrapper::AddStateListener(napi_env env,
                                                napi_callback_info info) {
  napi_value js_this;
  size_t argc = 1;
  napi_value argv[1];
  napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);
  auto stateListener = std::make_shared<HarmonyStateListener>(env, argv[0]);
  debugrouter::common::DebugRouter::GetInstance().AddStateListener(
      stateListener);
  return nullptr;
}

napi_value DebugRouterWrapper::Constructor(napi_env env,
                                           napi_callback_info info) {
  size_t argc = 0;
  napi_value args[1];
  napi_value js_this;
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);
  return js_this;
}

}  // namespace devtool
}  // namespace lynx
