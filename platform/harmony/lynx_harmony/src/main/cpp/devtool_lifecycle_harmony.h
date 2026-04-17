// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_DEVTOOL_LIFECYCLE_HARMONY_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_DEVTOOL_LIFECYCLE_HARMONY_H_

#include <napi/native_api.h>

namespace lynx {
namespace tasm {
namespace harmony {

class DevToolLifecycleHarmony {
 public:
  static napi_value Init(napi_env env, napi_value exports);

 private:
  static napi_value Constructor(napi_env env, napi_callback_info info);
  static napi_value GetState(napi_env env, napi_callback_info info);
  static napi_value IsAttached(napi_env env, napi_callback_info info);
  static napi_value IsInitialized(napi_env env, napi_callback_info info);
  static napi_value IsEnabled(napi_env env, napi_callback_info info);
  static napi_value IsConnected(napi_env env, napi_callback_info info);
  static napi_value OnAttached(napi_env env, napi_callback_info info);
  static napi_value OnEnabled(napi_env env, napi_callback_info info);
  static napi_value OnDisabled(napi_env env, napi_callback_info info);
  static napi_value OnInitialized(napi_env env, napi_callback_info info);
  static napi_value OnConnected(napi_env env, napi_callback_info info);
  static napi_value OnDisconnected(napi_env env, napi_callback_info info);
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_DEVTOOL_LIFECYCLE_HARMONY_H_
