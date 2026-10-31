// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_devtool/src/main/cpp/debug_bridge_harmony.h"

#include <js_native_api.h>

#include "devtool/embedder/core/debug_bridge_embedder.h"

namespace lynx {
namespace devtool {

napi_value DebugBridgeHarmony::Init(napi_env env, napi_value exports) {
#define DECLARE_NAPI_STATIC_FUNCTION(name, func) \
  {(name), nullptr, (func), nullptr, nullptr, nullptr, napi_static, nullptr}

  napi_property_descriptor properties[] = {
      DECLARE_NAPI_STATIC_FUNCTION("initialize", Initialize),
  };
#undef DECLARE_NAPI_STATIC_FUNCTION

  constexpr size_t size = std::size(properties);

  napi_value cons;
  napi_status status =
      napi_define_class(env, "DebugBridgeHarmony", NAPI_AUTO_LENGTH,
                        Constructor, nullptr, size, properties, &cons);
  assert(status == napi_ok);

  status = napi_set_named_property(env, exports, "DebugBridgeHarmony", cons);
  return exports;
}

napi_value DebugBridgeHarmony::Constructor(napi_env env,
                                           napi_callback_info info) {
  napi_value js_this;
  napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);
  return js_this;
}

napi_value DebugBridgeHarmony::Initialize(napi_env env,
                                          napi_callback_info info) {
  (void)env;
  (void)info;
  (void)DebugBridgeEmbedder::GetInstance();
  return nullptr;
}

}  // namespace devtool
}  // namespace lynx
