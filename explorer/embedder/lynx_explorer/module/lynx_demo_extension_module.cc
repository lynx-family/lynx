// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "explorer/embedder/lynx_explorer/module/lynx_demo_extension_module.h"
#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_defines.h"
#endif

namespace lynx {
namespace example {

const uint64_t kDemoExtensionModuleID =
    reinterpret_cast<uint64_t>(&kDemoExtensionModuleID);
namespace {
napi_value EXTestMethod(napi_env env, napi_callback_info info) {
  void* data;
  lynx_napi_get_instance_data(env, kDemoExtensionModuleID, &data);
  auto* module = reinterpret_cast<LynxDemoExtensionModule*>(data);
  if (!module) {
    return nullptr;
  }

  size_t argc = 1;
  napi_value argv[1];
  napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
  if (argc < 1) {
    return nullptr;
  }
  int32_t arg1;
  napi_get_value_int32(env, argv[0], &arg1);
  napi_value result;
  napi_get_boolean(env, arg1 > 0, &result);
  return result;
}

Napi::Value LynxDemoExtensionModuleCreator(Napi::Env env, Napi::Value exports,
                                           const char* module_name,
                                           LynxDemoExtensionModule& module) {
  napi_value func;
  // TODO(wujintian): Replace all NAPI C APIs in the module with C++ APIs
  napi_create_function(static_cast<napi_env>(env), "exTestMethod", 1,
                       &EXTestMethod, 0, &func);
  napi_set_named_property(static_cast<napi_env>(env),
                          static_cast<napi_value>(exports), "exTestMethod",
                          func);
  return exports;
}

}  // namespace

lynx_extension_module_t* LynxDemoExtensionModule::CreateCModule(void* opaque) {
  auto* module = new LynxDemoExtensionModule();
  lynx_extension_module_t* c_module =
      lynx_extension_module_create_with_finalizer(
          module, [](lynx_extension_module_t* m, void* user_data) {
            auto* demo_module =
                reinterpret_cast<LynxDemoExtensionModule*>(user_data);
            if (demo_module) {
              delete demo_module;
            }
          });
  module->SetCModule(c_module);
  module->SetNapiModuleCreator(&LynxDemoExtensionModuleCreator);
  return c_module;
}

void LynxDemoExtensionModule::OnRuntimeAttach(
    Napi::Env env, std::unique_ptr<pub::VSyncObserver> vsync_observer) {
  lynx_napi_set_instance_data(static_cast<napi_env>(env),
                              kDemoExtensionModuleID, this, nullptr, nullptr);
}

void LynxDemoExtensionModule::Destroy() {
  // do something
}

}  // namespace example
}  // namespace lynx
