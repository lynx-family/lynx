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
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  uint64_t proxy_num = base::NapiUtil::ConvertToPtr(env, args[0]);

  auto owner_harmony_ptr = new InspectorOwnerHarmony(
      reinterpret_cast<LynxDevToolProxy *>(proxy_num));
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

InspectorOwnerHarmony::InspectorOwnerHarmony(LynxDevToolProxy *proxy) {
  owner_ = std::make_shared<InspectorOwnerEmbedder>();
  owner_->Init(proxy, owner_);
}

}  // namespace devtool
}  // namespace lynx
