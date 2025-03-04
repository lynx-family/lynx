// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_BINDINGS_JSI_MOCK_MODULE_DELEGATE_H_
#define CORE_RUNTIME_BINDINGS_JSI_MOCK_MODULE_DELEGATE_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/debug/lynx_error.h"
#include "core/runtime/bindings/jsi/modules/module_delegate.h"
#include "core/runtime/jsi/jsi.h"
#include "lynx/base/include/closure.h"
#include "lynx/core/runtime/bindings/jsi/modules/lynx_jsi_module_callback.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace piper {
namespace test {
class MockModuleDelegate : public ModuleDelegate {
 public:
  MockModuleDelegate(Runtime* rt) : rt_(rt){};
  MockModuleDelegate(){};
  ~MockModuleDelegate() override = default;

  int64_t RegisterJSCallbackFunction(Function func) override {
    functions_.push_back(std::move(func));
    return functions_.size() - 1;
  }
  void CallJSCallback(
      const std::shared_ptr<ModuleCallback>& callback,
      int64_t id_to_delete = ModuleCallback::kInvalidCallbackId) override {
    if (rt_) {
      auto index = callback->callback_id();
      functions_[index].call(*rt_, nullptr, 0);
    }
  }
  MOCK_METHOD(void, OnErrorOccurred, (base::LynxError), (override));
  MOCK_METHOD(void, OnMethodInvoked,
              (const std::string&, const std::string&, int32_t code),
              (override));
  void FlushJSBTiming(piper::NativeModuleInfo timing) override{};
  void RunOnJSThread(base::closure func) override{};
  void RunOnPlatformThread(base::closure func) override{};

 private:
  Runtime* rt_{nullptr};
  std::vector<Function> functions_;
};

}  // namespace test
}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_BINDINGS_JSI_MOCK_MODULE_DELEGATE_H_
