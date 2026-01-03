// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define protected public
#define private public

#include "devtool/lynx_devtool/js_debug/lepus/inspector_lepus_debugger_impl.h"

#include <memory>

#include "core/renderer/utils/devtool_lifecycle.h"
#include "core/renderer/utils/lynx_env.h"
#include "devtool/testing/mock/devtool_platform_facade_mock.h"
#include "devtool/testing/mock/inspector_client_ng_mock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

class InspectorLepusDebuggerImplTest : public ::testing::Test {
 public:
  InspectorLepusDebuggerImplTest() {}
  ~InspectorLepusDebuggerImplTest() override {}
  void SetUp() override {
    debugger_ = std::make_shared<InspectorLepusDebuggerImpl>(nullptr);
  }

 private:
  std::shared_ptr<InspectorLepusDebuggerImpl> debugger_;
};

TEST_F(InspectorLepusDebuggerImplTest, GetInspectorLepusObserver) {
  auto observer1 = debugger_->GetInspectorLepusObserver();
  EXPECT_NE(observer1, nullptr);
  auto observer2 = debugger_->GetInspectorLepusObserver();
  EXPECT_EQ(observer1, observer2);
}

TEST_F(InspectorLepusDebuggerImplTest, NullDecodeDebugInfo) {
  std::string debug_info = "eJWDAAAAAAE=";
  std::string result;
  debugger_->DecodeDebugInfo(debug_info, result);
  EXPECT_EQ(result, "");
}

TEST_F(InspectorLepusDebuggerImplTest, SpecialCharDecodeDebugInfo) {
  std::string debug_info =
      "eJwFwcERgCAMBMBWrg7bsIIYg2QGAgPn+"
      "LUOP7ZoCe6uUnsx7LadBzxSG1XoLXA5M2Y3dSnQLEOUNuYC2iS0BS2I733uH2JDGms=";
  std::string result;
  debugger_->DecodeDebugInfo(debug_info, result);
  EXPECT_EQ(
      result,
      "Sample debug information with special characters: test content 🚀");
}

TEST_F(InspectorLepusDebuggerImplTest, GetDebugInfo) {
  std::string result;

  {
    std::shared_ptr<lynx::testing::DevToolPlatformFacadeMock> platform =
        std::make_shared<lynx::testing::DevToolPlatformFacadeMock>();
    debugger_->SetDevToolPlatformFacade(platform);
    result = debugger_->GetDebugInfo("test");
    EXPECT_EQ(result, "test GetLepusDebugInfo");
  }

  result = debugger_->GetDebugInfo("test");
  EXPECT_EQ(result, "");
}

TEST_F(InspectorLepusDebuggerImplTest, SetDebugInfoUrl) {
  debugger_->SetDebugInfoUrl("test-url", "test-name");
  EXPECT_EQ(debugger_->GetDebugInfoUrl("test-name"), "test-url");
}

TEST_F(InspectorLepusDebuggerImplTest, OnInspectorInited) {
  std::shared_ptr<lynx::testing::InspectorClientNGMock> client1 =
      std::make_shared<lynx::testing::InspectorClientNGMock>();
  debugger_->OnInspectorInited(kKeyEngineLepus, kKeyEngineLepus, client1);
  auto it = debugger_->delegates_.find(kKeyEngineLepus);
  EXPECT_NE(it, debugger_->delegates_.end());

  auto delegate = it->second;
  EXPECT_NE(delegate->view_id_to_bundle_.find(kDefaultViewID),
            delegate->view_id_to_bundle_.end());
  EXPECT_EQ(delegate->target_id_, kKeyEngineLepus);
  EXPECT_EQ(delegate->client_wp_.lock(), client1);
  EXPECT_EQ(client1->delegate_wp_.lock(), delegate);

  EXPECT_EQ(delegate->target_created_, false);
  EXPECT_EQ(client1->message_queue_.size(), 0);

  tasm::DevToolLifecycle::GetInstance().OnAttached();
  tasm::DevToolLifecycle::GetInstance().OnEnabled();
  tasm::DevToolLifecycle::GetInstance().OnInitialized();
  tasm::DevToolLifecycle::GetInstance().OnConnected();
  std::shared_ptr<lynx::testing::InspectorClientNGMock> client2 =
      std::make_shared<lynx::testing::InspectorClientNGMock>();
  debugger_->OnInspectorInited(kKeyEngineLepus, kKeyEngineLepus, client2);
  EXPECT_EQ(debugger_->delegates_.size(), 1);
  EXPECT_EQ(delegate->client_wp_.lock(), client2);
  EXPECT_EQ(client2->delegate_wp_.lock(), delegate);

  EXPECT_EQ(delegate->target_created_, true);
  EXPECT_EQ(client2->message_queue_.size(), 2);
  EXPECT_EQ(client2->message_queue_.front(),
            "{\"id\":0,\"method\":\"Debugger.enable\"}");
  EXPECT_EQ(client2->message_queue_.back(),
            "{\"id\":0,\"method\":\"Profiler.enable\"}");

  std::shared_ptr<lynx::testing::InspectorClientNGMock> client3 =
      std::make_shared<lynx::testing::InspectorClientNGMock>();
  debugger_->OnInspectorInited(kKeyEngineLepus, "Lepus:test", client3);
  EXPECT_EQ(debugger_->delegates_.size(), 2);
  EXPECT_NE(debugger_->delegates_.find("Lepus:test"),
            debugger_->delegates_.end());
}

TEST_F(InspectorLepusDebuggerImplTest, OnContextDestroyed) {
  std::shared_ptr<lynx::testing::InspectorClientNGMock> client =
      std::make_shared<lynx::testing::InspectorClientNGMock>();
  debugger_->OnInspectorInited(kKeyEngineLepus, kKeyEngineLepus, client);
  debugger_->OnContextDestroyed(kKeyEngineLepus);
  debugger_->delegates_.begin()->second->target_created_ = false;

  debugger_->OnContextDestroyed("test");
}

TEST_F(InspectorLepusDebuggerImplTest, PrepareForScriptEval) {
  std::shared_ptr<lynx::testing::InspectorClientNGMock> client =
      std::make_shared<lynx::testing::InspectorClientNGMock>();
  debugger_->OnInspectorInited(kKeyEngineLepus, kKeyEngineLepus, client);
  debugger_->PrepareForScriptEval("test");
  EXPECT_EQ(client->stop_at_entry_, false);
  debugger_->PrepareForScriptEval(kKeyEngineLepus);
  EXPECT_EQ(client->stop_at_entry_, false);

  tasm::DevToolLifecycle::GetInstance().OnAttached();
  tasm::DevToolLifecycle::GetInstance().OnEnabled();
  tasm::DevToolLifecycle::GetInstance().OnInitialized();
  tasm::DevToolLifecycle::GetInstance().OnConnected();
  debugger_->PrepareForScriptEval("test");
  EXPECT_EQ(client->stop_at_entry_, false);
  debugger_->PrepareForScriptEval(kKeyEngineLepus);
  EXPECT_EQ(client->stop_at_entry_, false);

  DevToolConfig::SetStopAtEntry(true, true);
  debugger_->PrepareForScriptEval("test");
  EXPECT_EQ(client->stop_at_entry_, false);
  debugger_->PrepareForScriptEval(kKeyEngineLepus);
  EXPECT_EQ(client->stop_at_entry_, true);

  // reset for next test
  DevToolConfig::SetStopAtEntry(false, true);
  DevToolConfig::SetFetchDebugInfo(false, true);
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
