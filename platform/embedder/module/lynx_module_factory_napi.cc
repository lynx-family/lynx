// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/module/lynx_module_factory_napi.h"

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_defines.h"
#endif

extern "C" bool napi_find_module_weak(const char* name, napi_module* out);

namespace lynx {
namespace embedder {

namespace {
void BindLynxViewContext(napi_env env, void* view_context) {
  if (env == nullptr) {
    return;
  }
  lynx_napi_set_instance_data(env, LYNX_NAPI_ENV_LYNX_VIEW_TAG, view_context,
                              nullptr, nullptr);
}
}  // namespace

LynxModuleFactoryNAPI::LynxModuleFactoryNAPI(
    Napi::Env env,
    std::unordered_map<std::string, std::pair<napi_module_creator, void*>>
        module_creators)
    : env_(env), module_creators_(std::move(module_creators)) {}

LynxModuleFactoryNAPI::LynxModuleFactoryNAPI(
    void* view_context,
    std::unordered_map<std::string, std::pair<napi_module_creator, void*>>
        module_creators)
    : view_context_(view_context),
      module_creators_(std::move(module_creators)) {}

LynxModuleFactoryNAPI::~LynxModuleFactoryNAPI() = default;

LynxMTSModuleFactoryNAPI::LynxMTSModuleFactoryNAPI(
    void* view_context,
    std::unordered_map<std::string, std::pair<napi_module_creator, void*>>
        module_creators)
    : LynxModuleFactoryNAPI(view_context, std::move(module_creators)) {}

LynxMTSModuleFactoryNAPI::~LynxMTSModuleFactoryNAPI() = default;

void LynxMTSModuleFactoryNAPI::AttachOpaqueContext(void* context) {
  auto env = static_cast<napi_env>(context);
  BindViewContext(env);
  env_.store(env, std::memory_order_release);
}

void LynxMTSModuleFactoryNAPI::DetachOpaqueContext(void* context) {
  auto env = static_cast<napi_env>(context);
  auto expected_env = env;
  env_.compare_exchange_strong(expected_env, nullptr,
                               std::memory_order_acq_rel);
  UnbindViewContext(env);
}

void LynxModuleFactoryNAPI::Detach() {
  // Erase the module map before LynxView destroyed.
  std::lock_guard<std::mutex> lock(mutex_);
  module_creators_.clear();
  env_.store(nullptr, std::memory_order_release);
  is_detached_ = true;
}

std::shared_ptr<runtime::LynxNativeModule> LynxModuleFactoryNAPI::CreateModule(
    const std::string& name) {
  return CreateModuleWithEnv(name, env_.load(std::memory_order_acquire));
}

std::shared_ptr<runtime::LynxNativeModule>
LynxModuleFactoryNAPI::CreateModuleWithEnv(const std::string& name,
                                           napi_env env) {
  if (env == nullptr) {
    return nullptr;
  }
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
    // napi_find_module_weak now returns a bool and writes the module info into
    // the out parameter to avoid letting the caller free memory allocated in a
    // different DLL.
    napi_module module;
    if (!napi_find_module_weak(name.c_str(), &module)) {
      return nullptr;
    }
    napi_value ret_exports;
    napi_get_undefined(env, &ret_exports);
    if (module.nm_register_func) {
      ret_exports = module.nm_register_func(env, Napi::Object::New(env));
      napi_value exception;
      napi_get_and_clear_last_exception(env, &exception);
    }
    return std::make_shared<LynxNativeModuleNAPI>(env, ret_exports);
  }
  auto exports = Napi::Object::New(env);
  napi_value ret_exports =
      creator_pair.first(env, exports, name.c_str(), creator_pair.second);
  return std::make_shared<LynxNativeModuleNAPI>(env, ret_exports);
}

void LynxModuleFactoryNAPI::BindViewContext(napi_env env) {
  BindLynxViewContext(env, view_context_);
}

void LynxModuleFactoryNAPI::UnbindViewContext(napi_env env) {
  BindLynxViewContext(env, nullptr);
}

}  // namespace embedder
}  // namespace lynx
