// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/tasm/config.h"
#if ENABLE_INSPECTOR
#include "platform/embedder/lynx_service/lynx_devtool_service.h"
#endif
#include "platform/embedder/lynx_env_platform.h"
#include "platform/embedder/module/global_module_registry.h"
#include "platform/embedder/public/capi/lynx_env_capi.h"

LYNX_EXTERN_C int lynx_env_prewarm_async() {
#if defined(_WIN32)
  return lynx::embedder::PrewarmPlatformAsync();
#else
  return 0;
#endif
}

namespace {
static std::string g_icu_data_path;
}

LYNX_EXTERN_C const char* lynx_env_get_sdk_version() {
  // Refer to a version number when building lynx sdk.
  return lynx::tasm::Config::GetCurrentLynxVersion().c_str();
}

LYNX_EXTERN_C void lynx_env_set_icu_data_path(const char* icu_data_path) {
  g_icu_data_path = icu_data_path ? icu_data_path : "";
}

LYNX_EXTERN_C const char* lynx_env_get_icu_data_path() {
  return g_icu_data_path.c_str();
}

LYNX_EXTERN_C void lynx_env_set_devtool_app_info(const char* name,
                                                 const char* value) {
#if ENABLE_INSPECTOR
  if (auto* service = lynx::embedder::GetDevToolService()) {
    service->SetAppInfo(name ? name : "", value ? value : "");
  }
#endif
}

LYNX_EXTERN_C void lynx_env_enable_devtool(int enable) {
#if ENABLE_INSPECTOR
  if (auto* service = lynx::embedder::GetDevToolService()) {
    service->EnableDevTool(enable);
  }
#endif
}

LYNX_EXTERN_C int lynx_env_is_devtool_enabled() {
#if ENABLE_INSPECTOR
  if (auto* service = lynx::embedder::GetDevToolService()) {
    return service->IsDevToolEnabled();
  }
#endif
  return 0;
}

LYNX_EXTERN_C int lynx_env_get_devtool_local_port() {
#if ENABLE_INSPECTOR
  if (auto* service = lynx::embedder::GetDevToolService()) {
    return service->GetLocalPort();
  }
#endif
  return -1;
}

LYNX_EXTERN_C int lynx_env_connect_devtool(const char* url) {
#if ENABLE_INSPECTOR
  if (auto* service = lynx::embedder::GetDevToolService()) {
    return service->Connect(url ? url : "", {});
  }
#endif
  return 0;
}

LYNX_EXTERN_C void lynx_env_set_open_card_callback(
    lynx_env_open_card_callback callback, void* user_data) {
#if ENABLE_INSPECTOR
  if (auto* service = lynx::embedder::GetDevToolService()) {
    service->SetOpenCardCallback(
        callback ? std::function<void(const std::string&)>(
                       [callback, user_data](const std::string& url) {
                         callback(user_data, url.c_str());
                       })
                 : nullptr);
  }
#endif
}

LYNX_EXTERN_C void lynx_env_set_close_card_callback(
    lynx_env_close_card_callback callback, void* user_data) {
#if ENABLE_INSPECTOR
  if (auto* service = lynx::embedder::GetDevToolService()) {
    service->SetCloseCardCallback(
        callback ? std::function<void()>(
                       [callback, user_data]() { callback(user_data); })
                 : nullptr);
  }
#endif
}

LYNX_EXTERN_C void lynx_env_enable_logbox(int enable) {
#if ENABLE_INSPECTOR
  if (auto* service = lynx::embedder::GetDevToolService()) {
    service->EnableLogBox(enable);
  }
#endif
}

LYNX_EXTERN_C int lynx_env_is_logbox_enabled() {
#if ENABLE_INSPECTOR
  if (auto* service = lynx::embedder::GetDevToolService()) {
    return service->IsLogBoxEnabled();
  }
#endif
  return 0;
}

LYNX_EXTERN_C void lynx_env_register_native_module(const char* name,
                                                   napi_module_creator creator,
                                                   void* opaque) {
#if ENABLE_NAPI_BINDING
  lynx::embedder::GlobalModuleRegistry::GetInstance().RegisterNativeModule(
      name, creator, opaque);
#endif
}

LYNX_EXTERN_C void lynx_env_register_extension_module(
    const char* name, extension_module_creator creator, bool is_lazy_create,
    void* opaque) {
#if ENABLE_NAPI_BINDING
  lynx::embedder::GlobalModuleRegistry::GetInstance().RegisterExtensionModule(
      name, creator, is_lazy_create, opaque);
#endif
}
