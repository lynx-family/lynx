// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_BINDINGS_COMMON_MODULES_MOCK_NATIVE_MODULE_H_
#define CORE_RUNTIME_BINDINGS_COMMON_MODULES_MOCK_NATIVE_MODULE_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/public/jsb/native_module_factory.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace pub {
namespace test {

class MockNativeModule : public piper::LynxNativeModule {
 public:
  static std::shared_ptr<MockNativeModule> Create(std::string name) {
    return std::make_shared<MockNativeModule>(name);
  }
  ~MockNativeModule() override = default;

  const std::string& GetName() { return name_; }

  explicit MockNativeModule(std::string name)
      : LynxNativeModule(), name_(name) {}

  base::expected<std::unique_ptr<pub::Value>, std::string> InvokeMethod(
      const std::string& method_name, std::unique_ptr<pub::Value> args,
      size_t count, const piper::CallbackMap& callbacks) override {
    return std::make_unique<pub::ValueImplLepus>(lepus::Value("return_string"));
  }

 private:
  std::string name_;
};

}  // namespace test
}  // namespace pub
}  // namespace lynx
#endif  // CORE_RUNTIME_BINDINGS_COMMON_MODULES_MOCK_NATIVE_MODULE_H_
