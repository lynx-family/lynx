// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_LYNX_ENV_H_
#define PLATFORM_EMBEDDER_PUBLIC_LYNX_ENV_H_

#include <string>
#include <unordered_map>

#include "capi/lynx_env_capi.h"
#include "lynx_native_module.h"

namespace lynx {
namespace pub {

class LynxEnv {
 public:
  static LynxEnv& GetInstance();
  const char* GetVersion();
  void SetDevtoolAppInfo(const char* name, const char* value);
  void SetDevtoolEnabled(bool enable);
  bool IsDevtoolEnabled();
  bool ConnectDevtool(const char* url);
  void SetLogboxEnabled(bool enable);
  bool IsLogboxEnabled();
  void RegisterNativeModule(const std::string& module_name,
                            napi_module_creator creator, void* opaque);
  void RegisterExtensionModule(const std::string& module_name,
                               extension_module_creator creator,
                               bool is_lazy_create, void* opaque);

  LynxEnv(const LynxEnv&) = delete;
  LynxEnv& operator=(const LynxEnv&) = delete;

 private:
  LynxEnv() = default;
  ~LynxEnv() = default;
};

inline LynxEnv& LynxEnv::GetInstance() {
  static LynxEnv instance;
  return instance;
}

inline const char* LynxEnv::GetVersion() { return lynx_env_get_sdk_version(); }

inline void LynxEnv::SetDevtoolAppInfo(const char* name, const char* value) {
  lynx_env_set_devtool_app_info(name, value);
}

inline void LynxEnv::SetDevtoolEnabled(bool enable) {
  lynx_env_enable_devtool(enable);
}

inline bool LynxEnv::IsDevtoolEnabled() {
  return lynx_env_is_devtool_enabled();
}

inline bool LynxEnv::ConnectDevtool(const char* url) {
  return lynx_env_connect_devtool(url);
}

inline void LynxEnv::SetLogboxEnabled(bool enable) {
  lynx_env_enable_logbox(enable);
}

inline bool LynxEnv::IsLogboxEnabled() { return lynx_env_is_logbox_enabled(); }

inline void LynxEnv::RegisterNativeModule(const std::string& module_name,
                                          napi_module_creator creator,
                                          void* opaque) {
  lynx_env_register_native_module(module_name.c_str(), creator, opaque);
}

inline void LynxEnv::RegisterExtensionModule(const std::string& module_name,
                                             extension_module_creator creator,
                                             bool is_lazy_create,
                                             void* opaque) {
  lynx_env_register_extension_module(module_name.c_str(), creator,
                                     is_lazy_create, opaque);
}

}  // namespace pub
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_PUBLIC_LYNX_ENV_H_
