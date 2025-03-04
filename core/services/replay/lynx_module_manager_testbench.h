// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_REPLAY_LYNX_MODULE_MANAGER_TESTBENCH_H_
#define CORE_SERVICES_REPLAY_LYNX_MODULE_MANAGER_TESTBENCH_H_
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "core/services/replay/lynx_module_binding_testbench.h"
#include "core/services/replay/lynx_module_testbench.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace piper {

using ModuleTestBenchPtr = std::shared_ptr<lynx::piper::ModuleTestBench>;
using LynxModuleBindingPtrTestBench =
    std::shared_ptr<lynx::piper::LynxModuleBindingTestBench>;

typedef std::function<void()> InitRecordModuleDataCallback;

class ModuleManagerTestBench {
 public:
  ModuleManagerTestBench();
  void Destroy();
  void initBindingPtr(std::weak_ptr<ModuleManagerTestBench> weak_manager,
                      const std::shared_ptr<ModuleDelegate> &delegate,
                      LynxModuleBindingPtr lynxPtr);
  LynxModuleBindingPtrTestBench bindingPtr;
  void initRecordModuleData(Runtime *rt,
                            InitRecordModuleDataCallback callback = nullptr);

  void SetGroupInterceptor(std::shared_ptr<GroupInterceptor> interceptor) {
    group_interceptor_ = std::move(interceptor);
  }

 protected:
  LynxModuleProviderFunction BindingFunc(
      std::weak_ptr<ModuleManagerTestBench> weak_manager,
      const std::shared_ptr<ModuleDelegate> &delegate);

 private:
  ModuleTestBenchPtr getModule(const std::string &name,
                               const std::shared_ptr<ModuleDelegate> &delegate);
  rapidjson::Document recordData;
  std::unordered_map<std::string, ModuleTestBenchPtr> moduleMap;
  void syncToPlatform(const rapidjson::Value &sync_attrs, Runtime *rt,
                      const piper::Value *args, size_t count);

  void fetchRecordData(const std::string &module_name, Runtime &runtime,
                       InvokeMethodCallback callback);
  rapidjson::Document jsb_ignored_info_;
  rapidjson::Document jsb_settings_;

  void resetModuleRecordData(const std::string &module_name,
                             InvokeMethodCallback callback);

  std::shared_ptr<GroupInterceptor> group_interceptor_;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_SERVICES_REPLAY_LYNX_MODULE_MANAGER_TESTBENCH_H_
