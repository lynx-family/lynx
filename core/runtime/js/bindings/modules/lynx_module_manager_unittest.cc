// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bindings/modules/lynx_module_manager.h"

#include <memory>

#include "core/public/jsb/native_module_factory.h"
#include "core/runtime/js/bindings/mock_module_delegate.h"
#include "core/runtime/js/bindings/modules/lynx_jsi_module.h"
#include "core/runtime/js/bindings/modules/lynx_jsi_module_callback.h"
#include "core/runtime/js/bindings/modules/lynx_module_timing.h"
#include "core/runtime/js/bindings/modules/module_delegate.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace runtime {
namespace js {
class MockNativeModule : public LynxNativeModule {
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
      size_t count, const CallbackMap& callbacks) override {
    return std::unique_ptr<pub::Value>(nullptr);
  }

 private:
  std::string name_;
};

class MockPlatformModuleFactory : public NativeModuleFactory {
 public:
  MockPlatformModuleFactory()
      : mock_module_delegate_(std::make_shared<test::MockModuleDelegate>()){};
  virtual ~MockPlatformModuleFactory() = default;

  virtual void Register(const std::string& name, ModuleCreator creator) {
    creators_.emplace(name, std::move(creator));
  }

 private:
  std::unordered_map<std::string, ModuleCreator> creators_;
  std::shared_ptr<test::MockModuleDelegate> mock_module_delegate_;
};

class LynxModuleManagerTest : public ::testing::Test {
 protected:
  LynxModuleManagerTest() = default;
  ~LynxModuleManagerTest() override = default;

  void SetUp() override {
    module_manager_ = std::make_shared<LynxModuleManager>();
    module_manager_->initBindingPtr(module_manager_, nullptr);

    auto platform_module_factory = std::make_unique<NativeModuleFactory>();
    platform_module_factory->Register("platform_module", []() {
      return std::make_shared<MockNativeModule>("platform_module");
    });
    module_manager_->SetPlatformModuleFactory(
        std::move(platform_module_factory));

    auto native_module_factory = std::make_unique<NativeModuleFactory>();
    native_module_factory->Register("native_module", []() {
      return std::make_shared<MockNativeModule>("native_module");
    });
    module_manager_->SetModuleFactory(std::move(native_module_factory));
    module_manager_->SetRecordID(12345);

    auto native_module_manager =
        std::make_shared<pub::LynxNativeModuleManager>();
    use_native_module_manager_ =
        std::make_shared<LynxModuleManager>(std::move(*native_module_manager));
    use_native_module_manager_->initBindingPtr(use_native_module_manager_,
                                               nullptr);
    auto platform_module_factory_2 = std::make_unique<NativeModuleFactory>();
    platform_module_factory_2->Register("platform_module", []() {
      return std::make_shared<MockNativeModule>("platform_module");
    });
    use_native_module_manager_->SetPlatformModuleFactory(
        std::move(platform_module_factory_2));

    auto native_module_factory_2 = std::make_unique<NativeModuleFactory>();
    native_module_factory_2->Register("native_module", []() {
      return std::make_shared<MockNativeModule>("native_module");
    });
    use_native_module_manager_->SetModuleFactory(
        std::move(native_module_factory_2));
    use_native_module_manager_->SetRecordID(12345);
  }

  void TearDown() override {}

  std::shared_ptr<LynxModuleManager> module_manager_;
  std::shared_ptr<LynxModuleManager> use_native_module_manager_;
};

TEST_F(LynxModuleManagerTest, CheckPlatformFactory) {
  EXPECT_NE(module_manager_->GetPlatformModuleFactory(), nullptr);
  EXPECT_NE(use_native_module_manager_->GetPlatformModuleFactory(), nullptr);
}

TEST_F(LynxModuleManagerTest, GetNativeModule) {
  auto module_1 = module_manager_->bindingPtr->GetModule("native_module");
  EXPECT_NE(module_1, nullptr);
  auto module_2 =
      use_native_module_manager_->bindingPtr->GetModule("native_module");
  EXPECT_NE(module_2, nullptr);
}

TEST_F(LynxModuleManagerTest, GetPlatformModule) {
  auto module_1 = module_manager_->bindingPtr->GetModule("platform_module");
  EXPECT_NE(module_1, nullptr);

  auto module_2 =
      use_native_module_manager_->bindingPtr->GetModule("platform_module");
  EXPECT_NE(module_2, nullptr);
}

TEST_F(LynxModuleManagerTest, GetRecordId) {
  EXPECT_EQ(module_manager_->record_id_, 12345);
  EXPECT_EQ(use_native_module_manager_->record_id_, 12345);
}

}  // namespace js

}  // namespace runtime
}  // namespace lynx
