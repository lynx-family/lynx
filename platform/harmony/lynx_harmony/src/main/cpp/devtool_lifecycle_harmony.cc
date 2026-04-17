// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/devtool_lifecycle_harmony.h"

#include <iterator>

#include "core/renderer/utils/devtool_lifecycle.h"
#include "core/renderer/utils/devtool_state.h"

namespace lynx {
namespace tasm {
namespace harmony {

namespace {

napi_value CreateBoolean(napi_env env, bool value) {
  napi_value result = nullptr;
  napi_get_boolean(env, value, &result);
  return result;
}

napi_value CreateUndefined(napi_env env) {
  napi_value result = nullptr;
  napi_get_undefined(env, &result);
  return result;
}

int GetStateValue() {
  auto& lifecycle = DevToolLifecycle::GetInstance();
  if (lifecycle.IsConnected()) {
    return static_cast<int>(DevToolState::CONNECTED);
  }
  if (lifecycle.IsInitialized()) {
    return static_cast<int>(DevToolState::INITIALIZED);
  }
  if (lifecycle.IsEnabled()) {
    return static_cast<int>(DevToolState::ENABLED);
  }
  if (lifecycle.IsAttached()) {
    return static_cast<int>(DevToolState::ATTACHED);
  }
  return static_cast<int>(DevToolState::UNAVAILABLE);
}

}  // namespace

napi_value DevToolLifecycleHarmony::Init(napi_env env, napi_value exports) {
#define DECLARE_NAPI_STATIC_FUNCTION(name, func) \
  {(name), nullptr, (func), nullptr, nullptr, nullptr, napi_static, nullptr}

  napi_property_descriptor properties[] = {
      DECLARE_NAPI_STATIC_FUNCTION("getState", GetState),
      DECLARE_NAPI_STATIC_FUNCTION("isAttached", IsAttached),
      DECLARE_NAPI_STATIC_FUNCTION("isInitialized", IsInitialized),
      DECLARE_NAPI_STATIC_FUNCTION("isEnabled", IsEnabled),
      DECLARE_NAPI_STATIC_FUNCTION("isConnected", IsConnected),
      DECLARE_NAPI_STATIC_FUNCTION("onAttached", OnAttached),
      DECLARE_NAPI_STATIC_FUNCTION("onEnabled", OnEnabled),
      DECLARE_NAPI_STATIC_FUNCTION("onDisabled", OnDisabled),
      DECLARE_NAPI_STATIC_FUNCTION("onInitialized", OnInitialized),
      DECLARE_NAPI_STATIC_FUNCTION("onConnected", OnConnected),
      DECLARE_NAPI_STATIC_FUNCTION("onDisconnected", OnDisconnected),
  };
#undef DECLARE_NAPI_STATIC_FUNCTION

  napi_value cons = nullptr;
  napi_define_class(env, "DevToolLifecycleHarmony", NAPI_AUTO_LENGTH,
                    Constructor, nullptr, std::size(properties), properties,
                    &cons);
  napi_set_named_property(env, exports, "DevToolLifecycleHarmony", cons);
  return exports;
}

napi_value DevToolLifecycleHarmony::Constructor(napi_env env,
                                                napi_callback_info info) {
  napi_value js_this = nullptr;
  napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);
  return js_this;
}

napi_value DevToolLifecycleHarmony::GetState(napi_env env,
                                             napi_callback_info info) {
  napi_value result = nullptr;
  napi_create_int32(env, GetStateValue(), &result);
  return result;
}

napi_value DevToolLifecycleHarmony::IsAttached(napi_env env,
                                               napi_callback_info info) {
  return CreateBoolean(env, DevToolLifecycle::GetInstance().IsAttached());
}

napi_value DevToolLifecycleHarmony::IsInitialized(napi_env env,
                                                  napi_callback_info info) {
  return CreateBoolean(env, DevToolLifecycle::GetInstance().IsInitialized());
}

napi_value DevToolLifecycleHarmony::IsEnabled(napi_env env,
                                              napi_callback_info info) {
  return CreateBoolean(env, DevToolLifecycle::GetInstance().IsEnabled());
}

napi_value DevToolLifecycleHarmony::IsConnected(napi_env env,
                                                napi_callback_info info) {
  return CreateBoolean(env, DevToolLifecycle::GetInstance().IsConnected());
}

napi_value DevToolLifecycleHarmony::OnAttached(napi_env env,
                                               napi_callback_info info) {
  DevToolLifecycle::GetInstance().OnAttached();
  return CreateUndefined(env);
}

napi_value DevToolLifecycleHarmony::OnEnabled(napi_env env,
                                              napi_callback_info info) {
  DevToolLifecycle::GetInstance().OnEnabled();
  return CreateUndefined(env);
}

napi_value DevToolLifecycleHarmony::OnDisabled(napi_env env,
                                               napi_callback_info info) {
  DevToolLifecycle::GetInstance().OnDisabled();
  return CreateUndefined(env);
}

napi_value DevToolLifecycleHarmony::OnInitialized(napi_env env,
                                                  napi_callback_info info) {
  DevToolLifecycle::GetInstance().OnInitialized();
  return CreateUndefined(env);
}

napi_value DevToolLifecycleHarmony::OnConnected(napi_env env,
                                                napi_callback_info info) {
  DevToolLifecycle::GetInstance().OnConnected();
  return CreateUndefined(env);
}

napi_value DevToolLifecycleHarmony::OnDisconnected(napi_env env,
                                                   napi_callback_info info) {
  DevToolLifecycle::GetInstance().OnDisconnected();
  return CreateUndefined(env);
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
