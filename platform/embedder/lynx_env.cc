// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/tasm/config.h"
#if ENABLE_INSPECTOR
#include "devtool/embedder/common/debugger_embedder.h"
#include "platform/embedder/lynx_devtool/devtool_env_embedder.h"
#endif
#include "platform/embedder/module/global_module_registry.h"
#include "platform/embedder/public/capi/lynx_env_capi.h"

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
  lynx::embedder::DevToolEnvEmbedder::GetInstance().SetAppInfo(name, value);
#endif
}

LYNX_EXTERN_C void lynx_env_enable_devtool(int enable) {
#if ENABLE_INSPECTOR
  lynx::embedder::DevToolEnvEmbedder::GetInstance().EnableDevTool(enable);
#endif
}

LYNX_EXTERN_C int lynx_env_is_devtool_enabled() {
#if ENABLE_INSPECTOR
  return lynx::embedder::DevToolEnvEmbedder::GetInstance().IsDevToolEnabled();
#else
  return 0;
#endif
}

LYNX_EXTERN_C int lynx_env_connect_devtool(const char* url) {
#if ENABLE_INSPECTOR
  std::unordered_map<std::string, std::string> options;
  return lynx::devtool::DebuggerEmbedder::ConnectDevtools(url ? url : "",
                                                          options);
#else
  return 0;
#endif
}

LYNX_EXTERN_C void lynx_env_enable_logbox(int enable) {
#if ENABLE_INSPECTOR
  lynx::embedder::DevToolEnvEmbedder::GetInstance().SetDevToolSwitch(
      lynx::tasm::LynxEnv::kLynxEnableLogBox, enable);
#endif
}

LYNX_EXTERN_C int lynx_env_is_logbox_enabled() {
#if ENABLE_INSPECTOR
  return lynx::embedder::DevToolEnvEmbedder::GetInstance().IsLogBoxEnabled();
#else
  return 0;
#endif
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
