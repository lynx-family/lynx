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
  const base::LogContext& GetLogContextForTest() const {
    return GetLogContext();
  }

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
    module_manager_->SetLogContext(log_context_);
    module_manager_->initBindingPtr(module_manager_, nullptr);

    auto platform_module_factory = std::make_unique<NativeModuleFactory>();
    platform_module_factory->Register("platform_module", []() {
      return std::make_shared<MockNativeModule>("platform_module");
    });
    module_manager_->SetPlatformModuleFactory(
        std::move(platform_module_factory));

    auto native_module_factory = std::make_unique<NativeModuleFactory>();
    native_module_factory->Register("native_module", [this]() {
      native_module_ = std::make_shared<MockNativeModule>("native_module");
      return native_module_;
    });
    module_manager_->SetModuleFactory(std::move(native_module_factory));
    module_manager_->SetRecordID(12345);

    auto native_module_manager =
        std::make_shared<pub::LynxNativeModuleManager>();
    use_native_module_manager_ =
        std::make_shared<LynxModuleManager>(std::move(*native_module_manager));
    use_native_module_manager_->SetLogContext(log_context_);
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

  base::LogContext log_context_{1, 2, 3};
  std::shared_ptr<MockNativeModule> native_module_;
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
  EXPECT_NE(&module_1->GetLogContext(), &log_context_);
  EXPECT_EQ(module_1->GetLogContext().runtime_id, log_context_.runtime_id);
  ASSERT_NE(native_module_, nullptr);
  EXPECT_EQ(native_module_->GetLogContextForTest().runtime_id,
            log_context_.runtime_id);
  auto module_2 =
      use_native_module_manager_->bindingPtr->GetModule("native_module");
  EXPECT_NE(module_2, nullptr);
  EXPECT_NE(&module_2->GetLogContext(), &log_context_);
  EXPECT_EQ(module_2->GetLogContext().runtime_id, log_context_.runtime_id);
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

TEST_F(LynxModuleManagerTest, CopiesAndPropagatesLogContext) {
  auto module = module_manager_->bindingPtr->GetModule("native_module");
  ASSERT_NE(module, nullptr);
  ASSERT_NE(native_module_, nullptr);

  EXPECT_NE(&module_manager_->GetLogContext(), &log_context_);
  log_context_ = {4, 5, 6};
  EXPECT_EQ(module_manager_->GetLogContext().view_id, 1);
  EXPECT_EQ(module_manager_->GetLogContext().engine_id, 2);
  EXPECT_EQ(module_manager_->GetLogContext().runtime_id, 3);

  module_manager_->SetLogContext(log_context_);
  EXPECT_EQ(module_manager_->GetLogContext().view_id, 4);
  EXPECT_EQ(module_manager_->GetLogContext().engine_id, 5);
  EXPECT_EQ(module_manager_->GetLogContext().runtime_id, 6);
  EXPECT_EQ(module->GetLogContext().view_id, 4);
  EXPECT_EQ(module->GetLogContext().engine_id, 5);
  EXPECT_EQ(module->GetLogContext().runtime_id, 6);
  EXPECT_EQ(native_module_->GetLogContextForTest().view_id, 4);
  EXPECT_EQ(native_module_->GetLogContextForTest().engine_id, 5);
  EXPECT_EQ(native_module_->GetLogContextForTest().runtime_id, 6);
}

TEST_F(LynxModuleManagerTest, LateModuleUsesCurrentLogContext) {
  const base::LogContext updated_context{4, 5, 6};
  module_manager_->SetLogContext(updated_context);

  auto module = module_manager_->bindingPtr->GetModule("platform_module");
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(module->GetLogContext().view_id, updated_context.view_id);
  EXPECT_EQ(module->GetLogContext().engine_id, updated_context.engine_id);
  EXPECT_EQ(module->GetLogContext().runtime_id, updated_context.runtime_id);
}

TEST_F(LynxModuleManagerTest, RetainsSnapshotAfterSourceIsDestroyed) {
  std::shared_ptr<LynxModule> module;
  {
    const base::LogContext transient_context{7, 8, 9};
    module_manager_->SetLogContext(transient_context);
    module = module_manager_->bindingPtr->GetModule("native_module");
  }

  ASSERT_NE(module, nullptr);
  ASSERT_NE(native_module_, nullptr);
  EXPECT_EQ(module_manager_->GetLogContext().view_id, 7);
  EXPECT_EQ(module_manager_->GetLogContext().engine_id, 8);
  EXPECT_EQ(module_manager_->GetLogContext().runtime_id, 9);
  EXPECT_EQ(module->GetLogContext().runtime_id, 9);
  EXPECT_EQ(native_module_->GetLogContextForTest().runtime_id, 9);
}

}  // namespace js

}  // namespace runtime
}  // namespace lynx
