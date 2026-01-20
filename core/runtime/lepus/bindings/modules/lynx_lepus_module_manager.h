// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_BINDINGS_MODULES_LYNX_LEPUS_MODULE_MANAGER_H_
#define CORE_RUNTIME_LEPUS_BINDINGS_MODULES_LYNX_LEPUS_MODULE_MANAGER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "core/runtime/common/bindings/modules/lynx_native_module_manager.h"
#include "core/runtime/lepus/bindings/modules/lepus_module_callback.h"
#include "core/runtime/lepus/bindings/modules/lynx_lepus_module.h"
#include "core/runtime/lepus/context.h"

namespace lynx {
namespace lepus {
// LynxLepusModuleManager is a further encapsulation of LynxNativeModuleManager.
// It binds to PrimJS through PrimJS API (Lepus & LepusNG). It has two
// functions:
// 1. Connecting PrimJS and NativeModules (implemented on Android and iOS).
// 2. Data conversion, converting LepusValue to PubValue.
class LynxLepusModuleManager : public pub::LynxNativeModuleManager {
 public:
  explicit LynxLepusModuleManager() = default;
  explicit LynxLepusModuleManager(
      pub::LynxNativeModuleManager&& native_module_manager);
  virtual ~LynxLepusModuleManager();

  // Manager Type
  ManagerType Type() override { return ManagerType::LEPUS; };

  // Get the LepusModule object, which inherits LynxNativeModule implemented by
  // Android(LynxModuleAndroid) or iOS(LynxModuleDarwin).
  virtual Value GetModule(Context* context, const std::string& module_name);

 private:
  // Module cache
  std::unordered_map<std::string, fml::RefPtr<LynxLepusModule>> module_map_;
  std::shared_ptr<LepusModuleDelegate> execute_delegate_;
};
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_BINDINGS_MODULES_LYNX_LEPUS_MODULE_MANAGER_H_
