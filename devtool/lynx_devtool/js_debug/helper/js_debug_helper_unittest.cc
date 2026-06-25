// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/js_debug/helper/js_debug_helper.h"

#include "core/runtime/mts_context.h"
#include "devtool/js_inspect/inspector_const.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

namespace {

class FakeLepusInspectorManager : public lepus::LepusInspectorManager {
 public:
  void InitInspector(
      runtime::MTSContext* context,
      const std::shared_ptr<lepus::InspectorLepusObserver>& observer,
      const std::string& context_name) override {}
  void SetDebugInfo(const std::string& debug_info_url,
                    const std::string& file_name) override {}
  void DestroyInspector() override {}
  std::shared_ptr<lepus::InspectorLepusObserver> UpdateInspector(
      const std::shared_ptr<lepus::InspectorLepusObserver>& observer) override {
    return observer;
  }
};

}  // namespace

class JSDebugHelperTest : public ::testing::Test {
 public:
  JSDebugHelperTest() = default;
  ~JSDebugHelperTest() override = default;

  void SetUp() override { ResetHelper(); }

  void TearDown() override { ResetHelper(); }

 protected:
  void ResetHelper() {
    helper_ = JSDebugHelper::GetInstance();
    helper_->SetV8Proxy(nullptr);
    helper_->SetQuickJSProxy(nullptr);
    helper_->SetLepusProxy(nullptr);
    helper_->SetRTSInspectorManagerCreator(nullptr);
  }

  JSDebugHelper* helper_{nullptr};
};

TEST_F(JSDebugHelperTest, IsDebugAvailable) {
  EXPECT_FALSE(helper_->IsJSDebugAvailable());
  EXPECT_FALSE(helper_->IsLepusDebugAvailable());
}

TEST_F(JSDebugHelperTest, CreateRuntimeInspectorManager) {
  EXPECT_EQ(helper_->CreateRuntimeInspectorManager(kKeyEngineV8), nullptr);
  EXPECT_EQ(helper_->CreateRuntimeInspectorManager(kKeyEngineQuickjs), nullptr);
}

TEST_F(JSDebugHelperTest, CreateLepusInspectorManager) {
  EXPECT_EQ(helper_->CreateLepusInspectorManager(
                runtime::ContextType::LepusNGContextType),
            nullptr);
}

TEST_F(JSDebugHelperTest, MakeRuntime) {
  EXPECT_EQ(helper_->MakeRuntime(kKeyEngineV8), nullptr);
  EXPECT_EQ(helper_->MakeRuntime(kKeyEngineQuickjs), nullptr);
}

TEST_F(JSDebugHelperTest, CreateRTSInspectorManagerAfterSetCreator) {
  helper_->SetRTSInspectorManagerCreator(
      []() -> std::unique_ptr<lepus::LepusInspectorManager> {
        return std::make_unique<FakeLepusInspectorManager>();
      });

  auto manager = helper_->CreateLepusInspectorManager(
      runtime::ContextType::RTSContextType);

  EXPECT_NE(manager, nullptr);
}

TEST_F(JSDebugHelperTest,
       CreateRTSInspectorManagerReturnsCreatorResultWhenAlreadyRegistered) {
  helper_->SetRTSInspectorManagerCreator(
      []() -> std::unique_ptr<lepus::LepusInspectorManager> {
        return std::make_unique<FakeLepusInspectorManager>();
      });

  auto manager = helper_->CreateLepusInspectorManager(
      runtime::ContextType::RTSContextType);

  EXPECT_NE(manager, nullptr);
}

TEST_F(JSDebugHelperTest,
       CreateRTSInspectorManagerReturnsNullWhenFactorySymbolMissing) {
  helper_->SetRTSInspectorManagerCreator(nullptr);

  auto manager = helper_->CreateLepusInspectorManager(
      runtime::ContextType::RTSContextType);

  EXPECT_EQ(manager, nullptr);
}

TEST_F(JSDebugHelperTest, ClearRTSInspectorManagerCreatorDisablesRTSInspector) {
  helper_->SetRTSInspectorManagerCreator(
      []() -> std::unique_ptr<lepus::LepusInspectorManager> {
        return std::make_unique<FakeLepusInspectorManager>();
      });
  EXPECT_NE(helper_->CreateLepusInspectorManager(
                runtime::ContextType::RTSContextType),
            nullptr);

  helper_->SetRTSInspectorManagerCreator(nullptr);

  EXPECT_EQ(helper_->CreateLepusInspectorManager(
                runtime::ContextType::RTSContextType),
            nullptr);
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
