// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define protected public
#define private public

#include "devtool/lynx_devtool/js_debug/inspector_client_delegate_impl.h"

#include "devtool/lynx_devtool/js_debug/java_script_debugger_ng.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

class JavaScriptDebuggerNGMock : public JavaScriptDebuggerNG {
 public:
  explicit JavaScriptDebuggerNGMock(
      const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
      : JavaScriptDebuggerNG(devtool_mediator) {}
  ~JavaScriptDebuggerNGMock() override = default;

  void DispatchMessage(const std::string& message,
                       const std::string& session_id = "") override {
    dispatch_message_buf_.push(message);
  }
  void RunOnTargetThread(base::closure&& closure,
                         bool run_now = true) override {}

  std::queue<std::string> dispatch_message_buf_;
};

class InspectorClientDelegateImplTest : public ::testing::Test {
 public:
  InspectorClientDelegateImplTest() {}
  ~InspectorClientDelegateImplTest() override {}
  void SetUp() override {
    v8_delegate_ = InspectorClientDelegateProvider::GetInstance()->GetDelegate(
        kKeyEngineV8);
    qjs_delegate_ = InspectorClientDelegateProvider::GetInstance()->GetDelegate(
        kKeyEngineQuickjs);
    lepus_delegate_ =
        InspectorClientDelegateProvider::GetInstance()->GetDelegate(
            kKeyEngineLepus);
  }

 private:
  std::shared_ptr<InspectorClientDelegateImpl> v8_delegate_;
  std::shared_ptr<InspectorClientDelegateImpl> qjs_delegate_;
  std::shared_ptr<InspectorClientDelegateImpl> lepus_delegate_;
};

TEST_F(InspectorClientDelegateImplTest, GetDelegate) {
  std::shared_ptr<InspectorClientDelegateImpl> new_v8_delegate =
      InspectorClientDelegateProvider::GetInstance()->GetDelegate(kKeyEngineV8);
  std::shared_ptr<InspectorClientDelegateImpl> new_qjs_delegate =
      InspectorClientDelegateProvider::GetInstance()->GetDelegate(
          kKeyEngineQuickjs);
  std::shared_ptr<InspectorClientDelegateImpl> new_lepus_delegate =
      InspectorClientDelegateProvider::GetInstance()->GetDelegate(
          kKeyEngineLepus);

  EXPECT_EQ(v8_delegate_, new_v8_delegate);
  EXPECT_EQ(qjs_delegate_, new_qjs_delegate);
  EXPECT_NE(lepus_delegate_, new_lepus_delegate);
}

TEST_F(InspectorClientDelegateImplTest,
       StaleRuntimeDestroyedDoesNotInvalidateReplacementSession) {
  constexpr int kViewId = 11;
  constexpr int64_t kOldRuntimeId = 201;
  constexpr int64_t kNewRuntimeId = 202;
  constexpr int kReplacementScriptId = 88;
  constexpr char kSharedGroupId[] = "shared-group";

  auto debugger = std::make_shared<JavaScriptDebuggerNGMock>(nullptr);
  v8_delegate_->InsertDebugger(debugger, false);
  v8_delegate_->OnInspectorInited(kViewId, kOldRuntimeId, kSharedGroupId);
  v8_delegate_->OnInspectorInited(kViewId, kNewRuntimeId, kSharedGroupId);
  v8_delegate_->InsertScriptId(kViewId, kReplacementScriptId);

  v8_delegate_->OnRuntimeDestroyed(kViewId, kOldRuntimeId);
  EXPECT_EQ(v8_delegate_->GetViewIdByRuntimeId(kOldRuntimeId), kErrorViewID);
  EXPECT_EQ(v8_delegate_->GetViewIdByRuntimeId(kNewRuntimeId), kViewId);
  EXPECT_FALSE(v8_delegate_->IsScriptIdInvalid(kReplacementScriptId));

  v8_delegate_->OnRuntimeDestroyed(kViewId, kNewRuntimeId);
  EXPECT_TRUE(v8_delegate_->IsScriptIdInvalid(kReplacementScriptId));
  v8_delegate_->RemoveDebugger(kViewId);
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
