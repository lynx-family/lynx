// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/module/lynx_module_factory_napi.h"

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

namespace lynx {
namespace embedder {

LynxModuleFactoryNAPI::LynxModuleFactoryNAPI(
    Napi::Env env,
    std::unordered_map<std::string, std::pair<napi_module_creator, void*>>
        module_creators)
    : env_(env), module_creators_(std::move(module_creators)) {}

LynxModuleFactoryNAPI::~LynxModuleFactoryNAPI() = default;

void LynxModuleFactoryNAPI::Detach() {
  // Erase the module map before LynxView destroyed.
  std::lock_guard<std::mutex> lock(mutex_);
  module_creators_.clear();
  is_detached_ = true;
}

std::shared_ptr<runtime::LynxNativeModule> LynxModuleFactoryNAPI::CreateModule(
    const std::string& name) {
  // It will erase the module map before LynxView destroyed,
  // so it's safe to create new module here.
  std::pair<napi_module_creator, void*> creator_pair = {nullptr, nullptr};
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_detached_) {
      return nullptr;
    }
    auto creator_holder = module_creators_.find(name);
    if (creator_holder != module_creators_.end()) {
      creator_pair = creator_holder->second;
    }
  }
  if (creator_pair.first == nullptr) {
    // If no corresponding creator is found, try searching in globally
    // registered NAPI modules
    const napi_module* module = napi_find_module(name.c_str());
    if (!module) {
      return nullptr;
    }
    napi_env env = env_;
    napi_value ret_exports;
    env->napi_get_undefined(env, &ret_exports);
    if (module->nm_register_func) {
      ret_exports = module->nm_register_func(env, Napi::Object::New(env_));
      napi_value exception;
      env->napi_get_and_clear_last_exception(env, &exception);
    }
    return std::make_shared<LynxNativeModuleNAPI>(env_, ret_exports);
  }
  auto exports = Napi::Object::New(env_);
  napi_value ret_exports =
      creator_pair.first(env_, exports, name.c_str(), creator_pair.second);
  return std::make_shared<LynxNativeModuleNAPI>(env_, ret_exports);
}

}  // namespace embedder
}  // namespace lynx
