// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/public/jsb/native_module_factory.h"

#include <memory>

#include "core/runtime/bindings/jsi/modules/lynx_jsi_module_callback.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace piper {

class TestNativeModule : public LynxNativeModule {
 public:
  static std::shared_ptr<LynxNativeModule> Create() {
    return std::make_shared<TestNativeModule>();
  }

  static const std::string& GetName() { return name_; }

  TestNativeModule() : LynxNativeModule() {
    NativeModuleMethod m("test", 17);
    RegisterMethod(
        m, reinterpret_cast<NativeModuleInvocation>(&TestNativeModule::Test));
  }

  base::expected<std::unique_ptr<pub::Value>, std::string> InvokeMethod(
      const std::string& method_name, std::unique_ptr<pub::Value> args,
      size_t count, const CallbackMap& callbacks) override {
    auto invocation_itr = invocations_.find(method_name);
    if (invocation_itr != invocations_.end()) {
      return (this->*(invocation_itr->second))(std::move(args), callbacks);
    }
    return std::unique_ptr<pub::Value>(nullptr);
  }

  std::unique_ptr<pub::Value> Test(std::unique_ptr<pub::Value> args,
                                   const CallbackMap& callbacks) {
    return args;
  }

 protected:
  void RegisterMethod(const NativeModuleMethod& method,
                      NativeModuleInvocation invocation) {
    methods_.emplace(method.name, method);
    invocations_.emplace(method.name, std::move(invocation));
  }

 private:
  static const std::string name_;
  std::unordered_map<std::string, NativeModuleInvocation> invocations_;
};

const std::string TestNativeModule::name_ = "TestNativeModule";

class NativeModuleFactoryTest : public ::testing::Test {
 protected:
  NativeModuleFactoryTest() = default;
  ~NativeModuleFactoryTest() override = default;

  void SetUp() override {
    module_factory_ = std::make_shared<NativeModuleFactory>();
  }

  void TearDown() override {}

  std::shared_ptr<NativeModuleFactory> module_factory_;
};  // NativeModuleFactoryTest

TEST_F(NativeModuleFactoryTest, NativeModuleFactoryTotalTest) {
  module_factory_->Register(TestNativeModule::GetName(),
                            TestNativeModule::Create);
  auto module = module_factory_->CreateModule("TestNativeModule");
  pub::PubValueFactoryDefault value_factory;
  auto arr = value_factory.CreateArray();
  arr->PushBoolToArray(true);
  CallbackMap callbacks;
  auto callback = std::make_shared<lynx::piper::ModuleCallback>(10);
  callback->SetModuleName("TestNativeModule");
  callback->SetMethodName("test");
  ASSERT_TRUE(callback->module_name_ == "TestNativeModule");
  ASSERT_TRUE(callback->method_name_ == "test");
  ASSERT_TRUE(callback->callback_id() == 10);
  callbacks.emplace(1, std::move(callback));
  auto ret = module->InvokeMethod("test", std::move(arr), 1, callbacks);
  ASSERT_TRUE(ret.has_value());
  ASSERT_TRUE(ret.value()->IsArray());
  ASSERT_TRUE(ret.value()->Length() == 1);
}

}  // namespace piper
}  // namespace lynx
