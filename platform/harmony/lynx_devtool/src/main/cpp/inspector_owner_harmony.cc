// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_devtool/src/main/cpp/inspector_owner_harmony.h"

#include <js_native_api.h>

#include <string>
#include <vector>

#include "base/include/platform/harmony/napi_util.h"

namespace lynx {
namespace devtool {

napi_value InspectorOwnerHarmony::Init(napi_env env, napi_value exports) {
#define DECLARE_NAPI_FUNCTION(name, func) \
  {(name), nullptr, (func), nullptr, nullptr, nullptr, napi_default, nullptr}
  napi_property_descriptor properties[] = {
      DECLARE_NAPI_FUNCTION("destroy", Destroy),
      DECLARE_NAPI_FUNCTION("getSessionId", GetSessionId),
      DECLARE_NAPI_FUNCTION("flushConsoleMessages", FlushConsoleMessages),
      DECLARE_NAPI_FUNCTION("getConsoleObject", GetConsoleObject),
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

InspectorOwnerHarmony::InspectorOwnerHarmony(napi_env env, napi_value js_ref,
                                             LynxDevToolProxy *proxy)
    : env_(env) {
  napi_create_reference(env_, js_ref, 0, &ref_);
  owner_ = std::make_shared<InspectorOwnerEmbedderHarmony>(env_, ref_);
  owner_->Init(proxy, owner_);
}

InspectorOwnerHarmony::~InspectorOwnerHarmony() {
  napi_delete_reference(env_, ref_);
  owner_->Destroy();
}

}  // namespace devtool
}  // namespace lynx
