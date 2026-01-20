// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/common/bindings/modules/lynx_native_module_manager.h"

#include "core/public/jsb/lynx_native_module.h"
#include "core/public/jsb/native_module_factory.h"
#include "core/runtime/js/bindings/mock_module_delegate.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace pub {

class MockNativeModule : public runtime::LynxNativeModule {
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
      size_t count, const runtime::CallbackMap& callbacks) override {
    return std::unique_ptr<pub::Value>(nullptr);
  }

 private:
  std::string name_;
};

class LynxNativeModuleManagerTest : public ::testing::Test {
 protected:
  LynxNativeModuleManagerTest() = default;
  ~LynxNativeModuleManagerTest() override = default;

  void SetUp() override {
    native_module_manager_ = std::make_shared<LynxNativeModuleManager>();

    auto platform_module_factory =
        std::make_unique<runtime::NativeModuleFactory>();
    platform_module_factory->Register("platform_module", []() {
      return std::make_shared<MockNativeModule>("platform_module");
    });
    native_module_manager_->SetPlatformModuleFactory(
        std::move(platform_module_factory));

    auto native_module_factory =
        std::make_unique<runtime::NativeModuleFactory>();
    native_module_factory->Register("native_module", []() {
      return std::make_shared<MockNativeModule>("native_module");
    });
    native_module_manager_->SetModuleFactory(std::move(native_module_factory));
    native_module_manager_->SetRecordID(12345);
  }

  void TearDown() override {}

  std::shared_ptr<LynxNativeModuleManager> native_module_manager_;
};

TEST_F(LynxNativeModuleManagerTest, GetModuleWithNativeFactory) {
  auto module = native_module_manager_->GetModule("native_module");
  EXPECT_NE(module, nullptr);
}

TEST_F(LynxNativeModuleManagerTest, GetModuleWithPlatformFactory) {
  auto module = native_module_manager_->GetModule("platform_module");
  EXPECT_NE(module, nullptr);
}

TEST_F(LynxNativeModuleManagerTest, GetRecordId) {
  EXPECT_EQ(native_module_manager_->record_id_, 12345);
}

}  // namespace pub
};  // namespace lynx
