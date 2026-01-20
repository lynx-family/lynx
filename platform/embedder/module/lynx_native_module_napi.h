// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_MODULE_LYNX_NATIVE_MODULE_NAPI_H_
#define PLATFORM_EMBEDDER_MODULE_LYNX_NATIVE_MODULE_NAPI_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "core/public/jsb/lynx_native_module.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "platform/embedder/public/capi/lynx_native_module_capi.h"
#include "third_party/binding/napi/shim/shim_napi.h"
#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

namespace lynx {
namespace embedder {

class LynxNativeModuleNAPI : public runtime::LynxNativeModule {
 public:
  LynxNativeModuleNAPI(Napi::Env env, napi_value exports);
  ~LynxNativeModuleNAPI() override;

  base::expected<std::unique_ptr<pub::Value>, std::string> InvokeMethod(
      const std::string& method_name, std::unique_ptr<pub::Value> args,
      size_t count, const runtime::CallbackMap& callbacks) override;

  runtime::NativeModuleMethods AdoptMethods() { return methods_; }

 private:
  void ExtractMethods();
  void ReleaseMemberRefs();

  static napi_value Callback(napi_env env, napi_callback_info info);
  static void Cleanup(void* arg);
  void CleanupSelf();

  napi_env env_;
  napi_ref exports_ref_;
  std::unordered_map<std::string, napi_ref> method_refs_;
  std::unordered_map<std::string, napi_ref> field_refs_;
};

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_undefs.h"
#endif

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_MODULE_LYNX_NATIVE_MODULE_NAPI_H_
