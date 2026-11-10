// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_devtool/src/main/cpp/inspector_owner_harmony.h"

#include <js_native_api.h>

#include <memory>
#include <string>
#include <vector>

#include "base/include/platform/harmony/napi_util.h"
#include "platform/harmony/lynx_devtool/src/main/cpp/cdp_event_listener_sender_harmony.h"
#include "platform/harmony/lynx_devtool/src/main/cpp/invoke_cdp_from_sdk_sender_harmony.h"
#include "platform/harmony/lynx_devtool/src/main/cpp/message_handler_harmony.h"

namespace lynx {
namespace devtool {

napi_value InspectorOwnerHarmony::Init(napi_env env, napi_value exports) {
#define DECLARE_NAPI_FUNCTION(name, func) \
  {(name), nullptr, (func), nullptr, nullptr, nullptr, napi_default, nullptr}
  napi_property_descriptor properties[] = {
      DECLARE_NAPI_FUNCTION("attachProxy", AttachProxy),
      DECLARE_NAPI_FUNCTION("destroy", Destroy),
      DECLARE_NAPI_FUNCTION("getSessionId", GetSessionId),
      DECLARE_NAPI_FUNCTION("flushConsoleMessages", FlushConsoleMessages),
      DECLARE_NAPI_FUNCTION("getConsoleObject", GetConsoleObject),
      DECLARE_NAPI_FUNCTION("subscribeMessage", SubscribeMessage),
      DECLARE_NAPI_FUNCTION("unsubscribeMessage", UnsubscribeMessage),
      DECLARE_NAPI_FUNCTION("invokeCDPFromSDK", InvokeCDPFromSDK),
      DECLARE_NAPI_FUNCTION("addCDPEventListener", AddCDPEventListener),
      DECLARE_NAPI_FUNCTION("removeCDPEventListener", RemoveCDPEventListener),
      DECLARE_NAPI_FUNCTION("updateInputWindowInfo", UpdateInputWindowInfo),
      DECLARE_NAPI_FUNCTION("invalidateInputWindow", InvalidateInputWindow),
  };
#undef DECLARE_NAPI_FUNCTION

  constexpr size_t size = std::size(properties);

  napi_value cons;
  napi_status status =
      napi_define_class(env, "InspectorOwnerHarmony", NAPI_AUTO_LENGTH,
                        Constructor, nullptr, size, properties, &cons);
  assert(status == napi_ok);

  status = napi_set_named_property(env, exports, "InspectorOwnerHarmony", cons);
  return exports;
}

napi_value InspectorOwnerHarmony::Constructor(napi_env env,
                                              napi_callback_info info) {
  napi_value js_this;
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  uint64_t proxy_num = base::NapiUtil::ConvertToPtr(env, args[1]);

  auto owner_harmony_ptr = new InspectorOwnerHarmony(
      env, args[0], reinterpret_cast<LynxDevToolProxy *>(proxy_num));
  napi_wrap(
      env, js_this, owner_harmony_ptr, [](napi_env env, void *data, void *) {},
      nullptr, nullptr);
  return js_this;
}

napi_value InspectorOwnerHarmony::AttachProxy(napi_env env,
                                              napi_callback_info info) {
  napi_value js_this;
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  InspectorOwnerHarmony *owner_harmony_ptr = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void **>(&owner_harmony_ptr));
  NAPI_THROW_IF_FAILED_NULL(env, status,
                            "InspectorOwnerHarmony AttachProxy failed!");

  if (!owner_harmony_ptr) {
    LOGE("napi unwrap object is null when AttachProxy");
    return nullptr;
  }

  uint64_t proxy_num = base::NapiUtil::ConvertToPtr(env, args[0]);
  owner_harmony_ptr->owner_->AttachProxy(
      reinterpret_cast<LynxDevToolProxy *>(proxy_num));
  return nullptr;
}

napi_value InspectorOwnerHarmony::Destroy(napi_env env,
                                          napi_callback_info info) {
  napi_value js_this;
  size_t argc = 0;
  napi_get_cb_info(env, info, &argc, nullptr, &js_this, nullptr);

  InspectorOwnerHarmony *owner_harmony_ptr;
  napi_status status = napi_remove_wrap(
      env, js_this, reinterpret_cast<void **>(&owner_harmony_ptr));
  NAPI_THROW_IF_FAILED_NULL(env, status,
                            "InspectorOwnerHarmony napi_remove_wrap failed!");

  delete owner_harmony_ptr;
  return nullptr;
}

napi_value InspectorOwnerHarmony::GetSessionId(napi_env env,
                                               napi_callback_info info) {
  napi_value js_this;
  size_t argc = 0;
  napi_get_cb_info(env, info, &argc, nullptr, &js_this, nullptr);
  InspectorOwnerHarmony *owner_harmony_ptr = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void **>(&owner_harmony_ptr));
  NAPI_THROW_IF_FAILED_NULL(env, status,
                            "InspectorOwnerHarmony GetSessionId failed!");
  napi_value result;
  if (!owner_harmony_ptr) {
    LOGE("napi unwrap object is null when GetSessionId");
    napi_create_int32(env, -1, &result);
  } else {
    napi_create_int32(env, owner_harmony_ptr->owner_->GetSessionId(), &result);
  }
  return result;
}

napi_value InspectorOwnerHarmony::FlushConsoleMessages(
    napi_env env, napi_callback_info info) {
  napi_value js_this;
  size_t argc = 0;
  napi_get_cb_info(env, info, &argc, nullptr, &js_this, nullptr);

  InspectorOwnerHarmony *owner_harmony_ptr = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void **>(&owner_harmony_ptr));
  NAPI_THROW_IF_FAILED_NULL(
      env, status, "InspectorOwnerHarmony FlushConsoleMessages failed!");

  if (!owner_harmony_ptr) {
    LOGE("napi unwrap object is null when FlushConsoleMessages");
  } else {
    owner_harmony_ptr->owner_->FlushConsoleMessages();
  }
  return nullptr;
}

napi_value InspectorOwnerHarmony::GetConsoleObject(napi_env env,
                                                   napi_callback_info info) {
  napi_value js_this;
  size_t argc = 3;
  napi_value args[3];
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  InspectorOwnerHarmony *owner_harmony_ptr = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void **>(&owner_harmony_ptr));
  NAPI_THROW_IF_FAILED_NULL(env, status,
                            "InspectorOwnerHarmony GetConsoleObject failed!");

  if (!owner_harmony_ptr) {
    LOGE("napi unwrap object is null when GetConsoleObject");
  } else {
    std::string object_id = base::NapiUtil::ConvertToString(env, args[0]);
    bool need_stringify = base::NapiUtil::ConvertToBoolean(env, args[1]);
    int callback_id = base::NapiUtil::ConvertToInt32(env, args[2]);
    owner_harmony_ptr->owner_->GetConsoleObject(object_id, need_stringify,
                                                callback_id);
  }

  return nullptr;
}

napi_value InspectorOwnerHarmony::SubscribeMessage(napi_env env,
                                                   napi_callback_info info) {
  napi_value js_this;
  size_t argc = 2;
  napi_value args[2];
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  InspectorOwnerHarmony *owner_harmony_ptr = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void **>(&owner_harmony_ptr));
  NAPI_THROW_IF_FAILED_NULL(env, status,
                            "InspectorOwnerHarmony SubscribeMessage failed!");

  if (!owner_harmony_ptr) {
    LOGE("napi unwrap object is null when SubscribeMessage");
  } else {
    std::string type = base::NapiUtil::ConvertToString(env, args[0]);
    napi_value js_message_handler = args[1];
    auto message_handler =
        std::make_shared<MessageHandlerHarmony>(env, js_message_handler);
    owner_harmony_ptr->owner_->SubscribeMessage(type, message_handler);
  }

  return nullptr;
}

napi_value InspectorOwnerHarmony::UnsubscribeMessage(napi_env env,
                                                     napi_callback_info info) {
  napi_value js_this;
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  InspectorOwnerHarmony *owner_harmony_ptr = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void **>(&owner_harmony_ptr));
  NAPI_THROW_IF_FAILED_NULL(env, status,
                            "InspectorOwnerHarmony UnsubscribeMessage failed!");

  if (!owner_harmony_ptr) {
    LOGE("napi unwrap object is null when UnsubscribeMessage");
  } else {
    std::string type = base::NapiUtil::ConvertToString(env, args[0]);
    owner_harmony_ptr->owner_->UnsubscribeMessage(type);
  }

  return nullptr;
}

napi_value InspectorOwnerHarmony::InvokeCDPFromSDK(napi_env env,
                                                   napi_callback_info info) {
  napi_value js_this;
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  InspectorOwnerHarmony *owner_harmony_ptr = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void **>(&owner_harmony_ptr));
  NAPI_THROW_IF_FAILED_NULL(env, status,
                            "InspectorOwnerHarmony InvokeCDPFromSDK failed!");

  if (!owner_harmony_ptr) {
    LOGE("napi unwrap object is null when InvokeCDPFromSDK");
    return nullptr;
  }
  if (argc < std::size(args) || args[0] == nullptr || args[1] == nullptr) {
    LOGE("Invalid arguments when invoking CDP from SDK");
    return nullptr;
  }
  napi_valuetype callback_type = napi_undefined;
  napi_typeof(env, args[1], &callback_type);
  if (callback_type != napi_function) {
    LOGE("CDP result callback is not a function");
    return nullptr;
  }

  auto callback_sender =
      std::make_shared<InvokeCDPFromSDKSenderHarmony>(env, args[1]);
  owner_harmony_ptr->owner_->InvokeCDPFromSDK(
      base::NapiUtil::ConvertToString(env, args[0]),
      [callback_sender](const std::string &result) {
        callback_sender->SendMessage("CDP", result);
      });
  return nullptr;
}

napi_value InspectorOwnerHarmony::AddCDPEventListener(napi_env env,
                                                      napi_callback_info info) {
  napi_value js_this;
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  InspectorOwnerHarmony *owner_harmony_ptr = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void **>(&owner_harmony_ptr));
  NAPI_THROW_IF_FAILED_NULL(
      env, status, "InspectorOwnerHarmony AddCDPEventListener failed!");

  if (!owner_harmony_ptr) {
    LOGE("napi unwrap object is null when AddCDPEventListener");
    return nullptr;
  }
  if (argc < std::size(args) || args[0] == nullptr || args[1] == nullptr) {
    LOGE("Invalid arguments when adding a CDP event listener");
    return nullptr;
  }
  napi_valuetype listener_type = napi_undefined;
  napi_typeof(env, args[1], &listener_type);
  if (listener_type != napi_object) {
    LOGE("CDP event listener is not an object");
    return nullptr;
  }

  auto listener_sender =
      std::make_shared<CDPEventListenerSenderHarmony>(env, args[1]);
  owner_harmony_ptr->owner_->AddCDPEventListener(
      base::NapiUtil::ConvertToString(env, args[0]), listener_sender);
  return nullptr;
}

napi_value InspectorOwnerHarmony::RemoveCDPEventListener(
    napi_env env, napi_callback_info info) {
  napi_value js_this;
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  InspectorOwnerHarmony *owner_harmony_ptr = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void **>(&owner_harmony_ptr));
  NAPI_THROW_IF_FAILED_NULL(
      env, status, "InspectorOwnerHarmony RemoveCDPEventListener failed!");

  if (!owner_harmony_ptr) {
    LOGE("napi unwrap object is null when RemoveCDPEventListener");
    return nullptr;
  }
  if (argc < std::size(args) || args[0] == nullptr) {
    LOGE("Invalid arguments when removing a CDP event listener");
    return nullptr;
  }

  owner_harmony_ptr->owner_->RemoveCDPEventListener(
      base::NapiUtil::ConvertToString(env, args[0]));
  return nullptr;
}

napi_value InspectorOwnerHarmony::UpdateInputWindowInfo(
    napi_env env, napi_callback_info info) {
  napi_value js_this;
  size_t argc = 7;
  napi_value args[7];
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  InspectorOwnerHarmony *owner_harmony_ptr = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void **>(&owner_harmony_ptr));
  NAPI_THROW_IF_FAILED_NULL(
      env, status, "InspectorOwnerHarmony UpdateInputWindowInfo failed!");
  if (!owner_harmony_ptr || argc != std::size(args)) {
    LOGE("Invalid arguments when updating the input window");
    return nullptr;
  }

  HarmonyInputWindowInfo window_info;
  window_info.window_id = base::NapiUtil::ConvertToInt32(env, args[0]);
  window_info.display_id = base::NapiUtil::ConvertToInt32(env, args[1]);
  window_info.left_px = base::NapiUtil::ConvertToInt32(env, args[2]);
  window_info.top_px = base::NapiUtil::ConvertToInt32(env, args[3]);
  window_info.width_px = base::NapiUtil::ConvertToInt32(env, args[4]);
  window_info.height_px = base::NapiUtil::ConvertToInt32(env, args[5]);
  window_info.pixel_ratio = base::NapiUtil::ConvertToFloat(env, args[6]);
  owner_harmony_ptr->owner_->UpdateInputWindowInfo(window_info);
  return nullptr;
}

napi_value InspectorOwnerHarmony::InvalidateInputWindow(
    napi_env env, napi_callback_info info) {
  napi_value js_this;
  size_t argc = 0;
  napi_get_cb_info(env, info, &argc, nullptr, &js_this, nullptr);

  InspectorOwnerHarmony *owner_harmony_ptr = nullptr;
  napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void **>(&owner_harmony_ptr));
  NAPI_THROW_IF_FAILED_NULL(
      env, status, "InspectorOwnerHarmony InvalidateInputWindow failed!");
  if (owner_harmony_ptr) {
    owner_harmony_ptr->owner_->InvalidateInputWindow();
  }
  return nullptr;
}

InspectorOwnerHarmony::InspectorOwnerHarmony(napi_env env, napi_value js_ref,
                                             LynxDevToolProxy *proxy)
    : env_(env) {
  napi_create_reference(env_, js_ref, 0, &ref_);
  owner_ = std::make_shared<InspectorOwnerEmbedderHarmony>(env_, ref_);
  owner_->Init(proxy, owner_);
}

InspectorOwnerHarmony::~InspectorOwnerHarmony() {
  owner_->DetachProxy();
  napi_delete_reference(env_, ref_);
  owner_->Destroy();
}

}  // namespace devtool
}  // namespace lynx
