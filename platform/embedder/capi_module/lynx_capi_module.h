// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_CAPI_MODULE_LYNX_CAPI_MODULE_H_
#define PLATFORM_EMBEDDER_CAPI_MODULE_LYNX_CAPI_MODULE_H_

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

#include "core/public/jsb/native_module_factory.h"
#include "platform/embedder/public/capi/lynx_capi_module_capi.h"

namespace lynx {
namespace embedder {

struct LynxCapiModuleRegistration {
  LynxCapiModuleRegistration() = default;
  ~LynxCapiModuleRegistration();
  LynxCapiModuleRegistration(const LynxCapiModuleRegistration&) = delete;
  LynxCapiModuleRegistration& operator=(const LynxCapiModuleRegistration&) =
      delete;

  runtime::NativeModuleMethods methods;
  lynx_capi_module_invoke invoke = nullptr;
  void* opaque = nullptr;
  lynx_capi_module_finalizer finalizer = nullptr;
};

using LynxCapiModuleMap =
    std::unordered_map<std::string,
                       std::shared_ptr<LynxCapiModuleRegistration>>;

class LynxCapiModuleRegistry {
 public:
  explicit LynxCapiModuleRegistry(LynxCapiModuleMap modules);
  ~LynxCapiModuleRegistry();

  // The caller holds the registration until its invocation has finished.
  std::shared_ptr<LynxCapiModuleRegistration> GetModule(
      const std::string& name);
  void Detach();
  bool IsActive();

 private:
  std::mutex mutex_;
  LynxCapiModuleMap modules_;
  bool detached_ = false;
};

class LynxCapiModuleFactory final : public runtime::NativeModuleFactory {
 public:
  explicit LynxCapiModuleFactory(
      std::shared_ptr<LynxCapiModuleRegistry> registry)
      : registry_(std::move(registry)) {}

  std::shared_ptr<runtime::LynxNativeModule> CreateModule(
      const std::string& name) override;

 private:
  std::shared_ptr<LynxCapiModuleRegistry> registry_;
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_CAPI_MODULE_LYNX_CAPI_MODULE_H_
